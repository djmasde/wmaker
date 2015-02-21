

#include "WINGsP.h"

#include <X11/keysym.h>



static void
alertPanelOnClick(WMWidget *self, void *clientData)
{
    WMAlertPanel *panel = clientData;

    panel->done = 1;
    if (self == panel->defBtn) {
	panel->result = WAPRDefault;
    } else if (self == panel->othBtn) {
	panel->result = WAPROther;
    } else if (self == panel->altBtn) {
	panel->result = WAPRAlternate;
    }
}


static void
handleKeyPress(XEvent *event, void *clientData)
{
    WMAlertPanel *panel = (WMAlertPanel*)clientData;

    if (event->xkey.keycode == panel->retKey) {
	WMPerformButtonClick(panel->defBtn);
    }
}


int 
WMRunAlertPanel(WMScreen *scrPtr, char *title, char *msg, char *defaultButton,
		char *alternateButton, char *otherButton)
{
    WMAlertPanel *panel;
    int tmp;
    
    panel = WMCreateAlertPanel(scrPtr, title, msg, defaultButton,
			       alternateButton, otherButton);
    
    WMMapWidget(panel->win);

    while (!panel->done) {
	XEvent event;
	
	WMNextEvent(scrPtr->display, &event);
	WMHandleEvent(&event);
    }
    

    tmp = panel->result;

    WMDestroyAlertPanel(panel);
    
    return tmp;
}


void
WMDestroyAlertPanel(WMAlertPanel *panel)
{
    WMUnmapWidget(panel->win);
    WMDestroyWidget(panel->win);
    free(panel);
}


WMAlertPanel* 
WMCreateAlertPanel(WMScreen *scrPtr, char *title, char *msg, char *defaultButton,
		   char *alternateButton, char *otherButton)
{
    WMAlertPanel *panel;
    int x, dw=0, aw=0, ow=0, w;
    
    
    panel = wmalloc(sizeof(WMAlertPanel));
    memset(panel, 0, sizeof(WMAlertPanel));

        
    panel->retKey = XKeysymToKeycode(scrPtr->display, XK_Return);
        
    panel->win = WMCreateWindow(scrPtr, "alertPanel");

    if (scrPtr->applicationIcon) {		
	panel->iLbl = WMCreateLabel(panel->win);
	WMResizeWidget(panel->iLbl, scrPtr->applicationIcon->width,
		       scrPtr->applicationIcon->height);
	WMMoveWidget(panel->iLbl, 8 + (64 - scrPtr->applicationIcon->width)/2,
		     (75 - scrPtr->applicationIcon->height)/2);
	WMSetLabelImage(panel->iLbl, scrPtr->applicationIcon);
	WMSetLabelImagePosition(panel->iLbl, WIPImageOnly);
    }
    
    if (title) {
	WMFont *largeFont;
	
	largeFont = WMBoldSystemFontOfSize(scrPtr, 24);

	panel->tLbl = WMCreateLabel(panel->win);
	WMMoveWidget(panel->tLbl, 80, (80 - largeFont->height)/2);
	WMResizeWidget(panel->tLbl, 400 - 70, largeFont->height+4);
	WMSetLabelText(panel->tLbl, title);
	WMSetLabelTextAlignment(panel->tLbl, WALeft);
	WMSetLabelFont(panel->tLbl, largeFont);
	
	WMReleaseFont(largeFont);
    }


    if (msg) {
	panel->mLbl = WMCreateLabel(panel->win);
	WMMoveWidget(panel->mLbl, 10, 95);
	WMResizeWidget(panel->mLbl, 380, scrPtr->normalFont->height*3);
	WMSetLabelText(panel->mLbl, msg);
	WMSetLabelTextAlignment(panel->mLbl, WACenter);
    }
    
    
    /* create divider line */
    
    panel->line = WMCreateFrame(panel->win);
    WMMoveWidget(panel->line, 0, 80);
    WMResizeWidget(panel->line, 400, 2);
    WMSetFrameRelief(panel->line, WRGroove);

    /* create buttons */
    if (otherButton) 
	ow = XTextWidth(scrPtr->normalFont->font, otherButton, 
			strlen(otherButton));
    
    if (alternateButton)
	aw = XTextWidth(scrPtr->normalFont->font, alternateButton, 
			strlen(alternateButton));
    
    if (defaultButton)
	dw = XTextWidth(scrPtr->normalFont->font, defaultButton,
			strlen(defaultButton));
    
    w = dw + (scrPtr->buttonArrow ? scrPtr->buttonArrow->width : 0);
    if (aw > w)
	w = aw;
    if (ow > w)
	w = ow;

    w += 30;
    x = 400;

    if (defaultButton) {
	x -= w + 10;
	
	panel->defBtn = WMCreateCustomButton(panel->win, WBBPushInMask
					     |WBBPushChangeMask
					     |WBBPushLightMask);
	WMSetButtonAction(panel->defBtn, alertPanelOnClick, panel);
	WMMoveWidget(panel->defBtn, x, 144);
	WMResizeWidget(panel->defBtn, w, 24);
	WMSetButtonText(panel->defBtn, defaultButton);
	WMSetButtonImage(panel->defBtn, scrPtr->buttonArrow);
	WMSetButtonAltImage(panel->defBtn, scrPtr->pushedButtonArrow);
	WMSetButtonImagePosition(panel->defBtn, WIPRight);
    }
    if (alternateButton) {
	x -= w + 10;

	panel->altBtn = WMCreateCommandButton(panel->win);
	WMMoveWidget(panel->altBtn, x, 144);
	WMResizeWidget(panel->altBtn, w, 24);
	WMSetButtonAction(panel->altBtn, alertPanelOnClick, panel);
	WMSetButtonText(panel->altBtn, alternateButton);
    }
    if (otherButton) {
	x -= w + 10;
	
	panel->othBtn = WMCreateCommandButton(panel->win);
	WMSetButtonAction(panel->othBtn, alertPanelOnClick, panel);
	WMMoveWidget(panel->othBtn, x, 144);
	WMResizeWidget(panel->othBtn, w, 24);
	WMSetButtonText(panel->othBtn, otherButton);	
    }

    panel->done = 0;
    
    WMCreateEventHandler(W_VIEW(panel->win), KeyPressMask,
			 handleKeyPress, panel);

    WMRealizeWidget(panel->win);
    WMMapSubwidgets(panel->win);

    return panel;
}





static void
inputBoxOnClick(WMWidget *self, void *clientData)
{
    WMInputPanel *panel = clientData;

    panel->done = 1;
    if (self == panel->defBtn) {
	panel->result = WAPRDefault;
    } else if (self == panel->altBtn) {
	panel->result = WAPRAlternate;
    }
}



static void
handleKeyPress2(XEvent *event, void *clientData)
{
    WMInputPanel *panel = (WMInputPanel*)clientData;

    if (event->xkey.keycode == panel->retKey) {
	WMPerformButtonClick(panel->defBtn);
    }
}


char*
WMRunInputPanel(WMScreen *scrPtr, char *title, char *msg, char *defaultText, 
		char *okButton, char *cancelButton)
{
    WMInputPanel *panel;
    char *tmp;

    panel = WMCreateInputPanel(scrPtr, title, msg, defaultText, okButton, 
			       cancelButton);
    

    WMMapWidget(panel->win);

    while (!panel->done) {
	XEvent event;
	
	WMNextEvent(scrPtr->display, &event);
	WMHandleEvent(&event);
    }


    if (panel->result == WAPRDefault)
	tmp = WMGetTextFieldText(panel->text);
    else
	tmp = NULL;

    WMDestroyInputPanel(panel);

    return tmp;
}


void
WMDestroyInputPanel(WMInputPanel *panel)
{
    WMUnmapWidget(panel->win);
    WMDestroyWidget(panel->win);
    free(panel);
}


WMInputPanel*
WMCreateInputPanel(WMScreen *scrPtr, char *title, char *msg, char *defaultText,
		   char *okButton, char *cancelButton)
{
    WMInputPanel *panel;
    int x, dw=0, aw=0, w;
    
    
    panel = wmalloc(sizeof(WMInputPanel));
    memset(panel, 0, sizeof(WMInputPanel));

    panel->retKey = XKeysymToKeycode(scrPtr->display, XK_Return);
    
    panel->win = WMCreateWindow(scrPtr, "inputPanel");

    WMResizeWidget(panel->win, 320, 160);
    
    if (title) {
	WMFont *largeFont;
	
	largeFont = WMBoldSystemFontOfSize(scrPtr, 24);
	
	panel->tLbl = WMCreateLabel(panel->win);
	WMMoveWidget(panel->tLbl, 20, 16);
	WMResizeWidget(panel->tLbl, 320 - 40, largeFont->height+4);
	WMSetLabelText(panel->tLbl, title);
	WMSetLabelTextAlignment(panel->tLbl, WALeft);
	WMSetLabelFont(panel->tLbl, largeFont);
	
	WMReleaseFont(largeFont);
    }


    if (msg) {
	panel->mLbl = WMCreateLabel(panel->win);
	WMMoveWidget(panel->mLbl, 20, 50);
	WMResizeWidget(panel->mLbl, 320 - 40,scrPtr->normalFont->height*2);
	WMSetLabelText(panel->mLbl, msg);
	WMSetLabelTextAlignment(panel->mLbl, WALeft);
    }
    
    panel->text = WMCreateTextField(panel->win);
    WMMoveWidget(panel->text, 20, 85);
    WMResizeWidget(panel->text, 320 - 40, WMWidgetHeight(panel->text));
    WMSetTextFieldText(panel->text, defaultText);
	
    /* create buttons */
    if (cancelButton)
	aw = XTextWidth(scrPtr->normalFont->font, cancelButton, 
			strlen(cancelButton));
    
    if (okButton)
	dw = XTextWidth(scrPtr->normalFont->font, okButton,
			strlen(okButton));
    
    w = dw + (scrPtr->buttonArrow ? scrPtr->buttonArrow->width : 0);
    if (aw > w)
	w = aw;

    w += 30;
    x = 310;

    if (okButton) {
	x -= w + 10;

	panel->defBtn = WMCreateCustomButton(panel->win, WBBPushInMask
					     |WBBPushChangeMask
					     |WBBPushLightMask);
	WMSetButtonAction(panel->defBtn, inputBoxOnClick, panel);
	WMMoveWidget(panel->defBtn, x, 124);
	WMResizeWidget(panel->defBtn, w, 24);
	WMSetButtonText(panel->defBtn, okButton);
	WMSetButtonImage(panel->defBtn, scrPtr->buttonArrow);
	WMSetButtonAltImage(panel->defBtn, scrPtr->pushedButtonArrow);
	WMSetButtonImagePosition(panel->defBtn, WIPRight);
    }
    if (cancelButton) {
	x -= w + 10;

	panel->altBtn = WMCreateCommandButton(panel->win);
	WMSetButtonAction(panel->altBtn, inputBoxOnClick, panel);
	WMMoveWidget(panel->altBtn, x, 124);
	WMResizeWidget(panel->altBtn, w, 24);
	WMSetButtonText(panel->altBtn, cancelButton);
    }

    panel->done = 0;
    
    WMCreateEventHandler(W_VIEW(panel->win), KeyPressMask,
			 handleKeyPress2, panel);

    WMRealizeWidget(panel->win);
    WMMapSubwidgets(panel->win);

    W_SetFocusToView(WMWidgetView(panel->text));

    return panel;
}



