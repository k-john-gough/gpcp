
REM @echo off
REM this compiles all of the standard java-sourced libraries for GPCP
mkdir dest
javac -d dest Console.java
javac -d dest CPJ.java
javac -d dest CPJrts.java
javac -d dest XHR.java
javac -d dest CPmain.java
javac -d dest Error.java
javac -d dest GPFiles_FILE.java
javac -d dest GPFiles.java
javac -d dest GPBinFiles_FILE.java
javac -d dest GPBinFiles.java
javac -d dest GPTextFiles_FILE.java
javac -d dest GPTextFiles.java
javac -d dest ProcType.java
javac -d dest ProgArgs.java
javac -d dest RTS.java
javac -d dest StdIn.java
javac -d dest VecBase.java
javac -d dest VecChr.java
javac -d dest VecI32.java
javac -d dest VecI64.java
javac -d dest VecR32.java
javac -d dest VecR64.java
javac -d dest VecBase.java
javac -d dest VecRef.java
cd dest
jar cvf cprts.jar .
copy cprts.jar ..
cd ..
pause

