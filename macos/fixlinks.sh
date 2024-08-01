#!/bin/sh

MODULE=$1

if [ "x$MODULE" = "x" ] ; then
   echo "give the gretl4py module name"
   exit 1
fi

EXE_PATH="@executable_path"
LIB_PATH="/Applications/Gretl.app/Contents/Resources/lib"

for dlb in libgretl-1.0.*.dylib libglib-2.0.0.dylib libfftw3.3.dylib libomp.dylib ; do
   install_name_tool -change ${EXE_PATH}/../lib/$dlb ${APP_PATH}/$dlb \
   ${MODULE}
done

