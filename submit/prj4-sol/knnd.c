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
  mode_t mode;
  unsigned initValue;
} SemOpenArgs;

static SemOpenArgs semArgs[] = {
  { .posixName = SERVER_SEM_NAME,
    .oflags = O_RDWR|O_CREAT,
    .mode = ALL_RW_PERMS,
    .initValue = 1,
  },
  { .posixName = REQUEST_SEM_NAME,
    .oflags = O_RDWR|O_CREAT,
    .mode = ALL_RW_PERMS,
    .initValue = 0,
  },
  { .posixName = RESPONSE_SEM_NAME,
    .oflags = O_RDWR|O_CREAT,
    .mode = ALL_RW_PERMS,
    .initValue = 0,
  },
};

static void make_daemon(void) {
	pid_t pid = fork();
	if(pid) {
		exit(EXIT_SUCCESS);
	}
	else if(!pid) {
		//if(chdir("/") == -1)
		//	error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		umask(0);
		if(setsid() < 0)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		return; /* Daemon goes back to main */
	} else {
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	}
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	if(argc < 2 || argc > 4) {
		printf("Usage: ./knnd DATA_DIR [K]\n");
		exit(EXIT_FAILURE);
	}
	assert(sizeof(ShmObj) == SHM_SIZE);
	/* Setup daemon */
	char *data_dir = argv[1];
	int k = (argc >= 3) ? atoi(argv[2]) : 3;
	make_daemon();

	/* Setup semaphores */
	sem_t *sems[N_SEMS];
	for(int i = 0; i < N_SEMS; i++) {
		const SemOpenArgs *p = &semArgs[i];
		if((sems[i] = sem_open(p->posixName, p->oflags, p->mode, p->initValue)) == NULL)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	}

	/* Setup shared memory */	
	int shmfd = shm_open(SHM_NAME, O_RDWR|O_CREAT, ALL_RW_PERMS);
	if(shmfd < 0)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	if(ftruncate(shmfd, SHM_SIZE) < 0) {
		error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		goto DEALLOC;
	}
	void *buf = NULL;
	if ((buf = mmap(NULL, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED) {
		error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
		goto DEALLOC;
	}
	printf("Shared memory attached at %p\n", buf);
	
	/* Read image data */
	const struct LabeledDataListKnn *training_data = read_labeled_data_knn(data_dir, TRAINING_DATA, TRAINING_LABELS);
	//if(chdir("/") == -1) {
	//	error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	//	free_labeled_data_knn((struct LabeledDataListKnn*)training_data);
	//	goto DEALLOC;
	//}

	/* Enter server loop */
	ShmObj *shmdata = (ShmObj*)buf;
	for(;;) {
		//printf("Daemon: before wait\n");
		if(sem_wait(sems[REQUEST_SEM]) < 0) {
			error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
			free_labeled_data_knn((struct LabeledDataListKnn*)training_data);
			goto DEALLOC;
		}
		struct DataBytesKnn tmp;
		tmp.bytes = shmdata->image;
		tmp.len = 784;
		struct DataBytesKnn *test = &tmp;
		shmdata->nearest_index = knn_from_data_bytes(training_data, test, k);
		//printf("Daemon: Result of knn is %u\n", shmdata->nearest_index);
		const struct LabeledDataKnn *train = labeled_data_at_index_knn(training_data, shmdata->nearest_index);
		shmdata->train_label = labeled_data_label_knn(train);
		if(sem_post(sems[RESPONSE_SEM]) < 0) {
			error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
			free_labeled_data_knn((struct LabeledDataListKnn*)training_data);
			goto DEALLOC;
		}
		//printf("Daemon: Reached end of loop\n");
	} // for(;;)

	free_labeled_data_knn((struct LabeledDataListKnn*)training_data);
	DEALLOC:
	for(int i = 0; i < N_SEMS; i++) {
		const SemOpenArgs *p = &semArgs[i];
		if(sem_unlink(p->posixName))
			error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	}
	if(close(shmfd) < 0)
		error("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	exit(EXIT_SUCCESS);
}
