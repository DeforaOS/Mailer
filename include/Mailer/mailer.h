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



#ifndef DESKTOP_MAILER_MAILER_H
# define DESKTOP_MAILER_MAILER_H


/* Mailer */
/* types */
typedef struct _Mailer Mailer;

typedef enum _MailerFolderColumn
{
	MFC_ACCOUNT = 0, MFC_ENABLED, MFC_DELETE, MFC_FOLDER, MFC_ICON, MFC_NAME
} MailerFolderColumn;
# define MFC_LAST MFC_NAME
# define MFC_COUNT (MFC_LAST + 1)

typedef enum _MailerHeaderColumn
{
	MHC_ACCOUNT = 0, MHC_FOLDER, MHC_MESSAGE, MHC_ICON, MHC_SUBJECT,
	MHC_FROM, MHC_FROM_EMAIL, MHC_TO, MHC_TO_EMAIL, MHC_DATE,
	MHC_DATE_DISPLAY, MHC_READ, MHC_WEIGHT
} MailerHeaderColumn;
# define MHC_LAST MHC_WEIGHT
# define MHC_COUNT (MHC_LAST + 1)

/* folders */
typedef struct _MailerFolder MailerFolder;

/* messages */
typedef struct _MailerMessage MailerMessage;

#endif /* !DESKTOP_MAILER_MAILER_H */
