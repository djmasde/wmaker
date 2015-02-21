/* RunLoop.c:

   Handles the run loop, dispatching incoming connections etc.

   */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "proplist.h"
#include "util.h"

#include "Main.h"
#include "Common.h"
#include "Commands.h"

proplist_t clients = NULL;
proplist_t callbacks = NULL;
proplist_t sock_key;
proplist_t auth_key;

extern void CheckChange(void);

void CleanUpRunLoop(void)
{
  if(clients) PLRelease(clients);
  if(callbacks) PLRelease(callbacks);
  if(sock_key) PLRelease(sock_key);
  if(auth_key) PLRelease(auth_key);
}

BOOL Accept(int sock)
{
  int newconn;
  struct sockaddr addr;
  int addrlen;
  proplist_t dict;
  proplist_t sock_pl, auth_pl;
  BOOL no = NO;

  if((newconn = accept(sock, &addr, &addrlen)) < 0)
    {
      fprintf(stderr, "gsdd: Error accepting\n");
      return NO;
    }

  if(!clients)
    clients = PLMakeArrayFromElements(NULL);
  if(!sock_key)
    sock_key = PLMakeString("Socket");
  if(!auth_key)
    auth_key = PLMakeString("Auth");

  sock_pl = PLMakeData((void *)&newconn, sizeof(int));
  auth_pl = PLMakeData((void *)&no, sizeof(BOOL));
  
  dict = PLMakeDictionaryFromEntries(sock_key, sock_pl,
				     auth_key, auth_pl,
				     NULL);
  PLRelease(sock_pl); PLRelease(auth_pl);

  PLAppendArrayElement(clients, dict);

  PLRelease(dict);

  return YES;
}

/* Returns NO if no bytes could be read. That means that the socket
   has been closed. */
BOOL HandleConnection(proplist_t pl, int index)
{
  char *line;
  char *cmd;
  int sock;
  proplist_t el;

  el = PLGetArrayElement(pl, index);

  sock = *(int *)PLGetDataBytes(PLGetDictionaryEntry(el, sock_key));
  
  if(!(line = ReadStringAnySize(sock)))
    return NO;

  
  cmd = (char *)strtok(line, " \t\n\r");

  if(!cmd) return YES;
  if(!strcasecmp(cmd, "help"))
    HelpCmd(pl, index);
  else if(!strcasecmp(cmd, "list"))
    ListCmd(pl, index);
  else if(!strcasecmp(cmd, "get"))
    GetCmd(pl, index);
  else if(!strcasecmp(cmd, "auth"))
    AuthCmd(pl, index);
  else if(!strcasecmp(cmd, "set"))
    SetCmd(pl, index, NO);
  else if(!strcasecmp(cmd, "set-nonotify"))
    SetCmd(pl, index, YES);
  else if(!strcasecmp(cmd, "remove"))
    RemoveCmd(pl, index, NO);
  else if(!strcasecmp(cmd, "remove-nonotify"))
    RemoveCmd(pl, index, YES);
  else if(!strcasecmp(cmd, "register"))
    RegisterCmd(pl, index);
  else if(!strcasecmp(cmd, "unregister"))
    UnregisterCmd(pl, index);
  else UnknownCmd(pl, index);

  MyFree(__FILE__, __LINE__, line);
  return YES;
}

BOOL CloseConnection(proplist_t pl, int index)
{
  int sock;
  proplist_t el;

  el = PLGetArrayElement(pl, index);

  sock = *(int *)PLGetDataBytes(PLGetDictionaryEntry(el, sock_key));

  PLRemoveArrayElement(pl, index);

  close(sock);
  return YES;
}

void RunLoop(int sock)
{
  fd_set readfds, writefds, exceptfds;
  int retval;
  int numfds;
  int i, n;
  int s;
  proplist_t el;
  struct timeval tv;
  
  while(1)
    {
      CheckChange();
      FD_ZERO(&readfds);
      FD_ZERO(&writefds);
      FD_ZERO(&exceptfds);
      tv.tv_sec = 2;
      tv.tv_usec = 0;

      FD_SET(sock, &readfds); 
      if(clients)
	{
	  n = PLGetNumberOfElements(clients);
	  for(i=0; i<n; i++)
	    {
	      el = PLGetArrayElement(clients, i);
	      s =
		*(int *)(PLGetDataBytes(PLGetDictionaryEntry(el, sock_key)));
	      FD_SET(s, &readfds);
	      FD_SET(s, &exceptfds);
	    }
	}
      
      numfds = 255;

      retval = select(numfds, &readfds, &writefds, &exceptfds, &tv);

      if(retval<0)
	GIVEUP("Error selecting", "select");
      
      if(FD_ISSET(sock, &readfds)) /* a new connection */
	Accept(sock);

      if(clients)
	{
	  n = PLGetNumberOfElements(clients);
	  for(i=0; i<n; i++)
	    {
	      el = PLGetArrayElement(clients, i);
	      s =
		*(int *)(PLGetDataBytes(PLGetDictionaryEntry(el, sock_key)));
	      if(FD_ISSET(s, &readfds))
		{
		  if(!HandleConnection(clients, i))
		    {
		      CloseConnection(clients, i);
		      /* side-effect of CloseConnection: Removes one
			 element from clients */
		      i--;
		      n--;
		    }
		}
	      else if(FD_ISSET(s, &exceptfds))
		{
		  printf("Exception!\n");
		}
	    }
	}
    }
}



