#include <stdlib.h>

#define BUF_SIZE 4*1024

void
buf_init(void);

int
readn_buf(int fd, void *dest, size_t n);

size_t
writen(int fd, const void *vptr, size_t n);

int
write_file(int fd, FILE *lfile, size_t n);