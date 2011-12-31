/*
 *  $Id: clam.h,v 1.2 2006/11/18 20:26:33 simsek Exp $
 */

#ifndef CLAMAV_H__
#define CLAMAV_H__

#define CL_OK 0
#define CL_VIRUS 1
#define CL_ERROR 2

#define  SA struct sockaddr

#ifndef SUN_LEN
  # define    SUN_LEN (su) \ (sizeof (*(su)) - sizeof ((su)->sun_path) + strlen((su)->sun_path))
#endif

/* For older OSes */
#ifndef AF_LOCAL
  #define AF_LOCAL AF_UNIX
#endif

char virname[64];

enum avconntypes {
        AVCONN_UNIX = 0,
        AVCONN_TCP = 1,
        AVCONN_UDP,
        AVCONN_BINEXEC
};

int cl_connect(const char *addr, short port);
int cl_copyvirinfo(char *dst, const char *src, int maxlen);
int cl_scandir(const char *path);
int cl_disconnect(void);


#endif /* CLAMAV_H__ */


