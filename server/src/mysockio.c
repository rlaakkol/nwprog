#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "mysockio.h"

char	buf[BUF_SIZE];
char 	*readptr;
size_t 	buffered;

/* Read n bytes from socket fd and store into vptr
 * DISCLAIMER: This function is copied directly from Stevens UNP book example
 */
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

/* Write "n" bytes to a descriptor. 
 * DISCLAIMER: This function is copied directly from Stevens UNP book example
 */
ssize_t                         
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

/* Write local file stream contents to socket */
int
write_file(int fd, FILE *lfile, size_t n)
{
	char 	wbuf[BUF_SIZE];
	size_t 	nleft, nread;

	nleft = n;

	while (nleft > 0) {
		/* Read file stream */
		if ((nread = fread(wbuf, sizeof(char), BUF_SIZE, lfile)) == 0) {
			break;
		}

		/* Write to socket */
		nleft -= writen(fd, wbuf, nread);
	}

	return nleft;

}

void
buf_init(void)
{
	buffered = 0;
	readptr = buf;
}

int
readn_buf(int fd, void *dest, size_t n)
{
	int 	readlen;



	if (buffered >= n) {
		memcpy(dest, readptr, n);
		buffered -= n;
		readptr += n;
		return n;
	}
	if (buffered > 0) {
		memcpy(dest, readptr, buffered);
		dest = (char *)dest + buffered;
		n -= buffered;
		buf_init();
	}
	readlen = read(fd, buf, BUF_SIZE);

	/* TODO: Error handling */
	if (readlen < 0) {
		return -1;
	}

	if ((unsigned int)readlen < n) {
		memcpy(dest, buf, readlen);
		buf_init();
		return buffered + readlen;
	}

	memcpy(dest, buf, n);
	buffered = readlen - n;
	readptr = buf + n;

	return n;
}
