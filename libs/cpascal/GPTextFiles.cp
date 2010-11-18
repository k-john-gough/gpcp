FOREIGN MODULE GPTextFiles;

IMPORT GPFiles;

TYPE
  FILE* = POINTER TO RECORD (GPFiles.FILE) END;


PROCEDURE findLocal*(IN fileName : ARRAY OF CHAR) : FILE;
(** Find file with given name in current directory *)

PROCEDURE findOnPath*(IN pathName : ARRAY OF CHAR;
		      IN fileName : ARRAY OF CHAR) : FILE;
(** Find file with given name on path given as property *)

PROCEDURE getFullPathName*(f : FILE) : GPFiles.FileNameArray;
(** Return full name of file *)

PROCEDURE openFile*(IN fileName : ARRAY OF CHAR) : FILE;
(** Open file with given absolute name *)

PROCEDURE openFileRO*(IN fileName : ARRAY OF CHAR) : FILE;
(** Open file READ-ONLY with given absolute name *)

PROCEDURE CloseFile*(file : FILE);

PROCEDURE createFile*(IN fileName : ARRAY OF CHAR) : FILE;
(** Create file and open for reading *)

PROCEDURE createPath*(IN pathName : ARRAY OF CHAR) : FILE;
(** Create file and any necessary directories and opens file for reading *)

PROCEDURE readChar*(file : FILE) : CHAR;
 
PROCEDURE readNChars*(file : FILE; OUT buffPtr : ARRAY OF CHAR;
                     requestedChars : INTEGER) : INTEGER;
(** Return value is number actually read *)
 
PROCEDURE WriteEOL*(file : FILE);

PROCEDURE WriteChar*(file : FILE; ch : CHAR);

PROCEDURE WriteNChars*(file : FILE; IN buffPtr : ARRAY OF CHAR;
                      requestedChars : INTEGER);

END GPTextFiles.
