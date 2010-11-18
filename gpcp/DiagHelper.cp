(* ============================================================ *)
(* 								*)
(*	   Gardens Point Component Pascal Library Module.	*)
(* 	   Copyright (c) K John Gough 1999, 2000		*)
(*	   Created : 26 December 1999 kjg			*)
(* 								*)
(* ============================================================ *)
MODULE DiagHelper;

  IMPORT 
	GPCPcopyright,
	RTS,
	Console;

  PROCEDURE Indent*(j : INTEGER);
    VAR i : INTEGER;
  BEGIN
    Console.WriteString("D:");
    FOR i := 0 TO j-1 DO Console.Write(" ") END;
  END Indent;

  PROCEDURE Class*(IN str : ARRAY OF CHAR; o : ANYPTR; i : INTEGER);
  BEGIN
    Indent(i);
    Console.WriteString(str);
    Console.WriteString(" RTSclass ");
    RTS.ClassMarker(o);
    Console.WriteLn;
  END Class;

END DiagHelper.
