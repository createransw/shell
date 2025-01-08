#include <sys/types.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

typedef enum { ord, par, or, and, cnv } op_type;

typedef char **list;
typedef struct Node *tree;

typedef struct CmdInf { /* all information for executing command */
	char **arg;
	char *inpf;
	int outpflags;
	char *outpf;
	struct Node *subshell;
} cmd_inf;

typedef struct OpInf { /* all information to process control symbols */
	op_type type; /* type of operation */
	struct Node *left; /* left leaf */
	struct Node *right; /* right leaf */
} op_inf;

typedef struct Node {
	int opflg; /* indicates type of node */
	op_inf *op; /* not NULL if this is node of &, &&, |, etc. */
	cmd_inf *cmd; /* not NULL if this is node of command */
} node;

static list lst;
static tree tr;
static int curp = 0;

extern void *remove_list(void);
extern list new_list(void);

tree new_tree(void);
void remove_tree(void);
