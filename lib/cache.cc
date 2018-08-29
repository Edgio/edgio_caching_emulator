// Copyright 2018, Oath Inc.
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <string>
#include <vector>

#include "status.h"
#include "bloomfilter.h"
#include "em_structs.h"
#include "cache_policy.h"
#include "second_hit_admission.h"
#include "null_admission.h"
#include "lru_eviction.h"
#include "fifo_eviction.h"
#include "cache.h"

using namespace std;

Cache::Cache (bool store_access_line_and_url, bool do_hourly_purging,
              bool respect_lower_admission,
              unsigned long long size)
{
    next = NULL;

    unsigned long long max_size = size *1024*1024*1024;

    // Zero out all the counters
    miss = 0;
    byte_miss = 0;
    hit = 0;
    byte_hit = 0;
    reads_from_origin = 0;

    admission = NULL;
    eviction = NULL;

    // Aet all the counters, etc.
    max_cache_size = max_size;
    cache_total_size = 0;
    number_of_purges = 0;
    number_of_bytes_per_read = 512;
    number_of_bytes_per_write = 512;
    number_of_reads = 0;
    number_of_writes = 0;
    this->store_access_line_and_url = store_access_line_and_url;
    this->do_hourly_purging = do_hourly_purging;
    this->respect_lower_admission = respect_lower_admission;

    size_of_purges = 0;
}

Cache::~Cache()
{
}

/* Set the admission and eviction policies */
void Cache::set_admission(CacheAdmission* new_ad) {
    admission = new_ad;
}
void Cache::set_eviction(CacheEviction* new_ev) {
    eviction = new_ev;
}

bool Cache::process(item_packet* ip_inst){
    bool penalize_url = false;
    string cache_key = ip_inst->city64_str;


    // Actually Check the cache
    if (!check(cache_key, ip_inst->size, ip_inst->ts, penalize_url,
               0, // customer id for penalize url
               ip_inst->bytes_out, ip_inst->customer_id, ip_inst->url)){

        //Miss!

        // Local stats
        miss++;
        byte_miss += ip_inst->size;


        // Is there another layer to check?
        if (next != NULL) {
            // Call the next gues process function
            // If they add it, we add it
            if (next->process(ip_inst) == true) {
                return add(cache_key, ip_inst->size, ip_inst->ts, penalize_url,
                           0, ip_inst->bytes_out, ip_inst->customer_id, ip_inst->url);
            } else {
                // You might turn this on if your first cache is a kernel cache, etc.
                if (respect_lower_admission == true) {
                    // For whatever reason the lower layer didn't take it, so we
                    // won't. 
                    return false;
                }
                else {
                    // Lower level didn't take it, but we will (might)
                    return add(cache_key, ip_inst->size, ip_inst->ts, penalize_url,
                               0, ip_inst->bytes_out, ip_inst->customer_id, ip_inst->url);
                }
            }
        }
        else {
            // We were the last stand! origin pull!
            reads_from_origin += ip_inst->size;
            // Add it (or do whatever the policy says)
            return add(cache_key, ip_inst->size, ip_inst->ts, penalize_url,
                       0, ip_inst->bytes_out, ip_inst->customer_id, ip_inst->url);
        }
    }
    else {
        // Hit!
        hit++;
        byte_hit += ip_inst->size;

        // Ok so it was a hit here, cache it above
        return true;
    }
}

/* Set the next cache to given */
void Cache::set_next(Cache* next_cache){
    next = next_cache;
}

Cache* Cache::get_next() {
    return next;
}

/* Dump periodic cache info */
void Cache::periodic_output(unsigned long ts, ostringstream& outlogfile) {

    float hitrate, bytehitrate;

    // Output stuff that is generic, all cache track
    outlogfile << " |\tcache ";

    // Compute hit rate and bytehit rate first
    if ((get_hm_local()) > 0){
        hitrate = (float) (get_hit()) / (float) (get_hm_local());
    } else {
        hitrate = 0;
    }
    if ((get_hm_bytes_local()) > 0){
        bytehitrate = (float) (get_hit_bytes()) / (float) (get_hm_bytes_local());
    } else {
        bytehitrate = 0;
    }

    outlogfile << hitrate << " ";
    outlogfile << bytehitrate << " ";

    outlogfile << hit << " ";
    outlogfile << miss << " ";
    outlogfile << byte_hit << " ";
    outlogfile << byte_miss << " ";

    outlogfile << number_of_reads << " ";
    outlogfile << number_of_writes << " ";
    outlogfile << number_of_purges << " ";

    outlogfile << reads_from_origin << " ";

    // Call the admission to see if it has anything
    admission->periodic_output(ts, outlogfile);
    // Call the eviction
    eviction->periodic_output(ts, outlogfile);

    // Clear the local cache counters 
    clear_counters();
    // Reset all the read/write counters
    reset_disk_counters();
    return;
}


/* Getters for hit and miss */
unsigned long Cache::get_hit() {
    return hit;
}
unsigned long Cache::get_miss() {
    return miss;
}
unsigned long long Cache::get_hit_bytes() {
    return byte_hit;
}
unsigned long long Cache::get_miss_bytes() {
    return byte_miss;
}

/* Get the number of hits + misses at this cache */
unsigned long Cache::get_hm_local(){
    return hit + miss;
}

/* Get number of hits at this cache and down */
unsigned long Cache::get_hit_total() {
    Cache* curr_cache;
    unsigned long total=0;

    curr_cache = this;
    while (curr_cache != NULL) {
        total += curr_cache->hit;
        curr_cache = curr_cache->next;
    }

    return total;
}

/* Get the total number of hit and missed bytes at this cache*/
unsigned long long Cache::get_hm_bytes_local() {
    return byte_hit + byte_miss;
}

/*Get bytes of hits all the way down */
unsigned long long Cache::get_hit_bytes_total() {
    Cache* curr_cache;
    unsigned long long total=0;

    curr_cache = this;
    while (curr_cache != NULL) {
        total += curr_cache->byte_hit;
        curr_cache = curr_cache->next;
    }

    return total;
}

/* Decends the tree and asks for origin reads from
 * the last one (should be 0 everywhere above). */
unsigned long long Cache::get_origin_reads_total() {
    Cache* curr_cache;

    curr_cache = this;
    while (curr_cache->next != NULL) {
        curr_cache = curr_cache->next;
    }

    return curr_cache->reads_from_origin;
}

/* Just blow away the stuff */
void Cache::clear_counters() {
    hit = 0;
    miss = 0;
    byte_hit = 0;
    byte_miss = 0;
    reads_from_origin = 0;
}

unsigned long long  Cache::get_total_size()
{
    //return cache_total_size;
    return eviction->get_size();
}

unsigned long Cache::get_nb_reads()
{
    return number_of_reads;
}

unsigned long Cache::get_nb_writes()
{
    return number_of_writes;
}

unsigned long Cache::get_nb_purges()
{
    return number_of_purges;
}

unsigned long Cache::get_size_purges()
{
    return size_of_purges;
}

void Cache::reset_disk_counters()
{
    number_of_reads = 0;
    number_of_writes = 0;
    number_of_purges = 0;
    size_of_purges = 0;
}

bool Cache::check(string url, unsigned long size, unsigned long ts,  bool penalize_url,
        int customer_id, unsigned long bytes_out, string customer_id_str, string orig_url)
{
    // This should check its own contents, call out to Admission and Eviction to 
    // let them know (ie update LRU) and then return the value


    if(eviction->check(url, ts)) { // found
        //kc->add(url,size, ts, bytes_out, customer_id_str);
        eviction->get(url, ts, bytes_out, customer_id_str);
        number_of_reads += (size / number_of_bytes_per_read) + 1;
        return true;
    }

    return false;
}

bool Cache::add(string url, unsigned long size, unsigned long ts,  bool penalize_url,
        int customer_id, unsigned long bytes_out, string customer_id_str, string orig_url)
{
    // We should ask the admission policy if we should allow it. If not,
    //  return now, otherwise, put it in the cache
    if (!admission->check(url, bytes_out, size, ts, customer_id_str)) {
        // Didn't have it, don't add it!
        return false;
    } else {
    // Otherwise, go ahead and let it in
        if(store_access_line_and_url)
            eviction->put(url, size, ts, bytes_out, customer_id_str, orig_url);
        else
            eviction->put(url, size, ts, bytes_out, customer_id_str, "NA");
        number_of_writes += (size / number_of_bytes_per_write) + 1;
        return true;
    }
}

void Cache::hourly_purging(unsigned long ts) {
    // If we are set, do it, otherwise skip
    if (do_hourly_purging == true) {
        eviction->hourly_purging(ts);
    }
}

