//
//  zmake.c
//  zync
//
//  Created by Ron Vergis on 12/28/12.
//  Copyright (c) 2012 Ron Vergis. All rights reserved.
//

#include <stdio.h>
#include "zync.h"
#include <zlib.h>
#include <CommonCrypto/CommonCrypto.h>
#include <sys/stat.h>

int calc_adler32(unsigned char *buf, int len)
{
    uLong adler = 1;
    uLong val = adler32(adler, buf, len);
    return (int) val;
}

int calc_rolling_adler32(unsigned char current_ch, int len, unsigned char previous_ch, int rolling_adler)
{
    uint32_t a = current_ch & 0xFF - previous_ch & 0xFF;
    uint32_t b = a * len;
    rolling_adler = rolling_adler + a + (b << 16);
    
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
