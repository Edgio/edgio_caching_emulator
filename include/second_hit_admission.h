// Copyright 2021 Edgio Inc
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/* 
 * The Second Hit Caching admission policy
 *
 */

#ifndef SECOND_HIT_ADMISSION_H_
#define SECOND_HIT_ADMISSION_H_

#include "bloomfilter.h"
#include "cache_policy.h"

/************************
 *
 * A single Bloom Filter
 *
 ***********************/
class SecondHitAdmission : public CacheAdmission {
    private:
        BloomFilter * BF;
	    std::vector<std::string> no_bf_cust;

    public:
        SecondHitAdmission(std::string file_name, size_t _nfuncs,
                    unsigned long size, int _NVAL,
                    std::vector<std::string> no_bf_cust);
        ~SecondHitAdmission();

        bool check(std::string key, unsigned long data, unsigned long long size,
                   unsigned long ts, std::string customer_id_str);
	    bool check_customer_in_list(std::string custid) const;

        float get_fill_percentage();
        // Reporting
        void periodic_output(unsigned long ts, std::ostringstream& outlogfile);
};

/****************************
 * 
 * 2 Rotating Bloom Filters 
 *
 ****************************/

struct BFEntry
{
    BloomFilter * BF;
    unsigned long int init_time;

    BFEntry* next;

};


class SecondHitAdmissionRot : public CacheAdmission {
    private:
        unsigned long max_age;

        BFEntry * head;

	    std::vector<std::string> no_bf_cust;

        /* Initialization stuff */
        std::string file_name;
        size_t _nfuncs;
        unsigned long bf_size;
        int _NVAL;

/****************************
 * 
 * 2 Rotating Bloom Filters 
 *
 ****************************/
    public:
        SecondHitAdmissionRot(std::string file_name, size_t _nfuncs,
                    unsigned long size, int _NVAL,
                    std::vector<std::string> no_bf_cust,
                    unsigned long max_age);
        ~SecondHitAdmissionRot();

        bool check(std::string key, unsigned long data, unsigned long long size,
                   unsigned long ts, std::string customer_id_str);
	    bool check_customer_in_list(std::string custid) const;

        float get_fill_percentage();
        // Reporting
        void periodic_output(unsigned long ts, std::ostringstream& outlogfile);

};


#endif /* SECOND_HIT_ADMISSION_H__ */
