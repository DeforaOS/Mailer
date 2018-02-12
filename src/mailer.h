/* $Id$ */
/* Copyright (c) 2006-2012 Pierre Pronchery <khorben@defora.org> */
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



#ifndef MAILER_SRC_MAILER_H
# define MAILER_SRC_MAILER_H

# include <System.h>
# include <gtk/gtk.h>
# include "../include/Mailer.h"
# include "account.h"


/* Mailer */
/* constants */
# define MAILER_CONFIG_FILE	".mailer"

# define MAILER_MESSAGES_FONT	"Monospace 9"


/* functions */
Mailer * mailer_new(void);
void mailer_delete(Mailer * mailer);

/* accessors */
char const * mailer_get_config(Mailer * mailer, char const * variable);
SSL_CTX * mailer_get_ssl_context(Mailer * mailer);

gboolean mailer_is_online(Mailer * mailer);

void mailer_set_online(Mailer * mailer, gboolean online);
void mailer_set_status(Mailer * mailer, char const * status);

/* useful */
int mailer_error(Mailer * mailer, char const * message, int ret);

void mailer_refresh_all(Mailer * mailer);

/* accounts */
int mailer_account_add(Mailer * mailer, Account * account);
#if 0 /* FIXME deprecate? */
int mailer_account_disable(Mailer * mailer, Account * account);
int mailer_account_enable(Mailer * mailer, Account * account);
#endif
/* FIXME implement
int mailer_account_remove(Mailer * mailer, Account * account); */

/* plug-ins */
int mailer_load(Mailer * mailer, char const * plugin);
int mailer_unload(Mailer * mailer, char const * plugin);

void mailer_compose(Mailer * mailer);

/* selection */
void mailer_delete_selected(Mailer * mailer);

void mailer_open_selected_source(Mailer * mailer);

void mailer_reply_selected(Mailer * mailer);
void mailer_reply_selected_to_all(Mailer * mailer);

gboolean mailer_save_selected(Mailer * mailer, char const * filename);
gboolean mailer_save_selected_dialog(Mailer * mailer);

void mailer_select_all(Mailer * mailer);
void mailer_unselect_all(Mailer * mailer);

/* clipboard */
void mailer_cut(Mailer * mailer);
void mailer_copy(Mailer * mailer);
void mailer_paste(Mailer * mailer);

/* messages */
gboolean mailer_message_open(Mailer * mailer, char const * filename);
gboolean mailer_message_open_dialog(Mailer * mailer);

/* interface */
void mailer_show_about(Mailer * mailer, gboolean show);
void mailer_show_body(Mailer * mailer, gboolean show);
void mailer_show_headers(Mailer * mailer, gboolean show);
void mailer_show_plugins(Mailer * mailer, gboolean show);
void mailer_show_preferences(Mailer * mailer, gboolean show);

#endif /* !MAILER_SRC_MAILER_H */
