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



#ifndef MAILER_SRC_MESSAGE_H
# define MAILER_SRC_MESSAGE_H

# include <gtk/gtk.h>
# include "../include/Mailer.h"


/* Message */
/* types */


/* functions */
Message * message_new(AccountMessage * message, GtkTreeStore * store,
		GtkTreeIter * iter);
Message * message_new_open(Mailer * mailer, char const * filename);
void message_delete(Message * message);

/* accessors */
GtkTextBuffer * message_get_body(Message * message);
AccountMessage * message_get_data(Message * message);
gboolean message_get_iter(Message * message, GtkTreeIter * iter);
GtkTreeStore * message_get_store(Message * message);

int message_set_body(Message * message, char const * buf, size_t cnt,
		gboolean append);
int message_set_header(Message * message, char const * header);
int message_set_header_value(Message * message, char const * header,
		char const * value);
void message_set_read(Message * message, gboolean read);

#endif /* !MAILER_SRC_MAILER_H */
