#!/bin/sh

OPT_VFLIBCAP=vflibcap-tex

./vfl2bdf -o min48.bdf  -p 48 -v ${OPT_VFLIBCAP} \
         -font-family Mincho.Fixed -94x94 \
         -charset-registry jisx0208.1990 -charset-encoding 0 \
         min10.pk  0x2121 0x747e
bdftopcf min48.bdf > min48.pcf

./vfl2bdf -o minh48.bdf -p 48 -v ${OPT_VFLIBCAP} \
         -font-family Mincho.Fixed -94x94 \
         -charset-registry jisx0212.1990 -charset-encoding 0 \
         minh10.pk 0x2121 0x747e
bdftopcf minh48.bdf > minh48.pcf

./vfl2bdf -o 24x48.bdf -p 48 -v ${OPT_VFLIBCAP} \
         -font-family Fixed \
         -charset-registry iso8859 -charset-encoding 1 \
         12x24.pcf 0x00 0xff
bdftopcf 24x48.bdf > 24x48.pcf
