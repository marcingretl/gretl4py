#!/bin/sh

MODULE=$1

if [ "x$MODULE" = "x" ] ; then
   echo "give the gretl4py module name"
   exit 1
fi

EXE_PATH="@executable_path/../lib"
LIB_PATH="/Applications/Gretl.app/Contents/Resources/lib"
TOOL=install_name_tool

for line in `otool -L ${MODULE}`; do
   if grep -q "${EXE_PATH}" <<< "$line"; then
      lib=`awk -F' ' '{print $1}' <<< $line`
      targ=`basename $lib`
      ${TOOL} -change ${EXE_PATH}/$targ ${LIB_PATH}/$targ ${MODULE}
   fi
done


