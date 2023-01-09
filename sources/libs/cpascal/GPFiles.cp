FOREIGN MODULE GPFiles;
  TYPE
	FILE* = POINTER TO ABSTRACT RECORD END;
	FileNameArray* = POINTER TO ARRAY OF CHAR;

  VAR
	pathSep- : CHAR; (* path separator on this platform *)
	fileSep- : CHAR; (* filename separator character    *)
	optChar- : CHAR; (* option introduction character   *)

PROCEDURE isOlder*(first : FILE; second : FILE) : BOOLEAN;

PROCEDURE MakeDirectory*(dirName : ARRAY OF CHAR);

PROCEDURE CurrentDirectory*(): FileNameArray;

PROCEDURE exists*(fName : ARRAY OF CHAR) : BOOLEAN;

PROCEDURE FileList*(IN dirPath : ARRAY OF CHAR) : 
              POINTER TO ARRAY OF FileNameArray;

END GPFiles.


