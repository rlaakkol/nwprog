#include "myhttp.h"

#define HEADER_BUF 1000
#define CRLF "\r\n"

void
generate_request(request_type type, const char *uri, const char *host, const char *iam, const char *payload_filename, int close, http_request *req)
{
	struct stat 	file_info;

	req->type = type;
	req->uri[0] = '\0';
	strncat(req->uri, host, MAX_FIELDLEN-1);
	strncat(req->uri, uri, MAX_FIELDLEN-strlen(req->uri)-1);
	req->iam[0] = '\0';
	strncat(req->iam, iam, MAX_FIELDLEN-1);
	req->close = close;

	if (payload_filename != NULL) {
		stat(payload_filename, &file_info);
		req->payload_len = (unsigned long) file_info.st_size;
	} else {
		req->payload_len = 0;
	}

}

int
parse_response_startline(char *line, http_response *res)
{
	if (strcasecmp(line, HTTP_OK) == 0) {
		res->type = OK;
		return 0;
	} else if (strcasecmp(line, HTTP_NFOUND) == 0) {
		res->type = NFOUND;
		return 0;
	} else if (strcasecmp(line, HTTP_CREATED) == 0) {
		res->type = CREATED;
		return 0;
	}
	return -1;
}

int
parse_response_headerline(char *line, http_response *res)
{
	size_t	len;
	char	*ptr1, *ptr2;
	len = 0;
	if (strncmp(line, "\r\n", 2) == 0) return 0;	// End of header
	while(line[len++] != '\n');
	if ((ptr1 = strchr(line, ':')) == NULL) {
//		syslog(LOG_INFO, "Malformed header");
		return -1;
	}
	ptr2 = ptr1 + 1;
	while (*ptr2 == ' ') ptr2++;
	if (strncasecmp(line, "content-length", ptr1 - line) == 0) {
		if ((res->payload_len = atoi(ptr2)) == 0) {
//			syslog(LOG_INFO, "Malformed header: %s: %s", line, ptr2);
			return -1;
		}
//	} else if (strncasecmp(line, "host", ptr1 - line) == 0) {
//		strncpy(res->host, ptr1, line + len - ptr2);
//		res->host[line + len - ptr2 - 2] = '\0';
//	} else if (strncasecmp(line, "connection", ptr1 -line) == 0) {
//		if (strncasecmp(ptr2, "close\r", line + len - ptr2)) res->close = 1;
//	} else if (strncasecmp(line, "iam", ptr1 - line) == 0) {
//		strncpy(res->iam, ptr1, line + len - ptr2);
//		res->iam[line + len - ptr2 - 2] = '\0';
	}
	return 0;
}

void
parse_response(int sock, http_response *res)
{
	char			line[HEADER_BUF];
	unsigned int	n;
	int				first, empty;

	res->fd = sock;
	first = 1;
	empty = 0;
	
	while(1) {
		n = 0;
		do {
			readn_buf(sock, line + n, 1);
			n++;
		} while ( n < 2 || strncmp(CRLF, line[n-2], 2) != 0)
		line[n] = '\0';
		if (strncmp(CRLF, line, HEADER_BUF) == 0) {
			if (empty) break;
			else empty = 1;
		}
		if (first) {
			parse_response_startline(line, res);
			first = 0;
		}
		else parse_headerline(line, res);
	}
	

}