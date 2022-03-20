#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

volatile int stop;
int ball = 1;
int fd[2];
volatile pid_t father;
volatile pid_t son;

void sigHnd(int s)
{
	int tmp;
	pid_t some_pid = getpid();

	printf("sigHnd started in %i\n", some_pid);
	printf("Father: %i\nSon: %i\nCurrent pid: %i\n", father, son, some_pid);

	tmp = read(fd[0], &ball, sizeof(int));
	if (tmp == 0)
	{
		printf("Something went wrong in reading in sigHnd\n");
		exit(0);
		return;
	}

	printf("%i has ball %i\n", some_pid, ball);

	if (ball == stop)
	{
		printf("%i == %i in %i\n", ball, stop, some_pid);
		if (some_pid == son || son == 0)
		{
			printf("\n========================================\nStopped on son with ball %d\n========================================\n\n", ball);
			kill(father, SIGUSR2);
		}
		else if (some_pid == father)
		{
			printf("\n========================================\nStopped on father with ball %d\n========================================\n\n", ball);
			kill(son, SIGUSR2);
		}

		return;
	}	
	
	ball++;
	tmp = write(fd[1], &ball, sizeof(ball));
	if (tmp == 0)
	{
		printf("Something went wrong in writing in sigHnd\n");
		return;
	}

	printf("Writing ball = %i in %i\n", ball, some_pid);

	signal(SIGUSR1, sigHnd);
	if (some_pid == son || son == 0)
	{
		kill(father, SIGUSR1);
	}
	else if (some_pid == father)
	{
		kill(son, SIGUSR1);
	}
	printf("End of sigHnd\n");
	return;
}

void extHnd(int s)
{
	pid_t cur_pid = getpid();

	printf("extHnd started in %i\n", cur_pid);

	if (cur_pid == son)
	{
		printf("Sent kill sig to father\n");		
		kill(father, SIGUSR2);
		close(fd[0]);
		close(fd[1]);
		printf("Son has died\n");
		exit(0);
	}
	else if (cur_pid == father)
	{
		printf("Exiting in father\n");
			
		kill(son, SIGUSR2);
		printf("Sent kill sig to son\n");
		close(fd[0]);
		close(fd[1]);
		waitpid(son, NULL, 0);
		printf("Father has died\n");
		exit(0);

	}
	else
	{
		printf("Something went wrong\n");
		exit(0);
	}

	signal(SIGUSR2, extHnd);
}

int main(int argc, char **argv)
{
	int tmp;

	if (argc != 2)
	{
		printf("No args or too many args\n");
		return 1;
	}

	stop = atoi(argv[1]);

	if (stop == 0)
	{
		printf("\n========================================\nStopped of father with ball 0\n======================================\n\n");
		return 0;
	}
	tmp = pipe(fd);
	if (tmp == -1)
	{
		printf("Something went wrong in pipe\n");
		perror("ERROR in pipe\n");
		return 1;
	}

	father = getpid();
	
	signal(SIGUSR1, sigHnd);
	signal(SIGUSR2, extHnd);

	son = fork();
	if (son == -1)
	{
		printf("Something went wrong in fork\n");
		perror("ERROR in fork\n");
		return 1;
	}

	if (!son) /* Мы в сыне */
	{
		son = getpid();
		son = getpid();

		while (1)
		{
			printf("Waiting for father\n");
			pause();
		}

	}
	else if (getpid() == father) /* Мы в отце */
	{

		printf("Father: %i\nSon: %i\n\n", father, son);

		tmp = write(fd[1], &ball, sizeof(int));
		if (tmp == 0)
		{
			printf("Something went wrong\n");
			return 1;
		}
		kill(son, SIGUSR1);

		printf("Father sent first ball\n");

		while (1)
		{
			printf("Waiting for son\n");
			pause();
		}

	}
	else /* Че то не то */
	{
		printf("There is something strange\n");
		return 1;
	}
}
