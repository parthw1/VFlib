ALL=   vflibcap-tex      vflibcap-tex-ja-pcf \
       vflibcap-tex-pk   vflibcap-tex-pk-ja-pcf \
       vflibcap-tex-ja-ttf 

default all:  ${ALL}
clean veryclean distclean:
	-rm -f *~ 

vflibcap-tex:			../src/vflmkcaptex
	../src/vflmkcaptex standard -i    > $@
vflibcap-tex-ja-pcf:		../src/vflmkcaptex
	../src/vflmkcaptex standard-ja -i > $@
vflibcap-tex-pk:		../src/vflmkcaptex
	../src/vflmkcaptex simple -i      > $@
vflibcap-tex-pk-ja-pcf: 	../src/vflmkcaptex
	../src/vflmkcaptex simple-ja -i   > $@
vflibcap-tex-ja-ttf:		../src/vflmkcaptex \
		../ascii-jtex/fonts.lst ../ascii-jtex/fontsh.lst \
		../ascii-jtex/fontsc.lst
	../src/vflmkcaptex kakugawa -i    > $@
