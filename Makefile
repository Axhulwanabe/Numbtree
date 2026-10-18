# Numbtree
#
#   make              -> ./Numbtree
#   make debug        -> ./Numbtree-debug (no optimizations, sanitizers on)

EXE    = Numbtree
CC     = gcc
CFLAGS = -std=c17 -Wall -Wextra -O3 -march=native -flto

SRC     = $(wildcard src/*.c)
HEADERS = $(wildcard src/*.h)

all: $(EXE)

$(EXE): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(SRC) -o $(EXE)

debug: $(SRC) $(HEADERS)
	$(CC) -std=c17 -Wall -Wextra -O0 -g -fsanitize=address,undefined $(SRC) -o $(EXE)-debug

clean:
	rm -f $(EXE) $(EXE)-debug $(EXE).exe

.PHONY: all debug clean
