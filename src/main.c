/*
 * Copyright (C) 2004-2006  Baris Simsek @ EnderUNIX SDT @ Tr
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
 * $Id: main.c,v 1.10 2007/04/16 16:06:10 simsek Exp $
 *
 */


#include "qsheff-config.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#ifdef ENABLE_CLAMAV
#include <clamav.h>
#include "clam.h"
#endif

#include "main.h"
#include "loadconfig.h"
#include "log.h"
#include "smtp.h"
#include "toolkit.h"

#ifdef ENABLE_CUSTOM_PROG
#include "exec.h"
#endif

#include "scanengine.h"
#include "djb.h"

#define SIGNATURE_SIZE 76

extern int errno;

static int fd;

#ifdef QMAILQUEUE
static char *binqqargs[2] = { "bin/qmail-queue", 0 };
#else
static char *binqqargs[2] = { "bin/qmail-queue.orig", 0 };
#endif

/*------------------------------- Functions -------------------------------*/

#ifdef _DEBUG_
static void print_greeting()
{
        printf("\n");
	fprintf(stdout, "qSheff v%s - $Id: main.c,v 1.10 2007/04/16 16:06:10 simsek Exp $\n", VERSION);
	fprintf(stdout, "(C) Copyright 2004-2006 Baris Simsek, http://www.enderunix.org/simsek/\n\n");
	fprintf(stdout, "Ready for input stream.\n\n");

}
#endif


static void disable_filters()
{
	enable_header_filter = 0;
	enable_body_filter = 0;
	enable_attach_filter = 0;
}


static int run_queue()
{
	pid_t qpid;
	int pstat;

	#ifdef _DEBUG_
	printf("- qmail-queue\n");
	#endif

	if((fd = open(msgfile, O_RDONLY)) == -1) {
		errlog(__FILE__, __LINE__, errno);
		return -11;
	}

	if (chdir(QMAILDIR) == -1) { errlog(__FILE__, __LINE__, errno); return -11; }

	switch(qpid = vfork()) {
		case -1:
			close(fd);
			errlog(__FILE__, __LINE__, errno);
			return -11;
		case 0:
			close(0);
			dup2(fd,0);
			if (execv(*binqqargs, binqqargs) == -1)
				errlog(__FILE__, __LINE__, errno);
			_exit(-11);
	}

        /* wait for queue to get exit status. */
        if ( waitpid(qpid, &pstat, 0) == -1 ) {
		errlog(__FILE__, __LINE__, errno);
		close(fd);
		return -11;
	}

	close(fd);
	
        /* check if the child died on a signal */
        if ( WIFSIGNALED(pstat) ) { 
		syslog(LOG_NOTICE, "qSheff got signal."); return -11; }

        /* qmail-queue exit status */
        if ( WIFEXITED(pstat) ) return(WEXITSTATUS(pstat));

        /* check if the child stopped. */
        if ( WIFSTOPPED(pstat) ) { 
		syslog(LOG_NOTICE, "qSheff stopped."); return -11; }

        /* should never come here */
        return -11;
}


static int run_ripmime()
{
        int pid;
        int pstat;

        /* fork ripmime */
        switch(pid = vfork()) {
                case -1:
			errlog(__FILE__, __LINE__, errno);
                        return -11;
			break;
                case 0:
                        /* Child process. */
                        close(1);
                        close(2);
                        if(execl(RIPMIME, RIPMIME, "-e", "-i", msgfile, "-d", tempdir, NULL ) == -1) {
				errlog(__FILE__, __LINE__, errno);
				_exit(-11);
			}
			exit(-11); /* Does it possible? */
        }

        /* wait for ripmime to get return status. */
        if (waitpid(pid, &pstat, 0) == -1) {
		errlog(__FILE__, __LINE__, errno);
                return -11;
        }

        /* check if the child died on a signal */
        if (WIFSIGNALED(pstat)) return -11;

        /* check if the child stopped. */
        if (WIFSTOPPED(pstat)) return -11;

        /* if it exited normal, return the status */
        if (WIFEXITED(pstat)) {
                return(WEXITSTATUS(pstat));
        }

        /* should never come here */
        return -11;
}


static int search_ignored()
{
	int ret;
	FILE *fd;
	char line[256];

	if ((fd = fopen(IGNLISTFILE, "r")) == NULL) {
		errlog(__FILE__, __LINE__, errno);
		return -1;
	}

	while(!feof(fd)) {
		memset(line, 0, sizeof(line));
	
		if (fgets(line, sizeof(line), fd) == NULL) {
			if (feof(fd)) {
				fclose(fd);
				return 0;
			}
			else {
				fclose(fd);
				errlog(__FILE__, __LINE__, errno);
				return -1;
			}
		}

		if ((line[0] == '#') || (line[0] == ' ') || (line[2] == '\0')) continue;

		line[strlen(line)-1] = '\0';

		str_cpy(spam_word, line, sizeof(spam_word)-1);
	
		if ((ret = do_regex(mailfrom, line)) == 1) {fclose(fd);return 1;}
		if ((ret = do_regex(remoteip, line)) == 1) {fclose(fd);return 1;}
	}

	fclose(fd);

	return 0;
}


static int free_rules()
{
	rulelist *tp1, *tp2;

	if(rule_sp != NULL) {
		tp1 = rule_sp->next;
		tp2 = tp1;
		while(tp1->ruleline[0] != '\0') {
			tp2 = tp2->next;
			free(tp1);
			tp1 = tp2;
		}
		free(rule_sp);
	}

	return 0;
}

#if defined (SPAM_TAGGING) || (VIRUS_TAGGING)
static int alter_subj(const char *path, int modno)
{
	FILE *from_fd, *to_fd;
	static char *bp;
	int blen = 1024;
	int mtx = 0;
	char to_path[1024];
	char new[256];

	if ((from_fd = fopen(path, "r")) == NULL) return -1;
	memset(to_path, 0x0, sizeof(to_path));
	snprintf(to_path, sizeof(to_path)-1, "%s.tmp", path);
	if ((to_fd = fopen(to_path, "w")) == NULL) return -1;

	if ((bp = malloc(blen)) == NULL) return -1;
	memset(bp, 0, blen);

	while (fgets(bp, (size_t)blen, from_fd) != NULL) {
		if ((mtx == 0) && (strncasecmp(bp, "Subject:", 8) == 0)) {
			mtx = 1;
			if (modno == 5) {
				if (strstr(subject, SPAM_TAG) == NULL) {
					fputs("X-qSheff-Match: spam\n", to_fd);
					snprintf(new, 255, "Subject: %s %s\n", SPAM_TAG, subject);
				}
				else
					strncpy(new, bp, 255);
			}
			else {
				if (strstr(subject, VIRI_TAG) == NULL) {
					fputs("X-qSheff-Match: virus\n", to_fd);
					snprintf(new, 255, "Subject: %s %s\n", VIRI_TAG, subject);
				}
				else
					strncpy(new, bp, 255);
			}
			if (fputs(new, to_fd) == EOF) {
				fclose(from_fd); fclose(to_fd);
				unlink(to_path);
				free(bp);
				return -1;
			}
			break;
		}
		else {
			if (fputs(bp, to_fd) == EOF) {
				fclose(from_fd); fclose(to_fd);
				unlink(to_path);
				free(bp);
				return -1;
			}
		}
		memset(bp, 0x0, 1024);
	}

	#if defined (ENABLE_CLAMAV) && (VIRUS_TAGGING)
	if(modno == 6) {
		while (fgets(bp, (size_t)blen, from_fd) != NULL) {
			if ((*bp != '\n') && (strncmp(bp, "Content-Type:", 13)) && (strstr(bp, "boundary") == NULL)) {
				if (fputs(bp, to_fd) == EOF) {
					fclose(from_fd); fclose(to_fd);
					unlink(to_path);
					free(bp);
					return -1;
				}
			}
			else break;
			memset(bp, 0, 1024);
		}
		fputs("\n", to_fd);
		fputs(VIRI_CENSORED, to_fd);
		fputs("\n\n", to_fd);
		fputs("Virus name: ", to_fd); fputs(virname, to_fd);
		fputs("\n\n", to_fd);
		fputs(custom_sign, to_fd);
		fputs("\n", to_fd);
	}
	#endif

	#ifdef SPAM_TAGGING
	if(modno == 5)
	while (fgets(bp, (size_t)blen, from_fd) != NULL) {
		if (fputs(bp, to_fd) == EOF) {
			fclose(from_fd); fclose(to_fd);
			unlink(to_path);
			free(bp);
			return -1;
		}
		memset(bp, 0, 1024);
	}
	#endif

	fclose(from_fd); fclose(to_fd);
	free(bp);

	/* Remove original file */
	unlink(path);

	/* Rename new file */
	if(rename(to_path, path) == -1) return -1;

	return 0;
}

static int subject_tag(int modno)
{
	int ret = -1;

	if(alter_subj(msgfile, modno) == 0) ret = run_queue();

	#ifdef _DEBUG_
	printf("qmail-queue: %d\n\n", ret);
	#endif

	return ret;
}
#endif


#ifdef CUSTOM_ERROR
static int custom_msg_exit(int module)
{
	char reject_message[254];

	memset(reject_message, 0, sizeof(reject_message));
	/* Custom message can be maximum 254 bytes. It shouldn't include newline (\n) */
	spam_word[32] = '\0';
	virname[32] = '\0';
	if(module == 5) {
		snprintf(reject_message, sizeof(reject_message),
		"D%s (%s) / %s", SPAMMSG, spam_word, custom_sign);
	}
	else if(module == 6) {
		snprintf(reject_message, sizeof(reject_message),
		"D%s (%s) / %s", VIRUSMSG, virname, custom_sign);
	}
	else {
		snprintf(reject_message, sizeof(reject_message),
		"D%s / %s", DEFAULTMSG, custom_sign);
	}

	/* Write custom message to qmail-smtpd fd. */
	write(4, reject_message, strlen(reject_message));

	_exit(EX_CUSTOM);

	return -1;
}
#endif


static int clean_exit(int status, int modno)
{
	int ret;

	ret = free_rules();
	rrm_dir(tempdir);

	putlog(status, modno);

	#ifdef BACKUP_ALL
	backup(msgfile, "backup");
	#endif

	#if defined (SPAM_TAGGING)
	if ((modno == 5) && (status == EX_PERMANENT)) {
		ret = subject_tag(5);
		closelog();
		unlink(msgfile);

		_exit(ret);
	}
	#endif

	#if defined (ENABLE_CLAMAV) && (VIRUS_TAGGING)
	if ((modno == 6) && (status == EX_PERMANENT)) {
		ret = subject_tag(6);
		closelog();
		unlink(msgfile);

		_exit(ret);
	}
	#endif

	closelog();

	if(status == EX_PERMANENT) {
		if ((enable_quarantine == 1) && (modno == 5)) {
			backup(msgfile, "quarantine");
		}

		/* If SPAM check enable_spam_blackhole */
		if (((modno == 5) && (enable_spam_blackhole == 0)) || ((modno == 6) && (enable_virus_blackhole == 0))) {
			unlink(msgfile);
			#ifdef CUSTOM_ERROR
			/* This works only if the qmail-queue custom error patch is installed. */
			custom_msg_exit(modno);
			#else
			_exit(EX_PERMANENT);
			#endif
		}
		else {
			status = 0;
		}
	}

	unlink(msgfile);

	_exit(status);


	return 0;
}


int main(int argc, char *argv[])
{
	int ret;
	DIR *dirp;
	struct stat sb;
	struct dirent *direntp;
	char *remoteipaddr;
	char buf[128];
	char buffer[1024];

	if(argc > 1) {
		fprintf(stdout, "qSheff v%s - $Id: main.c,v 1.10 2007/04/16 16:06:10 simsek Exp $\n", VERSION);
		fprintf(stdout, "(C) Copyright 2004-2006 Baris Simsek, http://www.enderunix.org/simsek/\n");
		_exit(-1);
	}

	msgsize = 0;
	relayclient = 0;

	relayclient = 0;
	lflag = 0;

	if((remoteipaddr = get_remote_ip()) == NULL) {
		lflag = 1;       /* local flag.   */
		str_cpy(remoteip, "127.0.0.1");
	}
	else {
		str_cpy(remoteip, remoteipaddr);
	}

	if (env_get("RELAYCLIENT")) {
		relayclient = 1; /* relay client. */
		lflag = 1;       /* local flag.   */
	}

	#ifndef ENABLE_LOCAL_USERS
	if (lflag == 1) {
		#ifdef _DEBUG_
		printf("  . local msg\n");
		#endif
		if (chdir(QMAILDIR) == -1) { errlog(__FILE__, __LINE__, errno);_exit(EX_TEMPORARY); }
		execv(*binqqargs, binqqargs);
	}
	#endif

	/* Load qsheff.conf. */
	if (loadconfig(CFGFILE) == -1) {
		closelog();
		_exit(EX_TEMPORARY);
	}

	umask(0022);
	gen_queue_id(qid, sizeof(qid));

	memset(msgfile, 0, sizeof(msgfile));
	snprintf(msgfile, sizeof(msgfile)-1, "%s/spool/%s.msg", QSHEFFDIR, qid);

	if((fd = open(msgfile, O_WRONLY|O_CREAT|O_TRUNC,0644)) == -1) {
		errlog(__FILE__, __LINE__, errno);
		closelog();
		_exit(EX_TEMPORARY);
	}

	/* Write the qSheff signature. */
	if (enable_qsheff_sign == 1) {
		snprintf(buf, sizeof(buf)-1, "X-Mail-Scanner: Scanned by qSheff-II-%s (http://www.enderunix.org/qsheff/)\n", VERSION);
		write(fd, buf, strlen(buf));
	}

	#ifdef _DEBUG_
	print_greeting();
	#endif

	memset(buffer, 0, sizeof(buffer));
	for(;;) {
		ret = read(STDIN_FILENO, buffer, sizeof(buffer)-1);

		if (ret == -1) if (errno == error_intr) continue;

		if (ret == -1) {
			errlog(__FILE__, __LINE__, errno);
			unlink(msgfile);
			closelog();
			_exit(EX_TEMPORARY);
		}

		if (ret == 0) break;

		if(write(fd, buffer, ret) < ret) {
			errlog(__FILE__, __LINE__, errno);
			close(fd);
			closelog();
			unlink(msgfile);
			_exit(EX_TEMPORARY);
		}
		memset(buffer, 0, sizeof(buffer));
	}

	close(fd);

	#ifdef _DEBUG_
	printf("\n- End of File\n\n");
	#endif

	/* Get message size. */
	if(stat(msgfile, &sb) == 0)
		msgsize = sb.st_size - SIGNATURE_SIZE;
	else
		errlog(__FILE__, __LINE__, errno);

	snprintf(tempdir, sizeof(tempdir)-1, "%s/tmp/%s", QSHEFFDIR, qid);

	if(mkdir(tempdir, 0755) == -1) {
		errlog(__FILE__, __LINE__, errno);
		unlink(msgfile);
		closelog();
		_exit(EX_TEMPORARY);
	}

/**********************************
 ** Module 1: Parse the message. **
 **********************************/

	memset(err_error, 0, sizeof(err_error));

	#ifdef _DEBUG_
	printf("- ripmime\n");
	#endif

	ret = run_ripmime();

	#ifdef _DEBUG_
	printf("  . ret: %d\n", ret);
	#endif

	if(ret != 0) {
		clean_exit(EX_TEMPORARY, 1);
	}

	/* Go to temp directory is including part files. */
	if(chdir(tempdir) != 0) {
		errlog(__FILE__, __LINE__, errno);
		clean_exit(EX_TEMPORARY, 1);
	}

	#ifdef _DEBUG_
	printf("- parse\n");
	#endif

	if((ret = parse_header() == -1)) {
		clean_exit(EX_TEMPORARY, 1);
	}

	#ifdef _DEBUG_
	printf("  . ret: %d\n", ret);
	#endif


/**********************************
 ** Module 2: Check header.      **
 **********************************/

	#ifdef _DEBUG_
	printf("- header policy\n");
	#endif

	#ifdef ENABLE_LOCAL_USERS
	/* Filter local users other than MAILERDAEMON. */
	if ( (lflag == 1) && (strncmp(mailfrom, MAILERDAEMON, sizeof(mailfrom)) == 0)) {
		#ifdef _DEBUG_
		printf("  . local msg from %s to %s\n", mailfrom, mailto);
		#endif

		ret = run_queue();

		#ifdef _DEBUG_
		printf("  . ret: %d\n", ret);
		#endif

		clean_exit(ret, 8);

	}
	#endif

	if ((drop_empty_from == 1) && (mailfrom[0] == '\0')) {
		#ifdef _DEBUG_
		printf("  . empty from line\n");
		#endif
		clean_exit(0, 2);
	}

	if (enable_ignore_list == 1) {
		#ifdef _DEBUG_
		printf("- ignore list\n");
		#endif
		ret = search_ignored();
		if (ret == 1) {
			#ifdef _DEBUG_
			printf("  . ignored address\n");
			#endif

			disable_filters();

			#ifdef _DEBUG_
			printf("  . ret: %d\n", ret);
			#endif
		}
		else if (ret == -1) {
			clean_exit(EX_TEMPORARY, 2);
		}
	}

	#ifdef _DEBUG_
	printf("  . ret: 0\n");
	#endif



/**********************************
 ** Module 4: Attachment filter. **
 **********************************/

	memset(err_error, 0, sizeof(err_error));

	if(enable_attach_filter == 1) {
		#ifdef _DEBUG_
		printf("- attach\n");
		#endif

		if(load_attachlist() == -1) {
			#ifdef _DEBUG_
			printf("  . ret: -1\n");
			#endif
			clean_exit(EX_TEMPORARY, 4);
		}
	
		ret = attach_filter();

		#ifdef _DEBUG_
		printf("  . ret: %d\n", ret);
		#endif

		if(ret > 0) {
			/* if we found a denied file as attached. */
			clean_exit(EX_PERMANENT, 4);
		}
		else if(ret == -1) {
			clean_exit(EX_TEMPORARY, 4);
		}
	}


/**********************************
 ** Module 6: ClamAv Client.     **
 **********************************/

	#ifdef ENABLE_CLAMAV
	memset(err_error, 0, sizeof(err_error));

	if(enable_clamd == 1) {
		#ifdef _DEBUG_
		printf("- clamd\n");
		#endif

		ret = cl_scandir(tempdir);

		#ifdef _DEBUG_
		printf("  . ret: %d\n", ret);
		#endif

		if(ret == CL_VIRUS) {
			clean_exit(EX_PERMANENT, 6);
		}
		if(ret == CL_ERROR) {
			clean_exit(EX_TEMPORARY, 6);
		}

	}
	#endif


/**********************************
 ** Module 5: Spam filters.      **
 **********************************/

	#ifdef _DEBUG_
	printf("- load rulelist\n");
	#endif

	if((enable_header_filter == 1) || (enable_body_filter == 1))
		if((rule_sp = load_rulelist()) == NULL) {
			clean_exit(EX_TEMPORARY, 5);
		}

	#ifdef _DEBUG_
	printf("  . ret: 0\n");
	#endif

	if(enable_header_filter == 1) {
		#ifdef _DEBUG_
		printf("- header\n");
		#endif

		ret = body_filter("_headers_", 'h');

		#ifdef _DEBUG_
		printf("  . ret: %d\n", ret);
		#endif

		if(ret > 0) { /* if found spam */
			clean_exit(EX_PERMANENT, 5);
		}
		if(ret == -1) {
			 clean_exit(EX_TEMPORARY, 5);
		}
	}

	if(enable_body_filter == 1) {
		#ifdef _DEBUG_
		printf("- body\n");
		#endif

		if((dirp = opendir(tempdir)) == NULL) {
			errlog(__FILE__, __LINE__, errno);
			clean_exit(EX_TEMPORARY, 5);
		}
		ret = 0;
		while((direntp = readdir(dirp)) != NULL) {
			if(((strncmp(direntp->d_name, "textfile", 8) == 0)) || (strncmp(direntp->d_name, attachname, sizeof(direntp->d_name)) == 0)) {
				ret = body_filter(direntp->d_name, 'b');

				#ifdef _DEBUG_
				printf("  . ret for %s: %d\n", direntp->d_name, ret);
				#endif

				if(ret > 0) { /* if found spam */
					closedir(dirp);
					clean_exit(EX_PERMANENT, 5);
				}
			}
		}

		closedir(dirp);

		if(ret == -1) {
			 clean_exit(EX_TEMPORARY, 5);
		}
	}


/**********************************
 ** Module 7: Custom prog.       **
 **********************************/

	#ifdef ENABLE_CUSTOM_PROG
	if (enable_custom_prog == 1) {
		#ifdef _DEBUG_
		printf("- custom program\n");
		#endif

		parse_cmdline(CUSTOM_PROG);	

		ret = exec_cmd();

		#ifdef _DEBUG_
		printf("  . ret: %d\n", ret);
		#endif

		free_args();

		if ((ret >= CUSTOM_RET_MIN) && (ret <= CUSTOM_RET_MAX)) {
			clean_exit(EX_PERMANENT, 7);
		}

		if ((ret < 0) || (ret == CUSTOM_RET_ERR)) {
			clean_exit(EX_TEMPORARY, 7);
		}
	}
	#endif


/**********************************
 ** Module 9: qmail-queue.       **
 **********************************/

	memset(err_error, 0, sizeof(err_error));

	ret = run_queue();

	#ifdef _DEBUG_
	printf("  . ret: %d\n", ret);
	#endif

	clean_exit(ret, 8);

	return 0;
}


