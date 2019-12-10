#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <math.h>

#define SE 0
#define SF 1
#define SB 2

#define pqty 5
#define pn 3
#define cn 3

struct sembuf lockproducer[2] = {{SE, -1, 1}, {SB, -1, 1}};
struct sembuf unlockproducer[2] = {{SB, 1, 1}, {SF, 1, 1}};
struct sembuf lockconsumer[2] = {{SF, -1, 1}, {SB, -1, 1}};
struct sembuf unlockconsumer[2] = {{SB, 1, 1}, {SE, 1, 1}};

void ermsg(const char *msg) {
	perror(msg);
	exit(1);
}

void consume_num(int fds, int *addr, int id) {
	sleep(rand() % 4);
	if (semop(fds, lockconsumer, 2) == -1) {ermsg("lock consumer");}
	int cindex = *(addr + 1) + 2;
	printf("c%d: %d\n", id, *(addr + cindex));
	*(addr + 1) = cindex - 1;
	if (semop(fds, unlockconsumer, 2) == -1) {ermsg("unlock consumer");}
}

void produce_num(int fds, int *addr, int id) {
	sleep(rand() % 3);
	if (semop(fds, lockproducer, 2) == -1) {ermsg("lock producer");}
	int pindex = (*addr) + 2;
	int num = pindex - 2;
	printf("p%d: %d\n", id, num);
	*(addr + pindex) = num;
	pindex -= 1;
	*addr = pindex;
	if (semop(fds, unlockproducer, 2) == -1) {ermsg("unlock producer");}
}

void consume(int fds, int fdm, int n, int i) {
	printf("consumer #%d\n", i);
	int *addr = (int *)shmat(fdm, 0, 0);
	if ((void*)addr == (void *) -1){ermsg("shmat");}
	for (int j = 0; j < n; j++) {
		consume_num(fds, addr, i);
	}
	
	if(shmdt(addr)) {ermsg("shmdt");}
	printf("consumer #%d is over\n", i);
}

void produce(int fds, int fdm, int n, int i) {
	printf("producer #%d\n", i);
	int *addr = (int *)shmat(fdm, 0, 0);
	if ((void*)addr == (void *) -1){ermsg("shmat");}
	for (int j = 0; j < n; j++) {
		produce_num(fds, addr, i);
	}
	if(shmdt(addr)) {ermsg("shmdt");}
	printf("producer #%d is over\n", i);
}

int main(void) {
	int consumers[cn];
	int producers[pn];

	int perms = S_IRWXU|S_IRWXG|S_IRWXO;
	
	int fds = semget(1, 3, IPC_CREAT|perms);
	if (fds == -1) {ermsg("semget");}

	int ctle, ctlf, ctlb;
	ctle = semctl(fds, SE, SETVAL, pqty * pn);
	ctlf = semctl(fds, SF, SETVAL, 0);
	ctlb = semctl(fds, SB, SETVAL, 1);
	if (ctle == -1 || ctlf == -1 || ctlb == -1) {ermsg("semctl");}

	size_t sizem = (pn * pqty + 2) * sizeof(int);
	int fdm = shmget(3, sizem, IPC_CREAT|perms);
	if (fdm == -1) {ermsg("shmget");}
	int* addr = (int*)shmat(fdm, 0, 0);
	if ((void*)addr == (void *) -1){ermsg("shmat");}
	*addr = 0; //produce index
	*(addr + 1) = 0; //consume index

	int cqty = pqty * pn / cn;
	int parentflag = 1;
	for (int i = 0; i < cn + pn; i++) {
		if (parentflag) {
			if (i < cn) {
				consumers[i] = fork();
				if (consumers[i] == -1) {
					ermsg("fork");
				}
				if (consumers[i] == 0) {
					parentflag = 0;
					consume(fds, fdm, cqty, i);
				}
				else {
					printf("%d forked consumer %d\n", getpid(), consumers[i]);
				}
			}
			else {
				producers[i] = fork();
				if (producers[i] == -1) {
					ermsg("fork");
				}
				if (producers[i] == 0) {
					parentflag = 0;
					produce(fds, fdm, pqty, i - cn);
				}
				else {
					printf("%d forked producer %d\n", getpid(), producers[i]);
				}
			}
		}
	}

	if (parentflag) {
		int status, child;
		for (int i = 0; i < cn + pn; i++) {
			child = wait(&status);
			printf("Child has finished: PID = %d CODE = %d\n", child, WEXITSTATUS(status));
		}
		if (shmctl (fdm, IPC_RMID, (struct shmid_ds *) 0) == -1) {
			ermsg("shmctl");}
	}	
}
