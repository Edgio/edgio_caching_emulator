// Copyright 2018, Oath Inc.
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/* 
 * LRU Cache Eviction Policy
 *
 */

#ifndef SIZE_LRU_EVICTION_H_
#define SIZE_LRU_EVICTION_H_

struct s_item_attr
{
    s_item_attr()
        : score(0)
    {}
    double score;
};

struct SizeLRUEvictionEntry
{
    std::string key; // hash key
    std::string customer_id;
    std::string orig_url; // original URL
    std::string access_log_entry_string;
    unsigned long data;
    unsigned long timestamp;
    unsigned long count; // keep track of request count
    SizeLRUEvictionEntry* prev;
    SizeLRUEvictionEntry* next;
};



class SizeLRUEviction : public CacheEviction {
    private:
        const EmConfItems* sci;

        std::tr1::unordered_map< std::string, SizeLRUEvictionEntry*>	_mapping;
        std::vector<unsigned int>			avg_oldest_requested_file_vector;
        unsigned long long				current_size;
        unsigned long long				total_capacity;
        SizeLRUEvictionEntry*			head;
        SizeLRUEvictionEntry*			tail;
        std::string 							cache_id; // k=kernel, h=hdd
        unsigned int					purge_size_based_limit;
        unsigned long long				cache_item_count; // used for size based LRU only
        unsigned long long				max_cache_item_count; // used for size based LRU only
        unsigned long								previous_hour_timestamp;
        unsigned long								previous_hour_timestamp_put;
        unsigned long long				total_items_purged;
        unsigned long long				total_junk_items_purged;
        unsigned int					total_junk_purge_operations;
        unsigned int					total_hourly_purge_intervals;
        bool							cache_filled_once;

        int 							base_purge_algo;
        unsigned int					regular_purge_interval;

        unsigned long								previous_hour_timestamp_ingress;
        unsigned long								current_ingress_item_timestamp;
        unsigned long long				ingress_total_size;
        unsigned long					ingress_total_count;

        unsigned long								previous_hour_timestamp_egress;
        unsigned long long				egress_total_size;
        unsigned long					egress_total_count;
        unsigned int					number_of_bins_for_histogram;
        int 							lru_algo3_reg_purge;
        double 							m_w_origin_cost, w_size, w_age, w_item_attr_val;
        std::tr1::unordered_map<std::string, s_item_attr> m_item_unordered_map;

        double							running_size_mu, running_size_var;
        double							alpha_running_size_mu, alpha_running_size_var;
        long							hour_count;

        // Customer hit stats brought in here from emstructs and the old
        // reporting variables objects
	    std::tr1::unordered_map<std::string,
		std::tr1::unordered_map<std::string, unsigned long> > customer_hit_stats;
	    void compute_periodic_stats(bool floor_customer_loss);


    public:
        SizeLRUEviction(unsigned long long size, std::string id, const EmConfItems * sci);
        ~SizeLRUEviction();

        /*
         * to print the age of all files
         * in cache using a histogram (based on location)
         */
        void print_cache_file_age_histogram();
        /*
         * to print the hit count of all files
         */
        void print_cache_file_hits();
        /*
         * We purge only once an hour (currently in use)
         *
         */
        void hourly_purging(unsigned long timestamp);

        // for debugging, also keeps track of count.
        unsigned long long initial_put_count(std::string key, unsigned long data, unsigned long timestamp,
                                             unsigned long bytes_out,
                                             std::string customer_id, std::string orig_url);
        // to pre-populate the cache (from cache dump)
        unsigned long long initial_put(std::string key, unsigned long data, unsigned long timestamp, unsigned long bytes_out,
                            std::string customer_id, std::string orig_url, std::string access_log_entry_string);
        void print_oldest_file_age(unsigned long timestamp, std::ostream &output);
        void print_oldest_file_age_for_monitored_customers(unsigned long currentTimeStamp);
        unsigned long long put(std::string key, unsigned long data, unsigned long timestamp, unsigned long bytes_out, std::string customer_id, std::string orig_url);

        void print_avg_oldest_requested_file(unsigned long timestamp);
        unsigned long get(std::string key, unsigned long ts, unsigned long bytes_out, std::string url_original);
        int check_and_print(std::string key);	// to check if present.
        int check(std::string key, unsigned long ts);	// to check if present.

        unsigned long manual_delete(std::string key);
        /*
         * default purge: we delete the least recently requested file
         */
        bool purge_regular();
        void weighted_purge();
        /*
         * faster version (based on approximation)
         * in progress
         */
        void purge_customer_based_approx();
        /*
         *
         * this is the slow version. it is accurate.
         * but very slow
         * never ran this completely.
         */
        void purge_customer_based();
        void dump_cache_contents(std::string filename);
        void dump_cache_contents_cout();
        unsigned long long get_size();
        unsigned long long get_total_capacity();

        void print_max_cache_item_count();

        void set_total_capacity();

        void set_total_capacity_by_value(unsigned long long size);

        // Reporting
        void periodic_output(unsigned long ts, std::ostringstream& outlogfile);

    private:
        void decide_items_based_on_score();
        void update_size_running_mean(SizeLRUEvictionEntry* node);
        void compute_scores();
        void update_cost_based_score(SizeLRUEvictionEntry* node);

        void detach(SizeLRUEvictionEntry* node);
        void attach(SizeLRUEvictionEntry* node);

        void purge_size_based_multimap();

        /*
         * FOR LRU ONLY
         * Here we purge based on size.
         * we look at the least recently requested "purge_size_based_limit" files
         * and pick the one with the largest size
         */
        void purge_size_based();

        std::string return_customer_id(std::string url);
};

#endif /* LRU_EVICTION_H_ */
