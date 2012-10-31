/* $Id$ */
/* Copyright (c) 2012 Pierre Pronchery <khorben@defora.org> */
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



#include <stdio.h>
#include <string.h>
#include "Mailer.h"


/* date */
static int _date(char const * progname, char const * date, char const * str)
{
	struct tm tm;
	char buf[32];

	printf("%s: Testing \"%s\"\n", progname, str);
	mailer_helper_get_date(str, &tm);
	strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &tm);
	if(strcmp(buf, date) != 0)
	{
		fprintf(stderr, "%s: %s: %s\n", progname, buf,
				"Does not match the date");
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
	return ret;
}
