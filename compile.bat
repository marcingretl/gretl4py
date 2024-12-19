@ECHO off

REM This program is aimed FOR native MS Windows compilation only.
REM If you want to compile gretl4py module under MSYS2,
REM pleas look at 'winmsys2.mk'.
REM Note: you need a gretlapi MS SDK stack.

REM paths to executables
REM SET "CMD=C:\"Program Files"\Python311\python.exe"
SET "CMD=python.exe"
SET "CXX=clang++.exe"

REM libgretl stack path
SET "STACK_PATH=E:\Marcin\gretlapi\stack_minimal"

REM Python main path and version
FOR /f "tokens=*" %%i IN ('%CMD% -c "import sysconfig; print(sysconfig.get_paths()[r'data'])"') DO SET "PYTHON_PATH=%%i"
FOR /f %%i IN ('%CMD% -c "import sysconfig; print(sysconfig.get_config_var(r'VERSION'))"') DO SET "PYTHON_VER=%%i"

SET "CXXFLAGS=-march=x86-64 -mtune=generic -O3 -msahf -pipe -Wp,-D_FORTIFY_SOURCE=2 -fstack-protector-strong -fwrapv -Wall -std=c++17"
REM -DNDEBUG -g

SET "GRETLIN=-I%STACK_PATH%\include -I%STACK_PATH%\include\glib-2.0 -I%STACK_PATH%\lib\glib-2.0\include"
SET "GRETLLD=-L%STACK_PATH%\lib -l"gretl-1.0" -lm -lxml2 -lfftw3 -l"glib-2.0" -lintl -l"gobject-2.0""

SET "PYTHONIN=-I"%PYTHON_PATH%\Include""
SET "PYTHONLD=-L"%PYTHON_PATH%\libs" -lpython%PYTHON_VER% -lversion -lshlwapi -lpathcch -lbcrypt"

FOR /f "tokens=*" %%i IN ('%CMD% -c "import pybind11; print(pybind11.get_include())"') DO SET "PYBIND=-I%%i"

REM module suffix
FOR /f %%i IN ('%CMD% -c "import sysconfig; print(sysconfig.get_config_var(r'EXT_SUFFIX'))"') DO SET SUFFIX=%%i
SET "MODULE=gretl/_gretl%SUFFIX%"

%CXX% %CXXFLAGS% -c src\gretl4py_data.cpp -o gretl4py_data.o %PYBIND% %GRETLIN% %PYTHONIN%
%CXX% %CXXFLAGS% -c src\gretl_calls.cpp -o gretl_calls.o %PYBIND% %GRETLIN% %PYTHONIN%
%CXX% %CXXFLAGS% -c src\gretl4py.cpp -o gretl4py.o %PYBIND% %GRETLIN% %PYTHONIN%
%CXX% %CXXFLAGS% gretl4py.o gretl4py_data.o gretl_calls.o -shared -o %MODULE% %GRETLLD% %PYTHONLD%
