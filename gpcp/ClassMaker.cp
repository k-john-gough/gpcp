(* ============================================================ *)
(*  ClassMaker is the abstract class for all code emitters.	*)
(*  The method Target.Select(mod, <name>) will allocate a 	*)
(*  ClassMaker  object of an appropriate kind, and will call	*)
(*	classMaker.Emit()					*)
(*  Copyright (c) John Gough 1999, 2000.			*)
(* ============================================================ *)

MODULE ClassMaker;

  IMPORT 
	GPCPcopyright,
	Console,
	IdDesc;

(* ============================================================ *)

  TYPE
    ClassEmitter* = POINTER TO ABSTRACT 
 		    RECORD 
		      mod* : IdDesc.BlkId;
		    END;

    Assembler*    = POINTER TO ABSTRACT 
 		    RECORD 
		    END;

(* ============================================================ *)

  PROCEDURE (maker : ClassEmitter)Init*(),NEW,EMPTY;
  PROCEDURE (maker : ClassEmitter)ObjectFeatures*(),NEW,EMPTY;
  PROCEDURE (maker : ClassEmitter)Emit*(),NEW,ABSTRACT;
  PROCEDURE (asmbl : Assembler)Assemble*(),NEW,EMPTY;

(* ============================================================ *)
END ClassMaker.
(* ============================================================ *)
