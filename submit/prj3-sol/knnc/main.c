#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define KNOWN_FIFO "notify.fifo"

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: ./knnc SERVER_DIR DATA_DIR [N_TESTS]\n");
		exit(EXIT_FAILURE);
	}

	char *fifo = strcat(argv[1], KNOWN_FIFO);
	int fd = open(fifo, O_RDWR);
	if(fd == -1) {
		perror("ERROR"); exit(EXIT_FAILURE);
	}
	char pid[20];
	sprintf(pid, "%ld", (long)getpid());
	write(fd, pid, sizeof(pid));
	printf("wrote: '%s' to fifo\n", pid);
}
