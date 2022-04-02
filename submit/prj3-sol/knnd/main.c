#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define KNOWN_FIFO "notify.fifo"

void worker(void) {

	exit(EXIT_SUCCESS);
}

void service(int argc, char *argv[]) {
	if(access(KNOWN_FIFO, F_OK)) {
		if(mkfifo(KNOWN_FIFO, 0666) == -1) {
			perror("ERROR"); exit(EXIT_FAILURE);
		}
	}
	int fd = open(KNOWN_FIFO, O_RDWR);
	if(fd == -1) {
		perror("ERROR"); exit(EXIT_FAILURE);
	}
	char buf[20];
	for(;;) {
		read(fd, buf, sizeof(buf));
		printf("buf:%s\n", buf);
	}
	exit(EXIT_FAILURE);
}
void setup_daemon(int argc, char *argv[]) {
	if(setsid() == -1) {
		perror("ERROR"); exit(EXIT_FAILURE);
	}
	long rc = fork();
	if(rc) {
		printf("Daemon with PID %ld created.\n", rc);
		exit(EXIT_SUCCESS);
	} else if(!rc) {
		if(chdir(argv[1]) == -1) {
			perror("ERROR"); exit(EXIT_FAILURE);
		}
		umask(0);
		service(argc, argv);
	} else {
		perror("ERROR"); exit(EXIT_FAILURE);
	}
	exit(EXIT_FAILURE);
}
int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: ./knnd SERVER_DIR DATA_DIR [K]\n");
		exit(EXIT_FAILURE);
	}
	//service(argc, argv);
	long rc = fork();
	if(rc) {
		//printf("Daemon with PID %ld created.\n", rc);
		exit(EXIT_SUCCESS);
	} else if(!rc) {
		setup_daemon(argc, argv);
	} else {
		perror("ERROR"); exit(EXIT_FAILURE);
	}
	exit(EXIT_FAILURE);
}
