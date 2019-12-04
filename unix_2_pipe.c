#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(void) {
	pid_t childpid1, childpid2;
	char msg1[] = "hello ";
	char msg2[] = "world!";
	char buf[16];
	char all_buf[16];
	int fd[2];

	if (pipe(fd) == -1) {
		perror("pipe");
		return 1;
	}
	if ((childpid1 = fork()) == -1) {
		perror("fork");
		exit(1);
	}
	else if (childpid1 == 0) {
		printf("Childpid1 = %d\n", getpid());
		close(fd[0]);
		write(fd[1], msg1, sizeof(msg1));
	
	}
	else {
		if ((childpid2 = fork()) == -1) {
			perror("fork");
			exit(1);
		}
		else if (childpid2 == 0) {
			printf("Childpid2 = %d\n", getpid());
			close(fd[0]);
			write(fd[1], msg2, sizeof(msg2));
		}
		else {
			printf("parentid = %d\n", getpid());
			int status;
			close(fd[1]);
			read(fd[0], all_buf, sizeof(buf));
			read(fd[0], buf, sizeof(buf));
			strncat(all_buf, buf, sizeof(buf));
			printf("parent: %s\n", all_buf);
			
			pid_t child_pid = wait(&status);
			printf("child has finished: PID = %d\n", child_pid);
			if (WIFEXITED(status))
				printf("exit normal, code = %d\n", WEXITSTATUS(status));
			else
				printf("child terminated not normal");

			child_pid = wait(&status);
			printf("child has finished: PID = %d\n", child_pid);
			if (WIFEXITED(status))
				printf("exit normal, code = %d\n", WEXITSTATUS(status));
			else
				printf("child terminated not normal");

			return 0;
		}
	}
}
