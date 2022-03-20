#include <stdio.h>
#define FIRST_ARG 1

unsigned int swap(unsigned int);
unsigned int new_pow(unsigned int, unsigned int);
unsigned int getNomber(int, char **);

int main(int argc, char **argv)
{
	unsigned int a;
	unsigned int res;

	if (argc == 2)
	{
		a = getNomber(argc, argv);
		res = swap(a);

		printf("%d\n", res);
	}
	else
	{
		printf("Нет аргументов!\n");
		return 1;
	}
	return 0;
}

unsigned int swap(unsigned int n)
{
	unsigned int a = 0;
	unsigned int i;

	unsigned int b = 0;

	for (i = 0; i < sizeof(unsigned int) * 8; i+=2)
		a += new_pow((unsigned int)2,i);
	b = ~a;
	a = (n>>1) & a;
	b = (n<<1) & b;
	return (a | b);
}

unsigned int new_pow(unsigned int base, unsigned int new_power)
{
	unsigned int res = 1;
	int i;

	for (i = 0; i < new_power; i++)
		res *= base;
	return (unsigned int)res;
}

unsigned getNomber(int argc, char **argv)
{
	char c;
	int i;
	unsigned int result = 0;
	unsigned int zero = '0';

	for (i = 0; c = argv[FIRST_ARG][i], c != '\0'; i++)
	{
		result *= 10;
		result += (int)c - (int)zero;
	}
	printf("%d\n", result);
	return result;

}

