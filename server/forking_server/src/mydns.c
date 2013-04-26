int
expand_to_qname(char *name, void *buf)
{
	char c;
	char *last = name, *first = name;
	unsigned int offset = 0
	uint8_t octets;

	while (1) {
		if (*last == '.' || *last == '\0') {
			octets = last - first;
			memcpy(buf + offset, octets, 1);
			memcpy(buf + offset + 1, first, octets);
			offset += octets + 1;
			first = last + 1;
			if (*last == '\0') {
				memcpy(buf + offset, 0, 1);
				return offset + 1;
			}
		} 
		last++;
	}

	return -1;
}

int
qname_to_str(char *name, void* buf)
{
	unsigned int offset = 0;
	char c, *nptr = name;
	while(1) {
		c = (buf + offset);
		if (c == 0) return offset + 1;
		memcpy(nptr, buf + offset, c);
		nptr += c;
		offset += c + 1;
	}

	return -1;

}

int
generate_query_msg(dns_msg *q, void *buf)
{
	uint16_t tmp;
	unsigned int offset = 0;
	/* header */

	/*id*/
	memcpy(&htons((uint16_t) getpid()), buf, 2)
	offset += 2;
	/*line2*/
	tmp = 0x0000; /* query type */
	tmp |= ((q->opcode << 11) & 0x7800);
	tmp |= (q->flags << 7 & 0x0780);
	memcpy(buf + offset, &tmp, 2)
	offset += 2;
	/*line3*/
	tmp = htons(q->qcount);
	memcpy(buf, &tmp, 2);
	offset += 2;
	/*only queries*/
	bzero(buf + offset, 6);
	offset += 6;

	/*append label-formatted qname */
	offset += expand_to_qname(q->name, buf + offset);
	memcpy(buf + offset, htons(q->type), 2);
	offset += 2;
	memcpy(buf + offset, htons(q->class))
	offset += 2;

	return offset;
}

int
parse_rr(void *rr_msg, dns_rr *rr)
{
	unsigned int offset = 0;
	uint16_t tmps;
	uint32_t tmpl;
	uint8_t tmp[16];
	struct in6_addr addr6;
	struct in_addr addr;

	offset += qname_to_str(rr->name, rr_msg);

	memcpy(&tmps, rr_msg + offset, 2);
	offset += 2;
	rr->type = htons(tmps);

	if (!(rr->type == RR_TYPE_A || rr->type == RR_TYPE_AAAA) return -1;

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
		memcpy(tmpl, rr_msg + offset, 4);
		offset += 4;
		memcpy(in_addr.s_addr, tmpl, 4);
		rr->addr = inet_ntop(AF_INET, addr);
	} else if (rr->type == RR_TYPE_AAAA) {
		if (rr->rdlength != 16) return -1;
		memcpy(tmp, rr_msg + offset, 16);
		offset += 16;
		memcpy(addr6.s6_addr, tmp, 16);
		rr->addr = inet_ntop(AF_INET6, addr6);
	}
	else return -1;

	return offset;

}

int
parse_dns_response(void *msg, dns_msg *r)
{
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

	if (rcode != 0) return 1;

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
	r->arcount = ntohs(tmp);

	for (i = 0; i < r->ancount; i++) {
		offset += parse_rr(msg + offset, (r->rr + i));
	}

	for (i = 0; i < r->nscount; i++) {
		/* TODO */
	}

	for (i = 0; i < r->arcount; i++) {
		/* TODO */
	}

	return 0;

}