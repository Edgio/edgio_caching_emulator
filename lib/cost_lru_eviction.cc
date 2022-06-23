// Copyright 2021 Edgio Inc
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/*
 *  Created on: Jun 13, 2013
 *      Corrected by: Harkeerat Bedi
 */

#include <assert.h>
#include <map>
#include <algorithm>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <math.h>
#include <algorithm>
#include <sstream>
#include <fstream>

#include "em_structs.h"
#include "cache_policy.h"
#include "cost_lru_eviction.h"

using namespace std;

CostLRUEviction::CostLRUEviction(unsigned long long size, string id, const
                                 EmConfItems * sci, double w_age,
                                 double w_size, int lru_interval,
                                 int eviction_formula, int ef4_y, float ef4_e){

    name = "cost_lru";

    this->sci = sci;

    current_size = 0;
    total_capacity = size;
    cache_id = id;
    head = new CostLRUEvictionEntry;
    tail = new CostLRUEvictionEntry;
    head->prev = NULL;
    head->next = tail;
    tail->next = NULL;
    tail->prev = head;

    previous_hour_timestamp = 0;
    total_items_purged = 0;

    total_junk_items_purged = 0;
    total_junk_purge_operations = 0;

    total_hourly_purge_intervals = 0;
    cache_filled_once = false;
    regular_purge_interval = sci->regular_purge_interval;

    previous_hour_timestamp_put = 0;

    previous_hour_timestamp_ingress = 0;
    ingress_total_size = 0;
    ingress_total_count = 0;

    previous_hour_timestamp_egress = 0;
    egress_total_size = 0;
    egress_total_count = 0;

    current_ingress_item_timestamp = 0;

    purge_size_based_limit = sci->LRU_list_size;

    cache_item_count = 0;
    max_cache_item_count = 0;
    number_of_bins_for_histogram = 1000;
    lru_algo3_reg_purge = 6;

    m_w_origin_cost = 0;

    this->w_size = w_size;
    this->w_age = w_age;
    this->lru_interval = lru_interval;
    this->eviction_formula = eviction_formula;
    this->ef4_y = ef4_y;
    this->ef4_e = ef4_e;


    w_item_attr_val = 0;

    running_size_mu = 0;
    running_size_var = 0;
    alpha_running_size_mu = 0.25;
    alpha_running_size_var = 0.25;

    hour_count = 0;

}

CostLRUEviction::~CostLRUEviction()
{
    delete head;
    delete tail;
}


/*
 * to print the age of all files
 * in cache using a histogram (based on location)
 */
void CostLRUEviction::print_cache_file_age_histogram() {
    if (cache_item_count < number_of_bins_for_histogram) {
        return;
    }
    cout << "\nprint_cache_file_age_histogram ";
    //int newest_file_ts = (int) head->next->timestamp;
    //int oldest_file_ts = (int) tail->prev->timestamp;
    //float seconds_per_bin = ((float) newest_file_ts - oldest_file_ts) / bins;
    float items_per_bin = (float) cache_item_count / number_of_bins_for_histogram;

    int item_count = 0; float sum_of_ts = 0;
    CostLRUEvictionEntry *currentNode = head;
    for (currentNode = currentNode->next; currentNode != tail; currentNode = currentNode->next) {
        if (item_count < items_per_bin) {
            sum_of_ts += (int) (currentNode->timestamp - tail->prev->timestamp);
            item_count++;
        }
        else {
            cout << round((float) sum_of_ts / item_count) << " ";
            item_count = 0;
            sum_of_ts = 0;
        }
    }
    cout << endl;
}

/*
 * to print the hit count of all files
 */
void CostLRUEviction::print_cache_file_hits() {
    cout << "\nprint_cache_file_hits ";
    CostLRUEvictionEntry *currentNode = head;
    for (currentNode = currentNode->next; currentNode != tail; currentNode = currentNode->next) {
        cout << currentNode->count << " ";
    }
    cout << endl;
}

/*
 * We purge only once an hour (currently in use)
 *
 */
void CostLRUEviction::hourly_purging(unsigned long timestamp) {
    total_hourly_purge_intervals++;

    decide_items_based_on_score();
}

// for debugging, also keeps track of count.
unsigned long long CostLRUEviction::initial_put_count(string key, unsigned long data, unsigned long timestamp, unsigned long bytes_out, string customer_id, string orig_url)
{
    CostLRUEvictionEntry* node = _mapping[key];
    //	cerr << node << " " << key << "\t" << data << "\t" << url_original << endl;
    if(node)
    {
        assert(0); // we should not reach here, because the cache dump should have unique entries.
    }
    else{
        node = new CostLRUEvictionEntry;
        node->key = key;
        node->data = data;
        node->timestamp = timestamp;
        node->customer_id = customer_id;
        node->count = 1;
        node->orig_url = orig_url;
        _mapping[key] = node;
        attach(node);

        ++cache_item_count; // used for size based LRU only
        if (max_cache_item_count < cache_item_count) {
            max_cache_item_count = cache_item_count;
        }
    }
    return current_size;
}


// to pre-populate the cache (from cache dump)
unsigned long long CostLRUEviction::initial_put(string key, unsigned long data, unsigned long timestamp, unsigned long bytes_out, string customer_id, string orig_url, string access_log_entry_string)
{
    CostLRUEvictionEntry* node = _mapping[key];
    if(node)
    {
        // cerr << "\nduplicate_entry_in_initial_cache_dump " << key;
        node->count = node->count + 1;
        // assert(0); // we should not reach here, because the cache dump should have unique entries.
    }
    else{
        node = new CostLRUEvictionEntry;
        node->key = key;
        node->data = data;
        node->timestamp = timestamp;
        node->customer_id = customer_id;
        node->count = 1;
        node->orig_url = orig_url;
        node->access_log_entry_string = access_log_entry_string;
        _mapping[key] = node;
        attach(node);

        ++cache_item_count; // used for size based LRU only
        if (max_cache_item_count < cache_item_count) {
            max_cache_item_count = cache_item_count;
        }
    }
    //update_cost_based_score(node);
    update_size_running_mean(node);
    return current_size;
}

void CostLRUEviction::print_oldest_file_age(unsigned long timestamp, ostream &output) {
    if (cache_item_count == 0) {
        return;
    }
    output << "\noverall_oldest_file_age_days "
        << ((float) timestamp - tail->prev->timestamp)/60/60/24 << "\n";
}

void CostLRUEviction::print_oldest_file_age_for_monitored_customers(unsigned long currentTimeStamp) {
    if(sci->monitor_customers_list.size() < 1) {
        return;
    }
    cout << endl;
    for(vector<string>::const_iterator i = sci->monitor_customers_list.begin(); i != sci->monitor_customers_list.end(); ++i) {
        CostLRUEvictionEntry* currentNode = tail;
        bool item_found = false;
        for (currentNode = currentNode->prev; currentNode != head; currentNode = currentNode->prev) {
            if(currentNode->customer_id.compare(*i) == 0) {
                item_found = true;
                cout << "print_oldest_file_age_days "
                    << *i << " "
                    << (float) (currentTimeStamp - currentNode->timestamp)/60/60/24 // days
                    << "\n";
                break;
            }
        }
        if(!item_found) {
            cout << "print_oldest_file_age_days "
                << *i << " "
                << 0
                << "\n";
        }
    }
    cout << endl;
}

unsigned long long CostLRUEviction::put(string key, unsigned long data, unsigned long timestamp, unsigned long bytes_out, string customer_id, string orig_url)
{
    CostLRUEvictionEntry* node = _mapping[key];
    if(node)
    {
        assert(0); // we should not reach here, because we always 'check' before we 'put'.
    }
    else
    {
        current_ingress_item_timestamp = timestamp;
        if(sci->print_hdd_ingress_stats) {
            ingress_total_count++;
            ingress_total_size += data;

            if(timestamp - previous_hour_timestamp_ingress > 3600) {
                previous_hour_timestamp_ingress = timestamp;

                cout << "\nprint_hourly_hdd_ingress_stats "
                    << timestamp << " "
                    << ingress_total_count << " "
                    << ingress_total_size << " "
                    << "\n";
                ingress_total_count = 0;
                ingress_total_size = 0;
            }
        }

        node = new CostLRUEvictionEntry;
        node->key = key;
        node->data = data;
        node->timestamp = timestamp;
        node->customer_id = customer_id;
        node->orig_url = orig_url;
        node->count = 1;
        _mapping[key] = node;
        attach(node);

        // Don't know why this is commented out, but is in original code
        //update_cost_based_score(node);
        update_size_running_mean(node);

        ++cache_item_count; // used for size based LRU only

        if (max_cache_item_count < cache_item_count) {
            max_cache_item_count = cache_item_count;
        }

        // Bring it back under size if needed
        if(current_size > total_capacity) {
            decide_items_based_on_score();
        }

    }
    return current_size;
}

void CostLRUEviction::print_avg_oldest_requested_file(unsigned long timestamp) {
    if (avg_oldest_requested_file_vector.size() < 1) {
        return;
    }
    sort (avg_oldest_requested_file_vector.begin(), avg_oldest_requested_file_vector.end());

    unsigned long item_loc = round((float) avg_oldest_requested_file_vector.size() * 0.05);
    unsigned int avg_oldest = avg_oldest_requested_file_vector.at(item_loc);
    cout << "\ncurrent_ts_minus_air_t " << ((float) timestamp - avg_oldest)/60/60/24;
    cout << "\nair_t_minus_oldest_ts " << ((float) avg_oldest - tail->prev->timestamp)/60/60/24;
    cout << "\noldest_minus_avg_oldest_in_days_95_prc "
        << (((float) timestamp - tail->prev->timestamp)/60/60/24)
        - (((float) timestamp - avg_oldest)/60/60/24);

    /*if ((((float) timestamp - tail->prev->timestamp)/60/60/24)
      - (((float) timestamp - avg_oldest)/60/60/24) < 0) {
      cout << "\n" << timestamp << " " << tail->prev->timestamp << " " << avg_oldest << "\n\n";
      for( std::vector<unsigned int>::const_iterator i = avg_oldest_requested_file_vector.begin(); i != avg_oldest_requested_file_vector.end(); ++i)
      std::cout << *i << ' ';
      cout << "\n";
      exit(1);
      }*/

    item_loc = round((float) avg_oldest_requested_file_vector.size() * 0.01);
    avg_oldest = avg_oldest_requested_file_vector.at(item_loc);
    cout << "\noldest_minus_avg_oldest_in_days_99_prc "
        << (((float) timestamp - tail->prev->timestamp)/60/60/24)
        - (((float) timestamp - avg_oldest)/60/60/24);

    item_loc = round((float) avg_oldest_requested_file_vector.size() * 0.95);
    avg_oldest = avg_oldest_requested_file_vector.at(item_loc);
    cout << "\noldest_minus_avg_oldest_in_days_05_prc "
        << (((float) timestamp - tail->prev->timestamp)/60/60/24)
        - (((float) timestamp - avg_oldest)/60/60/24);

    avg_oldest_requested_file_vector.clear();
}

unsigned long CostLRUEviction::get(string key, unsigned long ts, unsigned long bytes_out, string url_original)
{
    CostLRUEvictionEntry* node = _mapping[key];
    if(node)
    {
        detach(node);
        attach(node);
        node->count = node->count + 1;

        avg_oldest_requested_file_vector.push_back(node->timestamp);

        node->timestamp = ts;

        //update_cost_based_score(node);
        update_size_running_mean(node);

        return node->data;
    }
    else
    {
        assert(0); // we should not reach here, because we always 'check' before we 'get'.
        return 0;
    }
}

int CostLRUEviction::check_and_print(string key)	// to check if present.
{
    CostLRUEvictionEntry* node = _mapping[key];
    if(node) {
        /*	cout
            << node->timestamp << "\t"
            << node->key << "\t"
            << node->customer_id << "\t"
            << node->data << "\t"
            << node->orig_url << "\t"
            << node->count << "\t"
            << node->access_log_entry_string << "\n"; */

        return 1;
    }
    else
        return 0;
}

int CostLRUEviction::check(string key, unsigned long ts)	// to check if present.
{
    CostLRUEvictionEntry* node = _mapping[key];
    if(node)
        return 1;
    else
        return 0;
}

unsigned long CostLRUEviction::manual_delete(string key) {
    CostLRUEvictionEntry* node = _mapping[key];
    unsigned long data;

    if(node == head) {
        assert(0); // linked list empty (manual_delete)
    }

    if(node) {
        data = node->data;
        detach(node);
        _mapping.erase(node->key);
        delete node;
    }
    else
        assert(0); // we should not reach here, because we always 'check' before we 'get'.

    return data;
}

/*
 * default purge: we delete the least recently requested file
 */
bool CostLRUEviction::purge_regular() {
    CostLRUEvictionEntry* node = tail->prev;	//switch 1 of 2; for LRU vs. MRU; LRU(tail->prev); 		MRU(head->next)
    if(node == head) {							//switch 2 of 2; for LRU vs. MRU; LRU(node == head); 	MRU(node == tail)
        return false;
    }

    if(sci->print_hdd_egress_stats) {
        egress_total_count++;
        egress_total_size += node->data;

        if(current_ingress_item_timestamp - previous_hour_timestamp_egress > 3600) {
            previous_hour_timestamp_egress = current_ingress_item_timestamp;

            cout << "\nprint_hourly_hdd_egress_stats "
                << current_ingress_item_timestamp << " "
                << egress_total_count << " "
                << egress_total_size << " "
                << "\n";
            egress_total_count = 0;
            egress_total_size = 0;
        }
    }

    // CostLRUEvictionEntry* headNode = head->next;
    // cout << "\npurge_regular " << headNode->timestamp << " " << node->timestamp << " " << node->data << " " << node->key << "\n";

    detach(node);
    _mapping.erase(node->key);
    delete node;

    --cache_item_count;
    return true;
}



void CostLRUEviction::dump_cache_contents(string filename) {
    ofstream myfile;
    myfile.open (filename.c_str());
    CostLRUEvictionEntry *currentNode = head;
    for (currentNode = currentNode->next; currentNode != tail; currentNode = currentNode->next) {
        myfile
            << currentNode->timestamp << "\t"
            << currentNode->key << "\t"
            << currentNode->customer_id << "\t"
            << currentNode->data << "\t"
            << currentNode->orig_url << "\t"
            << currentNode->count << "\t"
            << currentNode->access_log_entry_string << "\n";
    }
    myfile.close();
}

void CostLRUEviction::dump_cache_contents_cout() {
    cout << "dump_cache_contents_cout(): " << endl;
    CostLRUEvictionEntry *currentNode = head;
    for (currentNode = currentNode->next; currentNode != tail; currentNode = currentNode->next) {
        cout << cache_id
            << " customer_id " << currentNode->customer_id
            << " timestamp " << currentNode->timestamp
            << " count " << currentNode->count << "\t"
            << "data " << currentNode->data << "\t"
            << "orig_url " << currentNode->orig_url << "\t"
            << "key " << currentNode->key << "\n";
    }
}

unsigned long long CostLRUEviction::get_size() {
    return current_size;
}

unsigned long long CostLRUEviction::get_total_capacity() {
    return total_capacity;
}

void CostLRUEviction::print_max_cache_item_count() {
    cout << "cache_id " << cache_id << " max_cache_item_count " << max_cache_item_count << endl;
    /*cout << "total_hourly_intervals " << total_hourly_intervals
      << " total_items_purged " << total_items_purged
      << " average " << total_items_purged / total_hourly_intervals
      << " total_junk_items_purged " << total_junk_items_purged
      << " total_junk_purge_operations " << total_junk_purge_operations
      << endl;*/
    dump_cache_contents_cout();
}

void CostLRUEviction::set_total_capacity_by_value(unsigned long long size) {
    total_capacity = size;
}

void CostLRUEviction::decide_items_based_on_score() {
    compute_scores();
    unsigned long long _total_items_purged = 0;
    std::vector<std::pair<double, string> > score_vector; // pair = score, hash
    for (std::unordered_map<string, s_item_attr>::iterator it = m_item_unordered_map.begin(); it != m_item_unordered_map.end(); ++it ) {
        score_vector.push_back(std::make_pair(it->second.score, it->first));
    }
    std::sort(score_vector.begin(), score_vector.end()); // ascending order

    for(std::vector<std::pair<double, string> >::reverse_iterator it = score_vector.rbegin(); it != score_vector.rend(); ++it) {
        if(current_size > total_capacity) {
            CostLRUEvictionEntry* node = _mapping[it->second];
            if (sci->debug) {
                cout << "\ndecide_items_based_on_score "
                    << head->next->timestamp << " "
                    << m_item_unordered_map[it->second].score << " "
                    << node->timestamp << " " << node->data << " " << it->second << "\n";
            }
            detach(node);
            _mapping.erase(node->key);
            delete node;
            --cache_item_count;
            _total_items_purged++;

            m_item_unordered_map.erase(it->second);
        }
    }
    //cout << endl << "total_items_purged " << _total_items_purged << endl;
}




void CostLRUEviction::update_size_running_mean(CostLRUEvictionEntry* node) {
    unsigned long size = node->data;
    if (0 == size) { size = 1; }
    running_size_mu = (alpha_running_size_mu * log2(size)) + ((1 - alpha_running_size_mu) * running_size_mu);
    double current_var = pow(log2(size) - running_size_mu, 2);
    running_size_var = (alpha_running_size_var * current_var) + ((1 - alpha_running_size_var) * running_size_var);
}

void CostLRUEviction::compute_scores() {
    int size_formula = 1; // 1 is linear; 2 is cubic root (refer slides)
    int age_formula = 1;  // 1 is linear; 2 is cubic
    int deviations = 4;

    // here we compute the scores right before we begin the hourly eviction cycle
    double upper_range = running_size_mu + deviations * sqrt(running_size_var);
    double lower_range = running_size_mu - deviations * sqrt(running_size_var);

    CostLRUEvictionEntry *currentNode = tail;
    for (currentNode = currentNode->prev; currentNode != head; currentNode = currentNode->prev) {
        double size_score = 0;
        if (log2(currentNode->data) >= upper_range) {
            size_score = 1;
        } else if (log2(currentNode->data) <= lower_range) {
            size_score = 0;
        } else {
            if (1 == size_formula) {
                size_score = 0.5 + ((log2(currentNode->data) - running_size_mu) / (2 * deviations * sqrt(running_size_var)));
            }
            else if (2 == size_formula) {
                double value = (log2(currentNode->data) - running_size_mu) / (2 * deviations * sqrt(running_size_var));
                size_score = (cbrt(value) + cbrt(0.5)) * 1/6.0;
            }
        }

        if (size_score > 1 || size_score < 0) {
            cerr << "\nsize_scoreError "
                << running_size_mu << " "
                << running_size_var << " "
                << size_score << " "
                << log2(currentNode->data) << " ua "
                << upper_range << " "
                << 6*sqrt(running_size_var) << " "
                << currentNode->data << " "
                << currentNode->orig_url << " "
                << currentNode->key
                ;
            exit(1);
        }

        double age_score = 0;
        if (1 == age_formula) {
            age_score = (double) (head->next->timestamp - currentNode->timestamp) / (head->next->timestamp - tail->prev->timestamp);
        }
        else if (2 == age_formula) {
            age_score = (double) (head->next->timestamp - currentNode->timestamp) / (head->next->timestamp - tail->prev->timestamp);
            age_score = pow(age_score, 3);
        }
        else {
            cout << "\nUndefined value of age_formula. Exiting.\n";
            exit(1);
        }

        if (age_score < 0 || age_score > 1) {
            cout << "\nage_score value error.\n";
            exit(1);
        }

        double eviction_score = 0;
        if (1 == eviction_formula) {
            eviction_score = (age_score * w_age) + (size_score * w_size); // higher score = evicted sooner
        }

        else if (2 == eviction_formula) {
            double saiflish_perf = 0.5; // default is 0.5, else 1.0.
            bool _b = sci->check_customer_in_list(currentNode->customer_id, sci->no_bf_cust);
            if(_b) {
                // this content was added on 1st hit
                // so we will push these guys further towards the end
                saiflish_perf = 1;
            }
            eviction_score = ((age_score * w_age) + (size_score * w_size)) * saiflish_perf; // higher score = evicted sooner
        }

        else if (3 == eviction_formula) {
            eviction_score = (head->next->timestamp - currentNode->timestamp) * (size_score * w_size); // higher score = evicted sooner
        }

        else if (4 == eviction_formula) {
            // A_i^y * (S_i * w + E)
            eviction_score = pow((head->next->timestamp - currentNode->timestamp), ef4_y)
                * ((size_score * w_size) + ef4_e); // higher score = evicted sooner
        }
        else if (5 == eviction_formula) {
            // A_i^y * (S_i * w + A_i)
            eviction_score = pow((head->next->timestamp - currentNode->timestamp), ef4_y)
                * ((size_score * w_size) + (head->next->timestamp - currentNode->timestamp)); // higher score = evicted sooner
        }

        else if (6 == eviction_formula) {
            // A_i^y + (S_i * w * A_i)
            eviction_score = pow((head->next->timestamp - currentNode->timestamp), ef4_y)
                + ((size_score * w_size) * (head->next->timestamp - currentNode->timestamp)); // higher score = evicted sooner
        }

        else if (7 == eviction_formula) {
            // A_i^y  * (w_s * A_0 * S + e)
            eviction_score = pow((head->next->timestamp - currentNode->timestamp), ef4_y)
                * ((size_score * w_size * (head->next->timestamp - tail->prev->timestamp)) + ef4_e); // higher score = evicted sooner
        }

        else if (8 == eviction_formula) {
            // same as eviction_formula 1, but every lru_interval hours we do a regular LRU
            if (0 == hour_count % lru_interval) {
                eviction_score = age_score;
                //cout << ".";
            }
            else {
                eviction_score = (age_score * w_age) + (size_score * w_size); // higher score = evicted sooner
               // cout << "*";
            }
        }

        else {
            cout << "\nUndefined value of eviction_formula. Exiting.\n";
            exit(1);
        }
        if (sci->debug) {
            cout << "hour_count " << hour_count << " " << head->next->timestamp
                << " d_t " << (head->next->timestamp - currentNode->timestamp)
                << " " << currentNode->key << " s " << currentNode->data
                << " as " << age_score << " ss " << size_score << " es " << eviction_score << "\n";
        }
        m_item_unordered_map[currentNode->key].score = eviction_score;
    }
}

void CostLRUEviction::update_cost_based_score(CostLRUEvictionEntry* node) {
    // here we update the score of item upon each request. we use score2.
    // older technique, not in use anymore. we moved to a different way of doing it.
    // we noticed that even with a size weight of 1. we were not aggressive enough.
    // and here time was not normalized between 0 and 1.

    // compute score of size factor
    // cout << node->orig_url << "\n";
    running_size_mu = (alpha_running_size_mu * node->data)
        + ((1.0 - alpha_running_size_mu) * running_size_mu);
    double current_var = pow(node->data - running_size_mu, 2);
    running_size_var = (alpha_running_size_var * current_var)
        + ((1 - alpha_running_size_var) * running_size_var);
    double upper_range = running_size_mu + 3 * sqrt(running_size_var);
    double lower_range = running_size_mu - 3 * sqrt(running_size_var);
    if (upper_range < 1) upper_range = 1;
    if (lower_range < 1) lower_range = 1;
    double size_score = log2(node->data) / (log2(upper_range) - log2(lower_range));

    // todo - add other factors, origin cost, content-type, etc.
    double oldest_file_age = ((float) node->timestamp - tail->prev->timestamp) / 60 / 60 / 24;
    double oldest_file_age_secs = (float) node->timestamp - tail->prev->timestamp;
    double overall_size_score = size_score * w_size * oldest_file_age;
    double score = (float) node->timestamp * (1 - overall_size_score);
    double score2 = (float) node->timestamp - (size_score * w_size * ((float) node->timestamp - tail->prev->timestamp));

    if (size_score < 0 || size_score > 1) {
        cout << "\nsize_score < 0 \n";
        cout << "\nsqrt(running_size_var) upper_range lower_range size_score " << sqrt(running_size_var) << " " << upper_range << " " << lower_range << " " << size_score << "\n";
        cout << "\nrunning_size_mu current_var running_size_var size_score w_size oldest_file_age overall_size_score node->timestamp node->data score score2 oldest_file_age_secs node->url"
            << "\ntest1vals "
            << running_size_mu << " "
            << current_var << " "
            << running_size_var << " "
            << size_score << " "
            << w_size << " "
            << oldest_file_age << " "
            << overall_size_score << " "
            << node->timestamp << " "
            << node->data << " "
            << score << " "
            << score2 << " "
            << oldest_file_age_secs << " "
            << node->orig_url << " "
            << "\n";

        exit(1);
    }

    // if (size_score > 300) exit(1);

    m_item_unordered_map[node->key].score = score2;
}

void CostLRUEviction::detach(CostLRUEvictionEntry* node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    current_size = current_size - node->data;
}

void CostLRUEviction::attach(CostLRUEvictionEntry* node)
{
    node->next = head->next;
    node->prev = head;
    head->next = node;
    node->next->prev = node;
    current_size = current_size + node->data;
}

void CostLRUEviction::purge_size_based_multimap() {
    if (cache_item_count < purge_size_based_limit) {
        cerr << "cache_item_count " << cache_item_count
            << ", is less than purge_size_based_limit " << purge_size_based_limit << endl;
        cerr << "Setting purge_size_based_limit to half of cache_item_count." << endl;
        purge_size_based_limit = cache_item_count / 2;
    }

    unsigned long total_items_purged = 0;
    std::multimap<unsigned long, string> size_based_purge_list;

    CostLRUEvictionEntry* node = tail->prev;
    size_based_purge_list.insert(std::pair<unsigned long, string>(node->data, node->key));

    // populate the initial list
    for(unsigned int i = 1; i < purge_size_based_limit; i++) {
        node = node->prev;
        //if (customer_hit_stats[node->customer_id]["skip_size_based_deletion"] != 1) {
        // NOTE: For now, take it no matter what, don't worry about customer
        size_based_purge_list.insert(std::pair<unsigned long, string>(node->data, node->key));
        //    //	cout << "\nentry inserted " << node->key << " " << node->data << "\n";
        //}
    }

    //cout << endl << "current_size  " << current_size << " " << total_capacity << endl;
    //cout << "size_based_purge_list.size() " << size_based_purge_list.size() << endl;

    while (current_size > total_capacity) {
        std::multimap<unsigned long, string>::iterator it = (size_based_purge_list.end()); it--;
        //cout << "\nitem_to_delete " << it->first << " " << it->second << endl;
        CostLRUEvictionEntry* node_to_delete = _mapping[it->second];

        detach(node_to_delete);
        _mapping.erase(node_to_delete->key);
        delete node_to_delete;
        size_based_purge_list.erase(it);
        --cache_item_count;
        total_items_purged++;

        node = node->prev;
        if (customer_hit_stats[node->customer_id]["skip_size_based_deletion"] != 1) {
            size_based_purge_list.insert(std::pair<unsigned long, string>(node->data, node->key));
        }

        if (size_based_purge_list.size() <= 1) return;
    }
    //cout << "total_items_purged by purge_size_based_multimap " << total_items_purged << endl;
}

string CostLRUEviction::return_customer_id(string url) {
    std::vector<std::string> v; // http://rosettacode.org/wiki/Tokenize_a_string#C.2B.2B
    std::istringstream buf(url);
    for(std::string token; getline(buf, token, '/'); )
        v.push_back(token);

    // v[3] is customer ID.
    v[3] = v[3].substr(2,4);

    return v[3];
}

void CostLRUEviction::compute_periodic_stats(bool floor_customer_loss) {
	typedef std::unordered_map<string, unsigned long> inner_map;
	typedef std::unordered_map<string, inner_map> outer_map;

	for (outer_map::iterator i = customer_hit_stats.begin(), iend = customer_hit_stats.end(); i != iend; ++i)
	{
		// i->first = customer ID (e.g. 307A)
		if (customer_hit_stats[i->first]["periodic_hits_and_misses"] > 0) {
			unsigned long periodic_hit_ratio = 100 * (float) customer_hit_stats[i->first]["periodic_hits"]
												   / (float) customer_hit_stats[i->first]["periodic_hits_and_misses"];
			customer_hit_stats[i->first]["periodic_hit_ratio"] = periodic_hit_ratio;
		}

		if (customer_hit_stats[i->first]["periodic_bytes_hit_and_miss"] > 0) {
			unsigned long periodic_byte_hit_ratio = 100 * (float) customer_hit_stats[i->first]["periodic_bytes_hit"]
												   / (float) customer_hit_stats[i->first]["periodic_bytes_hit_and_miss"];

			if (floor_customer_loss == true) {
				int diff = periodic_byte_hit_ratio - customer_hit_stats[i->first]["periodic_byte_hit_ratio"];
				if (diff > 100) {
					cerr << "compute_periodic_stats(): diff > 100" << __LINE__ << " " << __FILE__;
					exit(1);
				}
				else if (diff <= -2 && customer_hit_stats[i->first]["skip_size_based_deletion"] != 1) {
					// the ratio reduced more than 2% since last time. so we need to protect the customer
					customer_hit_stats[i->first]["skip_size_based_deletion"] = 1;
				}
				else if (diff >= 1 && customer_hit_stats[i->first]["skip_size_based_deletion"] != 0) {
					// things got better, so no need to protect them.
					customer_hit_stats[i->first]["skip_size_based_deletion"] = 0;
				}
			}

			customer_hit_stats[i->first]["periodic_byte_hit_ratio"] = periodic_byte_hit_ratio;
		}

		customer_hit_stats[i->first]["periodic_hits"] = 0;
		customer_hit_stats[i->first]["periodic_bytes_hit"] = 0;
		customer_hit_stats[i->first]["periodic_hits_and_misses"] = 0;
		customer_hit_stats[i->first]["periodic_bytes_hit_and_miss"] = 0;
	}
}

void CostLRUEviction::periodic_output(unsigned long ts, std::ostringstream& outlogfile){
    double oldest_file_age;


    // Output specifcs
    outlogfile << " : " << name << " ";

    // Total size
    outlogfile << get_size() << " ";
    // Oldest file age

    oldest_file_age = ((float) ts - tail->prev->timestamp)/60/60/24;
    outlogfile << oldest_file_age << " ";

}
