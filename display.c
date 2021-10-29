/*
	display.c of ue/Gulam

The functions in this file handle redisplay.  There are two halves,
the ones that update the virtual display screen, and the ones that
make the physical display screen the same as the virtual display
screen.  These functions use hints that are left in the windows by the
commands.

prabhaker mateti, 11-14-86, many improvements
prabhaker mateti, 1-1-86, ST520 changes
Steve Wilhite, 1-Dec-85  - massive cleanup on code.

*/

#include        "ue.h"
#include <stdarg.h>

#define WFDEBUG 0						/* Window flag debug. */
#define VFCHG   0x0001					/* Changed. */
#define MDLIN   0x0002					/* Mode line indicator. */


TERM term;

int mpresf;								/* TRUE if message in last line */
int sgarbf;								/* TRUE if screen is garbage */
int currow;								/* Working cursor row           */
int curcol;								/* Working cursor column        */
int screenwidth;

static int vtrow;						/* Row location of SW cursor    */
static int vtcol;						/* Column location of SW cursor */
static int ttrow;						/* Row location of HW cursor    */
static int ttcol;						/* Column location of HW cursor */


static uchar *mcp = NULL;
static LINE *mdlp;						/* spare LINE for mode line */
static uchar *ms;						/* area used by mlwrite     */
static uchar *mlstr;						/* holds the mesg line message  */
static int mloff;						/* == strlen(mlstr)     */
static VIDEO **vscreen;					/* Virtual screen       */
static VIDEO **pscreen;					/* Physical screen      */


/* Initialize the data structures used by the display code.  The edge
vectors used to access the screens are set up.  pm: (1+term.t_nrow)
because message line is now a window for minibuffer.  Gets called
initially, and then every time we change nrows, or ncols. */

void vtinit(void)								/* redone by pm */
{
	register int nc,
	 j,
	 nr;
	register uchar *p;

	if (mcp)
		free(mcp);
	sgarbf = TRUE;
	mpresf = FALSE;
	vtrow = vtcol = 0;
	ttrow = ttcol = HUGE;

	topen();
	term.t_nrow = nr = getnrow() - 1;
	screenwidth = term.t_ncol = nc = getncol();
	j = 2 * (1 + nr) * (((uint) sizeof(VIDEO *)) + ((uint) sizeof(VIDEO)) + nc);
	p = mcp = malloc(((uint) sizeof(LINE)) + 6 * nc + 2 + 2 + nc + j);
	if (p == NULL)
		outofroom();					/* which exits */

	mdlp = (LINE *) (p);
	p += ((uint) sizeof(LINE)) + 3 * nc + 2;
	ms = p;
	p += 3 * nc;
	mlstr = p;
	*p = '\0';
	p += nc + 2;
	mloff = 0;
	vscreen = (VIDEO **) p;
	p += (1 + nr) * ((uint) sizeof(VIDEO *));
	pscreen = (VIDEO **) p;
	p += (1 + nr) * ((uint) sizeof(VIDEO *));
	for (j = 0; j <= nr; ++j)
	{
		vscreen[j] = (VIDEO *) p;
		p += (((uint) sizeof(VIDEO)) + nc);
		pscreen[j] = (VIDEO *) p;
		p += (((uint) sizeof(VIDEO)) + nc);
	}
}


void uefreeall(void)
{
	freeall();
	mcp = NULL;
}

/* Write a line to the virtual screen.  The virtual row and column are
updated.  If the line is too long put a "$" in the last column.  This
routine only puts printing characters into the virtual terminal
buffers.  Only column overflow is checked.  pm.  */

/* row# on the virtual screen */
/* show lp->l_text on the above row */
static void vtputln(int n, LINE *lp)
{
	register uchar *p,
	*q,
	*r;
	register unsigned int c;

	q = vscreen[vtrow = n]->v_text;
	r = q + term.t_ncol;
	p = lp->l_text;
	n = llength(lp);
	while (n-- > 0)						/* invariant: q <= r */
	{
		c = *p++;
	  isqLTr:if (q == r)
		{
			r[-1] = '$';
			break;
		}
		if (c == '\t')
		{
			do
			{
				*q++ = ' ';				/* not nec to test q < r */
			} while ((q - r + term.t_ncol) & 0x07);
			continue;
		}								/* display a control char */
		if (c < 0x20)
		{
			*q++ = '^';
			c += '@';
			goto isqLTr;
		}
		*q++ = c;
	}
	vtcol = (int)(q - r) + term.t_ncol;
}

/* Erase from the end of the software cursor to the end of the line on
which the software cursor is located.  */

static void vteeol(void)
{
	register char *p,
	 c;
	register int i,
	 j;

	i = vtcol;
	j = vtcol = term.t_ncol;
	c = ' ';
	p = &vscreen[vtrow]->v_text[i];
	while (i++ < j)
		*p++ = c;
}

/* Erase the physical screen line #i */

static void erasepscrln(int i)
{
	register uchar *p,
	 c;

	p = pscreen[i]->v_text;
	c = '\0';
	for (i = term.t_ncol; --i >= 0;)
		*p++ = c;
}

/* Refresh the screen.  With no argument, it just does the refresh.
With an argument it recenters "." in the current window.  Bound to
"C-L".  */

int refresh(int f, int n)
{
	(void) n;
	if (f == FALSE)
		sgarbf = TRUE;
	else
	{
		curwp->w_force = 0;				/* Center dot. */
		curwp->w_flag |= WFFORCE;
	}
	keysetup();
	return TRUE;
}

/* Winodw framing is not acceptable, compute a new value for the line
at the top of the window.  Then set the "WFHARD" flag to force full
redraw.  */

static void reframe(WINDOW *wp)
{
	register LINE *lp;
	register int i;

	i = wp->w_force;
	if (i > 0)
	{
		--i;
		if (i >= wp->w_ntrows)
			i = wp->w_ntrows - 1;
	} else if (i < 0)
	{
		i += wp->w_ntrows;
		if (i < 0)
			i = 0;
	} else
		i = wp->w_ntrows / 2;
	lp = wp->w_dotp;
	while (i > 0 && lback(lp) != wp->w_bufp->b_linep)
	{
		--i;
		lp = lback(lp);
	}
	wp->w_linep = lp;
	wp->w_flag |= WFHARD;				/* Force full. */
}


static void curlnupdate(WINDOW *wp)
{
	register LINE *lp;
	register int i;

	i = wp->w_toprow;
	for (lp = wp->w_linep; lp != wp->w_dotp; lp = lforw(lp))
		++i;
	vscreen[i]->v_flag |= VFCHG;
	vtputln(i, lp);
	vteeol();
}


static void hardupdate(WINDOW *wp)
{
	register LINE *lp;
	register int i,
	 j;

	lp = wp->w_linep;
	j = wp->w_toprow + wp->w_ntrows;
	for (i = wp->w_toprow; i < j; i++)
	{
		vscreen[i]->v_flag |= VFCHG;
		vscreen[i]->v_flag &= ~MDLIN;
		vtrow = i;
		vtcol = 0;						/* vtmove(i, 0); */
		if (lp != wp->w_bufp->b_linep)
		{
			vtputln(i, lp);
			lp = lforw(lp);
		}
		vteeol();
	}
}


/* Always recompute the row and column number of the hardware cursor.
This is the only update for simple moves.  */

static void computerowcol(void)
{
	register LINE *lp;
	register uchar *p,
	*q;
	register int c;

	currow = curwp->w_toprow;
	for (lp = curwp->w_linep; lp != curwp->w_dotp; lp = lforw(lp))
		++currow;
	curcol = 0;
	if (currow == term.t_nrow)
		curcol = mloff;
	p = lp->l_text;
	q = (uchar *) ((long) p + (long) curwp->w_doto);
	while (p < q)
	{
		c = *p++;
		if (c == '\t')
			curcol |= 0x07;
		else if (c < 0x20)
			++curcol;
		++curcol;
	}
	if (curcol >= term.t_ncol)
		curcol = term.t_ncol - 1;
}

/* Special hacking if the screen is garbage.  Clear the hardware
screen, and update your copy to agree with it.  Set all the virtual
screen change bits, to force a full update.  */

static void scrgarb(void)
{
	register int i;

	for (i = 0; i <= term.t_nrow; ++i)
	{
		vscreen[i]->v_flag |= VFCHG;
		erasepscrln(i);
	}
	sgarbf = FALSE;						/* Erase-page clears */
	mpresf = FALSE;						/* the message area. */
	screenerase();
}


/* Send a command to the terminal to move the hardware cursor to row
"row" and column "col".  The row and column arguments are origin 0.
Optimize out random calls.  Update "ttrow" and "ttcol".  */

static void movecursor(int row, int col)
{
	if (row != ttrow || col != ttcol)
	{
		mvcursor(row, col);
		ttrow = row;
		ttcol = col;
	}
}


/* Update a single line.  This does not know how to use insert or
delete character sequences; we are using VT52 functionality.  Update
the physical row and column variables.  It does try and exploit erase
to end of line.  */
static void updateline(int row, uchar *vline, uchar *pline)
{
	register uchar *vp,
	*pp,
	*vq,
	*pq,
	*vr,
	*vs,
	 c;
	register int nbflag,
	 ncol;

	if (vscreen[row]->v_flag & MDLIN)
		onreversevideo();
	ncol = term.t_ncol;
	if (row == term.t_nrow)
	{
		if (mpresf)
			goto ret;					/* let the mesg stay on screen  */
		else
		{
			vq = ms;
			cpymem(vq, mlstr, mloff);
			vq += mloff;
			cpymem(vq, vline, ncol);
			vline = ms;
		}
	}

	vp = &vline[0];
	vs = vp + ncol;
	pp = &pline[0];

	/* Compute left match.  */
	while (vp < vs && vp[0] == pp[0])
	{
		++vp;
		++pp;
	}
	/*
	   This can still happen, even though we only call this routine on changed
	   lines. A hard update is always done when a line splits, a massive
	   change is done, or a buffer is displayed twice. This optimizes out most
	   of the excess updating. A lot of computes are used, but these tend to
	   be hard operations that do a lot of update, so I don't really care.
	 */
	if (vp == vs)
		goto ret;						/* All equal. */

	nbflag = FALSE;
	vq = vs;							/* Compute right match. */
	pq = &pline[ncol];

	/* Note if any nonblank in right match. */
	while (vq[-1] == pq[-1])
	{
		--vq;
		--pq;
		if (vq[0] != ' ')
			nbflag = TRUE;
	}

	vr = vq;

	if (nbflag == FALSE)
	{
		while (vr != vp && vr[-1] == ' ')
			--vr;
		if ((int) (vq - vr) <= 3)
			vr = vq;
		/* Use only if erase is fewer characters. */
	}

	movecursor(row, (int) (vp - &vline[0]));
	ttcol += (int) (vr - vp);
	while (vp < vr)
	{
		*pp++ = c = *vp++;
		gputchar(c);
	}

	if (vr != vq)
	{
		toeolerase();
		while (vp++ != vq)
			*pp++ = ' ';
	}
  ret:
	if (vscreen[row]->v_flag & MDLIN)
		offreversevideo();
}


/* Make a modeline for a regular/gulam buffer */
static void makemodeline(uchar *md, BUFFER *bp)
{
	register uchar *cp,
	*mp,
	 patc;
	register uchar *p1,
	*p2,
	*tp,
	*pmdncol;

	if (bp == setgulambp(FALSE))
	{
		tp = GulamLogo;					/* looks: >>gulam<< */
		p1 = " cwd: ";
		p2 = gfgetcwd();
	} else
	{
		tp = uE;
		p1 = " buf: ";
		p2 = bp->b_bname;
	}
	patc = bp->b_modec;
	pmdncol = &md[term.t_ncol];
	for (mp = md; mp < pmdncol;)
		*mp++ = patc;
	if ((bp->b_flag & BFCHG) != 0)
		md[1] = md[2] = '*';

	mp = &md[4];
	*mp++ = ' ';
	while ((*mp++ = *tp++) != 0)
		;				/* title    */
	*--mp = ' ';
	mp += 5;
	if ((bp->b_flag & BFTEMP) != 0)
		*mp++ = 't';
	if ((bp->b_flag & BFRDO) != 0)
		*mp++ = 'r';
	for (mp += 2; (*mp++ = *p1++) != 0;)
		;	/* cwd/buf  */
	for (mp--; (*mp++ = *p2++) != 0;)		/* cwd/buf  */
		;
	*--mp = ' ';
	if (bp->b_fname[0])					/* File name    */
	{
		cp = &bp->b_fname[0];
		for (mp += 4; (*mp++ = *cp++) != 0;)
			;
		mp[-1] = ' ';
	}
}


/* Redisplay the mode line for the window pointed to by the "wp".
This is the only routine that has any idea of how the modeline is
formatted.  You can change the modeline format by hacking at this
routine.  Called by "update" any time there is a dirty window.  */

static void modeline(WINDOW *wp)
{
	register uchar *md;
	register int n;
	LINE *lp;
	uchar mdbuf[sizeof(LINE) + 2 * 150];

	if (wp == mlwp)
		return;
	/* tired of range checks!   */
	lp = (LINE *) mdbuf;
	if (lp == NULL)
		return;
	md = lp->l_text;
	lp->l_used = term.t_ncol;
	makemodeline(md, wp->w_bufp);
	n = wp->w_toprow + wp->w_ntrows;	/* Location. */
	vscreen[n]->v_flag |= VFCHG | MDLIN;	/* Redraw next time. */
	vtputln(n, lp);
}


/* Make sure that the display is right.  This is a three part process.
First, scan through all of the windows looking for dirty ones.  Check
the framing, and refresh the screen.  Second, make sure that "currow"
and "curcol" are correct for the current window.  Third, make the
virtual and physical screens the same.  */

void update(void)
{
	register WINDOW *wp;
	register LINE *lp;
	register VIDEO *vp1;
	register VIDEO *vp2;
	register int i;

	for (wp = wheadp; wp; wp = wp->w_wndp)
	{
		if (wp->w_flag)
		{								/* If not force reframe, check the framing. */
			if ((wp->w_flag & WFFORCE) == 0)
			{
				lp = wp->w_linep;
				for (i = 0; i < wp->w_ntrows; ++i)
				{
					if (lp == wp->w_dotp)
						goto out;
					if (lp == wp->w_bufp->b_linep)
						break;
					lp = lforw(lp);
				}
			}
			reframe(wp);

		  out:if ((wp->w_flag & ~WFMODE) == WFEDIT)
				curlnupdate(wp);
			else if (wp->w_flag & (WFEDIT | WFHARD))
				hardupdate(wp);
			if (wp->w_flag & WFMODE)
				modeline(wp);
			wp->w_flag = 0;
			wp->w_force = 0;
		}
	}
	computerowcol();
	if (sgarbf != FALSE)
		scrgarb();

	/* Make sure that the physical and virtual displays agree. */
	invisiblecursor();
	i = (exitue == -1 ? term.t_nrow : 0);	/* kludge, for now! */
	for (; i <= term.t_nrow; ++i)
	{
		vp1 = vscreen[i];
		if (vp1->v_flag & VFCHG)
		{
			vp1->v_flag &= ~VFCHG;
			vp2 = pscreen[i];
			updateline(i, vp1->v_text, vp2->v_text);
		}
	}

	/* Finally, update the hardware cursor and flush out buffers. */
	visiblecursor();
	movecursor(currow, curcol);
}


/* Erase the message line. */

void mlerase(void)
{
	movecursor(term.t_nrow, 0);
	toeolerase();
	mpresf = FALSE;
	vscreen[term.t_nrow]->v_flag |= VFCHG;
	erasepscrln(term.t_nrow);
}

/* Ask a yes or no question in the message line.  Return either TRUE,
FALSE, or ABORT.  The ABORT status is returned if the user bumps out
of the question with a ^G.  Used any time a confirmation is required.
*/

int mlyesno(uchar *prompt)
{
	register int s;
	uchar buf[4];

	for (;;)
	{
		buf[0] = '\0';
		s = mlreply(prompt, buf, ((uint) sizeof(buf)));
		if (s == '\007')
			return (ABORT);
		return (buf[0] == 'y' || buf[0] == 'Y');
	}
}


/* Write a prompt into the message line, then read back a response.
Keep track of the physical position of the cursor.  If we are in a
keyboard macro throw the prompt away, and return the remembered
response.  This lets macros run at full speed.  The reply is always
terminated by a carriage return.  Handle erase, kill, and abort keys.
Returns the terminating char.  */

int mlreply(uchar *prompt, uchar *buf, int nbuf)
{
	register uchar c;

	mlmesg(prompt);
	c = getuserinput(buf, nbuf);
	mlmesg(ES);
	if (c == '\007')
		ctrlg(FALSE, 0);
	return c;
}

/* Write a message into the message line.  Keep track of the physical
cursor position.  A small class of printf like format items is
handled.  Set the "message line" flag TRUE.  Should merge with sprintp()
of util.c */

void mlwrite(const char *fmt, ...)						/* rewritten by pm */
{
	register int c,
	 r;
	uchar *p;
	va_list ap;
	long v;
	
	va_start(ap, fmt);
	p = ms;
	while ((c = *fmt++) != 0)
	{
		if (c != '%')
		{
			*p++ = c;
			continue;
		}
		c = *fmt++;
		r = 0;
		switch (c)
		{
		case 'x':
			r += 6;
		case 'd':
			r += 2;
		case 'o':
			r += 8;
			v = va_arg(ap, int);
			strcpy(p, itoar(v, r));
			p += strlen(p);
			break;
		case 'X':
			r += 6;
		case 'D':
			r += 2;
		case 'O':
			r += 8;
			v = va_arg(ap, long);
			strcpy(p, itoar(v, r));
			p += strlen(p);
			break;
		case 's':
			strcpy(p, va_arg(ap, char *));
			p += strlen(p);
			break;
		default:
			*p++ = c;
			break;
		}
	}
	*p = '\0';
	va_end(ap);

	ttcol = HUGE;
	mlerase();
	offreversevideo();					/* to be sure!! */
	gputs(ms);
	mpresf = TRUE;
}


/* Display in the message line area the string pted by p.  p can == NULL,
which means display the old mlstr again. The new mesg actually gets
seen by the user because there is an update() happening soon after
calling this rtn. */

void mlmesg(uchar *p)
{
	if (p)
		strcpy(mlstr, p);
	mloff = (int)strlen(mlstr);
	mlerase();
}

/* As above, but with no erase.  We explicitly update() because otherwise
it does not get seen by the user. */

void mlmesgne(uchar *p)
{
	register int n;

	if (p)
		strcpy(mlstr, p);
	mloff = (int)strlen(mlstr);
	vscreen[term.t_nrow]->v_flag |= VFCHG;
	n = exitue;
	exitue = -1;
	update();
	exitue = n;
}
