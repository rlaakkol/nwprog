#include <inttypes.h>


#define MAX_RRS 20
#define DNS_QUERY 0
#define DNS_STANDARD 0
#define RR_TYPE_A 0
#define RR_TYPE_AAAA 28
#define MAX_RETRY 1
#define MAXLINE 1024


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
	uint8_t		type;
	uint8_t		opcode;
	uint8_t 	flags;
	uint8_t		z;
	uint8_t 	rcode;
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