#!/bin/sh

case $# in
2)
  ;;
*)
  echo "Usage: mkfonts.sh DBFILE OUTFILE" 
  echo "Example: mkfonts.sh def.dat fonts" 
  exit 1
  ;;
esac


FONTLIST=$1
OUT=$2

VAL=`sed 's/#.*//' $FONTLIST \
     | awk '{ if ((NF != 0) && (NF != 3)) \
                printf("Line %d: %s\n", NR, $0); }'`
if [ ! -z "$VAL" ] 
then
  echo "Input file is broken!" >&2
  echo $VAL
  exit 1
fi


VAL=`sed 's/#.*//' $FONTLIST | grep -v '^[ 	]*$' \
     | awk '{print $3}' | sort | uniq -d `
if [ ! -z "$VAL" ]
then
  echo "Oops!! Font names duplicates!!!"
  echo $VAL
  exit 1
fi

sed 's/#.*//' $FONTLIST  \
  | grep -v '^[ 	]*$' \
  | awk '{print $3, $2}' \
  | sort \
    > ${OUT}.lst


sed 's/#.*//' $FONTLIST \
  | grep -v '^[ 	]*$' \
  | sed 's/\.tt[fc]//' \
  | awk '{printf("\\font\\%s=%s\n", $3, $3)}' \
  | sort \
    > ${OUT}.tex

#EOF
