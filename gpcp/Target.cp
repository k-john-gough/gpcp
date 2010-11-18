(* ============================================================ *)
(*  Target is the module which selects the target ClassMaker.	*)
(*  Copyright (c) John Gough 1999, 2000.			*)
(* ============================================================ *)

MODULE Target;

  IMPORT 
	GPCPcopyright,
	Symbols,
	CompState,
	ClassMaker,
	JavaMaker,
(*
 *	DCodeMaker,
 *)
	MsilMaker,
	IdDesc;

(* ============================================================ *)

  VAR
    maker : ClassMaker.ClassEmitter;
    assmb : ClassMaker.Assembler;

(* ============================================================ *)

  PROCEDURE Select*(mod : IdDesc.BlkId; 
		 IN str : ARRAY OF CHAR);
  BEGIN
    IF str = "jvm" THEN
      maker := JavaMaker.newJavaEmitter(mod);
      assmb := JavaMaker.newJavaAsm();
      Symbols.SetTargetIsNET(FALSE);
    ELSIF str = "net" THEN
      maker := MsilMaker.newMsilEmitter(mod);
      assmb := MsilMaker.newMsilAsm();
      Symbols.SetTargetIsNET(TRUE);
(*
 *  ELSIF str = "dcf" THEN
 *    maker := DCodeMaker.newDCodeEmitter(mod);		(* write DCode	*)
 *    assmb := DCodeMaker.newDCodeAsm();		(* call dgen	*)
 *  ELSIF ...
 *)
    ELSE
      CompState.Message("Unknown emitter name <" + str + ">");
    END;
  END Select;

(* ============================================================ *)

  PROCEDURE Init*();
  BEGIN
    maker.Init();
  END Init;

  PROCEDURE Emit*();
  BEGIN
    maker.Emit();
  END Emit;

  PROCEDURE Assemble*();
  BEGIN
    assmb.Assemble();
  END Assemble;

(* ============================================================ *)
END Target.
(* ============================================================ *)
