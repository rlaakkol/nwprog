#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "myhttp.h"
#include "mysockio.h"

void
generate_request(request_type type, const char *uri, const char *host, const char *iam, const char *payload_filename, int close, http_request *req)
{
	struct stat 	file_info;

	req->type = type;
	req->uri[0] = '\0';
	strncat(req->uri, "http://", 7);
	strncat(req->uri, host, MAX_FIELDLEN-1);
	strncat(req->uri, "/", 1);
	strncat(req->uri, uri, MAX_FIELDLEN-strlen(req->uri)-1);
	req->host[0] = '\0';
	strncat(req->host, host, MAX_FIELDLEN-1);
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
	} else if (strcasecmp(line, HTTP_NFOUND) == 0) {
		res->type = NFOUND;
	} else if (strcasecmp(line, HTTP_CREATED) == 0) {
		res->type = CREATED;
	} else return -1;
	return 0;
}

int
parse_response_headerline(char *line, http_response *res)
{
/*	size_t	len; */
	char	*ptr1, *ptr2;

	if (strncmp(line, "\r\n", 2) == 0) return 1;	/* End of header */
/*	len = strlen(line); */
	if ((ptr1 = strchr(line, ':')) == NULL) {
/*		syslog(LOG_INFO, "Malformed header");*/
		return -1;
	}
	ptr2 = ptr1 + 1;
	while (*ptr2 == ' ') ptr2++;
	if (strncasecmp(line, "content-length", ptr1 - line) == 0) {
		if ((res->payload_len = atoi(ptr2)) == 0) {
/*			syslog(LOG_INFO, "Malformed header: %s: %s", line, ptr2);*/
			return -1;
		}
/*	} else if (strncasecmp(line, "host", ptr1 - line) == 0) { 
//		strncpy(res->host, ptr1, line + len - ptr2);
//		res->host[line + len - ptr2 - 2] = '\0';
//	} else if (strncasecmp(line, "connection", ptr1 -line) == 0) {
//		if (strncasecmp(ptr2, "close\r", line + len - ptr2)) res->close = 1;
//	} else if (strncasecmp(line, "iam", ptr1 - line) == 0) {
//		strncpy(res->iam, ptr1, line + len - ptr2);
//		res->iam[line + len - ptr2 - 2] = '\0';*/
	}
	return 0;
}

void
parse_response(int sock, http_response *res)
{
	char			line[HEADER_BUF];
	unsigned int	n;
	int				first;

	res->fd = sock;
	first = 1;
	
	while(1) {
		n = 0;
		do {
			readn_buf(sock, line + n, 1);
			n++;
		} while ( n < 2 || strncmp(CRLF, line + n - 2, 2) != 0);
			line[n] = '\0';
		if (first) {
			parse_response_startline(line, res);
			first = 0;
		} else if (parse_response_headerline(line, res) == 1) break;
	}
	

}

int
store_response_payload(FILE *outfile, http_response *res)
{
	size_t	remaining, next;
	int		rbytes, wbytes;
	char	buf[BUF_SIZE];

	remaining = res->payload_len;

	while (remaining) {
		next = remaining < BUF_SIZE ? remaining : BUF_SIZE;
		rbytes = readn_buf(res->fd, buf, next);
		if (rbytes < 1) {
			return rbytes;
		}
		if ((wbytes = fwrite(buf, 1, rbytes, outfile)) < rbytes) {
			return -1;
		}
		remaining -= rbytes;
	}
	return 0;
}

int
send_request(int fd, http_request *req, FILE *payload)
{
	char 	header[HEADER_BUF], *rtype;

	rtype = malloc(4);

	if (req->type == GET) {
		strncpy(rtype, "GET", 4);
	} else {
		strncpy(rtype, "PUT", 4);
	}
	header[0] = '\0';

	sprintf(header, STARTLINEFMT, rtype, req->uri);
	sprintf(header + strlen(header), SHEADERFMT, "Host", req->host);
	sprintf(header + strlen(header), SHEADERFMT, "Iam", req->iam);

	if (req->payload_len > 0) {
		sprintf(header + strlen(header), IHEADERFMT, "Content-Length", req->payload_len);
	}
	sprintf(header + strlen(header), CRLF);

	writen(fd, header, strlen(header));
	if (req->payload_len > 0) {
		write_file(fd, payload, req->payload_len);
	}

	return 0;

}