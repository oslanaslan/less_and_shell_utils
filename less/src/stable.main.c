#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#define SPACES_CNT 4

/* Переменные */

int cur_loc_x = 0;
int cur_loc_y = 0;
int cur_str_max_len = 0;
int cur_first_node = 0;
int max_num_node = 0;
int max_left = 0;
int loc_str_tail = 0;
int prev_str_len = 0;
int next_str_len = 0;
int step_for_scr_hnd = 0;
char *cur_start;
char *cur_stop;
char *search_text = NULL;
int search_ctr = 0;
void **malloc_array;
long malloc_array_num = 0;
char *file_name;
struct termios old_attributes;

int isStart = 0;

/* Структуры */
struct list_node
{
	char *str;
	struct list_node *prev;
	struct list_node *next;
};

typedef struct 
{
	size_t max_len_str; 
	size_t num_elements;
	struct list_node *head;
	struct list_node *tail;
} Bidirect_list;

struct malloc_node
{
	void *ptr;
	struct malloc_node *prev;
	struct malloc_node *next;
};

typedef struct
{
	size_t num_elements;
	struct malloc_node *head;
	struct malloc_node *tail;
} Malloc_list;

Malloc_list *malloc_list;
Malloc_list *free_list;
Bidirect_list *main_list;

/* Функции */

/* Добавить строку в конец списка */
int list_append(Bidirect_list *main_list, char *buffer);
/* Вывод строки на экран */
void list_print(Bidirect_list *list, int need_num);
void list_remove(Bidirect_list *list);
/* Перевести терминал в рабочий режим */
struct termios turnOnTerm();
/* Перевести терминал в исходный режим */
int turnOffTerm(struct termios old_attributes);
/* Получить размер окна */
void getTermSize(int *x_y);
/* Очистить терминал */
void clear();
/* Создает пустой список */
Bidirect_list *list_init();
/* Создает строку, удобную для вывода в list_print */
char *get_str_for_pr(int need_num);
/* Управляет выводом на экран */
void screen_handler(int key, Bidirect_list *list, int need_num);
/* Возвращате массив unsignt int, содержащий коды нажатой клавиши */
int getKeyPress();
char *str_push(char *str, char *what, int where);
char *str_cut(char *from, char *to);
int read_from_file(Bidirect_list *list, char *name);
char *get_command(Bidirect_list *list, int need_num, char sym);
/*struct search_res *search(Bidirect_list *list, char *text); */
int *search(Bidirect_list *list, char *str, int which);

int find_text(Bidirect_list *list, int need_num);


void save_file(Bidirect_list *list, char *name);

void *my_malloc(size_t size);
void *my_realloc(void *ptd, size_t size);
void garbage_collector();
void exit_prog();
void my_subst(Bidirect_list *list, char *str1, char *str2);
char *my_strcat(char *str1, char *str2);
void my_free(void *ptr);
char **get_str_for_subst(char *command, char *str1, char *str2);
void start();
int my_itoa(char *str);
void goto_x_y(int x, int y);
int rus_let_ctr(char *str);
/* Загрузка */
void loading(int *par);

/*MA*/

int main(int argc, char **argv)
{
	int key = 0;

	start();
	if (argc != 1)
	{
		read_from_file(main_list, argv[1]);
		file_name = argv[1];
	}
	else
		exit_prog();
	printf("here\n");
	cur_str_max_len = (int)strlen(main_list->head->str);
	max_num_node = main_list->num_elements;
	
	old_attributes = turnOnTerm();
	screen_handler(0, main_list, 0); 

	while ((key = getKeyPress()) != (int)'\004')
	{
		screen_handler(key, main_list, 0);
	}
	exit_prog();
	return 0;
}

/*LA*/

/* Добавляет строку в конец списка */
int list_append(Bidirect_list *main_list, char *buffer)
{
	int i;
	struct list_node *new_node;
	char *strstr;
	
/*	printf("starting list_append\n"); */

	new_node = (struct list_node *)my_malloc(sizeof(struct list_node));
	if (new_node == NULL)
	{
		printf("my_malloc error\n");
		exit_prog();
	}
	strstr = (char *)my_malloc(sizeof(char)*strlen(buffer));
	if (strstr == NULL)
	{
		printf("my_malloc error in list_append\n");
		exit_prog();
	}
	
/*	strcpy(strstr, buffer); */
	strstr = (char *)my_malloc((2+strlen(buffer))*sizeof(char));
	for (i = 0; i < (int)strlen(buffer); i++)
		strstr[i] = buffer[i];
	strstr[strlen(buffer)] = '\0';

	if (strlen(strstr) > main_list->max_len_str)
		main_list->max_len_str = (int)strlen(strstr);

	new_node->str = strstr;
	main_list->num_elements++;
	if (main_list->tail != NULL)
	{
		new_node->next = NULL;
		new_node->prev = main_list->tail;
		main_list->tail->next = new_node;
		main_list->tail = new_node;
	}
	else
	{
		new_node->next = NULL;
		new_node->prev = NULL;
		main_list->tail = new_node;
		main_list->head = new_node;
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

/*PL*/

void list_print(Bidirect_list *list, int need_num)
{
	int i, ctr;
	int win_size[2];
	struct list_node *node;
	
/*	printf("starting list_print\n"); */

/*	printf("%d %d \n", cur_loc_x, cur_loc_y); */
/*	clear(); */
	printf("\n");
	getTermSize(win_size);
	node = list->head;
	for (i = 0; i< cur_first_node; i++)
	{
		node = node->next;
	}
/*	printf("%d %d %p\n", cur_first_node, cur_loc_y, node->prev); */
	for (ctr = cur_first_node; (node != NULL) && (ctr < win_size[1] + cur_first_node- 1); ctr++)
	{
		char *pr_str;
		int need_curs;

/*		printf("circle started\n");  */
		if (ctr == cur_loc_y)
		{
			need_curs = 1;
			cur_str_max_len = (int)strlen(node->str) - rus_let_ctr(node->str);
			loc_str_tail = cur_str_max_len - cur_loc_x - max_left;
			
			if (node->next != NULL)
				next_str_len = (int)strlen(node->next->str) - rus_let_ctr(node->next->str);
			else
				next_str_len = (int)strlen(node->str) - rus_let_ctr(node->str);
			if (node->prev != NULL)
				prev_str_len = (int)strlen(node->prev->str) - rus_let_ctr(node->prev->str);
			else
				prev_str_len = (int)strlen(node->str) - rus_let_ctr(node->str);
			if (next_str_len == 0)
				next_str_len = 1;
			if (prev_str_len == 0)
				prev_str_len = 1;
		}
		else
			need_curs = -1;

		if (need_num != -1)
			need_num = ctr;

		pr_str = get_str_for_pr(need_num);
		step_for_scr_hnd = (int)strlen(pr_str); /* чтобы понять зачем, см код screen_handler */
		
		printf("%s", pr_str);

		if ((int)strlen(node->str) < max_left) /* Чтобы не лезть в чужую память */
		{
			printf("\n");
		}
		else
		{
			int j =0;
			for (i = 0; i < max_left; i++)
			{
				j++;
				if ((node->str)[j-1] < 0)
					j++;
			}

			if (need_curs != -1)
			{
				if (strlen(node->str) == 0)
					printf("%s %s", cur_start, cur_stop);
				else
				{
					for (i = 0; i < cur_loc_x; i++)
					{
						printf("%c", (node->str)[j++]);
						if ((node->str)[j-1] < 0)
						{
							printf("%c", (node->str)[j++]);
						}
					}
					printf("%s%c", cur_start, (node->str)[j++]);
					if ((node->str)[j-1] < 0)
					{
						printf("%c%s", (node->str)[j++], cur_stop);
					}
					else
					{
						printf("%s", cur_stop);
					}
					for (i = cur_loc_x; (i < (win_size[0] - (int)strlen(pr_str) - 1)) && ((node->str)[j] != '\0'); i++)
					{
						printf("%c", (node->str)[j++]);
						if ((node->str)[j-1] < 0)
						{
							printf("%c", (node->str)[j++]);
						}
					}
				}
				printf("\n");
			}
			else
			{
				if (strlen(node->str) == 0)
					printf(" ");
				else
				{
					for (i = 0; (i < win_size[0] - (int)strlen(pr_str)) && ((node->str)[j] != '\0'); i++)
					{
						printf("%c", (node->str)[j++]);
						if ((node->str)[j-1] < 0)
						{
							printf("%c", (node->str)[j++]);
						}
					}
				}
				printf("\n");
/*				printf("%d %d %d\n", i, win_size[0], (int)strlen(pr_str)); */
			}
		}

		node = node->next;

	}

	getTermSize(win_size);
	for (;ctr < (win_size[1] - 1); ctr++)
		printf("\n"); 
	
/*	printf("end print list\n"); */

	return;
}

Bidirect_list *list_init()
{
	Bidirect_list *list = (Bidirect_list *)my_malloc(sizeof(Bidirect_list));
	if (list == NULL)
	{
		printf("my_malloc error\n");
		exit_prog();
	}
	list->head = NULL;
	list->tail = NULL;
	list->num_elements = 0;
	list->max_len_str = 0;
	return list;
}

void list_remove(Bidirect_list *list)
{
	struct list_node *node = list->head;
	struct list_node *n_node = list->head;
	while (node != NULL)
	{
		n_node = node->next;
		node = n_node;
	}
	list->max_len_str = 0;
	list->head = NULL;
	list->tail = NULL;
	list->num_elements = 0;
	return;
}

/*GS*/

char *get_str_for_pr(int need_num)
{
	int win_size[2];
	char *buffer1, *buffer2, *buffer3;
	
/*	printf("starting get_str_for_pr\n");  */

	getTermSize(win_size);

	if (need_num != -1)
	{
		int i,n;

		
		buffer1 = (char *)my_malloc((max_num_node +1)*sizeof(char));
		if (buffer1 == NULL)
		{
			printf("my_malloc error in get_str_for_pr \n");
			exit_prog();
		}

		buffer2 = (char *)my_malloc((max_num_node+1)*sizeof(char));
		if (buffer2 == NULL)
		{
			printf("my_malloc error in get_str_for_pr\n");
			exit_prog();
		}

		sprintf(buffer1, "%d :<", max_num_node);
		n = (int)strlen(buffer1);
		sprintf(buffer2, "%d", need_num);
		for (i = 0; i < (int)strlen(buffer2); i++)
			buffer1[i] = buffer2[i];
		for (i = (int)strlen(buffer2); i < n-3; i++)
			buffer1[i] = ' ';
		if (!max_left)
			buffer1[n-1] = '|';
		
		return buffer1;
	}
	else
	{
		buffer3 = (char *)my_malloc(sizeof(char)*2);

		if (!max_left)
			buffer3[0] = '|';
		else
			buffer3[0] = '<';
		buffer3[1] = '\0';

		return buffer3;
		
	}
}

/*SH*/

void screen_handler(int key, Bidirect_list *list, int need_num)
{
	int win_size[2];

/*	printf("starting screen_handler\n");*/ 
	
	getTermSize(win_size);
	printf("%d %d %d %d %d\n", cur_loc_x, cur_loc_y, cur_str_max_len, prev_str_len, next_str_len);
	if (key == 65) /* UP */
	{
		if (cur_loc_y - cur_first_node == 0)
		{
			if (cur_first_node > 0)
			{
				cur_first_node -= 1;
				cur_loc_y--;
			}
		}
		else
		{
			cur_loc_y -= 1;
		}

		if (prev_str_len - 1 - max_left < cur_loc_x)
		{
			if (max_left >= prev_str_len)
				max_left = prev_str_len -1;
			cur_loc_x = prev_str_len - 1 - max_left;	
		}
		list_print(list, need_num);
		printf("%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 

	}
	else if (key == 66) /* DOWN */
	{
		if (cur_loc_y - cur_first_node == (win_size[1] - 2) && ((int)list->num_elements >= (cur_first_node + win_size[1])))
		{
			cur_first_node++;
			cur_loc_y++;
		}
		else if (cur_loc_y + 2 < cur_first_node + win_size[1] && (int)list->num_elements > cur_loc_y +1)
		{
			cur_loc_y ++;
		}
		if (next_str_len - 1 - max_left < cur_loc_x)
		{
			if (max_left >= next_str_len)
				max_left = next_str_len - 1;
			cur_loc_x = next_str_len - 1 - max_left;
		}
		list_print(list, need_num);
		printf("%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 

	}
	else if (key == 68) /* LEFT */
	{

		if (cur_loc_x == 0)
		{
			if (max_left != 0)
			{
				max_left--;
			}
		}
		else
		{
			cur_loc_x--;
		}
		list_print(list, need_num);
		printf("%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 

	}
	else if (key == 67) /* RIGHT */
	{
		if (loc_str_tail - 1 <= 0)
		{
		}
		else if (cur_loc_x == win_size[0] - step_for_scr_hnd - 1)
		{
			max_left++;
		}
		else
		{
			cur_loc_x++;
		}
		list_print(list, need_num);
		printf("%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 

	}
	else if (key == 58) /* : */
	{
		char *command;
		int n;

		cur_start = "";
		cur_stop = "";		
		list_print(list, need_num);

		command = get_command(list, need_num, ':');
		cur_start = "\033[7m";
		cur_stop = "\033[27m";

		if (command == NULL)
		{
			list_print(list, need_num);
			printf("%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
			return;
		}

		if ((n = my_itoa(command)) != -1)
		{
			if (n < 0)
				n = 0;
			if (n > (int)list->num_elements-1)
				n = list->num_elements-1;
			goto_x_y(cur_loc_x + max_left, n);
			list_print(list, need_num);
			printf("%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
	
		}
		else if (strstr(command, "write") == command && strstr(command, " ") != NULL)
		{
			char *name;

			if (strlen(command) <= (int)strlen("write "))
				name = file_name;
			else
				name = command + sizeof(char)*strlen("write ");
			save_file(list, name);
			list_print(list, need_num);
			printf("Файл сохранен");
		}
		else if (strstr(command, "subst ") == command)
		{
			char *str1, *str2;
			char **tmp;

			str1 = NULL; 
			str2 = NULL;
			printf("comm: '%s'\n", command);
			tmp = get_str_for_subst(command, str1, str2);
			if (tmp == NULL)
			{
				list_print(list, need_num);
				printf("Неверный формат команды");
			}
			else
			{
				str1 = tmp[0];
				str2 = tmp[1];
				my_subst(list, str1, str2);
				list_print(list, need_num);
				printf("Заменено");

			}

		}
		else
		{
			list_print(list, need_num);
			printf("Неизвестная команда"); 
		} 

	}
	else if (key == 47) /* / */
	{
		char *text;

/*		cur_start = ""; 
		cur_stop = "";*/
		list_print(list, need_num);
		text = get_command(list, need_num, '/');
/*		cur_start = "\033[7m";
		cur_stop = "\033[27m"; */
		printf("here text '%s'\n", text);
		if (text != NULL)
		{
			int *ans;

			if (text[0] == '\0')
			{
				if (search_text == NULL)
				{
					list_print(list, need_num);
					printf("Нет истории поиска");
					return;
				}
				else
				{
					text = search_text;
				}
					
			}
			else
			{

				search_text = text;
				search_ctr = 0;
			}
			ans = search(list, search_text, search_ctr);
			if (ans[0] != -1 && ans[1] != -1)
			{
				goto_x_y(ans[0], ans[1]);

				list_print(list, need_num);
				printf("%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
			}
			else
			{
				list_print(list, need_num);
				printf("Не найдено");
			}
		}
		else
		{
			list_print(list, need_num);
			printf("%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
		}
	}
	else if (key == 'n') /* n */
	{
		int *ans;

		if (search_text == NULL)
		{
			list_print(list, need_num);
			printf("Нет истории поиска");
			return;
		}
		else
		ans = search(list, search_text, search_ctr);
		if (ans[0] != -1 && ans[1] != -1)
		{
			goto_x_y(ans[0], ans[1]);

			list_print(list, need_num);
			printf("%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
		}
		else
		{
			list_print(list, need_num);
			printf("Не найдено");
		}

	}
	else if (key == 'q') /* q */
	{
		clear();
		printf("Exiting...\n");
		exit_prog();
	}
	else /* SOMETHING ELSE */
	{
		list_print(list, need_num);
		printf("%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 

	}

	
	return;
}


int getKeyPress()
{
	int key;
	char symbol;

/*	printf("starting getKeyPress\n"); */
	
	symbol = getchar();
	{
/*		printf("got symbil %c\n",symbol); */
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

char *str_cut(char  *from, char *to)
{
	char *new_str;
	int i;

/*	printf("starting str_cut\n");  */

	new_str = (char *)my_malloc(sizeof(char));
	if (new_str == NULL)
	{
		printf("my_malloc error in cat_str\n");
		exit_prog();
	}
/*	printf("%c %d %d\n", str[to-1], from, to); */

	for (i = 0; from  < to; from+=sizeof(char))
	{
		new_str[i++] = from[0];
		new_str = (char *)my_realloc(new_str, (i+2)*sizeof(char));
		new_str[i] = '\0';
	}

/*	printf("str_cut stopped\n"); */
	return new_str;
}

char *str_push(char *str, char *what, int where)
{
	char *new_str;
	int i = 0;
	int n;
	int m = (int)strlen(what);
	int j = 0;

	printf("starting str_push '%s', '%s' %d\n", str, what, where); 

	n = (int)strlen(str);

	if (where >= n)
	{
		return NULL;
	}

	new_str = (char *)my_malloc((n + m + 1)*sizeof(char));
	
	for (i = 0; i < where; i++)
	{
		new_str[j++] = str[i];
	}
	for (i = 0; i < m; i++)
	{
		new_str[j++] = what[i];
	}
	for (i = where; i < n; i++)
	{
		new_str[j++] = str[i];
	}
	new_str[j] = '\0';
	return new_str;
}

/*RF*/

int read_from_file(Bidirect_list *list, char *name)
{
	FILE *fd;
	char *buffer, sym;
	int i = 0;
	int ctr = 0;

	printf("read_from_file started\n");
	fd = fopen(name, "r");
	buffer = (char *)my_malloc(sizeof(char));
	while ((sym = fgetc(fd)) != EOF)
	{
		ctr++;
		if (sym == '\n')
		{
			buffer[i] = '\0';
/*			printf("%s\n", buffer); */
			list_append(list, buffer);
			i = 0;
			buffer = (char *)my_malloc(sizeof(char));
		}
		else if (sym == '\t')
		{
			int j;

			for (j = 0; j < SPACES_CNT; j++)
			{
				buffer[i++] = ' ';
				buffer = (char *)my_realloc(buffer, sizeof(char)*(i+1));
			}
		}
/*		else if (sym < ' ')
		{
			char *buf = (char *)my_malloc(((int)sym)*sizeof(char));
			int j = 0;

			buffer[i++] = '\\';
			buffer = (char *)my_realloc(buffer, sizeof(char)*(i+1));
			sprintf(buf, "%d", sym);
			for (j = 0; j < (int)strlen(buf); j++)
			{
				buffer[i++] = buf[j];
				buffer = (char *)my_realloc(buffer, sizeof(char)*(i+1));
			}
		} */
		else
		{
			buffer[i++] = sym;
			buffer = (char *)my_realloc(buffer, sizeof(char)*(i+1));
		}
	}	
	if (i != 0 )
	{
		buffer[i] = '\0';
/*		printf("%s\n", buffer); */
		list_append(list, buffer);
	}
	
	/* Чтобы отследить пустой файл */
	if (ctr == 0)
	{
		buffer[0] = '\0';
		list_append(list, buffer);
	}
	fclose(fd);
	printf("read_from_file ended\n");
	return 0;
}
/*GC*/
char *get_command(Bidirect_list *list, int need_num, char sym)
{
	char *format_str, symbol, *text, *n_text;
	int win_size[2];
	int i, j, step;

	i = 0;
	step = 0;
	getTermSize(win_size);
	text = (char *)my_malloc(sizeof(char));
	step = 0;
	text[0] = '\0';
	printf("%c", sym);
	while ((symbol = getKeyPress()) != '\n')
	{
		if (symbol == 27)
		{
			return NULL;
		}
		else if (symbol == 127)
		{
			if (i > 0)
			{
				if (text[i-1] < 0)
				{
					text[--i] = '\0';
					if (step != 0)
						step--;
				}
				text[--i] = '\0';
				if (step != 0)
					step--;
			}
				
		}
		else
		{
			text = (char *)my_realloc(text, sizeof(char)*(i + 2));
			text[i++] = symbol;
			text[i] = '\0';
			if (text[i-1] < 0)
			{
				symbol = getKeyPress();
				text = (char *)my_realloc(text, sizeof(char)*(i + 2));
				text[i++] = symbol;
				text[i] = '\0';
			}
			if (i - rus_let_ctr(text) >= win_size[0] - 2)
			{
				if (text[step] < 0)
				{
					step++;
				}
				step++;
			}
		}
		list_print(list, need_num);
		printf("%c%s",sym, text + sizeof(char)*step);
	}
	text[i] = '\0';
	n_text = (char *)my_malloc((strlen(text)+2)*sizeof(char));
	if (strlen(text) == 1)
	{
		n_text[0] = text[0];
		n_text[1] = '\0';
	}
	else
	{
		for(j = 0,i = 0; i < (int)strlen(text)-1;i++)
		{
			if (text[i] == '\\' && text[i+1] == 'n')
			{
				n_text[j++] = '\n';
				i++;
			}
			else if (text[i] == '\\' && text[i+1] == '\\')
			{
				n_text[j++] = '\\';
				i++;
			}
			else
			{
				n_text[j++] = text[i];
			}
		}
		n_text[j++] = text[strlen(text)-1];
		n_text[j] = '\0';
	}
	printf("\ncom: %s\n", n_text); 
	return n_text;
}

/*SE*/
int *search(Bidirect_list *list, char *str, int which)
{
	int ctr, i, j, k, *ans, strt_x, strt_y, b;
	struct list_node *node;

	printf("search started '%s'\n", str); 

	ans = (int*)my_malloc(sizeof(int)*2);

	/* Если ищем '\n' */
	if (strstr(str, "\n") == str && strlen(str) == strlen("\n"))
	{
		ans[0] = 0;
		ans[1] = (cur_loc_y + which + 1)%(list->num_elements);
		return ans;
	}

	node = list->head;
	for (ctr = 0; ctr < cur_loc_y; ctr++)
		node = node->next;

	k = 0;
	strt_x = -1;
	strt_y = -1;
	for (i = 0; i < max_left + cur_loc_x; i++)
	{
		k++;
		if ((node->str)[k-1] < 0)
			k++;
	}
	i = k + 1;
	k = 0;
	k = 0;
	j = 0;
	b = 1;
	while (ctr < (int)list->num_elements || b == 1)
	{
/*		printf("%d %d\n", ctr, list->num_elements); */
		if (node == NULL)
		{
/*			printf("start from top\n"); */
			node = list->head;
			ctr = 0; 
			b = 0;
			continue;
		} 
/*		printf("while %d < %d && %p != NULL\n", ctr, list->num_elements, node);	*/
		if (j == (int)strlen(str))
		{
/*			printf("j == (int)strlen(str): %d == %d\n", j, (int)strlen(str)); */
			if (k == which)
			{
				ans[1] = strt_y;
				node = list->head;
				for (i = 0; i < strt_y; i++)
				{
					node = node->next;
				}
				ans[0] = 0;
				for (i = 0; i < strt_x; i++)
				{
					ans[0]++;
					if ((node->str)[i] < 0)
						i++;
				}
/*				printf("%s %d %d\n",node->str, i, ctr); */
				return ans;
			}
			k++;
			j = 0;
			strt_x = -1;
			strt_y = -1;
			continue;
		}
		if ((j < (int)strlen(str) -1) && (str[j] == '\\') && (str[j+1] == 'n'))
		{
/*			printf("if ((j < (int)strlen(str) -1) && (str[j] == '\\') && (str[j+1] == 'n')): %d < %d && %c == '\\' && %c == n %d %d\n", j, (int)strlen(str) -1, str[j], str[j+1], i, (int)strlen(node->str)); */
			if (i  == (int)strlen(node->str))
			{
				if (strt_x == -1)
				{
					strt_x = 0;
					strt_y = ctr+1;
				}
				i = 0;
				j += 2;
				ctr++;
				node = node->next;
			}
			else
			{
				j = 0;
				strt_x = -1;
				strt_y = -1;
			}
			continue;
		}
		if ((j < (int)strlen(str) -1) && (str[j] == '\\') && (str[j+1] == '\\'))
		{
			j++;
			continue;
		}
		if (i < (int)strlen(node->str) && j < (int)strlen(str) && (node->str)[i] == str[j])
		{
/*			printf("if (i < (int)strlen(node->str) && j < (int)strlen(str) && (node->str)[i] == str[j]) : %d < %d && %d < %d && %c == %c\n", i, (int)strlen(node->str), j, (int)strlen(str), (node->str)[i], str[j]); */
			i++;
			j++;
			if (strt_x == -1)
			{
				strt_x = i - 1;
				strt_y = ctr;
			}
			continue;
		}
		else
		{
			j = 0;
			strt_x = -1;
			strt_y = -1;
			if (i < (int)(strlen(node->str) - 1) && (int)strlen(node->str) >0)
			{
				i++;
			}
			else
			{
				i = 0;
				ctr++;
				node = node->next;
			}
		}
	}
	ans[0] = -1;
	ans[1] = -1;
	return ans;
}

/* GT */
void goto_x_y(int x, int y)
{
	int ans[2], ctr2, ln;
	struct list_node *node;
	int win_size[2];

/*	printf("goto started\n"); */
	/* костыль */
	ans[0] = x;
	ans[1] = y;
	getTermSize(win_size);

	cur_loc_y = ans[1];
	if (cur_loc_y < 0)
		cur_loc_y = 0;
	else if (cur_loc_y > max_num_node - 1)
	       cur_loc_y = max_num_node;	
	else if (cur_loc_y < cur_first_node)
		cur_first_node = cur_loc_y;
	else if (cur_loc_y - cur_first_node >= (win_size[1] - 2))
		cur_first_node = cur_loc_y - win_size[1] + 2;

	node = main_list->head;
	for (ctr2 = 0; ctr2 < cur_loc_y; ctr2++)
		node = node->next;
	ln = (int)strlen(node->str);

	if (ans[0] < 0)
	{
		ans[0] = 0;
	}
	else if (ans[0] > ln-1)
		ans[0] = ln -1;
	if (ans[0] > win_size[0] -1 - step_for_scr_hnd + max_left)
	{
		printf("%d %d %d %d\n", max_left, win_size[0], step_for_scr_hnd, ans[0]);
		max_left = ans[0] - win_size[0] + 1 + step_for_scr_hnd;
		cur_loc_x = win_size[0] - 1 - step_for_scr_hnd;
	}
	else if (ans[0] < max_left)
	{
		max_left = ans[0];
		cur_loc_x = 0;
	}
	else
		cur_loc_x = ans[0] - max_left;
	printf("goto stopped\n");
	return;
}

int my_itoa(char *str)
{
	int i, res;

	res = 0;
	for (res = 0, i = 0; i < (int)strlen(str); i++)
	{
		if (str[i] >= '0' && str[i] <= '9')
		{
			res *= 10;
			res += str[i] - '0';
		}
		else
			return -1;

	}
	return res;

}

void save_file(Bidirect_list *list, char *name)
{
	FILE *fd;
	struct list_node *node;

	node = list->head;
	fd = fopen(name, "w");
	while (node != NULL)
	{
		fprintf(fd, "%s\n", node->str);
		node = node->next;
	}
	fclose(fd);
	return;
}

void *my_malloc(size_t size)
{
	void *ptr;
	struct malloc_node *node;

/*	printf("my_malloc started %d\n", size); */
	ptr = malloc(size);
	if (ptr == NULL)
		return NULL;
	if (malloc_list->tail == NULL)
	{
		node = (struct malloc_node *)malloc(sizeof(struct malloc_node));
		malloc_list->num_elements++;
		node->ptr = ptr;
		node->next = NULL;
		node->prev = NULL;
		malloc_list->head = node;
		malloc_list->tail = node;
	}
	else
	{
		node = (struct malloc_node *)malloc(sizeof(struct malloc_node));
		node->ptr = ptr;
		malloc_list->num_elements++;
		node->next = NULL;
		malloc_list->tail->next = node;
		node->prev = malloc_list->tail;
		malloc_list->tail = node;
	}
/*	printf("end my_malloc\n"); */
	return ptr;
}

void *my_realloc(void *ptr, size_t size)
{
	void *n_ptr;
	struct malloc_node *node;

	if (ptr == NULL)
	{
		n_ptr = malloc(size);
		if (n_ptr == NULL)
		{
			printf("Realloc error\n");
			exit_prog();
		}
		node = (struct malloc_node *)malloc(sizeof(struct malloc_node));
		node->ptr = n_ptr;
		node->next = NULL;
		node->prev = malloc_list->tail;
		malloc_list->tail->next = node;

		malloc_list->tail = node;
		malloc_list->num_elements++;
	}
	else
	{
		n_ptr = realloc(ptr, size);
		if (n_ptr == NULL)
		{
			printf("Realloc error\n");
			exit_prog();
		}
		if (n_ptr != ptr)
		{
			struct malloc_node *node = (struct malloc_node *)malloc(sizeof(struct malloc_node));

			node->ptr = ptr;
			node->next = NULL;
			free_list->num_elements++;
			if (free_list->tail == NULL)
			{
				free_list->head = node;
				free_list->head->prev = NULL;
			}
			else
			{
				free_list->tail->next = node;
				node->prev = free_list->tail;
			}
			free_list->tail = node;

			node = (struct malloc_node *)malloc(sizeof(struct malloc_node));
			node->ptr = n_ptr;
			node->next = NULL;
			node->prev = malloc_list->tail;
			malloc_list->tail->next = node;

			malloc_list->tail = node;
			malloc_list->num_elements++;


		}


	}
	return n_ptr;
}

void garbage_collector()
{
	struct malloc_node *node;
	struct malloc_node *f_node;
	int ld_par = 0;
	int i = 0;
	int n = malloc_list->num_elements;

	node = malloc_list->head;
	while (node != NULL)
	{
		int key = 0;

		f_node = free_list->head;
		while (f_node != NULL)
		{
			if (f_node->ptr == node->ptr)
				key = 1;
			f_node = f_node->next;
		}
		if (key == 0)
			free(node->ptr);
		if (node->prev != NULL)
			free(node->prev);
		node = node->next;

		/* Загрузка */
/*		loading(&ld_par); */
		printf("\r%d in %d done", ++i, n);

	}
	printf("\n");
	f_node = free_list->head;
	n = free_list->num_elements;
	i = 0;
	while (f_node != NULL)
	{
		if (f_node->prev != NULL)
			free(f_node->prev);
		f_node = f_node->next;

		/* Загрузка */
/*		loading(&ld_par); */
		printf("\r%d in %d done", ++i, n);
	}

	free(malloc_list->tail);
	free(malloc_list);
	free(free_list->tail);
	free(free_list);
	return;
}

void exit_prog()
{
	printf("Turning off the term...\n");
	turnOffTerm(old_attributes);
	printf("Turned off\nCollecting garbage...\n");
	garbage_collector();
	printf("\nCollected\n");
	exit(0);
}

/* SB */
void my_subst(Bidirect_list *list, char *str1, char *str2)
{
	char *all_str = (char *)my_malloc(sizeof(char));
	char *ptr, *old_ptr;
	struct list_node *node;

/*	printf("my_subst started\n"); */
	if (str1[0] == '\0')
	{
		return;
	}
	node = list->head;
	all_str[0] = '\0';
	if (str1[0] == '\n' && str1[1] == '\0' && str2[0] == '\0')
	{
		printf("here\n");
		str1 = (char *)my_malloc((strlen("\n\n")+1)*sizeof(char));
		str2 = (char *)my_malloc((strlen("\n")+1)*sizeof(char));
		str1[0] = '\n';
		str1[1] = '\n';
		str1[2] = '\0';
		str2[0] = '\n';
		str2[1] = '\0';
	} 
	while (node != NULL)
	{
		char *buf;

		buf = my_strcat(all_str, node->str);
		all_str = buf;
		if (node->next != NULL)
			all_str = my_strcat(all_str, "\n");
		node = node->next;
	}

/*	all_str = my_strcat(all_str, "\n"); */
	ptr = all_str;
	printf("hsebf: '%s'|'%s'\n", str1, str2); 
	while ((ptr = strstr(ptr, str1)) != NULL)
	{
		char *buffer1, *buffer2, *buffer3;

		buffer1 = str_cut(all_str, ptr);
		buffer2 = str_cut(ptr + strlen(str1)*sizeof(char), ptr + strlen(ptr)*sizeof(char)); 
		buffer3 = my_strcat(buffer1, str2);
		all_str = my_strcat(buffer3, buffer2);
		ptr = strstr(all_str, buffer2);
/*		ptr += (1+strlen(str2))*sizeof(char); */
	}

	/* Тут выкинул весь список, если идея со сборщиком мусора не сработает, почистить тут */

	node = list->head;
	list_remove(list);
	
	ptr = all_str;
	old_ptr = ptr;
	while ((ptr = strstr(ptr, "\n")) != NULL)	
	{
		*ptr = '\0';
		ptr += sizeof(char);
		list_append(list, old_ptr);
		old_ptr = ptr;
	}

	all_str[strlen(all_str)-1] = '\0';
	list_append(list, old_ptr);
	goto_x_y(0, 0);
	max_num_node = list->num_elements;
	printf("SB stopped\n");
	return;
}

char *my_strcat(char *str1, char *str2)
{
	char *res = (char *)my_malloc((strlen(str1)+strlen(str2)+2)*sizeof(char));
	int i = 0;
	int j = 0;

	for (i = 0; i < (int)strlen(str1);i++)
	{
		res[j++] = str1[i];
	}
	for (i = 0; i < (int)strlen(str2);i++)
	{
		res[j++] = str2[i];
	}
	res[j] = '\0';
	return res;

}

void my_free(void *ptr)
{
	struct malloc_node *node;

	node = malloc_list->head;
	while (node != NULL && node->ptr != ptr)
		node = node->next;
	/* Если пытаемся освободить свободную память */
	if (node == NULL)
		return;
	if (node->prev == NULL && node->next == NULL)
	{
		malloc_list->head = NULL;
		malloc_list->tail = NULL;
	}
	else if (node->prev == NULL && node->next != NULL)
	{
		malloc_list->head = node->next;
		node->next->prev = NULL;
	}
	else if (node->prev != NULL && node->next == NULL)
	{
		malloc_list->tail = node->prev;
		node->prev->next = NULL;
	}
	else
	{
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	malloc_list->num_elements--;
	free(node->ptr);
	free(node);
	return;
}

void start()
{
	malloc_list = (Malloc_list *)malloc(sizeof(Malloc_list));
	free_list = (Malloc_list *)malloc(sizeof(Malloc_list));
	malloc_list->head = NULL;
	malloc_list->tail = NULL;
	free_list->head = NULL;
	free_list->tail = NULL;
	main_list = list_init();
	cur_loc_x = 0;
	cur_loc_y = 0;
	cur_str_max_len = 0;
	cur_first_node = 0;
	max_num_node = 0;
	max_left = 0;
	loc_str_tail = 0;
	prev_str_len = 0;
	next_str_len = 0;
	step_for_scr_hnd = 0;
	cur_start = (char *)my_malloc((strlen("\033[27m")+2)*sizeof(char));
	cur_stop = (char *)my_malloc((strlen("\033[7m")+2)*sizeof(char));
	cur_start = "\033[7m\0";
	cur_stop = "\033[27m\0";

	return;
}

char **get_str_for_subst(char *command, char *str1, char *str2)
{
	char *buffer;
	char **res;
	int c = 0;
	int j = 0;
	int i = 0;

	printf("cmd '%s'\n", command); 
	if ((buffer = strstr(command, "subst /")) != command)
		return NULL;
	str1 = (char *)my_malloc(sizeof(char));
	str2 = (char *)my_malloc(sizeof(char));
	str1[0] = '\0';
	str2[0] = '\0';
	buffer = strstr(command, "/") + sizeof(char);
	for (i = 0; i < (int)strlen(buffer); i++)
	{
		printf("str1: '%s', str2: '%s', c: %d, buffer[%d] == %c\n", str1, str2, c, i, buffer[i]);
		if (c > 1)
		{
			return NULL;
		}

		if (buffer[i] == '\\')
		{
			/* Экранирование */

			if (i != (int)strlen(buffer)-1 && buffer[i+1] == '/')
			{
				printf("here1\n");
				if (c == 0)
				{
					str1[j++] = '/';
					str1 = (char *)my_realloc(str1, (j+1)*sizeof(char));
					str1[j] = '\0';
				}
				else
				{
					str2[j++] = '/';
					str2 = (char *)my_realloc(str2, (j+1)*sizeof(char));
					str2[j] = '\0';
				}
				i++;
			}
			else if (i != (int)strlen(buffer)-1 && buffer[i+1] == '\\')
			{
				if (c == 0)
				{
					str1[j++] = '\\';
					str1 = (char *)my_realloc(str1, (j+1)*sizeof(char));
					str1[j] = '\0';
				}
				else
				{
					str2[j++] = '\\';
					str2 = (char *)my_realloc(str2, (j+1)*sizeof(char));
					str2[j] = '\0';
				}
				i++;
			}
			else if (i != (int)strlen(buffer)-1 && buffer[i+1] == '\n')
			{
				if (c == 0)
				{
					str1[j++] = '\\';
					str1 = (char *)my_realloc(str1, (j+1)*sizeof(char));
					str1[j++] = 'n';
					str1 = (char *)my_realloc(str1, (j+1)*sizeof(char));
					str1[j] = '\0';
				}
				else
				{
					str2[j++] = '\\';
					str2 = (char *)my_realloc(str2, (j+1)*sizeof(char));
					str2[j++] = 'n';
					str2 = (char *)my_realloc(str2, (j+1)*sizeof(char));
					str2[j] = '\0';
				}
				i++;
			}
			
		}
		else if (buffer[i] == '/')
		{
			c++;
			j = 0;
		}
		else
		{
			if (c == 0)
			{
				str1[j++] = buffer[i];
				str1 = (char *)my_realloc(str1, (j+1)*sizeof(char));
				str1[j] = '\0';
			}
			else
			{
				str2[j++] = buffer[i];
				str2 = (char *)my_realloc(str2, (j+1)*sizeof(char));
				str2[j] = '\0';
			}
		}
	}
	if (c == 2)
	{
		res = (char **)my_malloc(2*sizeof(char*));
		res[0] = str1;
		res[1] = str2;
		printf("res[0]: '%s'\nres[1]: '%s'\n", res[0], res[1]);
		return res;
	}
	else
		return NULL;
}

void loading(int *par)
{
	int sig_par = *par;

	if (sig_par == 0)
	{
		printf("\r- ");
		sig_par = 1;
	}
	else if (sig_par == 1)
	{
		printf("\r\\ ");
		sig_par = 2;
	}
	else if (sig_par == 2)
	{
		printf("\r| ");
		sig_par = 3;
	}
	else if (sig_par == 3)
	{
		printf("\r/ ");
		sig_par = 0;
	}
	fflush(stdout);
	*par = sig_par;
	return;
}

int rus_let_ctr(char *str)
{
	int i = 0;
	int j = 0;

	for (i = 0; i < strlen(str); i++)
	{
		if (str[i] < 0)
		{
			j++;
			i++;
		}
	}
	return j;
}
