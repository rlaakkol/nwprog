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