// Copyright 2021 Edgio Inc
// Licensed under the terms of the Apache 2.0 open source license
// See LICENSE file for terms.

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
#include "hashfunc.h"


#ifndef BLOOMFILTER_H__
#define BLOOMFILTER_H__

typedef unsigned char* bitmap;
/* gives a 0 if the bit is not set something else otherwise */
#define getbit(n,bmp) ((bmp[(n)>>3])&(0x80>>((n)&0x07)))

/* sets the bit to 1 */
#define setbit(n,bmp) {bmp[(n)>>3]|=(0x80>>((n)&0x07));}

/* computes the size in bytes of a bitmap */
#define bitmapsize(n) (((int)(n)+7)>>3)

/* initializes the bitmap with all zeroes */
#define fillzero(bmp,n) {int internali;for(internali=(((int)(n)-1)>>3);internali>=0;internali--){bmp[internali]=0x00;}}

#define to_uint64(buffer,n) ((uint64_t)*(uint64_t*)(buffer + n))

static const unsigned char bit_set_table_256[256] = 
{
#   define B2(n) n,     n+1,     n+1,     n+2
#   define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
#   define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
    B6(0), B6(1), B6(1), B6(2)
};

struct bloom_filter_stats
{
    unsigned int nfuncs;
    unsigned int size_MB;
    unsigned int flush_period_days;
    unsigned long num_of_set_bits;
    float fill_percentage;
    float theoretical_FPR_percentage;
};

class BloomFilter {

    private:
        bitmap bloomfilter;
        unsigned long bfsize;
        size_t nfuncs;
        static const size_t max_hash = 10;
        int NVAL;	// overall n value for n-hit caching (when =1, then works as 2nd hit caching)
        int CUS_NVAL; // n value for individual customers

#ifdef CBF
        uint8_t * cbf;
        unsigned long int cbf_full_bucket_count;
#endif

    public:
        BloomFilter(const char * BFfilename, size_t _nfuncs, unsigned long _size, int _NVAL){

            FILE *bfFile;
            nfuncs = _nfuncs;
            bfsize = _size;

#ifdef CNVAL
            CUS_NVAL = _NVAL;
            NVAL = 1;	// (when =1, then works as 2nd hit caching)
#else
            CUS_NVAL = _NVAL;
            NVAL = _NVAL;
#endif

            if (_nfuncs>max_hash){
                printf("Number of hash functions (%d) is more than the Max hash functions (%d)! \n", (int)nfuncs , (int)max_hash);
                exit(1);
            }

            //memory allocation
            if(!(bloomfilter=(bitmap) calloc(bitmapsize(bfsize), sizeof(char)))) {
                fprintf(stderr, "Unable to create the bitmap for the bloom filter (calloc error)! \n");
                exit(1);
            }

#ifdef CBF
            cbf = new uint8_t[bfsize]();
            cbf_full_bucket_count = 0;
#endif

            //open file
            if((bfFile = fopen(BFfilename, "rb"))!=NULL){

                // Loading the bloom filter from the file
                if(fread(bloomfilter, bitmapsize(bfsize), 1, bfFile) != 1){
                    printf( "File %s:  read error!\n", BFfilename );
                }
                fclose(bfFile);
            } // else	fprintf(stderr,"Unable to open the bloom-filter file %s or it does not exist! \n", BFfilename);
        }

        unsigned long int count_ones(){
            unsigned long int _count=0;
            for (unsigned char * p = bloomfilter; p != bloomfilter + bitmapsize(bfsize); p++)
                _count +=  bit_set_table_256[*(unsigned char*)p];
            return _count;
        }

        inline void add(char * url){
            for (unsigned int i=0; i<nfuncs; i++)
#ifndef CBF
                setbit(bkdr_hash_64_2_ind(url,i)%bfsize, bloomfilter);
#else
            if(cbf[bkdr_hash_64_2_ind(url,i)%bfsize] < NVAL) {
                cbf[bkdr_hash_64_2_ind(url,i)%bfsize]++;
                if(cbf[bkdr_hash_64_2_ind(url,i)%bfsize] == NVAL)
                    cbf_full_bucket_count++;
            }
#endif
        }

        inline bool check(char * url){
            for (unsigned int i=0; i<nfuncs; i++)
#ifndef CBF
                if (!(getbit(bkdr_hash_64_2_ind(url,i)%bfsize, bloomfilter)))
#else
                    if(cbf[bkdr_hash_64_2_ind(url,i)%bfsize] < NVAL)
#endif
                        return false;
            return true;
        }

#ifdef CNVAL
        inline void c_add(char * url){	// add for a particular customer
            for (unsigned int i=0; i<nfuncs; i++)
#ifndef CBF
                setbit(bkdr_hash_64_2_ind(url,i)%bfsize, bloomfilter);
#else
            if(cbf[bkdr_hash_64_2_ind(url,i)%bfsize] < CUS_NVAL) {
                cbf[bkdr_hash_64_2_ind(url,i)%bfsize]++;
                if(cbf[bkdr_hash_64_2_ind(url,i)%bfsize] == CUS_NVAL)
                    cbf_full_bucket_count++;	// this will be inconsistent now
            }
#endif
        }

        inline bool c_check(char * url){
            for (unsigned int i=0; i<nfuncs; i++)
#ifndef CBF
                if (!(getbit(bkdr_hash_64_2_ind(url,i)%bfsize, bloomfilter)))
#else
                    if(cbf[bkdr_hash_64_2_ind(url,i)%bfsize] < CUS_NVAL)
#endif
                        return false;
            return true;
        }
#endif

        void write_to_disk(std::string m_file_name)
        {
            int bfd, bfdc;
            struct flock lock;
            bitmap bloomfilter_old, bloomfilter_res;

            //memory allocation for old bloom filter
            if (!(bloomfilter_old = (bitmap) calloc(bitmapsize(bfsize), sizeof(char))))
                printf("Unable to create the bitmap for the bloom filter (calloc error)! \n");


            //open file
            if ((bfd = open(m_file_name.c_str(), O_RDWR )) != -1)
            {
                memset (&lock, 0, sizeof(lock));

                //locking
                lock.l_type = F_RDLCK;
                if (fcntl (bfd, F_SETLKW, &lock) != -1)
                {
                    // Loading the bloom filter from the file
                    if (read(bfd, bloomfilter_old, bitmapsize(bfsize)) != bitmapsize(bfsize))
                        printf ("File %s:  read error!\n", m_file_name.c_str() );

                    //unlocking
                    lock.l_type = F_UNLCK;
                    if (fcntl(bfd, F_SETLK, &lock) == -1)
                        printf( "Cannot unlock reading the old  %s!! \n", m_file_name.c_str());

                } else  printf("Cannot lock for reading the old %s. It can be because it is already locked!\n", m_file_name.c_str());

                if (!(bloomfilter_res = (bitmap) calloc(bitmapsize(bfsize), sizeof(char))))
                    printf("Unable to create the bitmap for the bloom filter (calloc error)! \n");


                for (unsigned int i = 0; i < bitmapsize(bfsize)/sizeof(uint64_t); i++)
                {
                    uint64_t or_res = (to_uint64(bloomfilter_old, i * sizeof(uint64_t))) | (to_uint64(bloomfilter, i * sizeof(uint64_t)));
                    memcpy(bloomfilter_res + i * sizeof(uint64_t), &or_res, sizeof(uint64_t));
                }
                lseek(bfd, 0, SEEK_SET);

                //locking
                lock.l_type = F_WRLCK;
                if (fcntl (bfd, F_SETLKW, &lock) != -1)
                {
                    if (write(bfd,bloomfilter_res ,bitmapsize(bfsize)) != bitmapsize(bfsize))
                        printf("File %s : write error!\n", m_file_name.c_str());

                    //unlocking
                    lock.l_type = F_UNLCK;
                    if (fcntl(bfd, F_SETLKW, &lock) == -1)
                        printf( "Cannot unlock writing the old  %s!! \n", m_file_name.c_str());

                } else  printf("Cannot lock for writing %s! It can be because it is already locked!\n", m_file_name.c_str() ) ;
                free(bloomfilter_res);
            }
            else
            {

                printf("Unable to open the file %s ! file is created!\n", m_file_name.c_str());

                if((bfdc = open(m_file_name.c_str(), O_CREAT | O_WRONLY, 0664)) != -1)
                {

                    memset (&lock, 0, sizeof(lock));
                    //locking
                    lock.l_type = F_WRLCK;
                    if (fcntl (bfdc, F_SETLKW, &lock) != -1){
                        if( write(bfdc,bloomfilter ,bitmapsize(bfsize)) != bitmapsize(bfsize))
                            printf("File %s : write error!\n", m_file_name.c_str());

                        //unlocking
                        lock.l_type = F_UNLCK;
                        if (fcntl(bfdc, F_SETLKW, &lock) == -1)
                            printf( "Cannot unlock writing on the new  %s!! \n", m_file_name.c_str());

                    } else printf("Cannot lock for reading %s. It can be because it is already locked!\n", m_file_name.c_str());

                } else printf("Unable to create the file %s ! \n", m_file_name.c_str());

            }
            free(bloomfilter_old);

        }

        inline void flush(){
            memset (bloomfilter, 0, bitmapsize(bfsize));
        }

        inline unsigned int get_size_KB()               {return bfsize/1024/8;}

        ~BloomFilter(){
            free(bloomfilter);
#ifdef CBF
            delete[] cbf;
#endif

        }

        void get_live_stats(struct bloom_filter_stats & stat)
        {
            stat.nfuncs = nfuncs;
            unsigned long int _count=0;
#ifndef CBF
            stat.size_MB = bfsize / 1024 / 1024 / 8;
            for (unsigned char * p = bloomfilter; p != bloomfilter + bitmapsize(bfsize); p++)
                _count += bit_set_table_256[*(unsigned char*)p];
#else
            stat.size_MB = (8*bfsize) / 1024 / 1024 / 8;
            _count = cbf_full_bucket_count;
#endif

            stat.num_of_set_bits = _count;
            stat.fill_percentage = 100.00 * _count / bfsize;
            stat.theoretical_FPR_percentage = 100 * pow (1.00 * _count / bfsize, nfuncs);
        }
};

#endif
