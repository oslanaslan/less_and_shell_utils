#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct list_node
{
	char *str;
	struct list_node *prev;
	struct list_node *next;
};

typedef struct
{
	unsigned int num_elements;
	struct list_node *head;
} Bidirect_list;

Bidirect_list *list;

void list_fun(char *str);
void list_append(char *str);
void list_print();
void list_remove();
int cmp(char *str, char *str2);
void node_remove(struct list_node *node);

int main(int argc, char **argv)
{
	char c;
	char *buffer;
	int j = 0;

	if (argc == 1)
	{
		printf("Введите параметр\n");
		return 1;
	}
	list = (Bidirect_list *)malloc(sizeof(Bidirect_list));	
	list->num_elements = 0;
	list->head = NULL;
	buffer = (char *)malloc(sizeof(char));
	buffer[0] = '\0';
	if (buffer == NULL)
		return 1;
	j = 0;
	while ((c = getchar()) != EOF)
	{
		if (c == '\n')
		{
			if (buffer[0] == '\0')
			{
				free(buffer);
				break;
			}
			list_append(buffer);
			j = 0;
			buffer = (char *)malloc(sizeof(char));
			if (buffer == NULL)
				return 1;
			buffer[0] = '\0';
		}
		else
		{
			buffer[j++] = c;
			buffer = (char *)realloc(buffer, (j+2)*sizeof(char));
			if (buffer == NULL)
				return 1;
			buffer[j] = '\0';
		}
	}
	printf("\nДо:\n");
	list_print();

	list_fun(argv[1]);
	printf("\nПосле:\n");
	list_print();

	list_remove();
	return 0;
}

void list_append(char *str)
{
	struct list_node *node;

	node = (struct list_node *)malloc(sizeof(struct list_node));
	if (node == NULL)
		exit(1);
	node->str = str;
	list->num_elements++;
	if (list->head == NULL)
	{
		node->next = node;
		node->prev = node;
		list->head = node;
	}
	else
	{
		node->next = list->head;
		node->prev = list->head->prev;
		list->head->prev->next = node;
		list->head->prev = node;
	}
	return;
}

void list_print()
{
	struct list_node *node = list->head;

	if (node == NULL)
	{
		printf("Пустой список\n");
		return;
	}
	printf("%s\n", node->str);
	node = node->next;
	while (node != list->head)
	{
		printf("%s\n", node->str);
		node = node->next;
	}
	return;
}

void list_remove()
{
	struct list_node *node, *n_node;

	node = list->head;
	if (node == NULL)
		return;
	do
	{
		n_node = node->next;
		free(node->str);
		free(node);
		node = n_node;
	}
	while (node != list->head);
	free(list);
	return;
}

void list_fun(char *str)
{
	struct list_node *node, *n_node;

	node  = list->head;
	if (list->head == NULL)
	{
		char *buffer = (char *)malloc(sizeof(char)*(strlen(str)+1));
		node = (struct list_node *)malloc(sizeof(struct list_node));
		strcpy(buffer, str);
		node->next = node;
		node->prev = node;
		node->str = buffer;
		list->head = node;
		return;
	}
	do
	{
		if (cmp(node->str, str))
		{
			if (list->num_elements <= 1)
			{
				list->head = NULL;
				list->num_elements = 0;
				break;
			}			
			else
			{
				node->next->prev = node->prev;
				node->prev->next = node->next;
				list->num_elements--;
				if (list->head == node)
				{
					list->head = node;
				}
				n_node = node->next;
				free(node->str);
				free(node);
				node = n_node;
			}
			
		}
		else
		{
			list->head = node;
			break; 
		}
	}
	while (node != list->head);
	return;
}

int cmp(char *str1, char *str2)
{
	return strcmp(str1, str2) < 0 ? 1 : 0;	
}
