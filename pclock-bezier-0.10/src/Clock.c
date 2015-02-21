/*
 * $XConsortium: Clock.c,v 1.28 94/04/17 20:37:56 rws Exp $
 *
Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
 */

/*
 * Clock.c
 *
 * a NeWS clone clock
 */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/Converters.h>
#include <X11/Xos.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <X11/extensions/shape.h>
#include "transform.h"

#ifdef X_NOT_STDC_ENV
extern struct tm *localtime();
#define Time_t long
extern Time_t time();
#else
#include <time.h>
#define Time_t time_t
#endif

#ifndef PI	/* may be found in <math.h> */
# define PI (3.14159265358979323846)
#endif

#define offset(field) XtOffsetOf(ClockRec, clock.field)
#define goffset(field) XtOffsetOf(WidgetRec, core.field)

#define min(a,b) (((a) < (b)) ? (a) : (b))

#undef offset
#undef goffset

TPoint *
bezier_points (int npoints, TPoint control_points[4])
{
  int i;
  TPoint *points = NULL;
  if (npoints < 8 || control_points == NULL)
    {
      fprintf (stderr, "bezier_points: invalid parameters\n");
      return NULL;
    }
  if ((points = malloc (npoints * sizeof (*points))) == NULL)
    {
      fprintf (stderr, "bezier_points: out of memory error !\n");
      return NULL;
    }
  for (i = 0; i < npoints; i++)
    {
      double array[4];
      double u, u2, u3;

      u = (double) i / (double) (npoints - 1);
      u2 = u * u;
      u3 = u2 * u;
      array[0] = -u3 + 3. * u2 - 3. * u + 1.;
      array[1] = 3. * u3 - 6. * u2 + 3. * u;
      array[2] = -3. * u3 + 3. * u2;
      array[3] = u3;
      points[i].x = array[0] * control_points[0].x + array[1] * control_points[1].x + array[2] * control_points[2].x + array[3] * control_points[3].x;
      points[i].y = array[0] * control_points[0].y + array[1] * control_points[1].y + array[2] * control_points[2].y + array[3] * control_points[3].y;
    }
  return points;
}
