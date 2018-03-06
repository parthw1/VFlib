#! /bin/sh


RECMKDIR=../recmkdir 

INSTALL_DIR=$1
FONTDEF=$2
FONTS_FILE=$3
SRC_TFM_Y=$4
SRC_TFM_T=$5

if [ $# -eq 0 ]
then
  echo "Error."
  echo "  Run:  make install-jfm"
  exit
fi

DUP=`sed 's/#.*//' ${FONTDEF} \
       | grep -v '^[ 	]*$$' \
       | awk '{print $1}' | sort | uniq -d`
if [ ! -z "$DUP" ] ; then
  echo "Duplicated font definition!" $DUP >&2
  exit
fi


if [ ! -f ${FONTS_FILE} ] ; then
  echo ""
  echo "No such file:" ${FONTS_FILE} >&2;
  echo ""
  exit 0;
fi


### KPSEWHICH 
X=`which kpsewhich` 2> /dev/null
if [ "X-$X" = "X-" ] ; then
  echo "Not found: kpsewhich" >&2;
  exit 0;
fi

### TFM_Y
SRC_TFM_Y=`kpsewhich ${SRC_TFM_Y}`
if [ -z "${SRC_TFM_Y}" -o ! -f "${SRC_TFM_Y}" ] ; then
  echo "Not found: ${SRC_TFM_Y}" >&2;
  exit 0;
fi

### TFM_T
SRC_TFM_T=`kpsewhich ${SRC_TFM_T}`
if [ -z "${SRC_TFM_T}" -o ! -f "${SRC_TFM_T}" ] ; then
  echo "Not found: ${SRC_TFM_T}" >&2;
  exit 0;
fi


### JFM list

FONT_LIST=`awk '{ print $1 }' ${FONTS_FILE}`


### JFM dir

if [ ! -d ${INSTALL_DIR} ]
then
echo  ${RECMKDIR}  ${INSTALL_DIR}
  ${RECMKDIR}  ${INSTALL_DIR}
fi

cd ${INSTALL_DIR}


### Make JFMs

for F in ${FONT_LIST}
do
  echo "$F.tfm t$F.tfm ..."
  rm -f $F.tfm t$F.tfm
  cp ${SRC_TFM_Y} $F.tfm
  cp ${SRC_TFM_T} t$F.tfm
     # "ln -s" should not be used, since files does not appear in "ls-R".
done


#EOF
