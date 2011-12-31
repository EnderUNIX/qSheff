/*
	MISCELLANOUS UTILITIES  LIBRARY
	
	$Id: miscutil.c,v 1.1.1.1 2006/07/21 08:59:27 simsek Exp $

	Copyright (c) 2004, Murat Balaban
	All rights reserved.

	Redistribution and use in source and binary forms, with or without 
	modification, are permitted provided that the following conditions 
	are met:

	Redistributions of source code must retain the above copyright notice, 
	this list of conditions and the following disclaimer. 
	Redistributions in binary form must reproduce the above copyright notice, 
	this list of conditions and the following disclaimer in the documentation 
	and/or other materials provided with the distribution. 
	Neither the name of the Murat Balaban nor the names of its contributors may 
	be used to endorse or promote products derived from this software without 
	specific prior written permission. 
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
	GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
	OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
	DAMAGE.
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <netdb.h>
#include <syslog.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>


#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/utsname.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "miscutil.h"

static int debuglevel = 0;
static char logdir[1024];
static char logfile[1024];
static FILE *logp = NULL;
static int foreground = 0;

#ifdef __REENTRANT
static pthread_mutex_t logmtx = PTHREAD_MUTEX_INITIALIZER;
#endif


#define read_lock(fd) \
        misc_lockreg(fd, F_SETLK, F_RDLCK, 0, SEEK_END, 0)
#define readw_lock(fd) \
        misc_lockreg(fd, F_SETLKW, F_RDLCK, 0, SEEK_END, 0)
#define write_lock(fd) \
        misc_lockreg(fd, F_SETLK, F_WRLCK, 0, SEEK_END, 0)
#define writew_lock(fd) \
        misc_lockreg(fd, F_SETLKW, F_WRLCK, 0, SEEK_END, 0)
#define un_lock(fd) \
        misc_lockreg(fd, F_SETLK, F_UNLCK, 0, SEEK_END, 0)

void 
misc_setlogtype(int t)
{
#ifdef __REENTRANT
	pthread_mutex_lock(&logmtx);
#endif
	if (t == 1)
		foreground = 1;
	else
		foreground = 0;
#ifdef __REENTRANT
	pthread_mutex_unlock(&logmtx);
#endif
}

void 
misc_setlogdir(const char *l)
{
#ifdef __REENTRANT
	pthread_mutex_lock(&logmtx);
#endif
	strncpy(logdir, l, 1023);
#ifdef __REENTRANT
	pthread_mutex_unlock(&logmtx);
#endif
}

void
misc_setlogfile(const char *l)
{
#ifdef __REENTRANT
	pthread_mutex_lock(&logmtx);
#endif
	strncpy(logfile, l, 1023);
#ifdef __REENTRANT
	pthread_mutex_unlock(&logmtx);
#endif
}

void 
misc_setloglevel(int l)
{
#ifdef __REENTRANT
	pthread_mutex_lock(&logmtx);
#endif
	debuglevel = l;
#ifdef __REENTRANT
	pthread_mutex_unlock(&logmtx);
#endif
}

int 
misc_getloglevel()
{
	int ret = 0;
#ifdef __REENTRANT
	pthread_mutex_lock(&logmtx);
#endif
	ret = debuglevel;
#ifdef __REENTRANT
	pthread_mutex_unlock(&logmtx);
#endif
	return ret;
}


int 
misc_openlog()
{
	char logpath[1024];

	snprintf(logpath, 1023, "%s/%s", logdir, logfile);
#ifdef __REENTRANT
	pthread_mutex_lock(&logmtx);
#endif
	if ((logp = fopen(logpath, "a")) == NULL) {
		syslog(LOG_ERR, "cannot open %s for writing: %s", logpath, strerror(errno));
#ifdef __REENTRANT
		pthread_mutex_unlock(&logmtx);
#endif
		return -1;
	}
#ifdef __REENTRANT
	pthread_mutex_unlock(&logmtx);
#endif
	return 0;
}


int 
misc_closelog()
{
	if (logp == NULL)
		return 0;
#ifdef __REENTRANT
	pthread_mutex_lock(&logmtx);
#endif
	if (fclose(logp) != 0) {
		syslog(LOG_ERR, "cannot close logfile: %s\n", strerror(errno));
#ifdef __REENTRANT
		pthread_mutex_unlock(&logmtx);
#endif
		return -errno;
	}
	logp = NULL;
#ifdef __REENTRANT
		pthread_mutex_unlock(&logmtx);
#endif
	return 0;
}

int 
misc_rotatelog()
{
	time_t tv;
	struct tm tm;
	char tbuf[64];
	char movepath[1024];
	char logpath[1024];

	time(&tv);
	localtime_r(&tv, &tm);
	strftime(tbuf, 63, "%Y.%m.%d-%H.%M.%S", &tm);
	snprintf(movepath, 1023, "%s/%s-%s", logdir, logfile, tbuf);
	snprintf(logpath, 1023, "%s/%s", logdir, logfile);

	misc_debug(0, "Switching main server log from %s to %s\n", logpath, movepath);
	if (misc_closelog() < 0)
		return -1;

#ifdef __REENTRANT
	pthread_mutex_lock(&logmtx);
#endif
	if ((rename(logpath, movepath)) < 0) {
		syslog(LOG_ERR, "Cannot rename %s to %s: %s\n", logpath, movepath, strerror(errno));
#ifdef __REENTRANT
		pthread_mutex_unlock(&logmtx);
#endif
		return -1;
	}
#ifdef __REENTRANT
	pthread_mutex_unlock(&logmtx);
#endif
	misc_openlog();
	misc_debug(0, "Logrotate finished successfully\n");
	return 0;
}


char *
misc_trim(char *s, int len)
{
	int i, j = 0;
	char *tmp = (char *)malloc(len);

	len = strlen(s);
	for (i = 0, j = 0; i < (len - 1);  i++) {
		if (s[i] == ' ')
			continue;
		tmp[j++] = s[i];
	}
	tmp[j] = '\0';
	memset(s, 0, len);
	strncpy(s, tmp, len - 1);
	free(tmp);
	return s;
}

char *
misc_trimnewline(char *s, int len)
{
	int i, j = 0;
	char *tmp;
	
	tmp = (char *)malloc(len);
	for (i = 0, j = 0; i < (len - 1); i++) {
		if (s[i] == '\r' || s[i] == '\n')
			continue;
		tmp[j++] = s[i];
	}
	tmp[j] = '\0';
	memset(s, 0, len);
	strncpy(s, tmp, len - 1);
	free(tmp);
	return s;
}

void 
misc_debug(int l, char *fmt, ...)
{
	va_list ap;
	FILE *lp;
	time_t tv;
	struct tm tm;
	char lfmt[4096];
	char tbuf[64];

	if (logp == NULL)
		return;

	if (l > debuglevel)
		return;

#ifdef __REENTRANT
	pthread_mutex_lock(&logmtx);
#endif
	if (foreground == 1)
		lp = stdout;
	else
		lp = logp;

	time(&tv);
	localtime_r(&tv, &tm);
	strftime(tbuf, 63, "%d/%m/%Y %H:%M:%S", &tm);
	snprintf(lfmt, sizeof(lfmt) - 1, "%s: %s", tbuf, fmt);
	va_start(ap, fmt);
	if (vfprintf(lp, lfmt, ap) < 1) {
#ifdef __REENTRANT
		pthread_mutex_unlock(&logmtx);
#endif
		misc_closelog();
		misc_openlog();
#ifdef __REENTRANT
		pthread_mutex_lock(&logmtx);
#endif
	}
	if (fflush(lp) != 0) {
#ifdef __REENTRANT
		pthread_mutex_unlock(&logmtx);
#endif
		misc_closelog();
		misc_openlog();
#ifdef __REENTRANT
		pthread_mutex_lock(&logmtx);
#endif
	}
#ifdef __REENTRANT
	pthread_mutex_unlock(&logmtx);
#endif
	va_end(ap);
}

char * 
misc_getunamestr(char *uname_str, int len)
{
	struct utsname uts;

	if (uname(&uts) < 0)
		strncpy(uname_str, "Undefined host", sizeof(uname_str)-1);
	else
		snprintf(uname_str, len - 1, "%s [%s %s %s %s]",
				uts.nodename, uts.sysname, uts.release, uts.version, uts.machine);
	return uname_str;
}

char * 
misc_getuptimestr(char *uptime_str, int len, time_t firetime)
{
	char fmt[32];
	time_t now;
	time_t diff;

	time(&now);
	diff = now - firetime;
	memset(uptime_str, 0, len);
	if (diff > 86400) {
		snprintf(fmt, 31, "%d days ", (int)(diff / 86400));
		diff -= (diff / 86400) * 86400;
		strncpy(uptime_str, fmt, len - 1);
	}
	if (diff > 3600) {
		snprintf(fmt, 31, "%d saat ", (int)(diff / 3600));
		diff -= (diff / 3600) * 3600;
		strncat(uptime_str, fmt, len - 1);
	}
	if (diff > 60) {
		snprintf(fmt, 31, "%d dakika ", (int)(diff / 60));
		diff -= (diff / 60) * 60;
		strncat(uptime_str, fmt, len - 1);
	}
	if (diff > 0) {
		snprintf(fmt, 31,  "%d saniye", (int)diff);
		strncat(uptime_str, fmt, len - 1);
	}
	return uptime_str;
}



/* Returns formatted current time	*/
int
misc_strftime(char *out, int len, char *fmt)
{
	time_t tv;
	struct tm tm;

    time(&tv);
    localtime_r(&tv, &tm);
    return strftime(out, len, fmt, &tm);
}

/* Returns formatted given time	*/
int
misc_strftimegiven(char *out, int len, char *fmt, time_t tv)
{
	struct tm tm;

   localtime_r(&tv, &tm);
   return strftime(out, len, fmt, &tm);
}

int
misc_lockreg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
        struct flock lock;

        lock.l_type = type;
        lock.l_start = offset;
        lock.l_whence = whence;
        lock.l_len = len;
        return fcntl(fd, cmd, &lock);
}

int
misc_trimnongraph(char *str, int len)
{
        register int i = 0;

        for (i = 0; i < len; ) {
                if (!isgraph(str[i]))
                        memmove(&str[i], &str[i + 1], --len);
                else
                        i++;
        }
        str[len] = '\0';
        return len;
}


char *
misc_inet_ntoa(int ip)
{
	   struct in_addr in;

	      in.s_addr = ip;
	         return inet_ntoa(in);
}

int
misc_inet_addr(char *ip)
{
	   return inet_addr(ip);
}


