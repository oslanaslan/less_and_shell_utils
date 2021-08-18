#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>
#include <wchar.h>
#include <locale.h>
#define SPACES_CNT 4
#define POSIX_C_SOURCE 100500L

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
wchar_t *cur_start;
wchar_t *cur_stop;
wchar_t *search_text = NULL;
int search_ctr = 0;
void **malloc_array;
long malloc_array_num = 0;
struct termios old_attributes;
int isStart = 0;

struct list_node
{
	wchar_t *str;
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

int list_append(Bidirect_list *main_list, wchar_t *buffer);
void list_print(Bidirect_list *list, int need_num);
void list_remove(Bidirect_list *list);
struct termios turnOnTerm();
int turnOffTerm(struct termios old_attributes);
void getTermSize(int *x_y);
void clear();
Bidirect_list *list_init();
wchar_t *get_str_for_pr(int need_num);
void screen_handler(int key, Bidirect_list *list, int need_num);
int getKeyPress();
wchar_t *str_push(wchar_t *str, wchar_t *what, int where);
wchar_t *str_cut(wchar_t *from, wchar_t *to);
int read_from_file(Bidirect_list *list, char *name);
wchar_t *get_command(Bidirect_list *list, int need_num, wchar_t sym);
int *search(Bidirect_list *list, wchar_t *str, int which);
int find_text(Bidirect_list *list, int need_num);
void save_file(Bidirect_list *list, wchar_t *name);
void *my_malloc(size_t size);
void *my_realloc(void *ptd, size_t size);
void garbage_collector(int quiet);
void exit_prog(int quiet);
void my_subst(Bidirect_list *list, wchar_t *str1, wchar_t *str2);
wchar_t *my_strcat(wchar_t *str1, wchar_t *str2);
void my_free(void *ptr);
wchar_t **get_str_for_subst(wchar_t *command, wchar_t *str1, wchar_t *str2);
void start();
int my_itoa(wchar_t *str);
void goto_x_y(int x, int y);
int rus_let_ctr(wchar_t *str);
void print_help();
/*MA*/
int main(int argc, char **argv)
{
	int needNum = -1;
	char *name = NULL;
	int opt;
	int key = 0;

	setlocale(LC_ALL, "");
	wprintf(L"\n");
	start();
	old_attributes = turnOnTerm();
	while ((opt = getopt(argc, argv, "no:vh")) != -1)
	{
		switch (opt)
		{
			case 'n':
				needNum = 0;
				break;
			case 'v':
				wprintf(L"Less v. 1.0.0 \n\tby Aslan Ashabokov\n");
				exit_prog(1);
				break;
			case '?':
			case 'h':
				print_help();
				exit_prog(1);
				break;
			case 'o':
				name = optarg;
				break;	
		}
	}
	if (name != NULL)
	{
		int tmp;

		tmp = read_from_file(main_list, name);
		if (tmp == -1)
		{
			wprintf(L"Файл с именем '%s' не найден\n", name);
			exit_prog(1);
		}
	}
	else
	{
		print_help();
		exit_prog(1);
	}
	if (!isatty(1))
	{
		struct list_node *node = main_list->head;

		while (node != NULL)
		{
			wprintf(L"%ls\n", node->str);
			node = node->next;
		}
		exit_prog(1);
	}
	cur_str_max_len = (int)wcslen(main_list->head->str);
	max_num_node = main_list->num_elements;
	screen_handler(0, main_list, needNum); 
	while ((key = getKeyPress()) != (int)L'\004')
	{
		screen_handler(key, main_list, needNum);
	}
	exit_prog(0);
	return 0;
}
/*LA*/
int list_append(Bidirect_list *main_list, wchar_t *buffer)
{
	int i;
	struct list_node *new_node;
	wchar_t *strstr;
	
	new_node = (struct list_node *)my_malloc(sizeof(struct list_node));
	if (new_node == NULL)
	{
		wprintf(L"my_malloc error\n");
		exit_prog(0);
	}
	strstr = (wchar_t *)my_malloc((2+wcslen(buffer))*sizeof(wchar_t));
	if (strstr == NULL)
	{
		wprintf(L"my_malloc error in list_append\n");
		exit_prog(0);
	}
	for (i = 0; i < (int)wcslen(buffer); i++)
		strstr[i] = buffer[i];
	strstr[wcslen(buffer)] = L'\0';
	if (wcslen(strstr) > main_list->max_len_str)
		main_list->max_len_str = (int)wcslen(strstr);
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

int turnOffTerm(struct termios old_attributes)
{
	tcsetattr(0, TCSANOW, &old_attributes);
	return 0;	
}

void getTermSize(int *ss)
{
	struct winsize ws;

	ioctl(0, TIOCGWINSZ, &ws);
	ss[1] = ws.ws_row;
	ss[0] = ws.ws_col;
	return;
}

void clear()
{
	wprintf(L"\x1b[2J");
	wprintf(L"\x1b[H");
	return;
}
/*PL*/
void list_print(Bidirect_list *list, int need_num)
{
	int i, ctr;
	int win_size[2];
	struct list_node *node;
	
	clear();
	getTermSize(win_size);
	node = list->head;
	for (i = 0; i< cur_first_node; i++)
	{
		node = node->next;
	}
	for (ctr = cur_first_node; (node != NULL) && (ctr < win_size[1] + cur_first_node- 1); ctr++)
	{
		wchar_t *pr_str;
		int need_curs;

		if (ctr == cur_loc_y)
		{
			need_curs = 1;
			cur_str_max_len = (int)wcslen(node->str);
			loc_str_tail = cur_str_max_len - cur_loc_x - max_left;
			if (cur_str_max_len == 0)
				loc_str_tail = 0;
			if (node->next != NULL)
				next_str_len = (int)wcslen(node->next->str);
			else
				next_str_len = (int)wcslen(node->str);
			if (node->prev != NULL)
				prev_str_len = (int)wcslen(node->prev->str);
			else
				prev_str_len = (int)wcslen(node->str);
			if (next_str_len == 0)
				next_str_len = 1;
			if (prev_str_len == 0)
				prev_str_len = 1;
		}
		else
		{
			need_curs = -1;
		}
		if (need_num != -1)
			need_num = ctr;
		pr_str = get_str_for_pr(need_num);
		step_for_scr_hnd = (int)wcslen(pr_str); /* чтобы понять зачем, см код screen_handler */
		wprintf(L"%ls", pr_str);
		if ((int)wcslen(node->str) < max_left) /* Чтобы не лезть в чужую память */
		{
			wprintf(L"\n");
		}
		else
		{
			wchar_t *strtmp = node->str;

			if (need_curs != -1)
			{
				if (wcslen(strtmp) == 0)
					wprintf(L"%ls %ls", cur_start, cur_stop);
				else
				{
					for (i = max_left; i < cur_loc_x + max_left; i++)
					{
						wprintf(L"%lc", strtmp[i]);
					}
					wprintf(L"%ls%lc%ls", cur_start, strtmp[i++], cur_stop);
					for (; (i < (win_size[0] - (int)wcslen(pr_str)) + max_left) && (strtmp[i] != L'\0'); i++)
					{
						wprintf(L"%lc", strtmp[i]);
					}
				}
				wprintf(L"\n");
			}
			else
			{
				if (wcslen(strtmp) == 0)
					printf(" ");
				else
				{
					for (i = max_left; (i < win_size[0] - (int)wcslen(pr_str) + max_left) && ((node->str)[i] != L'\0'); i++)
					{
						wprintf(L"%lc", (node->str)[i]);
					}
				}
				wprintf(L"\n");
			}
		}
		node = node->next;
	}
	getTermSize(win_size);
	for (;ctr < (win_size[1] - 1); ctr++)
		wprintf(L"\n"); 
	return;
}

Bidirect_list *list_init()
{
	Bidirect_list *list = (Bidirect_list *)my_malloc(sizeof(Bidirect_list));
	if (list == NULL)
	{
		printf("my_malloc error\n");
		exit_prog(0);
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
wchar_t *get_str_for_pr(int need_num)
{
	int win_size[2];
	wchar_t *buffer1 = NULL; 
	wchar_t *buffer2 = NULL; 
	wchar_t *buffer3 = NULL;
	
	getTermSize(win_size);
	if (need_num != -1)
	{
		int i,n;

		buffer1 = (wchar_t *)my_malloc((max_num_node*4 +1)*sizeof(wchar_t));
		if (buffer1 == NULL)
		{
			printf("my_malloc error in get_str_for_pr \n");
			exit_prog(0);
		}
		buffer2 = (wchar_t *)my_malloc((max_num_node*4 + 1)*sizeof(wchar_t));
		if (buffer2 == NULL)
		{
			printf("my_malloc error in get_str_for_pr\n");
			exit_prog(0);
		}
		swprintf(buffer1, (max_num_node*4 + 1)*sizeof(wchar_t), L"%d :<", max_num_node);
		n = (int)wcslen(buffer1);
		swprintf(buffer2, (max_num_node*4 + 1)*sizeof(wchar_t), L"%d", need_num);
		for (i = 0; i < (int)wcslen(buffer2); i++)
			buffer1[i] = buffer2[i];
		for (i = (int)wcslen(buffer2); i < n-3; i++)
			buffer1[i] = L' ';
		if (!max_left)
			buffer1[n-1] = L'|';
		return buffer1;
	}
	else
	{
		buffer3 = (wchar_t *)my_malloc(sizeof(wchar_t)*2);

		if (!max_left)
			buffer3[0] = L'|';
		else
			buffer3[0] = L'<';
		buffer3[1] = L'\0';
		return buffer3;	
	}
}
/*SH*/
void screen_handler(int key, Bidirect_list *list, int need_num)
{
	int win_size[2];

	getTermSize(win_size);
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
		wprintf(L"%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
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
		wprintf(L"%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
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
		wprintf(L"%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
	}
	else if (key == 67) /* RIGHT */
	{
		if (loc_str_tail - 1 <= 0)
		{
			;
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
		wprintf(L"%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
	}
	else if (key == L':') /* : */
	{
		wchar_t *command;
		int n;

		cur_start = L"";
		cur_stop = L"";		
		list_print(list, need_num);
		command = get_command(list, need_num, L':');
		cur_start = L"\033[7m";
		cur_stop = L"\033[27m";
		if (command == NULL)
		{
			list_print(list, need_num);
			wprintf(L"%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
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
			wprintf(L"%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
		}
		else if (wcsstr(command, L"write") == command)
		{
			wchar_t *name;

			list_print(list, need_num);
			if (wcslen(command) <= wcslen(L"write "))
			{
				wprintf(L"Неверный формат команды");
			}
			else
			{
				name = wcsstr(command, L" ");
				name = &(name[1]);
				save_file(list, name);
				wprintf(L"Файл сохранен");
			}
		}
		else if (wcsstr(command, L"subst ") == command)
		{
			wchar_t *str1, *str2;
			wchar_t **tmp;

			str1 = NULL; 
			str2 = NULL;
			wprintf(L"comm: '%ls'\n", command);
			tmp = get_str_for_subst(command, str1, str2);
			if (tmp == NULL)
			{
				list_print(list, need_num);
				wprintf(L"Неверный формат команды");
			}
			else
			{
				str1 = tmp[0];
				str2 = tmp[1];
				my_subst(list, str1, str2);
				list_print(list, need_num);
				wprintf(L"Заменено");
			}
		}
		else
		{
			list_print(list, need_num);
			wprintf(L"Неизвестная команда"); 
		} 
	}
	else if (key == L'/') /* / */
	{
		wchar_t *text;

		list_print(list, need_num);
		text = get_command(list, need_num, L'/');
		wprintf(L"here text '%ls'\n", text);
		if (text != NULL)
		{
			int *ans;
			if (text[0] == L'\0')
			{
				if (search_text == NULL)
				{
					list_print(list, need_num);
					wprintf(L"Нет истории поиска");
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
				wprintf(L"%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
			}
			else
			{
				list_print(list, need_num);
				wprintf(L"Не найдено");
			}
		}
		else
		{
			list_print(list, need_num);
			wprintf(L"%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
		}
	}
	else if (key == L'n') /* n */
	{
		int *ans;

		if (search_text == NULL)
		{
			list_print(list, need_num);
			wprintf(L"Нет истории поиска");
			return;
		}
		else
		ans = search(list, search_text, search_ctr);
		if (ans[0] != -1 && ans[1] != -1)
		{
			goto_x_y(ans[0], ans[1]);
			list_print(list, need_num);
			wprintf(L"%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
		}
		else
		{
			list_print(list, need_num);
			wprintf(L"Не найдено");
		}
	}
	else if (key == L'q') /* q */
	{
		clear();
		wprintf(L"Exiting...\n");
		exit_prog(0);
	}
	else /* SOMETHING ELSE */
	{
		list_print(list, need_num);
		wprintf(L"%d(%d) %d(%d)", cur_loc_y, (int)list->num_elements-1, cur_loc_x + max_left+1, cur_str_max_len); 
	}
	return;
}

wchar_t getKeyPress()
{
	wchar_t key;
	wchar_t symbol;

	symbol = fgetwc(stdin);
	{
		if (symbol == (char)21)
		{
			symbol = fgetwc(stdin);
			symbol = fgetwc(stdin);
			key = 0 - symbol;
		}
		else
		{
			key = symbol;
		}
		
	}
	return key;
}

wchar_t *str_cut(wchar_t  *from, wchar_t *to)
{
	wchar_t *new_str;
	int i;

	new_str = (wchar_t *)my_malloc(sizeof(wchar_t));
	if (new_str == NULL)
	{
		wprintf(L"my_malloc error in cat_str\n");
		exit_prog(0);
	}
	for (i = 0; (unsigned int)i < (to - from)/sizeof(wchar_t) && from[i] != L'\0'; i++)
	{
		new_str[i] = from[i];
		new_str = (wchar_t *)my_realloc(new_str, (i+2)*sizeof(wchar_t));
	}
	new_str[i] = L'\0';
	return new_str;
}

wchar_t *str_push(wchar_t *str, wchar_t *what, int where)
{
	wchar_t *new_str;
	int i = 0;
	int n;
	int m = (int)wcslen(what);
	int j = 0;

	n = (int)wcslen(str);
	if (where >= n)
	{
		return NULL;
	}
	new_str = (wchar_t *)my_malloc((n + m + 1)*sizeof(wchar_t));
	
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
	new_str[j] = L'\0';
	return new_str;
}
/*RF*/
int read_from_file(Bidirect_list *list, char *name)
{
	FILE *fd;
	wchar_t *buffer, sym;
	int i = 0;
	int ctr = 0;

	fd = fopen(name, "r");
	if (fd == 0)
	{
		return -1;
	}
	buffer = (wchar_t *)my_malloc(sizeof(wchar_t));
	while ((sym = fgetwc(fd)) != EOF)
	{
		ctr++;
		if (sym == L'\n')
		{
			buffer[i] = L'\0';
			list_append(list, buffer);
			i = 0;
			buffer = (wchar_t *)my_malloc(sizeof(wchar_t));
		}
		else if (sym == L'\t')
		{
			int j;

			for (j = 0; j < SPACES_CNT; j++)
			{
				buffer[i++] = L' ';
				buffer = (wchar_t *)my_realloc(buffer, sizeof(wchar_t)*(i+1));
			}
		}
		else
		{
			buffer[i++] = sym;
			buffer = (wchar_t *)my_realloc(buffer, sizeof(wchar_t)*(i+1));
		}
	}	
	if (i != 0 )
	{
		buffer[i] = L'\0';
		list_append(list, buffer);
	}
	/* Чтобы отследить пустой файл */
	if (ctr == 0)
	{
		buffer[0] = L'\0';
		list_append(list, buffer);
	}
	fclose(fd);
	return 0;
}
/*GC*/
wchar_t *get_command(Bidirect_list *list, int need_num, wchar_t sym)
{
	wchar_t symbol, *text;
	int win_size[2];
	int i, step;

	i = 0;
	step = 0;
	getTermSize(win_size);
	text = (wchar_t *)my_malloc(sizeof(wchar_t));
	step = 0;
	text[0] = L'\0';
	wprintf(L"%lc", sym);
	while ((symbol = getKeyPress()) != L'\n')
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
					text[--i] = L'\0';
					if (step != 0)
						step--;
				}
				text[--i] = L'\0';
				if (step != 0)
					step--;
			}
		}
		else
		{
			text = (wchar_t *)my_realloc(text, sizeof(wchar_t)*(i + 2));
			text[i++] = symbol;
			text[i] = L'\0';
			if (text[i-1] < 0)
			{
				symbol = getKeyPress();
				text = (wchar_t *)my_realloc(text, sizeof(wchar_t)*(i + 2));
				text[i++] = symbol;
				text[i] = L'\0';
			}
			if (i >= win_size[0] - 2)
			{
				if (text[step] < 0)
				{
					step++;
				}
				step++;
			}
		}
		list_print(list, need_num);
		wprintf(L"%lc%ls",sym, text + sizeof(wchar_t)*step);
	}
	text[i] = L'\0';
	wprintf(L"\ncom: %ls\n", text); 
	return text;
}
/*SE*/
int *search(Bidirect_list *list, wchar_t *str, int which)
{
	int ctr, i, j, k, *ans, strt_x, strt_y, b;
	struct list_node *node;

	ans = (int*)my_malloc(sizeof(int)*2);
	/* Если ищем '\n' */
	if (wcsstr(str, L"\n") == str && wcslen(str) == wcslen(L"\n"))
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
		if (node == NULL)
		{
			node = list->head;
			ctr = 0; 
			b = 0;
			continue;
		} 
		if (j == (int)wcslen(str))
		{
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
				return ans;
			}
			k++;
			j = 0;
			strt_x = -1;
			strt_y = -1;
			continue;
		}
		if ((j < (int)wcslen(str) -1) && (str[j] == L'\\') && (str[j+1] == L'n'))
		{
			if (i  == (int)wcslen(node->str))
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
		if ((j < (int)wcslen(str) -1) && (str[j] == L'\\') && (str[j+1] == L'\\'))
		{
			j++;
			continue;
		}
		if (i < (int)wcslen(node->str) && j < (int)wcslen(str) && (node->str)[i] == str[j])
		{
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
			if (i < (int)(wcslen(node->str) - 1) && (int)wcslen(node->str) >0)
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
	ln = (int)wcslen(node->str);
	if (ans[0] < 0)
	{
		ans[0] = 0;
	}
	else if (ans[0] > ln-1)
		ans[0] = ln -1;
	if (ans[0] > win_size[0] -1 - step_for_scr_hnd + max_left)
	{
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
	if (max_left < 0)
		max_left = 0;
	return;
}

int my_itoa(wchar_t *str)
{
	int i, res;

	res = 0;
	for (res = 0, i = 0; i < (int)wcslen(str); i++)
	{
		if (str[i] >= L'0' && str[i] <= L'9')
		{
			res *= 10;
			res += str[i] - L'0';
		}
		else
			return -1;
	}
	return res;

}

void save_file(Bidirect_list *list, wchar_t *name)
{
	FILE *fd;
	struct list_node *node;
	char *buffer = (char *)malloc(4*sizeof(wchar_t)*sizeof(char)*wcslen(name));
	char c;
	int i;

	node = list->head;
	fd = fopen(".less.tmp", "w");
	fwprintf(fd , L"%ls\n", name);
	fclose(fd);
	fd = fopen(".less.tmp", "r");
	i = 0;
	while ((c = fgetc(fd)) != '\n')
	{
		buffer[i++] = c;	
	}
	fclose(fd);
	remove(".less.tmp");
	buffer[i] = '\0';
	wprintf(L"%s\n", buffer);
	fd = fopen(buffer, "w");
	free(buffer);
	while (node != NULL)
	{
		fwprintf(fd, L"%ls\n", node->str);
		node = node->next;
	}
	fclose(fd);
	return;
}

void *my_malloc(size_t size)
{
	void *ptr = NULL;
	struct malloc_node *node = NULL;

	ptr = malloc(size);
	if (ptr == NULL)
	{
		wprintf(L"ERROR in my_malloc\n");
		exit(0);
	}
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
			wprintf(L"Realloc error\n");
			exit_prog(0);
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
			wprintf(L"Realloc error\n");
			exit_prog(0);
		}
		if (n_ptr != ptr)
		{
			struct malloc_node *node = malloc_list->head;

			while (node != NULL && node->ptr != ptr)
				node = node->next;
			if (node == NULL)
			{
				wprintf(L"ERROR my_realloc node == NULL\n");
				exit(0);
			}
			node->ptr = n_ptr;
		}
	}
	return n_ptr;
}

void garbage_collector(int quiet)
{
	struct malloc_node *node;
	struct malloc_node *f_node;
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
		if (!quiet)
			wprintf(L"\r%d in %d done", ++i, n);
	}
	if (!quiet)
		wprintf(L"\n");
	free(malloc_list->tail);
	free(malloc_list);
	free(free_list);
	return;
}

void exit_prog(int quiet)
{
	if (!quiet)
	{
		wprintf(L"Turning off the term...\n");
		turnOffTerm(old_attributes);
		wprintf(L"Turned off\nCollecting garbage...\n");
		garbage_collector(quiet);
		wprintf(L"\nCollected\n");
		exit(0);
	}
	else
	{
		turnOffTerm(old_attributes);
		garbage_collector(quiet);
		exit(0);
	}
}
/* SB */
void my_subst(Bidirect_list *list, wchar_t *str1, wchar_t *str2)
{
	wchar_t *all_str = (wchar_t *)my_malloc(sizeof(wchar_t));
	wchar_t *ptr, *old_ptr;
	struct list_node *node;
	int i = 0;
	int j = 0;

	if (str1[0] == L'\0')
	{
		return;
	}
	node = list->head;
	all_str[0] = L'\0';
	if (str1[0] == L'\n' && str1[1] == L'\0' && str2[0] == L'\0')
	{
		str1 = (wchar_t *)my_malloc((wcslen(L"\n\n")+1)*sizeof(wchar_t));
		str2 = (wchar_t *)my_malloc((wcslen(L"\n")+1)*sizeof(wchar_t));
		str1[0] = L'\n';
		str1[1] = L'\n';
		str1[2] = L'\0';
		str2[0] = L'\n';
		str2[1] = L'\0';
	} 
	while (node != NULL)
	{
		wchar_t *buf;

		buf = my_strcat(all_str, node->str);
		all_str = buf;
		if (node->next != NULL)
			all_str = my_strcat(all_str, L"\n");
		node = node->next;
	}
	j = 0;
	ptr = (wchar_t *)my_malloc(sizeof(wchar_t));
	for (i = 0; i < (int)wcslen(all_str); i++)
	{
		if (wcsstr(all_str, str1) != &(all_str[i]))
		{
			ptr[j++] = all_str[i];
			ptr = (wchar_t *)my_realloc(ptr, (j+2)*sizeof(wchar_t));
			ptr[j] = L'\0';
		}
		else
		{
			int k = 0;

			for (k = 0; k < (int)wcslen(str2); k++)
			{
				ptr[j++] = str2[k];
				ptr = (wchar_t *)my_realloc(ptr, (j+2)*sizeof(wchar_t));
				ptr[j] = L'\0';
			}
			all_str = &(all_str[i + wcslen(str1)]);
			i = -1;
		}
	}
	all_str = ptr;
	/* Тут выкинул весь список, если идея со сборщиком мусора не сработает, почистить тут */
	list->head = NULL;
	list->tail =NULL;
	list->num_elements = 0;
	list->max_len_str = 0;
	node = list->head;
	ptr = all_str;
	old_ptr = ptr;
	j = 0;
	old_ptr = (wchar_t *)my_malloc(sizeof(wchar_t));
	old_ptr[j] = L'\0';
	for (i = 0; i < (int)wcslen(all_str); i++)
	{
		if (all_str[i] == L'\n')
		{
			list_append(list, old_ptr);
			j = 0;
			old_ptr = (wchar_t *)my_malloc(sizeof(wchar_t));
			old_ptr[j] = L'\0';
		}
		else
		{
			old_ptr[j++] = all_str[i];
			old_ptr = (wchar_t *)my_realloc(old_ptr, (j+2)*sizeof(wchar_t));
			old_ptr[j] = L'\0';
		}
	}
	if (old_ptr[0] != L'\0')
	{
		list_append(list, old_ptr);
	}
	goto_x_y(0, 0);
	max_num_node = list->num_elements;
	wprintf(L"SB stopped %d\n", max_num_node);
	return;
}

wchar_t *my_strcat(wchar_t *str1, wchar_t *str2)
{
	wchar_t *res = (wchar_t *)my_malloc((wcslen(str1)+wcslen(str2)+2)*sizeof(wchar_t));
	int i = 0;
	int j = 0;

	for (i = 0; i < (int)wcslen(str1);i++)
	{
		res[j++] = str1[i];
	}
	for (i = 0; i < (int)wcslen(str2);i++)
	{
		res[j++] = str2[i];
	}
	res[wcslen(str1) + wcslen(str2)] = L'\0';
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
	malloc_list->num_elements = 0;
	free_list = (Malloc_list *)malloc(sizeof(Malloc_list));
	free_list->num_elements = 0;
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
	cur_start = (wchar_t *)my_malloc((wcslen(L"\033[27m")+2)*sizeof(wchar_t));
	cur_stop = (wchar_t *)my_malloc((wcslen(L"\033[7m")+2)*sizeof(wchar_t));
	cur_start = L"\033[7m\0";
	cur_stop = L"\033[27m\0";
	return;
}

wchar_t **get_str_for_subst(wchar_t *command, wchar_t *str1, wchar_t *str2)
{
	wchar_t *buffer;
	wchar_t **res;
	int c = 0;
	int j = 0;
	int i = 0;

	wprintf(L"cmd '%ls'\n", command); 
	if (wcsstr(command, L"subst /") != command)
	{
		return NULL;
	}
	str1 = (wchar_t *)my_malloc(sizeof(wchar_t));
	str2 = (wchar_t *)my_malloc(sizeof(wchar_t));
	str1[0] = L'\0';
	str2[0] = L'\0';
	buffer = &(command[7]);
	for (i = 0; i < (int)wcslen(buffer); i++)
	{
		wprintf(L"str1: '%ls', str2: '%ls', c: %d, buffer[%d] == %lc\n", str1, str2, c, i, buffer[i]);
		if (c > 1)
		{
			return NULL;
		}
		if (i < (int)wcslen(buffer)-1 && buffer[i] == L'\\' && buffer[i+1] == L'/')
		{
			if (c == 0)
			{
				str1[j++] = L'/';
				str1 = (wchar_t *)my_realloc(str1, (j+1)*sizeof(wchar_t));
				str1[j] = L'\0';
			}
			else
			{
				str2[j++] = L'/';
				str2 = (wchar_t *)my_realloc(str2, (j+1)*sizeof(wchar_t));
				str2[j] = L'\0';
			}
			i++;
		}
		else if (i < (int)wcslen(buffer) -1 && buffer[i] == L'\\' && buffer[i+1] == L'\\')
		{
			if (c == 0)
			{
				str1[j++] = L'\\';
				str1 = (wchar_t *)my_realloc(str1, (j+1)*sizeof(wchar_t));
				str1[j] = L'\0';
			}
			else
			{
				str2[j++] = L'\\';
				str2 = (wchar_t *)my_realloc(str2, (j+1)*sizeof(wchar_t));
				str2[j] = L'\0';
			}
			i++;
		}
		else if (i < (int)wcslen(buffer) - 1 && buffer[i] == L'\\' && buffer[i+1] == L'n')
		{
			if (c == 0)
			{
				str1[j++] = L'\n';
				str1 = (wchar_t *)my_realloc(str1, (j+1)*sizeof(wchar_t));
				str1[j] = '\0';
			}
			else
			{
				str2[j++] = L'\n';
				str2 = (wchar_t *)my_realloc(str2, (j+1)*sizeof(wchar_t));
				str2[j] = '\0';
			}
			i++;
		}
		else if (buffer[i] == L'/')
		{
			c++;
			j = 0;
		}
		else
		{
			if (c == 0)
			{
				str1[j++] = buffer[i];
				str1 = (wchar_t *)my_realloc(str1, (j+1)*sizeof(wchar_t));
				str1[j] = L'\0';
			}
			else
			{
				str2[j++] = buffer[i];
				str2 = (wchar_t *)my_realloc(str2, (j+1)*sizeof(wchar_t));
				str2[j] = L'\0';
			}
		}
	}
	if (c == 2)
	{
		res = (wchar_t **)my_malloc(2*sizeof(wchar_t*));
		res[0] = str1;
		res[1] = str2;
		wprintf(L"res[0]: '%ls'\nres[1]: '%ls'\n", res[0], res[1]);
		return res;
	}
	else
	{
		return NULL;
	}
}

void print_help()
{
	wprintf(L"./less [-n] [-o file_name] [-v] [-h]\n");
	wprintf(L"\t-v \t\t: Версия программы\n");
	wprintf(L"\t-h \t\t: Помощь\n");
	wprintf(L"\t-n \t\t: Нумерация строк\n");
	wprintf(L"\t-o file_name \t: Название файла\n");
	wprintf(L"\n\n");
	return;
}
