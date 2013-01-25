
#include "npbcli.h"

int main(int argc, char **argv)
{
	int c, mode;
	char *url, *lfilename, *rfilename, *service;
	if (argc != 9) {
		fprintf(stderr, "usage: npbcli -d/-u -l localfile -r remotefile -p port/service host\n");
		return EXIT_FAILURE;
	}



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
		url = malloc((strlen(argv[optind]) + 1)*sizeof(char));
		strcpy(url, argv[optind]);
	} else {
		fprintf(stderr, "usage: npbcli -d/-u -l localfile -r -p port/service remotefile host\n");
		return EXIT_FAILURE;
	}



	if (mode == 'd') {
		return httprecv(url, service, rfilename, lfilename);
	}
	else {
		return httpsend(url, service, rfilename, lfilename);
	}

}

