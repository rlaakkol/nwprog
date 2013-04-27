#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


#include "mydns.h"


int
str_to_rrtype(const char *str)
{
	if (!strncasecmp(str, "AAAA", 4)) return RR_TYPE_AAAA;
	else if (!strncasecmp(str, "A", 1)) return RR_TYPE_A;
	else return -1;
}

int
expand_to_qname(char *name, char *buf)
{
	char *last = name, *first = name;
	unsigned int offset = 0;
	uint8_t octets;

	while (1) {
		if (*last == '.' || *last == '\0') {
			octets = last - first;
			memcpy(buf + offset, &octets, 1);
			memcpy(buf + offset + 1, first, octets);
			offset += octets + 1;
			printf("qoffset: %u\n", offset);
			
			if (*last == '\0') {
				buf[offset] = 0;
				return offset + 1;
			}

			first = last + 1;
		} 
		last++;
	}

	return -1;
}

int
qnamelen(char* buf)
{
	unsigned int offset = 0;
	char c;
	while(1) { 
		c = buf[offset];
		if (c == 0) return offset + 1;
		offset += c + 1;
	}

	return -1;

}

int
generate_query_msg(dns_msg *q, char *buf)
{
	uint16_t tmp;
	unsigned int offset = 0;
	int 	qname_len;
	/* header */

	/*id*/
	q->id = (uint16_t) getpid();
	tmp = htons(q->id);
	memcpy(buf, &tmp, 2);
	offset += 2;
	printf("offset: %u\n", offset);
	/*line2*/
	tmp = q->type << 15; /* query type */
	tmp |= ((q->opcode << 11) & 0x7800);
	tmp |= (q->flags << 7 & 0x0780);
	memcpy(buf + offset, &tmp, 2);
	offset += 2;
	printf("offset: %u\n", offset);
	/*line3*/
	tmp = htons(q->qcount);
	memcpy(buf, &tmp, 2);
	offset += 2;
	printf("offset: %u\n", offset);
	/*only queries*/
	bzero(buf + offset, 6);
	offset += 6;
	printf("offset: %u\n", offset);

	/*append label-formatted qname */
	qname_len = expand_to_qname(q->name, buf + offset);
	offset += qname_len;
	printf("offset: %u\n", offset);
	tmp = htons(q->type);
	memcpy(buf + offset, &tmp, 2);
	offset += 2;
	printf("offset: %u\n", offset);
	tmp = htons(0);
	memcpy(buf + offset, &tmp, 2);
	offset += 2;
	printf("offset: %u\n", offset);

	return offset;
}

int
parse_rr(char *rr_msg, dns_rr *rr)
{
	unsigned int offset = 0;
	uint16_t tmps;
	uint32_t tmpl;
	uint8_t tmp[16];
	struct in6_addr addr6;
	struct in_addr addr;
	char *addrstr;

	offset += qnamelen(rr_msg);
	memcpy(rr->name, rr_msg, offset);

	memcpy(&tmps, rr_msg + offset, 2);
	offset += 2;
	rr->type = htons(tmps);

	if (!(rr->type == RR_TYPE_A || rr->type == RR_TYPE_AAAA)) return -1;

	memcpy(&tmps, rr_msg + offset, 2);
	offset += 2;
	rr->class = htons(tmps);

	memcpy(&tmpl, rr_msg + offset, 4);
	offset += 4;
	rr->ttl = htons(tmpl);

	memcpy(&tmps, rr_msg + offset, 2);
	offset += 2;
	rr->rdlength = htons(tmps);

	if (rr->type == RR_TYPE_A) {
		if (rr->rdlength != 4) return -1;
		addrstr = malloc(20*sizeof(char));
		memcpy(&tmpl, rr_msg + offset, 4);
		offset += 4;
		memcpy(&(addr.s_addr), &tmpl, 4);
		rr->addr = inet_ntop(AF_INET, &addr, addrstr, 20);
	} else if (rr->type == RR_TYPE_AAAA) {
		if (rr->rdlength != 16) return -1;
		addrstr = malloc(30*sizeof(char));
		memcpy(tmp, rr_msg + offset, 16);
		offset += 16;
		memcpy(&(addr6.s6_addr), tmp, 16);
		rr->addr = inet_ntop(AF_INET6, &addr6, addrstr, 30);
	}
	else return -1;

	return offset;

}

int
parse_dns_response(char *msg, dns_msg *r)
{
	int i;
	uint16_t tmp;
	unsigned int offset = 0;

	memcpy(&tmp, msg, 2);
	offset += 2;
	r->id = ntohs(tmp);
	

	memcpy(&tmp, msg + offset, 2);
	offset += 2;
	r->type = (tmp & 0x8000) >> 15;
	r->opcode = (tmp & 0x7800) >> 11;
	r->flags = (tmp & 0x0780) >> 7;
	r->z = (tmp & 0x0070) >> 4;
	r->rcode = (tmp & 0x000F);

	if (r->rcode != 0) return 1;

	memcpy(&tmp, msg + offset, 2);
	offset += 2;
	r->qcount = ntohs(tmp);
	if (r->qcount > 0) {
		return -1; /*query from server?*/
	}
	memcpy(&tmp, msg + offset, 2);
	offset += 2;
	r->ancount = ntohs(tmp);
	memcpy(&tmp, msg + offset, 2);
	offset += 2;
	r->nscount = ntohs(tmp);
	memcpy(&tmp, msg + offset, 2);
	offset += 2;
	r->atcount = ntohs(tmp);

	for (i = 0; i < r->ancount; i++) {

		offset += parse_rr(msg + offset, (r->rr + i));
		(r->rr_count)++;
	}

	for (i = 0; i < r->nscount; i++) {
		/* TODO */
	}

	for (i = 0; i < r->atcount; i++) {
		/* TODO */
	}

	return 0;

}