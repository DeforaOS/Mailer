/* $Id$ */
/* Copyright (c) 2012 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Mailer */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */



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
	NULL,
	NULL,
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
