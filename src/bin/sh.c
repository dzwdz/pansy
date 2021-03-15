#include "tty.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LEN 256

char** env;

char** split_args(const char* str) {
	static
	void*  shared_buf = NULL;
	if (!shared_buf)
	       shared_buf = malloc(64 * sizeof(char*)
	                         + MAX_LEN + 64);

	char** parts_start = shared_buf;
	char** parts       = parts_start;

	char* buf_start    = shared_buf + 64 * sizeof(char*);
	char* buf          = buf_start;

	bool quoted        = false;

	while (*str != '\0') {
		switch (*str) {
			// WHITESPACE
			case ' ':
			case '\n':
			case '\t':
				// if we're in a quote, just add the whitespace to the buffer
				if (quoted) {
					*buf++ = *str;
					break;
				}

				// ignore repeating whitespace
				if (buf == buf_start) break;

				*buf++ = '\0';
				*parts++ = buf_start;
				buf_start = buf;
				break;

			// QUOTES
			case '"':
				quoted ^= true;
				if (!quoted) {
					*buf++ = '\0';
					*parts++ = buf_start;
					buf_start = buf;
				}
				break;

			// ESCAPED CHARS
			case '\\': {
				char c = *++str;
				switch (c) {
					case 't': c = '\t'; break;
					case 'r': c = '\r'; break;
					case 'n': c = '\n'; break;
					case '0': c = '\0'; break;
				}
				*buf++ = c;
				break;
			}

			default:
				*buf++ = *str;
		}
		str++;
	}

	if (buf != buf_start)  {
		*buf++ = '\0';
		*parts++ = buf_start;
	}

	*parts = NULL;
	return parts_start;
}

int run(char** args) {
	// some builtins
	if (!strcmp(args[0], "exit"))
		exit(0);

	if (!strcmp(args[0], "cd")) {
		chdir(args[1]);
		return 0;
	}

	// the command isn't a builtin, try running an executable
	if (!fork()) {
		execve(args[0], args, env);
		puts("pansh: execve() error");
	}

	int status;
	wait(&status);
	return status;
}

int main(int argc __attribute__((unused)),
	     char** argv __attribute__((unused)),
	     char** envp) {

	env = envp;

	char *buf = malloc(MAX_LEN);

	while (1) {
		getcwd(buf, MAX_LEN);
		printf("\t%s> ", buf);

		readline(buf, MAX_LEN);
		if (buf[0] == '\0')
			continue;

		char** args = split_args(buf);
		run(args);
	}

	return 0;
}
