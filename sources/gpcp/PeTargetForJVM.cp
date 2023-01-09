

(* ============================================================ *)
(*  Return the PE-file target emitter. .JVM version             *)
(*  Copyright (c) John Gough 2018.                              *)
(* ============================================================ *)
(* ============================================================ *)

MODULE PeTarget;

  IMPORT 
        RTS,
        GPCPcopyright,
        MsilUtil;

(* ============================================================ *)
(*                       Factory Method                         *)
(* ============================================================ *)

  PROCEDURE newPeFile*(
     IN nam : ARRAY OF CHAR; isDll : BOOLEAN) : MsilUtil.MsilFile;
  BEGIN
    IF RTS.defaultTarget = "net" THEN
      THROW("Wrong version of PeTarget compiled");
    ELSE
      THROW("Reflection Emit emitter not available on JVM"); 
    END;
  END newPeFile;

(* ============================================================ *)
(* ============================================================ *)
END PeTarget.
(* ============================================================ *)
(* ============================================================ *)
