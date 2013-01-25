#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>


int tcp_connect(const char *host, const char *serv);

int httprecv(char* url, char* service, char* rfilename, char* lfilename);

int httpsend(char* url, char* service, char* rfilename, char* lfilename);
