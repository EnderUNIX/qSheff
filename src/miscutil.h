#ifndef MISCUTIL_H
#define MISCUTIL_H

/*
	MISCELLANOUS UTILITIES  LIBRARY
	
	$Id: miscutil.h,v 1.1.1.1 2006/07/21 08:59:27 simsek Exp $

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


#include <time.h>

#define	MISC_LOGFG	1
#define	MISC_LOGBG	0

#define  O_LOGHEX    0x00000001
#define  O_LOGCHAR   0x00000002


void misc_setlogtype(int t);	 
void misc_setlogdir(const char *);
void misc_setlogfile(const char *);
void misc_setloglevel(int);
int misc_getlogleve();
int misc_openlog();
int misc_closelog();
void misc_debug(int, char *, ...);
void misc_devmsglog(char *a, int flags, unsigned char *buf, int len);
void misc_devlogx(char *a, int flags, unsigned char *buf, int len, char direction);
void misc_devlog(char *a, char *fmt, ...);
int misc_rotatelog();
char * misc_getunamestr(char *, int);
char * misc_getuptimestr(char *, int, time_t);

char *misc_inet_ntoa(int);
int misc_inet_addr(char *);
char *misc_trim(char *, int);
char *misc_trimnewline(char *, int);
int misc_hexstr2raw(char *str, char *out, int len);
int misc_hexchar2int(char *str);
int misc_substr(char *out, char *in, int offset, int len);
int misc_strftime(char *out, int len, char *fmt);
int misc_strftimegiven(char *out, int len, char *fmt, time_t tv);
int misc_strstr(char *out, int outlen, char *in, int inlen, char sep, int sepix);
double misc_getamount(char *stramount, int currencycode);
int misc_trimnongraph(char *str, int len);
int misc_getdayofmonth(time_t *tv);
int misc_getmonth(time_t *tv);
int misc_getyear(time_t *tv);


#endif

