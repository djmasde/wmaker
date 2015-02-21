



#include "WINGsP.h"


typedef struct ColumnList {
    char *title;
    char *selectedText;		       /* can't be freed */
    WMListItem *column;
    int topRow;
    struct ColumnList *nextPtr;
} ColumnList;


typedef struct W_Browser {
    W_Class widgetClass;
    W_View *view;

    WMList **visibleColumns;
    WMLabel **visibleTitles;
    
    ColumnList *columns;
    short columnCount;
    short visibleColumnCount;
    short minColumnWidth;
 
    short maxVisibleColumns;
    short firstVisibleColumn;

    short titleHeight;

    WMSize columnSize;
    

    void *clientData;
    WMAction *action;
    void *doubleClientData;
    WMAction *doubleAction;

    WMBrowserFillColumn *fillColumn;
    
    WMScroller *scroller;

    char *pathSeparator;
    
    struct {
	unsigned int isTitled:1;
	unsigned int allowMultipleSelection:1;
	unsigned int hasScroller:1;
	
	/* */
	
	unsigned int redrawPending:1;
	unsigned int loaded:1;
	unsigned int loadingColumn:1;
    } flags;
} Browser;


#define COLUMN_SPACING 	4
#define TITLE_SPACING 2
#define SCROLLER_SPACING 2

#define DEFAULT_WIDTH		305
#define DEFAULT_HEIGHT		200
#define DEFAULT_HAS_SCROLLER	True

#define DEFAULT_SEPARATOR	"/"

#define COLUMN_IS_VISIBLE(b, c)	((c) >= (b)->firstVisibleColumn \
				&& (c) < (b)->firstVisibleColumn + (b)->maxVisibleColumns)


static void handleEvents(XEvent *event, void *data);
static void destroyBrowser(WMBrowser *bPtr);

static void setupScroller(WMBrowser *bPtr);

static void scrollToColumn(WMBrowser *bPtr, int column);

static void setupVisibleColumns(WMBrowser *bPtr);

static void paintItem(WMList *lPtr, Drawable d, char *text, int state, 
		      WMRect *rect);

static void loadColumn(WMBrowser *bPtr, int column);

W_ViewProcedureTable _BrowserViewProcedures = {
    NULL,
	NULL,
	NULL,
	NULL
};



WMBrowser*
WMCreateBrowser(WMWidget *parent)
{
    WMBrowser *bPtr;

    bPtr = wmalloc(sizeof(WMBrowser));
    memset(bPtr, 0, sizeof(WMBrowser));

    bPtr->widgetClass = WC_Browser;
    
    bPtr->view = W_CreateView(W_VIEW(parent));
    if (!bPtr->view) {
	free(bPtr);
	return NULL;
    }

    WMCreateEventHandler(bPtr->view, ExposureMask|StructureNotifyMask
			 |ClientMessageMask, handleEvents, bPtr);

    /* default configuration */
    W_ResizeView(bPtr->view, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    bPtr->flags.hasScroller = DEFAULT_HAS_SCROLLER;

    bPtr->maxVisibleColumns = 2;
    
    bPtr->columnSize.width = 150;
    bPtr->columnSize.height = 150;

    bPtr->titleHeight = 20;
    bPtr->flags.isTitled = 1;

    bPtr->pathSeparator = wstrdup(DEFAULT_SEPARATOR);

    if (bPtr->flags.hasScroller)
	setupScroller(bPtr);

    setupVisibleColumns(bPtr);

    return bPtr;
}


void
WMSetBrowserColumnTitle(WMBrowser *bPtr, int column, char *title)
{
    ColumnList *clPtr;
    int i;

    assert(column >= 0);
    assert(column < bPtr->columnCount);
    
    i = column;
    clPtr = bPtr->columns;
    while (i-- > 0) {
	clPtr = clPtr->nextPtr;
    }
    
    if (clPtr->title)
	free(clPtr->title);
    if (title)
	clPtr->title = wstrdup(title);
    else
	clPtr->title = NULL;
        
    if (COLUMN_IS_VISIBLE(bPtr, column)) {	
	i = column - bPtr->firstVisibleColumn;

	WMSetLabelText(bPtr->visibleTitles[i], title);
    }
}



static void
freeColumnItems(WMBrowser *bPtr, WMListItem *list)
{
    WMListItem *tmp;
    
    while (list) {
	free(list->text);
	
	tmp = list->nextPtr;
	free(list);
	list = tmp;
    }
}


void 
WMSetBrowserFillColumnProc(WMBrowser *bPtr, WMBrowserFillColumn *proc)
{
    bPtr->fillColumn = proc;
}


static void
removeColumn(WMBrowser *bPtr, int column)
{
    ColumnList *clPtr, *tmp = NULL;
    int i;

    if (column >= bPtr->columnCount)
	return;
    
    i = column;
    clPtr = bPtr->columns;
    while (i-- > 0) {
	tmp = clPtr;
	clPtr = clPtr->nextPtr;
    }

    if (tmp)
	tmp->nextPtr = NULL;
    
    while (clPtr) {
	bPtr->columnCount--;
	freeColumnItems(bPtr, clPtr->column);
	if (clPtr->title)
	    free(clPtr->title);
	if (COLUMN_IS_VISIBLE(bPtr, column)) {
	    int index;
	    
	    index = column-bPtr->firstVisibleColumn;
	    WMHangData(bPtr->visibleColumns[index], NULL);
	    WMSetListItems(bPtr->visibleColumns[index], NULL);
	    if (bPtr->flags.isTitled)
		WMSetLabelText(bPtr->visibleTitles[index], NULL);
	}
	column++;
	tmp = clPtr->nextPtr;
	free(clPtr);
	clPtr = tmp;
    }
}


WMListItem*
WMGetBrowserSelectedItemInColumn(WMBrowser *bPtr, int column)
{
    return WMGetListSelectedItem(bPtr->visibleColumns[column-bPtr->firstVisibleColumn]);
}



static void
insertSortedItem(ColumnList *clPtr, WMListItem *item)
{
    WMListItem *tmp;
    
    if (!clPtr->column) {
	clPtr->column = item;
    } else if (strcmp(clPtr->column->text, item->text) > 0) {
	item->nextPtr = clPtr->column;
	clPtr->column = item;
    } else {	
	int added = 0;
	
	tmp = clPtr->column;
	while (tmp->nextPtr) {
	    if (strcmp(tmp->nextPtr->text, item->text) >= 0) {
		item->nextPtr = tmp->nextPtr;
		tmp->nextPtr = item;
		added = 1;
		break;
	    }
	    tmp = tmp->nextPtr;
	}
	if (!added) {
	    tmp->nextPtr = item;
	}
    }
}


Bool
WMAddBrowserItem(WMBrowser *bPtr, int column, char *text, Bool isBranch)
{
    ColumnList *clPtr;
    int i;

    assert(column >= 0);
    assert(column < bPtr->columnCount);
    
    i = column;
    clPtr = bPtr->columns;
    while (i-- > 0) {
	clPtr = clPtr->nextPtr;
    }

    if (!bPtr->flags.loadingColumn && COLUMN_IS_VISIBLE(bPtr, column)) {
	WMListItem *item;

	i = column - bPtr->firstVisibleColumn;
	
	item = WMAddSortedListItem(bPtr->visibleColumns[i], text);
	item->isBranch = isBranch;
	
	clPtr->column = WMGetListItems(bPtr->visibleColumns[i]);
    } else {
	WMListItem *item;
    
	item = wmalloc(sizeof(WMListItem));
	memset(item, 0, sizeof(WMListItem));
	item->text = wstrdup(text);
	item->isBranch = isBranch;

	insertSortedItem(clPtr, item);
    }

    return True;
}


static void
paintItem(WMList *lPtr, Drawable d, char *text, int state, WMRect *rect)
{
    WMView *view = W_VIEW(lPtr);
    W_Screen *scr = view->screen;
    int width, height, x, y;

    width = rect->size.width;
    height = rect->size.height;
    x = rect->pos.x;
    y = rect->pos.y;

    if (state & WLDSSelected)
	XFillRectangle(scr->display, d, W_GC(scr->white), x, y,
		       width, height);
    else 
	XClearArea(scr->display, d, x, y, width, height, False);
	
    W_PaintText(view, d, scr->normalFont,  x+2, y, width,
		WALeft, scr->normalFontGC, False, text, strlen(text));
    
    if (state & WLDSIsBranch) {
	XDrawLine(scr->display, d, W_GC(scr->darkGray), x+width-11, y+3,
		  x+width-6, y+height/2);
	if (state & WLDSSelected)
	    XDrawLine(scr->display, d,W_GC(scr->gray), x+width-11, y+height-5,
		      x+width-6, y+height/2);
	else
	    XDrawLine(scr->display, d,W_GC(scr->white), x+width-11, y+height-5,
		      x+width-6, y+height/2);
	XDrawLine(scr->display, d, W_GC(scr->black), x+width-12, y+3,
		  x+width-12, y+height-5);
    }
}


static void
scrollCallback(WMWidget *scroller, void *self)
{
    WMBrowser *bPtr = (WMBrowser*)self;
    WMScroller *sPtr = (WMScroller*)scroller;
    int newFirst;
#define LAST_VISIBLE_COLUMN  bPtr->firstVisibleColumn+bPtr->maxVisibleColumns

    switch (WMGetScrollerHitPart(sPtr)) {
     case WSDecrementLine:
	if (bPtr->firstVisibleColumn > 0) {
	    scrollToColumn(bPtr, bPtr->firstVisibleColumn-1);
	}
	break;
	
     case WSDecrementPage:
	if (bPtr->firstVisibleColumn > 0) {
	    newFirst = bPtr->firstVisibleColumn - bPtr->maxVisibleColumns;

	    scrollToColumn(bPtr, newFirst);
	}
	break;

	
     case WSIncrementLine:
	if (LAST_VISIBLE_COLUMN < bPtr->columnCount) {
	    scrollToColumn(bPtr, bPtr->firstVisibleColumn+1);
	}
	break;
	
     case WSIncrementPage:
	if (LAST_VISIBLE_COLUMN < bPtr->columnCount) {
	    newFirst = bPtr->firstVisibleColumn + bPtr->maxVisibleColumns;

	    if (newFirst+bPtr->maxVisibleColumns >= bPtr->columnCount)
		newFirst = bPtr->columnCount - bPtr->maxVisibleColumns;

	    scrollToColumn(bPtr, newFirst);
	}
	break;
	
     case WSKnob:
	{
	    float floatValue;
	    float value = bPtr->columnCount - bPtr->maxVisibleColumns;
	    
	    floatValue = WMGetScrollerFloatValue(bPtr->scroller);
	    
	    floatValue = (floatValue*value)/value;
	    
	    newFirst = floatValue*(float)(bPtr->columnCount - bPtr->maxVisibleColumns);

	    if (bPtr->firstVisibleColumn != newFirst)
		scrollToColumn(bPtr, newFirst);
	    else
		WMSetScrollerParameters(bPtr->scroller, floatValue,
					bPtr->maxVisibleColumns/(float)bPtr->columnCount);

	}
	break;

     case WSKnobSlot:
     case WSNoPart:
	/* do nothing */
	break;
    }
#undef LAST_VISIBLE_COLUMN
}


static void
setupScroller(WMBrowser *bPtr)
{
    WMScroller *sPtr;
    int y;

    y = bPtr->columnSize.height + SCROLLER_SPACING + 1;
    if (bPtr->flags.isTitled)
	y += TITLE_SPACING + bPtr->titleHeight;
    
    sPtr = WMCreateScroller(bPtr);
    WMSetScrollerAction(sPtr, scrollCallback, bPtr);
    WMMoveWidget(sPtr, 1, y);
    WMResizeWidget(sPtr, bPtr->view->size.width-2, SCROLLER_WIDTH);
    
    bPtr->scroller = sPtr;
    
    WMMapWidget(sPtr);
}


void
WMSetBrowserAction(WMBrowser *bPtr, WMAction *action, void *clientData)
{
    bPtr->action = action;
    bPtr->clientData = clientData;
}


void
WMSetBrowserHasScroller(WMBrowser *bPtr, int hasScroller)
{
    bPtr->flags.hasScroller = hasScroller;
}


static char*
selectItem(ColumnList *clist, char *text)
{
    WMListItem *item, *theItem=NULL;

    item = clist->column;
    while (item) {
	if (strcmp(item->text, text)==0) {
	    theItem = item;
	    item->selected = True;
	} else {
	    item->selected = False;
	}
	item = item->nextPtr;
    }

    if (theItem) 
	return theItem->text;
    else
	return NULL;
}


Bool
WMSetBrowserPath(WMBrowser *bPtr, char *path)
{
    int i;
    ColumnList *clist;
    char *str = wstrdup(path);
    char *tmp;
    Bool ok = True;

    removeColumn(bPtr, 1);

    i = 0;
    clist = bPtr->columns;
    tmp = strtok(str, bPtr->pathSeparator);
    while (tmp) {
	/* select it in the column */
	clist->selectedText = selectItem(clist, tmp);

	if (!clist->selectedText) {
	    ok = False;
	    break;
	}
	/* load next column */
	WMAddBrowserColumn(bPtr);

	loadColumn(bPtr, i+1);
	
	tmp = strtok(NULL, bPtr->pathSeparator);
	
	clist = clist->nextPtr;
	i++;
    }
    free(str);
	    
    scrollToColumn(bPtr, bPtr->columnCount-bPtr->maxVisibleColumns);

    return ok;
}


char*
WMGetBrowserPath(WMBrowser *bPtr)
{
    return WMGetBrowserPathToColumn(bPtr, bPtr->columnCount);
}


char*
WMGetBrowserPathToColumn(WMBrowser *bPtr, int column)
{
    int i, size;
    ColumnList *clist;
    char *path;
    
    if (column >= bPtr->columnCount)
	column = bPtr->columnCount-1;
    
    clist = bPtr->columns;
    size = 0;
    for (i = 0; i <= column; i++) {
	if (!clist->selectedText)
	    break;
	size += strlen(clist->selectedText);
	clist = clist->nextPtr;
    }
    
    clist = bPtr->columns;
    path = wmalloc(size+(column+1)*strlen(bPtr->pathSeparator)+1);
    /* ignore first / */
    *path = 0;
    for (i = 0; i <= column; i++) {
	strcat(path, bPtr->pathSeparator);
	if (!clist->selectedText)
	    break;
	strcat(path, clist->selectedText);
	clist = clist->nextPtr;
    }

    return path;
}


static void
loadColumn(WMBrowser *bPtr, int column)
{
    assert(bPtr->fillColumn);
    
    bPtr->flags.loadingColumn = 1;
    (*bPtr->fillColumn)(bPtr, column);
    bPtr->flags.loadingColumn = 0;
}


static void
paintBrowser(WMBrowser *bPtr)
{
    int y;

    if (!bPtr->view->flags.mapped)
	return;
    
    y = bPtr->columnSize.height + SCROLLER_SPACING;
    if (bPtr->flags.isTitled)
	y += TITLE_SPACING + bPtr->titleHeight;    

    W_DrawRelief(bPtr->view->screen, bPtr->view->window, 0, y,
		 bPtr->view->size.width, 22, WRSunken);
}


static void
handleEvents(XEvent *event, void *data)
{
    WMBrowser *bPtr = (WMBrowser*)data;

    CHECK_CLASS(data, WC_Browser);


    switch (event->type) {
     case Expose:
	paintBrowser(bPtr);
	break;
	
     case DestroyNotify:
	destroyBrowser(bPtr);
	break;
	
    }
}


extern int _listTopItem();
extern void _listSetTopItem();



static void
scrollToColumn(WMBrowser *bPtr, int column)
{
    int i, lim;
    ColumnList *listPtr;

    if (column < 0)
	column = 0;
        
    if (column + bPtr->maxVisibleColumns < bPtr->columnCount)
	lim = bPtr->maxVisibleColumns;
    else
	lim = bPtr->columnCount - column;
    
    listPtr = bPtr->columns;
    i = column;
    while (i-- > 0)
	listPtr = listPtr->nextPtr;

    for (i = 0; i < bPtr->maxVisibleColumns; i++) {
	ColumnList *tmp;
	
	if (i < lim) {
	    tmp = WMGetHangedData(bPtr->visibleColumns[i]);
	    if (tmp!=listPtr || !tmp) {
		if (tmp)
		    tmp->topRow = _listTopItem(bPtr->visibleColumns[i]);
		
		WMHangData(bPtr->visibleColumns[i], listPtr);
		
		WMSetListItems(bPtr->visibleColumns[i], listPtr->column);
		_listSetTopItem(bPtr->visibleColumns[i], listPtr->topRow);

		if (bPtr->flags.isTitled)
		    WMSetLabelText(bPtr->visibleTitles[i], listPtr->title);
	    }	    
	    listPtr = listPtr->nextPtr;
	} else {
	    WMSetListItems(bPtr->visibleColumns[i], NULL);
	    WMHangData(bPtr->visibleColumns[i], NULL);
	    if (bPtr->flags.isTitled)
		WMSetLabelText(bPtr->visibleTitles[i], "");
	}
    }

    bPtr->firstVisibleColumn = column;
    
    /* update the scroller */
    if (bPtr->columnCount > bPtr->maxVisibleColumns) {
	float value, proportion;

	value = bPtr->firstVisibleColumn
	    /(float)(bPtr->columnCount-bPtr->maxVisibleColumns);
	proportion = bPtr->maxVisibleColumns/(float)bPtr->columnCount;
	WMSetScrollerParameters(bPtr->scroller, value, proportion);
    } else {
	WMSetScrollerParameters(bPtr->scroller, 0, 1);
    }
}


static void
listCallback(void *self, void *clientData)
{
    WMBrowser *bPtr = (WMBrowser*)clientData;
    WMList *lPtr = (WMList*)self;
    WMListItem *item;
    ColumnList *clPtr;
    int i;

    for (i = 0; i < bPtr->maxVisibleColumns; i++) {
	if (self == bPtr->visibleColumns[i])
	    break;
    }
    
    clPtr = WMGetHangedData(lPtr);
    item = WMGetListSelectedItem(lPtr);
    if (!item)
	return;
    
    clPtr->selectedText = item->text;

    /* columns at right must be cleared */
    removeColumn(bPtr, i+bPtr->firstVisibleColumn+1);

    /* open directory */
    if (item->isBranch) {	
	WMAddBrowserColumn(bPtr);
	
	loadColumn(bPtr, bPtr->columnCount-1);

	if (bPtr->columnCount < bPtr->maxVisibleColumns)
	    i = 0;
	else
	    i = bPtr->columnCount-bPtr->maxVisibleColumns;

	scrollToColumn(bPtr, i);
    }
    
    /* call callback for click */
    if (bPtr->action)
	(*bPtr->action)(bPtr, bPtr->clientData);
}


void
WMLoadBrowserColumnZero(WMBrowser *bPtr)
{
    if (!bPtr->flags.loaded) {
	/* create column 0 */
	WMAddBrowserColumn(bPtr);

	loadColumn(bPtr, 0);
	    
	/* make column 0 visible */
	scrollToColumn(bPtr, 0);

	bPtr->flags.loaded = 1;
    } 
}


static void
setupVisibleColumns(WMBrowser *bPtr)
{
    WMScreen *scr = bPtr->view->screen;
    int columnY, columnX;
    int i;
    
    if (bPtr->visibleColumns==NULL) {
	bPtr->visibleColumns = 
	    wmalloc(sizeof(WMList*)*bPtr->maxVisibleColumns);
	bPtr->visibleTitles =
	    wmalloc(sizeof(WMLabel*)*bPtr->maxVisibleColumns);
    } else {
	printf("reconfigurevisiblecolumns not supported");
	abort();
    }
    
    columnY = 0;
    columnX = 0;
        
    if (bPtr->flags.isTitled) {
	columnY = TITLE_SPACING + bPtr->titleHeight;
    }

    for (i = 0; i < bPtr->maxVisibleColumns; i++) {
	WMList *list;
	WMLabel *label;

	list = WMCreateList(bPtr);
	WMSetListAction(list, listCallback, bPtr);
	WMSetListUserDrawProc(list, paintItem);
	WMMoveWidget(list, columnX, columnY);
	WMResizeWidget(list,bPtr->columnSize.width, bPtr->columnSize.height);
	WMMapWidget(list);
	
	if (bPtr->flags.isTitled) {
	    label = WMCreateLabel(bPtr);
	    WMMoveWidget(label, columnX, 0);
	    WMResizeWidget(label, bPtr->columnSize.width, bPtr->titleHeight);
	    WMSetWidgetBackgroundColor(label, scr->darkGray);
	    WMSetLabelFont(label, scr->boldFont);
	    WMSetLabelTextColor(label, scr->white);
	    WMSetLabelRelief(label, WRSunken);
	    WMSetLabelText(label, "");
	    WMSetLabelTextAlignment(label, WACenter);
	    WMMapWidget(label);
	} else {
	    label = NULL;
	}
	
	bPtr->visibleColumns[i] = list;
	bPtr->visibleTitles[i] = label;
	
	columnX += bPtr->columnSize.width + COLUMN_SPACING;
    }
}


int
WMAddBrowserColumn(WMBrowser *bPtr)
{
    ColumnList *tmp, *listPtr;

    listPtr = wmalloc(sizeof(ColumnList));
    memset(listPtr, 0, sizeof(ColumnList));

    bPtr->columnCount++;
    
    /* add column to the end of the list */
    tmp = bPtr->columns;
    if (tmp == NULL) {
	bPtr->columns = listPtr;
    } else {
	while (tmp->nextPtr!=NULL) {
	    tmp = tmp->nextPtr;
	}
	tmp->nextPtr = listPtr;
    }

    /* update the scroller */
    if (bPtr->columnCount > bPtr->maxVisibleColumns) {
	float value, proportion;

	value = bPtr->firstVisibleColumn
	    /(float)(bPtr->columnCount-bPtr->maxVisibleColumns);
	proportion = bPtr->maxVisibleColumns/(float)bPtr->columnCount;
	WMSetScrollerParameters(bPtr->scroller, value, proportion);
    }

    return bPtr->columnCount-1;
}



static void
destroyBrowser(WMBrowser *bPtr)
{
    ColumnList *listPtr;
    int i;

    i = 0;
    while (bPtr->columns != NULL) {
	free(bPtr->columns->title);
	/* do not free column data that is used in WMLists, because
	 * they will be freed by the WMLists */
	if (i < bPtr->firstVisibleColumn 
	    || i >= bPtr->firstVisibleColumn+bPtr->maxVisibleColumns)
	    freeColumnItems(bPtr, bPtr->columns->column);
	
	listPtr = bPtr->columns->nextPtr;
	free(bPtr->columns);
	bPtr->columns = listPtr;
	i++;
    }
    
    free(bPtr->pathSeparator);
    
    free(bPtr);
}


