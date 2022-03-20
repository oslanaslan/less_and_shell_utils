#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

int mainFun(FILE *);
int isValid(FILE *);
int printList(uint64_t *, uint64_t *, uint64_t);

int main(int argc, char **argv)
{
	FILE *stream;

	if (argc != 2)
	{
		printf("Введите путь!\n");
		return 1;
	}
	else
	{
		stream = fopen(argv[1], "rb");
		if (stream == NULL) 
		{
			printf("Неверное имя файла\n");
			return 1;
		}
		mainFun(stream);
		fclose(stream);
		return 0;
	}
}

int mainFun(FILE *stream)
{
	uint64_t n = 0;
	uint64_t m = 0;
	double value;
	double max;
	double min;
	uint64_t i;
	uint64_t j;
	uint64_t *maxsx;
	uint64_t *maxsy;
	uint64_t *minsx;
	uint64_t *minsy;
	uint64_t minc;
	uint64_t maxc;
	uint64_t posX;
	uint64_t posY;
	int flag = 1;

	if (isValid(stream))
	{
		printf("File is broken\n");
		return 1;
	}

	fread(&n, sizeof(uint64_t), 1, stream);
	fread(&m, sizeof(uint64_t), 1, stream);
	fread(&value, sizeof(double), 1,stream);
	if (n == EOF || m == EOF || value == EOF)
	{
		printf("Unexpected end of file\n");
		return 1;
	}
	printf("%f ", value);	
	max = value;
	min = value;
	maxsx = (uint64_t *)malloc(sizeof(uint64_t));
	maxsy = (uint64_t *)malloc(sizeof(uint64_t));
	minsx = (uint64_t *)malloc(sizeof(uint64_t));
	minsy = (uint64_t *)malloc(sizeof(uint64_t));
	maxc = 0;
	minc = 0;
	minsx[minc] = 0;
	minsy[minc] = 0;
	maxsx[maxc] = 0;
	maxsy[maxc] = 0;

	for (i = 0; i< n; i++)
	{
		for (j = 0; j<m; j++)
		{
			if (flag == 1)
			{
				flag = 0;
				continue;
			}
			fread(&value, sizeof(double), 1, stream);
			if (value == EOF)
			{
				printf("Unexpected end of file\n");
				return 1;
			}
			if (value == max)
			{
				maxc++;
				maxsx = (uint64_t *)realloc(maxsx, sizeof(uint64_t)*maxc);
				maxsy = (uint64_t *)realloc(maxsy, sizeof(uint64_t)*maxc);
				maxsx[maxc] = i;
				maxsy[maxc] = j;
			}
	
			if (value > max)
			{
				max = value;
				free(maxsx);
				free(maxsy);
				maxsx = (uint64_t *)malloc(sizeof(uint64_t));
				maxsy = (uint64_t *)malloc(sizeof(uint64_t));
				maxc = 0;
				maxsx[maxc] = i;
				maxsy[maxc] = j;
	
			}
	
			if (value == min)
			{
				minc++;
				minsx = (uint64_t *)realloc(minsx, sizeof(uint64_t)*minc);
				minsy = (uint64_t *)realloc(minsy, sizeof(uint64_t)*minc);
				minsx[minc] = i;
				minsy[minc] = j;
			}
	
			if (value < min)
			{
				min = value;
				free(minsx);
				free(minsy);
				minsx = (uint64_t *)malloc(sizeof(uint64_t));
				minsy = (uint64_t *)malloc(sizeof(uint64_t));
				minc = 0;
				minsx[minc] = i;
				minsy[minc] = j;
			}
	
			printf("%f ", value);
		}
		printf("\n");
	}
	printf("\nMax is %f\n", max);
	printList(maxsx, maxsy, maxc);
	printf("Min is %f\n", min);
	printList(minsx, minsy, minc);

	return 0;


}

int isValid(FILE *stream)
{
	char *type = "MATRIX";
	char *str;
	int i;
	int flag = 0;

	for (i = 0; i < 6; i++)
	{
		if (type[i] != fgetc(stream))
		{
			flag = 1;
			break;
		}
	}
	return flag;
}

int printList(uint64_t *mtrx, uint64_t *mtry, uint64_t last)
{
	uint64_t posX, posY,i;

	for (i = 0; i <= last; i++)
	{
		posX = mtrx[i] + 1;
		posY = mtry[i] + 1;
		printf("( %u ; %u )\n", posX, posY);
	}
	return 0;
}

