#!/bin/sh

INCDIR=`pkg-config --cflags-only-I gretl | awk '{print $1}' | sed s/-I//`

for h in `find $INCDIR/gretl -name "*.h" -print` ; do
   if grep ^MODEL $h > /dev/null ; then
      echo
      echo "*** `basename $h` ***"
      ./model_funcs $h
   fi
done
