#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <dirent.h>
#include <err.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>


#define EARGC (1)
#define SEM_INOTIFY "/inotify_sem"
#define SHM_INOTIFY "/inotify_shm"
/* inotify example - reinventing inotyfywait/inotifywatch */


struct llist {
	struct llist *next;
	uint8_t udata[];
};

struct file_data {
	struct stat statbuf;
	uint32_t fname_buf_len;
	char fname_buf[];
};

struct llist* llist_add(struct llist *head, struct llist *new)
{
	new->next = head;
	return new;
}
struct llist* llist_remove(struct llist *head)
{
	return head->next;
}



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



int64_t read_dir_data_size(DIR *dir, struct llist **head)
{
	struct dirent *entry=NULL;
	int64_t shm_len = 0;
	int64_t fname_len=0;
	int64_t fcnt = 0;
	struct llist *file_stat = NULL;
	assert(head);
	if (!head)
		return 0;
	rewinddir(dir);
	while (entry = readdir(dir)) {
		struct file_data *fdata = NULL;
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;
		++fcnt;
		fname_len += strlen(entry->d_name) + 2;
		file_stat = malloc(sizeof(*file_stat)
				   + sizeof(struct file_data) + fname_len);
		if(!file_stat) errx(1, "malloc");
		fdata = (struct file_data*) (file_stat->udata);
		fdata->fname_buf_len = fname_len;
		strncpy(fdata->fname_buf, entry->d_name, fname_len);
		*head = llist_add(*head, file_stat);



		/* read the file properties */
	}
	shm_len = fname_len + fcnt * sizeof(struct file_data);
	return shm_len;
}


void print_list(struct llist *flist)
{
	while(flist) {
		printf("fname = \"%s\"\n", ((struct file_data*)(flist->udata))->fname_buf);
		flist = flist->next;
	}
}

int main(int argc, char* argv[])
{
	const char *dirname = ".";
	sem_t *sem;
	int shmfd;
	int64_t shm_len = 0;
	DIR *dir=NULL;
	struct llist *flist = NULL;
	struct dirent *entry=NULL;

	cleanup();
	if (argc > 2) {
		usage(*argv);
		return EARGC;
	}
	if (argc == 2) dirname = *argv;
	//if ((sem = sem_open(SEM_INOTIFY, O_CREAT, 0666, 0)) == SEM_FAILED) {
	//	errx(1, "sem_open");
	//}
	//if ((shmfd = shm_open(SHM_INOTIFY, O_CREAT | O_RDWR | O_TRUNC,
	//		 S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
	//	errx(1, "shm_open");
	//}

	//while (1) {
	do {
		int64_t fname_len=0;
		int64_t fcnt = 0;
		if (!(dir = opendir(dirname))) {
			cleanup();
			errx(1, "opendir");
		}
		shm_len = read_dir_data_size(dir, &flist);
		print_list(flist);
		//ftruncate(shmfd, shm_len);
		/* resize the shared memory */
		while (entry = readdir(dir)) {
			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
				continue;
			/* read the file names and data */
		}
		/* close the directory */
		if (closedir(dir)) {
			cleanup();
			errx(1, "closedir");
		}
		//sem_post(sem);
		/* wait for the change in folder */
		/* read all the events - non-blocking */
		//sem_wait(sem);
	//}
	} while(0);
	return 0;
}

