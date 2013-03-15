#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <glib.h>

#include "mysockio.h"
#include "myhttp.h"

#define MAXFD 64



void
make_nonblocking(int fd)
{
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

my_cli *
cli_init(int fd)
{
	printf("Initiating client struct\n");
	my_cli 	*cli;

	cli = malloc(sizeof(my_cli));
	printf("Initiating buffer\n");
	cli->buf = buf_init();
	cli->req = malloc(sizeof(http_request));
	cli->res = malloc(sizeof(http_response));
	cli->linebuf[0] = '\0';
	cli->state = ST_INIT;
	cli->read_bytes = 0;
	cli->written_bytes = 0;
	cli->fd = fd;

	return cli;
}

void
cli_del(my_cli *cli)
{
	close(cli->fd);
	close(cli->localfd);
	free(cli->req);
	free(cli->res);
	free(cli->buf);
	free(cli);
}



int
daemonize(char *wd)
{
	int 	pid,i ;
		/* Create child, terminate parent
	   - shell thinks command has finished, child continues in background
	   - inherits process group ID => not process group leader
	   - => enables setsid() below
	 */
	if ( (pid = fork()) < 0)
		return -1;  // error on fork
	else if (pid)
		exit(0);			/* parent terminates */

	/* child 1 continues... */

	/* Create new session
	   - process becomes session leader, process group leader of new group
	   - detaches from controlling terminal (=> no SIGHUP when terminal
	     session closes)
	 */
	if (setsid() < 0)			/* become session leader */
		return -1;

	/* Ignore SIGHUP. When session leader terminates, children will
	   will get SIGHUP (see below)
	 */
	signal(SIGHUP, SIG_IGN);

	/* Create a second-level child, terminate first child
	   - second child is no more session leader. If daemon would open
	     a terminal session, it may become controlling terminal for
	     session leader.
	 */
	if ( (pid = fork()) < 0)
		return -1;
	else if (pid)
		exit(0);			/* child 1 terminates */

	/* child 2 continues... */

	/* change to "safe" working directory. If daemon uses a mounted
	   device as WD, it cannot be unmounted.
	 */
	if(chdir(wd) < 0) {/* change working directory */
		syslog(LOG_ERR, "Cannot change working directory");
		exit(-1);
	}
	/* close off file descriptors (including stdin, stdout, stderr) */
	// (may have been inherited from parent process)
	for (i = 0; i < MAXFD; i++)
		close(i);

	/* redirect stdin, stdout, and stderr to /dev/null */
	// Now read always returns 0, written buffers are ignored
	// (some third party libraries may try to use these)
	open("/dev/null", O_RDONLY); // fd 0 == stdin
	open("/dev/null", O_RDWR); // fd 1 == stdout
	open("/dev/null", O_RDWR); // fd 2 == stderr

	return 0;
}

void sig_int(int signo)
{
	syslog(LOG_INFO, "received signal %d", signo);
	exit(0);
}

int
main(int argc, char **argv)
{


	int 		listenfd, connfd, maxfd, fail;
	socklen_t	  clilen, addrlen;
	struct sockaddr	  *cliaddr;
	my_cli 			*current;
	GSList 			*clients, *next;
	fd_set 	readset, writeset, exset;
	char 		wd[128], laddr[128], *lport, c;
	int 		daemon = 0;

	getcwd(wd, 128);
	laddr[0] = '\0';
	//

	clients = NULL;


	printf("%d\n", EXIT_SUCCESS);
	while ((c = getopt(argc, argv, "dp:b:")) != -1) {
			switch (c) {
				case 'd':
				daemon = 1;
				break;
				case 'p':
				strncpy(wd, optarg, 128);
				break;
				case 'b':
				strncpy(laddr, optarg, 128);
				break;
				default:
				break;
			}
	}
	if (optind >= argc) {
		fprintf(stderr, "usage: %s [ -d ] [ -p <path> ] [ -b <host> ] <port#>\n", argv[0]);
		exit(-1);
	} else {
		lport = argv[optind];
	}

	printf("Working directory is %s\n", wd);
	if (daemon) daemonize(wd);

	// open syslog
	openlog(argv[0], LOG_PID, LOG_LOCAL7);

	if (strlen(laddr) == 0)
		listenfd = tcp_listen(NULL, lport, &addrlen);
	else 
		listenfd = tcp_listen(laddr, lport, &addrlen);

	make_nonblocking(listenfd);

	// addrlen may be variable, because we may have AF_INET or AF_INET6
	// sockets
	cliaddr = malloc(addrlen);
	if (cliaddr == NULL) {
		syslog(LOG_ERR, "malloc");
		return -1;
	}

	printf("ST_INIT=%d\nST_SETUP=%d\nST_PUT=%d\nST_GET=%d\nST_RESPOND=%d\nST_FINISHED=%d\n", ST_INIT, ST_SETUP, ST_PUT, ST_GET, ST_RESPOND, ST_FINISHED);

	// set special signal handlers
	//signal(SIGCHLD, sig_chld); // defined in common.c
	signal(SIGINT, sig_int);

	for ( ; ; ) {

		maxfd = listenfd;

		FD_ZERO(&readset);
		FD_ZERO(&writeset);
		FD_ZERO(&exset);

		FD_SET(listenfd, &readset);

		//printf("Selecting clients to listen\n");
		next = clients;
		while (next != NULL) {
			current = next->data;
			maxfd = current->fd > maxfd ? current->fd : maxfd;
			FD_SET(current->fd, &readset);
			if(current->state == ST_GET || current->state == ST_RESPOND || current->state == ST_SETUP) {
				FD_SET(current->fd, &writeset);
			}
			next = g_slist_next(next);

		}

		//printf("Going to select\n");
		if (select(maxfd+1, &readset, &writeset, &exset, NULL) < 0) {
			perror("select");
			return -1;
		}
		//printf("Out from select\n");

		if (FD_ISSET(listenfd, &readset)) {
			printf("New connection!\n");
			clilen = addrlen;
			connfd = accept(listenfd, cliaddr, &clilen);
			if (connfd < 0) {
				perror("accept");
			} else if (connfd > FD_SETSIZE) {
				close(connfd);
			} else {
				printf("Connection successful!\n");
				make_nonblocking(connfd);
				clients = g_slist_append(clients, cli_init(connfd));
				printf("Appended to list!\n");
			}
        }

		// clilen = addrlen;
		// if ( (connfd = accept(listenfd, cliaddr, &clilen)) < 0) {
		// 	if (errno == EINTR)
		// 		continue;		/* back to for() */
		// 	else {
		// 		syslog(LOG_ERR, "accept error");
		// 		return -1;
		// 	}
		// }

		// if ( (childpid = fork()) == 0) {	/* child process */
		// 	close(listenfd);	/* close listening socket */
		// 	web_child(connfd);	/* process request */
		// 	exit(0);
		// }
		// close(connfd);			/* parent closes connected socket */
		next = clients;
		while (next != NULL) {
			current = next->data;
			fail = 0;
			//printf("Client state: %d\n", current->state);
			if (current->buf->buffered > 0 || FD_ISSET(current->fd, &readset)) {
				
				if (current->state == ST_INIT) {
					fail = parse_request(current);
				} else if (current->state == ST_SETUP) {
					fail = files_setup(current);
				} else if (current->state == ST_PUT) {
					fail = sock_to_file(current);
				}
			} else if (FD_ISSET(current->fd, &writeset)) {
				if (current->state == ST_SETUP) {
					fail = files_setup(current);
				} else if (current->state == ST_GET) {
					fail = file_to_sock(current);
				} else if (current->state == ST_RESPOND) {
					fail = send_response(current);
				}
			} else if ((fail && current->state != ST_RESPOND) || current->state == ST_FINISHED) {
				printf("Removing client\n");
				next = g_slist_next(next);
				clients = g_slist_remove(clients, current);
				cli_del(current);
				continue;
			}
			next = g_slist_next(next);
		}

	}

	free(cliaddr);

	return 0;				/* success */
}


/* end serv01 */



/* end sigint */
