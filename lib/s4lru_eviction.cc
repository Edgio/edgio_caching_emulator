// Copyright 2018, Oath Inc.
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/*
 * used as S4LRU in our emulation
 *
 *  Created on: Jun 13, 2013
 *      Corrected by: Harkeerat Bedi
 */

#include <assert.h>
#include <map>
#include <algorithm>
#include <iostream>
#include <vector>
#include <tr1/unordered_map>
#include <math.h>
#include <algorithm>
#include <sstream>
#include <fstream>

#include "em_structs.h"
#include "cache_policy.h"
#include "s4lru_eviction.h"

using namespace std;

S4LRUEviction::S4LRUEviction(unsigned long long size, unsigned short queue_count,
                             string id, const EmConfItems * sci){
    name = "s4lru";
    this->sci = sci;

    current_size = 0;
    total_capacity = size / queue_count;
    cache_id = id;

    current_size = new unsigned long long[queue_count];

    this->queue_count = queue_count;
    head = new S4LRUEvictionEntry*[queue_count];
    tail = new S4LRUEvictionEntry*[queue_count];


    for (int i = 0; i < queue_count; i++) {
        // Actually make the obj
        head[i] = new S4LRUEvictionEntry;
        tail[i] = new S4LRUEvictionEntry;
        current_size[i] = 0;


        (head[i])-> prev = NULL;
        (head[i])-> next = tail[i];
        (tail[i])-> next = NULL;
        (tail[i])-> prev = head[i];

    }

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

    if(id.compare("k") == 0) {
        assert(0); // KC uses lru_k.hpp
    }
    if(id.compare("h") == 0) {
        purge_size_based_limit = sci->LRU_list_size;
    }
    cache_item_count = 0;
    max_cache_item_count = 0;
    number_of_bins_for_histogram = 1000;

    hour_count = 0;

}

S4LRUEviction::~S4LRUEviction()
{
    delete [] head;
    delete [] tail;
}


/*
 * We purge only once an hour
 *
 */
void S4LRUEviction::hourly_purging(unsigned long timestamp) {
    return;
}


// to pre-populate the cache (from cache dump)
unsigned long long S4LRUEviction::initial_put(string key, unsigned long data, unsigned long timestamp, unsigned long bytes_out, string customer_id, string orig_url, string access_log_entry_string)
{
    return 0;
}


unsigned long long S4LRUEviction::put(string key, unsigned long data, unsigned long timestamp, unsigned long bytes_out, string customer_id, string orig_url)
{
    /* If Put is being called, it means it's a miss and the admission policy has
    allowed it -- so it should go in queue 0 */


    S4LRUEvictionEntry* node = _mapping[key];

    if(node)
    {
        assert(0); // we should not reach here, because we always 'check' before we 'put'.
    }
    else
    {
        // HD Ingress stats
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

        node = new S4LRUEvictionEntry;
        node->key = key;
        node->data = data;
        node->timestamp = timestamp;
        node->customer_id = customer_id;
        node->orig_url = orig_url;
        node->count = 1;
        node-> queue = 0; // Everything starts in queue 0
        _mapping[key] = node;
        attach(node, 0);

        ++cache_item_count; // used for size based LRU only

        if (max_cache_item_count < cache_item_count) {
            max_cache_item_count = cache_item_count;
        }


        // Don't let it go over disk size!
        purge_regular();


    }
    return 0;
}

unsigned long S4LRUEviction::get(string key, unsigned long ts, unsigned long bytes_out, string url_original)
{
    S4LRUEvictionEntry* node = _mapping[key];
    if(node)
    {
        detach(node);
        // You got fetched! You get promoted to next queue
        attach(node, node->queue + 1);
        node->count = node->count + 1;

        // Actually we also need to purge here -- since you maybe filled up
        // a queue with the move
        purge_regular();

        avg_oldest_requested_file_vector.push_back(node->timestamp);

        node->timestamp = ts;

        return node->data;
    }
    else
    {
        assert(0); // we should not reach here, because we always 'check' before we 'get'.
        return 0;
    }
}

int S4LRUEviction::check_and_print(string key)	// to check if present.
{
    S4LRUEvictionEntry* node = _mapping[key];
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

int S4LRUEviction::check(string key, unsigned long ts)	// to check if present.
{
    S4LRUEvictionEntry* node = _mapping[key];
    if(node)
        return 1;
    else
        return 0;
}

/*
 * default purge: we delete the least recently requested file
 */
bool S4LRUEviction::purge_regular() {

    // Here, we have to loop through queues and move backwards
    for (int j = queue_count - 1; j >= 0; j--) {

        while(current_size[j] > total_capacity) {
            S4LRUEvictionEntry* node = tail[j]->prev;	//switch 1 of 2; for LRU vs. MRU; LRU(tail->prev); 		MRU(head->next)
            if(node == head[j]) {							//switch 2 of 2; for LRU vs. MRU; LRU(node == head); 	MRU(node == tail)
                return false;
            }

            // S4LRUEvictionEntry* headNode = head->next;
            // cout << "\npurge_regular " << headNode->timestamp << " " << node->timestamp << " " << node->data << " " << node->key << "\n";
            // It has to leave this queue, detach it
            detach(node);

            // We are at the bottom...
            if (node->queue == 0) {
                // Log some stats
                /* Only do this stuff if its getting kicked out all the way */
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
                // Clean out the node
                _mapping.erase(node->key);
                delete node;
                --cache_item_count;
            }
            else {
                // Ok here, it's just getting bumped down a level
                attach(node, j - 1);
            }


        }
    }
    return true;


}

unsigned long long S4LRUEviction::get_size() {
    unsigned long long total_size = 0;

    for (int i=0; i < queue_count; i++) {
        total_size += current_size[i];
    }
    return total_size;
}

unsigned long long S4LRUEviction::get_total_capacity() {
    return total_capacity;
}

void S4LRUEviction::set_total_capacity_by_value(unsigned long long size) {
    total_capacity = size/queue_count;
}



void S4LRUEviction::detach(S4LRUEvictionEntry* node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    // Need the queue to know how to remove
    unsigned short queue = node->queue;


    current_size[queue] = current_size[queue] - node->data;
}

void S4LRUEviction::attach(S4LRUEvictionEntry* node, unsigned short queue)
{
    // To make logic simpler elsewhere, queue is maxed out here
    if (queue > queue_count - 1) {
        queue = queue_count - 1;
    }

    node->next = head[queue]->next;
    node->prev = head[queue];
    head[queue]->next = node;
    node->next->prev = node;
    current_size[queue] = current_size[queue] + node->data;

    // Set the queue in the node
    node-> queue = queue; // Everything starts in queue 0
}

string S4LRUEviction::return_customer_id(string url) {
    std::vector<std::string> v; // http://rosettacode.org/wiki/Tokenize_a_string#C.2B.2B
    std::istringstream buf(url);
    for(std::string token; getline(buf, token, '/'); )
        v.push_back(token);

    // v[3] is customer ID.
    v[3] = v[3].substr(2,4);

    return v[3];
}

void S4LRUEviction::print_oldest_file_age(unsigned long timestamp, ostream &output) {
    cout << "Oldest file age not supported!" << endl;
}

void S4LRUEviction::periodic_output(unsigned long ts, std::ostringstream& outlogfile){
    // Output specifcs
    outlogfile << " : " << name << " ";

    // Total size
    outlogfile << get_size() << " ";
    // Oldest file age

}

