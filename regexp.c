/*
 *	Copyright (c) 1986 by University of Toronto.
 *	Written by Henry Spencer.  Not derived from licensed software.
 */

#include "regexp.h"
#include "gu.h"

#include <string.h>
#include <stddef.h>

#define malloc(x) gmalloc(x)


/* definition	number	opnd?	meaning */
#define	END	0							/* no   End of program. */
#define	BOL	1							/* no   Match "" at beginning of line. */
#define	EOL	2							/* no   Match "" at end of line. */
#define	ANY	3							/* no   Match any one character. */
#define	ANYOF	4						/* str  Match any character in this string. */
#define	ANYBUT	5						/* str  Match any character not in this string. */
#define	BRANCH	6						/* node Match this alternative, or the next... */
#define	BACK	7						/* no   Match "", "next" ptr points backward. */
#define	EXACTLY	8						/* str  Match this string. */
#define	NOTHING	9						/* no   Match empty string. */
#define	STAR	10						/* node Match this (simple) thing 0 or more times. */
#define	PLUS	11						/* node Match this (simple) thing 1 or more times. */
#define	OPEN	20						/* no   Mark this point in input as start of #n. */
			/*  OPEN+1 is number 1, etc. */
#define	CLOSE	30						/* no   Analogous to OPEN. */

/*
 * Opcode notes:
 *
 * BRANCH	The set of branches constituting a single choice are hooked
 *		together with their "next" pointers, since precedence prevents
 *		anything being concatenated to any individual branch.  The
 *		"next" pointer of the last BRANCH in a choice points to the
 *		thing following the whole choice.  This is also where the
 *		final "next" pointer of each individual branch points; each
 *		branch starts with the operand node of a BRANCH node.
 *
 * BACK		Normal "next" pointers all implicitly point forward; BACK
 *		exists to make loop structures possible.
 *
 * STAR,PLUS	'?', and complex '*' and '+', are implemented as circular
 *		BRANCH structures using BACK.  Simple cases (one character
 *		per match) are implemented with STAR and PLUS for speed
 *		and to minimize recursive plunges.
 *
 * OPEN,CLOSE	...are numbered at compile time.
 */

/*
 * A node is one char of opcode followed by two chars of "next" pointer.
 * "Next" pointers are stored as two 8-bit pieces, high order first.  The
 * value is a positive offset from the opcode of the node containing it.
 * An operand, if any, simply follows the node.  (Note that much of the
 * code generation knows about this implicit relationship.)
 *
 * Using two bytes for the "next" pointer is vast overkill for most things,
 * but allows patterns to get big without disasters.
 */
#define	OP(p)	(*(p))
#define	NEXT(p)	((((*((p) + 1)) & 0377) << 8) + ((*((p) + 2)) & 0377))
#define	OPERAND(p)	((p) + 3)

/*
 * See regmagic.h for one further detail of program structure.
 */


/*
 * Utility definitions.
 */
#ifndef CHARBITS
#define	UCHARAT(p)	((int)*(unsigned char *)(p))
#else
#define	UCHARAT(p)	((int)*(p)&CHARBITS)
#endif

#define	FAIL(m)	{ regerror(m); return(NULL); }
#define	ISMULT(c)	((c) == '*' || (c) == '+' || (c) == '?')
#define	META	"^$.[()|?+*\\"

/*
 * Flags to be passed up and down.
 */
#define	HASWIDTH	01					/* Known never to match null string. */
#define	SIMPLE		02					/* Simple enough to be STAR/PLUS operand. */
#define	SPSTART		04					/* Starts with * or +. */
#define	WORST		0					/* Worst case. */

/*
 * Global work variables for regcomp().
 */
static char *regparse;					/* Input-scan pointer. */
static int regnpar;						/* () count. */
static char regdummy;
static char *regcode;					/* Code-emit pointer; &regdummy = don't. */
static long regsize;					/* Code size. */

/*
 - regc - emit (if appropriate) a byte of code
 */
static void regc(char b)
{
	if (regcode != &regdummy)
		*regcode++ = b;
	else
		regsize++;
}


/*
 - regnode - emit a node
 */
static char *regnode(char op)
{
	register char *ret;
	register char *ptr;

	ret = regcode;
	if (ret == &regdummy)
	{
		regsize += 3;
		return (ret);
	}

	ptr = ret;
	*ptr++ = op;
	*ptr++ = '\0';						/* Null "next" pointer. */
	*ptr++ = '\0';
	regcode = ptr;

	return (ret);
}


static char *reg(int paren, int *flagp);

/*
 - regatom - the lowest level
 *
 * Optimization:  gobbles an entire sequence of ordinary characters so that
 * it can turn them into a single node, which is smaller to store and
 * faster to run.  Backslashed characters are exceptions, each becoming a
 * separate node; the code is simpler that way and it's not worth fixing.
 */
static char *regatom(int *flagp)
{
	register char *ret;
	int flags;

	*flagp = WORST;						/* Tentatively. */

	switch (*regparse++)
	{
	case '^':
		ret = regnode(BOL);
		break;
	case '$':
		ret = regnode(EOL);
		break;
	case '.':
		ret = regnode(ANY);
		*flagp |= HASWIDTH | SIMPLE;
		break;
	case '[':
		{
			register int class;
			register int classend;

			if (*regparse == '^')
			{							/* Complement of range. */
				ret = regnode(ANYBUT);
				regparse++;
			} else
				ret = regnode(ANYOF);
			if (*regparse == ']' || *regparse == '-')
				regc(*regparse++);
			while (*regparse != '\0' && *regparse != ']')
			{
				if (*regparse == '-')
				{
					regparse++;
					if (*regparse == ']' || *regparse == '\0')
						regc('-');
					else
					{
						class = UCHARAT(regparse - 2) + 1;
						classend = UCHARAT(regparse);
						if (class > classend + 1)
							FAIL("invalid [] range");
						for (; class <= classend; class++)
							regc(class);
						regparse++;
					}
				} else
					regc(*regparse++);
			}
			regc('\0');
			if (*regparse != ']')
				FAIL("unmatched []");
			regparse++;
			*flagp |= HASWIDTH | SIMPLE;
		}
		break;
	case '(':
		ret = reg(1, &flags);
		if (ret == NULL)
			return (NULL);
		*flagp |= flags & (HASWIDTH | SPSTART);
		break;
	case '\0':
	case '|':
	case ')':
		FAIL("internal urp");			/* Supposed to be caught earlier. */
	case '?':
	case '+':
	case '*':
		FAIL("?+* follows nothing");
	case '\\':
		if (*regparse == '\0')
			FAIL("trailing \\");
		ret = regnode(EXACTLY);
		regc(*regparse++);
		regc('\0');
		*flagp |= HASWIDTH | SIMPLE;
		break;
	default:
		{
			register int len;
			register char ender;

			regparse--;
			len = (int)strcspn(regparse, META);
			if (len <= 0)
				FAIL("internal disaster");
			ender = *(regparse + len);
			if (len > 1 && ISMULT(ender))
				len--;					/* Back off clear of ?+* operand. */
			*flagp |= HASWIDTH;
			if (len == 1)
				*flagp |= SIMPLE;
			ret = regnode(EXACTLY);
			while (len > 0)
			{
				regc(*regparse++);
				len--;
			}
			regc('\0');
		}
		break;
	}

	return (ret);
}


/*
 - reginsert - insert an operator in front of already-emitted operand
 *
 * Means relocating the operand.
 */
static void reginsert(char op, char *opnd)
{
	register char *src;
	register char *dst;
	register char *place;

	if (regcode == &regdummy)
	{
		regsize += 3;
		return;
	}

	src = regcode;
	regcode += 3;
	dst = regcode;
	while (src > opnd)
		*--dst = *--src;

	place = opnd;						/* Op node, where operand used to be. */
	*place++ = op;
	*place++ = '\0';
	*place++ = '\0';
}


/*
 - regnext - dig the "next" pointer out of a node
 */
static char *regnext(char *p)
{
	int offset;

	if (p == &regdummy)
		return (NULL);

/**	#define	NEXT(p)	(((*((p)+1)&0377)<<8) + *((p)+2)&0377)	*/
/**
	q = (char *) &offset;
	*q = p[1];
	q[1] = p[2];
**/ offset = NEXT(p);
	if (offset == 0)
		return (NULL);

	if (OP(p) == BACK)
		return (p - offset);
	else
		return (p + offset);
}


/*
 - regtail - set the next-pointer at the end of a node chain
 */
static void regtail(char *p, char *val)
{
	register char *scan;
	register char *temp;
	register int offset;

	if (p == &regdummy)
		return;

	/* Find last node. */
	scan = p;
	for (;;)
	{
		temp = regnext(scan);
		if (temp == NULL)
			break;
		scan = temp;
	}

	if (OP(scan) == BACK)
		offset = (int)(scan - val);
	else
		offset = (int)(val - scan);
	*(scan + 1) = (offset >> 8) & 0377;
	*(scan + 2) = offset & 0377;
}


/*
 - regoptail - regtail on operand of first argument; nop if operandless
 */
static void regoptail(char *p, char *val)
{
	/* "Operandless" and "op != BRANCH" are synonymous in practice. */
	if (p == NULL || p == &regdummy || OP(p) != BRANCH)
		return;
	regtail(OPERAND(p), val);
}


/*
 - regpiece - something followed by possible [*+?]
 *
 * Note that the branching code sequences used for ? and the general cases
 * of * and + are somewhat optimized:  they use the same NOTHING node as
 * both the endmarker for their branch list and the body of the last branch.
 * It might seem that this node could be dispensed with entirely, but the
 * endmarker role is not redundant.
 */
static char *regpiece(int *flagp)
{
	register char *ret;
	register char op;
	register char *next;
	int flags;

	ret = regatom(&flags);
	if (ret == NULL)
		return (NULL);

	op = *regparse;
	if (!ISMULT(op))
	{
		*flagp = flags;
		return (ret);
	}

	if (!(flags & HASWIDTH) && op != '?')
		FAIL("*+ operand could be empty");
	*flagp = (op != '+') ? (WORST | SPSTART) : (WORST | HASWIDTH);

	if (op == '*' && (flags & SIMPLE))
		reginsert(STAR, ret);
	else if (op == '*')
	{
		/* Emit x* as (x&|), where & means "self". */
		reginsert(BRANCH, ret);			/* Either x */
		regoptail(ret, regnode(BACK));	/* and loop */
		regoptail(ret, ret);			/* back */
		regtail(ret, regnode(BRANCH));	/* or */
		regtail(ret, regnode(NOTHING));	/* null. */
	} else if (op == '+' && (flags & SIMPLE))
		reginsert(PLUS, ret);
	else if (op == '+')
	{
		/* Emit x+ as x(&|), where & means "self". */
		next = regnode(BRANCH);			/* Either */
		regtail(ret, next);
		regtail(regnode(BACK), ret);	/* loop back */
		regtail(next, regnode(BRANCH));	/* or */
		regtail(ret, regnode(NOTHING));	/* null. */
	} else if (op == '?')
	{
		/* Emit x? as (x|) */
		reginsert(BRANCH, ret);			/* Either x */
		regtail(ret, regnode(BRANCH));	/* or */
		next = regnode(NOTHING);		/* null. */
		regtail(ret, next);
		regoptail(ret, next);
	}
	regparse++;
	if (ISMULT(*regparse))
		FAIL("nested *?+");

	return (ret);
}


/*
 - regbranch - one alternative of an | operator
 *
 * Implements the concatenation operator.
 */
static char *regbranch(int *flagp)
{
	register char *ret;
	register char *chain;
	register char *latest;
	int flags;

	*flagp = WORST;						/* Tentatively. */

	ret = regnode(BRANCH);
	chain = NULL;
	while (*regparse != '\0' && *regparse != '|' && *regparse != ')')
	{
		latest = regpiece(&flags);
		if (latest == NULL)
			return (NULL);
		*flagp |= flags & HASWIDTH;
		if (chain == NULL)				/* First piece. */
			*flagp |= flags & SPSTART;
		else
			regtail(chain, latest);
		chain = latest;
	}
	if (chain == NULL)					/* Loop ran zero times. */
		(void) regnode(NOTHING);

	return (ret);
}


/*
 - reg - regular expression, i.e. main body or parenthesized thing
 *
 * Caller must absorb opening parenthesis.
 *
 * Combining parenthesis handling with the base level of regular expression
 * is a trifle forced, but the need to tie the tails of the branches to what
 * follows makes it hard to avoid.
 */
static char *reg(int paren, int *flagp)
{
	register char *ret;
	register char *br;
	register char *ender;
	register int parno;
	int flags;

	*flagp = HASWIDTH;					/* Tentatively. */

	/* Make an OPEN node, if parenthesized. */
	if (paren)
	{
		if (regnpar >= NSUBEXP)
			FAIL("too many ()");
		parno = regnpar;
		regnpar++;
		ret = regnode(OPEN + parno);
	} else
	{
		ret = NULL;
		parno = 0;
	}
	
	/* Pick up the branches, linking them together. */
	br = regbranch(&flags);
	if (br == NULL)
		return (NULL);
	if (ret != NULL)
		regtail(ret, br);				/* OPEN -> first. */
	else
		ret = br;
	if (!(flags & HASWIDTH))
		*flagp &= ~HASWIDTH;
	*flagp |= flags & SPSTART;
	while (*regparse == '|')
	{
		regparse++;
		br = regbranch(&flags);
		if (br == NULL)
			return (NULL);
		regtail(ret, br);				/* BRANCH -> BRANCH. */
		if (!(flags & HASWIDTH))
			*flagp &= ~HASWIDTH;
		*flagp |= flags & SPSTART;
	}

	/* Make a closing node, and hook it on the end. */
	ender = regnode((paren) ? CLOSE + parno : END);
	regtail(ret, ender);

	/* Hook the tails of the branches to the closing node. */
	for (br = ret; br != NULL; br = regnext(br))
		regoptail(br, ender);

	/* Check for proper termination. */
	if (paren && *regparse++ != ')')
	{
		FAIL("unmatched ()");
	} else if (!paren && *regparse != '\0')
	{
		if (*regparse == ')')
		{
			FAIL("unmatched ()");
		} else
			FAIL("junk on end");		/* "Can't happen". */
		/* NOTREACHED */
	}

	return (ret);
}


/*
 - regcomp - compile a regular expression into internal code
 *
 * We can't allocate space until we know how big the compiled form will be,
 * but we can't compile it (and thus know how big it is) until we've got a
 * place to put the code.  So we cheat:  we compile it twice, once with code
 * generation turned off and size counting turned on, and once "for real".
 * This also means that we don't allocate space until we are sure that the
 * thing really will compile successfully, and we never have to move the
 * code and thus invalidate pointers into it.  (Note that it has to be in
 * one piece because free() must be able to free it all.)
 *
 * Beware that the optimization-preparation code in here knows about some
 * of the structure of the compiled regexp.
 */
regexp *regcomp(char *exp)
{
	register regexp *r;
	register char *scan;
	register char *longest;
	register int len;
	int flags;

	if (exp == NULL)
		FAIL("NULL argument");

	/* First pass: determine size, legality. */
	regparse = exp;
	regnpar = 1;
	regsize = 0L;
	regcode = &regdummy;
	regc(MAGIC);
	if (reg(0, &flags) == NULL)
		return (NULL);

	/* Small enough for pointer-storage convention? */
	if (regsize >= 32767L)				/* Probably could be 65535L. */
		FAIL("regexp too big");

	/* Allocate space. */
	r = (regexp *) malloc(((unsigned int) sizeof(regexp)) + (unsigned) regsize);
	if (r == NULL)
		FAIL("out of space");

	/* Second pass: emit code. */
	regparse = exp;
	regnpar = 1;
	regcode = r->program;
	regc(MAGIC);
	if (reg(0, &flags) == NULL)
		return (NULL);

	/* Dig out information for optimizations. */
	r->regstart = '\0';					/* Worst-case defaults. */
	r->reganch = 0;
	r->regmust = NULL;
	r->regmlen = 0;
	scan = r->program + 1;				/* First BRANCH. */
	if (OP(regnext(scan)) == END)
	{									/* Only one top-level choice. */
		scan = OPERAND(scan);

		/* Starting-point info. */
		if (OP(scan) == EXACTLY)
			r->regstart = *OPERAND(scan);
		else if (OP(scan) == BOL)
			r->reganch++;

		/*
		 * If there's something expensive in the r.e., find the
		 * longest literal string that must appear and make it the
		 * regmust.  Resolve ties in favor of later strings, since
		 * the regstart check works with the beginning of the r.e.
		 * and avoiding duplication strengthens checking.  Not a
		 * strong reason, but sufficient in the absence of others.
		 */
		if (flags & SPSTART)
		{
			longest = NULL;
			len = 0;
			for (; scan != NULL; scan = regnext(scan))
				if (OP(scan) == EXACTLY && strlen(OPERAND(scan)) >= len)
				{
					longest = OPERAND(scan);
					len = (int)strlen(OPERAND(scan));
				}
			r->regmust = longest;
			r->regmlen = len;
		}
	}

	return (r);
}


/*
 * regexec and friends
 */

/*
 * Global work variables for regexec().
 */
static char *reginput;					/* String-input pointer. */
static char *regbol;					/* Beginning of input, for ^ check. */
static char **regstartp;				/* Pointer to startp array. */
static char **regendp;					/* Ditto for endp. */

#ifdef DEBUG
int regnarrate = 0;
#endif


/*
 - regrepeat - repeatedly match something simple, report how many
 */
static int regrepeat(char *p)
{
	register int count = 0;
	register char *scan;
	register char *opnd;

	scan = reginput;
	opnd = OPERAND(p);
	switch (OP(p))
	{
	case ANY:
		count = (int)strlen(scan);
		scan += count;
		break;
	case EXACTLY:
		while (*opnd == *scan)
		{
			count++;
			scan++;
		}
		break;
	case ANYOF:
		while (*scan != '\0' && strchr(opnd, *scan) != NULL)
		{
			count++;
			scan++;
		}
		break;
	case ANYBUT:
		while (*scan != '\0' && strchr(opnd, *scan) == NULL)
		{
			count++;
			scan++;
		}
		break;
	default:							/* Oh dear.  Called inappropriately. */
		regerror("internal foulup");
		count = 0;						/* Best compromise. */
		break;
	}
	reginput = scan;

	return (count);
}


/*
 - regmatch - main matching routine
 *
 * Conceptually the strategy is simple:  check to see whether the current
 * node matches, call self recursively to see whether the rest matches,
 * and then act accordingly.  In practice we make some effort to avoid
 * recursion, in particular by going through "ordinary" nodes (that don't
 * need to know whether the rest of the match failed) by a loop instead of
 * by recursion.
 */
/* 0 failure, 1 success */
static int regmatch(char *prog)
{
	register char *scan;				/* Current node. */
	char *next;							/* Next node. */

	scan = prog;
#ifdef DEBUG
	if (scan != NULL && regnarrate)
		fprintf(stderr, "%s(\n", regprop(scan));
#endif
	while (scan != NULL)
	{
#ifdef DEBUG
		if (regnarrate)
			fprintf(stderr, "%s...\n", regprop(scan));
#endif
		next = regnext(scan);

		switch (OP(scan))
		{
		case BOL:
			if (reginput != regbol)
				return (0);
			break;
		case EOL:
			if (*reginput != '\0')
				return (0);
			break;
		case ANY:
			if (*reginput == '\0')
				return (0);
			reginput++;
			break;
		case EXACTLY:
			{
				register int len;
				register char *opnd;

				opnd = OPERAND(scan);
				/* Inline the first character, for speed. */
				if (*opnd != *reginput)
					return (0);
				len = (int)strlen(opnd);
				if (len > 1 && strncmp(opnd, reginput, ((size_t) len)) != 0)
					return (0);
				reginput += len;
			}
			break;
		case ANYOF:
			if (*reginput == '\0' || strchr(OPERAND(scan), *reginput) == NULL)
				return (0);
			reginput++;
			break;
		case ANYBUT:
			if (*reginput == '\0' || strchr(OPERAND(scan), *reginput) != NULL)
				return (0);
			reginput++;
			break;
		case NOTHING:
			break;
		case BACK:
			break;
		case OPEN + 1:
		case OPEN + 2:
		case OPEN + 3:
		case OPEN + 4:
		case OPEN + 5:
		case OPEN + 6:
		case OPEN + 7:
		case OPEN + 8:
		case OPEN + 9:
			{
				register int no;
				register char *save;

				no = OP(scan) - OPEN;
				save = reginput;

				if (regmatch(next))
				{
					/*
					 * Don't set startp if some later
					 * invocation of the same parentheses
					 * already has.
					 */
					if (regstartp[no] == NULL)
						regstartp[no] = save;
					return (1);
				} else
					return (0);
			}
		case CLOSE + 1:
		case CLOSE + 2:
		case CLOSE + 3:
		case CLOSE + 4:
		case CLOSE + 5:
		case CLOSE + 6:
		case CLOSE + 7:
		case CLOSE + 8:
		case CLOSE + 9:
			{
				register int no;
				register char *save;

				no = OP(scan) - CLOSE;
				save = reginput;

				if (regmatch(next))
				{
					/*
					 * Don't set endp if some later
					 * invocation of the same parentheses
					 * already has.
					 */
					if (regendp[no] == NULL)
						regendp[no] = save;
					return (1);
				} else
					return (0);
			}
		case BRANCH:
			{
				register char *save;

				if (OP(next) != BRANCH)	/* No choice. */
					next = OPERAND(scan);	/* Avoid recursion. */
				else
				{
					do
					{
						save = reginput;
						if (regmatch(OPERAND(scan)))
							return (1);
						reginput = save;
						scan = regnext(scan);
					} while (scan != NULL && OP(scan) == BRANCH);
					return (0);
					/* NOTREACHED */
				}
			}
			break;
		case STAR:
		case PLUS:
			{
				register char nextch;
				register int no;
				register char *save;
				register int min;

				/*
				 * Lookahead to avoid useless match attempts
				 * when we know what character comes next.
				 */
				nextch = '\0';
				if (OP(next) == EXACTLY)
					nextch = *OPERAND(next);
				min = (OP(scan) == STAR) ? 0 : 1;
				save = reginput;
				no = regrepeat(OPERAND(scan));
				while (no >= min)
				{
					/* If it could work, try it. */
					if (nextch == '\0' || *reginput == nextch)
						if (regmatch(next))
							return (1);
					/* Couldn't or didn't -- back up. */
					no--;
					reginput = save + no;
				}
				return (0);
			}
		case END:
			return (1);					/* Success! */
		default:
			regerror("memory corruption");
			return (0);
		}

		scan = next;
	}

	/*
	 * We get here only if there's trouble -- normally "case END" is
	 * the terminating point.
	 */
	regerror("corrupted pointers");
	return (0);
}


/*
 - regtry - try match at specific point
 */
/* 0 failure, 1 success */
static int regtry(regexp *prog, char *string)
{
	register int i;
	register char **sp;
	register char **ep;

	reginput = string;
	regstartp = prog->startp;
	regendp = prog->endp;

	sp = prog->startp;
	ep = prog->endp;
	for (i = NSUBEXP; i > 0; i--)
	{
		*sp++ = NULL;
		*ep++ = NULL;
	}
	if (regmatch(prog->program + 1))
	{
		prog->startp[0] = string;
		prog->endp[0] = reginput;
		return (1);
	} else
		return (0);
}


/*
 - regexec - match a regexp against a string
 */
int regexec(regexp *prog, char *string)
{
	register char *s;

	/* Be paranoid... */
	if (prog == NULL || string == NULL)
	{
		regerror("NULL parameter");
		return (0);
	}

	/* Check validity of program. */
	if (UCHARAT(prog->program) != MAGIC)
	{
		regerror("corrupted program");
		return (0);
	}

	/* If there is a "must appear" string, look for it. */
	if (prog->regmust != NULL)
	{
		s = string;
		while ((s = strchr(s, prog->regmust[0])) != NULL)
		{
			if (strncmp(s, prog->regmust, ((size_t) (prog->regmlen))) == 0)
				break;					/* Found it. */
			s++;
		}
		if (s == NULL)					/* Not present. */
			return (0);
	}

	/* Mark beginning of line for ^ . */
	regbol = string;

	/* Simplest case:  anchored match need be tried only once. */
	if (prog->reganch)
		return regtry(prog, string);

	/* Messy cases:  unanchored match. */
	s = string;
	if (prog->regstart != '\0')
		/* We know what char it must start with. */
		while ((s = strchr(s, prog->regstart)) != NULL)
		{
			if (regtry(prog, s))
				return (1);
			s++;
	} else
		/* We don't -- general case. */
		do
		{
			if (regtry(prog, s))
				return (1);
		} while (*s++ != '\0');

	/* Failure. */
	return (0);
}


#ifdef DEBUG

/*
 - regprop - printable representation of opcode
 */
static char *regprop(char *op)
{
	register char *p;
	static char buf[50];

	(void) strcpy(buf, ":");

	switch (OP(op))
	{
	case BOL:
		p = "BOL";
		break;
	case EOL:
		p = "EOL";
		break;
	case ANY:
		p = "ANY";
		break;
	case ANYOF:
		p = "ANYOF";
		break;
	case ANYBUT:
		p = "ANYBUT";
		break;
	case BRANCH:
		p = "BRANCH";
		break;
	case EXACTLY:
		p = "EXACTLY";
		break;
	case NOTHING:
		p = "NOTHING";
		break;
	case BACK:
		p = "BACK";
		break;
	case END:
		p = "END";
		break;
	case OPEN + 1:
	case OPEN + 2:
	case OPEN + 3:
	case OPEN + 4:
	case OPEN + 5:
	case OPEN + 6:
	case OPEN + 7:
	case OPEN + 8:
	case OPEN + 9:
		sprintf(buf + strlen(buf), "OPEN%d", OP(op) - OPEN);
		p = NULL;
		break;
	case CLOSE + 1:
	case CLOSE + 2:
	case CLOSE + 3:
	case CLOSE + 4:
	case CLOSE + 5:
	case CLOSE + 6:
	case CLOSE + 7:
	case CLOSE + 8:
	case CLOSE + 9:
		sprintf(buf + strlen(buf), "CLOSE%d", OP(op) - CLOSE);
		p = NULL;
		break;
	case STAR:
		p = "STAR";
		break;
	case PLUS:
		p = "PLUS";
		break;
	default:
		regerror("corrupted opcode");
		break;
	}
	if (p != NULL)
		(void) strcat(buf, p);
	return (buf);
}

/*
 - regdump - dump a regexp onto stdout in vaguely comprehensible form
 */
void regdump(regexp *r)
{
	register char *s;
	register char op = EXACTLY;			/* Arbitrary non-END op. */
	register char *next;


	s = r->program + 1;
	while (op != END)
	{									/* While that wasn't END last time... */
		op = OP(s);
		printf("%2d%s", s - r->program, regprop(s));	/* Where, what. */
		next = regnext(s);
		if (next == NULL)				/* Next ptr. */
			printf("(0)");
		else
			printf("(%d)", (s - r->program) + (next - s));
		s += 3;
		if (op == ANYOF || op == ANYBUT || op == EXACTLY)
		{
			/* Literal string, where present. */
			while (*s != '\0')
			{
				putchar(*s);
				s++;
			}
			s++;
		}
		putchar('\n');
	}

	/* Header fields of interest. */
	if (r->regstart != '\0')
		printf("start `%c' ", r->regstart);
	if (r->reganch)
		printf("anchored ");
	if (r->regmust != NULL)
		printf("must have \"%s\"", r->regmust);
	printf("\n");
}

#endif

/*
 * The following is provided for those people who do not have strcspn() in
 * their C libraries.  They should get off their butts and do something
 * about it; at least one public-domain implementation of those (highly
 * useful) string routines has been published on Usenet.
 */
#ifdef STRCSPN
/*
 * strcspn - find length of initial segment of s1 consisting entirely
 * of characters not from s2
 */

static size_t strcspn(const char *s1, const char *s2)
{
	register const char *scan1;
	register const char *scan2;
	register size_t count;

	count = 0;
	for (scan1 = s1; *scan1 != '\0'; scan1++)
	{
		for (scan2 = s2; *scan2 != '\0';)	/* ++ moved down. */
			if (*scan1 == *scan2++)
				return count;
		count++;
	}
	return count;
}
#endif
