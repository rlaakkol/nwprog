#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "myhttp.h"
#include "tcp_connect.h"
#include "mysockio.h"

void
print_usage(void)
{
	fprintf(stderr, "usage: ./hdcli -d/-u -i iam -l localfile -r remotefile -p port/service host\n");
}

int
main(int argc, char **argv)
{
	int c, mode = 'd', sockfd;
	char *host, *lfilename, *rfilename, *service, *iam;
	FILE *lfile = NULL;
	http_request 	*req;
	http_response 	*res;
	if (argc != 11) {
		print_usage();
		return EXIT_FAILURE;
	}
	req = malloc(sizeof(http_request));
	res = malloc(sizeof(http_response));

	res->payload_len = 0;

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
			
			lfilename = optarg;
			break;
			case 'r':
			
			rfilename = optarg;
			break;
			case 'p':
			
			service = optarg;
			break;
			case 'i':
			
			iam = optarg;
			break;
			default:
			break;
		}
	}

	/* Main logic inside a do-while loop, break for freeing resources after failure */
	do {
		if (optind < argc) {
			host = argv[optind];
		} else {
			print_usage();
			break;
		}

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
				if (store_response_payload(lfile, res) < 0) break;
			} else {
				/* If not OK, print error message and possible HTTP response content */
				fprintf(stderr, "Non-OK response code: %d!\n", restype_to_int(res));
				if (res->payload_len > 0) {
					fprintf(stderr, "Error message payload:\n---\n");
			
					store_response_payload(stderr, res);
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
					store_response_payload(stderr, res);
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

