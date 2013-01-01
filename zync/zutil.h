//
//  zmake.h
//  zync
//
//  Created by Ron Vergis on 12/28/12.
//  Copyright (c) 2012 Ron Vergis. All rights reserved.
//

#ifndef zync_zmake_h
#define zync_zmake_h

#include <CommonCrypto/CommonCrypto.h>
#include <stdint.h>
#include <sys/types.h>
#include <zlib.h>
#include <sys/stat.h>

#define MAX_BLOCKS 1 << 20

uint32_t calc_adler32(unsigned char *buf, int len);

uint32_t calc_rolling_adler32(uint32_t rolling_adler,
                              uint32_t len,
                              unsigned char old_ch,
                              unsigned char new_ch);

CC_MD5_CTX * init_md5();

void update_md5(CC_MD5_CTX *ctx,
                const void *data,
                int len);

void finalize_md5(CC_MD5_CTX *ctx,
                  unsigned char *buf);

void calc_md5(const void *data,
              int len,
              unsigned char *buf);

#endif
