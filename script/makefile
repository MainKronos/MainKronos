CC := g++
CFLAGS := -Iutils -fdiagnostics-color=always -pthread -std=c++20 -Wall -Wextra -Werror -g -O0 -m64

main: main.o
	$(CC) $(CFLAGS) $^ -o $@.bin

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@