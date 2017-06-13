(* ============================================================ *)
(*  Target is the module which selects the target ClassMaker.	*)
(*  Copyright (c) John Gough 1999, 2017.			*)
(* ============================================================ *)

MODULE JavaTarget; (* JavaTargetForJVM.cp *)

  IMPORT 
        RTS,
	GPCPcopyright,
	CompState,
        AsmUtil,
        JavaUtil,
        ClassUtil;

(* ============================================================ *)

  PROCEDURE NewJavaEmitter*(IN fileName : ARRAY OF CHAR) : JavaUtil.JavaFile;
  BEGIN
    IF CompState.doAsm5 THEN 
      RETURN AsmUtil.newAsmEmitter(fileName);
    ELSIF CompState.doDWC THEN 
      RETURN ClassUtil.newClassFile(fileName);
    ELSE 
      THROW( "no jvm emitter chosen" );
    END; 
  END NewJavaEmitter;

(* ============================================================ *)
BEGIN
  IF RTS.defaultTarget # "jvm" THEN
    CompState.Abort("Wrong JavaTarget implementation: Use JavaTargetForCLR.cp");
  END;
END JavaTarget.
(* ============================================================ *)
