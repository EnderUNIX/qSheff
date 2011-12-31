#ifndef _QSHEFF_TOOLKIT_H
#define _QSHEFF_TOOLKIT_H

int gen_queue_id(char *qid, int len);
int rm_dir(const char *dirname);
int rrm_dir(const char *dirname);
int removespaces(char *buf);
int copy(char *src, char *dst);
int backup(char *msgfile, char *bckdir);
char ret_subdir(char *root);

char qid[64];

#endif
