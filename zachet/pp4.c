#include <math.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>

/*
 * план работы:
 * 4 семафора
 * разд память
 * созд 3 проц
 * у одного сем 1
 * у других 0
 * в цикле вычитаем единицу из сема
 * по крайней мере пытаемся
 * увелич знач мяча в разд пам, если удалось
 * увелич семафор след проц
 * проц на кот остан удаляет семы
 */

int main(int argc, char* argv[]) 
{
    if (argc < 5) 
    {
        printf("Малова-то аргументов\n");
        exit(0);
    }
    struct sembuf sem0, sem1;
    int shmid, semid;
    key_t key = ftok("/etc/passwd", 'q');
    shmid = shmget(key, sizeof(int), 0666 | IPC_CREAT | IPC_EXCL);
    int* ball = shmat(shmid, NULL, 0);
    semid = semget(key, 4, 0666 | IPC_CREAT | IPC_EXCL);
    
    semctl(semid, 0, SETVAL, (int) 1);
    semctl(semid, 1, SETVAL, (int) 0);
    semctl(semid, 2, SETVAL, (int) 0);
    semctl(semid, 3, SETVAL, (int) 0);
    
    int ind;
    (*ball) = 0;

    for (int i = 1; i <= 3; ++i) 
    {
        if (fork()) 
	{
            ind = 0;
        } 
        else 
	{
            ind = i;
            break;
        }
    }

    int max = atoi(argv[ind + 1]);

    sem0.sem_num = ind;
    sem0.sem_op = -1;
    sem0.sem_flg = 0;

    sem1.sem_num = (ind + 1) % 4;
    sem1.sem_op = 1;
    sem1.sem_flg = 0;

    while (semop(semid, &sem0, 1) != -1) 
    {
        ++(*ball);
        printf("%d %d \n", *ball, ind);
        
	if ((*ball) >= max) 
	{
            semctl(semid, 0, IPC_RMID);
        }
        else 
	{
            semop(semid, &sem1, 1);
        }
    }
    
    while(wait(NULL) != -1);
    shmctl(shmid, 0, IPC_RMID);
    exit(0);
}
