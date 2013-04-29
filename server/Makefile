CC = gcc
CFLAGS = -std=gnu99 -c -Wall -Wextra -pedantic -g
SRC = src/main.c src/npbserver.c src/myhttp.c src/myconnect.c src/mydns.c src/mysockio.c
HDRS = src/myhttp.h src/npbserver.h src/myconnect.h src/mydns.h src/mysockio.h
OBJ = $(SRC:.c=.o)
EXE = hdserver
TESTER = src/dnstester.c

all: $(EXE)

$(EXE): $(OBJ) 
	$(CC) $(OBJ) -o $@

$(OBJ): $(HDRS)

clean:
	rm -f $(OBJ)
