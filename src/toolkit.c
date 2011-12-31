/*
 *  Copyright (C) 2004 Baris Simsek, EnderUNIX SDT @ Tr
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 *  $Id: toolkit.c,v 1.1.1.1 2006/07/21 08:59:27 simsek Exp $
 *
 */


#include "qsheff-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>

#include "log.h"
#include "djb.h"
#include "toolkit.h"
#include "loadconfig.h"

extern int errno;

int gen_queue_id(char *qid, int len)
{
	struct timeval time_str;

	gettimeofday(&time_str,(struct timezone *) 0);
	snprintf(qid, len-1, "q%ld-%ld-%ld", time_str.tv_sec, time_str.tv_usec, (long int)getpid());

	return 0;
}

int rm_dir(const char *dirname)
{
	if(chdir(QSHEFFDIR) != 0) {
		errlog(__FILE__, __LINE__, errno);
		return -1;
	}

	if(rmdir(dirname) != 0) {
		errlog(__FILE__, __LINE__, errno);
		return -1;
	}
	else
		return 0;
}

int removespaces(char *buf)
{
	char *cp = buf;
	char *sv = buf;
	int len;

	len = strlen(buf);

	for (; *buf != '\0' && *buf != '\r' && *buf != '\n' && ((buf - sv) < len); buf++)
		if (*buf == ' ')
			continue;
		else
			*cp++ = *buf;
	*cp = 0x0;
	return cp - sv;
}

int rrm_dir(const char *dirpath)
{
	char newpath[1024];
	DIR *dirp;
	struct dirent *direntp;
	struct stat sb;

	#ifdef _DEBUG_
	printf("  . removing path %s\n", dirpath);
	#endif

	if(chdir(QSHEFFDIR) != 0) errlog(__FILE__, __LINE__, errno);

	if ((dirp = opendir(dirpath)) == NULL) {
		errlog(__FILE__, __LINE__, errno);
		return 2;
	}

	while((direntp = readdir(dirp)) != NULL) {
		if(strcmp(direntp->d_name, ".") !=0 && strcmp(direntp->d_name, "..") != 0) {
			snprintf(newpath, sizeof(newpath)-1, "%s/%s", dirpath, direntp->d_name);
			if(lstat(newpath, &sb) == -1) {
				errlog(__FILE__, __LINE__, errno);
				closedir(dirp);
				return 2;
			}
			if(S_ISDIR(sb.st_mode)) {
				rrm_dir(newpath);
			}
			else {
				if(unlink(newpath) == -1) {
					errlog(__FILE__, __LINE__, errno);
				}
				else {
					#ifdef _DEBUG_
					printf("  . file %s is removed\n", newpath);
					#endif
				}
			}
		}
	}

	closedir(dirp);

	if(rmdir(dirpath) == -1) errlog(__FILE__, __LINE__, errno);

	return 0;
}

int copy(char *src, char *dst)
{
	static int blen = 1024;
	static char *bp;
	register int nread, from_fd, to_fd;

	if ((from_fd = open(src, O_RDONLY, 0)) < 0) return -1;

	if ((to_fd = open(dst, O_CREAT | O_EXCL | O_TRUNC | O_WRONLY, 0400)) < 0) return -1;

	if((bp = malloc(blen)) == NULL) return -1;
	memset(bp, 0, 1024);

	for(;;) {
		nread = read(from_fd, bp, (size_t)blen);

		if (nread == -1) if (errno == error_intr) continue;

		if (nread == -1) {
			close(from_fd); close(to_fd);
			free(bp);
			errlog(__FILE__, __LINE__, errno);
			return -1;
		}

		if (nread == 0) break;

		if (write(to_fd, bp, (size_t)nread) < nread) {
			close(from_fd); close(to_fd);
			free(bp);
			errlog(__FILE__, __LINE__, errno);
			return -1;
		}
		memset(bp, 0, 1024);
	}

	free(bp);

	close(from_fd); close(to_fd);

	/* Check the last read. */
	if (nread < 0) return -1;

	return 0;
}


char ret_subdir(char *root)
{
	int i, j;
	struct stat sb;
	char HEX_TBL[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	char *path;

	i = 0;
	if ((path = (char *) malloc(1024)) == NULL) {
		errlog(__FILE__, __LINE__, errno);
		return 1;
	}
	memset(path, 0x0, 1024);

	while (i < 16)
	{
		strncpy(path, root, 1000);
		j = strlen(path);
		path[j] = '/';
		path[j+1] = HEX_TBL[i];
		if (stat(path, &sb) == -1) {
			free(path);
			return 1;
		}
		/* Allow maximum MAX_FILES files in a sub dir. */
		if (sb.st_nlink < MAX_FILES) break;
		path[j] = '\0';
		path[j+1] = '\0';
		i++;
	}

	if (i == 16) {
		free(path);
		return 'x'; /* Queue is full. */
	}

	free(path);

	return HEX_TBL[i];
}


int backup(char *msgfile, char *bckdir)
{
	char subdir;
	char *dst;

	if ((dst = (char *) malloc(1024)) != NULL ) {
		snprintf(dst, 1023, "%s/%s", QSHEFFDIR, bckdir);
		subdir = ret_subdir(dst);
		if(subdir > 2) {
			memset(dst, 0, 1024);
			snprintf(dst, 1023, "%s/%s/%c/%s", QSHEFFDIR, bckdir, subdir, qid);
			if (mkdir(dst, 0700) == -1) {
				free(dst);
				return -1;
			}
			snprintf(dst, 1023, "%s/%s/%c/%s/mesg", QSHEFFDIR, bckdir, subdir, qid);
			copy(msgfile, dst);
		}
		free(dst);
	}
	else return -1;
	
	return 0;
}

