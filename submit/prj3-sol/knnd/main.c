#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <knn_ocr.h>

#define KNOWN_FIFO "notify"

void error_handle(void) {
	perror("Error");
	exit(EXIT_FAILURE);
}

int open_worker_fifo(char pid[20]) {
	return access(pid, F_OK) ? open(pid, O_RDONLY) : error_handle();
}

void do_work(char pid[20]) {
	int fd = open_worker_fifo(pid);
	if(fd == -1)
		error_handle();
	for(;;) {
		if(read(fd, buf, sizeof(buf)) == -1)
			error_handle();

		if(buf[0] == '\0') break;
	} 
	exit(EXIT_SUCCESS);
}

void setup_worker(char pid[20]) {
	long rc = fork();
	if(rc) {
		exit(EXIT_SUCCESS);
	} else if(!rc) {
		if(rc = fork()) { 
			exit(EXIT_SUCCESS);
		} else if(!rc) {
			do_work(pid);
		} else {
			error_handle();
		}
	} else { 
		error_handle();
	}
}

void service(int argc, char *argv[]) {
	if(access(KNOWN_FIFO, F_OK) && mkfifo(KNOWN_FIFO, 0666) == -1)
		error_handle();
	int fd = open(KNOWN_FIFO, O_RDWR);
	if(fd == -1)
		error_handle();
	char buf[20];
	for(;;) {
		if(read(fd, buf, sizeof(buf)) == -1)
			error_handle();
		//printf("buf:%s\n", buf);
		setup_worker(buf);
		memset(buf, 0, sizeof(buf));
	}
	exit(EXIT_FAILURE);
}

void setup_daemon(int argc, char *argv[]) {
	if(setsid() == -1)
		error_handle();
	long rc = fork();
	if(rc) {
		printf("Daemon with PID %ld created.\n", rc);
		exit(EXIT_SUCCESS);
	} else if(!rc) {
		if(chdir(argv[1]) == -1)
			error_handle();
		umask(0);
		service(argc, argv);
	} else {
		error_handle();
	}
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: ./knnd SERVER_DIR DATA_DIR [K]\n");
		exit(EXIT_FAILURE);
	}
	long rc = fork();
	if(rc) {
		exit(EXIT_SUCCESS);
	} else if(!rc) {
		setup_daemon(argc, argv);
	} else {
		error_handle();
	}
	exit(EXIT_FAILURE);
}
