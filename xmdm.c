/*
	xmdm.c of Gulam -- Xmodem Send / Receive Module

	Generic Xmodem send/receive code posted to SIG*ATARI on CompuServe
	adapted by bammi@cwru.edu, and later by pm@cwru.edu	03/13/87
 */

#include "ue.h"
#include <setjmp.h>

#if XMDM /* whole file */

				/* Xmodem specific defines */
#define	SECSIZ		0x80				/* Size of a xmodem sector/block */
#define	ERRORMAX	20					/* MAX ERRORS BEFORE ABORT very conservative */
#define	RETRYMAX	15					/* MAXIMUM RETRYS BEFORE ABORT */
#define	SOH		1						/* START OF SECTOR CHAR */
#define	EOT		4						/* end of transmission char */
#define	ACK		6						/* acknowledge sector transmission */
#define	NAK		21						/* error in transmission detected */
#define CAN		('X'&037)				/* Cancel transmission      */

/*
0         1         2         3         4         5         6     
012345678901234567890123456789012345678901234567890123456789012345
bytes 123456789 packets 123 errors 123 timeouts 123 rx filename...
*/
#define	SZXMRS	85

#define	BYTES		8
#define	BYTESEND	15					/* byte ctr ends 1 char left of this */
#define	PACKETS		24					/* #pkts bgn here   */
#define	ERRORS		35					/* #errs bgn here   */
#define	TIMEOUTS	48					/* #tmos bgn here   */
#define	MESSAGE		55
#define	FILENAME	55
#define	RSX		52

static char xmrs[SZXMRS];
static char xmi[SZXMRS] = "bytes         0 packets 000 errors 000 timeouts 000 rx ?";


/* static char COF[] = "> Cannot open file <"; */
static char TCS[] = "> Transfer cancelled by sender <";
static char TCR[] = "> Transfer cancelled by receiver <";
static char EWF[] = "> Error writing file -- aborting <";
static char ERF[] = "> Error reading file -- aborting <";
static char TMO[] = "> Too many time outs -- aborting <";
static char TME[] = "> Too many errors -- aborting <";
static char RNK[] = "> Receiver not sending NAK's -- aborting <";
static char NNS[] = "> No acknowledgment of sector -- aborting <";

static uchar *bufr;						/* file buffer */
static long BUFSIZ;						/* size of this buffer  */

jmp_buf abort_env;						/* Long jump when transfers are aborted */
jmp_buf time_env;						/* Long jump on timeout       */


#ifdef	DEBUG
static int debug = 1;

#define	dprintf	printf
#define	mlwrite	printf
#endif

static int fd;							/* File descriptor for opened file */
static int bufptr;						/* Ptr to file buffer    */
static int mtimeout;					/* Time out period      */
static int npsent;						/* #packets sent successfully   */
static int nprecd;						/* #packets received successfully */
static int npackets;					/* #packets received/sent   */
static int npdup;						/* #duplicate Packets       */
static int npbad;						/* #bad packets         */
static int ntimeout;					/* #Timeouts            */
static int nnaked;						/* #naked packets       */

static uint sectnum;					/* The currently expected sector #  */
static uint sectors;					/* The received/sent sector #       */
static uint sectcomp;					/* The 1's complements of sectcurr  */
static uint errors;						/* #errors encountered in this recv */
static uint attempts;					/* #of attempts to send         */
static uint errorflag;					/* Just a flag              */

static long nbx;						/* #bytes sent/received sucessfully */


/* display module for rx/sx	*/
static void xmmsginit(char rsc, char *s)
{
	int n;

	cpymem(xmrs, xmi, MESSAGE);
	xmrs[RSX] = rsc;

	n = (int)strlen(s);
	if (n > SZXMRS - MESSAGE - 1)
		n = SZXMRS - MESSAGE - 1;
	cpymem(xmrs + MESSAGE, s, n);
	xmrs[n + MESSAGE] = '\0';
	mlmesgne(xmrs);
}


/* Open file for read/write, and send the remote cmd to remote system
*/

static int openfile(char *file, int rs)
{
	char *p;
	char *q;

	fd = MINFH - 1;
	if (rs == 's')
	{
		fd = (int)gfopen(file, 0);
		p = "sx_remote_cmd";
	} else
	{
		if (flnotexists(file) || mlyesno(sprintp("file '%s' exists; overwrite it?", file)))
		{
			fd = (int)gfcreate(file, 0);
		}
		p = "rx_remote_cmd";
	}
	if (fd < MINFH)
		return FALSE;
	p = varstr(p);
	if (p && *p)						/* send the cmd to get the remote mc started */
	{
		if ((q = strrchr(file, '\\')) != NULL)
			q++;
		else
			q = file;
		q = sprintp("%s %s\r\n", p, q);
		writemodem(q, ((int) strlen(q)));
	}
	xmmsginit(rs, file);
	return TRUE;
}


static void ufb(int x, int n)
{
	xmrs[x + 2] = n % 10 + '0';
	n /= 10;
	xmrs[x + 1] = n % 10 + '0';
	n /= 10;
	xmrs[x] = n % 10 + '0';
	mlmesgne(xmrs);
}


/* Updates the bytes display  */

static void ufbbytes(long count)
{
	char *p;
	int j;
	int k;

	p = itoal(count);
	k = (int)strlen(p);
	j = BYTESEND - k;
	while ((xmrs[j++] = *p++) != 0)
		;
	xmrs[BYTESEND] = ' ';
	mlmesgne(xmrs);
}


/* All sectors that needed to be sent are done.  Now send the EOT to
complete transfer.  The EOT may or may not get ACK'ed, warn the user
if it does'nt */

static void sendeot(void)
{
	char *p;

	attempts = 0;
	for (;;)
	{
		alarm(mtimeout);
		if (setjmp(time_env))
		{
			if (attempts < (2 * ERRORMAX))
			{
				ufb(TIMEOUTS, ++ntimeout);
				flushinput();
				continue;
			}
		}
		sendchar(EOT);
		attempts++;
		if (readmodem() == ACK)
			break;
		if (attempts == RETRYMAX)
			break;
	}
	alarm(0);
	p = attempts == RETRYMAX
		 ? "%s\r\n%D bytes sent; EOF acknowledgment not received though!\r\n" : "%s\r\n%D bytes sent.\r\n";
	mlwrite(p, xmrs, nbx);
}

/* Cancel the transfer */

static int cancel(char *p)
{
	int i;

	if (fd >= 0)
		gfclose(fd);
	fd = -1;

	for (i = 0; i < 5; i++)
		sendchar(CAN);

	/* Get rid of 4 of them, some receivers expect at least 2
	   CAN to confirm that the first CAN was not a corrupted
	   ACK, but eat only one (Ymodem for instance). */

	for (i = 0; i < 4; i++)
		sendchar('\b');

	alarm(0);
	flushinput();
	emsg = p;
	valu = -1;
	return FALSE;
}


/* Send packet: Make no more than RETRYMAX attempts on errors; if too many
timeouts, just cancel send */

static int sendsector(void)
{
	unsigned short checksum;
	uint c;
	int j;

	attempts = 0;
	do
	{
	  s_ag2:
		alarm(mtimeout);
		if (setjmp(time_env))
		{
			if (attempts < (2 * ERRORMAX))	/* on time out */
			{
				ufb(TIMEOUTS, ++ntimeout);
				flushinput();
				goto s_ag2;
			}
			return cancel(TMO);
		}
		/* Send a packet */
		ufb(PACKETS, ++npackets);
		sendchar(SOH);
		sendchar(sectnum);
		sendchar(~sectnum);
		writemodem(&bufr[bufptr], SECSIZ);
		checksum = 0;
		for (j = bufptr; j < (bufptr + SECSIZ); j++)
			checksum += bufr[j];
		sendchar(checksum);
		flushinput();
		npsent++;

		attempts++;
		c = readmodem();				/* Did it get sent over ? */
		if (c != ACK)
			ufb(ERRORS, ++nnaked);
#ifdef DEBUG
		if (debug)
			dprintf("Response char is %d\r\n", c);
#endif
		if (c == CAN)
			return cancel(TCR);
	} while ((c != ACK) && (attempts < RETRYMAX));
	return c == ACK;
}


/* Xmodem Send -- See comments for readfile() concerning timeouts/retries.
*/
static int sendfile(char *file)
{
	char c;
	int j;
	int nb;

	if (openfile(file, 's') == FALSE)
		return FALSE;
	sectnum = 1;
	j = 0;
	attempts = 0;
	do
	{
	  s_again:
		if (setjmp(time_env))
		{
			if (attempts < (2 * ERRORMAX))	/* On Time Out */
			{
				ufb(TIMEOUTS, ++ntimeout);
				flushinput();
				goto s_again;
			}
			return cancel(TMO);
		}
		alarm(mtimeout);				/* Wait for First char from receiver */
		c = readmodem();
#ifdef DEBUG
		if (debug)
			dprintf("Ate char %d\r\n", c);
#endif
		/* Eat everything but a NAK or CAN */
	} while ((c != NAK) && (c != CAN) && (j++ < (ERRORMAX * 2)));

	if (c == CAN)
		return cancel(TCR);
	if (j >= (ERRORMAX * 2))
		return cancel(RNK);

#ifdef DEBUG
	if (debug)
		dprintf("Got ACK\r\n");
#endif

	flushinput();						/* Get rid of any junk on the line */
	alarm(mtimeout);
	while ((j = (int)gfread(fd, bufr, BUFSIZ)) != 0 && (attempts != RETRYMAX))
	{
		if (j < 0)
			return cancel(ERF);
		nb = j;
		while (j < BUFSIZ)
			bufr[j++] = (char) 0;		/* pm */

		for (bufptr = 0; bufptr < nb; bufptr += SECSIZ)
		{
			ufbbytes(nbx);
			sendsector();
			if (attempts >= RETRYMAX)
				break;
			nbx += SECSIZ;
			sectnum++;
		}
	}
	gfclose(fd);
	fd = -1;
	if (attempts >= RETRYMAX)
		return cancel(NNS);
	else
		sendeot();
	return TRUE;
}


static void checksector(unsigned short checksum)
{
	unsigned short recvcsum;

	recvcsum = readmodem();
	if (checksum == recvcsum)
	{
		nprecd++;
		sectnum++;
		errors = 0;
		bufptr += SECSIZ;
		nbx += SECSIZ;
		ufbbytes(nbx);

		if (bufptr == BUFSIZ)
		{								/* Buffer Full - flush it out to file */
			bufptr = 0;
			if (gfwrite(fd, bufr, BUFSIZ) != BUFSIZ)
			{
				cancel(EWF);
				return;
			}
		}
		flushinput();
		sendchar(ACK);					/* Every thing ok - send the ACK */
	} else
	{									/* Received checksum does not match computed checksum */
		ufb(ERRORS, ++npbad);
		errorflag = TRUE;
#ifdef DEBUG
		if (debug)
			dprintf("Checksum mismatch %d %d", checksum, recvcsum);
#endif
	}
}


/* Try hard to receive one sector */

static void recvsector(void)
{
	unsigned short checksum;
	int j;

	sectors = readmodem();
	sectcomp = readmodem();

	ufb(PACKETS, ++npackets);

#ifdef DEBUG
	if (debug)
		dprintf("Sector %d  %d\n", sectors, sectcomp);
#endif
	if ((sectors + sectcomp) == (unsigned) 255)	/* => Valid sector # */
	{
		if (sectors == ((sectnum + 1) & 0xff))
		{								/* And sector in expected sequence */
			checksum = 0;
			for (j = bufptr; j < (bufptr + SECSIZ); j++)
			{
				bufr[j] = readmodem();	/* Pick up data */
				checksum = (checksum + bufr[j]) & 0xff;
			}
			checksector(checksum);
		} else
		{								/* Sector received is not the expected one */
			/* Could be a duplicate sector, or just a corrupted sector */
			if (sectors == (sectnum & 0xff))
			{							/* Duplicate sector - just ignore it and ACK */
				flushinput();
				sendchar(ACK);
			} else
			{							/* Garbage */
				ufb(ERRORS, ++npbad);
				errorflag = TRUE;
#ifdef DEBUG
				if (debug)
					dprintf("sync error\n");
#endif
			}
		}
	} else
	{
		ufb(ERRORS, ++npbad);
		errorflag = TRUE;
#ifdef DEBUG
		if (debug)
			dprintf("sector number error\r\n");
#endif
	}
}


/* Xmodem Receive: We use extremely conservative timeout/retry
parameters to deal with lazy dogs like CompuServe.  The program could
be enhanced to be adaptive by increasing the timeout as and when
timeouts occur, dynamically.  */

static int recvfile(char *file)
{
	int firstchar;				/* of the packet    */

	if (openfile(file, 'r') == FALSE)
		return FALSE;
	sectnum = errors = bufptr = 0;

  r_again:
	flushinput();
	sendchar(NAK);						/* Send NAK to let sender know that we are ready */
#ifdef	DEBUG
	dprintf("sent first NAK\r\n");
#endif
	if (setjmp(time_env))
	{
		if (++errors < ERRORMAX)		/* On a Timeout */
		{
			ufb(TIMEOUTS, ++ntimeout);
			goto r_again;
		}
		return cancel(TMO);
	}

	firstchar = 0;
	while (firstchar != EOT && errors != ERRORMAX)
	{
		errorflag = FALSE;
		alarm(mtimeout);				/* set timeout trap */
		do								/* get sync char */
		{
			firstchar = readmodem();
#ifdef DEBUG
			if (debug)
				dprintf("Got char %d \r\n", firstchar);
#endif
		} while (firstchar != SOH && firstchar != EOT && firstchar != CAN);

		if (firstchar == CAN)
			return cancel(TCS);
		if (firstchar == SOH)
			recvsector();
		if (errorflag == TRUE)
		{
			errors++;
			flushinput();
			sendchar(NAK);
		}
	}
	if ((firstchar == EOT) && (errors < ERRORMAX))
	{
		sendchar(ACK);					/* Last packet */
		if (gfwrite(fd, bufr, bufptr) != bufptr)
			return cancel(EWF);
		gfclose(fd);
		alarm(0);
		mlwrite("%s\r\n%D bytes received.\r\n", xmrs, nbx);
		return TRUE;
	}
	return cancel(TME);
}


static void xmdm(char rsc, char *fnm)
{
	int ok;

	BUFSIZ = 0x4000;					/* ask for this much, and see ... */
	bufr = maxalloc(&BUFSIZ);
	if (bufr == NULL)
	{
		valu = ENSMEM;
		return;
	}

#ifdef DEBUG
	dprintf("max allocd %d\n", BUFSIZ);
#endif
	mtimeout = 10;
	nbx = 0L;
	ntimeout = npbad = nnaked = 0;
	npsent = nprecd = npackets = npdup = 0;
	fd = -1;
	ok = TRUE;

	if (setjmp(abort_env))
	{
		emsg = "> User Aborted Transfer. <";
		ok = FALSE;
		goto ret;
	}
	setrs232speed();
	setrs232buf();
	offflowcontrol();

	ok = rsc == 'r' ? recvfile(fnm) : sendfile(fnm);

  ret:
	onflowcontrol();

	maxfree(bufr);
	if (ok == FALSE)
		cancel(emsg);
	resetrs232buf();
}

/* Entry points for Gulam: initialize variables for a Xmodem transfers
and invoke the recv/send routine.  */

void rxmdm(uchar *p)
{
	xmdm('r', p);
}

void sxmdm(uchar *p)
{
	xmdm('s', p);
}

#endif /* XMDM */
