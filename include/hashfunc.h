// Copyright 2018, Oath Inc.
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.

#ifndef _HASHFUNC_H_
#define _HASHFUNC_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BKDR_SEED_SIZE 10

/// The famous DJB hash function for strings
inline uint32_t djb_hash_32(const char *str)
{
    uint32_t hash = 5381;

    for (const char *s = str; *s; s++)
        hash = ((hash << 5) + hash) + *s;

    hash &= ~(1 << 31); /* strip the highest bit */
    return hash;
}

/// Just like djb_hash_32() but hash two strings as if they were concatenated
inline uint32_t djb_hash_32_2(const char *str1, const char *str2)
{
    uint32_t hash = 5381;
    for (const char *s = str1; *s; s++)
        hash = ((hash << 5) + hash) + *s;

    for (const char *s = str2; *s; s++)
        hash = ((hash << 5) + hash) + *s;

    hash &= ~(1 << 31); /* strip the highest bit */
    return hash;
}

/// Just like djb_hash_64_2() but hash two strings as if they were concatenated
inline uint64_t bkdr_hash_64_2(const char* str1, const char* str2)
{
    const uint64_t seed = 131; // 31 131 1313 13131 131313 etc..
    uint64_t hash = 0;

    for (const char* s = str1; *s; ++s)
        hash = (hash * seed) + *s;

    for (const char* s = str2; *s; ++s)
        hash = (hash * seed) + *s;

    return (hash & 0x7fffffffffffffffUL);
}


inline uint64_t bkdr_hash_64_2_rev(const char* str1, const char* str2)
{
    const uint64_t seed = 131; // 31 131 1313 13131 131313 etc..
    uint64_t hash = 0;

    for (unsigned int  s = strlen(str1); s > 0; --s)
        hash = (hash * seed) + str1[s];

    for (unsigned int  s = strlen(str2); s > 0; --s)
        hash = (hash * seed) + str2[s];

    return (hash & 0x7fffffffffffffffUL);
}


inline uint64_t bkdr_hash_64_2_ind(const char* str1, unsigned int ind)
{
    const uint64_t seed[] = {31,  131,  1313,  13131, 131313,  1313131,  13131313, 131313131,  1313131313, 13131313131}; 
    uint64_t hash = 0;

    for (const char* s = str1; *s; ++s)
        hash = (hash * seed[ind]) + *s;


    return (hash & 0x7fffffffffffffffUL);
}


#endif // _HASHFUNC_H_
