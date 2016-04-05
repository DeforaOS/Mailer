/* $Id$ */
/* Copyright (c) 2011-2012 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Mailer */
/* Redistribution and use in source and binary forms, with or without
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
 * 3. Neither the name of the authors nor the names of the contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. */



#ifndef DESKTOP_MAILER_ACCOUNT_H
# define DESKTOP_MAILER_ACCOUNT_H

# include <openssl/ssl.h>
# include "folder.h"
# include "message.h"


/* Account */
/* types */
typedef struct _Account Account;

/* AccountIdentity */
typedef struct _AccountIdentity
{
	char * from;
	char * email;
	char * signature;
} AccountIdentity;


/* AccountConfig */
typedef enum _AccountConfigType
{
	ACT_NONE = 0,
	/* values */
	ACT_STRING,
	ACT_PASSWORD,
	ACT_FILE,
	ACT_UINT16,
	ACT_BOOLEAN,
	/* layout */
	ACT_SEPARATOR
} AccountConfigType;

typedef struct _AccountConfig
{
	char const * name;
	char const * title;
	AccountConfigType type;
	void * value;
} AccountConfig;


/* AccountStatus */
typedef enum _AccountStatus
{
	AS_CONNECTING = 0,
	AS_CONNECTED,
	AS_DISCONNECTED,
	AS_AUTHENTICATED,
	AS_IDLE
} AccountStatus;


/* AccountEvent */
typedef enum _AccountEventType
{
	AET_STARTED = 0,
	AET_STOPPED,
	AET_STATUS
} AccountEventType;

typedef union _AccountEvent
{
	AccountEventType type;

	/* AET_STATUS */
	struct
	{
		AccountEventType type;
		AccountStatus status;
		char const * message;
	} status;
} AccountEvent;


/* AccountPlugin */
typedef struct _AccountPluginHelper
{
	Account * account;
	/* accessors */
	SSL_CTX * (*get_ssl_context)(Account * account);
	/* useful */
	int (*error)(Account * account, char const * message, int ret);
	void (*event)(Account * account, AccountEvent * event);
	/* authentication */
	char * (*authenticate)(Account * account, char const * message);
	int (*confirm)(Account * account, char const * message);
	/* folders */
	Folder * (*folder_new)(Account * account, AccountFolder * folder,
			Folder * parent, FolderType type, char const * name);
	void (*folder_delete)(Folder * folder);
	/* messages */
	Message * (*message_new)(Account * account, Folder * folder,
			AccountMessage * message);
	void (*message_delete)(Message * message);
	void (*message_set_flag)(MailerMessage * message,
			MailerMessageFlag flag);
	int (*message_set_header)(Message * message, char const * header);
	int (*message_set_body)(Message * message, char const * buf, size_t cnt,
			int append);
} AccountPluginHelper;

typedef struct _AccountPlugin AccountPlugin;

typedef const struct _AccountPluginDefinition
{
	char const * type;
	char const * name;
	char const * icon;
	char const * description;
	AccountConfig const * config;
	AccountPlugin * (*init)(AccountPluginHelper * helper);
	int (*destroy)(AccountPlugin * plugin);
	AccountConfig * (*get_config)(AccountPlugin * plugin);
	char * (*get_source)(AccountPlugin * plugin, AccountFolder * folder,
			AccountMessage * message);
	int (*start)(AccountPlugin * plugin);
	void (*stop)(AccountPlugin * plugin);
	int (*refresh)(AccountPlugin * plugin, AccountFolder * folder,
			AccountMessage * message);
} AccountPluginDefinition;

#endif /* !DESKTOP_MAILER_ACCOUNT_H */
