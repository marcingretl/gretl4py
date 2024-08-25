@echo off

set "CMD=C:\"Program Files"\Python312\python.exe"

for /f "tokens=*" %%i in ('%CMD% -m site --user-site') do set "PYTHON_PATH=%%i"
for /f %%i in ('%CMD% -c "import sysconfig; print(sysconfig.get_python_version())"') do set "PYTHON_VER=%%i"
for /f %%i in ('%CMD% -c "import sysconfig; print(sysconfig.get_config_var(r'EXT_SUFFIX'))"') do set SUFFIX=%%i
set "MODULE_PATH=%PYTHON_PATH%\gretl"
set "MODULE=_gretl%SUFFIX%"

IF EXIST %MODULE_PATH% RMDIR /S /Q "%MODULE_PATH%"
mkdir %MODULE_PATH%
mkdir %MODULE_PATH%\DLL

copy gretl\*.py "%MODULE_PATH%"
copy builds\win64\DLL\*.dll "%MODULE_PATH%"\DLL
copy "builds\win64\%PYTHON_VER%\%MODULE%" "%MODULE_PATH%"
