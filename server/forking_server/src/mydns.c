void
generate_query_msg(dns_msg *q, uint16_t *lines)
{
	/* header */

	/*id*/
	bytes[0] = (char) rand();
	/*line2*/
	bytes[1] = ((q->opcode << OPCODE_SHIFT) & OPCODE_MASK);
	bytes[1] |= (q->flags & FLAGS_MASK);
	bytes[1] &= 0xFF00;
	/*line3*/
	bytes[2] = (uint16_t) q->count;

	bytes[3] = bytes[4] = bytes[5] = (uint16_t) 0;


}