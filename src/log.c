/*
 *  Copyright (C) 2004 Baris Simsek, EnderUNIX SDT@ Tr
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 *  $Id: log.c,v 1.2 2007/06/18 07:13:51 simsek Exp $
 *
 */


#include "qsheff-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>

#include "miscutil.h"
#include "clam.h"
#include "toolkit.h"
#include "main.h"
#include "loadconfig.h"
#include "scanengine.h"
#include "log.h"
#include "smtp.h"
#include "djb.h"

extern int errno;

static int LEVELS[9] = {0, 0, 15, 7, 13, 11, 3, 5, 0};
static char TAGS[9][8] = {"MAIN", "PARSE", "HEADER", "WBLIST", "ATTACH", "SPAM", "VIRUS", "CUSTOM", "QUEUE" };

void errlog(const char *source, const int line, const int errorno)
{
	#ifdef ENABLE_SYSLOG
	openlog("qsheff", LOG_PID, LOG_MAIL);
	if (errorno > 0)
		syslog(LOG_ERR, "%s:%d: %s", source, line, strerror(errorno));
	closelog();
	#endif

	#ifdef _DEBUG_
	printf("  . %s:%d: %s", source, line, strerror(errorno));
	#endif

	snprintf(err_error, sizeof(err_error)-1, "%s:%d: %s", source, line, strerror(errorno));
}

int putlog(const int errcode, const int modno)
{
	int loglevel;
	char logline[1024];
	char tmpline[1024];
	char fromstr[16];
	
	loglevel = 0;
	memset(fromstr, 0, 10);
	memset(logline, 0, 1024);
	memset(tmpline, 0, 1024);

	if(relayclient == 1)
		strncpy(fromstr, "relayfrom", sizeof(fromstr)-1);
	else
		strncpy(fromstr, "recvfrom", sizeof(fromstr)-1);

	snprintf(logline, sizeof(logline)-1, "[qSheff] %s", TAGS[modno]);
	loglevel = LEVELS[modno];
	snprintf(tmpline, sizeof(logline)-1, "%s, queue=%s, %s=%s, from=`%s', to=`%s', subj=`%s', size=%d", logline, qid, fromstr, remoteip, mailfrom, mailto, subject, msgsize);
	strncpy(logline, tmpline, sizeof(logline)-1);

	if((modno == 8) && (errcode != 0))  {
	/* Check the qmail-queue result. */
		snprintf(tmpline, sizeof(tmpline)-1, "%s, error=`%s', exitcode=%d", logline, err_error, errcode);
		strncpy(logline, tmpline, sizeof(logline)-1);
	}
	else if(errcode == EX_PERMANENT) {
	/* If spam or attachment filter */
		if(modno == 4)
			snprintf(tmpline, sizeof(tmpline)-1, "%s, filename=`%s', rule=`%s'", logline, spam_word, rule_word);
		else if(modno == 5)
			snprintf(tmpline, sizeof(tmpline)-1, "%s, spam=`%s', rule=`%s'", logline, spam_word, rule_word);
		else if(modno == 6)
			snprintf(tmpline, sizeof(tmpline)-1, "%s, virus=`%s',", logline, virname);
		else if(modno == 2)
			snprintf(tmpline, sizeof(tmpline)-1, "%s, domain=`%s', action=`ignore'", logline, spam_word);

		strncpy(logline, tmpline, sizeof(logline)-1);
	}
	else if(errcode == EX_TEMPORARY) {
	/* If an error occured. */
		snprintf(tmpline, sizeof(tmpline)-1, "%s, error=`%s'", logline, err_error);
		strncpy(logline, tmpline, sizeof(logline)-1);
	}
	else {
	/* Safe */
		snprintf(tmpline, sizeof(tmpline)-1, "%s,,", logline);
		strncpy(logline, tmpline, sizeof(logline)-1);
	}

	misc_setlogdir("/");
	misc_setlogfile(LOGFILE);
	if (misc_openlog() != 0) return -1;
	misc_setloglevel(debug_level);
	misc_debug(loglevel, "%s\n", logline);
	misc_closelog();

	#ifdef ENABLE_SYSLOG
	openlog("qsheff", LOG_PID, LOG_MAIL);
	syslog(LOG_NOTICE, "%s", logline);
	closelog();
	#endif

	return 0;
}



