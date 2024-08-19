@echo off
rem paths to executables
set "CMD=C:\"Program Files"\Python312\python.exe"
set "CXX=clang++.exe"

rem libgretl stack path
set "STACK_PATH=E:\Marcin\gretlapi\stack"

rem Python main path and version
for /f "tokens=*" %%i in ('%CMD% -c "import sysconfig; print(sysconfig.get_paths()[r'data'])"') do set "PYTHON_PATH=%%i"
for /f %%i in ('%CMD% -c "import platform; print(platform.python_version_tuple()[0]+platform.python_version_tuple()[1])"') do set "PYTHON_VER=%%i"

set "CXXFLAGS=-O2 -msahf -pipe -Wp,-D_FORTIFY_SOURCE=2 -fstack-protector-strong -fwrapv -Wall"
rem -DNDEBUG -g

set "GRETLIN=-I%STACK_PATH%\include -I%STACK_PATH%\include\glib-2.0 -I%STACK_PATH%\lib\glib-2.0\include"
set "GRETLLD=-L%STACK_PATH%\lib -l"gretl-1.0" -lm -lxml2 -lfftw3 -l"glib-2.0" -lintl -l"gobject-2.0""

set "PYTHONIN=-I"%PYTHON_PATH%\Include""
set "PYTHONLD=-L"%PYTHON_PATH%\libs" -lpython%PYTHON_VER% -lversion -lshlwapi -lpathcch -lbcrypt"

for /f "tokens=*" %%i in ('%CMD% -c "import pybind11; print(pybind11.get_include())"') do set "PYBIND=-I%%i"
rem set "PYBIND=-IC:\Users\Katedra\AppData\Roaming\Python\Python311\site-packages\pybind11\include"

rem module suffix
for /f %%i in ('%CMD% -c "import sysconfig; print(sysconfig.get_config_var(r'EXT_SUFFIX'))"') do set SUFFIX=%%i
set "MODULE=gretl/_gretl%SUFFIX%"

rem ZIPFILE = gretl4py.zip
rem set "PYFILES=gretl/gretl4py_addons.py gretl/gretl4py_classes.py"

%CXX% %CXXFLAGS% -c src\gretl4py_data.cpp -o gretl4py_data.o %PYBIND% %GRETLIN% %PYTHONIN%
%CXX% %CXXFLAGS% -c src\gretl_calls.cpp -o gretl_calls.o %PYBIND% %GRETLIN% %PYTHONIN%
%CXX% %CXXFLAGS% -c src\gretl4py.cpp -o gretl4py.o %PYBIND% %GRETLIN% %PYTHONIN%
%CXX% %CXXFLAGS% gretl4py.o gretl4py_data.o gretl_calls.o -shared -o %MODULE% %GRETLLD% %PYTHONLD%
