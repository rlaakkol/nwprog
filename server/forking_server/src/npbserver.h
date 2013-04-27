#ifndef NPBSERVER_H
#define NPBSERVER_H

#include <sys/socket.h>



#define LISTENQ 5
#define	MAXN	16384		/* max # bytes to request from server */
#define MAXLINE 1024

int tcp_listen(const char *host, const char *serv, socklen_t *addrlenp);
void web_child(int sockfd);
void sig_chld(int signo);
/*int tcp_connect(const char *host, const char *serv);*/
/*ssize_t readn(int fd, void *vptr, size_t n);*/
/*ssize_t writen(int fd, const void *vptr, size_t n);*/

#endif
