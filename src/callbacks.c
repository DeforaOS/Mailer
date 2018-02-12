/* $Id$ */
/* Copyright (c) 2006-2018 Pierre Pronchery <khorben@defora.org> */
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
/* TODO:
 * - check that the messages font button is initialized correctly */



#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <Desktop.h>
#include "compose.h"
#include "mailer.h"
#include "callbacks.h"
#include "../config.h"

/* constants */
#ifndef PROGNAME
# define PROGNAME_MAILER "mailer"
#endif


/* public */
/* functions */
/* callbacks */
/* on_closex */
gboolean on_closex(gpointer data)
{
	(void) data;

	/* FIXME may be composing or viewing messages */
	gtk_main_quit();
	return FALSE;
}


/* on_headers_closex */
gboolean on_headers_closex(gpointer data)
{
	Mailer * mailer = data;

	mailer_show_headers(mailer, FALSE);
	return TRUE;
}


/* on_body_closex */
gboolean on_body_closex(gpointer data)
{
	Mailer * mailer = data;

	mailer_show_body(mailer, FALSE);
	return TRUE;
}


/* on_plugins_closex */
gboolean on_plugins_closex(gpointer data)
{
	Mailer * mailer = data;

	mailer_show_plugins(mailer, FALSE);
	return TRUE;
}


/* file menu */
/* on_file_new_mail */
void on_file_new_mail(gpointer data)
{
	on_new_mail(data);
}


/* on_file_open */
void on_file_open(gpointer data)
{
	on_open(data);
}


/* on_file_send_receive */
void on_file_send_receive(gpointer data)
{
	Mailer * mailer = data;

	mailer_refresh_all(mailer);
}


/* on_file_quit */
void on_file_quit(gpointer data)
{
	on_closex(data);
}


/* message menu */
void on_message_delete(gpointer data)
{
	Mailer * mailer = data;

	mailer_delete_selected(mailer);
}


void on_message_reply(gpointer data)
{
	Mailer * mailer = data;

	mailer_reply_selected(mailer);
}


void on_message_reply_to_all(gpointer data)
{
	Mailer * mailer = data;

	mailer_reply_selected_to_all(mailer);
}


void on_message_forward(gpointer data)
{
	on_forward(data);
}


void on_message_save_as(gpointer data)
{
	Mailer * mailer = data;

	mailer_save_selected_dialog(mailer);
}


void on_message_view_source(gpointer data)
{
	Mailer * mailer = data;

	mailer_open_selected_source(mailer);
}


/* edit menu */
void on_edit_cut(gpointer data)
{
	Mailer * mailer = data;

	mailer_cut(mailer);
}


void on_edit_copy(gpointer data)
{
	Mailer * mailer = data;

	mailer_copy(mailer);
}


void on_edit_paste(gpointer data)
{
	Mailer * mailer = data;

	mailer_paste(mailer);
}


void on_edit_preferences(gpointer data)
{
	Mailer * mailer = data;

	mailer_show_preferences(mailer, TRUE);
}


void on_edit_select_all(gpointer data)
{
	Mailer * mailer = data;

	mailer_select_all(mailer);
}


void on_edit_unselect_all(gpointer data)
{
	Mailer * mailer = data;

	mailer_unselect_all(mailer);
}


/* help menu */
/* on_help_contents */
void on_help_contents(gpointer data)
{
	(void) data;

	desktop_help_contents(PACKAGE, PROGNAME_MAILER);
}


/* on_help_about */
void on_help_about(gpointer data)
{
	Mailer * mailer = data;

	mailer_show_about(mailer, TRUE);
}


/* toolbar */
/* on_delete */
void on_delete(gpointer data)
{
	Mailer * mailer = data;

	mailer_delete_selected(mailer);
}


/* on_forward */
void on_forward(gpointer data)
{
	/* FIXME return directly if there is no selection */
	on_new_mail(data);
	/* FIXME really implement */
}


/* on_new_mail */
void on_new_mail(gpointer data)
{
	Mailer * mailer = data;

	mailer_compose(mailer);
}


/* on_open */
void on_open(gpointer data)
{
	Mailer * mailer = data;

	mailer_message_open_dialog(mailer);
}


/* on_preferences */
void on_preferences(gpointer data)
{
	Mailer * mailer = data;

	mailer_show_preferences(mailer, TRUE);
}


/* on_quit */
void on_quit(gpointer data)
{
	on_closex(data);
}


/* on_reply */
void on_reply(gpointer data)
{
	Mailer * mailer = data;

	mailer_reply_selected(mailer);
}


/* on_reply_to_all */
void on_reply_to_all(gpointer data)
{
	Mailer * mailer = data;

	mailer_reply_selected_to_all(mailer);
}


/* on_view_source */
void on_view_source(gpointer data)
{
	Mailer * mailer = data;

	mailer_open_selected_source(mailer);
}
