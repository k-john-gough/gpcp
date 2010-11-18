(* ============================================================ *)
(**	Interface to the Jasmin Byte-code assembler.		*)
(*      Copyright (c) John Gough 1999, 2000.			*)
(*	K John Gough, 10th June 1999				*)
(*	Modifications:						*)
(*		Version for GPCP V0.3 April 2000 (kjg)		*)
(* ============================================================ *)
(*	The real code is in JasminAsm.java			*)	
(* ============================================================ *)

FOREIGN MODULE JasminAsm;
  IMPORT GPCPcopyright;

  PROCEDURE Init*();

  PROCEDURE Assemble*(IN fil : ARRAY OF CHAR);

END JasminAsm.
