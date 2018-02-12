/* $Id$ */
/* Copyright (c) 2012 Pierre Pronchery <khorben@defora.org> */
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



#ifndef DESKTOP_MAILER_PLUGIN_H
# define DESKTOP_MAILER_PLUGIN_H

# include <gtk/gtk.h>
# include "mailer.h"


/* MailerPlugin */
/* types */
typedef struct _MailerPluginHelper
{
	Mailer * mailer;
	int (*error)(Mailer * mailer, char const * message, int ret);
} MailerPluginHelper;

typedef struct _MailerPlugin MailerPlugin;

typedef const struct _MailerPluginDefinition
{
	char const * name;
	char const * icon;
	char const * description;
	MailerPlugin * (*init)(MailerPluginHelper * helper);
	void (*destroy)(MailerPlugin * plugin);
	GtkWidget * (*get_widget)(MailerPlugin * plugin);
	void (*refresh)(MailerPlugin * plugin, MailerFolder * folder,
			MailerMessage * message);
} MailerPluginDefinition;

#endif /* !DESKTOP_MAILER_PLUGIN_H */
