/* $Id$ */
/* Copyright (c) 2011-2015 Pierre Pronchery <khorben@defora.org> */
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



#include <stdlib.h>
#include "Mailer/folder.h"
#include "Mailer/message.h"
#include "Mailer/plugin.h"


/* Mailing-lists */
/* private */
/* types */
typedef struct _MailerPlugin MailingLists;

struct _MailerPlugin
{
	MailerPluginHelper * helper;

	/* widgets */
	GtkWidget * vbox;
	GtkWidget * folder;
	GtkWidget * message;
	GtkWidget * name;
};


/* protected */
/* prototypes */
/* plug-in */
static MailerPlugin * _ml_init(MailerPluginHelper * helper);
static void _ml_destroy(MailingLists * ml);
static GtkWidget * _ml_get_widget(MailingLists * ml);
static void _ml_refresh(MailingLists * ml, MailerFolder * folder,
		MailerMessage * message);


/* public */
/* variables */
/* plug-in */
MailerPluginDefinition plugin =
{
	"Mailing-lists",
	"mail-reply-all",
	"Mailing-lists management",
	_ml_init,
	_ml_destroy,
	_ml_get_widget,
	_ml_refresh
};


/* protected */
/* functions */
/* plug-in */
/* ml_init */
static MailerPlugin * _ml_init(MailerPluginHelper * helper)
{
	MailingLists * ml;
	PangoFontDescription * bold;

	if((ml = malloc(sizeof(*ml))) == NULL)
		return NULL;
	ml->helper = helper;
	/* widgets */
	bold = pango_font_description_new();
	pango_font_description_set_weight(bold, PANGO_WEIGHT_BOLD);
#if GTK_CHECK_VERSION(3, 0, 0)
	ml->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	ml->vbox = gtk_vbox_new(FALSE, 4);
#endif
	ml->folder = gtk_label_new("");
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_override_font(ml->folder, bold);
#else
	gtk_widget_modify_font(ml->folder, bold);
#endif
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(ml->folder, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(ml->folder), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(ml->vbox), ml->folder, FALSE, TRUE, 0);
	ml->message = gtk_label_new("");
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(ml->message, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(ml->message), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(ml->vbox), ml->message, FALSE, TRUE, 0);
	ml->name = gtk_label_new("");
#if GTK_CHECK_VERSION(3, 14, 0)
	g_object_set(ml->name, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(ml->name), 0.0, 0.5);
#endif
	gtk_box_pack_start(GTK_BOX(ml->vbox), ml->name, FALSE, TRUE, 0);
	pango_font_description_free(bold);
	return ml;
}


/* ml_destroy */
static void _ml_destroy(MailingLists * ml)
{
	free(ml);
}


/* ml_get_widget */
static GtkWidget * _ml_get_widget(MailingLists * ml)
{
	return ml->vbox;
}


/* ml_refresh */
static void _ml_refresh(MailingLists * ml, MailerFolder * folder,
		MailerMessage * message)
{
	char const * id;

	if(folder == NULL)
	{
		gtk_widget_hide(ml->folder);
		gtk_widget_hide(ml->message);
		gtk_widget_hide(ml->name);
		return;
	}
	gtk_label_set_text(GTK_LABEL(ml->folder), folder_get_name(folder));
	gtk_widget_show(ml->folder);
	if(message == NULL)
	{
		gtk_widget_hide(ml->message);
		gtk_widget_hide(ml->name);
		return;
	}
	if((id = message_get_header(message, "List-Id")) == NULL)
	{
		gtk_label_set_text(GTK_LABEL(ml->message),
				"Not a mailing-list");
		gtk_widget_show(ml->message);
		gtk_widget_hide(ml->name);
		return;
	}
	/* XXX parse and beautify the list's name */
	gtk_widget_hide(ml->message);
	gtk_label_set_text(GTK_LABEL(ml->name), id);
	gtk_widget_show(ml->name);
}
