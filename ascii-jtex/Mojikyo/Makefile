default:
	@echo "nothing to do."

# developers only
sample:
	gcc -o mksample mksample.c
	-rm -f sample.tex
	./mksample -ttf   -f 0 -t 3000 > samplet.tex
	./mksample -type1 -f 0 -t 3000 > samplep.tex
	platex samplet.tex
	platex samplep.tex
	make clean


koukotsu:
	gcc -o mksample mksample.c
	-rm -f sample.tex
	./mksample -f 95181 -t 99581 -n 10 -s large > koukotsu.tex
	platex koukotsu.tex
	make clean

clean:
	-rm a.out mksample *.aux *.log *.core core *~

