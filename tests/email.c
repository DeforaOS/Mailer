/* $Id$ */
/* Copyright (c) 2012 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Mailer */
/* All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */



#include <stdio.h>
#include <string.h>
#include "Mailer.h"


/* email */
static int _email(char const * progname, char const * name, char const * email,
		char const * str)
{
	int ret = 0;
	char * n;
	char * e;

	printf("%s: Testing \"%s\"\n", progname, str);
	n = mailer_helper_get_name(str);
	e = mailer_helper_get_email(str);
	if(n == NULL || strcmp(name, n) != 0)
	{
		fprintf(stderr, "%s: %s: %s\n", progname, n,
				"Does not match the name");
		ret += 1;
	}
	if(e == NULL || strcmp(email, e) != 0)
	{
		fprintf(stderr, "%s: %s: %s\n", progname, e,
				"Does not match the e-mail");
		ret += 1;
	}
	free(e);
	free(n);
	return ret;
}


/* main */
int main(int argc, char * argv[])
{
	int ret = 0;

	ret += _email(argv[0], "john@doe.com", "john@doe.com", "john@doe.com");
	ret += _email(argv[0], "john@doe.com", "john@doe.com",
			"<john@doe.com>");
	ret += _email(argv[0], "John Doe", "john@doe.com",
			"john@doe.com (John Doe)");
	ret += _email(argv[0], "John Doe", "john@doe.com",
			"John Doe <john@doe.com>");
	ret += _email(argv[0], "John Doe", "john@doe.com",
			"\"John Doe\" <john@doe.com>");
	ret += _email(argv[0], "John Doe", "john@doe.com",
			"'John Doe' <john@doe.com>");
	return ret;
}
