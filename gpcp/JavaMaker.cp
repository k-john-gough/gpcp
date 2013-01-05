(* ============================================================ *)
(*  JavaMaker is the concrete class for emitting java           *)
(*  class files.                                                *)
(*  Diane Corney - September,2000.                              *)
(* ============================================================ *)

MODULE JavaMaker;

  IMPORT 
        GPCPcopyright,
        ASCII,
        Error,
        Console,
        L := LitValue,
        CPascalS,
        FileNames,
        ClassMaker,
        JavaBase,
        ClassUtil,
        JsmnUtil,
        Cst := CompState,
        Jvm := JVMcodes,
        Ju  := JavaUtil,
        Bi := Builtin,
        Sy := Symbols,
        Id := IdDesc,
        Ty := TypeDesc,
        Xp := ExprDesc,
        St := StatDesc;

(* ------------------------------------ *)

  TYPE JavaWorkList* = 	
        POINTER TO 
          RECORD (JavaBase.ClassEmitter)
         (* --------------------------- *
          * mod* : Id.BlkId; 		*
          * --------------------------- *)
            tide : INTEGER;
            high : INTEGER;
            work : POINTER TO ARRAY OF JavaEmitter;
          END;

(* ------------------------------------ *)

  TYPE JavaEmitter* = 	
        POINTER TO ABSTRACT
          RECORD (JavaBase.ClassEmitter)
         (* --------------------------- *
          * mod* : Id.BlkId; 		*
          * --------------------------- *)
            outF  : Ju.JavaFile;
          END;

(* ------------------------------------ *)

  TYPE JavaModEmitter* = 	
        POINTER TO 
          RECORD (JavaEmitter);
         (* --------------------------- *
          * mod* : Id.BlkId; 		*
          * outF : JavaBase.JavaFile;   *
          * --------------------------- *)
          END;

(* ------------------------------------ *)

  TYPE JavaRecEmitter* = 	
        POINTER TO 
          RECORD (JavaEmitter)
         (* --------------------------- *
          * mod* : Id.BlkId; 		*
          * outF : Ju.JavaFile; 		*
          * --------------------------- *)
            recT : Ty.Record;
          END;

(* ------------------------------------ *)

  TYPE JavaProcTypeEmitter* = 	
        POINTER TO 
          RECORD (JavaEmitter)
         (* --------------------------- *
          * mod* : Id.BlkId; 		*
          * outF : Ju.JavaFile; 		*
          * --------------------------- *)
            prcT : Ty.Procedure;
          END;

(* ------------------------------------ *)

  TYPE JavaAssembler* = 	
        POINTER TO 
          RECORD (ClassMaker.Assembler)
          END;


(* ------------------------------------ *)

  VAR
        asmList : LitValue.CharOpenSeq;
        currentLoopLabel : Ju.Label;

(* ============================================================ *)

  PROCEDURE Append(list : JavaWorkList; 
        	   emit : JavaEmitter);
    VAR temp : POINTER TO ARRAY OF JavaEmitter;
        i    : INTEGER;
  BEGIN
    IF list.tide > list.high THEN (* must expand *)
      temp := list.work;
      list.high := list.high * 2 + 1;
      NEW(list.work, (list.high+1));
      FOR i := 0 TO list.tide-1 DO list.work[i] := temp[i] END;
    END;
    list.work[list.tide] := emit; INC(list.tide);
  END Append;

(* ============================================================ *)
 
  PROCEDURE newJavaEmitter*(mod : Id.BlkId) : JavaWorkList;
    VAR emitter : JavaWorkList;
        modEmit : JavaModEmitter;
        modName : LitValue.CharOpen;
  BEGIN
    modName := Sy.getName.ChPtr(mod);
   (*
    *  Allocate a new worklist object.
    *)
    NEW(emitter);
    emitter.mod := mod;
    NEW(emitter.work, 4);
    emitter.tide := 0;
    emitter.high := 3;
    JavaBase.worklist := emitter;
   (*
    *  Allocate a JavaModEmitter to be first item
    *  on the worklist.  All later items will be of
    *  JavaRecEmitter type.
    *)
    NEW(modEmit);
    modEmit.mod  := mod;
   (*
    *  Now append the mod-emitter to the worklist.
    *)
    Append(emitter, modEmit);
    RETURN emitter;
  END newJavaEmitter;

(* ============================================================ *)

  PROCEDURE newJavaAsm*() : JavaAssembler;
    VAR asm : JavaAssembler;
  BEGIN
    NEW(asm);
    LitValue.ResetCharOpenSeq(asmList);
    RETURN asm;
  END newJavaAsm;

(* ============================================================ *)

  PROCEDURE (list : JavaWorkList)AddNewRecEmitter*(inTp : Ty.Record);
    VAR emit : JavaRecEmitter;
  BEGIN
    NEW(emit);
    emit.mod  := list.mod;
   (*
    *  Set the current record type for this class.
    *)
    emit.recT := inTp;
   (*
    *  Now append the new RecEmitter to the worklist.
    *)
    Append(list, emit);
  END AddNewRecEmitter;

(* ============================================================ *)

  PROCEDURE (list : JavaWorkList)AddNewProcTypeEmitter*(inTp : Ty.Procedure);
    VAR emit : JavaProcTypeEmitter;
  BEGIN
    NEW(emit);
    emit.mod  := list.mod;
   (*
    *  Set the current record type for this class.
    *)
    emit.prcT := inTp;
   (*
    *  Now append the new RecEmitter to the worklist.
    *)
    Append(list, emit);
  END AddNewProcTypeEmitter;

(* ============================================================ *)
(*  Mainline emitter, consumes worklist emitting assembler	*)
(*  files until the worklist is empty.				*)
(* ============================================================ *)

  PROCEDURE (this : JavaWorkList)Emit*();
    VAR ix : INTEGER;
  BEGIN
   (*
    *  First construct the base class-name string in the BlkId.
    *)
    Ju.Init();
    Ju.MkBlkName(this.mod);

    ix := 0;
    WHILE ix < this.tide DO
      this.work[ix].Emit();
      INC(ix);
    END;
  END Emit;

(* ============================================================ *)
(*  Creates basic imports for java.lang, and inserts a few type *)
(*  descriptors for Object, Exception, and String.		*)
(* ============================================================ *)

  PROCEDURE (this : JavaWorkList)Init*();
    VAR tId : Id.TypId;
        blk : Id.BlkId;
        obj : Id.TypId;
        cls : Id.TypId;
        str : Id.TypId;
        exc : Id.TypId;
        xhr : Id.TypId;
  BEGIN
   (*
    *  Create import descriptor for java.lang
    *)
    Bi.MkDummyImport("java_lang", "java.lang", blk);
	Cst.SetSysLib(blk);
   (*
    *  Create various classes.
    *)
    Bi.MkDummyClass("Object", blk, Ty.isAbs, obj);
    Cst.ntvObj := obj.type;
    Bi.MkDummyClass("String", blk, Ty.noAtt, str);
    Cst.ntvStr := str.type;
    Bi.MkDummyClass("Exception", blk, Ty.extns, exc);
    Cst.ntvExc := exc.type;
    Bi.MkDummyClass("Class", blk, Ty.noAtt, cls);
    Cst.ntvTyp := cls.type;
   (*
    *  Create import descriptor for CP.RTS
    *)
    Bi.MkDummyImport("RTS", "", blk);
    Bi.MkDummyAlias("NativeType", blk, cls.type, Cst.clsId);
    Bi.MkDummyAlias("NativeObject", blk, obj.type, Cst.objId);
    Bi.MkDummyAlias("NativeString", blk, str.type, Cst.strId);
    Bi.MkDummyAlias("NativeException", blk, exc.type, Cst.excId);

    Bi.MkDummyVar("dblPosInfinity",blk,Bi.realTp,Cst.dblInf);
    Bi.MkDummyVar("dblNegInfinity",blk,Bi.realTp,Cst.dblNInf);
    Bi.MkDummyVar("fltPosInfinity",blk,Bi.sReaTp,Cst.fltInf);
    Bi.MkDummyVar("fltNegInfinity",blk,Bi.sReaTp,Cst.fltNInf);
    INCL(blk.xAttr, Sy.need);
   (*
    *  Uplevel addressing stuff.
    *)
    Bi.MkDummyImport("$CPJrts$", "CP.CPJrts", blk);
    Bi.MkDummyClass("XHR", blk, Ty.isAbs, xhr);
    Cst.rtsXHR := xhr.type;
    Cst.xhrId.recTyp := Cst.rtsXHR;
    Cst.xhrId.type   := Cst.rtsXHR;
  END Init;

(* ============================================================ *)

  PROCEDURE (this : JavaWorkList)ObjectFeatures*();
    VAR prcSig : Ty.Procedure; 
        thePar : Id.ParId;
  BEGIN
	NEW(prcSig);
    prcSig.retType := Cst.strId.type;
    Id.InitParSeq(prcSig.formals, 2);
    Bi.MkDummyMethodAndInsert("toString", prcSig, Cst.ntvObj, Cst.sysLib, Sy.pubMode, Sy.var, Id.extns);

	NEW(prcSig);
    prcSig.retType := Bi.intTp;
    Id.InitParSeq(prcSig.formals, 2);
    Bi.MkDummyMethodAndInsert("hashCode", prcSig, Cst.ntvObj, Cst.sysLib, Sy.pubMode, Sy.var, Id.extns);

	NEW(prcSig);
    prcSig.retType := Cst.ntvObj;
    Id.InitParSeq(prcSig.formals, 2);
    Bi.MkDummyMethodAndInsert("clone", prcSig, Cst.ntvObj, Cst.sysLib, Sy.protect, Sy.var, Id.extns);

	NEW(prcSig);
	NEW(thePar);
    prcSig.retType := Bi.boolTp;
    Id.InitParSeq(prcSig.formals, 2);
	thePar.parMod := Sy.val;
	thePar.type := Cst.ntvObj;
	thePar.varOrd := 1;
	Id.AppendParam(prcSig.formals, thePar);
    Bi.MkDummyMethodAndInsert("equals", prcSig, Cst.ntvObj, Cst.sysLib, Sy.pubMode, Sy.var, IdDesc.extns);
  END ObjectFeatures;

(* ============================================================ *)
  PROCEDURE (this : JavaAssembler)Assemble*();
    VAR ix : INTEGER;
  BEGIN
    IF asmList.tide > 0 THEN
      Cst.Message("Jasmin Assmbler no longer supported");
      Cst.Message("The following jasmin text files were created:");
      FOR ix := 0 TO asmList.tide-1 DO
        Console.Write(ASCII.HT); 
        Console.WriteString(asmList.a[ix]^);
        Console.WriteLn;
      END;
    END;
  END Assemble;

(* ============================================================ *)

  PROCEDURE (t : JavaEmitter)EmitBody(f : Ju.JavaFile),NEW,ABSTRACT;
  PROCEDURE^ (e : JavaEmitter)EmitProc(proc : Id.Procs),NEW;
  PROCEDURE^ (e : JavaEmitter)EmitStat(stat : Sy.Stmt; OUT ok : BOOLEAN),NEW;
  PROCEDURE^ (e : JavaEmitter)PushCall(callX : Xp.CallX),NEW;
  PROCEDURE^ (e : JavaEmitter)PushValue(exp : Sy.Expr; typ : Sy.Type),NEW;
  PROCEDURE^ (e : JavaEmitter)FallFalse(exp : Sy.Expr; tLb : Ju.Label),NEW;
  PROCEDURE^ (e : JavaEmitter)ValueCopy(act : Sy.Expr; fmT : Sy.Type),NEW;
  PROCEDURE^ (e : JavaEmitter)PushArg(act : Sy.Expr;
        			      frm : Id.ParId;
        			  VAR seq : Sy.ExprSeq),NEW;

(* ============================================================ *)

  PROCEDURE (t : JavaRecEmitter)CopyProc(),NEW;
    VAR out  : Ju.JavaFile;
        junk : INTEGER;
        indx : INTEGER;
        idnt : Sy.Idnt;
        fTyp : Sy.Type;
        
  BEGIN
   (*
    *   Emit the copy procedure "__copy__()
    *)
    out := t.outF;
    out.CopyProcHead(t.recT);
    junk := out.newLocal();	(* create space for two locals *)
    junk := out.newLocal();
   (*
    *    Recurse to super class, if necessary.
    *) 
    IF (t.recT.baseTp # NIL) & 
       (t.recT.baseTp IS Ty.Record) & 
       ~t.recT.baseTp.isNativeObj() THEN
      out.Code(Jvm.opc_aload_0);
      out.Code(Jvm.opc_aload_1);
      out.ValRecCopy(t.recT.baseTp(Ty.Record));
    END;
   (*
    *    Emit field-by-field copy.
    *)
    FOR indx := 0 TO t.recT.fields.tide-1 DO
      idnt := t.recT.fields.a[indx];
      fTyp := idnt.type;
      out.Code(Jvm.opc_aload_0);
      IF (fTyp.kind = Ty.recTp) OR 
         (fTyp.kind = Ty.arrTp) THEN
        out.PutGetF(Jvm.opc_getfield, t.recT, idnt(Id.FldId));
      END;
      out.Code(Jvm.opc_aload_1);
      out.PutGetF(Jvm.opc_getfield, t.recT, idnt(Id.FldId));
      WITH fTyp : Ty.Array DO 
          out.ValArrCopy(fTyp);
      | fTyp : Ty.Record DO
          out.ValRecCopy(fTyp);
      ELSE
        out.PutGetF(Jvm.opc_putfield, t.recT, idnt(Id.FldId));
      END;
    END;
    out.VoidTail();
  END CopyProc;

(* ============================================================ *)

  PROCEDURE (this : JavaProcTypeEmitter)EmitBody(out : Ju.JavaFile);
  (** Create the assembler for a class file for this proc-type wrapper. *)
    VAR pType : Ty.Procedure; (* The procedure type that is being emitted *)
	    proxy : Ty.Record;    (* The record that stands for the proc-type *)
		invoke : Id.MthId;    (* The abstract invoke method for this      *)
  BEGIN
    pType := this.prcT;
	proxy := pType.hostClass;
	proxy.idnt := pType.idnt;
	proxy.recAtt := Ty.isAbs;
	out.StartRecClass(proxy);

   (* Emit the no-arg constructor *) 
	out.RecMakeInit(proxy, NIL);
	out.CallSuperCtor(proxy, NIL);
	out.VoidTail();

   (* Emit the abstract Invoke method *)
    invoke := Ju.getProcVarInvoke(pType);
	Ju.MkProcName(invoke);
	Ju.RenumberLocals(invoke);
	out.theP := invoke;
	out.StartProc(invoke);
	out.EndProc();
  END EmitBody;

(* ============================================================ *)

  PROCEDURE (this : JavaRecEmitter)EmitBody(out : Ju.JavaFile);
  (** Create the assembler for a class file for this record. *)
    VAR index  : INTEGER;
        parIx  : INTEGER;
        clsId  : Sy.Idnt;
        ident  : Sy.Idnt;
        ctorD  : Id.PrcId;
        sCtor  : Id.PrcId;
        sCtTy  : Ty.Procedure;
        baseT  : Sy.Type;
        field  : Id.FldId;
        method : Id.MthId;
        record : Ty.Record;
        impRec : Sy.Idnt;
        attr   : INTEGER;
        form   : Id.ParId;
        expr   : Sy.Expr;
        live   : BOOLEAN;
        retn   : Sy.Type;
  BEGIN
    record := this.recT;
    out.StartRecClass(record); 
   (*
    *  Emit all the fields ...
    *)
    out.InitFields(record.fields.tide);
    FOR index := 0 TO record.fields.tide-1 DO
      out.EmitField(record.fields.a[index](Id.FldId));
    END;
    out.InitMethods(record.methods.tide+2);
   (*
    *  Emit the no-arg constructor
    *)
    IF ~(Sy.noNew IN record.xAttr) &
       ~(Sy.xCtor IN record.xAttr) THEN
      out.RecMakeInit(record, NIL);
      out.CallSuperCtor(record, NIL);
      out.VoidTail();
    END;
   (*
    *  Emit constructors with args
    *)
    FOR index := 0 TO record.statics.tide-1 DO
      sCtTy := NIL;
      ctorD := record.statics.a[index](Id.PrcId);
      out.RecMakeInit(record, ctorD);
     (*
      *  Copy args for super constructors with args.
      *)
      IF ctorD # NIL THEN
        sCtor := ctorD.basCll.sprCtor(Id.PrcId);
        IF sCtor # NIL THEN
          sCtTy := sCtor.type(Ty.Procedure);
          IF sCtTy.xName = NIL THEN Ju.MkCallAttr(sCtor, sCtTy) END;
          FOR parIx := 0 TO ctorD.basCll.actuals.tide-1 DO
            form := sCtTy.formals.a[parIx];
            expr := ctorD.basCll.actuals.a[parIx];
            this.PushArg(expr, form, ctorD.basCll.actuals);
          END;
        END;
      END;
     (*
      *  Now call the super constructor
      *)
      out.CallSuperCtor(record, sCtTy);
      IF (ctorD # NIL) & (ctorD.body # NIL) THEN
        IF ctorD.rescue # NIL THEN out.Try END;
        this.EmitStat(ctorD.body, live); 
        IF ctorD.rescue # NIL THEN
          out.Catch(ctorD);
          this.EmitStat(ctorD.rescue, live);
        END;
      END;
      out.EndProc();
    END;
    IF ~(Sy.noCpy IN record.xAttr) THEN this.CopyProc() END;
   (*
    *  Emit all the (non-forward) methods ...
    *)
    FOR index := 0 TO record.methods.tide-1 DO
      ident  := record.methods.a[index];
      method := ident(Id.MthId);
      IF method.kind = Id.conMth THEN
	    IF method.scopeNm = NIL THEN
		  Ju.MkProcName(method);
		  Ju.RenumberLocals(method);
		END;
        this.EmitProc(method)
      END;
    END;
  END EmitBody;

(* ============================================================ *)

  PROCEDURE (this : JavaModEmitter)EmitBody(out : Ju.JavaFile);
  (** Create the assembler for a class file for this module. *)
    VAR index : INTEGER;
        objIx : INTEGER;
        proc  : Id.Procs;
        type  : Sy.Type;
        varId : Id.VarId;
        returned : BOOLEAN;
  BEGIN
    out.StartModClass(this.mod);
    FOR index := 0 TO this.mod.procs.tide-1 DO
     (*
      *  Create the mangled name for all non-forward procedures
      *)
      proc := this.mod.procs.a[index];
      IF (proc.kind = Id.conPrc) OR 
         (proc.kind = Id.conMth) THEN
        Ju.MkProcName(proc);
        Ju.RenumberLocals(proc);
      END;
    END;
   (* 
    *  Do all the fields (ie. static vars) 
    *)
    out.InitFields(this.mod.locals.tide);
    FOR index := 0 TO this.mod.locals.tide-1 DO
      varId := this.mod.locals.a[index](Id.VarId);
      out.EmitField(varId);  
    END;
	(*
	FOR index := 0 TO this.mod.procs.tide-1 DO
     (*
      *  Create the mangled name for all non-forward procedures
      *)
      proc := this.mod.procs.a[index];
      IF (proc.kind = Id.conPrc) OR 
         (proc.kind = Id.conMth) THEN
        Ju.MkProcName(proc);
        Ju.RenumberLocals(proc);
      END;
    END;
	*)
   (* 
    *  Do all the procs, including <init> and <clinit> 
    *)
    out.InitMethods(this.mod.procs.tide+3);
    out.ModNoArgInit();
    out.ClinitHead();
    out.InitVars(this.mod);
    IF this.mod.main THEN
     (*
      *   Emit <clinit>, and module body as main() 
      *)
      out.VoidTail();
      out.MainHead();
      this.EmitStat(this.mod.modBody, returned);
      IF returned THEN
        this.EmitStat(this.mod.modClose, returned);
      END;
      out.VoidTail();
    ELSE
     (*
      *   Emit single <clinit> incorporating module body
      *)
      this.EmitStat(this.mod.modBody, returned);
      out.VoidTail();
    END;
   (* 
    *  Emit all of the static procedures
    *)
    FOR index := 0 TO this.mod.procs.tide-1 DO
      proc := this.mod.procs.a[index];
      IF (proc.kind = Id.conPrc) &
         (proc.dfScp.kind = Id.modId) THEN this.EmitProc(proc) END;
    END;
   (* 
    *  And now, just in case exported types have been missed ...
	*  For example, if they are unreferenced in this module.
    *)
    FOR index := 0 TO this.mod.expRecs.tide-1 DO
      type := this.mod.expRecs.a[index];
      IF type.xName = NIL THEN 
	    WITH type : Ty.Record DO
		  Ju.MkRecName(type);
		| type : Ty.Procedure DO
		  Ju.MkProcTypeName(type);
		END;
	  END;
    END;
  END EmitBody;

(* ============================================================ *)

  PROCEDURE (this : JavaEmitter)Emit*();
  (** Create the assembler for a class file for this module. *)
  VAR fileName  : FileNames.NameString;
      cf : ClassUtil.ClassFile;
      jf : JsmnUtil.JsmnFile;
  BEGIN
   (*
    *  Create the classFile structure, and open the output file.
    *  The default for the JVM target is to write a class file
    *  directly.  The -jasmin option writes a jasmin output file
    *  but does not call the (now unavailable) assembler.
    *)
    IF Cst.doCode & ~Cst.doJsmn THEN
      WITH this : JavaModEmitter DO
          L.ToStr(this.mod.xName, fileName);
      | this : JavaRecEmitter DO
          L.ToStr(this.recT.xName, fileName);
      | this : JavaProcTypeEmitter DO
          L.ToStr(this.prcT.xName, fileName);
      END;
      fileName := fileName + ".class";
      cf := ClassUtil.newClassFile(fileName);
      this.outF := cf;
    ELSE
      WITH this : JavaModEmitter DO
          Sy.getName.Of(this.mod, fileName);
      | this : JavaRecEmitter DO
          FileNames.StripUpToLast("/", this.recT.xName, fileName);
	  | this : JavaProcTypeEmitter DO
	      FileNames.StripUpToLast("/", this.prcT.xName, fileName);
      END;
      fileName := fileName + ".j";
      jf := JsmnUtil.newJsmnFile(fileName);
      this.outF := jf;
     (*
      *   Add this file to the list to assemble
      *)
      LitValue.AppendCharOpen(asmList, LitValue.strToCharOpen(fileName));
    END;
    IF this.outF = NIL THEN
      CPascalS.SemError.Report(177, 0, 0);
      Error.WriteString("Cannot create out-file <" + fileName + ">");
      Error.WriteLn;
      RETURN;
    ELSE
      IF Cst.verbose THEN Cst.Message("Created "+ fileName) END;
      this.outF.Header(Cst.srcNam);
      this.EmitBody(this.outF);
      this.outF.Dump();
    END; 
  END Emit;

(* ============================================================ *)
(*		Shared code-emission methods			*)
(* ============================================================ *)

  PROCEDURE (e : JavaEmitter)EmitProc(proc : Id.Procs),NEW;
    VAR out  : Ju.JavaFile;
        live : BOOLEAN;
        retn : Sy.Type;
        indx : INTEGER;
        nest : Id.Procs;
        procName : FileNames.NameString;
  BEGIN
   (*
    *   Recursively emit nested procedures first.
    *)
    FOR indx := 0 TO proc.nestPs.tide-1 DO 
      nest := proc.nestPs.a[indx];
      IF nest.kind = Id.conPrc THEN e.EmitProc(nest) END;
    END;
    out := e.outF;
    out.theP := proc;
    out.StartProc(proc);
   (*
    *  Output the body if not ABSTRACT
    *)
    IF ~out.isAbstract() THEN
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
      *  through ending, copy OUT params and return.
      *)
      IF live & proc.type.isProperProcType() THEN
        out.FixOutPars(proc, retn);
        out.Return(retn);
      END;
      IF proc.rescue # NIL THEN
        out.Catch(proc);
        e.EmitStat(proc.rescue, live);
        IF live & proc.type.isProperProcType() THEN
          out.FixOutPars(proc, retn);
          out.Return(retn);
        END;
      END;
    END;
    out.EndProc();
  END EmitProc;

(* ============================================================ *)
(*		    Expression Handling Methods			*)
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

(* ============================================================ *)

  PROCEDURE (e : JavaEmitter)UbyteClear(),NEW;
    VAR out  : Ju.JavaFile;
  BEGIN
    out  := e.outF;
    out.PushInt(255);
    out.Code(Jvm.opc_iand);
  END UbyteClear;

(* ============================================================ *)

  PROCEDURE (e : JavaEmitter)newLeaf(rd : INTEGER; tp : Sy.Type) : Xp.IdLeaf,NEW;
    VAR id : Id.LocId;
  BEGIN
    id := Id.newLocId();
    id.varOrd := rd;
    id.type   := tp;
    id.dfScp  := e.outF.getScope();
    RETURN Xp.mkIdLeaf(id);
  END newLeaf;

(* ============================================================ *)

   PROCEDURE RevTest(tst : INTEGER) : INTEGER;
   BEGIN
     CASE tst OF
     | Xp.equal  : RETURN Xp.notEq;
     | Xp.notEq  : RETURN Xp.equal;
     | Xp.greT   : RETURN Xp.lessEq;
     | Xp.lessT  : RETURN Xp.greEq;
     | Xp.greEq  : RETURN Xp.lessT;
     | Xp.lessEq : RETURN Xp.greT;
     END;
   END RevTest;

(* ============================================================ *)

  PROCEDURE (e : JavaEmitter)DoCmp(cmpE : INTEGER;
                                   tLab : Ju.Label;
                                   type : Sy.Type),NEW;
   (**  Compare two TOS elems and jump to tLab if true. *)
   (* ------------------------------------------------- *)
    VAR out  : Ju.JavaFile;
        code : INTEGER;
        tNum : INTEGER;
   (* ------------------------------------------------- *)
    PROCEDURE test(t : INTEGER) : INTEGER;
    BEGIN
      CASE t OF
      | Xp.greT   : RETURN Jvm.opc_ifgt;
      | Xp.greEq  : RETURN Jvm.opc_ifge;
      | Xp.notEq  : RETURN Jvm.opc_ifne;
      | Xp.lessEq : RETURN Jvm.opc_ifle;
      | Xp.lessT  : RETURN Jvm.opc_iflt;
      | Xp.equal  : RETURN Jvm.opc_ifeq;
      END;
    END test;
   (* ------------------------------------------------- *)
  BEGIN
    out  := e.outF;
    code := test(cmpE);		(* default code *)
    WITH type : Ty.Base DO
      tNum := type.tpOrd;
      CASE tNum OF
      | Ty.strN, Ty.sStrN : out.CallRTS(Ju.StrCmp,2,1);
      | Ty.realN : out.Code(Jvm.opc_dcmpl);
      | Ty.sReaN : out.Code(Jvm.opc_fcmpl);
      | Ty.lIntN : out.Code(Jvm.opc_lcmp);
      | Ty.anyRec, Ty.anyPtr :
          CASE cmpE OF
          | Xp.notEq  : code := Jvm.opc_if_acmpne;
          | Xp.equal  : code := Jvm.opc_if_acmpeq;
          END;
      ELSE (* Ty.boolN,Ty.sChrN,Ty.charN,Ty.byteN,Ty.sIntN,Ty.intN,Ty.setN *)
          CASE cmpE OF
          | Xp.greT   : code := Jvm.opc_if_icmpgt; (* override default code *)
          | Xp.greEq  : code := Jvm.opc_if_icmpge;
          | Xp.notEq  : code := Jvm.opc_if_icmpne;
          | Xp.lessEq : code := Jvm.opc_if_icmple;
          | Xp.lessT  : code := Jvm.opc_if_icmplt;
          | Xp.equal  : code := Jvm.opc_if_icmpeq;
          END;
      END;
    ELSE  (* This must be a reference or string comparison *)
      IF type.isCharArrayType() THEN out.CallRTS(Ju.StrCmp,2,1);
      ELSIF cmpE = Xp.equal     THEN code := Jvm.opc_if_acmpeq;
      ELSIF cmpE = Xp.notEq     THEN code := Jvm.opc_if_acmpne;
      END;
    END;
    out.CodeLb(code, tLab);
  END DoCmp;

(* ================= old code =========================== *
 *  IF type IS Ty.Base THEN
 *    tNum := type(Ty.Base).tpOrd;
 *    IF (tNum = Ty.strN) OR (tNum = Ty.sStrN) THEN
 *      out.CallRTS(Ju.StrCmp,2,1);
 *    ELSIF tNum = Ty.realN THEN
 *      out.Code(Jvm.opc_dcmpl);
 *    ELSIF tNum = Ty.sReaN THEN
 *      out.Code(Jvm.opc_fcmpl);
 *    ELSIF tNum = Ty.lIntN THEN
 *      out.Code(Jvm.opc_lcmp);
 *    ELSE		(* Common, integer cases use separate instructions  *)
 *      CASE cmpE OF
 *      | Xp.greT   : code := Jvm.opc_if_icmpgt;	(* override default *)
 *      | Xp.greEq  : code := Jvm.opc_if_icmpge;
 *      | Xp.notEq  : code := Jvm.opc_if_icmpne;
 *      | Xp.lessEq : code := Jvm.opc_if_icmple;
 *      | Xp.lessT  : code := Jvm.opc_if_icmplt;
 *      | Xp.equal  : code := Jvm.opc_if_icmpeq;
 *      END;
 *    END;
 *  ELSE  (* This must be a reference or string comparison *)
 *    IF type.isCharArrayType() THEN
 *      out.CallRTS(Ju.StrCmp,2,1);
 *    ELSIF cmpE = Xp.equal THEN
 *      code := Jvm.opc_if_acmpeq;
 *    ELSIF cmpE = Xp.notEq THEN
 *      code := Jvm.opc_if_acmpne;
 *    END;
 *  END;
 *  out.CodeLb(code, tLab);
 *END DoCmp;
 * ================= old code =========================== *)

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)SetCmp(lOp,rOp : Sy.Expr; 
                                    theLabl : Ju.Label;
                                    theTest : INTEGER),NEW;
    VAR out : Ju.JavaFile;
        l,r : INTEGER;
        xit : Ju.Label;
  BEGIN
    out := e.outF;
    e.PushValue(lOp, Bi.setTp);
    CASE theTest OF
    (* ---------------------------------- *)
    | Xp.equal: 
        e.PushValue(rOp, Bi.setTp);
        out.CodeLb(Jvm.opc_if_icmpeq, theLabl);
    (* ---------------------------------- *)
    | Xp.notEq : 
        e.PushValue(rOp, Bi.setTp);
        out.CodeLb(Jvm.opc_if_icmpne, theLabl);
    (* ---------------------------------- *)
    | Xp.greEq, Xp.lessEq : 
       (*
        *   The semantics are implemented by the identities
        *
        *   (L <= R) == (L AND R = L)
        *   (L >= R) == (L OR  R = L)
        *)
        out.Code(Jvm.opc_dup);
        e.PushValue(rOp, Bi.setTp);
        IF theTest = Xp.greEq THEN
          out.Code(Jvm.opc_ior);
        ELSE
          out.Code(Jvm.opc_iand);
        END;
        out.CodeLb(Jvm.opc_if_icmpeq, theLabl);
    (* ---------------------------------- *)
    | Xp.greT, Xp.lessT : 
       (*
        *   The semantics are implemented by the identities
        *
        *   (L < R) == (L AND R = L) AND NOT (L = R)
        *   (L > R) == (L OR  R = L) AND NOT (L = R)
        *)
        l := out.newLocal();
        r := out.newLocal();
        xit := out.newLabel();
        out.Code(Jvm.opc_dup);            (* ... L,L       *)
        out.Code(Jvm.opc_dup);            (* ... L,L,L     *)
        out.StoreLocal(l, Bi.setTp);      (* ... L,L,      *)
        e.PushValue(rOp, Bi.setTp);       (* ... L,L,R     *)
        out.Code(Jvm.opc_dup);            (* ... L,L,R,R   *)
        out.StoreLocal(r, Bi.setTp);      (* ... L,L,R     *)
        IF theTest = Xp.greT THEN        
          out.Code(Jvm.opc_ior);          (* ... L,LvR     *)
        ELSE
          out.Code(Jvm.opc_iand);         (* ... L,L^R     *)
        END; 
        out.CodeLb(Jvm.opc_if_icmpne, xit);
        out.LoadLocal(l, Bi.setTp);       (* ... L@R,l     *)
        out.LoadLocal(r, Bi.setTp);       (* ... L@R,l,r   *)
        out.CodeLb(Jvm.opc_if_icmpne, theLabl);
        out.ReleaseLocal(r);
        out.ReleaseLocal(l);
        out.DefLab(xit);
    END;
  END SetCmp;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)BinCmp(exp : Sy.Expr; 
                                    tst : INTEGER;
                                    rev : BOOLEAN;  (* reverse sense *)
                                    lab : Ju.Label),NEW;
    VAR binOp : Xp.BinaryX;
        lType : Sy.Type;
  BEGIN
    binOp := exp(Xp.BinaryX);
    lType := binOp.lKid.type;
    IF rev THEN tst := RevTest(tst) END;
    IF  lType = Bi.setTp THEN (* only partially ordered *)
      e.SetCmp(binOp.lKid, binOp.rKid, lab, tst);
    ELSE                      (* a totally ordered type *)
      e.PushValue(binOp.lKid, lType);
      IF isStrExp(binOp.lKid) THEN
        e.outF.CallRTS(Ju.StrToChrOpen,1,1);
      END;
      e.PushValue(binOp.rKid, binOp.rKid.type);
      IF isStrExp(binOp.rKid) THEN
        e.outF.CallRTS(Ju.StrToChrOpen,1,1);
      END;
      e.DoCmp(tst, lab, lType);
    END;
  END BinCmp;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)FallTrue(exp : Sy.Expr; fLb : Ju.Label),NEW;
   (** Evaluate exp, fall through if true, jump to fLab otherwise *)
    VAR binOp : Xp.BinaryX;
        label : Ju.Label;
        out   : Ju.JavaFile;
  BEGIN
    out := e.outF;
    CASE exp.kind OF
    | Xp.tBool :				(* just do nothing *)
    | Xp.fBool : 
        out.CodeLb(Jvm.opc_goto, fLb);
    | Xp.blNot :
        e.FallFalse(exp(Xp.UnaryX).kid, fLb);
    | Xp.greT, Xp.greEq, Xp.notEq, Xp.lessEq, Xp.lessT, Xp.equal :
        e.BinCmp(exp, exp.kind, TRUE, fLb);
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
        out.CodeT(Jvm.opc_instanceof, binOp.rKid(Xp.IdLeaf).ident.type);
        out.CodeLb(Jvm.opc_ifeq, fLb);
    | Xp.inOp :
        binOp := exp(Xp.BinaryX);
        out.Code(Jvm.opc_iconst_1);
        e.PushValue(binOp.lKid, binOp.lKid.type);
        out.Code(Jvm.opc_ishl);
        out.Code(Jvm.opc_dup);
        e.PushValue(binOp.rKid, binOp.rKid.type);
        out.Code(Jvm.opc_iand);
        out.CodeLb(Jvm.opc_if_icmpne, fLb);
    ELSE (* Xp.fnCll, Xp.qualId, Xp.index, Xp.selct  *)
      e.PushValue(exp, exp.type);		(* boolean variable *)
      out.CodeLb(Jvm.opc_ifeq, fLb);
    END;
  END FallTrue;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)FallFalse(exp : Sy.Expr; tLb : Ju.Label),NEW;
   (** Evaluate exp, fall through if false, jump to tLb otherwise *)
    VAR binOp : Xp.BinaryX;
        label : Ju.Label;
        out   : Ju.JavaFile;
  BEGIN
    out := e.outF;
    CASE exp.kind OF
    | Xp.fBool :				(* just do nothing *)
    | Xp.tBool : 
        out.CodeLb(Jvm.opc_goto, tLb);
    | Xp.blNot :
        e.FallTrue(exp(Xp.UnaryX).kid, tLb);
    | Xp.greT, Xp.greEq, Xp.notEq, Xp.lessEq, Xp.lessT, Xp.equal :
        e.BinCmp(exp, exp.kind, FALSE, tLb);
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
        out.CodeT(Jvm.opc_instanceof, binOp.rKid(Xp.IdLeaf).ident.type);
        out.CodeLb(Jvm.opc_ifne, tLb);
    | Xp.inOp :
        binOp := exp(Xp.BinaryX);
        out.Code(Jvm.opc_iconst_1);
        e.PushValue(binOp.lKid, binOp.lKid.type);
        out.Code(Jvm.opc_ishl);
        out.Code(Jvm.opc_dup);
        e.PushValue(binOp.rKid, binOp.rKid.type);
        out.Code(Jvm.opc_iand);
        out.CodeLb(Jvm.opc_if_icmpeq, tLb);
    ELSE (* Xp.fnCll, Xp.qualId, Xp.index, Xp.selct  *)
      e.PushValue(exp, exp.type);		(* boolean variable *)
      out.CodeLb(Jvm.opc_ifne, tLb);
    END;
  END FallFalse;

(* ============================================================ *)

  PROCEDURE (e : JavaEmitter)PushUnary(exp : Xp.UnaryX; dst : Sy.Type),NEW;
    VAR dNum : INTEGER;
        code : INTEGER;
        labl : Ju.Label;
        out  : Ju.JavaFile;
  (* ------------------------------------- *)
    PROCEDURE MkBox(emt : JavaEmitter; exp : Xp.UnaryX);
      VAR dst : Sy.Type;
          src : Sy.Type;
          out : Ju.JavaFile;
    BEGIN
      out := emt.outF;
      src := exp.kid.type;
      dst := exp.type(Ty.Pointer).boundTp;
      IF isStrExp(exp.kid) THEN
        emt.PushValue(exp.kid, src);
        out.CallRTS(Ju.StrToChrOpen,1,1);
      ELSE 
        emt.ValueCopy(exp.kid, dst);
      END;
    END MkBox;
  (* ------------------------------------- *)
  BEGIN
    IF exp.kind = Xp.mkBox THEN MkBox(e,exp); RETURN END; (* PRE-EMPTIVE RET *)
    e.PushValue(exp.kid, exp.kid.type);
    out := e.outF;
    CASE exp.kind OF
    | Xp.mkStr, Xp.deref : (* skip *)
    | Xp.tCheck :
  	out.CodeT(Jvm.opc_checkcast, exp.type.boundRecTp()(Ty.Record));
    | Xp.mkNStr :
        IF ~isStrExp(exp.kid) THEN 
          out.CallRTS(Ju.ChrsToStr,1,1);
        END;
    | Xp.strChk :			(* Some range checks required *)
        out.Code(Jvm.opc_dup);
        out.CallRTS(Ju.StrCheck,1,0);	
    | Xp.compl :
        out.Code(Jvm.opc_iconst_m1);
        out.Code(Jvm.opc_ixor);
    | Xp.neg :
        dNum := dst(Ty.Base).tpOrd;
        IF    dNum = Ty.realN THEN
          code := Jvm.opc_dneg;
        ELSIF dNum = Ty.sReaN THEN
          code := Jvm.opc_fneg;
        ELSIF dNum = Ty.lIntN THEN
          code := Jvm.opc_lneg;
        ELSE 				(* all INTEGER cases *)
          code := Jvm.opc_ineg;
        END;
        out.Code(code);
    | Xp.absVl :
        dNum := dst(Ty.Base).tpOrd;
        IF    dNum = Ty.realN THEN
          out.Code(Jvm.opc_dup2);
          out.Code(Jvm.opc_dconst_0);
          out.Code(Jvm.opc_dcmpg);
          code := Jvm.opc_dneg;
        ELSIF dNum = Ty.sReaN THEN
          out.Code(Jvm.opc_dup);
          out.Code(Jvm.opc_fconst_0);
          out.Code(Jvm.opc_fcmpg);
          code := Jvm.opc_fneg;
        ELSIF dNum = Ty.lIntN THEN
          out.Code(Jvm.opc_dup2);
          out.Code(Jvm.opc_lconst_0);
          out.Code(Jvm.opc_lcmp);
          code := Jvm.opc_lneg;
        ELSE 				(* all INTEGER cases *)
          out.Code(Jvm.opc_dup);
          code := Jvm.opc_ineg; 
        END;
        labl := out.newLabel();
        out.CodeLb(Jvm.opc_ifge, labl);	(* NOT ifle, Aug2001 *)
        out.Code(code);
        out.DefLab(labl);
    | Xp.entVl :
        dNum := dst(Ty.Base).tpOrd;
        IF dNum = Ty.sReaN THEN out.Code(Jvm.opc_f2d) END;
         (*
         // We _could_ check if the value is >= 0.0, and 
         // skip the call in that case, falling through
         // into the round-to-zero mode opc_d2l.
         *)
          out.CallRTS(Ju.DFloor,1,1);	
          out.Code(Jvm.opc_d2l);
    | Xp.capCh :
        out.CallRTS(Ju.ToUpper,1,1);	
    | Xp.blNot :
        out.Code(Jvm.opc_iconst_1);
        out.Code(Jvm.opc_ixor); 
    | Xp.strLen :
        out.CallRTS(Ju.StrLen,1,1);	
    | Xp.oddTst :
        IF exp.kid.type.isLongType() THEN out.Code(Jvm.opc_l2i) END;
        out.Code(Jvm.opc_iconst_1);
        out.Code(Jvm.opc_iand); 
    | Xp.getTp :
        out.CallGetClass();
    END;
  END PushUnary;

(* ============================================================ *)

  PROCEDURE (e : JavaEmitter)PushVecElemHandle(lOp,rOp : Sy.Expr),NEW;
    VAR vTp : Ty.Vector;
        eTp : Sy.Type;
        tde : INTEGER;
        out : Ju.JavaFile;
        xLb : Ju.Label;
  BEGIN
    out := e.outF;
    vTp := lOp.type(Ty.Vector);
    eTp := vTp.elemTp;
    tde := out.newLocal();
    xLb := out.newLabel();

    e.PushValue(lOp, eTp);              (* vRef ...                *)
    out.Code(Jvm.opc_dup);              (* vRef, vRef ...          *)
    out.GetVecLen();                    (* tide, vRef ...          *)
    out.StoreLocal(tde, Bi.intTp);      (* vRef ...                *)

    e.outF.GetVecArr(eTp);              (* arr ...                 *)
    e.PushValue(rOp, Bi.intTp);         (* idx, arr ...            *)
    out.Code(Jvm.opc_dup);              (* idx, idx, arr ...       *)
    out.LoadLocal(tde, Bi.intTp);       (* tide, idx, idx, arr ... *)

    out.CodeLb(Jvm.opc_if_icmplt, xLb);
    out.Trap("Vector index out of bounds");

    out.DefLab(xLb);                    (* idx, arr ...            *)
    out.ReleaseLocal(tde);
  END PushVecElemHandle;

(* ============================================================ *)

  (* Assert: lOp is already pushed. *)
  PROCEDURE RotateInt(e : JavaEmitter; lOp : Sy.Expr; rOp : Sy.Expr);
    VAR
      temp, ixSv : INTEGER; (* local vars    *)
	  indx : INTEGER;       (* literal index *)
	  rtSz : INTEGER;
	  out  : Ju.JavaFile;
  BEGIN
    out := e.outF;
    IF lOp.type = Bi.sIntTp THEN 
      rtSz := 16;
	  out.ConvertDn(Bi.intTp, Bi.charTp);
	ELSIF (lOp.type = Bi.byteTp) OR (lOp.type = Bi.uBytTp) THEN
	  rtSz := 8;
	  out.ConvertDn(Bi.intTp, Bi.uBytTp);
	ELSE
	  rtSz := 32;
	END;
	temp := out.newLocal();          
    IF rOp.kind = Xp.numLt THEN
          indx := intValue(rOp) MOD rtSz;
      IF indx = 0 THEN  (* skip *)
	  ELSE (* 
	    *  Rotation is achieved by means of the identity
	    *  Forall 0 <= n < rtSz: 
	    *    ROT(a, n) = LSH(a,n) bitwiseOR LSH(a,n-rtSz);
	    *)
	    out.Code(Jvm.opc_dup);
		out.StoreLocal(temp, Bi.intTp);
		out.PushInt(indx);
		out.Code(Jvm.opc_ishl);
		out.LoadLocal(temp, Bi.intTp);
		out.PushInt(rtSz - indx);
		out.Code(Jvm.opc_iushr);
		out.Code(Jvm.opc_ior);
		out.ConvertDn(Bi.intTp, lOp.type);
      END;
    ELSE
	  ixSv := out.newLocal();          
	  out.Code(Jvm.opc_dup);          (* TOS: lOp, lOp, ...             *)
	  out.StoreLocal(temp, Bi.intTp); (* TOS: lOp, ...                  *)
      e.PushValue(rOp, rOp.type);     (* TOS: rOp, lOp, ...             *)
	  out.PushInt(rtSz-1);            (* TOS: 31, rOp, lOp, ...         *)
	  out.Code(Jvm.opc_iand);         (* TOS: rOp', lOp, ...            *)
	  out.Code(Jvm.opc_dup);          (* TOS: rOp', rOp', lOp, ...      *)
	  out.StoreLocal(ixSv, Bi.intTp); (* TOS: rOp', lOp, ...            *)
	  out.Code(Jvm.opc_ishl);         (* TOS: lRz, ...                  *)
	  out.LoadLocal(temp, Bi.intTp);  (* TOS: lOp, lRz, ...             *)
	  out.PushInt(rtSz);              (* TOS: 32, lOp, lRz, ...         *)
	  out.LoadLocal(ixSv, Bi.intTp);  (* TOS: rOp',32, lOp, lRz, ...    *)
	  out.Code(Jvm.opc_isub);         (* TOS: rOp'', lOp, lRz, ...      *)
	  out.Code(Jvm.opc_iushr);        (* TOS: rRz, lRz, ...             *)
	  out.Code(Jvm.opc_ior);          (* TOS: ROT(lOp, rOp), ...        *)
	  out.ReleaseLocal(ixSv);
	  out.ConvertDn(Bi.intTp, lOp.type);
	END;
	out.ReleaseLocal(temp);
  END RotateInt;

(* ============================================================ *)

  (* Assert: lOp is already pushed. *)
  PROCEDURE RotateLong(e : JavaEmitter; lOp : Sy.Expr; rOp : Sy.Expr);
    VAR
	  tmp1,tmp2, ixSv : INTEGER; (* local vars    *)
	  indx : INTEGER;            (* literal index *)
	  out  : Ju.JavaFile;
  BEGIN
    out := e.outF;
	tmp1 := out.newLocal();      (* Pair of locals *)     
	tmp2 := out.newLocal();          
    IF rOp.kind = Xp.numLt THEN
      indx := intValue(rOp) MOD 64;
      IF indx = 0 THEN  (* skip *)
	  ELSE (* 
		*  Rotation is achieved by means of the identity
		*  Forall 0 <= n < rtSz: 
		*    ROT(a, n) = LSH(a,n) bitwiseOR LSH(a,n-rtSz);
		*)
		out.Code(Jvm.opc_dup2);
		out.StoreLocal(tmp1, Bi.lIntTp);
		out.PushInt(indx);
		out.Code(Jvm.opc_lshl);
		out.LoadLocal(tmp1, Bi.lIntTp);
		out.PushInt(64 - indx);
		out.Code(Jvm.opc_lushr);
		out.Code(Jvm.opc_lor);
      END;
    ELSE
	  ixSv := out.newLocal();          
	  out.Code(Jvm.opc_dup2);            (* TOS: lOp, lOp, ...             *)
	  out.StoreLocal(tmp1, Bi.lIntTp);   (* TOS: lOp, ...                  *)
      e.PushValue(rOp, rOp.type);        (* TOS: rOp, lOp, ...             *)
	  out.PushInt(63);                   (* TOS: 31, rOp, lOp, ...         *)
	  out.Code(Jvm.opc_iand);            (* TOS: rOp', lOp, ...            *)
	  out.Code(Jvm.opc_dup);             (* TOS: rOp', rOp', lOp, ...      *)
	  out.StoreLocal(ixSv, Bi.intTp);    (* TOS: rOp', lOp, ...            *)
	  out.Code(Jvm.opc_lshl);            (* TOS: lRz, ...                  *)
	  out.LoadLocal(tmp1, Bi.lIntTp);    (* TOS: lOp, lRz, ...             *)
	  out.PushInt(64);                   (* TOS: 32, lOp, lRz, ...         *)
	  out.LoadLocal(ixSv, Bi.intTp);     (* TOS: rOp',32, lOp, lRz, ...    *)
	  out.Code(Jvm.opc_isub);            (* TOS: rOp'', lOp, lRz, ...      *)
	  out.Code(Jvm.opc_lushr);           (* TOS: rRz, lRz, ...             *)
	  out.Code(Jvm.opc_lor);             (* TOS: ROT(lOp, rOp), ...        *)
	  out.ReleaseLocal(ixSv);
	END;
	out.ReleaseLocal(tmp2);
	out.ReleaseLocal(tmp1);
  END RotateLong;

(* ============================================================ *)

  PROCEDURE (e : JavaEmitter)PushBinary(exp : Xp.BinaryX; dst : Sy.Type),NEW;
    VAR out  : Ju.JavaFile;
        lOp  : Sy.Expr;
        rOp  : Sy.Expr;
 
        dNum : INTEGER;
        sNum : INTEGER;
        code : INTEGER;
        indx : INTEGER;
        rLit : LONGINT;
        exLb : Ju.Label;
        tpLb : Ju.Label;
        long : BOOLEAN;
    (* -------------------------------- *)
    PROCEDURE plusCode(tNnm : INTEGER) : INTEGER;
    BEGIN
      CASE tNnm OF
      | Ty.realN : RETURN Jvm.opc_dadd;
      | Ty.sReaN : RETURN Jvm.opc_fadd;
      | Ty.lIntN : RETURN Jvm.opc_ladd;
      ELSE         RETURN Jvm.opc_iadd;
      END;
    END plusCode;
    (* -------------------------------- *)
    PROCEDURE minusCode(tNnm : INTEGER) : INTEGER;
    BEGIN
      CASE tNnm OF
      | Ty.realN : RETURN Jvm.opc_dsub;
      | Ty.sReaN : RETURN Jvm.opc_fsub;
      | Ty.lIntN : RETURN Jvm.opc_lsub;
      ELSE         RETURN Jvm.opc_isub;
      END;
    END minusCode;
    (* -------------------------------- *)
    PROCEDURE multCode(tNnm : INTEGER) : INTEGER;
    BEGIN
      CASE tNnm OF
      | Ty.realN : RETURN Jvm.opc_dmul;
      | Ty.sReaN : RETURN Jvm.opc_fmul;
      | Ty.lIntN : RETURN Jvm.opc_lmul;
      ELSE         RETURN Jvm.opc_imul;
      END;
    END multCode;
    (* -------------------------------- *)
  BEGIN (* PushBinary *)
    out := e.outF;
    lOp := exp.lKid;
    rOp := exp.rKid;
    CASE exp.kind OF
    (* -------------------------------- *)
    | Xp.index :
        IF exp.lKid.type IS Ty.Vector THEN
          e.PushVecElemHandle(lOp, rOp);
          out.GetVecElement(dst);                 (* load the element   *)
(* 
 *        vTp := lOp.type(Ty.Vector);
 *        e.PushValue(lOp, lOp.type);             (* push array designator*)
 *        out.GetVecArr(vTp.elemTp);
 *        e.PushValue(rOp, rOp.type);             (* push index value   *)
 *        out.GetVecElement(vTp.elemTp);          (* load the element   *)
 *)
        ELSE
		  IF rOp.type = NIL THEN rOp.type := Bi.intTp END;
          e.PushValue(lOp, lOp.type);             (* push arr. desig.   *)
          e.PushValue(rOp, rOp.type);             (* push index value   *)
(*
 *        out.GetElement(dst);                    (* load the element   *)
 *)
          out.GetElement(lOp.type(Ty.Array).elemTp);  (* load the element   *)
          IF dst = Bi.uBytTp THEN e.UbyteClear() END;
        END;
    (* -------------------------------- *)
    | Xp.range :                                (* set i..j range ...	*)
       (* We want to create an integer with bits--	*)
       (*      [0...01...10...0]			*)
       (* MSB==31    j   i    0==LSB			*)
       (* One method is A				*)
       (* 1)   [0..010........0]  1 << (j+1)		*)
       (* 2)   [1..110........0]  negate(1)		*)
       (* 3)   [0.......010...0]  1 << i		*)
       (* 4)   [1.......110...0]  negate(3)		*)
       (* 5)   [0...01...10...0]  (2)xor(4)		*)
       (* Another method is B				*)
       (* 1)   [1.............1]  -1			*)
       (* 2)   [0...01........1]  (1) >>> (31-j)	*)
       (* 3)   [0........01...1]  (2) >> i		*)
       (* 4)   [0...01...10...0]  (3) << i		*)
       (* --------------------------------------------- *
        *      (*					*
        *	* Method A				*
        *	*)					*
        *	out.Code(Jvm.opc_iconst_1);		*
        *	out.Code(Jvm.opc_iconst_1);		*
        *	e.PushValue(rOp, Bi.intTp);		*
        *      (* Do unsigned less than 32 test here *)	*
        *	out.Code(Jvm.opc_iadd);			*
        *	out.Code(Jvm.opc_ishl);			*
        *	out.Code(Jvm.opc_ineg);			*
        *	out.Code(Jvm.opc_iconst_1);		*
        *	e.PushValue(lOp, Bi.intTp);		*
        *      (* Do unsigned less than 32 test here *) *
        *	out.Code(Jvm.opc_ishl);			*
        *	out.Code(Jvm.opc_ineg);			*
        *	out.Code(Jvm.opc_ixor);			*
        * -------------------------------------------- *)
       (*
        * Method B
        *)
        IF rOp.kind = Xp.numLt THEN
          (* out.PushInt(-1 >>> (31 - intValue(rOp))); *)
          out.PushInt(ORD({0 .. intValue(rOp)}));
        ELSE
          out.Code(Jvm.opc_iconst_m1);
          out.PushInt(31);
          e.PushValue(rOp, Bi.intTp);
         (* Do unsigned less than 32 test here ...*)
          out.Code(Jvm.opc_isub);
          out.Code(Jvm.opc_iushr);
        END;
        IF lOp.kind = Xp.numLt THEN
          (* out.PushInt(-1 << intValue(lOp)); *)
          out.PushInt(ORD({intValue(lOp) .. 31}));
          out.Code(Jvm.opc_iand);
        ELSE
          e.PushValue(lOp, Bi.intTp);
         (* Do unsigned less than 32 test here ...*)
          out.Code(Jvm.opc_dup_x1);
          out.Code(Jvm.opc_ishr);
          out.Code(Jvm.opc_swap);
          out.Code(Jvm.opc_ishl);
        END;
    (* -------------------------------- *)
    | Xp.lenOf :
        e.PushValue(lOp, lOp.type);
        IF lOp.type IS Ty.Vector THEN
          out.GetVecLen();
        ELSE
          FOR indx := 0 TO intValue(rOp) - 1 DO
            out.Code(Jvm.opc_iconst_0);
            out.Code(Jvm.opc_aaload);
          END;
          out.Code(Jvm.opc_arraylength);
        END;
    (* -------------------------------- *)
    | Xp.maxOf, Xp.minOf :
        long := dst.isLongType();
        tpLb := out.newLabel();
        exLb := out.newLabel();
       (*
        * Push left operand, duplicate
        * stack is (top) lOp lOp ...
        *)
        e.PushValue(lOp, dst);
        IF long THEN 
          out.Code(Jvm.opc_dup2);
        ELSE 
          out.Code(Jvm.opc_dup);
        END;
       (*
        * Push right operand
        * stack is (top) rOp lOp lOp ...
        *)
        e.PushValue(rOp, dst); 
       (*
        * Duplicate and stow
        * stack is (top) rOp lOp rOp lOp ...
        *)
        IF long THEN 
          out.Code(Jvm.opc_dup2_x2);
        ELSE 
          out.Code(Jvm.opc_dup_x1);
        END;
       (*
        *  Compare two top items and jump
        *  stack is (top) rOp lOp ...
        *)
        IF exp.kind = Xp.maxOf THEN
          e.DoCmp(Xp.lessT, tpLb, dst);
        ELSE
          e.DoCmp(Xp.greT, tpLb, dst);
        END;
        indx := out.getDepth();
       (*
        *  Discard top item
        *  stack is (top) lOp ...
        *)
        IF long THEN 
          out.Code(Jvm.opc_pop2);
        ELSE 
          out.Code(Jvm.opc_pop);
        END;
        out.CodeLb(Jvm.opc_goto, exLb);
        out.DefLab(tpLb);
        out.setDepth(indx);
       (*
        *  Swap top two items and discard top
        *  stack is (top) rOp ...
        *)
        IF long THEN
          out.Code(Jvm.opc_dup2_x2);
          out.Code(Jvm.opc_pop2);
          out.Code(Jvm.opc_pop2);
        ELSE
          out.Code(Jvm.opc_swap);
          out.Code(Jvm.opc_pop);
        END;
        out.DefLab(exLb);
    (* -------------------------------- *)
    | Xp.bitAnd :
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
       (*
        *  A literal bitAnd might be a long
        *  operation, from a folded MOD.
        *)
        IF dst.isLongType() THEN
          out.Code(Jvm.opc_land);
        ELSE
          out.Code(Jvm.opc_iand);
        END;
    (* -------------------------------- *)
    | Xp.bitOr :
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        out.Code(Jvm.opc_ior);
    (* -------------------------------- *)
    | Xp.bitXor :
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        out.Code(Jvm.opc_ixor);
    (* -------------------------------- *)
    | Xp.plus :
        dNum := dst(Ty.Base).tpOrd;
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        out.Code(plusCode(dNum));
    (* -------------------------------- *)
    | Xp.minus :
        dNum := dst(Ty.Base).tpOrd;
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        out.Code(minusCode(dNum));
    (* -------------------------------- *)
    | Xp.mult :
        dNum := dst(Ty.Base).tpOrd;
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        out.Code(multCode(dNum));
    (* -------------------------------- *)
    | Xp.slash :
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        out.Code(Jvm.opc_ddiv);
    (* -------------------------------- *)
    | Xp.modOp :
        dNum := dst(Ty.Base).tpOrd;
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        IF dNum = Ty.lIntN THEN
          out.CallRTS(Ju.ModL,4,2);
        ELSE
          out.CallRTS(Ju.ModI,2,1);
        END;
    (* -------------------------------- *)
    | Xp.divOp :
(*
 *	dNum := dst(Ty.Base).tpOrd;
 *	e.PushValue(lOp, dst);
 *	e.PushValue(rOp, dst);
 *	IF dNum = Ty.lIntN THEN
 *	  out.CallRTS(Ju.DivL,4,2);
 *	ELSE
 *	  out.CallRTS(Ju.DivI,2,1);
 *	END;
 *
 *  Alternative, inline code ...
 *)
        e.PushValue(lOp, dst);
        long := dst(Ty.Base).tpOrd = Ty.lIntN;
        IF (rOp.kind = Xp.numLt) & (longValue(rOp) > 0) THEN
          tpLb := out.newLabel();
          IF long THEN
            rLit := longValue(rOp);
            out.Code(Jvm.opc_dup2);
            out.PushLong(0);
            out.Code(Jvm.opc_lcmp);
            out.CodeLb(Jvm.opc_ifge, tpLb);
            out.PushLong(rLit-1);
            out.Code(Jvm.opc_lsub);
            out.DefLab(tpLb);
            out.PushLong(rLit);
            out.Code(Jvm.opc_ldiv);
          ELSE
            indx := intValue(rOp);
            out.Code(Jvm.opc_dup);
            out.CodeLb(Jvm.opc_ifge, tpLb);
            out.PushInt(indx-1);
            out.Code(Jvm.opc_isub);
            out.DefLab(tpLb);
            out.PushInt(indx);
            out.Code(Jvm.opc_idiv);
          END;
        ELSE
          e.PushValue(rOp, dst);
          IF long THEN
            out.CallRTS(Ju.DivL,4,2);
          ELSE
            out.CallRTS(Ju.DivI,2,1);
          END;
        END;
    (* -------------------------------- *)
    | Xp.rem0op :
        dNum := dst(Ty.Base).tpOrd;
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        IF dNum = Ty.lIntN THEN
          out.Code(Jvm.opc_lrem);
        ELSE
          out.Code(Jvm.opc_irem);
        END;
    (* -------------------------------- *)
    | Xp.div0op :
        dNum := dst(Ty.Base).tpOrd;
        e.PushValue(lOp, dst);
        e.PushValue(rOp, dst);
        IF dNum = Ty.lIntN THEN
          out.Code(Jvm.opc_ldiv);
        ELSE
          out.Code(Jvm.opc_idiv);
        END;
    (* -------------------------------- *)
    | Xp.blOr,  Xp.blAnd,  Xp.greT,  Xp.greEq, 
      Xp.notEq, Xp.lessEq, Xp.lessT, Xp.equal, Xp.inOp :
        tpLb := out.newLabel();
        exLb := out.newLabel();
       (* 
        *  Jumping code is mandated for blOr and blAnd...
        * 
        *  For the Relational Ops this next seems crude, but
        *  appears to be the only way that the JVM allows
        *  construction of boolean values.
        *)
        e.FallTrue(exp, tpLb);
        out.Code(Jvm.opc_iconst_1);
        out.CodeLb(Jvm.opc_goto, exLb);
        out.DefLab(tpLb);
        out.Code(Jvm.opc_iconst_0);
        out.DefLab(exLb);
    (* -------------------------------- *)
    | Xp.isOp :
        e.PushValue(lOp, lOp.type);
        out.CodeT(Jvm.opc_instanceof, rOp(Xp.IdLeaf).ident.type);
    (* -------------------------------- *)
    | Xp.rotInt :
        e.PushValue(lOp, lOp.type);
		IF lOp.type = Bi.lIntTp THEN
		  RotateLong(e, lOp, rOp);
		ELSE
		  RotateInt(e, lOp, rOp);
		END;
    (* -------------------------------- *)
    | Xp.ashInt, Xp.lshInt :
(* FIXME: What about long types (here but not for .NET???) *)
        e.PushValue(lOp, lOp.type);
        IF rOp.kind = Xp.numLt THEN
          indx := intValue(rOp);
          IF indx = 0 THEN  (* skip *)
          ELSIF indx < 0 THEN
            out.PushInt(-indx);
           (*
            *  A literal, negative ASH might be
            *  a long operation from a folded DIV.
            *)
            IF dst.isLongType() THEN 
			  out.Code(Jvm.opc_lshr);
            ELSIF exp.kind = Xp.ashInt THEN
			  out.Code(Jvm.opc_ishr);
            ELSE
			  out.Code(Jvm.opc_iushr);
            END;
          ELSE
            out.PushInt(indx);
            out.Code(Jvm.opc_ishl);
          END;
        ELSE
          tpLb := out.newLabel();
          exLb := out.newLabel();
         (*
          *  This is a variable shift. Do it the hard way.
          *  First, check the sign of the right hand op.
          *)
          e.PushValue(rOp, rOp.type);
          out.Code(Jvm.opc_dup);
          out.CodeLb(Jvm.opc_iflt, tpLb);
         (*
          *  Positive selector ==> shift left;
          *)
          out.Code(Jvm.opc_ishl);
          out.CodeLb(Jvm.opc_goto, exLb);
         (*
          *  Negative selector ==> shift right;
          *)
          out.DefLab(tpLb);
          out.Code(Jvm.opc_ineg);
		  IF exp.kind = Xp.ashInt THEN
            out.Code(Jvm.opc_ishr);
		  ELSE
            out.Code(Jvm.opc_iushr);
		  END;
          out.DefLab(exLb);
        END;
    (* -------------------------------- *)
    | Xp.strCat :
        e.PushValue(lOp, lOp.type);
        e.PushValue(rOp, rOp.type);
        IF (lOp.type = Bi.strTp) &
           (lOp.kind # Xp.mkStr) OR 
            lOp.type.isNativeStr() THEN
          IF (rOp.type = Bi.strTp) &
             (rOp.kind # Xp.mkStr) OR 
              rOp.type.isNativeStr() THEN
            out.CallRTS(Ju.StrCatSS,2,1);
          ELSE
            out.CallRTS(Ju.StrCatSA, 2, 1);
          END;
        ELSE
          IF (rOp.type = Bi.strTp) &
             (rOp.kind # Xp.mkStr) OR 
              rOp.type.isNativeStr() THEN
            out.CallRTS(Ju.StrCatAS, 2, 1);
          ELSE
            out.CallRTS(Ju.StrCatAA, 2, 1);
          END;
        END;
    (* -------------------------------- *)
    END;
  END PushBinary;

(* ============================================================ *)

  PROCEDURE (e : JavaEmitter)PushValue(exp : Sy.Expr; typ : Sy.Type),NEW;
    VAR out : Ju.JavaFile;
        rec : Ty.Record;
        ix  : INTEGER;
        elm : Sy.Expr;
        emt : BOOLEAN;		(* ==> more than one set element expr *)
  BEGIN
    out := e.outF;
    WITH exp : Xp.IdLeaf DO
        IF exp.isProcLit() THEN
          out.MakeAndPushProcLitValue(exp, typ(Ty.Procedure));
        ELSIF exp.kind = Xp.typOf THEN
          out.LoadType(exp.ident);
        ELSE
          out.GetVar(exp.ident);
          IF typ = Bi.uBytTp THEN e.UbyteClear() END;
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
            out.Code(Jvm.opc_ishl);
          END;
          IF ~emt THEN out.Code(Jvm.opc_ior) END;
          emt := FALSE;
        END;
       (*
        *   If neither of the above emitted anything, emit zero!
        *)
        IF emt THEN out.Code(Jvm.opc_iconst_0) END;
    | exp : Xp.LeafX DO
        CASE exp.kind OF
        | Xp.tBool  : out.Code(Jvm.opc_iconst_1);
        | Xp.fBool  : out.Code(Jvm.opc_iconst_0);
        | Xp.nilLt  : out.Code(Jvm.opc_aconst_null);
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
              out.GetVar(Cst.dblInf);
            ELSE
              out.GetVar(Cst.fltInf);
            END;
        | Xp.nInfLt :
            IF typ = Bi.realTp THEN
              out.GetVar(Cst.dblNInf);
            ELSE
              out.GetVar(Cst.fltNInf);
            END;
        END;
    | exp : Xp.CallX DO
        e.PushCall(exp);
    | exp : Xp.IdentX DO
        e.PushValue(exp.kid, exp.kid.type);
        IF exp.kind = Xp.selct THEN
          rec := exp.kid.type(Ty.Record);
          out.PutGetF(Jvm.opc_getfield, rec, exp.ident(Id.FldId));
          IF typ = Bi.uBytTp THEN e.UbyteClear() END;
        ELSIF exp.kind = Xp.cvrtUp THEN
          out.ConvertUp(exp.kid.type, typ);
        ELSIF exp.kind = Xp.cvrtDn THEN
          out.ConvertDn(exp.kid.type, typ);
  	END;
    | exp : Xp.UnaryX DO
        e.PushUnary(exp, typ);
    | exp : Xp.BinaryX DO
        e.PushBinary(exp, typ);
    END;
  END PushValue;

(* ---------------------------------------------------- *)

  PROCEDURE SwapHandle(out : Ju.JavaFile; exp : Sy.Expr; long : BOOLEAN);
   (* Precondition: exp must be a variable designator 		*)
   (* A value is below a handle of 0,1,2 words. Swap val to top *)
    VAR hSiz : INTEGER;
        idnt : Sy.Idnt;
        type : Sy.Type;
  BEGIN
    type := exp.type;
    IF (type IS Ty.Record) OR 
       ((type IS Ty.Array) & (type.kind # Ty.vecTp)) THEN
      hSiz := 1;
    ELSE
      WITH exp : Xp.IdLeaf DO
        idnt := exp.ident;
        WITH idnt : Id.LocId DO
          IF Id.uplevA IN idnt.locAtt THEN hSiz := 1 ELSE hSiz := 0 END;
        ELSE 
          hSiz := 0;
        END;
      | exp : Xp.BinaryX DO
        hSiz := 2;
      ELSE
        hSiz := 1;
      END;                          (* -------------------- *)
    END;                            (* -------------------- *)
                                    (*  Before ==>  After	*)
    IF hSiz = 1 THEN				(* -------------------- *)
      IF ~long THEN 				(* [hndl]  ==>	[valu]	*)
        out.Code(Jvm.opc_swap);	    (* [valu]	[hndl]	*)
                                    (* -------------------- *)
      ELSE                          (* [hndl]  ==>	[val2]	*)
        out.Code(Jvm.opc_dup_x2);   (* [val2]	[val1]	*)
        out.Code(Jvm.opc_pop);      (* [val1]	[hndl]	*)
      END;	                        (* -------------------- *)
    ELSIF hSiz = 2 THEN				(* -------------------- *)
      IF ~long THEN 				(* [indx]  ==>	[valu]	*)
        out.Code(Jvm.opc_dup2_x1);  (* [hndl]	[indx]	*)
        out.Code(Jvm.opc_pop2);	    (* [valu]	[hndl]	*)
                                    (* -------------------- *)
      ELSE                          (* [indx]  ==>	[val2]	*)
        out.Code(Jvm.opc_dup2_x2);  (* [hdnl]	[val1]	*)
        out.Code(Jvm.opc_pop2);     (* [val2]	[indx]	*)
      END;                          (* [val1]	[hndl]	*)
    (* ELSE nothing to do *)        (* -------------------- *)
    END;
  END SwapHandle;

(* -------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)PushHandle(exp : Sy.Expr; typ : Sy.Type),NEW;
   (* Precondition: exp must be a variable designator 		*)
    VAR idnt : Sy.Idnt;
  BEGIN
    ASSERT(exp.isVarDesig());
    IF (typ IS Ty.Record) OR ((typ IS Ty.Array) & (typ.kind # Ty.vecTp)) THEN
      e.PushValue(exp, typ);
    ELSE
      WITH exp : Xp.IdentX DO
          e.PushValue(exp.kid, exp.kid.type);
      | exp : Xp.BinaryX DO
          IF exp.lKid.type IS Ty.Vector THEN
            e.PushVecElemHandle(exp.lKid, exp.rKid);
(*
 *          e.PushValue(exp.lKid, exp.lKid.type);
 *          e.outF.GetVecArr(exp.lKid.type(Ty.Vector).elemTp);
 *          e.PushValue(exp.rKid, Bi.intTp);
 *)
          ELSE
            e.PushValue(exp.lKid, exp.lKid.type);
            e.PushValue(exp.rKid, Bi.intTp);
          END;
      | exp : Xp.IdLeaf DO
          idnt := exp.ident;
          WITH idnt : Id.LocId DO (* check if implemented inside XHR *)
            IF Id.uplevA IN idnt.locAtt THEN e.outF.XhrHandle(idnt) END;
          ELSE (* skip *)
          END;
      END;
    END;
  END PushHandle;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)ScalarAssign(exp : Sy.Expr),NEW;
    VAR out : Ju.JavaFile;
        rec : Ty.Record;
  BEGIN
    out := e.outF;
    WITH exp : Xp.IdLeaf DO
        (* stack has ... value, (top)	*)
        out.PutVar(exp.ident);
    | exp : Xp.IdentX DO
        (* stack has ... obj-ref, value, (top)	*)
        rec := exp.kid.type(Ty.Record);
        out.PutGetF(Jvm.opc_putfield, rec, exp.ident(Id.FldId));
    | exp : Xp.BinaryX DO
        (* stack has ... arr-ref, index, value, (top)	*)
        IF exp.lKid.type IS Ty.Vector THEN
          out.PutVecElement(exp.type);
        ELSE
          out.PutElement(exp.type);
        END;
    ELSE
      Console.WriteString("BAD SCALAR ASSIGN"); Console.WriteLn;
      exp.Diagnose(0);
      ASSERT(FALSE);
    END;
  END ScalarAssign;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)ValueCopy(act : Sy.Expr; fmT : Sy.Type),NEW;
    VAR out : Ju.JavaFile;
  BEGIN
   (*
    *  Copy this actual, where fmT is either an array or record.
    *)
    out := e.outF;
    WITH fmT : Ty.Record DO
      out.MkNewRecord(fmT);			(* (top) dst...		*)
      out.Code(Jvm.opc_dup);			(* (top) dst,dst...	*)
      e.PushValue(act, fmT);			(* (top) src,dst,dst...	*)
      out.ValRecCopy(fmT);			(* (top) dst...		*)
    | fmT : Ty.Array DO
     (*
      *  Array case: ordinary value copy
      *)
      IF fmT.length = 0 THEN 			(* open array case	*)
        e.PushValue(act, fmT);			(* (top) src...		*)
        out.Code(Jvm.opc_dup);			(* (top) src,src...	*)
        IF act.kind = Xp.mkStr THEN
          out.CallRTS(Ju.StrLP1,1,1);		(* (top) len,src...	*)
          out.Alloc1d(Bi.charTp);		(* (top) dst,src...	*)
        ELSE
          out.MkArrayCopy(fmT);			(* (top) dst,src...	*)
        END;
        out.Code(Jvm.opc_dup_x1);    		(* dst,src,dst...	*)
        out.Code(Jvm.opc_swap);			(* (top) src,dst,dst...	*)
      ELSE 					(* fixed array case 	*)
        out.MkNewFixedArray(fmT.elemTp, fmT.length);	
        out.Code(Jvm.opc_dup);			(* (top) dst,dst...	*)
        e.PushValue(act, fmT);			(* (top) src,dst,dst...	*)
      END;
      IF act.kind = Xp.mkStr THEN
        out.CallRTS(Ju.StrVal, 2, 0);		(* (top) dst...		*)
      ELSE
        out.ValArrCopy(fmT);			(* (top) dst...		*)
      END;
    ELSE
      e.PushValue(act, fmT);
    END;
  END ValueCopy;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)StringCopy(act : Sy.Expr; fmT : Ty.Array),NEW;
    VAR out : Ju.JavaFile;
  BEGIN
    out := e.outF;
    IF act.kind = Xp.mkStr THEN
      e.ValueCopy(act, fmT);
    ELSIF fmT.length = 0 THEN 		(* str passed to open array 	*)
      e.PushValue(act, fmT);
      out.CallRTS(Ju.StrToChrOpen,1,1);
    ELSE				(* str passed to fixed array	*)
      out.MkNewFixedArray(Bi.charTp, fmT.length);	
      out.Code(Jvm.opc_dup);
      e.PushValue(act, fmT); 
      out.CallRTS(Ju.StrToChrs,2,0);
    END;
  END StringCopy;

(* ============================================================ *)

  PROCEDURE (e : JavaEmitter)Invoke(exp : Sy.Expr; typ : Ty.Procedure),NEW;
    VAR code : INTEGER;
        prcI : Id.PrcId;
        mthI : Id.MthId;
  BEGIN
    IF exp.isProcVar() THEN
	  mthI := Ju.getProcVarInvoke(exp.type(Ty.Procedure));
	  code := Jvm.opc_invokevirtual;
	  e.outF.CallIT(code, mthI, typ);
	ELSE
      WITH exp : Xp.IdLeaf DO (* qualid *)
          prcI := exp.ident(Id.PrcId);
          IF prcI.kind = Id.ctorP THEN
            code := Jvm.opc_invokespecial;
          ELSE
            code := Jvm.opc_invokestatic;
          END;
          e.outF.CallIT(code, prcI, typ);
      | exp : Xp.IdentX DO (* selct *)
          mthI := exp.ident(Id.MthId);
          IF exp.kind = Xp.sprMrk THEN 
            code := Jvm.opc_invokespecial;
          ELSIF mthI.bndType.isInterfaceType() THEN
            code := Jvm.opc_invokeinterface;
          ELSE 
            code := Jvm.opc_invokevirtual;
          END;
          e.outF.CallIT(code, mthI, typ);
          IF Id.covar IN mthI.mthAtt THEN
            e.outF.CodeT(Jvm.opc_checkcast, typ.retType);
		  END;
      END;
    END;
  END Invoke;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)PushAndGetReturn(act : Sy.Expr;
                                              typ : Sy.Type;
                                          OUT ret : Sy.Expr),NEW;
   (* ----------------------------------------- *)
    VAR out   : Ju.JavaFile;
        local : INTEGER;
        recXp : Sy.Expr;
        array : Sy.Expr;
        index : Sy.Expr;
   (* ----------------------------------------- *)
    PROCEDURE simple(x : Sy.Expr) : BOOLEAN;
    BEGIN
      IF x.kind = Xp.deref THEN x := x(Xp.UnaryX).kid END;
      RETURN x IS Xp.LeafX;	(* IdLeaf or LeafX *)
    END simple;
   (* ----------------------------------------- *)
  BEGIN
   (*
    *  Assert: the expression is a (possibly complex)
    *  variable designator. Is some part of the handle
    *  worth saving? Note saving is mandatory for calls.
    *)
    out := e.outF;
    ret := act;
    WITH act : Xp.IdLeaf DO
       (*
        *  This is a simple variable. Result will be
        *  stored directly using the same expression.
        *)
        e.PushValue(act, typ); 
    | act : Xp.IdentX DO
        ASSERT(act.kind = Xp.selct);
       (*
        *  This is a field select.  If the handle is
        *  sufficiently complicated it will be saved.
        *)
        recXp := act.kid;
        e.PushValue(recXp, recXp.type);
        IF ~simple(recXp) THEN 
          local := out.newLocal();
          out.Code(Jvm.opc_dup);
          out.StoreLocal(local, NIL);
         (*
          *  The restore expression is a mutated 
          *  version of the original expression.
          *)
          act.kid := e.newLeaf(local, recXp.type);
          act.kid.type := recXp.type;
        END;
        out.PutGetF(Jvm.opc_getfield, 
        			recXp.type(Ty.Record), act.ident(Id.FldId));
    | act : Xp.BinaryX DO
        ASSERT(act.kind = Xp.index);
       (*
        *  This is an index select.  If the handle, or
        *  index (or both) are complicated they are saved.
        *)
        array := act.lKid;
        index := act.rKid;
        e.PushValue(array, array.type);
        IF simple(array) THEN		(* don't save handle  *)
          e.PushValue(index, Bi.intTp);
          IF ~simple(index) THEN	(* must save index    *)
            local := out.newLocal();
            out.Code(Jvm.opc_dup);
            out.StoreLocal(local, Bi.intTp); (* #### *)
            act.rKid := e.newLeaf(local, Bi.intTp);
            act.rKid.type := Bi.intTp;
          END;
        ELSE				(* must save handle   *)
          local := out.newLocal();
          out.Code(Jvm.opc_dup);
          out.StoreLocal(local, NIL);
          act.lKid := e.newLeaf(local, array.type);
          act.lKid.type := array.type;
          e.PushValue(index, Bi.intTp);
          IF ~simple(index) THEN	(* save index as well *)
            local := out.newLocal();
            out.Code(Jvm.opc_dup);
            out.StoreLocal(local, Bi.intTp); (* #### *)
            act.rKid := e.newLeaf(local, Bi.intTp);
            act.rKid.type := Bi.intTp;
          END;
        END;
        out.GetElement(typ);
    ELSE
      act.Diagnose(0); THROW("Bad PushAndGetReturn");
    END;
  END PushAndGetReturn;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)PushArg(act : Sy.Expr;
        			     frm : Id.ParId;
        			 VAR seq : Sy.ExprSeq),NEW;
   (* ------------------------- *)
    VAR idExp : Xp.IdentX;
        out   : Ju.JavaFile;
        local : INTEGER;
   (* ----------------------------------------- *)
    PROCEDURE boxNumber(exp : Sy.Expr) : INTEGER;
    BEGIN
      RETURN exp(Xp.IdLeaf).ident(Id.ParId).boxOrd;
    END boxNumber;
   (* ----------------------------------------- *)
    PROCEDURE boxedPar(exp : Sy.Expr) : BOOLEAN;
      VAR idnt : Sy.Idnt;
    BEGIN
      WITH exp : Xp.IdLeaf DO
        idnt := exp.ident;
        WITH idnt : Id.ParId DO
          RETURN (idnt.boxOrd # Ju.retMarker) & Ju.needsBox(idnt);
        ELSE 
          RETURN FALSE;
        END;
      ELSE 
        RETURN FALSE;
      END;
    END boxedPar;
   (* ----------------------------------------- *)
  BEGIN
    out := e.outF;
    IF Ju.needsBox(frm) THEN (* value is returned *)
      NEW(idExp);
      idExp.ident := frm;
      IF frm.parMod = Sy.out THEN (* no value push *)
        idExp.kid := act;
      ELSE
        e.PushAndGetReturn(act, frm.type, idExp.kid);
      END;
      IF frm.boxOrd # Ju.retMarker THEN 
       (* ==> out value but not in return slot *)
        frm.rtsTmp := out.newLocal();
        IF boxedPar(act) THEN
          out.LoadLocal(boxNumber(act), NIL);
        ELSE
          out.MkNewFixedArray(frm.type, 1);
        END;
        out.Code(Jvm.opc_dup);
        out.StoreLocal(frm.rtsTmp, NIL);
      END;
      Sy.AppendExpr(seq, idExp);
    ELSIF (frm.type IS Ty.Array) &
          ((act.type = Bi.strTp) OR act.type.isNativeStr()) THEN
      e.StringCopy(act, frm.type(Ty.Array));    (* special string case	*)
    ELSIF (frm.parMod = Sy.val) &
          ((frm.type IS Ty.Record) OR 
(* #### *)
           ((frm.type IS Ty.Array) & (frm.type.kind # Ty.vecTp))) THEN
(* #### *)
      e.ValueCopy(act, frm.type);
    ELSE
      e.PushValue(act, frm.type);
    END;
  END PushArg;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)CopyOut(exp : Sy.Expr; idD : Sy.Idnt),NEW;
    VAR out : Ju.JavaFile;
        par : Id.ParId;
  BEGIN
   (* Assert : this is an unboxed type *)
    out := e.outF;
    par := idD(Id.ParId);
    e.PushHandle(exp, par.type);
    IF par.boxOrd # Ju.retMarker THEN 
      out.LoadLocal(par.rtsTmp, NIL);
      out.Code(Jvm.opc_iconst_0);
      out.GetElement(par.type);
    ELSE (* result is below handle *)
      SwapHandle(out, exp, par.type.isLongType());
    END;
    e.ScalarAssign(exp);
  END CopyOut;

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

  PROCEDURE (e : JavaEmitter)PushCall(callX : Xp.CallX),NEW;
    VAR jFile : Ju.JavaFile;
        mark0 : INTEGER;	  (* local ord limit on entry *)
        tide0 : INTEGER;	  (* parameter tide on entry  *)
        index : INTEGER;	  (* just a counter for loops *)
		prVar : BOOLEAN;      (* Procedure variable call  *)
        formT : Ty.Procedure; (* formal type of procedure *)
        formP : Id.ParId;	  (* current formal parameter *)
        prExp : Sy.Expr;
        idExp : Xp.IdentX;
 (* ---------------------------------------------------- *)
    PROCEDURE CheckCall(expr : Sy.Expr; pTyp : Ty.Procedure);
      VAR prcI : Id.PrcId;
          mthI : Id.MthId;
		  idnt : Sy.Idnt;
    BEGIN
      WITH expr : Xp.IdLeaf DO (* qualid *)
	    idnt := expr.ident;
	    WITH idnt : Id.PrcId DO
            (* prcI := expr.ident(Id.PrcId); *)
            IF pTyp.xName = NIL THEN Ju.MkCallAttr(idnt, pTyp) END;
		| idnt : Id.AbVar DO
		    mthI := Ju.getProcVarInvoke(pTyp);
		    IF mthI.type.xName = NIL THEN Ju.MkCallAttr(mthI, mthI.type(Ty.Procedure)) END;
		END;
      | expr : Xp.IdentX DO (* selct *)
          mthI := expr.ident(Id.MthId);
          IF pTyp.xName = NIL THEN Ju.MkCallAttr(mthI, pTyp) END;
      END;
    END CheckCall;
 (* ---------------------------------------------------- *)
    PROCEDURE isNested(exp : Xp.IdLeaf) : BOOLEAN;
    BEGIN
      RETURN exp.ident(Id.PrcId).lxDepth > 0;
    END isNested;
 (* ---------------------------------------------------- *)
  BEGIN
    jFile := e.outF;
    mark0 := jFile.markTop();
    tide0 := callX.actuals.tide;
    prExp := callX.kid;
    formT := prExp.type(Ty.Procedure);
   (*
    *  Before we push any arguments, we must ensure that
    *  the formal-type name is computed, and the first
    *  out-value is moved to the return-slot, if possible.
    *)
	prVar := prExp.isProcVar();
	CheckCall(prExp, formT);
   (*
    *  We must first deal with the receiver if this is a method.
    *)
	IF prVar THEN
	  e.PushValue(prExp, prExp.type);
	  formT := Ju.getProcVarInvoke(formT).type(Ty.Procedure);
    ELSIF formT.receiver # NIL THEN
      idExp := prExp(Xp.IdentX);
      formP := idExp.ident(Id.MthId).rcvFrm;
      e.PushArg(idExp.kid, formP, callX.actuals);
    ELSE
      WITH prExp : Xp.IdLeaf DO
        IF prExp.ident.kind = Id.ctorP THEN
          jFile.CodeT(Jvm.opc_new, callX.type);
          jFile.Code(Jvm.opc_dup);
        ELSIF isNested(prExp) THEN
          jFile.PushStaticLink(prExp.ident(Id.Procs));
        END;
      ELSE (* skip *)
      END;
    END;
   (*
    *  We push the arguments from left to right.
    *  New IdentX expressions are appended to the argument 
    *  list to describe how to save any returned values.
    *)
    FOR index := 0 TO tide0-1 DO
      formP := formT.formals.a[index];
      e.PushArg(callX.actuals.a[index], formP, callX.actuals);
    END;
   (*
    *  Now emit the actual call instruction(s)
    *)
    e.Invoke(prExp, formT);
   (*
    *  Now we save any out arguments from the appended exprs.
    *)
    FOR index := tide0 TO callX.actuals.tide-1 DO
      prExp := callX.actuals.a[index]; 
      idExp := prExp(Xp.IdentX);
      e.CopyOut(idExp.kid, idExp.ident);
    END;
    jFile.ReleaseAll(mark0);
   (*
    *  Normally an CallX expression can only be evaluated once,
    *  so it does not matter if PushCall() is not idempotent.
    *  However, there is a pathological case if a predicate in a
    *  while loop has a function call with OUT formals. Since the
    *  GPCP method of laying out while loops evaluates the test 
    *  twice, the actual list must be reset to its original length.
    *)
    callX.actuals.ResetTo(tide0);
  END PushCall;

(* ---------------------------------------------------- *)

  PROCEDURE IncByLit(out : Ju.JavaFile; ord : INTEGER; inc : INTEGER);
  BEGIN
    IF (ord < 256) & (inc >= -128) & (inc <= 127) THEN
      out.CodeInc(ord, inc);
    ELSE
      out.LoadLocal(ord, Bi.intTp);
      out.PushInt(inc);
      out.Code(Jvm.opc_iadd);
      out.StoreLocal(ord, Bi.intTp);
    END;
  END IncByLit;

  PROCEDURE LitIncLocal(out : Ju.JavaFile; proc, vOrd, incr : INTEGER);
  BEGIN
    IF proc = Bi.decP THEN incr := -incr END;
    IncByLit(out, vOrd, incr);
  END LitIncLocal;

  (* ------------------------------------------ *)

  PROCEDURE (e : JavaEmitter)EmitStdProc(callX : Xp.CallX),NEW;
    CONST fMsg = "Assertion failure ";
    VAR out  : Ju.JavaFile;
        prId : Id.PrcId;
        flId : Id.FldId;
        pOrd : INTEGER;
        arg0 : Sy.Expr;
        argX : Sy.Expr;
        dstT : Sy.Type;
        idX0 : Sy.Idnt;
        argN : INTEGER;
        numL : INTEGER;
        incr : INTEGER;
        vRef : INTEGER;
        tide : INTEGER;
        okLb : Ju.Label;
        long : BOOLEAN;
        c    : INTEGER;
  BEGIN
    out  := e.outF;
    prId := callX.kid(Xp.IdLeaf).ident(Id.PrcId);
    arg0 := callX.actuals.a[0];	(* Always need at least one arg *)
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
          out.Trap(fMsg + LitValue.intToCharOpen(numL)^);
        ELSE
          numL := callX.token.lin;
          out.Trap(fMsg + Cst.srcNam +":"+ LitValue.intToCharOpen(numL)^);
        END;
        out.DefLab(okLb);
   (* --------------------------- *)
    | Bi.incP, Bi.decP :
        argX := callX.actuals.a[1];
        dstT := arg0.type;
        long := dstT.isLongType();
       (*
        *   Is this a local variable?
        *   There is a special instruction for incrementing
        *   word-sized local variables, provided the increment is 
        *   by a literal 8-bit amount, and local index is 8-bit.
        *)
        e.PushHandle(arg0, dstT);
        WITH arg0 : Xp.IdLeaf DO

            idX0 := arg0.ident;
            WITH idX0 : Id.LocId DO
              IF Id.uplevA IN idX0.locAtt THEN (* uplevel addressing case *)
                out.Code(Jvm.opc_dup);	(* handle is one slot only *)
                out.PutGetX(Jvm.opc_getfield, idX0);
              ELSIF (argX.kind = Xp.numLt) & ~long THEN (* PREMATURE EXIT *)
        	LitIncLocal(out, pOrd, idX0.varOrd, intValue(argX)); RETURN;
              ELSE
                out.LoadLocal(idX0.varOrd, dstT);
              END;
            ELSE
              e.PushValue(arg0, dstT);
            END;
        | arg0 : Xp.IdentX DO
            flId := arg0.ident(Id.FldId);
            out.Code(Jvm.opc_dup);	(* handle is one slot only *)
            out.PutGetF(Jvm.opc_getfield, arg0.kid.type(Ty.Record), flId);
        | arg0 : Xp.BinaryX DO
            out.Code(Jvm.opc_dup2);	(* handle is two slots here *)
            out.GetElement(dstT);
        END;
        e.PushValue(argX, dstT);
        IF long THEN
          IF pOrd = Bi.incP THEN c := Jvm.opc_ladd ELSE c := Jvm.opc_lsub END;
        ELSE
          IF pOrd = Bi.incP THEN c := Jvm.opc_iadd ELSE c := Jvm.opc_isub END;
        END;
        out.Code(c);
        e.ScalarAssign(arg0);
   (* --------------------------- *)
    | Bi.cutP :
       (* ------------------------------------- *
        *  Emit the code ...
        *     <push vector ref>
        *     dup
        *     getfield CP/CPJvec/VecBase/tide I // tide, vRef ...
        *     <push arg1>                       // arg1, tide, vRef ...
        *     dup_x1                            // arg1, tide, arg1, vRef ...
        *     if_icmpge okLb                    // arg1, vRef ...
        *     <throw index trap>
        *  okLb:                                // arg1, vRef ...
        *     putfield CP/CPJvec/VecBase/tide I // (empty)
        * ------------------------------------- *)
        argX := callX.actuals.a[1];
        okLb := out.newLabel();
        e.PushValue(arg0, arg0.type);
        out.Code(Jvm.opc_dup);
        out.GetVecLen();
        e.PushValue(argX, Bi.intTp);
        out.Code(Jvm.opc_dup_x1);

        out.Code(Jvm.opc_iconst_1); (* Chop the sign bit *)
        out.Code(Jvm.opc_ishl);     (* asserting, for    *)
        out.Code(Jvm.opc_iconst_1); (* correctness, that *)
        out.Code(Jvm.opc_iushr);    (* argX >> minInt.   *)

        out.CodeLb(Jvm.opc_if_icmpge, okLb);
        out.Trap("Vector index out of bounds");
        out.DefLab(okLb);
        out.PutVecLen();
   (* --------------------------- *)
    | Bi.apndP :  
       (* -------------------------------------- *
        *  Emit the code ...
        *     <push vector ref>
        *     dup
        *     astore R                           // vRef ...
        *     getfield CP/CPJvec/VecBase/tide I  // tide ...
        *     istore T
        *     aload  R                           // vRef ...
        *     getfield CP/CPJvec/VecXXX/elems [X // elems ...
        *     arraylength                        // aLen ...
        *     iload  T                           // tide, aLen ...
        *     if_icmpgt okLb                     
        *     aload  R                           // vRef
        *     <call expand()>
        *  okLb:
        *     aload  R                           // vRef
        *     getfield CP/CPJvec/VecXXX/elems [X // elems ...
        *     iload  T                           // tide, elems ...
        *     <push arg1>                        // arg1, tide, elems ...
        *     Xastore 
        *     aload  R                           // vRef ...
        *     iload  T                           // tide, vRef ...
        *     iconst_1                           // 1, tide, vRef ...
        *     iadd                               // tide', vRef ...
        *     putfield CP/CPJvec/VecBase/tide I  // (empty)
        * -------------------------------------- *)
        argX := callX.actuals.a[1];
        dstT := arg0.type(Ty.Vector).elemTp;
        vRef := out.newLocal();
        tide := out.newLocal();
        okLb := out.newLabel();
        e.PushValue(arg0, arg0.type);
        out.Code(Jvm.opc_dup);
        out.StoreLocal(vRef, NIL);
        out.GetVecLen();
        out.StoreLocal(tide, Bi.intTp);
        out.LoadLocal(vRef, NIL);
        out.GetVecArr(dstT);
        out.Code(Jvm.opc_arraylength);
        out.LoadLocal(tide, Bi.intTp);
        out.CodeLb(Jvm.opc_if_icmpgt, okLb);
        out.LoadLocal(vRef, NIL);
        out.InvokeExpand(dstT);
        out.DefLab(okLb);
        out.LoadLocal(vRef, NIL);
        out.GetVecArr(dstT);
        out.LoadLocal(tide, Bi.intTp);
        e.ValueCopy(argX, dstT);
        out.PutVecElement(dstT);
        out.LoadLocal(vRef, NIL);
        out.LoadLocal(tide, Bi.intTp);
        out.Code(Jvm.opc_iconst_1);
        out.Code(Jvm.opc_iadd);
        out.PutVecLen();
        out.ReleaseLocal(tide);
        out.ReleaseLocal(vRef);
   (* --------------------------- *)
    | Bi.exclP, Bi.inclP :
        dstT := arg0.type;
        argX := callX.actuals.a[1];

        e.PushHandle(arg0, dstT);
        WITH arg0 : Xp.IdLeaf DO
            idX0 := arg0.ident;
            WITH idX0 : Id.LocId DO
              IF Id.uplevA IN idX0.locAtt THEN (* uplevel addressing case *)
                out.Code(Jvm.opc_dup);	(* handle is one slot only *)
                out.PutGetX(Jvm.opc_getfield, idX0);
              ELSE
                out.LoadLocal(idX0.varOrd, dstT);
              END;
            ELSE
              e.PushValue(arg0, dstT);
            END;
        | arg0 : Xp.BinaryX DO
            ASSERT(arg0.kind = Xp.index);
            out.Code(Jvm.opc_dup2);
            out.GetElement(dstT);
        | arg0 : Xp.IdentX DO
            ASSERT(arg0.kind = Xp.selct);
            out.Code(Jvm.opc_dup);
            out.PutGetF(Jvm.opc_getfield, 
        		arg0.kid.type(Ty.Record), arg0.ident(Id.FldId));
        END;
        IF argX.kind = Xp.numLt THEN
          out.PushInt(ORD({intValue(argX)}));
        ELSE
          out.Code(Jvm.opc_iconst_1);
          e.PushValue(argX, Bi.intTp);
          out.Code(Jvm.opc_ishl);
        END;
        IF pOrd = Bi.inclP THEN
          out.Code(Jvm.opc_ior);
        ELSE
          out.Code(Jvm.opc_iconst_m1);
          out.Code(Jvm.opc_ixor);
          out.Code(Jvm.opc_iand);
        END;
        e.ScalarAssign(arg0);
   (* --------------------------- *)
    | Bi.haltP :
        out.PushInt(intValue(arg0));
        out.CallRTS(Ju.SysExit,1,0);
        out.PushJunkAndReturn();
   (* --------------------------- *)
    | Bi.throwP :
        IF Cst.ntvExc.assignCompat(arg0) THEN
          e.PushValue(arg0, Cst.ntvExc);
          out.Code(Jvm.opc_athrow);
        ELSE
          out.MkNewException();
          out.Code(Jvm.opc_dup);
          e.PushValue(arg0, Cst.ntvStr);
          out.InitException();
          out.Code(Jvm.opc_athrow);
        END;
   (* --------------------------- *)
    | Bi.newP :
       (*
        *   arg0 is a pointer to a Record or Array, or else a vector type.
        *)
        e.PushHandle(arg0, arg0.type);
        IF argN = 1 THEN
         (*
          *  No LEN argument implies either:
          *	pointer to record, OR
          *	pointer to a fixed array.
          *)
          dstT := arg0.type(Ty.Pointer).boundTp;
          WITH dstT : Ty.Record DO
              out.MkNewRecord(dstT);
          | dstT : Ty.Array DO
              out.MkNewFixedArray(dstT.elemTp, dstT.length);
          END;
        ELSIF arg0.type.kind = Ty.ptrTp THEN
          FOR numL := 1 TO argN-1 DO
            argX := callX.actuals.a[numL];
            e.PushValue(argX, Bi.intTp);
          END;
          dstT := arg0.type(Ty.Pointer).boundTp;
          out.MkNewOpenArray(dstT(Ty.Array), argN-1);
        ELSE (* must be a vector type *)
          dstT := arg0.type(Ty.Vector).elemTp;
          out.MkVecRec(dstT);
          out.Code(Jvm.opc_dup);
          e.PushValue(callX.actuals.a[1], Bi.intTp);
          out.MkVecArr(dstT);
        END;
        e.ScalarAssign(arg0);
   (* --------------------------- *)
    END;
  END EmitStdProc;

(* ============================================================ *)
(*		    Statement Handling Methods			*)
(* ============================================================ *)

  PROCEDURE (e : JavaEmitter)EmitAssign(stat : St.Assign),NEW;
    VAR lhTyp : Sy.Type;
  BEGIN
   (*
    *  This is a value assign in CP.
    *)
    lhTyp := stat.lhsX.type;
    e.PushHandle(stat.lhsX, lhTyp);
    e.PushValue(stat.rhsX, lhTyp);
    WITH lhTyp : Ty.Vector DO
        e.ScalarAssign(stat.lhsX);
    | lhTyp : Ty.Array DO 
        IF stat.rhsX.kind = Xp.mkStr THEN
          e.outF.CallRTS(Ju.StrVal, 2, 0);
        ELSIF stat.rhsX.type = Bi.strTp THEN
          e.outF.CallRTS(Ju.StrToChrs,2, 0);
        ELSE
          e.outF.ValArrCopy(lhTyp);
        END;
    | lhTyp : Ty.Record DO
        e.outF.ValRecCopy(lhTyp);
    ELSE
      e.ScalarAssign(stat.lhsX);
    END;
  END EmitAssign;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)EmitCall(stat : St.ProcCall),NEW;
    VAR expr : Xp.CallX;	(* the stat call expression *)
  BEGIN
    expr := stat.expr(Xp.CallX);
    IF (expr.kind = Xp.prCall) & expr.kid.isStdProc() THEN
      e.EmitStdProc(expr);
    ELSE
      e.PushCall(expr);
    END;
  END EmitCall;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)EmitIf(stat : St.Choice; OUT ok : BOOLEAN),NEW;
    VAR out  : Ju.JavaFile;
        high : INTEGER;			(* Branch count.  *)
        exLb : Ju.Label;			(* Exit label	  *)
        nxtP : Ju.Label;			(* Next predicate *)
        indx : INTEGER;
        live : BOOLEAN;			(* then is live   *)
        else : BOOLEAN;			(* else not seen  *)
        then : Sy.Stmt;
        pred : Sy.Expr;
  BEGIN
    ok := FALSE;
    out := e.outF;
    exLb := out.newLabel();
    else := FALSE;
    high := stat.preds.tide - 1;
    FOR indx := 0 TO high DO
      live := TRUE;
      pred := stat.preds.a[indx];
      then := stat.blocks.a[indx];
      nxtP := out.newLabel();
      IF pred = NIL THEN else := TRUE ELSE e.FallTrue(pred, nxtP) END;
      IF then # NIL THEN e.EmitStat(then, live) END;
      IF live THEN 
        ok := TRUE;
        IF indx < high THEN out.CodeLb(Jvm.opc_goto, exLb) END;
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

  PROCEDURE (e : JavaEmitter)EmitRanges
        	       (locV : INTEGER;   	(* select Var   *)
        		stat : St.CaseSt;  	(* case stat    *)
        		minR : INTEGER;  	(* min rng-ix   *)
        		maxR : INTEGER;  	(* max rng-ix   *)
        		minI : INTEGER;  	(* min index    *)
        		maxI : INTEGER;  	(* max index    *)
        		labs : ARRAY OF Ju.Label),NEW;
   (* --------------------------------------------------------- * 
    *   This procedure emits the code for a single,
    *   dense range of selector values in the label-list.
    * --------------------------------------------------------- *)
    VAR out  : Ju.JavaFile;
        loIx : INTEGER;		(* low selector value for dense range  *)
        hiIx : INTEGER;		(* high selector value for dense range *)
        rNum : INTEGER;		(* total number of ranges in the group *)
        peel : INTEGER;		(* max index of range to be peeled off *)
        indx : INTEGER;
        pos  : INTEGER;
        rnge : St.Triple;
        dfLb : Ju.Label;
        lab  : Ju.Label;
  BEGIN
    out := e.outF;
    dfLb := labs[0];
    rNum := maxR - minR + 1;
    rnge := stat.labels.a[minR];
    IF rNum = 1 THEN		(* single range only *)
      lab := labs[rnge.ord+1]; 
      out.EmitOneRange(locV, rnge.loC, rnge.hiC, minI, maxI, dfLb, lab);
    ELSIF rNum < 4 THEN	
     (*
      *    Two or three ranges only.
      *    Peel off the lowest of the ranges, and recurse.
      *)
      loIx := rnge.loC;
      peel := rnge.hiC;
      out.LoadLocal(locV, Bi.intTp);
     (*
      *   There are a number of special cases
      *   that can benefit from special code.
      *)
      IF loIx = peel THEN 
       (* 
        *   A singleton.  Leave minI unchanged, unless peel = minI.
        *)
        out.PushInt(peel);
        out.CodeLb(Jvm.opc_if_icmpeq, labs[rnge.ord + 1]);
        IF minI = peel THEN minI := peel+1 END;
        INC(minR);
      ELSIF loIx = minI THEN
       (* 
        *   A range starting at the minimum selector value.
        *)
        out.PushInt(peel);
        out.CodeLb(Jvm.opc_if_icmple, labs[rnge.ord + 1]);
        minI := peel+1;
        INC(minR);
      ELSE
       (* 
        *   We must peel the default range from minI to loIx.
        *)
        out.PushInt(loIx);
        out.CodeLb(Jvm.opc_if_icmplt, dfLb);
        minI := loIx;	(* and minR is unchanged! *)
      END;
      e.EmitRanges(locV, stat, minR, maxR, minI, maxI, labs);
    ELSE 
     (*
      *   Four or more ranges.  Emit a dispatch table.
      *)
      loIx := rnge.loC;			(* low of min-range  *)
      hiIx := stat.labels.a[maxR].hiC;	(* high of max-range *)
      out.LoadLocal(locV, Bi.intTp);
      out.CodeSwitch(loIx, hiIx, dfLb);
      pos := 0;
      FOR indx := minR TO maxR DO
        rnge := stat.labels.a[indx];
        WHILE loIx < rnge.loC DO 
          out.AddSwitchLab(labs[0],pos); INC(pos); INC(loIx);
        END;
        WHILE loIx <= rnge.hiC DO 
          out.AddSwitchLab(labs[rnge.ord+1],pos); INC(pos); INC(loIx);
        END;
      END;
      out.LstDef(labs[0]);
    END;
  END EmitRanges;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)EmitGroups
        	       (locV : INTEGER;		(* select vOrd  *)
        		stat : St.CaseSt;  	(* case stat  	*)
        		minG : INTEGER;  	(* min grp-indx	*)
        		maxG : INTEGER;  	(* max grp-indx	*)
        		minI : INTEGER;  	(* min index  	*)
        		maxI : INTEGER;  	(* max index  	*)
        		labs : ARRAY OF Ju.Label),NEW;
   (* --------------------------------------------------------- * 
    *  This function emits the branching code which sits on top
    *  of the selection code for each dense range of case values.
    * --------------------------------------------------------- *)
    VAR out   : Ju.JavaFile;
        newLb : Ju.Label;
        midPt : INTEGER;
        group : St.Triple;
        range : St.Triple;
  BEGIN
    IF maxG = -1 THEN RETURN; (* Empty case statment *)
    ELSIF minG = maxG THEN (* only one remaining dense group *)
      group := stat.groups.a[minG];
      e.EmitRanges(locV, stat, group.loC, group.hiC, minI, maxI, labs);
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
      out.LoadLocal(locV, Bi.intTp);
      out.PushInt(range.loC);
      out.CodeLb(Jvm.opc_if_icmpge, newLb);
     (*
      *    Recurse!
      *)
      e.EmitGroups(locV, stat, minG, midPt-1, minI, range.loC-1, labs);
      out.DefLab(newLb);
      e.EmitGroups(locV, stat, midPt, maxG, range.loC, maxI, labs);
    END;
  END EmitGroups;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)EmitCase(stat : St.CaseSt; OUT ok : BOOLEAN),NEW;
    VAR out  : Ju.JavaFile;
        indx : INTEGER;
        dfLb : Ju.Label;
        exLb : Ju.Label;
        selV : INTEGER;
        live : BOOLEAN;
        minI : INTEGER;
        maxI : INTEGER;
        labs : POINTER TO ARRAY OF Ju.Label;
  BEGIN
   (* ---------------------------------------------------------- *
    *  CaseSt* = POINTER TO RECORD (Sy.Stmt)
    *	     (* ----------------------------------------- *
    *	      *	kind-  : INTEGER;	(* tag for unions *)
    *	      *	token* : S.Token;	(* stmt first tok *)
    *	      * ----------------------------------------- *)
    *		select* : Sy.Expr;	(* case selector  *)
    *		chrSel* : BOOLEAN;	(* ==> use chars  *)
    *		blocks* : Sy.StmtSeq;	(* case bodies    *)
    *		elsBlk* : Sy.Stmt;	(* elseCase | NIL *)
    *		labels* : TripleSeq;	(* label seqence  *)
    *		groups- : TripleSeq;	(* dense groups   *)
    *	      END;
    * --------------------------------------------------------- *
    *  Notes on the semantics of this structure. "blocks" holds	*
    *  an ordered list of case statement code blocks. "labels"	*
    *  is a list of ranges, intially in textual order,with flds	*
    *  loC, hiC and ord corresponding to the range min, max and	*
    *  the selected block ordinal number.  This list is later 	*
    *  sorted on the loC value, and adjacent values merged if 	*
    *  they select the same block. The "groups" list of triples *
    *  groups ranges into dense subranges in the selector space	*
    *  The fields loC, hiC, and ord to hold the lower and upper	*
    *  indices into the labels list, and the number of non-	*
    *  default values in the group. Groups are guaranteed to	*
    *  have density (nonDefN / (max-min+1)) > DENSITY		*
    * --------------------------------------------------------- *)
    ok := FALSE;
    out := e.outF;
    exLb := out.newLabel();
    NEW(labs,stat.blocks.tide+1);
    out.getLabelRange(labs);
    selV := out.newLocal();

    IF stat.chrSel THEN 
      minI := 0; maxI := ORD(MAX(CHAR));
    ELSE 
      minI := MIN(INTEGER); 
      maxI := MAX(INTEGER);
    END;

   (*
    *    Push the selector value, and save in local variable;
    *)
    e.PushValue(stat.select, stat.select.type);
    out.StoreLocal(selV, Bi.intTp);
    e.EmitGroups(selV, stat, 0, stat.groups.tide-1, minI, maxI, labs);
   (*
    *    Now we emit the code for the cases.
    *    If any branch returns, then exLb is reachable.
    *)
    FOR indx := 0 TO stat.blocks.tide-1 DO
      out.DefLab(labs[indx + 1]);
      e.EmitStat(stat.blocks.a[indx], live);
      IF live THEN
        ok := TRUE;
        out.CodeLb(Jvm.opc_goto, exLb);
      END;
    END;
   (*
    *    Now we emit the code for the elespart.
    *    If the elsepart returns then exLb is reachable.
    *)
    out.DefLabC(labs[0], "Default case");
    IF stat.elsBlk # NIL THEN
      e.EmitStat(stat.elsBlk, live);
      IF live THEN ok := TRUE END;
    ELSE
      out.CaseTrap(selV);
    END;
    out.ReleaseLocal(selV);
    IF ok THEN out.DefLabC(exLb, "Case exit label") END;
  END EmitCase;
 
(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)
        		EmitWhile(stat : St.TestLoop; OUT ok : BOOLEAN),NEW;
    VAR out  : Ju.JavaFile;
        lpLb : Ju.Label;
        exLb : Ju.Label;
  BEGIN
    out := e.outF;
    lpLb := out.newLabel();
    exLb := out.newLabel();
    e.FallTrue(stat.test, exLb);	(* goto exLb if eval false *)
    out.DefLabC(lpLb, "Loop header");
    e.EmitStat(stat.body, ok);
    IF ok THEN e.FallFalse(stat.test, lpLb) END;
    out.DefLabC(exLb, "Loop exit");
  END EmitWhile;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)
        		EmitRepeat(stat : St.TestLoop; OUT ok : BOOLEAN),NEW;
    VAR out  : Ju.JavaFile;
        lpLb : Ju.Label; 
  BEGIN
    out := e.outF;
    lpLb := out.newLabel();
    out.DefLabC(lpLb, "Loop header");
    e.EmitStat(stat.body, ok);
    IF ok THEN e.FallTrue(stat.test, lpLb) END; (* exit on eval true *)
  END EmitRepeat;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)EmitFor(stat : St.ForLoop; OUT ok : BOOLEAN),NEW;
   (* ----------------------------------------------------------- *
    *   This code has been split into the four cases:
    *   -	long control variable, counting up;
    *   -	long control variable, counting down;
    *   -	int control variable, counting up;
    *   -	int control variable, counting down;
    *   Of course, it is possible to fold all of this, and have 
    *   tests everywhere, but the following is cleaner, and easier 
    *   to enhance in the future.
    *
    *   Note carefully the use of ForLoop::isSimple().  It is 
    *   essential to use exactly the same function here as is
    *   used by ForLoop::flowAttr() for initialization analysis.
    *   If this were not the case, the verifier could barf.
    * ----------------------------------------------------------- *)
    PROCEDURE SetVar(cv : Id.AbVar; ln : BOOLEAN; ou : Ju.JavaFile);
    BEGIN
      WITH cv : Id.LocId DO (* check if implemented inside XHR *)
        IF Id.uplevA IN cv.locAtt THEN 
          ou.XhrHandle(cv);
          IF ~ln THEN 
            ou.Code(Jvm.opc_swap);
          ELSE
            ou.Code(Jvm.opc_dup_x2);
            ou.Code(Jvm.opc_pop);
          END;
        END;
      ELSE (* skip *)
      END;
      ou.PutVar(cv);
    END SetVar;
   (* ----------------------------------------------------------- *)
    PROCEDURE LongForUp(e: JavaEmitter; stat: St.ForLoop; OUT ok: BOOLEAN);
      VAR out  : Ju.JavaFile;
          cVar : Id.AbVar;
          top1 : INTEGER;
          top2 : INTEGER;
          exLb : Ju.Label;
          lpLb : Ju.Label;
          step : LONGINT;
          smpl : BOOLEAN;
    BEGIN
      out := e.outF;
      lpLb := out.newLabel();
      exLb := out.newLabel();
      cVar := stat.cVar(Id.AbVar);
      step := longValue(stat.byXp); 
      smpl := stat.isSimple();
      IF smpl THEN
        out.PushLong(longValue(stat.loXp));
        SetVar(cVar, TRUE, out);
        top1 := -1;			(* keep the verifier happy! *)
        top2 := -1;			(* keep the verifier happy! *)
      ELSE
        top1 := out.newLocal();	(* actually a pair of locals *)
        top2 := out.newLocal();
        e.PushValue(stat.hiXp, Bi.lIntTp);
        out.Code(Jvm.opc_dup2);
        out.StoreLocal(top1, Bi.lIntTp);
        e.PushValue(stat.loXp, Bi.lIntTp);
        out.Code(Jvm.opc_dup2);
        SetVar(cVar, TRUE, out);
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
          out.LoadLocal(top1, Bi.lIntTp);
        END;
        out.GetVar(cVar);			(* (top) cv,hi		*)
        out.PushLong(step);
        out.Code(Jvm.opc_ladd);			(* (top) cv',hi		*)
        out.Code(Jvm.opc_dup2);			(* (top) cv',cv',hi	*)
        SetVar(cVar, TRUE, out);
        e.DoCmp(Xp.greEq,  lpLb, Bi.lIntTp);
      END;
     (*
      *   The exit label.
      *)
      out.DefLabC(exLb, "Loop trailer");
    END LongForUp;

   (* ----------------------------------------- *)

    PROCEDURE LongForDn(e: JavaEmitter; stat: St.ForLoop; OUT ok: BOOLEAN);
      VAR out  : Ju.JavaFile;
          cVar : Id.AbVar;
          top1 : INTEGER;
          top2 : INTEGER;
          exLb : Ju.Label;
          lpLb : Ju.Label;
          step : LONGINT;
          smpl : BOOLEAN;
    BEGIN
      out := e.outF;
      lpLb := out.newLabel();
      exLb := out.newLabel();
      cVar := stat.cVar(Id.AbVar);
      step := longValue(stat.byXp);
      smpl := stat.isSimple();
      IF smpl THEN
        out.PushLong(longValue(stat.loXp));
        SetVar(cVar, TRUE, out);
        top1 := -1;			(* keep the verifier happy! *)
        top2 := -1;			(* keep the verifier happy! *)
      ELSE
        top1 := out.newLocal();	(* actually a pair of locals *)
        top2 := out.newLocal();
        e.PushValue(stat.hiXp, Bi.lIntTp);
        out.Code(Jvm.opc_dup2);
        out.StoreLocal(top1, Bi.lIntTp);
        e.PushValue(stat.loXp, Bi.lIntTp);
        out.Code(Jvm.opc_dup2);
        SetVar(cVar, TRUE, out);
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
          out.LoadLocal(top1, Bi.lIntTp);
        END;
        out.GetVar(cVar);			(* (top) cv,hi		*)
        out.PushLong(step);
        out.Code(Jvm.opc_ladd);			(* (top) cv',hi		*)
        out.Code(Jvm.opc_dup2);			(* (top) cv',cv',hi	*)
        SetVar(cVar, TRUE, out);
        e.DoCmp(Xp.lessEq, lpLb, Bi.lIntTp);
      END;
     (*
      *   The exit label.
      *)
      out.DefLabC(exLb, "Loop trailer");
    END LongForDn;

   (* ----------------------------------------- *)

    PROCEDURE IntForUp(e: JavaEmitter; stat: St.ForLoop; OUT ok: BOOLEAN);
      VAR out  : Ju.JavaFile;
          cVar : Id.AbVar;
          topV : INTEGER;
          exLb : Ju.Label;
          lpLb : Ju.Label;
          step : INTEGER;
          smpl : BOOLEAN;
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
      IF smpl THEN
        out.PushInt(intValue(stat.loXp));
        SetVar(cVar, FALSE, out);
        topV := -1;			(* keep the verifier happy! *)
      ELSE
        topV := out.newLocal();
        e.PushValue(stat.hiXp, Bi.intTp);
        out.Code(Jvm.opc_dup);
        out.StoreLocal(topV, Bi.intTp);
        e.PushValue(stat.loXp, Bi.intTp);
        out.Code(Jvm.opc_dup);
        SetVar(cVar, FALSE, out);
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
          out.LoadLocal(topV, Bi.intTp);
        END;
        out.GetVar(cVar);			(* (top) cv,hi		*)
        out.PushInt(step);
        out.Code(Jvm.opc_iadd);			(* (top) cv',hi		*)
        out.Code(Jvm.opc_dup);			(* (top) cv',cv',hi	*)
        SetVar(cVar, FALSE, out);
        e.DoCmp(Xp.greEq, lpLb, Bi.intTp);
      END;
     (*
      *   The exit label.
      *)
      out.DefLabC(exLb, "Loop trailer");
    END IntForUp;
  
   (* ----------------------------------------- *)

    PROCEDURE IntForDn(e: JavaEmitter; stat: St.ForLoop; OUT ok: BOOLEAN);
      VAR out  : Ju.JavaFile;
          cVar : Id.AbVar;
          topV : INTEGER;
          exLb : Ju.Label; 
          lpLb : Ju.Label;
          step : INTEGER;
          smpl : BOOLEAN;
    BEGIN
      out := e.outF;
      lpLb := out.newLabel();
      exLb := out.newLabel();
      cVar := stat.cVar(Id.AbVar);
      step := intValue(stat.byXp);
      topV := out.newLocal();
      smpl := stat.isSimple();
      IF smpl THEN
        out.PushInt(intValue(stat.loXp));
        SetVar(cVar, FALSE, out);
        topV := -1;			(* keep the verifier happy! *)
      ELSE
        e.PushValue(stat.hiXp, Bi.intTp);
        out.Code(Jvm.opc_dup);
        out.StoreLocal(topV, Bi.intTp);
        e.PushValue(stat.loXp, Bi.intTp);
        out.Code(Jvm.opc_dup);
        SetVar(cVar, FALSE, out);
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
          out.LoadLocal(topV, Bi.intTp);
        END;
        out.GetVar(cVar);			(* (top) cv,hi		*)
        out.PushInt(step);
        out.Code(Jvm.opc_iadd);			(* (top) cv',hi		*)
        out.Code(Jvm.opc_dup);			(* (top) cv',cv',hi	*)
        SetVar(cVar, FALSE, out);
        e.DoCmp(Xp.lessEq, lpLb, Bi.intTp);
      END;
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

  PROCEDURE (e : JavaEmitter)
        		EmitLoop(stat : St.TestLoop; OUT ok : BOOLEAN),NEW;
    VAR out  : Ju.JavaFile;
        lpLb : Ju.Label;
        tmpLb : Ju.Label;
  BEGIN
    out := e.outF;
    lpLb  := out.newLabel();
    tmpLb := currentLoopLabel;
    currentLoopLabel := out.newLabel();
    out.DefLabC(lpLb, "Loop header");
    e.EmitStat(stat.body, ok);
    IF ok THEN out.CodeLb(Jvm.opc_goto, lpLb) END;
    out.DefLabC(currentLoopLabel, "Loop exit");
    currentLoopLabel := tmpLb;
  END EmitLoop;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)EmitWith(stat : St.Choice; OUT ok : BOOLEAN),NEW;
    VAR out  : Ju.JavaFile;
        high : INTEGER;			(* Branch count.  *)
        exLb : Ju.Label;			(* Exit label	  *)
        nxtP : Ju.Label;			(* Next predicate *)
        indx : INTEGER;
        live : BOOLEAN;
        then : Sy.Stmt;
        pred : Sy.Expr;
        tVar : Id.LocId;
   (* --------------------------- *)
    PROCEDURE WithTest(je : JavaEmitter; 
        	       os : Ju.JavaFile; 
        	       pr : Sy.Expr; 
        	       nx : Ju.Label;
        	       tm : INTEGER);
      VAR bX : Xp.BinaryX;
          ty : Sy.Type;
    BEGIN
      bX := pr(Xp.BinaryX);
      ty := bX.rKid(Xp.IdLeaf).ident.type;
      je.PushValue(bX.lKid, bX.lKid.type);
      os.CodeT(Jvm.opc_instanceof, ty);
      os.CodeLb(Jvm.opc_ifeq, nx);
     (*
      *   We must also generate a checkcast, because the verifier
      *   seems to understand the typeflow consequences of the
      *   checkcast bytecode, but not instanceof.
      *)
      je.PushValue(bX.lKid, bX.lKid.type);
      os.CodeT(Jvm.opc_checkcast, ty);
      os.StoreLocal(tm, ty);
    END WithTest;
   (* --------------------------- *)
  BEGIN
    tVar := NIL;
    pred := NIL;
    ok := FALSE;
    out := e.outF;
    exLb := out.newLabel();
    high := stat.preds.tide - 1;
    FOR indx := 0 TO high DO
      live := TRUE;
      pred := stat.preds.a[indx];
      then := stat.blocks.a[indx];
      tVar := stat.temps.a[indx](Id.LocId);
      nxtP := out.newLabel();
      IF pred # NIL THEN 
        tVar.varOrd := out.newLocal();
        WithTest(e, out, pred, nxtP, tVar.varOrd);
      END;
      IF then # NIL THEN e.EmitStat(then, live) END;
      IF live THEN 
        ok := TRUE;
       (*
        *  If this is not the else case, skip over the
        *  later cases, or jump over the WITH ELSE trap.
        *)
        IF pred # NIL THEN out.CodeLb(Jvm.opc_goto, exLb) END;
      END;
      IF tVar # NIL THEN out.ReleaseLocal(tVar.varOrd) END;
      out.DefLab(nxtP);
    END;
    IF pred # NIL THEN out.WithTrap(pred(Xp.BinaryX).lKid(Xp.IdLeaf).ident) END;
    out.DefLab(exLb);
  END EmitWith;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)EmitExit(stat : St.ExitSt),NEW;
  BEGIN
    e.outF.CodeLb(Jvm.opc_goto, currentLoopLabel);
  END EmitExit;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)EmitReturn(stat : St.Return),NEW;
    VAR out : Ju.JavaFile;
        pId : Id.Procs;
        ret : Sy.Type;
  BEGIN
    out := e.outF;
    pId := out.getScope()(Id.Procs);
   (*
    *  Because the return slot may be used for the first
    *  OUT or VAR parameter, the real return type might
    *  be different to that shown in the formal type.
    *  FixOutPars() returns this real return type.
    *)
    IF (stat.retX # NIL) &
       (pId.kind # Id.ctorP) THEN e.PushValue(stat.retX, stat.retX.type) END;
    out.FixOutPars(pId, ret);
    out.Return(ret);
  END EmitReturn;

(* ---------------------------------------------------- *)

  PROCEDURE (e : JavaEmitter)EmitBlock(stat : St.Block; OUT ok : BOOLEAN),NEW;
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

  PROCEDURE (e : JavaEmitter)EmitStat(stat : Sy.Stmt; OUT ok : BOOLEAN),NEW;
    VAR depth : INTEGER;
  BEGIN
    IF (stat = NIL) OR (stat.kind = St.emptyS) THEN ok := TRUE; RETURN END;
    IF stat.kind # St.blockS THEN 
      e.outF.Line(stat.token.lin);
    END;
    depth := e.outF.getDepth();
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
    e.outF.setDepth(depth);
  END EmitStat;


(* ============================================================ *)
(* ============================================================ *)
END JavaMaker.
(* ============================================================ *)
(* ============================================================ *)
