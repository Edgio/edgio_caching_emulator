// Copyright 2021 Edgecast Inc
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/* 
 * Null admission policy
 *
 */

#ifndef NULL_ADMISSION_H_
#define NULL_ADMISSION_H_

#include "cache_policy.h"

class NullAdmission : public CacheAdmission {

    public:
        NullAdmission();
        ~NullAdmission();

        bool check(std::string key, unsigned long data, unsigned long long size,
                   unsigned long ts, std::string customer_id_str);
        float get_fill_percentage();

        // Reporting
        void periodic_output(unsigned long ts, std::ostringstream& outlogfile);
};


#endif /* NULL_ADMISSION_H__ */
