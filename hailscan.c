/*
 *  $Id$
 *  H A I L S C A N . C
 *
 *  Copyright (c) 1999 `awgn' <awgn@cosmos.it>
 *            (c) 2002 Bonelli Nicola <bonelli@antifork.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Tested on Linux and *BSD.
 *
 */

#define ID "$Id$"

#include "hail.h"

#if defined(HAVE_BSD_SYS_QUEUE_H)
#include <sys/queue.h>
#else
#include "missing/queue.h"
#endif

SLIST_HEAD(, entry) head;

	struct entry {
		unsigned long s_addr;
		char *addr;
		     SLIST_ENTRY(entry) next;
	};

	struct entry *target_host;

	char ports[8192];
	char allsv[8192];

	long int vhost;

	int portcount = 1;
	int timeout = 25;
	int nsock = 32;
	int sockcount;
	int lpause;
	int opts;
	int lport;

/******** prototypes... *********/

	long getlongbyname(u_char *);
	void usage(char *);
	void getrange(char *, unsigned short *, unsigned short *);
	void loadhosts(char *);
	void loadhostsfromfile(char *);
	void pushhost(unsigned long int, char *);
	void loadallports(void);
	void loadports(int, char *[], int);
	void reset_sheet(Sweep *, int);
	void rm_entry(Sweep *);
	void sweep_action(Sweep *);
	int push_socket(Sweep *);
	int time_out(long int, long int, long int, long int, int);

/******************************/

int
main(int argc, char *argv[])
{
	char c, filehost[100], vhost_s[80];
	Sweep *sheet, *top_sheet;

	SLIST_INIT(&head);

	if (argc == 1)
		usage(argv[0]);

	while ((c = getopt(argc, argv, "ht:w:n:udcf:v:l:")) != EOF) {
		switch (c) {
		case 'h':
			usage(argv[0]);
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		case 'w':
			lpause = atoi(optarg);
			break;
		case 'n':
			nsock = atoi(optarg);
			break;
		case 'u':
			opts |= O_NOVERB;
			break;
		case 'd':
			opts |= O_DEBUG;
			break;
		case 'f':
			opts |= O_HOSTFILE;
			strcpy(filehost, optarg);
			break;
		case 'c':
			opts |= O_CONT;
			break;
		case 'v':
			vhost = getlongbyname(optarg);
			strncpy(vhost_s, optarg, 80);
			break;
		case 'l':
			lport = atoi(optarg);
			break;

		}

	}

	sheet = (Sweep *) malloc(nsock * sizeof(Sweep));
	top_sheet = sheet;

	printf("hailscan %s %s by awgn@cosmos.it\n", VERSION, ID);

	if (vhost)
		printf("Trying to use vhost: %s\n", vhost_s);

	if (lport)
		printf("Binding port  :%d\n", lport);

	if (!(opts & O_HOSTFILE)) {
		loadhosts(argv[optind]);
		optind++;
	} else
		loadhostsfromfile(filehost);

	loadallports();
	loadports(argc, argv, optind);

	reset_sheet(sheet, nsock);

	setservent(1);
	portcount = 1;
	sheet = top_sheet;

	sweep_action(sheet);

	exit(0);
}

void
usage(char *prg)
{
	fprintf(stderr, "hailscan %s %s by awgn@cosmos.it\n"
	   "usage:%s [options] [target || -f file] port1 port2 low-high..\n"
		"targets    : host.domain.tld\n"
		"             192.168.0.2      \n"
		"             192.168.0.2-10   \n"
		"ports      : 21               \n"
		"             21-56 79-110 6667\n"
		"-f file    : load hosts from file\n"
		"-t sec     : timeout value. Default 25 sec\n"
		"-w msec    : pause between sweep. Default 0 msec\n"
		"-n sockets : number of used socket. Default value 32\n"
		"-u         : unverbose : suppress timeout output\n"
		"-d         : debug mode: show closed ports\n"
		"-c         : continuous scan, by default it'll read ports from /etc/services\n"
		"-v vhost   : vhost support\n"
		"-l port    : local port binded during the port scan\n"
		"-h         : print this help\n", VERSION, ID, prg);

	exit(0);
}

long
getlongbyname(u_char * host)
{
	struct in_addr addr;
	struct hostent *host_ent;

	if ((addr.s_addr = inet_addr(host)) == -1) {
		if (!(host_ent = gethostbyname(host))) {
			fprintf(stderr, "gethostbyname() or inet_addr() err: invalid data!\n");
			return 0;
		}
		bcopy(host_ent->h_addr, (char *) &addr.s_addr, host_ent->h_length);
	}
	return addr.s_addr;
}

void
getrange(char *opt, unsigned short *start, unsigned short *end)
{
	int mn;
	mn = sscanf(opt, "%hu-%hu", start, end);

	switch (mn) {
	case 0:
		fprintf(stderr, "getrange() err: invalid data!\n");
		exit(1);
		break;
	case 1:
		*end = *start;
		break;
	case 2:
		if ((*end < *start)) {
			fprintf(stderr, "getrange() err: invalid range!\n");
			exit(1);
		}
		break;
	}
	return;
}

void
loadhosts(char *host)
{
	char netaddr[100];
	char ip[100];
	char *point;
	char *min;
	unsigned long int temp;
	int low;
	int high;

	low = 0;
	high = 0;
	ip[0] = 0;
	netaddr[0] = 0;

	if (host == NULL) {
		fprintf(stderr, "err: host/ip is requered!\n");
		exit(1);
	}
	if ((point = strrchr(host, '.'))) {
		if ((min = strchr((point + 1), '-'))) {
			strncpy(ip, host, strlen(host) - strlen(min));
			if (inet_addr(ip) != -1) {
				*min = 0;
				low = atoi((strrchr(host, '.') + 1));
				high = atoi((char *) (min + 1));

				if ((high > 255) || (low > high)) {
					fprintf(stderr, "rangeip() err: invalid range data.\n");
					exit(1);
				}
				point = strrchr(ip, '.');
				*point = 0;

				for (; low <= high; low++) {
					sprintf(netaddr, "%s.%d", ip, low);
					pushhost(inet_addr(netaddr), netaddr);
				}

				target_host = SLIST_FIRST(&head);

				return;
			}
		}
	}
	if ((temp = getlongbyname(host)))
		pushhost(temp, host);
	target_host = SLIST_FIRST(&head);

	return;
}

int
time_out(long int sec_a, long int usec_a, long int sec_b, long int usec_b, int tout)
{
	if ((sec_a - sec_b) < tout)
		return 0;
	if ((sec_a - sec_b) == tout && (usec_a < usec_b))
		return 0;
	return 1;
}

void
rm_entry(Sweep * sheet)
{
	bzero(sheet->addr, 80);
	sheet->host = 0;
	sheet->port = 0;
	sheet->socket = 0;
	sheet->sec = 0;
	sheet->usec = 0;
	sheet->status = 0;
	return;
}

void
reset_sheet(Sweep * sheet_loc, int nsock)
{
	register int i;

	for (i = 0; i < nsock; i++) {
		rm_entry(sheet_loc);
		sheet_loc++;
	}
	return;
}

void
loadhostsfromfile(char *filename)
{
	char line[1024];
	char word[120];
	FILE *input;

	printf("Loading hosts from :%s\n", filename);
	if (!(input = fopen(filename, "r"))) {
		fprintf(stderr, "loadhostfromfile() err: check %s\n", filename);
		exit(1);
	}
	fprintf(stdout, "Resolving hosts ");
	fflush(stdout);

	while (fgets(line, 1024, input)) {
		if (sscanf(line, "%s", word)) {
			loadhosts(word);
			fprintf(stdout, ".");
			fflush(stdout);
		}
	}
	fclose(input);
	printf(" done\n\n");
	return;
}

void
pushhost(unsigned long int host, char *alfaname)
{
	struct entry *p;

	p = (struct entry *) malloc(sizeof(struct entry));
	p->s_addr = host;
	p->addr = strdup(alfaname);
	SLIST_INSERT_HEAD(&head, p, next);

	return;
}

void
loadallports()
{
	struct servent *n;

	setservent(1);
	while ((n = getservent()) != NULL)
		SETB(allsv, ntohs(n->s_port));

	endservent();
	return;
}

void
loadports(int end_port, char *port_str[], int port)
{
	unsigned short port_s = 0, port_e = 0;

	fprintf(stderr, (opts & O_CONT) ? "Continuous mode on.\n\n" : "Fast scanning...\n\n");

	if (port == end_port) {
		if (opts & O_CONT)
			memset(ports, 0xff, 8192);
		else
			memcpy(ports, allsv, 8192);

	} else
		for (; port < end_port; port++) {
			getrange(port_str[port], &port_s, &port_e);
			SETB(ports, port_s);
			SETB(ports, port_e);
			for (; port_s <= port_e; port_s++)
				if ((opts & O_CONT) || CHKB(allsv, port_s))
					SETB(ports, port_s);
		}

	return;
}

int
push_socket(Sweep * sheet)
{
	struct timeval actually;
	struct linger linger_;
	int val = 0;
	int opt = 1;

	linger_.l_onoff = 0;
	rm_entry(sheet);
	gettimeofday(&actually, NULL);

	if (target_host == NULL)
		target_host = SLIST_FIRST(&head);

	while (target_host != NULL && target_host->s_addr) {
		if (CHKB(ports, portcount)) {
			strcpy(sheet->addr, target_host->addr);
			sheet->host = target_host->s_addr;
			sheet->sec = actually.tv_sec;
			sheet->usec = actually.tv_usec;
			sheet->port = portcount;
			sheet->socket = socket(AF_INET, SOCK_STREAM, 0);
			if ((val = fcntl(sheet->socket, F_GETFL, 0)) == -1) {
				return 0;
			}
			val |= O_NDELAY;
			fcntl(sheet->socket, F_SETFL, val);

			setsockopt(sheet->socket, SOL_SOCKET, SO_LINGER, (char *) &linger_, sizeof(linger_));
			setsockopt(sheet->socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt));

			portcount++;
			sockcount++;
			return 1;
		}
		portcount++;
		if (portcount > 65535) {
			portcount = 1;
			SLIST_REMOVE_HEAD(&head, next);
			target_host = SLIST_FIRST(&head);
		}
	}
	return 0;
}

void
frontend(char *addr, int port, int stat)
{
	struct servent *n;

	switch (stat) {
	case S_TIMEOUT:
		fprintf(stderr, "[0;34m%-35s %5d/tcp %15s[0m\n", addr, port, "TIMEOUT");
		break;
	case S_ESTABLISHED:
		n = getservbyport(htons(port), NULL);
		fprintf(stderr, "%-35s %5d/tcp %15s\n", addr, port, (n != NULL) ? n->s_name : "unknown");
		break;
	case S_REFUSED:
		fprintf(stderr, "[0;31m%-35s %5d/tcp %15s[0m\n", addr, port, strerror(errno));
		break;
	}

	return;
}

void
sweep_action(Sweep * sheet)
{
	register int i = 0, ft = 1;
	struct timeval now;
	Sweep *restart;

	restart = sheet;

	while (sockcount || ft) {
		sheet = restart;
		ft = 0;

		for (i = 0; i < nsock; i++) {
			gettimeofday(&now, NULL);
			if (sheet->socket) {
				if (!(sheet->status & S_BINDED)) {
					sheet->sa.sin_family = AF_INET;
					if (vhost)
						sheet->sa.sin_addr.s_addr = vhost;
					else
						sheet->sa.sin_addr.s_addr = INADDR_ANY;

					if (lport)
						sheet->sa.sin_port = htons((u_short) lport);
					else
						sheet->sa.sin_port = 0;

					if (vhost || lport) {
						if (bind(sheet->socket, (struct sockaddr *) & sheet->sa, sizeof(sheet->sa)) == -1)
							switch (errno) {
							case EACCES:
							case EADDRNOTAVAIL:
								fprintf(stderr, "err:%s!\n", strerror(errno));
								exit(1);
								break;
							}
					}
					sheet->sa.sin_addr.s_addr = (long) sheet->host;
					sheet->sa.sin_port = htons((u_short) sheet->port);
					sheet->status |= S_BINDED;
				}
				if (connect(sheet->socket, (struct sockaddr *) & sheet->sa, sizeof(sheet->sa)) == 0) {
					close(sheet->socket);
					sockcount--;
					frontend(sheet->addr, sheet->port, S_ESTABLISHED);
					push_socket(sheet);
				} else if (sheet->socket && time_out(now.tv_sec, now.tv_usec, sheet->sec, sheet->usec, timeout)) {
					if (!(opts & O_NOVERB))
						frontend(sheet->addr, sheet->port, S_TIMEOUT);
					close(sheet->socket);
					sockcount--;
					push_socket(sheet);
				} else {
					switch (errno) {
					case ETIMEDOUT:
					case EINVAL:
					case ECONNREFUSED:
					case EADDRNOTAVAIL:
						if (opts & O_DEBUG)
							frontend(sheet->addr, sheet->port, S_REFUSED);
						close(sheet->socket);
						sockcount--;
						push_socket(sheet);
						break;
					case EISCONN:
						close(sheet->socket);
						sockcount--;
						frontend(sheet->addr, sheet->port, S_ESTABLISHED);
						push_socket(sheet);
						break;
					}
				}
			} else
				push_socket(sheet);

			sheet++;
		}
		msec(lpause);
	}
	return;
}
