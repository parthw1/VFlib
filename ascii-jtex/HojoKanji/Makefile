PLATEX2E=platex

.SUFFIXES: .tex .dvi .tfm .pl 

all:  sample.dvi
tfm jfm:
	./mkhojo.sh

sample.dvi: sample.tex
	$(PLATEX2E) sample.tex

clean veryclean distclean:
	-rm -f *.aux *.log *.core core *~
allclean: clean
	-rm -f *.dvi *.tfm 
