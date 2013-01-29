char	buf[BUF_SIZE];
char 	*readptr;
size_t 	buffered;

void
buf_init(void)
{
	buffered = 0;
	readptr = buf;
}

void
readn(int fd, void *vptr, size_t n)
{
    size_t	nleft;
    ssize_t	nread;
    char	*ptr;
    
    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
	if ( (nread = read(fd, ptr, nleft)) < 0) {
	    if (errno == EINTR)
			nread = 0;		/* and call read() again */
	    else
			return(-1);
	} else if (nread == 0)
	    break;				/* EOF */

	nleft -= nread;
	ptr   += nread;
    }
    return(n - nleft);		/* return >= 0 */
}

size_t
readn_buf(int fd, void *dest, size_t n)
{
	size_t 	readlen, temp, rem;

	if (buffered >= n) {
		memcpy(dest, buf, n);
		buffered -= n;
		return n;
	}
	temp = buffered;
	memcpy(dest, buf, buffered);
	dest += buffered + 1;
	rem = n - buffered;
	readlen = readn(fd, buf, BUF_SIZE);
	/* TODO: Error handling */
	memcpy(dest, buf, readlen);
	if (readlen < rem) {
		buf_init();
		return buffered + readlen;
	}
	buffered -= rem;
	readptr = buf + rem + 1;

	return n;
}
