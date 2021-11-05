// Copyright 2021 Edgecast Inc
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.

# include <string>
# include "cache_policy.h"

CacheAdmission::~CacheAdmission() {
}

CacheEviction::~CacheEviction() {
}


void CacheEviction::set_total_capacity_by_value(unsigned long long size) {
    total_capacity = size;
}
