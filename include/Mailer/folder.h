/* $Id$ */
/* Copyright (c) 2011-2018 Pierre Pronchery <khorben@defora.org> */
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



#ifndef DESKTOP_MAILER_FOLDER_H
# define DESKTOP_MAILER_FOLDER_H

# include "mailer.h"


/* Folder */
/* types */
typedef struct _MailerFolder Folder;

typedef struct _AccountFolder AccountFolder;

typedef enum _MailerFolderType
{
	FT_INBOX = 0,
	FT_DRAFTS,
	FT_SENT,
	FT_TRASH,
	FT_FOLDER
} FolderType;
# define FT_LAST FT_FOLDER
# define FT_COUNT (FT_LAST + 1)


/* public */
/* functions */
/* accessors */
char const * folder_get_name(MailerFolder * folder);
FolderType folder_get_type(MailerFolder * folder);
void folder_set_type(MailerFolder * folder, FolderType type);

#endif /* !DESKTOP_MAILER_FOLDER_H */
