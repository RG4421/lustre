/* Simple test to check that device nodes are correctly created and visible */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define TEST_MINOR 120
#define TEST_MAJOR 25

void usage(char *prog)
{
	fprintf(stderr, "usage: %s <filename>\n", prog);
	exit(1);
}

int main( int argc, char **argv)
{
	char *prog = argv[0];
	char *filename = argv[1];
	int rc;
	struct stat st;
	dev_t device = makedev(TEST_MAJOR, TEST_MINOR);

	if (argc != 2) 
		usage(prog);

	unlink(filename);
	/* First try block devices */
	rc = mknod(filename, 0700 | S_IFBLK, device);
	if ( rc < 0 ) {
		fprintf(stderr, "%s: mknod(%s) failed: rc %d: %s\n",
			prog, filename, errno, strerror(errno));
		return 2;
	}

	rc = stat(filename, &st);
	if ( rc < 0 ) {
		fprintf(stderr, "%s: stat(%s) failed: rc %d: %s\n",
			prog, filename, errno, strerror(errno));
		return 3;
	}
	if ( st.st_rdev != device) {
		fprintf(stderr, "%s: created device other than requested: (%d,%d) instead of (%d,%d)\n", prog, major(st.st_rdev),minor(st.st_rdev),major(device),minor(device));
		return 4;
	}
	if ( ! (st.st_mode | S_IFBLK) ) {
		fprintf(stderr, "%s: created device of different type. Requested block device, got mode %o\n", prog, st.st_mode);
		return 5;
	}

	rc = unlink(filename);
	if ( rc < 0 ) {
		fprintf(stderr, "%s: Cannot unlink created device %s, rc %d: %s\n",
			prog, filename, errno, strerror(errno));
		return 6;
	}

	/* Second try char devices */
	rc = mknod(filename, 0700 | S_IFCHR, device);
	if ( rc < 0 ) {
		fprintf(stderr, "%s: mknod(%s) failed: rc %d: %s\n",
			prog, filename, errno, strerror(errno));
		return 7;
	}

	rc = stat(filename, &st);
	if ( rc < 0 ) {
		fprintf(stderr, "%s: stat(%s) failed: rc %d: %s\n",
			prog, filename, errno, strerror(errno));
		return 8;
	}
	if ( st.st_rdev != device) {
		fprintf(stderr, "%s: created device other than requested: (%d,%d) instead of (%d,%d)\n", prog, major(st.st_rdev),minor(st.st_rdev),major(device),minor(device));
		return 9;
	}
	if ( ! (st.st_mode | S_IFCHR) ) {
		fprintf(stderr, "%s: created device of different type. Requested char device, got mode %o\n", prog, st.st_mode);
		return 10;
	}

	rc = unlink(filename);
	if ( rc < 0 ) {
		fprintf(stderr, "%s: Cannot unlink created device %s, rc %d: %s\n",
			prog, filename, errno, strerror(errno));
		return 11;
	}

	printf("%s: device nodes created correctly\n", prog);

	return 0;
}
