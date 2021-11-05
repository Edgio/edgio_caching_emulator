// Copyright 2021 Edgecast Inc
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
 /*
 * The generaic cache object. Inteded a s a class to be inherited by others.
 *
 * Created On: May 5, 2017
 *
 */

#ifndef CACHE_H_
#define CACHE_H_

struct item_packet;
class CacheAdmission;
class CacheEviction;

class Cache {
    private:
        // Admission Policy
        CacheAdmission* admission;

        // Eviction Policy
        CacheEviction* eviction;

        // The next one in the chain!
        Cache* next;

        // Local Logging
        unsigned long miss;
        unsigned long long byte_miss;
        unsigned long hit;
        unsigned long long byte_hit;
        unsigned long long reads_from_origin;


        // Storage book-keeping
        unsigned long long cache_total_size;
        unsigned long long max_cache_size;
        unsigned long number_of_reads;
        unsigned long number_of_writes;
        unsigned long number_of_purges;
        unsigned long size_of_purges;
        unsigned int number_of_bytes_per_read;
        unsigned int number_of_bytes_per_write;

        bool store_access_line_and_url;

        bool do_hourly_purging;

        bool respect_lower_admission;

    public:

        Cache (bool store_access_line_and_url, bool do_hourly_purging,
               bool respect_lower_admission, unsigned long long size);
        ~Cache();

        // Setup Interfaces
        void set_admission(CacheAdmission* new_ad);
        void set_eviction(CacheEviction* new_ev);

        // Main Cache interface 
        bool process(item_packet* ip_inst);

        // Cache Setup
        void set_next(Cache* next_cache);
        Cache* get_next();

        // Accessors for the cache status
        unsigned long get_hit();
        unsigned long get_miss();
        unsigned long long get_hit_bytes();
        unsigned long long get_miss_bytes();

        unsigned long get_hm_local();
        unsigned long get_hit_total();

        unsigned long long get_hm_bytes_local();
        unsigned long long get_hit_bytes_total();

        unsigned long long get_origin_reads_total();

        // Setters
        void clear_counters();

        // Storage book keeping
        unsigned long long  get_total_size();
        unsigned long get_nb_reads();
        unsigned long get_nb_writes();
        unsigned long get_nb_purges();
        unsigned long get_size_purges();
        void reset_disk_counters();

        // Cache Interaction
        bool check(std::string url, unsigned long size, unsigned long ts, bool penalize_url,
                int customer_id, unsigned long bytes_out, std::string
                customer_id_str, std::string orig_url); // uses new LRU function
        bool add(std::string url, unsigned long size, unsigned long ts, bool penalize_url,
                int customer_id, unsigned long bytes_out, std::string
                customer_id_str, std::string orig_url); // uses new LRU function

        // Reporting!
        void periodic_output(unsigned long ts, std::ostringstream& outlogfile);
        void hourly_purging(unsigned long ts);
};


#endif /* CACHE_H_ */
