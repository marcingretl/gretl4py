#!/bin/sh

HERE=`pwd`
MYDIR=`python3 -m site --user-site`

if [ ! -d $MYDIR ] ; then
   mkdir -p $MYDIR || exit 1
fi

cd $MYDIR && unzip -o $HERE/gretl4py.zip
