// Copyright 2021 Edgio Inc
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/* 
 * LRU Cache Eviction Policy
 *
 */

#ifndef S4LRU_EVICTION_H_
#define S4LRU_EVICTION_H_

struct S4LRUEvictionEntry
{
    std::string key; // hash key
    std::string customer_id;
    std::string orig_url; // original URL
    std::string access_log_entry_string;
    unsigned long data;
    unsigned long timestamp;
    unsigned long count; // keep track of request count

    unsigned short queue;  // Which queue is it in?

    S4LRUEvictionEntry* prev;
    S4LRUEvictionEntry* next;
};



class S4LRUEviction : public CacheEviction {
    private:
        const EmConfItems* sci;

        std::unordered_map< std::string, S4LRUEvictionEntry*>	_mapping;
        std::vector<unsigned int>			avg_oldest_requested_file_vector;

        unsigned long long*				current_size;
        unsigned long long				total_capacity;


        /* Now instead of a just H/T, we need one for each queue */
        unsigned short                  queue_count;


        S4LRUEvictionEntry**		    head;
        S4LRUEvictionEntry**		    tail;


        std::string						cache_id; // k=kernel, h=hdd
        unsigned int					purge_size_based_limit;
        unsigned long long				cache_item_count; // used for size based LRU only
        unsigned long long				max_cache_item_count; // used for size based LRU only
        unsigned long					previous_hour_timestamp;
        unsigned long					previous_hour_timestamp_put;
        unsigned long long				total_items_purged;
        unsigned long long				total_junk_items_purged;
        unsigned int					total_junk_purge_operations;
        unsigned int					total_hourly_purge_intervals;
        bool							cache_filled_once;

        int                             base_purge_algo;
        unsigned int					regular_purge_interval;

        unsigned long					previous_hour_timestamp_ingress;
        unsigned long					current_ingress_item_timestamp;
        unsigned long long				ingress_total_size;
        unsigned long					ingress_total_count;

        unsigned long								previous_hour_timestamp_egress;
        unsigned long long				egress_total_size;
        unsigned long					egress_total_count;
        unsigned int					number_of_bins_for_histogram;


        long                            hour_count;

    public:
        S4LRUEviction(unsigned long long size, unsigned short queue_count,
                      std::string id, const EmConfItems * sci);
        ~S4LRUEviction();

        /*
         * We purge only once an hour (currently in use)
         *
         */
        void hourly_purging(unsigned long timestamp);

        // to pre-populate the cache (from cache dump)
        unsigned long long initial_put(std::string key, unsigned long data, unsigned long timestamp, unsigned long bytes_out,
                            std::string customer_id, std::string orig_url, std::string access_log_entry_string);
        unsigned long long put(std::string key, unsigned long data, unsigned long timestamp, unsigned long bytes_out, std::string customer_id, std::string orig_url);

        unsigned long get(std::string key, unsigned long ts, unsigned long bytes_out, std::string url_original);
        int check_and_print(std::string key);	// to check if present.
        int check(std::string key, unsigned long ts);	// to check if present.

        /*
         * default purge: we delete the least recently requested file
         */
        bool purge_regular();

        unsigned long long get_size();
        unsigned long long get_total_capacity();

        void print_oldest_file_age(unsigned long timestamp, std::ostream &output);

        void set_total_capacity();

        void set_total_capacity_by_value(unsigned long long size);

        // Reporting
        void periodic_output(unsigned long ts, std::ostringstream& outlogfile);

    private:

        void attach(S4LRUEvictionEntry* node, unsigned short queue);
        void detach(S4LRUEvictionEntry* node);

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

#endif /* S4LRU_EVICTION_H_ */
