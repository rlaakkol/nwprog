#include "npbcli.h"

#define MAXLINE 20000

int httprecv(char* hosturl, char* service, char* rfilename, char* lfilename)
{
	int sockfd, l, n, first, header;
	char sendbuf[500], recvbuf[MAXLINE], *end_of_header, *end_of_startline, resp_startline[80];
	FILE* errorfile, *outfile, *lfile;
	char ok_resp[] = "HTTP/1.1 200 OK";
	char errorfname[] = "error.txt";
	if ((lfile = fopen(lfilename, "w")) == NULL) {
		fprintf(stderr, "Error opening file %s for writing.\n", lfilename);
		return EXIT_FAILURE;
	}

	errorfile = fopen(errorfname, "w");

	if ((sockfd = tcp_connect(hosturl, service)) == -1) {
		fprintf(stderr, "error connecting\n");
		return EXIT_FAILURE;
	}
	/* Create request with no keep-alive */
	sprintf(sendbuf, "GET /%s HTTP/1.1\r\nConnection: close\r\nHost: %s\r\nIam: rlaakkol\r\n\r\n", rfilename, hosturl);
	l = strlen(sendbuf);
	if (write(sockfd, sendbuf, l) < 0) {
		fprintf(stderr, "write error\n");
		return EXIT_FAILURE;
	}
	
	first = 1;
	header = 1;
	outfile = lfile;
	while((n = read(sockfd, recvbuf, MAXLINE)) > 0) {
		recvbuf[n] = '\0';

		if (header) {
			if (first && (end_of_startline = strstr(recvbuf, "\r\n")) != NULL) {
				first = 0;
				strncpy(resp_startline, recvbuf, end_of_startline - recvbuf);
				if (strcmp(resp_startline, ok_resp) != 0) {
					fprintf(stderr, "ERROR: %s\nMessage readable in %s\n", resp_startline, errorfname);
					outfile = errorfile;
				}
			}


			end_of_header = strstr(recvbuf, "\r\n\r\n");
			if (end_of_header != NULL) {
				header = 0;
				if (fputs(end_of_header+4, outfile) == EOF) {
					fprintf(stderr, "fputs error\n");
					return EXIT_FAILURE;
				}
			}
		}
		else if (fputs(recvbuf, outfile) == EOF) {
			fprintf(stderr, "fputs error\n");
			return EXIT_FAILURE;
		}
	}
	if (n < 0) {
		fprintf(stderr, "read error\n");
		return EXIT_FAILURE;
	}

	fclose(lfile);
	fclose(errorfile);

	return EXIT_SUCCESS;


}

int httpsend(char* hosturl, char* service, char* rfilename, char* lfilename)
{
	char sendbuf[MAXLINE], recvbuf[MAXLINE], statusline[500], *end_of_statusline;
	int l, sockfd, n, writelen, ret;
	FILE* lfile;
	struct stat buf;
	unsigned long content_length,remaining;
	unsigned int maxwrite, wremaining;
	socklen_t sockbufsize;
	socklen_t intlen = sizeof(int);
	char ok_resp[] = "HTTP/1.1 201 Created";

	printf("foo1\n");

	stat(lfilename, &buf);
	content_length = (unsigned long)(buf.st_size);
	printf("%ld\n", content_length);
	if ((lfile = fopen(lfilename, "r")) == NULL) {
		fprintf(stderr, "Error opening file %s for writing.\n", lfilename);
		return EXIT_FAILURE;
	}

	printf("foo2\n");
	
	if ((sockfd = tcp_connect(hosturl, service)) == -1) {
		fprintf(stderr, "error connecting\n");
		return EXIT_FAILURE;
	}

	printf("foo3\n");
	if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,
				&sockbufsize, &intlen) < 0) {
		fprintf(stderr, "getsockopt error\n");
		return EXIT_FAILURE;
	}
	printf("Socket send buffer size: %d\n", sockbufsize);
	maxwrite = sockbufsize < MAXLINE ? sockbufsize : MAXLINE;

	printf("Maximum write length is %d\n",maxwrite);


	sprintf(sendbuf, "PUT /%s HTTP/1.1\r\nHost: %s\r\nContent-Type: text/plain\r\nIam: rlaakkol\r\nContent-Length: %ld\r\n\r\n", rfilename, hosturl, content_length);
	l = strlen(sendbuf);
	
	printf("%s",sendbuf);
	if (write(sockfd, sendbuf, l) < 0) {
		fprintf(stderr, "write error\n");
		return EXIT_FAILURE;
	}
	
	remaining = content_length;
	wremaining = 0;
	while (remaining > 0 && (ret = fread(sendbuf, sizeof(char), maxwrite, lfile)) > 0) {
		printf("%d", ret);
		printf("file remaining: %ld\n", remaining);

		wremaining = ret;

		while (wremaining > 0) {
			
			writelen = wremaining < maxwrite ? wremaining : maxwrite;

			if ((l = write(sockfd, sendbuf, writelen)) < 0) {
				fprintf(stderr, "%d byte write error:%s\n%s\n", writelen, strerror(errno) ,sendbuf);
				return EXIT_FAILURE;
			}
			wremaining -= l;
			printf("wrote %d bytes\n", l);
		} 
		remaining -= ret;
	}
	if (ret <= 0) {
		fprintf(stderr, "fgets error\n");
		return EXIT_FAILURE;
	}
	printf("waiting for server response");
	if ((n = read(sockfd, recvbuf, MAXLINE)) < 0) {
		recvbuf[n] = '\0';
		fprintf(stderr, "read error\n");
		return EXIT_FAILURE;
	}
	if ((end_of_statusline = strstr(recvbuf, "\r\n")) != NULL) {
		strncpy(statusline, recvbuf, end_of_statusline - recvbuf);
		if (strcmp(ok_resp, statusline) !=0) {
			fprintf(stderr, "ERROR: %s\n", statusline);
			return EXIT_FAILURE;
		}
	} 

	close(sockfd);

	fclose(lfile);

	return EXIT_SUCCESS;
}


