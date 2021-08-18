#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc, char **argv)
{
	FILE *stream;
	int i;
	const char *str = {"MATRIX\0"};
	uint64_t n = 10;
	uint64_t m = 10;
	double a = 1.0;

	if (argc < 2) 
	{
	       printf("NO ARGS\n");	
		return 1; 
	}
	else
	{
		stream = fopen(argv[1], "wb");
		fwrite(str, sizeof(char), 6, stream);
		fwrite(&n, sizeof(uint64_t), 1, stream);
		fwrite(&m, sizeof(uint64_t), 1, stream);
		for (i = 0; i < n*m; i++)
		{
			fwrite(&a, sizeof(double), 1, stream);
			a += 1;
		}
	}	
	fclose(stream);
	return 0;

}

