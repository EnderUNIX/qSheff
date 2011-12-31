#ifndef _QSHEFF_LOG_H
#define _QSHEFF_LOG_H

void errlog(const char *source, const int line, const int errorno);
int putlog(const int errcode, const int modno);

char err_domain[32];
char err_error[64];

char spam_word[64];
char rule_word[64];

#endif

