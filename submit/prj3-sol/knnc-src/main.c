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

void notify_daemon(char *SERVER_DIR, char pid[20]) {
	char *fifo = strncat(SERVER_DIR, KNOWN_FIFO, sizeof(KNOWN_FIFO));
	int fd = open(fifo, O_RDWR);
	if(fd == -1)
		error_handle();
	if(write(fd, pid, 20) == -1)
		error_handle();
	//printf("wrote: '%s' to fifo\n", pid);
	close(fd);
}

void init_private_fifo(char *out_fifo, int *out, int *in) {
	char in_fifo[30];
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
	if(argc < 3 || argc > 5) {
		printf("Usage: ./knnc SERVER_DIR DATA_DIR [N_TESTS]\n");
		exit(EXIT_FAILURE);
	}

	char pid[20];
	sprintf(pid, "%ld", (long)getpid());
	char *server_dir;
	strncpy(server_dir, argv[1], sizeof(argv[1]));

	char *part = strncat(argv[1], pid, sizeof(pid));
	int *out_p; int *in_p;
	init_private_fifo(part, out_p, in_p);
	int out = *out_p;
	int in = *in_p;
	notify_daemon(argv[1], pid);

	const char *data_dir = argv[2];
	const unsigned n_tests = (argc >= 4) ? atoi(argv[3]) : 0;

	const struct LabeledDataListKnn *test_data = read_labeled_data_knn(data_dir, TEST_DATA, TEST_LABELS);

	const unsigned n_test_data = n_labeled_data_knn(test_data);
	const unsigned n = (n_tests == 0) ? n_test_data : n_tests;

	unsigned n_ok = 0;
	unsigned recieve[2];
	int send_index[1];

	for(int i = 0; i < n; i++) {
		send_index[1] = i;
		if(write(out, &send_index, sizeof(send_index)) == -1)
			error_handle();

		const struct LabeledDataKnn *test = labeled_data_at_index_knn(test_data, i);
		const struct DataKnn *test_data = labeled_data_data_knn(test);
		struct DataBytesKnn send = data_bytes_knn(test_data);
		if(write(out, &send, sizeof(send)) == -1)
			error_handle();

		if(read(in, recieve, sizeof(recieve)) == -1)
			error_handle();
		
		unsigned test_label = labeled_data_label_knn(test);
		if(test_label == recieve[1]) {
			n_ok++;
		} else {
			const char *digits = "0123456789";
			printf("%c[%u] %c[%u]\n", digits[recieve[1]], recieve[0], digits[test_label], i);
		}
	}
	send_index[1] = -1;
	if(write(out, send_index, sizeof(send_index)) == -1)
		error_handle();

	printf("%g%% success\n", n_ok*100.0/n);

	if(close(in) == -1)
		error_handle();
	if(close(out) == -1)
		error_handle();
  	free_labeled_data_knn((struct LabeledDataListKnn*)test_data);

	/* Delete FIFOs */
	char *path_in;
	char *path_out;
	sprintf(path_in, "%ld_in", (long)getpid());
	sprintf(path_out, "%ld_out", (long)getpid());
	if(remove(path_in) == -1)
		error_handle();
	if(remove(path_out) == -1)
		error_handle();
	exit(EXIT_SUCCESS);
}
