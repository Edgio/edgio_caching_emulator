// Copyright 2018, Oath Inc.
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/*
 * em_conf_struct.c
 *
 *  Created on: 2 May 2017
 *      Author: mflores
 */


/*
 * To hold the various structs used by the Caching Emulator
 *
 */

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <unistd.h>

#include "em_structs.h"

using namespace std;

/*
 * Sample output (1 line):
 * 		customer_hit_stats 72C8 hits 64  hits_and_misses 155
 * 			bytes_hit 2766443  bytes_hit_and_miss 11050683
 * 			hit_ratio 0.412903 byte_hit_ratio 0.250341
 */
void ReportingVariables::dump_customer_stats() {
	typedef std::unordered_map<string, unsigned long> inner_map;
	typedef std::unordered_map<string, inner_map> outer_map;

	int total_customers = 0; float hit_ratio_sum = 0; float byte_hit_ratio_sum = 0;
	customer_stats.clear();
	customer_stats.str("");

	for (outer_map::iterator i = customer_hit_stats.begin(), iend = customer_hit_stats.end(); i != iend; ++i)
	{
		++total_customers;
		inner_map &innerMap = i->second;
		customer_stats << "\ncustomer_hit_stats " << i->first;

		for (inner_map::iterator j = innerMap.begin(), jend = innerMap.end(); j != jend; ++j)
		{
			if (j->first.compare("periodic_hit_ratio") != 0 || j->first.compare("periodic_byte_hit_ratio") != 0) {
				customer_stats << " " << j->first << " " << j->second << " ";
			}
		}

		if (customer_hit_stats[i->first]["hits_and_misses"] > 0) {
			float hit_ratio = (float) customer_hit_stats[i->first]["hits"] / (float) customer_hit_stats[i->first]["hits_and_misses"];
			hit_ratio_sum = hit_ratio_sum + hit_ratio;
			customer_stats << " hit_ratio " << hit_ratio;
		}

		if (customer_hit_stats[i->first]["bytes_hit_and_miss"] > 0) {
			float byte_hit_ratio = (float) customer_hit_stats[i->first]["bytes_hit"] / (float) customer_hit_stats[i->first]["bytes_hit_and_miss"];
			byte_hit_ratio_sum = byte_hit_ratio_sum + byte_hit_ratio;
			customer_stats << " byte_hit_ratio " << byte_hit_ratio;
		}
	}
	customer_stats << "\navg_hit_ratio_per_customer " << hit_ratio_sum / (float) total_customers;
	customer_stats << " avg_byte_hit_ratio_per_customer " << byte_hit_ratio_sum / (float) total_customers << endl;
}


void ReportingVariables::reset_customer_stats() {
	typedef std::unordered_map<string, unsigned long> inner_map;
	typedef std::unordered_map<string, inner_map> outer_map;

	for (outer_map::iterator i = customer_hit_stats.begin(), iend = customer_hit_stats.end(); i != iend; ++i)
	{
		customer_hit_stats[i->first]["hits_and_misses"] = 0;
		customer_hit_stats[i->first]["hits"] = 0;
		customer_hit_stats[i->first]["bytes_hit_and_miss"] = 0;
		customer_hit_stats[i->first]["bytes_hit"] = 0;
	}
}

bool ReportingVariables::check_customer_in_list(string custid, vector<string> m_list) {
	vector<string>::iterator it;
	if(find(m_list.begin(), m_list.end(), custid) != m_list.end())
		return true;
	else
		return false;
}


void ReportingVariables::print_and_reset_monitored_customer_stats(vector<string> m_monitor_customers_list) {
	typedef std::unordered_map<string, unsigned long> inner_map;
	typedef std::unordered_map<string, inner_map> outer_map;

	for (outer_map::iterator i = monitored_customers.begin(), iend = monitored_customers.end(); i != iend; ++i)
	{
		inner_map &innerMap = i->second;
		if(check_customer_in_list(i->first, m_monitor_customers_list)) {
			cout << "\nmonitored_customers_stats " << i->first;
			for (inner_map::iterator j = innerMap.begin(), jend = innerMap.end(); j != jend; ++j)
			{
				cout << " " << j->first << " " << j->second << " ";
				j->second = 0;
			}
		}
	}
}

void ReportingVariables::compute_periodic_stats(bool floor_customer_loss) {
	typedef std::unordered_map<string, unsigned long> inner_map;
	typedef std::unordered_map<string, inner_map> outer_map;

	for (outer_map::iterator i = customer_hit_stats.begin(), iend = customer_hit_stats.end(); i != iend; ++i)
	{
		// i->first = customer ID (e.g. 307A)
		if (customer_hit_stats[i->first]["periodic_hits_and_misses"] > 0) {
			unsigned long periodic_hit_ratio = 100 * (float) customer_hit_stats[i->first]["periodic_hits"]
												   / (float) customer_hit_stats[i->first]["periodic_hits_and_misses"];
			customer_hit_stats[i->first]["periodic_hit_ratio"] = periodic_hit_ratio;
		}

		if (customer_hit_stats[i->first]["periodic_bytes_hit_and_miss"] > 0) {
			unsigned long periodic_byte_hit_ratio = 100 * (float) customer_hit_stats[i->first]["periodic_bytes_hit"]
												   / (float) customer_hit_stats[i->first]["periodic_bytes_hit_and_miss"];

			if (floor_customer_loss == true) {
				int diff = periodic_byte_hit_ratio - customer_hit_stats[i->first]["periodic_byte_hit_ratio"];
				if (diff > 100) {
					cerr << "compute_periodic_stats(): diff > 100" << __LINE__ << " " << __FILE__;
					exit(1);
				}
				else if (diff <= -2 && customer_hit_stats[i->first]["skip_size_based_deletion"] != 1) {
					// the ratio reduced more than 2% since last time. so we need to protect the customer
					customer_hit_stats[i->first]["skip_size_based_deletion"] = 1;
				}
				else if (diff >= 1 && customer_hit_stats[i->first]["skip_size_based_deletion"] != 0) {
					// things got better, so no need to protect them.
					customer_hit_stats[i->first]["skip_size_based_deletion"] = 0;
				}
			}

			customer_hit_stats[i->first]["periodic_byte_hit_ratio"] = periodic_byte_hit_ratio;
		}

		/*if (i->first.compare("04B0") == 0
				|| i->first.compare("326C") == 0)
		{
			cout << endl << i->first
					<< " periodic_hits "<< customer_hit_stats[i->first]["periodic_hits"]
					<< " periodic_hits_and_misses " << customer_hit_stats[i->first]["periodic_hits_and_misses"]
					<< " periodic_hit_ratio " << customer_hit_stats[i->first]["periodic_hit_ratio"]
					<< " periodic_bytes_hit "<< customer_hit_stats[i->first]["periodic_bytes_hit"]
					<< " periodic_bytes_hit_and_miss " << customer_hit_stats[i->first]["periodic_bytes_hit_and_miss"]
					<< " periodic_byte_hit_ratio " << customer_hit_stats[i->first]["periodic_byte_hit_ratio"]
					<< " periodic_hits " << customer_hit_stats[i->first]["periodic_hits"] << endl;
		}*/

		customer_hit_stats[i->first]["periodic_hits"] = 0;
		customer_hit_stats[i->first]["periodic_bytes_hit"] = 0;
		customer_hit_stats[i->first]["periodic_hits_and_misses"] = 0;
		customer_hit_stats[i->first]["periodic_bytes_hit_and_miss"] = 0;
	}
}

/*******************************************************/

EmConfItems::EmConfItems() {

    _NVAL = 1; // 1 = 2nd hit-caching
	_PVAL = 100;

	//kc_gig = 20;
	//hd_gig = 200;

    bf_reset_int = 604800; // 7 days in seconds

	char buff[20];
	time_t now = time(NULL);
	strftime(buff, 20, "%Y-%m-%d_%H.%M.%S", localtime(&now));
	periodic_reporting_logs_path = buff;
	periodic_reporting_logs_path.append(".log");

	periodic_reporting_err_logs_path = buff;
	periodic_reporting_err_logs_path.append(".err.log");

	emulator_input_log_file_path = "";
	cacheMgrDatFile_initial = "";
	cacheMgrDatFile_final = "";
	bloom_filter_file = "";
	log_path_dir = "logs";

    admission_size = 1024 * 1024 * 1024;
    admission_prob = .5;
    hoc_ttl = 0;

    hd_gig = 1000;
    kc_gig = 2;

	second_hit_caching_hd = false;
	second_hit_caching_kc = false;
	disable_kc = false;
	synthetic_test = false;
	print_customer_hit_stats = false;
	base_mode = true;
	print_customer_hit_stats_per_day = false;
	print_hdd_ingress_stats = false;
	print_hdd_egress_stats = false;
	generate_bf_stats = false;

	LRU_ID = 1;
	LRU_list_size = 1000*10;
	regular_purge_interval = 12;
	floor_customer_loss = false;

	enable_probabilistic_bf_add = false;
	w_size = 0;
	w_age = 1;
	read_em_dir = false;

	eviction_formula = 1;
	ef4_y = 1;
	ef4_e = 1;
	lru_interval = 6;

	debug = false;

}

bool EmConfItems::check_customer_in_list(string custid, vector<string> m_list) const{
	vector<string>::iterator it;
	if(find(m_list.begin(), m_list.end(), custid) != m_list.end())
		return true;
	else
		return false;
}

void EmConfItems::print_em_conf_items() {
	time_t cur_time = time(0);
	cout << ctime(&cur_time);

	cout << "\nEmulator configuration: \n";
	cout << left
			<< setw(50) << "_NVAL (1 = 2nd hit-caching)" << setw(50) << _NVAL << endl
			<< setw(50) << "_PVAL " << setw(50) << _PVAL << endl
			<< setw(50) << "kc_gig " << setw(50) << kc_gig << endl
			<< setw(50) << "hd_gig " << setw(50) << hd_gig << endl
			<< setw(50) << "read_em_dir " << setw(50) << read_em_dir << endl

            << setw(50) << "bf_reset_int" << setw(50) << bf_reset_int << endl

			<< setw(50) << "periodic_reporting_logs_path " << setw(50) << periodic_reporting_logs_path << endl
			<< setw(50) << "periodic_reporting_err_logs_path" << setw(50) << periodic_reporting_err_logs_path << endl
			<< setw(50) << "emulator_input_log_file_path" << setw(50) << emulator_input_log_file_path << endl
			<< setw(50) << "cacheMgrDatFile_initial" << setw(50) << cacheMgrDatFile_initial << endl
			<< setw(50) << "cacheMgrDatFile_final" << setw(50) << cacheMgrDatFile_final << endl
			<< setw(50) << "bloom_filter_file" << setw(50) << bloom_filter_file << endl
			<< setw(50) << "log_path_dir" << setw(50) << log_path_dir << endl

            << setw(50) << "admission_size" << setw(50) << admission_size << endl
            << setw(50) << "admission_prob" << setw(50) << admission_prob << endl
            << setw(50) << "hoc_ttl" << setw(50) << hoc_ttl<< endl

			<< setw(50) << "second_hit_caching_hd" << setw(50) << second_hit_caching_hd << endl
			<< setw(50) << "second_hit_caching_kc" << setw(50) << second_hit_caching_kc << endl
			<< setw(50) << "synthetic_test" << setw(50) << synthetic_test << endl
			<< setw(50) << "print_customer_hit_stats" << setw(50) << print_customer_hit_stats << endl
			<< setw(50) << "print_customer_hit_stats_per_day" << setw(50) << print_customer_hit_stats_per_day << endl
			<< setw(50) << "print_hdd_ingress_stats" << setw(50) << print_hdd_ingress_stats << endl
			<< setw(50) << "print_hdd_egress_stats" << setw(50) << print_hdd_egress_stats << endl
			<< setw(50) << "base_mode" << setw(50) << base_mode << endl
			<< setw(50) << "LRU_ID" << setw(50) << LRU_ID << endl
			<< setw(50) << "enable_probabilistic_bf_add" << setw(50) << enable_probabilistic_bf_add << endl
			<< setw(50) << "LRU_list_size" << setw(50) << LRU_list_size << endl
			<< setw(50) << "w_size" << setw(50) << w_size << endl
			<< setw(50) << "w_age " << setw(50) << w_age << endl
			<< setw(50) << "ef4_e " << setw(50) << ef4_e << endl
			<< setw(50) << "ef4_y " << setw(50) << ef4_y << endl
			<< setw(50) << "eviction_formula" << setw(50) << eviction_formula << endl

			<< setw(50) << "regular_purge_interval" << setw(50) << regular_purge_interval << endl
			<< setw(50) << "floor_customer_loss" << setw(50) << floor_customer_loss << endl
			<< setw(50) << "generate_bf_stats" << setw(50) << generate_bf_stats << endl
			<< setw(50) << "lru_interval" << setw(50) << lru_interval << endl
			<< setw(50) << "debug " << setw(50) << debug << endl
	;
	cout << "no_bf_cust (we disable BF for them)" << "\t\t  ";
	for(vector<string>::const_iterator i = no_bf_cust.begin(); i != no_bf_cust.end(); ++i)
		cout << *i << ' ';
	cout << "\nmonitor_customers (monitor them for stats)" << "\t  ";
	for(vector<string>::const_iterator i = monitor_customers_list.begin(); i != monitor_customers_list.end(); ++i)
		cout << *i << ' ';
	cout << "\n\n";
}

void EmConfItems::command_line_parser(int argc, char* argv[]) {

    int c;

    // Let's go ahead and read all that getopt goodness
	while ((c = getopt (argc, argv, "N:S:P:T:H:K:R:")) != -1)
		switch (c)
		{
			case 'N':
				_NVAL = atoi(optarg);
				break;
            case 'S':
                admission_size = atoi(optarg);
                break;
            case 'P':
                admission_prob = atof(optarg);
                break;
            case 'T':
                hoc_ttl = atoi(optarg);
                break;
            case 'H':
                hd_gig = atoi(optarg);
                break;
            case 'K':
                kc_gig = atoi(optarg);
                break;
            case 'R':
                bf_reset_int = atoi(optarg);
                break;
			default:
				abort ();
		}


}

void EmConfItems::config_file_parser(string input_config_file) {
	string line;
	ifstream myfile(input_config_file.c_str());
	if (myfile.is_open()) {
		while (getline(myfile, line)) {
			string buf; // Have a buffer string
			stringstream ss(line); // Insert the string into a stream
			vector<string> tokens; // Create vector to hold our words
			while (ss >> buf) {
				tokens.push_back(buf);
			}

			if (tokens.size() > 1) {
				if (tokens.at(0).substr(0, 1).compare("#") != 0) {
					if(tokens.at(0).compare("LRU_list_size") == 0) {
						LRU_list_size = atoi(tokens.at(1).c_str());
					}

					if(tokens.at(0).compare("LRU_ID") == 0) {
						LRU_ID = atoi(tokens.at(1).c_str());
					}

					if(tokens.at(0).compare("_NVAL") == 0) {
						_NVAL = atoi(tokens.at(1).c_str());
					}

					if(tokens.at(0).compare("_PVAL") == 0) {
						_PVAL = atoi(tokens.at(1).c_str());
					}

					if(tokens.at(0).compare("kc_gig") == 0) {
						kc_gig = atoi(tokens.at(1).c_str());
					}

					if(tokens.at(0).compare("hd_gig") == 0) {
						hd_gig = atoi(tokens.at(1).c_str());
					}

					if(tokens.at(0).compare("lru_interval") == 0) {
						lru_interval = atoi(tokens.at(1).c_str());
					}

					if(tokens.at(0).compare("disable_bf_per_customer") == 0) {
						string str = tokens.at(1);
						istringstream ss(str);
						string _token;
						while(getline(ss, _token, ',')) {
							no_bf_cust.push_back(_token);
						}
					}

					if(tokens.at(0).compare("monitor_customers") == 0) {
						string str = tokens.at(1);
						istringstream ss(str);
						string _token;
						while(getline(ss, _token, ',')) {
							monitor_customers_list.push_back(_token);
						}
					}

					if(tokens.at(0).compare("regular_purge_interval") == 0) {
						regular_purge_interval = atoi(tokens.at(1).c_str());
					}

					if(tokens.at(0).compare("w_size") == 0) {
						w_size = atof(tokens.at(1).c_str());
					}

					if(tokens.at(0).compare("w_age") == 0) {
						w_age = atof(tokens.at(1).c_str());
					}

					if(tokens.at(0).compare("eviction_formula") == 0) {
						eviction_formula = atoi(tokens.at(1).c_str());
					}
					if(tokens.at(0).compare("ef4_y") == 0) {
						ef4_y = atoi(tokens.at(1).c_str());
					}
					if(tokens.at(0).compare("ef4_e") == 0) {
						ef4_e = atof(tokens.at(1).c_str());
					}

					if(tokens.at(0).compare("debug") == 0) {
						int value_ = atoi(tokens.at(1).c_str());
						if (value_ == 1) {
							debug = true;
						}
					}

					if(tokens.at(0).compare("base_mode") == 0) {
						int value_ = atoi(tokens.at(1).c_str());
						if (value_ == 0) {
							base_mode = false;
							cout << "\nbase_mode set to 0, verify log generation in source (todo).\n";
						}
					}

					if(tokens.at(0).compare("floor_customer_loss") == 0) {
						int value_ = atoi(tokens.at(1).c_str());
						if (value_ == 1) {
							floor_customer_loss = true;
						}
					}

					if(tokens.at(0).compare("generate_bf_stats") == 0) {
						int value_ = atoi(tokens.at(1).c_str());
						if (value_ == 1) {
							generate_bf_stats = true;
						}
					}

					if(tokens.at(0).compare("print_customer_hit_stats") == 0) {
						int value_ = atoi(tokens.at(1).c_str());
						if (value_ == 1) {
							print_customer_hit_stats = true;
						}
					}

					if(tokens.at(0).compare("second_hit_caching_hd") == 0) {
						int value_ = atoi(tokens.at(1).c_str());
						if (value_ == 1) {
							second_hit_caching_hd = true;
						}
					}

					if(tokens.at(0).compare("synthetic_test") == 0) {
						int value_ = atoi(tokens.at(1).c_str());
						if (value_ == 1) {
							synthetic_test = true;
						}
					}

					if(tokens.at(0).compare("print_customer_hit_stats_per_day") == 0) {
						int value_ = atoi(tokens.at(1).c_str());
						if (value_ == 1) {
							print_customer_hit_stats_per_day = true;
						}
					}

					if(tokens.at(0).compare("print_hdd_ingress_stats") == 0) {
						int value_ = atoi(tokens.at(1).c_str());
						if (value_ == 1) {
							print_hdd_ingress_stats = true;
						}
					}

					if(tokens.at(0).compare("print_hdd_egress_stats") == 0) {
						int value_ = atoi(tokens.at(1).c_str());
						if (value_ == 1) {
							print_hdd_egress_stats = true;
						}
					}

					if(tokens.at(0).compare("read_em_dir") == 0) {
						int value_ = atoi(tokens.at(1).c_str());
						if (value_ == 1) {
							read_em_dir = true;
						}
					}

					if(tokens.at(0).compare("second_hit_caching_kc") == 0) {
						int value_ = atoi(tokens.at(1).c_str());
						if (value_ == 1) {
							second_hit_caching_kc = true;
							cerr << "We don't use 2nd hit caching in Kernel cache (in production). Exiting" << endl;
							exit(1);
						}
					}

					if(tokens.at(0).compare("periodic_reporting_logs_path") == 0) {
						periodic_reporting_logs_path = tokens.at(1);
					}

					if(tokens.at(0).compare("periodic_reporting_err_logs_path") == 0) {
						periodic_reporting_err_logs_path = tokens.at(1);
					}

					if(tokens.at(0).compare("log_path_dir") == 0) {
						log_path_dir = tokens.at(1);
					}

					if(tokens.at(0).compare("emulator_input_log_dir_path") == 0) {
						emulator_input_log_file_path = tokens.at(1);
					}

					if(tokens.at(0).compare("cacheMgrDatFile_initial") == 0) {
						cacheMgrDatFile_initial = tokens.at(1);
					}

					if(tokens.at(0).compare("cacheMgrDatFile_final") == 0) {
						cacheMgrDatFile_final = tokens.at(1);
					}

					if(tokens.at(0).compare("bloom_filter_file") == 0) {
						bloom_filter_file = tokens.at(1);
					}
				}
			}
		}
		myfile.close();

		if (true == synthetic_test) {
			disable_kc = true;
			second_hit_caching_hd = false;
		}

		cerr << "\nconfig file parsing complete." << endl;
	}

	else {
		cout << "Unable to open configuration file. Exiting.";
		exit(1);
	}
}

