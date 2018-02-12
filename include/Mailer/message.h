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



#ifndef DESKTOP_MAILER_MESSAGE_H
# define DESKTOP_MAILER_MESSAGE_H

# include "mailer.h"


/* Message */
/* types */
typedef struct _MailerMessage Message;

typedef enum _MailerMessageFlag
{
	MMF_READ	= 0x1,
	MMF_ANSWERED	= 0x2,
	MMF_URGENT	= 0x4,
	MMF_DRAFT	= 0x8,
	MMF_DELETED	= 0x10
}
MailerMessageFlag;

typedef struct _AccountMessage AccountMessage;


/* functions */
/* accessors */
/* flags */
int message_get_flags(MailerMessage * message);
void message_set_flag(MailerMessage * message, MailerMessageFlag flag);
void message_set_flags(MailerMessage * message, int flags);

/* headers */
char const * message_get_header(MailerMessage * message, char const * header);

/* useful */
int message_save(MailerMessage * message, char const * filename);

#endif /* !DESKTOP_MAILER_MESSAGE_H */
