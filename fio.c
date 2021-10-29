/*
 * $Header: f:/src/gulam\RCS\fio.c,v 1.1 1991/09/10 01:02:04 apratt Exp $ $Locker:  $
 * ======================================================================
 * $Log: fio.c,v $
 * Revision 1.1  1991/09/10  01:02:04  apratt
 * First CI of AKP
 *
 * Revision: 1.3 89.06.16.17.23.26 apratt 
 * Header style change.
 * 
 * Revision: 1.2 89.02.22.14.42.38 Author: apratt
 * Changed userfeedback level of (Reading...) and (Read...) messages to 2.
 * At verbosity 1, they won't appear (useful for mwc which starts gu which
 * reads gulam.g each time).
 * 
 * Revision: 1.1 88.06.03.15.38.26 Author: apratt
 * Initial Revision
 * 
 */

/*			
	fio.c of Gulam/uE					April 1986

	copyright (c) 1987 pm@cwru.edu

The routines in this file read and write ASCII files from the disk.
All of the knowledge about files are here.  A better message writing
scheme should be used.  This is a complete rewrite, by pm@cwru.edu, of
what was in microEmacs.

*/

#include        "ue.h"

int lnn;								/* line number          */
int evalu;
long ntotal;

static int fd;							/* File descriptor, all functions. */
static int rwflag;						/* 0=read or 1=write flag */
static long x;
static long n;
static long lsz;
static uchar *txtp;
static uchar TWD[] = "that was a directory!";

/* Open a file for writing.  Return FIOSUC if all is well, and FIOERR
on errors.  */

int ffwopen(char *fn)
{
	if (isdir(fn))
	{
		userfeedback(TWD, -1);
		return FIOERR;
	}
	if ((fd = (int)gfcreate(fn, 0)) < 0)
	{
		userfeedback("Cannot open file for writing", -1);
		return FIOERR;
	}

	/* we try to allocate a large buffer for read/write */
	lsz = 0x0FFFFFFFL;
	txtp = maxalloc(&lsz);
	x = n = 0L;
	rwflag = 1;
	userfeedback(sprintp("(Writing %s ...)", fn), 1);
	return FIOSUC;
}


static int twrite(void)
{
	if (gfwrite(fd, txtp, x) == x)
	{
		x = 0;
		return FIOSUC;
	}
	userfeedback("Write I/O error", -1);
	return FIOERR;
}

/* Close a file. Should look at the status in all systems. */

int ffclose(void)
{
	int r;

	r = FIOSUC;
	if ((rwflag == 1) && (x < lsz))
		r = twrite();
	gfclose(fd);						/* check for errors ??  */
	maxfree(txtp);
	return r;
}

/* Write a line to the already opened file.  The "buf" points to the
buffer, and the "nbuf" is its length, less the free newline.  Return
the status.  Check only at the newline.  */

int ffputline(char *buf, int nb)
{
	int i;
	char *p;

	p = buf;
  xxxx:
	while (nb > 0)
	{
		if (x + nb > lsz && twrite() == FIOERR)
			return FIOERR;
		i = (int)(x + nb <= lsz ? nb : lsz - x);
		cpymem(txtp + x, p, i);
		p += i;
		nb -= i;
		x += i;
	}
	if (p != &DFLNSEP[LDFLNSEP])
	{
		p = DFLNSEP;
		nb = LDFLNSEP;
		goto xxxx;
	}
	return FIOSUC;
}

/* Open a file for reading, and apply func to each of its lines.  If
file not found return.  */

int frdapply(char *fnm, void (*fn)(uchar *q, int n))
{
	if (isdir(fnm))
	{
		userfeedback(TWD, -1);
		return FIOERR;
	}
	if ((fd = (int)gfopen(fnm, 0)) < 0)
		return FIOFNF;
	userfeedback(sprintp("(Reading %s ...)", fnm), 2);
	lnn = evalu = 0;
	ntotal = 0;
	eachline(fd, fn);					/* Fcloses(fd) also */

	if (evalu < 0)
		emsg = "File read error";
	if (emsg)
	{
		mlmesg(emsg);
		return FIOERR;
	}
	userfeedback(sprintp("(Read %s, %D bytes in %d lines)", fnm, ntotal, lnn), 2);
	return FIOSUC;
}
