//
//  ztool.h
//  zync
//
//  Created by Ron Vergis on 12/29/12.
//  Copyright (c) 2012 Ron Vergis. All rights reserved.
//

#ifndef zync_ztool_h
#define zync_ztool_h

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include "zync.h"

int zmake(char * sourcefile_url, char * targetfile_url, zync_block_size_t block_size);

#endif
