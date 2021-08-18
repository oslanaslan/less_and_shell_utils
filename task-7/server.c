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
 * 0 - максимальное количество клиентов 
 * 1 - сейчас работает клиент
 * 2 - сейчас работает сервер
 * 3 - сервер занятс с клиентом
 */

/* GLOB */
int semid, shmid,exid;
pid_t pid = 0;
char **argv;
char *glob_buffer = NULL;
int buf_ctr = 0;
struct data_st *shmaddr;
int str_ptr = 0;


/*FU*/
int str_fun(char *str);
void handler(int sig);
void down(int n);
void up(int n);
int reading_client();
void print_data();

struct data_st
{
	char buf[NMX];
	int cmd; /* 0 - ввод закончился, 1 - ввод не кончился, 2 - выход */
	pid_t pid;
};

/* MA */
int main(int argc, char **argv1)
{
	key_t key;
	char str[NMX];
	int *isExit;
	int n;

	signal(SIGINT, handler);
	argv=argv1;
	if (argc != 3)
	{
		printf("ERROR: No args\n");
		exit(0);
	}
	n = atoi(argv[2]);
	if (n == 0 && argv[2][0] != '0')
	{
		printf("ERROR: Wrong args\n");
		exit(1);
	}
	key = ftok("/etc/passwd", 'A');
	if ((semid = semget(key, 4, 0666|IPC_CREAT)) < 0) { printf("ERROR in semget\n"); exit(1); }
	printf("Сервер готов к работе\n");
	if ((shmid = shmget(key, sizeof(struct data_st), 0666|IPC_CREAT)) < 0) { printf("ERROR in shmget\n"); exit(1); }
	shmaddr = (struct data_st *)shmat(shmid, NULL, 0);
	semctl(semid, 0, SETVAL, (int)n);
	semctl(semid, 1, SETVAL, (int)1); 
	semctl(semid, 2, SETVAL, (int)0); 
	semctl(semid, 3, SETVAL, (int)1);
/*	if ((pid = fork()) == 0)
	{
		sem.sem_op = -1;
		sem.sem_num = 3;
		semop(semid, &sem , 1);
		kill(getppid(), SIGUSR1);
		exit(0);
	}*/
/*		sem.sem_op = -1;ddddddddddddd
		sem.sem_num = 1;
		semop(semid, &sem,1);
		sem.sem_op = 1;
		sem.sem_num = 0;
		semop(semid, &sem, 1);
*/
	while (1)
	{
		int session;

		down(2);
		session = reading_client();
		if (!session)
		{
			print_data();
		}
		up(1);
	}
	return 0;
}

/* SF */
int str_fun(char *str)
{
	int i;
	char *ser = argv[1];
	int str_ctr = 0;
	char *ptr = glob_buffer;
	

/*	for (i = 0; i < strlen(str); i++)
	{
		if (str_ptr >= strlen(ser))
		{
			str_ctr++;
			str_ptr = 0;
		}
		if (str[i] == ser[str_ptr])
		{
			str_ptr++;
		}
		else
		{
			str_ptr = 0;
			if (str[i] == ser[str_ptr])
				str_ptr++;
		}
	}*/
	while ((ptr = strstr(ptr, ser)) != NULL)
	{
		str_ctr++;
		ptr += sizeof(char);
	}
	return str_ctr;
}

void handler(int sig)
{
	switch (sig)
	{
		case SIGINT:
			printf("Завершение работы сервера...\n");
			if (shmdt(shmaddr) < 0) { printf("ERROR in shmdt\n"); exit(1); }
			if (semctl(semid, 0, IPC_RMID, 0) < 0) { printf("ERROR in semctl\n"); exit(1); }
			if (shmctl(shmid, IPC_RMID, NULL) < 0) { printf("ERROR in shmctl\n"); exit(1); }
			if (glob_buffer != NULL)
				free(glob_buffer);	
			exit(0);
			break;
		default:
			break;
	}
	signal(sig, handler);
	return;
}

void down(int n)
{
	struct sembuf sem;

	sem.sem_flg = 0;
	sem.sem_op = -1;
	sem.sem_num = n;
	if (semop(semid, &sem, 1) < 0) { printf("ERROR in semop\n"); exit(1); }
	return;
}

void up(int n)
{
	struct sembuf sem;

	sem.sem_flg = 0;
	sem.sem_op = 1;
	sem.sem_num = n;
	if (semop(semid, &sem, 1) < 0) { printf("ERROR in semop\n"); exit(1); }
	return;
}

/* RC */
int reading_client()
{
	struct data_st *data = shmaddr;
	int i = 0;

	pid = data->pid;
	for (i = 0; i < NMX && shmaddr->buf[i] != '\0'; i++)
	{
		glob_buffer = (char *)realloc(glob_buffer, sizeof(char)*(buf_ctr+2));
		glob_buffer[buf_ctr++] = shmaddr->buf[i];
		glob_buffer[buf_ctr] = '\0';
	}
	if (shmaddr->cmd == 2) /* Клиент нас завершает */
	{
		printf("Клиент завершает работу сервера...\n");
		if (shmdt(shmaddr) < 0) { printf("ERROR in shmdt\n"); exit(1); }
		if (semctl(semid, 0, IPC_RMID, 0) < 0) { printf("ERROR in semctl\n"); exit(1); }
		if (shmctl(shmid, IPC_RMID, NULL) < 0) { printf("ERROR in shmctl\n"); exit(1); }
		if (glob_buffer != NULL)
			free(glob_buffer);	
		exit(0);

	}
	return shmaddr->cmd;
}

/* PD */
void print_data()
{
	printf("%d: %d\n", pid, str_fun(glob_buffer));
	printf("%s\n", glob_buffer);
	free(glob_buffer);
	glob_buffer = NULL;
	buf_ctr = 0;
	return;	
}

