
(*
 *  Define various types and values
 *  for use by the other Asm* modules.
 *  This must not depend of any other
 *  gpcp module.
 *)

MODULE AsmDefinitions;
  IMPORT 
    GPCPcopyright,
    RTS,
    JL := java_lang,

    ASM := org_objectweb_asm;

  CONST versionDefault* = "7";

(* ============================================================ *)

  TYPE JlsArr* = POINTER TO ARRAY OF RTS.NativeString;
       JloArr* = POINTER TO ARRAY OF RTS.NativeObject;

(* ============================================================ *)

  PROCEDURE GetClassVersion*( IN s : ARRAY OF CHAR) : INTEGER;
  BEGIN
    IF s = "" THEN RETURN GetClassVersion( versionDefault );
    ELSIF s = "5" THEN RETURN ASM.Opcodes.V1_5;
    ELSIF s = "6" THEN RETURN ASM.Opcodes.V1_6;
    ELSIF s = "7" THEN RETURN ASM.Opcodes.V1_7;
    ELSIF s = "8" THEN RETURN ASM.Opcodes.V1_8;
    ELSE THROW( "Bad class version " + s );
    END;
  END GetClassVersion;

END AsmDefinitions.

