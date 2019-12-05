#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define SE 0
#define SF 1
#define SB 2

#define N 5
#define pn 1
#define cn 1

struct sembuf lockproducer[2] = {{SE, -1, 1}, {SB, -1, 1}};
struct sembuf unlockproducer[2] = {{SB, 1, 1}, {SF, 1, 1}};
struct sembuf lockconsumer[2] = {{SF, -1, 1}, {SB, -1, 1}};
struct sembuf unlockconsumer[2] = {{SB, 1, 1}, {SE, 1, 1}};

void ermsg(const char *msg) {
	perror(msg);
	exit(1);
}

void consume_num(int fds, int **addr, int id) {
	sleep(rand() % 4);
	if (semop(fds, lockconsumer, 2) == -1) {ermsg("lock consumer");}
	printf("c%d: %d\n", id, **(addr));
	(*addr) ++;
	if (semop(fds, unlockconsumer, 2) == -1) {ermsg("unlock consumer");}
}

void produce_num(int fds, int **addr, int num, int id) {
	sleep(rand() % 2);
	if (semop(fds, lockproducer, 2) == -1) {ermsg("lock producer");}
	printf("p%d: %d\n", id, num);
	**addr = num;
	(*addr)++;
	if (semop(fds, unlockproducer, 2) == -1) {ermsg("unlock producer");}
}

void consume(int fds, int fdm, int n, int i) {
	printf("consumer #%d\n", i);
	int *addr = (int *)shmat(fdm, 0, 0);
	if ((void*)addr == (void *) -1){ermsg("shmat");}
	int *head = addr;
	for (int j = 0; j < n; j++) {
		consume_num(fds, &addr, i);
	}
	
	if(shmdt(head)) {ermsg("shmdt");}
	printf("consumer #%d is over\n", i);
}

void produce(int fds, int fdm, int n, int i) {
	printf("producer #%d\n", i);
	int *addr = (int *)shmat(fdm, 0, 0);
	if ((void*)addr == (void *) -1){ermsg("shmat");}
	int *head = addr;
	for (int j = 0; j < n; j++) {
		produce_num(fds, &addr, j, i);
	}
	if(shmdt(head)) {ermsg("shmdt");}
	printf("producer #%d is over\n", i);
} 

void fill(int *arr, int n) {
	for (int i = 0; i < n; i++) {
		arr[i] = -1;
	}
}

int main(void) {
	int consumers[cn];
	fill (consumers, cn);
	int producers[pn];
	fill (producers, pn);

	int perms = S_IRWXU|S_IRWXG|S_IRWXO;
	
	int fds = semget(1, 3, IPC_CREAT|perms);
	if (fds == -1) {ermsg("semget");}

	int ctle, ctlf, ctlb;
	ctle = semctl(fds, SE, SETVAL, N);
	ctlf = semctl(fds, SF, SETVAL, 0);
	ctlb = semctl(fds, SB, SETVAL, 1);
	if (ctle == -1 || ctlf == -1 || ctlb == -1) {ermsg("semctl");}

	int fdm = shmget(1, N * sizeof(int), IPC_CREAT|perms);
	if (fdm == -1) {ermsg("shmget");}

	int parentflag = 1;
	for (int i = 0; i < cn; i++) {
		if (parentflag) {
			consumers[i] = fork();
			parentflag *= consumers[i];
			printf("%d fork %d; p - %d\n", getpid(), consumers[i], parentflag);
		}
	}

	for (int i = 0; i < pn; i++) {
		if (parentflag) {
			producers[i] = fork();
			parentflag *= producers[i];
			printf("%d fork %d; p - %d\n", getpid(), producers[i], parentflag);
		}
	}
	
	for (int i = 0; i < cn; i++) {
		if (consumers[i] == 0) {
			consume(fds, fdm, N, i);
		}
	}

	for (int i = 0; i < pn; i++) {
		if (producers[i] == 0) {
			produce(fds, fdm, N, i);
		}
	}

	if (parentflag) {
		int status, child;
		for (int i = 0; i < cn + pn; i++) {
			child = wait(&status);
			printf("Child has finished: PID = %d CODE = %d\n", child, WEXITSTATUS(status));
		}
	}	
}
