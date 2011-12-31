/*
 * Copyright (C) 2004-2006 Baris Simsek, EnderUNIX SDT @ Tr
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
 * $Id: exec.c,v 1.1.1.1 2006/07/21 08:59:27 simsek Exp $
 *
 */


#include "qsheff-config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "exec.h"
#include "log.h"
#include "main.h"
#include "smtp.h"
#include "djb.h"

/* FIX ME! This should be defined by autoconf. */
#define HASWAITPID 1

int parse_cmdline(char *cmdline)
{
	int i, j;
	char *s;

	s = cmdline;

	i = 0;
	for(;;) {
		if ((args[i] = (char *) malloc(256)) == NULL) {
			errlog(__FILE__, __LINE__, errno);
			return -1;
		}
		memset(args[i], 0, 256);

		j = 0;
		while(*s == ' ') s++;
		while((*s != '\0') && (*s != ' ')) args[i][j++] = *s++;
		args[i][j] = '\0';
		i++;
		if (*s == '\0') break;
	}

	for(j=0;j<i;j++) {
		if (strcmp(args[j], "%%mailfrom%%") == 0) {
			str_cpy(args[j], mailfrom);
		}
		else if (strcmp(args[j], "%%mailto%%") == 0) {
			str_cpy(args[j], mailto);
		}
		else if (strcmp(args[j], "%%remoteip%%") == 0) {
			str_cpy(args[j], remoteip);
		}
		else if (strcmp(args[j], "%%msgfile%%") == 0) {
			str_cpy(args[j], msgfile);
		}
		else if (strcmp(args[j], "%%tempdir%%") == 0) {
			str_cpy(args[j], tempdir);
		}
	}

	#ifdef _DEBUG_
	for(j=0;j<i;j++) printf("  . args[%d]: %s\n", j, args[j]);
	#endif

	return 0;
}


int exec_cmd()
{
	pid_t pid;
	int pstat;

	switch((pid = vfork())) {
		case -1:
			errlog(__FILE__, __LINE__, errno);
			return -11;
			break;
		case 0:
			if(execve(args[0], args, NULL) == -1) {
				errlog(__FILE__, __LINE__, errno);
				exit(-11);
			}
			break;
		default:
			if (wait(&pstat) == -1) {
				errlog(__FILE__, __LINE__, errno);
				return -11;
			}

			if (WIFEXITED(pstat))
				return WEXITSTATUS(pstat);
			break;
	}
	return -11; /* Normally, program never come here */
}

void free_args()
{
	int i=0;

	while(args[i] != NULL) free(args[i++]);

	return;
}

#ifdef HASWAITPID

int wait_pid(wstat,pid) int *wstat; int pid;
{
  int r;

  do
    r = waitpid(pid,wstat,0);
  while ((r == -1) && (errno == error_intr));
  return r;
}

#else

/* XXX untested */
/* XXX breaks down with more than two children */
static int oldpid = 0;
static int oldwstat; /* defined if(oldpid) */

int wait_pid(wstat,pid) int *wstat; int pid;
{
  int r;

  if (pid == oldpid) { *wstat = oldwstat; oldpid = 0; return pid; }

  do {
    r = wait(wstat);
    if ((r != pid) && (r != -1)) { oldwstat = *wstat; oldpid = r; continue; }
  }
  while ((r == -1) && (errno == error_intr));
  return r;
}

#endif



