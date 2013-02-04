#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "myhttp.h"
#include "tcp_connect.h"
#include "mysockio.h"

int main(int argc, char **argv)
{
	int c, mode, sockfd;
	char *host, *lfilename, *rfilename, *service;
	FILE *lfile = NULL;
	http_request 	*req;
	http_response 	*res;
	if (argc != 9) {
		fprintf(stderr, "usage: npbcli -d/-u -l localfile -r remotefile -p port/service host\n");
		return EXIT_FAILURE;
	}
	req = malloc(sizeof(http_request));
	res = malloc(sizeof(http_response));

	while ((c = getopt(argc, argv, "dul:r:p:")) != -1) {
		switch (c) {
			case 'd':
			mode = 'd';
			break;
			case 'u':
			mode = 'u';
			break;
			case 'l':
			lfilename = malloc((strlen(optarg) + 1)*sizeof(char));
			strcpy(lfilename, optarg);
			break;
			case 'r':
			rfilename = malloc((strlen(optarg) + 1)*sizeof(char));
			strcpy(rfilename, optarg);
			break;
			case 'p':
			service = malloc((strlen(optarg) + 1)*sizeof(char));
			strcpy(service, optarg);
			break;
			default:
			break;
		}
	}
	do {
		if (optind < argc) {
			host = malloc((strlen(argv[optind]) + 1)*sizeof(char));
			strcpy(host, argv[optind]);
		} else {
			fprintf(stderr, "usage: npbcli -d/-u -l localfile -r -p port/service remotefile host\n");
			break;
		}

		if ((sockfd = tcp_connect(host, service)) == -1) {
			break;
		}

		if (mode == 'd') {
			if ((lfile = fopen(lfilename, "w")) == NULL) {
				fprintf(stderr, "Error opening file %s for writing: %s\n", lfilename, strerror(errno));
				break;
			}

			generate_request(GET, rfilename, host, "rlaakkol", NULL, 0, NULL, req);

			if (send_request(sockfd, req, NULL) < 0) break;

			if (parse_response(sockfd, res) < 0) break;
			if (res->type == OK) {
				printf("Writing response to file\n");
				if (store_response_payload(lfile, res) < 0) break;
			} else {
				fprintf(stderr, "Non-OK response code: %d!\n", restype_to_int(res));
				if (res->payload_len > 0) {
					fprintf(stderr, "Error message payload:\n---\n");
			
					store_response_payload(stderr, res);
				}
				break;
			}

		} else {
			if ((lfile = fopen(lfilename, "r")) == NULL) {
				fprintf(stderr, "Error opening file %s for reading: %s\n", lfilename, strerror(errno));
				break;
			}
			generate_request(PUT, rfilename, host, "rlaakkol", lfilename, 0, "text/plain", req);
			
			if (send_request(sockfd, req, lfile) < 0) break;

			if (parse_response(sockfd, res) < 0) break;

			if (res->type == CREATED || res->type == OK) {
				fprintf(stdout, "Successfully created remote file\n");
			} else {
				fprintf(stderr, "Error creating remote file! Code %d\n", restype_to_int(res));
				if (res->payload_len > 0) {
					 fprintf(stderr, "Error message payload:\n---\n");
					store_response_payload(stderr, res);
				}
				break;
			}

		}

		free(host);
		free(lfilename);
		free(rfilename);
		free(service);
		free(req);
		free(res);
		close(sockfd);
		fclose(lfile);

		return EXIT_SUCCESS;
	} while (0);

	free(host);
	free(lfilename);
	free(rfilename);
	free(service);
	free(req);
	free(res);
	if (sockfd != -1) close(sockfd);
	if (lfile != NULL) fclose(lfile);
	return EXIT_FAILURE;
}

