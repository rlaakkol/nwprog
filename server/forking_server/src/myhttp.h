#ifndef MYHTTP_H
#define MYHTTP_H

#include <sys/types.h>
typedef struct 
{
	enum http_comm {GET, PUT}	command;
	char				uri[80];
	char				host[80];
	int				close;
	char				iam[80];
	ssize_t				length;
} Http_info;

int parse_startline(char *line, Http_info *specs);
int parse_header(char *line, Http_info *specs);
int process_get(int sockfd, Http_info *specs);
int process_put(int sockfd, Http_info *specs, char *buf, ssize_t rem);



#include "npbserver.h"

#define REPLY_404 "HTTP/1.1 404 Not Found\r\nContent-Length: 3\r\n\r\n404\0"
#define REPLY_200 "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\n200\0"
#define REPLY_201 "HTTP/1.1 201 Created\r\nContent-Length: 3\r\n\r\n201\0"
#define REPLY_400 "HTTP/1.1 400 Bad request\r\nContent-Length: 3\r\n\r\n400\0"
#define REPLY_200F "HTTP/1.1 200 OK\r\nContent-Length: %zi\r\nContent-Type: %s\r\n\r\n"

#endif
