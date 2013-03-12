#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "mysockio.h"

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
write_file(int fd, FILE *lfile)
{
	char 	wbuf[BUF_SIZE];
	size_t 	written, nread;

	written = 0;


		/* Read file stream */
		if ((nread = fread(wbuf, sizeof(char), BUF_SIZE, lfile)) == 0) {
			break;
		}

		/* Write to socket */
		written = writen(fd, wbuf, nread);


	return written;

}

my_buf
*buf_init(void)
{
	my_buf 	*buf;

	buf = malloc(sizeof(my_buf));

	buf->buffered = 0;
	buf->readptr = buf->buf;

}


int
readn_buf(my_buf *buf, int fd, void *dest, size_t n)
{
	int 	readlen, prev;



	if (buf->buffered >= n) {
		memcpy(dest, buf->readptr, n);
		buf->buffered -= n;
		buf->readptr += n;
		return n;
	}
	if (buf->buffered > 0) {
		memcpy(dest, buf->readptr, buf->buffered);
		dest = (char *)dest + buf->buffered;
		n -= buf->buffered;
		buf->buffered = 0;
		buf->readptr = buf->buf;
	}
	readlen = read(fd, buf->buf, BUF_SIZE);

	/* TODO: Error handling */
	if (readlen < 0) {
		return -1;
	}

	if ((unsigned int)readlen < n) {
		memcpy(dest, buf->buf, readlen);
		prev = buf->buffered;
		buf->buffered = 0;
		buf->readptr = buf->buf;
		return prev + readlen;
	}

	memcpy(dest, buf->buf, n);
	buf->buffered = readlen - n;
	buf->readptr = buf + n;

	return n;
}
