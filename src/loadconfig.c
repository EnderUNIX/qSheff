/*
 * Ismail Yenigul's config reader/parser
 * This code derived from EnderUNIX isoqlog project.
 *
 * http://www.enderunix.org/isoqlog
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "log.h"
#include "loadconfig.h"

int loadconfig(char *cfgfile)	/* load isoqlog configuration file */
{
	FILE *fd = NULL;
	char buf[256];
	char keyword[KEYSIZE];
	char value[VALSIZE];
	int len = 0;
	char *cp1 = NULL, *cp2 = NULL;
	char *variables[] = { "Invalid",
		"QSHEFFDIR",
		"LOGFILE",
		"RIPMIME",
		"enable_qsheff_sign",
		"debug_level",
		"paronia_level",
		"drop_empty_from",
		"enable_spam_blackhole",
		"enable_virus_blackhole",
		"enable_quarantine",
		"enable_ignore_list",
		"enable_header_filter",
		"enable_body_filter",
		"enable_attach_filter",
		"enable_clamd",
		"enable_custom_prog",
		"CUSTOM_PROG",
		"CUSTOM_RET_MIN",
		"CUSTOM_RET_MAX",
		"CUSTOM_RET_ERR",
		"custom_sign"
	};

	int i, j, key, line, keyword_nums = sizeof(variables)/sizeof(char *);

	if ((fd = fopen(cfgfile, "r")) == NULL) {
		errlog(__FILE__, __LINE__, errno);
		return -1;
	}

	line = 0;

	while ((fgets(buf, sizeof(buf), fd)) != NULL) {
		line++;
		if (buf[0] == '#')
			continue;
		if ((len = strlen(buf)) <= 1)
			continue;
		cp1 = buf;
		cp2 = keyword;
		j = 0;
		while (isspace((int)*cp1) && ((cp1 - buf) < len)) 
			cp1++;
		while(isgraph((int)*cp1) && *cp1 != '=' && (j++ < KEYSIZE - 1) && (cp1 - buf) < len)
			*cp2++ = *cp1++;
		*cp2 = '\0';
		cp2 = value;
		while ((*cp1 != '\0') && (*cp1 !='\n') && (*cp1 !='=') && ((cp1 - buf) < len))
			cp1++;
		cp1++; 
		while (isspace((int)*cp1) && ((cp1 - buf) < len))
			cp1++; 
		if (*cp1 == '"') 
			cp1++;
		j = 0;
		while ((*cp1 != '\0') && (*cp1 !='\n') && (*cp1 !='"') && (j++ < VALSIZE - 1) && ((cp1 - buf) < len))
			*cp2++ = *cp1++;
		*cp2-- = '\0';
		if (keyword[0] =='\0' || value[0] =='\0')
			continue;
		key = 0;
		for (i = 0; i < keyword_nums; i++) {
			if ((strncmp(keyword, variables[i], sizeof(keyword))) == 0) {
				key = i;
				break;
			}
		}

		switch(key) {
		case 0:
			fprintf(stderr, "illegal keyword in config file: %s\n", keyword);
			break;
		case 1:
			strncpy(QSHEFFDIR, value, VALSIZE-1);
			break;
		case 2:
			strncpy(LOGFILE, value, VALSIZE-1);
			break;
		case 3:
			strncpy(RIPMIME, value, VALSIZE-1);
			break;
		case 4:
			enable_qsheff_sign = atoi(value);
			break;
		case 5:
			debug_level = atoi(value);
			break;
	 	case 6:
	 		paronia_level = atoi(value);
	 		break;
		case 7:
			drop_empty_from = atoi(value);
			break;
		case 8:
			enable_spam_blackhole = atoi(value);
			break;
		case 9:
			enable_virus_blackhole = atoi(value);
			break;
	 	case 10:
	 		enable_quarantine = atoi(value);
	 		break;
		case 11:
		 	enable_ignore_list = atoi(value);
		 	break;
		case 12:
			enable_header_filter = atoi(value);
		 	break;
		case 13:
			enable_body_filter = atoi(value);
			break;
		case 14:
			enable_attach_filter = atoi(value);
			break;
		case 15:
			enable_clamd = atoi(value);
			break;
		case 16:
			enable_custom_prog = atoi(value);
			break;
		case 17:
			strncpy(CUSTOM_PROG, value, VALSIZE-1);
			break;
		case 18:
			CUSTOM_RET_MIN = atoi(value);
			break;
		case 19:
			CUSTOM_RET_MAX = atoi(value);
			break;
		case 20:
			CUSTOM_RET_ERR = atoi(value);
			break;
		case 21:
			strncpy(custom_sign, value, VALSIZE-1);
			break;
		}
	}

	fclose(fd);

	return 0;
}

