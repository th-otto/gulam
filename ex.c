/*
	expand.c of gulam -- token expansion, file name matching etc

	copyright (c) 1987 pm@Case			01/31/87
*/

#include "ue.h"
#include "regexp.h"

uchar Parentexp[] = "parent_expand";

static uchar DOTS3[] = "...";

/* map a filename reg exp to egrep-style    */
/* p != NULL, & it does not have DSC        */
static uchar *mapfnmregexp(uchar *p)
{
	register uchar *q,
	*bq,
	 c;

	bq = q = gmalloc(((uint) (2 * strlen(p) + 3)));
	if (q == NULL)
		return q;
	for (*q++ = '^'; (c = *p++) != 0; *q++ = c)
	{
		if (c == '*')
			*q++ = '.';
		else if (c == '.' || c == '+')
			*q++ = '\\';
		else if (c == '?')
			c = '.';
	}
	*q++ = '$';
	*q = '\0';
	return bq;
}


/* In the list lst of (file) names, sublist those matching p, which is
the leaf filename pattern.  If flag, update lst by deleting the
unmatched strings; ow leave the lst unchanged.  */

int matchednms(uchar *lst, uchar *p, int flag)
{
	register int nmatches;
	register uchar *f,
	*g,
	*q;
	register regexp *re;

	nmatches = 0;
	f = g = lst;
	if (f == NULL)
		return 0;
	q = mapfnmregexp(p);
	if (q == NULL)
		return 0;
	re = regcomp(q);
	gfree(q);
	if (re)
		while (*g)
		{
			if (regexec(re, g) == 1)
			{
				nmatches++;
				if (flag)
				{
					while ((*f++ = *g++) != 0)
						;
					continue;
				}
			}
			g += strlen(g) + 1;
		}
	/** else emsg = "bad reg exp"; **/
	if (flag)
		*f = '\0';
	gfree(re);
	return nmatches;
}

/* Return ptr to the leftmost meta uchar in p; if none, return NULL */

uchar *ptometach(uchar *p)
{
	static uchar notmeta[256];
	static int metanotinit = 1;			/* ==0 => notmeta initialized  */
	register uchar *r;

	if (metanotinit)
	{
		metanotinit = 0;
		charset(notmeta, "[()|?*", 0);
		notmeta[0] = '\0';
	}
	for (r = p - 1; notmeta[*++r];) ;
	return (*r ? r : NULL);
}

/* Expand the name q in the context of list of words given in lst.
Normally the lst does not contain duplicate names.  Atari TOS can some
times produce >1 file with the same name.  The test for q[j] != 0 in
the do-while below is needed for this reason.  q must point to a
long-enough area to contain the expanded word.  */

int nameexpand(uchar *lst, uchar *q, int tenex)
{
	register int i,
	 n;

	if (q == NULL)
		return 0;
	/* q[1..i-1] may/may not contain meta uchars; so we append a '*' */
	i = (int)strlen(q);
	if (tenex)
	{
		q[i] = '*';
		q[i + 1] = '\0';
	}
	n = matchednms(lst, q, 1);			/* 1 => delete unmatching names */
	q[i] = '\0';
	if (n == 1 && (tenex || ptometach(q)))
		strcpy(q, lst);
	else /* n >= 1 => lst != NULL */ if (n > 1 && tenex)	/* extend w/o losing matches */
	{
		register int j,
		 k;

		q[i] = '*';
		do
		{
			for (j = 0; q[j] == lst[j];)
				j++;
			for (k = i + 1; k >= j; k--)
				q[k + 1] = q[k];
			q[j] = lst[j];				/* extend by one uchar */
			i++;						/* 0 below => dont delete unmatching names */
		} while (q[j] && n == matchednms(lst, q, 0));
		q[i - 1] = '\0';
	}
	return n;
}


/* Remove redundant \.., and \. 	*/
static void simplifypath(uchar *name)
{
	register uchar *p,
	*q,
	 c;

	for (; (q = strsub(name, DS1)) != NULL; name = q)
	{
		c = q[2];
		if (c == '\0' || c == DSC)
			strcpy(q, &q[2]);
		else if (c == '.')				/* q[0..2] ==  \.. */
		{
			*q = '\0';
			p = strrchr(name, DSC);
			if (p)
				strcpy(p, &q[3]);
			else
				*q++ = DSC;
		} else
			q++;
	}
}

/* Name is a filename as given by the user.  Convert it into a full,
canonical pathname with Gulam conventions, and return a gmalloc'd area
containing this path.  */

uchar *stdpathname(uchar *nm)
{
	register uchar *p,
	*q,
	*r;
	register int len;

	p = gstrdup(nm);
	if (p == NULL)
		return p;
	len = (int)strlen(p);
	if (len == 4 && (strcmp(p, "con:") == 0 || strcmp(p, "aux:") == 0 || strcmp(p, "prn:") == 0))
		goto fin;
	if (len > 2)
		for (;;)						/* check if a new path is given, eg e:\.....\d:\src\xy.c */
		{
			if ((q = strchr(p + 2, ':')) != NULL)
				q--;
			else if ((q = strsub(p, DS2)) != NULL)
				q++;
			else if ((q = strsub(p, DS3)) != NULL)
				q++;
			if (q == NULL)
				break;
			strcpy(p, q);
		}
	if ((p[0] != '\0') && (p[1] == ':'))
	{
		if (p[2] != DSC)
		{
			p[1] = '\0';
			p = str3cat(r = p, DS4, p + 2);
			gfree(r);
		}
	} else /* drive is not specified */ if (p[0] != '~')
	{									/* does not begin with ~, and drive is not specified */
		if ((q = gfgetcwd()) != NULL)
		{
			r = q + strlen(q) - 1;
			if (*r == DSC)
				*r = '\0';
			if (*p == DSC)
			{
				q[2] = '\0';
				strcpy(p, p + 1);
			}
			p = str3cat(q, DS0, r = p);
			gfree(r);
			gfree(q);
		}
	}
  fin:
	simplifypath(p);
	if (p[2] == '\0')
	{
		p = str3cat(q = p, DS0, ES);
		gfree(q);
	}
	return p;
}

/* Return ptr to leaf of the path, and also produce a string of file
names in the parent dir of path p; result in strg.  Note that this
rtn cd()'s to the parent dir.  If we do this too many times with invalid
path names for dirs, TOS will then refuse to cd() to evn known legitimate
paths.  So don't call this rtn when you are almost sure that the parent dir
path is invalid; see e.g., mxp() below. */

uchar *fnmsinparent(uchar *p)
{
	register uchar *q,
	*oldcwd;

	strg = emsg = NULL;
	valu = 0L;
	if (p == NULL)
		return NULL;
	if ((q = strrchr(p, DSC)) != NULL)
	{
		oldcwd = gfgetcwd();
		*q = '\0';
		cd(p);
		if (valu == 0L && emsg == NULL)
			wls(1);
		*q = DSC;
		if (oldcwd)
		{
			cd(oldcwd);
			gfree(oldcwd);
		}
		p = q + 1;
	} else
		wls(1);							/* this sets strg   */

	emsg = NULL;						/* ignore emsgs, if any, triggered by wls() */
	return p;
}

/* Expand the path expression of parent dir of path rs.  The leaf name
will remain as-is even if it contains meta uchars. Return a gmalloc'd
area.  Given arg rs will get freed. */

static uchar *parentexp(uchar *rs)
{
	register uchar *r,
	*pp;
	register WS *ws;

	rs = stdpathname(r = rs);
	gfree(r);
	if ((r = strrchr(rs, DSC)) != NULL)
	{
		*r++ = '\0';
		pp = gstrdup(rs);
		ws = expand(pp, 0);				/* expand() gfree()s pp */
		if (ws->ns != 1 || ws->ps == NULL)
			*--r = DSC;
		else
		{
			pp = str3cat(ws->ps, DS0, r);	/* r is within rs[..] */
			gfree(rs);
			rs = pp;
		}
		freews(ws);
	}
	return rs;
}


/* esc complete, in TENEX style, the current command, given in *p.
Returns *p, as is, if gmalloc failed, or expansion failed.  Otherwise,
gfree *p and return in *p a new gmalloc'd string; make sure it is
SZcmd long, because the result is given to getuserinput().  Meta-chars
in the leaf of the path are expanded, but elsewhere on the path are
treated as regulars.  *q contains, upon return, a gmalloc'd string of
possible completions; **q != '\0', if there are some completions, **q
== '\0', otherwise.  *q == NULL => a gmalloc failed somewhere.  */

/*
 es := catenated lexmes of p, except the last;
 rs := last lexeme of p;
 if (rs begins with !) rs := rs with history expanded
 else if (rs == path\leaf) rs := expand rs's path | \leaf
 r  :=  the leaf of rs;
 strg :=  wls list of fnms in the dir given by path;
 pp := es | rs;
 pp := (pp is a dir? append \ : append blank )
 q := strg; strg := NULL;
 n := #matches
*/
int fnmexpand(uchar **p, uchar **q, int pexp, int tenex)
{
	register uchar *r,
	*rs,
	*es,
	*pp,
	*last;
	register int n;

	*q = NULL;
	if ((p == NULL) || (*p == NULL))
		return 0;
	lex(*p, DELIMS, TKN2);
	rs = gstrdup(lexlastword());
	es = gstrdup(lexhead());
	/* lexlastword is never NULL; lexhead ruins lexlastword() string */
	if (rs)
	{
		if (*rs == '!')
			rs = substhist(rs);
		else if (pexp || varnum(Parentexp))
			rs = parentexp(rs);
	}
	if (rs == NULL || es == NULL)
		return 0;

	n = (int)strlen(es);
	pp = gmalloc(((uint) (strlen(rs) + n + 1 + SZcmd)));
	if (pp == NULL)
		return 0;
	if (n)
	{
		strcpy(pp, es);
		pp[n++] = ' ';
	}
	strcpy(&pp[n], rs);
	gfree(rs);
	last = &pp[n];
	/* r will == leaf name within string pp, and strg has fnms */
	r = fnmsinparent(&pp[n]);
	n = nameexpand(strg, r, tenex);
	if (tenex && n == 1)
		strcat(pp, (isdir(last) ? DS0 : " "));
	/* now pp contains the expansion */
	gfree(*p);
	*p = pp;
	*q = strg;
	strg = NULL;
	return n;
}

/* Expand the given name to a full path name, using Gulam conventions.
Occasionally, a space terminates the first fnm in name; so take care!
The resulting name better not have any meta uchars */

#define	NFILEN	80						/* see ue.h */

int fullname(uchar *name)
{
	register uchar *p;
	uchar *pp,
	*qq;
	register int n;

	lexpush();
	if ((pp = gstrdup(name)) != NULL && (qq = strchr(pp, ' ')) != NULL)
		*qq = '\0';
	n = fnmexpand(&pp, &qq, 1, 0);		/* 0 ==> No TENEX expansion */
	if ((p = pp) != NULL)
	{
		if (n > 1)						/* then take 1st name from qq */
		{
			if ((p = strrchr(p, DSC)) != NULL)
				p++;
			else
				p = pp;
			strcpy(p, qq);				/* there is enough room */
			p = pp;
		}
		if (strlen(p) > NFILEN - 1)
		{
			mlwrite("'%s' too long", p);
			p[NFILEN - 1] = '\0';
		}
		strcpy(name, p);
	}
	gfree(pp);
	gfree(qq);
	lexpop();
	return n;
}

/* Add the filenames in the subtree rooted at dir p to ws.  The p is a
full path name.  It does not end with a \ */

static void addfnmsofdir(WS *ws, uchar *p)
{
	register uchar *q;
	register WS *xs;
	register int n,
	 m;

	q = str3cat(p, DS0, ES);
	(void) fnmsinparent(q);				/* strg is set as a result */
	xs = prefixws(q, strg);
	gfree(q);
	gfree(strg);
	strg = NULL;
	n = ws->nc - 1;
	appendws(ws, xs, 0);
	freews(xs);
	m = ws->nc - 1;
	while (n < m)
	{
		p = &(ws->ps[++n]);				/* ws->ps ptr may change as we add to ws */
		n += (int)strlen(p);
		if (useraborted())
			break;
		if (isdir(p))
			addfnmsofdir(ws, p);
	}
}

/* p is a full std path name of a (supposed) dir preceding 3 dots.
Expand it, and return the stringlets in WS format.  */

static WS *dots3(uchar *p)
{
	register WS *ws;

	if ((ws = initws()) != NULL)
	{
		strwcat(ws, p, 1);
		if (isdir(p))
			addfnmsofdir(ws, p);
	}
	return ws;
}

/* Metaexpand work horse.  ds is a seq of full path names of dirs, lf is
a string free of DSC, but possibly containing meta uchars.  Expand the
pattern lf in each of these dirs, and return the resulting seq of fnms. */

static WS *mxp(WS *ds, uchar *lf)
{
	register WS *ws,
	*xs;
	register uchar *p,
	*q;
	register int i;

	ws = initws();
	if (ds == NULL)
		return ws;
	for (i = ds->ns, p = ds->ps; i--; p += strlen(p) + 1)
	{
		if (strcmp(lf, DOTS3))
		{
			if (isdir(p))
			{
				q = str3cat(p, DS0, ES);
				(void) fnmsinparent(q);	/* strg = list of names */
				matchednms(strg, lf, 1);
				xs = prefixws(q, strg);
				gfree(q);
				gfree(strg);
				strg = NULL;
			} else
				xs = NULL;
		} else
			xs = dots3(p);
		appendws(ws, xs, 0);
		freews(xs);
	}
	freews(ds);
	return ws;
}

/* Expand a path name exp p, which can contain meta uchars and 3-dots
in between the back-slashes also.  */

WS *metaexpand(uchar *p)
{
	register uchar *q,
	 c;
	register WS *ds;

	ds = NULL;
	if (p && ptometach(p))
	{
		if ((q = strrchr(p, DSC)) != NULL)
		{
			*q = '\0';
			ds = mxp(metaexpand(p), q + 1);
			*q = DSC;
		} else
		{
			if ((q = fnmsinparent(p)) != NULL)
			{
				/* n = */ matchednms(strg, q, 1);
				c = *q;
				*q = '\0';			/* end the prefix */
				ds = prefixws(p, strg);
				*q = c;
			}
			gfree(strg);
			strg = NULL;
		}
	} else if ((q = strsub(p, DOTS3)) != NULL)
	{
		if (q == p)
			p = ".";
		else if ((c = *--q) != DSC)
			q = NULL;
		if (q)
		{
			*q = '\0';
			ds = dots3(p);
			*q = c;
		}
	} else
	{
		ds = initws();
		q = NULL;
		strwcat(ds, p, 1);
	}
	return ds;
}

#if 00
static WS *fullmetaexpand(uchar *p)
{
	register WS *ds;

	p = stdpathname(p);
	ds = metaexpand(p);
	gfree(p);
	return ds;
}
#endif

typedef struct SE
{
	WS *ws;
	struct SE *next;

} SE;

static SE *spargws = NULL;


/* Push the argument ws onto the stack.  As a side effect, lex.c gives
up its remaining lexemes.  This we do because the given w is the
result of the last lex()'ing, and the next lex() call would attempt to
gfree the old ws lex.c hold onto unless it is NULL.  */

void pushws(WS *w)
{
	register SE *tp;

	(void) useuplexws();
	tp = (SE *) gmalloc(((uint) sizeof(SE)));
	if (tp == NULL)
		return;
	tp->ws = w;
	tp->next = spargws;
	spargws = tp;
}

void popargws(void)
{
	register SE *tp;

	tp = spargws;
	if (tp == NULL)
		return;
	spargws = tp->next;
	freews(tp->ws);
	gfree(tp);
}

#if 00
/*
 Expand the dot (.): . -> dh, .\ -> dh\ ; .. -> dh\.. where  dh == $home with any
 trailing \ deleted.  Note that  '.' is a SUBDELIM.
*/
static uchar *dot(void)
{
	register uchar *q,
	*r,
	*qq,
	 c;

	qq = q = gstrdup(varstr("cwd"));
	if (q && *q)
	{									/* get rid off trailing \  */
		r = q + strlen(q) - 1;
		if (*r == DSC)
			*r = '\0';
		r = lexgetword();
		c = *r;
		if (c == '\0' || c == DSC)
			r = DS0;
		else if (c == '.')
			r = DS5;
		else
			q = ".";
		q = str3cat(q, r, ES);
	}
	gfree(qq);
	return q;
}
#endif

/* Expand the wiggle (~): ~ -> dh; ~zz -> dh\..\zz where dh == $home
with any trailing \ deleted.  Note that ~ is not a SUBDELIM (because
filenames can have them; eg., 9~.n or f.c~).  */

static uchar *wiggle(uchar *w)
{
	register uchar *q,
	*r,
	*qq;

	qq = q = gstrdup(varstr("home"));
	if (q && *q)
	{									/* get rid off trailing \  */
		r = q + strlen(q) - 1;
		if (*r == DSC)
			*r = '\0';
		if (w[1])
		{
			q = str3cat(qq, DS5, w + 1);
			gfree(qq);
		}
	}
	return q;
}

/* Expand $p, where p is a subword immediately following a '$' */

static uchar *dollar(int *n)
{
	register uchar *p,
	*q,
	 c;
	register int braceflag;
	register WS *argws;
	uchar buf[256];

	*n = -1;
	argws = (spargws ? spargws->ws : NULL);
	p = lexgetword();
	c = *p;
	if ('0' <= c && c <= '9')
	{
		p = (argws ? nthstr(argws->ps, *n = atoi(p)) : ES);
	} else if (c == '*')
	{
		p = (argws ? catall(argws, 0) : ES);
		*n = 0x7FFF;
	} else if (c == '-')
	{
		p = (argws ? catall(argws, 1) : ES);
		*n = 0x7FFF;
	} else if (c == '<')
	{
		q = varstr("ginprompt");
		if (*q == '\0')
			q = "$< ";
		mlmesg(q);
		*buf = '\0';
		c = getuserinput(p = buf, 256);
		mlmesg(ES);
	} else
	{
		if (c == '{')					/* see next few tokens, as eg in ${abcd}e */
		{
			p = lexgetword();
			braceflag = 1;
		} else
			braceflag = 0;
		q = varstr(p);
		if (q == NULL || *q == '\0')
			q = ggetenv(p);
		p = q;
		if (braceflag)
			lexgetword();				/* better be } */
	}
	return gstrdup(p);
}

/* Apply only $var, and wildcard-expansions to the string p.  History
substitutions are already done.  Efficiency in this routine is
important.  If addremargs != 0, we add the remaining args to the
expanded result.  */

WS *expand(uchar *p, int addremargs)
{
	WS *ws,
	*ws2,
	*ws3;
	register uchar *q,
	*lemsg;
	register int dqsflag,
	 maxdn;
	int ii;

	lemsg = NULL;
	maxdn = 0;							/* maxdn == max # of the dollar arg used    */

	lex(p, DELIMS, TKN2);
	gfree(p);
	ws = initws();
	while ((p = lexgetword()) != NULL && *p && *p != '#')
	{
		lexpush();
		lex(p, SUBDELIMS, EMPTY2);
		ws2 = initws();
		while ((p = lexgetword()) != NULL && *p)
		{
			dqsflag = 0;				/* dbl quoted string flag */
			switch (*p)
			{
			case '$':
				p = dollar(&ii);
				if (maxdn < ii)
					maxdn = ii;
				break;
			case '"':
				dqsflag = 1;
				q = gstrdup(p);
				unquote(q, q);
				lexpush();
				ws3 = expand(q, 0);
				p = catall(ws3, 0);
				if (p == ES)
					p = ws3->ps;
				gfree(ws3);
				lexpop();
				break;
			case '~':
				p = wiggle(p);
				break;
			default:
				p = gstrdup(p);
				break;
			}
			if (dqsflag)
			{
				q = p;
				p = str3cat("\"", q, "\"");
				gfree(q);
			}
			strwcat(ws2, p, 0);
			gfree(p);
		}
		if (ws2 && (p = ws2->ps) != NULL)
		{
			lexpush();
			for (lex(p, DELIMS, TKN2); (p = lexgetword()) != NULL && *p;)
			{
				if (*p != '\'' && *p != '"')
				{
					if (ptometach(p) || strsub(p, DOTS3))
					{
						ws3 = metaexpand(p);
									 /** full?? **/
						if (ws3 == NULL || ws3->ns == 0 || ws3->ps == NULL)
							lemsg = sprintp("'%s' failed", p);
						appendws(ws, ws3, 0);
						freews(ws3);
						p = NULL;
					}
				}
				strwcat(ws, p, 1);
			}
			lexpop();
		}
		freews(ws2);
		lexpop();
	}
	if (addremargs && spargws)
		appendws(ws, spargws->ws, ++maxdn);
	freews(useuplexws());
	if (emsg == NULL)
		emsg = lemsg;
	return ws;
}

/* Alias expand the stringlets in ws.  */

WS *aliasexp(WS *ws)
{
	register uchar *q,
	*r;

	if (ws == NULL)
		return NULL;					/* q == r => no alias for r */
	q = getalias(r = ws->ps);
	if (q == r)
		return ws;
	pushws(ws);
	ws = expand(gstrdup(q), 1);
	popargws();
	return ws;
}

/* Rules of expansion:
(1) Divide line into words.
(2) Divide each word into subwords.
(3) For each subword do:
(4) Just before docmd(), alias expand.
*/
