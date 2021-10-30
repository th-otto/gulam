/*
 keynames.h of ue/Gulam			 01/18/86

 Copyright (c) 1986 pm@Case

 This file defines ST520 keynames; the values are the ascii codes returned
 by ST BIOS.  The 'key-to-ascii' mapping set up is given in file keymap.c
*/


#define CTRL    0x0100                  /* Control flag, or'ed in       */
#define META    0x0200                  /* Meta flag, or'ed in          */
#define CTLX    0x0400                  /* ^X flag, or'ed in            */
#define KFUNC   0x0800

/*	The values below are chosen with certain patterns:
	to SHIFT	add 0040
	F1-10		0201 to 0212
	keypad 0-9	0260 to 0272 (subtract -0200 to get ascii digits)
	keypad()/ *-+.cr	add 0300 to the ascii ()/ *-+.cr

	Other choices may be better, I don't know.  However, I think
	it is unwise to use numbers in the range 0000 to 0177, because
	they are regular ascii codes.

*/
#define F1	(KFUNC+0x0001)
#define F2  (KFUNC+0x0002)
#define F3  (KFUNC+0x0003)
#define F4	(KFUNC+0x0004)
#define F5	(KFUNC+0x0005)
#define F6  (KFUNC+0x0006)
#define F7	(KFUNC+0x0007)
#define F8	(KFUNC+0x0008)
#define F9	(KFUNC+0x0009)
#define F10	(KFUNC+0x000a)

#define HELP    (KFUNC+0x000b)
#define UNDO	(KFUNC+0x000c)
#define INSERT	(KFUNC+0x000d)
#define HOME	(KFUNC+0x000e)
#define DELETE  0x7f

#define	K0	(KFUNC+0x00b0)
#define	K1	(KFUNC+0x00b1)
#define	K2	(KFUNC+0x00b2)
#define	K3	(KFUNC+0x00b3)
#define	K4	(KFUNC+0x00b4)
#define	K5	(KFUNC+0x00b5)
#define	K6	(KFUNC+0x00b6)
#define	K7	(KFUNC+0x00b7)
#define	K8	(KFUNC+0x00b8)
#define	K9	(KFUNC+0x00b9)

#define UPARRO	(KFUNC+0x00bb)
#define DNARRO	(KFUNC+0x00bc)
#define LTARRO	(KFUNC+0x00bd)
#define RTARRO	(KFUNC+0x00be)

#define	KLP     (KFUNC+0x00c0)
#define	KRP     (KFUNC+0x00c1)
#define	KSTAR	(KFUNC+0x00c2)
#define	KPLUS	(KFUNC+0x00c2)
#define	KENTER	(KFUNC+0x00c3)
#define	KMINUS	(KFUNC+0x00c4)
#define	KDOT	(KFUNC+0x00c5)
#define	KSLASH	(KFUNC+0x00c6)


/*
	The above are the unshifted values.  Shifted, as well as 'capslocked'
	(for only these keys) values that I use are these +0040.
*/
#define SHIFTED 0x0020
