CFLAGS = -O

all: gulam.fns gulam.vrs gulam.dvi

gulam.dvi : gulam.texinfo gulam.vrs gulam.fns
	tex gulam.texinfo

gulam.vrs : gulam.vr
	texindex gulam.vr
	touch gulam.texinfo

gulam.fns : gulam.fn
	texindex gulam.fn

gulam.vr : gulam.texinfo
	tex gulam.texinfo

gulam.fn : gulam.texinfo
	tex gulam.texinfo

print : gulam.dvi
	dviimagen gulam
	lpr -Pimlp gulam.imp

##
## emacs is GnuEmacs - change path as appro.
##
gulam.info:
	emacs -l gulam.el -f make-gulam-info-file
