/* $Id$ */
/* Copyright (c) 2012-2015 Pierre Pronchery <khorben@defora.org> */
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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define _AccountFolder _MailerFolder
#define _AccountMessage _MailerMessage
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
		Folder * parent, FolderType type, char const * name);
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
	imap4->channel = (GIOChannel *)-1; /* XXX */
	imap4->queue = cmd;
	imap4->queue_cnt = 1;
	if((ret = _parse_context(imap4, fetch)) == 0)
		ret = (cmd->data.fetch.size == size) ? 0
			: -error_set_print(progname, 1, "%s", "Wrong size");
	imap4->channel = NULL;
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
	AccountMessage message;

	printf("%s: Testing %s\n", progname, title);
	if((cmd = malloc(sizeof(*cmd))) == NULL)
		return -1;
	memset(cmd, 0, sizeof(*cmd));
	cmd->context = I4C_FETCH;
	cmd->data.fetch.folder = &folder;
	cmd->data.fetch.message = &message;
	cmd->data.fetch.id = id;
	cmd->data.fetch.status = I4FS_FLAGS;
	memset(&folder, 0, sizeof(folder));
	memset(&message, 0, sizeof(message));
	imap4->channel = (GIOChannel *)-1; /* XXX */
	imap4->queue = cmd;
	imap4->queue_cnt = 1;
	ret = _context_fetch(imap4, flags);
	imap4->channel = NULL;
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
	imap4->channel = (GIOChannel *)-1; /* XXX */
	imap4->queue = cmd;
	imap4->queue_cnt = 1;
	ret = _parse_context(imap4, list);
	imap4->channel = NULL;
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
		Folder * parent, FolderType type, char const * name)
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


/* helper_message_set_flag */
static void _helper_message_set_flag(Message * message, MailerMessageFlag flag)
{
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
	helper.message_set_flag = _helper_message_set_flag;
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
