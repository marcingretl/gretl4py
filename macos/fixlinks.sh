#!/bin/sh

MODULE=$1

if [ "x$MODULE" = "x" ] ; then
   echo "give the gretl4py module name"
   exit 1
fi

EXE_PATH="@executable_path"
LIB_PATH="/Applications/Gretl.app/Contents/Resources/lib"
TOOL=install_name_tool

for line in `otool -L ${MODULE}`; do
   if grep -q "${EXE_PATH}" <<< "$line"; then
      lib=`awk -F' ' '{print $1}' <<< $line`
      echo $lib
   fi
done

exit 0

for dlb in libgretl-1.0.*.dylib libglib-2.0.0.dylib libfftw3.3.dylib libomp.dylib ; do
   ${TOOL} -change ${EXE_PATH}/../lib/$dlb ${APP_PATH}/$dlb ${MODULE}
done

