// Copyright 2018, Oath Inc.
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/*
 * Emmulator.h
 *
 * Hold all the statte for the emulation currently running
 *
 */

#ifndef EMULATOR_H_
#define EMULATOR_H_

class Cache;
class ReportingVariables;
class EmConfItems;

/* Various log parse and mod utils */

void set_status_string_and_code(std::string status_code_full,
                                std::string & status_code_string,
                                int & status_code_number);

std::string return_status_code_number(std::string status_code);

std::string url_cachekey(std::string url,
                         bool modify_cache_key);

std::string url_cachekey_partial(std::string url,
                                 std::string full_line);

std::string return_status_code_number(std::string status_code);


void modify_cachekey(std::string & cachekey, std::string access_log_entry);

/* Main emulator context function */

class Emulator{
    public:
        Emulator(std::string input_config_file, std::ostream &output_file,
                  bool partial_object);
        Emulator(std::ostream &output_file,
                  bool partial_object, int argc, char* argv[]);
        ~Emulator();

        // Reporting Variables
        ReportingVariables* rv_inst;
        // Config items
        EmConfItems* sci;
        // Current Status of Cache (from pre-pop)
        cache_stat_packet* csp_inst;

        // Set to FE mode so it doesn't filter cache status
        void set_front_end_mode();

        // For adding new caches
        void add_to_tail(Cache* new_cache);

        // This is the infinite cache with no evictions. Stored here in case we
        // Want to compare other protocols
        std::unordered_map<std::string, bool> requested_item_map;
        // Counters for infini cache
        unsigned long requested_item_map_hit;
        unsigned long requested_item_map_miss;
        unsigned long long requested_item_map_hit_bytes;
        unsigned long long requested_item_map_miss_bytes;

        void clear_emulator_stats();

        // The first layer of caching
        Cache* head;
        Cache* tail;

        // Emulator Context
        time_t start_time;

        // Functions to Drive the Emulation
        int process_access_log_line(std::string log_line);
        void populate_access_log_cache();

        std::ostream &output;

    private:
        /* Helpers and suchs */
        void emulator_periodic_reporting(item_packet* ip_inst);
        void execute_periodic_functions(item_packet* ip_inst);

        /* Are we using partial object caching */
        bool partial_object_caching;
        bool front_end_mode;

};

#endif /* EMULATOR_H_ */
