#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <errors.h>
#include <knn_ocr.h>

int main(int argc, char *argv[]) {
	if(argc < 2 || argc > 4) {
		printf("Usage: ./knnc DATA_DIR [N_TESTS]\n");
		exit(EXIT_FAILURE);
	}

	char data_dir[] = argv[1];
	const unsigned n_tests = (argc == 3) ? atoi(argv[2]) : 0;
	const unsigned n_test_data = n_labeled_data_knn(test_data);
	const unsigned n = (n_tests == 0) ? n_test_data : n_tests;

	const struct LabeledDataListKnn *test_data = read_labeled_data_knn(data_dir, TEST_DATA, TEST_LABELS);
	unsigned n_ok = 0;

}
