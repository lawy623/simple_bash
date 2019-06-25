/*
 *  author: Yue Luo
 *  userName: lawy623
 *  UID: yl4003
 *  email: yl4003@columbia.edu
 *  data: 18 Sep 2018
 */

#include "./shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#define MAXARG 30
#define MAXLOG 100
#define LEN 300

#define min(a, b) ((a < b) ? a : b)
#define max(a, b) ((a > b) ? a : b)

char *arg_logs[MAXLOG];
int log_count;
int start_count;

/* This is the function to do multiple pipes.  */
void do_pipes(char *input, int pipes_num)
{
	int sec = pipes_num + 1;
	char *inputs[sec];
	int err = splits_pipes(inputs, input, sec);

	if (err == -1)
		return;

	int fd[2];
	int fin;
	pid_t pid;
	int i = 0;

	while (i < sec) {
		char *sub_arg[MAXARG];

		tokenize(inputs[i], sub_arg);
		if (pipe(fd) == -1) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			return;
		}

		pid = fork();
		if (pid == -1) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			return;
		} else if (pid == 0) {
			dup2(fin, 0);
			if (i != sec-1)
				dup2(fd[1], 1);
			close(fd[0]);
			execute(sub_arg);
			exit(1);
		} else {
			if (waitpid(pid, NULL, 0) < 0) {
				fprintf(stderr, "error: %s\n", strerror(errno));
				return;
			}
			close(fd[1]);
			fin = fd[0];
			i++;
		}
	}
}

/* This is the function to run one pipe only. */
void do_2pipes(char *input)
{
	int sec = 2;
	char *inputs[sec];
	int err = splits_pipes(inputs, input, sec);

	if (err == -1)
		return;

	char *sub_arg1[MAXARG];
	char *sub_arg2[MAXARG];

	tokenize(inputs[0], sub_arg1);
	tokenize(inputs[1], sub_arg2);

	int fd[2];
	pid_t c1, c2;

	if (pipe(fd) == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return;
	}
	c1 = fork();
	if (c1 < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(1);
	} else if (c1 == 0) {
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		execute(sub_arg1);
		exit(1);
	}

	c2 = fork();
	if (c2 < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(1);
	} else if (c2 == 0)  {
		close(fd[1]);
		dup2(fd[0], STDIN_FILENO);
		execute(sub_arg2);
		exit(1);
	}

	close(fd[0]);
	close(fd[1]);
	if (waitpid(c1, NULL, 0) < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(1);
	}
	if (waitpid(c2, NULL, 0) < 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(1);
	}
}

/* Split the input by '|' */
int splits_pipes(char *inputs[], char *input, int sec)
{
	int i = 0;
	int l = 0;
	int r = 0;

	for (; i < sec; i++) {
		while (input[r+1] != '|' && input[r+1] != '\0')
			r++;
		inputs[i] = strndup(input+l, r-l+1);
		if (inputs[i] == NULL) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			return -1;
		}
		r = r+2;
		l = r;
	}
	return 0;
}

/*  Print prompt. Default is '$' only. */
void print_prompt(void)
{
	printf("$");
}

/*  Get the num of '|' in the input string.
 *  Return -1 if the '|' is invalid (front / last / with nothing between)
 */
int get_pipes(char *input)
{
	int len = strlen(input);
	int i = 0;
	int j = 0;
	int first = 0;
	int f = 0;
	int count = 0;

	while (i < len) {
		if (input[i] == '|') {
			if (i == 0 || i == len-1)
				return -1;

			if (first == 0) {
				while (input[f] == ' ')
					f++;

				if (f == i)
					return -1;
				first = 1;
			}

			j = i;

			while (input[j+1] == ' ' && (j+1) != len)
				j++;

			if (input[j+1] == '|')
				return -1;

			i = j;
			count++;
		}
		i++;
	}

	if (j == len-1 && input[j] == ' ')
		return -1;
	return count;
}

/*  Function to replace all !! and !str in the input.
 *  Return 1 if ! is isolated, or !str is not found in logs, or malloc error.
 */
int replace_sign(char *new_input, char *input)
{
	int total_len = strlen(input);
	int i = 0;
	int j = 0;

	while (i < total_len) {
		if (input[i] == '!') {
			if (i == total_len - 1 || log_count == start_count)
				return 1;


			if (input[i + 1] == '|' || input[i + 1] == ' ')
				return 1;

		}
		if (input[i] == '!' && input[i + 1] == '!') {
			char *par = strndup(input + j, i - j);

			if (par == NULL) {
				fprintf(stderr, "error: %s\n", strerror(errno));
				return 1;
			}
			strcat(new_input, par);
			free(par);

			int t = (log_count - 1) % MAXLOG;

			strcat(new_input, arg_logs[t]);

			j = i + 2;
			i = j;
		} else if (input[i] == '!' && input[i + 1] != '!') {
			char *par = strndup(input+j, i-j);

			if (par == NULL) {
				fprintf(stderr, "error: %s\n", strerror(errno));
				return 1;
			}
			strcat(new_input, par);
			free(par);

			j = i;
			while (strspn(&input[j+1], "| !") == 0
				&& j != total_len-1)
				j++;

			char *head;
			int l1 = j-i;

			head = strndup(input+i+1, l1);

			if (head == NULL) {
				fprintf(stderr, "error: %s\n", strerror(errno));
				return 1;
			}
			int r = log_count-1;
			int end = max(start_count, log_count-MAXLOG+1);
			int find = 0;

			for (; r >= end; r--) {
				char *pre;

				pre = strndup(arg_logs[r%MAXLOG], l1);

				if (pre == NULL) {
					fprintf(stderr, "error: %s\n",
						strerror(errno));
					return 1;
				}
				if (strcmp(head, pre) == 0) {
					find = 1;
					int k = r % MAXLOG;

					strcat(new_input, arg_logs[k]);
					free(pre);
					break;
				}
				free(pre);
			}
			free(head);
			if (find == 0)
				return 1;

			i = j+1;
			j = i;
		} else {
			i++;
		}
	}
	if (j < total_len) {
		char *par = strndup(input + j, i - j);

		if (par == NULL) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			return 1;
		}
		strcat(new_input, par);
		free(par);
	}
	return 0;
}

/*  Tokenize the string into a set of arguments.
 */
void tokenize(char *input, char *arglist[])
{
	int arg_count = 0;
	char *splits = strtok(input, " ");

	while (splits != NULL) {
		arglist[arg_count++] = splits;
		splits = strtok(NULL, " ");
	}
	arglist[arg_count] = NULL;
	free(splits);
	splits = NULL;
}

/* Implement build-in commands "cd", "history", and "exit".
 * Other command will be executed by system call.
 */
int execute(char *arglist[])
{
	if (strcmp(arglist[0], "cd") == 0) {
		if (arglist[1] == NULL) {
			fprintf(stderr, "error: %s\n", "invalid");
			return -1;
		}
		if (chdir(arglist[1]) < 0) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			return -1;
		}
	} else if (strcmp(arglist[0], "history") == 0) {
		if (arglist[1] == NULL
			|| strspn(arglist[1], "0123456789")
			== strlen(arglist[1])) {
			if (arglist[2] != NULL && arglist[1] != NULL) {
				fprintf(stderr, "error: %s\n", "Invalid");
				return -1;
			}
			int maxLen = MAXLOG;

			if (arglist[1] != NULL)
				maxLen = min(maxLen, atoi(arglist[1]));
			if ((log_count - start_count) < maxLen) {
				int h = start_count;

				for (; h <= log_count; h++) {
					fprintf(stdout,	"%d %s\n", h,
						arg_logs[h%MAXLOG]);
				}
			} else {
				int i = maxLen - 1;
				int rc = 0, pos = 0;

				for (; i >= 0; i--) {
					rc = log_count - i;
					pos = rc % MAXLOG;
					fprintf(stdout, "%d %s\n",
						rc, arg_logs[pos]);
				}
			}
		} else if (strcmp(arglist[1], "-c") == 0) {
			if (arglist[2] != NULL) {
				fprintf(stderr, "error: %s\n", "invalid");
				return -1;
			}
			start_count = log_count+1;
		} else {
			fprintf(stderr, "error: %s\n", "invalid argument");
			return -1;
		}
	} else if (strcmp(arglist[0], "exit") == 0) {
		if (arglist[1] != NULL) {
			fprintf(stderr, "error: %s\n", "too many arguments");
			exit(1);
		} else {
			exit(0);
		}
	} else {
		pid_t pid, pid2;
		int status;

		pid = fork();
		if (pid < 0) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			return -1;
		} else if (pid == 0) {
			if (execvp(arglist[0], arglist) < 0) {
				fprintf(stderr, "error: %s\n", strerror(errno));
				exit(1);
			}
			exit(0);
		} else {
			pid2 = waitpid(-1, &status, 0);
			if (pid2 < 0) {
				fprintf(stderr, "error: %s\n", strerror(errno));
				return -1;
			}
		}
	}
	return 0;
}

/* Main function */
int main(void)
{
	size_t str_size;

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	log_count = 0;
	start_count = 0;

	while (1) {
		char *input;
		char *arglist[MAXARG];

		print_prompt();

		/* Exit shell when enter EOF(ctrl+d) */
		if (getline(&input, &str_size, stdin) == -1) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			exit(1);
		}
		/* Only 'Enter(\n)' without content just continue */
		if (strcmp(input, "\n") == 0)
			continue;

		if (strlen(input) == 0) {
			fprintf(stderr, "error: %s\n", "Input size error.");
			continue;
		} else {
			input[strlen(input) - 1] = '\0';
		}

		/* If all the inputs are white space, continue */
		int p = 0;
		int all_space = 1;

		for (; p < strlen(input); p++) {
			if (input[p] != ' ')
				all_space = 0;
		}
		if (all_space == 1)
			continue;

		/* Replace all the !! and !str in the string */
		char *new_input = malloc(LEN);
		int invalid = 0;

		invalid = replace_sign(new_input, input);
		if (invalid == 1) {
			fprintf(stderr, "error: %s\n", "Argument not found.");
			continue;
		}

		arg_logs[log_count % MAXLOG] = malloc(strlen(new_input) + 1);
		if (arg_logs[log_count % MAXLOG] == NULL) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			continue;
		}

		strcpy(arg_logs[log_count % MAXLOG], new_input);

		/* Gets num of '|' in new string, to see what to do next */
		int pipes_num;

		pipes_num = get_pipes(new_input);

		if (pipes_num == -1) {
			fprintf(stderr, "error: %s\n", "Invalid pipes.");
		} else if (pipes_num == 0) {
			tokenize(new_input, arglist);
			execute(arglist);
		} else {
			do_pipes(new_input, pipes_num);
		}

		log_count++;
	}
	return 0;
}
