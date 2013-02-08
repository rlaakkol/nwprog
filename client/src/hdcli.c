#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "myhttp.h"
#include "tcp_connect.h"
#include "mysockio.h"


FILE 			*lfile;
int 			sockfd;

void
print_usage(void)
{
	fprintf(stderr, "usage: ./hdcli -d/-u -i iam -l localfile -r remotefile -p port/service host\n");
}

/* Signal handler (Close socket and stream on SIGINT) */
void
handler(int signo)
{
	if (signo == SIGINT) {
		if (sockfd != -1) close(sockfd);
		if (lfile != NULL) fclose(lfile);
		exit(EXIT_FAILURE);
	}
}

int
parse_url(char *url, char *iam, char *host, char *service, char *path)
{
	char bufa[256], bufb[256], bufc[256], scheme[16];

	if (sscanf(url, "%64[^:]://%256s", scheme, bufa) < 2) {
		return -1;
	}


	if (sscanf(bufa, "%64[^@]@%256s", iam, bufb) < 2) {
		strcpy(iam, "none");
		strcpy(bufb, bufa);
	}

	if (sscanf(bufb, "%64[^/]/%64s", bufc, path) < 2){
		return -1;
	}
	if (sscanf(bufc, "%64[^:]:%256s", host, service) < 2) {
		strcpy(service, "80");
		strcpy(host, bufc); 
	}

	return 0;

}

int
main(int argc, char **argv)
{
	int 				c, mode = 'd';
	char 				host[64], lfilename[64], rfilename[64], service[64], iam[64];
	struct sigaction	sa;
	size_t				remaining;
	http_request 		*req;
	http_response 		*res;

	lfile = NULL;
	sockfd = -1;

	if (argc > 11) {
		print_usage();
		return EXIT_FAILURE;
	}

	/* Handle SIGINT */
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	
	
	req = malloc(sizeof(http_request));
	res = malloc(sizeof(http_response));

	res->payload_len = 0;

	if (argc == 11) {
	/* Read command line arguments */
		while ((c = getopt(argc, argv, "dul:r:p:i:")) != -1) {
			switch (c) {
				case 'd':
				mode = 'd';
				break;
				case 'u':
				mode = 'u';
				break;
				case 'l':
				strcpy(lfilename, optarg);
				break;
				case 'r':
				strcpy(rfilename, optarg);
				break;
				case 'p':
				strcpy(service, optarg);
				break;
				case 'i':
				strcpy(iam, optarg);
				break;
				default:
				break;
			}
		}
		if (optind < argc) {
			strcpy(host, argv[optind]);
		} else {
			print_usage();
			return EXIT_FAILURE;
		}
	}
	else if (argc == 5) {
		while ((c = getopt(argc, argv, "dul:")) != -1) {
			switch (c) {
				case 'd':
				mode = 'd';
				break;
				case 'u':
				mode = 'u';
				break;
				case 'l':
				strcpy(lfilename, optarg);
				break;
				default:
				break;
			}
		}
		if (parse_url(argv[4], iam, host, service, rfilename) < 0) {
			fprintf(stderr, "Malformed URL\n");
			return EXIT_FAILURE;
		}
	} else {
		print_usage();
		return EXIT_FAILURE;
	}

	/* Main logic inside a do-while loop, break for freeing resources after failure */
	do {


		/* Connect to server */
		if ((sockfd = tcp_connect(host, service)) == -1) {
			break;
		}

		if (mode == 'd') {
			/* Download (GET) mode */

			/* Open local file for writing */
			if ((lfile = fopen(lfilename, "w")) == NULL) {
				fprintf(stderr, "Error opening file %s for writing: %s\n", lfilename, strerror(errno));
				break;
			}

			/* Generate GET request */
			generate_request(GET, rfilename, host, iam, NULL, 0, NULL, req);

			/* Send the request to server */
			if (send_request(sockfd, req, NULL) < 0) break;

			/* Parse the response */
			if (parse_response(sockfd, res) < 0) break;
			if (res->type == OK) {
				/* If response is 200 OK, write content to file */
				printf("Writing response to file\n");
				if (store_response_payload(lfile, res, &remaining) < 0) { 
					break;
				}
				if (remaining > 0) {
					fprintf(stderr, "Only partially downloaded before connection closed! %lu bytes missing from the end. Stopping!\n", (long unsigned int)remaining);
					break;
				}
			} else {
				/* If not OK, print error message and possible HTTP response content */
				fprintf(stderr, "Non-OK response code: %d!\n", restype_to_int(res));
				if (res->payload_len > 0) {
					fprintf(stderr, "Error message payload:\n---\n");
			
					store_response_payload(stderr, res, &remaining);
				}
				break;
			}
		} else {
			/* Upload (PUT) mode */

			/* Open local file for reading */
			if ((lfile = fopen(lfilename, "r")) == NULL) {
				fprintf(stderr, "Error opening file %s for reading: %s\n", lfilename, strerror(errno));
				break;
			}

			/* Generate PUT request */
			generate_request(PUT, rfilename, host, iam, lfilename, 0, "text/plain", req);
			
			/* Send request to server */
			if (send_request(sockfd, req, lfile) < 0) break;

			/* Parse the response */
			if (parse_response(sockfd, res) < 0) break;

			if (res->type == CREATED || res->type == OK) {
				/* If response type is CREATED (new file) or OK (existing file), print success message */
				fprintf(stdout, "Successfully created remote file\n");
			} else {
				/* If response type is some failure, print data */
				fprintf(stderr, "Error creating remote file! Code %d\n", restype_to_int(res));
				if (res->payload_len > 0) {
					 fprintf(stderr, "Error message payload:\n---\n");
					store_response_payload(stderr, res, &remaining);
				}
				break;
			}

		}

		/* Everything was successful :) Free resources */
		free(req);
		free(res);
		close(sockfd);
		fclose(lfile);

		return EXIT_SUCCESS;
	} while (0);

	/* Something went wrong :( Free resources */
	free(req);
	free(res);
	if (sockfd != -1) close(sockfd);
	if (lfile != NULL) fclose(lfile);
	return EXIT_FAILURE;
}

