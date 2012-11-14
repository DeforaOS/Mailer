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



#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
		if(_date_do(date, "%a, %d %b %Y %T %z", tm) == 0
				|| _date_do(date, "%d %b %Y %T %z", tm) == 0
				|| _date_do(date, "%d/%m/%Y %T %z", tm) == 0
				|| _date_do(date, "%d/%m/%Y %T", tm) == 0
				|| _date_do(date, "%FT%TZ", tm) == 0)
			return mktime(tm);
	/* XXX fallback to the current time and date */
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
