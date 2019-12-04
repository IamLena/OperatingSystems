#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
	pid_t childpid1, childpid2;
	if ((childpid1 = fork()) == -1) {
		perror("Can't fork 1");
		exit(1);
	}
	else if (childpid1 == 0) {
		printf("forked Child pid = %d, ppid = %d, gr = %d;\n", getpid(), getppid(), getgid());
		if (execl("/bin/ps", "ps", "ax", NULL) == -1) {
			perror ("exec");
			printf("here\n");
			exit(1);
		}
	}
	else {
		if ((childpid2 = fork()) == -1) {
			perror("Can't fork 2");
			exit(1);
		}
		else if (childpid2 == 0) {
			printf("forked Child pid = %d, ppid = %d, gr = %d;\n", getpid(), getppid(), getgid());
			if (execl("/bin/ls", "ls", "-a", NULL) == -1) {
				perror ("exec");
				printf("here\n");
				exit(1);
			}
		}
		else {
			printf("Parent pid = %d, chpid1 = %d, chpid2 = %d, gr = %d;\n", getpid(), childpid1, childpid2, getgid());
			int status;
			int childpid = wait(&status);
			printf("Child has finished: PID = %d\n", childpid);
			if (WIFEXITED(status))
				printf("Child exited with code %d\n", WEXITSTATUS(status));
			else
				printf("Child terminated abnormally\n");
			childpid = wait(&status);
			printf("Child has finished: PID = %d\n", childpid);
			if (WIFEXITED(status))
				printf("Child exited with code %d\n", WEXITSTATUS(status));
			else
				printf("Child terminated abnormally\n");
			return 0;
		}
	}
}
