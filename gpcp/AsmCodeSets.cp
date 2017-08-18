(* 
 *  Code sets for ASM, to check for membership of
 *  particular forbidden or permitted bytecode values.
 *)
MODULE AsmCodeSets;
  IMPORT Jvm := JVMcodes;

  TYPE ByteBitSet = ARRAY 8 OF SET;

  VAR forbidden    : ByteBitSet; (* bytecodes not allowed by ASM5 *)
  VAR deltaSpecial : ByteBitSet; (* bytecodes with variable delta *)


(* ============================================================ *)
(*               Handling of byte code bit maps                 *)
(* ============================================================ *)

  PROCEDURE Insert( VAR set : ByteBitSet; val : INTEGER );
  BEGIN
    IF (val < 0) OR (val > 255) THEN
        THROW( "Illegal insert in bit set - " );
    END;
    INCL(set[val DIV 32], val MOD 32);
  END Insert;

(* -------------------- *)

  PROCEDURE badCode*( code : INTEGER ) : BOOLEAN;
  BEGIN
    RETURN (code MOD 32) IN forbidden[code DIV 32];
  END badCode;

(* -------------------- *)

  PROCEDURE badFix*( code : INTEGER ) : BOOLEAN;
  BEGIN
    RETURN (code MOD 32) IN deltaSpecial[code DIV 32];
  END badFix;

(* -------------------- *)

  PROCEDURE isOk*( code : INTEGER ) : INTEGER;
  BEGIN
    IF (code MOD 32) IN forbidden[code DIV 32] THEN
      THROW( "Illegal code called - " + Jvm.op[code] );
     (* Permissive variant *)
      (* Hlp.Msg( "Illegal code called - " + Jvm.op[code] ); *)
    END;
    RETURN code;
  END isOk;

(* ============================================================ *)
(*               Initialization of module globals               *)
(* ============================================================ *)
BEGIN
 (* -------------------------------------------- *)
 (*       Initialize forbidden code set          *)
 (* -------------------------------------------- *)
  Insert( deltaSpecial, Jvm.opc_getstatic );
  Insert( deltaSpecial, Jvm.opc_putstatic );
  Insert( deltaSpecial, Jvm.opc_getfield );
  Insert( deltaSpecial, Jvm.opc_putfield );
  Insert( deltaSpecial, Jvm.opc_invokevirtual );
  Insert( deltaSpecial, Jvm.opc_invokespecial );
  Insert( deltaSpecial, Jvm.opc_invokestatic);
  Insert( deltaSpecial, Jvm.opc_invokeinterface );
  Insert( deltaSpecial, Jvm.opc_multianewarray );

 (* -------------------------------------------- *)
 (*       Initialize forbidden code set          *)
 (* -------------------------------------------- *)
  Insert( forbidden, Jvm.opc_iload_0 );
  Insert( forbidden, Jvm.opc_iload_1 );
  Insert( forbidden, Jvm.opc_iload_2 );
  Insert( forbidden, Jvm.opc_iload_3 );
  Insert( forbidden, Jvm.opc_aload_0 );
  Insert( forbidden, Jvm.opc_aload_1 );
  Insert( forbidden, Jvm.opc_aload_2 );
  Insert( forbidden, Jvm.opc_aload_3 );
  Insert( forbidden, Jvm.opc_lload_0 );
  Insert( forbidden, Jvm.opc_lload_1 );
  Insert( forbidden, Jvm.opc_lload_2 );
  Insert( forbidden, Jvm.opc_lload_3 );
  Insert( forbidden, Jvm.opc_fload_0 );
  Insert( forbidden, Jvm.opc_fload_1 );
  Insert( forbidden, Jvm.opc_fload_2 );
  Insert( forbidden, Jvm.opc_fload_3 );
  Insert( forbidden, Jvm.opc_dload_0 );
  Insert( forbidden, Jvm.opc_dload_1 );
  Insert( forbidden, Jvm.opc_dload_2 );
  Insert( forbidden, Jvm.opc_dload_3 );

  Insert( forbidden, Jvm.opc_istore_0 );
  Insert( forbidden, Jvm.opc_istore_1 );
  Insert( forbidden, Jvm.opc_istore_2 );
  Insert( forbidden, Jvm.opc_istore_3 );
  Insert( forbidden, Jvm.opc_astore_0 );
  Insert( forbidden, Jvm.opc_astore_1 );
  Insert( forbidden, Jvm.opc_astore_2 );
  Insert( forbidden, Jvm.opc_astore_3 );
  Insert( forbidden, Jvm.opc_lstore_0 );
  Insert( forbidden, Jvm.opc_lstore_1 );
  Insert( forbidden, Jvm.opc_lstore_2 );
  Insert( forbidden, Jvm.opc_lstore_3 );
  Insert( forbidden, Jvm.opc_fstore_0 );
  Insert( forbidden, Jvm.opc_fstore_1 );
  Insert( forbidden, Jvm.opc_fstore_2 );
  Insert( forbidden, Jvm.opc_fstore_3 );
  Insert( forbidden, Jvm.opc_dstore_0 );
  Insert( forbidden, Jvm.opc_dstore_1 );
  Insert( forbidden, Jvm.opc_dstore_2 );
  Insert( forbidden, Jvm.opc_dstore_3 );

 (* ------------------------------------ *)
END AsmCodeSets.
(* ============================================================ *)


