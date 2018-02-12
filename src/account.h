/* $Id$ */
/* Copyright (c) 2006-2013 Pierre Pronchery <khorben@defora.org> */
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



#ifndef MAILER_ACCOUNT_H
# define MAILER_ACCOUNT_H

# include <System.h>
# include "../include/Mailer.h"


/* Account */
/* types */


/* functions */
Account * account_new(Mailer * mailer, char const * type, char const * title,
		GtkTreeStore * store);
void account_delete(Account * account);

/* accessors */
int account_get_enabled(Account * account);
void account_set_enabled(Account * account, int enabled);

AccountConfig const * account_get_config(Account * account);
char const * account_get_name(Account * account);
char const * account_get_title(Account * account);
char const * account_get_type(Account * account);
int account_set_name(Account * account, char const * name);

/* useful */
int account_config_load(Account * account, Config * config);
int account_config_save(Account * account, Config * config);

int account_init(Account * account);
int account_quit(Account * account);

void account_refresh(Account * account);
int account_start(Account * account);
void account_stop(Account * account);

void account_store(Account * account, GtkTreeStore * store);

GtkTextBuffer * account_select(Account * account, Folder * folder,
		Message * message);
GtkTextBuffer * account_select_source(Account * account, Folder * folder,
		Message * message);

#endif /* !MAILER_ACCOUNT_H */
