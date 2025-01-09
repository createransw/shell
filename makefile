CC=gcc
CFLAGS=-Wpedantic -g
NAME=my_shell
TEST_NAME=test_runner
DEPS=src/list.h src/tree.h src/run.h
OBJ=src/list.o src/tree.o src/run.o src/my_shell.o
TEST_OBJ=src/list.o src/tree.o src/run.o  tests/test_run.o

all: $(NAME) $(TEST_NAME)

src/%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

tests/%.o: tests/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(NAME): $(DEPS) $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

$(TEST_NAME): $(DEPS) $(TEST_OBJ)
	$(CC) -o test_runner $^ $(CFLAGS) -lcheck

clean:
	rm -f $(NAME) $(OBJ) test_runner $(TEST_OBJ)

test:
	./test_runner
