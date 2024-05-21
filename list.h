#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define Block_size 10

typedef char **list;

static int size;
static int cur;

static void swap(char **a, char **b) {
	char *tmp;
	tmp = *a;
	*a = *b;
	*b = tmp;
}

static int len(char *s) { /* count length of string ('\0' also counts)*/
	char *t = s;
	while (*t != '\0')
		t++;
	return t - s + 1;
}

static void cp_str(char **a , char *b) { /* make a similar to b */
	int i;
	if (!b) { /* b is NULL */
		*a = NULL;
		return;
	}
	(*a) = calloc(len(b), sizeof(char));
	if (!(*a)) { /* no more memory */
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}
	for (i = 0; b[i] != '\0'; i++)
		(*a)[i] = b[i];
	(*a)[i] = '\0';
}

void add(list *lst, char *elem) {
	if ((cur) >= (size) - 1) { 
		/* next free is *lst[size], which don't exist */
		/* add memory for new element , increase size */
		*lst = reallocarray(*lst, size += Block_size, sizeof(char*));
		if (!(*lst)) { /* no more memory */
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
	}
	cp_str(&((*lst)[(cur)++]), elem); /* new element to last place */
}

void finish(list *lst) { /* last step */
	add(lst, NULL); /* put NULL to the end */
	/* remove not used part */
	*lst = reallocarray(*lst, cur + 1, sizeof(char*));
	size = cur;
	if (!(*lst)) { /* if there is no free memory */
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}
}

void new(list *lst) { /* new empty list */
	*lst = NULL;
	cur = 0;
	size = 0;
}

static int bigger(char *a, char *b) { /* compare strings */
	while((*a) && (*b) && ((*a) == (*b))) {
		a++;
		b++;
	}
	if (*a == '\0') {
		return !(*b);
	}
	if (*b == '\0') {
		return *a;
	}
	return ((*a) > (*b));
}

void sort(list lst) { /* bubble sort */
	int i, j;
	for (i = 0; i < size - 1; i++)
		for (j = i + 1; j < size - 1; j++)
			if (bigger(lst[i], lst[j]))
				swap(&lst[i], &lst[j]);
}

void print(list lst) { /* print list with spaces */
	for(; *lst; ++lst) {
		printf("%s ", *lst);
	}
	putchar('\n');
}

void destroy(list *lst) { /* free all memory */
	int i = 0;
	char *t = *lst[i];
	while (t != NULL) {
		free(t);
		i++;
		t = (*lst)[i];
	}
	free(*lst);
	*lst = NULL;
}

int get_size(void) {
	return size;
}
