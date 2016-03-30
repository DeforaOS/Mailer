/* $Id$ */
/* Copyright (c) 2015-2016 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Mailer */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */



#include <string.h>
#include <netdb.h>


/* prototypes */
static int _common_lookup(char const * hostname, uint16_t port,
		struct addrinfo ** ai);


/* functions */
/* common_lookup */
static int _common_lookup(char const * hostname, uint16_t port,
		struct addrinfo ** ai)
{
	struct addrinfo hints;
	int res;
	char buf[6];

	if(hostname == NULL)
		return -error_set_code(1, "%s", strerror(errno));
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_NUMERICSERV;
	snprintf(buf, sizeof(buf), "%hu", port);
	if((res = getaddrinfo(hostname, buf, &hints, ai)) != 0)
		return -error_set_code(1, "%s", gai_strerror(res));
	return 0;
}


/* common_lookup_print */
static char * _common_lookup_print(struct addrinfo * ai)
{
	char buf[128];
	struct sockaddr_in * sin;
	struct sockaddr_in6 * sin6;

	switch(ai->ai_family)
	{
		case AF_INET:
			sin = (struct sockaddr_in *)ai->ai_addr;
			if(inet_ntop(ai->ai_family, &sin->sin_addr, buf,
						sizeof(buf)) == NULL)
				return NULL;
			break;
		case AF_INET6:
			sin6 = (struct sockaddr_in6 *)ai->ai_addr;
			if(inet_ntop(ai->ai_family, &sin6->sin6_addr, buf,
						sizeof(buf)) == NULL)
				return NULL;
			break;
		default:
			return NULL;
	}
	return strdup(buf);
}
