/* $Id$ */
/* Copyright (c) 2011-2024 Pierre Pronchery <khorben@defora.org> */
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
/* FIXME:
 * - do not erroneously parse body/header data as potential command completion
 * - do not fetch messages when there are none available in the first place
 * - openssl should be more explicit when SSL_set_fd() is missing (no BIO)
 * - support multiple connections? */



#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <glib.h>
#include <System.h>
#include "Mailer/account.h"
#include "common.c"


/* IMAP4 */
/* private */
/* types */
struct _AccountFolder
{
	Folder * folder;

	char * name;

	AccountMessage ** messages;
	size_t messages_cnt;

	AccountFolder ** folders;
	size_t folders_cnt;
};

struct _AccountMessage
{
	Message * message;

	unsigned int id;
};

typedef enum _IMAP4CommandStatus
{
	I4CS_QUEUED = 0,
	I4CS_SENT,
	I4CS_ERROR,
	I4CS_PARSING,
	I4CS_OK
} IMAP4CommandStatus;

typedef enum _IMAP4ConfigValue
{
	I4CV_USERNAME = 0,
	I4CV_PASSWORD,
	I4CV_HOSTNAME,
	I4CV_PORT,
	I4CV_SSL,
	I4CV_PADDING0,
	I4CV_PREFIX
} IMAP4Config;
#define I4CV_LAST I4CV_PREFIX
#define I4CV_COUNT (I4CV_LAST + 1)

typedef enum _IMAP4Context
{
	I4C_INIT = 0,
	I4C_FETCH,
	I4C_LIST,
	I4C_LOGIN,
	I4C_NOOP,
	I4C_SELECT,
	I4C_STATUS
} IMAP4Context;

typedef enum _IMAP4FetchStatus
{
	I4FS_ID = 0,
	I4FS_COMMAND,
	I4FS_FLAGS,
	I4FS_HEADERS,
	I4FS_BODY
} IMAP4FetchStatus;

typedef struct _IMAP4Command
{
	uint16_t id;
	IMAP4CommandStatus status;
	IMAP4Context context;
	char * buf;
	size_t buf_cnt;

	union
	{
		struct
		{
			AccountFolder * folder;
			AccountMessage * message;
			unsigned int id;
			IMAP4FetchStatus status;
			unsigned int size;
		} fetch;

		struct
		{
			AccountFolder * parent;
		} list;

		struct
		{
			AccountFolder * folder;
			AccountMessage * message;
		} select;

		struct
		{
			AccountFolder * folder;
		} status;
	} data;
} IMAP4Command;

typedef struct _AccountPlugin
{
	AccountPluginHelper * helper;

	AccountConfig * config;

	struct addrinfo * ai;
	struct addrinfo * aip;
	int fd;
	SSL * ssl;
	guint source;

	GIOChannel * channel;
	char * rd_buf;
	size_t rd_buf_cnt;
	guint rd_source;
	guint wr_source;

	IMAP4Command * queue;
	size_t queue_cnt;
	uint16_t queue_id;

	AccountFolder folders;
} IMAP4;


/* variables */
static char const _imap4_type[] = "IMAP4";
static char const _imap4_name[] = "IMAP4 server";

AccountConfig const _imap4_config[I4CV_COUNT + 1] =
{
	{ "username",	"Username",		ACT_STRING,	NULL	},
	{ "password",	"Password",		ACT_PASSWORD,	NULL	},
	{ "hostname",	"Server hostname",	ACT_STRING,	NULL	},
	{ "port",	"Server port",		ACT_UINT16,	(void *)143 },
	{ "ssl",	"Use SSL",		ACT_BOOLEAN,	0	},
#if 0 /* XXX not implemented yet */
	{ "sent",	"Sent mails folder",	ACT_NONE,	NULL	},
	{ "draft",	"Draft mails folder",	ACT_NONE,	NULL	},
#endif
	{ NULL,		NULL,			ACT_SEPARATOR,	NULL	},
	{ "prefix",	"Prefix",		ACT_STRING,	NULL	},
	{ NULL,		NULL,			ACT_NONE,	NULL	}
};


/* prototypes */
/* plug-in */
static IMAP4 * _imap4_init(AccountPluginHelper * helper);
static int _imap4_destroy(IMAP4 * imap4);
static AccountConfig * _imap4_get_config(IMAP4 * imap4);
static int _imap4_start(IMAP4 * imap4);
static void _imap4_stop(IMAP4 * imap4);
static int _imap4_refresh(IMAP4 * imap4, AccountFolder * folder,
		AccountMessage * message);

/* useful */
static IMAP4Command * _imap4_command(IMAP4 * imap4, IMAP4Context context,
		char const * command);
static int _imap4_parse(IMAP4 * imap4);

/* events */
static void _imap4_event(IMAP4 * imap4, AccountEventType type);
static void _imap4_event_status(IMAP4 * imap4, AccountStatus status,
		char const * message);

/* folders */
static AccountFolder * _imap4_folder_new(IMAP4 * imap4, AccountFolder * parent,
		char const * name);
static void _imap4_folder_delete(IMAP4 * imap4,
		AccountFolder * folder);
static AccountFolder * _imap4_folder_get_folder(IMAP4 * imap4,
		AccountFolder * folder, char const * name);
static AccountMessage * _imap4_folder_get_message(IMAP4 * imap4,
		AccountFolder * folder, unsigned int id);

/* messages */
static AccountMessage * _imap4_message_new(IMAP4 * imap4,
		AccountFolder * folder, unsigned int id);
static void _imap4_message_delete(IMAP4 * imap4,
		AccountMessage * message);

/* callbacks */
static gboolean _on_connect(gpointer data);
static gboolean _on_noop(gpointer data);
static gboolean _on_watch_can_connect(GIOChannel * source,
		GIOCondition condition, gpointer data);
static gboolean _on_watch_can_handshake(GIOChannel * source,
		GIOCondition condition, gpointer data);
static gboolean _on_watch_can_read(GIOChannel * source, GIOCondition condition,
		gpointer data);
static gboolean _on_watch_can_read_ssl(GIOChannel * source,
		GIOCondition condition, gpointer data);
static gboolean _on_watch_can_write(GIOChannel * source, GIOCondition condition,
		gpointer data);
static gboolean _on_watch_can_write_ssl(GIOChannel * source,
		GIOCondition condition, gpointer data);


/* public */
/* variables */
AccountPluginDefinition account_plugin =
{
	_imap4_type,
	_imap4_name,
	NULL,
	NULL,
	_imap4_config,
	_imap4_init,
	_imap4_destroy,
	_imap4_get_config,
	NULL,
	_imap4_start,
	_imap4_stop,
	_imap4_refresh
};


/* protected */
/* functions */
/* plug-in */
/* imap4_init */
static IMAP4 * _imap4_init(AccountPluginHelper * helper)
{
	IMAP4 * imap4;

	if((imap4 = malloc(sizeof(*imap4))) == NULL)
		return NULL;
	memset(imap4, 0, sizeof(*imap4));
	imap4->helper = helper;
	if((imap4->config = malloc(sizeof(_imap4_config))) == NULL)
	{
		free(imap4);
		return NULL;
	}
	memcpy(imap4->config, &_imap4_config, sizeof(_imap4_config));
	imap4->ai = NULL;
	imap4->aip = NULL;
	imap4->fd = -1;
	return imap4;
}


/* imap4_destroy */
static int _imap4_destroy(IMAP4 * imap4)
{
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(imap4 == NULL) /* XXX _imap4_destroy() may be called uninitialized */
		return 0;
	_imap4_stop(imap4);
#if 1 /* XXX anything wrong here? */
	_imap4_folder_delete(imap4, &imap4->folders);
#endif
	for(i = 0; i < sizeof(_imap4_config) / sizeof(*_imap4_config); i++)
		if(_imap4_config[i].type == ACT_STRING
				|| _imap4_config[i].type == ACT_PASSWORD)
			free(imap4->config[i].value);
	free(imap4->config);
	free(imap4);
	return 0;
}


/* imap4_get_config */
static AccountConfig * _imap4_get_config(IMAP4 * imap4)
{
	return imap4->config;
}


/* imap4_start */
static int _imap4_start(IMAP4 * imap4)
{
	_imap4_event(imap4, AET_STARTED);
	if(imap4->fd >= 0 || imap4->source != 0)
		/* already started */
		return 0;
	imap4->source = g_idle_add(_on_connect, imap4);
	return 0;
}


/* imap4_stop */
static void _imap4_stop(IMAP4 * imap4)
{
	size_t i;

	if(imap4->ssl != NULL)
		SSL_free(imap4->ssl);
	imap4->ssl = NULL;
	if(imap4->rd_source != 0)
		g_source_remove(imap4->rd_source);
	imap4->rd_source = 0;
	if(imap4->wr_source != 0)
		g_source_remove(imap4->wr_source);
	imap4->wr_source = 0;
	if(imap4->source != 0)
		g_source_remove(imap4->source);
	imap4->source = 0;
	if(imap4->channel != NULL)
	{
		g_io_channel_shutdown(imap4->channel, TRUE, NULL);
		g_io_channel_unref(imap4->channel);
		imap4->fd = -1;
	}
	imap4->channel = NULL;
	for(i = 0; i < imap4->queue_cnt; i++)
		free(imap4->queue[i].buf);
	free(imap4->queue);
	imap4->queue = NULL;
	imap4->queue_cnt = 0;
	if(imap4->fd >= 0)
		close(imap4->fd);
	imap4->fd = -1;
	imap4->aip = NULL;
	if(imap4->ai != NULL)
		freeaddrinfo(imap4->ai);
	imap4->ai = NULL;
	_imap4_event(imap4, AET_STOPPED);
}


/* imap4_refresh */
static int _imap4_refresh(IMAP4 * imap4, AccountFolder * folder,
		AccountMessage * message)
{
	IMAP4Command * cmd;
	int len;
	char * buf;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %u\n", __func__, (message != NULL)
			? message->id : 0);
#endif
	if((len = snprintf(NULL, 0, "EXAMINE \"%s\"", folder->name)) < 0
			|| (buf = malloc(++len)) == NULL)
		return -1;
	snprintf(buf, len, "EXAMINE \"%s\"", folder->name);
	cmd = _imap4_command(imap4, I4C_SELECT, buf);
	free(buf);
	if(cmd == NULL)
		return -1;
	cmd->data.select.folder = folder;
	cmd->data.select.message = message;
	return 0;
}


/* private */
/* functions */
/* useful */
/* imap4_command */
static IMAP4Command * _imap4_command(IMAP4 * imap4, IMAP4Context context,
		char const * command)
{
	IMAP4Command * p;
	size_t len;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", %p)\n", __func__, command,
			(void *)imap4->channel);
#endif
	/* abort if the command is invalid */
	if(command == NULL || (len = strlen(command)) == 0)
		return NULL;
	/* abort if there is no active connection */
	if(imap4->channel == NULL)
		return NULL;
	/* queue the command */
	len += 9;
	if((p = realloc(imap4->queue, sizeof(*p) * (imap4->queue_cnt + 1)))
			== NULL)
		return NULL;
	imap4->queue = p;
	p = &imap4->queue[imap4->queue_cnt];
	p->id = imap4->queue_id++;
	p->context = context;
	p->status = I4CS_QUEUED;
	if((p->buf = malloc(len)) == NULL)
		return NULL;
	p->buf_cnt = snprintf(p->buf, len, "a%04x %s\r\n", p->id, command);
	memset(&p->data, 0, sizeof(p->data));
	if(imap4->queue_cnt++ == 0)
	{
		if(imap4->source != 0)
		{
			/* cancel the pending NOOP operation */
			g_source_remove(imap4->source);
			imap4->source = 0;
		}
		imap4->wr_source = g_io_add_watch(imap4->channel, G_IO_OUT,
				(imap4->ssl != NULL) ? _on_watch_can_write_ssl
				: _on_watch_can_write, imap4);
	}
	return p;
}


/* imap4_parse */
static int _parse_context(IMAP4 * imap4, char const * answer);
static int _context_fetch(IMAP4 * imap4, char const * answer);
static int _context_fetch_body(IMAP4 * imap4, char const * answer);
static int _context_fetch_command(IMAP4 * imap4, char const * answer);
static int _context_fetch_flags(IMAP4 * imap4, char const * answer);
static int _context_fetch_headers(IMAP4 * imap4, char const * answer);
static int _context_fetch_id(IMAP4 * imap4, char const * answer);
static int _context_init(IMAP4 * imap4);
static int _context_list(IMAP4 * imap4, char const * answer);
static int _context_login(IMAP4 * imap4, char const * answer);
static int _context_select(IMAP4 * imap4);
static int _context_status(IMAP4 * imap4, char const * answer);

static int _imap4_parse(IMAP4 * imap4)
{
	AccountPluginHelper * helper = imap4->helper;
	size_t i;
	size_t j;
	IMAP4Command * cmd;
	char buf[8];

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	for(i = 0, j = 0;; j = ++i)
	{
		/* look for carriage return sequences */
		for(; i < imap4->rd_buf_cnt; i++)
			if(imap4->rd_buf[i] == '\r' && i + 1 < imap4->rd_buf_cnt
					&& imap4->rd_buf[++i] == '\n')
				break;
		/* if no carriage return was found wait for more input */
		if(i == imap4->rd_buf_cnt)
			break;
		/* if no command was sent read more lines */
		if(imap4->queue_cnt == 0)
			continue;
		imap4->rd_buf[i - 1] = '\0';
		cmd = &imap4->queue[0];
		/* if we have just sent a command match the answer */
		if(cmd->status == I4CS_SENT)
		{
			snprintf(buf, sizeof(buf), "a%04x ", cmd->id);
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() expecting \"%s\"\n",
					__func__, buf);
#endif
			if(strncmp(&imap4->rd_buf[j], "* ", 2) == 0)
				j += 2;
			else if(strncmp(&imap4->rd_buf[j], buf, 6) == 0)
			{
				j += 6;
				cmd->status = I4CS_PARSING;
				if(strncmp("BAD ", &imap4->rd_buf[j], 4) == 0)
					/* FIXME report and pop queue instead */
					helper->error(NULL,
							&imap4->rd_buf[j + 4],
							1);
			}
		}
		if(_parse_context(imap4, &imap4->rd_buf[j]) != 0)
			cmd->status = I4CS_ERROR;
	}
	if(j != 0)
	{
		imap4->rd_buf_cnt -= j;
		memmove(imap4->rd_buf, &imap4->rd_buf[j], imap4->rd_buf_cnt);
	}
	return 0;
}

static int _parse_context(IMAP4 * imap4, char const * answer)
{
	int ret = -1;
	IMAP4Command * cmd = &imap4->queue[0];

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\") %u, %u\n", __func__, answer,
			cmd->context, cmd->status);
#endif
	switch(cmd->context)
	{
		case I4C_FETCH:
			return _context_fetch(imap4, answer);
		case I4C_INIT:
			return _context_init(imap4);
		case I4C_LIST:
			return _context_list(imap4, answer);
		case I4C_LOGIN:
			return _context_login(imap4, answer);
		case I4C_NOOP:
			if(cmd->status != I4CS_PARSING)
				return 0;
			cmd->status = I4CS_OK;
			return 0;
		case I4C_SELECT:
			return _context_select(imap4);
		case I4C_STATUS:
			return _context_status(imap4, answer);
	}
	return ret;
}

static int _context_fetch(IMAP4 * imap4, char const * answer)
{
	IMAP4Command * cmd = &imap4->queue[0];

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, answer);
#endif
	if(cmd->status == I4CS_PARSING)
	{
		cmd->status = I4CS_OK;
		return 0;
	}
	switch(cmd->data.fetch.status)
	{
		case I4FS_ID:
			return _context_fetch_id(imap4, answer);
		case I4FS_COMMAND:
			return _context_fetch_command(imap4, answer);
		case I4FS_FLAGS:
			return _context_fetch_flags(imap4, answer);
		case I4FS_BODY:
			return _context_fetch_body(imap4, answer);
		case I4FS_HEADERS:
			return _context_fetch_headers(imap4, answer);
	}
	return -1;
}

static int _context_fetch_body(IMAP4 * imap4, char const * answer)
{
	AccountPluginHelper * helper = imap4->helper;
	IMAP4Command * cmd = &imap4->queue[0];
	AccountMessage * message = cmd->data.fetch.message;
	size_t i;

	/* check the size */
	if(cmd->data.fetch.size == 0)
	{
		/* XXX may not be the case (flags...) */
		if(strcmp(answer, ")") == 0)
		{
			cmd->data.fetch.status = I4FS_ID;
			return 0;
		}
		else
		{
			cmd->data.fetch.status = I4FS_COMMAND;
			return _context_fetch(imap4, answer);
		}
	}
	if((i = strlen(answer) + 2) <= cmd->data.fetch.size)
		cmd->data.fetch.size -= i;
	helper->message_set_body(message->message, answer, strlen(answer), 1);
	helper->message_set_body(message->message, "\r\n", 2, 1);
	return 0;
}

static int _context_fetch_command(IMAP4 * imap4, char const * answer)
{
	IMAP4Command * cmd = &imap4->queue[0];
	AccountFolder * folder = cmd->data.fetch.folder;
	AccountMessage * message = cmd->data.fetch.message;
	unsigned int id = cmd->data.fetch.id;
	char * p;
	size_t i;

	/* skip spaces */
	for(i = 0; answer[i] == ' '; i++);
	if(strncmp(&answer[i], "FLAGS ", 6) == 0)
	{
		cmd->data.fetch.status = I4FS_FLAGS;
		return _context_fetch(imap4, answer);
	}
	/* XXX assumes this is going to be a message content */
	/* skip the command's name */
	for(; answer[i] != '\0' && answer[i] != ' '; i++);
	/* skip spaces */
	for(; answer[i] == ' '; i++);
	/* obtain the size */
	if(answer[i] != '{')
		return -1;
	cmd->data.fetch.size = strtoul(&answer[++i], &p, 10);
	if(answer[i] == '\0' || *p != '}' || cmd->data.fetch.size == 0)
		return -1;
	if((message = _imap4_folder_get_message(imap4, folder, id)) != NULL)
	{
		cmd->data.fetch.status = I4FS_HEADERS;
		cmd->data.fetch.message = message;
	}
	return (message != NULL) ? 0 : -1;
}

static int _context_fetch_flags(IMAP4 * imap4, char const * answer)
{
	AccountPluginHelper * helper = imap4->helper;
	IMAP4Command * cmd = &imap4->queue[0];
	AccountMessage * message = cmd->data.fetch.message;
	size_t i;
	size_t j;
	size_t k;
	struct
	{
		char const * name;
		MailerMessageFlag flag;
	} flags[] = {
		{ "\\Answered",	MMF_ANSWERED	},
		{ "\\Draft",	MMF_DRAFT	}
	};

	/* skip spaces */
	for(i = 0; answer[i] == ' '; i++);
	if(strncmp(&answer[i], "FLAGS ", 6) != 0)
	{
		cmd->data.fetch.status = I4FS_ID;
		return -1;
	}
	/* skip spaces */
	for(i += 6; answer[i] == ' '; i++);
	if(answer[i] != '(')
		return -1;
	for(i++; answer[i] != '\0' && answer[i] != ')';)
	{
		for(j = i; isalpha((unsigned char)answer[j])
				|| answer[j] == '\\' || answer[j] == '$'; j++);
		/* give up if there is no flag */
		if(j == i)
		{
			for(; answer[i] != '\0' && answer[i] != ')'; i++);
			break;
		}
		/* apply the flag */
		for(k = 0; k < sizeof(flags) / sizeof(*flags); k++)
			if(strncmp(&answer[i], flags[k].name, j - i) == 0)
			{
				/* FIXME make sure message != NULL */
				if(message == NULL)
					continue;
				helper->message_set_flag(message->message,
						flags[k].flag);
			}
		/* skip spaces */
		for(i = j; answer[i] == ' '; i++);
	}
	if(answer[i] != ')')
		return -1;
	/* skip spaces */
	for(i++; answer[i] == ' '; i++);
	if(answer[i] == ')')
	{
		/* the current command seems to be completed */
		cmd->data.fetch.status = I4FS_ID;
		return 0;
	}
	cmd->data.fetch.status = I4FS_COMMAND;
	return _context_fetch(imap4, &answer[i]);
}

static int _context_fetch_headers(IMAP4 * imap4, char const * answer)
{
	AccountPluginHelper * helper = imap4->helper;
	IMAP4Command * cmd = &imap4->queue[0];
	AccountMessage * message = cmd->data.fetch.message;
	size_t i;

	/* check the size */
	if((i = strlen(answer) + 2) <= cmd->data.fetch.size)
		cmd->data.fetch.size -= i;
	if(strcmp(answer, "") == 0)
	{
		/* beginning of the body */
		cmd->data.fetch.status = I4FS_BODY;
		helper->message_set_body(message->message, NULL, 0, 0);
	}
	/* XXX check this before parsing anything */
	else if(cmd->data.fetch.size == 0 || i > cmd->data.fetch.size)
		return 0;
	else
		helper->message_set_header(message->message, answer);
	return 0;
}

static int _context_fetch_id(IMAP4 * imap4, char const * answer)
{
	IMAP4Command * cmd = &imap4->queue[0];
	unsigned int id = cmd->data.fetch.id;
	char * p;
	size_t i;

	id = strtol(answer, &p, 10);
	if(answer[0] == '\0' || *p != ' ')
		return 0;
	cmd->data.fetch.id = id;
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() id=%u\n", __func__, id);
#endif
	answer = p;
	if(strncmp(answer, " FETCH ", 7) != 0)
		return -1;
	/* skip spaces */
	for(i = 7; answer[i] == ' '; i++);
	if(answer[i++] != '(')
		return 0;
	/* fallback */
	answer = &answer[i];
	cmd->data.fetch.status = I4FS_COMMAND;
	return _context_fetch_command(imap4, answer);
}

static int _context_init(IMAP4 * imap4)
{
	IMAP4Command * cmd = &imap4->queue[0];
	char const * p;
	char const * q;
	gchar * r;

	cmd->status = I4CS_OK;
	if((p = imap4->config[I4CV_USERNAME].value) == NULL || *p == '\0')
		return -1;
	if((q = imap4->config[I4CV_PASSWORD].value) == NULL || *q == '\0')
		return -1;
	r = g_strdup_printf("%s %s %s", "LOGIN", p, q);
	cmd = _imap4_command(imap4, I4C_LOGIN, r);
	g_free(r);
	return (cmd != NULL) ? 0 : -1;
}

static int _context_list(IMAP4 * imap4, char const * answer)
{
	IMAP4Command * cmd = &imap4->queue[0];
	AccountFolder * folder;
	AccountFolder * parent = cmd->data.list.parent;
	char const * p = answer;
	gchar * q;
	char const haschildren[] = "\\HasChildren";
	int recurse = 0;
	char reference = '\0';
	char buf[64];

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, answer);
#endif
	if(strncmp("OK", p, 2) == 0)
	{
		cmd->status = I4CS_OK;
		return 0;
	}
	if(strncmp("LIST ", p, 5) != 0)
		return -1;
	p += 5;
	if(*p == '(') /* parse flags */
	{
		for(p++; *p == '\\';)
		{
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() flag \"%s\"\n", __func__,
					p);
#endif
			if(strncmp(p, haschildren, sizeof(haschildren) - 1)
					== 0)
			{
				p += sizeof(haschildren) - 1;
				recurse = 1;
			}
			else
				/* skip until end of flag */
				for(p++; isalnum((unsigned char)*p); p++);
			/* skip spaces */
			for(; *p == ' '; p++);
			if(*p == ')')
				break;
		}
		if(*p != ')')
			return -1;
		p++;
	}
	if(*p == ' ') /* skip spaces */
		for(p++; *p != '\0' && *p == ' '; p++);
	if(*p == '\"') /* skip reference */
	{
		if(p[1] != '\0' && p[1] != '"' && p[2] == '"')
			reference = p[1];
		for(p++; *p != '\0' && *p++ != '\"';);
	}
	if(*p == ' ') /* skip spaces */
		for(p++; *p != '\0' && *p == ' '; p++);
	/* read the folder name */
	buf[0] = '\0';
	if(*p == '\"')
		sscanf(++p, "%63[^\"]", buf);
	else
		sscanf(p, "%63s", buf);
	buf[63] = '\0';
	if(buf[0] != '\0' && (folder = _imap4_folder_get_folder(imap4, parent,
					buf)) != NULL
			/* FIXME escape the mailbox' name instead */
			&& strchr(buf, '"') == NULL)
	{
		q = g_strdup_printf("%s \"%s\" (%s)", "STATUS", buf,
				"MESSAGES RECENT UNSEEN");
		if((cmd = _imap4_command(imap4, I4C_STATUS, q)) != NULL)
			cmd->data.status.folder = folder;
		g_free(q);
		if(cmd != NULL && recurse == 1 && reference != '\0')
		{
			q = g_strdup_printf("%s \"\" \"%s%c%%\"", "LIST", buf,
					reference);
			if((cmd = _imap4_command(imap4, I4C_LIST, q)) != NULL)
				cmd->data.list.parent = folder;
			g_free(q);
		}
	}
	return (cmd != NULL) ? 0 : -1;
}

static int _context_login(IMAP4 * imap4, char const * answer)
{
	AccountPluginHelper * helper = imap4->helper;
	IMAP4Command * cmd = &imap4->queue[0];
	char const * prefix = imap4->config[I4CV_PREFIX].value;
	gchar * q;

	if(cmd->status != I4CS_PARSING)
		return 0;
	if(strncmp("OK", answer, 2) != 0)
		return -helper->error(helper->account, "Authentication failed",
				1);
	cmd->status = I4CS_OK;
	if((q = g_strdup_printf("%s \"\" \"%s%%\"", "LIST", (prefix != NULL)
					? prefix : "")) == NULL)
		return -1;
	cmd = _imap4_command(imap4, I4C_LIST, q);
	g_free(q);
	if(cmd == NULL)
		return -1;
	cmd->data.list.parent = &imap4->folders;
	return 0;
}

static int _context_select(IMAP4 * imap4)
{
	IMAP4Command * cmd = &imap4->queue[0];
	AccountFolder * folder;
	AccountMessage * message;
	char buf[64];

	if(cmd->status != I4CS_PARSING)
		return 0;
	cmd->status = I4CS_OK;
	if((folder = cmd->data.select.folder) == NULL)
		return 0; /* XXX really is an error */
	if((message = cmd->data.select.message) == NULL)
		/* FIXME queue commands in batches instead */
		snprintf(buf, sizeof(buf), "%s %s %s", "FETCH", "1:*",
				"(FLAGS BODY.PEEK[HEADER])");
	else
		snprintf(buf, sizeof(buf), "%s %u %s", "FETCH", message->id,
				"BODY.PEEK[]");
	if((cmd = _imap4_command(imap4, I4C_FETCH, buf)) == NULL)
		return -1;
	cmd->data.fetch.folder = folder;
	cmd->data.fetch.message = message;
	cmd->data.fetch.id = (message != NULL) ? message->id : 0;
	cmd->data.fetch.status = I4FS_ID;
	cmd->data.fetch.size = 0;
	return 0;
}

static int _context_status(IMAP4 * imap4, char const * answer)
{
	IMAP4Command * cmd = &imap4->queue[0];
	char const * p;
	char const messages[] = "MESSAGES";
	char const recent[] = "RECENT";
	char const unseen[] = "UNSEEN";
	unsigned int u;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, answer);
#endif
	p = answer;
	if(strncmp("OK", p, 2) == 0)
	{
		cmd->status = I4CS_OK;
		return 0;
	}
	else if(strncmp("NO", p, 2) == 0)
	{
		cmd->status = I4CS_ERROR;
		return 0;
	}
	if(strncmp("STATUS ", p, 7) != 0)
		return 0;
	p += 7;
	if(*p == '\"') /* skip reference */
		for(p++; *p != '\0' && *p++ != '\"';);
	if(*p == ' ') /* skip spaces */
		for(p++; *p != '\0' && *p == ' '; p++);
	if(*p == '(')
		/* parse the data items */
		for(p++; *p != '\0' && *p != ')'; p++)
		{
			if(strncmp(p, messages, sizeof(messages) - 1) == 0
					&& p[sizeof(messages) - 1] == ' ')
			{
				/* number of messages in the mailbox */
				p += sizeof(messages);
				sscanf(p, "%u", &u);
				/* FIXME really implement */
#ifdef DEBUG
				fprintf(stderr, "DEBUG: %s() %u messages\n",
						__func__, u);
#endif
			}
			else if(strncmp(p, recent, sizeof(recent) - 1) == 0
					&& p[sizeof(recent) - 1] == ' ')
			{
				/* number of recent messages in the mailbox */
				p += sizeof(recent);
				sscanf(p, "%u", &u);
				/* FIXME really implement */
#ifdef DEBUG
				fprintf(stderr, "DEBUG: %s() %u recent\n",
						__func__, u);
#endif
			}
			else if(strncmp(p, unseen, sizeof(unseen) - 1) == 0
					&& p[sizeof(unseen) - 1] == ' ')
			{
				/* number of unseen messages in the mailbox */
				p += sizeof(unseen);
				sscanf(p, "%u", &u);
				/* FIXME really implement */
#ifdef DEBUG
				fprintf(stderr, "DEBUG: %s() %u unseen\n",
						__func__, u);
#endif
			}
			else
				/* skip until the next space */
				for(; *p != '\0' && *p != ' ' && *p != ')';
						p++);
			/* skip until the next space */
			for(; *p != '\0' && *p != ' ' && *p != ')'; p++);
		}
	return 0;
}


/* imap4_event */
static void _imap4_event(IMAP4 * imap4, AccountEventType type)
{
	AccountPluginHelper * helper = imap4->helper;
	AccountEvent event;

	memset(&event, 0, sizeof(event));
	switch((event.status.type = type))
	{
		case AET_STARTED:
		case AET_STOPPED:
			break;
		default:
			/* XXX not handled */
			return;
	}
	helper->event(helper->account, &event);
}


/* imap4_event_status */
static void _imap4_event_status(IMAP4 * imap4, AccountStatus status,
		char const * message)
{
	AccountPluginHelper * helper = imap4->helper;
	AccountEvent event;

	memset(&event, 0, sizeof(event));
	event.status.type = AET_STATUS;
	event.status.status = status;
	event.status.message = message;
	helper->event(helper->account, &event);
}


/* imap4_folder_new */
static AccountFolder * _imap4_folder_new(IMAP4 * imap4, AccountFolder * parent,
		char const * name)
{
	AccountPluginHelper * helper = imap4->helper;
	AccountFolder * folder;
	AccountFolder ** p;
	FolderType type = FT_FOLDER;
	struct
	{
		char const * name;
		FolderType type;
	} name_type[] =
	{
		{ "Inbox",	FT_INBOX	},
		{ "Drafts",	FT_DRAFTS	},
		{ "Trash",	FT_TRASH	},
		{ "Sent",	FT_SENT		}
	};
	size_t i;
	size_t len;

	if((p = realloc(parent->folders, sizeof(*p) * (parent->folders_cnt
						+ 1))) == NULL)
		return NULL;
	parent->folders = p;
	if((folder = object_new(sizeof(*folder))) == NULL)
		return NULL;
	folder->name = strdup(name);
	if(parent == &imap4->folders)
		for(i = 0; i < sizeof(name_type) / sizeof(*name_type); i++)
			if(strcasecmp(name_type[i].name, name) == 0)
			{
				type = name_type[i].type;
				name = name_type[i].name;
				break;
			}
	/* shorten the name as required if has a parent */
	if(parent->name != NULL && (len = strlen(parent->name)) > 0
			&& strncmp(name, parent->name, len) == 0)
		name = &name[len + 1];
	folder->folder = helper->folder_new(helper->account, folder,
			parent->folder, type, name);
	folder->messages = NULL;
	folder->messages_cnt = 0;
	folder->folders = NULL;
	folder->folders_cnt = 0;
	if(folder->folder == NULL || folder->name == NULL)
	{
		_imap4_folder_delete(imap4, folder);
		return NULL;
	}
	parent->folders[parent->folders_cnt++] = folder;
	return folder;
}


/* imap4_folder_delete */
static void _imap4_folder_delete(IMAP4 * imap4, AccountFolder * folder)
{
	size_t i;

	if(folder->folder != NULL)
		imap4->helper->folder_delete(folder->folder);
	free(folder->name);
	for(i = 0; i < folder->messages_cnt; i++)
		_imap4_message_delete(imap4, folder->messages[i]);
	free(folder->messages);
	for(i = 0; i < folder->folders_cnt; i++)
		_imap4_folder_delete(imap4, folder->folders[i]);
	free(folder->folders);
	/* XXX rather ugly */
	if(folder != &imap4->folders)
		object_delete(folder);
}


/* imap4_folder_get_folder */
static AccountFolder * _imap4_folder_get_folder(IMAP4 * imap4,
		AccountFolder * folder, char const * name)
{
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", \"%s\")\n", __func__, folder->name,
			name);
#endif
	for(i = 0; i < folder->folders_cnt; i++)
		if(strcmp(folder->folders[i]->name, name) == 0)
			return folder->folders[i];
	return _imap4_folder_new(imap4, folder, name);
}


/* imap4_folder_get_message */
static AccountMessage * _imap4_folder_get_message(IMAP4 * imap4,
		AccountFolder * folder, unsigned int id)
{
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\", %u)\n", __func__, folder->name, id);
#endif
	for(i = 0; i < folder->messages_cnt; i++)
		if(folder->messages[i]->id == id)
			return folder->messages[i];
	return _imap4_message_new(imap4, folder, id);
}


/* imap4_message_new */
static AccountMessage * _imap4_message_new(IMAP4 * imap4,
		AccountFolder * folder, unsigned int id)
{
	AccountPluginHelper * helper = imap4->helper;
	AccountMessage * message;
	AccountMessage ** p;

	if((p = realloc(folder->messages, sizeof(*p)
					* (folder->messages_cnt + 1))) == NULL)
		return NULL;
	folder->messages = p;
	if((message = object_new(sizeof(*message))) == NULL)
		return NULL;
	message->id = id;
	if((message->message = helper->message_new(helper->account,
					folder->folder, message)) == NULL)
	{
		_imap4_message_delete(imap4, message);
		return NULL;
	}
	folder->messages[folder->messages_cnt++] = message;
	return message;
}


/* imap4_message_delete */
static void _imap4_message_delete(IMAP4 * imap4,
		AccountMessage * message)
{
	if(message->message != NULL)
		imap4->helper->message_delete(message->message);
	object_delete(message);
}


/* callbacks */
/* on_idle */
static int _connect_address(IMAP4 * imap4, char const * hostname,
		struct addrinfo * ai);
static int _connect_channel(IMAP4 * imap4);

static gboolean _on_connect(gpointer data)
{
	IMAP4 * imap4 = data;
	AccountPluginHelper * helper = imap4->helper;
	char const * hostname;
	char const * p;
	uint16_t port;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	imap4->source = 0;
	/* get the hostname and port */
	if((hostname = imap4->config[I4CV_HOSTNAME].value) == NULL)
	{
		helper->error(helper->account, "No hostname set", 1);
		return FALSE;
	}
	if((p = imap4->config[I4CV_PORT].value) == NULL)
		return FALSE;
	port = (unsigned long)p;
	/* lookup the address */
	if(_common_lookup(hostname, port, &imap4->ai) != 0)
	{
		helper->error(helper->account, error_get(NULL), 1);
		return FALSE;
	}
	for(imap4->aip = imap4->ai; imap4->aip != NULL;
			imap4->aip = imap4->aip->ai_next)
		if(_connect_address(imap4, hostname, imap4->aip) == 0)
			break;
	if(imap4->aip == NULL)
		_imap4_stop(imap4);
	return FALSE;
}

static int _connect_address(IMAP4 * imap4, char const * hostname,
		struct addrinfo * ai)
{
	AccountPluginHelper * helper = imap4->helper;
	int res;
	char * q;
	char buf[128];

	/* create the socket */
	if((imap4->fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol))
			== -1)
		return -helper->error(helper->account, strerror(errno), 1);
	if((res = fcntl(imap4->fd, F_GETFL)) >= 0
			&& fcntl(imap4->fd, F_SETFL, res | O_NONBLOCK) == -1)
		/* ignore this error */
		/* FIXME report properly as a warning instead */
		helper->error(NULL, strerror(errno), 1);
	/* report the current status */
	if((q = _common_lookup_print(ai)) != NULL)
		snprintf(buf, sizeof(buf), "Connecting to %s (%s)", hostname,
				q);
	else
		snprintf(buf, sizeof(buf), "Connecting to %s", hostname);
	free(q);
	_imap4_event_status(imap4, AS_CONNECTING, buf);
	/* connect to the remote host */
	if((connect(imap4->fd, ai->ai_addr, ai->ai_addrlen) != 0
				&& errno != EINPROGRESS && errno != EINTR)
			|| _connect_channel(imap4) != 0)
	{
		snprintf(buf, sizeof(buf), "%s (%s)", "Connection failed",
				strerror(errno));
		return -helper->error(helper->account, buf, 1);
	}
	imap4->wr_source = g_io_add_watch(imap4->channel, G_IO_OUT,
			_on_watch_can_connect, imap4);
	return 0;
}

static int _connect_channel(IMAP4 * imap4)
{
	AccountPluginHelper * helper = imap4->helper;
	GError * error = NULL;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* prepare queue */
	if((imap4->queue = malloc(sizeof(*imap4->queue))) == NULL)
		return -helper->error(helper->account, strerror(errno), 1);
	imap4->queue[0].id = 0;
	imap4->queue[0].context = I4C_INIT;
	imap4->queue[0].status = I4CS_SENT;
	imap4->queue[0].buf = NULL;
	imap4->queue[0].buf_cnt = 0;
	imap4->queue_cnt = 1;
	imap4->queue_id = 1;
	/* setup channel */
	imap4->channel = g_io_channel_unix_new(imap4->fd);
	g_io_channel_set_encoding(imap4->channel, NULL, &error);
	g_io_channel_set_buffered(imap4->channel, FALSE);
	return 0;
}


/* on_noop */
static gboolean _on_noop(gpointer data)
{
	IMAP4 * imap4 = data;

	_imap4_command(imap4, I4C_NOOP, "NOOP");
	imap4->source = 0;
	return FALSE;
}


/* on_watch_can_connect */
static gboolean _on_watch_can_connect(GIOChannel * source,
		GIOCondition condition, gpointer data)
{
	IMAP4 * imap4 = data;
	AccountPluginHelper * helper = imap4->helper;
	int res;
	socklen_t s = sizeof(res);
	char const * hostname = imap4->config[I4CV_HOSTNAME].value;
	SSL_CTX * ssl_ctx;
	char buf[128];
	char * q;

	if(condition != G_IO_OUT || source != imap4->channel)
		return FALSE; /* should not happen */
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() connected\n", __func__);
#endif
	if(getsockopt(imap4->fd, SOL_SOCKET, SO_ERROR, &res, &s) != 0
			|| res != 0)
	{
		snprintf(buf, sizeof(buf), "%s (%s)", "Connection failed",
				strerror(res));
		helper->error(helper->account, buf, 1);
		/* FIXME try the next address instead */
		_imap4_stop(imap4);
		return FALSE;
	}
	if(imap4->aip != NULL && (q = _common_lookup_print(imap4->aip)) != NULL)
	{
		snprintf(buf, sizeof(buf), "Connected to %s (%s)", hostname,
				q);
		free(q);
	}
	else
		snprintf(buf, sizeof(buf), "Connected to %s", hostname);
	_imap4_event_status(imap4, AS_CONNECTED, buf);
	imap4->wr_source = 0;
	/* setup SSL */
	if(imap4->config[I4CV_SSL].value != NULL)
	{
		if((ssl_ctx = helper->get_ssl_context(helper->account)) == NULL)
			/* FIXME report error */
			return FALSE;
		if((imap4->ssl = SSL_new(ssl_ctx)) == NULL)
		{
			helper->error(helper->account, ERR_error_string(
						ERR_get_error(), buf), 1);
			return FALSE;
		}
		if(SSL_set_fd(imap4->ssl, imap4->fd) != 1)
		{
			ERR_error_string(ERR_get_error(), buf);
			SSL_free(imap4->ssl);
			imap4->ssl = NULL;
			helper->error(helper->account, buf, 1);
			return FALSE;
		}
		SSL_set_connect_state(imap4->ssl);
		/* perform initial handshake */
		imap4->wr_source = g_io_add_watch(imap4->channel, G_IO_OUT,
				_on_watch_can_handshake, imap4);
		return FALSE;
	}
	/* wait for the server's banner */
	imap4->rd_source = g_io_add_watch(imap4->channel, G_IO_IN,
			_on_watch_can_read, imap4);
	return FALSE;
}


/* on_watch_can_handshake */
static int _handshake_verify(IMAP4 * imap4);

static gboolean _on_watch_can_handshake(GIOChannel * source,
		GIOCondition condition, gpointer data)
{
	IMAP4 * imap4 = data;
	AccountPluginHelper * helper = imap4->helper;
	int res;
	int err;
	char buf[128];

	if((condition != G_IO_IN && condition != G_IO_OUT)
			|| source != imap4->channel || imap4->ssl == NULL)
		return FALSE; /* should not happen */
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	imap4->wr_source = 0;
	imap4->rd_source = 0;
	if((res = SSL_do_handshake(imap4->ssl)) == 1)
	{
		if(_handshake_verify(imap4) != 0)
		{
			_imap4_stop(imap4);
			return FALSE;
		}
		/* wait for the server's banner */
		imap4->rd_source = g_io_add_watch(imap4->channel, G_IO_IN,
				_on_watch_can_read_ssl, imap4);
		return FALSE;
	}
	err = SSL_get_error(imap4->ssl, res);
	ERR_error_string(err, buf);
	if(res == 0)
	{
		helper->error(helper->account, buf, 1);
		_imap4_stop(imap4);
		return FALSE;
	}
	if(err == SSL_ERROR_WANT_WRITE)
		imap4->wr_source = g_io_add_watch(imap4->channel, G_IO_OUT,
				_on_watch_can_handshake, imap4);
	else if(err == SSL_ERROR_WANT_READ)
		imap4->rd_source = g_io_add_watch(imap4->channel, G_IO_IN,
				_on_watch_can_handshake, imap4);
	else
	{
		helper->error(helper->account, buf, 1);
		_imap4_stop(imap4);
		return FALSE;
	}
	return FALSE;
}

static int _handshake_verify(IMAP4 * imap4)
{
	AccountPluginHelper * helper = imap4->helper;
	X509 * x509;
	char buf[256] = "";

	if(SSL_get_verify_result(imap4->ssl) != X509_V_OK)
		return helper->confirm(helper->account, "The certificate could"
				" not be verified.\nConnect anyway?");
	x509 = SSL_get_peer_certificate(imap4->ssl);
	X509_NAME_get_text_by_NID(X509_get_subject_name(x509), NID_commonName,
			buf, sizeof(buf));
	if(strcasecmp(buf, imap4->config[I4CV_HOSTNAME].value) != 0)
		return helper->confirm(helper->account, "The certificate could"
				" not be matched.\nConnect anyway?");
	return 0;
}


/* on_watch_can_read */
static gboolean _on_watch_can_read(GIOChannel * source, GIOCondition condition,
		gpointer data)
{
	IMAP4 * imap4 = data;
	AccountPluginHelper * helper = imap4->helper;
	char * p;
	gsize cnt = 0;
	GError * error = NULL;
	GIOStatus status;
	IMAP4Command * cmd;
	const int inc = 256;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(condition != G_IO_IN || source != imap4->channel)
		return FALSE; /* should not happen */
	if((p = realloc(imap4->rd_buf, imap4->rd_buf_cnt + inc)) == NULL)
		return TRUE; /* XXX retries immediately (delay?) */
	imap4->rd_buf = p;
	status = g_io_channel_read_chars(source,
			&imap4->rd_buf[imap4->rd_buf_cnt], inc, &cnt, &error);
#ifdef DEBUG
	fprintf(stderr, "%s", "DEBUG: IMAP4 SERVER: ");
	fwrite(&imap4->rd_buf[imap4->rd_buf_cnt], sizeof(*p), cnt, stderr);
#endif
	imap4->rd_buf_cnt += cnt;
	switch(status)
	{
		case G_IO_STATUS_NORMAL:
			break;
		case G_IO_STATUS_ERROR:
			helper->error(helper->account, error->message, 1);
			g_error_free(error);
			_imap4_stop(imap4);
			return FALSE;
		case G_IO_STATUS_EOF:
		default:
			_imap4_event_status(imap4, AS_DISCONNECTED, NULL);
			_imap4_stop(imap4);
			return FALSE;
	}
	if(_imap4_parse(imap4) != 0)
	{
		_imap4_stop(imap4);
		return FALSE;
	}
	if(imap4->queue_cnt == 0)
		return TRUE;
	cmd = &imap4->queue[0];
	if(cmd->buf_cnt == 0)
	{
		if(cmd->status == I4CS_SENT || cmd->status == I4CS_PARSING)
			/* begin or keep parsing */
			return TRUE;
		else if(cmd->status == I4CS_OK || cmd->status == I4CS_ERROR)
			/* the current command is completed */
			memmove(cmd, &imap4->queue[1], sizeof(*cmd)
					* --imap4->queue_cnt);
	}
	if(imap4->queue_cnt == 0)
	{
		_imap4_event_status(imap4, AS_IDLE, NULL);
		imap4->source = g_timeout_add(30000, _on_noop, imap4);
	}
	else
		imap4->wr_source = g_io_add_watch(imap4->channel, G_IO_OUT,
				_on_watch_can_write, imap4);
	return TRUE;
}


/* on_watch_can_read_ssl */
static gboolean _on_watch_can_read_ssl(GIOChannel * source,
		GIOCondition condition, gpointer data)
{
	IMAP4 * imap4 = data;
	char * p;
	int cnt;
	IMAP4Command * cmd;
	char buf[128];
	const int inc = 16384; /* XXX not reliable with a smaller value */

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((condition != G_IO_IN && condition != G_IO_OUT)
			|| source != imap4->channel)
		return FALSE; /* should not happen */
	if((p = realloc(imap4->rd_buf, imap4->rd_buf_cnt + inc)) == NULL)
		return TRUE; /* XXX retries immediately (delay?) */
	imap4->rd_buf = p;
	if((cnt = SSL_read(imap4->ssl, &imap4->rd_buf[imap4->rd_buf_cnt], inc))
			<= 0)
	{
		if(SSL_get_error(imap4->ssl, cnt) == SSL_ERROR_WANT_WRITE)
			/* call SSL_read() again when it can send data */
			imap4->rd_source = g_io_add_watch(imap4->channel,
					G_IO_OUT, _on_watch_can_read_ssl,
					imap4);
		else if(SSL_get_error(imap4->ssl, cnt) == SSL_ERROR_WANT_READ)
			/* call SSL_read() again when it can read data */
			imap4->rd_source = g_io_add_watch(imap4->channel,
					G_IO_IN, _on_watch_can_read_ssl, imap4);
		else
		{
			/* unknown error */
			imap4->rd_source = 0;
			ERR_error_string(SSL_get_error(imap4->ssl, cnt), buf);
			_imap4_event_status(imap4, AS_DISCONNECTED, buf);
			_imap4_stop(imap4);
		}
		return FALSE;
	}
#ifdef DEBUG
	fprintf(stderr, "%s", "DEBUG: IMAP4 SERVER: ");
	fwrite(&imap4->rd_buf[imap4->rd_buf_cnt], sizeof(*p), cnt, stderr);
#endif
	imap4->rd_buf_cnt += cnt;
	if(_imap4_parse(imap4) != 0)
	{
		_imap4_stop(imap4);
		return FALSE;
	}
	if(imap4->queue_cnt == 0)
		return TRUE;
	cmd = &imap4->queue[0];
	if(cmd->buf_cnt == 0)
	{
		if(cmd->status == I4CS_SENT || cmd->status == I4CS_PARSING)
			/* begin or keep parsing */
			return TRUE;
		else if(cmd->status == I4CS_OK || cmd->status == I4CS_ERROR)
			/* the current command is completed */
			memmove(cmd, &imap4->queue[1], sizeof(*cmd)
					* --imap4->queue_cnt);
	}
	if(imap4->queue_cnt == 0)
	{
		_imap4_event_status(imap4, AS_IDLE, NULL);
		imap4->source = g_timeout_add(30000, _on_noop, imap4);
	}
	else
		imap4->wr_source = g_io_add_watch(imap4->channel, G_IO_OUT,
				_on_watch_can_write_ssl, imap4);
	return TRUE;
}


/* on_watch_can_write */
static gboolean _on_watch_can_write(GIOChannel * source, GIOCondition condition,
		gpointer data)
{
	IMAP4 * imap4 = data;
	AccountPluginHelper * helper = imap4->helper;
	IMAP4Command * cmd = &imap4->queue[0];
	gsize cnt = 0;
	GError * error = NULL;
	GIOStatus status;
	char * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(condition != G_IO_OUT || source != imap4->channel
			|| imap4->queue_cnt == 0 || cmd->buf_cnt == 0)
		return FALSE; /* should not happen */
	status = g_io_channel_write_chars(source, cmd->buf, cmd->buf_cnt, &cnt,
			&error);
#ifdef DEBUG
	fprintf(stderr, "%s", "DEBUG: IMAP4 CLIENT: ");
	fwrite(cmd->buf, sizeof(*p), cnt, stderr);
#endif
	if(cnt != 0)
	{
		cmd->buf_cnt -= cnt;
		memmove(cmd->buf, &cmd->buf[cnt], cmd->buf_cnt);
		if((p = realloc(cmd->buf, cmd->buf_cnt)) != NULL)
			cmd->buf = p; /* we can ignore errors... */
		else if(cmd->buf_cnt == 0)
			cmd->buf = NULL; /* ...except when it's not one */
	}
	switch(status)
	{
		case G_IO_STATUS_NORMAL:
			break;
		case G_IO_STATUS_ERROR:
			helper->error(helper->account, error->message, 1);
			g_error_free(error);
			_imap4_stop(imap4);
			return FALSE;
		case G_IO_STATUS_EOF:
		default:
			_imap4_event_status(imap4, AS_DISCONNECTED, NULL);
			_imap4_stop(imap4);
			return FALSE;
	}
	if(cmd->buf_cnt > 0)
		return TRUE;
	cmd->status = I4CS_SENT;
	imap4->wr_source = 0;
	if(imap4->rd_source == 0)
		/* XXX should not happen */
		imap4->rd_source = g_io_add_watch(imap4->channel, G_IO_IN,
				_on_watch_can_read, imap4);
	return FALSE;
}


/* on_watch_can_write_ssl */
static gboolean _on_watch_can_write_ssl(GIOChannel * source,
		GIOCondition condition, gpointer data)
{
	IMAP4 * imap4 = data;
	IMAP4Command * cmd = &imap4->queue[0];
	int cnt;
	char * p;
	char buf[128];

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((condition != G_IO_IN && condition != G_IO_OUT)
			|| source != imap4->channel || imap4->queue_cnt == 0
			|| cmd->buf_cnt == 0)
		return FALSE; /* should not happen */
	if((cnt = SSL_write(imap4->ssl, cmd->buf, cmd->buf_cnt)) <= 0)
	{
		if(SSL_get_error(imap4->ssl, cnt) == SSL_ERROR_WANT_READ)
			imap4->wr_source = g_io_add_watch(imap4->channel,
					G_IO_IN, _on_watch_can_write_ssl,
					imap4);
		else if(SSL_get_error(imap4->ssl, cnt) == SSL_ERROR_WANT_WRITE)
			imap4->wr_source = g_io_add_watch(imap4->channel,
					G_IO_OUT, _on_watch_can_write_ssl,
					imap4);
		else
		{
			ERR_error_string(SSL_get_error(imap4->ssl, cnt), buf);
			_imap4_event_status(imap4, AS_DISCONNECTED, buf);
			_imap4_stop(imap4);
		}
		return FALSE;
	}
#ifdef DEBUG
	fprintf(stderr, "%s", "DEBUG: IMAP4 CLIENT: ");
	fwrite(cmd->buf, sizeof(*p), cnt, stderr);
#endif
	cmd->buf_cnt -= cnt;
	memmove(cmd->buf, &cmd->buf[cnt], cmd->buf_cnt);
	if((p = realloc(cmd->buf, cmd->buf_cnt)) != NULL)
		cmd->buf = p; /* we can ignore errors... */
	else if(cmd->buf_cnt == 0)
		cmd->buf = NULL; /* ...except when it's not one */
	if(cmd->buf_cnt > 0)
		return TRUE;
	cmd->status = I4CS_SENT;
	imap4->wr_source = 0;
	if(imap4->rd_source == 0)
		/* XXX should not happen */
		imap4->rd_source = g_io_add_watch(imap4->channel, G_IO_IN,
				_on_watch_can_read_ssl, imap4);
	return FALSE;
}
