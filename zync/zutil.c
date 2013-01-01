//
//  zmake.c
//  zync
//
//  Created by Ron Vergis on 12/28/12.
//  Copyright (c) 2012 Ron Vergis. All rights reserved.
//

#include <stdio.h>
#include "zync.h"
#include "adler32.h"

uint32_t calc_adler32(unsigned char *buf,
                      int len)
{
    uint32_t val = rsync_adler32(buf, len);
    return (uint32_t) val;
}

uint32_t calc_rolling_adler32(uint32_t rolling_adler,
                              uint32_t len,
                              unsigned char old_ch,
                              unsigned char new_ch)
{
    rolling_adler = rsync_rolling_adler32(rolling_adler, len, old_ch, new_ch);
    return rolling_adler;
}

CC_MD5_CTX * init_md5()
{
    CC_MD5_CTX *ctx = malloc(sizeof(CC_MD5_CTX));
    CC_MD5_Init(ctx);
    return ctx;
}

void update_md5(CC_MD5_CTX *ctx, const void *data, int len)
{
    CC_MD5_Update(ctx, data, len);
}

void finalize_md5(CC_MD5_CTX *ctx, unsigned char *buf)
{
    CC_MD5_Final(buf, ctx);
    free(ctx);
}

void calc_md5(const void *data, int len, unsigned char *buf)
{
    CC_MD5_CTX *ctx = init_md5();
    update_md5(ctx, data, len);
    finalize_md5(ctx, buf);
}
