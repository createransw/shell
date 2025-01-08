#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "tree.h"

static int execute_ord(tree t, int mpid);
static int execute_log(tree t, int mpid);
static int execute_conv(tree t, int mpid);
static void execute_cmd(tree t, int mpid);

void dialog(void);
