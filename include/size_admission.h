// Copyright 2018, Oath Inc.
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/* 
 * Null admission policy
 *
 */

#ifndef SIZE_ADMISSION_H_
#define SIZE_ADMISSION_H_

#include "cache_policy.h"

/* Size threshold admission */
class SizeAdmission : public CacheAdmission {

    public:
        SizeAdmission(unsigned long long threshold);
        ~SizeAdmission();

        bool check(std::string key, unsigned long data, unsigned long long size,
                   unsigned long ts, std::string customer_id_str);
        float get_fill_percentage();
        // Reporting
        void periodic_output(unsigned long ts, std::ostringstream& outlogfile);

    private:
        unsigned long long threshold;

};

/* Prob. Admission */
class ProbAdmission : public CacheAdmission {

    public:
        ProbAdmission(double threshold);
        ~ProbAdmission();

        bool check(std::string key, unsigned long data, unsigned long long size,
                   unsigned long ts, std::string customer_id_str);

        float get_fill_percentage();
        // Reporting
        void periodic_output(unsigned long ts, std::ostringstream& outlogfile);
    private:
        double prob;

};

/* Size based Prob. admission */
class ProbSizeAdmission : public CacheAdmission {

    public:
        ProbSizeAdmission(unsigned long long threshold);
        ~ProbSizeAdmission();

        bool check(std::string key, unsigned long data, unsigned long long size,
                   unsigned long ts, std::string customer_id_str);

        float get_fill_percentage();
        // Reporting
        void periodic_output(unsigned long ts, std::ostringstream& outlogfile);
    private:
        unsigned long long c;

};

#endif /* SIZE_ADMISSION_H__ */
