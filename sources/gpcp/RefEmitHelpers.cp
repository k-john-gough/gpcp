

(* ============================================================ *)
(*  RefEmitHelpers is the module which helps write PE files     *)
(*  using the System.Reflection.Emit library.                   *)
(*  Copyright (c) John Gough 2018.                              *)
(* ============================================================ *)
(* ============================================================ *)

MODULE RefEmitHelpers;

  IMPORT 
        GPCPcopyright,
        Mu  := MsilUtil,
        Id  := IdDesc,
        Lv  := LitValue,
        Sy  := Symbols,
        Ty  := TypeDesc,
        Cs  := CompState,
		Fn  := FileNames,

        Sys    := "[mscorlib]System",
        SysRfl := "[mscorlib]System.Reflection",
        RflEmt := "[mscorlib]System.Reflection.Emit";

(* ============================================================ *)

  PROCEDURE binDir*() : Sys.String;
  BEGIN
    IF Cs.binDir = "" THEN RETURN NIL ELSE RETURN MKSTR(Cs.binDir$) END;
  END binDir;

(* ============================================================ *)

  PROCEDURE CPtypeToCLRtype*(cpTp : Sy.Type) : Sys.Type;
  BEGIN
    RETURN NIL;
  END CPtypeToCLRtype;

(* ============================================================ *)
(* ============================================================ *)
END RefEmitHelpers.
(* ============================================================ *)
(* ============================================================ *)
