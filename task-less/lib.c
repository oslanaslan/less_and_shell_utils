#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "header.h"


/* Двусвязный список, который будет содержать исходный текст */

/* Добавляет строку в конец списка */
int list_append(Bidirect_list *main_list, char *buffer)
{
	struct list_node *last_node = main_list->tail;
	int n,i;
	struct list_node *new_node;
	char *strstr;

	new_node = (struct list_node *)malloc(sizeof(struct list_node));
	if (new_node == NULL)
	{
		printf("Malloc error\n");
		exit(1);
	}
	strstr = (char *)malloc(strlen(buffer));
	if (strstr == NULL)
	{
		printf("Malloc error\n");
		exit(1);
	}
	
/*
	n = strlen(buffer);
	for (i = 0; i < n; i++)
	{
		strstr[i] = buffer[i];
	} */
	strcpy(strstr, buffer);

	if (strlen(strstr) > main_list->max_len_str)
		main_list->max_len_str = strlen(strstr);

	new_node->str = strstr;
	if (last_node != NULL)
	{
		new_node->prev = last_node;
		last_node->next = new_node;
		new_node->next = NULL;
		main_list->tail = new_node;
		main_list->num_elements++;
	}
	else
	{
		main_list->tail = new_node;
		main_list->head = new_node;
		main_list->num_elements = 1;
	}
	return 0;
}

/* Переводит терминал в режим. Возвращает структуру
 * содержащую структуру данных, содержащих
 * старые параметры терминала 
 * */
struct termios turnOnTerm()
{
	struct termios old_attributes, new_attributes;
	
	tcgetattr(0, &old_attributes);
	memcpy(&new_attributes, &old_attributes, sizeof(struct termios));
	new_attributes.c_lflag &= ~ECHO;
	new_attributes.c_lflag &= ~ICANON;
	new_attributes.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_attributes);
	return old_attributes;
}

/* Возвращает терминал к первоначальным настройкам.
 * Получаем старые параметры
 * */
int turnOffTerm(struct termios old_attributes)
{
	int tmp;

	tcsetattr(0, TCSANOW, &old_attributes);
	
	return 1;	
}

/* Получить размеры окна */
void getTermSize(int *ss)
{
	struct winsize ws;

	ioctl(0, TIOCGWINSZ, &ws);
	ss[1] = ws.ws_row;
	ss[0] = ws.ws_col;
	return;
}

/* Отчистить терминал */
void clear()
{
	int s[2];
	int i = 0;

	getTermSize(s);
	
	for (i = 0; i <= s[0]; i++)
		printf("\n");

	return;
}

int list_print(Bidirect_list *list, int need_num, int *curs_x, int *curs_y, int max_left, int first, int *cur_str_len)
{
	int n,i, ctr;
	int win_size[2];
	struct list_node *node;

	node = list->head;
	for (i = 0; i< first; i++)
	{
		node = node->next;
	}
	n = list->num_elements;
	for (ctr = first; node != NULL; ctr++)
	{
		char *pr_str;
		int need_curs;

		if (ctr == (*curs_y))
		{
			need_curs = (*curs_x);
			(*cur_str_len) = strlen(node->str);
		}
		else
			need_curs = -1;
		if (need_num != -1)
			need_num = ctr;

		pr_str = get_str_for_pr(node->str, need_num, n, need_curs, max_left, curs_x);
		
		printf("%s\n", pr_str);

		node = node->next;
		free(pr_str); /* потому что в get_str_for_pr используется malloc */
	}

	getTermSize(win_size);
	for (;ctr < (win_size[1] - 1); ctr++)
		printf("\n");

	return 0;
}

Bidirect_list *list_init()
{
	Bidirect_list *list = (Bidirect_list *)malloc(sizeof(Bidirect_list));
	if (list == NULL)
	{
		printf("Malloc error\n");
		exit(1);
	}
	list->head = NULL;
	list->tail = NULL;
	list->num_elements = 0;
	list->max_len_str = 0;
	return list;
}

int list_remove(Bidirect_list *list)
{
	struct list_node *node = list->head;
	
	while (node != NULL)
	{
		free(node->str);
		node = node->next;
		free(node->prev);
	}
	return 0;
}

char *get_str_for_pr(char *str, int need_num, int n, int need_curs, int max_left, int *cur_loc_x)
{
	int win_size[2];
	char buffer1[40];
	char *buffer2 = "\033[7m";
	char *buffer3 = "\033[27m";

	char *res;
	int i = 0;
	int l = 0;

	getTermSize(win_size);
	/* Выделяем памаять под строку с запасом (с учетом длины esc посл */
	res = (char *)malloc(sizeof(char) * (win_size[0] + 20));
	if (res == NULL)
	{
		printf("Malloc error in get_str_for_pr\n");
		exit(1);
	}
	
	if (need_num != -1)
	{
		int m;
		
		sprintf(buffer1, "%d", n);
		m = strlen(buffer1);
		sprintf(buffer1, "%d", need_num);
		while (buffer1[i] != '\0')
		{
			res[i] = buffer1[i];
			i++;
		}

		while (i < m)
		{
			res[i] = ' ';
			i++;
		}

		res[i++] = ' ';
		res[i++] = ':';
/*		res[i++] = ' '; */
	}

	if (max_left != 0)
		res[i++] = '<';
	else
		res[i++] = '|';
	
	if (need_curs == -1)
	{
		while (i < win_size[0] && str[max_left+l] != '\0')
		{
			res[i] = str[max_left+l];
			i++;
			l++;
		}
	}
	else
	{
		int k;

		for (;(l<(max_left + need_curs)) && (str[max_left + l+1] != '\0') ; i++, l++)
			res[i] = str[max_left+l];

/*		if (str[max_left + l] == '\0')
		{
			res[i] = '\0';
			return res;
		} */

		for (k = 0; buffer2[k] != '\0'; k++)
		{
			res[i] = buffer2[k];
			i++;
		}
		res[i++] = str[max_left+l];
		l++;
		for (k = 0; buffer3[k] != '\0'; k++)
		{
			res[i] = buffer3[k];
			i++;
		}
		if (str[max_left + l] == '\0')
		{
			res[i] = '\0';
/*			printf("%d - %d\n", (i-14*sizeof(char)), (*cur_loc_x)); */
			(*cur_loc_x) = i - 14*sizeof(char);
			return res;
		}
		for (;(i < win_size[0]) && (str[max_left + l] != '\0'); l++)
		{
			res[i++] = str[max_left + l];

			printf("%d %d %d\n", i, win_size[0], win_size[1]);
		}

	}
	res[i] = '\0';
	return res;
}

char *screen_handler(int key, int *cur_loc_x, int *cur_loc_y, int* max_left, int *first, Bidirect_list *list, int need_num, int *cur_str_len)
{
	int tmp;
	int win_size[2];

	getTermSize(win_size);

	if (key == 65)
	{
		if ((*cur_loc_y) == 0)
		{
			if ((*first) > 0)
			{
				(*first) -= 1;
			}
		}
		else
		{
			(*cur_loc_y) -= 1;
		}	
	}
	else if (key == 66)
	{
		if ((*cur_loc_y) == (win_size[1] - 2) && (list->num_elements > ((*first)+win_size[1])))
		{
			(*first)++;
		}
		else if (list->num_elements > ((*cur_loc_y) + (*first)+1))
		{
			(*cur_loc_y) ++;
		}

	}
	else if (key == 68)
	{
		if ((*cur_loc_x) == 0)
		{
			if ((*max_left) != 0)
				(*max_left)--;
		}
		else
		{
			(*cur_loc_x)--;
		}
	}
	else if (key == 67)
	{
		if (need_num == -1)
		{
			if ((*cur_loc_x) == (win_size[0] - 1))
			{
				if (list->max_len_str > ((*max_left) + (*cur_loc_x) + 1))
					(*max_left)++;
			}
			else if ((*cur_str_len) -1 > (*cur_loc_x)) 
			{
				(*cur_loc_x)++;
			}
		}
		else
		{

		}
	}
	else if (key == 58)
	{
		printf("Not ready yet\n");
		exit(1);
	}
	
	tmp = list_print(list, need_num, cur_loc_x, cur_loc_y, (*max_left), (*first), cur_str_len);

	return NULL;
}


int getKeyPress()
{
	int key;
	int i = 0;
	char symbol;
	
	symbol = getchar();
	{
		printf("got symbil %c\n",symbol);
		if (symbol == (char)21)
		{
			symbol = getchar();
			symbol = getchar();
			key = 0 - symbol;
		}
		else
		{
			key = symbol;
		}
		
	}
	return key;
}
