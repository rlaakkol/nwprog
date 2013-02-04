/*
 *DISCLAIMER!
 *This code is (almost) entirely copied from the tcp_connect.c -example of the course lectures!
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>


#include "tcp_connect.h"

/* Get IPV4 or IPV6 address from struct sockaddr (in network format)
 * to be used with inet_ntop
 * Copied from http://beej.us/guide/bgnet/examples/client.c */
void
*get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int
tcp_connect(const char *host, const char *serv)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;
	char addrstr[INET6_ADDRSTRLEN];
/*	char outbuf[80]; */
 	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0) {
		fprintf(stderr, "tcp_connect error for %s, port %s: %s\n",
			host, serv, gai_strerror(n));
		return -1;
	}
	ressave = res;

	do {
		sockfd = socket(res->ai_family, res->ai_socktype,
			res->ai_protocol);
		fprintf(stdout, "Trying address %s\n", inet_ntop(res->ai_family, get_in_addr(res->ai_addr), addrstr, sizeof(addrstr)));
		if (sockfd < 0)
			continue;	/* ignore this one */

			if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0){
				fprintf(stdout, "Connected to address %s\n", addrstr);
				break;		/* success */
			}

		close(sockfd);	/* ignore this one */
		} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {	/* errno set from final connect() */
		fprintf(stderr, "tcp_connect error for %s, port %s: %s\n", host, serv, strerror(errno));
		sockfd = -1;
	} /* else {
		struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
		const char *ret = inet_ntop(res->ai_family, &(sin->sin_addr),
		outbuf, sizeof(outbuf));
	} */

freeaddrinfo(ressave);

return(sockfd);
}
