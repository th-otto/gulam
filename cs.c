/*
 * $Header: f:/src/gulam\RCS\cs.c,v 1.1 1991/09/10 01:02:04 apratt Exp $ $Locker:  $
 * ======================================================================
 * $Log: cs.c,v $
 * Revision 1.1  1991/09/10  01:02:04  apratt
 * First CI of AKP
 *
 * Revision: 1.4 89.12.19.13.32.44 apratt 
 * Initial Revision
 * 
 * Revision: 1.3 89.06.16.17.47.36 apratt 
 * Header style change.
 * 
 * Revision: 1.2 89.02.22.14.42.18 Author: apratt
 * Changed userfeedback level of ES after (Read xx bytes in yy lines) to 2.
 * At verbosity 1, they won't appear (useful for mwc which starts gu which
 * reads gulam.g each time).
 * 
 * Revision: 1.1 88.06.03.15.38.22 Author: apratt
 * Initial Revision
 * 
 */

/*
	cs.c of gulam -- control structures

	copyright (c) 1987 pm@Case
*/

#define	WS1	1

#include "ue.h"

#define	BOT	0
#define	NEW	1
#define	BGNIF	2
#define	SKIPIF	3
#define	WHILE	4
#define	FOREACH	5

static uchar ZERO[] = "0";
static uchar ONE[] = "1";
static uchar *dummy;

typedef struct STE
{
	int op;
	int tf;
	int nn;
	LINE *lp;
	WS *wp;
	uchar *varp;						/* nn, wp, varp used only for FOREACH   */
	struct STE *next;
} STE;

#define	MXsnl		25					/* dflt depth of batch file nesting */

static LINE *curcmdlp;
static STE stbot = { BOT, TRUE, 0, NULL, NULL, NULL, NULL };

static STE *stp = &stbot;
static int snl = 0;
static int lpagain = 0;					/* 0, 1, or 2           */

static void stpush(int op, uchar *f)
{
	STE *s;

	state = (f ? atoi(f) : 0) != 0;
	s = (STE *) gmalloc(((uint) sizeof(STE)));
	if (s)
	{
		s->op = op;
		s->tf = state;
		s->nn = 0;
		s->lp = curcmdlp;
		s->varp = NULL;
		s->wp = NULL;
		s->next = stp;
		stp = s;
	}
}

/* pop one element (or give back the top).  stp is never == NULL. */

static STE *stpop(void)
{
	STE *s;

	s = stp;
	if (s != &stbot)
	{
		stp = s->next;
		if (s->wp)
			freews(s->wp);
		if (s->varp)
			gfree(s->varp);
		gfree(s);
	}
	state = stp->tf;
	return s;
}

static void stfree(void)
{
	int op;

	for (;;)
	{
		op = stpop()->op;
		if (op == NEW || op == BOT)
			break;
	}
}

int inbatchfile(void)
{
	return snl > 0;
}

void csexit(int n)
{
	lpagain = 2;
	valu = n;
}


void csexecbuf(BUFFER *bp)
{
	uchar *q;
	unsigned char i;
	LINE *lp;
	LINE *slp;
	int sfda[4];

	for (i = 0; i < 4; i++)
	{
		sfda[i] = fda[i];
		fda[i] = MINFH - 1;
	}
	slp = curcmdlp;
	snl++;
	stpush(NEW, (stp->tf ? ONE : ZERO));
	for (lp = lforw(bp->b_linep); lp != bp->b_linep;)
	{
		if (useraborted())
		{
			valu = -3;
			goto ret;
		}
		q = gstrdup(makelnstr(lp));
		if (varnum("batch_echo"))
			userfeedback(q, 0);
		curcmdlp = lp;
		lpagain = 0;
		processcmd(q, 0);				/* 0 ==> don't put cmd in history */
		if (lpagain == 2)
			goto ret;
		lp = (lpagain ? curcmdlp : lforw(curcmdlp));
	}
	valu = 0;

  ret:
	snl--;								/* stfree pops to the (NEW, NULL);  */
	stfree();							/* user's ^C may have brought us here */
	curcmdlp = slp;
	for (i = 0; i < 4; i++)
		fda[i] = sfda[i];
}


/* take commands from file given by p   */
int batch(uchar *p, uchar *cmdln, uchar *envp)
{
	uchar *q;
	BUFFER *bp;

	UNUSED(envp);
	valu = -1;
	if (p == NULL || *p == '\0' || flnotexists(p))
	{
		emsg = "shell file not found";
		return -33;
	}
	if (stackoverflown(512))
		return -1;
	if ((bp = opentempbuf(p)) == NULL)
	{
		emsg = "is shell file nesting too deep?";
		return -1;
	}
	/* do puts(CRLF) because of the (Read bb bytes in nn lines) mesg */
	userfeedback(ES, 2);
	/* cmdln[0] == length */
	q = (cmdln ? str3cat(p, " ", cmdln + 1) : NULL);
	pushws(lex(q, DELIMS, TKN2));
	gfree(q);

	csexecbuf(bp);

	closebuf(bp);
	popargws();
	return (int)valu;
}

/* Predicate on file name: c is in {e, f, d, h, v, m, r} */

uchar *fnmpred(uchar c, uchar *p)
{
	int a;
	DTA *dta;

	if ((p == NULL) || flnotexists(p))
		a = 0;
	else if (c == 'e')
		a = 1;
	else
	{
		dta = (DTA *) gfgetdta();
		a = dta->attr;
		if (c == 'd')
			a &= 0x10;					/* dir      */
		else if (c == 'f')
			a = !(a & 0x10);			/* ord file */
		else if (c == 'h')
			a &= 0x06;					/* hidden file  */
		else if (c == 'v')
			a &= 0x08;					/* volume   */
		else if (c == 'm')
			a &= 0x20;					/* archived */
		else
			a = 0;
	}
	return a ? ONE : ZERO;
}

/* atom ::= number | filename  | filepred | { exp } | ! atom */

static uchar *atom(uchar *p)
{
	uchar c;
	uchar *r;
	uchar *nextword;

	c = *p;
	if (c == '{')
	{
		r = csexp(lexgetword(), &nextword);
		if (*nextword != '}')
			emsg = "missing }";
	} else
	{
		if (('0' <= c) && (c <= '9'))
			r = p;
		else if (c == '-')
			r = fnmpred(p[1], lexgetword());
		else if (c == '!')
		{
			r = atom(lexgetword());
			r = (r && atoi(r) ? ZERO : ONE);
		} else
			r = p;
		r = gstrdup(r);
	}
	return r;
}

/* Compute the arithmetic expression [arithexp ::= exp using +-/%*]
Exp with no operators, ie an atom, is a special case because we want
the string form of the atom (eg as for "blah" in "set s blah").  */

static uchar *arithexp(uchar *p, uchar **nextword)
{
	uchar *q;
	uchar *r;
	long i;
	long nr;
	int niter;
	uchar op;

	niter = 0;
	q = NULL;
	i = 0;
	op = '+';
	*nextword = ES;
	while (p && *p)
	{
		r = atom(p);
		if (niter++ == 0)
			q = gstrdup(r);
		if (r)
		{
			nr = atoi(r);
			gfree(r);
		} else
		{
			nr = 0;
		}
		switch (op)
		{
		case '+':
			i += nr;
			break;
		case '-':
			i -= nr;
			break;
		case '*':
			i *= nr;
			break;
		case '/':
			i = (nr != 0 ? i / nr : 0x7FFF);
			break;
		case '%':
			i = (nr != 0 ? i % nr : 0x7FFF);
			break;
		}
		*nextword = lexgetword();
		op = **nextword;
		if (strchr("+-/*%", op) == NULL)
			break;						/* <=== */
		p = lexgetword();
	}
	return niter == 1 ? q : gstrdup(itoal(i));
}


static uchar *relaexp(uchar *p, uchar *op, uchar **nextword)
{
	uchar a;
	uchar b;
	int np;
	int nr;

	a = op[0];
	b = op[1];
	np = atoi(p);
	nr = atoi(arithexp(lexgetword(), nextword));
	if ((a == '<') && (b == '='))
		nr = (np <= nr);
	else if ((a == '<') && (b == '\0'))
		nr = (np < nr);
	else if ((a == '>') && (b == '='))
		nr = (np >= nr);
	else if ((a == '>') && (b == '\0'))
		nr = (np > nr);
	else if ((a == '=') && (b == '='))
		nr = (np == nr);
	else if ((a == '!') && (b == '='))
		nr = (np != nr);
	else
		nr = 0;
	return gstrdup(nr ? ONE : ZERO);
}

/** no short-circuit eval yet	**/
static uchar *andor(uchar *p, uchar *op, uchar **nextword)
{
	uchar a;
	uchar b;
	int np;
	int nr;

	a = op[0];
	b = op[1];
	np = atoi(p);
	nr = atoi(csexp(lexgetword(), nextword));
	if ((a == '&') && (b == '&'))
		nr = (np && nr);
	else if ((a == '|') && (b == '|'))
		nr = (np || nr);
	else
		nr = 0;
	return gstrdup(nr ? ONE : ZERO);
}

/* exp	::= aexp | aexp '||' exp | aexp && exp | aexp relop aexp */

uchar *csexp(uchar *p, uchar **nextword)
{
	uchar *r;
	uchar *q;

	r = arithexp(p, &q);
	switch (*q)
	{
	case '<':
	case '>':
	case '=':
		return relaexp(r, q, nextword);
	case '|':
	case '&':
		return andor(r, q, nextword);
	default:
		*nextword = q;
		break;
	}
	return r;
}

void csif(uchar *arg)
{
	uchar *q;

	UNUSED(arg);
	if (stp->tf)
		stpush(BGNIF, csexp(lexgetword(), &q));
	else
		stpush(SKIPIF, ZERO);
}

void cselse(uchar *arg)
{
	STE *s;
	int f;
	uchar *p;
	uchar *q;

	UNUSED(arg);
	s = stp;
	if (s->op == BGNIF)
	{
		f = s->tf;
		stpop();
		if (f)
			stpush(SKIPIF, ZERO);
		else
		{
			p = lexgetword();
			p = (*p ? csexp(p, &q) : ONE);
			stpush(BGNIF, p);
		}
	} else if (s->op != SKIPIF)
		emsg = "unexpected else";
}

void csendif(uchar *arg)
{
	UNUSED(arg);
	if (stp->op == BGNIF || stp->op == SKIPIF)
		stpop();
	else
		emsg = "extraneous endif";
}

void cswhile(uchar *arg)
{
	UNUSED(arg);
	stpush(WHILE, (stp->tf ? csexp(lexgetword(), &dummy) : ZERO));
}

void csendwhile(uchar *arg)
{
	UNUSED(arg);
	if (stp->op == WHILE)
	{
		if (stp->tf)
		{
			curcmdlp = stp->lp;
			lpagain = 1;
		}
		stpop();
	} else
	{
		emsg = "extraneous endwhile";
	}
}

void csforeach(uchar *arg)
{
	WS *ws;
	uchar *p;
	uchar *loopvar;
	uchar *q;

	UNUSED(arg);
	loopvar = lexgetword();
	p = lexgetword();
	if (*p != '{')
		emsg = "missing {";
	ws = initws();
	if (stp->tf)
	{
		p = lexgetword();				/* make a ws list of loop var values    */
		while (p)						/* p must never be NULL, but ...    */
		{
			if (*p == '}')
				break;
			if (*p == '\0')
			{
				emsg = "missing }";
				break;
			}
			strwcat(ws, csexp(p, &q), 1);
			p = q;
		}
		insertvar(loopvar, nthstr(ws->ps, 0));
	}
	stpush(FOREACH, (stp->tf ? ONE : ZERO));
	stp->varp = gstrdup(loopvar);
	stp->wp = ws;
	stp->nn = 1;
}

void csendfor(uchar *arg)
{
	uchar *p;
	STE *s;

	UNUSED(arg);
	s = stp;
	if (s->op == FOREACH)
	{
		if (s->tf && (p = nthstr(s->wp->ps, s->nn++)) != NULL && *p)
		{
			insertvar(s->varp, p);
			curcmdlp = s->lp;
		} else
			stpop();
	} else
		emsg = "extraneous endfor";
}
