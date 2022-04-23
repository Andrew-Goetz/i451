#include <assert.h>
#include <fcntl.h>
#include <ndbm.h>
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

#define DBM_NAME "dbm

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
		//	panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
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
	char data_dir[] = argv[1];
	int k = atoi(argv[2]);
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
	if(ftruncate(shmfd, SHM_SIZE) < 0)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	char *buf = NULL;
	if ((buf = mmap(NULL, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0)) == MAP_FAILED)
		panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	printf("Shared memory attached at %s\n", buf);
	
	/* Read image data */
	const struct LabeledDataListKnn *training_data = read_labeled_data_knn(argv[2], TRAINING_DATA, TRAINING_LABELS);

	/* Enter server loop */
	for(;;) {
		if(sem_wait(sems[REQUEST_SEM]) < 0)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);

		

		if(sem_post(sems[RESPONSE_SEM]) < 0)
			panic("Error in function %s on line %d in file %s:", __func__, __LINE__, __FILE__);
	} // for(;;)
}
