// Copyright 2021 Edgio Inc
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/* 
 * FIFO Cache Eviction Policy
 *
 */

#ifndef FIFO_EVICTION_H_
#define FIFO_EVICTION_H_

struct FIFOEvictionEntry
{
    std::string key; // hash key
    std::string customer_id;
    std::string orig_url; // original URL
    std::string access_log_entry_string;
    unsigned long data;
    unsigned long timestamp;
    unsigned long count; // keep track of request count
    FIFOEvictionEntry* prev;
    FIFOEvictionEntry* next;
};



class FIFOEviction : public CacheEviction {
    private:
        const EmConfItems* sci;

        std::unordered_map< std::string, FIFOEvictionEntry*>	_mapping;
        std::vector<unsigned int>			avg_oldest_requested_file_vector;
        unsigned long long				current_size;
        unsigned long long				total_capacity;
        FIFOEvictionEntry*              head;
        FIFOEvictionEntry*              tail;
        std::string                     cache_id; // k=kernel, h=hdd
        unsigned int					purge_size_based_limit;
        unsigned long long				cache_item_count; // used for size based FIFO only
        unsigned long long				max_cache_item_count; // used for size based FIFO only
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

        long							hour_count;

    public:
        FIFOEviction(unsigned long long size, std::string id, const EmConfItems * sci);
        ~FIFOEviction();

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
        void detach(FIFOEvictionEntry* node);
        void attach(FIFOEvictionEntry* node);
};

#endif /* FIFO_EVICTION_H_ */
