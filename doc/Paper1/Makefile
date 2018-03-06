# makefile for
#
#    VFlib 3.5 --- a general font library that supports 
#    multiple font formats
# 
#  by Hirotsugu Kakugawa
#

LATEX2e=latex
BIBTEX=bibtex 
DVI2PS=dvips -f

#---
SRC=vflib35.tex cah-gut.cls vflib35.bib \
    comic-jp.eps comic-jp.gif internal.eps internal.fig \
    xmdvi-image-bw.eps \
    Makefile

default: vflib35.ps

vflib35.ps:  vflib35.tex vflib35.bib
	-rm  vflib35.aux vflib35.bbl
	${LATEX2e} vflib35
	${BIBTEX} vflib35
	${LATEX2e} vflib35
	${LATEX2e} vflib35
	-${DVI2PS} vflib35.dvi > vflib35.ps
	-rm *.dvi *.log *.aux *.bbl *.blg missfont.log 

clean:
	-rm *.log *.aux *.bbl *.blg missfont.log
distclean: clean
	-rm *~
veryclean: clean
	-rm *~ *.ps *.dvi
