#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: ./knnc SERVER_DIR DATA_DIR [N_TESTS]\n");
		exit(EXIT_FAILURE);
	}

}
