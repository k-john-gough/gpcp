#!/bin/sh

# ===================================
# This shell script creates a new distribution rooted at ./gpcp-NET
# The file must be executed from $CROOT/sources/gpcp
# Most of the files are recompiled from the sources of the distribution
# rooted at $CROOT using the binaries in $CROOT/bin. However some 
# files (documentation, examples, .NET symbols files) are copied
# from the existing distribution.
# ===================================

#tracing
#set -x

if [ "$CROOT" = "" ]
then
  echo CROOT is not defined, terminating.
  exit 1
fi

echo CROOT = $CROOT
HOME=`pwd`
SOURCES_GPCP=$CROOT/sources/gpcp
TRGT=$HOME/gpcp-NET
# Check if this is being executed from $CROOT/sources/gpcp
if [ "$HOME" != "$SOURCES_GPCP" ]
then
  echo Current directory not $SOURCES_GPCP, terminating
  exit 2
fi

echo HOME = $HOME
echo SOURCES_GPCP = $SOURCES_GPCP

# ===================================
echo Cleaning old gpcp-NET filetree
# ===================================
rm -rf gpcp-NET 

# - Make sure that JavaTarget.cp is the CLR version.
# - And make sure that PeTarget.cp is the NET version.
cp JavaTargetForCLR.cp JavaTarget.cp
cp PeTargetForNET.cp PeTarget.cp

mkdir -p gpcp-NET/bin
mkdir -p gpcp-NET/sources
mkdir -p gpcp-NET/symfiles
mkdir -p gpcp-NET/documentation/Examples
mkdir -p gpcp-NET/symfiles/NetSystem
mkdir -p gpcp-NET/symfiles/HtmlBrowseFiles

# ===================================
echo Building C# library implementations
# ===================================
cd $CROOT/sources/libs/csharp
csc /t:library /debug /nologo RTS.cs \
  && csc /t:library /r:RTS.dll /debug /nologo GPFiles.cs \
  && csc /t:library /r:GPFiles.dll /debug /nologo GPBinFiles.cs \
  && csc /t:library /r:GPFiles.dll /debug /nologo GPTextFiles.cs
if [ $? -gt 0 ]
then
  cd $HOME
  exit 3
fi

# ===================================
echo moving PE files to TRGT/bin
# ===================================
for i in *.dll; do mv $i $TRGT/bin; done
for i in *.pdb; do mv $i $TRGT/bin; done
cd $HOME/libs/csharp
# ===================================
echo compiling MsilAsm 
# ===================================
csc /t:library /debug /nologo MsilAsm.cs
if [ $? -gt 0 ]
then
  cd $HOME
  exit 4
fi

mv MsilAsm.dll $TRGT/bin
mv MsilAsm.pdb $TRGT/bin

# The source of PeToCpsUtils.cp is included.
# It is required to allow PeToCps v1.4.06 to be
# compiled using the gpcp v1.4.04 executables. 
# It is not required when re-compiling with gpcp 1.4.06

# csc /t:library /debug /nologo PeToCpsUtils.cs
# PeToCps PeToCpsUtils.dll
# move PeToCpsUtils.dll %TRGT%\bin
# move PeToCpsUtils.pdb %TRGT%\bin
# move PeToCpsUtils_.cps %TRGT%\symfiles
# if errorlevel 1 goto :fail
# move MsilAsm.dll %TRGT%\bin
# move MsilAsm.pdb %TRGT%\bin

# ===================================
echo Compiling CP library sources
# ===================================
cd $CROOT/sources/libs/cpascal
mono $CROOT/bin/gpcp.exe -special -nowarn \
    ASCII.cp Console.cp CPmain.cp Error.cp GPFiles.cp GPBinFiles.cp \
    GPTextFiles.cp ProgArgs.cp RTS.cp STA.cp StdIn.cp WinMain.cp \
  && mono $CROOT/bin/gpcp.exe -nowarn -bindir=$TRGT/bin/ RealStr.cp StringLib.cp
if [ $? -gt 0 ]
then
  cd $HOME
  exit 5
fi
for i in *.cps; do mv $i $TRGT/symfiles; done
rm -rf *.il

# ===================================
echo NOT Copying the PERWAPI symbol files
#  Have to leave this in until PERWAPI is removed from distro
# ===================================
# CD %CROOT%\symfiles
# copy QUT_*.cps %TRGT%\symfiles

# ===================================
echo Copying the NetSystem symbol files
# ===================================
cd $CROOT/symfiles/NetSystem
for i in *.cps; do cp $i $TRGT/symfiles/NetSystem; done

# ===================================
echo Generating html browse files for NetSystem libraries
# ( -verbose removed PDR 21.12.22 )
# ===================================
cd $TRGT/symfiles/NetSystem
echo DST=$TRGT/symfiles/HtmlBrowseFiles
mono $CROOT/bin/Browse.exe -html -sort \
  -dst:$TRGT/symfiles/HtmlBrowseFiles *.cps
if [ $? -gt 0 ]
then
  cd $HOME
  exit 6
fi

# ===================================
echo Generating html browse files for CP libraries
# ===================================
cd $TRGT/symfiles
mono $CROOT/bin/Browse.exe -html -sort -dst:HtmlBrowseFiles *.cps
if [ $? -gt 0 ]
then
  cd $HOME
  exit 7
fi

# ===================================
echo Generating index for html browse files
# ( -verbose removed PDR 21.12.22 )
# ===================================
mono $CROOT/bin/MakeIndex.exe -dst:HtmlBrowseFiles

# ===================================
echo Building compiler-tools exes and dlls
# ===================================
cd $HOME
mono $CROOT/bin/CPMake.exe -all -bindir=$TRGT/bin/ -nowarn gpcp \
  && mono $CROOT/bin/gpcp.exe -bindir=$TRGT/bin/ -nowarn \
    ModuleHandler.cp SymbolFile.cp CPMake.cp \
  && mono $CROOT/bin/gpcp.exe -bindir=$TRGT/bin/ -nowarn \
    BrowseLookup.cp BrowsePopups.cp Browse.cp \
  && mono $CROOT/bin/CPMake.exe -all -bindir=$TRGT/bin/ -nowarn PeToCps.cp
if [ $? -gt 0 ]
then
  cd $HOME
  exit 8
fi

# ===================================
echo Building MakeIndex.exe
# ===================================
cd $HOME/MakeIndex
mono $CROOT/bin/CPMake.exe -all -bindir=$TRGT/bin/ -nowarn MakeIndex.cp
if [ $? -gt 0 ]
then
  cd $HOME
  exit 9
fi

# ===================================
# ===================================
#  This is only necessary until the new PeToCps
#  uses System.Reflection to build symbol files.
# ===================================
# ===================================
echo NOT Copying PERWAPI to gpcp-NET/bin
# ===================================
# CD %CROOT%\bin
# copy QUT*.* %TRGT%\bin
# ===================================

# ===================================
echo Copying the documentation
# ===================================
cd $CROOT/documentation
for i in *.pdf; do cp $i $TRGT/documentation; done
cd examples
for i in *; do cp $i $TRGT/documentation/Examples; done

# ===================================
echo Getting ready to copy the sources
# ===================================
cd $CROOT
mkdir -p $TRGT/sources/gpcp/libs/java
mkdir -p $TRGT/sources/gpcp/libs/csharp
mkdir -p $TRGT/sources/gpcp/MakeIndex
mkdir -p $TRGT/sources/libs/java
mkdir -p $TRGT/sources/libs/csharp
mkdir -p $TRGT/sources/libs/cpascal

# ===================================
echo Copying GPCP sources
# ===================================
cp sources/env.bat $TRGT
cp sources/env.sh $TRGT
for i in sources/gpcp/*.cp; do cp $i $TRGT/sources/gpcp; done
cp sources/gpcp/MkNetDistro.bat $TRGT/sources/gpcp
cp sources/gpcp/MkNetDistro.sh $TRGT/sources/gpcp
for i in sources/gpcp/MakeIndex/*.cp; do cp $i $TRGT/sources/gpcp/MakeIndex; done
cp sources/gpcp/libs/java/MsilAsm.java $TRGT/sources/gpcp/libs/java
cp sources/gpcp/libs/csharp/MsilAsm.cs $TRGT/sources/gpcp/libs/csharp

# ===================================
echo Copy helper files for .NET bin directory
# ===================================
cp sources/gpcp/CopyNetLibs.bat $TRGT/bin
cp sources/gpcp/_README-NET.txt $TRGT/bin
cp sources/gpcp/CopyNetLibs.bat $TRGT/sources/gpcp
cp sources/gpcp/_README-NET.txt $TRGT/sources/gpcp

# ===================================
echo Copying CP library sources
# ===================================
for i in sources/libs/cpascal/*.cp; do cp $i $TRGT/sources/libs/cpascal; done
for i in sources/libs/cpascal/*.bat; do cp $i $TRGT/sources/libs/cpascal; done
for i in sources/libs/cpascal/*.sh; do cp $i $TRGT/sources/libs/cpascal; done

# ===================================
echo Copying C# library sources
# ===================================
for i in sources/libs/csharp/*.cs; do cp $i $TRGT/sources/libs/csharp; done

# ===================================
echo Copying java library sources
# ===================================
for i in sources/libs/java/*; do cp $i $TRGT/sources/libs/java; done

# ===================================
echo \(No Longer\) Copying PERWAPI-project.zip to gpcp-NET/sources
# ===================================
# copy sources\PERWAPI-project.zip %TRGT%\sources

cd $HOME
