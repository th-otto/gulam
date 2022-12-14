/*
	ue.c of Gulam -- builtin microEmacs

This file is a revised version of the public domain one written by
Dave G.  Conroy.  It contains the main driving routine of uE, and some
keyboard processing code, for the microEmacs screen editor.

Revisions by: prabhaker mateti, sep-86, made it part of gulam;
prabhaker mateti, 3-Jan-86, ported to Atari 520ST Steve Wilhite,
30-Nov-85

*/

#include        "ue.h"
#include	"keynames.h"

static uchar msginit[] = "\xe6\xee as adapted by tho  2022/12/02";		/* my ego!			*/

int thisflag;							/* Flags, this command          */
int lastflag;							/* Flags, last command          */
KEY lastkey;

int exitue = 2;							/* == 2    => uebody is not active      */

			/* == 3    => tried to enter, but fatal err */
			/* == 1,4  => uebody active, but about to leave */
			/* == 0    => within enter() --> uebody()   */
			/* == -1   => uebody active, via >mini<     */

static int nctlxe;						/* arg of ctlxe         */
static KEY kbdm[NKBDM];				/* kbd macro area               */
static KEY *kbdmip;					/* Input  for above             */
static BUFFER *fgbp;
static int rdonly;

static void enter(void)
{
	refresh(FALSE, 1);
	exitue = 0;
	setvarnum(Verbosity, varnum(Verbosity) + 1);
	uebody();
	if (exitue == 4)
	{
		vttidy();
		mlmesg(ES);
		fgbp = curbp;
		mlwrite(Tempexit);
	} else
	{
		fgbp = NULL;
		ueexit();
	}
	exitue = 2;
	setvarnum(Verbosity, varnum(Verbosity) - 1);
}


void fg(uchar *arg)
{
	UNUSED(arg);
	if (fgbp == NULL)
		return;
	switchwindow(wheadp);
	switchbuffer(fgbp);
	mlmesg(ES);
	enter();
}


void ue(uchar *arg)
{
	int n;								/* &n is used below */
	uchar *p;

	UNUSED(arg);
	mlmesg(msginit);
	togulambuf(TRUE, 1);

	for (;;)
	{
		p = lexgetword();
		if (*p == '\0')
			break;
		if (*p == '-')
			rdonly++;
		else
		{
			n = flvisit(p);
			if (rdonly)
				curbp->b_flag |= BFRDO;
			if (n != TRUE)
				break;
		}
	}
	enter();
}


void moreue(uchar *arg)
{
	rdonly = 1;
	ue(arg);
}


void outofroom(void)
{
	gputs("gulam's ueinit() ran out of room\r\n");
	exit(-1);
}

/* This is the general command execution routine.  It handles the fake
binding of all the keys to "self-insert".  It also clears out the
"thisflag" word, and arranges to move it to the "lastflag", so that
the next command can look at it.  Return the status of command.  */

static int execute(int f, int n, KEY c)
{
	KB *ktp;
	int status;
	KEY kc;

	lastkey = c;
	for (ktp = kba[curbp->b_kbn]; (kc = ktp->k_code) != KEOTBL; ktp++)
	{
		if (kc == c)
		{
			thisflag = 0;
			status = (*(fpfs[ktp->k_fx])) (f, n);
			lastflag = thisflag;
			goto ret;
		}
	}

	if (Selfinserting(c))
	{
		if (n <= 0)						/* Fenceposts.          */
		{
			lastflag = 0;
			status = (n < 0 ? FALSE : TRUE);
			goto ret;
		}
		thisflag = 0;					/* For the future.      */
		status = linsert(n, c);
		lastflag = thisflag;
		goto ret;
	}
	lastflag = 0;						/* Fake last flags.     */
	status = FALSE;
  ret:
	if (status != TRUE)
		nctlxe = 0;
	return status;
}

/* Normally bound to ^U	*/

int getarg(int f, int n)
{
	int c;
	int mflag;

	UNUSED(f);
	n = 4;								/* with argument of 4 */
	mflag = 0;							/* that can be discarded. */
	mlwrite("Arg: 4");
	while (((c = getctlkey()) >= '0' && c <= '9') || c == (CTRL | 'U') || c == '-')
	{
		if (c == (CTRL | 'U'))
			n *= 4;
		else
			/* If dash, and start of argument string, set arg to -1.  */
			/* Otherwise, insert it.  */
		if (c == '-')
		{
			if (mflag)
				break;
			n = 0;
			mflag = -1;
		}
		/* If first digit entered, replace previous argument */
		/* with digit and set sign.  Otherwise, append to arg. */
		else
		{
			if (!mflag)
			{
				n = 0;
				mflag = 1;
			}
			n = 10 * n + c - '0';
		}
		mlwrite("Arg: %d", (mflag >= 0) ? n : (n ? -n : -1));
	}
	/* Make arguments preceded by a minus sign negative and change */
	/* the special argument "^U -" to an effective "^U -1". */
	if (mflag == -1)
	{
		if (n == 0)
			n++;
		n = -n;
	}
	return execute(TRUE, n, c);
}

/* Initialize all of the buffers and windows.  The buffer name is
passed down as an argument, because the main routine may have been
told to read in a file by default, and we want the buffer name to be
right.  */

void ueinit(void)
{
	vtinit();
	kbddisplayinit();
	keysetup();
	bufinit();
	wininit();
	rdonly = 0;
	exitue = 2;
}


void ueexit(void)
{
	update();							/* so the modeline doesn't have ** */
	kdelete();
	uefreeall();						/* in display.c */
	ueinit();
	tominibuf();
	vttidy();
	sgarbf = FALSE;
}

/* body of microEmacs: rearranged from original	*/

void uebody(void)
{
	int c;

	mousecursor();
	lastflag = 0;						/* Fake last flags.     */
	while (exitue <= 0)
	{
		if (nctlxe <= 0)
			update();					/* Fix up the screen    */
		c = getctlkey();
		if (mpresf != FALSE)
		{
			mlerase();
			update();
			if (c == ' ')
				continue;				/* ITS EMACS does this  */
		}
		execute(FALSE, 1, c);
	}
	mouseregular();
}


static KEY ueinkey(void)
{
	KEY i;

	i = inkey();						/* see util.c */
	if (kbdmip)
	{
		if (kbdmip > &kbdm[NKBDM - 4])
		{
			*kbdmip++ = '\030';
			*kbdmip++ = ')';
			*kbdmip = '\0';
			kbdmip = NULL;
			mlmesg("[kbd macro forced to end; no more room]");
		} else
			*kbdmip++ = i;		/* room for ^X, rt-paren,\0 */
	}
	return i;
}


/* Get a key.  Apply control modifications to the read key. */

static KEY getctl(void)
{
	KEY c;

	c = ueinkey();
	if (c >= 'a' && c <= 'z')
		c -= 0x20;						/* Force to upper   */
	if (/* c >= 0x00 && */ c <= 0x1F)
		c = CTRL | (c + '@');			/* C0 control -> C- */
	return c;
}


KEY getkey(void)
{
	KEY c;

	c = ueinkey();
	if (c == METACH)
		c = META | getctl();
	else if (c == CCHR('X'))
		c = CTLX | getctl();
	else if (/* c >= 0x00 && */ c <= 0x1F)
		c = CTRL | (c + '@');
	return c;
}


KEY getctlkey(void)
{
	KEY c;

	c = ueinkey();
	if (/* c >= 0x00 && */ c <= 0x1F)
		c = CTRL | (c + '@');
	return c;
}

/* Got META/^X.  Now get next key and execute it. */

int metanext(int f, int n)
{
	return execute(f, n, META | getctl());
}


int ctlxnext(int f, int n)
{
	return execute(f, n, CTLX | getctl());
}


/* Begin a keyboard macro.  Error if not at the top level in keyboard
processing.  Set up variables and return.  */

int ctlxlp(int f, int n)
{
	UNUSED(f);
	UNUSED(n);
	if (kbdmip != NULL || nctlxe > 0)
	{
		mlwrite("sorry, cannot nest key board macros");
		return FALSE;
	}
	mlwrite("[Start macro]");
	kbdmip = &kbdm[0];
	return TRUE;
}


/* End keyboard macro.  This is invoked not only at the end of
defining the kbd macro, but also at the end of each execution of the
macro.  */

int ctlxrp(int f, int n)
{
	UNUSED(f);
	UNUSED(n);
	if (nctlxe > 1)
	{
		storekeys(kbdm);
		nctlxe--;
	} else
		nctlxe = 0;
	if (kbdmip)
	{
		mlwrite("[End macro]");
		*kbdmip = '\0';
	}
	kbdmip = NULL;
	return TRUE;
}

int showkbdmacro(int f, int n)
{
	UNUSED(f);
	UNUSED(n);
	outstr(sprintp("kbd macro is :%s:", kbdm));
	return TRUE;
}

/* Execute a macro.  The command argument is the number of times to
loop.  Quit as soon as a command gets an error.  Return TRUE if all
ok, else FALSE.  */


int ctlxe(int f, int n)
{
	UNUSED(f);
	if (kbdmip != NULL || nctlxe > 0)
	{
		mlwrite("sorry, cannot do recursive kbd macro!");
		return FALSE;
	}
	if (n <= 0)
		return TRUE;
	if (kbdm[0])
	{
		nctlxe = n;
		storekeys(kbdm);
	}
	return TRUE;
}


/* Abort.  Beep the beeper.  Kill off any keyboard macro, etc., that
is in progress.  Sometimes called as a routine, to do general aborting
of stuff.  */

int ctrlg(int f, int n)
{
	UNUSED(f);
	UNUSED(n);
	mlwrite("^G...ok!");
	if (kbdmip)
	{
		kbdm[0] = '\0';
		kbdmip = NULL;
	}
	nctlxe = 0;
	return ABORT;
}


/* temporary exit from ue to gulam; usr expects to return to ue */
int tempexit(int f, int n)
{
	UNUSED(f);
	UNUSED(n);
	exitue = 4;
	return TRUE;
}


/* Fancy quit command, as implemented by Norm.  If the current buffer
has changed do a write current buffer and exit emacs, otherwise simply
exit.  */

int quickexit(int f, int n)
{
	if ((curbp->b_flag & BFCHG) != 0	/* Changed.             */
		&& (curbp->b_flag & BFTEMP) == 0)	/* Real.                */
		filesave(FALSE, 1);
	return quit(f, n);						/* conditionally quit   */
}

/* Quit command.  If an argument, always quit.  Otherwise confirm if a
buffer has been changed and not written out.  Normally bound to "C-X
C-C".  Look through the list of buffers, giving the user a chance to
save them.  Return FALSE if there are any changed buffers afterwards,
or the user does not wish to quit after all.  Buffers that don't have
an associated file don't count.  Return TRUE if there are no changed
buffers, or user insists on quitting.  */

int quit(int f, int n)
{
	int s;

	UNUSED(n);
	if (f != FALSE || anycb() == FALSE)
		goto doquit;

	s = savebuffers(FALSE, 1);
	if (s != TRUE)
	{
		s = mlyesno("Unsaved buffers exist! Quit? ");
		if (s != TRUE)
			return s;
	}
  doquit:
	exitue = 2;
	return TRUE;
}
