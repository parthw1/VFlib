PLATEX=platex


ALL: 
	@echo '** '
	@echo '** make tfm       - to make tfm files for eKanji fonts'
	@echo '** make samples   - to make character lists of eKanji fonts'
	@echo '** '

tfm:
	cd tfm; ./mktfm

default all: $(ALL)


distclean: clean
	-rm -f *~ core *.core a.out 
	-rm -f sample-u.* sample-k.* sample-m.*
clean:
	-rm -f *.aux *.log 
realclean: distclean
	-rm -f *.tfm 

samples:
	gcc mksample.c
	./a.out -u >sample-u.tex
	./a.out -k >sample-k.tex
	./a.out -m >sample-m.tex
	$(PLATEX) sample-u.tex
	$(PLATEX) sample-k.tex
	$(PLATEX) sample-m.tex
	-rm -f a.out *.log *.aux
