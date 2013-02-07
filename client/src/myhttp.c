#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

#include "myhttp.h"
#include "mysockio.h"

/* Return HTTP response type as a number */
int
restype_to_int(http_response *res)
{
	return res->type;
}

/* Generate a HTTP request struct in req */
void
generate_request(request_type type, const char *uri, const char *host, const char *iam, const char *payload_filename, int close, const char *content_type, http_request *req)
{
	struct stat 	file_info;

	fprintf(stdout, "Generating %s-request\n", type == GET ? "GET" : "PUT");

	/* Fill the fields */
	req->type = type;
	req->uri[0] = '\0';
	strncat(req->uri, "/", 1);
	strncat(req->uri, uri, MAX_FIELDLEN-strlen(req->uri)-1);
	req->host[0] = '\0';
	strncat(req->host, host, MAX_FIELDLEN-1);
	req->iam[0] = '\0';
	strncat(req->iam, iam, MAX_FIELDLEN-1);
	req->close = close;
	req->content_type[0] = '\0';
	if (req->type == PUT && req->content_type != NULL) {
		strncat(req->content_type, content_type, MAX_FIELDLEN-1);
	}

	if (payload_filename != NULL) {
		/* If there is payload in the request, count the size */
		stat(payload_filename, &file_info);
		req->payload_len = (unsigned long) file_info.st_size;
	} else {
		req->payload_len = 0;
	}

}

/* Parse the first line of a HTTP response (store the response type)*/
int
parse_response_startline(char *line, http_response *res)
{
	int 	code;
	char 	http_ver[10];

	if (sscanf(line, "%8s %d", http_ver, &code) != 2) {
		fprintf(stderr, "Malformed response startline!\n");
		res->type = OTHER;
		return -1;
	}

	res->type = code;

	return 0;
}

/* Parse a single line of a HTTP header (ends with "\r\n\0") */
int
parse_response_headerline(char *line, http_response *res)
{
/*	size_t	len; */
	char		field[MAX_FIELDLEN], value[MAX_FIELDLEN];
	long int 	len;



	if (strncmp(line, "\r\n", 2) == 0) return 1;	/* End of header */
	/* Store fieldame and value strings into different buffers */
	sscanf(line, "%[^:]:%s", field, value);


	if (strcasecmp(field, "content-length") == 0) {
		/* Store content length */
		if ((len = atol(value)) < 0) {
			fprintf(stderr, "Received malformed \"%s\" header line:\n%s", field, line);

			return -1;
		} else {
			res->payload_len = len;
			fprintf(stdout, "Content-Length: %ld\n", len);
		}
	} else if (strcasecmp(field, "content-type") == 0) {
		/* Store content type */
		strcpy(res->content_type, value);
	}


	return 0;
}

/* Parse the entire response (using above helper functions) and store values into res */
int
parse_response(int sock, http_response *res)
{
	char			line[MAX_FIELDLEN];
	unsigned int	n;
	int				first, ret;

	printf("Parsing response\n");
	res->fd = sock;
	first = 1;
	
	/* Start loop */
	while(1) {
		n = 0;
		do {
			/* Read socket one byte at a time until CRLF (end of line) */
			if (readn(sock, line + n, 1) < 0) {
				fprintf(stderr, "Error reading from socket: %s\n", strerror(errno));
				return EXIT_FAILURE;
			}
			n++;
		} while (n < MAX_FIELDLEN-1 && (n < 2 || strncmp(CRLF, line + n - 2, 2) != 0));
			/* Terminate line */
			line[n] = '\0';
		if (first) {
			/* If first line, parse accordingly */
			if (parse_response_startline(line, res) < 0) return EXIT_FAILURE;
			first = 0;
			/* Else parse as headerline, return value from parser means end of header */
		} else if ((ret = parse_response_headerline(line, res)) == 1) break;
		else if (ret < 0) return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

/* Write response payload into local file stream from socket */
int
store_response_payload(FILE *outfile, http_response *res, size_t *remaining)
{
	size_t	next;
	int		rbytes, wbytes;
	char	buf[BUF_SIZE];


	*remaining = res->payload_len;

	while (*remaining) {
		/* Calculate maximum read amount and read from socket */
		next = *remaining < BUF_SIZE ? *remaining : BUF_SIZE;
		rbytes = readn(res->fd, buf, next);
		if (rbytes < 1) {
			/* If read returns 0 or less, return */
			return rbytes;
		}
		/* Write into file stream */
		if ((wbytes = fwrite(buf, 1, rbytes, outfile)) < rbytes) {
			fprintf(stderr, "Error writing local file: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}
		*remaining -= rbytes;
	}
	return EXIT_SUCCESS;
}

/* Create header string and send (with possible payload) to server socket */
int
send_request(int fd, http_request *req, FILE *payload)
{
	char 	header[HEADER_BUF], *rtype;

	fprintf(stdout, "Sending request\n");

	rtype = malloc(4);

	if (req->type == GET) {
		strncpy(rtype, "GET", 4);
	} else {
		strncpy(rtype, "PUT", 4);
	}
	header[0] = '\0';

	/* Format header string */
	sprintf(header, STARTLINEFMT, rtype, req->uri);
	sprintf(header + strlen(header), SHEADERFMT, "Host", req->host);
	if (strcasecmp(req->iam, "none") != 0) {
		sprintf(header + strlen(header), SHEADERFMT, "Iam", req->iam);
	}
	if (req->payload_len > 0) {
		sprintf(header + strlen(header), IHEADERFMT, "Content-Length", req->payload_len);
	}
	if (strlen(req->content_type) > 0) {
		sprintf(header + strlen(header), SHEADERFMT, "Content-Type", req->content_type);
	}
	sprintf(header + strlen(header), CRLF);

	/* Informative stdout print */
	fprintf(stdout, "Request header:\n---\n%s---\n", header);

	/* Write header to socket */
	writen(fd, header, strlen(header));
	if (req->payload_len > 0) {
		/* If there is payload, write it too */
		if (write_file(fd, payload, req->payload_len) > 0) {
			fprintf(stderr, "Error sending message payload: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}
	}

	free(rtype);
	return EXIT_SUCCESS;

}