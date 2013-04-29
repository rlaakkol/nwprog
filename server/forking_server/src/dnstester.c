#include <stdio.h>
#include <inttypes.h>

#include "npbserver.h"
#include "myconnect.h"
#include "mydns.h"

int
main(int argc, char *argv[])
{
	int udpsock, len, i;
	uint16_t 	tmp;
	ssize_t recvd;
	char *server, sendbuf[10*MAXLINE], recvbuf[MAXN];
	struct timeval timeout;
	dns_msg query, response;

   /* tmp = (0 << 15) & 0x8000;
	tmp |= ((DNS_STANDARD << 11) & 0x7800);
	tmp |= ((1 << 9) & 0x0F00);
	printf("second uint: %X \n", tmp);*/

	response.rr_count = 0;

	server = argv[1];


	/* Connect to dns server */
	udpsock = myconnect(server, argv[2], SOCK_DGRAM);
	query.type = DNS_QUERY;
	query.opcode = DNS_STANDARD;
	query.flags = 0;
	query.z = 0;
	query.rcode = 0;
	query.qcount = 1;
	query.name = argv[3];
	query.type = str_to_rrtype(argv[4]);

	printf("generating query\n");
	len = generate_query_msg(&query, sendbuf);

	for (i = 0; i < len; i++) {
		printf("%X ", sendbuf[i]);
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