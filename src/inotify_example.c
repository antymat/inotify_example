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
#include <sys/inotify.h>
#include <limits.h>


#define EARGC (1)
#define SEM_INOTIFY "/inotify_sem"
#define SHM_INOTIFY "/inotify_shm"
#define INOTIFY_BUF_LEN (sizeof(struct inotify_event) + NAME_MAX + 1)
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

struct llist *llist_free(struct llist *head)
{
	struct llist *oldh;
	while (head) {
		oldh = head;
		head = head->next;
		free(oldh);
	}
	return head;
}


int64_t read_dir_data_size(const char *dirname, struct llist **head)
{
	static char fpath[PATH_MAX];
	struct dirent *entry = NULL;
	int64_t shm_len = 0;
	int64_t fname_len_sum = 0;
	int64_t fcnt = 0;
	struct llist *file_stat = NULL;
	DIR *dir=NULL;
	int64_t fname_len = 0;
	assert(head);
	if (!head)
		return 0;
	if (!(dir = opendir(dirname))) {
		cleanup();
		errx(1, "opendir: %s", dirname);
	}
	//rewinddir(dir);
	while (entry = readdir(dir)) {
		struct file_data *fdata = NULL;
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;
		++fcnt;
		fname_len = strlen(entry->d_name) + 2;
		fname_len_sum += fname_len;
		file_stat = malloc(sizeof(*file_stat)
				   + sizeof(struct file_data) + fname_len);
		if(!file_stat) {
			cleanup();
			errx(1, "malloc");
		}
		fdata = (struct file_data*) (file_stat->udata);
		fdata->fname_buf_len = fname_len;
		snprintf(fdata->fname_buf, fname_len, "%s", entry->d_name);
		*head = llist_add(*head, file_stat);
		snprintf(fpath, PATH_MAX, "%s/%s", dirname, entry->d_name);
		/* read the file properties */
		if (stat(fpath, &(fdata->statbuf))) {
			cleanup();
			errx(1, "stat");
		}
	}
	if (closedir(dir)) {
		cleanup();
		errx(1, "closedir");
	}
	shm_len = fname_len_sum + fcnt * sizeof(struct file_data);
	return shm_len;
}


void print_list(struct llist *flist)
{
	while(flist) {
		printf("fname = \"%s\",\tsize = \"%d\"\n",
		       ((struct file_data*)(flist->udata))->fname_buf,
		       ((struct file_data*)(flist->udata))->statbuf.st_size);
		flist = flist->next;
	}
}


/**
 * @brief Wait for any notification on the watched directory
 *
 * @param dirname name of the directory to watch
 */
void wait_inot(const char *dirname)
{
	int inot;
	int watch;
	static char inot_buf[INOTIFY_BUF_LEN];
	int readb;
	if ((inot = inotify_init()) < 0) {
		cleanup();
		errx(1, "inotify_init");
	}
	watch = inotify_add_watch(inot, dirname, IN_ALL_EVENTS);
	if (watch < 0) {
		cleanup();
		errx(1, "inotify_add_watch");
	}
	readb = read(inot, inot_buf, INOTIFY_BUF_LEN);
	if (!readb) {
		cleanup();
		errx(1, "read returned 0");
	}
	inotify_rm_watch(inot, watch);
	close(inot);
}

int main(int argc, char* argv[])
{
	const char *dirname = ".";
	sem_t *sem;
	int shmfd;
	int64_t shm_len = 0;
	struct llist *flist = NULL;
	struct dirent *entry = NULL;

	cleanup();
	if (argc > 2) {
		usage(*argv);
		return EARGC;
	}
	if (argc == 2) dirname = argv[1];
	//if ((sem = sem_open(SEM_INOTIFY, O_CREAT, 0666, 0)) == SEM_FAILED) {
	//	errx(1, "sem_open");
	//}
	//if ((shmfd = shm_open(SHM_INOTIFY, O_CREAT | O_RDWR | O_TRUNC,
	//		 S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
	//	errx(1, "shm_open");
	//}

	//while (1) {
	do {
		shm_len = read_dir_data_size(dirname, &flist);
		print_list(flist);
		flist = llist_free(flist);
		//ftruncate(shmfd, shm_len);
		/* resize the shared memory */
		//while (entry = readdir(dir)) {
		//	if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
		//		continue;
		//	/* read the file names and data */
		//}
		/* close the directory */
		//if (closedir(dir)) {
		//	cleanup();
		//	errx(1, "closedir");
		//}
		//sem_post(sem);
		/* wait for the change in folder */
		wait_inot(dirname);


		/* read all the events - non-blocking */
		//sem_wait(sem);
	//}
	} while(0);
	return 0;
}

