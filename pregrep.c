/*
	pregrep.c of gulam -- print, lpr, and grep functions	aug 24, 86

	copyright (c) 1986 pm@Case
*/

/* 890111 kbad	fixed bug in "prout" which would cause an extra header to
*		print(without formfeed) after the last page of a document.
*/

#include "ue.h"
#include "regexp.h"

static regexp *re;
static int plnn;							/* line number          */
static char *fnm;						/* name of current file     */
static int lfnm;

#if	LPRINT

#define	LPPG	55						/* default lines per page   */

static int nerr;						/* #errors          */
static int lppg;						/* lines per page       */

static char *hdr;						/* top of page hdr      */
static char *pgp;						/* pos of pageno in hdr     */
static long page;						/* # current page       */
static int lprf;						/* 1 iff lpr; 0 iff print   */

static void header(void);


/* printer initialization strings   */
/* the one below is for epson mx80      */
/* "\033\D\010\020\030\040\050\060\070\100\200\"	set tabs	*/
/* "\033\C\102"					form length = 66	*/

/* ouput to PRN:    */
static void prout(char *p, int n)
{
	register char c;

	if (n < 0)
		return;							/* jstncs   */
	while (n--)
	{
		c = *p++;
		do
		{
			if (useraborted())
			{
				valu = -1;
				nerr++;
				return;
			}
		} while (printstatus() == 0);
		printout(c);
		if (c == '\014' && n)
			header();
	}
}


static void prvar(char *p, char *q)
{
	p = varstr(p);
	if (p == NULL || *p == '\0')
		p = q;
	prout(p, ((int) strlen(p)));
}


static void header(void)
{
	plnn = 1;
	page++;
	if (hdr)
	{
		strcpy(pgp, itoal(page));
		strcat(pgp, "\r\n\n\n");
		prout(hdr, ((int) strlen(hdr)));
		/* 1st page doesn't have the FF\n, so
		 * we must generate the \n.
		 */
		if (page == 1)
			prvar("pr_eol", CRLF);
	}
}



static void prinit(void)							/* init before beginning to print/lpr */
{
	register char *p;

	nerr = 0;
	page = 0;
	hdr = NULL;
	lppg = varnum("pr_lpp");
	if (lppg <= 0)
		lppg = LPPG;
	prvar("pr_bof", ES);

	if (!lprf)
	{
		gsdatetime(ES);					/* strg has date-time string */
		p = str3cat(strg, "  ", fnm);
		hdr = str3cat(p, "  Page ", "0123456789\r\n\n\n");
		gfree(p);
		if (hdr)
			pgp = strrchr(hdr, '0');
		gfree(strg);
		strg = NULL;
	}
	header();
}


static void prline(char *q, int n)
{
	if (nerr)
		return;

	/* FF followed by other chars signals end of page.
	 * Header is sent by prout(), because we want an embedded FF
	 * in a document to generate a header.
	 */
	if ((plnn++ % lppg) == 0)
		prvar("pr_eop", "\014\n");
	prout(q, n);
	prvar("pr_eol", CRLF);
}


/* "print" one file */
static void lprint(uchar *pnm)
{
	register int fd;

	fd = (int)gfopen(pnm, 0);
	if (fd < 0)
		return;

	fnm = pnm;
	lfnm = (int)strlen(fnm);
	prinit();
	eachline(fd, prline);

	/* FF alone signals end of file, and will not generate
	 * a header in prout() after the last page.
	 */
	prvar("pr_eof", "\014");

	if (hdr)
		gfree(hdr);
}


void print(uchar *arg)
{
	UNUSED(arg);
	lprf = 0;
	doforeach("print", lprint);
}


void lpr(uchar *arg)
{
	UNUSED(arg);
	lprf = 1;
	doforeach("lpr", lprint);
}
#endif	/* LPRINT */


static void fgmatch(char *q, int n)
{
	register char *s;

	UNUSED(n);
	plnn++;
	if (regexec(re, q) == 1)
	{
		s = gmalloc(((uint) (lfnm + strlen(q) + 20)));
		if (s == NULL)
			return;
		strcpy(s, fnm);
		s[lfnm] = ' ';
		strcpy(s + lfnm + 1, itoal((long) plnn));
		strcat(s, ": ");
		strcat(s, q);
		outstr(s);
		gfree(s);
	}
}


static void legrep(char *pnm)
{
	register int fd;

	fd = (int)gfopen(pnm, 0);
	if (fd < 0)
	{
		emsg = "file not found";
		return;
	}

	fnm = pnm;
	lfnm = (int)strlen(fnm);
	plnn = 0;
	eachline(fd, fgmatch);
}


static void igrep(int flag)						/* flag => fgrep; else egrep    */
{
	char meta[256];
	register char *p,
	*q,
	*r,
	*s,
	 c;

	p = gstrdup(lexgetword());			/* p has pattern */
	if (flag && p && (r = q = gmalloc(((uint) (1 + 2 * strlen(p))))) != NULL)
	{
		charset(meta, "[()|?+*.$", 1);
		s = p;
		while ((c = *p++) != 0)
		{
			if (meta[c])
				*q++ = '\\';
			*q++ = c;
		}
		*q = '\0';
		gfree(s);
		p = r;
	}
	if (p)
	{
		re = regcomp(p);
		gfree(p);
		if (re == NULL)
			valu = -1;
		else
		{
			doforeach("egrep", legrep);
			gfree(re);
		}
	}
}


void egrep(uchar *arg)
{
	UNUSED(arg);
	igrep(0);
}


void fgrep(uchar *arg)
{
	UNUSED(arg);
	igrep(1);
}


/* called from within regexp.c  */
void regerror(const char *s)
{
	userfeedback(sprintp("ill formed regexp: %s", s), 0);
}
