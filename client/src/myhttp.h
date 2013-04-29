#include <stdio.h>

#define MAX_FIELDLEN 1024

#define HEADER_BUF 10*1024
#define CRLF "\r\n"

/* sprintf format helpers */
#define STARTLINEFMT "%s %s HTTP/1.1\r\n"
#define SHEADERFMT "%s: %s\r\n"
#define IHEADERFMT "%s: %ld\r\n"
#define POSTQFMT "Name=%s&Type=%s"

/* HTTP type code definitions */
#define OK 200
#define NFOUND 404
#define CREATED 201
#define OTHER -1

typedef int response_type;

/* Request type enum */
typedef enum {
	GET,
	PUT,
	POST
} request_type;

/* Response struct */
typedef struct {
	int				fd;
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
	char 			post_payload[MAX_FIELDLEN];
} http_request;

/* Return HTTP response type as a number */
int
restype_to_int(http_response *res);

/* Parse the entire response (using above helper functions) and store values into res */
int
parse_response(int sock, http_response *res);

/* Generate a HTTP request struct in req */
void
generate_request(request_type type, const char *uri, const char *host, const char *iam, const char *payload_filename, const char *post_name, const char *post_type, int close, const char *content_type, http_request *req);

/* Write response payload into local file stream from socket */
int
store_response_payload(FILE* outfile, http_response *res, size_t *remaining);

/* Create header string and send (with possible payload) to server socket */
int
send_request(int fd, http_request *req, FILE *payload);