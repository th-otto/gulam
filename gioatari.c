/*
 * $Header: f:/src/gulam\RCS\gioatari.c,v 1.1 1991/09/10 01:02:04 apratt Exp $
 * ======================================================================
 * $Log: gioatari.c,v $
 * Revision 1.1  1991/09/10  01:02:04  apratt
 * First CI of AKP
 *
 * Revision: 1.5 90.10.11.10.40.14 apratt 
 * Added 'extern' declarations for Font[] and Mdmport[].
 * 
 * Revision: 1.4 90.02.13.15.09.42 apratt 
 * Added setmdmport(), in response to "set mdmport X" -- calls
 * Bconmap().  This is far from bulletproof, esp. if TE has already
 * been initialized - the larger buffer was allocated for the original
 * mapping, and the new mapping is stuck with the ROM buffer & baud rate.
 * Gulam needs to know about this in more detail.
 * 
 * Revision: 1.3 90.02.05.15.18.36 apratt 
 * Added printing Scrninit before changing fonts because changing
 * fonts with cursor visible is dangerous.  Assumes somebody
 * downstream (vtinit?) will enable the cursor again.
 * 
 * Added handler font() for variable Font ("font") -- sets
 * 8x8, 8x10, or 8x16 font in any rez, sets nrows accordingly,
 * and calls vtinit like nrows does.
 * 
 * Revision: 1.2 89.09.15.15.03.18 apratt 
 * Added TT resolutions to "drawshadedrect" which "gem" prefix calls.
 * 
 */

/*
	gioatari.c of Gulam/uE -- screen display and key board interface

	copyright (c) 1987 pm@cwru.edu
*/

#include "ue.h"
#include "keynames.h"

extern int __mint;


uchar Scrninit[] = "\033E\033f\033v";	/* clr scrn, cursr off, wordwrap on */

/* initialize the screen display and key board	*/

void tioinit(void)
{
	mouseoff(NULL);
	gputs(Scrninit);
	mouseon(NULL);
}



/* N.B.  \033q is sent more often than should be necessary because the
ST remains in reverse video, in some situations, even after receiving
a \033q.  */

#define	SZinc	4*256
static KEY inc[SZinc];				/* storage for typeahead keys   */
static int inx = 0;						/* circular buffer is overkill  */

static void charinc(KEY c)
{
	if (inx < SZinc - 1)
		inc[inx++] = c;
}

/* Store the string p as if the chars are typed ahead */

void storekeys(KEY *p)
{
	if (p)
		while (*p)
			charinc(*p++);
}


/*
 * translate the input key to a uE key
 */
static KEY translatekey(long l)
{
	KEY i;
	short s;

	i = (KEY)l & 0xff;
	switch ((int)(l >> 16) & 0xff)
	{
	case 0x3b: i = F1; break;
	case 0x3c: i = F2; break;
	case 0x3d: i = F3; break;
	case 0x3e: i = F4; break;
	case 0x3f: i = F5; break;
	case 0x40: i = F6; break;
	case 0x41: i = F7; break;
	case 0x42: i = F8; break;
	case 0x43: i = F9; break;
	case 0x44: i = F10; break;
	case 0x54: i = F1 | SHIFTED; break;
	case 0x55: i = F2 | SHIFTED; break;
	case 0x56: i = F3 | SHIFTED; break;
	case 0x57: i = F4 | SHIFTED; break;
	case 0x58: i = F5 | SHIFTED; break;
	case 0x59: i = F6 | SHIFTED; break;
	case 0x5a: i = F7 | SHIFTED; break;
	case 0x5b: i = F8 | SHIFTED; break;
	case 0x5c: i = F9 | SHIFTED; break;
	case 0x5d: i = F10 | SHIFTED; break;
	case 0x61: i = UNDO; break;
	case 0x62: i = HELP; break;
	case 0x52: i = INSERT; break;
	case 0x53: i = DELETE; break;
	case 0x47: i = HOME; break;
	case 0x48: i = UPARRO; break;
	case 0x4b: i = LTARRO; break;
	case 0x4d: i = RTARRO; break;
	case 0x50: i = DNARRO; break;
	case 0x4a: i = KMINUS; break;
	case 0x4e: i = KPLUS; break;
	case 0x63: i = KLP; break;
	case 0x64: i = KRP; break;
	case 0x65: i = KSLASH; break;
	case 0x66: i = KSTAR; break;
	case 0x67: i = K7; break;
	case 0x68: i = K8; break;
	case 0x69: i = K9; break;
	case 0x6a: i = K4; break;
	case 0x6b: i = K5; break;
	case 0x6c: i = K6; break;
	case 0x6d: i = K1; break;
	case 0x6e: i = K2; break;
	case 0x6f: i = K3; break;
	case 0x70: i = K0; break;
	case 0x71: i = KDOT; break;
	case 0x72: i = KENTER; break;
	case 0x73: i = LTARRO | CTRL; break;
	case 0x74: i = RTARRO | CTRL; break;
	case 0x77: i = HOME | CTRL; break;
	default:
		return i;
	}
	s = Kbshift(-1);
	if (s & 0x03)
		i |= SHIFTED;
	return i;
}


/* Return the next key pressed by the user; it is in inc if inx > 0 */

KEY inkey(void)
{
	KEY i;

	if (inx > 0)
	{
		i = inc[0];
		--inx;
		cpymem(inc, inc + 1, inx * (int)sizeof(inc[0]));	/* inx is very small */
	} else
	{
		long l;
		
#ifndef	TEB
		l = ggetchar();
#else
	  checkagain:
		if (inkbdrdy())
			l = ggetchar();
		else
		{
			teupdate();
			goto checkagain;
		}
#endif
		i = translatekey(l);
	}
	return i;
}

#ifdef DEBUG

void showinc(void)
{
	inc[inx] = '\0';
	mlwrite("inx %d :%s:\r\n", inx, inc);
}
#endif

/* Give the latest key pressed by the user.  Used for job control such
as ^C, ^G, ^S and ^Q.  So always do constat and conin.  Save the key
if this is a typeahead.  */

KEY usertyped(void)
{
	long l;
	KEY c;

	c = 0;
	if (inkbdrdy())
	{
		l = ggetchar();
		c = translatekey(l);
		if (c != CTRLC && c != CTRLG && c != CTRLS && c != CTRLQ)
			charinc(c);
	}
	return c;
}

/* Return TRUE iff user typed a ^C or ^G */

int useraborted(void)
{
	KEY c;

	c = usertyped();
	return c == CTRLC || c == CTRLG;
}

/* Output string to screen; similar to Cconws, but should not treat ^C
as an exception.  */

void gputs(const char *s)
{
	int c;

	while ((c = *s++) != 0)
		gputchar(c);
}

/* Clean up the virtual terminal system, in anticipation for a return
to the operating system.  Move down to the last line and clear it out
(the next system prompt will be written in the line).  Shut down the
channel to the terminal.  */

void vttidy(void)
{
	mvcursor(getnrow() - 1, 0);
	gputs("\033K");
}


void mvcursor(int row, int col)
{
	static char es[5] = "\033Yrc";

	es[2] = (char) row + 32;
	es[3] = (char) col + 32;
	gputs(es);
}


/* 
 * added "gputs(Scrninit)" here because changing fonts when the cursor
 * is visible is dangerous.  Scrninit makes it invisible; 
 * vtinit makes it visible again.
 */

void nrow2550(void)								/* called from tv.c via a set var   */
{
	int n;

	if (varnum(OwnFonts) == 0)
	{
		if (Getrez() != 2)
			return;						/* Not in high rez */
		n = varnum(Nrows);
		switch (n)
		{
		case 50:
			gputs(Scrninit);
			hi50();
			break;
		case 40:
			gputs(Scrninit);
			hi40();
			break;
		case 25:
			gputs(Scrninit);
			hi25();
			break;
		}
	}
	vtinit();
	onlywind(1, 1);
}

/*
 * extension by AKP... This sets nrows to the return from the
 * appropriate font-setting routine, then calls vtinit etc. as above.
 */

void font(void)
{
	int n;

	if (varnum(OwnFonts) == 0)
	{
		n = varnum(Font);
		switch (n)
		{
		case 8:
			gputs(Scrninit);
			setvarnum(Nrows, font8());
			break;
		case 10:
			gputs(Scrninit);
			setvarnum(Nrows, font10());
			break;
		case 16:
			gputs(Scrninit);
			setvarnum(Nrows, font16());
			break;
		}
	}
	vtinit();
	onlywind(1, 1);
}

/*
 * extension by AKP... This uses Bconmap to set the port to be used
 * by Bcon calls on device 1 ("AUX"), Rsconf, and Iorec calls.
 * By default (in ROM) it's the ST MFP port.
 */

void setmdmport(void)
{
	Bconmap(varnum(Mdmport));		/* 43 is Bconmap() */
}

#ifdef DEBUG

static char *showpallete(void)
{
	int i;
	int j;
	int n;
	char *p;

	char bf[5];
	WS *ws;

	bf[3] = '-';
	bf[4] = '\0';
	ws = initws();
	strwcat(ws, "palette was ", 0);
	for (i = 0; i < 16; i++)
	{
		n = Setcolor(i, -1);
		for (j = 3; j;)
		{
			bf[--j] = hex[n & 0xF];
			n >>= 4;
		}
		strwcat(ws, bf, 0);
	}
	if (ws)
		p = ws->ps;
	gfree(ws);
	return p;
}
#endif


void pallete(void)
{
	char *p;
	char *q;
	int i;
	static int pal[16];

	/* pal has to be static, not auto; Setpallete does not work
	   othwerwise, because it takes effect at next Vsync()
	 */

	p = varstr("rgb");
	if (strlen(p) < 3)
		p = "006-770-707-070-";		/* default colors */
	q = p + strlen(p) - 3;
	for (i = 0; i < 16; p += 4)
	{
		pal[i] = (p < q ? (p[0] - '0') * 256 + (p[1] - '0') * 16 + (p[2] - '0') : 0);
		i++;
	}
	Setpalette(pal);
}

static struct r
{
	int p0, p1;
	int x1, y1;
	int c0, c1;
} rp[8] = {
	{ 0xFFFF, 0xFFFF, 0,  8, 0, 1 }, /* ST-Low */
	{ 0xFFFF, 0xFFFF, 0,  8, 0, 1 }, /* ST-Med */
	{ 0xAAAA, 0x5555, 0, 16, 1, 1 }, /* ST-High */
	{ 0, 0, 0, 0, 0, 0 },            /* Falcon modes */
	{ 0xAAAA, 0x5555, 0, 16, 0, 1 }, /* TT-Med */
	{ 0, 0, 0, 0, 0, 0 },
	{ 0xAAAA, 0xFFFF, 0, 32, 1, 1 }, /* TT-High */
	{ 0xFFFF, 0xFFFF, 0,  8, 0, 1 }  /* TT-Low */
};

/* This thing draws the gray-shaded rectangle that GEM-oriented programs
assume as their background. */

#ifdef __PUREC__
#include <linea.h>
#else
#include <mint/linea.h>
#endif

void drawshadedrect(void)
{
	unsigned int rez;
	unsigned short pattern[2];

	lineA();
	rez = Getrez();
	if (rez == 3 || rez == 5 || rez >= 8)
		return;
	
	pattern[0] = rp[rez].p0;
	pattern[1] = rp[rez].p1;

#ifdef __PUREC__
	Linea->wrt_mode = 0;
	Linea->fg_bp_1 = rp[rez].c0;
	Linea->fg_bp_2 = rp[rez].c1;
	Linea->fg_bp_3 = 0;
	Linea->fg_bp_4 = 0;
	Linea->clip = 0;
	Linea->patmsk = 1;
	Linea->patptr = &pattern[0];
	Linea->x1 = rp[rez].x1;
	Linea->y1 = rp[rez].y1;
	Linea->x2 = Vdiesc->v_rez_hz - 1;
	Linea->y2 = Vdiesc->v_rez_vt - 1;
	filled_rect(Linea->x1, Linea->y1, Linea->x2, Linea->y2);
#else
	WMODE = 0;							/* replace */
	COLBIT0 = rp[rez].c0;
	COLBIT1 = rp[rez].c1;
	COLBIT2 = COLBIT3 = 0;
	CLIP = 0;
	PATMSK = 1;
	MFILL = 0;
	PATPTR = (void *)&pattern[0];
	X1 = rp[rez].x1;
	Y1 = rp[rez].y1;
	X2 = V_X_MAX - 1;
	Y2 = V_Y_MAX - 1;

	linea5();
#endif
}

/* Turn mouse movements into cursor keys */

void mousecursor(void)
{
	static char defikbdcmd[3] = { 0x0a, 0x07, 0x07 };	/* fast enough */
	static char usrikbdcmd[3] = { 0x0a };
	char *p;

#ifdef __MINT__
	if (__mint)
		return;
#endif
	if ((p = varstr(Mscursor)) != NULL && *p)
	{
		usrikbdcmd[1] = (p[0] - '0') * 16 + p[1] - '0';
		usrikbdcmd[2] = (p[2] - '0') * 16 + p[3] - '0';
		p = usrikbdcmd;
	} else
	{
		p = defikbdcmd;
	}
	if (p[1] && p[2])
		Ikbdws(2, p);
}

/* Turn mouse back to its normal operation (relative coords) */

void mouseregular(void)
{
#ifdef __MINT__
	if (__mint)
		return;
#endif
	Ikbdws(0, "\010");					/* Ikbdws(0, put mouse back to relative) */
}


void keysetup(void)
{
	/* messing with keytables removed */
}


int keyreset(int f, int n)								/* should reset to the table we had */
{										/* prior to entering uE         */
	UNUSED(f);
	UNUSED(n);
	/* messing with keytables removed */
	return TRUE;
}
