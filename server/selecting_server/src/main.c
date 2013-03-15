// Server example 1 (concurrent server, one child / client)
// modified from W.R. Stevens example, Fig. 30.4, server/serv_01.c
//
// To build:
// gcc -o serv01 serv01.c common.c
// (i.e., common.c has the common functions used in this file)

#include "npbserver.h" // our own header for support functions in common.c
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <syslog.h>

#define MAXFD 64

int daemon_init(int argc, const char **argv, int facility)
{
	int	i;
	pid_t	pid;

	int		  listenfd, connfd;
	pid_t		  childpid;
	void		  sig_chld(int), sig_int(int), web_child(int);
	socklen_t	  clilen, addrlen;
	struct sockaddr	  *cliaddr;
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
	if(chdir(argv[1]) < 0) {/* change working directory */
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

	// open syslog
	openlog(argv[0], LOG_PID, facility);
	if (argc == 3)
		listenfd = tcp_listen(NULL, argv[2], &addrlen);
	else if (argc == 4)
		listenfd = tcp_listen(argv[2], argv[3], &addrlen);
	else {
		fprintf(stderr, "usage: npbsrv [ <host> ] <port#>\n");
		return -1;
	}

	// addrlen may be variable, because we may have AF_INET or AF_INET6
	// sockets
	cliaddr = malloc(addrlen);
	if (cliaddr == NULL) {
		syslog(LOG_ERR, "malloc");
		return -1;
	}

	// set special signal handlers
	signal(SIGCHLD, sig_chld); // defined in common.c
	signal(SIGINT, sig_int);

	for ( ; ; ) {
		clilen = addrlen;
		if ( (connfd = accept(listenfd, cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else {
				syslog(LOG_ERR, "accept error");
				return -1;
			}
		}

		if ( (childpid = fork()) == 0) {	/* child process */
			close(listenfd);	/* close listening socket */
			web_child(connfd);	/* process request */
			exit(0);
		}
		close(connfd);			/* parent closes connected socket */
	}

	free(cliaddr);

	return 0;				/* success */
}


int main(int argc, const char **argv)
{

	if (argc < 3 || argc > 4)
	{
		fprintf(stderr, "usage: npbsrv workdir [ <host> ] <port#>\n");
                return -1;
	}
	daemon_init(argc, argv, LOG_LOCAL7);

	return 0;
}
/* end serv01 */


void sig_int(int signo)
{
	syslog(LOG_INFO, "received signal %d", signo);
	exit(0);
}
/* end sigint */
