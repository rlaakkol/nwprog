CC = gcc 
CFLAGS = -Wall -Wextra -pedantic
SOURCES = src/hdcli.c src/myhttp.c src/mysockio.c src/myconnect.c
LIBS = src/myhttp.h src/mysockio.h src/myconnect.h
OBJS = $(SOURCES:.c=.o)
EXE = hdcli

all: $(EXE) clean

$(EXE): $(OBJS)
	$(CC) $(OBJS) -o $@

$(OBJS): $(LIBS)

clean:
	rm -f $(OBJS)
