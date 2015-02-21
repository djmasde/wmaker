#ifndef _WUTIL_H_
#define _WUTIL_H_

/* Include <X11/Xlib.h> before including this file. */

#include <sys/types.h>

void wfatal(const char *msg, ...);
void wwarning(const char *msg, ...);
void wsyserror(const char *msg, ...);

char *wfindfile(char *paths, char *file);

char *wfindfileinlist(char **path_list, char *file);

char *wexpandpath(char *path);

char *wgethomedir();

void *wmalloc(size_t size);
void *wrealloc(void *ptr, size_t newsize);

void wrelease(void *ptr);
void *wretain(void *ptr);

char *wstrdup(char *str);

#define wmsleep(usec) usleep(usec)

#endif
