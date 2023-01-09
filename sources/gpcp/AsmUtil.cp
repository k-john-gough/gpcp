
(* ============================================================ *)
(*  AsmUtil is the module which writes java classs file         *)
(*  structures using the ASM5.* libraries                       *)
(*  Copyright (c) John Gough 2016.                              *)
(* ============================================================ *)

MODULE AsmUtil;

  IMPORT 
        GPCPcopyright,
        RTS,
        ASCII,
        Console,
        JavaBase,
        FileNames,
        GPF := GPFiles,
        GPB := GPBinFiles,
        Hsh := NameHash,
        CSt := CompState,
        Psr := CPascalP,
        Jvm := JVMcodes,
        Sym := Symbols,
        Blt := Builtin,
        Id  := IdDesc,
        Xp  := ExprDesc,
        Ty  := TypeDesc,
        Lv  := LitValue,
        Ju  := JavaUtil,
        Cu  := ClassUtil,

        JL  := java_lang,
        Frm := AsmFrames,
        Hlp := AsmHelpers,
        Acs := AsmCodeSets,
        Def := AsmDefinitions,
        ASM := org_objectweb_asm;

(* ============================================================ *)
(* ============================================================ *)

  CONST versionDefault = "V1_7";

(* ============================================================ *)

  TYPE AsmLabel*    = POINTER TO RECORD (Ju.Label)
                     (*  defIx : INTEGER -- inherited field *)
                     (*  attr*  : SET;                      *)
                         serNm : Lv.CharOpen;
                         asmLb : ASM.Label;
                         evalSave : Frm.FrameSave;
                     END;

(* ============================================================ *)
(*                Main Emitter Class Definition                 *)
(* ============================================================ *)

  TYPE AsmEmitter* = POINTER TO RECORD (Ju.JavaFile)
                      (*
                       *  The classfile binary file
                       *) 
                       file : GPB.FILE;     
                      (*
                       *  The class writer for this emitter
                       *) 
                       clsWrtr : ASM.ClassWriter;
                       labelIx : INTEGER;
                      (*
                       *  Source filename of the module file.
                       *) 
                       srcFileName : Lv.CharOpen;
                      (*
                       *  Binary name of the static class
                       *  that contains the module static code.
                       *) 
                       xName : Lv.CharOpen;
                       nmStr : RTS.NativeString;
                      (*
                       *  The method visitor for the current method
                       *) 
                       thisMth : Id.Procs;
                       procNam : Lv.CharOpen;
                       thisMv  : ASM.MethodVisitor;
                      (*
                       *  Resources for tracking local variable use
                       *) 
                       entryLab   : AsmLabel;
                       exitLab    : AsmLabel;
                       rescueLab  : AsmLabel;
                       caseArray  : POINTER TO ARRAY OF ASM.Label;
                       caseEval   : Frm.FrameSave;
                       mthFrame   : Frm.MethodFrame;
                       emitStackFrames : BOOLEAN;
                       stackFramePending : BOOLEAN;
                     END;

(* ============================================================ *)
(*                Various Fixed Native Strings                  *)
(* ============================================================ *)

  VAR jloStr,     (* "java/lang/Object"    *)
      jlsStr,     (* "java/lang/String"    *)
      initStr,    (* "<init>"              *)
      clinitStr,  (* "<clinit>"            *)
      copyStr,    (* "__copy__"            *)
      jleStr,     (* "java/lang/Exception" *)
      jlmStr,     (* "java/lang/Math"      *)
      jlcStr,     (* "java/lang/Class"     *)
      jlchStr,    (* "java/lang/Character" *)
      jlsyStr,    (* "java/lang/System"    *)
      rtsStr,     (* "CP/CPJrts/CPJrts"    *)
      cpMain,     (* "CP/CPmain/CPmain"    *)
     (* ------------------------------------ *)
      withTrap,   (* "WithMesg"              *)
      withTrapSig,(* "(Ljlo;)Ljls"           *)
      caseTrap,   (* "CaseMesg"              *) 
      caseTrapSig,(* "(I)Ljava/lang/String;" *)
     (* ------------------------------------ *)
      mainSig,    (* "([Ljava/lang/String;)V *)
      strToVoid,  (* "(Ljava/lang/String;)V  *)
      noArgVoid : (* "()V"                   *) RTS.NativeString;
     (* ------------------------------------ *)

  VAR pubAcc,     (* ACC_PUBLIC            *)
      prvAcc,     (* ACC_PRIVATE;          *)
      finAcc,     (* ACC_FINAL             *)
      supAcc,     (* ACC_SUPER             *)
      staAcc,     (* ACC_STATIC            *)
      absAcc,     (* ACC_ABSTRACT          *)
      protec,     (* ACC_PACKAGE           *)
      pckAcc,     (* ACC_PACKAGE           *)

      pubSta,     (* pub + sta             *)
      modAcc :    (* pub + fin + sup       *) INTEGER;

      (* Kludge to make NIL typecheck with array formal *)
      jlsEmptyArr : Def.JlsArr;

  VAR procNames  : ARRAY 24 OF Lv.CharOpen;
      procSigs   : ARRAY 24 OF Lv.CharOpen;
      procRetS   : ARRAY 24 OF Lv.CharOpen;
      getClass   : Lv.CharOpen;
      IIretI     : Lv.CharOpen;
      JJretJ     : Lv.CharOpen;

  VAR typeArr    : ARRAY 16 OF INTEGER;
      typeArrArr : ARRAY 16 OF Ty.Array;

  VAR tagCount : INTEGER;

(* ============================================================ *)
(*                  Static Variables of Module                  *)
(* ============================================================ *)

  VAR fileSep : Lv.CharOpen;
      wrtrFlag : INTEGER;
      classVersion : INTEGER;

(* ============================================================ *)
(*    Forward Declarations of Static Procedures and Methods     *)
(* ============================================================ *)

  PROCEDURE^ MkNewAsmLabel( ) : AsmLabel;
  PROCEDURE^ InterfaceNameList( rec : Ty.Record ) : Def.JlsArr;
  PROCEDURE^ clsToVoidDesc(rec : Ty.Record) : Lv.CharOpen;

  PROCEDURE^ (lab : AsmLabel)FixTag(),NEW;

(* ============================================================ *)
(* ============================================================ *)
(*                   AsmEmitter Constructor Method              *)
(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE SetEmitVer();
  BEGIN
    IF CSt.asmVer = "" THEN 
      CSt.emitNam := BOX("AsmUtil - default (V1_" + Def.versionDefault + ")");
    ELSE
      CSt.emitNam := BOX("AsmUtil - V1_" + CSt.asmVer);
    END;
  END SetEmitVer;

  PROCEDURE newAsmEmitter*(fileName : ARRAY OF CHAR) : AsmEmitter;
    VAR result : AsmEmitter;
        pathName : Lv.CharOpen;
   (* ------------------------------------------------- *)
    PROCEDURE Warp(VAR s : ARRAY OF CHAR);
      VAR i : INTEGER;
    BEGIN
      FOR i := 0 TO LEN(s)-1 DO
        IF s[i] = "/" THEN s[i] := GPF.fileSep END;
      END;
    END Warp;
   (* ------------------------------------------------- *)
    PROCEDURE GetFullPath(IN fn : ARRAY OF CHAR) : Lv.CharOpen;
      VAR ps : Lv.CharOpen;
          ch : CHAR;
    BEGIN
      ps := BOX(CSt.binDir$);
      ch := ps[LEN(ps) - 2]; (* last character *)
      IF (ch # "/") & (ch # "\") THEN
        ps := BOX(ps^ + fileSep^ + fn);
      ELSE
        ps := BOX(ps^ + fn);
      END;
      RETURN ps;
    END GetFullPath;
   (* ------------------------------------------------- *)
  BEGIN
   (*
    *  Setting some module globals
    * --------------- *)
    wrtrFlag   := 0;
    SetEmitVer();
    IF CSt.doVersion THEN
      CSt.Message("Using " + CSt.emitNam^ + " emitter" );
    END;
    classVersion := Def.GetClassVersion(CSt.asmVer);
   (*
    *  
    *)
    IF CSt.binDir # "" THEN
      pathName := GetFullPath(fileName);
    ELSE
      pathName := BOX(fileName$);
    END;
    Warp(pathName);
    NEW(result);
    result.file := GPB.createPath(pathName);
    IF result.file = NIL THEN RETURN NIL 
    ELSE
      result.srcFileName := BOX(CSt.srcNam$); 
      result.emitStackFrames := 
            (classVersion # ASM.Opcodes.V1_1) &
            (classVersion > ASM.Opcodes.V1_5);
    END;
    RETURN result;
  END newAsmEmitter;


(* ============================================================ *)
(*         Type-bound methods of AsmEmitter record type         *)
(* ============================================================ *)

  PROCEDURE (emtr : AsmEmitter)EmitStackFrame(),NEW;
    VAR loclArr : POINTER TO ARRAY OF RTS.NativeObject;
        evalArr : POINTER TO ARRAY OF RTS.NativeObject;
        loclNum : INTEGER;
        evalNum : INTEGER;
  BEGIN
    evalNum := emtr.mthFrame.EvCount();
    loclNum := emtr.mthFrame.LcCount();
    evalArr := emtr.mthFrame.GetEvalArr();
    loclArr := emtr.mthFrame.GetLocalArr();
    emtr.thisMv.visitFrame( ASM.Opcodes.F_NEW,
        loclNum, loclArr, evalNum, evalArr );
    emtr.stackFramePending := FALSE;
  END EmitStackFrame;

  PROCEDURE (emtr : AsmEmitter)CheckFrame(),NEW;
  BEGIN
    IF emtr.stackFramePending THEN emtr.EmitStackFrame() END;
  END CheckFrame;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)SetProcData( ),NEW;
  BEGIN
   (*
    * Used for local variable table 
    *)
    IF CSt.debug THEN
      emtr.rescueLab := NIL;
      emtr.entryLab := NIL;
      emtr.exitLab := NIL;
     (*
      * This field is used to suppress emission
      * of multiple stack frames for the same offset.  
      *)
      emtr.stackFramePending := FALSE;
    END;
  END SetProcData;

 (* --------------------------------------------------------- *)
 (* * arrName is the binary name of the array type, e.g. [[D  *)
 (* * dms is the number of dimensions                         *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)MultiNew*(arrName : Lv.CharOpen;
                                             dms : INTEGER),NEW;
  BEGIN
    emtr.CheckFrame();
    emtr.thisMv.visitMultiANewArrayInsn( MKSTR(arrName^), dms);
    emtr.mthFrame.DeltaEvalDepth( 1 - dms );
    emtr.mthFrame.SetTosState( arrName );
  END MultiNew;

 (* --------------------------------------------------------- *)
 (* ------ Push local with index "ord" of a known type ------ *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)LoadLocal*(ord : INTEGER; typ : Sym.Type);
    VAR code : INTEGER;
  BEGIN
   (*
    *  ASM does not correctly compute stack height
    *  if the optimized loads ( xLOAD_N for N 1..4)
    *  are used.  Hence this -
    *)
    IF (typ # NIL) & (typ IS Ty.Base) THEN 
      code := Ju.typeLoad[typ(Ty.Base).tpOrd];
    ELSE
      code := ASM.Opcodes.ALOAD; (* all reference types *)
    END;
    emtr.CheckFrame();
    emtr.thisMv.visitVarInsn( code, ord );
    emtr.mthFrame.FixEvalStack( code, typ );
  END LoadLocal;


 (* --------------------------------------------------------- *)
 (* --------------------------------------------------------- *)
 (* -- Store TOS to local with index "ord" of a known type -- *)
 (* --------------------------------------------------------- *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)StoreLocal*(ord : INTEGER; type : Sym.Type);
    VAR code : INTEGER;
  BEGIN
   (*
    *  ASM does not correctly compute stack height
    *  if the optimized loads ( xLOAD_N for N 1..4)
    *  are used.  Hence this -
    *)
    IF (type # NIL) & (type IS Ty.Base) THEN 
      code := Ju.typeStore[type(Ty.Base).tpOrd];
    ELSE
      code := ASM.Opcodes.ASTORE;
    END;
    emtr.CheckFrame();
    emtr.thisMv.visitVarInsn( code, ord );
    emtr.mthFrame.TrackStore( ord );
    emtr.mthFrame.FixEvalStack( code, NIL ); (* No TOS type change *)
  END StoreLocal;

 (* --------------------------------------------------------- *)
 (*   Push local of reference type to local with index "ord"  *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)AloadLocal*( ord : INTEGER; 
                                            typ : Sym.Type );
  BEGIN
    emtr.CheckFrame();
    emtr.thisMv.visitVarInsn( ASM.Opcodes.ALOAD, ord );
    emtr.mthFrame.DeltaEvalDepth( 1 );
    emtr.mthFrame.SetTosType( typ );
  END AloadLocal;

 (* --------------------------------------------------------- *)
 (* --  Allocate new class emitter for static module class -- *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)StartModClass*(mod : Id.BlkId);
  BEGIN
    emtr.xName := mod.xName;
    emtr.nmStr := MKSTR(emtr.xName^);
    emtr.clsWrtr := ASM.ClassWriter.Init( wrtrFlag );
    emtr.clsWrtr.visit( 
        classVersion, modAcc, emtr.nmStr, NIL, jloStr, jlsEmptyArr );
    emtr.clsWrtr.visitSource( MKSTR(emtr.srcFileName^), NIL );
  END StartModClass;


 (* --------------------------------------------------------- *)
 (* --------------------------------------------------------- *)
 (* --  Allocate new class emitter for record module class -- *)
 (* --------------------------------------------------------- *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)StartRecClass*(rec : Ty.Record);
    VAR recAtt : INTEGER;
        index  : INTEGER;
        clsId  : Sym.Idnt;
        impRec : Sym.Type;
        baseNm : RTS.NativeString;
  BEGIN
    Hlp.EnsureRecName( rec );
    emtr.xName := rec.xName;
    IF rec.baseTp IS Ty.Record THEN 
      Hlp.EnsureRecName( rec.baseTp(Ty.Record) );
      baseNm := MKSTR( rec.baseTp.xName^ );
    ELSE
      baseNm := jloStr;
    END;
    IF rec.recAtt = Ty.noAtt THEN
      recAtt := supAcc + finAcc;
    ELSIF rec.recAtt = Ty.isAbs THEN
      recAtt := supAcc + absAcc;
    ELSE
      recAtt := supAcc;
    END;
    IF rec.bindTp = NIL THEN
      clsId := rec.idnt;
    ELSE
      clsId := rec.bindTp.idnt;
    END;
    IF clsId # NIL THEN
      IF clsId.vMod = Sym.pubMode THEN INC( recAtt, pubAcc ) END;
    END;
   (*
    *  Get names of interface, if any, into list
    *)
    emtr.clsWrtr := ASM.ClassWriter.Init( wrtrFlag );
    emtr.clsWrtr.visit( 
        classVersion, recAtt, MKSTR(rec.xName^), NIL, 
        baseNm, InterfaceNameList(rec) );
    emtr.clsWrtr.visitSource( MKSTR(emtr.srcFileName^), NIL );
  END StartRecClass;

 (* --------------------------------------------------------- *)
 (* ------   Dump the procedure local variable table   ------ *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)DumpLocalTable(),NEW;
    VAR local : Id.LocId;
        start : AsmLabel;
        vName : RTS.NativeString;
        tName : RTS.NativeString;
        proc  : Id.Procs;
        idx   : INTEGER;
        mv    : ASM.MethodVisitor;
  BEGIN
    mv := emtr.thisMv;
    proc := emtr.thisMth;
    IF proc # NIL THEN
      FOR idx := 0 TO proc.locals.tide - 1 DO
        local := proc.locals.a[idx](Id.LocId);
        vName := local.namStr;
        tName := MKSTR( Hlp.GetBinaryTypeName( local.type )^ );
        IF vName = NIL THEN 
          IF local(Id.ParId).isRcv THEN
            vName := MKSTR( "this" );
          ELSE
            vName := MKSTR( "__anon" + Ju.i2CO( idx )^ + "__" );
          END;
        END;
        IF local = proc.except THEN 
          start := emtr.rescueLab;
        ELSE
          start := emtr.entryLab;
        END;
        mv.visitLocalVariable( vName, tName, NIL,
            start.asmLb, emtr.exitLab.asmLb, local.varOrd );
      END;
    END;
  END DumpLocalTable;
  

 (* --------------------------------------------------------- *)
 (* ------   Allocate method visitor for any method    ------ *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)StartProc*(proc : Id.Procs);
    VAR attr : INTEGER;
        method : Id.MthId;
        mv : ASM.MethodVisitor;
  BEGIN
    emtr.thisMth := proc;
    emtr.procNam := proc.prcNm;
    IF proc.kind = Id.conMth THEN 
      attr := 0;
      method := proc(Id.MthId);
      IF method.mthAtt * Id.mask = {} THEN attr := finAcc END;
      IF method.mthAtt * Id.mask = Id.isAbs THEN INC(attr,absAcc) END;
      IF Id.widen IN method.mthAtt THEN INC(attr,pubAcc) END;
    ELSE
      attr := staAcc;
    END;
(*
 *  The following code fails for "implement-only" methods
 *  since the JVM places the "override method" in a different 
 *  slot! We must thus live with the insecurity of public mode.
 *
 *  IF proc.vMod = Sym.pubMode THEN     (* explicitly public *)
 *)
    IF (proc.vMod = Sym.pubMode) OR     (* explicitly public *)
       (proc.vMod = Sym.rdoMode) THEN   (* "implement only"  *)
      INC(attr,pubAcc);
    ELSIF proc.dfScp IS Id.PrcId THEN   (* nested procedure  *)
      INC(attr,prvAcc);
    END;

    mv := emtr.clsWrtr.visitMethod( attr, 
        MKSTR(proc.prcNm^), 
        MKSTR(proc.type.xName^), 
        NIL, jlsEmptyArr );
    emtr.thisMv := mv;
    emtr.procNam := proc.prcNm;

    emtr.mthFrame := Frm.NewMethodFrame( proc );
    emtr.SetProcData( );

    mv.visitCode();
    IF CSt.debug THEN
      emtr.entryLab := MkNewAsmLabel();
      mv.visitLabel( emtr.entryLab.asmLb );
    END;
  END StartProc;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)EndProc*(); (* NEW,EMPTY *)
    VAR mv : ASM.MethodVisitor;
        xL : AsmLabel;
  BEGIN
    mv := emtr.thisMv;
    (* ... *)

    (* ... *)
    IF CSt.debug THEN
      xL := MkNewAsmLabel();
      mv.visitLabel( xL.asmLb );
      xL.defIx := xL.asmLb.getOffset();
      emtr.exitLab := xL;
      emtr.DumpLocalTable();
    END;
    mv.visitMaxs( emtr.mthFrame.maxEval, emtr.mthFrame.maxLocal ); 
    mv.visitEnd(); 
    emtr.thisMv := NIL;
    emtr.thisMth := NIL;
  END EndProc;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)isAbstract*():BOOLEAN;
    VAR proc : Id.Procs;
  BEGIN
    proc := emtr.thisMth;
    IF proc = NIL THEN
      THROW( "Can only call isAbstract during a method visit" );
    END;
    WITH proc : Id.MthId DO
      RETURN proc.mthAtt * Id.mask = Id.isAbs;
    ELSE
      RETURN FALSE;
    END;
  END isAbstract;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)getScope*():Sym.Scope;
  BEGIN
    RETURN emtr.thisMth;
  END getScope;

 (* --------------------------------------------------------- *)
 (* ------         ------ *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter) EmitField*(field : Id.AbVar);
    VAR access : INTEGER;
        fv     : ASM.FieldVisitor;
  BEGIN
   (*
    *  emtr has already allocated a vector of fieldInfo
    *)
    CASE field.vMod OF
    | Sym.prvMode : access := pckAcc;
    | Sym.pubMode : access := pubAcc;
    | Sym.rdoMode : access := pubAcc;
    | Sym.protect : access := protec;
    END;
    IF field IS Id.VarId THEN INC( access, staAcc ) END;
    fv := emtr.clsWrtr.visitField( access, Hlp.idNam(field), Hlp.idSig(field), NIL, NIL );
    fv.visitEnd();
  END EmitField;

 (* --------------------------------------------------------- *)
 (* ------   Translation of NEW( var of type "typ" )   ------ *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)MkNewRecord*(typ : Ty.Record);
    VAR mv : ASM.MethodVisitor;
        cn : RTS.NativeString;
  BEGIN
    mv := emtr.thisMv;
    cn := Hlp.tyCls( typ );
    emtr.CheckFrame();
    mv.visitTypeInsn( ASM.Opcodes.`NEW, cn );
    emtr.mthFrame.DeltaEvalDepth( 1 );
    mv.visitInsn( ASM.Opcodes.DUP );
    emtr.mthFrame.DeltaEvalDepth( 1 );
    mv.visitMethodInsn(
        ASM.Opcodes.INVOKESPECIAL, cn, initStr, noArgVoid, FALSE );
    emtr.mthFrame.DeltaEvalDepth( -1 );
    emtr.mthFrame.SetTosType( typ );
  END MkNewRecord;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)MkNewFixedArray*(topE : Sym.Type;
                                                len0 : INTEGER);
    VAR dims : INTEGER;
        arTp : Ty.Array;
        elTp : Sym.Type;
  BEGIN
    (*
    //  Fixed-size, possibly multi-dimensional arrays.
    //  The code relies on the semantic property in CP
    //  that the element-type of a fixed array type cannot
    //  be an open array. This simplifies the code somewhat.
    *)
    emtr.PushInt(len0);
    dims := 1;
    elTp := topE;
   (*
    *  Find the number of dimensions ...
    *)
    LOOP
      WITH elTp : Ty.Array DO arTp := elTp ELSE EXIT END;
      elTp := arTp.elemTp;
      emtr.PushInt(arTp.length);
      INC(dims);
    END;
    IF dims = 1 THEN
      emtr.Alloc1d(elTp);
     (*
      *  Stack is (top) len0, ref...
      *)
      IF elTp.kind = Ty.recTp THEN emtr.Init1dArray(elTp, len0) END;
    ELSE
     (*
      *  Allocate the array headers for all dimensions.
      *  Stack is (top) lenN, ... len0, ref...
      *)
      emtr.MultiNew(Ju.cat2(Ju.brac, Hlp.GetBinaryTypeName(topE)), dims);
     (*
      *  Stack is (top) ref...
      *)
      IF elTp.kind = Ty.recTp THEN emtr.InitNdArray(topE, elTp) END;
    END;
  END MkNewFixedArray;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)MkNewOpenArray*(arrT : Ty.Array;
                                               dims : INTEGER);
    VAR elTp : Sym.Type;
        indx : INTEGER;
  BEGIN
   (* 
    *  Assert: lengths are pushed already...
    *  and we know from semantic analysis that
    *  the number of open array dimensions match
    *  the number of integer LENs in dims.
    *)
    elTp := arrT;
   (*
    *   Find the number of dimensions ...
    *)
    FOR indx := 0 TO dims-1 DO
      elTp := elTp(Ty.Array).elemTp;
    END;
   (*
    *   Allocate the array headers for all _open_ dimensions.
    *)
    IF dims = 1 THEN
      emtr.Alloc1d(elTp);
     (*
      *  Stack is now (top) ref ...
      *  and we _might_ need to initialize the elements.
      *)
      IF (elTp.kind = Ty.recTp) OR 
         (elTp.kind = Ty.arrTp) THEN 
        emtr.Init1dArray(elTp, 0);
      END;
    ELSE
      emtr.MultiNew( Hlp.GetBinaryTypeName(arrT), dims );
     (*
      *    Stack is now (top) ref ...
      *    Now we _might_ need to initialize the elements.
      *)
      IF (elTp.kind = Ty.recTp) OR 
         (elTp.kind = Ty.arrTp) THEN 
        emtr.InitNdArray(arrT.elemTp, elTp);
      END;
    END;
  END MkNewOpenArray;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)MkArrayCopy*(arrT : Ty.Array);
    VAR dims : INTEGER;
        elTp : Sym.Type;
  BEGIN
   (*
    *   Assert: we must find the lengths from the runtime 
    *   descriptors.  Find the number of dimensions.  The 
    *   array to copy is on the top of stack, which reads -
    *        (top) aRef, ...
    *)
    elTp := arrT.elemTp;
    IF elTp.kind # Ty.arrTp THEN
      emtr.Code(Jvm.opc_arraylength); (* (top) len0, aRef,...  *)
      emtr.Alloc1d(elTp);             (* (top) aRef, ...       *)
      IF elTp.kind = Ty.recTp THEN emtr.Init1dArray(elTp, 0) END; (*0 ==> open*)
    ELSE
      dims := 1;
      REPEAT
       (* 
        *  Invariant: an array reference is on the top of
        *  of the stack, which reads:
        *        (top) [arRf, lengths,] arRf ...
        *)
        INC(dims);
        elTp := elTp(Ty.Array).elemTp;
        emtr.Code(Jvm.opc_dup);         (*           arRf, arRf,... *)
        emtr.Code(Jvm.opc_arraylength); (*     len0, arRf, arRf,... *)
        emtr.Code(Jvm.opc_swap);        (*     arRf, len0, arRf,... *)
        emtr.Code(Jvm.opc_iconst_0);    (*  0, arRf, len0, arRf,... *)
        emtr.Code(Jvm.opc_aaload);      (*     arRf, len0, arRf,... *)
       (* 
        *  Stack reads:        (top) arRf, lenN, [lengths,] arRf ...
        *)
      UNTIL  elTp.kind # Ty.arrTp;
     (*
      *  Now get the final length...
      *)
      emtr.Code(Jvm.opc_arraylength);  
     (* 
      *   Stack reads:        (top) lenM, lenN, [lengths,] arRf ...
      *   Allocate the array headers for all dimensions.
      *)
      emtr.MultiNew( Hlp.GetBinaryTypeName(arrT), dims );
     (*
      *  Stack is (top) ref...
      *)
      IF elTp.kind = Ty.recTp THEN emtr.InitNdArray(arrT.elemTp, elTp) END;
    END;
  END MkArrayCopy;


(* ============================================================ *)
(* ============================================================ *)
(*                 Temporary Local Management                   *)
(* ============================================================ *)
(* ============================================================ *)

 (* --------------------------------------------------------- *)
 (*      newLocal allocates a temp and bumps LocalNum         *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)newLocal*( t : Sym.Type ) : INTEGER;
    VAR ord, new : INTEGER;
  BEGIN
    ord := emtr.mthFrame.LcHi();
    emtr.mthFrame.AddLocal( t );
    new := emtr.mthFrame.LcHi();
    RETURN new;
  END newLocal;

 (* --------------------------------------------------------- *)
 (*   newLongLocal allocates a 2-slot temp, bumps LocalNum    *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)newLongLocal*( t : Sym.Type ) : INTEGER;
    VAR ord,new : INTEGER;
  BEGIN
    ord := emtr.mthFrame.LcHi();
    emtr.mthFrame.AddLocal( t );
    new := emtr.mthFrame.LcHi();
    ASSERT( new = ord + 2 );
    RETURN new;
  END newLongLocal;

 (* --------------------------------------------------------- *)
 (*      ReleaseLocal discards the temporary at index "i"     *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)PopLocal*();
  BEGIN
    emtr.mthFrame.PopLocal1();
  END PopLocal;

 (* --------------------------------------------------------- *)
 (*  ReleaseLongLocal discards the temporary at index i,i+1   *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)PopLongLocal*();
  BEGIN
    emtr.mthFrame.PopLocal2();
  END PopLongLocal;

 (* --------------------------------------------------------- *)
 (*  Function markTop saves the local variable depth at the   *)
 (*  point where a call is about to be made. This call may    *)
 (*  allocate new temporaries during argument evaluation. All *)
 (*  of these will be dead at the return, and are discarded.  *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)markTop*() : INTEGER;
    VAR m : INTEGER;
  BEGIN
    m := emtr.mthFrame.LcLn();
    RETURN m;
  END markTop;

 (* --------------------------------------------------------- *)
 (*     ReleaseAll discards the temps, restoring localNum     *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)ReleaseAll*(m : INTEGER);
  BEGIN
    emtr.mthFrame.ReleaseTo( m );
  END ReleaseAll;

(* ============================================================ *)
(* ============================================================ *)
(*                   Shadow Stack Management                    *)
(* ============================================================ *)
(* ============================================================ *)

 (* --------------------------------------------------------- *)
 (*                Get Evaluation Stack Depth                 *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)getDepth*() : INTEGER;
  BEGIN
    RETURN LEN(emtr.mthFrame.evalStack);
  END getDepth;

 (* --------------------------------------------------------- *)
 (*                Set Evaluation Stack Depth                 *)
 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)setDepth*(i : INTEGER);
  BEGIN
    emtr.mthFrame.setDepth( i );
  END setDepth;

(* ============================================================ *)
(* ============================================================ *)
(*                      Label Management                        *)
(* ============================================================ *)
(* ============================================================ *)

 (* --------------------------------------------------------- *)
 (*             Allocate a single JavaUtil.Label              *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)newLabel*() : Ju.Label;
  BEGIN
    RETURN MkNewAsmLabel();
  END newLabel;

  PROCEDURE (emtr : AsmEmitter)newEmptystackLabel*() : Ju.Label;
    VAR label : AsmLabel; 
  BEGIN
    label := MkNewAsmLabel();
    INCL( label.attr, Ju.forceEmpty );
    RETURN label;
  END newEmptystackLabel;

  PROCEDURE (emtr : AsmEmitter)newLoopheaderLabel*() : Ju.Label;
    VAR label : AsmLabel; 
  BEGIN
    label := MkNewAsmLabel();
    INCL( label.attr, Ju.forceEmit );
    RETURN label;
  END newLoopheaderLabel;

 (* --------------------------------------------------------- *)
 (*            Allocate an array of JavaUtil.Labels           *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)getLabelRange*(VAR labs:ARRAY OF Ju.Label);
    VAR idx : INTEGER;
  BEGIN
    FOR idx := 0 TO LEN(labs) - 1 DO
      labs[idx] := MkNewAsmLabel();
    END;
  END getLabelRange;

 (* --------------------------------------------------------- *)
 (*                Assign a unique tag to label               *)
 (* --------------------------------------------------------- *)
  PROCEDURE (lab : AsmLabel)FixTag(),NEW;
  BEGIN
    INC(tagCount);
    lab.serNm := BOX( "label_" + Ju.i2CO(tagCount)^ );
  END FixTag;

 (* --------------------------------------------------------- *)
 (*            Check if label has seen a jump edge            *)
 (* --------------------------------------------------------- *)
 (* // NOW uses the inherited proc Ju.JumpSeen() 
  PROCEDURE (lab : AsmLabel)JumpSeen*() : BOOLEAN;
  BEGIN
    RETURN jumpSeen IN lab.attr;
  END JumpSeen;
  *)

 (* --------------------------------------------------------- *)
 (*   Check if label not fixed AND not previously jumped to   *)
 (* --------------------------------------------------------- *)
  PROCEDURE (lab : AsmLabel)FwdTarget() : BOOLEAN,NEW;
  BEGIN
    RETURN Ju.unfixed IN lab.attr;
  END FwdTarget;

 (* --------------------------------------------------------- *)
  PROCEDURE ( lab : AsmLabel )Str*() : Lv.CharOpen;
    VAR attr : ARRAY 12 OF CHAR;
        posn : ARRAY 12 OF CHAR;
  BEGIN
    attr := "{.,..,..,.}";
    IF (Ju.unfixed IN lab.attr)  THEN attr[1] := "?" END;
    IF (Ju.posFixed IN lab.attr) THEN attr[1] := "P" END;
    IF (Ju.forceEmpty IN lab.attr) THEN 
                      attr[3] := "m"; attr[4] := "t" END;
    IF (Ju.assertEmpty IN lab.attr) THEN 
                      attr[3] := "M"; attr[4] := "T" END;
    IF (Ju.jumpSeen IN lab.attr)  THEN   
                      attr[6] := "j"; attr[7] := "s" END;
    IF (Ju.forceEmit IN lab.attr) THEN   attr[9] := "!" END;
    IF lab.defIx > 0 THEN posn := ", @" + Ju.i2CO( lab.defIx )^;
                     ELSE posn := "" END; 
    RETURN BOX( lab.serNm^ + ": // " + attr + posn );
  END Str;

 (* --------------------------------------------------------- *)
 (*      Emit the no-arg instruction, after legality check    *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)Code*(code : INTEGER);
  BEGIN
    ASSERT( ~Acs.badCode( code ) );
    emtr.CheckFrame();
    emtr.thisMv.visitInsn( code );
    emtr.mthFrame.MutateEvalStack( code ); (* Compute TOS change *)
  END Code;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)CodeI*(code,val : INTEGER);
  BEGIN
   (* 
    * ASM5 allows bipush, sipush and newarray on basic types 
    *      gpcp only allows bipush and sipush.
    *) 
    ASSERT((code = Jvm.opc_bipush) OR
           (code = Jvm.opc_sipush));
    emtr.CheckFrame();
    emtr.thisMv.visitIntInsn( code, val );
    emtr.mthFrame.MutateEvalStack( code ); 
  END CodeI;

 (* --------------------------------------------------------- *)
 (*      Emit LDC for a long-int lit. Arg code is unused      *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)CodeL*(code : INTEGER; num : LONGINT);
  BEGIN
    (* code is ignored *)
    emtr.CheckFrame();
    emtr.thisMv.visitLdcInsn( Hlp.MkLong( num ) );
    emtr.mthFrame.FixEvalStack( Jvm.opc_ldc2_w, Blt.lIntTp );
  END CodeL;

 (* --------------------------------------------------------- *)
 (*     This is just a version of Code( c ) with a comment    *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)CodeC*(code : INTEGER; 
                                   IN str  : ARRAY OF CHAR);
  BEGIN
    ASSERT( ~Acs.badCode( code ) );
    emtr.CheckFrame();
    emtr.thisMv.visitInsn( code );
    emtr.mthFrame.MutateEvalStack( code ); (* Compute TOS change *)
  END CodeC;

 (* --------------------------------------------------------- *)
 (*   Emit LDC for a floating point lit. Arg code is unused   *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)CodeR*(code : INTEGER; 
                                      num  : REAL; short : BOOLEAN);
  BEGIN
    (* code is ignored *)
    emtr.CheckFrame();
    IF short THEN
      emtr.thisMv.visitLdcInsn( Hlp.MkFloat( SHORT(num) ) );
      emtr.mthFrame.FixEvalStack( Jvm.opc_ldc, Blt.sReaTp );
    ELSE
      emtr.thisMv.visitLdcInsn( Hlp.MkDouble( num ) );
      emtr.mthFrame.FixEvalStack( Jvm.opc_ldc2_w, Blt.realTp );
    END;
  END CodeR;

 (* --------------------------------------------------------- *)
 (*        Emit a jump instruction to the given label         *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)CodeLb*(code : INTEGER; lab : Ju.Label);
    VAR label : AsmLabel;
  BEGIN
    label := lab(AsmLabel);
    emtr.CheckFrame();
    emtr.thisMv.visitJumpInsn( code, label.asmLb );
   (*
    *  The eval stack effect of the jump must be applied
    *  before the stack state is copied to label.evalSave
    *)
    emtr.mthFrame.FixEvalStack( code, NIL );
    IF label.FwdTarget() THEN (* ==> this is a forward jump *)
      label.evalSave := emtr.mthFrame.GetFrameSave( label.evalSave );
      INCL( label.attr, Ju.jumpSeen );
    END;
  END CodeLb;

 (* --------------------------------------------------------- *)
 (*        Define a Label location and update lab.defIx       *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)DefLab*(lab : Ju.Label);
    VAR label : AsmLabel;
        undef : BOOLEAN; (* ==> eval stack is invalid *)
  BEGIN
    label := lab(AsmLabel);
    ASSERT( label.defIx = 0 ); (* Labels are only defined once *)
   (*
    *  Only emit a label if a prior jump uses
    *  this label as a target, or forceEmit is 
    *  set as is the case for all loop headers.
    *)
    IF ~(Ju.jumpSeen IN label.attr) &
       ~(Ju.forceEmit IN label.attr) THEN 
      (* CSt.PrintLn("SKIPPING DefLab"); *)
      RETURN;
    END;

    emtr.thisMv.visitLabel( label.asmLb );
    label.defIx := label.asmLb.getOffset(); 

    undef := emtr.mthFrame.InvalidStack();
    IF Ju.assertEmpty IN label.attr THEN
      ASSERT( ~undef & (emtr.mthFrame.EvLn() = 0 ) );
    END;

    IF Ju.jumpSeen IN label.attr THEN
      emtr.mthFrame.RestoreFrameState( label.evalSave );
    ELSIF Ju.forceEmpty IN label.attr THEN
      emtr.mthFrame.RestoreFrameState( NIL );
    ELSIF undef THEN
     (*
      *  State should not be undef, if the label has been
      *  fallen into, i.e. is not following an unconditional jump.
      *)
      THROW( "Undefined stack state at back-edge target label" );
    END;
    INCL( label.attr, Ju.posFixed );
    EXCL( label.attr, Ju.unfixed );
    emtr.mthFrame.ValidateEvalStack();
    IF emtr.emitStackFrames THEN
      emtr.stackFramePending := TRUE;
    END;
  END DefLab;

 (* --------------------------------------------------------- *)
 (*   Define a commented Label location and update lab.defIx  *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)DefLabC*(lab : Ju.Label; 
                                       IN c : ARRAY OF CHAR);
  BEGIN
    emtr.DefLab( lab );
  END DefLabC;

 (* --------------------------------------------------------- *)
 (*   Emit an iinc instruction on a local variable of param   *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)CodeInc*(localIx,incVal : INTEGER);
  BEGIN
    emtr.CheckFrame();
    emtr.thisMv.visitIincInsn( localIx, incVal );
   (* no stack frame change *)
  END CodeInc;

 (* --------------------------------------------------------- *)
 (* Emit an instruction that takes a type arg, e.g. checkcast *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)CodeT*(code : INTEGER; ty : Sym.Type);
    VAR name : RTS.NativeString;
  BEGIN
    name := Hlp.tyNam( ty ); (* not signature! *)
    emtr.CheckFrame();
    emtr.thisMv.visitTypeInsn( code, name );
   (* 
    *  instanceof, checkcast, new, anewarray 
    *  Stack is bumped for new, otherwise unchanged
    *)
    CASE code OF
    | Jvm.opc_new        : emtr.mthFrame.FixEvalStack( code, ty );
    | Jvm.opc_anewarray  : emtr.mthFrame.SetTosSig( Hlp.tyArrSig( ty ) );
    | Jvm.opc_checkcast  : emtr.mthFrame.SetTosType( ty )
    | Jvm.opc_instanceof : emtr.mthFrame.SetTosType( Blt.intTp );
    END;
  END CodeT;

 (* --------------------------------------------------------- *)
 (*  For ASM, this call just allocates the array of ASM.Label *)
 (*  which will be filled in by AddSwitchLab calls. The ASM   *)
 (*  call to emit the tableswitch op is in CodeSwitchEnd.     *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)CodeSwitch*(low,high : INTEGER; 
                                   defLab : Ju.Label);
    VAR asmLabs : POINTER TO ARRAY OF ASM.Label;
        newLen : INTEGER;
   (* ----------------------------- *)
    PROCEDURE EvalStackAfterTableswitch(f : Frm.MethodFrame) : Frm.FrameSave;
      VAR rslt : Frm.FrameSave;
    BEGIN
      f.DeltaEvalDepth( -1 );
      rslt := f.GetFrameSave( NIL );
      f.FixEvalStack( Jvm.opc_iconst_0, Blt.intTp );
      RETURN rslt;
    END EvalStackAfterTableswitch;
   (* ----------------------------- *)
  BEGIN
    newLen := high - low + 1;
    NEW( asmLabs, newLen );
    emtr.caseArray := asmLabs;
    emtr.caseEval := EvalStackAfterTableswitch(emtr.mthFrame);
  END CodeSwitch;

 (* --------------------------------------------------------- *)
 (*      The dispatch table is passed to visitTableSwitch     *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)CodeSwitchEnd*( lo, hi : INTEGER;
                                               defLab : Ju.Label );
    VAR default : ASM.Label;
  BEGIN
    emtr.CheckFrame();
    WITH defLab : AsmLabel DO
      default := defLab.asmLb;
      INCL( defLab.attr, Ju.jumpSeen );
      defLab.evalSave := emtr.caseEval;
      emtr.thisMv.visitTableSwitchInsn( lo, hi, default, emtr.caseArray );
      INCL( defLab.attr, Ju.forceEmpty );
    END;
    emtr.mthFrame.InvalidateEvalStack();
  END CodeSwitchEnd;

 (* --------------------------------------------------------- *)
 (*     Inserts an ASM.Label in the scratch array in emtr     *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)AddSwitchLab*(lab : Ju.Label; 
                                             pos : INTEGER);
  BEGIN
    WITH lab : AsmLabel DO
      lab.evalSave := emtr.caseEval;
      emtr.caseArray[pos] := lab.asmLb;
      INCL( lab.attr, Ju.forceEmpty );
      INCL( lab.attr, Ju.jumpSeen );
    END;
  END AddSwitchLab;

 (* --------------------------------------------------------- *)
 (*         Emit a literal string to the constant pool        *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)PushStr*(IN str : Lv.CharOpen);
  BEGIN
   (* Push a literal string *)
    emtr.CheckFrame();
    emtr.thisMv.visitLdcInsn( MKSTR(str^) );
    emtr.mthFrame.FixEvalStack( Jvm.opc_ldc, CSt.ntvStr );
  END PushStr;

 (* --------------------------------------------------------- *)
 (*                 Load an integer constant                  *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)LoadConst*(num : INTEGER);
  BEGIN
(* FIXME for byte case *)
    emtr.CheckFrame();
    IF (num >= MIN(SHORTINT)) & (num <= MAX(SHORTINT)) THEN
      emtr.thisMv.visitIntInsn( Jvm.opc_sipush, num );
      emtr.mthFrame.FixEvalStack( Jvm.opc_sipush, Blt.sIntTp );
    ELSE
      emtr.thisMv.visitLdcInsn( Hlp.MkInteger( num ) );
      emtr.mthFrame.FixEvalStack( Jvm.opc_ldc, Blt.intTp );
    END;
  END LoadConst;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)CallGetClass*();
  BEGIN
    emtr.CheckFrame();
    emtr.thisMv.visitMethodInsn( 
        ASM.Opcodes.INVOKEVIRTUAL,
        jloStr,
        MKSTR("getClass"), 
        MKSTR("()Ljava/lang/Class;"),
        FALSE ); (* ==> not an interface *)
  END CallGetClass; 

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)CallRTS*(ix,args,ret : INTEGER);
    VAR classStr : RTS.NativeString;
  BEGIN
   (*
    *  Select the class that supplies the operation
    *)
    IF ix = Ju.ToUpper THEN
       classStr := jlchStr        (* java/lang/Character *)
    ELSIF ix = Ju.DFloor THEN
       classStr := jlmStr         (* java/lang/Math      *)
    ELSIF ix = Ju.SysExit THEN
       classStr := jlsyStr        (* java/lang/System    *)
    ELSE
       classStr := rtsStr         (* CP/CPJrts/CPJrts    *);
    END;

    emtr.CheckFrame();
    emtr.thisMv.visitMethodInsn( 
        ASM.Opcodes.INVOKESTATIC, classStr, 
        MKSTR(procNames[ix]^), MKSTR(procSigs[ix]^), FALSE );
    emtr.mthFrame.DeltaEvalDepth( ret - args );
    IF ret > 0 THEN emtr.mthFrame.SetTosState( procRetS[ix] ) END;
  END CallRTS; 

 (* --------------------------------------------------------- *)
 (*         Call a proc with a statically known name          *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)CallIT*(code : INTEGER; 
                                       proc : Id.Procs; 
                                       type : Ty.Procedure);
    VAR interface : BOOLEAN;
  BEGIN
    interface := code = Jvm.opc_invokeinterface;
    emtr.CheckFrame();
    emtr.thisMv.visitMethodInsn( code, Hlp.idCls( proc ), 
               Hlp.idNam( proc ), Hlp.idSig( proc ), interface );
    emtr.mthFrame.DeltaEvalDepth( type.retN - type.argN );
   (* 
    *  Return size retN may be non-zero for a pure procedure
    *  due to the movement of an OUT or VAR parameter to the
    *  return position. The JVM implementation return-type
    *  is denoted by the target-specific field tgXtn.aux
    *)
    IF type.retN > 0 THEN emtr.mthFrame.SetTosType( Hlp.tyRetTyp( type ) ) END;
  END CallIT;


 (* --------------------------------------------------------- *)
 (*   Emit head of the constructor for static class features  *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)ClinitHead*();
    VAR mv : ASM.MethodVisitor;
  BEGIN
    mv := emtr.clsWrtr.visitMethod(
        pubSta, clinitStr, noArgVoid, NIL, jlsEmptyArr );
    emtr.thisMv := mv;
    emtr.procNam := BOX("<clinit>");

    emtr.mthFrame := Frm.SigFrame( BOX("()V"), emtr.procNam, NIL );
    emtr.SetProcData( );

    mv.visitCode();
  END ClinitHead;

 (* --------------------------------------------------------- *)
 (*     Emit head of main( array of String ) static method    *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)MainHead*();
    VAR mv : ASM.MethodVisitor;
   (* --------------- *)
    PROCEDURE mkPrcId() : Id.Procs;
      VAR rslt : Id.PrcId;
          parN : Id.ParId;
    BEGIN
      NEW(rslt);
      rslt.setPrcKind(Id.conPrc);
      parN := Id.newParId()(Id.ParId); 
      parN.parMod := Sym.in;
      parN.type := CSt.ntvStrArr;
      Sym.AppendIdnt(rslt.locals, parN);
      RETURN rslt;
    END mkPrcId;
   (* --------------- *)

  BEGIN
    emtr.thisMth := mkPrcId();
    mv := emtr.clsWrtr.visitMethod(
        pubSta, "main", "([Ljava/lang/String;)V", NIL, jlsEmptyArr ); 

    mv.visitCode();
    emtr.thisMv := mv; 
    emtr.procNam := BOX("main");

    emtr.mthFrame := Frm.SigFrame( 
           BOX("([Ljava/lang/String;)V"), emtr.procNam, NIL );
    emtr.SetProcData( );
    emtr.entryLab := MkNewAsmLabel();
   (*
    *  Save the command line args to the RTS 
    *)
    emtr.AloadLocal( 0, CSt.ntvStrArr );
    mv.visitMethodInsn( 
        ASM.Opcodes.INVOKESTATIC, cpMain, "PutArgs", mainSig , FALSE );
    emtr.mthFrame.DeltaEvalDepth( -1 ); (* no SetTos* *)
  END MainHead;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)VoidTail*();
    VAR mv : ASM.MethodVisitor;
  BEGIN
    mv := emtr.thisMv; 
    emtr.CheckFrame();
    mv.visitInsn( ASM.Opcodes.`RETURN );
   (* no SetTos* *)
    IF CSt.debug & (emtr.thisMth # NIL) THEN
      emtr.exitLab := MkNewAsmLabel();
      mv.visitLabel( emtr.exitLab.asmLb );
      emtr.exitLab.defIx := emtr.exitLab.asmLb.getOffset();
      emtr.DumpLocalTable();
    END;
    mv.visitMaxs( emtr.mthFrame.maxEval, emtr.mthFrame.maxLocal );  
    mv.visitEnd();
    emtr.thisMv := NIL;
    emtr.thisMth := NIL;
  END VoidTail;

 (* --------------------------------------------------------- *)
 (*           Constructor for the module body class           *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)ModNoArgInit*();
    VAR mv : ASM.MethodVisitor;
   (* --------------- *)
    PROCEDURE mkPrcId(e : AsmEmitter) : Id.Procs;
      VAR rslt : Id.PrcId;
          parN : Id.ParId;
          type : Ty.Opaque;
    BEGIN
      NEW(rslt);
      type := Ty.newNamTp();
      type.xName := e.xName;
      type.scopeNm := Ju.cat3(Ju.lCap, e.xName, Ju.semi);
      rslt.setPrcKind(Id.conMth);
      parN := Id.newParId()(Id.ParId); 
      parN.isRcv := TRUE;
      parN.parMod := Sym.in;
      parN.type := type;
      Sym.AppendIdnt(rslt.locals, parN);
      RETURN rslt;
    END mkPrcId;
   (* --------------- *)
  BEGIN
    emtr.thisMth := mkPrcId(emtr);
   (* 
    * Create a new method visitor and save in emitter
    *)
    mv := emtr.clsWrtr.visitMethod(
        ASM.Opcodes.ACC_PUBLIC, "<init>", "()V", NIL, jlsEmptyArr );
    emtr.thisMv := mv;
    emtr.procNam := BOX("<init>");

    emtr.mthFrame := Frm.SigFrame( BOX("()V"), emtr.procNam, NIL );
    emtr.SetProcData( );
    mv.visitCode();
    emtr.entryLab := MkNewAsmLabel();
    mv.visitLabel( emtr.entryLab.asmLb );
    
    mv.visitVarInsn( ASM.Opcodes.ALOAD, 0 );
    emtr.mthFrame.DeltaEvalDepth( 1 );
    mv.visitMethodInsn( 
        ASM.Opcodes.INVOKESPECIAL, jloStr, "<init>", "()V", FALSE );
    emtr.mthFrame.DeltaEvalDepth( -1 );
    emtr.VoidTail();
(* FIXME: need to emit LocalVariableTable *)
  END ModNoArgInit;

 (* --------------------------------------------------------- *)
 (*             Noarg Constructor for record class            *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)RecMakeInit*(rec : Ty.Record;
                                            prc : Id.PrcId);
    VAR mv : ASM.MethodVisitor;
        sg  : Lv.CharOpen;
        pTp : Ty.Procedure;
  BEGIN
    IF (prc = NIL) &
       ((Sym.noNew IN rec.xAttr) OR (Sym.xCtor IN rec.xAttr)) THEN
      RETURN;
    END;
   (*
    *  Get the procedure type, if any.
    *)
    IF prc # NIL THEN
      pTp := prc.type(Ty.Procedure);
      Ju.MkCallAttr(prc, pTp);
      sg := pTp.xName;
    ELSE
      pTp := NIL;
      sg := BOX("()V");
    END;
   (* 
    * Create a new method visitor and save in emitter
    *)
    mv := emtr.clsWrtr.visitMethod(
        ASM.Opcodes.ACC_PUBLIC, "<init>", MKSTR(sg^), NIL, jlsEmptyArr );
    emtr.thisMv := mv;
    emtr.procNam := BOX("<init>");
    emtr.mthFrame := Frm.SigFrame( sg, emtr.procNam, NIL );
    emtr.SetProcData( );
    mv.visitCode();

    emtr.AloadLocal( 0, rec );
  END RecMakeInit;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)CallSuperCtor*(rec : Ty.Record;
                                          pTy : Ty.Procedure);
    VAR idx : INTEGER;
        fld : Sym.Idnt;
        baseStr, sg : RTS.NativeString;
        mv : ASM.MethodVisitor;
  BEGIN
    emtr.CheckFrame();
    IF pTy # NIL THEN
      sg := MKSTR(pTy.xName^);
    ELSE
      sg := noArgVoid;
    END;
   (* 
    *  Initialize the embedded superclass object
    *  The receiver object is already pushed on the stack
    *  QUESTION: what happens if the ctor needs args?
    *)
    IF (rec.baseTp # NIL) & (rec.baseTp # Blt.anyRec) THEN
      baseStr := MKSTR(rec.baseTp(Ty.Record).xName^);
    ELSE
      baseStr := jloStr;
    END;
    mv := emtr.thisMv;
    mv.visitMethodInsn(
        ASM.Opcodes.INVOKESPECIAL, baseStr, initStr, sg, FALSE );
    emtr.mthFrame.DeltaEvalDepth( -1 );
   (* 
    *  Initialize fields as necessary
    *)
    FOR idx := 0 TO rec.fields.tide - 1 DO
      fld := rec.fields.a[idx];
      IF (fld.type IS Ty.Record) OR 
         ((fld.type IS Ty.Array) & ~(fld.type IS Ty.Vector)) THEN
        emtr.AloadLocal( 0, rec ); (* ?? *)
        emtr.VarInit(fld);
        emtr.PutGetF( ASM.Opcodes.PUTFIELD, rec, fld(Id.FldId) );
      END;
    END;
  END CallSuperCtor;

 (* --------------------------------------------------------- *)
 (*  Emit the header for the record (shallow) __copy__ method *)
 (*  this method makes a *value* copy of the bound record.    *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)CopyProcHead*(rec : Ty.Record);
    VAR mv : ASM.MethodVisitor;
        sg : Lv.CharOpen;
   (* --------------- *)
    PROCEDURE mkMthId(rec : Ty.Record) : Id.Procs;
      VAR rslt : Id.MthId;
          parN : Id.ParId;
    BEGIN
      NEW(rslt);
      rslt.setPrcKind(Id.conMth);
     (* Receiver (dst) *)
      parN := Id.newParId()(Id.ParId); 
      parN.isRcv := TRUE;
      parN.parMod := Sym.var;
      parN.type := rec;
      Sym.AppendIdnt(rslt.locals, parN);
     (* Source val record *)
      parN := Id.newParId()(Id.ParId); 
      parN.parMod := Sym.in;
      parN.type := rec;
      Sym.AppendIdnt(rslt.locals, parN);
      RETURN rslt;
    END mkMthId;
   (* --------------- *)
  BEGIN
    emtr.thisMth := mkMthId(rec);
    sg := clsToVoidDesc(rec);

    mv := emtr.clsWrtr.visitMethod( 
            pubAcc, copyStr, MKSTR(sg^), NIL, jlsEmptyArr );
    emtr.thisMv := mv;
    emtr.procNam := BOX(copyStr); (* Coerce JLS to CharOpen *)
    
    emtr.mthFrame := Frm.SigFrame( sg, emtr.procNam, rec.xName );
    emtr.SetProcData( );
    mv.visitCode();
    emtr.entryLab := MkNewAsmLabel();
    mv.visitLabel( emtr.entryLab.asmLb );
  END CopyProcHead;

 (* --------------------------------------------------------- *)
 (*       Emit a call to the bound type __copy__ method       *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)ValRecCopy*(typ : Ty.Record);
  BEGIN
   (*
    *  Stack at entry is (top) srcRef, dstRef, ...
    *)
    emtr.CheckFrame();
    emtr.thisMv.visitMethodInsn( ASM.Opcodes.INVOKEVIRTUAL,
        MKSTR(typ.xName^), copyStr, MKSTR(clsToVoidDesc(typ)^), FALSE );
    emtr.mthFrame.DeltaEvalDepth( -2 );
  END ValRecCopy;

 (* --------------------------------------------------------- *)
 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)Try*();
  BEGIN
    emtr.rescueLab := MkNewAsmLabel();
    INCL( emtr.rescueLab.attr, Ju.forceEmit );
    emtr.thisMv.visitTryCatchBlock( 
        emtr.entryLab.asmLb, 
        emtr.rescueLab.asmLb, 
        emtr.rescueLab.asmLb, jleStr );
  END Try;

 (* --------------------------------------------------------- *)
 (*    At the catch block label stack depth is exactly one,   *)
 (*    and variable state exactly as at entry to Try block.   *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)Catch*(prc : Id.Procs);
    VAR ix : INTEGER;
  BEGIN
    emtr.mthFrame.ValidateEvalStack();
    emtr.mthFrame.setDepth( 1 );
    emtr.mthFrame.InvalidateLocals();
    emtr.mthFrame.SetTosType( CSt.ntvExc );
    emtr.DefLabC( emtr.rescueLab, "Catch Block Entry" );
   (*
    *  Remark:  at this label, stack depth is just one!
    *           After this store the stack is empty.
    *)
    emtr.StoreLocal( prc.except.varOrd, NIL ); (* NIL ==> use astore *)
  END Catch;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)MkNewException*();
  BEGIN
    emtr.CheckFrame();
    emtr.thisMv.visitTypeInsn( Jvm.opc_new, jleStr );
    emtr.mthFrame.FixEvalStack( Jvm.opc_new, CSt.ntvExc );
  END MkNewException;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)InitException*();
  BEGIN
    emtr.CheckFrame();
    emtr.thisMv.visitMethodInsn( 
        ASM.Opcodes.INVOKESPECIAL, jleStr, initStr, strToVoid, FALSE );
    emtr.mthFrame.DeltaEvalDepth( -2 );
  END InitException;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)Dump*();
    VAR rslt : POINTER TO ARRAY OF BYTE;
        indx : INTEGER;
  BEGIN
    emtr.clsWrtr.visitEnd();
    rslt := emtr.clsWrtr.toByteArray();
    FOR indx := 0 TO LEN(rslt) - 1 DO
      GPB.WriteByte( emtr.file, rslt[indx] );
    END;
    GPB.CloseFile( emtr.file );
  END Dump;

 (* --------------------------------------------------------- *
  * 
  *  PutField and GetField for class *static* fields 
  *  JVM static fields occur in two contexts in GPCP --
  *  * Global variables of module SomeMod are static fields 
  *    of the class CP/SomeMod/SomeMod.class
  *  * Static fields of some JVM class defined in some other
  *    language are accessed using the same instructions
  *
  * --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)PutGetS*(code : INTEGER; 
                                    blk  : Id.BlkId; (* not used anymore *)
                                    fld  : Id.VarId);
  BEGIN
    emtr.CheckFrame();
    emtr.thisMv.visitFieldInsn( 
        code, Hlp.idCls(fld), Hlp.idNam(fld), Hlp.idSig(fld) );
    emtr.mthFrame.PutGetFix( code, fld.type );
  END PutGetS;

 (* --------------------------------------------------------- *
  * 
  *  PutField and GetField for class *instance* fields 
  *
  * --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)PutGetF*(code : INTEGER; 
                                    rec  : Ty.Record; (* not used anymore *)
                                    fld  : Id.AbVar);
  BEGIN
    emtr.CheckFrame();
    emtr.thisMv.visitFieldInsn( 
        code, Hlp.idCls(fld), Hlp.idNam(fld), Hlp.idSig(fld) );
    emtr.mthFrame.PutGetFix( code, fld.type );
  END PutGetF;

 (* --------------------------------------------------------- *)
 (*   Allocate a one-dimensional array of given element type  *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)Alloc1d*(elTp : Sym.Type);
    VAR tName : RTS.NativeString;
  BEGIN
    emtr.CheckFrame();
    WITH elTp : Ty.Base DO
      IF (elTp.tpOrd < Ty.anyRec) OR (elTp.tpOrd = Ty.uBytN) THEN
        emtr.thisMv.visitIntInsn(Jvm.opc_newarray, typeArr[elTp.tpOrd]);
        emtr.mthFrame.FixEvalStack( Jvm.opc_newarray, typeArrArr[elTp.tpOrd] );
      ELSE
        emtr.thisMv.visitTypeInsn( Jvm.opc_anewarray, jloStr );
        emtr.mthFrame.FixEvalStack( Jvm.opc_anewarray, CSt.ntvStrArr );
      END;
    ELSE
      emtr.thisMv.visitTypeInsn( Jvm.opc_anewarray, Hlp.tyNam( elTp ) );
      emtr.mthFrame.FixEvalSig( Jvm.opc_newarray, Hlp.tyArrSig( elTp ) );
    END;
  END Alloc1d;


 (* --------------------------------------------------------- *)
 (*  Initialize a declared variable -                         *)
 (*  Because the JVM type system does not have value          *)
 (*  aggregates ALL aggregate type have to be allocated on    *)
 (*  heap at the point of declaration. This method does this. *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)VarInit*(var : Sym.Idnt);
    VAR typ : Sym.Type;
  BEGIN
   (*
    *  Precondition: var is of a type that needs initialization
    *)
    typ := var.type;
    WITH typ : Ty.Record DO
        emtr.MkNewRecord(typ);
    | typ : Ty.Array DO
        emtr.MkNewFixedArray(typ.elemTp, typ.length);
    ELSE
      emtr.Code(Jvm.opc_aconst_null);
    END;
   (* --------------- *)
  END VarInit;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)Trap*(IN str : ARRAY OF CHAR);
    VAR mv : ASM.MethodVisitor;
  BEGIN
    emtr.CheckFrame();
    mv := emtr.thisMv;
    mv.visitTypeInsn( ASM.Opcodes.`NEW, jleStr );
    mv.visitInsn( ASM.Opcodes.DUP );
    mv.visitLdcInsn( MKSTR(str) );
    emtr.mthFrame.DeltaEvalDepth( 3 );
    mv.visitMethodInsn(
        ASM.Opcodes.INVOKESPECIAL, jleStr, initStr, strToVoid, FALSE );
    emtr.mthFrame.DeltaEvalDepth( -2 );
    mv.visitInsn( ASM.Opcodes.ATHROW );
    emtr.mthFrame.InvalidateEvalStack();
   (* no net stack change *)
  END Trap;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)CaseTrap*(i : INTEGER);
    VAR mv : ASM.MethodVisitor;
  BEGIN
    emtr.CheckFrame();
    mv := emtr.thisMv;
    mv.visitTypeInsn( ASM.Opcodes.`NEW, jleStr );
    mv.visitInsn( ASM.Opcodes.DUP );
    emtr.LoadLocal( i, Blt.intTp );
    emtr.mthFrame.DeltaEvalDepth( 3 );
    mv.visitMethodInsn(
        ASM.Opcodes.INVOKESTATIC, rtsStr, caseTrap, caseTrapSig, FALSE );
    mv.visitMethodInsn(
        ASM.Opcodes.INVOKESPECIAL, jleStr, initStr, strToVoid, FALSE );
    emtr.mthFrame.DeltaEvalDepth( -2 );
    mv.visitInsn( ASM.Opcodes.ATHROW );
    emtr.mthFrame.InvalidateEvalStack();
   (* no net stack change *)
  END CaseTrap;

 (* --------------------------------------------------------- *)

  PROCEDURE (emtr : AsmEmitter)WithTrap*(id : Sym.Idnt);
    VAR mv : ASM.MethodVisitor;
  BEGIN
    emtr.CheckFrame();
    mv := emtr.thisMv;
    mv.visitTypeInsn( ASM.Opcodes.`NEW, jleStr ); (* +1 *)
    mv.visitInsn( ASM.Opcodes.DUP );              (* +2 *)
    emtr.GetVar( id );                            (* +3 *)
    emtr.mthFrame.DeltaEvalDepth( 3 );
    mv.visitMethodInsn(
        ASM.Opcodes.INVOKESTATIC, rtsStr, withTrap, withTrapSig, FALSE );
    mv.visitMethodInsn(
        ASM.Opcodes.INVOKESPECIAL, jleStr, initStr, strToVoid, FALSE );
    emtr.mthFrame.DeltaEvalDepth( -2 );
    mv.visitInsn( ASM.Opcodes.ATHROW );
    emtr.mthFrame.InvalidateEvalStack();
   (* no net stack change *)
  END WithTrap;

 (* --------------------------------------------------------- *)
 (*     We presume that there is not need otherwise for       *)
 (*     a label at this location.  So we generate a dummy.    *)
 (* --------------------------------------------------------- *)
  PROCEDURE (emtr : AsmEmitter)Line*(lnNm : INTEGER);
    VAR dummy : ASM.Label;
        mv : ASM.MethodVisitor;
  BEGIN
    NEW( dummy );
    mv := emtr.thisMv;
    mv.visitLabel( dummy );
    mv.visitLineNumber( lnNm, dummy );
  END Line;

(* ============================================================ *)
(* ============================================================ *)
(*                        Static Methods                        *)
(* ============================================================ *)
(* ============================================================ *)

 (* ------------------------------------------------- *)
 (* ------------------------------------------------- *)
  PROCEDURE MkNewAsmLabel( ) : AsmLabel;
    VAR rslt : AsmLabel;
  BEGIN
    NEW( rslt );
    NEW( rslt.asmLb );
    rslt.defIx := 0;
    rslt.attr := { Ju.unfixed }; (* Alwasy born unfixed *)
    rslt.evalSave := NIL;
    rslt.FixTag();  (* assigns a dummy name to this label *)
    RETURN rslt;
  END MkNewAsmLabel;

 (* ------------------------------------------------- *)

  PROCEDURE InterfaceNameList( rec : Ty.Record ) : Def.JlsArr;
    VAR result  : Def.JlsArr;
        element : Lv.CharOpen;
        ix,len  : INTEGER;
  BEGIN
    len := rec.interfaces.tide;
    IF len > 0 THEN
      NEW(result, len);
      FOR ix := 0 TO len - 1 DO
        element := rec.interfaces.a[ix].boundRecTp()(Ty.Record).xName;
        result[ix] := MKSTR(element^);
      END;
    ELSE
      result := jlsEmptyArr;
    END;
    RETURN result;
  END InterfaceNameList;

 (* ------------------------------------------------- *)

  PROCEDURE clsToVoidDesc(rec : Ty.Record) : Lv.CharOpen;
  BEGIN
    Hlp.EnsureRecName( rec );
    RETURN Ju.cat3(Ju.lPar,rec.scopeNm,Ju.rParV); 
  END clsToVoidDesc;
  
(* ============================================================ *)
BEGIN  (* Module Body *)
  tagCount := 0;
 (* -------------------------------------------- *)
  jloStr        := MKSTR( "java/lang/Object"    );
  jlsStr        := MKSTR( "java/lang/String"    );
  initStr       := MKSTR( "<init>"              );
  clinitStr     := MKSTR( "<clinit>"            );
  copyStr       := MKSTR( "__copy__"            );
  jleStr        := MKSTR( "java/lang/Exception" );
  jlmStr        := MKSTR( "java/lang/Math"      );
  jlcStr        := MKSTR( "java/lang/Class"     );
  jlchStr       := MKSTR( "java/lang/Character" );
  jlsyStr       := MKSTR( "java/lang/System"    );

  cpMain        := MKSTR( "CP/CPmain/CPmain"    );
  rtsStr        := MKSTR( "CP/CPJrts/CPJrts"    );

  noArgVoid     := MKSTR( "()V"                    );
  strToVoid     := MKSTR( "(Ljava/lang/String;)V"  );
  mainSig       := MKSTR( "([Ljava/lang/String;)V" );
  
 (* --------------------------------------------- *)
  withTrap      := MKSTR( "WithMesg"               ); 
  withTrapSig   := 
     MKSTR("(Ljava/lang/Object;)Ljava/lang/String;");
  caseTrap      := MKSTR( "CaseMesg"               ); 
  caseTrapSig   := 
     MKSTR("(I)Ljava/lang/String;");
 (* --------------------------------------------- *)

  pubAcc        := ASM.Opcodes.ACC_PUBLIC;
  prvAcc        := ASM.Opcodes.ACC_PRIVATE;
  finAcc        := ASM.Opcodes.ACC_FINAL;
  supAcc        := ASM.Opcodes.ACC_SUPER;
  staAcc        := ASM.Opcodes.ACC_STATIC;
  absAcc        := ASM.Opcodes.ACC_ABSTRACT;
  protec        := ASM.Opcodes.ACC_PROTECTED;
  pckAcc        := 0;

  pubSta        := pubAcc + staAcc;
  modAcc        := pubAcc + finAcc + supAcc;

  fileSep := Lv.charToCharOpen(GPF.fileSep);

  procNames[Ju.StrCmp]  := Lv.strToCharOpen("strCmp");
  procNames[Ju.StrToChrOpen] := Lv.strToCharOpen("JavaStrToChrOpen");
  procNames[Ju.StrToChrs] := Lv.strToCharOpen("JavaStrToFixChr");
  procNames[Ju.ChrsToStr] := Lv.strToCharOpen("FixChToJavaStr");
  procNames[Ju.StrCheck] := Lv.strToCharOpen("ChrArrCheck");
  procNames[Ju.StrLen] := Lv.strToCharOpen("ChrArrLength");
  procNames[Ju.ToUpper] := Lv.strToCharOpen("toUpperCase");
  procNames[Ju.DFloor] := Lv.strToCharOpen("floor");
  procNames[Ju.ModI] := Lv.strToCharOpen("CpModI");
  procNames[Ju.ModL] := Lv.strToCharOpen("CpModL");
  procNames[Ju.DivI] := Lv.strToCharOpen("CpDivI");
  procNames[Ju.DivL] := Lv.strToCharOpen("CpDivL");
  procNames[Ju.StrCatAA] := Lv.strToCharOpen("ArrArrToString");
  procNames[Ju.StrCatSA] := Lv.strToCharOpen("StrArrToString");
  procNames[Ju.StrCatAS] := Lv.strToCharOpen("ArrStrToString");
  procNames[Ju.StrCatSS] := Lv.strToCharOpen("StrStrToString");
  procNames[Ju.StrLP1] := Lv.strToCharOpen("ChrArrLplus1");
  procNames[Ju.StrVal] := Lv.strToCharOpen("ChrArrStrCopy");
  procNames[Ju.SysExit] := Lv.strToCharOpen("exit");
  procNames[Ju.LoadTp1] := Lv.strToCharOpen("getClassByOrd");
  procNames[Ju.LoadTp2] := Lv.strToCharOpen("getClassByName");

  getClass := Lv.strToCharOpen("getClass");
  IIretI   := Lv.strToCharOpen("(II)I");
  JJretJ   := Lv.strToCharOpen("(JJ)J");

  procSigs[Ju.StrCmp]   := Lv.strToCharOpen("([C[C)I");
  procSigs[Ju.StrToChrOpen] := Lv.strToCharOpen("(Ljava/lang/String;)[C");
  procSigs[Ju.StrToChrs] := Lv.strToCharOpen("([CLjava/lang/String;)V");
  procSigs[Ju.ChrsToStr] := Lv.strToCharOpen("([C)Ljava/lang/String;");
  procSigs[Ju.StrCheck] := Lv.strToCharOpen("([C)V");
  procSigs[Ju.StrLen] := Lv.strToCharOpen("([C)I");
  procSigs[Ju.ToUpper] := Lv.strToCharOpen("(C)C");
  procSigs[Ju.DFloor] := Lv.strToCharOpen("(D)D");
  procSigs[Ju.ModI] := IIretI;
  procSigs[Ju.ModL] := JJretJ;
  procSigs[Ju.DivI] := IIretI;
  procSigs[Ju.DivL] := JJretJ;
  procSigs[Ju.StrCatAA] := Lv.strToCharOpen("([C[C)Ljava/lang/String;");
  procSigs[Ju.StrCatSA] := Lv.strToCharOpen(
                                "(Ljava/lang/String;[C)Ljava/lang/String;");
  procSigs[Ju.StrCatAS] := Lv.strToCharOpen(
                                "([CLjava/lang/String;)Ljava/lang/String;");
  procSigs[Ju.StrCatSS] := Lv.strToCharOpen(
                   "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
  procSigs[Ju.StrLP1] := procSigs[Ju.StrLen];
  procSigs[Ju.StrVal] := Lv.strToCharOpen("([C[C)V");
  procSigs[Ju.SysExit] := Lv.strToCharOpen("(I)V");
  procSigs[Ju.LoadTp1] := Lv.strToCharOpen("(I)Ljava/lang/Class;");
  procSigs[Ju.LoadTp2] := Lv.strToCharOpen(
                               "(Ljava/lang/String;)Ljava/lang/Class;");

  procRetS[Ju.StrVal] := NIL;
  procRetS[Ju.SysExit] := NIL;
  procRetS[Ju.StrToChrs] := NIL;
  procRetS[Ju.StrCheck] := NIL;

  procRetS[Ju.StrCmp] := Lv.strToCharOpen("I");
  procRetS[Ju.ModI]   := procRetS[Ju.StrCmp];
  procRetS[Ju.DivI]   := procRetS[Ju.StrCmp];
  procRetS[Ju.StrLen] := procRetS[Ju.StrCmp];
  procRetS[Ju.StrLP1] := procRetS[Ju.StrCmp];

  procRetS[Ju.DivL] := Lv.strToCharOpen( "J" );
  procRetS[Ju.ModL] := procRetS[Ju.DivL];

  procRetS[Ju.LoadTp1] := Lv.strToCharOpen( "Ljava/lang/Class;");
  procRetS[Ju.LoadTp2] := procRetS[Ju.LoadTp1];

  procRetS[Ju.ChrsToStr] := Lv.strToCharOpen("Ljava/lang/String;");
  procRetS[Ju.StrCatAA]  := procRetS[Ju.ChrsToStr];
  procRetS[Ju.StrCatSA]  := procRetS[Ju.ChrsToStr];
  procRetS[Ju.StrCatAS]  := procRetS[Ju.ChrsToStr];
  procRetS[Ju.StrCatSS]  := procRetS[Ju.ChrsToStr];

  procRetS[Ju.StrToChrOpen] := Lv.strToCharOpen("[C");
  procRetS[Ju.ToUpper] := Lv.strToCharOpen("C");
  procRetS[Ju.DFloor] := Lv.strToCharOpen("D");

 (* Mapping of base types *)
  typeArr[ Ty.boolN] := ASM.Opcodes.T_BOOLEAN;
  typeArr[ Ty.sChrN] := ASM.Opcodes.T_SHORT;
  typeArr[ Ty.charN] := ASM.Opcodes.T_CHAR;
  typeArr[ Ty.byteN] := ASM.Opcodes.T_BYTE;
  typeArr[ Ty.uBytN] := ASM.Opcodes.T_BYTE;
  typeArr[ Ty.sIntN] := ASM.Opcodes.T_SHORT;
  typeArr[  Ty.intN] := ASM.Opcodes.T_INT;
  typeArr[ Ty.lIntN] := ASM.Opcodes.T_LONG;
  typeArr[ Ty.sReaN] := ASM.Opcodes.T_FLOAT;
  typeArr[ Ty.realN] := ASM.Opcodes.T_DOUBLE;
  typeArr[  Ty.setN] := ASM.Opcodes.T_INT;

 (* Arrays of base types *)
  typeArrArr[ Ty.boolN] := Ty.mkArrayOf( Blt.boolTp );
  typeArrArr[ Ty.charN] := Ty.mkArrayOf( Blt.charTp );
  typeArrArr[ Ty.sChrN] := typeArrArr[ Ty.charN ];
  typeArrArr[ Ty.byteN] := Ty.mkArrayOf( Blt.byteTp );
  typeArrArr[ Ty.uBytN] := typeArrArr[ Ty.byteN ];
  typeArrArr[ Ty.sIntN] := Ty.mkArrayOf( Blt.sIntTp );
  typeArrArr[  Ty.intN] := Ty.mkArrayOf( Blt.intTp  );
  typeArrArr[ Ty.lIntN] := Ty.mkArrayOf( Blt.lIntTp );
  typeArrArr[ Ty.sReaN] := Ty.mkArrayOf( Blt.sReaTp );
  typeArrArr[ Ty.realN] := Ty.mkArrayOf( Blt.realTp );
  typeArrArr[  Ty.setN] := typeArrArr[ Ty.intN ];

END AsmUtil.
(* ============================================================ *)

