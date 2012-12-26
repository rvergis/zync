//
//  zync.c
//  zync
//
//  Created by Ron Vergis on 12/25/12.
//  Copyright (c) 2012 Ron Vergis. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zync.h"

int init_download_file(FILE *download_file, struct zync_state *zs)
{
    return write_zync_state(download_file, zs);
}

int zync_original_file(FILE *original_file, FILE *download_file, zync_file_size_t original_filelen)
{
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL);
    if (read_zync_state(download_file, zs) == -1)
    {
        free(zs);
        return -1;
    }
    
    off_t start_offset = 0;
    off_t curr_offset = start_offset;
    
    zync_block_size_t block_size = zs->block_size;
    
    if (curr_offset + block_size >= original_filelen)
    {
        free(zs);
        return 0;
    }
    
    if (fseek(original_file, curr_offset, SEEK_SET) == 0)
    {
        free(zs);
        return -1;
    }
    
    unsigned char *block_buf = malloc(sizeof(unsigned char) * block_size);
    if (fread(block_buf, block_size, 1, original_file) != 1)
    {
        free(block_buf);
        free(zs);
        return -1;
    }
    
    unsigned char prev_ch = block_buf[0] & 0xFF;
    unsigned char curr_ch = 0;
    
    int rolling_adler = calc_adler32(block_buf, block_size);
    
    free(block_buf);
    
    unsigned char *single_char_buf = malloc(sizeof(unsigned char) * 1);
    while (fread(single_char_buf, 1, 1, original_file) == 1)
    {        
        curr_ch = single_char_buf[0] & 0xFF;
        rolling_adler = calc_rolling_adler32(curr_ch, block_size, prev_ch, rolling_adler);
        prev_ch = curr_ch;
        
        struct zync_state *unfilled_zs = generate_unfilled_zync_state(zs);
        struct zync_block *unfilled_zb = unfilled_zs->start_block;
        if (unfilled_zb != NULL)
        {
            unsigned char *data_buf = malloc(sizeof(unsigned char) * block_size);
            unsigned char *checksum_buf = malloc(sizeof(unsigned char) * 16);
            if (fread(data_buf, block_size, 1, original_file) != 1)
            {
                free(single_char_buf);
                free(data_buf);
                free(checksum_buf);
                free(unfilled_zs);
                
                return -1;
            }
            calc_md5(data_buf, block_size, checksum_buf);
            while (unfilled_zb != NULL)
            {
                if (unfilled_zb->rsum == rolling_adler)
                {
                    if (memcmp(unfilled_zb->checksum, checksum_buf, 16) == 0)
                    {
                        if (fseek(download_file, 0, SEEK_SET) == 0)
                        {
                            free(single_char_buf);
                            free(data_buf);
                            free(checksum_buf);
                            free(unfilled_zs);
                            free(zs);
                            
                            return -1;
                        }
                        if (update_download_file(download_file, data_buf, unfilled_zb->block_index) == -1)
                        {
                            free(single_char_buf);
                            free(data_buf);
                            free(checksum_buf);
                            free(unfilled_zs);
                            free(zs);
                            
                            return -1;
                        }
                    }
                }
                unfilled_zb = unfilled_zb->next_block;
            }
            free(data_buf);
            free(checksum_buf);
        }
        
        free(unfilled_zs);
        
        curr_offset++;
        if (curr_offset + block_size >= original_filelen)
        {
            break;
        }
        if (fseek(original_file, curr_offset, SEEK_SET) == 0)
        {
            free(single_char_buf);
            free(zs);
            return -1;
        }
    }
    
    if (ferror(original_file))
    {
        free(single_char_buf);
        free(zs);
        return -1;
    }
    
    free(single_char_buf);
    free(zs);
    return 0;
}

int update_download_file(FILE *download_file, unsigned char *buf, zync_block_index_t block_index)
{
    if (fseek(download_file, 0, SEEK_SET) != 0)
    {
        return -1;
    }
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL);
    if (read_zync_state(download_file, zs) == -1)
    {
        free(zs);
        return -1;
    }
    zync_block_size_t block_size = zs->block_size;
    struct zync_block *zb = find_zync_block_by_index(zs, block_index);
    if (zb == NULL)
    {
        free(zs);
        return -1;
    }
    off_t offset = 0;
    if (zb->block_fill_flag == 1)
    {
        offset = zb->block_fill_id * block_size;
    }
    else
    {
        offset = zs->block_fill_count * block_size;
        zb->block_fill_id = zs->block_fill_count;
        zb->block_fill_flag = 1;
        zs->block_fill_count++;
    }
    if (fseek(download_file, 0, SEEK_SET) != 0)
    {
        free(zs);
        return -1;
    }
    if (write_zync_state(download_file, zs) == -1)
    {
        free(zs);
        return -1;
    }
    if (fseek(download_file, offset, SEEK_CUR) != 0)
    {
        free(zs);
        return -1;
    }
    if (fwrite((void *) buf, block_size, 1, download_file) != 1)
    {
        free(zs);
        return -1;
    }
    free(zs);
    return 0;
}

int finalize_download_file(FILE *in_file, FILE *out_file)
{
    if (fseek(in_file, 0, SEEK_SET) != 0)
    {
        return -1;
    }
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL);
    if (read_zync_state(in_file, zs) == -1)
    {
        free(zs);
        return -1;
    }
    off_t offset_start = ftell(in_file);
    if (offset_start == -1)
    {
        free(zs);
        return -1;
    }
    zync_block_size_t block_size = zs->block_size;
    zync_block_size_t read_buf_size = 0;
    struct zync_block *zb = zs->start_block;
    zync_block_index_t expected_write_count = (zs->filelen + block_size - 1) / block_size;
    zync_block_index_t actual_write_count = 0;
    zync_file_size_t bytes_written = 0;
    while (zb != NULL)
    {
        if (zb->block_fill_flag == 0)
        {
            free(zs);
            return -1;
        }
        off_t offset = offset_start + zb->block_fill_id * block_size;
        if (fseek(in_file, offset, SEEK_SET) != 0)
        {
            free(zs);
            return -1;
        }
        read_buf_size = MIN(block_size, (zync_block_size_t)(zs->filelen - (offset - offset_start)));
        char *buf = malloc(sizeof(char) * read_buf_size);
        zync_block_size_t actual_read_buf_size = fread((void *) buf, 1, read_buf_size, in_file);
        zync_block_size_t actual_write_buf_size = fwrite((void *) buf, 1, read_buf_size, out_file);
        if (actual_read_buf_size != actual_write_buf_size)
        {
            free(buf);
            free(zs);
            return -1;
        }
        bytes_written += actual_write_buf_size;
        free(buf);
        actual_write_count++;
        zb = zb->next_block;
    }
    if (expected_write_count != actual_write_count)
    {
        free(zs);
        return -1;
    }
    free(zs);
    return 0;
}

struct zync_state* init_zync_state(zync_block_size_t block_size,
                                   zync_file_size_t filelen,
                                   zync_block_index_t block_fill_count,
                                   unsigned char * checksum)
{
    struct zync_state *zs = calloc(sizeof *zs, 1);
    memset(zs->checksum, 0, sizeof(zs->checksum));
    if (checksum != NULL)
    {
        for (int i = 0; i < 16; i++)
        {
            zs->checksum[i] = checksum[i];
        }
    }
    zs->block_size = block_size;
    zs->block_fill_count = block_fill_count;
    zs->filelen = filelen;
    zs->start_block = NULL;
    return zs;
}

int generate_zync_state(FILE *in_file,
                        struct zync_state *zs,
                        zync_block_size_t block_size)
{
    if (block_size <= 0)
    {
        fprintf(stderr, "block_size <= 0");
        return -1;
    }
    
    zync_file_size_t in_filelen = 0;
    zync_block_index_t block_id = 0;
    
    CC_MD5_CTX *zync_ctx = init_md5();
    unsigned char *zync_checksum = malloc(sizeof(unsigned char) * 16);
    
    while (block_id < MAX_BLOCKS)
    {
        unsigned char *in_file_buf = calloc(block_size, 1);
        size_t in_file_read = fread(in_file_buf, 1, block_size, in_file);
        if (in_file_read != block_size)
        {
            if (ferror(in_file))
            {
                fprintf(stderr, "error reading source file %s", strerror(ferror(in_file)));
                free(in_file_buf);
                free(zync_checksum);
                free(zync_ctx);
                return -1;
            }
        }
        
        zync_rsum_t adler = (zync_rsum_t) calc_adler32(in_file_buf, block_size);
        
        unsigned char *md5_buf = malloc(sizeof(unsigned char) * 16);
        
        calc_md5((void *) in_file_buf, block_size, md5_buf);
        add_zync_block(zs, block_id++, 0, 0, adler, md5_buf);
        
        free(md5_buf);
        
        update_md5(zync_ctx, (void *) in_file_buf, block_size);
        
        in_filelen += in_file_read;
        
        free(in_file_buf);
        
        if (feof(in_file))
        {
            break;
        }
    }
    
    finalize_md5(zync_ctx, zync_checksum);
    
    for (int i = 0; i < 16; i++)
    {
        zs->checksum[i] = zync_checksum[i];
    }
    
    free(zync_checksum);
    
    zs->filelen = in_filelen;
    zs->block_size = block_size;
    zs->block_fill_count = 0;
    
    return 0;
}

int write_zync_state(FILE *out_file,
                     struct zync_state *zs)
{
    unsigned char *r_zync_checksum = zs->checksum;
    if (fwrite((void *) r_zync_checksum, sizeof(unsigned char[16]), 1, out_file) != 1)
    {
        return -1;
    }
    
    zync_file_size_t w_filelen = zs->filelen;
    if (fwrite((void *) &w_filelen, sizeof(zync_file_size_t), 1, out_file) != 1)
    {
        return -1;
    }
    
    zync_block_size_t w_block_size = zs->block_size;
    if (fwrite((void *) &w_block_size, sizeof(zync_block_size_t), 1, out_file) != 1)
    {
        return -1;
    }
    
    zync_block_index_t w_block_count = zs->block_fill_count;
    if (fwrite((void *) &w_block_count, sizeof(zync_block_index_t), 1, out_file) != 1)
    {
        return -1;
    }
    
    struct zync_block *zb = zs->start_block;
    while (zb != NULL)
    {
        zync_block_index_t w_block_index = zb->block_index;
        if (fwrite((void *) &w_block_index, sizeof(zync_block_index_t), 1, out_file) != 1)
        {
            return -1;
        }
        
        zync_block_index_t w_block_id = zb->block_fill_id;
        if (fwrite((void *) &w_block_id, sizeof(zync_block_index_t), 1, out_file) != 1)
        {
            return -1;
        }
        
        zync_block_flag_t w_block_flag = zb->block_fill_flag;
        if (fwrite((void *) &w_block_flag, sizeof(zync_block_flag_t), 1, out_file) != 1)
        {
            return -1;
        }
        
        zync_rsum_t w_rsum = zb->rsum;
        if (fwrite((void *) &w_rsum, sizeof(zync_rsum_t), 1, out_file) != 1)
        {
            return -1;
        }
        
        unsigned char *r_checksum = zb->checksum;
        if (fwrite((void *) r_checksum, sizeof(unsigned char[16]), 1, out_file) != 1)
        {
            return -1;
        }
        zb = zb->next_block;
    }

    return 0;
}

int read_zync_state(FILE *in_file, struct zync_state *zs)
{
    unsigned char *r_zync_checksum = (unsigned char *) malloc(sizeof(unsigned char) * 16);
    if (fread((void *) r_zync_checksum, sizeof(unsigned char) * 16, 1, in_file) != 1)
    {
        free(r_zync_checksum);
        return -1;
    }
    for (int i = 0; i < 16; i++)
    {
        zs->checksum[i] = r_zync_checksum[i];
    }
    free(r_zync_checksum);
    
    zync_file_size_t r_filelen = 0;
    if (fread((void *) &r_filelen, sizeof(zync_file_size_t), 1, in_file) != 1)
    {
        return -1;
    }
    zs->filelen = r_filelen;
    
    zync_block_size_t r_block_size = 0;
    if (fread((void *) &r_block_size, sizeof(zync_block_size_t), 1, in_file) != 1)
    {
        return -1;
    }
    zs->block_size = r_block_size;
    
    if (r_block_size == 0)
    {
        fprintf(stderr, "block size is 0\n");
        return -1;
    }
    
    zync_block_index_t r_block_count = 0;
    if (fread((void *) &r_block_count, sizeof(zync_block_index_t), 1, in_file) != 1)
    {
        return -1;
    }
    zs->block_fill_count = r_block_count;
    
    zync_block_size_t block_size = r_block_size;
    
    zync_block_index_t blocks = (zs->filelen + block_size - 1)/ block_size;
    for (zync_block_index_t i = 0; i < blocks; i++)
    {
        zync_block_index_t r_block_index = 0;
        if (fread((void *) &r_block_index, sizeof(zync_block_index_t), 1, in_file) != 1)
        {
            return -1;
        }
        
        zync_block_index_t r_block_id = 0;
        if (fread((void *) &r_block_id, sizeof(zync_block_index_t), 1, in_file) != 1)
        {
            return -1;
        }
        
        zync_block_flag_t r_block_flag = 0;
        if (fread((void *) &r_block_flag, sizeof(zync_block_flag_t), 1, in_file) != 1)
        {
            return -1;
        }
        
        zync_rsum_t r_rsum = 0;
        if (fread((void *) &r_rsum, sizeof(zync_rsum_t), 1, in_file) != 1)
        {
            return -1;
        }
        
        unsigned char *r_checksum = (unsigned char *) malloc(sizeof(unsigned char) * 16);
        if (fread((void *) r_checksum, sizeof(unsigned char) * 16, 1, in_file) != 1)
        {
            free(r_checksum);
            return -1;
        }
        
        add_zync_block(zs, r_block_index, r_block_id, r_block_flag, r_rsum, r_checksum);
        
        free(r_checksum);
    }
    return 0;
}

struct zync_block* add_zync_block(struct zync_state *zs,
                             zync_block_index_t block_index,
                             zync_block_index_t block_fill_id,
                             zync_block_flag_t  block_fill_flag,
                             zync_rsum_t rsum,
                             unsigned char checksum[16])
{
    struct zync_block *zb = (struct zync_block *) malloc(sizeof(struct zync_block));
    zb->block_index = block_index;
    zb->block_fill_id = block_fill_id;
    zb->block_fill_flag = block_fill_flag;
    zb->rsum = rsum;
    memset(zb->checksum, 0, sizeof(zb->checksum));
    if (checksum != NULL)
    {
        for (int i = 0; i < 16; i++)
        {
            zb->checksum[i] = checksum[i];
        }
    }
    zb->next_block = NULL;
    
    struct zync_block *end_block = zs->start_block;
    if (end_block == NULL)
    {
        zs->start_block = zb;
    }
    else
    {
        while (end_block->next_block != NULL)
        {
            end_block = end_block->next_block;
        }
        end_block->next_block = zb;
    }
    
    return zb;
}

zync_block_index_t count_zync_blocks(struct zync_state *zs)
{
    zync_block_index_t count = 0;
    struct zync_block *zb = zs->start_block;
    while (zb != NULL)
    {
        count++;
        zb = zb->next_block;
    }
    return count;
}

struct zync_block * find_zync_block_by_index(struct zync_state *zs, zync_block_index_t block_index)
{
    struct zync_block *zb = zs->start_block;
    while (zb != NULL)
    {
        if (zb->block_index == block_index)
        {
            return zb;
        }
        zb = zb->next_block;
    }
    return NULL;
}

struct zync_state * generate_unfilled_zync_state(struct zync_state *zs)
{
    struct zync_block *zb = zs->start_block;
    struct zync_state *unfilled_zs = init_zync_state(zs->block_size,
                                                             zs->filelen,
                                                             0,
                                                             zs->checksum);
    while (zb != NULL)
    {
        if (zb->block_fill_flag == 0)
        {
            add_zync_block(unfilled_zs, zb->block_index, zb->block_fill_id, zb->block_fill_flag, zb->rsum, zb->checksum);
        }
        zb = zb->next_block;
    }
    
    return unfilled_zs;
}
