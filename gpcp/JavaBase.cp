(* ============================================================ *)
(*  JavaBase is the abstract class for java byte code emitters.	*)
(*  The method Target.Select(mod, <name>) will allocate a 	*)
(*  ClassMaker  object of an appropriate kind, and will call	*)
(*	classMaker.Emit()					*)
(*  Copyright (c) John Gough 1999, 2000. 			*)
(*  Modified DWC September,2000		 			*)
(* ============================================================ *)

MODULE JavaBase;

  IMPORT 
	GPCPcopyright,
	Console,
	Ty := TypeDesc,
	ClassMaker;
	

(* ============================================================ *)

  TYPE
    ClassEmitter* = POINTER TO ABSTRACT 
 		      RECORD (ClassMaker.ClassEmitter) END;

(* ============================================================ *)
(*  Not very elegant, but we need to get at the worklist from   *)
(*  inside static procedures in JsmnUtil.			*)
(* ============================================================ *)

  VAR worklist*   : ClassEmitter;

  PROCEDURE (list : ClassEmitter)AddNewRecEmitter*(rec : Ty.Record),NEW,EMPTY;
  PROCEDURE (list : ClassEmitter)AddNewProcTypeEmitter*(prc : Ty.Procedure),NEW,EMPTY;

(* ============================================================ *)
END JavaBase.
(* ============================================================ *)
