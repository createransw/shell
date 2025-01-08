CC=gcc
CFLAGS=-Wpedantic -g
NAME=my_shell
DEPS=src/list.h src/tree.h src/run.h
OBJ=list.o tree.o run.o my_shell.o

%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(NAME): $(DEPS) $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(NAME) $(OBJ)
