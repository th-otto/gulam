/* functions.c of gulam+ue -- key to function bindings

  Most of this file is borrowed from MicroGnuEmacs.  Many thanks. -- pm
*/

#include	"ue.h"

/* static char snoop[] = "no-op"; */

static int noop(int f, int n)
{
	UNUSED(f);
	UNUSED(n);
	return TRUE;
}

/* function-code-ptr : function-string0name table ::= Ideally there should
 be no duplicates; but, it does not hurt.  The order of these functions
 has no effect on efficiency; they are grouped for their logical togetherness.
 DO NOT change the content, or order of these by hand.  This is generated by
 kbgen.tos from data in kbin.000, and is coupled to the #defines used in kb.c
*/
FPFS fpfs[] = {
	noop,
	escctrld,							/* "show-possible-expansions", */
	escesc,								/* "expand-name", */
	miniterm,							/* "terminate-mini-buffer", */
	filename,							/* "file-name", */
	gxpand,								/* "expand-name-gulam-style", */
	togulambuf,							/* "switch-to-gulam-buffer", */
	execbuf,							/* "execute-buffer", */
	fileread,							/* "read-file", */
	showkbdmacro,						/* "show-key-board-macro", */
#if XMDM
	temul,								/* "terminal-emulator", */
#else
	noop,								/* snoop, */
#endif
	mvupwind,							/* "move-window-up", */
	mvdnwind,							/* "move-window-dn", */
	quickexit,							/* "quick-exit", */
	tempexit,							/* "temporary-exit", */
	gforwline,							/* "gulam-forward-line", */
	gulam,								/* "gulam-do-newline", */
	tab,								/* "goto-next-tab", */
	quit,								/* "save-buffers-kill-emacs", */
	ctrlg,								/* "keyboard-quit", */
	help,								/* "help", */
	ctlxlp,								/* "start-kbd-macro", */
	ctlxrp,								/* "end-kbd-macro", */
	ctlxe,								/* "call-last-kbd-macro", */
	setfillcol,							/* , */
	refresh,							/* "redraw-display", */
	backchar,							/* "backward-char", */
	forwchar,							/* "forward-char", */
	backdel,							/* "backward-delete-char", */
	forwdel,							/* "delete-char", */
	gotobol,							/* "beginning-of-line", */
	gotoeol,							/* "end-of-line", */
	killtext,							/* "kill-line", */
	forwline,							/* "next-line", */
	openline,							/* "open-line", */
	backline,							/* "previous-line", */
	newline,							/* "insert-newline", */
	indent,								/* "newline-and-indent", */
	gotoline,							/* "goto-line", */
	gspawn,								/* "execute-one-Gulam-cmd", */
	killtobln,							/* */
	fillpara,							/* */
	noop,								/* snoop, */
	backsearch,							/* "search-backward", */
	forwsearch,							/* "search-forward", */
	lforwchar,							/* "line-forward-char", */
	lforwdel,							/* "line-delete-next-char", */
	queryrepl,							/* "query-replace", */
	setmark,							/* "set-mark-command", */
	selfinsert,							/* "self-insert", */
	reposition,							/* "recenter", */
	quote,								/* "quoted-insert", */
	twiddle,							/* "transpose-chars", */
	copyregion,							/* "copy-region-as-kill", */
	killregion,							/* "kill-region", */
	keyreset,							/* "keys-reset", */
	semireset,							/* */
	noop,								/* snoop, */
	noop,								/* snoop, */
	yank,								/* "yank", */
	fileinsert,							/* "insert-file", */
	filevisit,							/* "find-file", */
	filesave,							/* "save-buffer", */
	filewrite,							/* "write-file", */
	noop,								/* snoop, */
	deblank,							/* "delete-blank-lines", */
	swapmark,							/* "exchange-point-and-mark", */
	showcpos,							/* "what-cursor-position", */
	nextwind,							/* "next-window", */
	prevwind,							/* "previous-window", */
	shrinkwind,							/* "shrink-window", */
	enlargewind,						/* "enlarge-window", */
	noop,								/* snoop, */
	onlywind,							/* "delete-other-windows", */
	splitwind,							/* "split-window-vertically", */
	noop,								/* snoop */
	noop,								/* snoop, */
	bufferinsert,						/* "insert-buffer", */
	usebuffer,							/* "switch-to-buffer", */
	lbackdel,							/* "line-backward-delete-char", */
	listbuffers,						/* "list-buffers", */
	killbuffer,							/* "kill-buffer", */
	savebuffers,						/* "save-some-buffers", */
	lbackchar,							/* "line-backward-character", */
	gotoeob,							/* "end-of-buffer", */
	gotobob,							/* "beginning-of-buffer", */
	gotobop,							/* , */
	gotoeop,							/* , */
	noop,								/* , */
	delwhite,							/* "just-one-space", */
	backword,							/* "backward-word", */
	capword,							/* "capitalize-word", */
	delbword,							/* "kill-backward-word", */
	delfword,							/* "kill-word", */
	forwword,							/* "forward-word", */
	lowerword,							/* "downcase-word", */
	upperword,							/* "upcase-word", */
	forwpage,							/* "scroll-up", */
	backpage,							/* "scroll-down", */
	gxpshow,							/* "show-possible-completions", */
	noop,								/* snoop, */
	noop,								/* snoop, */
	noop,								/* snoop, */
	noop,								/* snoop, */
	noop,								/* snoop, */
	noop,								/* snoop, */
	noop,								/* snoop, */
	showmatch,							/* "blink-matching-paren-hack", */
	noop,								/* snoop, */
	noop,								/* snoop, */
	desckey,							/* "describe-key-briefly", */
	wallchart,							/* "describe-bindings", */
	getarg,
	metanext,
	ctlxnext,
	tenewline,
	totebuf,
	tesendtext,
	errfwd,								/* "error-forward", */
	NULL,								/* NULL, */
};
