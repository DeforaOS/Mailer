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



#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include "mailer.h"
#include "compose.h"
#include "../config.h"
#define _(string) gettext(string)

/* constants */
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef DATADIR
# define DATADIR	PREFIX "/share"
#endif
#ifndef LOCALEDIR
# define LOCALEDIR	DATADIR "/locale"
#endif
#ifndef PROGNAME
# define PROGNAME	"compose"
#endif


/* private */
/* prototypes */
static Compose * _compose(Config * config, char const * subject,
		int toc, char * tov[]);

static int _error(char const * message, int ret);
static int _usage(void);


/* functions */
/* compose */
static Compose * _compose(Config * config, char const * subject,
		int toc, char * tov[])
{
	Compose * compose;
	int i;

	if((compose = compose_new(config)) == NULL)
		return NULL;
	compose_set_standalone(compose, TRUE);
	/* recipients */
	if(toc > 0)
		compose_set_header(compose, "To:", tov[0], TRUE);
	for(i = 1; i < toc; i++)
		compose_add_field(compose, "To:", tov[i]);
	/* subject */
	if(subject != NULL)
		compose_set_subject(compose, subject);
	return compose;
}


/* compose_config */
static Config * _compose_config(void)
{
	Config * config;
	char const * homedir;
	String * filename;

	if((config = config_new()) == NULL)
		return NULL;
	if((homedir = getenv("HOME")) == NULL)
		homedir = g_get_home_dir();
	if((filename = string_new_append(homedir, "/" MAILER_CONFIG_FILE, NULL))
			!= NULL)
	{
		config_load(config, filename);
		string_delete(filename);
	}
	return config;
}


/* error */
static int _error(char const * message, int ret)
{
	fputs(PROGNAME, stderr);
	perror(message);
	return ret;
}


/* usage */
static int _usage(void)
{
	fprintf(stderr, _("Usage: %s [-s subject] address...\n"), PROGNAME);
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	Config * config;
	Compose * compose;
	char const * subject = NULL;
	int o;

	if(setlocale(LC_ALL, "") == NULL)
		_error("setlocale", 1);
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "s:")) != -1)
		switch(o)
		{
			case 's':
				subject = optarg;
				break;
			default:
				return _usage();
		}
	config = _compose_config();
	if((compose = _compose(config, subject, argc - optind, &argv[optind]))
			!= NULL)
	{
		gtk_main();
		compose_delete(compose);
	}
	if(config != NULL)
		config_delete(config);
	return 0;
}
