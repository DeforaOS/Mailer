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



#include <string.h>
#include "../src/account/imap4.c"


/* imap4 */
static int _imap4_status(char const * progname, char const * title,
		IMAP4 * imap4, char const * status)
{
	printf("%s: Testing %s\n", progname, title);
	return _context_status(imap4, status);
}


/* main */
int main(int argc, char * argv[])
{
	int ret = 0;
	IMAP4 imap4;
	char const status[] = "STATUS \"folder\""
		" (MESSAGES 3 RECENT 2 UNSEEN 1)";

	memset(&imap4, 0, sizeof(imap4));
	ret |= _imap4_status(argv[0], "STATUS (1/4)", &imap4, status);
	ret |= _imap4_status(argv[0], "STATUS (2/4)", &imap4, "()");
	ret |= _imap4_status(argv[0], "STATUS (3/4)", &imap4, "( )");
	ret |= _imap4_status(argv[0], "STATUS (4/4)", &imap4, "");
	return (ret == 0) ? 0 : 2;
}
