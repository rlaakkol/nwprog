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

// int
// parse_uri(char *uri, char *path)
// {
//         char bufa[512], bufb[512], scheme[16], host[256];
//         int ret;

//         if (sscanf(uri, "%64[^:]://%512s", scheme, bufa) < 1) {
//                 return -1;
//         }

//         if ((ret = sscanf(bufa, "%512[^/]/%256s", bufb, path)) < 1){
//                 return -1;
//         } else if (ret < 2) return 0;

//         strcpy(host, bufb)

//         return 0;

// }

char *
parse_uri(char *uri)
{
	char	*ptr1, *path;

	if (strlen(uri) > 6 && (!strncmp(uri, "http://", 7) || !strncmp(uri, "https://", 8))) {
		if ((ptr1 = strchr(uri, '/')) == NULL) {
			syslog(LOG_INFO, "Invalid path");
			return NULL;
		}
		path = ptr1+1;
	}
	else path = uri+1;
	return path;
}



/* Generate a HTTP request struct in req */
void
generate_response(my_cli *cli, response_type type, content_length)
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

int
files_setup(my_cli *cli)
{
	http_request *req;

	req = cli->req;


	cli->state = req->type == PUT ? PUT : GET;
	return 0;

}

/* Parse the first line of a HTTP response (store the response type)*/
int
parse_request_startline(char *line, http_request *req)
{
	char 	type[10], uri[64], http_ver[10];

	if (sscanf(line, "%s %s %s", type, uri, http_ver) != 3) {
		fprintf(stderr, "Malformed startline!\n");
		res->type = OTHER;
		return -1;
	}

	if (strncasecmp("GET", type, 3) == 0) req->type = GET;
	else if (strncasecmp("PUT", type, 3) == 0) req->type = PUT;
	else return -1;

	return 0;
}

/* Parse a single line of a HTTP header (ends with "\r\n\0") */
int
parse_request_headerline(char *line, http_request *req)
{
/*	size_t	len; */
	char		field[MAX_FIELDLEN], value[MAX_FIELDLEN];
	long int 	len;



	if (strncmp(line, "\r\n", 2) == 0) return 1;	/* End of header */
	/* Store fieldame and value strings into different buffers */
	
	if (sscanf(line, "%[^:]:%s", field, value) != 2) return -1;


	if (strcasecmp(field, "content-length") == 0) {
		/* Store content length */
		if ((len = atol(value)) < 0) {
			fprintf(stderr, "Received malformed \"%s\" header line:\n%s", field, line);

			return -1;
		} else {
			req->payload_len = len;
			fprintf(stdout, "Content-Length: %ld\n", len);
		}
	} else if (strcasecmp(field, "content-type") == 0) {
		/* Store content type */
		strcpy(req->content_type, value);
	} else if (strcasecmp(field, "iam") == 0) {

		strcpy(req->iam, value);
	}


	return 0;
}

/* Parse the entire response (using above helper functions) and store values into res */
int
parse_request(my_cli *cli)
{
	char			line[MAX_FIELDLEN];
	unsigned int	n;
	int				first, ret;

	printf("Parsing request\n");
	first = strlen(cli->req->uri) > 0 ? 0 : 1;


	
	/* Start loop */
	while(1) {
		n = strlen(cli->linebuf);
		do {
			/* Read socket one byte at a time until CRLF (end of line) */
			if (readn_buf(sock, cli->linebuf + n, 1) < 0) {
				fprintf(stderr, "Error reading from socket: %s\n", strerror(errno));
				return EXIT_FAILURE;
			}
/*			printf("%c", line[n]); */
			n++;
		} while (n < MAX_FIELDLEN-1 && (n < 2 || strncmp(CRLF, cli->linebuf + n - 2, 2) != 0));
			/* Terminate line */
			(cli->linebuf)[n] = '\0';
		if (first) {
			/* If first line, parse accordingly */
			if (parse_request_startline(cli->linebuf, res) < 0) return EXIT_FAILURE;
			first = 0;
			/* Else parse as headerline, return value from parser means end of header */
		} else if ((ret = parse_request_headerline(cli->linebuf, res)) == 1) break;
		else if (ret < 0) return EXIT_FAILURE;

		strcpy("\0", cli->linebuf);
	}

	cli->state = SETUP;

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
		rbytes = readn_buf(res->fd, buf, next);
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