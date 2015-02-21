/* Main.c

   This file handles forking on start-up and registers the handler for
   SIGTERM, so that the daemon can remove its pid file.

   */

#include <sys/file.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

#define __USE_XOPEN 1
#include <sys/time.h>
#include <time.h>

#include "RunLoop.h"
#include "proplist.h"

#include "plconf.h"
#include "util.h"

proplist_t defaults;
char *password;
char *filename;
time_t last_change;
proplist_t lastChangeDict;
extern proplist_t notifyDict;
BOOL useMultiple = YES;
char *pidfilename;


void CleanUpMain(void)
{
  if(notifyDict) PLRelease(notifyDict);
  if(lastChangeDict) PLRelease(lastChangeDict);
  
  if(unlink(pidfilename) < 0)
    if(errno != ENOENT)
      {
	fprintf(stderr, "gsdd: Couldn't delete PID file %s:\n", PIDFILE);
	perror("gsdd: unlink");
	fprintf(stderr, "gsdd: Remove by hand.\n");
      }
  if(!UnlockFile(filename))
    if(errno != ENOENT)
      {
	fprintf(stderr, "gsdd: Couldn't delete lock dir %s.lock:\n", filename);
	perror("gsdd: unlink");
	fprintf(stderr, "gsdd: Remove by hand.\n");
      }
  MyFree(__FILE__, __LINE__, filename);
  MyFree(__FILE__, __LINE__, pidfilename);
}  

void BailOut(void)
{
  CleanUpMain();
  CleanUpRunLoop();
  exit(1);
}

void HandleSig(int sig)
{
  fprintf(stderr, "gsdd: Caught signal %d. Giving up.\n", sig);

  BailOut();
}

proplist_t GetDomainNamesFromDir(char *dirname)
{
  DIR *dirp;
  struct dirent *ent;
  proplist_t retval;

  if(!(dirp = opendir(dirname))) return NULL;

  retval = PLMakeArrayFromElements(NULL);
  while((ent = readdir(dirp)))
    {
      proplist_t name;
      
      if(!(strcmp(ent->d_name, ".")) || !(strcmp(ent->d_name, "..")))
	continue;
      name = PLMakeString(ent->d_name);
      PLAppendArrayElement(retval, name);
      PLRelease(name);
    }

  closedir(dirp);
  return retval;
}

int ArrayIndex(proplist_t arr, proplist_t key)
{
  int i;
  for(i=0; i<PLGetNumberOfElements(arr); i++)
    if(PLIsEqual(key, PLGetArrayElement(arr, i)))
      return i;
  return -1;
}


void CheckChange_multiple(void)
{
  proplist_t domainNames, lastChangeNames;
  int i;

  if(!LockFile(filename))
    {
      fprintf(stderr,
	      "gsdd: Error: Couldn't lock %s\n"
	      "gsdd: Giving up.\n", filename);
      BailOut();
    }
	      
  domainNames = GetDomainNamesFromDir(filename);
  if (domainNames == NULL)
    return;

  lastChangeNames = PLGetAllDictionaryKeys(lastChangeDict);
  if (lastChangeNames==NULL)
    return;

  for(i=0; i<PLGetNumberOfElements(lastChangeNames); i++)
    {
      proplist_t el = PLGetArrayElement(lastChangeNames, i);
      int ind;

      if((ind = ArrayIndex(domainNames, el))<0) /* vanished */
	{
	  Notify(el, 0);
	  PLRemoveDictionaryEntry(lastChangeDict, el);
	  PLRemoveDictionaryEntry(notifyDict, el);
	}
      else
	{
	  struct stat buf;
	  proplist_t data;
	  time_t *lastChange;

	  if(!StatDomain(filename, el, &buf))
	    {
	      fprintf(stderr,
		      "gsdd: Warning: Couldn't stat %s, although dirent says it's there\n",
		      PLGetString(el));
	      continue;
	    }

	  data = PLGetDictionaryEntry(lastChangeDict, el);
	  lastChange = (time_t *)PLGetDataBytes(data);
	  if(*lastChange != buf.st_mtime) /* modified */
	    {
	      *lastChange = buf.st_mtime;
	      Notify(el, 0);
	    }

	  PLRemoveArrayElement(domainNames, ind);
	}
    }

  /* any non-handled domain names left? */
  for(i=0; i<PLGetNumberOfElements(domainNames); i++)
    {
      proplist_t el;
      struct stat buf;
      proplist_t data;
      char *actualFilename;

      el = PLGetArrayElement(domainNames, i);
      actualFilename = MyMalloc(__FILE__, __LINE__, strlen(filename) +
			      strlen(PLGetString(el)) + 2);
      sprintf(actualFilename, "%s/%s", filename, PLGetString(el));
      if(stat(actualFilename, &buf)<0)
	{
	  fprintf(stderr,
		  "gsdd: Warning: Couldn't stat %s, although dirent says it's there\n",
		  actualFilename);
	  MyFree(__FILE__, __LINE__, actualFilename);
	  continue;
	}
      MyFree(__FILE__, __LINE__, actualFilename);

      data = PLMakeData((void *)&(buf.st_mtime), sizeof(time_t));
      PLInsertDictionaryEntry(lastChangeDict, el, data);
      PLRelease(data);
      Notify(el, 0);
    }

  PLRelease(domainNames);
  PLRelease(lastChangeNames);
  if(!UnlockFile(filename))
    {
      fprintf(stderr,
	      "gsdd: Error: Couldn't unlock file %s\n"
	      "gsdd: Giving up\n", filename);
      BailOut();
    }
}

void CheckChange_single(void)
{
  struct stat dummy;
  proplist_t allkeys, arr, el, tmp, filename_pl;
  int i, j;
  pid_t current;

  if(stat(filename, &dummy)<0)
    {
      fprintf(stderr, "gsdd: Warning: Defaults file %s is gone!\n", filename);
      if(defaults)
	PLRelease(defaults);
      defaults = PLGetProplistWithPath(filename);
      if(!defaults)
	{
	  tmp = PLMakeDictionaryFromEntries(NULL, NULL);
	  filename_pl = PLMakeString(filename);
	  PLSetFilename(tmp, filename_pl);
	  PLRelease(filename_pl);
	  
	  if(!PLSave(tmp, YES))
	    {
	      fprintf(stderr, "gsdd: Could not create file ~/GNUstep/Defaults.\n");
	      fprintf(stderr, "gsdd: Giving up.\n");
	      BailOut();
	    }
	  PLRelease(tmp);
	  defaults =
	    PLGetProplistWithPath(NULL);
	}

      stat(filename, &dummy);
    }

  if(dummy.st_mtime != last_change)
    {
      last_change = dummy.st_mtime;
      if(defaults)
	PLRelease(defaults);
      defaults = PLGetProplistWithPath(filename);
      if(notifyDict)
	{
	  allkeys = PLGetAllDictionaryKeys(notifyDict);
	  for(i=0; i<PLGetNumberOfElements(allkeys); i++)
	    {
	      arr = PLGetDictionaryEntry(notifyDict,
					 PLGetArrayElement(allkeys, i));
	      for(j=0; j<PLGetNumberOfElements(arr); j++)
		{
		  el = PLGetArrayElement(arr, j);
		  current = *(pid_t *)PLGetDataBytes(el);
		  if(kill(current, SIGNAL)<0)
		    if(errno == ESRCH) /* Process is gone. */
		      {
			PLRemoveArrayElement(arr, i);
			i--;
		      }
		}
	    }
	  PLRelease(allkeys);
	}
    }
}

void CheckChange(void)
{
  if(!useMultiple)
    CheckChange_single();
  else
    CheckChange_multiple();
}

int main(int argc, char **argv)
{
  pid_t pid;
  FILE *pid_file;
  struct stat dummy;
  char buf[255];
  char salt[255];
  struct timeval tv;
  struct timezone tz;
  proplist_t tmp, filename_pl;
  int sock;
  int port;
  int i;
 
  pidfilename = ManglePath(PIDFILE);

  lastChangeDict = PLMakeDictionaryFromEntries(NULL, NULL);
  
  /* If we have a pid file, assume that a daemon is already active */
  if(stat(pidfilename, &dummy) == 0)
    {
      pid_file = fopen(pidfilename, "r");
      if(!pid_file)
	{
	  fprintf(stderr, "gsdd: Internal error: Can stat pid file, but can't read it\n");
	  exit(1);
	}
      fscanf(pid_file, "%d %d %s", &pid, &port, buf);
      fprintf(stderr, "gsdd: Another daemon is active (pid %d). Giving up.\n",
	      pid); 
      exit(1);
    }

  filename = MakeDefaultsFilename();
  if(stat(filename, &dummy) < 0) /* can't stat; not there. */
    {
      if(mkdir(filename, 600) < 0)
	{
	  fprintf(stderr,
		  "gsdd: Couldn't create defaults directory %s\n"
		  "gsdd: Giving up.\n", filename);
	  exit(1);
	}
      useMultiple = YES;
    }
  else
    { /* Is it a directory? */
      if(dummy.st_mode & S_IFDIR) useMultiple = YES;
      else useMultiple = NO;
    }
  

  if(!useMultiple)
    {
      defaults = PLGetProplistWithPath(filename);
      if(!defaults)
	{
	  /* Create empty dictionary. */
	  tmp = PLMakeDictionaryFromEntries(NULL, NULL);
	  filename_pl = PLMakeString(filename);
	  PLSetFilename(tmp, filename_pl);
	  PLRelease(filename_pl);
	  if(!PLSave(tmp, YES))
	    {
	      fprintf(stderr,
		      "gsdd: Could not create file %s\n"
		      "gsdd: Giving up.\n", filename);
	      exit(1);
	    }
	}
      defaults = PLGetProplistWithPath(NULL);
      if(stat(filename, &dummy) == 0)
	{
	  last_change = dummy.st_mtime;
	}
    }
  
  sock = GetServerSocket(MINPORT, MAXPORT, &port);
  if(sock<0)
    {
      fprintf(stderr, "gsdd: Could not open socket. Giving up.\n");
      perror("gsdd");
      exit(1);
    }

  signal(SIGINT, HandleSig);
  signal(SIGHUP, HandleSig);
  signal(SIGTERM, HandleSig);
  signal(SIGQUIT, HandleSig);
  signal(SIGSEGV, HandleSig);
  signal(SIGBUS, HandleSig);
  signal(SIGFPE, HandleSig);

  /* construct password */
  gettimeofday(&tv, &tz);
  sprintf(buf, "%d%d", tv.tv_usec, tv.tv_sec);
  srand((unsigned int)tv.tv_usec);

  for(i=0;i<2;i++)
    {
      do salt[i] = (char)rand();
      while((salt[i]<'0') || (salt[i]>'9' && salt[i]<'A') ||
	    (salt[i]>'Z' && salt[i]<'a') || (salt[i]>'z'));
    }
  
  salt[2]='\0';
  password = &((crypt(buf, salt))[2]); /* skip the salt */
   
  umask(0077);
  
  pid_file = fopen(pidfilename, "w+");
  
  if(!pid_file)
    {
      fprintf(stderr, "gsdd: Could not open pid file %s:\n", PIDFILE);
      perror("gsdd: fopen");
      fprintf(stderr, "gsdd: Giving up.\n");
      kill(pid, SIGTERM);
      exit(1);
    }
  
  fprintf(pid_file, "%d %d %s", getpid(), port, password);
  fflush(pid_file);
  fclose(pid_file);

  RunLoop(sock);
  exit(0);

}
