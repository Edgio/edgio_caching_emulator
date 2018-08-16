// Copyright 2018, Oath Inc.
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.
/*
 * Null Cache Admission Policy
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <string>
#include <sstream>
#include "cache_policy.h"
#include "null_admission.h"

using namespace std;

NullAdmission::NullAdmission() {
    name = "null";
}

NullAdmission::~NullAdmission() {
}

// Should we let this in?
bool NullAdmission::check(string key, unsigned long data, unsigned long long size,
                          unsigned long ts, string customer_id_str) {
    // Admit everything
    return true;
}

float NullAdmission::get_fill_percentage() {
    return 0;
}

void NullAdmission::periodic_output(unsigned long ts, std::ostringstream& outlogfile){
    // Just output the marker and move on
    outlogfile << " : " << name << " ";
}
