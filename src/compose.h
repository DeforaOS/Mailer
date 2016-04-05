/* $Id$ */
/* Copyright (c) 2006-2015 Pierre Pronchery <khorben@defora.org> */
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



#ifndef MAILER_COMPOSE_H
# define MAILER_COMPOSE_H

# include <sys/types.h>
# include <glib.h>
# include <System.h>
# include "message.h"


/* types */
typedef struct _Compose Compose;

/* methods */
Compose * compose_new(Config * config);
Compose * compose_new_copy(Compose * compose);
Compose * compose_new_open(Config * config, Message * message);
void compose_delete(Compose * compose);

/* accessors */
void compose_set_font(Compose * compose, char const * font);
void compose_set_from(Compose * compose, char const * from);
void compose_set_header(Compose * compose, char const * header,
		char const * value, gboolean visible);
void compose_set_modified(Compose * compose, gboolean modified);
void compose_set_standalone(Compose * compose, gboolean standalone);
void compose_set_subject(Compose * compose, char const * subject);
void compose_set_text(Compose * compose, char const * text);

/* useful */
void compose_add_field(Compose * compose, char const * field,
		char const * value);

int compose_append_signature(Compose * compose);
void compose_append_text(Compose * compose, char const * text);

void compose_attach_dialog(Compose * compose);

void compose_copy(Compose * compose);
void compose_cut(Compose * compose);
void compose_paste(Compose * compose);

int compose_error(Compose * compose, char const * message, int ret);

int compose_save(Compose * compose);
int compose_save_as_dialog(Compose * compose);

void compose_scroll_to_offset(Compose * compose, int offset);

void compose_select_all(Compose * compose);

void compose_send(Compose * compose);
void compose_send_cancel(Compose * compose);

void compose_show_about(Compose * compose, gboolean show);

#endif /* !MAILER_COMPOSE_H */
