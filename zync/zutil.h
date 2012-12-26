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

#define MAX_BLOCKS 1 << 20

int calc_adler32(unsigned char *buf, int len);

int calc_rolling_adler32(unsigned char current_ch, int len, unsigned char previous_ch, int rolling_adler);

CC_MD5_CTX * init_md5();

void update_md5(CC_MD5_CTX *ctx, const void *data, int len);

void finalize_md5(CC_MD5_CTX *ctx, unsigned char *buf);

void calc_md5(const void *data, int len, unsigned char *buf);

#endif
