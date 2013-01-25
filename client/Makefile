CC = gcc 
CFLAGS = -Wall -Wextra -pedantic
SOURCES = src/npbcli.c src/send_recv.c src/tcp_connect.c
LIBS = src/npbcli.h
OBJS = $(SOURCES:.c=.o)
EXE = npbcli

all: $(EXE) clean

$(EXE): $(OBJS)
	$(CC) $(OBJS) -o $@

$(OBJS): $(LIBS)

clean:
	rm -f $(OBJS)
