/* $Id$ */
/* Copyright (c) 2024 Pierre Pronchery <khorben@defora.org> */
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



#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "accountconfig.h"


/* AccountConfig */
/* public */
/* functions */
/* accountconfig_new */
AccountConfig * accountconfig_new_copy(AccountConfig const * config)
{
	AccountConfig * ac = NULL;
	AccountConfig * p;
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%p)\n", __func__, (void *)config);
#endif
	if(config == NULL)
		return NULL;
	for(i = 0;; i++)
	{
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() %zu \"%s\"\n", __func__, i,
				config[i].name);
#endif
		if((p = realloc(ac, sizeof(*ac) * (i + 2))) == NULL)
		{
			accountconfig_delete(ac);
			return NULL;
		}
		ac = p;
		ac[i].type = config[i].type;
		ac[i].name = (config[i].name != NULL) ? strdup(config[i].name)
			: NULL;
		ac[i].title = (config[i].title != NULL) ? strdup(config[i].title)
			: NULL;
		ac[i + 1].type = ACT_NONE;
		ac[i + 1].name = NULL;
		ac[i + 1].title = NULL;
		ac[i + 1].value = NULL;
		switch(config[i].type)
		{
			case ACT_BOOLEAN:
			case ACT_SEPARATOR:
			case ACT_UINT16:
				ac[i].value = config[i].value;
				continue;
			case ACT_PASSWORD:
			case ACT_STRING:
			case ACT_FILE:
				if(config[i].value == NULL)
					ac[i].value = NULL;
				else if((ac[i].value = strdup(config[i].value))
						== NULL)
				{
					accountconfig_delete(ac);
					return NULL;
				}
				continue;
			case ACT_NONE:
				ac[i].value = config[i].value;
				break;
		}
		if(config[i].type == ACT_NONE)
			break;
		assert(0);
	}
	return ac;
}


/* accountconfig_delete */
void accountconfig_delete(AccountConfig * config)
{
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%p)\n", __func__, (void *)config);
#endif
	for(i = 0; config[i].type != ACT_NONE; i++)
	{
		free(config[i].name);
		free(config[i].title);
		switch(config[i].type)
		{
			case ACT_BOOLEAN:
			case ACT_NONE:
			case ACT_SEPARATOR:
			case ACT_UINT16:
				/* nothing else to do */
				continue;
			case ACT_FILE:
			case ACT_PASSWORD:
			case ACT_STRING:
				free(config[i].value);
				continue;
		}
		assert(0);
	}
	free(config);
}


/* accessors */
/* accountconfig_set */
int accountconfig_set(AccountConfig * config, ...)
{
	int ret = 0;
	va_list ap;
	char const * name;
	size_t i;
	long l;
	char const * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%p)\n", __func__, (void *)config);
#endif
	va_start(ap, config);
	while((name = va_arg(ap, char const *)) != NULL)
	{
		for(i = 0; config[i].type != ACT_NONE
				&& strcmp(config[i].name, name) != 0; i++);
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() name=\"%s\" (%u)\n", __func__,
				config[i].name, config[i].type);
#endif
		if(config[i].type == ACT_NONE)
		{
			ret = -1;
			break;
		}
		switch(config[i].type)
		{
			case ACT_BOOLEAN:
			case ACT_UINT16:
				l = va_arg(ap, long);
				config[i].value = (void *)l;
				break;
			case ACT_FILE:
			case ACT_PASSWORD:
			case ACT_STRING:
				p = va_arg(ap, char const *);
				free(config[i].value);
				if(p == NULL)
					config[i].value = NULL;
				else if((config[i].value = strdup(p)) == NULL)
				{
					ret = -1;
					break;
				}
				break;
			case ACT_NONE:
			case ACT_SEPARATOR:
			default:
				assert(0);
				break;
		}
	}
	va_end(ap);
	return ret;
}
