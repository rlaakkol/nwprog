
#include "npbcli.h"

int main(int argc, char **argv)
{
	int c, mode;
	char *host, *lfilename, *rfilename, *service;
	FILE *lfile;
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
	if (optind < argc) {
		host = malloc((strlen(argv[optind]) + 1)*sizeof(char));
		strcpy(url, argv[optind]);
	} else {
		fprintf(stderr, "usage: npbcli -d/-u -l localfile -r -p port/service remotefile host\n");
		return EXIT_FAILURE;
	}

	if ((sockfd = tcp_connect(host, service)) == -1) {
		fprintf(stderr, "error connecting\n");
		return EXIT_FAILURE;
	}


	if (mode == 'd') {
		if ((lfile = fopen(lfilename, "w")) == NULL) {
			fprintf(stderr, "Error opening file %s for writing.\n", lfilename);
			return EXIT_FAILURE;
		}

		generate_request(GET, rfilename, host, "rlaakkol", NULL, 0, req);
		send_request(sockfd, req);
		parse_response(sockfd, res);
		if (res->type == OK) {
			store_response_payload(lfile, res);
			return 0;
		} else {
			fprintf(stderr, "Remote file not found\n");
			return EXIT_FAILURE;
		}

	} else {
		if ((lfile = fopen(lfilename, "r")) == NULL) {
			fprintf(stderr, "Error opening file %s for reading.\n", lfilename);
			return EXIT_FAILURE;
		}
		generate_request(PUT, rfilename, host, "rlaakkol", lfilename, 0, req);
		send_request(sockfd, req, lfile);
		parse_response(sockfod, res);
		if (res->type == CREATED) {
			fprintf(stdout, "Successfully created remote file\n"),
			return 0;
		} else {
			fprintf(stderr, "Error creating remote file\n");
			return EXIT_FAILURE;
		}

	}

}

