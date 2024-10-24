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
 * - really queue commands with callbacks
 * - openssl should be more explicit when SSL_set_fd() is missing (no BIO)
 * - support multiple connections? */



#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <glib.h>
#include <System.h>
#include "Mailer/account.h"
#include "common.c"


/* POP3 */
/* private */
/* types */
struct _AccountFolder
{
	Folder * folder;

	AccountMessage ** messages;
	size_t messages_cnt;
};

struct _AccountMessage
{
	Message * message;

	unsigned int id;
};

typedef enum _POP3CommandStatus
{
	P3CS_QUEUED = 0,
	P3CS_SENT,
	P3CS_ERROR,
	P3CS_PARSING,
	P3CS_OK
} POP3CommandStatus;

typedef enum _POP3ConfigValue
{
	P3CV_USERNAME = 0,
	P3CV_PASSWORD,
	P3CV_HOSTNAME,
	P3CV_PORT,
	P3CV_SSL,
	P3CV_DELETE
} POP3ConfigValue;
#define P3CV_LAST P3CV_DELETE
#define P3CV_COUNT (P3CV_LAST + 1)

typedef enum _POP3Context
{
	P3C_INIT = 0,
	P3C_AUTHORIZATION_USER,
	P3C_AUTHORIZATION_PASS,
	P3C_NOOP,
	P3C_TRANSACTION_LIST,
	P3C_TRANSACTION_RETR,
	P3C_TRANSACTION_STAT,
	P3C_TRANSACTION_TOP
} POP3Context;

typedef struct _POP3Command
{
	POP3CommandStatus status;
	POP3Context context;
	char * buf;
	size_t buf_cnt;

	union
	{
		struct
		{
			unsigned int id;
			gboolean body;
			AccountMessage * message;
		} transaction_retr, transaction_top;
	} data;
} POP3Command;

typedef struct _AccountPlugin
{
	AccountPluginHelper * helper;

	AccountConfig * config;

	struct addrinfo * ai;
	struct addrinfo * aip;
	int fd;
	SSL * ssl;
	guint source;

	AccountFolder inbox;
	AccountFolder trash;

	GIOChannel * channel;
	char * rd_buf;
	size_t rd_buf_cnt;
	guint rd_source;
	guint wr_source;

	POP3Command * queue;
	size_t queue_cnt;
} POP3;


/* variables */
static char const _pop3_type[] = "POP3";
static char const _pop3_name[] = "POP3 server";

static AccountConfig _pop3_config[P3CV_COUNT + 1] =
{
	{ "username",	"Username",		ACT_STRING,	NULL },
	{ "password",	"Password",		ACT_PASSWORD,	NULL },
	{ "hostname",	"Server hostname",	ACT_STRING,	NULL },
	{ "port",	"Server port",		ACT_UINT16,	(void *)110 },
	{ "ssl",	"Use SSL",		ACT_BOOLEAN,	NULL },
	{ "delete",	"Delete read mails on server",
						ACT_BOOLEAN,	NULL },
	{ NULL,		NULL,			ACT_NONE,	NULL }
};


/* prototypes */
/* plug-in */
static POP3 * _pop3_init(AccountPluginHelper * helper);
static int _pop3_destroy(POP3 * pop3);
static AccountConfig * _pop3_get_config(POP3 * pop3);
static int _pop3_start(POP3 * pop3);
static void _pop3_stop(POP3 * pop3);
static int _pop3_refresh(POP3 * pop3, AccountFolder * folder,
		AccountMessage * message);

/* useful */
static POP3Command * _pop3_command(POP3 * pop3, POP3Context context,
		char const * command);
static int _pop3_parse(POP3 * pop3);

/* events */
static void _pop3_event(POP3 * pop3, AccountEventType type);
static void _pop3_event_status(POP3 * pop3, AccountStatus status,
		char const * message);

static AccountMessage * _pop3_message_get(POP3 * pop3,
		AccountFolder * folder, unsigned int id);
static AccountMessage * _pop3_message_new(POP3 * pop3,
		AccountFolder * folder, unsigned int id);
static void _pop3_message_delete(POP3 * pop3,
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
	_pop3_type,
	_pop3_name,
	NULL,
	NULL,
	_pop3_config,
	_pop3_init,
	_pop3_destroy,
	_pop3_get_config,
	NULL,
	_pop3_start,
	_pop3_stop,
	_pop3_refresh
};


/* protected */
/* functions */
/* plug-in */
/* pop3_init */
static POP3 * _pop3_init(AccountPluginHelper * helper)
{
	POP3 * pop3;

	if((pop3 = malloc(sizeof(*pop3))) == NULL)
		return NULL;
	memset(pop3, 0, sizeof(*pop3));
	pop3->helper = helper;
	if((pop3->config = malloc(sizeof(_pop3_config))) == NULL)
	{
		free(pop3);
		return NULL;
	}
	memcpy(pop3->config, &_pop3_config, sizeof(_pop3_config));
	pop3->ai = NULL;
	pop3->aip = NULL;
	pop3->fd = -1;
	pop3->inbox.folder = pop3->helper->folder_new(pop3->helper->account,
			&pop3->inbox, NULL, FT_INBOX, "Inbox");
	pop3->trash.folder = pop3->helper->folder_new(pop3->helper->account,
			&pop3->trash, NULL, FT_TRASH, "Trash");
	pop3->source = g_idle_add(_on_connect, pop3);
	return pop3;
}


/* pop3_destroy */
static int _pop3_destroy(POP3 * pop3)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif

	if(pop3 == NULL) /* XXX _pop3_destroy() may be called uninitialized */
		return 0;
	_pop3_stop(pop3);
	free(pop3);
	return 0;
}


/* pop3_get_config */
static AccountConfig * _pop3_get_config(POP3 * pop3)
{
	return pop3->config;
}


/* pop3_start */
static int _pop3_start(POP3 * pop3)
{
	_pop3_event(pop3, AET_STARTED);
	if(pop3->fd >= 0 || pop3->source != 0)
		/* already started */
		return 0;
	pop3->source = g_idle_add(_on_connect, pop3);
	return 0;
}


/* pop3_stop */
static void _pop3_stop(POP3 * pop3)
{
	size_t i;

	if(pop3->ssl != NULL)
		SSL_free(pop3->ssl);
	pop3->ssl = NULL;
	if(pop3->rd_source != 0)
		g_source_remove(pop3->rd_source);
	free(pop3->rd_buf);
	if(pop3->wr_source != 0)
		g_source_remove(pop3->wr_source);
	if(pop3->source != 0)
		g_source_remove(pop3->source);
	if(pop3->channel != NULL)
	{
		g_io_channel_shutdown(pop3->channel, TRUE, NULL);
		g_io_channel_unref(pop3->channel);
		pop3->fd = -1;
	}
	for(i = 0; i < pop3->queue_cnt; i++)
		free(pop3->queue[i].buf);
	free(pop3->queue);
	if(pop3->fd >= 0)
		close(pop3->fd);
	pop3->fd = -1;
	pop3->aip = NULL;
	if(pop3->ai != NULL)
		freeaddrinfo(pop3->ai);
	pop3->ai = NULL;
	_pop3_event(pop3, AET_STOPPED);
}


/* pop3_refresh */
static int _pop3_refresh(POP3 * pop3, AccountFolder * folder,
		AccountMessage * message)
{
	POP3Command * cmd;
	char buf[32];
	(void) folder;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %u\n", __func__, (message != NULL)
			? message->id : 0);
#endif
	if(message == NULL)
		return 0;
	snprintf(buf, sizeof(buf), "%s %u", "RETR", message->id);
	if((cmd = _pop3_command(pop3, P3C_TRANSACTION_RETR, buf)) == NULL)
		return -1;
	cmd->data.transaction_retr.id = message->id;
	return 0;
}


/* private */
/* functions */
/* useful */
/* pop3_command */
static POP3Command * _pop3_command(POP3 * pop3, POP3Context context,
		char const * command)
{
	POP3Command * p;
	size_t len;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\") %p\n", __func__, command,
			(void *)pop3->channel);
#endif
	/* abort if the command is invalid */
	if(command == NULL || (len = strlen(command)) == 0)
		return NULL;
	/* abort if there is no active connection */
	if(pop3->channel == NULL)
		return NULL;
	/* queue the command */
	len += 3;
	if((p = realloc(pop3->queue, sizeof(*p) * (pop3->queue_cnt + 1)))
			== NULL)
		return NULL;
	pop3->queue = p;
	p = &pop3->queue[pop3->queue_cnt];
	p->context = context;
	p->status = P3CS_QUEUED;
	if((p->buf = malloc(len)) == NULL)
		return NULL;
	p->buf_cnt = snprintf(p->buf, len, "%s\r\n", command);
	memset(&p->data, 0, sizeof(p->data));
	if(pop3->queue_cnt++ == 0)
	{
		if(pop3->source != 0)
		{
			/* cancel the pending NOOP operation */
			g_source_remove(pop3->source);
			pop3->source = 0;
		}
		pop3->wr_source = g_io_add_watch(pop3->channel, G_IO_OUT,
				(pop3->ssl != NULL) ? _on_watch_can_write_ssl
				: _on_watch_can_write, pop3);
	}
	return p;
}


/* pop3_parse */
static int _parse_context(POP3 * pop3, char const * answer);
static int _parse_context_transaction_retr(POP3 * pop3,
		char const * answer);

static int _pop3_parse(POP3 * pop3)
{
	int ret = 0;
	AccountPluginHelper * helper = pop3->helper;
	size_t i;
	size_t j;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	for(i = 0, j = 0;; j = ++i)
	{
		/* look for carriage return sequences */
		for(; i < pop3->rd_buf_cnt; i++)
			if(pop3->rd_buf[i] == '\r' && i + 1 < pop3->rd_buf_cnt
					&& pop3->rd_buf[++i] == '\n')
				break;
		/* if no carriage return was found wait for more input */
		if(i == pop3->rd_buf_cnt)
			break;
		/* if no command was sent read more lines */
		if(pop3->queue_cnt == 0)
			continue;
		pop3->rd_buf[i - 1] = '\0';
		if(pop3->queue[0].status == P3CS_SENT
				&& strncmp("-ERR", &pop3->rd_buf[j], 4) == 0)
		{
			pop3->queue[0].status = P3CS_ERROR;
			helper->error(helper->account, &pop3->rd_buf[j + 4], 1);
		}
		else if(pop3->queue[0].status == P3CS_SENT
				&& strncmp("+OK", &pop3->rd_buf[j], 3) == 0)
			pop3->queue[0].status = P3CS_PARSING;
		if(_parse_context(pop3, &pop3->rd_buf[j]) != 0)
		{
			pop3->queue[0].status = P3CS_ERROR;
			ret = -1;
		}
	}
	if(j != 0)
	{
		pop3->rd_buf_cnt -= j;
		memmove(pop3->rd_buf, &pop3->rd_buf[j], pop3->rd_buf_cnt);
	}
	return ret;
}

static int _parse_context(POP3 * pop3, char const * answer)
{
	int ret = -1;
	POP3Command * cmd = &pop3->queue[0];
	char const * p;
	char * q;
	unsigned int u;
	unsigned int v;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\") %u, %u\n", __func__, answer,
			pop3->queue[0].context, pop3->queue[0].status);
#endif
	switch(cmd->context)
	{
		case P3C_INIT:
			if(cmd->status != P3CS_PARSING)
				return 0;
			cmd->status = P3CS_OK;
			if((p = pop3->config[0].value) == NULL)
				return -1;
			q = g_strdup_printf("%s %s", "USER", p);
			cmd = _pop3_command(pop3, P3C_AUTHORIZATION_USER, q);
			g_free(q);
			return (cmd != NULL) ? 0 : -1;
		case P3C_AUTHORIZATION_USER:
			if(cmd->status != P3CS_PARSING)
				return 0;
			cmd->status = P3CS_OK;
			if((p = pop3->config[1].value) == NULL)
				p = ""; /* assumes an empty password */
			q = g_strdup_printf("%s %s", "PASS", p);
			cmd = _pop3_command(pop3, P3C_AUTHORIZATION_PASS, q);
			g_free(q);
			return (cmd != NULL) ? 0 : -1;
		case P3C_AUTHORIZATION_PASS:
			if(cmd->status != P3CS_PARSING)
				return 0;
			cmd->status = P3CS_OK;
			return (_pop3_command(pop3, P3C_TRANSACTION_STAT,
						"STAT") != NULL) ? 0 : -1;
		case P3C_NOOP:
			if(strncmp(answer, "+OK", 3) == 0)
				cmd->status = P3CS_OK;
			return 0;
		case P3C_TRANSACTION_LIST:
			if(cmd->status != P3CS_PARSING)
				return 0;
			if(strncmp(answer, "+OK", 3) == 0)
				return 0;
			if(strcmp(answer, ".") == 0)
			{
				cmd->status = P3CS_OK;
				return 0;
			}
			if(sscanf(answer, "%u %u", &u, &v) != 2)
				return -1;
			/* FIXME may not be supported by the server */
			q = g_strdup_printf("%s %u 0", "TOP", u);
			cmd = _pop3_command(pop3, P3C_TRANSACTION_TOP, q);
			free(q);
			cmd->data.transaction_top.id = u;
			return (cmd != NULL) ? 0 : -1;
		case P3C_TRANSACTION_RETR:
		case P3C_TRANSACTION_TOP: /* same as RETR without the body */
			return _parse_context_transaction_retr(pop3, answer);
		case P3C_TRANSACTION_STAT:
			if(cmd->status != P3CS_PARSING)
				return 0;
			if(sscanf(answer, "+OK %u %u", &u, &v) != 2)
				return -1;
			cmd->status = P3CS_OK;
			return (_pop3_command(pop3, P3C_TRANSACTION_LIST,
						"LIST") != NULL) ? 0 : -1;
	}
	return ret;
}

static int _parse_context_transaction_retr(POP3 * pop3,
		char const * answer)
{
	AccountPluginHelper * helper = pop3->helper;
	POP3Command * cmd = &pop3->queue[0];
	AccountMessage * message;

	if(cmd->status != P3CS_PARSING)
		return 0;
	if((message = cmd->data.transaction_retr.message) == NULL
			&& strncmp(answer, "+OK", 3) == 0)
	{
		cmd->data.transaction_retr.body = FALSE;
		message = _pop3_message_get(pop3, &pop3->inbox,
					cmd->data.transaction_retr.id);
		cmd->data.transaction_retr.message = message;
		return 0;
	}
	if(strcmp(answer, ".") == 0)
	{
		cmd->status = P3CS_OK;
		return 0;
	}
	if(answer[0] == '\0')
	{
		cmd->data.transaction_retr.body = TRUE;
		helper->message_set_body(message->message, NULL, 0, 0);
		return 0;
	}
	if(cmd->data.transaction_retr.body)
	{
		helper->message_set_body(message->message, answer,
				strlen(answer), 1);
		helper->message_set_body(message->message, "\r\n", 2, 1);
	}
	else
		helper->message_set_header(message->message, answer);
	return 0;
}


/* pop3_event */
static void _pop3_event(POP3 * pop3, AccountEventType type)
{
	AccountPluginHelper * helper = pop3->helper;
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


/* pop3_event_status */
static void _pop3_event_status(POP3 * pop3, AccountStatus status,
		char const * message)
{
	AccountPluginHelper * helper = pop3->helper;
	AccountEvent event;

	memset(&event, 0, sizeof(event));
	event.status.type = AET_STATUS;
	event.status.status = status;
	event.status.message = message;
	helper->event(helper->account, &event);
}


/* pop3_message_get */
static AccountMessage * _pop3_message_get(POP3 * pop3,
		AccountFolder * folder, unsigned int id)
{
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%p, %u)\n", __func__, (void *)folder, id);
#endif
	for(i = 0; i < folder->messages_cnt; i++)
		if(folder->messages[i]->id == id)
			return folder->messages[i];
	return _pop3_message_new(pop3, folder, id);
}


/* pop3_message_new */
static AccountMessage * _pop3_message_new(POP3 * pop3,
		AccountFolder * folder, unsigned int id)
{
	AccountPluginHelper * helper = pop3->helper;
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
		_pop3_message_delete(pop3, message);
		return NULL;
	}
	folder->messages[folder->messages_cnt++] = message;
	return message;
}


/* pop3_message_delete */
static void _pop3_message_delete(POP3 * pop3,
		AccountMessage * message)
{
	if(message->message != NULL)
		pop3->helper->message_delete(message->message);
	object_delete(message);
}


/* callbacks */
/* on_idle */
static int _connect_address(POP3 * pop3, char const * hostname,
		struct addrinfo * ai);
static int _connect_channel(POP3 * pop3);

static gboolean _on_connect(gpointer data)
{
	POP3 * pop3 = data;
	AccountPluginHelper * helper = pop3->helper;
	char const * hostname;
	char const * p;
	uint16_t port;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	pop3->source = 0;
	/* get the hostname and port */
	if((hostname = pop3->config[P3CV_HOSTNAME].value) == NULL)
	{
		helper->error(helper->account, "No hostname set", 1);
		return FALSE;
	}
	if((p = pop3->config[P3CV_PORT].value) == NULL)
		return FALSE;
	port = (unsigned long)p;
	/* lookup the address */
	if(_common_lookup(hostname, port, &pop3->ai) != 0)
	{
		helper->error(helper->account, error_get(NULL), 1);
		return FALSE;
	}
	for(pop3->aip = pop3->ai; pop3->aip != NULL;
			pop3->aip = pop3->aip->ai_next)
		if(_connect_address(pop3, hostname, pop3->aip) == 0)
			break;
	if(pop3->aip == NULL)
		_pop3_stop(pop3);
	return FALSE;
}

static int _connect_address(POP3 * pop3, char const * hostname,
		struct addrinfo * ai)
{
	AccountPluginHelper * helper = pop3->helper;
	int res;
	char * q;
	char buf[128];

	/* create the socket */
	if((pop3->fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol))
			== -1)
		return -helper->error(helper->account, strerror(errno), 1);
	if((res = fcntl(pop3->fd, F_GETFL)) >= 0
			&& fcntl(pop3->fd, F_SETFL, res | O_NONBLOCK) == -1)
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
	_pop3_event_status(pop3, AS_CONNECTING, buf);
	/* connect to the remote host */
	if((connect(pop3->fd, ai->ai_addr, ai->ai_addrlen) != 0
				&& errno != EINPROGRESS && errno != EINTR)
			|| _connect_channel(pop3) != 0)
	{
		snprintf(buf, sizeof(buf), "%s (%s)", "Connection failed",
				strerror(errno));
		return -helper->error(helper->account, buf, 1);
	}
	pop3->wr_source = g_io_add_watch(pop3->channel, G_IO_OUT,
			_on_watch_can_connect, pop3);
	return 0;
}

static int _connect_channel(POP3 * pop3)
{
	AccountPluginHelper * helper = pop3->helper;
	GError * error = NULL;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* prepare queue */
	if((pop3->queue = malloc(sizeof(*pop3->queue))) == NULL)
		return -helper->error(helper->account, strerror(errno), 1);
	pop3->queue[0].context = P3C_INIT;
	pop3->queue[0].status = P3CS_SENT;
	pop3->queue[0].buf = NULL;
	pop3->queue[0].buf_cnt = 0;
	pop3->queue_cnt = 1;
	/* setup channel */
	pop3->channel = g_io_channel_unix_new(pop3->fd);
	g_io_channel_set_encoding(pop3->channel, NULL, &error);
	g_io_channel_set_buffered(pop3->channel, FALSE);
	return 0;
}


/* on_noop */
static gboolean _on_noop(gpointer data)
{
	POP3 * pop3 = data;

	_pop3_command(pop3, P3C_NOOP, "NOOP");
	pop3->source = 0;
	return FALSE;
}


/* on_watch_can_connect */
static gboolean _on_watch_can_connect(GIOChannel * source,
		GIOCondition condition, gpointer data)
{
	POP3 * pop3 = data;
	AccountPluginHelper * helper = pop3->helper;
	int res;
	socklen_t s = sizeof(res);
	char const * hostname = pop3->config[P3CV_HOSTNAME].value;
	SSL_CTX * ssl_ctx;
	char buf[128];
	char * q;

	if(condition != G_IO_OUT || source != pop3->channel)
		return FALSE; /* should not happen */
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() connected\n", __func__);
#endif
	if(getsockopt(pop3->fd, SOL_SOCKET, SO_ERROR, &res, &s) != 0
			|| res != 0)
	{
		snprintf(buf, sizeof(buf), "%s (%s)", "Connection failed",
				strerror(res));
		helper->error(helper->account, buf, 1);
		/* FIXME try the next address instead */
		_pop3_stop(pop3);
		return FALSE;
	}
	if(pop3->aip != NULL && (q = _common_lookup_print(pop3->aip)) != NULL)
	{
		snprintf(buf, sizeof(buf), "Connected to %s (%s)", hostname,
				q);
		free(q);
	}
	else
		snprintf(buf, sizeof(buf), "Connected to %s", hostname);
	_pop3_event_status(pop3, AS_CONNECTED, buf);
	pop3->wr_source = 0;
	/* setup SSL */
	if(pop3->config[P3CV_SSL].value != NULL)
	{
		if((ssl_ctx = helper->get_ssl_context(helper->account)) == NULL)
			/* FIXME report error */
			return FALSE;
		if((pop3->ssl = SSL_new(ssl_ctx)) == NULL)
		{
			helper->error(helper->account, ERR_error_string(
						ERR_get_error(), buf), 1);
			return FALSE;
		}
		if(SSL_set_fd(pop3->ssl, pop3->fd) != 1)
		{
			ERR_error_string(ERR_get_error(), buf);
			SSL_free(pop3->ssl);
			pop3->ssl = NULL;
			helper->error(helper->account, buf, 1);
			return FALSE;
		}
		SSL_set_connect_state(pop3->ssl);
		/* perform initial handshake */
		pop3->wr_source = g_io_add_watch(pop3->channel, G_IO_OUT,
				_on_watch_can_handshake, pop3);
		return FALSE;
	}
	/* wait for the server's banner */
	pop3->rd_source = g_io_add_watch(pop3->channel, G_IO_IN,
			_on_watch_can_read, pop3);
	return FALSE;
}


/* on_watch_can_handshake */
static int _handshake_verify(POP3 * pop3);

static gboolean _on_watch_can_handshake(GIOChannel * source,
		GIOCondition condition, gpointer data)
{
	POP3 * pop3 = data;
	AccountPluginHelper * helper = pop3->helper;
	int res;
	int err;
	char buf[128];

	if((condition != G_IO_IN && condition != G_IO_OUT)
			|| source != pop3->channel || pop3->ssl == NULL)
		return FALSE; /* should not happen */
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	pop3->wr_source = 0;
	pop3->rd_source = 0;
	if((res = SSL_do_handshake(pop3->ssl)) == 1)
	{
		if(_handshake_verify(pop3) != 0)
		{
			_pop3_stop(pop3);
			return FALSE;
		}
		/* wait for the server's banner */
		pop3->rd_source = g_io_add_watch(pop3->channel, G_IO_IN,
				_on_watch_can_read_ssl, pop3);
		return FALSE;
	}
	err = SSL_get_error(pop3->ssl, res);
	ERR_error_string(err, buf);
	if(res == 0)
	{
		helper->error(helper->account, buf, 1);
		_pop3_stop(pop3);
		return FALSE;
	}
	if(err == SSL_ERROR_WANT_WRITE)
		pop3->wr_source = g_io_add_watch(pop3->channel, G_IO_OUT,
				_on_watch_can_handshake, pop3);
	else if(err == SSL_ERROR_WANT_READ)
		pop3->rd_source = g_io_add_watch(pop3->channel, G_IO_IN,
				_on_watch_can_handshake, pop3);
	else
	{
		helper->error(helper->account, buf, 1);
		_pop3_stop(pop3);
		return FALSE;
	}
	return FALSE;
}

static int _handshake_verify(POP3 * pop3)
{
	AccountPluginHelper * helper = pop3->helper;
	X509 * x509;
	char buf[256] = "";

	if(SSL_get_verify_result(pop3->ssl) != X509_V_OK)
		return helper->confirm(helper->account, "The certificate could"
				" not be verified.\nConnect anyway?");
	x509 = SSL_get_peer_certificate(pop3->ssl);
	X509_NAME_get_text_by_NID(X509_get_subject_name(x509), NID_commonName,
			buf, sizeof(buf));
	if(strcasecmp(buf, pop3->config[P3CV_HOSTNAME].value) != 0)
		return helper->confirm(helper->account, "The certificate could"
				" not be matched.\nConnect anyway?");
	return 0;
}


/* on_watch_can_read */
static gboolean _on_watch_can_read(GIOChannel * source, GIOCondition condition,
		gpointer data)
{
	POP3 * pop3 = data;
	AccountPluginHelper * helper = pop3->helper;
	char * p;
	gsize cnt = 0;
	GError * error = NULL;
	GIOStatus status;
	POP3Command * cmd;
	const int inc = 256;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(condition != G_IO_IN || source != pop3->channel)
		return FALSE; /* should not happen */
	if((p = realloc(pop3->rd_buf, pop3->rd_buf_cnt + inc)) == NULL)
		return TRUE; /* XXX retries immediately (delay?) */
	pop3->rd_buf = p;
	status = g_io_channel_read_chars(source,
			&pop3->rd_buf[pop3->rd_buf_cnt], inc, &cnt, &error);
#ifdef DEBUG
	fprintf(stderr, "%s", "DEBUG: POP3 SERVER: ");
	fwrite(&pop3->rd_buf[pop3->rd_buf_cnt], sizeof(*p), cnt, stderr);
#endif
	pop3->rd_buf_cnt += cnt;
	switch(status)
	{
		case G_IO_STATUS_NORMAL:
			break;
		case G_IO_STATUS_ERROR:
			helper->error(helper->account, error->message, 1);
			g_error_free(error);
			_pop3_stop(pop3);
			return FALSE;
		case G_IO_STATUS_EOF:
		default:
			_pop3_event_status(pop3, AS_DISCONNECTED, NULL);
			_pop3_stop(pop3);
			return FALSE;
	}
	if(_pop3_parse(pop3) != 0)
	{
		_pop3_stop(pop3);
		return FALSE;
	}
	if(pop3->queue_cnt == 0)
		return TRUE;
	cmd = &pop3->queue[0];
	if(cmd->buf_cnt == 0)
	{
		if(cmd->status == P3CS_SENT || cmd->status == P3CS_PARSING)
			/* begin or keep parsing */
			return TRUE;
		else if(cmd->status == P3CS_OK || cmd->status == P3CS_ERROR)
			/* the current command is completed */
			memmove(cmd, &pop3->queue[1], sizeof(*cmd)
					* --pop3->queue_cnt);
	}
	if(pop3->queue_cnt == 0)
	{
		_pop3_event_status(pop3, AS_IDLE, NULL);
		pop3->source = g_timeout_add(30000, _on_noop, pop3);
	}
	else
		pop3->wr_source = g_io_add_watch(pop3->channel, G_IO_OUT,
				_on_watch_can_write, pop3);
	return TRUE;
}


/* on_watch_can_read_ssl */
static gboolean _on_watch_can_read_ssl(GIOChannel * source,
		GIOCondition condition, gpointer data)
{
	POP3 * pop3 = data;
	char * p;
	int cnt;
	POP3Command * cmd;
	char buf[128];
	const int inc = 16384; /* XXX not reliable with a smaller value */

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((condition != G_IO_IN && condition != G_IO_OUT)
			|| source != pop3->channel)
		return FALSE; /* should not happen */
	if((p = realloc(pop3->rd_buf, pop3->rd_buf_cnt + inc)) == NULL)
		return TRUE; /* XXX retries immediately (delay?) */
	pop3->rd_buf = p;
	if((cnt = SSL_read(pop3->ssl, &pop3->rd_buf[pop3->rd_buf_cnt], inc))
			<= 0)
	{
		if(SSL_get_error(pop3->ssl, cnt) == SSL_ERROR_WANT_WRITE)
			/* call SSL_read() again when it can send data */
			pop3->rd_source = g_io_add_watch(pop3->channel,
					G_IO_OUT, _on_watch_can_read_ssl, pop3);
		else if(SSL_get_error(pop3->ssl, cnt) == SSL_ERROR_WANT_READ)
			/* call SSL_read() again when it can read data */
			pop3->rd_source = g_io_add_watch(pop3->channel, G_IO_IN,
					_on_watch_can_read_ssl, pop3);
		else
		{
			/* unknown error */
			pop3->rd_source = 0;
			ERR_error_string(SSL_get_error(pop3->ssl, cnt), buf);
			_pop3_event_status(pop3, AS_DISCONNECTED, buf);
			_pop3_stop(pop3);
		}
		return FALSE;
	}
#ifdef DEBUG
	fprintf(stderr, "%s", "DEBUG: POP3 SERVER: ");
	fwrite(&pop3->rd_buf[pop3->rd_buf_cnt], sizeof(*p), cnt, stderr);
#endif
	pop3->rd_buf_cnt += cnt;
	if(_pop3_parse(pop3) != 0)
	{
		_pop3_stop(pop3);
		return FALSE;
	}
	if(pop3->queue_cnt == 0)
		return TRUE;
	cmd = &pop3->queue[0];
	if(cmd->buf_cnt == 0)
	{
		if(cmd->status == P3CS_SENT || cmd->status == P3CS_PARSING)
			/* begin or keep parsing */
			return TRUE;
		else if(cmd->status == P3CS_OK || cmd->status == P3CS_ERROR)
			/* the current command is completed */
			memmove(cmd, &pop3->queue[1], sizeof(*cmd)
					* --pop3->queue_cnt);
	}
	if(pop3->queue_cnt == 0)
	{
		_pop3_event_status(pop3, AS_IDLE, NULL);
		pop3->source = g_timeout_add(30000, _on_noop, pop3);
	}
	else
		pop3->wr_source = g_io_add_watch(pop3->channel, G_IO_OUT,
				_on_watch_can_write_ssl, pop3);
	return TRUE;
}


/* on_watch_can_write */
static gboolean _on_watch_can_write(GIOChannel * source, GIOCondition condition,
		gpointer data)
{
	POP3 * pop3 = data;
	AccountPluginHelper * helper = pop3->helper;
	POP3Command * cmd = &pop3->queue[0];
	gsize cnt = 0;
	GError * error = NULL;
	GIOStatus status;
	char * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(condition != G_IO_OUT || source != pop3->channel
			|| pop3->queue_cnt == 0 || cmd->buf_cnt == 0)
		return FALSE; /* should not happen */
	status = g_io_channel_write_chars(source, cmd->buf, cmd->buf_cnt, &cnt,
			&error);
#ifdef DEBUG
	fprintf(stderr, "%s", "DEBUG: POP3 CLIENT: ");
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
			_pop3_stop(pop3);
			return FALSE;
		case G_IO_STATUS_EOF:
		default:
			_pop3_event_status(pop3, AS_DISCONNECTED, NULL);
			_pop3_stop(pop3);
			return FALSE;
	}
	if(cmd->buf_cnt > 0)
		return TRUE;
	cmd->status = P3CS_SENT;
	pop3->wr_source = 0;
	if(pop3->rd_source == 0)
		/* XXX should not happen */
		pop3->rd_source = g_io_add_watch(pop3->channel, G_IO_IN,
				_on_watch_can_read, pop3);
	return FALSE;
}


/* on_watch_can_write_ssl */
static gboolean _on_watch_can_write_ssl(GIOChannel * source,
		GIOCondition condition, gpointer data)
{
	POP3 * pop3 = data;
	POP3Command * cmd = &pop3->queue[0];
	int cnt;
	char * p;
	char buf[128];

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((condition != G_IO_OUT && condition != G_IO_IN)
			|| source != pop3->channel || pop3->queue_cnt == 0
			|| cmd->buf_cnt == 0)
		return FALSE; /* should not happen */
	if((cnt = SSL_write(pop3->ssl, cmd->buf, cmd->buf_cnt)) <= 0)
	{
		if(SSL_get_error(pop3->ssl, cnt) == SSL_ERROR_WANT_READ)
			pop3->wr_source = g_io_add_watch(pop3->channel, G_IO_IN,
					_on_watch_can_write_ssl, pop3);
		else if(SSL_get_error(pop3->ssl, cnt) == SSL_ERROR_WANT_WRITE)
			pop3->wr_source = g_io_add_watch(pop3->channel,
					G_IO_OUT, _on_watch_can_write_ssl,
					pop3);
		else
		{
			ERR_error_string(SSL_get_error(pop3->ssl, cnt), buf);
			_pop3_event_status(pop3, AS_DISCONNECTED, buf);
			_pop3_stop(pop3);
		}
		return FALSE;
	}
#ifdef DEBUG
	fprintf(stderr, "%s", "DEBUG: POP3 CLIENT: ");
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
	cmd->status = P3CS_SENT;
	pop3->wr_source = 0;
	if(pop3->rd_source == 0)
		/* XXX should not happen */
		pop3->rd_source = g_io_add_watch(pop3->channel, G_IO_IN,
				_on_watch_can_read_ssl, pop3);
	return FALSE;
}
