/* TEMPORARY CODE */

/****************************************************************************
 * This module is all new
 * by Rob Nation 
 *
 * This code handles colormaps for fvwm.
 *
 * Copyright 1994 Robert Nation. No restrictions are placed on this code,
 * as long as the copyright notice is preserved . No guarantees or
 * warrantees of any sort whatsoever are given or implied or anything.
 ****************************************************************************/


#include "wconfig.h"

#include <stdio.h>
#include <unistd.h>
#include "WindowMaker.h"
#include <X11/Xatom.h>
#include "window.h"

WWindow *colormap_win;
Colormap last_cmap = None;


/***********************************************************************
 *
 *  Procedure:
 *	HandleColormapNotify - colormap notify event handler
 *
 * This procedure handles both a client changing its own colormap, and
 * a client explicitly installing its colormap itself (only the window
 * manager should do that, so we must set it correctly).
 *
 ***********************************************************************/
void HandleColormapNotify(WWindow *Tmp_win, XEvent *Event)
{
  XColormapEvent *cevent = (XColormapEvent *)Event;
  Bool ReInstall = False;
  XWindowAttributes attr;


  if(!Tmp_win)
    {
	
      return;
    }
  if(cevent->new)
    {
      XGetWindowAttributes(dpy,Tmp_win->client_win,&attr);
      if((Tmp_win  == colormap_win)&&(Tmp_win->cmap_window_no == 0))
	last_cmap = attr.colormap;
      ReInstall = True;
    }
  else if((cevent->state == ColormapUninstalled)&&
	  (last_cmap == cevent->colormap))
    {
      /* Some window installed its colormap, change it back */
      ReInstall = True;
    }
  
  while(XCheckTypedEvent(dpy,ColormapNotify,Event))
    {
	Tmp_win = wWindowFor(cevent->window);
      if((Tmp_win)&&(cevent->new))
	{
	  XGetWindowAttributes(dpy,Tmp_win->client_win,&attr);
	  if((Tmp_win  == colormap_win)&&(Tmp_win->cmap_window_no == 0))
	    last_cmap = attr.colormap;
	  ReInstall = True;
	}
      else if((Tmp_win)&&
	      (cevent->state == ColormapUninstalled)&&
	      (last_cmap == cevent->colormap))
	{
	  /* Some window installed its colormap, change it back */
	  ReInstall = True;
	}
      else if((Tmp_win)&&
	      (cevent->state == ColormapInstalled)&&
	      (last_cmap == cevent->colormap))
	{
	  /* The last color map installed was the correct one. Don't 
	   * change anything */
	  ReInstall = False;
	}
    }

  /* Reinstall the colormap that we think should be installed,
   * UNLESS and unrecognized window has the focus - it might be
   * an override-redirect window that has its own colormap. */
  if(ReInstall && last_cmap!=None)
    {
      XInstallColormap(dpy,last_cmap);    
    }
}

void InstallWindowColormaps (WWindow *tmp);

/************************************************************************
 *
 * Re-Install the active colormap 
 *
 *************************************************************************/
void ReInstallActiveColormap(void)
{
  InstallWindowColormaps(colormap_win);
}


static int RootPushes=0;
WWindow *Pushed_window=NULL;
static WWindow *MyRootWindow=NULL;
extern WScreen *wScreen;

/***********************************************************************
 *
 *  Procedure:
 *	InstallWindowColormaps - install the colormaps for one fvwm window
 *
 *  Inputs:
 *	type	- type of event that caused the installation
 *	tmp	- for a subset of event types, the address of the
 *		  window structure, whose colormaps are to be installed.
 *
 ************************************************************************/

void InstallWindowColormaps (WWindow *tmp)
{
  int i;
  XWindowAttributes attributes;
  Window w;
  Bool ThisWinInstalled = False;


  /* If no window, then install root colormap */
  if(!tmp) {
      if (!MyRootWindow) {
	  MyRootWindow=wmalloc(sizeof(WWindow));
	  MyRootWindow->cmap_window_no=0;
	  MyRootWindow->client_win = wScreen->root_win;
      }
      tmp=MyRootWindow;
  }

  colormap_win = tmp;
  /* Save the colormap to be loaded for when force loading of
   * root colormap(s) ends.
   */
  Pushed_window = tmp;
  /* Don't load any new colormap if root colormap(s) has been
   * force loaded.
   */
  if (RootPushes)
    {
      return;
    }

  if(tmp->cmap_window_no > 0)
    {
      for(i=tmp->cmap_window_no -1; i>=0;i--)
	{
	  w = tmp->cmap_windows[i];
	  if(w == tmp->client_win)
	    ThisWinInstalled = True;
	  XGetWindowAttributes(dpy,w,&attributes);
	  if (attributes.colormap==None)
	      attributes.colormap = wScreen->colormap;
	    
	  if(last_cmap != attributes.colormap)
	    {
	      last_cmap = attributes.colormap;
	      if (attributes.colormap!=None)
		  XInstallColormap(dpy,attributes.colormap);    
	    }
	}
    }

  if(!ThisWinInstalled)
    {
	XGetWindowAttributes(dpy,tmp->client_win,&attributes);
	if (attributes.colormap==None)
	  attributes.colormap=wScreen->colormap;
      if(last_cmap != attributes.colormap)
	{
	  last_cmap = attributes.colormap;
	  if (attributes.colormap!=None)
	      XInstallColormap(dpy,attributes.colormap);    
	}
    }
}


/***********************************************************************
 *
 *  Procedures:
 *	<Uni/I>nstallRootColormap - Force (un)loads root colormap(s)
 *
 *	   These matching routines provide a mechanism to insure that
 *	   the root colormap(s) is installed during operations like
 *	   rubber banding or menu display that require colors from
 *	   that colormap.  Calls may be nested arbitrarily deeply,
 *	   as long as there is one UninstallRootColormap call per
 *	   InstallRootColormap call.
 *
 *	   The final UninstallRootColormap will cause the colormap list
 *	   which would otherwise have be loaded to be loaded, unless
 *	   Enter or Leave Notify events are queued, indicating some
 *	   other colormap list would potentially be loaded anyway.
 ***********************************************************************/
void InstallRootColormap()
{
  WWindow *tmp;
  if (RootPushes == 0) 
    {
      tmp = Pushed_window;
      InstallWindowColormaps(NULL);
      Pushed_window = tmp;
    }
  RootPushes++;
  return;
}

/***************************************************************************
 * 
 * Unstacks one layer of root colormap pushing 
 * If we peel off the last layer, re-install th e application colormap
 * 
 ***************************************************************************/
void UninstallRootColormap()
{
  if (RootPushes)
    RootPushes--;
  
  if (!RootPushes) 
    {
      InstallWindowColormaps(Pushed_window);
    }
  
  return;
}



