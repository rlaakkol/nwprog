#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <syslog.h>
#include <dirent.h>

#include "npbserver.h"
#include "myhttp.h"


/* Start listening on port <serv>
 *
 * */
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


/*ssize_t writen(int fd, const void *vptr, size_t n)
*{
        size_t          nleft;
        ssize_t         nwritten;
        const char      *ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0) {
                if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
                        if (nwritten < 0 && errno == EINTR)
                                nwritten = 0;           /
                        else
                                return(-1);                     
                }

                nleft -= nwritten;
                ptr   += nwritten;
        }
        return(n);
}*/

void web_child(int sockfd)
{
        int             h;
        ssize_t         nread, rem;
        char            buf[MAXLINE], *line, *templine;
	Http_info	*specs;

	specs = malloc(sizeof(Http_info));
	
	memset(specs, 0, sizeof(Http_info));
	while (1) {
		/*nread = 0;
		while ((nread += read(sockfd, buf+nread, MAXLINE-nread)) > 0 && strstr(buf, "\r\n\r\n") == NULL);

		if (nread <= 0) break;

		
		// Parse startline
		if (parse_startline(buf, specs) < 0) {
			writen(sockfd, REPLY_400, strlen(REPLY_400));
			exit(-1);	// Malformed request
		}
		// Parse headers
		line = buf;
		while (1) {
			if ((templine = strchr(line, '\n')) == NULL) {
				syslog(LOG_INFO, "header error 1 (%zi read bytes): %s", nread, line);
				writen(sockfd, REPLY_400, strlen(REPLY_400));
				exit(-1);
			}
			line = templine;
			line++;

			h = parse_header(line, specs);
			if (h == 0) break; 	// End of header
			if (h < 0) {
				syslog(LOG_INFO, "header error 2");
				writen(sockfd, REPLY_400, strlen(REPLY_400));
				exit(-1); 	// Malformed header field
			}

		}
		// data remaining on buffer
		rem = nread - (line + 2 - buf);

		if (specs->command == GET) {	// Send file
			process_get(sockfd, specs);
		} else if (specs->command == PUT) {	// Store file
			process_put(sockfd, specs, line + 2, rem);
		} else {			// Unknown command
			writen(sockfd, REPLY_400, strlen(REPLY_400));
			exit(-1);
		}
		if (specs->close) break;
		memset(specs, 0, sizeof(Http_info));*/
		parse_request(sockfd, specs);

		if (specs->command == GET) {	// Send file
			process_get(sockfd, specs);
		} else if (specs->command == PUT) {	// Store file
			process_put(sockfd, specs);
		else if (specs->command == POST) {
			process_post(sockfd, specs);
		} else {			// Unknown command
			writen(sockfd, REPLY_400, strlen(REPLY_400));
			exit(-1);
		}
		if (specs->close) break;
		memset(specs, 0, sizeof(Http_info));
		

	}
	free(specs);

}




// Process SIGCHLD: when child dies, must use waitpid to clean it up
// otherwise it will remain as zombie
void sig_chld(int signo)
{
        pid_t   pid;
        int             stat;
	syslog(LOG_INFO, "received signal %d", signo);
        while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        }
        return;
}

