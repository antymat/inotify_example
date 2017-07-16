#include "inotify_example.h"




void usage(const char *name)
{
	fprintf(stderr, "Usage: %s <filename>. Empty <filename> for stdout.",
		name);
}


void print_html_header(FILE *handle)
{
	assert(handle);
	fprintf(handle, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\
		<HTML> <HEAD> <TITLE>A simple inotify example client output</TITLE>\
		</HEAD> <BODY><TABLE Border=\"3\" Cellpadding=\"6\" Cellspacing=\"1\"\
		Align=\"center\"> <TR> <TH>File Name</TH> <TH>File Size</TH>\
		</TR>");
}

void print_html_footer(FILE *handle)
{
	assert(handle);
	fprintf(handle, "</TABLE> </BODY> </HTML>");
}

int main(int argc, char* argv[])
{
	sem_t *sem_shm, *sem_inot;
	int shm_fd;
	size_t	shm_len = 0;
	void *shm_addr = NULL;
	FILE *fout = stdout;

	if (argc > 2) {
		usage(*argv);
		return EARGC;
	}
	if ((sem_shm = sem_open(SEM_SH_MEM, O_RDWR)) == SEM_FAILED) {
		errx(1, "sem_open");
	}
	if ((sem_inot = sem_open(SEM_INOTIFY, O_RDWR)) == SEM_FAILED) {
		errx(1, "sem_open");
	}
	if ((shm_fd = shm_open(SHM_INOTIFY, O_CREAT | O_RDWR,
			 S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
		errx(1, "shm_open");
	}
	if (argc == 2) {
		if (!(fout = fopen(argv[1], "w"))) {
			errx(1, "fopen %s", argv[1]);
		}
	}

	/*while (1)*/ {
		struct file_data *fdata = NULL;
		size_t dlen;
		sem_wait(sem_inot);
		sem_wait(sem_shm);
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
		shm_len -= sizeof(shm_len);
		print_html_header(fout);
		while(shm_len) {
			fdata = (struct file_data*)shm_addr;
			fprintf(fout,"<TR><TD>%s</TD><TD>%dl</TD></TR>",
			       fdata->fname_buf,
			       fdata->statbuf.st_size);
			dlen = fdata->fname_buf_len + sizeof(*fdata);
			shm_len -= dlen;
			shm_addr += dlen;
		}
		print_html_footer(fout);
		munmap(shm_addr, sizeof(shm_len));
		sem_post(sem_shm);
	}
	return 0;
}

