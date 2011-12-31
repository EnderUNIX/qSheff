#ifndef _QSHEFF_DJB_H
#define _QSHEFF_DJB_H

extern unsigned int str_cpy();
extern unsigned int str_len();
extern unsigned int str_chr();
extern int str_start(const char *,const char *);
extern /*@null@*/char *env_get(const char *);

extern char **environ;

extern int error_intr;


#endif

