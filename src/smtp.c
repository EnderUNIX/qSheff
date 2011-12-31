/*
 * Copyright (C) 2004 EnderUNIX Software Development Team @ Turkey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: smtp.c,v 1.1.1.1 2006/07/21 08:59:27 simsek Exp $
 *
 */


#include "qsheff-config.h"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "smtp.h"
#include "log.h"

static char ok_env_chars[] = "abcdefghijklmnopqrstuvwxyz" \
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
                            "1234567890_-.@";

/* From vpopmail.c,v 1.28 2004/01/13 15:59:42       */
/* Copyright (C) Inter7 Internet Technologies, Inc. */
char *get_remote_ip()
{
	char *ipenv;
	static char ipbuf[30];
	char *ipaddr;
	char *p;

	ipenv = getenv("TCPREMOTEIP");
	if ((ipenv == NULL) || (strlen(ipenv) > sizeof(ipbuf))) return ipenv;

	strcpy (ipbuf, ipenv);
	ipaddr = ipbuf;

	/* Convert ::ffff:127.0.0.1 format to 127.0.0.1
	 * While avoiding buffer overflow.
	 */
	if (*ipaddr == ':') {
		ipaddr++;
		if (*ipaddr != '\0') ipaddr++;
		while((*ipaddr != ':') && (*ipaddr != '\0')) ipaddr++;
		if (*ipaddr != '\0') ipaddr++;
	}

	/* remove invalid characters */
	for (p = ipaddr; *(p += strspn(p, ok_env_chars)); ) {*p='_';}

	return ipaddr;
}

int parse_header()
{
	FILE *fd;
	char line[1024];
	char *str;
	char *tmp;

	if((fd = fopen("_headers_", "r")) == NULL) {
		errlog(__FILE__, __LINE__, errno);
		return -1;
	}

	memset(mailfrom, 0, sizeof(mailfrom));
	memset(mailto, 0, sizeof(mailto));
	memset(subject, 0, sizeof(subject));
	memset(rfc821_name, 0, sizeof(rfc821_name));

	while(fgets(line, sizeof(line), fd)) {
		if (line[0] == '\n') break;
		line[strlen(line)-1] = '\0';

		if((strncasecmp(line, "From:", 5) == 0) && (mailfrom[0] == '\0')) {
			if((str = strchr(line, ':')) == NULL) continue;
			str++;
			while(*str == ' ') str++;
			if(*str == '\0') continue;
			if((tmp = strchr(str, '<')) != NULL) {
				tmp++;
				str = tmp;
				while((*tmp != '>') && (*tmp != '\0')) tmp++;
				*tmp = '\0';
			}
			strncpy(mailfrom, str, sizeof(mailfrom)-1);
		}
		else if((strncasecmp(line, "To:", 3) == 0) && (mailto[0] == '\0')) {
			if((str = strchr(line, ':')) == NULL) continue;
			str++;
			while(*str == ' ') str++;
			if(*str == '\0') continue;
			if((tmp = strchr(str, '<')) != NULL) {
				tmp++;
				str = tmp;
				while((*tmp != '>') && (*tmp != '\0')) tmp++;
				*tmp = '\0';
			}
			strncpy(mailto, str, sizeof(mailto)-1);
		}
		else if((strncmp(line, "Subject:", 8) == 0) && (subject[0] == '\0')) {
			if((str = strchr(line, ':')) == NULL) continue;
			str++;
			while(*str == ' ') str++;
			if(*str == '\0') continue;
			strncpy(subject, str, sizeof(subject)-1);
		}
		else if(strncmp(line, "Content-Type: text/html;", 24) == 0) {
			if((str = strstr(line, "name=")) != NULL) {
				str = str + 5;
				strncpy(rfc821_name, str, sizeof(rfc821_name)-1);
				while((*str != '\0') && (*str != ';') && (*str != ' ') && (*str != '\t') && (*str != '\n'))
					str++;
				*str = '\0';
			}
		}
	}

	fclose(fd);

#ifdef _DEBUG_
	printf("  . Remote IP: '%s'\n", remoteip);
	printf("  . From: '%s'\n", mailfrom);
	printf("  . To: '%s'\n", mailto);
	printf("  . Subject: '%s'\n", subject);
	printf("  . Name: '%s'\n", rfc821_name);
#endif

	return 0;
}



