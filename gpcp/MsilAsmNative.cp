(* ============================================================ *)
(**	Interface to the ILASM Byte-code assembler.		*)
(*      Copyright (c) John Gough 1999 -- 2002.			*)
(*	K John Gough, 10th June 1999				*)
(*	Modifications:						*)
(*		Version for GPCP V0.3 April 2000 (kjg)		*)
(* ============================================================ *)

MODULE MsilAsm;
  IMPORT 
      RTS,
      Console,
      CompState,
      GPCPcopyright,
      Diag := "[System]System.Diagnostics";

  VAR asm  : Diag.Process;

  PROCEDURE Init*();
  BEGIN
    IF asm = NIL THEN
      NEW(asm);
      asm.get_StartInfo().set_FileName("ilasm");
      asm.get_StartInfo().set_CreateNoWindow(TRUE);
      asm.get_StartInfo().set_UseShellExecute(FALSE);
    END;
  END Init;

  PROCEDURE Assemble*(IN file : ARRAY OF CHAR; 
		      IN optn : ARRAY OF CHAR;	(* "/debug" or "" *)
			 main : BOOLEAN); 	(* /exe or /dll   *)
    VAR retCode : INTEGER;
        optNm   : RTS.NativeString;
        suffx   : RTS.NativeString;
        fName   : RTS.NativeString;
        ignore  : BOOLEAN;
  BEGIN
    fName := MKSTR(file);
    IF main THEN
      optNm := "/exe "; suffx := ".exe";
    ELSE
      optNm := "/dll "; suffx := ".dll";
    END;
    optNm := optNm + MKSTR(optn) + " ";
    asm.get_StartInfo().set_Arguments(optNm+"/nologo /quiet "+fName+".il");
    ignore := asm.Start();
    asm.WaitForExit();
    retCode := asm.get_ExitCode();
    IF retCode # 0 THEN
      Console.WriteString("#gpcp: ilasm FAILED");
      Console.WriteInt(retCode, 0);
    ELSIF ~CompState.quiet THEN
      Console.WriteString("#gpcp: Created " + fName + suffx);
    END;
    Console.WriteLn;
  END Assemble;

  PROCEDURE DoAsm*(IN file : ARRAY OF CHAR; 
		   IN optn : ARRAY OF CHAR;	(* "/debug" or "" *)
		      main : BOOLEAN; 		(* /exe or /dll   *)
		      vbse : BOOLEAN; 		(* verbose or not *)
		  OUT rslt : INTEGER);		(* ilasm ret-code *)
    VAR optNm   : RTS.NativeString;
        suffx   : RTS.NativeString;
        fName   : RTS.NativeString;
        ignore  : BOOLEAN;
  BEGIN
    fName := MKSTR(file);
    IF main THEN
      optNm := "/exe "; suffx := ".exe";
    ELSE
      optNm := "/dll "; suffx := ".dll";
    END;
    optNm := optNm + MKSTR(optn) + " ";
    IF vbse THEN
      asm.get_StartInfo().set_CreateNoWindow(FALSE);
      asm.get_StartInfo().set_Arguments(optNm + "/nologo " + fName + ".il");
    ELSE
      asm.get_StartInfo().set_CreateNoWindow(TRUE);
      asm.get_StartInfo().set_Arguments(optNm+"/nologo /quiet "+fName+".il");
    END;
    ignore := asm.Start();
    asm.WaitForExit();
    rslt := asm.get_ExitCode();
    IF rslt = 0 THEN
      Console.WriteString("#gpcp: Created " + fName + suffx); Console.WriteLn;
    END;
  END DoAsm;

END MsilAsm.
