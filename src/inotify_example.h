#ifndef __INOTIFY_EXAMPLE_H__
#define __INOTIFY_EXAMPLE_H__
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <err.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#define _GNU_SOURCE
#include <sys/mman.h>


#define EARGC (1)
#define SEM_INOTIFY "/inotify_sem"
#define SHM_INOTIFY "/inotify_shm"
#define INOTIFY_BUF_LEN (sizeof(struct inotify_event) + NAME_MAX + 1)


struct file_data {
	struct stat statbuf;
	size_t fname_buf_len;
	char fname_buf[];
};



#endif /* __INOTIFY_EXAMPLE_H__ */
