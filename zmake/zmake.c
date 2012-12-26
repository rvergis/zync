#include "ztool.h"
#include "zync.h"
#include "stdlib.h"
#include "stdio.h"
#include "errno.h"

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		fprintf(stderr, "Usage: zmake sourcefile_path zyncfile_path block_size\n");
		return -1;
	}
	char *tail;
	errno = 0;
	zync_block_size_t block_size = (zync_block_size_t) strtol(argv[3], &tail, 0);
	if (errno)
	{
		fprintf(stderr, "block_size Overflow\n");
		return -1;
	}
	char *sourcefile_url = argv[1];
	char *targetfile_url = argv[2];
	int zmake_success = zmake(sourcefile_url, targetfile_url, block_size);
	if (zmake_success < 0)
	{
		fprintf(stderr, "zmake returned %d\n", zmake_success);
	}
	return zmake_success;
}