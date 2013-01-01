//
//  zync-internal.h
//  zync
//
//  Created by Ron Vergis on 1/2/13.
//  Copyright (c) 2013 Ron Vergis. All rights reserved.
//

#ifndef zync_zync_internal_h
#define zync_zync_internal_h

void attach_zync_block(struct zync_state *zs,
                       struct zync_block *zb);

int read_rolling_bufs(FILE *original_file, unsigned char *prev_buf, unsigned char *next_buf, zync_block_size_t block_size,
                      off_t prev_offset, off_t next_offset);

struct zync_state *read_zync_file(FILE *in_file);

unsigned char *safe_malloc(zync_block_size_t block_size);

int safe_fread(unsigned char *buf, size_t size, FILE *stream);

#endif
