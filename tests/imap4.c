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


/* imap4_list */
static int _imap4_list(char const * progname, char const * title,
		IMAP4 * imap4, char const * list)
{
	int ret;
	IMAP4Command cmd;
	AccountFolder folder;

	printf("%s: Testing %s\n", progname, title);
	memset(&cmd, 0, sizeof(cmd));
	cmd.data.list.parent = &folder;
	memset(&folder, 0, sizeof(folder));
	imap4->queue = &cmd;
	imap4->queue_cnt = 1;
	ret = _context_list(imap4, list);
	imap4->queue = NULL;
	imap4->queue_cnt = 0;
	return ret;
}


/* imap4_status */
static int _imap4_status(char const * progname, char const * title,
		IMAP4 * imap4, char const * status)
{
	printf("%s: Testing %s\n", progname, title);
	return _context_status(imap4, status);
}


/* helpers */
static Folder * _helper_folder_new(Account * account, AccountFolder * folder,
		Folder * parent, FolderType type, char const * Name)
{
	static AccountFolder af;

	memset(&af, 0, sizeof(af));
	return &af;
}


/* main */
int main(int argc, char * argv[])
{
	int ret = 0;
	AccountPluginHelper helper;
	IMAP4 imap4;
	char const list[] = "LIST (\\Noselect \\No \\Yes) \"/\" ~/Mail/foo";
	char const status[] = "STATUS \"~/Mail/foo\""
		" (MESSAGES 3 RECENT 2 UNSEEN 1)";

	memset(&helper, 0, sizeof(helper));
	helper.folder_new = _helper_folder_new;
	memset(&imap4, 0, sizeof(imap4));
	imap4.helper = &helper;
	ret |= _imap4_list(argv[0], "LIST (1/1)", &imap4, list);
	ret |= _imap4_status(argv[0], "STATUS (1/4)", &imap4, status);
	ret |= _imap4_status(argv[0], "STATUS (2/4)", &imap4, "()");
	ret |= _imap4_status(argv[0], "STATUS (3/4)", &imap4, "( )");
	ret |= _imap4_status(argv[0], "STATUS (4/4)", &imap4, "");
	return (ret == 0) ? 0 : 2;
}
