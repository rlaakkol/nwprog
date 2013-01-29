#include "myhttp.h"

#define HEADER_BUF 1000

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

void
parse_response(int sock, http_response *res)
{
	char			*buf;
	unsigned int	n;

	n = 0;
	buf = malloc(HEADER_BUF);
	

}