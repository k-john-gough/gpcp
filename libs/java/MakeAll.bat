@echo off
REM this compiles all of the standard java-sourced libraries for GPCP
javac -d . Console.java
javac -d . CPJ.java
javac -d . CPJrts.java
javac -d . XHR.java
javac -d . CPmain.java
javac -d . Error.java
javac -d . GPFiles_FILE.java
javac -d . GPFiles.java
javac -d . GPBinFiles_FILE.java
javac -d . GPBinFiles.java
javac -d . GPTextFiles_FILE.java
javac -d . GPTextFiles.java
javac -d . ProcType.java
javac -d . ProgArgs.java
javac -d . RTS.java
javac -d . StdIn.java
javac -d . VecBase.java
javac -d . VecChr.java
javac -d . VecI32.java
javac -d . VecI64.java
javac -d . VecR32.java
javac -d . VecR64.java
javac -d . VecBase.java
javac -d . VecRef.java
