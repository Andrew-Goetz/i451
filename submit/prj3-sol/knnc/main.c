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
	char *fifo = strcat(SERVER_DIR, KNOWN_FIFO);
	int fd = open(fifo, O_RDWR);
	if(fd == -1)
		error_handle();
	if(write(fd, pid, sizeof(pid)) == -1)
		error_handle();
	//printf("wrote: '%s' to fifo\n", pid);
	close(fd);
}

int init_private_fifo(char *SERVER_DIR, char pid[20]) {
	char *fifo = strcat(SERVER_DIR, pid);
	if(access(fifo, F_OK) && mkfifo(fifo, 0666) == -1)
		error_handle();
	return open(fifo, O_RDWR);
}

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: ./knnc SERVER_DIR DATA_DIR [N_TESTS]\n");
		exit(EXIT_FAILURE);
	}

	char pid[20];
	sprintf(pid, "%ld", (long)getpid());

	int send_images = init_private_fifo(argv[1], pid);
	if(send_images == -1)
		error_handle();
	notify_daemon(argv[1], pid);
}
