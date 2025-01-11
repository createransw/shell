#include <unistd.h>
#include "list.h"

#define size_str 10 /* number of symbols to read in a time */
#define scanf_option "%8[^\n]" /* number is str_size - 2 */
#define buffer_block 5

int SPEC_FLG = 1;
int END_FLG = 0;
int CUR_SEP = '"';

static int c; /* current processing symbol */
static list lst; /* list of words of line */
static char *buffer; /* for current word */
static int cur_b; /* current symbol in buffer */
static int size_b; /* current size of buffer */
static char str[size_str]; /* string to read */
static int cur_str; /* current symbol in str */

static int start(void);
static int stop(void);
static int next_line(void);
static int word(void);
static int special(void);

/*---------------------------assist functions-----------------------------*/

static int len_s(char *s) { /* counts length of string */
	char *t = s;
	while (*t != '\0')
		t++;
	return t - s;
}

static void get_sym(void) { /* returns new symbol of line */
    if (END_FLG) {
        c = EOF;
        return;
    }
	int i;
	if (str[cur_str] == '\0') { /* end of word */
		i = fscanf(stdin, scanf_option, str); /* read portion of symbols */
        if (i == EOF) {
            END_FLG = 1;
            c = '\n';
            return;
        }
		c = getchar(); /* read one symbol, may be \n or EOF */
		if (i != 0) /* if stream contains not only \n */
			i = len_s(str);
		str[i] = c;
		str[i + 1] = '\0';
		/* c to check end of file */
		cur_str = 0;
	}
    if (c == EOF) {
        END_FLG = 1;
    }
    c = str[cur_str++];
}

static int usual_sym(int c) { /* separates special symbols from usual */
	if (c == '\n')
		return 0;
	if ((c == EOF) || (c == ' ') || (c == '\t') || !c)
		return !SPEC_FLG;
	if ((c == ';') || (c == '<') || (c == '(') || (c == ')'))
		return !SPEC_FLG;
	if ((c == '|') || (c == '&') || (c == '>') || (c == '#'))
		return !SPEC_FLG;
	if (c == '$')
		return 0;
	if ((c == '"') || (c == '\''))
		return !SPEC_FLG && (c != CUR_SEP);
	return 1;
}

int name_sym(int c) { /* returns 1 if symbol can be in variables names */
	char s[64] = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_1234567890"; /* all symbols suitable for names */
	int i;
	for (i = 0; s[i] != '\0'; ++i) {
		if (c == s[i])
			return 1;
	}
	return 0;
}
	
static void end_of_line() { /* prepares list to next line */
	SPEC_FLG = 1;
	finish(&lst);
}

static void push(int c) { /* add symbol to buffer */
	if (cur_b >= size_b) /* buffer is small */
		buffer =
			reallocarray(buffer, size_b += buffer_block, sizeof(char*));
	if (!buffer) { /* not enough memory */
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}
	buffer[cur_b++] = c;
}

static void new_buf() { /* creates a buffer */
	buffer = NULL;
	size_b = 0;
	cur_b = 0;
}

static void finish_buf() { /* transform buffer to complete string */
	push('\0'); /* end of string */
	size_b = cur_b;
	/* remove unnecessary memory */
	buffer = reallocarray(buffer, size_b, sizeof(char*));
	if (!buffer) { /* not enough memory */
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}
}

static void clear_buf() { /* remove old buffer and create new */
	free(buffer);
	new_buf();
}

/*---------------------------status functions-----------------------------*/

int start(void) { /*decides what to do next: read word or go to next line*/
	while ((c == '\t') || (c == ' ')) /* skip split symbols */
		get_sym();
	if (c == EOF) /* work complete */
		return stop();
	if (c == '\n') /* end of line, time to print, sort etc. */
		return next_line();
	if (usual_sym(c)) /* beginning of usual word, reads whole */
		return word();
	/* last option is special symbol */
	/* works because recursion never returns and stops only with exit() */
	return special();
}

int stop(void) {
    printf("%c\n", '!');
    END_FLG = 1;
	if (!str[0]) /* if there are still line in list */
		end_of_line(); /* does all stuff for last word */
	return 0; /* starts shlopivanie recursii */
}

int next_line(void) {
	end_of_line(); /* does all stuff for last word */
	return 0;
}

int word(void) {
	while (usual_sym(c)) { /* reads whole word */
		push(c);
		get_sym();
	}
	if ((c != '"') && (c != '\'') && (c != '$')) { /* end of word */
		finish_buf(); /* transform buffer to complete string */
		add(&lst, buffer); /* add word to list */
		clear_buf(); /* buffer ready for next word */
	}
	return start();
}

void replace_var(void) { /* replace variable with it's value */
	int size = 1, j, i, all_flg = 0;
	char *name = NULL;
	char *val = NULL;
	get_sym();
	while (name_sym(c)) { /* reads all name symbols after $ */
		name = reallocarray(name, ++size, sizeof(char));
		if (!name) { /* not enough memory */
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		name[size - 2] = c;
		get_sym();
	}
	name[size - 1] = '\0';
	if (!strcmp(name, "HOME"))
		val = getenv("HOME"); /* read from environment */
	if (!strcmp(name, "USER")) {
		val = getlogin(); /* get user name */
		if (val == NULL) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
	}
	if (!strcmp(name, "EUID")) {
		i = geteuid(); /* get user id */
		for (size = 0; i > 0; ++size)
			i/= 10;
		val = reallocarray(val, size + 1, sizeof(char));
		if (!val) { /* not enough memory */
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		sprintf(val, "%d", geteuid()); /* transform number to string */
		all_flg = 1;
	}
	if (!strcmp(name, "SHELL")) {
		char s[] = "my_shell";
		val = s;
	}
	if (val == NULL) { /* no match with variables */
		val = malloc(sizeof(char));
		if (!val) { /* not enough memory */
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		*val = '\0';
		all_flg = 1;
	}
	for (i = 0; val[i] != '\0'; ++i) /*add variable value to current word*/
		push(val[i]);
	if (!usual_sym(c) && (c != '"') && (c != '\'') && (c != '$')) {
		/* end of current word */
		finish_buf();
		add(&lst, buffer);
		clear_buf();
	}
	free(name);
	if (all_flg)
		free(val);
}

int special(void) {
	int prev_c = c;
	if (c == '$') { /* don't stop word if it's variable */
		replace_var();
		return start();
	}
	if ((c == '"') || (c == '\'')) { /* process lines */
		if (SPEC_FLG) { /* current symbol is a beginning of line */
			SPEC_FLG = 0;
			CUR_SEP = c;
		} else  { /* current symbol is an end of line */
			SPEC_FLG = 1;
		}
		get_sym();
		if (!usual_sym(c) && (c != '"') && (c != '\'') & (c != '$')) {
			/* next symbol isn't connected to line */
			finish_buf();
			add(&lst, buffer);
			clear_buf();
		}
		return start();
	}
	if (c == '#') { /* comment */
		while ((c != '\n') && (c != EOF))
			get_sym();
		return start();
	}
	push(c); /* add first symbol */
	get_sym();
	if (!((c == ';') || (c == '<') || (c == '(') || (c == ')'))) {
		if (c == prev_c) { /* special word can contain 2 same symbols */
			push(c);
			get_sym();
		}
	}
	finish_buf(); /* transform buffer to complete string */
	add(&lst, buffer); /* add word to list */
	clear_buf(); /* add word to list */
	return start();
}

/*------------------------------------------------------------------------*/

void construct(void) {
	int i;
	cur_str = size_str - 1;
	for (i = 0; i < size_str; ++i) /* start with empty str */
		str[i] = '\0';
	new_buf();
	new(&lst);
	get_sym();
	start();
}

void remove_list() {
	destroy(&lst);
	new(&lst);
}

list new_list(void) {
    if (END_FLG)
        return NULL;
	int i;
	cur_str = size_str - 1;
	for (i = 0; i < size_str; ++i) /* start with empty str */
		str[i] = '\0';
	new_buf();
	new(&lst);
	get_sym();
	start();
	return lst;
}

