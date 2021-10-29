/*
	rehash.c of gulam -- rehash executable file names  08/03/86

	copyright (c) 1987 pm@cwru.edu
*/

#include "ue.h"

/*	uses a table created by tbl.c:
	key == 	leaf name of the executable
	elm ==	full path name, incl drive
*/

static struct TBLE *hp[2];
static int hx;

/* see if fnm p is in hash table hp[0], or hp[1] and return its full
path */

uchar *hashlookup(int i, uchar *p)
{
	struct TBLE *h;

	h = tblfind(hp[i], p);
	return h ? h->elm : NULL;
}

/* Add to hash table: prg-name, full pathname full path == dir | pnm
*/
static void addname(uchar *dir, uchar *pnm)
{
	uchar *p;
	uchar *nm;
	uchar *pt;

	pt = str3cat(dir, DS0, pnm);
	nm = gstrdup(pnm);
	if (nm == NULL || pt == NULL)
		return;
	/* freeing? forget it!  */
	deleteext(nm, p);					/* delete extensions (in some OS) */
	tblinsert(hp[hx], nm, pt);
}

/* find exec files in dir p; dont alter p */
static void findexecs(uchar *p)
{
	uchar *q;
	uchar *pbs;
	uchar *cwd;
	int n;

	if (p[0] == '.' && p[1] == '\0')
		hx = 0;
	if (hp[hx] == NULL)
		hp[hx] = tblcreate();
	cwd = gfgetcwd();
	cd(p);
	if (emsg || (valu < 0))
		goto freecwd;

	wls(0);								/* don't use wlstbl */
	if ((q = strg) != NULL)
	{
		pbs = p + strlen(p) - 1;
		if (*pbs != DSC)
			pbs = NULL;
		else
			*pbs = '\0';
		n = executables(q);
		for (; *q && (n-- > 0); q += strlen(q) + 1)
			addname(p, q);
		gfree(strg);					/* got this from wls(0) */
		strg = NULL;					/* ow we will try to output it */
		if (pbs)
			*pbs = DSC;
	}
  freecwd:if (cwd)
	{
		cd(cwd);
		gfree(cwd);
	}
	emsg = NULL;
	valu = 0L;
}


void which(int f)
{
	WS *ws;

	ws = initws();
	strwcat(ws, tblstr(hp[0], f), 0);
	strwcat(ws, "\r\n** cwd **\r\n", 0);
	strwcat(ws, tblstr(hp[1], f), 0);
	if (ws)
		strg = ws->ps;
	gfree(ws);
}


void cmdwhich(uchar *arg)
{
	uchar *p;
	uchar *q;
	struct TBLE *a;
	int i;

	UNUSED(arg);
	p = lexgetword();
	if (*p)
	{
		for (q = NULL, i = 0; i < 2; i++)
			if ((a = tblfind(hp[i], p)) != NULL)
				q = sprintp("%s is external cmd %s", p, a->elm);
		if (q == NULL)
			q = sprintp("%s is not in hash tbl", p);
		strg = gstrdup(q);
	} else
		which(1);
}

void rehash(uchar *arg)								/* rebuild the table of executable file names */
{
	uchar *q;
	uchar *p;
	int n;
	WS *ws;

	UNUSED(arg);
	for (n = 0; n < 2;)
	{
		if (hp[n])
			tblfree(hp[n]);
		hp[n++] = NULL;
	}
	q = ggetenv("PATH");
	if (q == NULL)
		return;
	hx = 1;
	ws = lex(q, COMMA, EMPTY2);
	p = ws->ps;
	n = ws->ns;
	while ((n > 0) && (q = nthstr(p, --n)) != NULL && *q)
	{
		findexecs(q);
		n--;							/* skip the ',' separator */
	}
	if (hx == 1)
	{
		hp[0] = hp[1];
		hp[1] = NULL;
	}
}
