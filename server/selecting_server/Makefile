CC = gcc
CFLAGS = -std=gnu99 -c -Wall -Wextra -pedantic -g $(shell pkg-config --cflags glib-2.0)
LDFLAGS = $(shell pkg-config --libs glib-2.0)
SRC = src/hdserver.c src/myhttp.c src/mysockio.c
LIBS = src/myhttp.h src/mysockio.h
OBJ = $(SRC:.c=.o)
EXECUTABLE = hdserver

all: $(SRC) $(EXECUTABLE)

$(EXECUTABLE): $(OBJ) 
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(OBJ): $(LIBS)

clean:
	rm -f $(OBJ)
