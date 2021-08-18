#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

/* 30, ... , 37 */
#define USER_COL 32
#define PWD_COL 34
#define CTRL_Z 25856

struct program 
{
	char *name;
	int argc;
	char **argv;
	char *input;
	char *output;
	int in;
	int out;
	int out_type; /* 1 - rewrite, 2 - append */
	int back; /* 1 - background, 2 - foreground */
	int conv;
};
/* Structs */
typedef struct
{
	struct program **com;
	int num_progs;
	char *str;
} Command;

struct hist_node
{
	char *cmd;
	struct hist_node *next;
	struct hist_node *prev;
};

struct
{
	struct hist_node *head;
	struct hist_node *tail;
	int hist_num;
} History;

struct job
{
	char *name;
	pid_t pid;
	struct job *next;
	struct job *prev;
};

struct
{
	int num;
	int stat;
	struct job *head;
	struct job *tail;
} jobs;


/* Globs */
char **shell_argv;
int shell_argc;
pid_t foreground;
struct termios old_attributes, new_attributes;

/* FU */

char *get_command();
char *get_path(char *path);
Command *parser(char *str);
int command_checker(Command *cmd);
void hist_append(char *buffer);
char *get_command_by_number(int n);
void start_command(Command *cmd);
char *get_user_name();
void handler(int sig);
void free_prog(struct program *prog);
void exit_prog(Command *cmd);
void free_cmd(Command* cmd);
void refresh_jobs();
char **get_jobs();
char **history();

/* MA */
int main(int argc, char **argv)
{
	char *buffer;
	char *user_name;
	char *pwd;
	Command *cmd;
	int correct;
	int i = 0;

	tcgetattr(0, &old_attributes);
	signal(SIGINT, handler);
	signal(SIGTTOU, handler);
	signal(SIGSTOP, SIG_IGN);
	signal(SIGTSTP, handler); 
	shell_argc = argc; shell_argv = argv;
	jobs.num = 0; jobs.stat = 0;
	jobs.head = NULL;
	jobs.tail = NULL;
	History.hist_num = 0;
	History.head = NULL;
	History.tail = NULL;

	while (1)
	{
		user_name = get_user_name();
		pwd = getcwd(NULL, 0); 
		tcsetattr(0, TCSANOW, &old_attributes);
		fflush(stdin);
		fflush(stdout);
		printf("\033[%d;1m%s@debian:\033[0m \033[%d;1m%s\033[0m$ ", USER_COL, user_name, PWD_COL, pwd); 
		free(user_name);
		free(pwd);
		fflush(stdout);
		buffer = get_command();
		if (buffer == NULL) /* Ошибка подстановки */
		{
			continue;
		}
		if (buffer[0] == '\0') /* Игнорировать простое нажате enter */
		{
			free(buffer);
			continue;
		}
		hist_append(buffer);
		cmd = parser(buffer);
		free(buffer);
		correct = command_checker(cmd);
/*		for (i = 0; i < cmd->num_progs; i++)
		{
			struct program *prog;
			int j = 0;
		
			prog = (cmd->com)[i];
	
			printf("name: %s\n", prog->name);
			printf("argc: %d\n", prog->argc);
			printf("argv: ");
			for (j = 0; j < prog->argc; j++)
			{
				printf("%s; ", (prog->argv)[j]);
			}
			printf("\n");
			printf("input: %s\n", prog->input);
			printf("output: %s\n", prog->output);
			printf("in: %d\n", prog->in);
			printf("out: %d\n", prog->out);
			printf("out_type: %d\n", prog->out_type);
			printf("back: %d\n", prog->back);
			printf("conv: %d\n", prog->conv);
			printf("\n"); 
		}*/
		if (!correct)
			start_command(cmd);
		else 
		{
			printf("ERROR: Команда введена неправильно\n");
		}
		free_cmd(cmd);
	}

	return 0;
}

/* GC */
char *get_command()
{
	char *buffer = NULL;
	char c;
	int i = 0;
	int flg = 0;

	while ((c = getchar()) != EOF)
	{
		if (buffer == NULL)
		{
			buffer = (char *)malloc(sizeof(char));
			buffer[0] = '\0';
		}
here:
		if (c == '\\' &&  flg < 2)
		{
			c = getchar();
			switch (c)
			{
				case 't':
					buffer[i++] = '\t';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					break;
				case 'r':
					buffer[i++] = '\r';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					break;
				case 'n':
					buffer[i++] = '\n';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					break;
				case '!':
					buffer[i++] = '!';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					break;
				case '\\':
					buffer[i++] = '\\';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					break;
				case '$':
					buffer[i++] = '$';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					break;
				case '\n':
					printf(">");
					fflush(stdout);
					break;
				case '&':
					buffer[i++] = '\'';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					buffer[i++] = '&';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					buffer[i++] = '\'';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					break;
				case '<':
					buffer[i++] = '\'';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					buffer[i++] = '<';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					buffer[i++] = '\'';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					break;
				case '>':
					buffer[i++] = '\'';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					buffer[i++] = '>';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					buffer[i++] = '\'';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					break;
				case '|':
					buffer[i++] = '\'';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					buffer[i++] = '|';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					buffer[i++] = '\'';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					break;
				case ';':
					buffer[i++] = '\'';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					buffer[i++] = ';';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					buffer[i++] = '\'';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					break;
				default:
					buffer[i++] = '\\';
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
					goto here;
					break;
			}
			
		}
		else if (c == '\n')
		{
			if (flg == 0)
			{
				return buffer;
			}
			else
			{
				printf(">");
				fflush(stdout);
			}
		}
		else if (c == '"')
		{
			if (i == 0 || buffer[i-1] != '\\')
			{
				if (flg == 0)
				{
					flg = 1;
				}
				else if (flg == 1)
				{
					flg = 0;
				}
			}
			buffer[i++] = '"';
			buffer = (char *)realloc(buffer, (i+1)*sizeof(buffer));
			buffer[i] = '\0';
		}
		else if (c == '\'')
		{
			if (i == 0 ||  buffer[i-1] != '\\')
			{
				if (flg == 2)
				{
					flg = 0;
				}
				else if (flg == 0)
				{
					flg = 2;
				}
			}
			buffer[i++] = '\'';
			buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
			buffer[i] = '\0';
		}
		else if (c == ' ' && flg == 0 && ((i > 0 && buffer[i-1] == ' ') || (i == 0)))
		{
			;
		}
		else if (c == '$' && flg < 2 && (i == 0 || (i > 0 && buffer[i-1] != '\\')))
		{
			char *path = (char *)malloc(sizeof(char));
			char *some;
			int j = 0;

			path[0] = '\0';
			c = getchar();
			while (c != ' ' && c != '\n' && c !=  '\'' && c != '"' && c != ';' && c != '&' && c != '<' && c != '>' && c != '!')
			{
				path[j++] = c;
				path = (char *)realloc(path, (j+1)*sizeof(char));
				path[j] = '\0';
				c = getchar();
			}
			some = get_path(path);
			free(path);
			path = some;
			for (j = 0; j < (int)strlen(path); j++)
			{
				buffer[i++] = path[j];
				buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
				buffer[i] = '\0';
			}
			free(path);
			goto here;
		}
		else if (c == '!' && flg < 2 && (i == 0 || (i > 1 && buffer[i-1] != '\\'))) /* HISTORY */
		{
			char *buf = (char *)malloc(sizeof(char));
			int j = 0;

			buf[0] = '\0';
			c = getchar();
			while (c <= '9' && c >= '0')
			{
				buf[j++] = c;
				buf = (char *)realloc(buf, (j+1)*sizeof(char));
				buf[j] = '\0';
				c = getchar();
			}
			if (strlen(buf) == 0)
			{
				buffer[i++] = '!';
				buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
				buffer[i] = '\0';
				free(buf);
			}
			else
			{
				j = atoi(buf);
				free(buf);
				buf = get_command_by_number(j);
				if (buf == NULL)
				{
					free(buffer);
					printf("!%d: Запись не найдена\n", j);
					free(buf);
					fflush(stdin);
					return NULL;
				}
				for (j = 0; j < (int)strlen(buf); j++)
				{
					buffer[i++] = buf[j];
					buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
					buffer[i] = '\0';
				}
				printf("%s\n", buf);
				free(buf);
			}
			buffer[i++] = ' ';
			buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
			buffer[i] = '\0';
			goto here;
		}
		else if (c == '#' && (i == 0 || (i > 0 && buffer[i-1] != '\\')) && flg == 0)
		{
			fflush(stdin);
			return buffer;	
		}
		else if (c == '#' && i != 0 && buffer[i-1] == '\\')
		{
			buffer[i-1] = '#';
			buffer[i] = '\0';
		}
		else if (c == '#' && flg != 0)
		{
			buffer[i++] = '#';
			buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
			buffer[i] = '\0';
		}
		else
		{
			buffer[i++] = c;
			buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
			buffer[i] = '\0';
		}
	}
	return NULL;
}

char *get_path(char *path)
{
	int i;
	char *buffer;

	if (strcmp(path, "0") == 0||atoi(path) > 0 && atoi(path) < shell_argc) /* $цифра */
	{
		buffer = (char *)malloc(sizeof(char)*(strlen(shell_argv[atoi(path)])+1));
		sprintf(buffer, "%s", shell_argv[atoi(path)]);
	}
	else if (strlen(path) == 1 && path[0] == '?') /* $? */
	{
		buffer = (char *)malloc(sizeof(char)*sizeof(int)*8);
		sprintf(buffer, "%d", jobs.stat);
	}
	else if (strcmp(path, "USER") == 0) /* $USER */
	{
		buffer = get_user_name();
	}
	else if (strcmp(path, "SHELL") == 0) /* $SHELL */
	{
		buffer = shell_argv[0];
	}
	else if (strcmp(path, "PID") == 0) /* $PID */
	{
		pid_t pid = getpid();

		buffer = (char *)malloc(sizeof(char)*(int)pid);
		sprintf(buffer, "%d", pid);
	}
	else if (strcmp(path, "UID") == 0) /* $UID */
	{
		unsigned int k = getuid();

		buffer = (char *)malloc(sizeof(char)*sizeof(unsigned int)*8);
		sprintf(buffer, "%u", k);	
	}
	else if (strcmp(path, "PWD") == 0) /* PWD */
	{
		buffer = getcwd(NULL, 0);
	}
	else if (strcmp(path, "#") == 0) /* # */
	{
		buffer = (char *)malloc(sizeof(char)*sizeof(int)*8);
		sprintf(buffer, "%d", shell_argc);
	}
	else if (getenv(path) != NULL) /* Переменная из окружения */
	{
		char *buf = getenv(path);

		buffer = (char *)malloc(sizeof(char)*(strlen(buf)+1));
		strcpy(buffer, buf);
	}
	else /* Если не удалось найти переменную, вернуть просто саму переменную */
	{
		buffer = (char *)malloc(sizeof(char)*(strlen(path)+2));
		i = 1;
			
		printf("%s\n", path);
		buffer[0] = '$';
		for (i = 0; i < (int)strlen(path); i++)
			buffer[i+1] = path[i];
		buffer[strlen(path)+1] = '\0';
	}
	return buffer;

}

/* PA */
Command *parser(char *n_str)
{
	Command *cmd = (Command *)malloc(sizeof(Command)); 
	struct program *prog = (struct program *)malloc(sizeof(struct program));
	char *buffer = (char *)malloc(sizeof(char));
	int i, j, in_flg, out_flg;
	char *str;

/*	printf("parser started '%s'\n", n_str); */
	cmd->com = (struct program **)malloc(sizeof(struct program));
	cmd->num_progs = 0;
	cmd->str = n_str;

	/* Костыль */
	i = strlen(n_str);
	str = (char *)malloc(sizeof(char)*(strlen(n_str)+1));
	if ((i > 1 && n_str[i-1] == ' ' && (n_str[i-2] == '&' || n_str[i-2] == ';')) || (i > 0 && (n_str[i-1] == '&' || n_str[i-1] == ';')))
	{
		strcpy(str, n_str);
	}
	else
	{
		str = (char *)realloc(str, (i+2)*sizeof(char));
		strcpy(str, n_str);
		str[i] = ';';
		str[i+1] = '\0'; 
	}
	i = 0;

/*	printf("%s\n", str); */

	prog->name = NULL;
	prog->argc = 0;
	prog->argv = NULL;
	prog->input = NULL;
	prog->output = NULL;
	prog->in = 0;
	prog->out = 0;
	prog->out_type = 0;
	prog->back = 2;
	prog->conv = 0;
/*	i - счетчик по str (исходная строка), j - счетчик по buffer (набиваемое слово) */
	i = 0; j = 0; in_flg = 0; out_flg = 0; buffer[0] = '\0'; 
	for (i = 0; i < (int)strlen(str); i++)
	{
		switch (str[i])
		{
			case '"':
				i++;
				do
				{
					if (str[i] == '\\' && str[i+1] == '"')
						i++;
					buffer[j++] = str[i];
					buffer = (char *)realloc(buffer, (j+1)*sizeof(char));
					buffer[j] = '\0';
					i++;
				}
				while (str[i] != '"' || str[i-1] == '\\');
/*				buffer[j++] = '"';
				buffer = (char *)realloc(buffer, (j+1)*sizeof(char));
				buffer[j] = '\0'; */
				break;
			case '\'':
				i++;
				do
				{
					if (str[i] == '\\' && str[i+1] == '\'')
						i++;
					buffer[j++] = str[i];
					buffer = (char *)realloc(buffer, (j+1)*sizeof(char));
					buffer[j] = '\0';
					i++;
				}
				while (str[i] != '\'' || str[i-1] == '\\');
/*				buffer[j++] = '\'';
				buffer = (char *)realloc(buffer, (j+1)*sizeof(char));
				buffer[j] = '\0'; */
				break;
			case ' ':
				if (buffer[0] == '\0') 
				{
					break;
				}
				if (j > 0 && buffer[j-1] == '\\')
				{
					buffer[j-1] = ' ';
					buffer[j] = '\0';
					break;
				}
				if (prog->name == NULL && in_flg == 0 && out_flg == 0)
				{
					prog->name = buffer;
				}
				else if (buffer[0] != '\0' && in_flg == 0 && out_flg == 0)
				{
					(prog->argc)++;
					if (prog->argv == NULL)
					{
						prog->argv = (char **)malloc(sizeof(char *)*2);
					}
					else
					{
						prog->argv = (char **)realloc(prog->argv, ((prog->argc)+1)*sizeof(char *));
					}
					(prog->argv)[prog->argc-1] = buffer;
					(prog->argv)[prog->argc] = NULL;
				}
				else if (buffer[0] != '\0' && in_flg == 0 && out_flg == 1)
				{
					prog->output = buffer;
					prog->out = 1;
					out_flg = 0;
				}
				else if (buffer[0] != '\0' && out_flg == 0 && in_flg == 1)
				{
					prog->input = buffer;
					prog->in = 1;
					in_flg = 0;
				}
/*				else 
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("error "));

					sprintf(buf, "error");
					if (prog->input != NULL)
						free(prog->input);
					prog->input = buf;
					buf = (char *)malloc(sizeof(char)*strlen("error "));
					sprintf(buf, "error");
					if (prog->output != NULL)
						free(prog->output);
					prog->output = buf;
				} */
				if (buffer[0] != '\0')
				{
					buffer = (char *)malloc(sizeof(char));
					j = 0;
					buffer[0] = '\0';
				}
				break;
			case '<':
				if (buffer[0] != '\0' && in_flg == 0 && out_flg == 0)
				{
					if (prog->name == NULL)
					{
						prog->name = buffer;
					}
					else
					{
						prog->argc ++;
						prog->argv = (char **)realloc(prog->argv, (prog->argc+1)*sizeof(char *));
						(prog->argv)[prog->argc-1] = buffer;
						(prog->argv)[prog->argc] = NULL;
					}
				}
				else if (buffer[0] != '\0' && in_flg == 0 && out_flg == 1)
				{
					prog->output = buffer;
					prog->out = 1;
					out_flg = 0;
				}
				else if (buffer[0] != '\0' && out_flg == 0 && in_flg == 1)
				{
					prog->input = buffer;
					prog->in = 1;
					in_flg = 0;
				}
/*				else 
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("error "));

					sprintf(buf, "error");
					if (prog->input != NULL)
					{
						free(prog->input);
					}
					prog->input = buf;
					buf = (char *)malloc(sizeof(char)*strlen("error "));
					sprintf(buf, "error");
					if (prog->output != NULL)
					{
						free(prog->output);
					}
					prog->output = buf;
				} */
				if (buffer[0] != '\0')
				{
					buffer = (char *)malloc(sizeof(char));
					j = 0;
					buffer[0] = '\0';
				}

				if (str[i+1] == '<')
				{
					i++;
				}
				in_flg = 1;
				break;
			case '>':
				if (buffer[0] != '\0' && in_flg == 0 && out_flg == 0)
				{
					if (prog->name == NULL)
					{
						prog->name = buffer;
					}
					else
					{
						prog->argc ++;
						prog->argv = (char **)realloc(prog->argv, (prog->argc+1)*sizeof(char *));
						(prog->argv)[prog->argc-1] = buffer;
						(prog->argv)[prog->argc] = NULL;
					}
				}
				else if (buffer[0] != '\0' && in_flg == 0 && out_flg == 1)
				{
					prog->output = buffer;
					prog->out = 1;
					out_flg = 0;
				}
				else if (buffer[0] != '\0' && out_flg == 0 && in_flg == 1)
				{
					prog->input = buffer;
					prog->in = 1;
					in_flg = 0;
				}
/*				else 
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("error "));

					sprintf(buf, "error");
					if (prog->input != NULL)
						free(prog->input);
					prog->input = buf;
					buf = (char *)malloc(sizeof(char)*strlen("error "));
					sprintf(buf, "error");
					if (prog->output != NULL)
						free(prog->output);
					prog->output = buf;
				} */
				if (buffer[0] != '\0')
				{
					buffer = (char *)malloc(sizeof(char));
					buffer[0] = '\0';
					j = 0;
				}

				if (str[i+1] == '>')
				{
					i++;
					prog->out_type = 2;
				}
				else
				{
					prog->out_type = 1;
				}
				out_flg = 1;
				break;
			case ';':
				
				if (buffer[0] != '\0' && in_flg == 0 && out_flg == 0)
				{
					if (prog->name == NULL)
					{
						prog->name = buffer;
					}
					else 
					{
						prog->argc++;
						prog->argv = (char **)realloc(prog->argv, (prog->argc+1)*sizeof(char *));
						(prog->argv)[prog->argc-1] = buffer;
						(prog->argv)[prog->argc] = NULL;
					}
				}	
				else if (buffer[0] != '\0' && in_flg == 0 && out_flg == 1)
				{
					prog->output = buffer;
					prog->out = 1;
					out_flg = 0;
				}
				else if (buffer[0] != '\0' && out_flg == 0 && in_flg == 1)
				{
					prog->input = buffer;
					prog->in = 1;
					in_flg = 0;
				}
/*				else 
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("error "));

					sprintf(buf, "error");
					if (prog->input != NULL)
						free(prog->input);
					prog->input = buf;
					buf = (char *)malloc(sizeof(char)*strlen("error "));
					sprintf(buf, "error");
					if (prog->output != NULL)
						free(prog->output);
					prog->output = buf;
				} */
				if (buffer[0] != '\0')
				{
					buffer = (char *)malloc(sizeof(char));
					j = 0;
					buffer[0] = '\0';

				}
				if (prog->input == NULL && in_flg == 0 && out_flg == 0)
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("stdin "));

					sprintf(buf, "stdin");
					prog->input = buf;
				}
				
				if (prog->output == NULL && in_flg == 0 && out_flg == 0)
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("stdout "));

					sprintf(buf, "stdout");
					prog->output = buf;
				}

				cmd->num_progs++;
				cmd->com = (struct program **)realloc(cmd->com, (cmd->num_progs)*sizeof(struct program *));
				(cmd->com)[cmd->num_progs-1] = prog;
				prog = (struct program *)malloc(sizeof(struct program));
				prog->name = NULL;
				prog->argc = 0;
				prog->argv = NULL;
				prog->input = NULL;
				prog->output = NULL;
				prog->in = 0;
				prog->out = 0;
				prog->out_type = 0;
				prog->back = 2;
				prog->conv = 0;
				break;
			case '&':
				if (buffer[0] != '\0' && in_flg == 0 && out_flg == 0)
				{
					if (prog->name == NULL)
					{
						prog->name = buffer;
					}
					else 
					{
						prog->argc++;
						prog->argv = (char **)realloc(prog->argv, (prog->argc+1)*sizeof(char *));
						(prog->argv)[prog->argc-1] = buffer;
						(prog->argv)[prog->argc] = NULL;
					}
				}	
				else if (buffer[0] != '\0' && in_flg == 0 && out_flg == 1)
				{
					prog->output = buffer;
					prog->out = 1;
					out_flg = 0;
				}
				else if (buffer[0] != '\0' && out_flg == 0 && in_flg == 1)
				{
					prog->input = buffer;
					prog->in = 1;
					in_flg = 0;
				}
/*				else 
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("error "));

					sprintf(buf, "error");
					if (prog->input != NULL)
						free(prog->input);
					prog->input = buf;
					buf = (char *)malloc(sizeof(char)*strlen("error "));
					sprintf(buf, "error");
					if (prog->output != NULL)
						free(prog->output);
					prog->output = buf;
				}*/
				if (buffer[0] != '\0')
				{
					buffer = (char *)malloc(sizeof(char));
					j = 0;
					buffer[0] = '\0';

				}

				if (prog->input == NULL && in_flg == 0 && out_flg == 0)
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("stdin "));

					sprintf(buf, "stdin");
					prog->input = buf;
				}
				if (prog->output == NULL && in_flg == 0 && out_flg == 0)
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("stdout "));

					sprintf(buf, "stdout");
					prog->output = buf;
				}
				prog->back = 1;
				cmd->num_progs++;
				cmd->com = (struct program **)realloc(cmd->com, (cmd->num_progs)*sizeof(struct program *));
				(cmd->com)[cmd->num_progs-1] = prog;
				prog = (struct program *)malloc(sizeof(struct program));
				prog->name = NULL;
				prog->argc = 0;
				prog->argv = NULL;
				prog->input = NULL;
				prog->output = NULL;
				prog->in = 0;
				prog->out = 0;
				prog->out_type = 0;
				prog->back = 2;
				prog->conv = 0;
				break;
			case '|':
				prog->conv = 1;
				if (buffer[0] != '\0' && in_flg == 0 && out_flg == 0)
				{
					if (prog->name == NULL)
					{
						prog->name = buffer;
					}
					else 
					{
						prog->argc++;
						prog->argv = (char **)realloc(prog->argv, (prog->argc+1)*sizeof(char *));
						(prog->argv)[prog->argc-1] = buffer;
						(prog->argv)[prog->argc] = NULL;
					}
				}
				else if (buffer[0] != '\0' && in_flg == 0 && out_flg == 1)
				{
					prog->output = buffer;
					prog->out = 1;
					out_flg = 0;
				}
				else if (buffer[0] != '\0' && out_flg == 0 && in_flg == 1)
				{
					prog->input = buffer;
					prog->in = 1;
					in_flg = 0;
				}
/*				else 
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("error "));

					sprintf(buf, "error");
					if (prog->input != NULL)
						free(prog->input);
					prog->input = buf;
					buf = (char *)malloc(sizeof(char)*strlen("error "));
					sprintf(buf, "error");
					if (prog->output != NULL)
						free(prog->output);
					prog->output = buf;
				}*/

				if (prog->output != NULL && in_flg == 0 && out_flg == 0 && prog->out == 0)
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("next "));

					sprintf(buf, "next");
					prog->out = 0;
					free(prog->output);
					prog->output = buf;
					buffer = (char *)malloc(sizeof(char));
					buffer[0] = '\0';
					j = 0;
				}
				else if (in_flg == 0 && out_flg == 0 && prog->out == 0)
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("next "));

					sprintf(buf, "next");
					prog->out = 0;
					prog->output = buf;
				}
				if (prog->input == NULL && in_flg == 0 && out_flg == 0)
				{
					char *buf = (char *)malloc(sizeof(char)*strlen("stdin "));

					sprintf(buf, "stdin");
					prog->in = 0;
					prog->input = buf;
				}
				if (buffer[0] != '\0')
				{
					buffer = (char *)malloc(sizeof(char));
					j = 0;
					buffer[0] = '\0';

				}

				prog->back = 2;
				cmd->num_progs++;
				cmd->com = (struct program **)realloc(cmd->com, (cmd->num_progs)*sizeof(struct program *));
				(cmd->com)[cmd->num_progs-1] = prog;
				prog = (struct program *)malloc(sizeof(struct program));
				prog->name = NULL;
				prog->argc = 0;
				prog->argv = NULL;
				prog->input = NULL;
				prog->output = NULL;
				prog->in = 0;
				prog->out = 0;
				prog->out_type = 0;
				prog->back = 2;
				prog->conv = 0;
				break;
			default:
				buffer[j++] = str[i];
				buffer = (char *)realloc(buffer, (j+1)*sizeof(char));
				buffer[j] = '\0';
				break;	
		}
	}
/*	if (buffer[0] != '\0')
	{
		if (prog->name == NULL)
		{
			prog->name = buffer;
		}
		else
		{
			prog->argc++;
			prog->argv = (char **)realloc(prog->argv, (prog->argc)*sizeof(char *));
			(prog->argv)[prog->argc-1] = buffer;
		}
	}
	else
	{
		free(buffer);
	}
	if (prog->input == NULL)
	{
		buffer = (char *)malloc(sizeof(char)*strlen("stdin "));

		sprintf(buffer, "stdin");
		prog->input = buffer;
		printf("here\n");
	}
	if (prog->output ==  NULL)
	{
		buffer = (char *)malloc(sizeof(char)*strlen("stdout "));

		sprintf(buffer, "stdout");
		prog->output = buffer;
		printf("here\n");
	}
	prog->out_type = 1;
	cmd->num_progs++;
	cmd->com = (struct program **)realloc(cmd->com, (cmd->num_progs)*sizeof(struct program *));
	(cmd->com)[cmd->num_progs-1] = prog; */
	if (prog->name != NULL)
		free(prog->name);
	if (prog->argc != 0)
	{
		for (i = 0; i < prog->argc; i++)
		{
			free((prog->argv)[i]);
		}
		free(prog->argv);	
	}
	if (prog->input != NULL)
		free(prog->input);
	if (prog->output != NULL)
		free(prog->output);
	free(prog);
	free(buffer);
	free(str);

	return cmd;
}

/* CC */
int command_checker(Command *cmd)
{
	int i,n,flg, next, closed;
	struct program *prog;
	
	flg = 0; next = 0; closed = 0;
	n = cmd->num_progs;
	for (i = 0; i < n; i++)
	{
		prog = (cmd->com)[i];
		if (prog->name == NULL)
		{
			flg = -1;
			break;
		}
		if (prog->input == NULL)
		{
			flg = -1;
			break;
		}
		if (prog->output == NULL)
		{
			flg = -1;
			break;
		}
		if (next == 1 && prog->in == 0)
		{
			char *buffer = (char *)malloc(sizeof(char)*strlen("prev "));

			free(prog->input);
			sprintf(buffer, "prev");
			prog->input = buffer;
			prog->in = 0;
			next = 0;
		}
		else if (next == 1 && prog->in == 1)
		{
			char *buffer = (char *)malloc(sizeof(char)*strlen("close "));

			((cmd->com)[i-1])->out = 0;
			free(((cmd->com)[i-1])->output);
			sprintf(buffer, "close");
			((cmd->com)[i-1])->output = buffer;
			next = 0;
		}
		if (closed == 1 && prog->in == 0)
		{
			char *buffer = (char *)malloc(sizeof(char)*strlen("close "));

			free(prog->input);
			sprintf(buffer, "close");
			prog->input = buffer;
			prog->in = 0;
			closed = 0;
		}
		if (strcmp(prog->output, "next") == 0)
			next = 1;

		if (prog->conv == 1 && prog->out == 1)
		{
			closed = 1;
		}
		if (prog->back == 0)
			prog->back = 2;
	}
	return flg;
		
}

/*void hist_append(char *)
{
	struct hist_node *node = (struct hist_node *)malloc(sizeof(struct hist_node));

	node->cmd = cmd;
	if (History.head == NULL)
	{
		node->next = NULL;
		node->prev = NULL;
		History.head = node;
		History.tail = node;
	}
	else
	{
		node->next = NULL;
		node->prev = History.tail;
		History.tail->next = node;
		History.tail = node;
	}
	return;
}*/

char *get_command_by_number(int n)
{
	int i = 0;
	struct hist_node *node = History.head;
	char *buf;

	for (i = 0; i < n; i++)
	{
		if (node == NULL)
			break;
		node = node->next;
	}
	if (node == NULL)
		return NULL;
	buf = (char *)malloc(sizeof(char)*(strlen(node->cmd)+2));
	strcpy(buf, node->cmd);
	buf[strlen(node->cmd)] = ' ';
	buf[strlen(node->cmd)+1] = '\0';
	return buf;
}

char **history()
{
	struct hist_node *node = History.head;
	char **res = (char **)malloc(sizeof(char *));
	int i = 0;
	res[0] = NULL;

	while (node != NULL)
	{
		char *buffer = (char *)malloc(sizeof(char)*(strlen(node->cmd)+3));

		sprintf(buffer, "\t%s\n", node->cmd);
		res[i++] = buffer;
		res = (char **)realloc(res, (i+1)*sizeof(char *));
		res[i] = NULL;
		node = node->next;

	}
	return res;
}

/* SC */
void start_command(Command *cmd)
{
	int i;
	struct program *prog;
	int fd[2];
	int prev = 0;
	pid_t son;
		
	for (i = 0; i < cmd->num_progs; i++)
	{
		prog = (cmd->com)[i];
		if (strcmp(prog->name, "exit") == 0) /* exit */
		{
			exit_prog(cmd); 
			exit(0);
		}
		else if (strcmp(prog->name, "history") == 0) /* history */
		{
			char *buffer = (char *)malloc(sizeof(char)*strlen("echo "));
			free(prog->name);
			sprintf(buffer, "echo");
			prog->name = buffer;
			if (prog->argc != 0)
			{
				int j = 0;
				for (j = 0; j < prog->argc; j++)
				{
					free(prog->argv[j]);
				}
				free(prog->argv);
				prog->argc = 0;
			}
			prog->argv = history();
			prog->argc = History.hist_num;

		}
		else if (strcmp(prog->name, "cd") == 0) /* cd */
		{
			if (prog->argc > 1)
			{
				printf("\"cd\": Слишком много аргументов\n");
				continue;
			}
			else if (prog->argc < 1)
			{
				char *user;
				char *buf;
				user = get_user_name();
				buf = (char *)malloc(sizeof(char)*(strlen("/home// ")+strlen(user)+1));
				prog->argv = (char **)malloc(sizeof(char*)*2);
				sprintf(buf, "/home/%s/", user);
				free(user);
				(prog->argv)[0] = buf;
				(prog->argv)[1] = NULL;

			}
			if (chdir(prog->argv[0]) < 0)
			{
				printf("\"cd\": \"%s\": Нет такого файла или каталога\n", prog->argv[0]);
				continue;
			}
			continue;
		}
		else if (strcmp(prog->name, "pwd") == 0) /* pwd */
		{
			char *pwd = getcwd(NULL, 0);
			char *buffer = (char *)malloc(sizeof(char)*strlen("echo "));
			free(prog->name);
			sprintf(buffer, "echo");
			prog->name = buffer;
			if (prog->argc != 0)
			{
				int j = 0;
				for (j = 0; j < prog->argc; j++)
				{
					free(prog->argv[j]);
				}
				free(prog->argv);
				prog->argc = 0;
			}
			prog->argc = 1;
			prog->argv = (char **)malloc(sizeof(char *)*2);
			prog->argv[0] = pwd;
			prog->argv[1] = NULL;

		}
		else if (strcmp(prog->name, "jobs") == 0) /* jobs */
		{
			char *buffer = (char *)malloc(sizeof(char)*strlen("echo "));
			free(prog->name);
			sprintf(buffer, "echo");
			prog->name = buffer;
			if (prog->argc != 0)
			{
				int j = 0;
				for (j = 0; j < prog->argc; j++)
				{
					free(prog->argv[j]);
				}
				free(prog->argv);
				prog->argc = 0;
			}
			prog->argv = get_jobs();
			prog->argc = jobs.num; 

		}
		else if (strcmp(prog->name, "fg") == 0) /* fg */
		{
			/*refresh_jobs();*/
			if (prog->argc < 1)
			{
				printf("\"fg\": введите номер задания\n");
			}
			else
			{
				struct job *jb = jobs.head;
				int n;

				n = atoi((prog->argv)[0]);
				printf("%d %d\n", n, jobs.num);

				if (n == 0 || n > jobs.num)
				{
					printf("\"fg\": Нет такой задачи\n");
				}
				else
				{
					int j = 0;
					
					for (j = 0; j < n-1 && jb != NULL; j++)
						jb = jb->next;
					if (jb == NULL)
					{
						printf("\"fg\": Нет такой задачи\n");
					}
					else
					{
						pid_t old_in, old_out;
						pid_t pid = getpgid(jb->pid);
						int stat;

						signal(SIGTTOU, SIG_IGN);
						foreground = jb->pid;
						old_in = tcgetpgrp(0);
						old_out = tcgetpgrp(1);	
						tcsetpgrp(1, pid);
						tcsetpgrp(0, pid);
						signal(SIGTSTP, SIG_IGN);
						kill(jb->pid, SIGCONT);

						waitpid(jb->pid, &stat, WUNTRACED);

						signal(SIGTSTP, SIG_IGN);
						if (WIFEXITED(stat) == 1)
						{
							jobs.stat = WEXITSTATUS(stat);
						}
						else 
							jobs,stat = -1;
						if (WIFSTOPPED(stat) == 1)
						{
							jobs.stat = -2;
						}	
						tcsetpgrp(0, old_in);
						tcsetpgrp(1, old_out);
						
						tcsetattr(0, TCSANOW, &old_attributes);
						foreground = 0;

						if (jobs.stat == -2)
						{
							continue;
						}
						if (jobs.num == 1)
						{
							jobs.num = 0;
							jobs.head = NULL;
							jobs.tail = NULL;
						}	
						else
						{
							jobs.num--;
							if (jb == jobs.head)
							{
								jb->next->prev = NULL;
								jobs.head = jb->next;
							}
							else if (jb == jobs.tail)
							{
								jb->prev->next = NULL;
								jobs.tail = jb->prev;
							}
							else
							{
								jb->next->prev = jb->prev;
								jb->prev->next = jb->next;
							}
						}
						/*free(jb->prog);*/
						free(jb->name);
						free(jb);
					}
				}

			}
			continue;
		}
		else if (strcmp(prog->name, "bg") == 0) /* bg */
		{
			/*refresh_jobs(); */
			if (prog->argc < 1)
			{
				printf("\"bg\": введите номер задания\n");
			}
			else
			{
				struct job *jb = jobs.head;
				int n;

				n = atoi((prog->argv)[0]);
				printf("%d %d\n", n, jobs.num);

				if (n <=0 || n > jobs.num)
				{
					printf("\"bg\": Нет такой задачи\n");
				}
				else
				{
					int j = 0;
					
					for (j = 0; j < n-1 && jb != NULL; j++)
						jb = jb->next;
					if (jb == NULL)
					{
						printf("\"bg\": Нет такой задачи\n");
					}
					else
					{
/*						kill(jb->pid, SIGCONT);*/

/*						kill(getpid(), SIGCONT); */
						
						setpgid(jb->pid, jb->pid);
						kill(jb->pid, SIGCONT);
						fflush(stdout);
							
						tcsetattr(0, TCSANOW, &old_attributes);

						printf("[%d]+ %s\n", n, jb->name);
					}
				}
			}		

			continue;
		}
		if (prog->out == 0 && strcmp(prog->output, "next")== 0)
		{
			if (pipe(fd) < 0)
			{
				printf("ERROR: pipe failed\n");
				exit(1);
			}
		}
		son = fork();
		signal(SIGTSTP, handler);
		if (son < 0)
		{
			printf("ERROR: fork failed\n");
			exit(1);
		}
		if (son == 0) /* EXEC */
		{
			char **buf = (char **)malloc(sizeof(char*)*2);
			int j;
			pid_t gid;
			int file;

			signal(SIGTTOU, SIG_DFL);
			signal(SIGSTOP, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			buf[0] = prog->name;
			buf[1] = NULL;
			for (j = 0; j < prog->argc; j++)
			{
				buf = (char **)realloc(buf, sizeof(char *)*(j+3));
				buf[j+1] = (prog->argv)[j];
				buf[j+2] = NULL;
			}
			
			if (prog->in == 1)
			{
				file = open(prog->input, O_RDONLY);
				if (file < 0)
				{
					int cl_fd[2];

					printf("ERROR: Файл \"%s\" не найден\n", prog->input);
					pipe(cl_fd);
					dup2(cl_fd[0], 0);
					close(cl_fd[0]);
					close(cl_fd[1]);
				}
				else
				{
					dup2(file, 0);
					close(file);
				}
			}
			else if (prev != 0 && prog->in == 0 && strcmp(prog->input, "prev") == 0)
			{
				dup2(prev, 0);
				close(prev);
			}
			else if (prog->in == 0 && strcmp(prog->input, "close") == 0)
			{
				int cl_fd[2];

				pipe(cl_fd);
				dup2(cl_fd[0], 0);
				close(cl_fd[1]);
				close(cl_fd[0]);
			}
			if (prog->out == 1)
			{
				if (prog->out_type == 1)
				{
					file = open(prog->output, O_WRONLY|O_TRUNC|O_CREAT, 0666);
				}
				else 
				{
					file = open(prog->output, O_WRONLY|O_APPEND|O_CREAT, 0666);
				}
				dup2(file, 1);
				close(file);
			}
			else if (prog->out == 0 && strcmp(prog->output, "next") == 0)
			{
				dup2(fd[1], 1);
				close(fd[0]);
				close(fd[1]);

			}
			if (prog->back == 1) /* Если это фоновый процесс */
			{
/*				printf("herefeefef\n");*/
				signal(SIGTTIN,SIG_DFL);
				gid = getpgid(getpid());
/*				printf("BACK: %d %d\n", getpgid(getpid()), getpid());*/
				setpgid(getpid(),getpid());
/*				printf("BACK: %d %d\n", getpgid(getpid()), getpid()); */
/*				printf("%d\n", daemon(0, 0));*/
			}

			execvp(prog->name, buf); 
			setpgid(getpid(), gid);
			printf("ERROR: Программа \"%s\" не найдена\n", prog->name); 
			execlp("echo", "echo");
			exit(1);
		}
		if (prog->out == 0 && strcmp(prog->output, "next") == 0)
		{
			if (prev != 0)
				close(prev);
			prev = fd[0];
			close(fd[1]);
		}
		else
		{
			if (prev != 0)
				close(prev);
			prev = 0;
		}
		if (prog->back == 1 && prog->conv == 0) /* BACKGROUND */
		{
			struct job *jb = (struct job *)malloc(sizeof(struct job));

			jb->pid = son;
			jb->name = (char *)malloc(sizeof(char)*(strlen(prog->name)+1));
			strcpy(jb->name, prog->name);
			jobs.num++;
			if (jobs.head == NULL)
			{
				jb->next = NULL;
				jb->prev = NULL;
				jobs.head = jb;
				jobs.tail = jb;
			}
			else
			{
				jb->prev = jobs.head;
				jb->next = NULL;
				jobs.tail->next = jb;
				jobs.tail = jb;
			}
			printf("[%d] %d %s\n", jobs.num, jb->pid, jb->name);
		}
		else if (prog->back == 2 && prog->conv == 1) /* Конвеер */
		{
			;
		}
		else if (prog->back == 2) /* FOREGROUND */
		{
			int stat = 0;

			foreground = son;
			signal(SIGINT, SIG_IGN);
/*			printf("waiting son here\n");*/
			signal(SIGTSTP, SIG_IGN); 
			waitpid(son, &stat, WUNTRACED); 
			signal(SIGINT, handler);
/*			signal(SIGTSTP, handler); */
			
			if (WIFEXITED(stat) == 1)
			{
				jobs.stat = WEXITSTATUS(stat);
			}
			else 
			{
				jobs.stat = -1;
			}
			if (WIFSTOPPED(stat) == 1)
			{
				jobs.stat = -2;
			}
/*			printf("stopped? %d\n", jobs.stat);*/
			if (jobs.stat == -2)
			{
				struct job *jb = (struct job *)malloc(sizeof(struct job));

				jb->pid = son;
				jb->name = (char *)malloc(sizeof(char)*(strlen(prog->name)+1));
				strcpy(jb->name, prog->name);
				jobs.num++;
				if (jobs.head == NULL)
				{
					jb->next = NULL;
					jb->prev = NULL;
					jobs.head = jb;
					jobs.tail = jb;
				}
				else
				{
					jb->prev = jobs.head;
					jb->next = NULL;
					jobs.tail->next = jb;
					jobs.tail = jb;
				}
				printf("[%d] %s\n", jobs.num, jb->name);
			}
			foreground = 0;
		}
		else 
		{
			printf("ERROR: Something went wrong in start_command\n");
			exit(1);
		}
		
	}
	return;
}

char *get_user_name()
{
	int i = 0;
	pid_t pid;
	char *buffer;
	int fd[2];
	char c;
	if (pipe(fd) == -1)
	{
		printf("ERROR: pipe failed\n");
		exit(1);
	}
		
	if ((pid = fork()) == -1)
	{
		printf("ERROR: fork failed\n");
		exit(1);
	}
	if (pid == 0)
	{
		dup2(fd[1], 1);
		execlp("whoami", "whoami", NULL);
		printf("ERROR: execlp faild\n");
		exit(1);
	}
	buffer = (char *)malloc(sizeof(char));
	i = 0;
	read(fd[0], &c, sizeof(char)); 
	while (c != 10)
	{
		buffer[i++] = c;
		buffer = (char *)realloc(buffer, (i+1)*sizeof(char));
		buffer[i] = '\0';
		read(fd[0], &c, sizeof(char));
	}
	close(fd[0]);
	close(fd[1]);
	return buffer;
}

void handler(int sig)
{
	switch (sig)
	{
	case SIGINT:
/*		fflush(stdin);
		printf("\n");
		main(shell_argc, shell_argv); */
		fflush(stdin);
		putc('\n', stdin);
		printf("\n");
		break;
	case SIGCHLD:
		{
		int stat;
		struct job *jb;
		pid_t pid;

		pid = wait(&stat);
		if (WIFEXITED(stat) == 1)
		{
			jobs.stat = WEXITSTATUS(stat);
		}
		else
		{
			jobs.stat = -1;
		}
		jb = jobs.head;
		while (jb != NULL && jb->pid != pid)
			jb = jb->next;
		if (jobs.num == 1)
		{
			jobs.num = 0;
			jobs.head = NULL;
			jobs.tail = NULL;
		}	
		else
		{
			jobs.num--;
			if (jb == jobs.head)
			{
				jb->next->prev = NULL;
				jobs.head = jb->next;
			}
			else if (jb == jobs.tail)
			{
				jb->prev->next = NULL;
				jobs.tail = jb->prev;
			}
			else
			{
				jb->next->prev = jb->prev;
				jb->prev->next = jb->next;
			}
		}
		/*free(jb->prog);*/
		free(jb->name);
		free(jb);
		}
		break;
	case SIGTTIN:
/*		printf("GOT SIGTTIN %d\n", getpid());*/
		break;
	case SIGTTOU:
/*		printf("GOT SIGTTOU %d\n", getpid());*/
		break;
	case SIGTSTP:
/*		printf("GOT CTRL+Z\n");*/
		if (foreground != 0)
		{
			kill(foreground, SIGTSTP);
/*			printf("%d\n", getpid());*/
		}
		break;
	default:
/*		printf("SIGNAL: Got signal %d\n", sig); */
		;
	}
	signal(sig,handler); 
	return;
}

void exit_prog(Command *cmd)
{
	struct job *jb;
	struct hist_node *hist;
	int i = 0;

/*	printf("exit_prog started\n"); */
	free_cmd(cmd);
	jb = jobs.head;
	i = 0;
	while (jb != NULL && i < jobs.num)
	{
		struct job *n_jb;
		i++;
		kill(jb->pid, SIGKILL);
		waitpid(jb->pid, NULL, 0);
		free(jb->name);
		n_jb = jb->next;
		free(jb);
		jb = n_jb;
	}
	hist = History.head;
	while (hist != NULL)
	{
		struct hist_node *l_hist;

		free(hist->cmd);
		l_hist = hist->next;
		free(hist);
		hist = l_hist;
	}
	while (waitpid(-1, NULL, WNOHANG) > 0)
	{
		;
	}
	tcsetattr(0, TCSANOW, &old_attributes);
	printf("Exiting ...\n");
	exit(0);
}

void free_prog(struct program *prog)
{
	int j = 0;

/*	printf("free_prog started\n"); */
	free(prog->name);
	free(prog->input);
	free(prog->output);
	for (j = 0; j < prog->argc; j++)
	{
		free((prog->argv)[j]);
	}
	free(prog->argv);
	free(prog);
/*	printf("free_prog stopped\n"); */
	return;
}

void hist_append(char *buffer)
{
	char *buf = (char *)malloc(sizeof(char)*(strlen(buffer)+1));
	struct hist_node *hist = (struct hist_node *)malloc(sizeof(struct hist_node));

	strcpy(buf, buffer);
	hist->cmd = buf;
	History.hist_num++;
	if (History.head == NULL)
	{
		hist->next = NULL;
		hist->prev = NULL;
		History.head = hist;
		History.tail = hist;
	}
	else
	{
		hist->next = NULL;
		History.tail->next = hist;
		hist->prev = History.tail;
		History.tail = hist;
	}
	return;

}

void free_cmd(Command *cmd)
{
	int i;

	for (i = 0; i < cmd->num_progs; i++)
	{
		free_prog((cmd->com)[i]);
	}
	free(cmd->com);
	free(cmd);

}

void refresh_jobs()
{
	struct job *jb, *n_jb;

	jb = jobs.head;
	while (jb != NULL)
	{
		kill(jb->pid, 0);
		if (errno == ESRCH)
		{
			int stat;

			jobs.num--;
			free(jb->name);
			if (jobs.head == jb)
			{
				jobs.head = jb->next;
			}
			if (jobs.tail == jb)
			{
				jobs.tail = jb->prev;
			}
			if (jb->next != NULL)
			{
				jb->next->prev = jb->prev;
			}
			if (jb->prev != NULL)
			{
				jb->prev->next = jb->next;
			}
			n_jb = jb->next;
			free(jb);
			jb = n_jb;
		}
		else
		{
			jb = jb->next;
		}
	}
	return;
}

char **get_jobs()
{
	struct job *jb;
	int i = 0;
	char **res = (char **)malloc(sizeof(char *));
	int j = 0;
	res[0] = NULL;

	/*refresh_jobs();*/
	jb = jobs.head;
	for (i = 0; jb != NULL; i++)
	{
		char *buffer;

		buffer = (char *)malloc(sizeof(char)*(strlen(jb->name)+10 + sizeof(int)*8));
		sprintf(buffer, "\t[%d] %d %s\n", i+1, jb->pid, jb->name);
		res[j++] = buffer;
		res = (char **)realloc(res, (j+1)*sizeof(char*));
		res[j] = NULL;
	       	jb = jb->next;	
	}	
	return res;
}
