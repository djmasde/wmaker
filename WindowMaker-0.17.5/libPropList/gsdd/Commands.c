/* Commands.c:

   Implements the commands usable over the connection.

   */

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "plconf.h"
#include "proplist.h"
#include "util.h"

#include "Main.h"

extern proplist_t defaults;
proplist_t notifyDict = NULL;
extern BOOL useMultiple;
extern char *filename;

#define HELPSTR "Known commands:\n" \
"\tlist\n" \
"\t\tReturns an array of domain names\n" \
"\tauth KEY\n"\
"\t\tAuthenticates you as the correct user\n" \
"\tget DOMAIN\n" \
"\t\tReturns a description of DOMAIN, or the empty string\n" \
"\tset DOMAIN DESCRIPTION     set-nonotify PID DOMAIN DESCRIPTION\n" \
"\t\tSets DOMAIN to DESCRIPTION. In the case of set-nonotify, does\n" \
"\t\tnot send a signal to process PID.\n" \
"\tremove DOMAIN     remove-nonotify PID DOMAIN\n" \
"\t\tRemoves DOMAIN. In the case of remove-nonotify, does not send\n" \
"\t\ta signal to process PID.\n" \
"\tregister PID [DOMAIN]\n" \
"\t\tRegisters process PID to be signalled when the defaults are\n" \
"\t\tchanged (if DOMAIN is supplied, a signal will be sent only\n" \
"\t\tif DOMAIN is changed\n" \
"\tunregister PID [DOMAIN]\n" \
"\t\tUnregisters process PID; i.e. signals will no longer be sent\n" \
"\t\twhen DOMAIN is changed. If DOMAIN is not supplied, no signals\n" \
"\t\twill be sent on any change.\n" \
"\thelp\n" \
"\t\tPrints this information.\n"

#define UNKNOWNSTR "Unknown command. Use \"help\" for list of known commands.\n"

#define ERRORSTR "Syntax error. Use \"help\" for a list of commands.\n"

#define NOPERMSTR "Use \"auth KEY\" to authentify yourself first.\n"

#define AUTHERRSTR "Incorrect password. Authentification not granted.\n"

extern proplist_t sock_key;
extern proplist_t auth_key;

extern char *password;

/*
 * Some utility functions
 */

proplist_t ScanProplist(char *str, int *end)
{
  int bracecount = 0;
  int parencount = 0;
  int quotecount = 0;
  int index = 0;
  int startindex;
  int len;
  char *tmp, *descr;
  char c;
  proplist_t ret;

  if(!str) return (proplist_t)NULL;
  
  len = strlen(str);

  tmp = (char *)MyMalloc(__FILE__, __LINE__, len+2);
  sprintf(tmp, "%s ", str); /* make sure there's a space at the end */

  do {
    if(index>=len)
      {
	MyFree(__FILE__, __LINE__, tmp);
	return (proplist_t)NULL;
      }
    c = tmp[index++];
  } while((c==' ') || (c=='\t') || (c=='\r') || (c=='\n'));

  startindex = index-1;
  
  switch(c)
    {
    case '\"':
      quotecount = 1;
      break;
    case '{':
      bracecount++;
      break;
    case '(':
      parencount++;
      break;
    default: /* Plain string */
      descr = (char *)strtok(&(tmp[startindex]), " \t\n\r");
      if(!descr)
	{
	  MyFree(__FILE__, __LINE__, tmp);
	  return (proplist_t)NULL;
	}
      *end = startindex+strlen(descr)-1;
      ret = PLGetProplistWithDescription(descr);
      MyFree(__FILE__, __LINE__, tmp);
      return ret;
    }
  while(quotecount || parencount || bracecount)
    {
      if(index>=len)
	{
	  MyFree(__FILE__, __LINE__, tmp);
	  return (proplist_t)NULL;
	}

      c = tmp[index++];

      switch(c)
	{
	case '\"':
	  if(tmp[index-2]!='\\')
	    {
	      if(quotecount) quotecount = 0;
	      else quotecount = 1;
	    }
	  break;
	case '{':
	  if(!quotecount && (tmp[index-2]!='\\'))
	    bracecount++;
	  break;
	case '}':
	  if(!quotecount && (tmp[index-2]!='\\'))
	    bracecount--;
	  break;
	case '(':
	  if(!quotecount && (tmp[index-2]!='\\'))
	    parencount++;
	  break;
	case ')':
	  if(!quotecount && (tmp[index-2]!='\\'))
	    parencount--;
	  break;
	}
      
    }

  *end = index-1;
  descr = (char *)MyMalloc(__FILE__, __LINE__, index-startindex+1);
  strncpy(descr,  &(tmp[startindex]), index-startindex);
  descr[index-startindex] = '\0'; /* Heisenbug correction */

  ret = PLGetProplistWithDescription(descr);
  MyFree(__FILE__, __LINE__, descr);
  MyFree(__FILE__, __LINE__, tmp);
  return ret;
}

BOOL Notify(proplist_t key, pid_t skip_pid)
{
  proplist_t arr, el;
  pid_t current;
  int i;

  if(notifyDict)
    {
      if(!key) return NO;

      arr = PLGetDictionaryEntry(notifyDict, key);

      if(arr)
	for(i=0; i<PLGetNumberOfElements(arr); i++)
	  {
	    el = PLGetArrayElement(arr, i);
	    current = *(pid_t *)PLGetDataBytes(el);

	    if(current != skip_pid)
	      if(kill(current, SIGNAL)<0)
		if(errno == ESRCH) /* Process is gone. */
		  {
		    PLRemoveArrayElement(arr, i);
		    i--;
		  }
	  }
    }
  return YES;
}

BOOL SetDomain(proplist_t key, proplist_t value, pid_t skip_pid)
{
  BOOL ret;

  if(!useMultiple)
    {
      if(PLGetDictionaryEntry(defaults, key))
	PLRemoveDictionaryEntry(defaults, key);
      
      PLInsertDictionaryEntry(defaults, key, value);
      if(!PLSynchronize(defaults))
	{
	  fprintf(stderr, "gsdd: Couldn't synchronize. Lock file present?\n");
	  BailOut();
	}
    }
  else
    {
      char *actualFilename;
      proplist_t filenamePL;
      struct stat buf;
      
      if(!LockFile(filename)) return NO;
      if(StatDomain(filename, key, &buf))
	{
	  if(!DeleteDomain(filename, key))
	    {
	      UnlockFile(filename);
	      return NO;
	    }
	}

      actualFilename = MyMalloc(__FILE__, __LINE__, 
			      strlen(filename) +
			      strlen(PLGetString(key)) + 2);
      sprintf(actualFilename, "%s/%s", filename, PLGetString(key));
      filenamePL = PLMakeString(actualFilename);
      PLSetFilename(value, filenamePL);
      PLRelease(filenamePL);
      MyFree(__FILE__, __LINE__, actualFilename);
      if(!PLSave(value, YES))
	{
	  fprintf(stderr, "gsdd: Couldn't synchronize %s. Lock file present?\n",
		  PLGetString(key));
	  BailOut();
	}
      UnlockFile(filename);
    }
  
  ret = Notify(key, skip_pid);
  PLRelease(key); PLRelease(value);
  return ret;
}

BOOL RemoveDomain(proplist_t key, pid_t skip_pid)
{
  BOOL ret;

  if(!useMultiple)
    {
      if(PLGetDictionaryEntry(defaults, key))
	{
	  PLRemoveDictionaryEntry(defaults, key);
	  if(!PLSynchronize(defaults))
	    {
	      fprintf(stderr, "gsdd: Couldn't synchronize. Lock file present?\n");
	      BailOut();
	    }
	  ret = Notify(key, skip_pid);
	}
      else
	ret = NO;
    }
  else
    {
      struct stat buf;
      
      if(!LockFile(filename)) return NO;
      if(StatDomain(filename, key, &buf))
	{
	  if(!DeleteDomain(filename, key))
	    {
	      UnlockFile(filename);
	      return NO;
	    }
	}
      UnlockFile(filename);
      ret = Notify(key, skip_pid);
    }      
  return ret;
}
	    
	    
BOOL HelpCmd(proplist_t pl, int index)
{
  int sock;
  proplist_t el;

  el = PLGetArrayElement(pl, index);
  sock =
    *(int *)PLGetDataBytes(PLGetDictionaryEntry(el, sock_key));

  return WriteString(sock, HELPSTR);
}

BOOL ListCmd(proplist_t pl, int index)
{
  proplist_t arr;
  char *descr, *str;
  int sock;
  BOOL res;
  proplist_t el;
  BOOL auth;

  el = PLGetArrayElement(pl, index);

  auth = *(BOOL *)PLGetDataBytes(PLGetDictionaryEntry(el, auth_key));
  sock = *(int *)PLGetDataBytes(PLGetDictionaryEntry(el, sock_key));

  if(!auth)
    return WriteString(sock, NOPERMSTR);

  if(!useMultiple)
    {
      if(!defaults)
	arr = PLMakeArrayFromElements(NULL);
      else
	arr = PLGetAllDictionaryKeys(defaults);
    }
  else
    arr = GetDomainNamesFromDir(filename);

  descr = PLGetDescription(arr);

  str = (char *)MyMalloc(__FILE__, __LINE__, strlen(descr)+2);
  sprintf(str, "%s\n", descr);
  
  res = WriteString(sock, str);

  MyFree(__FILE__, __LINE__, descr);
  MyFree(__FILE__, __LINE__, str);
  PLRelease(arr);
  
  return res;
}

BOOL ErrorCmd(proplist_t pl, int index)
{
  int sock;
  proplist_t el;

  el = PLGetArrayElement(pl, index);
  
  sock = *(int *)PLGetDataBytes(PLGetDictionaryEntry(el, sock_key));

  return WriteString(sock, ERRORSTR);
}

BOOL GetCmd(proplist_t pl, int index)
{
  int sock;
  char *desc, *str;
  proplist_t key, value;
  int end;
  BOOL res;
  proplist_t el;
  BOOL auth;

  el = PLGetArrayElement(pl, index);
  
  auth = *(BOOL *)PLGetDataBytes(PLGetDictionaryEntry(el, auth_key));
  sock = *(int *)PLGetDataBytes(PLGetDictionaryEntry(el, sock_key));

  if(!auth)
    return WriteString(sock, NOPERMSTR);
  
  desc = (char *)strtok(NULL, "\n\r");

  key = ScanProplist(desc, &end);
  if(!key)
    return ErrorCmd(pl, index);

  if(!useMultiple)
    {
      if(!defaults)
	return WriteString(sock, "nil\n");
      
      value = PLGetDictionaryEntry(defaults, key);
      PLRetain(value);
    }
  else
    value = ReadDomain(filename, key);
      
  PLRelease(key);

  if(!value)
    return WriteString(sock, "nil\n");

  desc = PLGetDescription(value);
  PLRelease(value);
  
  str = (char *)MyMalloc(__FILE__, __LINE__, strlen(desc)+2);
  sprintf(str, "%s\n", desc);
  
  res = WriteString(sock, str);

  MyFree(__FILE__, __LINE__, desc);
  MyFree(__FILE__, __LINE__, str);
  return res;
}

BOOL SetCmd(proplist_t pl, int index, BOOL skip)
{
  int sock;
  char *desc;
  char *tmp;
  proplist_t key, value;
  int end;
  pid_t skip_pid;

  BOOL auth;
  proplist_t el;

  el = PLGetArrayElement(pl, index);
  auth = *(BOOL *)PLGetDataBytes(PLGetDictionaryEntry(el, auth_key));
  sock = *(int *)PLGetDataBytes(PLGetDictionaryEntry(el, sock_key));

  if(!auth)
    return WriteString(sock, NOPERMSTR);

  if(skip)
    {
      tmp = (char *)strtok(NULL, " \t\n\r");
      if(!tmp) return ErrorCmd(pl, index);
      skip_pid = (pid_t)atoi(tmp);
      if(!skip_pid) return ErrorCmd(pl, index);
    }
  else skip_pid = 0;
  
  desc = (char *)strtok(NULL, "\n\r");
  if(!desc)
    return ErrorCmd(pl, index);
  
  key = ScanProplist(desc, &end);
  if(!key)
    return ErrorCmd(pl, index);

  if(end+2>=strlen(desc))
    return ErrorCmd(pl, index);

  value = ScanProplist(&(desc[end+1]), &end);
  if(!value)
      return ErrorCmd(pl, index);

  SetDomain(key, value, skip_pid);
  
  return YES;
}

BOOL RemoveCmd(proplist_t pl, int index, BOOL skip)
{
  int sock;
  char *desc, *tmp;
  proplist_t key, value;
  int end;
  BOOL ret;
  pid_t skip_pid;

  BOOL auth;
  proplist_t el;

  el = PLGetArrayElement(pl, index);
  auth = *(BOOL *)PLGetDataBytes(PLGetDictionaryEntry(el, auth_key));
  sock = *(int *)PLGetDataBytes(PLGetDictionaryEntry(el, sock_key));

  if(!auth)
    return WriteString(sock, NOPERMSTR);

  if(skip)
    {
      tmp = (char *)strtok(NULL, " \t\n\r");
      if(!tmp) return ErrorCmd(pl, index);
      skip_pid = (pid_t)atoi(tmp);
      if(!skip_pid) return ErrorCmd(pl, index);
    }
  else skip_pid = 0;
  
  desc = (char *)strtok(NULL, "\n\r");

  key = ScanProplist(desc, &end);
  if(!key)
    return ErrorCmd(pl, index);

  if(!useMultiple)
    {
      if(!defaults)
	return YES;
      
      value = PLGetDictionaryEntry(defaults, key);
      
      if(!value)
	return YES;
      
      ret = RemoveDomain(key, skip_pid);
    }
  else
    {
      struct stat buf;
      
      if(!StatDomain(filename, key, &buf))
	return YES;

      ret = RemoveDomain(key, skip_pid);
    }

  PLRelease(key);
  
  return ret;
}

BOOL RegisterCmd(proplist_t pl, int index)
{
  int sock;
  char *desc, *tmp;
  proplist_t key, arr, data;
  int end;
  pid_t pid;

  BOOL auth;
  proplist_t el;

  el = PLGetArrayElement(pl, index);
  auth = *(BOOL *)PLGetDataBytes(PLGetDictionaryEntry(el, auth_key));
  sock = *(int *)PLGetDataBytes(PLGetDictionaryEntry(el, sock_key));

  if(!auth)
    return WriteString(sock, NOPERMSTR);

  tmp = (char *)strtok(NULL, " \t\n\r");
  if(!tmp) return ErrorCmd(pl, index);
  pid = (pid_t)atoi(tmp);
  if(!pid) return ErrorCmd(pl, index);
  
  desc = (char *)strtok(NULL, "\n\r");

  key = ScanProplist(desc, &end);
  if(!key)
    return ErrorCmd(pl, index);

  if(!notifyDict)
    {
      data = PLMakeData((void *)&pid, sizeof(pid_t));
      arr = PLMakeArrayFromElements(data, NULL);
      PLRelease(data);
      notifyDict = PLMakeDictionaryFromEntries(key, arr, NULL);
      PLRelease(arr); PLRelease(key); 
    }
  else if(!(arr = PLGetDictionaryEntry(notifyDict, key)))
    {
      data = PLMakeData((void *)&pid, sizeof(pid_t));
      arr = PLMakeArrayFromElements(data, NULL);
      PLRelease(data);
      PLInsertDictionaryEntry(notifyDict, key, arr);
      PLRelease(key); PLRelease(arr);
    }
  else
    {
      data = PLMakeData((void *)&pid, sizeof(pid_t));
      PLAppendArrayElement(arr, data);
      PLRelease(data);
    }

  return YES;
}

BOOL UnregisterCmd(proplist_t pl, int index)
{
  int sock;
  char *desc, *tmp;
  proplist_t key, arr, allkeys, el;
  int i;
  int end;
  BOOL removed_sth = NO;
  pid_t pid;

  BOOL auth;

  el = PLGetArrayElement(pl, index);
  auth = *(BOOL *)PLGetDataBytes(PLGetDictionaryEntry(el, auth_key));
  sock = *(int *)PLGetDataBytes(PLGetDictionaryEntry(el, sock_key));

  if(!auth)
    return WriteString(sock, NOPERMSTR);

  tmp = (char *)strtok(NULL, " \t\n\r");
  if(!tmp) return ErrorCmd(pl, index);
  pid = (pid_t)atoi(tmp);
  if(!pid) return ErrorCmd(pl, index);
  
  desc = (char *)strtok(NULL, "\n\r");

  key = ScanProplist(desc, &end);
  if(key)
    {
      if(!notifyDict)
	{
	  PLRelease(key);
	  return NO;
	}
      arr = PLGetDictionaryEntry(notifyDict, key);
      if(!arr)
	{
	  PLRelease(key);
	  return NO;
	}
      for(i=0;i<PLGetNumberOfElements(arr); i++)
	{
	  el = PLGetArrayElement(arr, i);
	  if(*(pid_t *)PLGetDataBytes(el) == pid)
	    {
	      PLRemoveArrayElement(arr, i);
	      i--;
	      removed_sth = YES;
	    }
	}
    }
  else /* if(!key) */
    {
      if(!notifyDict)
	{
	  PLRelease(key);
	  return NO;
	}
      allkeys = PLGetAllDictionaryKeys(notifyDict);
      for(i=0; i<PLGetNumberOfElements(allkeys); i++)
	{
	  key = PLGetArrayElement(allkeys, i);
	  arr = PLGetDictionaryEntry(notifyDict, key);
	  for(i=0; i<PLGetNumberOfElements(arr); i++)
	    {
	      el = PLGetArrayElement(arr, i);
	      if(*(pid_t *)PLGetDataBytes(el) == pid)
		{
		  PLRemoveArrayElement(arr, i);
		  i--;
		  removed_sth = YES;
		}
	    }
	}
      PLRelease(allkeys);
    }

  return removed_sth;
}

BOOL AuthCmd(proplist_t pl, int index)
{
  proplist_t el;
  char *tmp;
  int sock;
  
  el = PLGetArrayElement(pl, index);

  tmp = strtok(NULL, " \t\n\r");

  if(!tmp)
    return ErrorCmd(pl, index);

  if(!strcmp(tmp, password)) /* OK */
    {
      *(BOOL *)PLGetDataBytes(PLGetDictionaryEntry(el, auth_key)) = YES;
      return YES;
    }
  else
    {
      sock = *(int *)PLGetDataBytes(PLGetDictionaryEntry(el, sock_key));
      return WriteString(sock, AUTHERRSTR);
    }
}
  
BOOL UnknownCmd(proplist_t pl, int index)
{
  int sock;
  proplist_t el;

  el = PLGetArrayElement(pl, index);
  sock = *(int *)PLGetDataBytes(PLGetDictionaryEntry(el, sock_key));

  return WriteString(sock, UNKNOWNSTR);
}




