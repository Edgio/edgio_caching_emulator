// Copyright 2021 Edgio Inc
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.

#include <iostream>
#include <fstream>
#include <sstream>

// Emulator stuff we will always need
#include "em_structs.h"
#include "emulator.h"
#include "cache.h"

// The specific policies we will consider
#include "second_hit_admission.h"
#include "null_admission.h"
#include "lru_eviction.h"
#include "fifo_eviction.h"

using namespace std;
/*
 * Main!
 *
 */
int main(int argc, char *argv[]) {

    cout << "\nExecutable: \t" << argv[0] << "\n";

    Emulator* em = new Emulator(cout, false, argc, argv);

    // Some random seeding work
    srand(time(NULL));
    ostringstream ossf;
    ossf << rand();

    // Some reused parameters

    unsigned long long kc_max_size_gig = em->sci->kc_gig;
    unsigned long long hd_max_size_gig = em->sci->hd_gig;
    unsigned long long kc_max_size_bytes = kc_max_size_gig *1024*1024*1024;
    unsigned long long hd_max_size_bytes = hd_max_size_gig *1024*1024*1024;

    string kc_file_name = string(ossf.str() + ".kcbf");
    string hd_file_name = string(ossf.str() + ".bf");

    // Let's make a k-kache
    Cache* kc = new Cache(0, // Unused
                          false, // Hourly purging?
                          false, // Respect admission of lower?
                          kc_max_size_gig // Size
                         );
    CacheAdmission* kc_ad = new NullAdmission();
    CacheEviction* kc_evict = new LRUEviction(kc_max_size_bytes, "h", em->sci);
    kc->set_admission(kc_ad);
    kc->set_eviction(kc_evict);


    // Let's make a hard drive
    Cache* hd = new Cache(0, false, false, hd_max_size_gig);
    CacheAdmission* hd_ad = new SecondHitAdmissionRot(hd_file_name, 5,
                                                   50*1024*1024*8,
                                                   em->sci->_NVAL,//2nd hit
                                                   em->sci->no_bf_cust,
                                                   em->sci->bf_reset_int);
    //CacheAdmission* hd_ad = new NullAdmission();
    CacheEviction* hd_evict = new LRUEviction(hd_max_size_bytes, "h", em->sci);
    hd->set_admission(hd_ad);
    hd->set_eviction(hd_evict);

    /* Config the cache layers we made */
    // TODO: KC disabled!
    //em->add_to_tail(kc);
    em->add_to_tail(hd);

    // Run it
    /**************************/
    em->populate_access_log_cache();
    /**************************/

    delete kc;
    delete kc_ad;
    delete kc_evict;

    delete hd;
    delete hd_ad;
    delete hd_evict;

    delete em;

    return 0;
}
