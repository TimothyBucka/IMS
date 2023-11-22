CC=g++
CFLAGS=-Wall -Wextra -pedantic -std=c++11 -lsimlib -lm
NAME=SimProgram

SimProgram: $(NAME).cpp
	$(CC) $(NAME).cpp -o $(NAME) $(CFLAGS)

clean:
	rm -f $(NAME)
