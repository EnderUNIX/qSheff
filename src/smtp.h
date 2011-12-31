#ifndef _QSHEFF_SMTP_H
#define _QSHEFF_SMTP_H

char *get_remote_ip();
int parse_header();

char remoteip[16];
char mailfrom[256];
char mailto[256];
char subject[256];
char rfc821_name[128];
char attachname[128];

int msgsize;
int relayclient;
int lflag;

#endif
