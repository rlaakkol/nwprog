#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "mysockio.h"

char	buf[BUF_SIZE];
char 	*readptr;
size_t 	buffered;

void
buf_init(void)
{
	buffered = 0;
	readptr = buf;
}

int
readn(int fd, void *vptr, size_t n)
{
	size_t	nleft;
	ssize_t	nread;
	char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR) {
				nread = 0;		/* and call read() again */
			} else {
				return(-1);
			}
		} else if (nread == 0) {
			break;				/* EOF */
		}

		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);		/* return >= 0 */
}

int
readn_buf(int fd, void *dest, size_t n)
{
	size_t 	rem;
	int 	readlen;

	if (buffered >= n) {
		memcpy(dest, buf, n);
		buffered -= n;
		return n;
	}
	memcpy(dest, buf, buffered);
	dest = (char *)dest + buffered;
	rem = n - buffered;
	readlen = readn(fd, buf, BUF_SIZE);
	/* TODO: Error handling */
	if (readlen < 0) {
		return -1;
	}
	memcpy(dest, buf, readlen);
	if ((unsigned int)readlen < rem) {
		buf_init();
		return buffered + readlen;
	}
	buffered -= rem;
	readptr = buf + rem;

	return n;
}

ssize_t                         /* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR) {
				nwritten = 0;	/* and call write() again */
			} else {
				return (-1);	/* error */
			}
		}

		nleft -= nwritten;
		ptr += nwritten;
	}
	return (n);
}

int
write_file(int fd, FILE *lfile, size_t n)
{
	char 	buf[BUF_SIZE];
	size_t 	nleft, nread;

	nleft = n;

	while (nleft > 0) {
		if ((nread = fread(buf, sizeof(char), BUF_SIZE, lfile)) == 0) {
			break;
		}

		nleft -= writen(fd, buf, nread);
	}

	return nleft;

}