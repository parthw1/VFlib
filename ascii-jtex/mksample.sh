#!/bin/sh

# Generate sample.tex and sample.dvi from fonts.tex file.

SAMPLE=sample
SAMPLE_TEX=sample.tex
FONTDEF=fonts.tex
TEXT="秋の田のかりほの庵の苫をあらみわが衣手は露にぬれつつ （天智天皇）"

rm -f tmp1.txt tmp1.txt
echo ${TEXT} > tmp1.txt
FONTS=`cat ${FONTDEF} | sed 's/\\\\font\\\\\(.*\)=.*/\1/'`

rm -f ${SAMPLE}.*
touch ${SAMPLE_TEX}

cat >> ${SAMPLE_TEX} <<__EOF__
\input fonts.tex
__EOF__

for fn in ${FONTS}
do
  echo -n "{\tt $fn}\quad " >> ${SAMPLE_TEX}
  echo -n "\\$fn "          >> ${SAMPLE_TEX}
  cat tmp1.txt              >> ${SAMPLE_TEX}
  echo '\par'               >> ${SAMPLE_TEX}
done

cat >> ${SAMPLE_TEX} <<__EOF__
\bye
__EOF__

ptex ${SAMPLE_TEX}
rm -f ${SAMPLE}.log 
rm -f tmp1.txt 
