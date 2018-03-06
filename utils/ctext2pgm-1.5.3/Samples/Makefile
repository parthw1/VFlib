#
# Makefile for Sample documents of ctext2pgm
#

DIRS=   CTEXT \
	CTEXT-HEBREW \
	CTEXT-MULE-ARABIC \
	ISO8859-1-LATIN1 \
	ISO8859-5-CYRILLIC \
	ISO8859-7-GREEK \
	ISO8859-8-HEBREW \
	EUC-JAPANESE \
	SHIFT-JIS

default all clean veryclean distclean clean-images:
	@for d in $(DIRS); do \
	  ( echo $$d; \
	    cd $$d; \
	    $(MAKE) $@) \
	done



#EOF
