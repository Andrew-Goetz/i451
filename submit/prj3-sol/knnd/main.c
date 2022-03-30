#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void worker(char *fifo[]) {

	exit(EXIT_SUCCESS);
}

void process_signal(void) {
	long rc = fork();
	if(rc) {
		
	} else if(!rc) {

	} else {
		perror("ERROR");
		exit(EXIT_FAILURE);
	}
}

void service(int argc, char *argv[]) {
	for(;;) {
		signal(SIGUSR1, spawn_worker);
	}
	exit(EXIT_FAILURE)
}

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: ./knnd SERVER_DIR DATA_DIR [K]\n");
		exit(EXIT_FAILURE);
	}
	long rc = fork();
	if(rc) {
		printf("Daemon with PID %ld created.\n", rc);
		exit(EXIT_SUCCESS);
	} else if(!rc) {
		service(argc, argv);
	} else {
		perror("ERROR");
		exit(EXIT_FAILURE);
	}
}
