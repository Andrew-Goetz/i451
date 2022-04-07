#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <knn_ocr.h>
#include <errors.h>

#define KNOWN_FIFO "notify"

void open_worker_fifos(const char pid[20], int *out, int *in) {
	char path_in[30]; char path_out[30];
	strncpy(path_in, pid, 20);
	strncpy(path_out, pid, 20);

	strncat(path_in, "_in", sizeof("_in"));
	strncat(path_out, "_out", sizeof("_out"));
	//printf("in: %s, out: %s\n", path_in, path_out);
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

void do_work(char pid[20], const unsigned k, const struct LabeledDataListKnn *training_data) {
	int *out_p = malloc(sizeof(out_p));
	int *in_p = malloc(sizeof(in_p));
	open_worker_fifos(pid, out_p, in_p);
	int out = *out_p; 
	int in = *in_p;
	free(out_p);
	free(in_p);

	unsigned to_client[2];
	int get_index;
	for(;;) {
		if(read(out, &get_index, sizeof(int)) == -1)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		//printf("After read, get_index == %d\n", get_index);
		if(get_index < 0) {
			printf("Exiting loop\n");
			break;
		}
		unsigned char *recieve = malloc(784);
		if(read(out, recieve, 784) == -1)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		/*
		printf("-----------knnd------------\n");
		for(int j = 0; j < 784; j++) {
			printf("%u	", recieve[j]);
			if(j % 30 == 0) printf("\n");
		}
		*/
		//printf("Read the recieve array\n");
		struct DataBytesKnn tmp;
		//printf("Read the recieve array\n");
		tmp.bytes = recieve;
		tmp.len = 784;
		struct DataBytesKnn *test = &tmp;
		//printf("knn running...\n");
		to_client[0] = knn_from_data_bytes(training_data, test, k);
		//printf("knn ran!\n");
		const struct LabeledDataKnn *train = labeled_data_at_index_knn(training_data, to_client[0]);
		to_client[1] = labeled_data_label_knn(train);
		//printf("Write result: %u, %u\n", to_client[0], to_client[1]);

		if(write(in, to_client, sizeof(to_client)) == -1)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		free(recieve);
	} 
	free_labeled_data_knn((struct LabeledDataListKnn*)training_data);
	if(close(in) == -1)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	if(close(out) == -1)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);

	exit(EXIT_SUCCESS);
}

void setup_worker(char pid[20], const unsigned k, const struct LabeledDataListKnn *training_data) {
	long rc = fork();
	if(rc) {
		return; /* daemon returns to service() */
	} else if(!rc) {
		if(rc = fork()) { 
			wait(NULL);
			exit(EXIT_SUCCESS);
		} else if(!rc) {
			do_work(pid, k, training_data);
		} else {
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		}
	} else { 
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	}
	exit(EXIT_FAILURE);
}

void service(int argc, char *argv[], const struct LabeledDataListKnn *training_data) {
	if(mkfifo(KNOWN_FIFO, 0666) == -1 && errno != EEXIST)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	int fd = open(KNOWN_FIFO, O_RDWR);
	if(fd == -1)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);

	const unsigned k = (argc >= 4) ? atoi(argv[3]) : 3;
	long pid;
	for(;;) {
		if(read(fd, &pid, sizeof(pid)) == -1)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		char buf[20];
		sprintf(buf, "%ld", pid);
		setup_worker(buf, k, training_data);
		memset(buf, 0, sizeof(buf));
	}
	exit(EXIT_FAILURE);
}

void setup_daemon(int argc, char *argv[]) {
	if(setsid() == -1)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	long rc = fork();
	if(rc) {
		printf("Daemon with PID %ld created.\n", rc);
		exit(EXIT_SUCCESS);
	} else if(!rc) {
		/* Read data before chdir */
		const struct LabeledDataListKnn *training_data = read_labeled_data_knn(argv[2], TRAINING_DATA, TRAINING_LABELS);
		if(chdir(argv[1]) == -1)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		umask(0);
		service(argc, argv, training_data);
	} else {
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
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
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	}
	exit(EXIT_FAILURE);
}
