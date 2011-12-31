#ifndef _QSHEFF_EXEC_H
#define _QSHEFF_EXEC_H

int parse_cmdline(char *cmdline);
int exec_cmd();
void free_args();
int wait_pid(int *wstat, int pid);

char *args[16];

#endif


