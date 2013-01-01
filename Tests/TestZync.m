//
//  TestZync.m
//  zync
//
//  Created by Ron Vergis on 12/25/12.
//  Copyright (c) 2012 Ron Vergis. All rights reserved.
//

#import <GHUnitIOS/GHUnit.h>
#import "zync.h"
#import <sys/stat.h>

@interface TestZync : GHTestCase

@end


@implementation TestZync

- (void) test_create_zync_state
{
    off_t filelen = 1000;
    off_t max_block_size = 128;
    
    struct zync_state *zs = init_zync_state(max_block_size, filelen, 0, NULL);
    unsigned char checksum[16];
    for (int i = 0; i < 16; i++) checksum[i] = (unsigned char)i;
    struct zync_block *zb0 = add_zync_block(zs, 7, 0, 0, 7777, checksum);
    struct zync_block *zb1 = add_zync_block(zs, 1, 0, 0, 1111, checksum);
    
    GHAssertTrue(zb0->block_index == 7, NULL);
    GHAssertTrue(zb0->rsum == 7777, NULL);
    for (int i = 0; i < 16; i++)
    {
        GHAssertTrue(zb0->checksum[i] == checksum[i], NULL);
    }
    
    GHAssertTrue(zb1->block_index == 1, NULL);
    GHAssertTrue(zb1->rsum == 1111, NULL);
    for (int i = 0; i < 16; i++)
    {
        GHAssertTrue(zb0->checksum[i] == checksum[i], NULL);
    }
    GHAssertTrue(zb1->next_block == NULL, NULL);
    
    GHAssertTrue(count_zync_blocks(zs) == 2, NULL);
    
    free(zs);
}

- (void) test_write_zync_state
{
    NSString *nssourcefile_url = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"test_image_server.png"];
    const char *sourcefile_url = [nssourcefile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *in_file = fopen(sourcefile_url, "r");
    
    NSString *nstargetfile_url = [[[[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] objectAtIndex:0] URLByAppendingPathComponent:@"test_image_server.png.zync"] path];
    const char *targetfile_url = [nstargetfile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *out_file = fopen(targetfile_url, "w");
    
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL);
    int generate_success = generate_zync_state(in_file, zs, 2048);
    GHAssertTrue(generate_success == 0, NULL);
    
    int write_success = write_zync_state(out_file, zs);
    GHAssertTrue(write_success == 0, NULL);
    
    free(zs);
    fclose(in_file);
    fclose(out_file);
}

- (void) test_read_zync_state
{
    NSString *nssourcefile_url = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"test_image_server.png"];
    const char *sourcefile_url = [nssourcefile_url cStringUsingEncoding:NSASCIIStringEncoding];
    
    struct stat stat_buf_in[1];
    stat(sourcefile_url, stat_buf_in);
    
    int filelen = stat_buf_in->st_size;
    
    NSString *nstargetfile_url = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"test_image_server.png.zync"];
    const char *targetfile_url = [nstargetfile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *file = fopen(targetfile_url, "r");
    
    struct stat stat_buf[1];
    stat(targetfile_url, stat_buf);
    
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL);
    
    int read_success = read_zync_state(file, zs);
    GHAssertTrue(read_success == 0, NULL);
    GHAssertTrue(zs->filelen == filelen, NULL);
    
    GHAssertTrue(zs->block_size == 2048, NULL);
    
    int block_count = count_zync_blocks(zs);
    
    int blocks = (filelen + 2048 - 1) / 2048;
    GHAssertTrue(block_count == blocks, NULL);
    
    free(zs);
    fclose(file);
}

- (void) test_generate_unfilled_zync_state_by_rsum
{
    NSString *nssourcefile_url = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"test_image_server.png"];
    const char *sourcefile_url = [nssourcefile_url cStringUsingEncoding:NSASCIIStringEncoding];
    
    struct stat stat_buf_in[1];
    stat(sourcefile_url, stat_buf_in);
    
    int filelen = stat_buf_in->st_size;
    
    NSString *nstargetfile_url = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"test_image_server.png.zync"];
    const char *targetfile_url = [nstargetfile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *file = fopen(targetfile_url, "r");
    
    struct stat stat_buf[1];
    stat(targetfile_url, stat_buf);
    
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL);
    
    int read_success = read_zync_state(file, zs);
    GHAssertTrue(read_success == 0, NULL);
    GHAssertTrue(zs->filelen == filelen, NULL);
    
    GHAssertTrue(zs->block_size == 2048, NULL);
    
    int block_count = count_zync_blocks(zs);
    
    int blocks = (filelen + 2048 - 1) / 2048;
    GHAssertTrue(block_count == blocks, NULL);
    
    zs->start_block->block_fill_flag = 1;
    zs->block_fill_count++;
    
    struct zync_block *unfilled_zb = find_unfilled_zync_block_by_rsum(zs, zs->start_block->rsum);
    
    GHAssertTrue(unfilled_zb == NULL, NULL);
    
    free(zs);
    fclose(file);
}

- (void) test_init_download_file
{
    NSString *nssourcefile_url = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"test_image_server.png"];
    const char *sourcefile_url = [nssourcefile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *in_file = fopen(sourcefile_url, "r");
    
    NSString *nstargetfile_url = [[[[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] objectAtIndex:0] URLByAppendingPathComponent:@"test_image_server.png.zync"] path];
    const char *targetfile_url = [nstargetfile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *out_file = fopen(targetfile_url, "w");
    
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL);
    int generate_success = generate_zync_state(in_file, zs, 2048);
    GHAssertTrue(generate_success == 0, NULL);
    
    int write_success = write_zync_state(out_file, zs);
    GHAssertTrue(write_success == 0, NULL);
    
    fclose(in_file);
    fclose(out_file);
    
    NSString *nsdownloadfile_url = [[[[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] objectAtIndex:0] URLByAppendingPathComponent:@"test_image_server.png.download"] path];
    const char *downloadfile_url = [nsdownloadfile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *download_file = fopen(downloadfile_url, "w");
    
    int init_download_success = init_download_file(download_file, zs);
    GHAssertTrue(init_download_success == 0, NULL);
    
    fclose(download_file);
    
    FILE *download_file2 = fopen(downloadfile_url, "r");
    
    struct zync_state *zs2 = init_zync_state(0, 0, 0, NULL);
    int read_success = read_zync_state(download_file2, zs2);
    GHAssertTrue(read_success == 0, NULL);
    
    GHAssertTrue(zs->block_fill_count == zs2->block_fill_count, NULL);
    GHAssertTrue(zs->block_size == zs2->block_size, NULL);
    GHAssertTrue(memcmp(zs->checksum, zs2->checksum, 16) == 0, NULL);
    
    free(zs);
    free(zs2);
    fclose(download_file2);

}

- (void) test_update_download_file
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
    
    fclose(in_file);
    fflush(download_file);
    fclose(download_file);
    
    in_file = fopen(sourcefile_url, "r");
    download_file = fopen(downloadfile_url, "r+");
    
    int block_count = (zs->filelen + (zs->block_size - 1)) / (zs->block_size);
    for (int write_block_index = block_count - 1; write_block_index >= 0; write_block_index--)
    {
        int offset = write_block_index * zs->block_size;
        if (fseek(in_file, offset, SEEK_SET) != 0)
        {
            fclose(in_file);
            fclose(download_file);
            GHFail(@"failed seek");
        }
        unsigned char *buf = calloc(zs->block_size, 1);
        fread(buf, zs->block_size, 1, in_file);
        if (update_download_file(download_file, buf, zs, write_block_index) == -1)
        {
            free(buf);
            free(zs);
            fclose(in_file);
            fclose(download_file);
            GHFail(@"failed update");
        }
        free(buf);
    }
    
    free(zs);
    fclose(in_file);
    fclose(download_file);
    
    download_file = fopen(downloadfile_url, "r+");
    struct zync_state *zs2 = init_zync_state(0, 0, 0, NULL);
    int read_success = read_zync_state(download_file, zs2);
    GHAssertTrue(read_success == 0, NULL);
    
    GHAssertTrue(zs2->block_fill_count == block_count, NULL);
    
    fclose(download_file);
    
}

- (void) test_finalize_download_file
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
    
    fclose(in_file);
    fflush(download_file);
    fclose(download_file);
    
    in_file = fopen(sourcefile_url, "r");
    download_file = fopen(downloadfile_url, "r+");
    
    int block_count = (zs->filelen + (zs->block_size - 1)) / (zs->block_size);
    for (int write_block_index = 0; write_block_index < block_count; write_block_index++)
    {
        int offset = write_block_index * zs->block_size;
        if (fseek(in_file, offset, SEEK_SET) != 0)
        {
            fclose(in_file);
            fclose(download_file);
            GHFail(@"failed seek");
        }
        unsigned char *buf = calloc(zs->block_size, 1);
        fread(buf, zs->block_size, 1, in_file);
        if (update_download_file(download_file, buf, zs, write_block_index) == -1)
        {
            free(buf);
            free(zs);
            fclose(in_file);
            fclose(download_file);
            GHFail(@"failed update");
        }
        free(buf);
    }
    
    free(zs);
    fclose(in_file);
    fclose(download_file);
    
    download_file = fopen(downloadfile_url, "r+");    
    
    NSString *nsfinalizedfile_url = [[[[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] objectAtIndex:0] URLByAppendingPathComponent:@"test_image_server.png.finalized"] path];
    const char *finalizedfile_url = [nsfinalizedfile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *finalized_file = fopen(finalizedfile_url, "w");
    
    int finalize_success = finalize_download_file(download_file, finalized_file);
    GHAssertTrue(finalize_success == 0, NULL);
    
    fclose(finalized_file);
    fclose(download_file);
    
    download_file = fopen(downloadfile_url, "r");
    struct zync_state *zs3 = init_zync_state(0, 0, 0, NULL);
    if (read_zync_state(in_file, zs3) != 0)
    {
        fclose(download_file);
    }
    int len = zs3->filelen;
    
    unsigned char *in_file_buf = calloc(len, 1);
    unsigned char *finalized_file_buf = calloc(len, 1);
    
    finalized_file = fopen(finalizedfile_url, "r");
    in_file = fopen(sourcefile_url, "r");
    
    if (fread(in_file_buf, len, 1, in_file) != 1)
    {
        fclose(in_file);
        fclose(finalized_file);
        fclose(download_file);
        GHFail(@"failed read");
    }
    
    if (fread(finalized_file_buf, len, 1, finalized_file) != 1)
    {
        fclose(in_file);
        fclose(finalized_file);
        fclose(download_file);
        GHFail(@"failed read");
    }
    
    GHAssertTrue(memcmp(in_file_buf, finalized_file_buf, len) == 0, NULL);
    
    fclose(in_file);
    fclose(finalized_file);
    fclose(download_file);

}

- (void) test_zync_txt_file
{
    NSString *nssourcefile_url = [[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"test_txt_file.txt"];
    const char *sourcefile_url = [nssourcefile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *in_file = fopen(sourcefile_url, "r");
    
    NSString *nsdownloadfile_url = [[[[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] objectAtIndex:0] URLByAppendingPathComponent:@"test_txt_file.txt.download"] path];
    const char *downloadfile_url = [nsdownloadfile_url cStringUsingEncoding:NSASCIIStringEncoding];
    FILE *download_file = fopen(downloadfile_url, "w");
    
    struct zync_state *zs = init_zync_state(0, 0, 0, NULL);
    int generate_success = generate_zync_state(in_file, zs, 2);
    GHAssertTrue(generate_success == 0, NULL);
    
    int write_success = init_download_file(download_file, zs);
    GHAssertTrue(write_success == 0, NULL);
    
    fclose(in_file);
    fflush(download_file);
    fclose(download_file);
    
    in_file = fopen(sourcefile_url, "r");
    download_file = fopen(downloadfile_url, "r+");
    
    int zync_original_file_status = zync_original_file(in_file, download_file, zs->filelen);
    GHAssertTrue(zync_original_file_status == 0, NULL);
    
    fclose(in_file);
    fclose(download_file);
    
    int block_count = (zs->filelen + (zs->block_size - 1)) / (zs->block_size);
    
    download_file = fopen(downloadfile_url, "r+");
    struct zync_state *zs2 = init_zync_state(0, 0, 0, NULL);
    int read_success = read_zync_state(download_file, zs2);
    GHAssertTrue(read_success == 0, NULL);
    
    GHAssertTrue(zs2->block_fill_count == block_count, NULL);
    
    free(zs2);
    fclose(download_file);
}

- (void) test_zync_png_file
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
    
    fclose(in_file);
    fflush(download_file);
    fclose(download_file);
    
    in_file = fopen(sourcefile_url, "r");
    download_file = fopen(downloadfile_url, "r+");
    
    int zync_original_file_status = zync_original_file(in_file, download_file, zs->filelen);
    GHAssertTrue(zync_original_file_status == 0, NULL);
    
    fclose(in_file);
    fclose(download_file);
    
    int block_count = (zs->filelen + (zs->block_size - 1)) / (zs->block_size);
    
    download_file = fopen(downloadfile_url, "r+");
    struct zync_state *zs2 = init_zync_state(0, 0, 0, NULL);
    int read_success = read_zync_state(download_file, zs2);
    GHAssertTrue(read_success == 0, NULL);
    
    GHAssertTrue(zs2->block_fill_count == block_count, NULL);
    
    free(zs2);
    fclose(download_file);
}

@end
