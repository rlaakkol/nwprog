#include <inttypes.h>


#define MAX_RRS 20
#define DNS_QUERY 0
#define DNS_STANDARD 0
#define RR_TYPE_A 1
#define RR_TYPE_AAAA 28
#define MAX_RETRY 3
#define MAXLINE 1024

#define HTTP_BODY_F "Name=%s&Addr="


typedef struct {
	char 		name[MAXLINE];
	uint16_t 	type;
	uint16_t 	class;
	uint32_t	ttl;
	uint16_t 	rdlength;
	const char 	*addr;
} dns_rr;

typedef struct {
	char 		*name;
	uint16_t 	id;
	uint16_t		type;
	uint16_t		opcode;
	uint16_t 	flags;
	uint16_t		z;
	uint16_t 	rcode;
	uint16_t	qcount;
	uint16_t	ancount;
	uint16_t	nscount;
	uint16_t 	atcount;
	dns_rr 		rr[MAX_RRS];
	int 		rr_count;
} dns_msg; 

int
generate_query_msg(dns_msg *q, char *buf);

int
parse_dns_response(char *msg, dns_msg *r);

int
str_to_rrtype(const char *str);