//
//  zync.c
//  zync
//
//  Created by Ron Vergis on 12/25/12.
//  Copyright (c) 2012 Ron Vergis. All rights reserved.
//

#include "zync.h"
#include "zync-internal.h"

int init_download_file(FILE *download_file, struct zync_state *zs)
{
    return write_zync_state(download_file, zs);
}

int zync_original_file(FILE *original_file, FILE *download_file, zync_file_size_t original_filelen)
{
    if (fseek(download_file, 0, SEEK_SET) != 0)
    {
        return -1;
    }
    struct zync_state *zs = read_zync_file(download_file); //zs+1
    if (zs == NULL)
    {
        return -1;
    }
    
    zync_block_size_t block_size = zs->block_size;
    
    unsigned char *adler32_buf = safe_malloc(sizeof(unsigned char) * block_size); //adler32_buf+1
    if (adler32_buf == NULL)
    {
        return -1;
    }
    if (fseek(download_file, 0, SEEK_SET) != 0)
    {
        free(zs); //zs-1
        free(adler32_buf); //adler32_buf-1
        return -1;
    }
    if (safe_fread(adler32_buf, block_size, original_file) == -1)
    {
        free(zs); //zs-1
        free(adler32_buf); //adler32_buf-1
        return -1;
    }
    zync_rsum_t rolling_adler32 = calc_adler32(adler32_buf, block_size);
    free(adler32_buf); //adler32_buf-1
    
    unsigned char *prev_buf = safe_malloc(sizeof(unsigned char) * block_size); //prev_buf+1
    if (prev_buf == NULL)
    {
        free(zs); //zs-1
        return -1;
    }
    
    unsigned char *next_buf = safe_malloc(sizeof(unsigned char) * block_size); //next_buf+1
    if (next_buf == NULL)
    {
        free(zs); //zs-1
        free(prev_buf); //prev_buf-1
        return -1;
    }
    
    unsigned char prev_ch = 0;
    unsigned char next_ch = 0;
    
    off_t prev_offset = 0;
    off_t next_offset = block_size;
    zync_file_size_t index = 0;
    if (read_rolling_bufs(original_file, prev_buf, next_buf, block_size, prev_offset, next_offset) != 0)
    {
        free(zs); //zs-1
        free(prev_buf); //prev_buf-1
        free(next_buf); //next_buf-1
        return -1;
    }
    
    while (true)
    {
        struct zync_block *unfilled_zb = find_unfilled_zync_block_by_rsum(zs, rolling_adler32);
        if (unfilled_zb != NULL)
        {
            unsigned char *data_buf = safe_malloc(sizeof(unsigned char) * block_size); //data_buf+1
            if (data_buf == NULL)
            {
                free(zs); //zs-1
                free(prev_buf); //prev_buf-1
                free(next_buf); //next_buf-1
                return -1;
            }
            unsigned char *checksum_buf = safe_malloc(sizeof(unsigned char) * 16); //checksum_buf+1
            if (checksum_buf == NULL)
            {
                free(zs); //zs-1
                free(prev_buf); //prev_buf-1
                free(next_buf); //next_buf-1
                free(data_buf); //data_buf-1
                return -1;
            }
            if (fseek(original_file, prev_offset, SEEK_SET) != 0)
            {
                free(zs); //zs-1
                free(prev_buf); //prev_buf-1
                free(next_buf); //next_buf-1
                free(data_buf); //data_buf-1
                free(checksum_buf); //checksum_buf-1
                return -1;
            }
            if (safe_fread(data_buf, block_size, original_file) == -1)
            {
                free(zs); //zs-1
                free(prev_buf); //prev_buf-1
                free(next_buf); //next_buf-1
                free(data_buf); //data_buf-1
                free(checksum_buf); //checksum_buf-1
                return -1;
            }
            calc_md5(data_buf, block_size, checksum_buf);
            if (unfilled_zb != NULL)
            {
                if (unfilled_zb->rsum == rolling_adler32)
                {
                    if (memcmp(unfilled_zb->checksum, checksum_buf, 16) == 0)
                    {
                        unfilled_zb->block_fill_flag = 1;
                        zs->block_fill_count++;
                        
                        if (fseek(download_file, 0, SEEK_SET) != 0)
                        {
                            free(zs); //zs-1
                            free(prev_buf); //prev_buf-1
                            free(next_buf); //next_buf-1
                            free(data_buf); //data_buf-1
                            free(checksum_buf); //checksum_buf-1
                            return -1;
                        }
                        if (update_download_file(download_file, data_buf, zs, unfilled_zb->block_index) == -1)
                        {
                            free(zs); //zs-1
                            free(prev_buf); //prev_buf-1
                            free(next_buf); //next_buf-1
                            free(data_buf); //data_buf-1
                            free(checksum_buf); //checksum_buf-1
                            return -1;
                        }
                    }
                }
            }
            free(data_buf); //data_buf-1
            free(checksum_buf); //checksum_buf-1
        }
        prev_ch = prev_buf[index];
        next_ch = next_buf[index];
        rolling_adler32 = calc_rolling_adler32(rolling_adler32, block_size, prev_ch, next_ch);
        index = (index + 1) % block_size;
        if (index == 0)
        {
            prev_offset += block_size;
            next_offset += block_size;
            
            if (prev_offset >= original_filelen)
            {
                break;
            }
            
            if (read_rolling_bufs(original_file, prev_buf, next_buf, block_size, prev_offset, next_offset) != 0)
            {
                free(zs); //zs-1
                free(prev_buf); //prev_buf-1
                free(next_buf); //next_buf-1
                return -1;
            }
        }
    }
    
    free(zs); //zs-1
    free(prev_buf); //prev_buf-1
    free(next_buf); //next_buf-1
    return 0;
}

int update_download_file(FILE *download_file, unsigned char *buf, struct zync_state *zs, zync_block_index_t block_index)
{
    zync_block_size_t block_size = zs->block_size;
    struct zync_block *zb = find_zync_block_by_index(zs, block_index);
    if (zb == NULL)
    {
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
        return -1;
    }
    if (write_zync_state(download_file, zs) == -1)
    {
        return -1;
    }
    if (fseek(download_file, offset, SEEK_CUR) != 0)
    {
        return -1;
    }
    if (fwrite((void *) buf, block_size, 1, download_file) != 1)
    {
        return -1;
    }
    return 0;
}

int finalize_download_file(FILE *in_file, FILE *out_file)
{
    if (fseek(in_file, 0, SEEK_SET) != 0)
    {
        return -1;
    }
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL); //zs+1
    if (read_zync_state(in_file, zs) == -1)
    {
        free(zs); //zs-1
        return -1;
    }
    off_t offset_start = ftell(in_file);
    if (offset_start == -1)
    {
        free(zs); //zs-1
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
            free(zs); //zs-1
            return -1;
        }
        off_t offset = offset_start + zb->block_fill_id * block_size;
        if (fseek(in_file, offset, SEEK_SET) != 0)
        {
            free(zs); //zs-1
            return -1;
        }
        read_buf_size = MIN(block_size, (zync_block_size_t)(zs->filelen - (offset - offset_start)));
        unsigned char *buf = safe_malloc(sizeof(char) * read_buf_size); //buf+1
        if (buf == NULL)
        {
            free(zs); //zs-1
            return -1;
        }
        zync_block_size_t actual_read_buf_size = (zync_block_size_t) fread((void *) buf, 1, read_buf_size, in_file);
        zync_block_size_t actual_write_buf_size = (zync_block_size_t) fwrite((void *) buf, 1, read_buf_size, out_file);
        if (actual_read_buf_size != actual_write_buf_size)
        {
            free(zs); //zs-1
            free(buf); //buf-1
            return -1;
        }
        bytes_written += actual_write_buf_size;
        free(buf); //buf-1
        actual_write_count++;
        zb = zb->next_block;
    }
    if (expected_write_count != actual_write_count)
    {
        free(zs); //zs-1
        return -1;
    }
    free(zs); //zs-1
    return 0;
}

struct zync_state* init_zync_state(zync_block_size_t block_size,
                                   zync_file_size_t filelen,
                                   zync_block_index_t block_fill_count,
                                   unsigned char * checksum)
{
    struct zync_state *zs = calloc(sizeof *zs, 1); //zs+1
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
    if (fseek(in_file, 0, SEEK_SET) != 0)
    {
        return -1;
    }
    zync_file_size_t in_filelen = 0;
    zync_block_index_t block_id = 0;
    
    unsigned char *zync_checksum = safe_malloc(sizeof(unsigned char) * 16); //zync_checksum+1
    if (zync_checksum == NULL)
    {
        return -1;
    }
    
    CC_MD5_CTX *zync_ctx = init_md5(); //zync_ctx+1
    
    while (block_id < MAX_BLOCKS)
    {
        unsigned char *in_file_buf = safe_malloc(sizeof(unsigned char) * block_size); //in_file_buf+1
        if (in_file_buf == NULL)
        {
            free(zync_checksum); //zync_checksum-1
            free(zync_ctx); //zync_ctx-1
            return -1;
        }
        size_t in_file_read = safe_fread(in_file_buf, block_size, in_file);
        if (in_file_read == -1)
        {
            free(zync_checksum); //zync_checksum-1
            free(zync_ctx); //zync_ctx-1
            free(in_file_buf); //in_file_buf-1
            return -1;
        }
        
        zync_rsum_t adler = (zync_rsum_t) calc_adler32(in_file_buf, block_size);
        
        unsigned char *md5_buf = safe_malloc(sizeof(unsigned char) * 16); //md5_buf+1
        if (md5_buf == NULL)
        {
            free(zync_checksum); //zync_checksum-1
            free(zync_ctx); //zync_ctx-1
            free(in_file_buf); //in_file_buf-1
            return -1;
        }
        
        calc_md5((void *) in_file_buf, block_size, md5_buf);
        add_zync_block(zs, block_id++, 0, 0, adler, md5_buf);
        
        free(md5_buf); //md5_buf-1
        
        update_md5(zync_ctx, (void *) in_file_buf, block_size);
        
        in_filelen += in_file_read;
        
        free(in_file_buf); //in_file_buf-1
        
        if (feof(in_file))
        {
            break;
        }
    }
    
    finalize_md5(zync_ctx, zync_checksum); //zync_ctx-1
    
    for (int i = 0; i < 16; i++)
    {
        zs->checksum[i] = zync_checksum[i];
    }
    
    free(zync_checksum); //zync_checksum-1
    
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
    unsigned char *r_zync_checksum = (unsigned char *) safe_malloc(sizeof(unsigned char) * 16); //r_zync_checksum+1
    if (r_zync_checksum == NULL)
    {
        return -1;
    }
    if (safe_fread((void *) r_zync_checksum, sizeof(unsigned char) * 16, in_file) == -1)
    {
        free(r_zync_checksum); //r_zync_checksum-1
        return -1;
    }
    for (int i = 0; i < 16; i++)
    {
        zs->checksum[i] = r_zync_checksum[i];
    }
    free(r_zync_checksum); //r_zync_checksum-1
    
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
        
        unsigned char *r_checksum = (unsigned char *) safe_malloc(sizeof(unsigned char) * 16); //r_checksum+1
        if (r_checksum == NULL)
        {
            free(r_checksum); //r_checksum-1
            return -1;
        }
        if (fread((void *) r_checksum, sizeof(unsigned char) * 16, 1, in_file) != 1)
        {
            free(r_checksum); //r_checksum-1
            return -1;
        }
        
        add_zync_block(zs, r_block_index, r_block_id, r_block_flag, r_rsum, r_checksum);
        
        free(r_checksum); //r_checksum-1
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
    struct zync_block *zb = (struct zync_block *) safe_malloc(sizeof(struct zync_block)); //zb+1
    if (zb == NULL)
    {
        return NULL;
    }
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
    
    attach_zync_block(zs, zb);
    
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

struct zync_block * find_unfilled_zync_block_by_rsum(struct zync_state *zs, zync_rsum_t rsum)
{
    struct zync_block *zb = zs->start_block;
    while (zb != NULL)
    {
        if (zb->rsum == rsum)
        {
            if (zb->block_fill_flag == 0)
            {
                return zb;
            }
        }
        zb = zb->next_block;
    }
    
    return NULL;
}

#pragma mark zync internal

void attach_zync_block(struct zync_state *zs, struct zync_block *zb)
{
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
}

int read_rolling_bufs(FILE *original_file, unsigned char *prev_buf, unsigned char *next_buf, zync_block_size_t block_size,
                      off_t prev_offset, off_t next_offset)
{
    memset(prev_buf, 0, block_size);
    if (fseek(original_file, prev_offset, SEEK_SET) != 0)
    {
        return -1;
    }
    if (safe_fread(prev_buf, block_size, original_file) == -1)
    {
        return -1;
    }
    memset(next_buf, 0, block_size);
    if (fseek(original_file, next_offset, SEEK_SET) != 0)
    {
        return -1;
    }
    if (safe_fread(next_buf, block_size, original_file) == -1)
    {
        return -1;
    }
    return 0;
}

unsigned char *safe_malloc(zync_block_size_t block_size)
{
    unsigned char *buf = malloc(sizeof(unsigned char) * block_size); //buf+1
    if (buf == NULL)
    {
        return NULL;
    }
    memset(buf, 0, block_size);
    return buf;
}

struct zync_state *read_zync_file(FILE *in_file)
{
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL); //zs+1
    if (read_zync_state(in_file, zs) == -1)
    {
        free(zs); //zs-1
        return NULL;
    }
    return zs;
}

int safe_fread(unsigned char *buf, size_t size, FILE *stream)
{
    int read_bytes = fread(buf, 1, size, stream);
    if (ferror(stream))
    {
        return -1;
    }
    return read_bytes;
}
