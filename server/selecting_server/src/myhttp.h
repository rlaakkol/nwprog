#ifndef MYHTTP_H
#define MYHTTP_H

#include <stdio.h>

#include "mysockio.h"


#define MAX_FIELDLEN 1024

#define HEADER_BUF 10*1024
#define CRLF "\r\n"

/* sprintf format helpers */
#define STARTLINEFMT "HTTP/1.1 %d %s\r\n"
#define SHEADERFMT "%s: %s\r\n"
#define IHEADERFMT "%s: %ld\r\n"

/* HTTP type code definitions */
#define OK 200
#define NFOUND 404
#define CREATED 201
#define BAD_REQUEST 400
#define CONFLICT 409
#define SERV_ERR 500



typedef enum {
	ST_INIT,
	ST_SETUP,
	ST_PUT,
	ST_GET,
	ST_RESPOND,
	ST_FINISHED
} cli_state;

typedef int response_type;

/* Request type enum */
typedef enum {
	REQ_GET,
	REQ_PUT,
	REQ_OTHER
} request_type;

/* Response struct */
typedef struct {
	response_type	type;
	char			content_type[MAX_FIELDLEN];
	unsigned long	payload_len;
} http_response;

/* Requst struct */
typedef struct {
	request_type	type;
	char			uri[MAX_FIELDLEN];
	char			host[MAX_FIELDLEN];
	char			iam[MAX_FIELDLEN];
	unsigned long	payload_len;
	char 			content_type[MAX_FIELDLEN];
	int				close;
} http_request;

typedef struct {
	my_buf 	*buf;
	http_request 	*req;
	http_response 	*res;
	char 	linebuf[MAX_FIELDLEN];
	int 	fd;
	int 	localfd;
	cli_state 	state;
	size_t 	read_bytes;
	size_t 	written_bytes;
	char 	filebuf[BUF_SIZE];

} my_cli;


/* Parse the entire response (using above helper functions) and store values into res */
int
parse_request(my_cli *cli);

/* Generate a HTTP request struct in req */
void
generate_response(my_cli *cli, response_type type, char* content_type ,unsigned long content_length);

/* Write response payload into local file stream from socket */
int
sock_to_file(my_cli *cli);

int
file_to_sock(my_cli *cli);

int
files_setup(my_cli *cli);

/* Create header string and send (with possible payload) to server socket */
int
send_response(my_cli *cli);

#endif