#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#define EARGC (1)
/* inotify example - reinventing inotyfywait/inotifywatch */

int main(int argc, char* argv[])
{
	const char *dirname = ".";
	if (argc > 1) return EARGC;
	if (argc) dirname = *argv;
	
	
	return 0;
}
