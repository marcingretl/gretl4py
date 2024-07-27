#!/bin/sh

INCDIR=`pkg-config --cflags-only-I gretl | awk '{print $1}' | sed s/-I//`

cat /dev/null > more_calls.h
cat /dev/null > more_calls.cpp
cat /dev/null > more_mod.cpp

for h in `find $INCDIR/gretl -name "*.h" -print` ; do
   if grep ^MODEL $h > /dev/null ; then
      echo
      echo "*** `basename $h` ***"
      ./model_funcs $h
   fi
done
