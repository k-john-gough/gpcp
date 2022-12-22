@echo off
if defined CROOT echo CROOT already defined: run this once in a new CMD
if defined CROOT goto end
set CROOT=%CD%
echo CROOT=%CROOT%
set PATH=%CROOT%\bin;%PATH%
REM or, e.g. set PATH=%CROOT%\bin;C:\windows\Microsoft.NET\Framework\v4.0.30319;%PATH%
echo PATH=%PATH%
set CPSYM=.;%CROOT%\symfiles;%CROOT%\symfiles\NetSystem
echo CPSYM=%CPSYM%
:end
