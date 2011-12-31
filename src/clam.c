/*
 *  clamav.c - ClamAv virus scanning interface.
 *
 *  $Id: clam.c,v 1.3 2007/06/29 19:23:37 simsek Exp $
 *
 */

#include "qsheff-config.h"

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "toolkit.h"
#include "log.h"
#include "clam.h"

static int sd = -1;

int cl_connect_local(const char *path)
{
	struct sockaddr_un laddr;

	if ((sd = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1) {
		errlog(__FILE__, __LINE__, errno);
		return -1;
	}

	memset(&laddr, 0, sizeof(laddr));
	laddr.sun_family = AF_LOCAL;
	strncpy(laddr.sun_path, path, sizeof(laddr.sun_path) - 1);

	if (connect(sd, (SA *) &laddr, sizeof(laddr)) == -1) {
		errlog(__FILE__, __LINE__, errno);
		return -1;
	}

	return sd;
}

int cl_copyvirinfo(char *dst, const char *src, int maxlen)
{
	register int j = 0, i = 0;

	for (i = 0; i < maxlen && isgraph(src[i]); i++)
		dst[j++] = src[i];
	return j;
}

int cl_scandir(const char *path)
{
	int ret = 0;
	char rbuf[1024];
	char sbuf[1024];
	int rlen = 0, slen = 0;
	int eofflg = 0;
	int vfflg = 0;
	char eofstr[1024];
	char *tok = NULL;

	memset(virname, 0, sizeof(virname));
	
	if ((ret = cl_connect_local(CLAMD_SOCKET)) == -1) {
		return 2;
	}

	snprintf(sbuf, sizeof(sbuf) - 1, "SCAN %s\r\n", path);
	if ((slen = send(sd, sbuf, strlen(sbuf), 0)) < strlen(sbuf)) {
		errlog(__FILE__, __LINE__, errno);
		cl_disconnect();
		return 2;
	}
	snprintf(eofstr, sizeof(eofstr) - 1, "%s:", path);
	while (!eofflg) {
		memset(rbuf, 0, sizeof(rbuf));
		if ((rlen = recv(sd, rbuf, sizeof(rbuf)-1, 0)) < 1) {
			cl_disconnect();
			break;
		}
		if ((tok = strstr(rbuf, "FOUND")) != NULL) {
			if ((tok = memchr(rbuf, ':', strlen(rbuf))) == NULL) {
				cl_disconnect();
				snprintf(virname, sizeof(virname)-1, "UNKNOWN");
			}
			cl_copyvirinfo(virname, (tok + 2), sizeof(virname));
			vfflg = 1;
			eofflg = 1;
		} else
		if ((tok = strstr(rbuf, eofstr)) != NULL) {
			if (memcmp(tok + (strlen(eofstr) + 1), "OK", 2) == 0) {
				snprintf(virname, sizeof(virname)-1, "CLEAN");
			} else {
				cl_copyvirinfo(virname, (tok + 2), sizeof(virname));
			}
			eofflg = 1;
		}
	}
	if (!eofflg) {
		cl_disconnect();
		return 2;
	}
	cl_disconnect();

	return vfflg;
}

int cl_disconnect()
{
	close(sd);
	sd = -1;

	return 0;
}



