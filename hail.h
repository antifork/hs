/*
 *  $Id$ 
 *  hailscan.h 
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>

/*
Check if the system supports POSIX.1 
*/

#if HAVE_UNISTD_H
# include <sys/types.h>
# include <unistd.h>
#endif

/*
See AC_HEADER_STDC in 'info autoconf'
*/

#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

/* 
See AC_HEADER_TIME in 'info autoconf' 
*/

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>


#define SERVICES 	"/etc/services"
#define VERSION 	"2.5"

#define preset(x)       ((x % 8) ? (allsv [x / 8] |= (0x01 << ((x % 8) - 1))) : (allsv [(x / 8) - 1] |= 0x80))
#define setbit(x)       ((x % 8) ? (ports [x / 8] |= (0x01 << ((x % 8) - 1))) : (ports [(x / 8) - 1] |= 0x80))
#define checkbit(x)     ((x % 8) ? (ports [x / 8] &  (0x01 << ((x % 8) - 1))) : (ports [(x / 8) - 1] &  0x80))
#define msec(x)        	(usleep(1000*x))

#define _TIMEOUT_	0
#define _ESTABLISHED_	1
#define _REFUSED_	2

#define O_NOVERB	0x01
#define O_DEBUG		0x02
#define O_HOSTFILE	0x04
#define O_CONT		0x08


/* 

 * structure of ports loaded in 8192 bytes.          
 * 
 * ------------------------------------------------------------------- 
 *         3  2  1 |  8  7  6  5  4  3  2  1 |  8  7  6  5  4  3  2  1|
 * ------------------------------------------------------------------- 
 * .. ..  19 18 17 | 16 15 14 13 12 11 10  9 |  8  7  6  5  4  3  2  1|
 * -------------------------------------------------------------------
 *              
 * 
 */



typedef struct _sweep_ {
	int             socket;
	char            addr[80];
	long            host;
	int             port;
	long            sec;
	long            usec;
} Sweep;

typedef struct _database_ {
	int             port;
	char            description[16];
	struct _database_ *next;

} Database;

typedef struct _hostname_ {
	unsigned long int s_addr;
	char            addr[80];
	struct _hostname_ *next;
} Hostname;
