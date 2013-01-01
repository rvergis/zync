//
//  zync.h
//  zync
//
//  Created by Ron Vergis on 12/25/12.
//  Copyright (c) 2012 Ron Vergis. All rights reserved.
//

#ifndef zync_zync_h
#define zync_zync_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zutil.h"

#define zync_rsum_t uint32_t
#define zync_block_index_t uint32_t
#define zync_block_size_t uint32_t
#define zync_block_flag_t uint8_t
#define zync_file_size_t uint32_t

struct zync_block {
    zync_block_index_t block_index;
    zync_block_index_t block_fill_id;
    zync_block_flag_t  block_fill_flag;
    zync_rsum_t rsum;
    unsigned char checksum[16];
    struct zync_block *next_block;
};

struct zync_state {
    unsigned char checksum[16];
    zync_block_size_t block_size;
    zync_block_index_t block_fill_count;
    zync_file_size_t filelen;
    struct zync_block *start_block;
};

int init_download_file(FILE *download_file,
                       struct zync_state *zs);

int zync_original_file(FILE *original_file,
                       FILE *download_file,
                       zync_file_size_t original_filelen);

int update_download_file(FILE *download_file,
                         unsigned char *buf,
                         struct zync_state *zs,
                         zync_block_index_t block_index);

int finalize_download_file(FILE *in_file,
                           FILE *out_file);


int write_zync_state(FILE *out_file,
                     struct zync_state *zs);

int read_zync_state(FILE *in_file,
                    struct zync_state *zs);

struct zync_state* init_zync_state(zync_block_size_t block_size,
                                   zync_file_size_t filelen,
                                   zync_block_index_t block_fill_count,
                                   unsigned char * checksum);

int generate_zync_state(FILE *in_file,
                        struct zync_state *zs,
                        zync_block_size_t block_size);

struct zync_block* add_zync_block(struct zync_state *zs,
                                  zync_block_index_t block_index,
                                  zync_block_index_t block_fill_id,
                                  zync_block_flag_t  block_fill_flag,
                                  zync_rsum_t rsum,
                                  unsigned char checksum[16]);

zync_block_index_t count_zync_blocks(struct zync_state *zs);

struct zync_block * find_zync_block_by_index(struct zync_state *zs,
                                             zync_block_index_t block_index);

struct zync_block * find_unfilled_zync_block_by_rsum(struct zync_state *zs,
                                                     zync_rsum_t rsum);

#endif
