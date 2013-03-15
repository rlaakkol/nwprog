CC = gcc
CFLAGS = -std=gnu99 -c -Wall -Wextra -pedantic -g
SRC = src/main.c src/npbserver.c src/myhttp.c
LIBS = src/myhttp.h src/npbserver.h
OBJ = $(SRC:.c=.o)
EXECUTABLE = npbsrv

all: $(SRC) $(EXECUTABLE)

$(EXECUTABLE): $(OBJ) 
	$(CC) $(OBJ) -o $@

$(OBJ): $(LIBS)

clean:
	rm -f $(OBJ)
