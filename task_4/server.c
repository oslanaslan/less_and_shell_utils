#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define LEN 6 /* длина слова SERVER */

void extHnd(int s)
{
	exit(0);
}

int ser_cnt = 0;

void start_daemon();

int main(int argc, char **argv)
{
	pid_t son = fork();

	if (!son) /* Сын */
	{
		signal(SIGUSR1, sigHnd);
		start_daemon();
	}
	else if (son == -1) /* Ошибка */
	{
		printf("ERROR: Problem with fork\n");
		return 1;
	}
	else /* Отец */
	{
		char c;

		signal(SIGUSR1, sigHnd);
		while ((c = getc()) != '\004');
		kill(son, SIGUSR1);
		waitpid(son);
		return 0;
	}
}

/* 0: кол-во клиентов
 * 1: есть ли в в памяти что-либо 
 * 2: пишет ли кто-то в память */
void start_daemon()
{
	key_t key;
	int semid, shmid, i;
	struct sembuf sem;

	key = ftok("/etc/passwd", 'a');
	semid = semget(key, 3, 0666|IPC_CREAT|IPC_EXCL);
	if (semid == -1)
	{
		printf("ERROR: Не удалось создать семафор\n");
		kill(getppid(), SIGUSR1);
		exit(0);
	}
	shmid = shmget(key, 256, 0666|IPC_CREAT|IPC_EXCL);
	if (shmid == -1)
	{
		printf("ERROR: Не удалось создать разделяемую память\n");
		for (i = 0; i < 3; i++)
			semctl(semid, i, IPC_RM);
		kill(getppid(), SIGUSR1);
		exit(0);
	}
	
	for (i = 0; i < 3; i++)
	{
		if (!sem_init(semid[1], 0, 0))
		{
			printf("ERROR: Не удалось инициализировать семафор %d\n", i);
			for (i = 0; i < 3; i++)
				semctl(semid, i, IPC_RM);
			shmctl(shmid, IPC_RMID, NULL);

			kill(getppid(), SIGUSR1);
			exit(0);
		}
	}
	
	while (1)
	{
		sem = {1, -1, 0};
		if (semop(semid, sem, 1) == -1) /* проверяем, есть ли что-то в раз. пам. */
		{
			printf("ERROR: Не удалось изменить значение семафора 0\n");
			for (i = 0; i < 3; i++)
				semctl(semid, i, IP_RM);
			shmctl(shmid, IPC_RMID, NULL);
			kill(getppid(), SIGUSR1);
			exit(0);
		}
		sem = {2, -1, 0};
		if (semop(semid, sem, 1) == -1) /* если да, то блокируем раз. пам. */
		{
			printf("ERROR: Не удалось изменить значение семафора 1\n");
			for (i = 0; i < 3; i++)
				semctl(semid, i, IP_RM);
			shmctl(shmid, IPC_RMID, NULL);
			kill(getppid(), SIGUSR1);
			exit(0);
		}
		
	}
}

int is_ser(char *str)
{
	char *word = "SERVER";
	int i;
	int fnd_ctr = 0;

	for (i = 0; i < strlen(str); i++)
	{
		if (ser_cnt == LEN)
		{
			fnd_ctr++;
			ser_cnt = 0;
		}
		if (str[i] == word[ser_cnt])
			ser_cnt++;
		else
			ser_cnt = 0;
	}
	return fnd_ctr;
}
