#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/file.h>

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
//         char bufa[512], bufb[512], scheme[16], host[256], *next;
//         int ret;

//         if ((ret = sscanf(uri, "%64[^:]://%512s", scheme, bufa)) < 1) {
//                 return -1;
//         } else if (ret < 2) next = uri;
//         else next = bufa;

//         printf("parsing: %s\n", next);

//         if ((ret = sscanf(next, "%512[^/]/%256s", bufb, path)) < 1){
//                 return -1;
//         } else if (ret < 2) {
//         	strcpy(path, next);
//         	return 0;
//         }

//         strcpy(host, bufb);

//         return 0;

// }

char *
parse_uri(char *uri)
{
	char	*ptr1, *path;


	if ((ptr1 = strchr(uri, '/')) == NULL) {
		syslog(LOG_INFO, "Invalid path");
		return NULL;
	}
	path = ptr1+1;

	return path;
}



/* Generate a HTTP request struct in req */
void
generate_response(my_cli *cli, response_type type, char* content_type ,unsigned long content_length)
{

	http_response 	*res;

	fprintf(stdout, "Generating response\n");

	res = cli->res;
	/* Fill the fields */
	res->type = type;

	res->content_type[0] = '\0';
	strncat(res->content_type, content_type, MAX_FIELDLEN-1);



	res->payload_len = content_length;


}

int
files_setup(my_cli *cli)
{
	int 		flags;
	char 		*path;
	struct stat 	file_info;
	http_request *req;



	req = cli->req;

	path = parse_uri(req->uri);

	printf("Setting up file %s\n", path);
	
	flags = req->type == REQ_PUT ? O_WRONLY | O_NONBLOCK | O_CREAT : O_RDONLY | O_NONBLOCK;

	if ((cli->localfd = open(path, flags)) < 0) {
		if (errno == EACCES) generate_response(cli, 400, "", 0);
		else if (errno == ENOENT) generate_response(cli, 404, "", 0);
		else generate_response(cli, 500, "", 0);
		cli->state = ST_RESPOND;
		return -1;
	}

	if (flock(cli->localfd, LOCK_EX | LOCK_NB) < 0) {
		if (errno == EWOULDBLOCK) {
			generate_response(cli, 409, "", 0);
			cli->state = ST_RESPOND;		
		}
		close(cli->localfd);
		return -1;
	}


	if (req->type == REQ_GET) {
		flock(cli->localfd, LOCK_UN);
		fstat(cli->localfd, &file_info);
		generate_response(cli, 200, "text/plain", (unsigned long)file_info.st_size);
	}

	cli->state = req->type == REQ_PUT ? ST_PUT : ST_RESPOND;
	return 0;

}

/* Parse the first line of a HTTP response (store the response type)*/
int
parse_request_startline(char *line, http_request *req)
{
	char 	type[10], uri[64], http_ver[10];

	printf("Parsing startline: %s", line);

	if (sscanf(line, "%s %s %s", type, uri, http_ver) != 3) {
		fprintf(stderr, "Malformed startline!\n");
		return -1;
	}

	if (strncasecmp("GET", type, 3) == 0) req->type = REQ_GET;
	else if (strncasecmp("PUT", type, 3) == 0) req->type = REQ_PUT;
	else return -1;

	strncpy(req->uri, uri, 64);

	return 0;
}

/* Parse a single line of a HTTP header (ends with "\r\n\0") */
int
parse_request_headerline(char *line, http_request *req)
{
/*	size_t	len; */
	char		field[MAX_FIELDLEN], value[MAX_FIELDLEN];
	long int 	len;


	printf("Parsing headerline: %s\n", line);
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
	unsigned int	n;
	int				first, fail, ret;
	http_request 	*req;

	printf("Parsing request\n");
	first = strlen(cli->req->uri) > 0 ? 0 : 1;
	fail = 0;

	req = cli->req;


	
	/* Start loop */
	while(1) {
		n = strlen(cli->linebuf);
		do {
			/* Read socket buffer one byte at a time until CRLF (end of line) */
			if (readn_buf(cli->buf, cli->fd, cli->linebuf + n, 1) < 0) {
				fprintf(stderr, "Error reading from socket: %s\n", strerror(errno));
				if (errno == EWOULDBLOCK) {
					cli->linebuf[n+1] = '\0';
					return EXIT_SUCCESS;	
				} 
				return EXIT_FAILURE;
			}
			//printf("%s\n", cli->linebuf); 
			n++;
		} while (n < MAX_FIELDLEN-1 && (n < 2 || strncmp(CRLF, cli->linebuf + n - 2, 2) != 0));
			/* Terminate line */
			(cli->linebuf)[n] = '\0';
		if (first) {
			/* If first line, parse accordingly */
			if (parse_request_startline(cli->linebuf, req) < 0) {
				fail = 1;
				break;
			}
			first = 0;
			/* Else parse as headerline, return value from parser means end of header */
		} else if ((ret = parse_request_headerline(cli->linebuf, req)) == 1) break;
		else if (ret < 0) {
			fail = 1;
			break;
		}

		(cli->linebuf)[0] = '\0';
	}

	if (fail) {
		generate_response(cli, 400, "", 0);
		cli->state = ST_RESPOND;
		return EXIT_FAILURE;
	}

	printf("Parsing complete\n");
	cli->state = ST_SETUP;

	return EXIT_SUCCESS;
}

/* Write response payload into local file stream from socket */
int
sock_to_file(my_cli *cli)
{
	size_t	remaining;
	int		rbytes, wbytes;

	remaining =  cli->req->payload_len - cli->written_bytes;

	if (remaining > 0) {
		if (cli->written_bytes == cli->read_bytes) {
			if ((rbytes = readn_buf(cli->buf, cli->fd, cli->filebuf, remaining < BUF_SIZE ? remaining : BUF_SIZE)) < 0) {
				if (errno != EWOULDBLOCK) {
					generate_response(cli, 500, "", 0);
					cli->state = ST_RESPOND;
					return -1;
				}
				return 0;
			}
			cli->read_bytes += rbytes;
		}
		else rbytes = cli->read_bytes - cli->written_bytes;

		printf("writing %d bytes: %s\n", rbytes, cli->filebuf);

		if ((wbytes = write(cli->localfd, cli->filebuf, rbytes)) < rbytes) {
			memmove(cli->filebuf, cli->filebuf + wbytes, rbytes - wbytes);
		}

		if (wbytes > 0) {
			cli->written_bytes += wbytes;
			remaining -= wbytes;
		}
	} 
	if (remaining == 0) {
		flock(cli->localfd, LOCK_UN);
		close(cli->localfd);
		generate_response(cli, CREATED, "", 0);
		cli->state = ST_RESPOND;
		
	}

	
	
	return EXIT_SUCCESS;
}

int
file_to_sock(my_cli *cli)
{
	size_t	remaining;
	int		rbytes, wbytes;



	printf("Sending file as payload\n");

	remaining =  cli->res->payload_len - cli->written_bytes;

	printf("remaining: %zd\n", remaining);

	if (remaining > 0) {
		if (cli->written_bytes == cli->read_bytes) {
			if ((rbytes = read(cli->localfd, cli->filebuf, remaining < BUF_SIZE ? remaining : BUF_SIZE)) < 0) {
				printf("read failed\n");
				if (errno != EWOULDBLOCK) {
					generate_response(cli, 500, "", 0);
					cli->state = ST_RESPOND;
					return -1;

				}
				
				return 0;

			}
			cli->read_bytes += rbytes;
		}
		else rbytes = cli->read_bytes - cli->written_bytes;

		printf("writing %d bytes: %s\n", rbytes, cli->filebuf);

		if ((wbytes = write(cli->fd, cli->filebuf, rbytes)) < rbytes) {
			memmove(cli->filebuf, cli->filebuf + wbytes, rbytes - wbytes);
		}

		if (wbytes > 0) {
			cli->written_bytes += wbytes;
			remaining -= wbytes;
		}
	} 
	if (remaining == 0) {
		cli->state = ST_FINISHED;
	}
	
	return EXIT_SUCCESS;
}

/* Create header string and send (with possible payload) to server socket */
int
send_response(my_cli *cli)
{
	char 	header[HEADER_BUF], rtype[32];
	http_response 	*res;

	printf("Sending responset\n");
	res = cli->res;

	switch (res->type) {
		case OK:
		strcpy(rtype, "OK");
		break;
		case NFOUND:
		strcpy(rtype, "Not Found");
		break;
		case CREATED:
		strcpy(rtype, "Created");
		break;
		case BAD_REQUEST:
		strcpy(rtype, "Bad Request");
		break;
		case CONFLICT:
		strcpy(rtype, "Conflict");
		break;
		case SERV_ERR:
		strcpy(rtype, "Internal Server Error");
		break;
		default:
		break;

	}

	/* Format header string */
	sprintf(header, STARTLINEFMT, res->type, rtype);

	if (res->payload_len > 0) {
		sprintf(header + strlen(header), IHEADERFMT, "Content-Length", res->payload_len);
	}
	if (strlen(res->content_type) > 0) {
		sprintf(header + strlen(header), SHEADERFMT, "Content-Type", res->content_type);
	}
	sprintf(header + strlen(header), CRLF);

	/* Informative stdout print */
	fprintf(stdout, "Request header:\n---\n%s---\n", header);

	/* Write header to socket */
	if (write(cli->fd, header, strlen(header)) < 0){
		if (errno != EWOULDBLOCK) {
			cli->state = ST_FINISHED;
		}	
		return EXIT_FAILURE;
	}
	

	if (cli->req->type == REQ_GET && res->type == OK) cli->state = ST_GET;
	else cli->state = ST_FINISHED;

	return EXIT_SUCCESS;

}