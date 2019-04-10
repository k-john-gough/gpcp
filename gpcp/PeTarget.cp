

(* ============================================================ *)
(*  Return the PE-file target emitter. .NET version             *)
(*  Copyright (c) John Gough 2018.                              *)
(* ============================================================ *)
(* ============================================================ *)

MODULE PeTarget;

  IMPORT 
        RTS,
        GPCPcopyright,
        MsilUtil,
        RefEmitUtil;

(* ============================================================ *)
(*                       Factory Method                         *)
(* ============================================================ *)

  PROCEDURE newPeFile*(
     IN nam : ARRAY OF CHAR; isDll : BOOLEAN) : MsilUtil.MsilFile;
  BEGIN
    IF RTS.defaultTarget # "net" THEN
      THROW("Wrong version of PeTarget compiled");
    END;
    RETURN RefEmitUtil.newPeFile(nam, isDll);
  END newPeFile;

(* ============================================================ *)
(* ============================================================ *)
END PeTarget.
(* ============================================================ *)
(* ============================================================ *)
