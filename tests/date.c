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
#include "../src/helper.c"


/* date */
static int _date(char const * progname, char const * expected, char const * str)
{
	struct tm tm;
	char buf[32];

	printf("%s: Testing \"%s\"\n", progname, str);
	mailer_helper_get_date(str, &tm);
	strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &tm);
	if(expected == NULL)
		return 0;
	if(strcmp(buf, expected) != 0)
	{
		fprintf(stderr, "%s: %s: %s\n", progname, buf,
				"Does not match the expected date");
		return 1;
	}
	return 0;
}


/* main */
int main(int argc, char * argv[])
{
	int ret = 0;
	char const expected[] = "10/11/2011 10:11:12";

	ret += _date(argv[0], expected, expected);
	ret += _date(argv[0], expected, "10 Nov 2011 10:11:12 -0000");
	ret += _date(argv[0], expected,
			"Thu, 10 Nov 2011 10:11:12 -0000 (CET)");
	ret += _date(argv[0], expected,
			"Thu, 10 Nov 2011 10:11:12 +0000");
	ret += _date(argv[0], NULL, "");
	ret += _date(argv[0], NULL, NULL);
	return (ret == 0) ? 0 : ret + 1;
}
