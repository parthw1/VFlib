#! /bin/sh

gzip -cd < ../UNICODE/ISO8859/8859-5.TXT.gz \
  | sed 's/#.*//' \
  | grep -v '^$' \
  | awk '{printf("%s %s\n", $2, $1); }'  \
  | sort +0 > __tmp1.txt

gzip -cd < KOI8-R.TXT.gz \
  | sed 's/#.*//' \
  | grep -v '^$' \
  | awk '{printf("%s %s\n", $2, $1); }'  \
  | sort +0 > __tmp2.txt

rm -f ISO8859-5-TO-KOI8-R.TXT  ISO8859-5-TO-KOI8-R.TXT.gz 
touch ISO8859-5-TO-KOI8-R.TXT
echo '# MAPPING TABLE FOR ISO8859-5 TO KOI8-R ' >> ISO8859-5-TO-KOI8-R.TXT
echo '# BY HIROTSUGU KAKUGAWA '                 >> ISO8859-5-TO-KOI8-R.TXT
echo '#  '                                      >> ISO8859-5-TO-KOI8-R.TXT
echo '#  COLUMN #1: ISO8859-5'                  >> ISO8859-5-TO-KOI8-R.TXT
echo '#  COLUMN #2: KOI8-R'                     >> ISO8859-5-TO-KOI8-R.TXT
echo '#  COLUMN #3: UNICODE'                    >> ISO8859-5-TO-KOI8-R.TXT
echo '#  '                                      >> ISO8859-5-TO-KOI8-R.TXT
join __tmp1.txt __tmp2.txt \
  | awk '{printf("%s\t%s\t%s\n", $2, $3, $1); }'  \
  | sort +0 >> ISO8859-5-TO-KOI8-R.TXT
gzip ISO8859-5-TO-KOI8-R.TXT

rm -f KOI8-R-TO-ISO8859-5.TXT KOI8-R-TO-ISO8859-5.TXT.gz
touch KOI8-R-TO-ISO8859-5.TXT
echo '# MAPPING TABLE FOR ISO8859-5 TO KOI8-R ' >> KOI8-R-TO-ISO8859-5.TXT
echo '# BY HIROTSUGU KAKUGAWA '                 >> KOI8-R-TO-ISO8859-5.TXT
echo '#  '                                      >> KOI8-R-TO-ISO8859-5.TXT
echo '#  COLUMN #1: ISO8859-5'                  >> KOI8-R-TO-ISO8859-5.TXT
echo '#  COLUMN #2: KOI8-R'                     >> KOI8-R-TO-ISO8859-5.TXT
echo '#  COLUMN #3: UNICODE'                    >> KOI8-R-TO-ISO8859-5.TXT
echo '#  '                                      >> KOI8-R-TO-ISO8859-5.TXT
join __tmp1.txt __tmp2.txt \
  | awk '{printf("%s\t%s\t%s\n", $3, $2, $1); }'  \
  | sort +0 > KOI8-R-TO-ISO8859-5.TXT
gzip KOI8-R-TO-ISO8859-5.TXT

rm -f __tmp1.txt __tmp2.txt

#EOF
