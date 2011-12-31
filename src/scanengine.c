/*
 * Copyright (C) 2005 Baris Simsek, <simsek@enderunix.org>
 *                        http://www.enderunix.org/simsek/
 *
 * $Id: scanengine.c,v 1.2 2006/12/08 14:32:44 simsek Exp $
 *
 */

#include "qsheff-config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <ctype.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "smtp.h"
#include "loadconfig.h"
#include "scanengine.h"
#include "toolkit.h"
#include "log.h"

extern int errno;

static char *get_ruleword(char *ruleword, char *line)
{
	int i = 0;

	line++;
	while((*line != ')') && (*line != '\0')) ruleword[i++] = *line++;
	ruleword[i] = '\0';

	return ruleword;
}

static char *strip_ruleline(char *ruleline)
{
	char ch;

	while(*ruleline != '\0') {
		ch = *ruleline;
		ruleline++;
		if((ch == ')') && (*ruleline == '(')) break;
	}

	if(strlen(ruleline) < 1) return NULL;

	return ruleline;
}

/* Regular expression support. */
static int match_pattern(regex_t *regexp, char *line)
{
	int start = 0;						/* The offset from the beginning of the line */
	size_t no_sub = regexp->re_nsub + 1;			/* How many matches are there in a line? */
	regmatch_t *result;

	if ((result = (regmatch_t *) malloc(sizeof(regmatch_t) * no_sub)) == 0)
	{
		errlog(__FILE__, __LINE__, errno);
		return -1;
	}
  
	while (regexec(regexp, line+start, no_sub, result, 0) == 0)
	{
		free(result);
		return 1;					/* Found a match. */
	}

	free(result);

	return 0;
}

int do_regex(char *line, char *pattern)
{
	int ret = 0;
	regex_t *regexp; /* Regular expression pointer for the filter. */

	/* Allocate space for the regular expressions. */
	if ((regexp = (regex_t *) malloc(sizeof(regex_t))) == NULL) {
		errlog(__FILE__, __LINE__, errno);
		return -1;
	}
	memset(regexp, 0, sizeof(regex_t));

	if ((ret = regcomp(regexp, pattern, 0)) != 0) /* Compile the regular expression. */
	{
		regfree(regexp);
		free(regexp);
		return -1;
	}

	ret = match_pattern(regexp, line);

	regfree(regexp);
	free(regexp);

	return ret;
}


/* The heart of the code */
int body_filter(char *filename, char rt)
{
	int i, lc;
	FILE *fd;
	int ret, flag;
	char *ruleline;
	rulelist *tmp;
	char *ruleword;
	char msgbody[MAXBODYLINE][1024];

	if((fd = fopen(filename, "r")) == NULL) {
		errlog(__FILE__, __LINE__, errno);
		return -1;
	}

	lc = 0;
	while ((!feof(fd)) && (lc < MAXBODYLINE))
	{
                memset(msgbody[lc], 0, 1024);
		if ((fgets(msgbody[lc], 1024, fd) == NULL) && (!feof(fd)))
		{
			errlog(__FILE__, __LINE__, errno);
			fclose(fd);
                        return -1;
                }
		else
		{
			msgbody[lc][strlen(msgbody[lc])-1] = '\0';
		}
		lc++;
        }

	if ((ruleword = (char *) malloc(256)) == NULL) {
		errlog(__FILE__, __LINE__, errno);
		fclose(fd);
		return -1;
	}

	tmp = rule_sp;
	while(tmp != NULL) {
		if((tmp->attr == rt) || (tmp->attr == 'a'))
		{
			ruleline = tmp->ruleline;
			memset(rule_word, 0x0, sizeof(rule_word));
			strncpy(rule_word, ruleline, sizeof(rule_word)-1);
			memset(ruleword, 0x0, 256);

			while ((ruleword = get_ruleword(ruleword, ruleline)) != NULL) {
				flag = 0;
				for(i=0; i<lc; i++) {
					memset(spam_word, 0, sizeof(spam_word));
					strncpy(spam_word, msgbody[i], sizeof(spam_word)-1);

					ret = do_regex(msgbody[i], ruleword);
					if (ret == 1) {
						flag = 1;
						break;
					}
					else if (ret == -1) {
						free(ruleword);
						fclose(fd);
						return -1;
					}
				}
				if(flag == 0) break;

				if ((ruleline = strip_ruleline(ruleline)) == NULL) {
					/* End of rule line. AND chain is TRUE. */
					break;
				}
			}
			if (flag == 1) {
				/* AND chain is TRUE. All patterns in the chain are matched. */
				free(ruleword);
				fclose(fd);
				return 1;
			}
		}
		tmp = tmp->next;
	}

	free(ruleword);
        fclose(fd);

	return 0;
}

int attach_filter()
{
	int i, j;
	DIR *dirp;
	struct dirent *direntp;
	struct stat statbuf;
	char filename[256];
	char str[256];

	memset(filename, 0, sizeof(filename));

	if((dirp = opendir(".")) == NULL) {
		errlog(__FILE__, __LINE__, errno);
		return -1;
	}

	while((direntp = readdir(dirp)) != NULL) {
		if(strlen(direntp->d_name) > sizeof(filename)-1) return 1;
		strncpy(filename, direntp->d_name, sizeof(filename)-1);

		if((strncmp(filename, "textfile", 8) !=0) && (strncmp(filename, "_headers_", 9) !=0) && (strcmp(filename, ".") !=0) && (strcmp(filename, "..") != 0)) {
			if(lstat(filename, &statbuf) == -1) {
				errlog(__FILE__, __LINE__, errno);
				return -1;
			}
			if(!S_ISDIR(statbuf.st_mode)) {
				i = 0;
				while((i < 256) && (strlen(attach_list[i]) > 0)) {
					memset(str, 0, sizeof(str));
					if(strlen(attach_list[i]) <= strlen(direntp->d_name)) {
						/* Compare tails */
						for(j=0;j<strlen(attach_list[i]);j++)
							str[j] = filename[strlen(filename)-strlen(attach_list[i]+j)];
						str[j] = '\0';
						#ifdef NOSTRCASESTR
						if(strstr(str, attach_list[i])) {
						#else
						if(strcasestr(str, attach_list[i])) {
						#endif
							strncpy(spam_word, filename, sizeof(spam_word)-1);
							strncpy(rule_word, str, sizeof(rule_word)-1);
							return 1;
						}
					}
					i++;
				}
			}
		}
		memset(filename, 0, sizeof(filename));
	}

	closedir(dirp);

	return 0;
}

int load_attachlist()
{
	int i = 0;
	FILE *fd;
	char pattern[128];

	if((fd = fopen(ATTACHFILE, "r")) == NULL) {
		errlog(__FILE__, __LINE__, errno);
		return -1;
	}

	while((i < 128) && (!feof(fd))) {
		memset(pattern, 0, sizeof(pattern));
		if((fscanf(fd, "%127s", pattern) < 0 ) && (!feof(fd))) {
			errlog(__FILE__, __LINE__, errno);
			fclose(fd);
			return -1;
		}

		if(strlen(pattern) > 1) {
			strncpy(attach_list[i], pattern, sizeof(attach_list[i])-1);
			i++;
		}
	}

	fclose(fd);

	return 0;
}


rulelist *load_rulelist()
{
	int i;
	FILE *fd;
	rulelist *ll, *sp;
	char ruleline[256];

	memset(ruleline, 0, sizeof(ruleline));

	if((fd = fopen(RULEFILE, "r")) == NULL) {
		errlog(__FILE__, __LINE__, errno);
		return NULL;
	}

	if((ll = (rulelist *) malloc(sizeof(rulelist))) == NULL) {
		errlog(__FILE__, __LINE__, errno);
		fclose(fd);
		return NULL;
	}

	sp = ll;
	while((fgets(ruleline, sizeof(ruleline)-1, fd) != 0) && (!feof(fd))) {
		if(((ruleline[0] == 'b') || (ruleline[0] == 'h') || (ruleline[0] == 'a')) && (ruleline[1] == ':') && (strlen(ruleline) > 4)) {
			ll->attr = ruleline[0];

			for(i=0;i<strlen(ruleline)-2;i++) ll->ruleline[i] = ruleline[i+2];
			ll->ruleline[i-1] = '\0';
			
			ll->next = (rulelist *) malloc(sizeof(rulelist));
			ll = ll->next;
		}
		memset(ruleline, 0, sizeof(ruleline));
	}
	
	ll = NULL;

	fclose(fd);

	return sp;
}


