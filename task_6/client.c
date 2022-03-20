#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>

#define NMX 256

/*
 * 0 - кол-во посетителей 
 * 1 - кол-во ожидающих клиентов
 * 2 - ресурс 
 * 3 - выход
 */

struct data_st
{
	char buf[NMX];
	int cmd;
	pid_t pid;
};

/* FU */
void up(int n);
void down(int n);
int connect();
void handler(int sig);

/* GLOB */
int semid, shmid;
struct data_st *shmaddr;
char *buffer = NULL;

/* MA */
int main(int argc, char **argv)
{
	key_t key;
	int i, ctr;
	char c;

	signal(SIGINT, handler);
	signal(SIGUSR1, handler);
	key = ftok("/etc/passwd", 'A');
	if ((shmid = shmget(key, sizeof(struct data_st), 0666)) < 0) { printf("Ресурс не доступен\n"); exit(0);}
	if ((shmaddr = (struct data_st *)shmat(shmid, NULL, 0)) < 0) { printf("Ресурс не доступен\n"); exit(0); }
	if ((semid = semget(key, 4, 0666)) < 0) { printf("Ресурс не доступен\n"); exit(0); }
	if (!connect())
	{
		printf("Ошибка подключения: сервер перегружен\n");
		if (shmdt(shmaddr) < 0) { printf("ERROR\n"); exit(1); }
		exit(0);
	}
	while (1)
	{
		buffer = (char *)malloc(sizeof(char));
		int j = 0;
		i = 0;
		printf("Введите текст\n");
		while ((c = getchar()) != '\n' && i != NMX-1)
		{
			buffer[i++] = c;
			buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
			buffer[i] = '\0';
		}
		down(3);
		if (strcmp(buffer, "exit") == 0)
		{
			down(1);
			shmaddr->cmd = 2;
			up(2);
			up(3);
			printf("Завершение клиента и сервера...\n");
			if (shmdt(shmaddr) < 0) { printf("ERROR\n"); exit(1); }
			free(buffer);
			exit(0);
		}
		i = 0;
		while (buffer[i] != '\0')
		{
/*			printf("blocked 1...\n");*/
			down(1);
/*			printf("unblocked 1...\n");*/
			while (buffer[i] != '\0' && j < NMX)
			{
				shmaddr->buf[j++] = buffer[i++];
			}
			if (buffer[i] == '\0')
			{
				shmaddr->buf[j] = '\0';
				shmaddr->cmd = 0;
			}
			else
				shmaddr->cmd = 1;
			shmaddr->pid = getpid();
/*			printf("blocked 2...\n");*/
			up(2);
/*			printf("unblocked 2...\n");*/
		}
/*		printf("blocked 3...\n");*/
/*		down(1);*/
/*		printf("unblocked 3...\nblocked 4...\n");*/
		up(3);
/*		printf("unblocked 4...\n");*/
		free(buffer);
		buffer = NULL;
	}
	return 0;
	
}

void handler(int sig)
{
	if (sig == SIGINT || sig == SIGUSR1)
	{
		printf("Завершение клиента...\n");
		up(0);
		if (shmdt(shmaddr) < 0) { printf("ERROR\n"); exit(1); }
		if (buffer != NULL)
			free(buffer);
		exit(0);
	}
	return;
}

void up(int n)
{
	struct sembuf sem;

	sem.sem_op = 1;
	sem.sem_num = n;
	sem.sem_flg = 0;
	if (semop(semid, &sem, 1) < 0)
	{
		printf("Сервер прекратил свое существование\nЗавершение работы...\n");
		shmdt(shmaddr);
		if (buffer != NULL)
			free(buffer);
		exit(1);
	}
	return;
}

void down(int n)
{
	struct sembuf sem;

	sem.sem_op = -1;
	sem.sem_num = n;
	sem.sem_flg = 0;
	if (semop(semid, &sem, 1) < 0)
	{
		printf("Сервер прекратил свое существование\nЗавершение работы...\n");
		shmdt(shmaddr);
		if (buffer != NULL)
			free(buffer);
		exit(1);
	}

	return;
}

int connect()
{
	struct sembuf sem;

	printf("Cnnecting...\n");
	sem.sem_op = -1;
	sem.sem_num = 0;
	sem.sem_flg = IPC_NOWAIT;
	return semop(semid, &sem, 1) == -1 ? 0 : 1;
}
