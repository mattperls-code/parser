CFLAGS = -Wall -Wextra -Werror -std=c++17

main: main.cpp parser.cpp
	clang++ $(CFLAGS) -o main parser.cpp main.cpp

.PHONY: clean
clean:
	rm -rf main