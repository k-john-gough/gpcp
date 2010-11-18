FOREIGN MODULE GPBinFiles;

IMPORT GPFiles;

TYPE
  FILE* = POINTER TO RECORD (GPFiles.FILE) END;


PROCEDURE length*(f : FILE) : INTEGER;

PROCEDURE findLocal*(IN fileName : ARRAY OF CHAR) : FILE;

PROCEDURE findOnPath*(IN pathName : ARRAY OF CHAR;
		      IN fileName : ARRAY OF CHAR) : FILE;

PROCEDURE getFullPathName*(f : FILE) : GPFiles.FileNameArray;

PROCEDURE openFile*(IN fileName : ARRAY OF CHAR) : FILE;
PROCEDURE openFileRO*(IN fileName : ARRAY OF CHAR) : FILE;

PROCEDURE CloseFile*(file : FILE);

PROCEDURE createFile*(IN fileName : ARRAY OF CHAR) : FILE;

PROCEDURE createPath*(IN pathName : ARRAY OF CHAR) : FILE;

PROCEDURE EOF*(file : FILE) : BOOLEAN;

PROCEDURE readByte*(file : FILE) : INTEGER;
 
PROCEDURE readNBytes*(file : FILE; OUT buffPtr : ARRAY OF UBYTE;
                     requestedBytes : INTEGER) : INTEGER;
 
PROCEDURE WriteByte*(file : FILE; b : INTEGER);

PROCEDURE WriteNBytes*(file : FILE; IN buffPtr : ARRAY OF UBYTE;
                       requestedBytes : INTEGER);

END GPBinFiles.
