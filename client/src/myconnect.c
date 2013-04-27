/*
 *DISCLAIMER!
 *This code is (almost) entirely copied from the tcp_connect.c -example of the course lectures!
 * My additions: IP-address printing and comments
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


#include "connect.h"

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

/* Connect to address host (IP or DNS name), port serv, return socket fd */
int
connect(const char *host, const char *serv, int proto)
{
	int				sockfd, n;
	struct addrinfo	hints, *res, *ressave;
	char addrstr[INET6_ADDRSTRLEN];
	/* Specify to get both IPV6 adn IPV4 addresses and use TCP*/
 	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = proto;

	fprintf(stdout, "Connecting to %s, port %s\n", host, serv);
	/* Do a DNS query */
	if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0) {
		fprintf(stderr, "Error getting DNS record for %s, port %s: %s\n",
			host, serv, gai_strerror(n));
		return -1;
	}
	/* Save result pointer for freeing purposes */
	ressave = res;

	/* Iterate over DNS query results */
	do {
		/* Create socket */
		sockfd = socket(res->ai_family, res->ai_socktype,
			res->ai_protocol);
		fprintf(stdout, "Trying address %s\n", inet_ntop(res->ai_family, get_in_addr(res->ai_addr), addrstr, sizeof(addrstr)));
		if (sockfd < 0) /* Socket creation failed, */
			continue;	/* ignore this one, go to next  */

			if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0){
				fprintf(stdout, "Connected to address %s\n", addrstr);
				break;		/* Connect successful! Stop iterating! */
			}

		close(sockfd);	/* Connect unsuccessful. Ignore this one, go to next */
		} while ( (res = res->ai_next) != NULL);

	if (res == NULL) {	/* Successful connection did not happen. errno set from final connect() */
		fprintf(stderr, "Error connecting to %s, port %s: %s\n", host, serv, strerror(errno));
		sockfd = -1;
	}

	/* free resources */
	freeaddrinfo(ressave);


	return(sockfd);
}
