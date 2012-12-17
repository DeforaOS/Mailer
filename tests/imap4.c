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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../src/account/imap4.c"


/* prototypes */
static int _imap4_fetch(char const * progname, char const * title,
		IMAP4 * imap4, unsigned int id, char const * fetch,
		unsigned int size);
static int _imap4_flags(char const * progname, char const * title,
		IMAP4 * imap4, unsigned int id, char const * flags);
static int _imap4_list(char const * progname, char const * title,
		IMAP4 * imap4, char const * list);
static int _imap4_status(char const * progname, char const * title,
		IMAP4 * imap4, char const * status);

/* helpers */
static void _helper_event(Account * account, AccountEvent * event);
static Folder * _helper_folder_new(Account * account, AccountFolder * folder,
		Folder * parent, FolderType type, char const * Name);
static Message * _helper_message_new(Account * account, Folder * folder,
		AccountMessage * message);


/* functions */
/* imap4_fetch */
static int _imap4_fetch(char const * progname, char const * title,
		IMAP4 * imap4, unsigned int id, char const * fetch,
		unsigned int size)
{
	int ret;
	IMAP4Command * cmd;
	AccountFolder folder;

	printf("%s: Testing %s\n", progname, title);
	if((cmd = malloc(sizeof(*cmd))) == NULL)
		return -1;
	memset(cmd, 0, sizeof(*cmd));
	cmd->context = I4C_FETCH;
	cmd->data.fetch.folder = &folder;
	cmd->data.fetch.id = id;
	cmd->data.fetch.status = I4FS_ID;
	memset(&folder, 0, sizeof(folder));
	imap4->channel = -1; /* XXX */
	imap4->queue = cmd;
	imap4->queue_cnt = 1;
	if((ret = _parse_context(imap4, fetch)) == 0)
		ret = (cmd->data.fetch.size == size) ? 0
			: -error_set_print(progname, 1, "%s", "Wrong size");
	imap4->channel = 0; /* XXX */
	_imap4_stop(imap4);
	return ret;
}


/* imap4_flags */
static int _imap4_flags(char const * progname, char const * title,
		IMAP4 * imap4, unsigned int id, char const * flags)
{
	int ret;
	IMAP4Command * cmd;
	AccountFolder folder;

	printf("%s: Testing %s\n", progname, title);
	if((cmd = malloc(sizeof(*cmd))) == NULL)
		return -1;
	memset(cmd, 0, sizeof(*cmd));
	cmd->context = I4C_FETCH;
	cmd->data.fetch.folder = &folder;
	cmd->data.fetch.id = id;
	cmd->data.fetch.status = I4FS_FLAGS;
	memset(&folder, 0, sizeof(folder));
	imap4->channel = -1; /* XXX */
	imap4->queue = cmd;
	imap4->queue_cnt = 1;
	ret = _context_fetch(imap4, flags);
	imap4->channel = 0; /* XXX */
	_imap4_stop(imap4);
	return ret;
}


/* imap4_list */
static int _imap4_list(char const * progname, char const * title,
		IMAP4 * imap4, char const * list)
{
	int ret;
	IMAP4Command * cmd;
	AccountFolder folder;

	printf("%s: Testing %s\n", progname, title);
	if((cmd = malloc(sizeof(*cmd))) == NULL)
		return -1;
	memset(cmd, 0, sizeof(*cmd));
	cmd->context = I4C_LIST;
	cmd->data.list.parent = &folder;
	memset(&folder, 0, sizeof(folder));
	imap4->channel = -1; /* XXX */
	imap4->queue = cmd;
	imap4->queue_cnt = 1;
	ret = _parse_context(imap4, list);
	imap4->channel = 0; /* XXX */
	_imap4_stop(imap4);
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
/* helper_event */
static void _helper_event(Account * account, AccountEvent * event)
{
}


/* helper_folder_new */
static Folder * _helper_folder_new(Account * account, AccountFolder * folder,
		Folder * parent, FolderType type, char const * Name)
{
	static AccountFolder af;

	memset(&af, 0, sizeof(af));
	return &af;
}


/* helper_message_new */
static Message * _helper_message_new(Account * account, Folder * folder,
		AccountMessage * message)
{
	static AccountMessage am;

	memset(&am, 0, sizeof(am));
	return &am;
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
	unsigned int fetch_id = 12;
	char const fetch[] = "12 FETCH (RFC822 {1024}";
	unsigned int fetch_size = 1024;
	unsigned int flags_id = 12;
	char const flags[] = "FLAGS (\\Seen \\Answered))";

	memset(&helper, 0, sizeof(helper));
	helper.event = _helper_event;
	helper.folder_new = _helper_folder_new;
	helper.message_new = _helper_message_new;
	memset(&imap4, 0, sizeof(imap4));
	imap4.helper = &helper;
	ret |= _imap4_list(argv[0], "LIST (1/1)", &imap4, list);
	ret |= _imap4_status(argv[0], "STATUS (1/4)", &imap4, status);
	ret |= _imap4_status(argv[0], "STATUS (2/4)", &imap4, "()");
	ret |= _imap4_status(argv[0], "STATUS (3/4)", &imap4, "( )");
	ret |= _imap4_status(argv[0], "STATUS (4/4)", &imap4, "");
	ret |= _imap4_fetch(argv[0], "FETCH (1/1)", &imap4, fetch_id, fetch,
			fetch_size);
	ret |= _imap4_flags(argv[0], "FLAGS (1/1)", &imap4, flags_id, flags);
	return (ret == 0) ? 0 : 2;
}
