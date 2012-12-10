/* $Id$ */
/* Copyright (c) 2011-2012 Pierre Pronchery <khorben@defora.org> */
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
