#include "tree.h"

static jmp_buf begin;

static void order(tree *t);
static void log(tree *t);
static void conv(tree *t);
static void cmd(tree *t);

static int cmpstr(char *a, char *b) { /* return 0, if strings are equal */
	int i;
	if (!a || !b) /* one string isn't allocated */
		return 1;
	for (i = 0; a[i] && b[i]; ++i)
		if (a[i] != b[i]) /* difference */
			return 1;
	return a[i] != b[i];
}

static int usual_word(char *w) { /*checks if word can be name or argument */
	if (w == NULL)
		return 0;
	return cmpstr(w, "&") && cmpstr(w, ";") && cmpstr(w, "||")
		&& cmpstr(w, "&&") && cmpstr(w, "|") && cmpstr(w, ">")
		&& cmpstr(w, ">>") && cmpstr(w, "<") && cmpstr(w, "(")
		&& cmpstr(w, ")");
}

static void cp_node(node **a, node *b) { /* copy information from a to b */
	if ((*a)->op)
		free((*a)->op);
	if ((*a)->cmd)
		free((*a)->cmd);
	if (b == NULL) {
		free(*a);
		*a = NULL;
		return;
	}
	(*a)->opflg = b->opflg;
	(*a)->op = b->op;
	(*a)->cmd = b->cmd;
}

static void destroy_tree(tree *t) { /* free memory, allocatet for tree */
	if (*t == NULL) /* empty tree */
		return;
	if ((*t)->cmd) { /* it isn't operarion, so it's leaf */
		destroy_tree(&((*t)->cmd->subshell));
		free((*t)->cmd->arg);
		free((*t)->cmd);
	}
	if ((*t)->op) {
		destroy_tree(&((*t)->op->left)); /* remove left sub tree */
		destroy_tree(&((*t)->op->right)); /* remove right sub tree */
		free((*t)->op);
	}
	free(*t);
	*t = NULL;
}

void print_tree(tree t) {
	if (t == NULL) /* empty tree */
		return;
	if (t->opflg == 0) { /* it isn't operarion, so it's leaf */
		print_tree(t->cmd->subshell);
		if (t->cmd->arg)
			for (int i = 0; (t->cmd->arg)[i] != NULL; ++i)
				printf("%s. ", (t->cmd->arg)[i]);
		if ((t->cmd->inpf) != NULL)
			printf("< %s ", (t->cmd->inpf));
		if((t->cmd->outpf) != NULL)
			printf("> %s ", (t->cmd->outpf));
		return;
	}
	print_tree(t->op->left); /* remove left sub tree */
	switch (t->op->type) {
		case ord: printf("; "); break;
		case par: printf("& "); break;
		case or: printf("|| "); break;
		case and: printf("&& "); break;
		case cnv: printf("| "); break;
	}
	print_tree(t->op->right); /* remove right sub tree */
}

static void error() {
	remove_list();
	destroy_tree(&tr);
	fprintf(stderr, "invalid syntax\n");
	longjmp(begin, 1);
}

static char *get_word(void) { /* returns new word from list */
	if ((curp > 0) && (lst[curp - 1] == NULL)) 
		return NULL;
	return lst[curp++];
}

static void put_word(void) { /* step back in list */
	if ((curp > 0) && (lst[curp - 1] != NULL))
		curp--;
}

static void order(tree *t) { /* create node for order operation */
	node *lleaf; /* left leaf of node */
	node *cur = *t;
	char *wrd;
	lleaf = malloc(sizeof(node)); /* next level function infill this node */
	if (!lleaf) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}
	lleaf->op = NULL;
	lleaf->cmd = NULL;
	cur->opflg = 1; /* it's operation */
	cur->op = malloc(sizeof(op_inf)); /* create structure for info */
	if (!(cur->op)) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}
	cur->op->right = NULL;
	cur->op->left = lleaf;
	log(&lleaf); /* get left leaf */
	cur->cmd = NULL;
	wrd = get_word(); /* read the next word of list */
	if (!cmpstr(wrd, ";") || !cmpstr(wrd, "&")) {
		/* need separate node for order operation */
		switch (wrd[0]) { /* set operation type */
			case ';': cur->op->type = ord; break;
			case '&': cur->op->type = par; break;
		}
		cur->op->right = malloc(sizeof(node));
		if (!(cur->op->right)) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		cur->op->right->op = NULL;
		cur->op->right->cmd = NULL;
		order(&(cur->op->right)); /* get right leaf, it can be NULL */
		return;
	}
	put_word(); /* return word to list */
	cp_node(t, lleaf); /* move information to current node */
	free(lleaf);
}

static void log(tree *t) { /* create node for logical operation */
	node *lleaf; /* left leaf of node */
	node *cur = *t;
	char *wrd;
	lleaf = malloc(sizeof(node)); /* next level function infill this node */
	if (!lleaf) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}
	lleaf->op = NULL;
	lleaf->cmd = NULL;
	cur->opflg = 1; /* it's operation */
	cur->op = malloc(sizeof(op_inf)); /* create structure for info */
	if (!(cur->op)) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}
	cur->op->right = NULL;
	cur->op->left = lleaf;
	conv(&lleaf); /* get left leaf */
	wrd = get_word(); /* get left leaf */
	if (!cmpstr(wrd, "||") || !cmpstr(wrd, "&&")) {
		/* need separate node fol logical */
		switch (wrd[0]) { /* set operation type */
			case '|': cur->op->type = or; break;
			case '&': cur->op->type = and; break;
		}
		cur->op->right = malloc(sizeof(node));
		if (!(cur->op->right)) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		cur->op->right->op = NULL;
		cur->op->right->cmd = NULL;
		cur->cmd = NULL;
		log(&(cur->op->right));
		if (cur->op->right == NULL) /* can't be empty */
			error();
		return;
	}
	put_word(); /* return word to list */
	cp_node(t, lleaf); /* move information to current node */
	free(lleaf);
}

static void conv(tree *t) { /* creat node for conveyor */
	node *lleaf; /* left leaf of node */
	node *cur = *t;
	char *wrd;
	lleaf = malloc(sizeof(node)); /* next level function infill this node */
	if (!lleaf) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}
	lleaf->op = NULL;
	lleaf->cmd = NULL;
	cur->opflg = 1; /* it's operation */
	cur->op = malloc(sizeof(op_inf)); /* create structure for info */
	if (!(cur->op)) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}
	cur->op->right = NULL;
	cur->op->left = lleaf;
	cmd(&lleaf); /* get left leaf */
	wrd = get_word(); /* read the next word of list */
	if (!cmpstr(wrd, "|")) {
		/* need separate node for conveyor */
		cur->op->type = cnv; /* set operation type */
		cur->op->right = malloc(sizeof(node));
		if (!(cur->op->right)) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		cur->op->right->op = NULL;
		cur->op->right->cmd = NULL;
		cur->cmd = NULL;
		conv(&(cur->op->right));
		if (cur->op->right == NULL) /* can't be empty */
			error();
		return;
	}
	put_word(); /* return next word to the list */
	cp_node(t, lleaf); /* move information to current node */
	free(lleaf);
}

static void cmd(tree *t) {
	char *wrd;
	node *cur = *t;
	int i = 0;
	wrd = get_word();
	if ((wrd == NULL) || !cmpstr(wrd, ")")) { /* end of list of words */
		free(*t); /* remove unnecessary node */
		*t = NULL;
		return;
	}
	cur->opflg = 0; /* it's not op */
	cur->op = NULL;
	if (!cmpstr(wrd, "(")) { /* line for sub shell */
		cur->cmd = malloc(sizeof(cmd_inf));
		if (!(cur->cmd)) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		cur->cmd->arg = NULL;
		cur->cmd->subshell = malloc(sizeof(node));
		if (!(cur->cmd->subshell)) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		cur->cmd->subshell->op = NULL;
		cur->cmd->subshell->cmd = NULL;
		order(&(cur->cmd->subshell));
		wrd = get_word(); /* remove ')' from queue */
		if(cmpstr(wrd, ")")) /* no balance */
			error();
		wrd = get_word();
	} else {
		if (!usual_word(wrd)) /* next word isn't name of program */
			error();
		/* here if syntax correct and there is no '(' */
		cur->cmd = malloc(sizeof(cmd_inf));
		if (!(cur->cmd)) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		cur->cmd->subshell = NULL;
		cur->cmd->arg = NULL;
		while (usual_word(wrd)) { /* fill arguments of program */
			cur->cmd->arg = reallocarray(cur->cmd->arg, ++i,sizeof(char*));
			/* memory for a new argument */
			if (!(cur->cmd->arg)) {
				fprintf(stderr, "%s\n", strerror(errno));
				exit(1);
			}
			cur->cmd->arg[i - 1] = wrd; /* add new argument to list */
			wrd = get_word();
		}
		cur->cmd->arg = reallocarray(cur->cmd->arg, ++i, sizeof(char*));
		cur->cmd->arg[i - 1] = NULL; /* end of list of arguments */
	}
	cur->cmd->inpf = NULL;
	cur->cmd->outpf = NULL;
	cur->cmd->outpflags = 0;
	while (1) { /* read information about files */
		if (usual_word(wrd)) {
			error();
		} else if (!cmpstr(wrd, "<")) { /* change input file */
			wrd = get_word();
			if (!usual_word(wrd)) /* it's not name of file */
				error();
			if (cur->cmd->inpf)/* it isn't first input file for programm */
				error();
			cur->cmd->inpf = wrd;
			wrd = get_word();
		} else if (!cmpstr(wrd, ">") || !cmpstr(wrd, ">>")) {
			/* change output file */
			char *wrdt = wrd;
			wrd = get_word();
			if (!usual_word(wrd)) /* it's not name of file */
				error();
			if (cur->cmd->outpf) /* it isn't first output file for prog */
				error();
			cur->cmd->outpf = wrd;
			switch (wrdt[1]) { /* set flags */
				case '>': cur->cmd->outpflags = O_CREAT|O_WRONLY|O_APPEND;
					break;
				case '\0': cur->cmd->outpflags = O_CREAT|O_WRONLY|O_TRUNC;
					break;
			}
			wrd = get_word();
		} else { /* separator of next-level operation */
			put_word(); /* return separator to list */
			break;
		}
	}
}

tree new_tree(void) {
	setjmp(begin);
	if (!(lst = new_list()))
		return NULL;
	tr = malloc(sizeof(node));
	tr->op = NULL;
	tr->cmd = NULL;
	curp = 0;
	order(&tr);
	return tr;
}

void remove_tree(void) {
	destroy_tree(&tr);
	remove_list();
}
