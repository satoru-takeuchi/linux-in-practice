#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <err.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#define PART_SIZE	(1024*1024*1024)
#define ACCESS_SIZE	(64*1024*1024)

static char *progname;

int main(int argc, char *argv[])
{
	progname = argv[0];
	if (argc != 6) {
		fprintf(stderr, "usage: %s <filename> <kernel's help> <r/w> <access pattern> <block size[KB]>\n", progname);
		exit(EXIT_FAILURE);
	}

	char *filename = argv[1];

	bool help;
	if (!strcmp(argv[2], "on")) {
		help = true;
	} else if (!strcmp(argv[2], "off")) {
		help = false;
	} else {
		fprintf(stderr, "kernel's help should be 'on' or 'off': %s\n",
			argv[2]);
		exit(EXIT_FAILURE);
	}

	int write_flag;
	if (!strcmp(argv[3], "r")) {
		write_flag = false;
	} else if (!strcmp(argv[3], "w")) {
		write_flag = true;
	} else {
		fprintf(stderr, "r/w should be 'r' or 'w': %s\n",
			argv[3]);
		exit(EXIT_FAILURE);
	}

	bool random;
	if (!strcmp(argv[4], "seq")) {
		random = false;
	} else if (!strcmp(argv[4], "rand")) {
			random = true;
	} else {
		fprintf(stderr, "access pattern should be 'seq' or 'rand': %s\n",
			argv[4]);
		exit(EXIT_FAILURE);
	}

	int part_size = PART_SIZE;
	int access_size = ACCESS_SIZE;
	
	int block_size = atoi(argv[5]) * 1024;
	if (block_size == 0) {
		fprintf(stderr, "block size should be > 0: %s\n",
			argv[5]);
		exit(EXIT_FAILURE);
	}
	if (access_size % block_size != 0) {
		fprintf(stderr, "access size(%d) should be multiple of block size: %s\n",
			access_size, argv[5]);
		exit(EXIT_FAILURE);
	}
	int maxcount = part_size / block_size;
	int count = access_size / block_size;

	int *offset = malloc(maxcount * sizeof(int));
	if (offset == NULL)
		err(EXIT_FAILURE, "malloc() failed");

	int flag = O_RDWR | O_EXCL;
	if (!help)
		flag |= O_DIRECT;

	int fd;
	fd = open(filename, flag);
	if (fd == -1)
		err(EXIT_FAILURE, "open() failed");

	int i;
	for (i = 0; i < maxcount; i++) {
		offset[i] = i;
	}
	if (random) {
		for (i = 0; i < maxcount; i++) {
			int j = rand() % maxcount;
			int tmp = offset[i];
			offset[i] = offset[j];
			offset[j] = tmp;
		}
	}

	int sector_size;
	if (ioctl(fd, BLKSSZGET, &sector_size) == -1)
		err(EXIT_FAILURE, "ioctl() failed");

	char *buf;
	int e;
	e = posix_memalign((void **)&buf, sector_size, block_size);
	if (e) {
		errno = e;
		err(EXIT_FAILURE, "posix_memalign() failed");
	}

	for (i = 0; i < count; i++) {
		ssize_t ret;
		if (lseek(fd, offset[i] * block_size, SEEK_SET) == -1)
			err(EXIT_FAILURE, "lseek() failed");
		if (write_flag) {
			ret = write(fd, buf, block_size);
			if (ret == -1)
				err(EXIT_FAILURE, "write() failed");
		} else {
			ret = read(fd, buf, block_size);
			if (ret == -1)
				err(EXIT_FAILURE, "read() failed");
		}
	}
	if (fdatasync(fd) == -1)
		err(EXIT_FAILURE, "fdatasync() failed");

	if (close(fd) == -1)
		err(EXIT_FAILURE, "close() failed");

	exit(EXIT_SUCCESS);
}
