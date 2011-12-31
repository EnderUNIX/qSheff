#include "djb.h"

int error_intr =
#ifdef EINTR
EINTR;
#else
-1;
#endif

unsigned int str_cpy(s,t)
register char *s;
register char *t;
{
  register int len;

  len = 0;
  for (;;) {
    if (!(*s = *t)) return len; ++s; ++t; ++len;
    if (!(*s = *t)) return len; ++s; ++t; ++len;
    if (!(*s = *t)) return len; ++s; ++t; ++len;
    if (!(*s = *t)) return len; ++s; ++t; ++len;
  }
}

unsigned int str_len(s)
register char *s;
{
  register char *t;

  t = s;
  for (;;) {
    if (!*t) return t - s; ++t;
    if (!*t) return t - s; ++t;
    if (!*t) return t - s; ++t;
    if (!*t) return t - s; ++t;
  }
}

unsigned int str_chr(s,c)
register char *s;
int c;
{
  register char ch;
  register char *t;

  ch = c;
  t = s;
  for (;;) {
    if (!*t) break; if (*t == ch) break; ++t;
    if (!*t) break; if (*t == ch) break; ++t;
    if (!*t) break; if (*t == ch) break; ++t;
    if (!*t) break; if (*t == ch) break; ++t;
  }
  return t - s;
}

int str_start(register const char *s,register const char *t)
{
  register char x;

  for (;;) {
    x = *t++; if (!x) return 1; if (x != *s++) return 0;
    x = *t++; if (!x) return 1; if (x != *s++) return 0;
    x = *t++; if (!x) return 1; if (x != *s++) return 0;
    x = *t++; if (!x) return 1; if (x != *s++) return 0;
  }
}

extern /*@null@*/char *env_get(const char *s)
{
  int i;
  unsigned int len;

  if (!s) return 0;
  len = str_len(s);
  for (i = 0;environ[i];++i)
    if (str_start(environ[i],s) && (environ[i][len] == '='))
      return environ[i] + len + 1;
  return 0;
}



