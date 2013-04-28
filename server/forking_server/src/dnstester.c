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
	query.flags = 8;
	query.z = 0;
	query.rcode = 0;
	query.qcount = 1;
	query.name = "nwprog1.netlab.hut.fi";
	query.type = str_to_rrtype("AAAA");

	printf("generating query\n");
	len = generate_query_msg(&query, sendbuf);

	for (i = 0; i < len; i++) {
		printf("%02x ", sendbuf[i]);
	}
	printf("\n");

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
		return -1;
	}
	printf("parsing response\n");
	parse_dns_response(recvbuf, &response);
	for (i = 0; i < response.rr_count; i++) printf("result: %s\n", response.rr[i].addr);


	return 0;
}