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

void open_worker_fifos(const char pid[20], int *out, int *in) {
	char in_str[30]; char out_str[30];
	strncpy(in_str, pid, 20);
	strncpy(out_str, pid, 20);

	strncat(in_str, "_in", sizeof("_in"));
	strncat(out_str, "_out", sizeof("_out"));

	access(out_str, F_OK) ? *out = open(pid, O_RDONLY) : error_handle();
	if(*out == -1)
		error_handle();
	access(in_str, F_OK) ? *in = open(pid, O_WRONLY) : error_handle();
	if(*in == -1)
		error_handle();
}

void do_work(char pid[20], const unsigned k, const char *data_dir) {
	int *out_p; int *in_p;
	open_worker_fifos(pid, out_p, in_p);
	int out = *out_p; 
	int in = *in_p;

	const struct LabeledDataListKnn *training_data = read_labeled_data_knn(data_dir, TRAINING_DATA, TRAINING_LABELS);

	unsigned to_client[3];
	unsigned char from_client[785];
	for(;;) {
		if(read(out, from_client, sizeof(from_client)) == -1)
			error_handle();
		if(from_client[784] == 10) break;
		/* Process Image */
		to_client[0] = knn(training_data, temp, k);
		//to_client[1] = 
		to_client[2] = from_client[784];
		const struct LabeledDataKnn *training = labeled_data_at_index_knn(training_data, to_client[0]);

		if(write(in, to_client, sizeof(to_client)))
			error_handle();
		memset(from_client, 0, sizeof(from_client));
	} 
	free_labeled_data_knn((struct LabeledDataListKnn*)training_data);
	if(close(in) == -1)
		error_handle();
	if(close(out) == -1)
		error_handle();
	exit(EXIT_SUCCESS);
}

void setup_worker(char pid[20], const unsigned k, const char *data_dir) {
	long rc = fork();
	if(rc) {
		return; /* daemon returns to service() */
	} else if(!rc) {
		if(rc = fork()) { 
			exit(EXIT_SUCCESS);
		} else if(!rc) {
			do_work(pid, k, data_dir);
		} else {
			error_handle();
		}
	} else { 
		error_handle();
	}
	exit(EXIT_FAILURE);
}

void service(int argc, char *argv[]) {
	if(access(KNOWN_FIFO, F_OK) && mkfifo(KNOWN_FIFO, 0666) == -1)
		error_handle();
	int fd = open(KNOWN_FIFO, O_RDWR);
	if(fd == -1)
		error_handle();

	const unsigned k = (argc >= 4) ? atoi(argv[3]) : 3;
	char buf[20];
	for(;;) {
		if(read(fd, buf, sizeof(buf)) == -1)
			error_handle();
		//printf("buf:%s\n", buf);
		setup_worker(buf, k, argv[2]);
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
	if(argc < 3 || argc > 5) {
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
