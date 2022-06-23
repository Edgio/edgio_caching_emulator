// Copyright 2021 Edgio Inc
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/* Structs needed for managing the emulation status */

#ifndef STATUS_H__
#define STATUS_H__

struct item_packet {
    unsigned long ts;
    unsigned long size;
    unsigned long bytes_out;
    std::string line;
    std::string url;
    std::string city64_str;
    std::string city64_str_unmodified;
    std::string customer_id;
    std::string status_code_full;
    std::string status_code_string;
    int status_code_number;
};

struct cache_stat_packet {
    unsigned long hd_hit;
    unsigned long hd_miss;
    unsigned long kc_hit;
    unsigned long kc_miss;
    unsigned long long traffic, reads_from_origin,
                  kc_byte_hit, kc_byte_miss, hd_byte_hit, hd_byte_miss;
    unsigned long requested_item_map_hit, requested_item_map_miss,
                  requested_item_map_hit_bytes, requested_item_map_miss_bytes;
    unsigned long initial_cache_dump_last_ts; // latest file in initial cache dump

    cache_stat_packet() {
        hd_hit 	= 0;
        hd_miss = 0;
        kc_hit 	= 0;
        kc_miss = 0;
        traffic = 0;
        reads_from_origin = 0;
        kc_byte_hit 	= 0;
        kc_byte_miss 	= 0;
        hd_byte_hit 	= 0;
        hd_byte_miss 	= 0;

        requested_item_map_hit = 0;
        requested_item_map_miss = 0;
        requested_item_map_hit_bytes = 0;
        requested_item_map_miss_bytes = 0;

        initial_cache_dump_last_ts = 0;
    }
};

#endif
