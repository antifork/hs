# Generated automatically from Makefile.in by configure.
#
# $Id$
# hailscan
# Copyright (c) 1999 bonelli `awgn' nicola <awgn@cosmos.it>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#

CC     = gcc 
CFLAGS = -Wall -Wstrict-prototypes -g -O2 
DEFS   = -DHAVE_CONFIG_H
LDFLAGS=  
LIBS   = 
INSTALL= /usr/bin/ginstall -c

prefix      = /usr/local
exec_prefix = ${prefix}
bindir      = ${exec_prefix}/bin

all :hs

hs:
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEFS) hailscan.c -o hs   	
clean:
	rm -f *~ hs *.log *.cache config.h 
install:
	$(INSTALL) -c -m 0755 -g bin ./hs $(bindir)
