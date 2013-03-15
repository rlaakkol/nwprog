#ifndef MYSOCKIO_H
#define MYSOCKIO_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



#define BUF_SIZE 4*1024
#define LISTENQ 5

typedef struct {
	char 	*buf;
	char 	*readptr;
	size_t 	buffered;
} my_buf;

int tcp_listen(const char *host, const char *serv, socklen_t *addrlenp);

int tcp_connect(const char *host, const char *serv);

/* Read n bytes from socket fd and store into vptr */
int
readn(int fd, void *dest, size_t n);

/* Write "n" bytes to a descriptor. */
ssize_t
writen(int fd, const void *vptr, size_t n);

/* Write local file stream contents to socket */
int
write_file(int fd, FILE *lfile, size_t n);

my_buf *
buf_init(void);

int
readn_buf(my_buf *buf, int fd, void *dest, size_t n);

#endif