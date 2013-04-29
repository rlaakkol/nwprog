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
	if (strlen(str) >= 4 && !strncasecmp(str, "AAAA", 4)) return RR_TYPE_AAAA;
	else return RR_TYPE_A;
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
qnamelen(char *buf)
{
	unsigned int offset = 0;
	char c;
	while(1) { 
		c = buf[offset];
		printf("%02x\n", c);
		if (c == 0) return offset + 1;
		if (c & 0xC0) return offset + 2;
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
	printf("id: %" SCNu16 "\n", q->id);
	tmp = htons(q->id);
	memcpy(buf + offset, &tmp, 2);
	offset += 2;

	/*line2*/
	
	/* tmp = ((q->type << 15) & 0x8000);
	tmp |= ((q->opcode << 11) & 0x7800);
	tmp |= ((q->flags << 9) & 0x0F00); */
	/*tmp = htons(tmp);*/

	tmp = 0x1 << 8; /*RD flag set for better functionality */
	memcpy(buf + offset, &tmp, 2);
	offset += 2;

	/*line3*/
	tmp = htons(q->qcount);
	memcpy(buf + offset, &tmp, 2);
	offset += 2;

	/*only queries*/
	bzero(buf + offset, 6);
	offset += 6;


	/*append label-formatted qname */
	qname_len = expand_to_qname(q->name, buf + offset);
	offset += qname_len;

	tmp = htons(q->type);
	memcpy(buf + offset, &tmp, 2);
	offset += 2;

	tmp = htons(1); /* QUERY CLASS IN */
	memcpy(buf + offset, &tmp, 2);
	offset += 2;


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
	printf("offset: %u\n", offset);

	memcpy(rr->name, rr_msg, offset);
	/*for (i = 0; i < offset; i++) {
		printf("%02x ", rr->name[i]);
	}*/

	memcpy(&tmps, rr_msg + offset, 2);
	offset += 2;
	rr->type = htons(tmps);

	printf("rr type: %" SCNu16 "\n", rr->type);

	if (!(rr->type == RR_TYPE_A || rr->type == RR_TYPE_AAAA)) return -1;

	memcpy(&tmps, rr_msg + offset, 2);
	offset += 2;
	rr->class = htons(tmps);

	printf("rr class: %" SCNu16 "\n", rr->class);

	memcpy(&tmpl, rr_msg + offset, 4);
	offset += 4;
	rr->ttl = htons(tmpl);

	printf("ttl: %" SCNu16 "\n", rr->ttl);

	memcpy(&tmps, rr_msg + offset, 2);
	offset += 2;
	rr->rdlength = htons(tmps);

	printf("rdlength: %" SCNu16 "\n", rr->rdlength);

	if (rr->type == RR_TYPE_A) {
		/*if (rr->rdlength != 4) return -1;*/
		addrstr = malloc(20*sizeof(char));
		memcpy(&tmpl, rr_msg + offset, 4);
		offset += 4;
		memcpy(&(addr.s_addr), &tmpl, 4);
		rr->addr = inet_ntop(AF_INET, &addr, addrstr, 20);
	} else if (rr->type == RR_TYPE_AAAA) {
		/*if (rr->rdlength != 16) return -1;*/
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
	int i, l;
	uint16_t tmp;
	unsigned int offset = 0;

	memcpy(&tmp, msg, 2);
	offset += 2;
	r->id = ntohs(tmp);
	printf("response id: %" SCNu16 "\n", r->id);
	

	memcpy(&tmp, msg + offset, 2);
	offset += 2;
	r->type = (tmp & 0x8000) >> 15;
	r->opcode = (tmp & 0x7800) >> 11;
	r->flags = (tmp & 0x0780) >> 7;
	r->z = (tmp & 0x0070) >> 4;
	r->rcode = (tmp & 0x000F);

	printf("response code: %" SCNu16 "\n", r->rcode);

	if (r->rcode != 0) return 1;

	memcpy(&tmp, msg + offset, 2);
	offset += 2;
	r->qcount = ntohs(tmp);
	printf("query count: %" SCNu16 "\n", r->qcount);
/*	if (r->qcount > 0) {
		return -1; 
	} */
	memcpy(&tmp, msg + offset, 2);
	offset += 2;
	r->ancount = ntohs(tmp);
	printf("answer count: %" SCNu16 "\n", r->ancount);
	memcpy(&tmp, msg + offset, 2);
	offset += 2;
	r->nscount = ntohs(tmp);
	memcpy(&tmp, msg + offset, 2);
	offset += 2;
	r->atcount = ntohs(tmp);

	for (i = 0; i < r->qcount; i++) {

		l = qnamelen(msg + offset);
		printf("response qnamelen: %d\n", l);
		offset += l + 4;
	}

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