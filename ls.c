/*
 * $Header: f:/src/gulam\RCS\ls.c,v 1.1 1991/09/10 01:02:04 apratt Exp $ $Locker:  $
 * ======================================================================
 * $Log: ls.c,v $
 * Revision 1.1  1991/09/10  01:02:04  apratt
 * First CI of AKP
 *
 * Revision: 1.6 89.12.27.12.39.22 apratt 
 * Oops.  tmstampcmp fix works now.
 * 
 * 
 * Revision: 1.5 89.12.27.11.18.56 apratt 
 * Re-fixed tmstampcmp -- making the result unsigned doesn't help when
 * the function's type is signed.  Instead, explicitly return 1, 0, or -1.
 * 
 * Revision: 1.4 89.06.16.17.23.54 apratt 
 * Header style change.
 * 
 * Revision: 1.3 89.02.01.18.13.30 Author: apratt
 * Added calls to tooold and yearstr for showing year rather
 * than time when a file is tooold() (twelve months).
 * 
 * Revision: 1.2 89.02.01.14.45.00 Author: apratt
 * Made 'i' unsigned in proc comparing dates for ls -t.
 * 
 * Revision: 1.1 88.06.03.15.38.36 Author: apratt
 * Initial Revision
 * 
 */

/*
	ls.c of gulam -- file names lister			 7/22/86

	copyright (c) 1986 pm@Case

*/

#include "ue.h"

typedef struct
{
	uchar *nmptr;
	GSTAT *statp;
} P2;

typedef GA P2GA;
typedef GA GSTATGA;

typedef struct
{
	WS *wlsws;
	GSTATGA *wlsga;
} WLSR;


static void recursels (int lsflag, uchar *p);

static TBLE *wtbl;						/* table of wls-entries     */
static int lrecurse;						/* levels of recursion      */
static int reversesort;					/* 1 => reverse order sort ls   */
static int curattr						/* attr for the files in readdir */
	= 0x01 | 0x10 | 0x20;

static void sizestr(long i, char *s)
{
	register char *p,
	*q;
	register int k;

	p = itoal(i);
	k = (int)strlen(p);
	for (q = s + 8 - k; s < q;)
		*s++ = ' ';
	while ((*s++ = *p++) != 0)
		;
	*--s = ' ';
}

static int fnmcompare(const void *_p, const void *_q)
{
	const P2 *p = _p;
	const P2 *q = _q;
	register int i;

	i = strcmp(p->nmptr, q->nmptr);
	if (reversesort)
		i = -i;
	return i;
}

/* AKP: time is unsigned, but the return value of this fn is signed. */
/* So we explicitly return 1, 0, or -1. */

static int tmstampcmp(const void *_p, const void *_q)
{
	const P2 *p = _p;
	const P2 *q = _q;
	register unsigned dp,
	 dq;
	register int i;

	dq = q->statp->date;
	dp = p->statp->date;
	if (dq == dp)
	{
		dq = q->statp->time;
		dp = p->statp->time;
	}
	if (dq > dp)
		i = 1;
	else if (dq == dp)
		i = 0;
	else
		i = -1;
	if (reversesort)
		i = -i;
	return i;
}

/* Sort the stringlets given in gs (then free gs).  Return the new
list in frs; if wa was given, construct the new one, free wa and
return the new one in fwa.  */

static void sortls(int style, int nc, uchar *gs, GSTATGA *wa, uchar **frs, GSTATGA **fwa)
{
	int nf;
	int offset;
	int (*pf) (const void *, const void *);
	uchar *tailstr,
	*new;
	register uchar *p,
	*q;
	register P2GA *ptga;
	P2 tmpp;
	GSTATGA *nsa;

	if (style == 1)
	{
		offset = FNMPOS;
		tailstr = CRLF;
	} else
	{
		offset = 0;
		tailstr = ES;
	}

	{									/* construct P2 array from gs */
		register GSTAT *lda;

		ptga = (P2GA *) initga(((uint) sizeof(P2)), 100);
		lda = (GSTAT *) (wa ? wa->el : NULL);
		if (gs && ptga)
			for (p = gs; *p; p += strlen(p) + 1)
			{
				p += offset;
				tmpp.nmptr = p;
				tmpp.statp = lda;
				if (lda)
					lda++;
				ptga = addelga(ptga, &tmpp);
			}
		nf = (ptga ? ptga->ne : 0);
	}
	pf = fnmcompare;
	reversesort = 0;
	if (style > 0)
	{
		reversesort = negopts['r'];
		if (negopts['t'])
			pf = tmstampcmp;
	}
	qsort(ptga->el, (size_t) nf, sizeof(P2), pf);
	{									/* re build strg */
		register P2 *pp;
		register int m,
		 k;

		m = (int)strlen(tailstr);
		new = p = gmalloc(1 + nc + nf * m);
		if (p == NULL)
			return;
		nsa = (GSTATGA *) (fwa ? initga(((uint) sizeof(GSTAT)), nf) : NULL);
		for (pp = (P2 *) ptga->el; nf--; pp++)
		{
			q = pp->nmptr - offset;
			k = (int)strlen(q) + 1;
			cpymem(p, q, k);
			p += k;
			if (m)
			{
				cpymem(--p, tailstr, m);
				p += m;
			}
			nsa = addelga(nsa, pp->statp);
		}
		*p = '\0';
	}
	gfree(ptga);
	if (new)
		gfree(gs);
	else
	{
		new = gs;
		valu = ENSMEM;
	}
	*frs = new;
	gfree(wa);
	if (fwa)
		*fwa = nsa;
	else
		gfree(nsa);
}

/* (Local) workhorse ls: read the current dir, and store the info in a
WLSR record.  The entries in WLSR are sorted by file name. */

static WLSR *lwlswr(void)
{
	register char *p;
	register WS *ws;
	register DTA *dta;
	register WLSR *wr;
	GSTATGA *wa;

	wr = (WLSR *) gmalloc(((uint) sizeof(WLSR)));
	ws = initws();
	if (wr == NULL || ws == NULL)
		return NULL;
	wa = initga(((uint) sizeof(GSTAT)), 10);

	dta = (DTA *) gfgetdta();
	p = dta->name;
	if (opencwdandread(curattr))
		do
		{								/*   *p == '\0'  *is* possible  */
			if (*p && (*p != '.' || (p[1] && p[1] != '.')))
			{
				strwcat(ws, lowercase(p), 1);
				wa = addelga(wa, (char *) dta + bgnGSTAT);
			}
		} while (readfromcwd());
	closecwd();
	if (ws->ns == 0)
		emsg = "(directory is empty)";
	sortls(0, ws->nc, ws->ps, wa, &ws->ps, &wa);
	wr->wlsws = ws;
	wr->wlsga = wa;
	return wr;
}

static TBLE *freetble(TBLE *p)
{
	register TBLE *q;
	register WLSR *wr;

	q = p->next;
	gfree(p->key);
	if ((wr = (WLSR *) p->elm) != NULL)
	{
		freews(wr->wlsws);
		gfree(wr->wlsga);
		gfree(wr);
	}
	gfree(p);
	return q;
}

static void freewlsr(char *k)
{
	register TBLE *p,
	*q;

	if (k == NULL || (p = wtbl) == NULL)
		return;
	do
	{
		q = p;
		p = p->next;
	} while (p && (strcmp(p->key, k)) != 0);
	if (p)
		q->next = freetble(p);
}

void freewtbl(void)								/* exported */
{
	register TBLE *p;

	if ((p = wtbl) == NULL)
		return;
	for (p = p->next; p;)
		p = freetble(p);
	gfree(wtbl);
	wtbl = NULL;
}

/* Get a WSPS format listing of current dir; flag==0 => update the wls
tbl.  The result is returned via the all-purpose global strg.  To update
the wls tbl, we just invoke lwlswr() and store the obtained record and
its corresp dir into the tbl.  */

/* 0 => don't use wlstbl; 1 => do */
void wls(int flag)
{
	register char *p;
	register WLSR *wr;
	register TBLE *wp;
	register int n;

	if (wtbl == NULL)
		wtbl = tblcreate();
	p = gfgetcwd();
	if ((wp = tblfind(wtbl, p)) != NULL)
	{
		if (flag)
		{
			wr = (WLSR *) wp->elm;
			gfree(p);
			goto mkstrg;
		}
		freewlsr(p);
	}
	wr = lwlswr();
	tblinsert(wtbl, p, (void *)wr);

  mkstrg:strg = NULL;
	if (wr)
	{
		strg = gmalloc(n = wr->wlsws->nc + 1);
		if (strg)
			cpymem(strg, wr->wlsws->ps, n);
	}
}

void fnmtbl(uchar *arg)								/* show wls tbl; if arg present delete corresp entry */
{
	register char *p;
	register TBLE *wp;
	register WLSR *wr;
	register int nn;

	UNUSED(arg);
	if (wtbl == NULL)
		return;
	p = lexgetword();
	if (*p == '-')
		freewtbl();
	else if (*p)
		freewlsr(p);
	else
		for (wp = wtbl->next; wp; wp = wp->next)
		{
			wr = (WLSR *) wp->elm;
			nn = wr->wlsws->nc;
			if (wr->wlsga)
				nn += (wr->wlsga->sz) * (wr->wlsga->na);
			outstr(sprintp("%d\t%s", nn, wp->key));
		}
}

/*  'Remove' the trailing back slashes by forcing the first in a series
of trailing bslashes to \0; return a ptr to this char.  If no trailing
bslashes, return NULL. */

static uchar *rmbslashtl(uchar *p)
{
	register uchar *q;

	q = p + strlen(p) - 1;
	if (q < p || *q != DSC)
		q = NULL;
	else
	{
		while (q > p && q[-1] == DSC)
			q--;
		*q = '\0';
	}
	return q;
}

/* Look up the DTA for the filename pp.  pp may contain trailing
backslashes, so take care!  */

static DTA *wlsdta(uchar *pp)
{
	register uchar *p,
	*q,
	*qq;
	register TBLE *wp;
	register GSTATGA *ga;
	register WLSR *wr;
	register DTA *dta;
	register uchar *pbs;
	register int n;


	dta = NULL;
	pbs = rmbslashtl(pp);
	q = qq = stdpathname(pp);
	if (q == NULL)
		goto ret;

	if ((p = strrchr(q, DSC)) != NULL)
	{
		q = gstrdup(p + 1);
		if (p[-1] == ':')
			p++;						/* e.g., we need a:\ not a: */
		*p = '\0';
		p = qq;
		if (q && *q == '\0')
			goto isdrive;
	} else
		goto freepq;
	/* now p == parent dir; q == leaf name; p or q == qq */
	if ((wp = tblfind(wtbl, p)) != NULL)
	{
		wr = (WLSR *) wp->elm;
		n = findstr(wr->wlsws->ps, q);
		if (n < 0)
			goto freepq;
		ga = wr->wlsga;					/* -bgnGSTAT so that this ptr is a DTA*  */
		qq = ga->el + n * (ga->sz) - bgnGSTAT;
		dta = (DTA *) qq;
		goto freepq;
	}

  isdrive:
	pp = stdpathname(pp);
	if (flnotexists(pp))
	{
		if ((strlen(pp) <= 3)			/* if (specialdir(pp)) */
			&& (pp[1] == ':') && ((pp[2] == DSC) || (pp[2] == '\0')) && (strchr(drvmap(), pp[0]) != NULL))
		{
			dta = (DTA *) gfgetdta();
			dta->attr = 0x10;			/* can, eg., be "a:\" */
			dta->date = dta->time = 0;
			dta->size = 0;
		}
	} else
		dta = (DTA *) gfgetdta();
	gfree(pp);
  freepq:gfree(p);
	gfree(q);
  ret:if (pbs)
		*pbs = DSC;
	return dta;
}

int isdir(char *p)							/* determine if p is pathname of a dir      */
{
	register DTA *dta;

	dta = wlsdta(p);
	return (dta ? (filetp(p, dta->attr) == DSC) : 0);
}

/* format of ls -l line:
 0         1         2         3         4         5
 012345678901234567890123456789012345678901234567890123456 7 8
 drwx---... 1 useridxxx size0000 mon dd hh:mm filename.ext\r\n\0
*/

#if	00									/* #defined in sysdepend.h */
FNMPOS 45								/* file name position in ls -l bgns here */
#endif
/* Format the fnms in a into 'ls -l'	*/
static void lsl(char *p)
{
	register WS *ws;
	register DTA *dta;
	register int nc,
	 fstyle,
	 shouldsort;
	GSTATGA *ga;
	char temp[5],
	*pp;
	char lsln[FNMPOS + 3];				/* timestr() needs 8 bytes */


	strg = pp = NULL;
	ws = initws();
	if (ws == NULL)
		return;
	if (*p == '\0')
	{
		wls(0);
		p = pp = strg;
	}
	ga = NULL;
	shouldsort = negopts['l'] + negopts['r'];
	if (negopts['t'])
	{
		ga = (GSTATGA *) initga(((uint) sizeof(GSTAT)), 10);
		shouldsort++;
	}
	for (fstyle = negopts['F']; *p; p += strlen(p) + 1)
	{
		dta = wlsdta(p);
		if (dta == NULL)
			continue;
		ga = addelga(ga, (char *)dta + bgnGSTAT);
		attrstr(dta->attr, lsln);
		userid(dta->user, &lsln[13]);
		sizestr(dta->size, &lsln[23]);
		datestr(dta->date, &lsln[32]);
		/* if date older than some amount, write year, not time AKP */
		if (tooold(dta->date))
			yearstr(dta->date, &lsln[39]);
		else
			timestr(dta->time, &lsln[39]);
		lsln[FNMPOS - 1] = ' ';
		lsln[FNMPOS] = '\0';			/* dta->name == leaf name only */

		strwcat(ws, lsln, 0);
		strwcat(ws, p, 0);

		nc = 0;
		if (fstyle)
			temp[nc++] = filetp(p, dta->attr);
		if (shouldsort == 0)
		{
			temp[nc++] = '\r';
			temp[nc++] = '\n';
		}
		temp[nc++] = '\0';
		strwcat(ws, temp, shouldsort);
	}
	gfree(pp);
	p = ws->ps;
	nc = ws->nc;
	gfree(ws);							/* not freews() */
	if (shouldsort == 0)
		strg = p;
	else
		sortls(1, nc, p, ga, &strg, NULL);	/* p and ga are gfree'd */
	if ((lrecurse-- > 0) && strg)
	{
		recursels(1, p = gstrdup(strg));
		gfree(p);
	}
}

/* Format the fnms in a into 'ls' list.  lss() uses the fact that
lex.c  has its words stored in WSPS format. */

static void lss(uchar *p)
{
	register uchar *q,
	*r;
	register WS *ws;
	register int argsgiven,
	 parentexp,
	 flagf;
	GSTATGA *ga;
	DTA *d;
	uchar fi[2];

	if (*p)
		argsgiven = 1;
	else
	{
		wls(0);
		p = strg;
		argsgiven = 0;
	}
	parentexp = varnum(Parentexp);
	flagf = negopts['F'];
	if (argsgiven || flagf || negopts['t'] || negopts['r'])
	{
		ws = initws();
		if (ws == NULL)
			return;
		ga = (negopts['t'] ? (GSTATGA *) initga(((uint) sizeof(GSTAT)), 10) : NULL);
		for (q = p, fi[0] = fi[1] = '\0'; *q; q += strlen(q) + 1)
		{
			r = (argsgiven && parentexp ? stdpathname(q) : q);
			if ((d = wlsdta(r)) != NULL)
			{
				strwcat(ws, r, 0);
				if (flagf)
				{
					fi[0] = filetp(q, d->attr);
					if (fi[0] == ' ')
						fi[0] = '\0';
				}
				strwcat(ws, fi, 1);
				ga = addelga(ga, (char *)d + bgnGSTAT);
			}
			if (r != q)
				gfree(r);
		}
		if (argsgiven == 0)
			gfree(p);
		sortls(2, ws->nc, ws->ps, ga, &ws->ps, NULL);
		p = ws->ps;
		gfree(ws);
	}
	strg = pscolumnize(p, -1, 0);
	if (lrecurse-- > 0)
		recursels(0, p);
	gfree(p);
}


static void recursels(int lsflag, uchar *p)
{
	register uchar *q,
	*pp,
	*cwd,
	*r;
	register int d;

	if (p == NULL)
		return;
	for (q = p;;)
	{
		outstrg();
		if (*q == '\0' || useraborted())
			break;						/* <=== */
		if (lsflag)
		{
			q += FNMPOS;
			pp = strchr(q, '\r');
			if (pp)
				*pp = '\0';
			else
				break;
		}
		d = isdir(q);
		if (d && strcmp(q, ".") != 0 && strcmp(q, "..") != 0)
		{
			emsg = NULL;
			cwd = gfgetcwd();
			cd(q);
			if (valu == 0 && emsg == NULL)
			{
				r = gfgetcwd();
				outstr(sprintp("\r\n%s:\r", r));
				gfree(r);
				if (lsflag)
					lsl(ES);
				else
					lss(ES);
			}
			cd(cwd);
			gfree(cwd);
		}
		if (lsflag)
		{
			*pp = '\r';
			q = pp + 2;
		} else
			q += strlen(q) + 1;
	}
}


void ls(uchar *arg)
{
	register char *p;

	UNUSED(arg);
	p = lexgetword();
	lrecurse = *p ? 1 : 0;
	if (negopts['R'])
		lrecurse = 0x7FFF;
	if (negopts['a'])
		curattr = 0xFF;
	if (negopts['l'] || negopts['L'])
		lsl(p);
	else
		lss(p);
	curattr = 0x01 | 0x10 | 0x20;
}
