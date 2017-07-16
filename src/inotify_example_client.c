#include "inotify_example.h"




void usage(const char *name)
{
	fprintf(stderr, "Usage: %s <dirname>\nEmpty <dirname> for '.'\n", name);
}

//sighandler_t set_signal_handler(int sig, sighandler_t handler)
//{
//	struct sigaction new;
//	struct sigaction old;
//	memset(&new, 0, sizeof(new));
//	memset(&old, 0, sizeof(old));
//	new.sa_handler = handler;
//	if (sigaction(SIGINT, &new, &old))
//		errx(1, "sigaction");
//	return old.sa_handler;
//}

/* ignore all the errors, just close all the shared resources */
void cleanup(void)
{
	sem_unlink(SEM_INOTIFY);
	shm_unlink(SHM_INOTIFY);
}








int main(int argc, char* argv[])
{
	sem_t *sem;
	int shm_fd;
	size_t	shm_len = 0;
	void *shm_addr = NULL;

	cleanup();
	if (argc > 1) {
		usage(*argv);
		return EARGC;
	}
	if ((sem = sem_open(SEM_INOTIFY, O_CREAT, 0666, 0)) == SEM_FAILED) {
		errx(1, "sem_open");
	}
	if ((shm_fd = shm_open(SHM_INOTIFY, O_CREAT | O_RDWR | O_TRUNC,
			 S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
		errx(1, "shm_open");
	}
	//while (1) {
	do {
		struct file_data *fdata = NULL;
		size_t dlen;
		sem_wait(sem);
		shm_addr = mmap(0, sizeof(shm_len), PROT_READ | PROT_WRITE,
				MAP_SHARED, shm_fd, 0);
		if (shm_addr == MAP_FAILED) {
			errx(1, "mmap");
		}

		shm_len = *(size_t*)shm_addr;
		munmap(shm_addr, sizeof(shm_len));
		shm_addr = mmap(0, shm_len, PROT_READ | PROT_WRITE,
				MAP_SHARED, shm_fd, 0);
		if (shm_addr == MAP_FAILED) {
			errx(1, "mmap");
		}
		shm_addr += sizeof(shm_len);
		shm_len -+ sizeof(shm_len);
		while(shm_len) {
			fdata = (struct file_data*)shm_addr;
			printf("fname = \"%s\",\tsize = \"%d\"\n",
			       fdata->fname_buf,
			       fdata->statbuf.st_size);
			dlen = fdata->fname_buf_len + sizeof(*fdata);
			shm_len -= dlen;
			shm_addr += dlen;
		}
		munmap(shm_addr, sizeof(shm_len));
		sem_post(sem);
	//}
	} while(0);
	cleanup();
	return 0;
}

