#define MAX_FIELDLEN 50


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
	unsigned long	payload_len;
	unsigned long	read_len;
	char			*payload;
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
generate_request_header(http_request *request, const char *payload_filename, char *buf)