#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define SB 0
#define SW 1
#define SAR 2
#define SWW 3
#define SWR 4

#define R 5
#define W 3

#define N 5

struct sembuf canread[3] = {{SW, 0, 1}, {SWW, 0, 1}, {SWR, 1, 1}};
//блокируется в ожидании момента, когда никто не пишет и нет ожидающих писателей
//увеличивает количество ждущих читателей

struct sembuf startread[3] = {{SWR, -1, 1}, {SAR, 1, 1}};
//уменьшает количество ждущих писателей, увеличивает количество активных читателей

struct sembuf stopread[1] = {{SAR, -1, 1}};
//уменьшает количество активных писателей

struct sembuf canwrite[3] = {{SAR, 0, 1}, {SW, 0, 1}, {SWW, 1, 1}};
//блокируется в ожидании момента, когда не будет активных читателей и активного писателя
//увеличивает количество ждущих писателей

struct sembuf startwrite[3] = {{SWW, -1, 1}, {SW, 1, 1}, {SB, -1, 1}};
//уменьшаем количество ожидающих писателей, делаем активного писателя
//захватываем буфер

struct sembuf stopwrite[2] = {{SW, -1, 1}, {SB, 1, 1}};
//Писатель не активен, буфер не занят

void ermsg(const char *msg) {
	perror(msg);
	exit(1);
}

void read_func(int fdm, int fds, int id) {
	printf("reader #%d\n", id);
	int* addr = (int*)shmat(fdm, 0, 0);
	if ((void*)addr == (void *) -1){ermsg("shmat");}
	for (int i = 0; i < N; i++) {
		sleep(rand() % 2);
		if (semop(fds, canread, 3) == -1) {ermsg("can_read");}
		if (semop(fds, startread, 3) == -1) {ermsg("start_read");}
		printf("reader %d read - %d\n", id, *addr);
		if (semop(fds, stopread, 1) == -1) {ermsg("stop_read");}
	}
	printf("reader #%d finished\n", id);
}

void write_func(int fdm, int fds, int id) {
	printf("writer #%d\n", id);
	int* addr = (int*)shmat(fdm, 0, 0);
	if ((void*)addr == (void *) -1){ermsg("shmat");}
	for (int i = 0; i < N; i++) {
		sleep(rand() % 4);
		if (semop(fds, canwrite, 3) == -1) {ermsg("can_write");}
		if (semop(fds, startwrite, 3) == -1) {ermsg("start_write");}
		(*addr)++;
		printf("writer %d changed - %d\n", id, *addr);
		if (semop(fds, stopwrite, 2) == -1) {ermsg("stop_write");}
	}
	printf("writer #%d finished\n", id);
}

int main(void) {
	int perms = S_IRWXU|S_IRWXG|S_IRWXO;

	int fds = semget(5, 5, IPC_CREAT|perms);
	if (fds == -1) {ermsg("semget");}

	int ctlb = semctl(fds, SB, SETVAL, 1);
	int ctlw = semctl(fds, SW, SETVAL, 0);
	int ctlar = semctl(fds, SAR, SETVAL, 0);
	int ctlwr = semctl(fds, SWR, SETVAL, 0);
	int ctlww = semctl(fds, SWW, SETVAL, 0);
	
	if (ctlb == -1 || ctlwr == -1 || ctlww == -1 || ctlar == -1) {ermsg("semctl");}

	int fdm = shmget(4, sizeof(int), IPC_CREAT|perms);
	if (fdm == -1) {ermsg("shmget");}
	int* addr = (int*)shmat(fdm, 0, 0);
	if ((void*)addr == (void *) -1){ermsg("shmat");}
	*addr = 0; //number to rewrite and read

	int pid;
	for (int i = 0; i < R; i++) {
		pid = fork();
		if (pid == -1) {ermsg("fork");}
		if (pid == 0) {
			read_func(fdm, fds, i);
			return 0;
		}
		else{printf("%d forked reader %d\n", getpid(), pid);}
	}
	
	for (int i = 0; i < W; i++) {
		pid = fork();
		if (pid == -1) {ermsg("fork");}
		if (pid == 0) {
			write_func(fdm, fds, i);
			return 0;
		}
		else{printf("%d forked writer %d\n", getpid(), pid);}
	}

	int status, child;
	for (int i = 0; i < R + W; i++) {
		child = wait(&status);
		printf("Child has finished: PID = %d CODE = %d\n", child, WEXITSTATUS(status));
	}
	if (shmctl (fdm, IPC_RMID, (struct shmid_ds *) 0) == -1) {ermsg("shmctl");}	
}
