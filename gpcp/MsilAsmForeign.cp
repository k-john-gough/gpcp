(* ============================================================ *)
(**	Interface to the ILASM Byte-code assembler.		*)
(*      Copyright (c) John Gough 1999, 2000.			*)
(*	K John Gough, 10th June 1999				*)
(*	Modifications:						*)
(*		Version for GPCP V0.3 April 2000 (kjg)		*)
(* ============================================================ *)
(*	The real code is in MsilAsm.cs				*)	
(* ============================================================ *)

FOREIGN MODULE MsilAsm;
  IMPORT GPCPcopyright;

  PROCEDURE Init*();

  PROCEDURE Assemble*(IN file : ARRAY OF CHAR; 
		     IN optn : ARRAY OF CHAR; (* "/debug" or "" *)
			    main : BOOLEAN); 	  (* /exe or /dll   *)

  PROCEDURE DoAsm*(IN file : ARRAY OF CHAR; 
		   IN optn : ARRAY OF CHAR;	(* "/debug" or "" *)
		      main : BOOLEAN; 		(* /exe or /dll   *)
		      vbse : BOOLEAN; 		(* verbose or not *)
		  OUT rslt : INTEGER);		(* ilasm ret-code *)

END MsilAsm.
