
@echo off
REM 
REM ===================================
REM This batch file creates a new distribution rooted at .\gpcp-NET
REM The file must be executed from %CROOT%\sources\gpcp
REM Most of the files are recompiled from the sources of the distribution
REM rooted at %CROOT% using the binaries in %CROOT%\bin. However some 
REM files (documentation, examples, .NET symbols files) are copied
REM from the existing distribution.
REM ===================================
REM
if defined CROOT goto :init
echo CROOT is not defined, terminating.
goto :EOF

:init
echo CROOT = %CROOT%
setlocal
set HOME=%CD%
set SOURCES_GPCP=%CROOT%\sources\gpcp
set TRGT=%HOME%\gpcp-NET
REM Check if this is being executed from %CROOT%\sources\gpcp
if /i %HOME% == %SOURCES_GPCP% goto :clean
echo Current directory not %SOURCES_GPCP%, terminating
goto :EOF

:clean
REM ===================================
echo Cleaning old gpcp-NET filetree
REM ===================================
if exist gpcp-NET rmdir /s /q gpcp-NET 

REM
REM - Make sure that JavaTarget.cp is the CLR version.
REM
copy JavaTargetForCLR.cp JavaTarget.cp

mkdir gpcp-NET\bin
mkdir gpcp-NET\sources
mkdir gpcp-NET\symfiles
mkdir gpcp-NET\documentation\Examples
mkdir gpcp-NET\symfiles\NetSystem
mkdir gpcp-NET\symfiles\HtmlBrowseFiles

REM ===================================
echo Building C# library implementations
REM ===================================
CD %CROOT%\sources\libs\csharp
set TOOL="CSC"
csc /t:library /debug /nologo RTS.cs
if errorlevel 1 goto :fail
csc /t:library /debug /nologo GPFiles.cs
if errorlevel 1 goto :fail
csc /t:library /r:GPFiles.dll /debug /nologo GPBinFiles.cs
if errorlevel 1 goto :fail
csc /t:library /r:GPFiles.dll /debug /nologo GPTextFiles.cs
if errorlevel 1 goto :fail
REM ===================================
echo moving PE files to TRGT\bin
REM ===================================
move *.dll %TRGT%\bin
move *.pdb %TRGT%\bin
CD %HOME%\libs\csharp
REM ===================================
echo compiling MsilAsm 
REM ===================================
csc /t:library /debug /nologo MsilAsm.cs
if errorlevel 1 goto :fail
move MsilAsm.dll %TRGT%\bin
move MsilAsm.pdb %TRGT%\bin
REM
REM The source of PeToCpsUtils.cp is included.
REM It is required to allow PeToCps v1.4.06 to be
REM compiled using the gpcp v1.4.04 executables. 
REM It is not required when re-compiling with gpcp 1.4.06
REM
REM csc /t:library /debug /nologo PeToCpsUtils.cs
REM PeToCps PeToCpsUtils.dll
REM move PeToCpsUtils.dll %TRGT%\bin
REM move PeToCpsUtils.pdb %TRGT%\bin
REM move PeToCpsUtils_.cps %TRGT%\symfiles
REM if errorlevel 1 goto :fail
REM move MsilAsm.dll %TRGT%\bin
REM move MsilAsm.pdb %TRGT%\bin
REM 
REM ===================================
echo Compiling CP library sources
REM ===================================
set TOOL="gpcp"
CD %CROOT%\sources\libs\cpascal
%CROOT%\bin\gpcp.exe /special /nowarn ASCII.cp Console.cp CPmain.cp Error.cp GPFiles.cp GPBinFiles.cp GPTextFiles.cp ProgArgs.cp RTS.cp STA.cp StdIn.cp WinMain.cp
if errorlevel 1 goto :fail
%CROOT%\bin\gpcp.exe /nowarn /bindir=%TRGT%\bin RealStr.cp StringLib.cp
if errorlevel 1 goto :fail
move *.cps %TRGT%\symfiles
del *.il /q

REM ===================================
echo NOT Copying the PERWAPI symbol files
REM  Have to leave this in until PERWAPI is removed from distro
REM ===================================
REM CD %CROOT%\symfiles
REM copy QUT_*.cps %TRGT%\symfiles

REM ===================================
echo Copying the NetSystem symbol files
REM ===================================
CD %CROOT%\symfiles\NetSystem
copy *.cps %TRGT%\symfiles\NetSystem

REM ===================================
echo Generating html browse files for NetSystem libraries
REM ===================================
CD %TRGT%\symfiles\NetSystem
echo DST=%TRGT%\symfiles\HtmlBrowseFiles
%CROOT%\bin\Browse.exe /verbose /html /sort /dst:%TRGT%\symfiles\HtmlBrowseFiles *.cps
if errorlevel 1 goto :fail

REM ===================================
echo Generating html browse files for CP libraries
REM ===================================
CD %TRGT%\symfiles
set TOOL="Browse"
%CROOT%\bin\Browse.exe /html /sort /dst:HtmlBrowseFiles *.cps

REM ===================================
echo Generating index for html browse files
REM ===================================
%CROOT%\bin\MakeIndex.exe /verbose /dst:HtmlBrowseFiles

REM ===================================
echo Building compiler-tools exes and dlls
REM ===================================
CD %HOME%
set TOOL="CPMake"
%CROOT%\bin\CPMake.exe /all /bindir=%TRGT%\bin /nowarn gpcp
if errorlevel 1 goto :fail
set TOOL="gpcp"
%CROOT%\bin\gpcp.exe /bindir=%TRGT%\bin /nowarn ModuleHandler.cp SymbolFile.cp CPMake.cp
if errorlevel 1 goto :fail
%CROOT%\bin\gpcp.exe /bindir=%TRGT%\bin /nowarn Browse.cp
if errorlevel 1 goto :fail
%CROOT%\bin\CPMake.exe /all /bindir=%TRGT%\bin /nowarn PeToCps.cp
if errorlevel 1 goto :fail

REM ===================================
echo Building MakeIndex.exe
REM ===================================
set TOOL="MakeIndex"
CD %HOME%\MakeIndex
%CROOT%\bin\CPMake.exe /all /bindir=%TRGT%\bin /nowarn MakeIndex
if errorlevel 1 goto :fail

REM ===================================
REM ===================================
REM  This is only necessary until the new PeToCps
REM  uses System.Reflection to build symbol files.
REM ===================================
REM ===================================
echo NOT Copying PERWAPI to gpcp-NET\bin
REM ===================================
REM CD %CROOT%\bin
REM copy QUT*.* %TRGT%\bin
REM ===================================

REM ===================================
echo Copying the documentation
REM ===================================
CD %CROOT%\documentation
copy *.pdf %TRGT%\documentation
CD examples
copy *.* %TRGT%\documentation\Examples

REM ===================================
echo Getting ready to copy the sources
REM ===================================
CD %CROOT%
mkdir %TRGT%\sources\gpcp\libs\java
mkdir %TRGT%\sources\gpcp\libs\csharp
mkdir %TRGT%\sources\gpcp\MakeIndex
mkdir %TRGT%\sources\libs\java
mkdir %TRGT%\sources\libs\csharp
mkdir %TRGT%\sources\libs\cpascal

REM ===================================
echo Copying GPCP sources
REM ===================================
copy sources\gpcp\*.cp %TRGT%\sources\gpcp
copy sources\gpcp\MkNetDistro.bat %TRGT%\sources\gpcp
copy sources\gpcp\MakeIndex\*.cp %TRGT%\sources\gpcp\MakeIndex
copy sources\gpcp\libs\java\MsilAsm.java %TRGT%\sources\gpcp\libs\java
copy sources\gpcp\libs\csharp\MsilAsm.cs %TRGT%\sources\gpcp\libs\csharp

REM ===================================
echo Copy helper files for .NET bin directory
REM ===================================
copy sources\gpcp\CopyNetLibs.bat %TRGT%\bin
copy sources\gpcp\_README-NET.txt %TRGT%\bin
copy sources\gpcp\CopyNetLibs.bat %TRGT%\sources\gpcp
copy sources\gpcp\_README-NET.txt %TRGT%\sources\gpcp

REM ===================================
echo Copying CP library sources
REM ===================================
copy sources\libs\cpascal\*.cp %TRGT%\sources\libs\cpascal
copy sources\libs\cpascal\*.bat %TRGT%\sources\libs\cpascal

REM ===================================
echo Copying C# library sources
REM ===================================
copy sources\libs\csharp\*.cs %TRGT%\sources\libs\csharp

REM ===================================
echo Copying java library sources
REM ===================================
copy sources\libs\java\*.* %TRGT%\sources\libs\java

REM ===================================
echo (Still) Copying PERWAPI-project.zip to gpcp-NET\sources
REM ===================================
copy sources\PERWAPI-project.zip %TRGT%\sources

REM ===================================
echo BUILD SUCCEDED
REM ===================================
CD %HOME%
goto :EOF

:fail
REM ===================================
echo BUILD FAILED IN %TOOL%
REM ===================================
CD %HOME%
REM
REM End of script
REM

