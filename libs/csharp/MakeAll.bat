csc /t:library /debug RTS.cs
csc /t:library /debug GPFiles.cs
csc /t:library /debug /r:GPFiles.dll GPBinFiles.cs
csc /t:library /debug /r:GPFiles.dll GPTextFiles.cs
