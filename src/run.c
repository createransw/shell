#include "run.h"

static tree tr;
static int fail = 0;

static void exits(void) {
	remove_tree();
	exit(0);
}

static void sigh(int s) {
	signal(SIGUSR1, sigh);
	++fail;
}

static void cd(tree t) {
	char *s;
	if (t->cmd->arg[1] == NULL) {
		s = getenv("HOME");
		if (chdir(s) < 0)
			fprintf(stderr, "%s\n", strerror(errno));
		return;
	}
	if (chdir(t->cmd->arg[1]) < 0)
		fprintf(stderr, "%s\n", strerror(errno));
}

static int execute(tree t, int mpid) {
	int pid;
	int status;
	if (t == tr) { /* inside commands */
		if ((tr->opflg == 0) && !strcmp(tr->cmd->arg[0], "cd")) {
			cd(t);
			return 0;
		}
		if ((tr->opflg == 0) && !strcmp(tr->cmd->arg[0], "exit")) {
			exits();
			return 0;
		}
	}
	if((pid = fork()) == 0) { /* start a sun for next duty  */
		if (t->opflg == 0)
			execute_cmd(t, mpid);
		else if ((t->op->type == ord) || (t->op->type == par))
			status = execute_ord(t, mpid);
		else if ((t->op->type == or) || (t->op->type == and))
			status = execute_log(t, mpid);
		else if (t->op->type == cnv)
			status = execute_conv(t, mpid);
		exit(status);
	}	
	waitpid(pid, &status, 0); /* wait for son to return status */
	if (t->opflg == 0)
		if (!WIFEXITED(status) || WEXITSTATUS(status))
			kill(mpid, SIGUSR1);
	if (!WIFEXITED(status)) /* sun was killed */
		return 1;
	return WEXITSTATUS(status);
}

static int execute_ord(tree t, int mpid) {
	signal(SIGINT, SIG_IGN); /* SIG_INT can't stop this process */
	int pid;
	int status;
	int fd;
	if ((pid = fork()) == 0) { /* sun */
		if (t->op->type == par) {
			if ((fd = open("/dev/null" , O_RDONLY)) < 0) {
				fprintf(stderr, "%s\n", strerror(errno));
				exit(1);
			}
			if (dup2(fd, 0) < 0) {
				fprintf(stderr, "%s\n", strerror(errno));
				exit(1);
			}
			if (close(fd) < 0) {
				fprintf(stderr, "%s\n", strerror(errno));
				exit(1);
			}
		}
		if(!(t->op->right) && (t->op->type == ord))
			signal(SIGINT, SIG_DFL);
		execute(t->op->left, mpid);
		exit(0);
	}
	if (t->op->type == ord)
		waitpid(pid, NULL, 0);
	if (t->op->right) { /* there is right sub tree */
		if ((pid = fork()) == 0) { /* sun */
			signal(SIGINT, SIG_DFL); /* SIG_INT can stop this process */
			status = execute(t->op->right, mpid);
			exit(status);
		}
		waitpid(pid, &status, 0);
		if (!WIFEXITED(status)) /* sun was killed */
			return 1;
		return WEXITSTATUS(status);
	}
	return 0;
}

static int execute_log(tree t, int mpid) {
	if (t == tr)
		signal(SIGINT, SIG_DFL);
	int pid;
	int status;
	if ((pid = fork()) == 0) { /* sun */
		status = execute(t->op->left, mpid); /* try left operation */
		exit(status);
	}
	waitpid(pid, &status, 0);
	if ((t->op->type == and) && WIFEXITED(status) && !WEXITSTATUS(status)) {
		if ((pid = fork()) == 0) { /* sun */
			status = execute(t->op->right, mpid); /* right operation */
			exit(status);
		}
		waitpid(pid, &status, 0);
	}
	if ((t->op->type==or) && (!WIFEXITED(status) || WEXITSTATUS(status))) {
		if ((pid = fork()) == 0) { /* sun */
			status = execute(t->op->right, mpid); /* right operation */
			exit(status);
		}
		waitpid(pid, &status, 0);
	}
	if (!WIFEXITED(status)) /* sun was killed */
		return  1;
	else
		return WEXITSTATUS(status);
}

static int execute_conv(tree t, int mpid) {
	if (t == tr)
		signal(SIGINT, SIG_DFL);
	int status;
	int pid, pid1;
	int p[2]; /* pipe */
	if (pipe(p) < 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}
	if ((pid1 = fork()) == 0) { /* left operation */
		if (close(p[0]) < 0) { /* don't read from pipe */
			fprintf(stderr, "%sa\n", strerror(errno));
			exit(1);
		}
		if (dup2(p[1], 1) < 0) { /* write to pipe */
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		if (close(p[1]) < 0) { /* copy is in 1 */
			fprintf(stderr, "%sb\n", strerror(errno));
			exit(1);
		}
		execute(t->op->left, mpid); /* execute will close other streams */
		exit(0);
	}
	if (close(p[1]) < 0) { /* don't write to pipe */
		fprintf(stderr, "%sc\n", strerror(errno));
		exit(1);
	}
	if ((pid = fork()) == 0) { /* right operation */
		if (dup2(p[0], 0) < 0) { /* read from pipe */
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		if (close(p[0]) < 0) { /* copy is in 2 */
			fprintf(stderr, "%sd\n", strerror(errno));
			exit(1);
		}
		status = execute(t->op->right, mpid);/*execute will close other streams*/
		exit(status);
	}
	if (close(p[0]) < 0) {
		fprintf(stderr, "%se\n", strerror(errno));
		exit(1);
	}
	waitpid(pid1, NULL, 0);
	waitpid(pid, &status, 0);
	if (!WIFEXITED(status)) /* sun was killed */
		return 1;
	return WEXITSTATUS(status);
}

static void execute_cmd(tree t, int mpid) {
	if (t == tr)
		signal(SIGINT, SIG_DFL);
	int fd;
	int status;
	if ((tr->opflg == 0) && !strcmp(tr->cmd->arg[0], "cd")) { /* ignore */
		exit(0);
	}
	if ((tr->opflg == 0) && !strcmp(tr->cmd->arg[0], "exit")) { /* ignore */
		exit(0);
	} 
	cmd_inf *com = t->cmd; /* command info structure */
	if(com->inpf) {
		if ((fd = open(com->inpf, O_RDONLY)) < 0) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		if (dup2(fd, 0) < 0) { /* replace input stream */
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		if (close(fd) < 0) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
	}
	if(com->outpf) {
		if ((fd = open(com->outpf, com->outpflags, 0666)) < 0) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		if (dup2(fd, 1) < 0) { /* replace output stream */
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
		if (close(fd) < 0) {
			fprintf(stderr, "%s\n", strerror(errno));
			exit(1);
		}
	}
	if (com->subshell) {
		status = execute(com->subshell, mpid);
		exit(status);
	}
	if (execvp(com->arg[0], com->arg) < 0) {
		fprintf(stderr, "%s\n", strerror(errno));
		exit(1);
	}
}

void dialog(void) {
	signal(SIGINT, SIG_IGN); /* SIG_INT can't stop shell */
	signal(SIGUSR1, sigh);
	tr = new_tree();
	while (1) {
		if (tr == NULL)
			break;
		execute(tr, getpid());
		remove_tree();
		tr = new_tree();
	}
}
