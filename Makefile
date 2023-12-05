CC=g++
CFLAGS=-Wall -Wextra -pedantic -std=c++11 -lsimlib -lm
NAME=SimProgram

SimProgram: $(NAME).cpp $(NAME).hpp
	$(CC) $(NAME).cpp -o $(NAME) $(CFLAGS)

run: $(NAME)
	make
	./$(NAME)

clean:
	rm -f $(NAME)
