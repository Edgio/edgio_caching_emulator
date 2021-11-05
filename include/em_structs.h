// Copyright 2021 Edgecast Inc
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/*
 * em_conf_struct.h
 *
 *  Created on: Jul 3, 2014
 *      Author: hbedi
 */


/*
 * To hold the various structs used by the Caching Emulator
 *
 */

#ifndef EM_STRUCTS_HH_
#define EM_STRUCTS_HH_


#include <vector>
#include <unordered_map>
#include "status.h"

class ReportingVariables{
    public:
	    // for hit/byte-hit ratios
	    std::unordered_map<std::string,
		std::unordered_map<std::string, unsigned long> > customer_hit_stats;
	    std::unordered_map<std::string,
	    std::unordered_map<std::string, unsigned long> > monitored_customers;
	    unsigned long number_of_urls;
	    unsigned int timer1, timer2, timer3, timer4, timer5;
	    std::ostringstream customer_stats;

	    void reporting_variables() {
	        number_of_urls = 0;
	        timer1 = 0; timer2 = 0; timer3 = 0, timer4 = 0; timer5 = 0;
	    }

	    void dump_customer_stats();
	    void reset_customer_stats();
	    bool check_customer_in_list(std::string custid, std::vector<std::string> m_list);
	    void print_and_reset_monitored_customer_stats(std::vector<std::string> m_monitor_customers_list);
	    void compute_periodic_stats(bool floor_customer_loss);

};

class EmConfItems {
    public:
        EmConfItems();
	    int _NVAL;
	    int _PVAL;

	    unsigned long long kc_gig;
	    unsigned long long hd_gig;

        unsigned long bf_reset_int;

	    std::string periodic_reporting_logs_path;
	    std::string periodic_reporting_err_logs_path;
	    std::string emulator_input_log_file_path;
	    std::string cacheMgrDatFile_initial;
	    std::string cacheMgrDatFile_final;
	    std::string bloom_filter_file;
	    std::string log_path_dir;
	    std::vector<std::string> no_bf_cust;
	    std::vector<std::string> monitor_customers_list;

	    bool second_hit_caching_hd;
	    bool second_hit_caching_kc;
	    bool disable_kc;
	    bool synthetic_test;
	    bool print_customer_hit_stats;
	    bool base_mode;
	    bool enable_probabilistic_bf_add;
	    bool print_customer_hit_stats_per_day;
	    bool print_hdd_ingress_stats;
	    bool print_hdd_egress_stats;
	    bool generate_bf_stats;
	    bool read_em_dir; // emulator input requires uncompressed log directory path
	    bool debug;

	    int LRU_ID;
	    unsigned int LRU_list_size;
	    int regular_purge_interval; // do a regular purge every X hours
	    bool floor_customer_loss; // for adaptive size based LRU

	    // cost-based-lru
	    double w_size;
	    double w_age;

	    int eviction_formula;
	    int ef4_y;
	    float ef4_e;
	    int lru_interval;


        // Admision policy
        int admission_size;
        double admission_prob;

        // Evistion Policy
        int hoc_ttl;

	    bool check_customer_in_list(std::string custid, std::vector<std::string> m_list) const;
	    void print_em_conf_items();
	    void config_file_parser(std::string input_config_file);
	    void command_line_parser(int argc, char *argv[]);

};

#endif /* EM_STRUCTS_HH_ */
