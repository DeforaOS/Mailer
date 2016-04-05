/* $Id$ */
/* Copyright (c) 2012-2013 Pierre Pronchery <khorben@defora.org> */
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



#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../include/Mailer/helper.h"


/* public */
/* functions */
/* mailer_helper_get_date */
static int _date_do(char const * date, char const * format, struct tm * tm);

time_t mailer_helper_get_date(char const * date, struct tm * tm)
{
	time_t t;

	if(date != NULL)
		/* FIXME check the standard(s) again */
		if(_date_do(date, "%a, %d %b %Y %T %z (%z)", tm) == 0
				|| _date_do(date, "%a, %d %b %Y %T %z", tm) == 0
				|| _date_do(date, "%d %b %Y %T %z", tm) == 0
				|| _date_do(date, "%d/%m/%Y %T %z", tm) == 0
				|| _date_do(date, "%d/%m/%Y %T", tm) == 0
				|| _date_do(date, "%FT%TZ", tm) == 0)
		{
			/* fix the date if missing the century */
			if(tm->tm_year >= -1830 && tm->tm_year < -1800)
				tm->tm_year += 1900;
			else if(tm->tm_year >= -1900 && tm->tm_year < -1830)
				tm->tm_year += 2000;
			return mktime(tm);
		}
	/* XXX fallback to the current time and date */
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, date);
#endif
	t = time(NULL);
	gmtime_r(&t, tm);
	return t;
}

static int _date_do(char const * date, char const * format, struct tm * tm)
{
	char const * p;

	memset(tm, 0, sizeof(*tm));
	if((p = strptime(date, format, tm)) != NULL && *p == '\0')
		return 0;
	/* check if we obtained enough information */
	if(p != NULL && tm->tm_year != 0 && tm->tm_mday != 0)
		/* XXX _apparently_ yes */
		return 0;
	return -1;
}


/* mailer_helper_get_email */
char * mailer_helper_get_email(char const * header)
{
	char * ret;
	size_t len;
	char * buf = NULL;

	if(header == NULL)
		return NULL;
	len = strlen(header);
	if((ret = malloc(len + 1)) == NULL || (buf = malloc(len + 1)) == NULL)
	{
		free(buf);
		free(ret);
		return NULL;
	}
	if(mailer_helper_is_email(header))
	{
		strcpy(ret, header);
		free(buf);
		return ret;
	}
	if(sscanf(header, "%[^(](%[^)])", ret, buf) == 2
			|| sscanf(header, "<%[^>]>", ret) == 1
			|| sscanf(header, "%[^<]<%[^>]>", buf, ret) == 2)
	{
		for(len = strlen(ret); len > 0
				&& isblank((unsigned char)ret[len - 1]); len--)
			ret[len - 1] = '\0';
		if(mailer_helper_is_email(ret))
		{
			free(buf);
			return ret;
		}
	}
	free(buf);
	free(ret);
	return NULL;
}


/* mailer_helper_get_name */
char * mailer_helper_get_name(char const * header)
{
	char * ret;
	size_t len;
	char * buf = NULL;
	int c;

	if(header == NULL)
		return NULL;
	len = strlen(header);
	if((ret = malloc(len + 1)) == NULL || (buf = malloc(len + 1)) == NULL)
	{
		free(buf);
		free(ret);
		return NULL;
	}
	if(mailer_helper_is_email(header))
	{
		strcpy(ret, header);
		free(buf);
		return ret;
	}
	if(sscanf(header, "%[^(](%[^)])", buf, ret) != 2
			&& sscanf(header, "<%[^>]>", ret) != 1
			&& sscanf(header, "%[^<]<%[^>]>", ret, buf) != 2)
	{
		free(buf);
		free(ret);
		return NULL;
	}
	free(buf);
	/* right-trim spaces */
	for(len = strlen(ret); --len > 0
			&& isspace((unsigned char)ret[len]); ret[len] = '\0');
	/* remove surrounding quotes */
	if((len = strlen(ret)) >= 2 && ((c = ret[0]) == '"' || c == '\'')
			&& ret[len - 1] == c)
	{
		memmove(ret, &ret[1], len - 2);
		ret[len - 2] = '\0';
	}
	return ret;
}


/* mailer_helper_is_email */
int mailer_helper_is_email(char const * header)
{
	size_t i = 0;
	int c;

	/* FIXME this is neither strict nor standard at the moment */
	for(i = 0; (c = (unsigned char)header[i]) != '@'; i++)
		if(c == '\0')
			return 0;
		else if(!isalnum(c) && c != '.' && c != '_')
			return 0;
	for(i++; (c = (unsigned char)header[i]) != '\0'; i++)
		if(!isalnum(c) && c != '.' && c != '_')
			return 0;
	return 1;
}
