// Copyright 2018, Oath Inc.
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/* 
 * LRU Cache Eviction Policy
 *
 */

#ifndef LRU_EVICTION_H_
#define LRU_EVICTION_H_

struct item_attr
{
    item_attr()
        : score(0)
    {}
    double score;
};

struct LRUEvictionEntry
{
    std::string key; // hash key
    std::string customer_id;
    std::string orig_url; // original URL
    std::string access_log_entry_string;
    unsigned long data;
    unsigned long timestamp;
    unsigned long count; // keep track of request count
    LRUEvictionEntry* prev;
    LRUEvictionEntry* next;
};



class LRUEviction : public CacheEviction {
    private:
        const EmConfItems* sci;

        std::tr1::unordered_map< std::string, LRUEvictionEntry*>	_mapping;
        std::vector<unsigned int>			avg_oldest_requested_file_vector;
        unsigned long long				current_size;
        unsigned long long				total_capacity;
        LRUEvictionEntry*               head;
        LRUEvictionEntry*               tail;
        std::string                     cache_id; // k=kernel, h=hdd
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

        int                             base_purge_algo;
        unsigned int					regular_purge_interval;

        unsigned long								previous_hour_timestamp_ingress;
        unsigned long								current_ingress_item_timestamp;
        unsigned long long				ingress_total_size;
        unsigned long					ingress_total_count;

        unsigned long								previous_hour_timestamp_egress;
        unsigned long long				egress_total_size;
        unsigned long					egress_total_count;
        unsigned int					number_of_bins_for_histogram;
        std::tr1::unordered_map<std::string, item_attr> m_item_unordered_map;

        long							hour_count;

    public:
        LRUEviction(unsigned long long size, std::string id, const EmConfItems * sci);
        ~LRUEviction();

        /*
         * We purge only once an hour (currently in use)
         *
         */
        void hourly_purging(unsigned long timestamp);

        // Put things in the cache
        unsigned long long put(std::string key, unsigned long data, unsigned long timestamp, unsigned long bytes_out, std::string customer_id, std::string orig_url);
        // "fetch" an object from the cache
        unsigned long get(std::string key, unsigned long ts, unsigned long bytes_out, std::string url_original);
        // Check if its present
        int check(std::string key, unsigned long ts);
        // default purge: we delete the least recently requested file
        bool purge_regular();


        unsigned long long get_size();
        unsigned long long get_total_capacity();


        void set_total_capacity();

        // Reporting
        void periodic_output(unsigned long ts, std::ostringstream& outlogfile);

        // Debugging only
        void print_avg_oldest_requested_file(unsigned long timestamp);
        void print_cache_file_age_histogram();
        void print_cache_file_hits();
        void dump_cache_contents(std::string filename);
        void dump_cache_contents_cout();
        void print_max_cache_item_count();


    private:
        void detach(LRUEvictionEntry* node);
        void attach(LRUEvictionEntry* node);

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
