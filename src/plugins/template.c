/* $Id$ */
/* Copyright (c) 2012-2015 Pierre Pronchery <khorben@defora.org> */
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



#include <stdlib.h>
#include "Mailer/folder.h"
#include "Mailer/message.h"
#include "Mailer/plugin.h"


/* Mailing-lists */
/* private */
/* types */
typedef struct _MailerPlugin Template;

struct _MailerPlugin
{
	MailerPluginHelper * helper;
};


/* protected */
/* prototypes */
/* plug-in */
static MailerPlugin * _template_init(MailerPluginHelper * helper);
static void _template_destroy(Template * template);
static GtkWidget * _template_get_widget(Template * template);
static void _template_refresh(Template * template, MailerFolder * folder,
		MailerMessage * message);


/* public */
/* variables */
/* plug-in */
MailerPluginDefinition plugin =
{
	"Template",
	"applications-development",
	"Template plug-in description",
	_template_init,
	_template_destroy,
	_template_get_widget,
	_template_refresh
};


/* protected */
/* functions */
/* plug-in */
/* template_init */
static MailerPlugin * _template_init(MailerPluginHelper * helper)
{
	Template * template;

	if((template = malloc(sizeof(*template))) == NULL)
		return NULL;
	template->helper = helper;
	/* FIXME implement */
	return template;
}


/* template_destroy */
static void _template_destroy(Template * template)
{
	free(template);
}


/* template_get_widget */
static GtkWidget * _template_get_widget(Template * template)
{
	/* FIXME implement */
	return NULL;
}


/* template_refresh */
static void _template_refresh(Template * template, MailerFolder * folder,
		MailerMessage * message)
{
	/* FIXME implement */
}
