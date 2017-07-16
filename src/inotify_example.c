#include <dirent.h>
#include <sys/inotify.h>
#include "inotify_example.h"


/* inotify example - reinventing inotyfywait/inotifywatch */


struct llist {
	struct llist *next;
	uint8_t udata[];
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

/* ignore all the errors, just close all the shared resources */
void cleanup(void)
{
	sem_unlink(SEM_INOTIFY);
	shm_unlink(SHM_INOTIFY);
}

/**
 * @brief De-allocate all the list
 *
 * @param head file list head
 *
 * @retval NULL
 */
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


/**
 * @brief Read directory information
 *
 * @param dirname - directory name
 * @param head - head of file list
 *
 * @return the size of all the stat structures and file names
 */
size_t read_dir_data_size(const char *dirname, struct llist **head)
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


/**
 * @brief Debug-print of the file list
 *
 * @param flist head of the file list
 */
void print_list(struct llist *flist)
{
	while(flist) {
		printf("fname = \"%s\",\tsize = \"%dl\"\n",
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

/**
 * @brief Put file data into the shared memory
 *
 * @param shm_addr shared memory address
 * @param flist list of file description structures
 */
void share_flist(void *shm_addr, struct llist *flist)
{
	struct file_data *fdata = NULL;
	while(flist) {
		size_t dlen;
		fdata = (struct file_data*) (flist->udata);
		dlen = fdata->fname_buf_len + sizeof(*fdata);
		memcpy(shm_addr, fdata, dlen);
		shm_addr += dlen;
		flist = flist->next;
	}
}



int main(int argc, char* argv[])
{
	const char *dirname = ".";
	sem_t *sem_shm, *sem_inot;
	int shm_fd;
	size_t	shm_len = 0;
	struct llist *flist = NULL;
	void *shm_addr = NULL;

	cleanup();
	if (argc > 2) {
		usage(*argv);
		return EARGC;
	}
	if (argc == 2) dirname = argv[1];
	if ((sem_inot = sem_open(SEM_INOTIFY, O_CREAT | O_RDWR, 0666, 0))
	    == SEM_FAILED) {
		errx(1, "sem_openi");
	}
	if ((sem_shm = sem_open(SEM_SH_MEM, O_CREAT | O_RDWR, 0666, 1))
	    == SEM_FAILED) {
		errx(1, "sem_open");
	}
	if ((shm_fd = shm_open(SHM_INOTIFY, O_CREAT | O_RDWR | O_TRUNC,
			 S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
		errx(1, "shm_open");
	}
	while (1) {
		shm_len = read_dir_data_size(dirname, &flist) + sizeof(shm_len);
		print_list(flist);
		sem_wait(sem_shm);
		/* resize the shared memory */
		if (ftruncate(shm_fd, shm_len) == -1) {
			cleanup();
			errx(1, "ftruncate");
		}
		shm_addr = mmap(0, shm_len, PROT_READ | PROT_WRITE,
				MAP_SHARED, shm_fd, 0);
		if (shm_addr == MAP_FAILED) {
			errx(1, "mmap");
		}
		*(size_t*)shm_addr = shm_len;
		shm_addr += sizeof(shm_len);
		share_flist(shm_addr, flist);
		munmap(shm_addr, shm_len);
		sem_post(sem_shm);
		sem_post(sem_inot);

		flist = llist_free(flist);
		/* wait for the change in folder */
		wait_inot(dirname);
	}
	cleanup();
	return 0;
}

