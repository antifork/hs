/* 
 *  $Id$
 *  H A I L S C A N . C
 *
 *  Copyright (c) 1999 bonelli `awgn' nicola <awgn@cosmos.it>
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

Database       *database_services,
               *database_start;

Hostname       *target_host,
               *target_host_save;

char            ports[8192],
                allsv[8192];

static char     unknown[]="unknown";

long int        vhost = 0;

int   		portcount 	= 1,
                timeout         = 25,
                nsock           = 32,
                sockcount 	= 0,
                lpause 		= 0,
		opts            = 0,
        	lport 		= 0;


struct sockaddr_in newsock;


/******** prototypes... *********/

long            getlongbyname (u_char *);

void            usage 			(char *);
void            getrange 		(char *, unsigned int *, unsigned int *);
void            loadhosts 		(char *);
void            loadhostsfromfile 	(char *);
void            loadservices 		(Database *);

void            pushhost 		(unsigned long int, char *);
void            loadports 		(int, char *[], int);
void            reset_sheet 		(Sweep *, int);
void            rm_entry 		(Sweep *);
void            sweep_action 		(Sweep *);

int             push_socket 		(Sweep *);
int             checkservices 		(int);
int             time_out 		(long int, long int, long int, long int, int);

/******************************/

int
main (int argc, char *argv[])
{
	char            c,
	                filehost[100],
			vhost_s[80];

	Sweep          *sheet,
	               *top_sheet;

	database_services 	= (Database *) malloc (sizeof (Database));
	database_start 		= database_services;
	target_host 		= (Hostname *) malloc (sizeof (Hostname));

	target_host_save 	= target_host;


	if (argc == 1)
		usage (argv[0]);

	while ((c = getopt (argc, argv, "ht:w:n:udcf:v:l:")) != EOF) {
		switch (c) {
		case 'h':
			usage (argv[0]);
			break;
		case 't':
			timeout = atoi (optarg);
			break;
		case 'w':
			lpause = atoi (optarg);
			break;
		case 'n':
			nsock = atoi (optarg);
			break;
		case 'u':
			opts |= O_NOVERB;
			break;
		case 'd':
			opts |= O_DEBUG;
			break;
		case 'f':
			opts |= O_HOSTFILE;
			strcpy (filehost, optarg);
			break;
		case 'c':
			opts |= O_CONT;
			break;
		case 'v':
			vhost = getlongbyname (optarg);
			strncpy(vhost_s,optarg,80);	
			break;
		case 'l':
			lport = atoi (optarg);
			break;

		}

	}

	sheet = (Sweep *) malloc (nsock * sizeof (Sweep));
	top_sheet = sheet;

	printf ("\nhailscan %s %s by awgn@cosmos.it\n\n", VERSION,ID);

	if (vhost)
		printf ("Trying to use vhost: %s\n",vhost_s);

	if (lport)
		printf ("Binding port  :%d\n", lport);

	loadservices (database_services);

	if (!(opts & O_HOSTFILE)) {
		loadhosts (argv[optind]);
		optind++;
	}
	else
		loadhostsfromfile (filehost);

	target_host = target_host_save;

	loadports (argc, argv, optind);

	reset_sheet (sheet, nsock);
	portcount = 1;
	sheet = top_sheet;
	sweep_action (sheet);
	exit (0);

}

void
usage (char *prg)
{
	fprintf (stderr,"\nhailscan %s %s by awgn@cosmos.it\n\n"
		"usage:\n%s [options] <target || -f file> port1 port2 low-high..\n\n"
		"targets    : host.domain.tld\n"
		"             192.168.0.2      \n"
		"             192.168.0.2-10   \n"
		"ports      : 21               \n"
		"             21-56 79-110 6667\n"
		"-f file    : load hosts from file\n"
		"-t sec     : timeout value. Default 25 sec\n"
		"-w msec    : pause between sweep. Default 0 msec\n"
		"-n sockets : number of used socket. Default value 32\n"
		"-h         : print this help\n"
		"-u         : unverbose : suppress timeout output\n"
		"-d         : debug mode: show closed ports\n"
		"-c         : continuous scan, by default it'll read ports from /etc/services\n"
		"-v vhost   : vhost support\n"
		"-l port    : local port binded during the port scan\n"
		"-h         : print this help\n",VERSION,ID,prg);

	exit (0);
}

long
getlongbyname (u_char * host)
{
	struct in_addr  addr;
	struct hostent *host_ent;

	if ((addr.s_addr = inet_addr (host)) == -1) {
		if (!(host_ent = gethostbyname (host))) {
			fprintf (stderr, "gethostbyname() or inet_addr() err: invalid data!\n");
			return 0;
		}
		bcopy (host_ent->h_addr, (char *) &addr.s_addr, host_ent->h_length);
	}
	return addr.s_addr;
}


void
getrange (char *opt, unsigned int *start, unsigned int *end)
{
	int		mn=0;

	mn=sscanf(opt,"%u-%u",start,end);

	switch(mn)
	{
	case 0:
		fprintf (stderr, "getrange() err: invalid data!\n");
		exit(1);
		break;
	case 1:
		*end=*start;
		if((*start>65535))
		{
		fprintf (stderr, "getrange() err: invalid port!\n");	
		exit(1);
		}
		break;
	case 2:
		if ((*end < *start) || (*end>65535) || (*start>65535) ) {
		fprintf (stderr, "getrange() err: invalid range!\n");
		exit (1);
		}
		break;
	}
	return;
}


void
loadhosts (char *host)
{
	unsigned long int temp;

	char           *point,
	               *minus,
	                ip	[100]="",
	                netaddr	[100]="";

	int             low	=0,
	                high	=0;


	if (host == NULL) {
		fprintf (stderr, "err: host/ip is requered!\n");
		exit (1);
	}
	if ((point = strrchr (host, '.'))) {

		if ((minus = strchr ((point + 1), '-'))) {

			strncpy (ip, host, strlen (host) - strlen (minus));

			if (inet_addr (ip) != -1) {
				*minus = 0x00;
				low 	= atoi ((strrchr (host, '.') + 1));
				high 	= atoi ((char *) (minus + 1));


				if ((high > 255) || (low > high)) {
					fprintf (stderr, "rangeip() err: invalid range data.\n");
					exit (1);
				}

				point = strrchr (ip, '.');
				*point = 0x00;

				for (; low <= high; low++) {
					sprintf (netaddr, "%s.%d", ip, low);
					pushhost (inet_addr (netaddr), netaddr);
				}

				return;

			}
		}
	}
	if ((temp = getlongbyname (host)))
		pushhost (temp, host);

	return;
}


int
time_out (long int sec_a, long int usec_a, long int sec_b, long int usec_b, int time_max)
{
	if ((((sec_a - sec_b) == time_max) && (usec_a > usec_b)) || ((sec_a - sec_b) > time_max))
		return 1;

	return 0;
}

void
rm_entry (Sweep * sheet)
{
	bzero (sheet->addr, 80);
	sheet->host 	= 0;
	sheet->port 	= 0;
	sheet->socket 	= 0;
	sheet->sec 	= 0;
	sheet->usec 	= 0;
	return;
}


void
reset_sheet (Sweep * sheet_loc, int nsock)
{
	register int             i;

	for (i = 0; i < nsock; i++) {
		rm_entry (sheet_loc);
		sheet_loc++;
	}
	return;

}


void
loadservices (Database * datas)
{
	FILE           *ser;
	char            line[1024],
	                descr[18],
	                stype[4];
	int             port;


	if (!(ser = fopen (SERVICES, "r"))) {
		fprintf (stderr, "loadservices() err: check you /etc/services!\n");
		exit (1);
	}
	while (fgets (line, 1024, ser)) {

		if ((sscanf (line, "%16s%u/%s", descr, &port, stype) == 3) && (!strstr (descr, "#")) && (strstr (stype, "tcp"))) {
			datas->next = (Database *) malloc (sizeof (Database));
			datas->port = port;
			preset (port);

			strcpy (datas->description, descr);
			datas = (Database *) datas->next;
		}
	}
	fclose (ser);
	datas->next = NULL;
	return;
}

void
loadhostsfromfile (char *filename)
{
	FILE           *input;
	char            line[1024],
	                word[120];

	printf ("Loading hosts from :%s\n", filename);
	if (!(input = fopen (filename, "r"))) {
		fprintf (stderr, "loadhostfromfile() err: check %s\n", filename);

		exit (1);
	}

	fprintf (stdout, "Resolving hosts ");
	fflush (stdout);

	while (fgets (line, 1024, input)) {

		if (sscanf (line, "%s", word)) {
			loadhosts (word);
			fprintf (stdout, ".");
			fflush (stdout);
		}
	}
	fclose (input);
	printf (" done\n\n");
	return;
}


char           *
getservices (int port)
{
	int		sp   = 0;
	char		flag = 0;

	
	sp = database_services->port;

	while ( (database_services->port != sp) || !flag ) 

	{

	if (database_services->port == sp) flag |= 0x01;

        if ((database_services->port) == port) 
                return database_services->description;

	database_services = (Database *) database_services->next;

	if (database_services->next == NULL)
		database_services = database_start;

	}

	return unknown;

}


int
checkservices (int port)
{

	while ((database_services->next != NULL)) {

		if ((database_services->port) == port) {

			database_services = database_start;
			return 1;
		}
		database_services = (Database *) database_services->next;
	}

	database_services = database_start;
	return 0;
}


void
pushhost (unsigned long int host, char *alfaname)
{
	target_host->next 	= (Hostname *) malloc (sizeof (Hostname));
	target_host->s_addr 	= host;

	bzero  (target_host->addr, 80);
	strcpy (target_host->addr, alfaname);

	target_host 		= (Hostname *) target_host->next;
	target_host->s_addr 	= 0;

	target_host->next 	= NULL;
	return;

}


void
loadports (int end_port, char *port_str[], int port)
{
	int    		port_0=0,
	                port_s=0,
	                port_e=0;


	if (opts & O_CONT)
		fprintf (stderr, "Continuous mode on.\n\n");
	else
		fprintf (stderr, "Fast scanning...\n\n");

	if (port == end_port) {

		if (opts & O_CONT)
			memset (ports, 0xff, 8192);
		else
			memcpy (ports, allsv, 8192);

		database_services = database_start;
	}
	else
		for (; port < end_port; port++) {

			getrange (port_str[port], (unsigned int *)&port_s, (unsigned int *)&port_e);

			port_0 = port_s;

			for (; port_s <= port_e; port_s++)
				if ((opts & O_CONT) || checkservices (port_s) || (port_s == port_e) || (port_s == port_0))
					setbit (port_s);

			database_services = database_start;

		}

	return;
}


int
push_socket (Sweep * sheet)
{
	struct timeval  actually;
	struct linger   linger_;
	int             val = 0;
	int             opt = 1;

	linger_.l_onoff = 0;

	rm_entry (sheet);

	gettimeofday (&actually, NULL);


	while (target_host->s_addr) {

		if (checkbit (portcount)) {

			strcpy (sheet->addr, target_host->addr);
			sheet->host = target_host->s_addr;
			sheet->sec = actually.tv_sec;
			sheet->usec = actually.tv_usec;
			sheet->port = portcount;
			sheet->socket = socket (AF_INET, SOCK_STREAM, 0);
			if ((val = fcntl (sheet->socket, F_GETFL, 0)) == -1) {
				return 0;
			}

			val |= O_NDELAY;

			fcntl (sheet->socket, F_SETFL, val);

			setsockopt (sheet->socket, SOL_SOCKET, SO_LINGER, 	(char *) &linger_, sizeof (linger_));
			setsockopt (sheet->socket, SOL_SOCKET, SO_REUSEADDR, 	(char *) &opt, sizeof (opt));
			setsockopt (sheet->socket, SOL_SOCKET, SO_OOBINLINE, 	(char *) &opt, sizeof (opt));

			portcount++;
			sockcount++;
			return 1;
		}
		portcount++;
		if (portcount > 65535) {
			portcount = 1;
			target_host = (Hostname *) target_host->next;
		}
	}

	return 0;
}



void
frontend (char *addr, int port, int flag)
{

	switch (flag) {

	case _TIMEOUT_:

		fprintf (stderr, "[0;34m%-35s %5d/tcp %15s[0m\n", addr, port, "TIMEOUT");
		break;

	case _ESTABLISHED_:

		fprintf (stderr, "%-35s %5d/tcp %15s\n", addr, port, getservices (port));
		break;

	case _REFUSED_:

		fprintf (stderr, "[0;31m%-35s %5d/tcp %15s[0m\n", addr, port, strerror (errno));
		break;


	}

	return;
}


void
sweep_action (Sweep * sheet)
{
	register int    i  = 0,
	                ft = 1;

	Sweep          *restart;
	struct timeval  now;

	restart = sheet;


	while (sockcount || ft) {
		sheet = restart;
		ft = 0;

		for (i = 0; i < nsock; i++) {

			gettimeofday (&now, NULL);
			memset (&newsock, 0, sizeof (newsock));

			if (sheet->socket) {

				newsock.sin_family = AF_INET;

                                if (vhost)
                                        newsock.sin_addr.s_addr = vhost;
                                else
                                        newsock.sin_addr.s_addr = INADDR_ANY;

                                if (lport)
                                        newsock.sin_port = htons ((u_short) lport);
                                else
                                        newsock.sin_port = 0;

                                if (vhost || lport) { 
                                        if (bind (sheet->socket, (struct sockaddr *) &newsock, sizeof (newsock)) == -1)
					switch(errno)
					{
					case EACCES:
					case EADDRNOTAVAIL:
						fprintf(stderr,"err:%s!\n",strerror(errno));
						exit(1);
						break;
					}
                                }

				newsock.sin_addr.s_addr = (long) sheet->host;
				newsock.sin_port = htons ((u_short) sheet->port);


				if (connect (sheet->socket, (struct sockaddr *) &newsock, sizeof (newsock)) == 0) {
					close (sheet->socket);
					sockcount--;
					frontend (sheet->addr, sheet->port, _ESTABLISHED_);
					push_socket (sheet);
				}
				else if (sheet->socket && time_out (now.tv_sec, now.tv_usec, sheet->sec, sheet->usec, timeout)) {
					if (!(opts & O_NOVERB ) )
						frontend (sheet->addr, sheet->port, _TIMEOUT_);
					close (sheet->socket);
					sockcount--;
					push_socket (sheet);
				}
				else {
					switch (errno) {
					case ETIMEDOUT:
					case EINVAL:
					case ECONNREFUSED:
					case EADDRNOTAVAIL:
						if (opts & O_DEBUG)
							frontend (sheet->addr, sheet->port, _REFUSED_);
						close (sheet->socket);
						sockcount--;
						push_socket (sheet);
						break;

					case EISCONN:
						close (sheet->socket);
						sockcount--;
						frontend (sheet->addr, sheet->port, _ESTABLISHED_);
						push_socket (sheet);
						break;
					}

				}
			}
			else
				push_socket (sheet);

			sheet++;
		}
		msec (lpause);
	}
	return;
}
