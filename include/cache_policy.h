// Copyright 2018, Oath Inc.
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.

/* cache_policy.h
 *
 * Holds the generic cache admission and eviction  classes. Others should
 * instantiate from these.
 */


#ifndef CACHE_POLICY_H_
#define CACHE_POLICY_H_

class CacheAdmission {
    protected:
        std::string name;

    public:
        virtual ~CacheAdmission();
        // Is this key present?
        virtual bool check(std::string key, unsigned long data, unsigned long long size,
                           unsigned long ts, std::string customer_id_str)=0;
        // Reporting
        virtual void periodic_output(unsigned long ts, std::ostringstream& outlogfile)=0;
};

class CacheEviction {

    public:
        virtual ~CacheEviction();

        void set_total_capacity_by_value(unsigned long long size);


        // Put an object in the cache
        virtual unsigned long long put(std::string key, unsigned long data, unsigned long timestamp, unsigned long bytes_out,
                               std::string customer_id, std::string orig_url)=0;
        // Get an object from the cache
        virtual unsigned long get(std::string key, unsigned long ts,
                                  unsigned long bytes_out, std::string url_original)=0;
        // Check to see if an object is in the cache
        virtual int check(std::string key, unsigned long ts)=0;	// to check if present.

        // Reporting and debugging 
        virtual unsigned long long get_size()=0;
        virtual unsigned long long get_total_capacity()=0;

        //virtual void print_avg_oldest_requested_file(unsigned long timestamp)=0;
        //virtual void print_cache_file_age_histogram()=0;
        //virtual void print_cache_file_hits()=0;


        // Hourly Purging
        virtual void hourly_purging(unsigned long timestamp)=0;
        virtual bool purge_regular()=0;

        // Reporting
        virtual void periodic_output(unsigned long ts, std::ostringstream& outlogfile)=0;

    protected:
        std::string name;

    private:
        unsigned long long total_capacity;

};

#endif /* CACHE_POLICY_H__ */
