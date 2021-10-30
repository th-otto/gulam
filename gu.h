/*
	gu.h -- include file for the shell 'gulam'  7/20/86

	copyright (c) 1986 pm@cwru.edu
*/

#define	GU	1

typedef	unsigned int	uint;
typedef	char	uchar;

#include "sysdpend.h"		/* this should come after OS #define	*/
#include <setjmp.h>

#define	FALSE	0
#define	TRUE	(!FALSE)
#define ABORT   2		/* Death, ^G, abort, etc.       */
#ifndef NULL
#define	NULL	0L
#endif

#ifndef UNUSED
# define UNUSED(x) ((void)(x))
#endif


				/* some std ascii chars		*/
#define	CTRLC	'\003'
#define	CTRLD	'\004'
#define	CTRLG	'\007'
#define	CTRLQ	'\021'
#define	CTRLS	'\023'
#define	CTRLO	'\017'
#define	ESC	'\033'

#define	Iwssz	100
typedef struct
{	int	nc;
	int	sz;
	int	ns;
	uchar 	*ps;
}	WS;

typedef	uchar	*WSPS;

typedef	struct	GA
{	int	sz;		/* size (in bytes) of an element	*/
	int	ne;		/* #elements in the array		*/
	int	na;		/* #elements allocated			*/
	int	ni;		/* increment for re allocation		*/
	uchar	el[MINELM];	/* elements themselves			*/
}	GA;

typedef	struct	TBLE
{	uchar *		key;
	uchar *		elm;
	struct TBLE *	next;
}	TBLE;

extern	uchar	CRLF[];		/* "\r\n"			*/
extern	uchar	ES[];		/* "\0", used as empty string	*/
extern	uchar	GulamHelp[];	/* = "gulam_help_file";		*/
extern	uchar	Nrows[];	/* = "nrows"			*/
extern	uchar	OwnFonts[];
extern	uchar	History[];
extern	uchar	Home[];
extern	uchar	Rgb[];
extern	uchar	Cwd[];
extern uchar Status[];				/* in tv.c  */
extern uchar Shell[];
extern uchar OS[];
extern uchar Gulam[];
extern uchar Verbosity[];
extern uchar GulamLogo[];
extern uchar uE[];
extern uchar Copyright[];
extern uchar defaultprompt[];
extern uchar Mini[];
extern uchar Font[];
extern uchar Completions[];
extern uchar Tempexit[];
extern uchar Scrninit[];							/* clr scrn, cursr off, wordwrap on */
extern uchar Parentexp[];
extern uchar Ncmd[];
extern long starttick;
extern long _stksize;
extern unsigned long masterdate;
extern uchar ext[];
extern uchar *rednm[2];
extern int outappend;

/* Delimiters when treating the subwords: must not contain meta-chars
?*()[].  By def, a subword has no white chars.  */

extern uchar DS0[];
extern uchar DS1[];
extern uchar DS2[];
extern uchar DS3[];
extern uchar DS4[];
extern uchar DS5[];
extern uchar DFLNSEP[];

extern	uchar	WHITEC[];	/* ==1 for white chars, 0 ow	*/
extern	uchar	WHITEDELIMS[],DELIMS[],SUBDELIMS[],COMMA[],BTRNQD[]; /* 0/1*/
extern	uchar	TKN2[],	EMPTY2[];	/* 0/1 vectors		*/

extern	uchar *	emsg;
extern	uchar *	strg;
extern	long	valu;
extern int fda[4];
extern int cmdprobe;				/* AKP */

extern	int	state;
extern	uchar	cnmcomplete;	/* name completion key char	*/
extern	uchar	cnmshow;

extern	uchar	negopts[], posopts[];
extern jmp_buf abort_env;
extern jmp_buf time_env;

#define	lowercase(xx)	chcase(xx, 0)
#define	uppercase(xx)	chcase(xx, 1)

WS *initws(void);
void freews(WS *ws);
WS *dupws(WS *ws);
int findstr(WSPS q, uchar *p);
WS *prefixws(uchar *p, WSPS q);
WS *lex(uchar *p, uchar *dlm, uchar *t2);
WS *useuplexws(void);
WS *expand(uchar *p, int addremargs);
WS *metaexpand(uchar *p);
WS *aliasexp(WS *ws);
WS *dupenvws(int flag);
WS *tblws(TBLE *t, int style);
GA *initga(int z, int i);
GA *addelga(GA *ga, void *e);
TBLE *tblfind(TBLE *t, uchar *k);
void tbldelete(TBLE *t, uchar *k);
void tblinsert(TBLE *t, uchar *k, uchar *e);
TBLE *tblcreate(void);
void tblfree(TBLE *t);
uchar *tblstr(TBLE *t, int style);
void strwcat(WS *ws, const uchar *p, int i);
void appendws(WS *u, WS *v, int m);
void shiftws(WS *ws, int m);
void showwsps(WSPS p);
void pushws(WS *w);
void popargws(void);
int isdir(char *p);
void freewtbl(void);
void wls(int flag);
void fnmtbl(uchar *arg);
void ls(uchar *arg);
unsigned long getticks(void);
void resetrs232buf(void);
void setrs232buf(void);
void setrs232speed(void);
void sbreak(void);
void alarm(uint n);
int readmodem(void);
void writemodem(char *buf, int len);
void flushinput(void);
void teexit(uchar *arg);
void bindkey(int n, uchar *keycode, uchar *cmdcode);
void fg(uchar *arg);
void ue(uchar *arg);
void moreue(uchar *arg);
void ueinit(void);
void ueexit(void);
void uebody(void);
KEY getkey(void);
KEY getctlkey(void);
void alias(uchar *arg);
void unalias(uchar *arg);
void printenv(uchar *arg);
void gsetenv(uchar *arg);
void gunsetenv(uchar *arg);
void readinenv(uchar *p);
int varnum(uchar *p);
void showvars(void);
void insertvar(uchar *p, uchar *q);
void setvarnum(uchar *p, int n);
void setvar(uchar *arg);
void unsetvar(uchar *arg);
void rdhelpfile(void);
uchar *findname(int x, int k);
uchar *getprompt(void);
void outstrg(void);
void gulamhelp(uchar *arg);
void getcmdanddoit(void);
void gtime(uchar *arg);
void processcmd(char *qq, int savehist);
int filetp(uchar *p, int a);
void attrstr(int a, uchar *p);
void gmkdir(uchar *p);
void grmdir(uchar *p);
void cd(uchar *p);
void gchmod(uchar *p);
void tch(uchar *p, _DOSTIME *td);
void touch(uchar *p);
int outisredir(void);
int doout(void *dummy, uchar *p);
void doredirections(void);
void undoredirections(void);
void computetime(void);
void stamptime(unsigned long *ip);
void datestr(int d, uchar *p);
int tooold(unsigned int date);
void yearstr(unsigned int date, char *s);
void timestr(int t, uchar *p);
void gsdatetime(uchar *s);
void Gem(uchar *arg);
void setgulam(void);
void mem(uchar *arg);
void lpeekw(uchar *arg);
void lpokew(uchar *arg);
void format(uchar *p);
uchar *drvmap(char *buf);
void df(uchar *p);
void dm(uchar *p);
int inbatchfile(void);
void csexit(int n);
int batch(uchar *p, uchar *cmdln, uchar *envp);
void csif(uchar *arg);
void cselse(uchar *arg);
void csendif(uchar *arg);
void cswhile(uchar *arg);
void csendwhile(uchar *arg);
void csendfor(uchar *arg);
void histinit(void);
void rehash(uchar *arg);
void addoptions(uchar *p);
void source(uchar *arg);
void echo(uchar *arg);
int asktodoop(uchar *op, uchar *p);
void doforeach(uchar *op, void (*f)(uchar *arg));
void setuekey(uchar *arg);
void showbuiltins(void);
void docmd(void);
void pwd(uchar *arg);
void cdcmd(uchar *arg);
void dirs(uchar *arg);
void pushd(uchar *arg);
void popd(uchar *arg);
void grename(uchar *arg);
void rm(uchar *p);
void cp(uchar *arg);
void mv(uchar *arg);
void cat(uchar *p);
void print(uchar *arg);
void lpr(uchar *arg);
void egrep(uchar *arg);
void fgrep(uchar *arg);
void csforeach(uchar *arg);
void rxmdm(uchar *p);
void sxmdm(uchar *p);
void cmdwhich(uchar *arg);




uchar *execcmd(WS *ws);
uchar *getoneline(void);
uchar *csexp(uchar *p, uchar **nextword);
uchar *fnmpred(uchar c, uchar *p);
uchar *substhist(uchar *s);
uchar *varstr(uchar *p);
uchar *ggetenv(uchar *p);
uchar *fnmsinparent(uchar *p);
uchar *getalias(char *p);
uchar *gfgetcwd(void);
uchar *hashlookup(int i, uchar *p);
void which(int f);
uchar *lexsemicol(void);
uchar *lexgetword(void);
uchar *lexhead(void);
uchar *lextail(void);
uchar *lexlastword(void);
void appendlextail(WS *w);
void lexaccept(WS *w);
void lexpush(void);
void lexpop(void);
void *gmalloc(unsigned int);
uchar *maxalloc(long *nb);
void maxfree(uchar *p);
uchar *catall(WS *ws, int m);
uchar *pscolumnize(uchar *r, int ns, int cw);
uchar *nthstr(WSPS p, int n);
uchar *str3cat(uchar *p, uchar *q, uchar *r);
uchar *strsub(uchar *p, uchar *q);
char *gstrdup(const char *p);
uchar *unquote(uchar *q, uchar *p);
uchar *chcase(uchar *p, int i);
uchar *itoar(long i, int r);
uchar *itoal(long i);
uchar *sprintp(uchar *fmt, ...);
long atoir(const char *p, int r);
int atoi(const char *p);
int stackoverflown(int n);
void eachline(int fd, void (*fn)(uchar *q, int n));
void streachline(uchar *text, int (*fn)(void *ap, uchar *p), void *ap);
int matchednms(uchar *lst, uchar *p, int flag);
uchar *ptometach(uchar *p);
int nameexpand(uchar *lst, uchar *q, int tenex);
uchar *stdpathname(uchar *nm);
int fnmexpand(uchar **p, uchar **q, int pexp, int tenex);
int fullname(uchar *name);
uchar *prevhist(void);
void remember(char *p, int n);
void history(uchar *arg);
void readhistory(void);
void savehistory(void);
void teupdate(void);
void charset(uchar *a, uchar *s, uint v);
void panic(const char *s);
