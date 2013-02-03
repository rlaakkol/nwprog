#include <stdio.h>

#define MAX_FIELDLEN 64

#define HEADER_BUF 1024
#define CRLF "\r\n"

#define STARTLINEFMT "%s %s HTTP/1.1\r\n"
#define SHEADERFMT "%s: %s\r\n"
#define IHEADERFMT "%s: %ld\r\n"

#define HTTP_OK "HTTP/1.1 200 OK\r\n"
#define HTTP_NFOUND "HTTP/1.1 404 Not Found\r\n"
#define HTTP_CREATED "HTTP/1.1 201 Created\r\n"

typedef enum {
	OK,
	CREATED,
	NFOUND
} response_type;

typedef enum {
	GET,
	PUT
} request_type;

typedef struct {
	int				fd;
	response_type	type;
	char			content_type[MAX_FIELDLEN];
	unsigned long	payload_len;
} http_response;

typedef struct {
	request_type	type;
	char			uri[MAX_FIELDLEN];
	char			host[MAX_FIELDLEN];
	char			iam[MAX_FIELDLEN];
	unsigned long	payload_len;
	int				close;
} http_request;

void
parse_response(int sock, http_response *res);

void
generate_request(request_type type, const char *uri, const char *host, const char *iam, const char *payload_filename, int close, http_request *req);

void
generate_request_header(http_request *request, const char *payload_filename, char *buf);

int
store_response_payload(FILE* outfile, http_response *res);

int
send_request(int fd, http_request *req, FILE *payload);