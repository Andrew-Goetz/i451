#include <assert.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <errors.h>
#include <knn_ocr.h>

#include "util.h"

typedef struct {
  const char *posixName;
  int oflags;
} SemOpenArgs;

static SemOpenArgs semArgs[] = {
  { .posixName = SERVER_SEM_NAME,
    .oflags = O_RDWR,
  },
  { .posixName = REQUEST_SEM_NAME,
    .oflags = O_RDWR,
  },
  { .posixName = RESPONSE_SEM_NAME,
    .oflags = O_RDWR,
  },
};

int main(int argc, char *argv[]) {
	if(argc < 2 || argc > 4) {
		printf("Usage: ./knnc DATA_DIR [N_TESTS]\n");
		exit(EXIT_FAILURE);
	}
	char *data_dir = argv[1];

	/* Setup semaphores */
	sem_t *sems[N_SEMS];
	for(int i = 0; i < N_SEMS; i++) {
		const SemOpenArgs *p = &semArgs[i];
		if ((sems[i] = sem_open(p->posixName, p->oflags)) == NULL)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	}

	/* Setup shared memory */
	int shmfd = shm_open(SHM_NAME, O_RDWR, 0);
	if(shmfd < 0)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	void *buf = NULL;
	if ((buf = mmap(NULL, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED) {
		error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		goto DEALLOC;
	}

	/* Read image data & wait*/
	const struct LabeledDataListKnn *test_data = read_labeled_data_knn(data_dir, TEST_DATA, TEST_LABELS);
	const unsigned n_tests = (argc == 3) ? atoi(argv[2]) : 0;
	const unsigned n_test_data = n_labeled_data_knn(test_data);
	const unsigned n = (n_tests == 0) ? n_test_data : n_tests;
	unsigned n_ok = 0;
	int sval;
	sem_getvalue(sems[SERVER_SEM], &sval);
	//printf("sem value: %d\n", sval);
	if(sem_wait(sems[SERVER_SEM]) < 0) {
		error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		free_labeled_data_knn((struct LabeledDataListKnn*)test_data);
		goto DEALLOC;
	}
	ShmObj *shmdata = (ShmObj*)buf;

	/* Enter client loop */
	for(int i = 0; i < n; i++) {
		const struct LabeledDataKnn *test = labeled_data_at_index_knn(test_data, i);
		const struct DataKnn *t = labeled_data_data_knn(test);
		struct DataBytesKnn tmp = data_bytes_knn(t);
		assert(tmp.len == 784);
		for(int j = 0; j < 784; j++) 
			shmdata->image[j] = tmp.bytes[j];
		//printf("before request post\n");
		if(sem_post(sems[REQUEST_SEM]) < 0) {
			error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
			free_labeled_data_knn((struct LabeledDataListKnn*)test_data);
			goto DEALLOC;
		}
		//printf("after request post\n");
		if(sem_wait(sems[RESPONSE_SEM]) < 0) {
			error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
			free_labeled_data_knn((struct LabeledDataListKnn*)test_data);
			goto DEALLOC;
		}
		//printf("after response wait\n");
		unsigned test_label = labeled_data_label_knn(test);
		if(test_label == shmdata->train_label) {
			n_ok++;
		} else {
			const char *digits = "0123456789";
			printf("%c[%u] %c[%u]\n", digits[shmdata->train_label], shmdata->nearest_index, digits[test_label], i);
		}
	}

	/* sem_post to allow another process access to server */
	if(sem_post(sems[SERVER_SEM]) < 0) {
		error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		free_labeled_data_knn((struct LabeledDataListKnn*)test_data);
		goto DEALLOC;
	}
	printf("%g%% success\n", n_ok*100.0/n);

	free_labeled_data_knn((struct LabeledDataListKnn*)test_data);
	DEALLOC:
	if(close(shmfd) < 0)
		error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);

	exit(EXIT_SUCCESS);
}
