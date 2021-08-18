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

Bidirect_list *main_list;

/* Функции */

/* Добавить строку в конец списка */
int list_append(Bidirect_list *main_list, char *buffer);

/* Вывод строки на экран */
int list_print(Bidirect_list *list, int need_num, int *cur_x, int *cur_y, int max_left, int first, int *cur_str_len);

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
char *get_str_for_pr(char *str, int need_num, int n, int need_curs, int max_left, int *cur_loc_x);

/* Управляет выводом на экран */
char *screen_handler(int key, int *cur_loc_x, int *cur_loc_y, int *max_left, int *first, Bidirect_list *list, int need_num, int *cur_str_len);

/* Возвращате массив unsignt int, содержащий коды нажатой клавиши */
int getKeyPress();
