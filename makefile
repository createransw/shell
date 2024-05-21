CC=gcc
CFLAGS=-Wpedantic -g
NAME=my_shell
DEPS=list.h tree.h run.h
OBJ=list.o tree.o run.o my_shell.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(NAME): $(DEPS) $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
