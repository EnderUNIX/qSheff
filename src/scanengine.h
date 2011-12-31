/*
 * Copyright (C) 2005 Baris Simsek, <simsek@enderunix.org>
 *                        http://www.enderunix.org/simsek/
 *
 * This program is free software; you can redistribute it and/or modify
 * it only as long as you attach these notices to your code!
 *
 */


#ifndef _QSHEFF_SCANENGINE_H
#define _QSHEFF_SCANENGINE_H

#define ENGINE_VERSION "0.3.2"

typedef struct wordlist wordlist;

struct wordlist {
	char word[64];
	wordlist *next;
};

char attach_list[256][256];

typedef struct rulelist rulelist;
struct rulelist {
	int attr;
	char ruleline[256];
	rulelist *next;
};

rulelist *rule_sp;

int do_regex(char *line, char *pattern);
int body_filter(char *path, char rt);
int load_attachlist();
int attach_filter();
rulelist *load_rulelist();

#endif

