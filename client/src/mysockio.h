#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 4*1024

/* Read n bytes from socket fd and store into vptr */
int
readn(int fd, void *dest, size_t n);

/* Write "n" bytes to a descriptor. */
ssize_t
writen(int fd, const void *vptr, size_t n);

/* Write local file stream contents to socket */
int
write_file(int fd, FILE *lfile, size_t n);