/* $Id$ */
/* Copyright (c) 2015-2016 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Mailer */
/* Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the authors nor the names of the contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. */



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
	char buf2[128];
	struct sockaddr_in * sin;
	struct sockaddr_in6 * sin6;

	switch(ai->ai_family)
	{
		case AF_INET:
			sin = (struct sockaddr_in *)ai->ai_addr;
			if(inet_ntop(ai->ai_family, &sin->sin_addr, buf,
						sizeof(buf)) == NULL)
				return NULL;
			snprintf(buf2, sizeof(buf2), "%s:%hu", buf,
					ntohs(sin->sin_port));
			break;
		case AF_INET6:
			sin6 = (struct sockaddr_in6 *)ai->ai_addr;
			if(inet_ntop(ai->ai_family, &sin6->sin6_addr, buf,
						sizeof(buf)) == NULL)
				return NULL;
			snprintf(buf2, sizeof(buf2), "[%s]:%hu", buf,
					ntohs(sin6->sin6_port));
			break;
		default:
			return NULL;
	}
	return strdup(buf2);
}
