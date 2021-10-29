set drive e:\mwc
unsetenv PATH
unsetenv HOME
unalias cc
setenv PATH $drive\bin,$drive\lib,c:\bin,d:\bin
setenv SUFF ,.prg,.ttp,.tos
setenv LIBPATH $drive\lib,$drive\bin,
setenv INCDIR $drive\include
setenv TMPDIR f:\
setenv HOME e:\bammi
setenv TIMEZONE EST:0:EDT
rehash
