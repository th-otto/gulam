/*
	ue.h of Gulam/uE

This file is the general header file for all parts of the MicroEMACS
display editor.  It contains definitions used by everyone, and it
contains the stuff you have to edit to create a version of the editor
for a specific operating system and terminal.  */


typedef	unsigned short KEY;

#include "gu.h"


#define NFILEN  80			/* # of bytes, file name        */
#define NBUFN   16			/* # of bytes, buffer name      */
#define NLINE   256			/* # of bytes, line             */
#define NKBDM   4*256			/* # of strokes, keyboard macro */
#define NPAT    80			/* # of bytes, pattern          */
#define HUGE    1000			/* Huge number                  */

#define AGRAVE  0x60			/* M- prefix,   Grave (LK201)   */
#define METACH  0x1B			/* M- prefix,   Control-[, ESC  */
#define CTMECH  0x1C			/* C-M- prefix, Control-\       */
#define EXITCH  0x1D			/* Exit level,  Control-]       */
#define CTRLCH  0x1E			/* C- prefix,   Control-^       */
#define HELPCH  0x1F			/* Help key,    Control-_       */
#define	KRANDOM	0xFF			/* A "no key" code. pm		*/
#define	KCHAR	0x00FF			/* The basic character code.	*/

#define BEL		007
#define	CCHR(c)		(c & 0x1F)
#define	ISLOWER(c)	(('a' <= c) && (c <= 'z'))
#define	ISUPPER(c)	(('A' <= c) && (c <= 'Z'))
#define	TOUPPER(c)	(c + 'A' - 'a')

#define FIOSUC  0			/* File I/O, success.           */
#define FIOFNF  1			/* File I/O, file not found.    */
#define FIOEOF  2			/* File I/O, end of file.       */
#define FIOERR  3			/* File I/O, error.             */

#define CFCPCN  0x0001			/* Last command was C-P, C-N    */
#define CFKILL  0x0002			/* Last command was a kill      */

#define KNONE	0			/* Flags for "ldelete"/"kinsert" */
#define KFORW	1
#define KBACK	2
#define	KEOTBL	0xffff			/* no key code matches this	*/

					/* key-to-fn bindings		*/
#define	REGKB	0
#define	GUKB	1
#define	MINIKB	2
#define	TEKB	3
	
typedef struct
{	KEY	k_code;			/* Key code                     */
	int	k_fx;			/* corresp fn index into FPFS	*/
}       KB;

typedef	int	(*FPFS)(int f, int n);		/* ptr to function		*/

/* All text is kept in circularly linked lists of LINE structures.
These begin at the header line (which is the blank line beyond the end
of the buffer).  This line is pointed to by the BUFFER.  Each line
contains a the number of bytes in the line (the "used" size), the size
of the text array, and the text.  The end of line is not stored as a
byte; it's implied.  Future additions will include update hints, and a
list of marks into the line.  */


typedef struct  LINE
{	struct	LINE *l_fp;		/* Link to the next line        */
        struct	LINE *l_bp;		/* Link to the previous line    */
#ifndef	PMMALLOC
 	int	l_zsize;		/* Allocated size               */
#endif
        int	l_used;			/* Used size                    */
	uchar	l_text[MINELM];		/* A bunch of characters.       */
}       LINE;	

#define lforw(lp)       ((lp)->l_fp)
#define lback(lp)       ((lp)->l_bp)
#define lgetc(lp, n)    ((lp)->l_text[(n)]&0xFF)
#define lputc(lp, n, c) ((lp)->l_text[(n)]=(c))
#define llength(lp)     ((lp)->l_used)

#ifdef	PMMALLOC
#define	lsize(lp) ((int)(((int *)lp)[-1]) - ((uint)sizeof(LINE)) - 3)
#else
#define	lsize(lp) (lp->l_zsize)
#endif

/* There is a WINDOW structure allocated for every active display
window.  The windows are kept in a big list, in top to bottom screen
order, with the listhead at "wheadp".  Each window contains its own
values of dot and mark.  The flag field contains some bits that are
set by commands to guide redisplay; although this is a bit of a
compromise in terms of decoupling, the full blown redisplay is just
too expensive to run for every input character.  */


typedef struct  WINDOW
{	LINE	*w_dotp;		/* Line containing "."          */
	LINE	*w_markp;		/* Line containing "mark"       */
	int	w_doto;			/* Byte offset for "."          */
	int	w_marko;		/* Byte offset for "mark"       */
	struct	WINDOW *w_wndp;		/* Next window                  */
	struct	BUFFER *w_bufp;		/* Buffer displayed in window   */
	LINE	*w_linep;		/* Top line in the window       */
	uchar	w_toprow;		/* Origin 0 top row of window   */
	uchar	w_ntrows;		/* # of rows of text in window  */
	uchar	w_force;		/* If NZ, forcing row.          */
	uchar	w_flag;			/* Flags.                       */
} WINDOW;	

#define WFFORCE 0x01			/* Window needs forced reframe  */
#define WFMOVE  0x02			/* Movement from line to line   */
#define WFEDIT  0x04			/* Editing within a line        */
#define WFHARD  0x08			/* Better to a full display     */
#define WFMODE  0x10			/* Update mode line.            */

/* Text is kept in buffers.  A buffer header, described below, exists
for every buffer in the system.  The buffers are kept in a big list,
so that commands that search for a buffer by name can find the buffer
header.  There is a safe store for the dot and mark in the header, but
this is only valid if the buffer is not being displayed (that is, if
"b_nwnd" is 0).  The text for the buffer is kept in a circularly
linked list of lines, with a pointer to the header line in "b_linep".
*/

typedef struct  BUFFER
{	LINE	*b_dotp;		/* Link to "." LINE structure   */
	LINE	*b_markp;		/* The same as the above two,   */
	int	b_doto;			/* Offset of "." in above LINE  */
	int	b_marko;		/* but for the "mark"           */
	struct	BUFFER *b_bufp;		/* Link to next BUFFER          */	
	LINE	*b_linep;		/* Link to the header LINE      */
	uchar	b_nwnd;			/* Count of windows on buffer   */
	uchar	b_flag;			/* Flags                        */
	uchar	b_fname[NFILEN];	/* File name                    */
	uchar	b_bname[NBUFN];		/* Buffer name                  */
	int	b_kbn;			/* key-to-fn bindings		*/
	uchar	b_modec;		/* char for this bufs modeline	*/
}       BUFFER;	

#define BFTEMP  0x01			/* Internal temporary buffer    */
#define BFCHG   0x02			/* Changed since last write     */
#define	BFRDO	0x04			/* Buffer is read-only		*/



typedef	int	RSIZE;

/* The starting position of a region, and the size of the region in
characters, is kept in a region structure.  Used by the region
commands.  */


typedef struct
{	LINE	*r_linep;		/* Origin LINE address.         */
	RSIZE	r_offset;		/* Origin LINE offset.          */
	RSIZE	r_size;			/* Length in characters.        */
}       REGION;	


/* The editor communicates with the display using a high level
interface.  A TERM structure holds useful variables, and indirect
pointers to routines that do useful operations.  The low level get and
put routines are here too.  This lets a terminal, in addition to
having non standard commands, have funny get and put character code
too.  The calls might get changed to "termp->t_field" style in the
future, to make it possible to run more than one terminal type.  */


typedef struct
{	int   t_nrow;                 /* Number of rows.              */
        int   t_ncol;                 /* Number of columns.           */
#ifndef	AtariST
        int     (*t_open)();            /* Open terminal at the start.  */
        int     (*t_close)();           /* Close terminal at end.       */
        int     (*t_getchar)();         /* Get character from keyboard. */
        int     (*t_putchar)();         /* Put character to display.    */
        int     (*t_flush)();           /* Flush output buffers.        */
        int     (*t_move)();            /* Move the cursor, origin 0.   */
        int     (*t_eeol)();            /* Erase to end of line.        */
        int     (*t_eeop)();            /* Erase to end of page.        */
        int     (*t_beep)();            /* Beep.                        */
#endif
}       TERM;

typedef struct  VIDEO
{	int	v_flag;                 /* Flags */
        uchar	v_text[1];              /* Screen data. */
}       VIDEO;


extern  uchar	pat[];                  /* Search pattern               */
extern  uchar	msginit[];		/* my ego!			*/
extern  int	exitue;			/* indicates where control is	*/
extern  int	currow;                 /* Cursor row                   */
extern  int	curcol;                 /* Cursor column                */
extern  int	thisflag;               /* Flags, this command          */
extern  int	lastflag;               /* Flags, last command          */
extern  int	mpresf;                 /* Stuff in message line        */
extern  int	sgarbf;                 /* State of screen unknown      */
extern  WINDOW	*wheadp;                /* Head of list of windows      */
extern  WINDOW	*curwp;                 /* Current window               */
extern  WINDOW	*mlwp;			/* message line window		*/
extern  BUFFER	*curbp;                 /* Current buffer               */
extern  BUFFER	*bheadp;                /* Head of list of buffers      */
extern  BUFFER	*gulambp;               /* gulam buffer                 */
extern  BUFFER	*minibp;		/* mini buffer			*/
extern  TERM	term;                   /* Terminal information.        */
extern int lnn;
extern int evalu;
extern long ntotal;
extern uchar Mdmport[];
extern uchar Mscursor[];
extern FPFS fpfs[];
extern KB tekeybind[];
extern KEY lastkey;
extern KB *kba[];
extern uchar Bufferlist[];
extern int screenwidth;



/* actually from gulam */
void csexecbuf(BUFFER *bp);




/*
 * basic.c
 */
int igotoeob(void);
int getgoal(LINE *dlp);
void isetmark(void);


/*
 * buffer.c
 */
void wwdotmark(WINDOW *wp2, WINDOW *wp);
void wbdotmark(WINDOW *wp, BUFFER *bp);
void bwdotmark(BUFFER *bp, WINDOW *wp);
int getbufname(uchar *fps, uchar *defnm, uchar *bnm);
BUFFER *nextbuffer(void);
int switchbuffer(BUFFER *bp);
int bufinsert(uchar *bufn);
int bufkill(BUFFER *bp);
WINDOW *popbuf(BUFFER *bp);
int addline(void *_bp, uchar *text);
int anycb(void);
BUFFER *bfind(uchar *bname, unsigned int cflag, unsigned int bflag, int keybinds, uchar bmodec);               /* Lookup a buffer by name      */
int bclear(BUFFER *bp);
void killcompletions(void);
void showcompletions(uchar *text);
void bufinit(void);


/*
 * display.c
 */
void vtinit(void);
void uefreeall(void);
void update(void);
void mlerase(void);
int mlyesno(uchar *prompt);
int mlreply(uchar *prompt, uchar *buf, int nbuf);
void mlwrite(const char *fmt, ...);
void mlmesg(uchar *p);
void mlmesgne(uchar *p);


/*
 * file.c
 */
int flvisit(uchar *f);
int insertborf(uchar *name, int flag);
int flsave(BUFFER *bp);
unsigned int userfnminput(uchar **p, int sz, void (*fn)(uchar *r), int pexp);


/*
 * fio.c
 */
int ffwopen(char *fn);
int ffclose(void);
int ffputline(char *buf, int nb);
int frdapply(char *fnm, void (*fn)(uchar *q, int n));


/*
 * gasmgnu.c
 */
short *lineA(void);
void hi50(void);
void hi40(void);
void hi25(void);
void mouseon(uchar *arg);
void mouseoff(uchar *arg);
int getnrow(void);
int getncol(void);
int font8(void);
int font10(void);
int font16(void);
void fontreset(void);


/*
 * gioatari.c
 */
void tioinit(void);
void storekeys(KEY *p);
KEY inkey(void);
void showinc(void);
KEY usertyped(void);
int useraborted(void);
void gputs(const char *s);
void vttidy(void);
void mvcursor(int row, int col);
void nrow2550(void);
void font(void);
void setmdmport(void);
void pallete(void);
void drawshadedrect(void);
void mousecursor(void);
void mouseregular(void);
void keysetup(void);


/*
 * kb.c
 */
void bindkey(int n, uchar *keycode, uchar *cmdcode);


/*
 * line.c
 */
LINE *lalloc(int used);              /* Allocate a line              */
LINE *lnlink(LINE *lx, uchar *q, int nb);
void lfree(LINE *lp);
void lbpchange(BUFFER *bp, int flag);
void lchange(int flag);
int lnewline(void);
int ldelete(RSIZE n, int kflag);
int ldelnewline(void);
void kdelete(void);
int kinsert(int c, int dir);
int kremove(int n);


/*
 * misc.c
 */
BUFFER *setgulambp(int f);
void setminibp(void);
void tominibuf(void);
void userfeedback(uchar *s, int n);
void outstr(uchar *text);
BUFFER *opentempbuf(uchar *p);
void closebuf(BUFFER *bp);
void addcurbuf(uchar *p);
uchar *makelnstr(LINE *lp);
uchar getuserinput(uchar *buf, int nbuf);


/*
 * random.c
 */
int getccol(int bflg);


/*
 * region.c
 */
int getregion(REGION *rp);


/*
 * rsearch.c
 */
void casesensitize(uchar *tpat);


/*
 * teb.c
 */
void teupdate(void);


/*
 * util.c
 */
void panic(const char *s);
void cpymem(void *d, void *s, int n);
uchar *sprintp(uchar *fmt, ...);


/*
 * window.c
 */
void wupdatemodeline(BUFFER *bp);
int switchwindow(WINDOW *wp);
void killwindow(WINDOW *wp);
WINDOW *makewind(int top, int ntr);
WINDOW *wpopup(void);              /* Pop up window creation       */
void wininit(void);


/*
 * pmalloc.c
 */
void freeall(void);
void showgumem(void);
int gfree(void *p);
void *gmalloc(unsigned int);



/*
 * functions that can be bound to keys
 */

/*
 * Defined by "ue.c".
 */
int ctrlg(int f, int n);					/* Abort out of things      */
int quit(int f, int n);						/* Quit             */
int ctlxlp(int f, int n);					/* Begin macro          */
int ctlxrp(int f, int n);					/* End macro            */
int ctlxe(int f, int n);					/* Execute macro        */

/*
 * Defined by "search.c".
 */
int forwsearch(int f, int n);				/* Search forward       */
int backsearch(int f, int n);				/* Search backwards     */
int searchagain(int f, int n);				/* Repeat last search command   */
int queryrepl(int f, int n);				/* Query replace        */

/*
 * Defined by "basic.c".
 */
int gotobol(int f, int n);					/* Move to start of line    */
int backchar(int f, int n);					/* Move backward by characters  */
int gotoeol(int f, int n);					/* Move to end of line      */
int forwchar(int f, int n);					/* Move forward by characters   */
int gotobob(int f, int n);					/* Move to start of buffer  */
int gotoeob(int f, int n);					/* Move to end of buffer    */
int forwline(int f, int n);					/* Move forward by lines    */
int backline(int f, int n);					/* Move backward by lines   */
int forwpage(int f, int n);					/* Move forward by pages    */
int backpage(int f, int n);					/* Move backward by pages   */
int pagenext(int f, int n);					/* Page forward next window */
int setmark(int f, int n);					/* Set mark         */
int swapmark(int f, int n);					/* Swap "." and mark        */
int gotoline(int f, int n);					/* Go to a specified line.  */

/*
 * Defined by "buffer.c".
 */
int listbuffers(int f, int n);				/* Display list of buffers  */
int usebuffer(int f, int n);				/* Switch a window to a buffer  */
int poptobuffer(int f, int n);				/* Other window to a buffer */
int killbuffer(int f, int n);				/* Make a buffer go away.   */
int savebuffers(int f, int n);				/* Save unmodified buffers  */
int bufferinsert(int f, int n);				/* Insert buffer into another   */

/*
 * Defined by "error.c".
 */
int errfwd(int f, int n);					/* Find an error forward    */

/*
 * Defined by "file.c".
 */
int filevisit(int f, int n);				/* Get a file, read write   */
int filewrite(int f, int n);				/* Write a file         */
int filesave(int f, int n);					/* Save current file        */
int fileinsert(int f, int n);				/* Insert file into buffer  */

/*
 * Defined by "line.c".
 */
int linsert(int n, int c);


/*
 * Defined by "match.c"
 */
int blinkparen(int f, int n);				/* Fake blink-matching-paren var */
int showmatch(int f, int n);				/* Hack to show matching paren   */

/*
 * Defined by "random.c".
 */
int selfinsert(int f, int n);				/* Insert character     */
int showcpos(int f, int n);					/* Show the cursor position */
int twiddle(int f, int n);					/* Twiddle characters       */
int quote(int f, int n);					/* Insert literal       */
int openline(int f, int n);					/* Open up a blank line     */
int newline(int f, int n);					/* Insert CR-LF         */
int deblank(int f, int n);					/* Delete blank lines       */
int delwhite(int f, int n);					/* Delete extra whitespace  */
int indent(int f, int n);					/* Insert CR-LF, then indent    */
int tab(int f, int n);						/* insert \t; or, set tabsize   */
int forwdel(int f, int n);					/* Forward delete       */
int backdel(int f, int n);					/* Backward delete in       */
int lbackdel(int f, int n);					/* line-backward-delete     */
int lbackchar(int f, int n);				/* line-backward-character  */
int lforwdel(int f, int n);					/* line-delete-next-character   */
int lforwchar(int f, int n);				/* line-forward-character   */
int killtext(int f, int n);					/* Kill forward         */
int yank(int f, int n);						/* Yank back from killbuffer.   */

/*
 * Defined by "region.c".
 */
int killregion(int f, int n);				/* Kill region.         */
int copyregion(int f, int n);				/* Copy region to kill buffer.  */
int lowerregion(int f, int n);				/* Lower case region.       */
int upperregion(int f, int n);				/* Upper case region.       */
int prefixregion(int f, int n);				/* Prefix all lines in region   */
int setprefix(int f, int n);				/* Set line prefix string   */

/*
 * Defined by "window.c".
 */
int reposition(int f, int n);				/* Reposition window        */
int refresh(int f, int n);					/* Refresh the screen       */
int nextwind(int f, int n);					/* Move to the next window  */
int prevwind(int f, int n);					/* Move to the previous window  */
int onlywind(int f, int n);					/* Make current window only one */
int splitwind(int f, int n);				/* Split current window     */
int delwind(int f, int n);					/* Delete current window    */
int enlargewind(int f, int n);				/* Enlarge display window.  */
int shrinkwind(int f, int n);				/* Shrink window.       */


/*
 * defined by "paragraph.c" - the paragraph justification code.
 */
int gotobop(int f, int n);					/* Move to start of paragraph.  */
int gotoeop(int f, int n);					/* Move to end of paragraph.    */
int fillpara(int f, int n);					/* Justify a paragraph.     */
int killpara(int f, int n);					/* Delete a paragraph.      */
int setfillcol(int f, int n);				/* Set fill column for justify. */
int fillword(int f, int n);					/* Insert char with word wrap.  */

/*
 * Defined by "word.c".
 */
int backword(int f, int n);					/* Backup by words      */
int forwword(int f, int n);					/* Advance by words     */
int upperword(int f, int n);				/* Upper case word.     */
int lowerword(int f, int n);				/* Lower case word.     */
int capword(int f, int n);					/* Initial capitalize word. */
int delfword(int f, int n);					/* Delete forward word.     */
int delbword(int f, int n);					/* Delete backward word.    */

/*
 * Defined by "extend.c".
 */
int extend(int f, int n);					/* Extended commands.       */
int desckey(int f, int n);					/* Help key.            */
int bindtokey(int f, int n);				/* Modify key bindings.     */
int unsetkey(int f, int n);					/* Unbind a key.        */
int wallchart(int f, int n);				/* Make wall chart.     */

/*
 * defined by prefix.c
 */
int help(int f, int n);						/* Parse help key.      */
int ctlx4hack(int f, int n);				/* Parse a pop-to key.      */

int escctrld(int f, int n);
int escesc(int f, int n);
int miniterm(int f, int n);
int filename(int f, int n);
int gxpand(int f, int n);
int gxpshow(int f, int n);
int togulambuf(int f, int n);
int execbuf(int f, int n);
int fileread(int f, int n);
int showkbdmacro(int f, int n);
int temul(int f, int n);
int totebuf(int f, int n);
int tenewline(int f, int n);
int tesendtext(int f, int n);
int mvupwind(int f, int n);
int mvdnwind(int f, int n);
int quickexit(int f, int n);
int tempexit(int f, int n);
int gforwline(int f, int n);
int gulam(int f, int n);
int quit(int f, int n);
int gspawn(int f, int n);
int semireset(int f, int n);
int killtobln(int f, int n);
int getarg(int f, int n);
int metanext(int f, int n);
int ctlxnext(int f, int n);
int keyreset(int f, int n);					/* keymap.c */
