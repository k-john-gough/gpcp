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

 (* Emitter initialization *)
  PROCEDURE (maker : ClassEmitter)Init*(),NEW,EMPTY;

 (* Define features of the type system base type *)
  PROCEDURE (maker : ClassEmitter)ObjectFeatures*(),NEW,EMPTY;

 (* Emit the code for the Module *)
  PROCEDURE (maker : ClassEmitter)Emit*(),NEW,ABSTRACT;

 (* Call the assembler, if necessary *)
  PROCEDURE (asmbl : Assembler)Assemble*(),NEW,EMPTY;

(* ============================================================ *)
END ClassMaker.
(* ============================================================ *)
