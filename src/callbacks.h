/* $Id$ */
/* Copyright (c) 2006-2012 Pierre Pronchery <khorben@defora.org> */
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



#ifndef MAILER_CALLBACKS_H
# define MAILER_CALLBACKS_H

# include <gtk/gtk.h>


/* mailer window */
gboolean on_closex(gpointer data);

/* file menu */
void on_file_new_mail(gpointer data);
void on_file_open(gpointer data);
void on_file_send_receive(gpointer data);
void on_file_quit(gpointer data);

/* edit menu */
void on_edit_cut(gpointer data);
void on_edit_copy(gpointer data);
void on_edit_paste(gpointer data);
void on_edit_preferences(gpointer data);
void on_edit_select_all(gpointer data);
void on_edit_unselect_all(gpointer data);

/* message menu */
void on_message_delete(gpointer data);
void on_message_forward(gpointer data);
void on_message_reply(gpointer data);
void on_message_reply_to_all(gpointer data);
void on_message_save_as(gpointer data);
void on_message_view_source(gpointer data);

/* help menu */
void on_help_contents(gpointer data);
void on_help_about(gpointer data);

/* toolbar */
void on_new_mail(gpointer data);
void on_open(gpointer data);
void on_reply(gpointer data);
void on_reply_to_all(gpointer data);
void on_forward(gpointer data);
void on_delete(gpointer data);
void on_preferences(gpointer data);
void on_quit(gpointer data);
void on_view_source(gpointer data);

/* body view */
gboolean on_body_closex(gpointer data);

/* folder view */
void on_folder_change(GtkTreeSelection * selection, gpointer data);

/* header view */
void on_header_change(GtkTreeSelection * selection, gpointer data);
gboolean on_headers_closex(gpointer data);

/* plug-ins */
gboolean on_plugins_closex(gpointer data);

#endif /* !MAILER_CALLBACKS_H */
