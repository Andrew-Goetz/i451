#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <knn_ocr.h>
#include <errors.h>

#define KNOWN_FIFO "notify"

void notify_daemon(void) {
	int fd = open(KNOWN_FIFO, O_RDWR);
	if(fd == -1)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	long pid = (long)getpid();
	if(write(fd, &pid, sizeof(pid)) == -1)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	close(fd);
}

void init_private_fifo(char *path_in, char *path_out, int *out, int *in) {
	if(mkfifo(path_out, 0666) == -1 && errno != EEXIST)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	*out = open(path_out, O_RDWR);
	if(*out == -1)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);

	if(mkfifo(path_in, 0666) == -1 && errno != EEXIST)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	*in = open(path_in, O_RDWR);
	if(*in == -1)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
}

int main(int argc, char *argv[]) {
	if(argc < 3 || argc > 5) {
		printf("Usage: ./knnc SERVER_DIR DATA_DIR [N_TESTS]\n");
		exit(EXIT_FAILURE);
	}

	const char *data_dir = argv[2];
	/* Read data before chdir */
	const struct LabeledDataListKnn *test_data = read_labeled_data_knn(data_dir, TEST_DATA, TEST_LABELS);

	if(chdir(argv[1]) == -1)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);

	char path_in[100];
	char path_out[100];
	sprintf(path_in, "%ld_in", (long)getpid());
	sprintf(path_out, "%ld_out", (long)getpid());

	int *out_p = malloc(sizeof(out_p));
	int *in_p = malloc(sizeof(in_p));
	init_private_fifo(path_in, path_out, out_p, in_p);

	int out = *out_p;
	int in = *in_p;
	free(out_p);
	free(in_p);
	//printf("%s, %s\n", path_in, path_out);
	char *server_dir = argv[1];
	notify_daemon();

	//printf("data_dir:%s\n", data_dir);
	const unsigned n_tests = (argc >= 4) ? atoi(argv[3]) : 0;

	const unsigned n_test_data = n_labeled_data_knn(test_data);
	const unsigned n = (n_tests == 0) ? n_test_data : n_tests;

	unsigned n_ok = 0;
	unsigned recieve[2];
	int send_index;

	for(int i = 0; i < n; i++) {
		send_index = i;
		if(write(out, &send_index, sizeof(int)) == -1)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		//printf("Write1 in knnd succeeded!\n");

		const struct LabeledDataKnn *test = labeled_data_at_index_knn(test_data, i);

		const struct DataKnn *t = labeled_data_data_knn(test);
		struct DataBytesKnn tmp = data_bytes_knn(t);
		unsigned char send[784];
		for(int j = 0; j < 784; j++) {
			send[j] = tmp.bytes[j];
			//printf("%u\n", send[j]);
		}
		/*
		printf("-----------knnc------------\n");
		for(int j = 0; j < 784; j++) {
			printf("%u	", send[j]);
			if(j % 30 == 0) printf("\n");
		}
		*/
		assert(tmp.len == 784);
		if(write(out, send, 784) == -1)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		//printf("Write2 in knnd succeeded!\n");

		unsigned test_label = labeled_data_label_knn(test);
		if(read(in, recieve, sizeof(recieve)) == -1)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		//printf("Read in knnd succeeded! test_label:%u, recieve_label:%u\n", test_label, recieve[1]);
		
		if(test_label == recieve[1]) {
			n_ok++;
		} else {
			const char *digits = "0123456789";
			printf("%c[%u] %c[%u]\n", digits[recieve[1]], recieve[0], digits[test_label], i);
		}
	}
	send_index = -1;
	if(write(out, &send_index, sizeof(int)) == -1)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	printf("Sent -1 index to knnd\n");

	printf("%g%% success\n", n_ok*100.0/n);

	if(close(in) == -1)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	if(close(out) == -1)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
  	free_labeled_data_knn((struct LabeledDataListKnn*)test_data);

	/* Delete FIFOs */
	if(unlink(path_in) == -1 && errno != ENOENT)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	if(unlink(path_out) == -1 && errno != ENOENT)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	exit(EXIT_SUCCESS);
}
