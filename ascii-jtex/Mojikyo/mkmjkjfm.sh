#!/bin/sh

YMIN=`kpsewhich min10.tfm`
TMIN=`kpsewhich  tmin10.tfm`

YMJK=" mojik101.tfm mojik102.tfm mojik103.tfm mojik104.tfm mojik105.tfm \
       mojik106.tfm mojik107.tfm mojik108.tfm mojik109.tfm mojik110.tfm \
       mojik111.tfm mojik112.tfm mojik113.tfm mojik114.tfm mojik115.tfm \
       mojik116.tfm mojik117.tfm mojik118.tfm mojik119.tfm mojik120.tfm \
       mojik121.tfm "
TMJK=" mojkv101.tfm mojkv102.tfm mojkv103.tfm mojkv104.tfm mojkv105.tfm \
       mojkv106.tfm mojkv107.tfm mojkv108.tfm mojkv109.tfm mojkv110.tfm \
       mojkv111.tfm mojkv112.tfm mojkv113.tfm mojkv114.tfm mojkv115.tfm \
       mojkv116.tfm mojkv117.tfm mojkv118.tfm mojkv119.tfm mojkv120.tfm \
       mojkv121.tfm"


DIR=`echo ${YMIN} | sed 's|/min10.tfm$|/Mojiko|' `
if [ ! -d $DIR ]; then  
  echo "Making directory ${DIR}"
  mkdir ${DIR};
fi
echo "Installing mojik101.tfm, ... ,mojik121.tfm in ${DIR}..."
for f in $YMJK; do
  cp ${YMIN} ${DIR}/$f
done

DIR=`echo ${TMIN} | sed 's|/tmin10.tfm$|/Mojiko|' `
if [ ! -d $DIR ]; then  
  echo "Making directory ${DIR}"
  mkdir ${DIR};
fi
echo "Installing mojkv101.tfm, ... ,mojkv121.tfm in ${DIR}..."
for f in $TMJK; do
  cp ${TMIN} ${DIR}/$f
done

echo "***"
echo "*** Probably, you may have to run 'MakeTeXls-R' or 'mktexlsr' program "
echo "*** to update the 'ls-R' file (database for TeX-related files)."
echo "***"


#end
