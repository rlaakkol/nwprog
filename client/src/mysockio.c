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