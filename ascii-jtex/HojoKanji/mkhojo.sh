#!/bin/sh

# mkhojo.sh
#   --- make TFM files for Hojo Kanji (JISX0212) fonts.
#  
#  by H. Kakugawa


FONTS="minh:hojo.pl:MINCHO:0 gothh:hojo.pl:GOTHIC:2 \
       tminh:hojot.pl:MINCHO:MRR tgothh:hojot.pl:GOTHIC:BRR"
POINTS="5 6 7 8 9 10"


case $# in
0)
  ;;
*)
  case $1 in
  -h|--help|-*)
    echo "mkhojo.sh" >&2
    echo "  -  A shell script to make JFM (TFM) files for JISX0212" >&2
    echo "    (Hojo Kanji) character set. All Kanji characters are the" >&2
    echo "    same size and no kernings between characters." >&2
    exit 1
    ;;
  esac
  ;;
esac


for f in ${FONTS};  do
  font=`echo $f | sed 's/^\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\)$/\1/g'`
  srcf=`echo $f | sed 's/^\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\)$/\2/g'`
  fami=`echo $f | sed 's/^\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\)$/\3/g'`
  face=`echo $f | sed 's/^\([^:]*\):\([^:]*\):\([^:]*\):\([^:]*\)$/\4/g'`
  echo "Making ${font}..."
  for p in ${POINTS};  do
    PL=${font}${p}.pl
    TFM=${font}${p}.tfm
    cat ${srcf} \
      |sed "s/@FAMILY@/${fami}/g" \
      | sed "s/@FACE@/${face}/g" \
      | sed "s/@SIZE@/${p}/g" \
      | cat > ${PL}
    pltotf ${PL} ${TFM}  >/dev/null
    rm -f ${PL}
  done 
done 
echo "done."

#EOF
