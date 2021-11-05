// Copyright 2021 Edgecast Inc
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
#include "size_admission.h"

using namespace std;

/* Size Based Admission */

SizeAdmission::SizeAdmission(unsigned long long threshold) {
    name = "size";
    this->threshold = threshold;
}

SizeAdmission::~SizeAdmission() {
}

// Should we let this in?
bool SizeAdmission::check(string key, unsigned long data, unsigned long long size,
                          unsigned long ts, string customer_id_str) {
    if (size < threshold) {
        return true;
    }

    return false;
}

float SizeAdmission::get_fill_percentage() {
    return 0;
}

void SizeAdmission::periodic_output(unsigned long ts, std::ostringstream& outlogfile){
    // Just output the marker and move on
    outlogfile << " : " << name << " ";
}

/**********************************************************/

/* Prob. Admission */
ProbAdmission::ProbAdmission(double threshold) {
    name = "prob";
    this->prob = threshold;
}

ProbAdmission::~ProbAdmission() {
}

// Should we let this in?
bool ProbAdmission::check(string key, unsigned long data, unsigned long long size,
                          unsigned long ts, string customer_id_str) {

    // Compute the probability of admission 
    double r = 0.0;

    // Flip a coin check the number
    r = ((double) rand() / (RAND_MAX));

    if (r < this->prob) {
        return true;
    }

    return false;
}

float ProbAdmission::get_fill_percentage() {
    return 0;
}

void ProbAdmission::periodic_output(unsigned long ts, std::ostringstream& outlogfile){
    // Just output the marker and move on
    outlogfile << " : " << name << " ";
}

/**********************************************************/


/* Prob. Size Based Admission */
ProbSizeAdmission::ProbSizeAdmission(unsigned long long c) {
    name = "prob_size";
    this->c = c;
}

ProbSizeAdmission::~ProbSizeAdmission() {
}

// Should we let this in?
bool ProbSizeAdmission::check(string key, unsigned long data, unsigned long long size,
                          unsigned long ts, string customer_id_str) {

    // First we need to change the type of the size

    double d_size = (double) size;

    // Compute the probability of admission 
    double prob = 0.0;
    double r = 0.0;

    prob = 1.0 / exp( d_size / (double)c);

    // Flip a coin check the number
    r = ((double) rand() / (RAND_MAX));

    if (r < prob) {
        return true;
    }

    return false;
}

float ProbSizeAdmission::get_fill_percentage() {
    return 0;
}

void ProbSizeAdmission::periodic_output(unsigned long ts, std::ostringstream& outlogfile){
    // Just output the marker and move on
    outlogfile << " : " << name << " ";
}

