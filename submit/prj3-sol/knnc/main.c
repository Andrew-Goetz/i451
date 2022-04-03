#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define KNOWN_FIFO "notify"

void error_handle(void) {
	perror("Error");
	exit(EXIT_FAILURE);
}

void notify_daemon(char *SERVER_DIR, char pid[20]) {
	char *fifo = strncat(SERVER_DIR, KNOWN_FIFO, sizeof(KNOWN_FIFO));
	int fd = open(fifo, O_RDWR);
	if(fd == -1)
		error_handle();
	if(write(fd, pid, sizeof(pid)) == -1)
		error_handle();
	//printf("wrote: '%s' to fifo\n", pid);
	close(fd);
}

void init_private_fifo(char *out_fifo, int *out, int *in) {
	char in_fifo[];
	strncpy(in_fifo, out_fifo, sizeof(out_fifo));
	strncat(in_fifo, "_in", sizeof("_in"));
	strncat(out_fifo, "_out", sizeof("_out"));

	if(access(in_fifo, F_OK) && mkfifo(in_fifo, 0666) == -1)
		error_handle();
	*out = open(out_fifo, O_RDWR);
	if(*out == -1)
		error_handle();

	if(access(out_fifo, F_OK) && mkfifo(out_fifo, 0666) == -1)
		error_handle();
	*in = open(in_fifo, O_RDWR);
	if(*in == -1)
		error_handle();
}

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: ./knnc SERVER_DIR DATA_DIR [N_TESTS]\n");
		exit(EXIT_FAILURE);
	}

	char pid[20];
	sprintf(pid, "%ld", (long)getpid());
	char *server_dir;
	strncpy(server_dir, argv[1], sizeof(argv[1]));

	char *part = strncat(argv[1], pid, sizeof(pid));
	int *out_p; int *in_p;
	init_private_fifo(part, out, in);
	notify_daemon(argv[1], pid);
}
