/*
	malloc.c -- provides C stdlib routines malloc(), free().

	(c) copyright 1987 pm@cwru.edu
*/
#undef DEBUG

#include "ue.h"

#define max(a,b) ((a > b)? a : b)

static int gflag = 0;

#if PMMALLOC

typedef unsigned int UI;
typedef unsigned long UL;

typedef struct MCB						/* Note 1:              */
{
	UI msize;							/* This UI limits malloc(x) requests to */
	struct MCB *next;					/* 0 <= x <= 0xFFFE         */
} MCB;									/* Change to unigned long, if you wish  */

/* The list of free MCBs of heap is always in the ascending order of
their addresses.  */

typedef struct HEAP
{
	struct HEAP *hlink;					/* must be the 1st field */
	MCB fmcb;
	UL hsize;
} HEAP;


static HEAP rheaplist = { (HEAP *) NULL };
static HEAP gheaplist = { (HEAP *) NULL };


/* The list of heaps is in descending order of their addresses */

#define HEAPSIZE 16384 - ((uint)sizeof(HEAP))	/* min size of each heap    */
#define MXWASTE  16						/* split MCB if waste is > than this    */
				/* must be >= sizeof(MCB)       */
#define	MNMLC	 ((uint)sizeof(UL))		/* min value of malloc'd areas      */
#define	MI	 ((uint)sizeof(UI))			/* sizeof msize field of MCB        */

#ifndef	DEBUG

#define	check(x,y,z)					/* empty */
#define checkheaps(i)					/* empty */

#else

#define	xszM	4000
int xsz = 0;
UI isz[xszM];
UL psz[xszM];


static check(HEAP *heap, char *s, MCB *mcb)
{
	MCB *p;
	MCB *r;
	int bad;

	bad = 0;
	for (r = &heap->fmcb, p = r->next; p;)
	{
		if (r->msize < MNMLC)
		{
			gputs(sprintp("bad %D heap mcb %D has msize %d\r\n", heap, r, r->msize));
			bad++;
			break;
		}
		if ((UL) p <= (UL) r + (UL) r->msize)
		{
			gputs(sprintp("bad %D heap %D <= %D + %d\r\n", heap, p, r, r->msize));
			bad++;
			break;
		}
		if ((UL) p <= (UL) heap)
		{
			gputs(sprintp("bad heap %D >= %D\r\n", heap, p));
			bad++;
			break;
		}
		r = p;
		p = p->next;
	}
	if (bad)
		gputs(sprintp("--> %s mcb %D mcb sz %d\r\n", s, mcb, mcb->msize));
	return bad;
}

checkheaps(flag)
{
	HEAP *heap;
	MCB *m;

	heap = (flag ? &gheaplist : &rheaplist);
	for (heap = heap->hlink; heap; heap = heap->hlink)
	{
		m = (&heap->fmcb);
		m = m->next;
		check(heap, "chk r heap", m);
	}
}

#endif


static void lstchunks(HEAP *heap)
{
	MCB *p;

	for (heap = heap->hlink; heap; heap = heap->hlink)
	{
		gputs(sprintp("heap at %D size %D\r\n", heap, heap->hsize));
		for (p = heap->fmcb.next; p; p = p->next)
			gputs(sprintp("  chunk at %D size %d\r\n", p, p->msize + MI));
	}
}

void showgumem(void)
{
	lstchunks(&gheaplist);
	gputs("---\r\n");
	lstchunks(&rheaplist);
}

/* Insert q into the list of free MCBs of heap.  This list is always
in the ascending order of their addresses.  If possible, coalesce the
new MCB q with its (left and/or right) adjacent MCBs. */

static void insert(HEAP *heap, MCB *q)
{
	MCB *p;
	MCB *r;

	for (r = &heap->fmcb, p = r->next; p && p < q; r = p, p = p->next)
		;
	check(heap, "bef freeins", r);		/*dbg */
	r->next = q;
	q->next = p;
	if ((UL) r + r->msize + MI == (UL) q)
	{
		r->next = p;
		r->msize += q->msize + MI;
		q = r;
	}
	if ((UL) q + q->msize + MI == (UL) p)	/* p can be NULL */
	{
		q->next = p->next;
		q->msize += p->msize + MI;
	}
	check(heap, "aft freeins", r);		/*dbg */
}

/* Get another heap from GEMDOS */

static HEAP *allocheap(UI x)
{
	HEAP *heap;
	UL lx;

	lx = x;
	lx += ((uint) sizeof(HEAP)) + MI;
	heap = (HEAP *) Malloc(lx);
	if (heap == NULL)
		return NULL;
	{									/* insert heap into heap list, which is in descending order of & */
		HEAP *h;
		HEAP *g;

		for (h = (gflag ? &gheaplist : &rheaplist), g = h->hlink; g > heap; h = g, g = g->hlink)
			;
		h->hlink = heap;
		heap->hlink = g;
	}
	{									/* initialize the fields of the heap, and the 1st MCB */
		MCB *m;

		heap->hsize = lx;
		heap->fmcb.msize = ((uint) sizeof(UL));	/* almost any # will do as long as */
		/* fmcb.next + fmcb.msize + MI never == m */
		heap->fmcb.next = m = (MCB *) (&heap[1]);
		m->msize = (UI) (lx - ((uint) sizeof(HEAP)) - MI);
		m->next = NULL;
	}
	return heap;
}


void *malloc(size_t x)
{
	MCB *mcb;
	MCB *m;
	HEAP *heap;
	UI n;
	int ret;

	if (x == (UI) 0xFFFF)
		return NULL;
	if (++x < MNMLC)
		x = MNMLC;
	x &= 0xFFFE;
	heap = (gflag ? &gheaplist : &rheaplist);
	ret = 0;
	for (heap = heap->hlink; heap; heap = heap->hlink)
	{
	  getchunk:						/* below does this: 'return getchunk(heap, x))' */

		for (m = &heap->fmcb, mcb = m->next; mcb && mcb->msize < x; m = mcb, mcb = mcb->next)
			;
		if (mcb)						/* m->next == mcb */
		{
			check(heap, "bef getchunk", mcb);	/*dbg */
			n = (UI)(mcb->msize - x);			/* mcb->msize - x will be >= 0 */
			if (n > MXWASTE)			/* remaining area is large enough */
			{
				m->next = (MCB *) ((UL) mcb + x + MI);
				m = m->next;
				m->msize = n - MI;
				mcb->msize = (UI)x;
			}
			m->next = mcb->next;		/* del from heap   */
			check(heap, "aft getchunk", m);	/*dbg */
			return (char *)mcb + MI;
		}
		if (ret)
			return NULL;				/* ret == 0 in the for-loop */
	}

	if ((heap = allocheap(((UI) (max(x, HEAPSIZE))))) != NULL)
		/* if (p = getchunk(heap, x)) return p; */
	{
		ret = 1;
		goto getchunk;
	}

	return NULL;
}

void free(void *_mcb)
{
	MCB *mcb = _mcb;
	HEAP *heap;
	HEAP *h;

	if (mcb == NULL)
		return;
	mcb = (MCB *) ((UL) mcb - MI);
	{									/* compute the heap that this mcb belongs to */
		for (h = (gflag ? &gheaplist : &rheaplist), heap = h->hlink; heap; h = heap, heap = heap->hlink)
		{
			if ((UL) mcb > (UL) heap && (UL) mcb < (UL) heap + heap->hsize)
				break;
		}
		if (heap == NULL)
		{
			gputs(sprintp("free(%D) has illegal arg\r\n", (UL) mcb + MI));
			getkey();
			return;
		}
	}
	insert(heap, mcb);
	{									/* GEMDOS Mfree() if this heap is now unused */
		if ((mcb = heap->fmcb.next) != NULL && (UL) mcb->msize + (UL) ((uint) sizeof(HEAP)) + (UL) MI == heap->hsize)
		{
			h->hlink = heap->hlink;
			Mfree(heap);
		}
	}
}

/* Gulam uses two independent heap lists to reduce memory
fragmentation caused by the markedly different expected life times of
the malloc'd areas: gheaplist for use in the shell part, and rheaplist
in the uE part.  */

#endif

void *gmalloc(unsigned int n)
{
	char *p;

	gflag = 1;
	p = malloc(n);
	gflag = 0;
	return p;
}

int gfree(void *p)
{
	gflag = 1;
	free(p);
	gflag = 0;
	return 0;
}

/* Fast freeing of all rheaplist heaps.  Used by uE when exiting cleanly
back to the single line oriented shell mode. */

void freeall(void)
{
#if PMMALLOC
	HEAP *heap;
	HEAP *h;

	for (heap = &rheaplist; (h = heap->hlink) != NULL;)
	{
		heap->hlink = h->hlink;
		Mfree(h);
	}
#endif
}
