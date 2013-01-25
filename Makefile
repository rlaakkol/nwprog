CC = gcc 
CFLAGS = -Wall -Wextra -pedantic
SOURCES = src/.c src/.c
LIBS = src/.h
OBJS = $(SOURCES:.c=.o)
EXE = 

all: $(EXE) clean

$(EXE): $(OBJS)
	$(CC) $(OBJS) -o $@

$(OBJS): $(LIBS)

clean:
	rm -f $(OBJS)
