/*	
 *	Gulam support routines for GCC
 *	  ++jrb
 */
#include "ue.h"

#ifdef __MINT__
#include <mintbind.h>
#include <sys/ioctl.h>
extern int __mint;
#endif

#ifndef TIOCGWINSZ
#define TIOCGWINSZ	(('T'<< 8) | 11)
struct winsize {
	short	ws_row;
	short	ws_col;
	short	ws_xpixel;
	short	ws_ypixel;
};
#endif


#ifdef __PUREC__
#include <linea.h>
/* map Pure-C names to GNU-C ones */
#  define linea0() linea_init()
#  define lineaa() hide_mouse()
#  define linea9() show_mouse(0)
#  define __FONT FONT_HDR
#  define __fonts Fonts->font
#  define __aline Linea
#  define V_OFF_AD Vdiesc->v_off_ad
#  define V_FNT_AD Vdiesc->v_fnt_ad
#  define V_CEL_HT Vdiesc->v_cel_ht
#  define V_CEL_MX Vdiesc->v_cel_mx
#  define V_CEL_MY Vdiesc->v_cel_my
#  define V_CEL_WR Vdiesc->v_cel_wr
#  define V_BYTES_LIN Vdiesc->bytes_lin
#  define V_Y_MAX Vdiesc->v_rez_vt
#  define CONTRL Linea->contrl
#  define INTIN Linea->intin
#  define VPLANES Linea->v_planes
#else
#include <mint/linea.h>
#endif


static unsigned long fnt_8x10_data[640];
struct _fonthdr {
   short  font_id;
   short  size;
   char   name[32];
   short  first_ade;
   short  last_ade;
   short  top;
   short  ascent;
   short  half;
   short  descent;
   short  bottom;
   short max_char_width;
   short max_cell_width;
   short left_offset;
   short right_offset;
   short  thicken;
   short  ul_size;
   short  lighten;
   short  skew;
   short  flags;
   char   *h_table;
   short  *off_table;
   char   *dat_table;
   short  form_width;
   short  form_height;
   struct _fonthdr *next_font;
};
static struct _fonthdr fnt_8x10;
static void *orig_fnt_ad;
static void *orig_off_ad;
static short orig_form_height;

/*
 * return base addr of lineA vars
 */
short *lineA(void)
{
	(void) linea0();
	return (short *) __aline;
}


static void switch_font(struct _fonthdr *f)
{
	if (orig_fnt_ad == 0)
	{
		orig_fnt_ad = V_FNT_AD;
		orig_off_ad = V_OFF_AD;
		orig_form_height = V_CEL_HT;
	}

	V_OFF_AD = (void *) f->off_table;	/* v_off_ad <- 8x8 offset tab addr */
	V_FNT_AD = (void *) f->dat_table;	/* v_fnt_ad <- 8x10 font data addr    */
	V_CEL_HT = f->form_height;			/* v_cel_ht <- 10   8x10 cell height    */

	V_CEL_MX = (V_BYTES_LIN / VPLANES) - 1;
	V_CEL_MY = (V_Y_MAX / f->form_height) - 1;			/* v_cel_my <- 39   maximum cell "Y"    */
	V_CEL_WR = V_BYTES_LIN * f->form_height;			/* v_cel_wr <- 800  offset to cell Y+1  */
}


/*
 * Make hi rez screen bios handle 50 lines of 8x8 characters.  Adapted
 * from original asm posting. look ma, no asm!
 */
/* ++ers: if under MiNT, don't do this */
void hi50(void)
{
	struct _fonthdr *f8x8;

#ifdef __MINT__
	if (__mint)
		return;
#endif
	lineA();
	f8x8 = (struct _fonthdr *)__fonts[1];					/* 8x8 font header          */
	switch_font(f8x8);
}


/*
 * Make hi rez screen bios handle 40 lines of 8x8 characters.  Adapted
 * from original asm posting.
 */
void hi40(void)
{
	struct _fonthdr *f8x8;

#ifdef __MINT__
	if (__mint)
		return;
#endif

	/*  make 8x8 font into 8x10 font */
	memset(fnt_8x10_data, 0, sizeof(fnt_8x10_data));	/* 640 = size of 8x10 fnt; longs */

	lineA();
	f8x8 = (struct _fonthdr *)__fonts[1];					/* 8x8 font header          */
	fnt_8x10 = *f8x8;
	memcpy(&fnt_8x10_data[64], f8x8->dat_table, 512 * 4L);	/* copy 8x8 fnt data
														   starting at second line of 8x10 fnt */
	fnt_8x10.dat_table = (void *)fnt_8x10_data;
	fnt_8x10.form_height = 10;
	switch_font(&fnt_8x10);
}


/*
 * Make hi rez screen bios handle 25 lines of 8x16 characters
 */
void hi25(void)
{
	struct _fonthdr *f8x16;

#ifdef __MINT__
	if (__mint)
		return;
#endif

	lineA();
	f8x16 = (struct _fonthdr *)__fonts[2];					/* 8x16 font header             */
	switch_font(f8x16);
}

/*
 * show/activate the mouse
 */
void mouseon(uchar *arg)
{
	UNUSED(arg);
#ifdef __MINT__
	if (__mint)
		return;
#endif
	lineA();
	if (!CONTRL)
		return;
	CONTRL[1] = 0;
	CONTRL[3] = 1;
	INTIN[0] = 0;
	(void) linea9();
	mouseregular();
}


void mouseoff(uchar *arg)
{
	UNUSED(arg);
#ifdef __MINT__
	if (__mint)
		return;
#endif
	lineA();
	(void) lineaa();					/* mouse lineA call */
	mousecursor();
}


/*
 * getnrow() -- get number of rows.
 */
int getnrow(void)
{
	struct winsize ws;

	if (Fcntl(0, (long)&ws, TIOCGWINSZ) >= 0)
		return ws.ws_row;
	lineA();
	return V_CEL_MY + 1;
}

/*
 * getncol() -- get number of columns.
 */
int getncol(void)
{
	struct winsize ws;

	if (Fcntl(0, (long)&ws, TIOCGWINSZ) >= 0)
		return ws.ws_col;
	lineA();
	return V_CEL_MX + 1;
}


/*
 * Extension by AKP: "set font 8" now means "use the 8x8 font"
 * and RETURNS whatever number of lines that yields (for setting nrows)
 *
 * "set font 16" means "use the 8x16 font," and "set font 10" means
 * "use the 8x8 font with two extra blank lines."
 * Again, these return the actual number of rows you get.
 */

int font8(void)
{
	struct _fonthdr *f8x8;

#ifdef __MINT__
	if (__mint)
		return getnrow();
#endif

	lineA();
	f8x8 = (struct _fonthdr *)__fonts[1];					/* 8x8 font header          */
	switch_font(f8x8);
	return V_CEL_MY + 1;
}

/*
 *  make 8x8 font into 8x10 font and make it current; return new nrows
 */
int font10(void)
{
	struct _fonthdr *f8x8;

#ifdef __MINT__
	if (__mint)
		return getnrow();
#endif

	/*  make 8x8 font into 8x10 font */
	memset(fnt_8x10_data, 0, sizeof(fnt_8x10_data));	/* 640 = size of 8x10 fnt; longs */
	
	lineA();
	f8x8 = (struct _fonthdr *)__fonts[1];					/* 8x8 font header          */
	memcpy(&fnt_8x10_data[64], f8x8->dat_table, 512 * 4L);	/* copy 8x8 fnt data
														   starting at second line of 8x10 fnt */
	fnt_8x10 = *f8x8;
	fnt_8x10.dat_table = (void *)fnt_8x10_data;
	fnt_8x10.form_height = 10;
	
	switch_font(&fnt_8x10);
	return V_CEL_MY + 1;
}


/*
 * Switch to 8x16 font and return new nrows
 */
int font16(void)
{
	struct _fonthdr *f8x16;

#ifdef __MINT__
	if (__mint)
		return getnrow();
#endif

	lineA();
	f8x16 = (struct _fonthdr *)__fonts[2];					/* 8x16 font header             */
	switch_font(f8x16);
	return V_CEL_MY + 1;
}


void fontreset(void)
{
	if (orig_fnt_ad != 0)
	{
		V_OFF_AD = (void *) orig_off_ad;
		V_FNT_AD = (void *) orig_fnt_ad;
		V_CEL_HT = orig_form_height;
		V_CEL_MX = (V_BYTES_LIN / VPLANES) - 1;
		V_CEL_MY = (V_Y_MAX / orig_form_height) - 1;
		V_CEL_WR = V_BYTES_LIN * orig_form_height;
		orig_fnt_ad = 0;
	}
}


#if 0
/*
/ Revector trap 1 and 13 to do stdout capture into Gulam buffer
/
/	.prvd
/savea7:	.long	0
/
/	.shri
/	.globl	gconout_, gcconws_, gfwrite_, oldp1_, oldp13_
/	.globl	mytrap1_, mytrap13_
/
// revector trap 13 to our own to handle Bconout(2, x)
//
/mytrap13_:
/	move.l	a7,savea7
/	move.l	usp, a7
/        move.w	(a7)+, d0	/ Bconout(2, c) == trap13(3, 2, c)
/	cmpi.w	$3,d0
/	bne	1f
/3:	move.w	(a7)+,d0
/	cmpi.w	$2, d0
/	bne	1f
/	jsr	gconout_	/ c is still on stack
/ 	movea.l	savea7,a7
/	rte
/1:	movea.l	savea7,a7
/	movea.l	oldp13_,a0
/ 	jmp	(a0)
/
// Revector trap 1 to handle Cconout, Cconws, and Fwrite(1,..) calls
//
/mytrap1_:
/	move.l	a7,savea7
/	move.l	usp, a7
/        move.w	(a7)+, d0
/	cmpi.w	$2,d0		/ Cconout(c) == trap1(0x2, c)
/	beq	2f
/	cmpi.w	$9,d0		/ Cconws(s) == trap1(0x9, s)
/	beq	9f
/	cmpi.w	$0x40,d0	/ Fwrite(1, ll, bf) == trap1(0x40, 1, ll, bf)
/ 	beq	4f
/1:	movea.l	savea7,a7
/	movea.l	oldp1_,a0
/	jmp	(a0)
/9:	jsr	gcconws_	/ s is still on stack
/	bra	0f
/4:	move.w	(a7)+,d0	/ d0 == 1 ?
/	cmpi.w	$1,d0
/	bne	1b
/	jsr	gfwrite_	/ gfwrite(ll, bf) long ll; char *bf;
/	bra	0f
/2:	jsr	gconout_
/0: 	movea.l	savea7,a7
/	rte
/
*/
#endif
