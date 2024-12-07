@ECHO OFF

REM please specify path to python executable
REM SET "CMD=C:\"Program Files"\Python313\python.exe"
SET "CMD=python.exe"

IF NOT EXIST "C:\Program Files\gretl\plugins" GOTO bailout_gretl

FOR /F %%i IN ('%CMD% -c "import sysconfig; print(sysconfig.get_python_version())"') DO SET "PYTHON_VER=%%i"
IF %PYTHON_VER% LSS 3.11 GOTO bailout_pyt

FOR /F "tokens=*" %%i IN ('%CMD% -m site --user-site') DO SET "PYTHON_PATH=%%i"
FOR /F %%i IN ('%CMD% -c "import sysconfig; print(sysconfig.get_config_var(r'EXT_SUFFIX'))"') DO SET SUFFIX=%%i
SET "MODULE_PATH=%PYTHON_PATH%\gretl"
SET "MODULE=_gretl%SUFFIX%"

IF EXIST %MODULE_PATH% RMDIR /S /Q "%MODULE_PATH%"
MKDIR %MODULE_PATH%

COPY gretl\*.py "%MODULE_PATH%"
ECHO builds\%PYTHON_VER%\%MODULE%
COPY "builds\%PYTHON_VER%\%MODULE%" "%MODULE_PATH%"
ECHO.
ECHO run '%CMD% -c "import gretl; gretl.about(True)"' to verify installation
GOTO exit

:bailout_gretl
ECHO gretl4py requires recent UCRT based "win64" gretl build installed in "C:\Program Files\gretl" [default]
GOTO exit

:bailout_pyt
ECHO gretl4py requires python 3.11 or higher, but %PYTHON_VER% was used

:exit
