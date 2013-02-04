#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUF_SIZE 4*1024

int
readn(int fd, void *dest, size_t n);

ssize_t
writen(int fd, const void *vptr, size_t n);

int
write_file(int fd, FILE *lfile, size_t n);