#include <stdio.h>
#include <inttypes.h>

#include "npbserver.h"
#include "myconnect.h"
#include "mydns.h"

int
main(int argc, char *argv[])
{
	int udpsock, len, i;
	ssize_t recvd;
	char *server, sendbuf[10*MAXLINE], recvbuf[MAXN];
	struct timeval timeout;
	dns_msg query, response;

	response.rr_count = 0;

	server = "localhost";


	/* Connect to dns server */
	udpsock = myconnect(server, "4500", SOCK_DGRAM);
	query.type = DNS_QUERY;
	query.opcode = DNS_STANDARD;
	query.flags = 0;
	query.z = 0;
	query.rcode = 0;
	query.qcount = 1;
	query.name = "www.aalto.fi";
	query.type = str_to_rrtype("A");

	printf("generating query\n");
	len = generate_query_msg(&query, sendbuf);

	timeout.tv_sec = 1;
	timeout.tv_usec = 500000;

	setsockopt(udpsock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

	for (i = 0; i < MAX_RETRY; i++) {
		printf("sending query\n");
		send(udpsock, sendbuf, len, 0);
		if ((recvd = recv(udpsock, recvbuf, MAXN, 0)) > 0) break;
		printf("no answer\n");
	}
	if (recvd <= 0) {
		/*error*/
	}

	parse_dns_response(recvbuf, &response);
	for (i = 0; i < response.rr_count; i++) printf("%s", response.rr[i].addr);


	return 0;
}