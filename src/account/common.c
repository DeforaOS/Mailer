/* $Id$ */
/* Copyright (c) 2015 Pierre Pronchery <khorben@defora.org> */
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
