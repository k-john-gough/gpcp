(* ==================================================================== *)
(*									*)
(*  FileNames Module for the Gardens Point Component Pascal Compiler.	*)
(*	Copyright (c) John Gough 1999, 2000.				*)
(*									*)
(* ==================================================================== *)

MODULE FileNames;
  IMPORT GPFiles;

  TYPE
	NameString* = ARRAY 128 OF CHAR;

(* ==================================================================== *)
(*				Utilities				*)
(* ==================================================================== *)

    PROCEDURE GetBaseName*(IN in : ARRAY OF CHAR; OUT out : ARRAY OF CHAR);
      VAR sPos : INTEGER;
	  dPos : INTEGER;
	  idx  : INTEGER;
	  chr  : CHAR;
    BEGIN
     (* ------------------------- *)
     (* Index 012...      v--LEN  *)
     (*       xxxxxxxxxxxx0       *)
     (*      ^---------------sPos *)
     (*                   ^--dPos *)
     (* ------------------------- *)
     (*           before          *)
     (* ------------------------- *)
     (*                           *) 
     (*       xxx/xxxxx.xx0       *)
     (*          ^-----------sPos *)
     (*                ^-----dPos *)
     (* ------------------------- *)
     (*           after           *)
     (* ------------------------- *)
      sPos := -1;
      dPos := LEN(in$); 
      FOR idx := 0 TO dPos DO
	chr := in[idx];
	IF chr = '.' THEN 
	  dPos := idx;
	ELSIF chr = GPFiles.fileSep THEN 
	  sPos := idx;
	END;
      END;
      FOR idx := 0 TO dPos-sPos-1 DO
	out[idx] := in[idx+sPos+1];
      END;
      out[dPos-sPos] := 0X;
    END GetBaseName;

(* ==================================================================== *)

    PROCEDURE StripUpToLast*(mark : CHAR; 
			 IN  in   : ARRAY OF CHAR;
			 OUT out  : ARRAY OF CHAR);
      VAR sPos : INTEGER;
	  dPos : INTEGER;
	  idx  : INTEGER;
	  chr  : CHAR;
    BEGIN
     (* ------------------------- *)
     (* Index 012...      v--LEN  *)
     (*       xxxxxxxxxxxx0       *)
     (*      ^---------------sPos *)
     (*                   ^--dPos *)
     (* ------------------------- *)
     (*           before          *)
     (* ------------------------- *)
     (*                           *) 
     (*       xxx!xxxxx.xx0       *)
     (*          ^-----------sPos *)
     (*                ^-----dPos *)
     (* ------------------------- *)
     (*           after           *)
     (* ------------------------- *)
      sPos := -1;
      dPos := LEN(in$); 
      FOR idx := 0 TO dPos DO
	chr := in[idx];
	IF chr = '.' THEN 
	  dPos := idx;
	ELSIF chr = mark THEN 
	  sPos := idx;
	END;
      END;
      FOR idx := 0 TO dPos-sPos-1 DO
	out[idx] := in[idx+sPos+1];
      END;
      out[dPos-sPos] := 0X;
    END StripUpToLast;

(* ==================================================================== *)

    PROCEDURE AppendExt*(IN in,ext : ARRAY OF CHAR; OUT out : ARRAY OF CHAR);
      VAR pos : INTEGER;
	  idx : INTEGER;
	  chr : CHAR;
    BEGIN
      pos := LEN(in$);
      FOR idx := 0 TO pos-1 DO out[idx] := in[idx] END;
      out[pos] := ".";
      FOR idx := 0 TO LEN(ext$) DO out[idx+pos+1] := ext[idx] END;
    END AppendExt;

(* ==================================================================== *)

    PROCEDURE StripExt*(IN in : ARRAY OF CHAR; OUT out : ARRAY OF CHAR);
      VAR pos : INTEGER;
	  idx : INTEGER;
	  chr : CHAR;
    BEGIN
      pos := LEN(in$);
      FOR idx := 0 TO pos DO
	chr := in[idx];
	IF chr = '.' THEN pos := idx END;
	out[idx] := chr;
      END;
      out[pos] := 0X;
    END StripExt;

(* ==================================================================== *)
BEGIN
END FileNames.

