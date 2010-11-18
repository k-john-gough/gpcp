(* ==================================================================== *)
(*									*)
(*  Driver Module for the Gardens Point Component Pascal Compiler.	*)
(*	Copyright (c) John Gough 1999, 2000.				*)
(*	This module was extensively modified from the driver		*)
(*	automatically produced by the M2 version of COCO/R, using	*)
(*	the CPascal.atg grammar used for the JVM version of GPCP.	*)
(*									*)
(* ==================================================================== *)

MODULE gpcp;
  IMPORT 
	GPCPcopyright,
	CPmain,
	GPFiles,
	ProgArgs,
	Main := CPascal;

(* ==================================================================== *)

  VAR
    chr0 : CHAR;
    parN : INTEGER;
    filN : INTEGER;
    junk : INTEGER;
    argN : ARRAY 256 OF CHAR;

(* ==================================================================== *)
(*			      Main Argument Loop			*)
(* ==================================================================== *)

BEGIN
  filN := 0;
  FOR parN := 0 TO ProgArgs.ArgNumber()-1 DO
    ProgArgs.GetArg(parN, argN);
    chr0 := argN[0];
    IF (chr0 = '-') OR (chr0 = GPFiles.optChar) THEN (* option string *)
      Main.DoOption(argN$);
    ELSE
      Main.Compile(argN$, junk); INC(filN);
    END;
  END;
  IF filN = 0 THEN Main.Message("No input files specified") END;
 (*
  *  Return the result code of the final compilation
  *)
  IF junk # 0 THEN HALT(1) END;
END gpcp.

(* ==================================================================== *)
