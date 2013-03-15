#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>

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

int tcp_listen(const char *host, const char *serv, socklen_t *addrlenp)
{
        int                             listenfd, n;
        const int               on = 1;
        struct addrinfo hints, *res, *ressave;

        bzero(&hints, sizeof(struct addrinfo));
        hints.ai_flags = AI_PASSIVE;
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0) {
                syslog(LOG_ERR, "tcp_listen error for %s, %s: %s",
                                 host, serv, gai_strerror(n));
		return -1;
	}
        ressave = res;

        do {
                listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
                if (listenfd < 0)
                        continue;               /* error, try next one */

                setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
                if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
                        break;                  /* success */

                close(listenfd);        /* bind error, close and try next one */
        } while ( (res = res->ai_next) != NULL);

        if (res == NULL) {        /* errno from final socket() or bind() */
                syslog(LOG_ERR, "tcp_listen error for %s, %s", host, serv);
		return -1;
	}

        if (listen(listenfd, LISTENQ) < 0) {
		syslog(LOG_ERR,"listen");
		return -1;
	}

        if (addrlenp)
                *addrlenp = res->ai_addrlen;    /* return size of protocol address */

        freeaddrinfo(ressave);

        return(listenfd);
}



/* Write "n" bytes to a descriptor. 
 * DISCLAIMER: This function is copied directly from Stevens UNP book example
 */
// ssize_t                         
// writen(int fd, const void *vptr, size_t n)
// {
// 	size_t nleft;
// 	ssize_t nwritten;
// 	const char *ptr;

// 	ptr = vptr;
// 	nleft = n;
// 	while (nleft > 0) {
// 		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
// 			if (nwritten < 0 && errno == EINTR) {
// 				nwritten = 0;	/* and call write() again */
// 			} else {
// 				return (-1);	/* error */
// 			}
// 		}

// 		nleft -= nwritten;
// 		ptr += nwritten;
// 	}
// 	return (n);
// }

/* Write local file stream contents to socket */
// int
// write_file(int fd, FILE *lfile)
// {
// 	char 	wbuf[BUF_SIZE];
// 	size_t 	written, nread;

// 	written = 0;


// 		/* Read file stream */
// 		if ((nread = fread(wbuf, sizeof(char), BUF_SIZE, lfile)) == 0) {
// 			break;
// 		}

// 		/* Write to socket */
// 		written = writen(fd, wbuf, nread);


// 	return written;

// }

my_buf
*buf_init(void)
{
	my_buf 	*buf;

	buf = malloc(sizeof(my_buf));

	buf->buf = malloc(BUF_SIZE);
	buf->buffered = 0;
	buf->readptr = buf->buf;

	return buf;

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
	buf->readptr = buf->buf + n;

	return n;
}
