//
//  TestZync.m
//  zync
//
//  Created by Ron Vergis on 12/25/12.
//  Copyright (c) 2012 Ron Vergis. All rights reserved.
//

#import <GHUnitIOS/GHUnit.h>
#import "zutil.h"
#import "zync.h"
#import <sys/stat.h>

@interface TestZutil : GHTestCase

@end


@implementation TestZutil

- (void) test_create_zync_state
{
    unsigned char test_buf1[4];
    test_buf1[0] = 'a'; test_buf1[1] = 'b'; test_buf1[2] = 'c'; test_buf1[3] = 'd';
    
    uint32_t adler32_1 = calc_adler32(test_buf1, 4);
    adler32_1 = calc_rolling_adler32(adler32_1, 4, 'a', 'e');
    
    unsigned char test_buf2[4];
    test_buf2[0] = 'b'; test_buf2[1] = 'c'; test_buf2[2] = 'd'; test_buf2[3] = 'e' ;
    uint32_t adler32_2 = calc_adler32(test_buf2, 4);
    
    GHAssertTrue(adler32_1 == adler32_2, NULL);
    
    NSString *nssourcefile_url = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"test_image_server.png"];
    const char *sourcefile_url = [nssourcefile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *in_file = fopen(sourcefile_url, "r");
    
    unsigned char adler32_buf_1[2048];
    if (fseek(in_file, 0, SEEK_SET) != 0)
    {
        GHFail(@"failed seek");
    }
    if (fread(adler32_buf_1, 1, 2048, in_file) != 2048)
    {
        GHFail(@"failed read");
    }
    unsigned char adler32_buf_2[2048];
    if (fseek(in_file, 1, SEEK_SET) != 0)
    {
        GHFail(@"failed seek");
    }
    if (fread(adler32_buf_2, 1, 2048, in_file) != 2048)
    {
        GHFail(@"failed read");
    }
    
    uint32_t start_adler32 = calc_adler32(adler32_buf_1, 2048);
    uint32_t expected_adler32 = calc_adler32(adler32_buf_2, 2048);
    
    unsigned char adler32_buf_3[4096];
    if (fseek(in_file, 0, SEEK_SET) != 0)
    {
        GHFail(@"failed seek");
    }
    if (fread(adler32_buf_3, 1, 4096, in_file) != 4096)
    {
        GHFail(@"failed read");
    }
    
    uint32_t rolling_adler32 = start_adler32;
    unsigned char old_ch = adler32_buf_3[0];
    unsigned char new_ch = adler32_buf_3[2048];
    rolling_adler32 = calc_rolling_adler32(rolling_adler32, 2048, old_ch, new_ch);
    
    GHAssertTrue(rolling_adler32 == expected_adler32, NULL);
    
    fclose(in_file);
}

- (void) test_read_zync_state
{
    NSString *nssourcefile_url = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"test_image_server.png"];
    const char *sourcefile_url = [nssourcefile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *in_file = fopen(sourcefile_url, "r");
    
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL);
    int generate_success = generate_zync_state(in_file, zs, 2048);
    GHAssertTrue(generate_success == 0, NULL);

    zync_rsum_t adler32_block1 = zs->start_block->rsum;
    zync_rsum_t adler32_block2 = zs->start_block->next_block->rsum;
    zync_rsum_t adler32_block3 = zs->start_block->next_block->next_block->rsum;
    
    fseek(in_file, 2048, SEEK_SET);
    
    unsigned char buf[2048];
    fread(buf, 1, 2048, in_file);
    
    zync_rsum_t calc_adler32_block2 = calc_adler32(buf, 2048);
    
    GHAssertTrue(calc_adler32_block2 == adler32_block2, NULL);
    
    zync_rsum_t rolling_adler32_block2 = adler32_block1;
    zync_rsum_t rolling_adler32_block3 = adler32_block1;
    
    fseek(in_file, 0, SEEK_SET);
    unsigned char double_buf[4096];
    fread(double_buf, 1, 4096, in_file);

    fseek(in_file, 0, SEEK_SET);
    unsigned char triple_buf[6144];
    fread(triple_buf, 1, 6144, in_file);

    unsigned char prev_ch = 0;
    unsigned char next_ch = 0;
    for (int i = 0; i < 2048; i++)
    {
        prev_ch = double_buf[i];
        next_ch = double_buf[2048 + i];
        rolling_adler32_block2 = calc_rolling_adler32(rolling_adler32_block2, 2048, prev_ch, next_ch);
    }
    
    prev_ch = 0;
    next_ch = 0;
    for (int i = 0; i < 4096; i++)
    {
        prev_ch = triple_buf[i];
        next_ch = triple_buf[2048 + i];
        rolling_adler32_block3 = calc_rolling_adler32(rolling_adler32_block3, 2048, prev_ch, next_ch);
    }
    
    GHAssertTrue(rolling_adler32_block3 == adler32_block3, NULL);
    
    free(zs);
    fclose(in_file);
}

- (void) test_read_zync_file
{
    NSString *nssourcefile_url = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"test_image_server.png"];
    const char *sourcefile_url = [nssourcefile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *in_file = fopen(sourcefile_url, "r");
    
    NSString *nsdownloadfile_url = [[[[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] objectAtIndex:0] URLByAppendingPathComponent:@"test_image_server.png.download"] path];
    const char *downloadfile_url = [nsdownloadfile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *download_file = fopen(downloadfile_url, "w");
    
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL);
    
    int generate_success = generate_zync_state(in_file, zs, 2048);
    GHAssertTrue(generate_success == 0, NULL);
    
    int write_success = init_download_file(download_file, zs);
    GHAssertTrue(write_success == 0, NULL);
    
    free(zs);
    fflush(download_file);
    fclose(download_file);
    
    download_file = fopen(downloadfile_url, "r");
    struct zync_state *zs2 = init_zync_state(0, 0, 0, NULL);
    int read_success = read_zync_state(download_file, zs2);
    GHAssertTrue(read_success == 0, NULL);
    
    zync_rsum_t adler32_block3 = zs2->start_block->next_block->next_block->rsum;
    
    fseek(in_file, 0, SEEK_SET);
    unsigned char buf[6144];
    fread(buf, 1, 6144, in_file);
    
    fseek(in_file, 0, SEEK_SET);
    unsigned char triple_buf[6144];
    fread(triple_buf, 1, 6144, in_file);
    
    unsigned char prev_ch = 0;
    unsigned char next_ch = 0;
    zync_rsum_t rolling_adler32 = calc_adler32(buf, 2048);
    for (int i = 0; i < 4096; i++)
    {
        prev_ch = triple_buf[i];
        next_ch = triple_buf[2048 + i];
        rolling_adler32 = calc_rolling_adler32(rolling_adler32, 2048, prev_ch, next_ch);
    }
    
    GHAssertTrue(adler32_block3 == rolling_adler32, NULL);
    
    free(zs2);
    fclose(download_file);
    fclose(in_file);
}

@end
