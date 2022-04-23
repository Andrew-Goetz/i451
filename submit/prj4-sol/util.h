#ifndef _UTILITY_H
#define _UTILITY_H

#define SHM_SIZE 784 + sizeof(unsigned)*2

#define POSIX_IPC_NAME_PREFIX "/knn_"

#define SHM_NAME POSIX_IPC_NAME_PREFIX "shm"
#define SERVER_SEM_NAME POSIX_IPC_NAME_PREFIX "server"
#define REQUEST_SEM_NAME POSIX_IPC_NAME_PREFIX "request"
#define RESPONSE_SEM_NAME POSIX_IPC_NAME_PREFIX "response"

enum {
	SERVER_SEM,
	REQUEST_SEM,
	RESPONSE_SEM,
	N_SEMS
};

typedef struct shmobj {
	char image[784];
	unsigned nearest;
	unsigned predicted;
	unsigned expected;
} ShmObj;

#define ALL_RW_PERMS (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWGRP)

#endif
