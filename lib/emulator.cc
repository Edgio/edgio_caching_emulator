// Copyright 2018, Oath Inc.
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.

#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <tr1/unordered_map>

#include "status.h"
#include "bloomfilter.h"
#include "em_structs.h"
#include "cache.h"
#include "emulator.h"

using namespace std;


/* set_status_string_and_code()
 *
 * Populates the given ponter with the string and code from the HTTP status code
 */
void set_status_string_and_code(string status_code_full,
        string & status_code_string, int & status_code_number) {

    replace(status_code_full.begin(), status_code_full.end(), '/', ' ');// replace slash with space

    string buf; // Have a buffer string
    stringstream ss(status_code_full); // Insert the string into a stream
    vector<string> tokens; // Create vector to hold our words
    while (ss >> buf) {
        tokens.push_back(buf);
    }

    status_code_string = tokens.at(0);
    status_code_number = atoi(tokens.at(1).c_str());// 1 = status code number (e.g. 200, 206, etc.)
}

/*
 * Make a cache key from the URL with the query string stripped off
 */
string url_cachekey(string url,
                    bool modify_cache_key) {

    string cachekey = url.substr(0, url.find('?'));

    return cachekey;
}

string url_cachekey_partial(string url, string full_line) {

    string byte_range;

    // Ge the base part of the url
    string cachekey = url.substr(0, url.find('?'));

    // Also get the byte range, if exists

    int start = full_line.find("bytes ") + 6;
    int end = full_line.find("\" :ECVOLATILE:") - 1;
    int len = end - start;

    byte_range = full_line.substr(start, len);

    // Ok we dug that out. Let's get piece for the key, piece for the size
    byte_range = byte_range.substr(0, byte_range.find("/"));

    cachekey = cachekey + byte_range;

    return cachekey;
}

string return_status_code_number(string status_code) {
    replace(status_code.begin(), status_code.end(), '/', ' ');// replace slash with space

    string buf; // Have a buffer string
    stringstream ss(status_code); // Insert the string into a stream
    vector<string> tokens; // Create vector to hold our words

    while (ss >> buf)
        tokens.push_back(buf);

    return tokens.at(1);	// 1 = status code number (e.g. 200, 206, etc.)
}

/*
 * Here we modify the cache key if necessary
 * That is, /var/... to /EC_Cache...
 */
void modify_cachekey(string & cachekey, string access_log_entry) {
    string srvtype = "cache";
    string protocol = "http";

    // split the cache key (url) from access log
    vector<string> vecSpltStrings;
    istringstream mainString(cachekey);
    string string1;
    while (std::getline(mainString, string1, '/')) {
        vecSpltStrings.push_back(string1);
    }

    if (vecSpltStrings.at(1).compare("var") == 0) {

        // identify the srvtype
        if (vecSpltStrings.at(3).compare("edge") == 0) {
            srvtype = "cache";
        }
        // split the whole access log entry
        vector<string> vecSpltLine;
        istringstream mainString2(access_log_entry);
        string string2;
        while (std::getline(mainString2, string2, ' ')) {
            vecSpltLine.push_back(string2);
        }

        // to get the protocol
        string port_number = vecSpltLine.at(5); // 9 = URL, 5 = port number
        if (port_number.compare("443") == 0) {
            protocol = "https";
        }

        // get the extension of file
        vector<string> vecSpltLine4;
        istringstream mainString4(vecSpltStrings.at(vecSpltStrings.size() - 1));
        string string4;
        while (std::getline(mainString4, string4, '.')) {
            vecSpltLine4.push_back(string4);
        }
        string filename_ext = vecSpltLine4.at(vecSpltLine4.size() - 1);
        string filename = vecSpltStrings.at(vecSpltStrings.size() - 1);

        // Rule 4
        // /EC_Cache/cache/http/80ACDC/ec1/dvrtest/smil::C7B67NATGUSTVNOW/media-u1let6dq9_b2383872_5152.ts:/c.ts
        // /var/EdgeCast/edge/sailfish/www/80ACDC/ec1/dvrtest/smil:C7B67NATGUSTVNOW/media-u1let6dq9_b2383872_5152.ts
        for (std::vector<string>::const_iterator i = vecSpltStrings.begin();
                i != vecSpltStrings.end(); ++i) {
            string str = *i;
            string str_modified = *i;
            std::size_t found = str.find(':');
            if (found != std::string::npos) {
                str_modified.insert(found, 1, ':');
                replace(vecSpltStrings.begin(), vecSpltStrings.end(), str,
                        str_modified);
            }
        }

        // Rule 1
        // ISM and ISML
        bool change_for_ism = 0;
        if ((cachekey.find(".ism/QualityLevels(") != std::string::npos)
                || (cachekey.find(".isml/QualityLevels(") != std::string::npos)) {
            change_for_ism = 1;
            filename.append(":/c");
            vecSpltStrings.pop_back(); // remove last element (file name)
            vecSpltStrings.insert(vecSpltStrings.end(), filename);
        }

        // Rule 2
        // Extension present
        // edit the extension of file (add ":/c." to filename_ext)
        if (vecSpltLine4.size() > 1) {
            if (change_for_ism == 0) {
                filename.append(":/c." + filename_ext);
                vecSpltStrings.pop_back(); // remove last element (file name)
                vecSpltStrings.insert(vecSpltStrings.end(), filename);
            }
        }

        // Rule 3
        // No extension
        if (vecSpltLine4.size() <= 1) {
            if (change_for_ism == 0) {
                filename.append(":/c");
                vecSpltStrings.pop_back(); // remove last element (file name)
                vecSpltStrings.insert(vecSpltStrings.end(), filename);
            }
        }

        // erase and add beginning new elements
        vecSpltStrings.erase(vecSpltStrings.begin(), vecSpltStrings.begin() + 6);
        vecSpltStrings.insert(vecSpltStrings.begin(), protocol);
        vecSpltStrings.insert(vecSpltStrings.begin(), srvtype);
        vecSpltStrings.insert(vecSpltStrings.begin(), "EC_Cache");

        // create a new string by adding "/"
        string cache_path = "";
        for (std::vector<string>::const_iterator i = vecSpltStrings.begin();
                i != vecSpltStrings.end(); ++i) {
            cache_path.append("/" + *i); //std::cout << *i << ' ';
        }
        cachekey = cache_path;
    }
}



/****************************************************
 *
 *
 *          Emulator Main Object!
 *
 *
 ***************************************************/

/* Instantiate everything we need */
Emulator::Emulator(ostream &output_file, bool partial_object,
                     int argc, char *argv[]):output(output_file)
    {
    sci = new EmConfItems();

    // Parse the config lines
    sci->command_line_parser(argc, argv);
    // Dump out the conf items 
    sci->print_em_conf_items();
    // If debug is on, harp a little so we are warned about how much comes out
    if (sci->debug) {
        cerr << "\nDebug mode is on, too much output will be produced.\n";
    }

    // Reporting Variables
    rv_inst = new ReportingVariables();
    rv_inst->reporting_variables();

    // Infinite Cache Reporting Vars
    requested_item_map_hit = 0;
    requested_item_map_miss = 0;
    requested_item_map_hit_bytes = 0;
    requested_item_map_miss_bytes = 0;


    // Go ahead and tag the start time
    start_time = time(0); // current date/time based on current system

    this->partial_object_caching = partial_object;


    tail = NULL;
    head = NULL;

    front_end_mode = false;

    // Set up the cache Status
    csp_inst = new cache_stat_packet();
}

/* Instantiate everything we need */
Emulator::Emulator(string input_config_file, ostream &output_file,
                     bool partial_object):output(output_file)
    {
    sci = new EmConfItems();

    // Parse the config file
    sci->config_file_parser(input_config_file);
    // Dump out the conf items 
    sci->print_em_conf_items();
    // If debug is on, harp a little so we are warned about how much comes out
    if (sci->debug) {
        cerr << "\nDebug mode is on, too much output will be produced.\n";
    }

    // Reporting Variables
    rv_inst = new ReportingVariables();
    rv_inst->reporting_variables();

    // Infinite Cache Reporting Vars
    requested_item_map_hit = 0;
    requested_item_map_miss = 0;
    requested_item_map_hit_bytes = 0;
    requested_item_map_miss_bytes = 0;


    // Go ahead and tag the start time
    start_time = time(0); // current date/time based on current system

    // Output the path of main logs when are using
    output << "emulator_input_log_file_path: " << sci->emulator_input_log_file_path
        << endl;

    this->partial_object_caching = partial_object;

    tail = NULL;
    head = NULL;

}

/* Tear it all down */
Emulator::~Emulator() {
    // Mark the end time and let us know how long it all took
    time_t end_time = time(0); // current date/time based on current system
    string start_time_str = ctime(&start_time); // convert now to string form
    string end_time_str = ctime(&end_time); // convert now to string form

    output << endl 	<< "Start time: " << start_time_str
        << "End time:   " << end_time_str << endl;

    delete sci;
    delete rv_inst;
}

void Emulator::set_front_end_mode() {
    front_end_mode = true;
}

/* Given a new cache object, add it to the tail*/
void Emulator::add_to_tail(Cache* new_cache){

    // If this is the first one, just set it
    if (tail == NULL) {
        // Set the tail, zero out next
        tail = new_cache;
        tail-> set_next(NULL);
        // Set head to tail, we are the only one
        head = tail;
    }

    // Staple it on the end
    tail->set_next(new_cache);
    // Terminate
    new_cache->set_next(NULL);
    // Scoot the tail down
    tail = new_cache;

    return;
}

/* 
 *
 * Process a single log line
 *
 */
int Emulator::process_access_log_line(string log_line) {
    item_packet ip_inst; //Item packet we will use for each iteration
    ip_inst = item_packet();
    ip_inst.line = log_line;

    if (ip_inst.line.length() > 0)
    {
        // split the whole access log entry
        vector<string> vecSpltLine;
        istringstream mainString(ip_inst.line);
        string string_;
        // Spin over the string pushing each piece onto the vector
        while (std::getline(mainString, string_, ' ')) {
            vecSpltLine.push_back(string_);
        }

        // collect access log entry information

        // Grab the time stamp, convert to long
        ip_inst.ts = atol(vecSpltLine.at(0).c_str());

        // Discard events that happened before the end of our cache mgr dump
        if (ip_inst.ts < csp_inst->initial_cache_dump_last_ts) {
            return 0;
        }

        // Make sure it has a valid size. If not, go ahead and skip it 
        // 3 -- size
        // 7 -- total bytes out

        //cout << log_line << endl;

        if (!isdigit(vecSpltLine.at(1).c_str()[0]) || !isdigit(vecSpltLine.at(4).c_str()[0])) {
            return 0; // not valid size OR byte value
        }


        // Save the size and total byes out
        ip_inst.size = atol(vecSpltLine.at(1).c_str());
        ip_inst.bytes_out = atol(vecSpltLine.at(4).c_str());
        // If no size is set, just take size to the the bytes out
        if (ip_inst.size == 0) {
            ip_inst.size = ip_inst.bytes_out; // to compensate for the chunk encoding issue (where size is '-')
        }

        // Get the status code
        ip_inst.status_code_full = vecSpltLine.at(3);

        // Populate status code info
        set_status_string_and_code(ip_inst.status_code_full,
                ip_inst.status_code_string,
                ip_inst.status_code_number);

        /* If we are in FE mode, only check for config_nocache, regular lines
        say NONE */
        if (front_end_mode == true) {
            if (ip_inst.status_code_string.compare("CONFIG_NOCACHE") == 0) {
                cout << "bad cache status:" << ip_inst.status_code_string <<  endl;
                return 0;
            }
        }
        /* otherwise, check for None, it means there was an issue */
        else {
            if (ip_inst.status_code_string.compare("CONFIG_NOCACHE") == 0 || ip_inst.status_code_string.compare("NONE") == 0) {
                return 0;
            }
        }

        // Take the url
        ip_inst.url = vecSpltLine.at(5);
        ip_inst.customer_id = "NA";

        // EMULATOR LOGIC BEGINS
        // This basically catches anyhting that passed data through
        if (ip_inst.url.size()
                && ip_inst.status_code_number >= 200
                && ip_inst.status_code_number <= 400) {

            // Flag to note if we've looked at the line, seems to be for
            // accounting
            bool process_line = true;

            if ((partial_object_caching == true) && (ip_inst.status_code_number == 206)) {
                ip_inst.city64_str = url_cachekey_partial(ip_inst.url,
                                                          ip_inst.line);
                // Set the size to bytes out...about right
                ip_inst.size = ip_inst.bytes_out;
            }
            else {
                ip_inst.city64_str = url_cachekey(ip_inst.url, 1);
            }

            if (process_line) {
                if (sci->debug) {
                    output << "\npopulate_access_log_cache4 " << ip_inst.url;
                    output << "\n" << ip_inst.city64_str << endl << ip_inst.city64_str_unmodified << endl;
                }
                // Counter
                rv_inst->number_of_urls++;

                // Does some work to tokenize the URL
                std::vector<std::string> v; // http://rosettacode.org/wiki/Tokenize_a_string#C.2B.2B
                std::istringstream buf(ip_inst.url);
                for (std::string token; getline(buf, token, '/');) {
                    v.push_back(token);
                }
                if ((v.size() > 3) && (v[3].length() == 6)) {
                    //cout << ip_inst.url << endl;
                    ip_inst.customer_id = v[3].substr(2, 4);
                }
                else {
                    // If no customer ID, take 0
                    ip_inst.customer_id = "0";
                }

                /*processed_urls->initial_put(ip_inst.city64_str,
                  ip_inst.size, ip_inst.ts,
                  ip_inst.bytes_out, ip_inst.customer_id,
                  ip_inst.url, ip_inst.line);*/

                //lookup_update_kc_hdd_cache(sci, rv_inst, &ip_inst, csp_inst, cache_kc_main,
                //        cache_hdd_main, 1, 1, requested_item_map);

                // Handle the infinite cache
                // store statistics for unlimited cache
                string cache_key = ip_inst.city64_str;
                if(requested_item_map[cache_key] == true) {
                    requested_item_map_hit++;
                    requested_item_map_hit_bytes += ip_inst.size;
                } else {
                    requested_item_map_miss++;
                    requested_item_map_miss_bytes += ip_inst.size;
                    requested_item_map[cache_key] = true;
                }

                csp_inst->traffic += ip_inst.size;

                // Call out to the head cache object
                head->process(&ip_inst);
                // Logging stuff
                execute_periodic_functions(&ip_inst);

                return 3;
            }

            else {
                /*unprocessed_urls->initial_put(ip_inst.city64_str,
                  ip_inst.size, ip_inst.ts,
                  ip_inst.bytes_out, ip_inst.customer_id,
                  ip_inst.url, ip_inst.line);*/
                output << "lines_unprocessed " << ip_inst.line
                    << endl;
                return 1;
            }
        } else {
            return 2;
            /*skipped_urls->initial_put(ip_inst.city64_str,
              ip_inst.size, ip_inst.ts,
              ip_inst.bytes_out, ip_inst.customer_id,
              ip_inst.url, ip_inst.line);*/
        }
        // We don't need to do this, it will just write over
        //delete ip_inst;
        //ip_inst = new item_packet();
    } else {
        output << "line.length() <= 0" << endl;
        return 0;
    }

}

/* 
 *
 * Process the log files as a stream
 *
 */
void Emulator::populate_access_log_cache() {
    unsigned long long lines_processed = 0; // access log entry contained a valid cache key
    unsigned long long lines_unprocessed = 0;// access log entry contained NO valid cache key
    unsigned long long lines_skipped = 0;
    item_packet ip_inst; //Item packet we will use for each iteration

    output << "\nBegin reading access logs through pipe...";

    // Master loop which processes line by line action. Basically the whole
    // system is an event replayer and the access logs are just playing 
    // back a series of events, and the majority of the work goes into
    // determining the impacts.
    ip_inst = item_packet();
    string curr_line;
    int ret_val;

    while (getline(cin, curr_line))
    {
        ret_val = process_access_log_line(curr_line);

        if (ret_val == 0) {
            // Not sure...
        } else if (ret_val == 1) {
            lines_unprocessed++;
        } else if (ret_val == 2) {
            lines_skipped++;
        } else if (ret_val == 3) {
            lines_processed++;
        }
    }

    output << "\n\nlines_processed " << lines_processed << " lines_unprocessed "
        << lines_unprocessed
        << " lines_skipped (e.g. different status code)" << lines_skipped
        << endl;
    output << "Reading access logs through pipe complete." << endl;

    output << "Dumping final info..." << endl;
    // DOn't do one final report
    //emulator_periodic_reporting(&ip_inst);
}

//void Emulator::dump_cust_data() {
//    // Go ahead and dump the customer stats if they were asked for
//    if (sci->print_customer_hit_stats && sci->print_customer_hit_stats_per_day)
//    {
//        output << endl << rv_inst->customer_stats.str() << endl;
//    }
//
//}

// Clear all the counters stored here at the emulator
void Emulator::clear_emulator_stats() {
    requested_item_map_hit = 0;
    requested_item_map_miss = 0;
    requested_item_map_hit_bytes = 0;
    requested_item_map_miss_bytes = 0;

    return;
}

void Emulator::emulator_periodic_reporting(item_packet* ip_inst){

    ostringstream outlogfile;
    outlogfile << "\nemulator_periodic_reporting ";
    /* Global Information */
    // Time stamp of the output
    outlogfile << ip_inst->ts << "  ";
    // Total traffic seen in the last timestamp, reset it
    outlogfile << csp_inst->traffic << "  ";
    csp_inst->traffic = 0;
    // Total numer of uRLs in last chunk, reset it
    outlogfile << rv_inst->number_of_urls;
    rv_inst->number_of_urls = 0;

    // Global Hit Rate
    outlogfile << "  |\tghr "; // hit and byte-hit ratio
    // Total Hit Ratio
    // The total hit ratio is given by the number of hits at all layers divided
    // by the number of hits and misses at the first layer
    if ((head->get_hm_local()) > 0) {
        outlogfile << (float) (head->get_hit_total()) / (float) (head->get_hm_local()) << "  ";
    } else {
        outlogfile << 0 << "  ";
    }

    // Total Byte Hit
    if ((head->get_hm_bytes_local()) > 0){
        outlogfile << (float) (head->get_hit_bytes_total()) / (float) (head->get_hm_bytes_local()) << "  ";
    } else {
        outlogfile << 0 << "  ";
    }

    // Stats related to the item map, from perfect caching
    outlogfile << " " << (float) requested_item_map_hit
        / (float) (requested_item_map_hit + requested_item_map_miss) << " ";
    requested_item_map_hit = 0;
    requested_item_map_miss = 0;

    outlogfile << " " << (float) requested_item_map_hit_bytes
        / (float) (requested_item_map_hit_bytes + requested_item_map_miss_bytes) << " ";
    requested_item_map_hit_bytes = 0;
    requested_item_map_miss_bytes = 0;

    /* Get stats from each individual cache */
    Cache* curr_cache;
    curr_cache = head;
    while (curr_cache != NULL){
        curr_cache->periodic_output(ip_inst->ts, outlogfile);
        curr_cache = curr_cache->get_next();
    }

    // Dump this stream to output
    outlogfile << endl;
    output << outlogfile.str();

    return;

}

void Emulator::execute_periodic_functions(item_packet* ip_inst){
    Cache* curr_cache;

    if (ip_inst->ts - rv_inst->timer1 > 1 * 60 * 15) { // 15 minutes
        rv_inst->timer1 = ip_inst->ts;
        emulator_periodic_reporting(ip_inst);
    }

    if (ip_inst->ts - rv_inst->timer2 > 1 * 60 * 60) { // 1 hour
        rv_inst->timer2 = ip_inst->ts;
        // Loop through and hourly purge -- they will ignore if they don't want
        curr_cache = head;
        while (curr_cache != NULL){
            curr_cache->hourly_purging(ip_inst->ts);
            curr_cache = curr_cache->get_next();
        }
    }

}
