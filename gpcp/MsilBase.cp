(* ============================================================ *)
(*  MsilBase is the abstract class for MSIL code		*)
(*  emitters. The method Target.Select(mod, <name>) will 	*)
(*  allocate a ClassMaker  object of an appropriate kind, and 	*)
(*  will call classMaker.Emit()					*)
(*  Copyright (c) John Gough 1999, 2000.			*)
(* ============================================================ *)

MODULE MsilBase;

  IMPORT 
	GPCPcopyright,
	Console,
	Sy := Symbols,
	ClassMaker;
	

(* ============================================================ *)

  TYPE
    ClassEmitter* = POINTER TO ABSTRACT 
 		      RECORD (ClassMaker.ClassEmitter) END;

(* ============================================================ *)
(*  Not very elegant, but we need to get at the worklist from   *)
(*  inside static procedures in IlasmUtil.			*)
(* ============================================================ *)

  VAR emitter* : ClassEmitter;

  PROCEDURE (list : ClassEmitter)AddNewRecEmitter*(inTp : Sy.Type),NEW,EMPTY;

(* ============================================================ *)
END MsilBase.
(* ============================================================ *)
