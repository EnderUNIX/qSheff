#ifndef LOADCONFIG_H
#define LOADCONFIG_H

#define VALSIZE 256
#define KEYSIZE 32

char QSHEFFDIR[VALSIZE];
char LOGFILE[VALSIZE];
char RIPMIME[VALSIZE];
int enable_qsheff_sign;
int debug_level;
int paronia_level;
int drop_empty_from;
int enable_spam_blackhole;
int enable_virus_blackhole;
int enable_quarantine;
int enable_ignore_list;
int enable_header_filter;
int enable_body_filter;
int enable_attach_filter;
int enable_clamd;
int enable_custom_prog;
char CUSTOM_PROG[VALSIZE];
int CUSTOM_RET_MIN;
int CUSTOM_RET_MAX;
int CUSTOM_RET_ERR;
char custom_sign[VALSIZE];

int loadconfig(char *cfgfile);

#endif

