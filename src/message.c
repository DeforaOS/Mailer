/* $Id$ */
/* Copyright (c) 2011-2012 Pierre Pronchery <khorben@defora.org> */
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



#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <System.h>
#include "mailer.h"
#include "message.h"


/* Message */
/* private */
/* types */
typedef struct _MessageHeader
{
	char * header;
	char * value;
} MessageHeader;

struct _MailerMessage
{
	GtkTreeStore * store;
	GtkTreeRowReference * row;

	int flags;

	MessageHeader * headers;
	size_t headers_cnt;

	char * body;
	size_t body_cnt;

	GtkTextBuffer * text;

	char ** attachments;
	size_t attachments_cnt;

	AccountMessage * data;
};


/* prototypes */
/* accessors */
static gboolean _message_set(Message * message, ...);
static int _message_set_date(Message * message, char const * date);
static int _message_set_from(Message * message, char const * from);
static int _message_set_status(Message * message, char const * status);
static int _message_set_to(Message * message, char const * to);

/* useful */
/* message headers */
static int _message_header_set(MessageHeader * mh, char const * header,
		char const * value);


/* constants */
static struct
{
	char const * header;
	MailerHeaderColumn column;
	int (*callback)(Message * message, char const * value);
} _message_columns[] =
{
	{ "Date",	0,			_message_set_date	},
	{ "From",	0,			_message_set_from	},
	{ "Status",	0,			_message_set_status	},
	{ "Subject",	MHC_SUBJECT,		NULL			},
	{ "To",		0,			_message_set_to		},
	{ NULL,		0,			NULL			}
};


/* public */
/* functions */
/* message_new */
Message * message_new(AccountMessage * message, GtkTreeStore * store,
		GtkTreeIter * iter)
{
	Message * ret;
	GtkTreePath * path;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%p, %p, %p)\n", __func__, (void *)message,
			(void *)store, (void *)iter);
#endif
	if((ret = object_new(sizeof(*ret))) == NULL)
		return NULL;
	if((ret->store = store) != NULL)
	{
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), iter);
		ret->row = gtk_tree_row_reference_new(GTK_TREE_MODEL(store),
				path);
		gtk_tree_path_free(path);
		gtk_tree_store_set(store, iter, MHC_MESSAGE, ret, -1);
	}
	else
		ret->row = NULL;
	ret->flags = 0;
	ret->headers = NULL;
	ret->headers_cnt = 0;
	ret->body = NULL;
	ret->body_cnt = 0;
	ret->text = gtk_text_buffer_new(NULL);
	ret->attachments = NULL;
	ret->attachments_cnt = 0;
	ret->data = message;
	_message_set_date(ret, NULL);
	_message_set_status(ret, NULL);
	return ret;
}


/* message_new_open */
Message * message_new_open(Mailer * mailer, char const * filename)
{
	Message * message;
	Config * config;
	Account * account;

	if((message = message_new(NULL, NULL, NULL)) == NULL)
		return NULL;
	if((config = config_new()) == NULL
			|| config_set(config, "title", "mbox", filename) != 0)
	{
		if(config != NULL)
			config_delete(config);
		message_delete(message);
		return NULL;
	}
	if((account = account_new(mailer, "mbox", "title", NULL)) == NULL
			|| account_init(account) != 0
			|| account_config_load(account, config) != 0
			|| account_start(account) != 0)
	{
		if(account != NULL)
			account_delete(account);
		config_delete(config);
		message_delete(message);
		return NULL;
	}
	/* FIXME really implement; possibly:
	 * - set different helpers for account;
	 * - delete the account once the message loaded;
	 * - implement and use the Transport class instead. */
	config_delete(config);
	account_delete(account);
	return message;
}


/* message_delete */
void message_delete(Message * message)
{
	if(message->row != NULL)
		gtk_tree_row_reference_free(message->row);
	g_object_unref(message->text);
	free(message->body);
	free(message->headers);
	object_delete(message);
}


/* accessors */
/* message_get_body */
GtkTextBuffer * message_get_body(Message * message)
{
	return message->text;
}


/* message_get_data */
AccountMessage * message_get_data(Message * message)
{
	return message->data;
}


/* message_get_flags */
int message_get_flags(Message * message)
{
	return message->flags;
}


/* message_get_header */
char const * message_get_header(Message * message, char const * header)
{
	size_t i;

	for(i = 0; i < message->headers_cnt; i++)
		if(strcmp(message->headers[i].header, header) == 0)
			return message->headers[i].value;
	return NULL;
}


/* message_get_iter */
gboolean message_get_iter(Message * message, GtkTreeIter * iter)
{
	GtkTreePath * path;

	if(message->row == NULL)
		return FALSE;
	if((path = gtk_tree_row_reference_get_path(message->row)) == NULL)
		return FALSE;
	return gtk_tree_model_get_iter(GTK_TREE_MODEL(message->store), iter,
			path);
}


/* message_get_store */
GtkTreeStore * message_get_store(Message * message)
{
	return message->store;
}


/* message_set_body */
int message_set_body(Message * message, char const * buf, size_t cnt,
		gboolean append)
{
	char * p;
	GtkTextIter iter;

	if(buf == NULL)
		buf = "";
	if(append != TRUE)
	{
		/* empty the message body */
		free(message->body);
		message->body = NULL;
		message->body_cnt = 0;
		gtk_text_buffer_set_text(message->text, "", 0);
	}
	if((p = realloc(message->body, (message->body_cnt + cnt) * sizeof(*p)))
			== NULL)
		return -1;
	message->body = p;
	memcpy(&message->body[message->body_cnt], buf, cnt);
	message->body_cnt += cnt;
	/* FIXME:
	 * - check encoding
	 * - parse MIME, etc... */
	gtk_text_buffer_get_end_iter(message->text, &iter);
	gtk_text_buffer_insert(message->text, &iter, buf, cnt);
	return 0;
}


/* message_set_flag */
void message_set_flag(Message * message, MailerMessageFlag flag)
{
	message->flags |= flag;
}


/* message_set_flags */
void message_set_flags(Message * message, int flags)
{
	message->flags = flags;
}


/* message_set_header */
int message_set_header(Message * message, char const * header)
{
	int ret;
	size_t i;
	char * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%p, \"%s\")\n", __func__, (void*)message,
			header);
#endif
	if(header == NULL)
		return -1;
	for(i = 0; header[i] != '\0' && header[i] != ':'; i++);
	/* FIXME white-space is optional */
	if(header[i] == '\0' || header[i + 1] != ' ')
		/* XXX unstructured headers are not supported */
		return -1;
	if((p = malloc(i + 1)) == NULL)
		return -1;
	snprintf(p, i + 1, "%s", header);
	ret = message_set_header_value(message, p, &header[i + 2]);
	free(p);
	return ret;
}


/* message_set_header_value */
int message_set_header_value(Message * message, char const * header,
		char const * value)
{
	size_t i;
	MessageHeader * p;
	MailerHeaderColumn column;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%p, \"%s\", \"%s\")\n", __func__,
			(void *)message, header, value);
#endif
	/* FIXME remove the header when value == NULL */
	for(i = 0; i < message->headers_cnt; i++)
		if(strcmp(message->headers[i].header, header) == 0)
			break;
	if(i == message->headers_cnt)
	{
		/* the header was not found */
		if(value == NULL)
			return 0;
		/* append the header */
		if((p = realloc(message->headers, sizeof(*p)
						* (message->headers_cnt + 1)))
				== NULL)
			return -1;
		message->headers = p;
		p = &message->headers[message->headers_cnt];
		memset(p, 0, sizeof(*p));
		if(_message_header_set(p, header, value) != 0)
			return -1;
		message->headers_cnt++;
	}
	else if(_message_header_set(&message->headers[i], NULL, value) != 0)
		return -1;
	/* FIXME parse/convert input */
	for(i = 0; _message_columns[i].header != NULL; i++)
	{
		if(strcmp(_message_columns[i].header, header) != 0)
			continue;
		if((column = _message_columns[i].column) != 0)
			_message_set(message, column, value, -1);
		if(_message_columns[i].callback == NULL)
			return 0;
		return _message_columns[i].callback(message, value);
	}
	return 0;
}


/* message_set_read */
void message_set_read(Message * message, gboolean read)
{
	char const * status;
	char * p;
	size_t i;

	if((status = message_get_header(message, "Status")) == NULL)
	{
		message_set_header(message, read ? "Status: RO" : "Status: O");
		return;
	}
	if(!read)
	{
		if((p = strdup(status)) == NULL)
			return; /* XXX report error */
		for(i = 0; p[i] != '\0' && p[i] != 'R'; i++);
		if(p[i] == 'R')
			for(; p[i] != '\0'; i++)
				p[i] = p[i + 1];
		message_set_header_value(message, "Status", p);
		free(p);
	}
	else if(strchr(status, 'R') == NULL)
	{
		i = strlen(status);
		if((p = malloc(i + 2)) == NULL)
			return; /* XXX report error */
		snprintf(p, i + 2, "%c%s", 'R', status);
		message_set_header_value(message, "Status", p);
		free(p);
	}
}


/* useful */
/* message_save
 * XXX may not save the message exactly like the original */
static int _save_from(MailerMessage * message, FILE * fp);
static int _save_headers(MailerMessage * message, FILE * fp);
static int _save_body(MailerMessage * message, FILE * fp);

int message_save(MailerMessage * message, char const * filename)
{
	FILE * fp;

	if((fp = fopen(filename, "w")) == NULL)
		return -1;
	if(_save_from(message, fp) != 0 || _save_headers(message, fp) != 0
			|| _save_body(message, fp) != 0)
	{
		fclose(fp);
		return -1;
	}
	if(fclose(fp) != 0)
		return -1;
	return 0;
}

static int _save_from(MailerMessage * message, FILE * fp)
{
	char const * p;

	if((p = message_get_header(message, "From")) == NULL)
		p = "unknown-sender";
	if(fputs("From ", fp) != 0 || fputs(p, fp) != 0)
		return -1;
	if((p = message_get_header(message, "Date")) != NULL)
		if(fputs(" ", fp) != 0 || fputs(p, fp) != 0)
			return -1;
	if(fputs("\n", fp) != 0)
		return -1;
	return 0;
}

static int _save_headers(MailerMessage * message, FILE * fp)
{
	size_t i;

	/* output the headers */
	for(i = 0; i < message->headers_cnt; i++)
		if(fputs(message->headers[i].header, fp) != 0
				|| fputs(": ", fp) != 0
				|| fputs(message->headers[i].value, fp) != 0
				|| fputs("\n", fp) != 0)
			return -1;
	if(fputs("\n", fp) != 0)
		return -1;
	return 0;
}

static int _save_body(MailerMessage * message, FILE * fp)
{
	GtkTextIter start;
	GtkTextIter end;
	gchar * text;
	int res;

	/* output the body */
	/* FIXME implement properly */
	gtk_text_buffer_get_start_iter(message->text, &start);
	gtk_text_buffer_get_end_iter(message->text, &end);
	text = gtk_text_buffer_get_text(message->text, &start, &end, TRUE);
	res = fputs(text, fp);
	g_free(text);
	return (res == 0) ? 0 : -1;
}


/* private */
/* functions */
/* accessors */
/* message_set */
static gboolean _message_set(Message * message, ...)
{
	va_list ap;
	GtkTreeIter iter;

	if(message_get_iter(message, &iter) != TRUE)
		return FALSE;
	va_start(ap, message);
	gtk_tree_store_set_valist(message->store, &iter, ap);
	va_end(ap);
	return TRUE;
}


/* message_set_date */
static int _message_set_date(Message * message, char const * date)
{
	struct tm tm;
	time_t t;
	char buf[32];

	t = mailer_helper_get_date(date, &tm);
	strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &tm);
	_message_set(message, MHC_DATE, t, MHC_DATE_DISPLAY, buf, -1);
	return 0;
}


/* message_set_from */
static int _message_set_from(Message * message, char const * from)
{
	char * name;
	char * email;

	if((email = mailer_helper_get_email(from)) == NULL)
		return -1;
	name = mailer_helper_get_name(from);
	_message_set(message, MHC_FROM, (name != NULL) ? name : email,
			MHC_FROM_EMAIL, email, -1);
	free(email);
	free(name);
	return 0;
}


/* message_set_status */
static int _message_set_status(Message * message, char const * status)
{
	gboolean read;
	GtkIconTheme * theme;
	GdkPixbuf * pixbuf;

	read = (status == NULL || strchr(status, 'R') != NULL) ? TRUE : FALSE;
	theme = gtk_icon_theme_get_default();
	pixbuf = gtk_icon_theme_load_icon(theme, read ? "mail-read"
			: "mail-unread", 16, 0, NULL);
	_message_set(message, MHC_READ, read, MHC_WEIGHT, (read)
			? PANGO_WEIGHT_NORMAL : PANGO_WEIGHT_BOLD, MHC_ICON,
			pixbuf, -1);
	return 0;
}


/* message_set_to */
static int _message_set_to(Message * message, char const * to)
{
	char * name;
	char * email;

	if((email = mailer_helper_get_email(to)) == NULL)
		return -1;
	name = mailer_helper_get_name(to);
	_message_set(message, MHC_TO, (name != NULL) ? name : email,
			MHC_TO_EMAIL, email, -1);
	free(email);
	free(name);
	return 0;
}


/* useful */
/* message headers */
/* message_header_set */
static int _message_header_set(MessageHeader * mh, char const * header,
		char const * value)
{
	int ret = 0;
	char * h = NULL;
	char * v = NULL;

	if(header != NULL && (h = strdup(header)) == NULL)
		ret |= -1;
	if(value != NULL && (v = strdup(value)) == NULL)
		ret |= -1;
	if(ret != 0)
		return ret;
	if(h != NULL)
	{
		free(mh->header);
		mh->header = h;
	}
	if(v != NULL)
	{
		free(mh->value);
		mh->value = v;
	}
	return 0;
}
