#ifdef linux

#include <stdio.h>
#include <fcntl.h>

#else

#include <stdio.h>
#include <sys/param.h>
#include <rpcsvc/rstat.h>

#ifdef SVR4

#include <netdb.h>
#include <sys/systeminfo.h>

#ifndef FSCALE
#define FSCALE	(1 << 8)
#endif

char hostname[MAXHOSTNAMELEN];
struct statstime res;

#endif

#endif

void GetLoad(float *,float *,float *);

#ifdef linux
/* linux version */
void GetLoad(float *small,float *medium,float *large)
{ 
  char buffer[100];
  int fd, len;
  int total;
  char *p;

  fd = open("/proc/loadavg", O_RDONLY);
  len = read(fd, buffer, sizeof(buffer)-1);
  close(fd);
  buffer[len] = '\0';
  
  sscanf(buffer,"%f%f%f",small,medium,large);
  /* pas de verif ... */
}

#else
/* SVR4 */
void GetLoad(float *small,float *medium,float *large)
{
   /*
    * originally written by Matthieu Herrb - Mon Oct  5 1992
    * modified for optimization
    */

    if (rstat(hostname, &res) != 0) {
	perror("rstat");
	*small = *medium = *large = 0.0;
	return;
    }

    *small  = (float)res.avenrun[0]/FSCALE;
    *medium = (float)res.avenrun[1]/FSCALE;
    *large  = (float)res.avenrun[2]/FSCALE;
}

#endif
