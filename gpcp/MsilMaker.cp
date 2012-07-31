(* ============================================================ *)
(*  MsilMaker is the concrete class for emitting COM2+          *)
(*  intermediate language for the VOS.                          *)
(*  Copyright (c) John Gough 1999, 2000.                        *)
(*      Ugly kludge for covariant OUT params comes out          *)
(*  next time. Search "fixup".  (kjg, 19 May 2001)              *)
(* ============================================================ *)

MODULE MsilMaker;

  IMPORT
        GPCPcopyright,
        Error,
        GPText,
        Console,
        FileNames,
        MsilAsm,
        MsilBase,
        ClassMaker,
        GPFiles,
        GPBinFiles,
        GPTextFiles,
        PeUtil,
        IlasmUtil,
        Nh  := NameHash,
        Scn := CPascalS,
        Psr := CPascalP,
        CSt := CompState,
        Asm := IlasmCodes,
        Mu  := MsilUtil,
        Lv  := LitValue,
        Bi  := Builtin,
        Sy  := Symbols,
        Id  := IdDesc ,
        Ty  := TypeDesc,
        Xp  := ExprDesc,
        St  := StatDesc;

(* ============================================================ *)

  CONST pubStat     = Asm.att_public + Asm.att_static;
        staticAtt   = Asm.att_static;
        extern      = Asm.att_extern;

  CONST inlineLimit = 4; (* limit for inline expansion of element copies *)

(* ============================================================ *)

  TYPE MsilEmitter* =
        POINTER TO
          RECORD (MsilBase.ClassEmitter)
         (* --------------------------- *
          * mod* : Id.BlkId;            *
          * --------------------------- *)
            work : Sy.IdSeq;
            outF : Mu.MsilFile;
          END;

(* ------------------------------------ *)

  TYPE MsilAssembler* =
        POINTER TO
          RECORD (ClassMaker.Assembler)
            emit : Mu.MsilFile;
          END;

(* ------------------------------------ *)

  VAR  asmName   : Lv.CharOpen;
       asmExe    : BOOLEAN;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE newMsilEmitter*(mod : Id.BlkId) : MsilEmitter;
    VAR emitter : MsilEmitter;
  BEGIN
    NEW(emitter);
    emitter.mod := mod;
    MsilBase.emitter := emitter;
    Sy.InitIdSeq(emitter.work, 4);
    Sy.AppendIdnt(emitter.work, mod);
    RETURN emitter;
  END newMsilEmitter;

(* ============================================================ *)

  PROCEDURE newMsilAsm*() : MsilAssembler;
    VAR asm : MsilAssembler;
  BEGIN
    NEW(asm);
    MsilAsm.Init();
    RETURN asm;
  END newMsilAsm;

(* ============================================================ *)

  PROCEDURE IdentOf(x : Sy.Expr) : Sy.Idnt;
  BEGIN
    WITH x : Xp.IdLeaf DO RETURN x.ident;
       | x : Xp.IdentX DO RETURN x.ident;
    ELSE      RETURN NIL;
    END;
  END IdentOf;

(* ============================================================ *)

  PROCEDURE fieldAttr(id : Sy.Idnt; in : SET) : SET;
  BEGIN
    IF id.type IS Ty.Event THEN (* backing field of event *)
      RETURN in + Asm.att_private;   
    ELSIF id.vMod # Sy.prvMode THEN
      RETURN in + Asm.att_public;
    ELSE
      RETURN in + Asm.att_assembly;
    END;
  END fieldAttr;

(* ============================================================ *)
(*  Creates basic imports for System, and inserts a few type  *)
(*  descriptors for Object, Exception, and String.              *)
(* ============================================================ *)

  PROCEDURE (this : MsilEmitter)Init*();
    VAR tId : Id.TypId;
        blk : Id.BlkId;
        obj : Id.TypId;
        str : Id.TypId;
        exc : Id.TypId;
        typ : Id.TypId;
        del : Id.TypId;
        evt : Id.TypId;
  BEGIN
   (*
    *  Create import descriptor for [mscorlib]System
    *)
    Builtin.MkDummyImport("mscorlib_System", "[mscorlib]System", blk);
	CSt.SetSysLib(blk);
   (*
    *  Create various classes.
    *)
    Builtin.MkDummyClass("Object", blk, Ty.isAbs, obj);
    CSt.ntvObj := obj.type;
    Builtin.MkDummyClass("String", blk, Ty.noAtt, str);
    Builtin.SetPtrBase(str, obj);
    CSt.ntvStr := str.type;
    Builtin.MkDummyClass("Exception", blk, Ty.extns, exc);
    Builtin.SetPtrBase(exc, obj);
    CSt.ntvExc := exc.type;
    Builtin.MkDummyClass("Type", blk, Ty.isAbs, typ);
    Builtin.SetPtrBase(typ, obj);
    CSt.ntvTyp := typ.type;

    Builtin.MkDummyClass("Delegate", blk, Ty.extns, del);
    Builtin.SetPtrBase(del, obj);
    Builtin.MkDummyClass("MulticastDelegate", blk, Ty.extns, evt);
    Builtin.SetPtrBase(evt, del);
    CSt.ntvEvt := evt.type;

    (* NEED SOME WORK HERE?? *)

    Builtin.MkDummyClass("ValueType", blk, Ty.extns, del);
    Builtin.SetPtrBase(del, obj);
    CSt.ntvVal := del.type.boundRecTp();

    Mu.SetNativeNames();

   (*
    *  Create import descriptor for [RTS]RTS
    *)
    Builtin.MkDummyImport("RTS", "[RTS]", blk);
    Builtin.MkDummyAlias("NativeType", blk, typ.type, CSt.clsId);
    Builtin.MkDummyAlias("NativeObject", blk, obj.type, CSt.objId);
    Builtin.MkDummyAlias("NativeString", blk, str.type, CSt.strId);
    Builtin.MkDummyAlias("NativeException", blk, exc.type, CSt.excId);
    INCL(blk.xAttr, Sy.need);
    CSt.rtsBlk := blk;
   (*
    *  Uplevel addressing stuff. This is part of RTS assembly.
    *)
    Builtin.MkDummyClass("XHR", blk, Ty.isAbs, typ);
    CSt.rtsXHR := typ.type;
    CSt.xhrId.recTyp := CSt.rtsXHR.boundRecTp();
    CSt.xhrId.type   := CSt.rtsXHR;
   (*
    *  Access to [RTS]RTS::dblPosInfinity, etc.
    *)
    Builtin.MkDummyVar("dblPosInfinity", blk, Bi.realTp, CSt.dblInf);
    Builtin.MkDummyVar("dblNegInfinity", blk, Bi.realTp, CSt.dblNInf);
    Builtin.MkDummyVar("fltPosInfinity", blk, Bi.sReaTp, CSt.fltInf);
    Builtin.MkDummyVar("fltNegInfinity", blk, Bi.sReaTp, CSt.fltNInf);
   (*
    *  Access to [RTS]ProgArgs::argList
    *)
    Builtin.MkDummyImport("ProgArgs", "", blk);
    Builtin.MkDummyVar("argList", blk, Ty.mkArrayOf(CSt.ntvStr), CSt.argLst);
    INCL(blk.xAttr, Sy.rtsMd);
    CSt.prgArg := blk;
  END Init;

(* ============================================================ *)

  PROCEDURE (this : MsilEmitter)ObjectFeatures*();
    VAR prcSig : Ty.Procedure; 
        thePar : Id.ParId;
  BEGIN
	NEW(prcSig);
    prcSig.retType := CSt.strId.type;
    Id.InitParSeq(prcSig.formals, 2);
    Bi.MkDummyMethodAndInsert("ToString", prcSig, CSt.ntvObj, CSt.sysLib, Sy.pubMode, Sy.var, Id.extns);

	NEW(prcSig);
    prcSig.retType := Bi.intTp;
    Id.InitParSeq(prcSig.formals, 2);
    Bi.MkDummyMethodAndInsert("GetHashCode", prcSig, CSt.ntvObj, CSt.sysLib, Sy.pubMode, Sy.var, Id.extns);

	NEW(prcSig);
    prcSig.retType := CSt.ntvObj;
    Id.InitParSeq(prcSig.formals, 2);
    Bi.MkDummyMethodAndInsert("MemberwiseClone", prcSig, CSt.ntvObj, CSt.sysLib, Sy.protect, Sy.var, Id.extns);

	NEW(prcSig);
	NEW(thePar);
    prcSig.retType := Bi.boolTp;
    Id.InitParSeq(prcSig.formals, 2);
	thePar.parMod := Sy.val;
	thePar.type := CSt.ntvObj;
	thePar.varOrd := 1;
	Id.AppendParam(prcSig.formals, thePar);
    Builtin.MkDummyMethodAndInsert("Equals", prcSig, CSt.ntvObj, CSt.sysLib, Sy.pubMode, Sy.var, IdDesc.extns);
  END ObjectFeatures;

(* ============================================================ *)

   PROCEDURE (this : MsilEmitter)mkThreadAssign() : Sy.Stmt,NEW; 
     VAR stmt : Sy.Stmt;
         text : ARRAY 3 OF Lv.CharOpen;
   BEGIN
     text[0] := BOX("__thread__ := mscorlib_System_Threading.Thread.init(__wrapper__);");
     text[1] := BOX("__thread__.set_ApartmentState(mscorlib_System_Threading.ApartmentState.STA);");
     text[2] := BOX("__thread__.Start(); END");
     stmt := Psr.parseTextAsStatement(text, CSt.thisMod);
     stmt.StmtAttr(CSt.thisMod);
     RETURN stmt;
   END mkThreadAssign;
   
(* ============================================================ *)

   PROCEDURE (this : MsilEmitter)AddStaMembers(),NEW; 
     VAR text : ARRAY 3 OF Lv.CharOpen;
         proc : Sy.Idnt;
   BEGIN
     text[0] := BOX("VAR __thread__ : mscorlib_System_Threading.Thread;");
     text[1] := BOX("PROCEDURE __wrapper__(); BEGIN END __wrapper__;");
     text[2] := BOX("END");
     Psr.ParseDeclarationText(text, CSt.thisMod);
     proc := Sy.bindLocal(Nh.enterStr("__wrapper__"), CSt.thisMod);
     proc(Id.PrcId).body := CSt.thisMod.modBody;
   END AddStaMembers;

(* ============================================================ *)

  PROCEDURE (this : MsilAssembler)Assemble*();
  (** Overrides EMPTY method in ClassMaker *)
    VAR rslt : INTEGER;
        optA : Lv.CharOpen;
   (* ------------------------------------ *)
    PROCEDURE buildOption(isExe : BOOLEAN) : Lv.CharOpen;
      VAR str : Lv.CharOpen;
          ext : ARRAY 5 OF CHAR;
    BEGIN
      str := NIL;
      IF isExe THEN ext := ".exe" ELSE ext := ".dll" END;
      IF CSt.binDir # "" THEN
        str := BOX("/OUT=" + CSt.binDir);
        IF str[LEN(str) - 2] = GPFiles.fileSep THEN
          str := BOX(str^ + asmName^ + ext);
        ELSE
          str := BOX(str^ + "\" + asmName^ + ext);
        END;
      END;
      IF CSt.debug THEN
        IF str = NIL THEN str := BOX("/debug");
        ELSE str := BOX(str^ + " /debug");
        END;
      END;
      IF str = NIL THEN RETURN BOX(" ") ELSE RETURN str END;
    END buildOption;
   (* ------------------------------------ *)
  BEGIN
    IF asmName # NIL THEN
      MsilAsm.DoAsm(asmName, buildOption(asmExe), asmExe, CSt.verbose, rslt);
      IF rslt # 0 THEN CSt.thisMod.IdError(298) END;
    END;
  END Assemble;

(* ============================================================ *)

  PROCEDURE (this : MsilEmitter)AddNewRecEmitter*(inTp : Sy.Type);
  (* Overrides AddNewRecEmitter() in MsilBase. *)
    VAR idnt : Sy.Idnt;
  BEGIN
    idnt := NIL;
    WITH inTp : Ty.Record DO
      IF inTp.bindTp # NIL THEN idnt := inTp.bindTp.idnt;
      ELSIF inTp.idnt # NIL THEN idnt := inTp.idnt;
      ELSE ASSERT(FALSE);
      END;
    ELSE
      idnt := inTp.idnt;
    END;
    Sy.AppendIdnt(this.work, idnt);
  END AddNewRecEmitter;

(* ============================================================ *)

  PROCEDURE^ (e : MsilEmitter)ValueCopy(act : Sy.Expr; fmT : Sy.Type),NEW;
  PROCEDURE^ (e : MsilEmitter)EmitProc(proc : Id.Procs; attr : SET),NEW;
  PROCEDURE^ (e : MsilEmitter)PushValue(exp : Sy.Expr; typ : Sy.Type),NEW;
  PROCEDURE^ (e : MsilEmitter)PushHandle(exp : Sy.Expr; typ : Sy.Type),NEW;
  PROCEDURE^ (e : MsilEmitter)PushRef(exp : Sy.Expr; typ : Sy.Type),NEW;
  PROCEDURE^ (e : MsilEmitter)EmitStat(stat : Sy.Stmt; OUT ok : BOOLEAN),NEW;
  PROCEDURE^ (e : MsilEmitter)PushCall(callX : Xp.CallX),NEW;
  PROCEDURE^ (e : MsilEmitter)FallFalse(exp : Sy.Expr; tLb : Mu.Label),NEW;

  PROCEDURE^ (e : MsilEmitter)RefRecCopy(typ : Ty.Record),NEW;
  PROCEDURE^ (e : MsilEmitter)RefArrCopy(typ : Ty.Array),NEW;
  PROCEDURE^ (e : MsilEmitter)GetArgP(act : Sy.Expr; frm : Id.ParId),NEW;

(* ============================================================ *)

  PROCEDURE (t : MsilEmitter)MakeInit(rec : Ty.Record;
                                      prc : Id.PrcId),NEW;
    VAR out : Mu.MsilFile;
        idx : INTEGER;
        fld : Sy.Idnt;
        spr : Id.PrcId;
        frm : Id.ParId;
        exp : Sy.Expr;
        spT : Ty.Procedure;
        lve : BOOLEAN;
  BEGIN
    spr := NIL;
    out := t.outF;
    out.Blank();
    IF prc = NIL THEN
      IF Sy.noNew IN rec.xAttr THEN
        out.Comment("There is no no-arg constructor for this class");
        out.Blank();
        RETURN;                                    (* PREMATURE RETURN HERE *)
      ELSIF Sy.xCtor IN rec.xAttr THEN
        out.Comment("There is an explicit no-arg constructor for this class");
        out.Blank();
        RETURN;                                    (* PREMATURE RETURN HERE *)
      END;
    END;
    out.MkNewProcInfo(prc);
    out.InitHead(rec, prc);
    IF prc # NIL THEN
      spr := prc.basCll.sprCtor(Id.PrcId);
      IF spr # NIL THEN
        spT := spr.type(Ty.Procedure);
        IF spT.xName = NIL THEN Mu.MkCallAttr(spr, out) END;
        FOR idx := 0 TO prc.basCll.actuals.tide - 1 DO
          frm := spT.formals.a[idx];
          exp := prc.basCll.actuals.a[idx];
          t.GetArgP(exp, frm);
        END;
      END;
    END;
    out.CallSuper(rec, spr);
   (*
    *    Initialize fields, as necessary.
    *)
    IF rec # NIL THEN
      FOR idx := 0 TO rec.fields.tide-1 DO
        fld := rec.fields.a[idx];
        IF Mu.needsInit(fld.type) THEN
          out.CommentT("Initialize embedded object");
          out.Code(Asm.opc_ldarg_0);
          out.StructInit(fld);
        END;
      END;
    END;
    IF (prc # NIL) & (prc.body # NIL) THEN
      IF prc.rescue # NIL THEN out.Try END;
      t.EmitStat(prc.body, lve);
      IF lve THEN out.DoReturn END;
      IF prc.rescue # NIL THEN
        out.Catch(prc);
        t.EmitStat(prc.rescue, lve);
        IF lve THEN out.DoReturn END;
        out.EndCatch;
      END;
    ELSE
      out.Code(Asm.opc_ret);
    END;
    out.InitTail(rec);
  END MakeInit;

(* ============================================================ *)

  PROCEDURE (t : MsilEmitter)CopyProc(recT : Ty.Record),NEW;
    VAR out  : Mu.MsilFile;
        indx : INTEGER;
        fTyp : Sy.Type;
        idnt : Id.FldId;
  BEGIN
   (*
    *   Emit the copy procedure "__copy__()
    *)
    out := t.outF;
    out.Blank();
    out.MkNewProcInfo(t.mod);
    out.CopyHead(recT);
   (*
    *    Recurse to super class, if necessary.
    *)
    IF (recT.baseTp # NIL) &
       ~recT.baseTp.isNativeObj() THEN
(*
 *     (recT.baseTp IS Ty.Record) THEN
 *)
      out.Code(Asm.opc_ldarg_0);
      out.Code(Asm.opc_ldarg_1);
      t.RefRecCopy(recT.baseTp(Ty.Record));
    END;
   (*
    *    Emit field-by-field copy.
    *)
    FOR indx := 0 TO recT.fields.tide-1 DO
      idnt := recT.fields.a[indx](Id.FldId);
      fTyp := idnt.type;
      out.Code(Asm.opc_ldarg_0);
      IF Mu.hasValueRep(fTyp) THEN
        out.Code(Asm.opc_ldarg_1);
        out.GetField(idnt);
        out.PutField(idnt);
      ELSE
        out.GetField(idnt);
        out.Code(Asm.opc_ldarg_1);
        out.GetField(idnt);
        WITH fTyp : Ty.Array DO
            t.RefArrCopy(fTyp);
        | fTyp : Ty.Record DO
            t.RefRecCopy(fTyp);
        END;
      END;
    END;
    out.Code(Asm.opc_ret);
    out.CopyTail;
  END CopyProc;

(* ============================================================ *)

  PROCEDURE (this : MsilEmitter)
      EmitMethod(out : Mu.MsilFile; method : Id.MthId),NEW;
    VAR mthSet : SET;
        attSet : SET;
  BEGIN
    mthSet := method.mthAtt * Id.mask;
   (*
    *  Get the extension bits:
    *     {} == att_final ==> inextensible, ie. final AND virtual
    *    {1} == att_isAbs ==> abstract AND virtual
    *    {2} == att_empty ==> empty, and thus virtual
    *  {1,2} == att_extns ==> extensible, thus virtual
    *)
    IF mthSet = {} THEN
      IF Id.newBit IN method.mthAtt THEN
        attSet :=  Asm.att_instance;
      ELSE
        attSet :=  Asm.att_final + Asm.att_virtual;
      END;
    ELSIF mthSet = Id.isAbs THEN
      attSet := Asm.att_virtual + Asm.att_abstract;
      IF Id.newBit IN method.mthAtt THEN
          attSet := attSet + Asm.att_newslot END;
    ELSE
      attSet := Asm.att_virtual;
      IF Id.newBit IN method.mthAtt THEN
          attSet := attSet + Asm.att_newslot END;
    END;
    IF Id.widen IN method.mthAtt THEN attSet := attSet + Asm.att_public END;
    this.EmitProc(method, attSet)
  END EmitMethod;

(* ============================================================ *)

  PROCEDURE (this : MsilEmitter)
                         EmitRecBody(out : Mu.MsilFile; typId : Id.TypId),NEW;
  (** Create the assembler for a class file for this record. *)
    VAR index  : INTEGER;
        ident  : Sy.Idnt;
        baseT  : Sy.Type;
        field  : Id.FldId;
        method : Id.MthId;
        attSet : SET;
        clsSet : SET;
        mthSet : SET;
        record : Ty.Record;
        valRec : BOOLEAN;
        mkInit : BOOLEAN;
        mkCopy : BOOLEAN;
        boxMth : Sy.IdSeq;

  BEGIN
    out.Blank();
    record := typId.type.boundRecTp()(Ty.Record);
    mkInit := Sy.clsTp IN record.xAttr;
    mkCopy := ~(Sy.noCpy IN record.xAttr);
    valRec := ~(Sy.clsTp IN record.xAttr);
   (*
    *   Account for the record attributes.
    *)
    CASE record.recAtt OF
    | Ty.noAtt : attSet := Asm.att_sealed;
    | Ty.isAbs : attSet := Asm.att_abstract;
    | Ty.cmpnd : attSet := Asm.att_abstract;
    | Ty.limit : attSet := Asm.att_empty;
    | Ty.extns : attSet := Asm.att_empty;
    | Ty.iFace : attSet := Asm.att_interface;
     mkInit := FALSE;
     mkCopy := FALSE;
    END;
   (*
    *   Account for the identifier visibility.
    *   It appears that the VOS only supports two kinds:
    *   "public" == exported from this assembly
    *   <empty>  == not exported from this assembly
    *   Note that private is enforced by the name mangling
    *   for types that are local to a procedure.
    *)
    IF typId.vMod = Sy.pubMode THEN
      attSet := attSet + Asm.att_public;
    END;
    clsSet := attSet;
    IF valRec THEN attSet := attSet + Asm.att_value END;
    out.Comment("RECORD " + record.name()^);
   (*
    *   Emit header with optional super class attribute.
    *)
    out.ClassHead(attSet, record, record.superType());
   (*
    *  List the interfaces, if any.
    *)
    IF record.interfaces.tide > 0 THEN
      out.MarkInterfaces(record.interfaces);
    END;
    out.OpenBrace(2);
   (*
    *  Emit all the fields ...
    *)
    FOR index := 0 TO record.fields.tide-1 DO
      ident := record.fields.a[index];
      field := ident(Id.FldId);
      out.EmitField(field, fieldAttr(field, Asm.att_empty));
    END;
   (*
    *  Emit any constructors.
    *)
    IF mkInit THEN this.MakeInit(record, NIL) END;
    FOR index := 0 TO record.statics.tide-1 DO
      ident  := record.statics.a[index];
      this.MakeInit(record, ident(Id.PrcId));
    END;
    IF mkCopy THEN this.CopyProc(record) END;
   (*
    *  Emit all the (non-forward) methods ...
    *)
    FOR index := 0 TO record.methods.tide-1 DO
      ident  := record.methods.a[index];
      method := ident(Id.MthId);
      IF (method.kind = Id.conMth) THEN
        IF valRec & (method.rcvFrm.type IS Ty.Pointer) THEN
          Sy.AppendIdnt(boxMth, method);
        ELSE
          this.EmitMethod(out, method);
        END;
      END;
    END;
    FOR index := 0 TO record.events.tide-1 DO
      out.EmitEventMethods(record.events.a[index](Id.AbVar));
    END;
    out.CloseBrace(2);
    out.ClassTail();
    IF valRec THEN (* emit boxed class type *)
      out.StartBoxClass(record, clsSet, this.mod);
      FOR index := 0 TO boxMth.tide-1 DO
        ident  := boxMth.a[index];
        method := ident(Id.MthId);
        this.EmitMethod(out, method);
      END;
      out.CloseBrace(2);
      out.ClassTail();
    END;
  END EmitRecBody;

(* ============================================================ *)

  PROCEDURE (this : MsilEmitter)EmitModBody(out : Mu.MsilFile; 
                                            mod : Id.BlkId),NEW;
  (** Create the assembler for a class file for this module. *)
    VAR index : INTEGER;
        proc  : Id.Procs;
        recT  : Sy.Type;
        varId : Sy.Idnt;
        cfLive : BOOLEAN; (* Control Flow is (still) live *)
        threadDummy : Sy.Stmt;
        threadField : Sy.Idnt;
  BEGIN
    out.MkBodyClass(mod);

    threadDummy := NIL; (* to avoid warning *)
    IF Sy.sta IN this.mod.xAttr THEN
      this.AddStaMembers();
      threadDummy := this.mkThreadAssign(); 
    END;

    out.OpenBrace(2);
    FOR index := 0 TO this.mod.procs.tide-1 DO
     (*
      *  Create the mangled name for all procedures 
      *  (including static and type-bound methods).
      *)
      proc := this.mod.procs.a[index];
      Mu.MkProcName(proc, out);
      Mu.RenumberLocals(proc, out);
    END;
   (*
    *  Emit all of the static fields
    *)
    FOR index := 0 TO this.mod.locals.tide-1 DO
      varId := this.mod.locals.a[index];
      out.EmitField(varId(Id.VarId), fieldAttr(varId, Asm.att_static));
    END;
   (*
    *  Emit all of the static event methods
    *)
    FOR index := 0 TO this.mod.locals.tide-1 DO
      varId := this.mod.locals.a[index];
      IF varId.type IS Ty.Event THEN out.EmitEventMethods(varId(Id.AbVar)) END;
    END;
   (*
    *  No constructor for the module "class",
    *  there are never any instances created.
    *)
    asmExe := this.mod.main;    (* Boolean flag for assembler *)
    IF asmExe THEN
     (*
      *   Emit '<clinit>' with variable initialization
      *)
      out.Blank();
      out.MkNewProcInfo(this.mod);
      out.ClinitHead();
      out.InitVars(this.mod);
      out.Code(Asm.opc_ret);
      out.ClinitTail();
      out.Blank();
     (*
      *   Emit module body as 'CPmain() or WinMain'
      *)
      out.MkNewProcInfo(this.mod);
      out.MainHead(this.mod.xAttr);
      IF Sy.sta IN this.mod.xAttr THEN
        out.Comment("Real entry point for STA");
        this.EmitStat(threadDummy, cfLive);
      ELSE
        this.EmitStat(this.mod.modBody, cfLive);
      END;
      IF cfLive THEN
        out.Comment("Continuing directly to CLOSE");
        this.EmitStat(this.mod.modClose, cfLive);
        (* Sequence point for the implicit RETURN *)
        out.LineSpan(Scn.mkSpanT(this.mod.endTok));       
        IF cfLive THEN out.Code(Asm.opc_ret) END;
      END;
      out.MainTail();
    ELSE
     (*
      *   Emit single <clinit> incorporating module body
      *)
      out.MkNewProcInfo(this.mod);
      out.ClinitHead();
      out.InitVars(this.mod);
      this.EmitStat(this.mod.modBody, cfLive);
      IF cfLive THEN out.Code(Asm.opc_ret) END;
      out.ClinitTail();
    END;
   (*
    *  Emit all of the static procedures
    *)
    out.Blank();
    FOR index := 0 TO this.mod.procs.tide-1 DO
      proc := this.mod.procs.a[index];
      IF (proc.kind = Id.conPrc) &
         (proc.dfScp.kind = Id.modId) THEN this.EmitProc(proc, staticAtt) END;
    END;
   (*
    *  And now, just in case exported types that
    *  have class representation have been missed ...
    *)
    FOR index := 0 TO this.mod.expRecs.tide-1 DO
      recT := this.mod.expRecs.a[index];
      IF recT.xName = NIL THEN Mu.MkTypeName(recT, out) END;
    END;
    out.CloseBrace(2);
    out.ClassTail();
  END EmitModBody;

(* ============================================================ *)
(*  Mainline emitter, consumes worklist emitting assembler      *)
(*  files until the worklist is empty.                          *)
(* ============================================================ *)

  PROCEDURE (this : MsilEmitter)MakeAbsName(),NEW;
    VAR nPtr : POINTER TO ARRAY OF CHAR;
        dPtr : POINTER TO ARRAY OF CHAR;
  BEGIN
    IF this.mod.main THEN 
      nPtr := BOX(this.mod.pkgNm$ + ".EXE");
    ELSE
      nPtr := BOX(this.mod.pkgNm$ + ".DLL");
    END;
    IF CSt.binDir # "" THEN
      dPtr := BOX(CSt.binDir$); 
      IF dPtr[LEN(dPtr) - 2] = GPFiles.fileSep THEN
        nPtr := BOX(dPtr^ + nPtr^);
      ELSE
        nPtr := BOX(dPtr^ + "\" + nPtr^);
      END;
    END;
    CSt.outNam := nPtr;
  END MakeAbsName;
  
  PROCEDURE (this : MsilEmitter)Emit*();
  (** Create the file-state structure for this output
      module: overrides EMPTY method in ClassMaker  *)
    VAR out       : Mu.MsilFile; 
        classIx   : INTEGER;
        idDesc    : Sy.Idnt;
        impElem   : Id.BlkId;
        callApi   : BOOLEAN;
  BEGIN
(*
 *  callApi := CSt.doCode & ~CSt.debug;
 *)
    callApi := CSt.doCode & ~CSt.doIlasm;
    Mu.MkBlkName(this.mod);
    IF callApi THEN
      out := PeUtil.newPeFile(this.mod.pkgNm, ~this.mod.main);
      this.outF := out;
    ELSE (* just produce a textual IL file *)
      out := IlasmUtil.newIlasmFile(this.mod.pkgNm);
      this.outF := out;
    END;

    IF ~out.fileOk() THEN
      Scn.SemError.Report(177, 0, 0);
      Error.WriteString("Cannot create out-file <" + out.outN^ + ">");
      Error.WriteLn;
      RETURN;
    END;
    IF CSt.verbose THEN CSt.Message("Created "+ out.outN^) END;
    out.Header(CSt.srcNam);
    IF this.mod.main THEN out.Comment("This module implements CPmain") END;
    out.Blank();
(*
 *  out.AsmDef(this.mod.pkgNm); (* Define this assembly  *)
 *)
    out.RefRTS();               (* Reference runtime asm *)
    out.ExternList();           (* Reference import list *)
    out.AsmDef(this.mod.pkgNm); (* Define this assembly  *)
    out.Blank();
    out.SubSys(this.mod.xAttr);

    IF Sy.wMain IN this.mod.xAttr THEN
      out.Comment("WinMain entry");
    ELSIF Sy.cMain IN this.mod.xAttr THEN
      out.Comment("CPmain entry");
    END;
    IF Sy.sta IN this.mod.xAttr THEN
      out.Comment("Single Thread Apartment");
    END;

    IF LEN(this.mod.xName$) # 0 THEN
      out.StartNamespace(this.mod.xName);
      out.OpenBrace(0);
    ELSE
      out.Comment("No Namespace");
    END;
    classIx := 0;
   (*
    *  Emit all classes on worklist until empty.
    *)
    WHILE classIx < this.work.tide DO
      idDesc := this.work.a[classIx];
      WITH idDesc : Id.BlkId DO
          this.EmitModBody(out, idDesc);
      | idDesc : Id.TypId DO
          IF idDesc.type IS Ty.Procedure THEN
            out.EmitPTypeBody(idDesc);
          ELSE
            this.EmitRecBody(out, idDesc);
          END;
      END;
      INC(classIx);
    END;
    IF callApi THEN
      out.Finish();
      IF ~CSt.quiet THEN CSt.Message("Emitted "+ out.outN^) END;
    ELSE (* just produce a textual IL file *)
      out.Blank();
      IF LEN(this.mod.xName$) # 0 THEN
        out.CloseBrace(0);
        out.Comment("end namespace " + this.mod.xName^);
      END;
      out.Comment("end output produced by gpcp");
      out.Finish();
     (*
      *   Store the filename for the assembler.
      *)
      asmName := this.mod.pkgNm;
    END;
    (* Set the output name for MSBuild *)
    this.MakeAbsName();
  END Emit;

(* ============================================================ *)
(*    Shared code-emission methods      *)
(* ============================================================ *)

  PROCEDURE (e : MsilEmitter)EmitProc(proc : Id.Procs; attr : SET),NEW;
    VAR out  : Mu.MsilFile;
        live : BOOLEAN;
        retn : Sy.Type;
        indx : INTEGER;
        nest : Id.Procs;
  BEGIN
   (*
    *   Recursively emit nested procedures first.
    *)
    FOR indx := 0 TO proc.nestPs.tide-1 DO
      nest := proc.nestPs.a[indx];
      IF nest.kind = Id.conPrc THEN e.EmitProc(nest, staticAtt) END;
    END;
    out := e.outF;
    out.MkNewProcInfo(proc);
    out.Blank();
    out.Comment("PROCEDURE " + proc.prcNm^);
   (*
    *  Compute the method attributes
    *)
    IF proc.vMod = Sy.pubMode THEN (* explicitly public *)
      attr := attr + Asm.att_public;
    ELSIF proc.dfScp IS Id.Procs THEN (* nested procedure  *)
      attr := attr + Asm.att_private;
    ELSIF Asm.att_public * attr = {} THEN 
     (* 
      *  method visiblibity could have been widened 
      *  to match the demanded semantics of the CLR.
      *)
      attr := attr + Asm.att_assembly;
    END;
    out.MethodDecl(attr, proc);
   (*
    *  Output the body if not ABSTRACT
    *)
    IF attr * Asm.att_abstract = {} THEN
      out.OpenBrace(4);
      out.LineSpan(Scn.mkSpanT(proc.token));
      out.Code(Asm.opc_nop);
     (*
      *  Initialize any locals which need this.
      *)
      out.InitVars(proc);
      IF proc.rescue # NIL THEN out.Try END;
     (*
      *  Finally! Emit the method body.
      *)
      e.EmitStat(proc.body, live);
     (*
      *  For proper procedure which reach the fall-
      *  through ending just return.
      *)
      IF live & proc.type.isProperProcType() THEN
        out.LineSpan(proc.endSpan);
        out.DoReturn;
      END;
      IF proc.rescue # NIL THEN
        out.Catch(proc);
        e.EmitStat(proc.rescue, live);
        IF live & proc.type.isProperProcType() THEN
          out.LineSpan(proc.endSpan);
          out.DoReturn;
        END;
        out.EndCatch;
      END;
      out.MethodTail(proc);
    END;
  END EmitProc;

(* ============================================================ *)
(*        Expression Handling Methods     *)
(* ============================================================ *)

  PROCEDURE longValue(lit : Sy.Expr) : LONGINT;
  BEGIN
    RETURN lit(Xp.LeafX).value.long();
  END longValue;

  PROCEDURE intValue(lit : Sy.Expr) : INTEGER;
  BEGIN
    RETURN lit(Xp.LeafX).value.int();
  END intValue;

  PROCEDURE isStrExp(exp : Sy.Expr) : BOOLEAN;
  BEGIN
    RETURN (exp.type = Bi.strTp) & 
           (exp.kind # Xp.mkStr) OR 
            exp.type.isNativeStr();
  END isStrExp;

  PROCEDURE isNilExp(exp : Sy.Expr) : BOOLEAN;
  BEGIN
    RETURN exp.kind = Xp.nilLt;
  END isNilExp;

(* ============================================================ *)

  PROCEDURE (e : MsilEmitter)PushSetCmp(lOp,rOp : Sy.Expr;
                                        theTest : INTEGER),NEW;
    VAR out  : Mu.MsilFile;
        l,r  : INTEGER;
  BEGIN
    out  := e.outF;
    e.PushValue(lOp, Bi.setTp);
    CASE theTest OF
    (* ---------------------------------- *)
    | Xp.equal, Xp.notEq : 
        e.PushValue(rOp, Bi.setTp);
        out.Code(Asm.opc_ceq);
        IF theTest = Xp.notEq THEN
          out.Code(Asm.opc_ldc_i4_1);
          out.Code(Asm.opc_xor);
        END;
    (* ---------------------------------- *)
    | Xp.greEq, Xp.lessEq : 
       (*
        *   The semantics are implemented by the identities
        *
        *   (L <= R) == (L AND R = L)
        *   (L >= R) == (L OR  R = L)
        *)
        out.Code(Asm.opc_dup);
        e.PushValue(rOp, Bi.setTp);
        IF theTest = Xp.greEq THEN
          out.Code(Asm.opc_or);
        ELSE
          out.Code(Asm.opc_and);
        END;
        out.Code(Asm.opc_ceq);
    (* ---------------------------------- *)
    | Xp.greT, Xp.lessT : 
        l := out.proc.newLocal(Bi.setTp);
        r := out.proc.newLocal(Bi.setTp);
       (*
        *   The semantics are implemented by the identities
        *
        *   (L < R) == (L AND R = L) AND NOT (L = R)
        *   (L > R) == (L OR  R = L) AND NOT (L = R)
        *)
        out.Code(Asm.opc_dup);            (* ... L,L       *)
        out.Code(Asm.opc_dup);            (* ... L,L,L     *)
        out.StoreLocal(l);                (* ... L,L,      *)
        e.PushValue(rOp, Bi.setTp);       (* ... L,L,R     *)
        out.Code(Asm.opc_dup);            (* ... L,L,R,R   *)
        out.StoreLocal(r);                (* ... L,L,R     *)
        IF theTest = Xp.greT THEN        
          out.Code(Asm.opc_or);           (* ... L,LvR     *)
        ELSE
          out.Code(Asm.opc_and);          (* ... L,L^R     *)
        END; 
        out.Code(Asm.opc_ceq);            (* ... L@R       *)
        out.PushLocal(l);                 (* ... L@R,l     *)
        out.PushLocal(r);                 (* ... L@R,l,r   *)
        out.Code(Asm.opc_ceq);            (* ... L@R,l=r   *)
        out.Code(Asm.opc_ldc_i4_1);       (* ... L@R,l=r,1 *)
        out.Code(Asm.opc_xor);            (* ... L@R,l#r   *)
        out.Code(Asm.opc_and);            (* ... result    *)
        out.proc.ReleaseLocal(r);
        out.proc.ReleaseLocal(l);
    END;
  END PushSetCmp;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)DoCmp(cmpE : INTEGER;
                                   tLab : Mu.Label;
                                   type : Sy.Type),NEW;
   (**  Compare two TOS elems and jump to tLab if true. *)
   (* ------------------------------------------------- *)
    VAR out  : Mu.MsilFile;
   (* ------------------------------------------------- *)
    PROCEDURE test(t : INTEGER; r : BOOLEAN) : INTEGER;
    BEGIN
      CASE t OF
      | Xp.equal  : RETURN Asm.opc_beq;
      | Xp.notEq  : RETURN Asm.opc_bne_un;
      | Xp.greT   : RETURN Asm.opc_bgt;
      | Xp.lessT  : RETURN Asm.opc_blt;
      | Xp.greEq  : IF r THEN RETURN Asm.opc_bge_un ELSE RETURN Asm.opc_bge END;
      | Xp.lessEq : IF r THEN RETURN Asm.opc_ble_un ELSE RETURN Asm.opc_ble END;
      END;
    END test;
   (* ------------------------------------------------- *)
  BEGIN
    out  := e.outF;
    IF (type IS Ty.Base) &
       ((type = Bi.strTp) OR (type = Bi.sStrTp)) OR
       ~(type IS Ty.Base) & type.isCharArrayType() THEN
     (*
      *  For strings and character arrays, we simply
      *  call the compare function, then compare the
      *  result with zero. Instructions are polymorphic.
      *)
      out.StaticCall(Mu.aaStrCmp, -1);
     (*
      *   function will have returned ...
      *   lessT : -1, equal : 0, greT : 1;
      *)
      IF cmpE = Xp.equal THEN
        out.CodeLb(Asm.opc_brfalse, tLab);
      ELSIF cmpE = Xp.notEq THEN
        out.CodeLb(Asm.opc_brtrue, tLab);
      ELSE
        out.PushInt(0);
        out.CodeLb(test(cmpE, FALSE), tLab);
      END;
    ELSE
      out.CodeLb(test(cmpE, type.isRealType()), tLab);
    END;
  END DoCmp;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)BinCmp(exp : Xp.BinaryX;
                                    tst : INTEGER;
                                    rev : BOOLEAN;
                                    lab : Mu.Label),NEW;
    VAR lType : Sy.Type;
  BEGIN
    lType := exp.lKid.type;
    IF lType = Bi.setTp THEN (* partially ordered type *)
      e.PushSetCmp(exp.lKid, exp.rKid, tst);
      IF rev THEN
        e.outF.CodeLb(Asm.opc_brfalse, lab); 
      ELSE
        e.outF.CodeLb(Asm.opc_brtrue, lab); 
      END;
    ELSE  (* totally ordered type *)
      e.PushValue(exp.lKid, lType);
      IF isStrExp(exp.lKid) & ~isNilExp(exp.rKid) THEN
       (*
        *  If this is a string, convert to a character array.
        *)
        e.outF.StaticCall(Mu.vStr2ChO, 0);
        lType := Bi.chrArr;
      END;

      e.PushValue(exp.rKid, exp.rKid.type);
      IF isStrExp(exp.rKid) & ~isNilExp(exp.lKid) THEN
       (*
        *  If this is a string, convert to a character array.
        *)
        e.outF.StaticCall(Mu.vStr2ChO, 0);
      END;
      IF rev THEN
        CASE tst OF
        | Xp.equal  : tst := Xp.notEq;
        | Xp.notEq  : tst := Xp.equal;
        | Xp.greT   : tst := Xp.lessEq;
        | Xp.lessT  : tst := Xp.greEq;
        | Xp.greEq  : tst := Xp.lessT;
        | Xp.lessEq : tst := Xp.greT;
        END;
      END;
      e.DoCmp(tst, lab, lType);
    END;
  END BinCmp;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)PushCmpBool(lOp,rOp : Sy.Expr; tst : INTEGER),NEW;
    VAR lType : Sy.Type;
  (* ------------------------------------- *)
    PROCEDURE test(t : INTEGER; r : BOOLEAN) : INTEGER;
    BEGIN
      CASE t OF
      | Xp.equal  : RETURN Asm.opc_ceq;
      | Xp.notEq  : RETURN Asm.opc_ceq;
      | Xp.lessT  : RETURN Asm.opc_clt;
      | Xp.greT   : RETURN Asm.opc_cgt;
      | Xp.lessEq : IF r THEN RETURN Asm.opc_cgt_un ELSE RETURN Asm.opc_cgt END;
      | Xp.greEq  : IF r THEN RETURN Asm.opc_clt_un ELSE RETURN Asm.opc_clt END;
      END;
    END test;
  (* ------------------------------------- *)
    PROCEDURE MkBool(tst : INTEGER; typ : Sy.Type; out : Mu.MsilFile);
    BEGIN
      IF (typ IS Ty.Base) &
         ((typ = Bi.strTp) OR (typ = Bi.sStrTp)) OR
         ~(typ IS Ty.Base) & typ.isCharArrayType() THEN
        out.StaticCall(Mu.aaStrCmp, -1);
        (*
         *   function will have returned ...
         *   lessT : -1, equal : 0, greT : 1;
         *)
        out.Code(Asm.opc_ldc_i4_0);
      END;
      out.Code(test(tst, typ.isRealType()));
      IF (tst = Xp.lessEq) OR (tst = Xp.greEq) OR (tst = Xp.notEq) THEN
        out.Code(Asm.opc_ldc_i4_1);
        out.Code(Asm.opc_xor);
      END;
    END MkBool;
  (* ------------------------------------- *)
  BEGIN
    IF lOp.isSetExpr() THEN e.PushSetCmp(lOp, rOp, tst); RETURN END;

    lType := lOp.type;
    e.PushValue(lOp, lOp.type);
    IF isStrExp(lOp) & ~isNilExp(rOp) THEN
     (*
      *  If this is a string, convert to a character array.
      *)
      e.outF.StaticCall(Mu.vStr2ChO, 0);
      lType := Bi.chrArr;
    END;

    e.PushValue(rOp, rOp.type);
    IF isStrExp(rOp) & ~isNilExp(lOp) THEN
     (*
      *  If this is a string, convert to a character array.
      *)
      e.outF.StaticCall(Mu.vStr2ChO, 0);
    END;

    MkBool(tst, lType, e.outF);
  END PushCmpBool;

(* ---------------------------------------------------- *)
(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)FallTrue(exp : Sy.Expr; fLb : Mu.Label),NEW;
   (** Evaluate exp, fall through if true, jump to fLab otherwise *)
    VAR binOp : Xp.BinaryX;
        label : Mu.Label;
        out   : Mu.MsilFile;
  BEGIN
    out := e.outF;
    CASE exp.kind OF
    | Xp.tBool :        (* just do nothing *)
    | Xp.fBool :
        out.CodeLb(Asm.opc_br, fLb);
    | Xp.blNot :
        e.FallFalse(exp(Xp.UnaryX).kid, fLb);
    | Xp.greT, Xp.greEq, Xp.notEq, Xp.lessEq, Xp.lessT, Xp.equal :
        e.BinCmp(exp(Xp.BinaryX), exp.kind, TRUE, fLb);
    | Xp.blOr :
        binOp := exp(Xp.BinaryX);
        label := out.newLabel();
        e.FallFalse(binOp.lKid, label);
        e.FallTrue(binOp.rKid, fLb);
        out.DefLab(label);
    | Xp.blAnd :
        binOp := exp(Xp.BinaryX);
        e.FallTrue(binOp.lKid, fLb);
        e.FallTrue(binOp.rKid, fLb);
    | Xp.isOp :
        binOp := exp(Xp.BinaryX);
        e.PushValue(binOp.lKid, binOp.lKid.type);
        out.CodeT(Asm.opc_isinst, binOp.rKid(Xp.IdLeaf).ident.type);
        (* if NIL then FALSE... *)
        out.CodeLb(Asm.opc_brfalse, fLb);
    | Xp.inOp :
        binOp := exp(Xp.BinaryX);
        out.Code(Asm.opc_ldc_i4_1);
        e.PushValue(binOp.lKid, binOp.lKid.type);
        out.Code(Asm.opc_shl);
        out.Code(Asm.opc_dup);
        e.PushValue(binOp.rKid, binOp.rKid.type);
        out.Code(Asm.opc_and);
        out.CodeLb(Asm.opc_bne_un, fLb);
    ELSE (* Xp.fnCll, Xp.qualId, Xp.index, Xp.selct  *)
      e.PushValue(exp, exp.type);   (* boolean variable *)
      out.CodeLb(Asm.opc_brfalse, fLb);
    END;
  END FallTrue;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)FallFalse(exp : Sy.Expr; tLb : Mu.Label),NEW;
   (** Evaluate exp, fall through if false, jump to tLb otherwise *)
    VAR binOp : Xp.BinaryX;
        label : Mu.Label;
        out   : Mu.MsilFile;
  BEGIN
    out := e.outF;
    CASE exp.kind OF
    | Xp.fBool :        (* just do nothing *)
    | Xp.tBool :
        out.CodeLb(Asm.opc_br, tLb);
    | Xp.blNot :
        e.FallTrue(exp(Xp.UnaryX).kid, tLb);
    | Xp.greT, Xp.greEq, Xp.notEq, Xp.lessEq, Xp.lessT, Xp.equal :
        e.BinCmp(exp(Xp.BinaryX), exp.kind, FALSE, tLb);
    | Xp.blOr :
        binOp := exp(Xp.BinaryX);
        e.FallFalse(binOp.lKid, tLb);
        e.FallFalse(binOp.rKid, tLb);
    | Xp.blAnd :
        label := out.newLabel();
        binOp := exp(Xp.BinaryX);
        e.FallTrue(binOp.lKid, label);
        e.FallFalse(binOp.rKid, tLb);
        out.DefLab(label);
    | Xp.isOp :
        binOp := exp(Xp.BinaryX);
        e.PushValue(binOp.lKid, binOp.lKid.type);
        out.CodeT(Asm.opc_isinst, binOp.rKid(Xp.IdLeaf).ident.type);
        (* if non-NIL then TRUE... *)
        out.CodeLb(Asm.opc_brtrue, tLb);
    | Xp.inOp :
        binOp := exp(Xp.BinaryX);
        out.Code(Asm.opc_ldc_i4_1);
        e.PushValue(binOp.lKid, binOp.lKid.type);
        out.Code(Asm.opc_shl);
        out.Code(Asm.opc_dup);
        e.PushValue(binOp.rKid, binOp.rKid.type);
        out.Code(Asm.opc_and);
        out.CodeLb(Asm.opc_beq, tLb);
    ELSE (* Xp.fnCll, Xp.qualId, Xp.index, Xp.selct  *)
      e.PushValue(exp, exp.type);   (* boolean variable *)
      out.CodeLb(Asm.opc_brtrue, tLb);
    END;
  END FallFalse;

(* ============================================================ *)

  PROCEDURE (e : MsilEmitter)PushUnary(exp : Xp.UnaryX; dst : Sy.Type),NEW;
    VAR dNum : INTEGER;
        code : INTEGER;
        labl : Mu.Label;
        out  : Mu.MsilFile;
        ovfl : BOOLEAN;
  (* ------------------------------------- *)
    PROCEDURE MkBox(emt : MsilEmitter; exp : Xp.UnaryX);
      VAR dst : Sy.Type;
          src : Sy.Type;
          out : Mu.MsilFile;
          rcT : Ty.Record;
    BEGIN
      out := emt.outF;
      src := exp.kid.type;
      dst := exp.type(Ty.Pointer).boundTp;
      IF isStrExp(exp.kid) THEN
        emt.PushValue(exp.kid, src);
        out.StaticCall(Mu.vStr2ChO, 0);
      ELSIF exp.kid.kind = Xp.mkStr THEN
        emt.ValueCopy(exp.kid, dst);
      ELSIF Mu.isRefSurrogate(src) THEN
        emt.ValueCopy(exp.kid, dst);
      ELSE 
        rcT := src(Ty.Record);
       (*
        *  We want to know if this is a
        *  foreign value type.  If so it
        *  must be boxed, since there is 
        *  no CP-defined Boxed_Rec type.
        *)
        IF Sy.isFn IN rcT.xAttr THEN
          emt.PushValue(exp.kid, src);
          out.CodeT(Asm.opc_box, src);
        ELSE (* normal case *)
          out.MkNewRecord(rcT); 
          out.Code(Asm.opc_dup);     
          out.GetValA(exp.type(Ty.Pointer));
          emt.PushValue(exp.kid, src);
          out.CodeT(Asm.opc_stobj, dst);
        END;
      END;
    END MkBox;
  (* ------------------------------------- *)
    PROCEDURE MkAdr(emt : MsilEmitter; exp : Sy.Expr);
    BEGIN
      IF Mu.isRefSurrogate(exp.type) THEN
        emt.PushValue(exp, exp.type); 
      ELSE
        emt.PushRef(exp, exp.type); 
      END;
      emt.outF.Code(Asm.opc_conv_i4);
    END MkAdr;
  (* ------------------------------------- *)
  BEGIN
   (* Eliminte special cases first *)
    IF exp.kind = Xp.mkBox THEN MkBox(e,exp); RETURN END; (* PRE-EMPTIVE RET *)
    IF exp.kind = Xp.adrOf THEN MkAdr(e,exp.kid); RETURN END; (* PRE-EMPTIVE *)
   (* Now do the mainstream cases  *)
    e.PushValue(exp.kid, exp.kid.type);
    out := e.outF;
    ovfl := out.proc.prId.ovfChk;
    CASE exp.kind OF
    | Xp.mkStr  : (* skip *)
    | Xp.deref  :
        IF Mu.isValRecord(dst) THEN    (* unbox field 'v$' *)
          out.GetVal(exp.kid.type(Ty.Pointer));
        END;
    | Xp.tCheck :
        IF Mu.isValRecord(exp.type) THEN
          out.CodeT(Asm.opc_unbox, exp.type.boundRecTp()(Ty.Record));
          out.CodeT(Asm.opc_ldobj, exp.type.boundRecTp()(Ty.Record));
        ELSE
          out.CodeT(Asm.opc_castclass, exp.type);
        END;
(*
 *      out.CodeT(Asm.opc_castclass, exp.type.boundRecTp()(Ty.Record));
 *)
    | Xp.mkNStr :
	IF ~isStrExp(exp.kid) THEN out.StaticCall(Mu.chs2Str, 0) END;
    | Xp.strChk :
        out.Code(Asm.opc_dup);
        out.StaticCall(Mu.aStrChk, -1);  (* Do some range checks *)
    | Xp.compl :
        out.Code(Asm.opc_ldc_i4_M1);
        out.Code(Asm.opc_xor);
    | Xp.neg :
        out.Code(Asm.opc_neg);
    | Xp.absVl :
        dNum := dst(Ty.Base).tpOrd;
        IF ~ovfl THEN
         (*
          *  This is the code to use for non-trapping cases
          *)
          out.Code(Asm.opc_dup);
          out.Code(Asm.opc_ldc_i4_0);
          IF    dNum = Ty.realN THEN
            out.Code(Asm.opc_conv_r8);
          ELSIF dNum = Ty.sReaN THEN
            out.Code(Asm.opc_conv_r4);
          ELSIF dNum = Ty.lIntN THEN
            out.Code(Asm.opc_conv_i8);
          (* ELSE do nothing for all INTEGER cases *)
          END;
          labl := out.newLabel();
          out.CodeLb(Asm.opc_bge, labl);
          out.Code(Asm.opc_neg);
          out.DefLab(labl);
        ELSE
         (*
          *  The following is the safe but slow code.
          *)
          IF    dNum = Ty.realN THEN
            out.StaticCall(Mu.dAbs, 0);
          ELSIF dNum = Ty.sReaN THEN
            out.StaticCall(Mu.fAbs, 0);
          ELSIF dNum = Ty.lIntN THEN
            out.StaticCall(Mu.lAbs, 0);
          ELSE
            out.StaticCall(Mu.iAbs, 0);
          END;
        END;
    | Xp.entVl :
        dNum := dst(Ty.Base).tpOrd;
        IF dNum = Ty.sReaN THEN out.Code(Asm.opc_conv_r8) END;
        (*
        // We _could_ check if the value is >= 0.0, and
        // skip the call in that case, falling through
        // into the round-to-zero mode opc_d2l.
        *)
        out.StaticCall(Mu.dFloor, 0);
        IF ~ovfl THEN
          out.Code(Asm.opc_conv_i8);
        ELSE
          out.Code(Asm.opc_conv_ovf_i8);
        END;
    | Xp.capCh :
        out.StaticCall(Mu.toUpper, 0);
    | Xp.blNot :
        out.Code(Asm.opc_ldc_i4_1);
        out.Code(Asm.opc_xor);
    | Xp.strLen :
        out.StaticCall(Mu.aStrLen, 0);
    | Xp.oddTst :
        IF exp.kid.type.isLongType() THEN out.Code(Asm.opc_conv_i4) END;
        out.Code(Asm.opc_ldc_i4_1);
        out.Code(Asm.opc_and);
    | Xp.getTp :
(*
 *    Currently value records cannot arise here, since all TYPEOF()
 *    calls are folded to their statically known type by ExprDesc.
 *    If ever this changes, the following code is needed (and has
 *    been checked against a non-folding version of ExprDesc.
 * 
 *      IF Mu.isValRecord(exp.kid.type) THEN        (* box the value... *)
 *        out.CodeT(Asm.opc_box, exp.kid.type);     (* CodeTn works too *)
 *      END;
 *)
        out.StaticCall(Mu.getTpM, 0);
    END;
  END PushUnary;

(* ============================================================ *)

  PROCEDURE (e : MsilEmitter)
      PushBinary(exp : Xp.BinaryX; dst : Sy.Type),NEW;
    VAR out  : Mu.MsilFile;
        lOp  : Sy.Expr;
        rOp  : Sy.Expr;
        dNum : INTEGER;
        sNum : INTEGER;
        code : INTEGER;
        indx : INTEGER;
        rLit : LONGINT;
        long : BOOLEAN;
        rasd : BOOLEAN;  (* Element type is erased *)
        temp : INTEGER;
        ovfl : BOOLEAN;
        exLb : Mu.Label;
        tpLb : Mu.Label;
        rpTp : Sy.Type;
        elTp : Sy.Type;
  BEGIN
    out := e.outF;
    lOp := exp.lKid;
    rOp := exp.rKid;
    ovfl := out.proc.prId.ovfChk & dst.isIntType();
    CASE exp.kind OF
    (* -------------------------------- *)
    | Xp.index :
        rasd := exp(Xp.BinaryX).lKid.type IS Ty.Vector;
        IF rasd THEN
          rpTp := Mu.vecRepElTp(exp.lKid.type(Ty.Vector));
        ELSE
          (* rpTp := dst; *)
          rpTp := lOp.type(Ty.Array).elemTp;
        END;
        e.PushHandle(exp, rpTp);
        out.GetElem(rpTp);     (* load the element *)
        IF rasd & (dst # rpTp) THEN
          IF Mu.isValRecord(dst) THEN 
            out.CodeT(Asm.opc_unbox, dst);
            out.CodeT(Asm.opc_ldobj, dst);
          ELSIF rpTp = Bi.anyPtr THEN
            out.CodeT(Asm.opc_castclass, dst);
          ELSE
            out.ConvertDn(rpTp, dst);
          END;
        END;
       (*
        * previous code ---
        *
        *   e.PushHandle(exp, dst);
        *   out.GetElem(dst);     (* load the element *)
        *)
    (* -------------------------------- *)
    | Xp.range :        (* set i..j range ... *)
       (* We want to create an integer with bits--  *)
       (*      [0...01...10...0]      *)
       (* MSB==31    j   i    0==LSB      *)
       (* One method is A       *)
       (* 1)   [0..010........0]  1 << (j+1)    *)
       (* 2)   [1..110........0]  negate(1)   *)
       (* 3)   [0.......010...0]  1 << i    *)
       (* 4)   [1.......110...0]  negate(3)   *)
       (* 5)   [0...01...10...0]  (2)xor(4)   *)
       (* Another method is B       *)
       (* 1)   [1.............1]  -1      *)
       (* 2)   [0...01........1]  (1) >>> (31-j)  *)
       (* 3)   [0........01...1]  (2) >> i    *)
       (* 4)   [0...01...10...0]  (3) << i    *)
       (* --------------------------------------------- *
        *      (*         *
        * * Method A        *
        * *)          *
        * out.Code(Asm.opc_ldc_i4_1);   *
        * out.Code(Asm.opc_ldc_i4_1);   *
        * e.PushValue(rOp, Bi.intTp);    *
        *      (* Do unsigned less than 32 test here *) *
        * out.Code(Asm.opc_add);      *
        * out.Code(Asm.opc_shl);      *
        * out.Code(Asm.opc_neg);      *
        * out.Code(Asm.opc_ldc_i4_1);   *
        * e.PushValue(lOp, Bi.intTp);    *
        *      (* Do unsigned less than 32 test here *) *
        * out.Code(Asm.opc_shl);      *
        * out.Code(Asm.opc_neg);      *
        * out.Code(Asm.opc_xor);      *
        * -------------------------------------------- *)
       (*
        * Method B
        *)
        IF rOp.kind = Xp.numLt THEN
          indx := intValue(rOp);
          IF indx = 31 THEN
            out.Code(Asm.opc_ldc_i4_M1);
          ELSE
            temp := ORD({0 .. indx});
            out.PushInt(temp);
          END;
        ELSE
          out.Code(Asm.opc_ldc_i4_M1);
          out.PushInt(31);
          e.PushValue(rOp, Bi.intTp);
         (* Do unsigned less than 32 test here ...*)
          out.Code(Asm.opc_sub);
          out.Code(Asm.opc_shr_un);
        END;
        IF lOp.kind = Xp.numLt THEN
          indx := intValue(lOp);
          IF indx > 0 THEN
            temp := ORD({indx .. 31});
            out.PushInt(temp);
            out.Code(Asm.opc_and);
          END;
        ELSE
          e.PushValue(lOp, Bi.intTp);
         (* Do unsigned less than 32 test here ...*)
          out.Code(Asm.opc_dup);
          temp := out.proc.newLocal(Bi.intTp);
          out.StoreLocal(temp);
          out.Code(Asm.opc_shr);
          out.PushLocal(temp);
          out.Code(Asm.opc_shl);
          out.proc.ReleaseLocal(temp);
        END;
    (* -------------------------------- *)
    | Xp.lenOf :
        e.PushValue(lOp, lOp.type);
        (* conventional arrays here *)
        IF lOp.type IS Ty.Vector THEN
          out.GetField(Mu.vecLeng(out));
        ELSE
          FOR indx := 0 TO intValue(rOp) - 1 DO
            out.Code(Asm.opc_ldc_i4_0);
            out.Code(Asm.opc_ldelem_ref);
          END;
          out.Code(Asm.opc_ldlen);
        END;
    (* -------------------------------- *)
    | Xp.maxOf, Xp.minOf :
        tpLb := out.newLabel();
        exLb := out.newLabel();
       (*
        * Push left operand, duplicate
        * stack is (top) lOp lOp...
        *)
        e.PushValue(lOp, dst);
        out.Code(Asm.opc_dup);
       (*
        * Push right operand, duplicate
        * stack is (top) rOp rOp lOp lOp ...
        *)
        e.PushValue(rOp, dst);
        out.Code(Asm.opc_dup);
       (*
        * Store rOp to temp
        * stack is (top) rOp lOp lOp ...
        *)
        temp := out.proc.newLocal(dst);
        out.StoreLocal(temp);
       (*
        *  Compare two top items and jump
        *  stack is (top) lOp ...
        *)
        IF exp.kind = Xp.maxOf THEN
          e.DoCmp(Xp.greT, exLb, dst);    (* leaving lOp on stack *)
        ELSE
          e.DoCmp(Xp.lessT, exLb, dst);   (* leaving lOp on stack *)
        END;
       (*
        *  Else: discard top item
        *  and push stored rOp instead
        *)
        out.Code(Asm.opc_pop);
        out.PushLocal(temp);
        out.DefLab(exLb);
        out.proc.ReleaseLocal(temp);
    (* -------------------------------- *)
    | Xp.bitAnd :
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        out.Code(Asm.opc_and);
    (* -------------------------------- *)
    | Xp.bitOr :
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        out.Code(Asm.opc_or);
    (* -------------------------------- *)
    | Xp.bitXor :
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        out.Code(Asm.opc_xor);
    (* -------------------------------- *)
    | Xp.plus :
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        IF ovfl THEN out.Code(Asm.opc_add_ovf) ELSE out.Code(Asm.opc_add) END;
    (* -------------------------------- *)
    | Xp.minus :
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        IF ovfl THEN out.Code(Asm.opc_sub_ovf) ELSE out.Code(Asm.opc_sub) END;
    (* -------------------------------- *)
    | Xp.mult :
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        IF ovfl THEN out.Code(Asm.opc_mul_ovf) ELSE out.Code(Asm.opc_mul) END;
    (* -------------------------------- *)
    | Xp.slash :
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        out.Code(Asm.opc_div);
    (* -------------------------------- *)
    | Xp.rem0op :
        dNum := dst(Ty.Base).tpOrd;
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        out.Code(Asm.opc_rem);
    (* -------------------------------- *)
    | Xp.modOp :
        long := dst(Ty.Base).tpOrd = Ty.lIntN;
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        IF (rOp.kind = Xp.numLt) & (intValue(rOp) > 0) THEN
          indx := intValue(rOp);
          tpLb := out.newLabel();
          out.Code(Asm.opc_rem);
          out.Code(Asm.opc_dup);
          IF long THEN out.PushLong(0) ELSE out.PushInt(0) END;
          out.CodeLb(Asm.opc_bge, tpLb);
          IF long THEN out.PushLong(indx) ELSE out.PushInt(indx) END;
          out.Code(Asm.opc_add);
          out.DefLab(tpLb);
        ELSIF long THEN
          out.StaticCall(Mu.CpModL, -1);
        ELSE
          out.StaticCall(Mu.CpModI, -1);
        END;
    (* -------------------------------- *)
    | Xp.div0op :
        dNum := dst(Ty.Base).tpOrd;
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        out.Code(Asm.opc_div);
    (* -------------------------------- *)
    | Xp.divOp :
        long := dst(Ty.Base).tpOrd = Ty.lIntN;
        e.PushValue(lOp, dst);
        IF (rOp.kind = Xp.numLt) & (longValue(rOp) > 0) THEN
          tpLb := out.newLabel();
          out.Code(Asm.opc_dup);
          IF long THEN
            rLit := longValue(rOp);
            out.PushLong(0);
            out.CodeLb(Asm.opc_bge, tpLb);
            out.PushLong(rLit-1);
            out.Code(Asm.opc_sub);
            out.DefLab(tpLb);
            out.PushLong(rLit);
          ELSE
            indx := intValue(rOp);
            out.PushInt(0);
            out.CodeLb(Asm.opc_bge, tpLb);
            out.PushInt(indx-1);
            out.Code(Asm.opc_sub);
            out.DefLab(tpLb);
            out.PushInt(indx);
          END;
          out.Code(Asm.opc_div);
        ELSE
          e.PushValue(rOp, dst);
          IF long THEN
            out.StaticCall(Mu.CpDivL, -1);
          ELSE
            out.StaticCall(Mu.CpDivI, -1);
          END;
        END;
    (* -------------------------------- *)
    | Xp.blOr,  Xp.blAnd :
        tpLb := out.newLabel();
        exLb := out.newLabel();
       (*
        *  Jumping code is mandated for blOr and blAnd...
        *)
        e.FallTrue(exp, tpLb);
        out.Code(Asm.opc_ldc_i4_1);
        out.CodeLb(Asm.opc_br, exLb);
        out.DefLab(tpLb);
        out.Code(Asm.opc_ldc_i4_0);
        out.DefLab(exLb);
    (* -------------------------------- *)
    | Xp.greT,  Xp.greEq, Xp.notEq,
      Xp.lessEq, Xp.lessT, Xp.equal :
        e.PushCmpBool(lOp, rOp, exp.kind);
    (* -------------------------------- *)
    | Xp.inOp :
        e.PushValue(rOp, rOp.type);
        e.PushValue(lOp, lOp.type);
        out.Code(Asm.opc_shr_un);
        out.Code(Asm.opc_ldc_i4_1);
        out.Code(Asm.opc_and);
    (* -------------------------------- *)
    | Xp.isOp :
        e.PushValue(lOp, lOp.type);
        out.CodeT(Asm.opc_isinst, rOp(Xp.IdLeaf).ident.type);
        out.Code(Asm.opc_ldnull);
        out.Code(Asm.opc_cgt_un);
    (* -------------------------------- *)
    | Xp.ashInt :
        e.PushValue(lOp, lOp.type);
        IF rOp.kind = Xp.numLt THEN
          indx := intValue(rOp);
          IF indx = 0 THEN (* skip *)
          ELSIF indx < 0 THEN
            out.PushInt(-indx);
            out.Code(Asm.opc_shr);
          ELSE
            out.PushInt(indx);
            out.Code(Asm.opc_shl);
          END;
        ELSE
          tpLb := out.newLabel();
          exLb := out.newLabel();
         (*
          *  This is a variable shift. Do it the hard way.
          *  First, check the sign of the right hand op.
          *)
          e.PushValue(rOp, rOp.type);
          out.Code(Asm.opc_dup);
          out.PushInt(0);
          out.CodeLb(Asm.opc_blt, tpLb);
         (*
          *  Positive selector ==> shift left;
          *)
          out.Code(Asm.opc_shl);
          out.CodeLb(Asm.opc_br, exLb);
         (*
          *  Negative selector ==> shift right;
          *)
          out.DefLab(tpLb);
          out.Code(Asm.opc_neg);
          out.Code(Asm.opc_shr);
          out.DefLab(exLb);
        END;
    (* -------------------------------- *)
    | Xp.strCat :
        e.PushValue(lOp, lOp.type);
        e.PushValue(rOp, rOp.type);
        IF isStrExp(lOp) THEN
          IF isStrExp(rOp) THEN
            out.StaticCall(Mu.CPJstrCatSS, -1);
          ELSE
            out.StaticCall(Mu.CPJstrCatSA, -1);
          END;
        ELSE
          IF isStrExp(rOp) THEN
            out.StaticCall(Mu.CPJstrCatAS, -1);
          ELSE
            out.StaticCall(Mu.CPJstrCatAA, -1);
          END;
        END;
    (* -------------------------------- *)
    END;
  END PushBinary;

(* ============================================================ *)

  PROCEDURE (e : MsilEmitter)PushValue(exp : Sy.Expr; typ : Sy.Type),NEW;
    VAR out : Mu.MsilFile;
        rec : Ty.Record;
        ix  : INTEGER;
        elm : Sy.Expr;
        emt : BOOLEAN;    (* ==> more than one set element expr *)
  BEGIN
    out := e.outF;
    WITH exp : Xp.IdLeaf DO
        IF exp.isProcLit() THEN
          out.Code(Asm.opc_ldnull);
          out.MkNewProcVal(exp.ident, typ);
        ELSIF exp.kind = Xp.typOf THEN
          out.LoadType(exp.ident);
        ELSE
          out.GetVar(exp.ident);
        END;
    | exp : Xp.SetExp DO
        emt := TRUE;
       (*
        *   Write out the constant part, if there is one.
        *)
        IF exp.value # NIL THEN
          out.PushInt(exp.value.int()); (* const part *)
          emt := FALSE;
        END;
       (*
        *   Write out the element expressions.
        *   taking the union with any part emitted already.
        *)
        FOR ix := 0 TO exp.varSeq.tide-1 DO
          elm := exp.varSeq.a[ix];
          IF elm.kind = Xp.range THEN
            e.PushValue(elm, Bi.intTp);
          ELSE
            out.PushInt(1);
            e.PushValue(exp.varSeq.a[ix], Bi.intTp);
            out.Code(Asm.opc_shl);
          END;
          IF ~emt THEN out.Code(Asm.opc_or) END;
          emt := FALSE;
        END;
       (*
        *   If neither of the above emitted anything, emit zero!
        *)
        IF emt THEN out.Code(Asm.opc_ldc_i4_0) END;
    | exp : Xp.LeafX DO
        CASE exp.kind OF
        | Xp.tBool  : out.Code(Asm.opc_ldc_i4_1);
        | Xp.fBool  : out.Code(Asm.opc_ldc_i4_0);
        | Xp.nilLt  : out.Code(Asm.opc_ldnull);
        | Xp.charLt : out.PushInt(ORD(exp.value.char()));
        | Xp.setLt  : out.PushInt(exp.value.int());
        | Xp.numLt  :
            IF typ = Bi.lIntTp THEN
              out.PushLong(exp.value.long());
            ELSE
              out.PushInt(exp.value.int());
            END;
        | Xp.realLt :
            IF typ = Bi.realTp THEN
              out.PushReal(exp.value.real());
            ELSE
              out.PushSReal(exp.value.real());
            END;
        | Xp.strLt  :
            IF typ = Bi.charTp THEN
              out.PushInt(ORD(exp.value.chr0()));
            ELSE
              out.PushStr(exp.value.chOpen());
            END;
        | Xp.infLt  : 
            IF typ = Bi.realTp THEN
              out.GetVar(CSt.dblInf);
            ELSE
              out.GetVar(CSt.fltInf);
            END;
        | Xp.nInfLt : 
            IF typ = Bi.realTp THEN
              out.GetVar(CSt.dblNInf);
            ELSE
              out.GetVar(CSt.fltNInf);
            END;
        END;
    | exp : Xp.CallX DO
       (* 
        *  EXPERIMENTAL debug marker ...
        *)
        (*out.LinePlus(exp.token.lin, exp.token.col + exp.token.len);*)
        e.PushCall(exp);
    | exp : Xp.IdentX DO
        IF exp.kind = Xp.selct THEN
          IF exp.isProcLit() THEN
            out.Comment("Make event value");
            e.PushValue(exp.kid, exp.kid.type);
            out.MkNewProcVal(exp.ident, typ);
          ELSE
            e.PushHandle(exp, exp.type);
            out.GetField(exp.ident(Id.FldId));
          END;
        ELSE
          e.PushValue(exp.kid, exp.kid.type);
          IF exp.kind = Xp.cvrtUp THEN
            out.ConvertUp(exp.kid.type, typ);
          ELSIF exp.kind = Xp.cvrtDn THEN
            out.ConvertDn(exp.kid.type, typ);
          END;
        END;
    | exp : Xp.UnaryX DO
        e.PushUnary(exp, typ);
    | exp : Xp.BinaryX DO
        e.PushBinary(exp, typ);
    END;
  END PushValue;

(* ---------------------------------------------------- *)
(* ---------------------------------------------------- *
 *
 *PROCEDURE (e : MsilEmitter)PushObjRef(exp : Sy.Expr),NEW;
 *BEGIN
 *  IF Mu.isValRecord(exp.type) THEN
 *    e.PushRef(exp,exp.type);
 *  ELSE
 *    e.PushValue(exp,exp.type);
 *  END;
 *END PushObjRef;
 *
 * ---------------------------------------------------- *)
(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)PushVectorIndex(exp : Xp.BinaryX),NEW;
    VAR out  : Mu.MsilFile;
        tide : INTEGER;
        okLb : Mu.Label;
        vecT : Ty.Vector;
  BEGIN
    out := e.outF;
    okLb := out.newLabel();
    vecT := exp.lKid.type(Ty.Vector);
    tide := out.proc.newLocal(Bi.intTp);
    out.Code(Asm.opc_dup);                (* ... vec, vec           *)
    out.GetField(Mu.vecLeng(out));        (* ... vec, len           *)
    out.StoreLocal(tide);                 (* ... vec                *)
    out.GetField(Mu.vecArrFld(vecT,out)); (* ... aRf                *)
    e.PushValue(exp.rKid, Bi.intTp);      (* ... aRf, idx           *)
    out.Code(Asm.opc_dup);                (* ... vec, idx, idx      *)
    out.PushLocal(tide);                  (* ... vec, idx, idx, len *)
    out.CodeLb(Asm.opc_blt, okLb);        (* ... vec, idx           *)
    out.IndexTrap();
    out.DefLab(okLb);
    out.proc.ReleaseLocal(tide);
  END PushVectorIndex;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)PushHandle(exp : Sy.Expr; typ : Sy.Type),NEW;
   (* Pre:  exp must be a variable designator     *)
   (* Post: Reference to containing object is on TOS. *)
    VAR idnt : Sy.Idnt;
        out  : Mu.MsilFile;
        cTmp : INTEGER;
        kid  : Sy.Expr;
  BEGIN
    out := e.outF;
    ASSERT(exp.isVarDesig());
    WITH exp : Xp.IdentX DO
        kid := exp.kid;
        IF Mu.isValRecord(kid.type) THEN
          e.PushRef(kid, kid.type);
        ELSE
          e.PushValue(kid, kid.type);
        END;
    | exp : Xp.BinaryX DO
        e.PushValue(exp.lKid, exp.lKid.type);
        IF exp.lKid.isVectorExpr() THEN
          e.PushVectorIndex(exp);
        ELSE
          e.PushValue(exp.rKid, Bi.intTp);
          IF Mu.isValRecord(typ) THEN out.CodeTn(Asm.opc_ldelema, typ) END;
        END;
    | exp : Xp.IdLeaf DO
        IF Mu.isRefSurrogate(typ) THEN
          e.PushValue(exp, typ);
        ELSE
          idnt := exp.ident;
          WITH idnt : Id.LocId DO
            IF Id.uplevA IN idnt.locAtt THEN
              out.XhrHandle(idnt);
            ELSE
              WITH idnt : Id.ParId DO
                IF idnt.boxOrd # Sy.val THEN out.PushArg(idnt.varOrd) END;
              ELSE (* skip *)
              END;
            END;
          ELSE (* skip *)
          END;
        END;
    | exp : Xp.UnaryX DO
        ASSERT(exp.kind = Xp.deref);
        e.PushValue(exp.kid, exp.kid.type);
        IF Mu.isValRecord(typ) THEN    (* get adr of boxed field *)
          out.GetValA(exp.kid.type(Ty.Pointer));
        END;
      (*
       *  ELSE
       *    e.PushValue(exp, typ);
       *)
    END;
  END PushHandle;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)PushRef(exp : Sy.Expr; typ : Sy.Type),NEW;
    VAR out : Mu.MsilFile;
        sav : INTEGER;
  BEGIN
    out := e.outF;
    WITH exp : Xp.IdLeaf DO (* A scalar variable *)
        out.GetVarA(exp.ident);
    | exp : Xp.IdentX DO  (* A field reference *)
        e.PushHandle(exp, typ);
        out.GetFieldAdr(exp.ident(Id.FldId));
    | exp : Xp.BinaryX DO (* An array element  *)
        e.PushValue(exp.lKid, exp.lKid.type);
(*
 *      e.PushValue(exp.rKid, Bi.intTp);
 *      out.CodeTn(Asm.opc_ldelema, typ);
 *)
        IF exp.lKid.isVectorExpr() THEN
          e.PushVectorIndex(exp);
          IF Mu.isValRecord(typ) THEN
            out.Code(Asm.opc_ldelem_ref);  (* ???? *)
            out.CodeT(Asm.opc_unbox, typ);
          ELSE
            out.CodeTn(Asm.opc_ldelema, typ);  (* ???? *)
          END;
        ELSE
          e.PushValue(exp.rKid, Bi.intTp);
          out.CodeTn(Asm.opc_ldelema, typ);  (* ???? *)
        END;

    | exp : Xp.CallX DO 
       (* 
        *   This case occurs where a (foreign) method is
        *   bound to a value class.  In CP, there would
        *   be a corresponding boxed type instead.
        *)
        sav := out.proc.newLocal(typ);
        e.PushValue(exp, typ);
        out.StoreLocal(sav);		(* Store in new local variable *)
	out.PushLocalA(sav);            (* Now take address of local   *)
        out.proc.ReleaseLocal(sav);
    | exp : Xp.UnaryX DO  (* Dereference node  *)
       (* 
        *  It is not always the case that typ and exp.type
        *  denote the same type.  In one usage exp is an
        *  actual argument, and typ is the type of the formal.
        *)
        e.PushValue(exp.kid, exp.kid.type);
        IF exp.kind = Xp.deref THEN
          IF Mu.isValRecord(typ) THEN
            out.GetValA(exp.kid.type(Ty.Pointer));
          END;
        ELSE 
          ASSERT(exp.kind = Xp.tCheck);
          IF Mu.isValRecord(typ) THEN 
            out.CodeT(Asm.opc_unbox, exp.type);
          END;
        END;
        (* e.PushHandle(exp, typ); *)
    ELSE (* skip *)
    END;
  END PushRef;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)ValueAssign(exp : Sy.Expr),NEW;
    VAR out : Mu.MsilFile;
  BEGIN
    out := e.outF;
    WITH exp : Xp.IdLeaf DO
        (* stack has ... value, (top) *)
        out.PutVar(exp.ident);
    | exp : Xp.IdentX DO
        (* stack has ... obj-ref, value, (top)  *)
        out.PutField(exp.ident(Id.FldId));
    | exp : Xp.BinaryX DO
       (*
        *  IF element type is a value-rec,
        *  THEN Stack: ... elem-adr, value, (top)
        *  ELSE  ... arr-ref, index, value, (top)
        *)
        out.PutElem(exp.type);
    | exp : Xp.UnaryX DO
       (*
        *  This is a deref of a value record
        *  and Stack: ... handle, value, (top)
        *)
        out.CodeT(Asm.opc_stobj, exp.type);
    ELSE
      Console.WriteString("BAD VALUE ASSIGN"); Console.WriteLn;
      exp.Diagnose(0);
      ASSERT(FALSE);
    END;
  END ValueAssign;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)EraseAndAssign(eT : Sy.Type; vT : Ty.Vector),NEW;
    VAR out : Mu.MsilFile;
        rT  : Sy.Type;   (* CLR representation elem type  *)
  BEGIN
    out := e.outF;

    rT := Mu.vecRepElTp(vT);

    IF eT # rT THEN  (* value of elTp is sitting TOS, vector needs rpTp *)
     (*
      *  For the gpcp-1.2.x design all rpTp uses of 
      *  int32, char use default conversions. All 
      *  other base types represent themselves
      *)
      IF Mu.isValRecord(eT) THEN out.CodeT(Asm.opc_box, eT) END;
    END;
    out.PutElem(rT);
  END EraseAndAssign;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)RefRecCopy(typ : Ty.Record),NEW;
    VAR nam : Lv.CharOpen;
  BEGIN
   (*
    *  We should use a builtin here for value type, but
    *  seem to need an element by element copy for classes.
    *
    *     Stack at entry is (top) srcRef, dstRef...
    *)
    IF typ.xName = NIL THEN Mu.MkRecName(typ, e.outF) END;
    e.outF.CopyCall(typ);
  END RefRecCopy;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)RefArrCopy(typ : Ty.Array),NEW;
    VAR out   : Mu.MsilFile;
        count : INTEGER;
        cardN : INTEGER;
        dstLc : INTEGER;
        srcLc : INTEGER;
        sTemp : INTEGER;
        elTyp : Sy.Type;
        label : Mu.Label;
  BEGIN
    elTyp := typ.elemTp;
   (*
    *    Stack at entry is (top) srcRef, dstRef...
    *)
    out := e.outF;
    label := out.newLabel();
   (*
    *   Allocate two local variables.
    *)
    dstLc := out.proc.newLocal(typ);
    srcLc := out.proc.newLocal(typ);
   (*
    *   Initialize the two locals.
    *)
    out.StoreLocal(srcLc);
    out.StoreLocal(dstLc);      (* Stack is now empty.  *)
    cardN := typ.length;
   (*
    *   Compute the length, either now or at runtime...
    *)
    IF  (cardN > 0) &                 (* ... not open array  *)
        (cardN <= inlineLimit) &      (* ... not too long    *)
        Mu.hasValueRep(elTyp) THEN    (* ... outer dimension *)
     (*
      *   Do inline <element assign> using a compile-time loop
      *)
      FOR count := 0 TO cardN-1 DO
        out.PushLocal(dstLc);
        out.PushInt(count);
        IF Mu.isValRecord(elTyp) THEN
          out.CodeTn(Asm.opc_ldelema, elTyp);
          out.PushLocal(srcLc);
          out.PushInt(count);
          out.CodeTn(Asm.opc_ldelema, elTyp);
          out.CodeT(Asm.opc_ldobj, elTyp);
          out.CodeT(Asm.opc_stobj, elTyp);
        ELSE
          IF (elTyp IS Ty.Array) OR
             (elTyp IS Ty.Record) THEN out.GetElem(elTyp) END;
          out.PushLocal(srcLc);
          out.PushInt(count);
          out.GetElem(elTyp);
          WITH elTyp : Ty.Record DO
              e.RefRecCopy(elTyp);
          | elTyp : Ty.Array DO
              e.RefArrCopy(elTyp);
          ELSE  (* scalar element type *)
            out.PutElem(elTyp);
          END;
        END;
      END;
    ELSE (* Do array copy using a runtime loop *)
      IF cardN = 0 THEN (* open array, get length from source desc *)
        out.PushLocal(srcLc);
        out.Code(Asm.opc_ldlen);
      ELSE
        out.PushInt(cardN);
      END;
     (*
      *   Allocate an extra local variable
      *)
      count := out.proc.newLocal(Bi.intTp);
      out.StoreLocal(count);
      out.DefLab(label);      (* The back-edge target *)
     (*
      *   Decrement the loop count.
      *)
      out.DecTemp(count);       (* Stack is now empty.  *)
     (*
      *   We now do the one-per-loop <element assign>
      *)
      out.PushLocal(dstLc);
      out.PushLocal(count);
      IF Mu.isValRecord(elTyp) THEN
        out.CodeTn(Asm.opc_ldelema, elTyp);
        out.PushLocal(srcLc);
        out.PushLocal(count);
        out.CodeTn(Asm.opc_ldelema, elTyp);
        out.CodeT(Asm.opc_ldobj, elTyp);
        out.CodeT(Asm.opc_stobj, elTyp);
      ELSE
        IF (elTyp IS Ty.Array) OR
           (elTyp IS Ty.Record) THEN out.GetElem(elTyp) END;
        out.PushLocal(srcLc);
        out.PushLocal(count);
        out.GetElem(elTyp);
        WITH elTyp : Ty.Record DO
            e.RefRecCopy(elTyp);
        | elTyp : Ty.Array DO
            e.RefArrCopy(elTyp);
        ELSE  (* scalar element type *)
          out.PutElem(elTyp);
        END;
      END;
     (*
      *   Loop back to label if count non-zero.
      *)
      out.PushLocal(count);
      out.CodeLb(Asm.opc_brtrue, label);
     (*
      *   release the extra local.
      *)
      out.proc.ReleaseLocal(count);
    END;
   (*
    *   ... and release the two locals.
    *)
    out.proc.ReleaseLocal(srcLc);
    out.proc.ReleaseLocal(dstLc);
  END RefArrCopy;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)ValueCopy(act : Sy.Expr; fmT : Sy.Type),NEW;
   (**  Make a copy of the value of expression act, into a newly  *)
   (*   allocated destination.  Leave dst reference on top of stack.  *)
    VAR out  : Mu.MsilFile;
        dTmp : INTEGER;
        sTmp : INTEGER;
  BEGIN
   (*
    *  Copy this actual, where fmT is either an array or record.
    *)
    out := e.outF;
    WITH fmT : Ty.Record DO
      out.MkNewRecord(fmT);       (* (top) dst...   *)
      out.Code(Asm.opc_dup);      (* (top) dst,dst... *)
      e.PushValue(act, fmT);      (* (top) src,dst,dst... *)
      e.RefRecCopy(fmT);          (* (top) dst...   *)
    | fmT : Ty.Array DO
     (*
      *  Array case: ordinary value copy
      *)
      dTmp := out.proc.newLocal(fmT);   (* Holds dst reference. *)
      sTmp := out.proc.newLocal(fmT);   (* Holds src reference. *)
      IF fmT.length = 0 THEN
       (*
        *   This is the open array destination case.
        *   Compute length of actual parameter and
        *   allocate an array of the needed length.
        *)
        e.PushValue(act, fmT);      (* (top) src...   *)
        out.Code(Asm.opc_dup);      (* (top) src,src... *)
        out.StoreLocal(sTmp);     (* (top) src...   *)

        IF act.kind = Xp.mkStr THEN   (* (top) src...   *)
          out.StaticCall(Mu.aStrLp1, 0); (* (top) len...   *)
          out.CodeTn(Asm.opc_newarr, Bi.charTp); (* (top) dst...   *)
        ELSE          (* (top) src...   *)
          out.MkArrayCopy(fmT);     (* (top) dst...   *)
        END;
        out.Code(Asm.opc_dup);      (* (top) dst,dst... *)
        out.StoreLocal(dTmp);     (* (top) dst...   *)
        out.PushLocal(sTmp);      (* (top) src,dst... *)
      ELSE
       (*
        *   This is the fixed-length destination case.
        *   We allocate an array of the needed length.
        *)
        out.MkFixedArray(fmT);
        out.Code(Asm.opc_dup);      (* (top) dst,dst... *)
        out.StoreLocal(dTmp);     (* (top) dst...   *)
        e.PushValue(act, fmT);      (* (top) src,dst... *)
      END;
      IF act.kind = Xp.mkStr THEN
        out.StaticCall(Mu.aaStrCopy, -2);  (* (top) ...    *)
      ELSE
        e.RefArrCopy(fmT);      (* (top) ...    *)
      END;
      out.PushLocal(dTmp);      (* (top) dst...   *)
      out.proc.ReleaseLocal(dTmp);
      out.proc.ReleaseLocal(sTmp);
    END;
  END ValueCopy;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)StringCopy(act : Sy.Expr; fmT : Ty.Array),NEW;
    VAR out : Mu.MsilFile;
  BEGIN
    out := e.outF;
    IF act.kind = Xp.mkStr THEN
      e.ValueCopy(act, fmT);
    ELSIF fmT.length = 0 THEN     (* str passed to open array   *)
      e.PushValue(act, fmT);
      e.outF.StaticCall(Mu.vStr2ChO, 0);
    ELSE        (* str passed to fixed array  *)
      out.PushInt(fmT.length);
      out.MkOpenArray(Ty.mkArrayOf(Bi.charTp));
      out.Code(Asm.opc_dup);
      e.PushValue(act, fmT);
      e.outF.StaticCall(Mu.vStr2ChF, -2);
    END;
  END StringCopy;

(* ============================================================ *)

  PROCEDURE (e : MsilEmitter)Invoke(exp : Sy.Expr; typ : Ty.Procedure),NEW;
    VAR code : INTEGER;
        prcI : Id.PrcId;
        mthI : Id.MthId;
  BEGIN
    IF exp.isProcVar() THEN
      e.outF.CallDelegate(typ);
    ELSE
      WITH exp : Xp.IdLeaf DO (* qualid *)
          code := Asm.opc_call;
          prcI := exp.ident(Id.PrcId);
          IF prcI.kind # Id.ctorP THEN
            e.outF.CallIT(code, prcI, typ);
          ELSE
            e.outF.CallCT(prcI, typ);
          END;
      | exp : Xp.IdentX DO (* selct *)
          mthI := exp.ident(Id.MthId);
          IF exp.kind = Xp.sprMrk THEN
            code := Asm.opc_call;
          ELSIF mthI.bndType.isInterfaceType() THEN
            code := Asm.opc_callvirt;
          
          ELSIF (mthI.mthAtt * Id.mask = Id.final) OR
                ~mthI.bndType.isExtnRecType() THEN
                (* Non-extensible record types can still have virtual *)
                (* methods (inherited from Object, say). It is a      *)
                (* verify error to callvirt on these. kjg April 2006  *)
            code := Asm.opc_call;
          ELSE
            code := Asm.opc_callvirt;
          END;
          e.outF.CallIT(code, mthI, typ);
          IF Id.covar IN mthI.mthAtt THEN
            e.outF.CodeT(Asm.opc_castclass, typ.retType);
          END;
      END;
    END;
  END Invoke;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)GetArgP(act : Sy.Expr;
                                     frm : Id.ParId),NEW;
    VAR out   : Mu.MsilFile;
  BEGIN
    out := e.outF;
    IF (frm.boxOrd # Sy.val) OR (Id.cpVarP IN frm.locAtt) THEN
      e.PushRef(act, frm.type);
    ELSIF (frm.type IS Ty.Array) &
          ((act.type = Bi.strTp) OR act.type.isNativeStr()) THEN (* a string *)
      e.StringCopy(act, frm.type(Ty.Array));
    ELSIF (frm.parMod = Sy.val) & Mu.isRefSurrogate(frm.type) THEN
      e.ValueCopy(act, frm.type);
    ELSE
      e.PushValue(act, frm.type);
    END;
  END GetArgP;

(* ============================================================ *)
(*    Possible structures of procedure call expressions are:    *)
(* ============================================================ *)
(*          o                               o                   *)
(*         /                               /                    *)
(*      [CallX]                         [CallX]                 *)
(*       / +--- actuals --> ...          / +--- actuals         *)
(*      /                               /                       *)
(*    [IdentX]                      [IdLeaf]                    *)
(*      /  +--- ident ---> [Procs]      +--- ident ---> [Procs] *)
(*     /                                                        *)
(* kid expr                                                     *)
(*                                                              *)
(* ============================================================ *)
(*  only the right hand case can be a standard proc or function *)
(* ============================================================ *)

  PROCEDURE (e : MsilEmitter)PushCall(callX : Xp.CallX),NEW;
    VAR iFile : Mu.MsilFile;
        index : INTEGER;  (* just a counter for loops *)
        formT : Ty.Procedure; (* formal type of procedure *)
        formP : Id.ParId; (* current formal parameter *)
        prExp : Sy.Expr;
        idExp : Xp.IdentX;
        dummy : BOOLEAN;  (* outPar for EmitStat call *)
        prVar : BOOLEAN;  (* ==> expr is a proc-var   *)
 (* ---------------------------------------------------- *)
    PROCEDURE CheckCall(expr : Sy.Expr; os : Mu.MsilFile);
      VAR prcI : Id.PrcId;
          mthI : Id.MthId;
    BEGIN
      WITH expr : Xp.IdLeaf DO (* qualid *)
          prcI := expr.ident(Id.PrcId);
          IF prcI.type.xName = NIL THEN Mu.MkCallAttr(prcI, os) END;
          expr.type := prcI.type;
      | expr : Xp.IdentX DO (* selct *)
          mthI := expr.ident(Id.MthId);
          IF mthI.type.xName = NIL THEN Mu.MkCallAttr(mthI, os) END;
          expr.type := mthI.type;
      END;
    END CheckCall;
 (* ---------------------------------------------------- *)
    PROCEDURE isNested(exp : Sy.Expr) : BOOLEAN;
    BEGIN
      WITH exp : Xp.IdLeaf DO (* qualid *)
        RETURN exp.ident(Id.PrcId).lxDepth > 0;
      ELSE RETURN FALSE;
      END;
    END isNested;
 (* ---------------------------------------------------- *)
  BEGIN
    iFile := e.outF;
    prExp := callX.kid;
    formT := prExp.type(Ty.Procedure);
   (*
    *  Before we push any arguments, we must make
    *  sure that the formal-type name is computed.
    *)
    prVar := prExp.isProcVar();
    IF ~prVar THEN CheckCall(prExp, iFile) END;
    formT := prExp.type(Ty.Procedure);
    IF prVar THEN
      iFile.CommentT("Start delegate call sequence");
      e.PushValue(prExp, prExp.type);
    ELSIF formT.receiver # NIL THEN
     (*
      *  We must first deal with the receiver if this is a method.
      *)
      iFile.CommentT("Start dispatch sequence");
      idExp := prExp(Xp.IdentX);
      formP := idExp.ident(Id.MthId).rcvFrm;
      e.GetArgP(idExp.kid, formP);
    ELSE
      iFile.CommentT("Start static call sequence");
      IF isNested(prExp) THEN
        iFile.PushStaticLink(prExp(Xp.IdLeaf).ident(Id.Procs));
      END;
    END;
   (*
    *  We push the arguments from left to right.
    *)
    FOR index := 0 TO callX.actuals.tide-1 DO
      formP := formT.formals.a[index];
      e.GetArgP(callX.actuals.a[index], formP);
    END;
   (*
    *  Now emit the actual call instruction(s)
    *)
    e.Invoke(prExp, formT);
   (*
    *  Now clean up.
    *)
    iFile.CommentT("End call sequence");
  END PushCall;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)EmitStdProc(callX : Xp.CallX),NEW;
    CONST fMsg = "Assertion failure ";
    VAR out  : Mu.MsilFile;
        cTmp : INTEGER;
        rTmp : INTEGER;
        prId : Id.PrcId;
        vrId : Id.FldId;
        pOrd : INTEGER;
        arg0 : Sy.Expr;
        argX : Sy.Expr;
        dstT : Sy.Type;
        argN : INTEGER;
        numL : INTEGER;
        incr : INTEGER;
        long : BOOLEAN;
        ovfl : BOOLEAN;
        subs : BOOLEAN;
        c    : INTEGER;
        okLb : Mu.Label;
        vecT : Ty.Vector;
   (* --------------------------- *)
  BEGIN
    out  := e.outF;
    prId := callX.kid(Xp.IdLeaf).ident(Id.PrcId);
    arg0 := callX.actuals.a[0]; (* Always need at least one arg *)
    argN := callX.actuals.tide;
    pOrd := prId.stdOrd;
    CASE pOrd OF
   (* --------------------------- *)
    | Bi.asrtP :
        okLb := out.newLabel();
        e.FallFalse(arg0, okLb);
       (*
        *   If expression evaluates to false, fall
        *   into the error code, else skip to okLb.
        *)
        IF argN > 1 THEN
          numL := intValue(callX.actuals.a[1]);
          out.Trap(fMsg + Lv.intToCharOpen(numL)^);
        ELSE
          numL := callX.token.lin;
          out.Trap(fMsg + CSt.srcNam +":"+ Lv.intToCharOpen(numL)^);
        END;
        out.DefLab(okLb);
   (* --------------------------- *)
    | Bi.incP, Bi.decP :
        argX := callX.actuals.a[1];
        dstT := arg0.type;
        ovfl := out.proc.prId.ovfChk;
        e.PushHandle(arg0, dstT);
        WITH arg0 : Xp.IdLeaf DO
            e.PushValue(arg0, dstT);
        | arg0 : Xp.IdentX DO
            vrId := arg0.ident(Id.FldId);
            out.Code(Asm.opc_dup);
            out.GetField(vrId);
        | arg0 : Xp.BinaryX DO
(*
 *  Here is the decision point: the stack is currently
 *      (top) index,arrRef,...
 *  we might reduce the 2-slot handle to an address, then go
 *      dup; ldind; <add-n>; stind
 *  but would this verify?
 *  Alternatively, we can wimp out, and store both the ref
 *  and the index then go
 *      ldloc ref; ldloc idx; ldloc ref; ldloc idx; ldelem; <add-n>; stelem
 *)
            rTmp := out.proc.newLocal(arg0.lKid.type);
            cTmp := out.proc.newLocal(Bi.intTp);
            out.StoreLocal(cTmp); (* (top) ref,...    *)
            out.Code(Asm.opc_dup);  (* (top) ref,ref,...    *)
            out.StoreLocal(rTmp); (* (top) ref,...    *)
            out.PushLocal(cTmp);  (* (top) idx,ref,...    *)
            out.PushLocal(rTmp);  (* (top) ref,idx,ref,...  *)
            out.PushLocal(cTmp);  (* (top) idx,ref,idx,ref,...  *)
            out.GetElem(dstT);    (* (top) idx,ref,...    *)
            out.proc.ReleaseLocal(cTmp);
            out.proc.ReleaseLocal(rTmp);
        END;
        e.PushValue(argX, dstT);
        IF pOrd = Bi.incP THEN
          IF ovfl THEN c := Asm.opc_add_ovf ELSE c := Asm.opc_add END;
        ELSE
          IF ovfl THEN c := Asm.opc_sub_ovf ELSE c := Asm.opc_sub END;
        END;
        out.Code(c);
        e.ValueAssign(arg0);
   (* --------------------------- *)
    | Bi.cutP  :
        argX := callX.actuals.a[1];
        vecT := arg0.type(Ty.Vector);
        cTmp := out.proc.newLocal(Bi.intTp);
        okLb := out.newLabel();
       (* 
        *  Push vector ref, and save tide
        *)
        e.PushValue(arg0, arg0.type);
        out.Code(Asm.opc_dup);
        out.GetField(Mu.vecLeng(out));
        out.StoreLocal(cTmp);
       (* 
        *  Push new leng, and check against tide
        *)
        e.PushValue(argX, Bi.intTp);
        out.Code(Asm.opc_dup);
        out.PushLocal(cTmp);
        out.CodeLb(Asm.opc_ble_un, okLb);
        out.IndexTrap();
       (* 
        *  If no trap, then assign the new tide
        *)
        out.DefLab(okLb);
        out.PutField(Mu.vecLeng(out));
        out.proc.ReleaseLocal(cTmp);
   (* --------------------------- *) 
    | Bi.apndP :  
        argX := callX.actuals.a[1];
        vecT := arg0.type(Ty.Vector);
        okLb := out.newLabel();
        cTmp := out.proc.newLocal(Bi.intTp);
        rTmp := out.proc.newLocal(Mu.vecRepTyp(vecT));
       (* 
        *  Push vector ref, and save ref and tide
        *)
        e.PushValue(arg0, vecT);
        out.Code(Asm.opc_dup);
        out.StoreLocal(rTmp);
        out.GetField(Mu.vecLeng(out));
        out.StoreLocal(cTmp);
       (*
        *  Fetch capacity and compare with tide
        *)
        out.PushLocal(rTmp);
        out.GetField(Mu.vecArrFld(vecT, out));
        out.Code(Asm.opc_ldlen);
        out.PushLocal(cTmp);
        out.CodeLb(Asm.opc_bgt, okLb);
       (*
        *  Call the RTS expand() method
        *)
        out.PushLocal(rTmp);
        out.InvokeExpand(vecT);
       (*
        *  Now insert the element
        *)
        out.DefLab(okLb);
        out.PushLocal(rTmp);
        out.GetField(Mu.vecArrFld(vecT, out));
        out.PushLocal(cTmp);
        IF Mu.isRefSurrogate(argX.type) THEN 
          (* e.ValueCopy(argX, argX.type); *)
          e.ValueCopy(argX, vecT.elemTp);
        ELSE
          (* e.PushValue(argX, argX.type); *)
          e.PushValue(argX, vecT.elemTp);
        END;
        e.EraseAndAssign(argX.type, vecT);
       (*
        *  Now increment tide;
        *)
        out.PushLocal(rTmp);
        out.PushLocal(cTmp);
        out.Code(Asm.opc_ldc_i4_1);
        out.Code(Asm.opc_add);
        out.PutField(Mu.vecLeng(out));

        out.proc.ReleaseLocal(rTmp);
        out.proc.ReleaseLocal(cTmp);
   (* --------------------------- *)
    | Bi.exclP, Bi.inclP :
        dstT := arg0.type;
        argX := callX.actuals.a[1];
        e.PushHandle(arg0, dstT);
        IF arg0 IS Xp.IdLeaf THEN
          e.PushValue(arg0, dstT);
        ELSE
          WITH arg0 : Xp.BinaryX DO
              ASSERT(arg0.kind = Xp.index);
              rTmp := out.proc.newLocal(arg0.lKid.type);
              cTmp := out.proc.newLocal(Bi.intTp);
              out.StoreLocal(cTmp);
              out.Code(Asm.opc_dup);
              out.StoreLocal(rTmp);
              out.PushLocal(cTmp);  (* (top) idx,ref,...    *)
              out.PushLocal(rTmp);  (* (top) ref,idx,ref,...  *)
              out.PushLocal(cTmp);  (* (top) idx,ref,idx,ref,...  *)
              out.GetElem(dstT);  (* (top) idx,ref,...    *)
              out.proc.ReleaseLocal(cTmp);
              out.proc.ReleaseLocal(rTmp);
          | arg0 : Xp.IdentX DO
              ASSERT(arg0.kind = Xp.selct);
              out.Code(Asm.opc_dup);
              out.GetField(arg0.ident(Id.FldId));
          END;
        END;
        IF argX.kind = Xp.numLt THEN
          out.PushInt(ORD({intValue(argX)}));
        ELSE
          out.Code(Asm.opc_ldc_i4_1);
          e.PushValue(argX, Bi.intTp);
          out.Code(Asm.opc_shl);
        END;
        IF pOrd = Bi.inclP THEN
          out.Code(Asm.opc_or);
        ELSE
          out.Code(Asm.opc_ldc_i4_M1);
          out.Code(Asm.opc_xor);
          out.Code(Asm.opc_and);
        END;
        e.ValueAssign(arg0);
   (* --------------------------- *)
    | Bi.subsP, Bi.unsbP :
        dstT := arg0.type;
        argX := callX.actuals.a[1];
        subs := pOrd = Bi.subsP;
        e.PushHandle(arg0, dstT);
        WITH argX : Xp.IdLeaf DO
            out.Code(Asm.opc_ldnull);
            out.MkAndLinkDelegate(argX.ident, IdentOf(arg0), dstT, subs);
        | argX : Xp.IdentX DO
            e.PushValue(argX.kid, CSt.ntvObj);
            out.MkAndLinkDelegate(argX.ident, IdentOf(arg0), dstT, subs);
        END;
   (* --------------------------- *)
    | Bi.haltP :
        out.PushInt(intValue(arg0));
        out.StaticCall(Mu.sysExit, -1);
       (*
        *  We now do a dummy return to signal
        *  the verifier that control exits here.
        *)
        out.PushJunkAndQuit(out.proc.prId);
   (* --------------------------- *)
    | Bi.throwP :
        IF CSt.ntvExc.assignCompat(arg0) THEN
          e.PushValue(arg0, CSt.ntvExc);
          out.Code(Asm.opc_throw);
        ELSE
          e.PushValue(arg0, CSt.ntvStr);
          out.Throw();
        END;
   (* --------------------------- *)
    | Bi.newP :
       (*
        *   arg0 is a a vector, or a pointer to a Record or Array type.
        *)
        e.PushHandle(arg0, arg0.type);
        IF argN = 1 THEN
         (*
          *  No LEN argument implies either:
          * pointer to record, OR
          * pointer to a fixed array.
          *)
          dstT := arg0.type(Ty.Pointer).boundTp;
          WITH dstT : Ty.Record DO
              out.MkNewRecord(dstT);
          | dstT : Ty.Array DO
              out.MkFixedArray(dstT);
          END;
        ELSIF arg0.type.kind = Ty.ptrTp THEN
          FOR numL := argN-1 TO 1 BY -1 DO
            argX := callX.actuals.a[numL];
            e.PushValue(argX, Bi.intTp);
          END;
          dstT := arg0.type(Ty.Pointer).boundTp;
          out.MkOpenArray(dstT(Ty.Array));
        ELSE (* must be a vector *)
          dstT := arg0.type(Ty.Vector).elemTp;
          out.MkVecRec(dstT);
          out.Code(Asm.opc_dup);
          e.PushValue(callX.actuals.a[1], Bi.intTp);
          out.MkVecArr(dstT);
        END;
        e.ValueAssign(arg0);
   (* --------------------------- *)
    | Bi.getP :
       (*
        *   arg0 is an integer value 
        *)
        argX := callX.actuals.a[1];
        e.PushHandle(argX, argX.type);
        e.PushValue(arg0, Bi.intTp);
        out.LoadIndirect(argX.type);
        e.ValueAssign(argX);
   (* --------------------------- *)
    | Bi.putP :
       (*
        *   arg0 is an integer value 
        *)
        argX := callX.actuals.a[1];
        e.PushValue(arg0, Bi.intTp);
        e.PushValue(argX, argX.type);
        out.StoreIndirect(argX.type);
   (* --------------------------- *)
    END;
  END EmitStdProc;

(* ============================================================ *)
(*        Statement Handling Methods      *)
(* ============================================================ *)

  PROCEDURE (e : MsilEmitter)EmitAssign(stat : St.Assign),NEW;
    VAR lhTyp : Sy.Type;
        erasd : BOOLEAN;
  BEGIN
   (*
    *  This is a value assign in CP.
    *)
    lhTyp := stat.lhsX.type;
   (* 
    *  Test if the erased type of the vector element
    *  has to be reconstructed by a type assertion 
    *)
    erasd := (stat.lhsX.kind = Xp.index) & 
             (stat.lhsX(Xp.BinaryX).lKid.type IS Ty.Vector);

    IF Mu.hasValueRep(lhTyp) THEN
      e.PushHandle(stat.lhsX, lhTyp);
      e.PushValue(stat.rhsX, lhTyp);
      IF erasd THEN
        e.EraseAndAssign(lhTyp, stat.lhsX(Xp.BinaryX).lKid.type(Ty.Vector));
      ELSE
        e.ValueAssign(stat.lhsX);
      END;
    ELSE (* a reference type *)
      e.PushValue(stat.lhsX, lhTyp);
      e.PushValue(stat.rhsX, lhTyp);
      WITH lhTyp : Ty.Array DO
        IF stat.rhsX.kind = Xp.mkStr THEN
          e.outF.StaticCall(Mu.aaStrCopy, -2);
        ELSIF isStrExp(stat.rhsX) THEN
          e.outF.StaticCall(Mu.vStr2ChF, -2);
        ELSE
          e.RefArrCopy(lhTyp);
        END;
      | lhTyp : Ty.Record DO
          e.RefRecCopy(lhTyp);
      END;
    END;
  END EmitAssign;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)EmitCall(stat : St.ProcCall),NEW;
    VAR expr : Xp.CallX;  (* the stat call expression *)
  BEGIN
    expr := stat.expr(Xp.CallX);
    IF (expr.kind = Xp.prCall) & expr.kid.isStdProc() THEN
      e.EmitStdProc(expr);
    ELSE (*  EXPERIMENTAL debug marking *)
      e.PushCall(expr);
      IF CSt.debug THEN e.outF.Code(Asm.opc_nop) END;
    END;
  END EmitCall;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)
                        EmitIf(stat : St.Choice; OUT ok : BOOLEAN),NEW;
    VAR out  : Mu.MsilFile;
        high : INTEGER;     (* Branch count.  *)
        indx : INTEGER;
        live : BOOLEAN;     (* then is live   *)
        else : BOOLEAN;     (* else not seen  *)
        then : Sy.Stmt;
        pred : Sy.Expr;
        nxtP : Mu.Label;   (* Next predicate *)
        exLb : Mu.Label;   (* Exit label   *)
  BEGIN
    ok := FALSE;
    out := e.outF;
    exLb := out.newLabel();
    else := FALSE;
    high := stat.preds.tide - 1;
    IF CSt.debug THEN out.Code(Asm.opc_nop) END;
    FOR indx := 0 TO high DO
      live := TRUE;
      pred := stat.preds.a[indx];
      then := stat.blocks.a[indx];
      nxtP := out.newLabel();
      IF pred = NIL THEN 
        else := TRUE;
      ELSE 
        out.LineSpan(pred.tSpan);
        e.FallTrue(pred, nxtP);
      END;
      IF then # NIL THEN e.EmitStat(then, live) END;
      IF live THEN
        ok := TRUE;
        IF indx < high THEN out.CodeLb(Asm.opc_br, exLb) END;
      END;
      out.DefLab(nxtP);
    END;
   (*
    *   If not ELSE has been seen, then control flow is still live!
    *)
    IF ~else THEN ok := TRUE END;
    out.DefLab(exLb);
  END EmitIf;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)EmitRanges
                       (locV : INTEGER;     (* select Var   *)
                        stat : St.CaseSt;   (* case stat    *)
                        minR : INTEGER;   (* min rng-ix   *)
                        maxR : INTEGER;   (* max rng-ix   *)
                        minI : INTEGER;   (* min index    *)
                        maxI : INTEGER;   (* max index    *)
                        dfLb : Mu.LbArr),NEW;  (* default Lb   *)
   (* --------------------------------------------------------- *
    *   This procedure emits the code for a single,
    *   dense range of selector values in the label-list.
    * --------------------------------------------------------- *)
    VAR out  : Mu.MsilFile;
        loIx : INTEGER;   (* low selector value for dense range  *)
        hiIx : INTEGER;   (* high selector value for dense range *)
        rNum : INTEGER;   (* total number of ranges in the group *)
        peel : INTEGER;   (* max index of range to be peeled off *)
        indx : INTEGER;
        rnge : St.Triple;
  BEGIN
    out := e.outF;
    rNum := maxR - minR + 1;
    rnge := stat.labels.a[minR];
    IF rNum = 1 THEN    (* single range only *)
      out.EmitOneRange(locV, rnge.loC, rnge.hiC, rnge.ord, minI, maxI, dfLb);
    ELSIF rNum < 4 THEN
     (*
      *    Two or three ranges only.
      *    Peel off the lowest of the ranges, and recurse.
      *)
      loIx := rnge.loC;
      peel := rnge.hiC;
      out.PushLocal(locV);
     (*
      *   There are a number of special cases
      *   that can benefit from special code.
      *)
      IF loIx = peel THEN
       (*
        *   A singleton.  Leave minI unchanged, unless peel = minI.
        *)
        out.PushInt(peel);
        out.CodeLb(Asm.opc_beq, dfLb[rnge.ord+1]);
        IF minI = peel THEN minI := peel+1 END;
      ELSIF loIx = minI THEN
       (*
        *   A range starting at the minimum selector value.
        *)
        out.PushInt(peel);
        out.CodeLb(Asm.opc_ble, dfLb[rnge.ord+1]);
        minI := peel+1;
      ELSE
        out.PushInt(loIx);
        out.Code(Asm.opc_sub);
        out.PushInt(peel-loIx);
        out.CodeLb(Asm.opc_ble_un, dfLb[rnge.ord+1]);
        (* leaving minI unchanged! *)
      END;
      e.EmitRanges(locV, stat, (minR+1), maxR, minI, maxI, dfLb);
    ELSE
     (*
      *   Four or more ranges.  Emit a dispatch table.
      *)
      loIx := rnge.loC;     (* low of min-range  *)
      hiIx := stat.labels.a[maxR].hiC;  (* high of max-range *)
      out.PushLocal(locV);
      IF loIx # 0 THEN
        out.PushInt(loIx);
        out.Code(Asm.opc_sub);
      END;
      out.SwitchHead(hiIx - loIx + 1);
      (* ---- *)
      FOR indx := minR TO maxR DO
        rnge := stat.labels.a[indx];
        WHILE loIx < rnge.loC DO
          out.LstLab(dfLb[0]); INC(loIx);
        END;
        WHILE loIx <= rnge.hiC DO
          out.LstLab(dfLb[rnge.ord+1]); INC(loIx);
        END;
      END;
      (* ---- *)
      out.SwitchTail();
      out.CodeLb(Asm.opc_br, dfLb[0])
    END;
  END EmitRanges;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)EmitGroups
                        (locV : INTEGER;   (* select vOrd  *)
                         stat : St.CaseSt; (* case stat    *)
                         minG : INTEGER;   (* min grp-indx *)
                         maxG : INTEGER;   (* max grp-indx *)
                         minI : INTEGER;   (* min index    *)
                         maxI : INTEGER;   (* max index    *)
                         dfLb : Mu.LbArr),NEW;  (* default lab  *)
   (* --------------------------------------------------------- *
    *  This function emits the branching code which sits on top
    *  of the selection code for each dense range of case values.
    * --------------------------------------------------------- *)
    VAR out   : Mu.MsilFile;
        newLb : Mu.Label;
        midPt : INTEGER;
        group : St.Triple;
        range : St.Triple;
  BEGIN
    IF maxG = -1 THEN RETURN (* This is an empty case statement *)
    ELSIF minG = maxG THEN   (* Only one remaining dense group  *)
      group := stat.groups.a[minG];
      e.EmitRanges(locV, stat, group.loC, group.hiC, minI, maxI, dfLb);
    ELSE
     (*
      *   We must bifurcate the group range, and recurse.
      *   We will split the value range at the lower limit
      *   of the low-range of the upper half-group.
      *)
      midPt := (minG + maxG + 1) DIV 2;
      group := stat.groups.a[midPt];
      range := stat.labels.a[group.loC];
     (*
      *  Test and branch at range.loC
      *)
      out := e.outF;
      newLb := out.newLabel();
      out.PushLocal(locV);
      out.PushInt(range.loC);
      out.CodeLb(Asm.opc_bge, newLb);
     (*
      *    Recurse!
      *)
      e.EmitGroups(locV, stat, minG, midPt-1, minI, range.loC-1, dfLb);
      out.DefLab(newLb);
      e.EmitGroups(locV, stat, midPt, maxG, range.loC, maxI, dfLb);
    END;
  END EmitGroups;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)
                        EmitCase(stat : St.CaseSt; OUT ok : BOOLEAN),NEW;
    VAR out  : Mu.MsilFile;
        indx : INTEGER;
        selV : INTEGER;
        live : BOOLEAN;
        minI : INTEGER;
        maxI : INTEGER;
        dfLb : Mu.LbArr;
        exLb : Mu.Label;
  BEGIN
   (* ---------------------------------------------------------- *
    *  CaseSt* = POINTER TO RECORD (Sy.Stmt)
    *      (* ----------------------------------------- *
    *       * kind-  : INTEGER; (* tag for unions *)
    *       * token* : S.Token; (* stmt first tok *)
    *       * ----------------------------------------- *)
    *   select* : Sy.Expr; (* case selector  *)
    *   chrSel* : BOOLEAN;  (* ==> use chars  *)
    *   blocks* : Sy.StmtSeq;  (* case bodies    *)
    *   elsBlk* : Sy.Stmt; (* elseCase | NIL *)
    *   labels* : TripleSeq;  (* label seqence  *)
    *   groups- : TripleSeq;  (* dense groups   *)
    *       END;
    * --------------------------------------------------------- *
    *  Notes on the semantics of this structure. "blocks" holds *
    *  an ordered list of case statement code blocks. "labels"  *
    *  is a list of ranges, intially in textual order,with flds *
    *  loC, hiC and ord corresponding to the range min, max and *
    *  the selected block ordinal number.  This list is later   *
    *  sorted on the loC value, and adjacent values merged if   *
    *  they select the same block. The "groups" list of triples *
    *  groups ranges into dense subranges in the selector space *
    *  The fields loC, hiC, and ord to hold the lower and upper *
    *  indices into the labels list, and the number of non- *
    *  default values in the group. Groups are guaranteed to  *
    *  have density (nonDefN / (max-min+1)) > DENSITY   *
    * --------------------------------------------------------- *)
    ok := FALSE;
    out := e.outF;
    exLb := out.newLabel();
    dfLb := out.getLabelRange(stat.blocks.tide+1);
    IF stat.chrSel THEN
      minI := 0; maxI := ORD(MAX(CHAR));
      selV := out.proc.newLocal(Bi.charTp);
    ELSE
      minI := MIN(INTEGER);
      maxI := MAX(INTEGER);
      selV := out.proc.newLocal(Bi.intTp);
    END;

   (*
    *    Push the selector value, and save in local variable;
    *)
    e.PushValue(stat.select, stat.select.type);
    out.StoreLocal(selV);
    e.EmitGroups(selV, stat, 0, stat.groups.tide-1, minI, maxI, dfLb);
   (*
    *    Now we emit the code for the cases.
    *    If any branch returns, then exLb is reachable.
    *)
    FOR indx := 0 TO stat.blocks.tide-1 DO
      out.DefLab(dfLb[indx+1]);
      e.EmitStat(stat.blocks.a[indx], live);
      IF live THEN
        ok := TRUE;
        out.CodeLb(Asm.opc_br, exLb);
      END;
    END;
   (*
    *    Now we emit the code for the elespart.
    *    If the elsepart returns then exLb is reachable.
    *)
    out.DefLabC(dfLb[0], "Default case");
    IF stat.elsBlk # NIL THEN
      e.EmitStat(stat.elsBlk, live);
      IF live THEN ok := TRUE END;
    ELSE
      out.CaseTrap(selV);
    END;
    out.proc.ReleaseLocal(selV);
    IF ok THEN out.DefLabC(exLb, "Case exit label") END;
  END EmitCase;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)
                        EmitWhile(stat : St.TestLoop; OUT ok : BOOLEAN),NEW;
    VAR out  : Mu.MsilFile;
        lpLb : Mu.Label;
        exLb : Mu.Label;
  BEGIN
    out := e.outF;
    lpLb := out.newLabel();
    exLb := out.newLabel();
    IF CSt.debug THEN out.Code(Asm.opc_nop) END;
    out.LineSpan(stat.test.tSpan);
    e.FallTrue(stat.test, exLb);  (* goto exLb if eval false *)
    out.DefLabC(lpLb, "Loop header");
    e.EmitStat(stat.body, ok);
    IF ok THEN
      out.LineSpan(stat.test.tSpan); 
      e.FallFalse(stat.test, lpLb); 
    END;
    out.DefLabC(exLb, "Loop exit");
  END EmitWhile;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)
                        EmitRepeat(stat : St.TestLoop; OUT ok : BOOLEAN),NEW;
    VAR out  : Mu.MsilFile;
        lpLb : Mu.Label;
  BEGIN
    out := e.outF;
    lpLb := out.newLabel();
    out.DefLabC(lpLb, "Loop header");
    e.EmitStat(stat.body, ok);
    IF ok THEN
      out.LineSpan(stat.test.tSpan);
      e.FallTrue(stat.test, lpLb); 
    END; (* exit on eval true *)
    out.CommentT("Loop exit");
  END EmitRepeat;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)
                        EmitFor(stat : St.ForLoop; OUT ok : BOOLEAN),NEW;
   (* ----------------------------------------------------------- *
    *   This code has been split into the four cases:
    *   - long control variable, counting up;
    *   - long control variable, counting down;
    *   - int control variable, counting up;
    *   - int control variable, counting down;
    *   Of course, it is possible to fold all of this, and have
    *   tests everywhere, but the following is cleaner, and easier
    *   to enhance in the future.
    *
    *   Note carefully the use of ForLoop::isSimple().  It is
    *   essential to use exactly the same function here as is
    *   used by ForLoop::flowAttr() for initialization analysis.
    *   If this were not the case, the verifier could barf.
    *
    *   23 August 2001 -- correcting error when reference
    *   param is used as a FOR-loop control variable (kjg)
    *
    *   07 February 2002 -- correcting error when control
    *   variable is stored in an XHR (uplevel) record (kjg)
    * ----------------------------------------------------------- *)
    PROCEDURE LongForUp(e: MsilEmitter; stat: St.ForLoop; OUT ok: BOOLEAN);
      VAR out  : Mu.MsilFile;
          cVar : Id.AbVar;
          step : LONGINT;
          smpl : BOOLEAN;
          isRP : BOOLEAN;
          isUl : BOOLEAN;        (* ==> cVar is an uplevel local var. *)
          indr : BOOLEAN;        (* ==> cVar is indirectly accessed   *)
          tmpL : INTEGER;
          topL : INTEGER;
          exLb : Mu.Label;
          lpLb : Mu.Label;
    BEGIN
      out := e.outF;
      lpLb := out.newLabel();
      exLb := out.newLabel();
      cVar := stat.cVar(Id.AbVar);
      step := longValue(stat.byXp); 
      smpl := stat.isSimple();
      isRP := (cVar IS Id.ParId) & (cVar(Id.ParId).boxOrd # Sy.val);
      isUl := (cVar IS Id.LocId) & (Id.uplevA IN cVar(Id.LocId).locAtt);
      indr := isRP OR isUl;

      IF indr THEN
        tmpL := out.proc.newLocal(Bi.lIntTp);
      ELSE
        tmpL := -1;                        (* keep the verifier happy! *)
      END;
      IF ~smpl THEN
        topL := out.proc.newLocal(Bi.lIntTp);
      ELSE
        topL := -1;                        (* keep the verifier happy! *)
      END;

      IF smpl THEN
        IF isRP THEN 
          out.PushArg(cVar.varOrd);
        ELSIF isUl THEN 
          out.XhrHandle(cVar(Id.LocId));
        END;
        out.PushLong(longValue(stat.loXp));
        out.PutVar(cVar);
      ELSE
        e.PushValue(stat.hiXp, Bi.lIntTp);
        out.Code(Asm.opc_dup);
        out.StoreLocal(topL);
        IF isRP THEN 
          out.PushArg(cVar.varOrd);
        ELSIF isUl THEN 
          out.XhrHandle(cVar(Id.LocId));
        END;
        e.PushValue(stat.loXp, Bi.lIntTp);
        out.Code(Asm.opc_dup);
        IF indr THEN out.StoreLocal(tmpL) END;
        out.PutVar(cVar);
        IF indr THEN out.PushLocal(tmpL) END;
       (*
        *   The top test is NEVER inside the loop.
        *)
        e.DoCmp(Xp.lessT, exLb, Bi.lIntTp);
      END;
      out.DefLabC(lpLb, "Loop header");
     (*
      *   Emit the code body.
      *   Stack contents are (top) hi, ...
      *   and exactly the same on the backedge.
      *)
      e.EmitStat(stat.body, ok);
     (*
      *   If the body returns ... do an exit test.
      *)
      IF ok THEN 
        IF smpl THEN
          out.PushLong(longValue(stat.hiXp));
        ELSE
          out.PushLocal(topL);
        END;
        IF isRP THEN 
          out.PushArg(cVar.varOrd);
        ELSIF isUl THEN 
          out.XhrHandle(cVar(Id.LocId));
        END;
        out.GetVar(cVar);                       (* (top) cv,hi       *)
        out.PushLong(step);
        out.Code(Asm.opc_add_ovf);              (* (top) cv',hi      *)
        out.Code(Asm.opc_dup);                  (* (top) cv',cv',hi  *)
        IF indr THEN out.StoreLocal(tmpL) END;
        out.PutVar(cVar);                       (* (top) cv',hi      *)
        IF indr THEN out.PushLocal(tmpL) END;
        e.DoCmp(Xp.greEq,  lpLb, Bi.lIntTp);
      END;
      IF indr THEN out.proc.ReleaseLocal(tmpL) END;
      IF ~smpl THEN out.proc.ReleaseLocal(topL) END;
     (*
      *   The exit label.
      *)
      out.DefLabC(exLb, "Loop trailer");
    END LongForUp;

   (* ----------------------------------------- *)

    PROCEDURE LongForDn(e: MsilEmitter; stat: St.ForLoop; OUT ok: BOOLEAN);
      VAR out  : Mu.MsilFile;
          cVar : Id.AbVar;
          tmpL : INTEGER;
          topL : INTEGER;
          step : LONGINT;
          smpl : BOOLEAN;
          isRP : BOOLEAN;
          isUl : BOOLEAN;        (* ==> cVar is an uplevel local var. *)
          indr : BOOLEAN;        (* ==> cVar is indirectly accessed   *)
          exLb : Mu.Label;
          lpLb : Mu.Label;
    BEGIN
      out := e.outF;
      lpLb := out.newLabel();
      exLb := out.newLabel();
      cVar := stat.cVar(Id.AbVar);
      step := longValue(stat.byXp);
      smpl := stat.isSimple();
      isRP := (cVar IS Id.ParId) & (cVar(Id.ParId).boxOrd # Sy.val);
      isUl := (cVar IS Id.LocId) & (Id.uplevA IN cVar(Id.LocId).locAtt);
      indr := isRP OR isUl;

      IF indr THEN
        tmpL := out.proc.newLocal(Bi.lIntTp);
      ELSE
        tmpL := -1;                        (* keep the verifier happy! *)
      END;
      IF ~smpl THEN
        topL := out.proc.newLocal(Bi.lIntTp);
      ELSE
        topL := -1;                        (* keep the verifier happy! *)
      END;

      IF smpl THEN
        IF isRP THEN 
          out.PushArg(cVar.varOrd);
        ELSIF isUl THEN 
          out.XhrHandle(cVar(Id.LocId));
        END;
        out.PushLong(longValue(stat.loXp));
        out.PutVar(cVar);
      ELSE
        e.PushValue(stat.hiXp, Bi.lIntTp);
        out.Code(Asm.opc_dup);
        out.StoreLocal(topL);
        IF isRP THEN 
          out.PushArg(cVar.varOrd);
        ELSIF isUl THEN 
          out.XhrHandle(cVar(Id.LocId));
        END;
        e.PushValue(stat.loXp, Bi.lIntTp);
        out.Code(Asm.opc_dup);
        IF indr THEN  out.StoreLocal(tmpL) END;
        out.PutVar(cVar);
        IF indr THEN out.PushLocal(tmpL) END;
       (*
        *   The top test is NEVER inside the loop.
        *)
        e.DoCmp(Xp.greT,  exLb, Bi.lIntTp);
      END;
      out.DefLabC(lpLb, "Loop header");
     (*
      *   Emit the code body.
      *   Stack contents are (top) hi, ...
      *   and exactly the same on the backedge.
      *)
      e.EmitStat(stat.body, ok);
     (*
      *   If the body returns ... do an exit test.
      *)
      IF ok THEN 
        IF smpl THEN
          out.PushLong(longValue(stat.hiXp));
        ELSE
          out.PushLocal(topL);
        END;
        IF isRP THEN 
          out.PushArg(cVar.varOrd);
        ELSIF isUl THEN 
          out.XhrHandle(cVar(Id.LocId));
        END;
        out.GetVar(cVar);                      (* (top) cv,hi      *)
        out.PushLong(step);
        out.Code(Asm.opc_add_ovf);             (* (top) cv',hi     *)
        out.Code(Asm.opc_dup);                 (* (top) cv',cv',hi *)
        IF indr THEN out.StoreLocal(tmpL) END;
        out.PutVar(cVar);                      (* (top) cv',hi     *)
        IF indr THEN out.PushLocal(tmpL) END;
        e.DoCmp(Xp.lessEq, lpLb, Bi.lIntTp);
      END;
      IF indr THEN out.proc.ReleaseLocal(tmpL) END;
      IF ~smpl THEN out.proc.ReleaseLocal(topL) END;
     (*
      *   The exit label.
      *)
      out.DefLabC(exLb, "Loop trailer");
    END LongForDn;

   (* ----------------------------------------- *)

    PROCEDURE IntForUp(e: MsilEmitter; stat: St.ForLoop; OUT ok: BOOLEAN);
      VAR out  : Mu.MsilFile;
          cVar : Id.AbVar;
          topV : INTEGER;
          tmpV : INTEGER;
          step : INTEGER;
          smpl : BOOLEAN;
          isRP : BOOLEAN;        (* ==> cVar is a reference parameter *)
          isUl : BOOLEAN;        (* ==> cVar is an uplevel local var. *)
          indr : BOOLEAN;        (* ==> cVar is indirectly accessed   *)
          exLb : Mu.Label;
          lpLb : Mu.Label;
    BEGIN
     (*
      *    This is the common case, so we work a bit harder.
      *)
      out := e.outF;
      lpLb := out.newLabel();
      exLb := out.newLabel();
      cVar := stat.cVar(Id.AbVar);
      step := intValue(stat.byXp);
      smpl := stat.isSimple();
      isRP := (cVar IS Id.ParId) & (cVar(Id.ParId).boxOrd # Sy.val);
      isUl := (cVar IS Id.LocId) & (Id.uplevA IN cVar(Id.LocId).locAtt);
      indr := isRP OR isUl;

      IF indr THEN
        tmpV := out.proc.newLocal(Bi.intTp);
      ELSE
        tmpV := -1;                        (* keep the verifier happy! *)
      END;
      IF ~smpl THEN
        topV := out.proc.newLocal(Bi.intTp);
      ELSE
        topV := -1;                        (* keep the verifier happy! *)
      END;

      IF smpl THEN
        IF isRP THEN 
          out.PushArg(cVar.varOrd);
        ELSIF isUl THEN 
          out.XhrHandle(cVar(Id.LocId));
        END;
        out.PushInt(intValue(stat.loXp));
        out.PutVar(cVar);
      ELSE
        e.PushValue(stat.hiXp, Bi.intTp);
        out.Code(Asm.opc_dup);
        out.StoreLocal(topV);
        IF isRP THEN
          out.PushArg(cVar.varOrd);
        ELSIF isUl THEN
          out.XhrHandle(cVar(Id.LocId));
        END;
        e.PushValue(stat.loXp, Bi.intTp);
        out.Code(Asm.opc_dup);
        IF indr THEN out.StoreLocal(tmpV) END;
        out.PutVar(cVar);
        IF indr THEN out.PushLocal(tmpV) END;
       (*
        *   The top test is NEVER inside the loop.
        *)
        e.DoCmp(Xp.lessT, exLb, Bi.intTp);
      END;
      out.DefLabC(lpLb, "Loop header");
     (*
      *   Emit the code body.
      *)
      e.EmitStat(stat.body, ok);
     (*
      *   If the body returns ... do an exit test.
      *)
      IF ok THEN 
        IF smpl THEN
          out.PushInt(intValue(stat.hiXp));
        ELSE
          out.PushLocal(topV);
        END;
        IF isRP THEN 
          out.PushArg(cVar.varOrd);
        ELSIF isUl THEN 
          out.XhrHandle(cVar(Id.LocId));
        END;
        out.GetVar(cVar);                       (* (top) cv,hi       *)
        out.PushInt(step);
        out.Code(Asm.opc_add_ovf);              (* (top) cv',hi      *)
        out.Code(Asm.opc_dup);                  (* (top) cv',cv',hi  *)
        IF indr THEN out.StoreLocal(tmpV) END;
        out.PutVar(cVar);                       (* (top) cv',hi      *)
        IF indr THEN out.PushLocal(tmpV) END;
        e.DoCmp(Xp.greEq, lpLb, Bi.intTp);
      END;
      IF indr THEN out.proc.ReleaseLocal(tmpV) END;
      IF ~smpl THEN out.proc.ReleaseLocal(topV) END;
     (*
      *   The exit label.
      *)
      out.DefLabC(exLb, "Loop trailer");
    END IntForUp;
  
   (* ----------------------------------------- *)

    PROCEDURE IntForDn(e: MsilEmitter; stat: St.ForLoop; OUT ok: BOOLEAN);
      VAR out  : Mu.MsilFile;
          cVar : Id.AbVar;
          tmpV : INTEGER;
          topV : INTEGER;
          step : INTEGER;
          smpl : BOOLEAN;
          isRP : BOOLEAN;        (* ==> cVar is a reference parameter *)
          isUl : BOOLEAN;        (* ==> cVar is an uplevel local var. *)
          indr : BOOLEAN;        (* ==> cVar is indirectly accessed   *)
          exLb : Mu.Label;
          lpLb : Mu.Label;
    BEGIN
      out := e.outF;
      lpLb := out.newLabel();
      exLb := out.newLabel();
      cVar := stat.cVar(Id.AbVar);
      step := intValue(stat.byXp);
      smpl := stat.isSimple();
      isRP := (cVar IS Id.ParId) & (cVar(Id.ParId).boxOrd # Sy.val);
      isUl := (cVar IS Id.LocId) & (Id.uplevA IN cVar(Id.LocId).locAtt);
      indr := isRP OR isUl;

      IF indr THEN
        tmpV := out.proc.newLocal(Bi.intTp);
      ELSE
        tmpV := -1;                        (* keep the verifier happy! *)
      END;
      IF ~smpl THEN
        topV := out.proc.newLocal(Bi.intTp);
      ELSE
        topV := -1;                        (* keep the verifier happy! *)
      END;

      IF smpl THEN
        IF isRP THEN 
          out.PushArg(cVar.varOrd);
        ELSIF isUl THEN 
          out.XhrHandle(cVar(Id.LocId));
        END;
        out.PushInt(intValue(stat.loXp));
        out.PutVar(cVar);
      ELSE
        e.PushValue(stat.hiXp, Bi.intTp);
        out.Code(Asm.opc_dup);
        out.StoreLocal(topV);
        IF isRP THEN
          out.PushArg(cVar.varOrd);
        ELSIF isUl THEN
          out.XhrHandle(cVar(Id.LocId));
        END;
        e.PushValue(stat.loXp, Bi.intTp);
        out.Code(Asm.opc_dup);
        IF indr THEN out.StoreLocal(tmpV) END;
        out.PutVar(cVar);
        IF indr THEN out.PushLocal(tmpV) END;
       (*
        *   The top test is NEVER inside the loop.
        *)
        e.DoCmp(Xp.greT, exLb, Bi.intTp);
      END;
      out.DefLabC(lpLb, "Loop header");
     (*
      *   Emit the code body.
      *)
      e.EmitStat(stat.body, ok);
     (*
      *   If the body returns ... do an exit test.
      *)
      IF ok THEN 
      IF smpl THEN
          out.PushInt(intValue(stat.hiXp));
      ELSE
          out.PushLocal(topV);
      END;
        IF isRP THEN 
          out.PushArg(cVar.varOrd);
        ELSIF isUl THEN
          out.XhrHandle(cVar(Id.LocId));
        END;
        out.GetVar(cVar);                       (* (top) cv,hi       *)
        out.PushInt(step);
        out.Code(Asm.opc_add_ovf);              (* (top) cv',hi      *)
        out.Code(Asm.opc_dup);                  (* (top) cv',cv',hi  *)
        IF indr THEN out.StoreLocal(tmpV) END;
        out.PutVar(cVar);                       (* (top) cv',hi      *)
        IF indr THEN out.PushLocal(tmpV) END;
        e.DoCmp(Xp.lessEq, lpLb, Bi.intTp);
      END;
      IF indr THEN out.proc.ReleaseLocal(tmpV) END;
      IF ~smpl THEN out.proc.ReleaseLocal(topV) END;
     (*
      *   The exit label.
      *)
      out.DefLabC(exLb, "Loop trailer");
    END IntForDn;

   (* ----------------------------------------- *)
  BEGIN (* body of EmitFor *)
    IF stat.cVar.type.isLongType() THEN
      IF longValue(stat.byXp) > 0 THEN LongForUp(e, stat, ok);
      ELSE LongForDn(e, stat, ok);
      END;
    ELSE
      IF longValue(stat.byXp) > 0 THEN IntForUp(e, stat, ok);
      ELSE IntForDn(e, stat, ok);
      END;
    END;
  END EmitFor;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)
                        EmitLoop(stat : St.TestLoop; OUT ok : BOOLEAN),NEW;
    VAR out  : Mu.MsilFile;
        lpLb : Mu.Label;
        exLb : Mu.Label;
  BEGIN
    out := e.outF;
    lpLb := out.newLabel();
    exLb := out.newLabel();
    stat.tgLbl := exLb;
    out.DefLabC(lpLb, "Loop header");
    e.EmitStat(stat.body, ok);
    IF ok THEN out.CodeLb(Asm.opc_br, lpLb) END;
    out.DefLabC(exLb, "Loop exit");
  END EmitLoop;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)
                        EmitWith(stat : St.Choice; OUT ok : BOOLEAN),NEW;
    VAR out  : Mu.MsilFile;
        high : INTEGER;     (* Branch count.  *)
        indx : INTEGER;
        live : BOOLEAN;
        then : Sy.Stmt;
        pred : Xp.BinaryX;
        tVar : Id.LocId;
        exLb : Mu.Label;   (* Exit label     *)
        nxtP : Mu.Label;   (* Next predicate *)
   (* --------------------------- *)
    PROCEDURE WithTest(je : MsilEmitter;
                       os : Mu.MsilFile;
                       pr : Xp.BinaryX;
                       nx : Mu.Label;
                       to : INTEGER);
                   VAR ty : Sy.Type;
    BEGIN
      ty := pr.rKid(Xp.IdLeaf).ident.type;
      je.PushValue(pr.lKid, pr.lKid.type);
      os.CodeT(Asm.opc_isinst, ty);
     (*
      *  isinst returns the cast type, or NIL
      *  We save this to the allocated temp or needed type.
      *)
      os.StoreLocal(to);
      os.PushLocal(to);
      os.CodeLb(Asm.opc_brfalse, nx);  (* branch on NIL *)
    END WithTest;
   (* --------------------------- *)
  BEGIN
    tVar := NIL;
    pred := NIL;
    ok := FALSE;
    out := e.outF;
    exLb := out.newLabel();
    high := stat.preds.tide - 1;
    IF CSt.debug THEN out.Code(Asm.opc_nop) END;
    FOR indx := 0 TO high DO
      live := TRUE;
      then := stat.blocks.a[indx];
      pred := stat.preds.a[indx](Xp.BinaryX);
      tVar := stat.temps.a[indx](Id.LocId);
      nxtP := out.newLabel();
      IF pred # NIL THEN
        tVar.varOrd := out.proc.newLocal(tVar.type);
        WithTest(e, out, pred, nxtP, tVar.varOrd);
      END;
      IF then # NIL THEN e.EmitStat(then, live) END;
      IF live THEN
        ok := TRUE;
       (*
        *  If this is not the else case, skip over the
        *  later cases, or jump over the WITH ELSE trap.
        *)
        IF pred # NIL THEN out.CodeLb(Asm.opc_br, exLb) END;
      END;
      out.DefLab(nxtP);
      IF tVar # NIL THEN out.proc.ReleaseLocal(tVar.varOrd) END;
    END;
    IF pred # NIL THEN out.WithTrap(pred(Xp.BinaryX).lKid(Xp.IdLeaf).ident) END;
    out.DefLab(exLb);
  END EmitWith;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)EmitExit(stat : St.ExitSt),NEW;
  BEGIN
    e.outF.CodeLb(Asm.opc_br, stat.loop(St.TestLoop).tgLbl(Mu.Label));
  END EmitExit;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)EmitReturn(stat : St.Return),NEW;
    VAR out : Mu.MsilFile;
        ret : Sy.Expr;
  BEGIN
    out := e.outF;
    IF CSt.debug THEN
      out.Code(Asm.opc_nop);
      out.LineSpan(stat.prId(Id.Procs).endSpan);
    END;
    ret := stat.retX;
    IF (stat.retX # NIL) & 
       (out.proc.prId.kind # Id.ctorP) THEN e.PushValue(ret, ret.type) END;
    out.DoReturn;
  END EmitReturn;

(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)
                        EmitBlock(stat : St.Block; OUT ok : BOOLEAN),NEW;
    VAR index, limit : INTEGER;
  BEGIN
    ok := TRUE;
    index := 0;
    limit := stat.sequ.tide;
    WHILE ok & (index < limit) DO
      e.EmitStat(stat.sequ.a[index], ok);
      INC(index);
    END;
  END EmitBlock;

(* ---------------------------------------------------- *)
(* ---------------------------------------------------- *)

  PROCEDURE (e : MsilEmitter)EmitStat(stat : Sy.Stmt; OUT ok : BOOLEAN),NEW;
    VAR depth : INTEGER;
        out   : Mu.MsilFile;
  BEGIN
    IF (stat = NIL) OR (stat.kind = St.emptyS) THEN ok := TRUE; RETURN END;
    out := e.outF;
    out.LineSpan(stat.Span());
    depth := out.proc.getDepth();
    CASE stat.kind OF
    | St.assignS  : e.EmitAssign(stat(St.Assign)); ok := TRUE;
    | St.procCall : e.EmitCall(stat(St.ProcCall)); ok := TRUE;
    | St.ifStat   : e.EmitIf(stat(St.Choice), ok);
    | St.caseS    : e.EmitCase(stat(St.CaseSt), ok);
    | St.whileS   : e.EmitWhile(stat(St.TestLoop), ok);
    | St.repeatS  : e.EmitRepeat(stat(St.TestLoop), ok);
    | St.forStat  : e.EmitFor(stat(St.ForLoop), ok);
    | St.loopS    : e.EmitLoop(stat(St.TestLoop), ok);
    | St.withS    : e.EmitWith(stat(St.Choice), ok);
    | St.exitS    : e.EmitExit(stat(St.ExitSt)); ok := TRUE;
    | St.returnS  : e.EmitReturn(stat(St.Return)); ok := FALSE;
    | St.blockS   : e.EmitBlock(stat(St.Block), ok);
    END;
    IF CSt.verbose & (depth # out.proc.getDepth()) THEN
          out.Comment("Depth adjustment") END;
    out.proc.SetDepth(depth);
  END EmitStat;

(* ============================================================ *)
(* ============================================================ *)
END MsilMaker.
(* ============================================================ *)
(* ============================================================ *)
