#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <syslog.h>
#include <dirent.h>
#include <sys/stat.h>

#include "myhttp.h"
#include "mysockio.h"



/* Parse startline of HTTP request
 * */
int parse_startline(char *line, Http_info *specs)
{
	char	*ptr1, *ptr2;

	ptr1 = strchr(line, ' ');
	if (ptr1 == NULL) {
		syslog(LOG_INFO, "Malformed request line");
		return -1;
	}
	if (!strncasecmp(line, "GET", ptr1 - line)) specs->command = GET;
	else if (!strncasecmp(line, "PUT", ptr1 - line)) specs->command = PUT;
	else if (!strncasecmp(line, "POST", ptr1 - line)) specs->command = POST;
	else {
		syslog(LOG_INFO, "Unsupported HTTP request");
		return -1;
	}
	ptr2 = strchr(ptr1+1, ' ');
	if (ptr2 == NULL) {
		syslog(LOG_INFO, "Malformed request line");
		return -1;
	}
	strncpy(specs->uri, ptr1+1, ptr2 - (ptr1+1));
	specs->uri[ptr2 - (ptr1+1)] = '\0';
	syslog(LOG_INFO, "request uri: %s", specs->uri);
	return 0;
}

/* Parse a single header line
 *
 * */
int parse_header(char *line, Http_info *specs)
{
	size_t	len;
	char	*ptr1, *ptr2;
	len = 0;
	if (strncmp(line, "\r\n", 2) == 0) return 0;	// End of header
	while(line[len++] != '\n');
	if ((ptr1 = strchr(line, ':')) == NULL) {
		syslog(LOG_INFO, "Malformed header");
		return -1;
	}
	ptr2 = ptr1 + 1;
	while (*ptr2 == ' ') ptr2++;
	if (strncasecmp(line, "host", ptr1 - line) == 0) {
		strncpy(specs->host, ptr1, line + len - ptr2);
		specs->host[line + len - ptr2 - 2] = '\0';
	} else if (strncasecmp(line, "connection", ptr1 -line) == 0) {
		if (strncasecmp(ptr2, "close\r", line + len - ptr2)) specs->close = 1;
	} else if (strncasecmp(line, "iam", ptr1 - line) == 0) {
		strncpy(specs->iam, ptr1, line + len - ptr2);
		specs->iam[line + len - ptr2 - 2] = '\0';
	} else if (strncasecmp(line, "content-length", ptr1 - line) == 0) {
		if ((specs->length = atoi(ptr2)) == 0) {
			syslog(LOG_INFO, "Malformed header: %s: %s", line, ptr2);
			return -1;
		}
	}
	return 1;

}

char *parse_uri(char *uri)
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

int
parse_request(int sock, Http_info *req)
{
	char			line[MAX_FIELDLEN];
	unsigned int	n;
	int				first, ret;

	printf("Parsing response\n");
	req->fd = sock;
	first = 1;

	buf_init();
	
	/* Start loop */
	while(1) {
		n = 0;
		do {
			/* Read socket one byte at a time until CRLF (end of line) */
			if (readn_buf(sock, line + n, 1) < 0) {
				fprintf(stderr, "Error reading from socket: %s\n", strerror(errno));
				return EXIT_FAILURE;
			}
/*			printf("%c", line[n]); */
			n++;
		} while (n < MAX_FIELDLEN-1 && (n < 2 || strncmp(CRLF, line + n - 2, 2) != 0));
			/* Terminate line */
			line[n] = '\0';
		if (first) {
			/* If first line, parse accordingly */
			if (parse_startline(line, req) < 0) return EXIT_FAILURE;
			first = 0;
			/* Else parse as headerline, return value from parser means end of header */
		} else if ((ret = parse_response_headerline(line, req)) == 1) break;
		else if (ret < 0) return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int
store_request_payload(int sockfd, FILE *outfile, Http_info *req, size_t *remaining)
{
	size_t	next;
	int		rbytes, wbytes;
	char	buf[BUF_SIZE];


	*remaining = req->length;

	while (*remaining) {
		/* Calculate maximum read amount and read from socket */
		next = *remaining < BUF_SIZE ? *remaining : BUF_SIZE;
		rbytes = readn_buf(sockfd, buf, next);
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

int process_put(int sockfd, Http_info *specs)
{
	int	new;
	FILE	*outfile;
	ssize_t	rem, readlen, n;
	char	*path, buf[MAXLINE], *reply;
	struct stat	fs;
	
	new = 0;
	path = parse_uri(specs->uri);
	if (stat(path, &fs) < 0) {
		if (errno == ENOENT) {
			new = 1;
			syslog(LOG_INFO, "new file");
		}else {
			syslog(LOG_INFO, "invalid path: %s", path);
			writen(sockfd, REPLY_400, strlen(REPLY_400));
			exit(-1);
		}
	}
	syslog(LOG_INFO, "writing file %s of size %zi", path, (ssize_t) specs->length);
	if ((outfile = fopen(path, "w")) == NULL) {
		syslog(LOG_INFO, "invalid path: %s", path);
		writen(sockfd, REPLY_400, strlen(REPLY_400));
		exit(-1);
	}
	if (specs->length == 0) {
		if (unlink(path) < 0) {
			syslog(LOG_INFO, "error removing %s", path);
			exit(0);
		}
		return 0;
	}



	if (fwrite(oldbuf, sizeof(char), oldrem, outfile) < (size_t) oldrem) {
		syslog(LOG_ERR, "fwrite error\n");
		return EXIT_FAILURE;
	}
	
	
	/*rem = specs->length - oldrem;
	n = 0;
	while(rem > 0) {
		readlen = MAXLINE < rem ? MAXLINE : rem;
		syslog(LOG_INFO, "reading %zi bytes", readlen);
		if ((n = read(sockfd, buf, readlen)) <= 0) {
			syslog(LOG_INFO, "read error");
			return -1;
		}
		buf[n] = '\0';
		if (fputs(buf, outfile) == EOF) {
			syslog(LOG_ERR, "fputs error\n");
			return EXIT_FAILURE;
		}
		rem -= n;
	}
	
	}*/

	store_request_payload(sockfd, outfile, specs, &rem);

	if (rem > 0) {
		syslog(LOG_INFO, "connection lost");
		return -1;
	}
	fclose(outfile);
	reply = new ? REPLY_201 : REPLY_200;
	if (writen(sockfd, reply, strlen(reply)) < (ssize_t) strlen(reply)) {
		syslog(LOG_INFO, "error sending response");
		return -1;
	


	return 0;
}


int process_get(int sockfd, Http_info *specs)
{
	struct stat	fs;
	char		*path, buf[MAXN], *temp, ptemp[80];
	FILE		*f;
	ssize_t		l, rem, ret, dl;
	DIR		*d;
	struct dirent	*de;
	

	path = parse_uri(specs->uri);
	if (strcasecmp(path, "index") == 0) {
		path = getcwd(ptemp, 80);
		if ((d = opendir(path)) == NULL) {
			syslog(LOG_INFO, "error reading working directory: %s", path);
			writen(sockfd, REPLY_404, strlen(REPLY_404));
			exit(-1);
		}
		dl = 0;
		while ((de = readdir(d)) != NULL) {
			strcpy(buf+dl, de->d_name);
			dl += strlen(de->d_name) + 1;
			buf[dl-1] = '\n';
		}
		buf[dl] = '\0';
		temp = malloc(dl*sizeof(char));
		strcpy(temp, buf);
		sprintf(buf, REPLY_200F, dl-1, "text/plain");
		if (writen(sockfd, buf, strlen(buf)) < (ssize_t) strlen(buf)) {
			syslog(LOG_INFO, "index write error 1");
			exit(-1);
		}
		if (writen(sockfd, temp, dl-1) < dl-1) {
			syslog(LOG_INFO, "index write error 1");
			exit(-1);
		}
		free(temp);
		if (specs->close) close(sockfd);
		return 0;

	}

	if (stat(path, &fs) < 0) {
		syslog(LOG_INFO, "invalid path: %s", path);
		writen(sockfd, REPLY_404, strlen(REPLY_404));
		exit(-1);
	}
	if ((f = fopen(path, "r")) == NULL) {
		syslog(LOG_INFO, "invalid path: %s", path);
		writen(sockfd, REPLY_404, strlen(REPLY_404));
		exit(-1);
	}

	snprintf(buf, MAXLINE, REPLY_200F, (ssize_t) fs.st_size, "text/plain");
	l = strlen(buf);
	if (writen(sockfd, buf, l) < l) {
		syslog(LOG_INFO, "write error 1");
		exit(-1);
	}
	
	rem = fs.st_size;
	
	while (rem > 0 && (ret = fread(buf, sizeof(char), MAXN, f)) > 0) {
		if (writen(sockfd, buf, ret) < ret) {
			syslog(LOG_INFO, "write error 2");
			exit(-1);
		}
		rem -= ret;
	}
	if (rem > 0) {
		syslog(LOG_INFO, "write error 3");
		exit(-1);
	}
	if (specs->close) close(sockfd);

	fclose(f);





	return 0;
}

int
parse_post_argument(const char *line, Http_info *specs)
{
	char		field[MAX_FIELDLEN], value[MAX_FIELDLEN];

	sscanf(line, "%[^=]:%s", field, value);

	if (strncasecmp(field, "Type", 4) == 0) {
		strcpy(specs->post_type, value);
	} else if (strncasecmp(field, "Name", 4) == 0) {
		strcpy(specs->post_name, value);
	}

}

int
parse_post_payload(int sockfd, Http_info *specs) 
{
	char line[MAXLINE];
	int n, m = 0;

	while(m < specs->length) {
		n = 0;
		do {
			/* Read socket (buffer) one byte at a time until CRLF (end of line) */
			if (readn_buf(sockfd, line + n, 1) < 0) {
				fprintf(stderr, "Error reading from socket: %s\n", strerror(errno));
				return EXIT_FAILURE;
			}
/*			printf("%c", line[n]); */
			n++;
			m++;
		} while (m < specs->length && *(line + n - 1) != '&'));
			/* Terminate line */
		line[n-1] = '\0';

		parse_post_argument(line, specs)
		
		else if (ret < 0) return EXIT_FAILURE;
	}

}

int
process_post(int sockfd, Http_info *specs)
{
	int udpsock;
	char *server, *uri;

	uri = parse_uri(specs->uri)

	if (!strncmp(uri, "dns-query", 9)) {
		writen(sockfd, REPLY_404, strlen(REPLY_404));
		return -1;
	}

	parse_post_payload(sockfd, specs)

	/* Connect to dns server */
	udpsock = connect(server, "domain", SOCK_DGRAM);






	return 0;
}