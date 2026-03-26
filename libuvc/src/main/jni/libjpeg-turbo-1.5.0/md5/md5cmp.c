

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "./md5.h"
#include "../tjutil.h"

int main(int argc, char *argv[])
{
	char *md5sum = NULL, buf[65];

	if (argc < 3) {
		fprintf(stderr, "USAGE: %s <correct MD5 sum> <file>\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) != 32)
		fprintf(stderr, "WARNING: MD5 hash size is wrong.\n");

	md5sum = MD5File(argv[2], buf);
	if (!md5sum) {
		perror("Could not obtain MD5 sum");
		return -1;
	}

	if (!strcasecmp(md5sum, argv[1])) {
		fprintf(stderr, "%s: OK\n", argv[2]);
		return 0;
	} else {
		fprintf(stderr, "%s: FAILED.  Checksum is %s\n", argv[2], md5sum);
		return -1;
	}
}
