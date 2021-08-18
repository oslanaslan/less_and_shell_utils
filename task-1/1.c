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
	size_t temp[3];
	double value;
	double max;
	double min;
	uint64_t i;
	uint64_t j;
	uint64_t *maxsx = NULL;
	uint64_t *maxsy = NULL;
	uint64_t *minsx = NULL;
	uint64_t *minsy = NULL;
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

	temp[0] = fread(&n, sizeof(uint64_t), 1, stream);
	temp[1] = fread(&m, sizeof(uint64_t), 1, stream);
	temp[2] = fread(&value, sizeof(double), 1,stream);
	if (temp[0] == 0 || temp[1] == 0 || temp[2] == 0)
	{
		printf("Unexpected end of file\n");
		return 1;
	}
	printf("%f ", value);	
	max = value;
	min = value;

	while (maxsx == NULL)
		maxsx = (uint64_t *)malloc(sizeof(uint64_t));
	while (maxsy == NULL)
		maxsy = (uint64_t *)malloc(sizeof(uint64_t));
	while (minsx == NULL)
		minsx = (uint64_t *)malloc(sizeof(uint64_t));
	while (minsy == NULL)
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
			uint64_t f_ret;
			if (flag == 1)
			{
				flag = 0;
				continue;
			}

			f_ret = fread(&value, sizeof(double), 1, stream);


			if (f_ret == 0)
			{
				printf("\nUnexpected end of file\n");
				printf("Your file is broken\n");
				return 1;
			}
			if (value == max)
			{
				uint64_t *tempp = NULL;
				maxc++;

				while (tempp == NULL)
					tempp = (uint64_t *)realloc(maxsx, sizeof(uint64_t)*maxc);
				maxsx = tempp;

				tempp = NULL;
				while (tempp == NULL)
					tempp = (uint64_t *)realloc(maxsy, sizeof(uint64_t)*maxc);
				maxsy = tempp;

				maxsx[maxc] = i;
				maxsy[maxc] = j;
			}
	
			if (value > max)
			{
				uint64_t *tempp = NULL;

				max = value;
				free(maxsx);
				free(maxsy);

				while (tempp == NULL)
					tempp = (uint64_t *)malloc(sizeof(uint64_t));
				maxsx = tempp;

				tempp = NULL;
				while (tempp == NULL)
					tempp = (uint64_t *)malloc(sizeof(uint64_t));
				maxsy = tempp;

				maxc = 0;
				maxsx[maxc] = i;
				maxsy[maxc] = j;
	
			}
	
			if (value == min)
			{
				uint64_t *tempp;

				minc++;
				while (tempp == NULL)
					tempp = (uint64_t *)realloc(minsx, sizeof(uint64_t)*minc);
				minsx = tempp;

				tempp = NULL;
				while (tempp == NULL)
					tempp = (uint64_t *)realloc(minsy, sizeof(uint64_t)*minc);
				minsy = tempp;

				minsx[minc] = i;
				minsy[minc] = j;
			}
	
			if (value < min)
			{
				uint64_t *tempp = NULL;

				min = value;
				free(minsx);
				free(minsy);

				while (tempp == NULL)
					tempp = (uint64_t *)malloc(sizeof(uint64_t));
				minsx = tempp;

				tempp = NULL;
				while (tempp == NULL)
					tempp = (uint64_t *)malloc(sizeof(uint64_t));
				minsy = tempp;

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

