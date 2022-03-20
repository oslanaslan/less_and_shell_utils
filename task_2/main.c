#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>

void itoa(int, char *);
void reverse(char *);

int main(int argc, char **argv)
{
	int father_son1[2], father_son2[2], son1_son2[2];
	int i, ball, stop, temp;
	char buffer[100];
	char *buffer1;
	int last_ball;

	pid_t pid_father, pid_son1, pid_son2;

	if (argc != 2)
	{
		printf("Something went wrong\n");
		return 1;
	}

	stop = atoi(argv[1]);
	ball = 1;
	
	if (stop == 0)
	{
		printf("Stopped on father with ball = 0\n");
		printf("Father pid is %i\n", pid_father);
		return 1;
	}

	pipe(father_son1);
	pipe(father_son2);
	pipe(son1_son2);
	
	pid_father = getpid();
	
	pid_son1 = fork();
	if (pid_son1 == -1)
	{
		printf("Could not create son1\n");
		perror("Trouble with fork\n");
		return 1;
	}

	if (!pid_son1) /* Мы в 1 сыне */
	{
		close(father_son1[1]);
		close(son1_son2[0]);
		close(father_son2[0]);
		close(father_son2[1]);
		while (1)
		{
			temp = read(father_son1[0], &ball, sizeof(ball));
			if (!temp)
			{
				printf("Reading error in son1\n");
				close(father_son1[0]);
				close(son1_son2[1]);
				return 1;
			}
			if (ball == -2)
			{
				temp = write(son1_son2[1], &ball, sizeof(ball));
				if (!temp)
				{
					close(father_son1[0]);
					close(son1_son2[1]);
					printf("Writing error in son1 when ball == stop\n");
					return 1;
				}
				close(father_son1[0]);
				close(son1_son2[1]);
				return 0;
			}
			if (ball == stop)
			{
				last_ball = ball;
				ball = -1;

				temp = write(son1_son2[1], &ball, sizeof(ball));
				if (!temp)
				{
					close(father_son1[0]);
					close(son1_son2[1]);
					printf("Writing error in son1 when ball == stop\n");
					return 1;
				}
				printf("Stopped on son1 with ball = %i\n", last_ball);
				printf("Son1 pid if %i\n", getpid());
				continue;
			}
			else
			{
				ball++;
			}
			temp = write(son1_son2[1], &ball, sizeof(ball));
			if (!temp)
			{
				close(father_son1[0]);
				close(son1_son2[1]);
				printf("Writing error in son1\n");
				return 1;
			}
		}
	}
	pid_son2 = fork();
	if (pid_son2 == -1)
	{
		printf("Could not create son2\n");
		perror("Trouble with fork\n");
		return 1;
	}

	else if (!pid_son2) /* Мы в 2 сыне */
	{
		close(son1_son2[1]);
		close(father_son2[0]);
		close(father_son1[0]);
		close(father_son1[1]);
		while (1)
		{
			temp = read(son1_son2[0], &ball, sizeof(ball));
			if (!temp)
			{
				close(son1_son2[0]);
				close(father_son2[1]);
				printf("Reading error in son2\n");
				return 1;
			}
			if (ball == -1)
			{
				temp = write(father_son2[1], &ball, sizeof(ball));
				if (!temp)
				{
					close(son1_son2[0]);
					close(father_son2[1]);
					printf("Writing error in son2 when ball was -1\n");
					return 1;
				}
				continue;
			}
			if (ball == -2)
			{
				temp = write(father_son2[1], &ball, sizeof(ball));
				if (!temp)
				{
					close(son1_son2[0]);
					close(father_son2[1]);
					printf("Writing error in son2 when ball was -1\n");
					return 1;
				}
				close(son1_son2[0]);
				close(father_son2[1]);
				return 0;
			}
			if (ball == stop)
			{
				last_ball = ball;
				ball = -1;
				temp = write(father_son2[1], &ball, sizeof(ball));
				close(son1_son2[0]);
				close(father_son2[1]);
				printf("Stopped on son2 with ball = %i\n", last_ball);
				printf("Son2 pid is %i\n", getpid());

				if (!temp)
				{
					printf("Writing error in son2 when ball == stop\n");
					return 1;
				}
				return 0;
			}
			else
			{
				ball++;
			}
			temp = write(father_son2[1], &ball, sizeof(ball));
			if (!temp)
			{
				close(son1_son2[0]);
				close(father_son2[1]);
				printf("Writing error on son2\n");
				return 1;
			}
		}
	}
	else /* Мы в отце */
	{
		close(father_son1[0]);
		close(father_son2[1]);
		close(son1_son2[0]);
		close(son1_son2[1]);

/*		ball++;  Нужно ли увеличивать при первом запуске? */

		temp = write(father_son1[1], &ball, sizeof(ball));
		if (!temp)
		{
			close(father_son1[1]);
			close(father_son2[0]);
			waitpid(pid_son1, NULL, 0); 
			waitpid(pid_son2, NULL, 0); 
			printf("Writing error in father\n");
			return 1;
		}
		ball++;
		while (1)
		{
			temp = read(father_son2[0], &ball, sizeof(ball));
			if (!temp)
			{
				close(father_son1[1]);
				close(father_son2[0]);
				waitpid(pid_son1, NULL, 0);
				waitpid(pid_son2, NULL, 0); 
				return 1;
			}
			if (ball == -1)
			{
				ball = -2;
				temp = write(father_son1[1], &ball, sizeof(ball));
				if (!temp)
				{
					close(father_son1[1]);
					close(father_son2[0]);
					waitpid(pid_son1, NULL, 0);
					waitpid(pid_son2, NULL, 0); 
					printf("Write error in father when ball == stop\n");
					return 1;
				}
				continue;
			}
			if (ball == stop)
			{
				last_ball = ball;
				ball = -2;
				temp = write(father_son1[1], &ball, sizeof(ball));
				if (!temp)
				{
					close(father_son1[1]);
					close(father_son2[0]);
					waitpid(pid_son1, NULL, 0);
					waitpid(pid_son2, NULL, 0); 
					printf("Write error in father when ball == stop\n");
					return 1;
				}

				waitpid(pid_son1, NULL, 0);
				waitpid(pid_son2, NULL, 0); 
				close(father_son1[1]);
				close(father_son2[0]);
				printf("Stopped on father with ball = %i\n", last_ball);
				printf("Father pid is %i\n", getpid());
				return 0;
			}
			if (ball == -2)
			{
				waitpid(pid_son1, NULL, 0);
				waitpid(pid_son2, NULL, 0);
				close(father_son1[1]);
				close(father_son2[0]);
				return 0;
			}
			ball ++;
			temp = write(father_son1[1], &ball , sizeof(ball));
			if (!temp)
			{
				close(father_son1[1]);
				close(father_son2[0]);
				waitpid(pid_son1, NULL, 0); 
				waitpid(pid_son2, NULL, 0); 
				printf("Writing error in father\n");
				perror("Trouble with write\n");
				return 1;
			}

		}
		
	}

	

}


/* itoa: конвертируем n в строку */
/*void itoa(int n, char s[])
{
	int i, sign;

	if ((sign = n) < 0)
		n = -n;
	i = 0;
	do 
	{
		s[i++] = n % 10 + '0';
	} 
	while ((n /= 10) > 0);
	if (sign < 0)
		s[i++] = '-';
	s[i] = '\0';
	reverse(s);
}
*/
/* reverse: переворачиваем строку s на месте */
/*void reverse(char s[])
{
	int i, j;
	char c;

	for (i = 0, j = strlen(s)-1; i < j; i++, j--)
	{
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

*/
