//
//  adler32.h
//  zync
//
//  Created by Ron Vergis on 1/1/13.
//  Copyright (c) 2013 Ron Vergis. All rights reserved.
//

#ifndef zync_adler32_h
#define zync_adler32_h

#include <stdint.h>
#include <sys/types.h>

#define BASE 1 << 16
#define MOD(a) a %= BASE

uint32_t rsync_adler32(unsigned char *buf,
                       uint32_t len);

uint32_t rsync_rolling_adler32(uint32_t rsync_rolling_adler32,
                               uint32_t len,
                               unsigned char old_ch,
                               unsigned char new_ch);

#endif
