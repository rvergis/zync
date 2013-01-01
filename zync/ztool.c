//
//  ztool.c
//  zync
//
//  Created by Ron Vergis on 12/29/12.
//  Copyright (c) 2012 Ron Vergis. All rights reserved.
//

#include "ztool.h"
#include "zync.h"

int zmake(char * sourcefile_url, char * targetfile_url, zync_block_size_t block_size)
{
    FILE *in_file = fopen(sourcefile_url, "r");
    FILE *out_file = fopen(targetfile_url, "w");
    
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL);
    int generate_success =  generate_zync_state(in_file, zs, block_size);
    if (generate_success < 0)
    {
        free(zs);
        fclose(in_file);
        fclose(out_file);
        return generate_success;
    }
    
    int write_success = write_zync_state(out_file, zs);
    
    free(zs);
    fclose(in_file);
    fflush(out_file);
    fclose(out_file);
    
    return write_success;
}