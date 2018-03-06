all:		vflib-ja.ps

vflib-ja.ps:	vflib-ja.dvi
	-dvips -f vflib-ja.dvi > vflib-ja.ps
	rm -f ml-web-page.eps

vflib-ja.dvi:	vflib-ja.tex
	gzip -cd  < ml-web-page.eps.gz  > ml-web-page.eps
	platex vflib-ja
	bibtex vflib-ja
	platex vflib-ja
	platex vflib-ja

clean:
	-rm *.aux *.log *.bbl *.blg
veryclean distclean: clean
	-rm *~
CLEAN: clean
	-rm *.dvi *.ps *~
