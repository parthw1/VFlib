#!/bin/sh

case $# in
2)
  ;;
*)
  echo "Usage: mkfontsc.sh DBFILE OUTFILE" 
  echo "Example: mkfontsc.sh defc.dat fonts" 
  exit 1
  ;;
esac


FONTLIST=$1
OUT=$2


VAL=`sed 's/#.*//' $FONTLIST \
     | awk '{ if ((NF != 0) && (NF != 5)) \
                printf("Line %d: %s\n", NR, $0); }'`
if [ ! -z "$VAL" ] 
then
  echo "Input file is broken!" >&2
  echo $VAL
  exit 1
fi


VAL=`sed 's/#.*//' $FONTLIST | grep -v '^[ 	]*$' \
     | awk '{print $1}' | sort | uniq -d `
if [ ! -z "$VAL" ]
then
  echo "Oops!! Font names duplicates!!!"
  echo $VAL
  exit 1
fi

sed 's/#.*//' $FONTLIST  \
  | grep -v '^[ 	]*$' \
  | awk '{print $1, $2, $3, $4, $5}' \
  | sort \
    > ${OUT}.lst


sed 's/#.*//' $FONTLIST \
  | grep -v '^[ 	]*$' \
  | sed 's/\.tt[fc]//' \
  | awk '{printf("\\font\\%s=%s\n", $1, $1)}' \
  | sort \
    > ${OUT}.tex

#EOF
