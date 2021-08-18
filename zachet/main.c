#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

/* type: Возможные типы сообщения
 * 1 - 1 процесс
 * 2 - 2 процесс
 * 3 - 3 процесс
 * 4 - 4 процесс
 * 6 - получить номер
 *
 * cmd: Возможные типы команд
 * 1 - продолжить работу
 * 2 - выход 
 */

void handler(int sig);

typedef struct 
{
	long type;
	unsigned int ball;
	int cmd;
} Msg;

int msgid;
/* MA */
int main(int argc, char **argv)
{
	Msg msg;
	unsigned int n, ball, is_first;
	key_t key;
	int type, w_type;
	
	if (argc != 2 || (atoi(argv[1]) == 0 && argv[1][0] != '0'))
	{
		printf("ERROR\n");
		exit(0);
	}
	signal(SIGINT, handler);
	key = ftok("/etc/passwd", 'A');
	is_first = 1;
	if ((msgid = msgget(key, 0666|IPC_CREAT|IPC_EXCL)) < 0)
	{
		if ((msgid = msgget(key, 0666)) < 0)
		{
			printf("ERROR: Something went wrong\n");
			exit(-1);
		}
		is_first = 0;
	}
	n = atoi(argv[1]);
	if (n == 0)
	{
		printf("1 - 0\n");
		msgctl(msgid, IPC_RMID, NULL);
		exit(0);
	}
	if (is_first)
	{
		ball = 0;
		msg.cmd = 2;
		msg.type = 6;
		if (msgsnd(msgid, &msg,sizeof(Msg) - sizeof(long),  0) < 0)
		{
			printf("ERROR\n");
			{
				msgctl(msgid, IPC_RMID, NULL);
			}

		}
		ball = 0;
		msg.cmd = 3;
		msg.type = 6;
		if (msgsnd(msgid, &msg, sizeof(Msg) - sizeof(long), 0) < 0)
		{
			printf("ERROR\n");
			{
				msgctl(msgid, IPC_RMID, NULL);
			}

		}
		ball = 0;
		msg.cmd = 4;
		msg.type = 6;
		if (msgsnd(msgid, &msg, sizeof(Msg) - sizeof(long), 0) < 0)
		{
			printf("ERROR\n");
			{
				msgctl(msgid, IPC_RMID, NULL);
			}

		}
		ball = 0;
		msg.cmd = 1;
		msg.type = 1;
		if (msgsnd(msgid, &msg,sizeof(Msg)- sizeof(long),  0) < 0)
		{
			printf("ERROR\n");
			{
				msgctl(msgid, IPC_RMID, NULL);
			}

		}
		type = 1;
	}
	else
	{
		if (msgrcv(msgid, &msg, sizeof(Msg) - sizeof(long), 6, IPC_NOWAIT) < 0)
		{
			printf("ERROR: Возможно в игре уже есть 4 процесса\n");
			{
				msgctl(msgid, IPC_RMID, NULL);
			}

			exit(-1);
		}
		type = msg.cmd;
	}
	while (1)
	{
		if (type == 4)
		{
			w_type = 1;
		}
		else
			w_type = type+1;
		if (msgrcv(msgid, &msg, sizeof(Msg) - sizeof(long), type, 0) < 0)
		{
			{
				msgctl(msgid, IPC_RMID, NULL);
			}

			exit(-1);
		}
		if (msg.cmd == 2)
		{
			{
				msgctl(msgid, IPC_RMID, NULL);
			}
			exit(0);
		}
		ball = msg.ball;
		printf("%d - %d\n", type, ball);
		if (ball >= n)
		{
			int i;
			for (i = 1; i < 5; i++)
			{
			msg.cmd = 2;
			msg.type = i;
			if (msgsnd(msgid, &msg, sizeof(Msg) - sizeof(long), 0) < 0)
			{
				
				msgctl(msgid, IPC_RMID, NULL);
			}
			}		
			{
				msgctl(msgid, IPC_RMID, NULL);
			}
			exit(0);
		}
		else
		{
			ball++;
			msg.ball = ball;
			msg.cmd = 1;
			msg.type = w_type;
			if (msgsnd(msgid, &msg, sizeof(Msg) - sizeof(long), 0) < 0)
			{
				{
					msgctl(msgid, IPC_RMID, NULL);
				}
				exit(0);
			}

		}
	}
	return 0;
	
}

void handler(int sig)
{
	msgctl(msgid, IPC_RMID, NULL);
	signal(SIGINT, handler);
}
