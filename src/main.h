#ifndef _QSHEFF_MAIN_H
#define _QSHEFF_MAIN_H

#define EX_PERMANENT 31 /* Permanent rejection */
#define EX_TEMPORARY 71 /* Temporary rejection */
#define EX_CUSTOM 82 /* Exit with custom reject message */

#define SPAM_TAG "{SPAM}"
#define VIRI_TAG "{VIRUS}"
#define VIRI_CENSORED "WARNING!!! Message body was removed by qSheff, because of virus."

/* Don't use newline (\n). The messages shouldn't be longer than 220 char. */
#define SPAMMSG "Your email was rejected by qSheff because it matched a spam filter"
#define VIRUSMSG "Your email was rejected by qSheff because of virus content"
#define DEFAULTMSG "Your email was rejected by qSheff"

char msgfile[1024];
char tempdir[1024];

#endif

