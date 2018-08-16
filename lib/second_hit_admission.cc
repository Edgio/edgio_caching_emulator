// Copyright 2018, Oath Inc.
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.

/*
 * Second Hit Caching Cache Admission Policy
 *
 */

#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include "bloomfilter.h"
#include "cache_policy.h"
#include "second_hit_admission.h"

using namespace std;

SecondHitAdmission::SecondHitAdmission(string file_name, size_t _nfuncs,
                                        unsigned long size, int _NVAL,
                                        vector<string> no_bf_cust) {
    name = "2hc";
    this->no_bf_cust = no_bf_cust;
    BF  = new BloomFilter ((char *)file_name.c_str(), _nfuncs, size, _NVAL);
}

SecondHitAdmission::~SecondHitAdmission() {
    delete BF;
}

// Should we let this in?
bool SecondHitAdmission::check(string key, unsigned long data, unsigned long long size,
                               unsigned long ts, string customer_id_str) {


    // Check to see if this customer bypasses the bloom filter. If so, just let
    // it in
    bool _b = check_customer_in_list(customer_id_str);
    if (_b == true) {
        return true;
    }

    // We have it in the bloom filter, go ahead and accept it!
    if (BF->check((char *)key.c_str())) {
        return true;
    } else {
        // We don't have it, let's add it, and return false
        BF->add((char *)key.c_str());
        return false;
    }

}

bool SecondHitAdmission::check_customer_in_list(string custid) const{
	vector<string>::iterator it;
	if(find(no_bf_cust.begin(), no_bf_cust.end(), custid) != no_bf_cust.end())
		return true;
	else
		return false;
}


float SecondHitAdmission::get_fill_percentage() {
    struct bloom_filter_stats bfstats;
    BF->get_live_stats(bfstats);
    return bfstats.fill_percentage;
}

void SecondHitAdmission::periodic_output(unsigned long ts, std::ostringstream& outlogfile){
    // Output LRUEviction specifcs
    outlogfile << " : " << name << " ";

    // Total size
    outlogfile << get_fill_percentage() << " ";
}

/****************************
 * 
 * 2 Rotating Bloom Filters 
 *
 ****************************/
SecondHitAdmissionRot::SecondHitAdmissionRot(string file_name, size_t _nfuncs,
                                        unsigned long size, int _NVAL,
                                        vector<string> no_bf_cust,
                                        unsigned long max_age) {

    name = "2hc_rot";
    /* Save all the init garbage */
    this->file_name = file_name;
    this->_nfuncs = _nfuncs;
    this->bf_size = size;
    this->_NVAL = _NVAL;

    /* Make one BF */
    head = new BFEntry;

    head->BF = new BloomFilter ((char *)file_name.c_str(), _nfuncs, size, _NVAL);
    head->init_time = 0; // Needs clever handling down stream
    head->next = NULL;

    /* Fill in the other junk */
    this->no_bf_cust = no_bf_cust;
    this->max_age = max_age;

}

SecondHitAdmissionRot::~SecondHitAdmissionRot() {
    /* Here, we have to walk our list and delete as we go */
    BFEntry *curr = head;
    BFEntry *next_entry;

    while (curr != NULL) {
        /* Delete the BF inside */
        delete curr->BF;
        /* Get the next one */
        next_entry = curr->next;
        /* Delete the durrent one*/
        delete curr;
        /* Pass pointer */
        curr = next_entry;
    }

}

// Should we let this in?
bool SecondHitAdmissionRot::check(string key, unsigned long data, unsigned long long size,
                               unsigned long ts, string customer_id_str) {

    BFEntry* new_bf;
    unsigned long age;

    // Check to see if this customer bypasses the bloom filter. If so, just let
    // it in
    bool _b = check_customer_in_list(customer_id_str);
    if (_b == true) {
        return true;
    }

    /* Check the age on the head, is it time to rotate */
    if (head->init_time == 0) {
        /* If it was the init, take the current time as the start */
        head->init_time = ts;
    }
    age = ts - head->init_time;
    /* It's time to rotate*/
    if (age > max_age) {
        // Check the next guy, delete him if he exists
        // Probably thhis should be a loop and not just a look ahead of 1
        cout << "Rotating BF!" << endl;
        if (head->next != NULL) {
            delete head->next->BF;
            delete head->next;
            head->next = NULL;
        }
        // Make a new one and stick it at the head
        new_bf = new BFEntry;
        new_bf->BF = new BloomFilter ((char *)file_name.c_str(), _nfuncs, bf_size, _NVAL);
        new_bf->init_time = ts; // Now
        new_bf->next = head;
        // stick it in front
        head = new_bf;

        cout << "Done rotating BF!" << endl;
    }

    // Ok now we start climbing down
    if (head->BF->check((char *)key.c_str())) {
        return true;
    } else {
        // We don't have it, let's add it
        head->BF->add((char *)key.c_str());

        // Now let's check the next one
        if ((head->next != NULL) &&
            (head->next->BF->check((char *)key.c_str()))) {
            // We had it in the old one, let it in
            return true;
        } else {
            // Else, either:
            //  not in the old one, return false
            //  There is no old one, return false
            return false;
        }
    }


    cout << "unexpected!" << endl;

}

bool SecondHitAdmissionRot::check_customer_in_list(string custid) const{
	vector<string>::iterator it;
	if(find(no_bf_cust.begin(), no_bf_cust.end(), custid) != no_bf_cust.end())
		return true;
	else
		return false;
}

float SecondHitAdmissionRot::get_fill_percentage() {
    struct bloom_filter_stats bfstats;
    head->BF->get_live_stats(bfstats);
    return bfstats.fill_percentage;
}

void SecondHitAdmissionRot::periodic_output(unsigned long ts, std::ostringstream& outlogfile){
    // Output LRUEviction specifcs
    outlogfile << " : " << name << " ";

    // Total size
    outlogfile << get_fill_percentage() << " ";
}
