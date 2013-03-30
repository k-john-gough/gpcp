(* ============================================================ *)
(*  PeUtil is the module which writes PE files using the        *)
(*  managed interface.                                          *)
(*  Copyright (c) John Gough 1999, 2002.                        *)
(*  Copyright (c) Queensland University of Technology 2002-2006 *)
(*  This is the PERWAPI-based prototype, March 2005             *)
(*    previous versions used the PE-file <writer> PEAPI.        *)
(* ============================================================ *)

MODULE PeUtil;

  IMPORT 
        GPCPcopyright,
        RTS, ASCII,
        Console,
        GPText,
        GPBinFiles,
        GPTextFiles,
        FileNames,
        ClassMaker,
        MsilBase,
        NameHash,
        Mu  := MsilUtil,
        Lv  := LitValue,
        Sy  := Symbols,
        Bi  := Builtin,
        Id  := IdDesc,
        Ty  := TypeDesc,
        Api := "[QUT.PERWAPI]QUT.PERWAPI",
        Scn := CPascalS,
        Asm := IlasmCodes,
        CSt := CompState,
        Sys := "[mscorlib]System";

(* ============================================================ *)

(*
 * CONST
 *      (* various ILASM-specific runtime name strings *)
 *      initPrefix  = "instance void ";
 *      initSuffix  = ".ctor() ";
 *      managedStr  = "il managed";
 *      specialStr  = "public specialname rtspecialname ";
 *      cctorStr    = "static void .cctor() ";
 *      objectInit  = "instance void $o::.ctor() ";
 *
 * CONST
 *      catchStr    = "      catch [mscorlib]System.Exception";
 *)

(* ============================================================ *)
(* ============================================================ *)

  TYPE PeFile*    = POINTER TO RECORD (Mu.MsilFile)
                 (*   Fields inherited from MsilFile *
                  *   srcS* : LitValue.CharOpen; (* source file name   *)
                  *   outN* : LitValue.CharOpen; (* output file name   *)
                  *   proc* : ProcInfo;
                  *)
                      peFl  : Api.PEFile;          (* Includes AssemblyDef  *)
                      clsS  : Api.ClassDef;        (* Dummy static ClassDef *)
                      clsD  : Api.ClassDef;        (* The current ClassDef  *)
                      pePI  : PProcInfo;
                      nmSp  : RTS.NativeString;
                     (*
                      *  Friendly access for system classes.
                      *)
                      rts      : Api.AssemblyRef;  (* "[RTS]"               *)
                      cprts    : Api.ClassRef;     (* "[RTS]CP_rts"         *)
                      progArgs : Api.ClassRef;     (* "[RTS]ProgArgs"       *)
                    END;

(* ============================================================ *)

  TYPE PProcInfo  = POINTER TO RECORD 
                      mthD  : Api.MethodDef;
                      code  : Api.CILInstructions;
                      tryB  : Api.TryBlock;
                    END;

(* ============================================================ *)

  TYPE PeLab     = POINTER TO RECORD (Mu.Label)
                      labl : Api.CILLabel;
                   END;

  TYPE TypArr    = POINTER TO ARRAY OF Api.Type;

(* ============================================================ *)

  VAR   cln2,                           (* "::"     *) 
        evtAdd,
        evtRem,
        boxedObj       : Lv.CharOpen;

(* ============================================================ *)

  VAR   ctAtt,                          (* public + special + RTspecial *)
        psAtt,                          (* public + static              *)
        rmAtt,                          (* runtime managed              *)
        ilAtt          : INTEGER;       (* cil managed                  *)

  VAR   xhrCl          : Api.ClassRef;  (* the [RTS]XHR class reference *)
        voidD          : Api.Type;      (* Api.PrimitiveType.Void       *)
        objtD          : Api.Type;      (* Api.PrimitiveType.Object     *)
        strgD          : Api.Type;      (* Api.PrimitiveType.String     *)
        charD          : Api.Type;      (* Api.PrimitiveType.Char       *)
        charA          : Api.Type;      (* Api.PrimitiveType.Char[]     *)
        int4D          : Api.Type;      (* Api.PrimitiveType.Int32      *)
        int8D          : Api.Type;      (* Api.PrimitiveType.Int64      *)
        flt4D          : Api.Type;      (* Api.PrimitiveType.Float32    *)
        flt8D          : Api.Type;      (* Api.PrimitiveType.Float64    *)
        nIntD          : Api.Type;      (* Api.PrimitiveType.NativeInt  *)

  VAR   vfldS          : RTS.NativeString;   (* "v$"    *)
        copyS          : RTS.NativeString;   (* "copy"  *)
        ctorS          : RTS.NativeString;   (* ".ctor" *)
        invkS          : RTS.NativeString;   (* Invoke  *)
        
  VAR   defSrc : Api.SourceFile;

  VAR   rHelper : ARRAY Mu.rtsLen OF Api.MethodRef;
        mathCls : Api.ClassRef;
        envrCls : Api.ClassRef;
        excpCls : Api.ClassRef;
        rtTpHdl : Api.ClassRef;
        loadTyp : Api.MethodRef;
        newObjt : Api.MethodRef;
        multiCD : Api.ClassRef;     (* System.MulticastDelegate *)
        delegat : Api.ClassRef;     (* System.Delegate          *)
        combine : Api.MethodRef;    (* System.Delegate::Combine *)
        remove  : Api.MethodRef;    (* System.Delegate::Remove  *)
        corlib  : Api.AssemblyRef;  (* [mscorlib]               *)
        
(* ============================================================ *)
(*      Data Structure for tgXtn field of BlkId descriptors     *)
(* ============================================================ *)

  TYPE BlkXtn = POINTER TO RECORD
                  asmD : Api.AssemblyRef; (* This AssemblyRef   *)
                  dscD : Api.Class;       (* Dummy Static Class *)
                END;

(* ============================================================ *)
(*          Data Structure for Switch Statement Encoding        *)
(* ============================================================ *)

  TYPE Switch = RECORD
                  list : POINTER TO ARRAY OF Api.CILLabel;
                  next : INTEGER;
                END;

  VAR  switch : Switch;

(* ============================================================ *)
(*      Data Structure for tgXtn field of procedure types       *)
(* ============================================================ *)

  TYPE DelXtn = POINTER TO RECORD
                  clsD : Api.Class;      (* Implementing class  *)
                  newD : Api.Method;     (* Constructor method  *)
                  invD : Api.Method;     (* The Invoke method   *)
                END;

(* ============================================================ *)
(*      Data Structure for tgXtn field of event variables       *)
(* ============================================================ *)

  TYPE EvtXtn = POINTER TO RECORD
                  fldD : Api.Field;      (* Field descriptor    *)
                  addD : Api.Method;     (* add_<field> method  *)
                  remD : Api.Method;     (* rem_<field> method  *)
                END;

(* ============================================================ *)
(*      Data Structure for tgXtn field of Record types          *)
(* ============================================================ *)

  TYPE RecXtn = POINTER TO RECORD
                  clsD : Api.Class;
                  boxD : Api.Class;
                  newD : Api.Method;
                  cpyD : Api.Method;
                  vDlr : Api.Field;
                END;

(* ============================================================ *)
(*                    Constructor Method                        *)
(* ============================================================ *)

  PROCEDURE newPeFile*(IN nam : ARRAY OF CHAR; isDll : BOOLEAN) : PeFile;
    VAR f : PeFile;
        ver : INTEGER;
   (* ------------------------------------------------------- *)
    PROCEDURE file(IN f,a : ARRAY OF CHAR; d : BOOLEAN) : Api.PEFile;
      VAR pef : Api.PEFile;
    BEGIN 
      pef := Api.PEFile.init(MKSTR(f), MKSTR(a));
      pef.SetIsDLL(d);
      IF CSt.binDir # "" THEN
        pef.SetOutputDirectory(MKSTR(CSt.binDir));
      END;
      RETURN pef;
    RESCUE (x) 
      RETURN NIL;
    END file;
   (* ------------------------------------------------------- *)
  BEGIN
    NEW(f);
(*
 *  f.peFl := file(nam, isDll);
 *)
    IF isDll THEN
      f.outN := BOX(nam + ".DLL");
    ELSE
      f.outN := BOX(nam + ".EXE");
    END;
(* -- start replacement -- *)
    f.peFl := file(f.outN, nam, isDll);
(* --- end replacement --- *)
   (*
    *  Initialize local variables holding common attributes.
    *)
    ctAtt := Api.MethAttr.Public + Api.MethAttr.SpecialRTSpecialName;
    psAtt := Api.MethAttr.Public + Api.MethAttr.Static;
    ilAtt := Api.ImplAttr.IL;
    rmAtt := Api.ImplAttr.Runtime;
   (*
    *  Initialize local variables holding primitive type-enums.
    *)
    voidD := Api.PrimitiveType.Void;
    objtD := Api.PrimitiveType.Object;
    strgD := Api.PrimitiveType.String;
    int4D := Api.PrimitiveType.Int32;
    int8D := Api.PrimitiveType.Int64;
    flt4D := Api.PrimitiveType.Float32;
    flt8D := Api.PrimitiveType.Float64;
    charD := Api.PrimitiveType.Char;
    charA := Api.ZeroBasedArray.init(Api.PrimitiveType.Char);
    nIntD := Api.PrimitiveType.IntPtr;

    f.peFl.SetNetVersion(Api.NetVersion.Version2);

    (*ver := f.peFl.GetNetVersion();*)

    RETURN f;
  END newPeFile;

(* ============================================================ *)

  PROCEDURE (t : PeFile)fileOk*() : BOOLEAN;
  BEGIN
    RETURN t.peFl # NIL;
  END fileOk;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkNewProcInfo*(proc : Sy.Scope);
    VAR p : PProcInfo;
  BEGIN
    NEW(os.proc);
    NEW(os.pePI);
    Mu.InitProcInfo(os.proc, proc);
  END MkNewProcInfo;

(* ============================================================ *)

  PROCEDURE (os : PeFile)newLabel*() : Mu.Label;
    VAR label : PeLab;
  BEGIN
    NEW(label); 
    label.labl := os.pePI.code.NewLabel();
    RETURN label;
  END newLabel;

(* ============================================================ *)
(*                    Various utilities                         *)
(* ============================================================ *)

  PROCEDURE^ (os : PeFile)CallCombine(typ : Sy.Type; add : BOOLEAN),NEW;
  PROCEDURE^ (os : PeFile)CodeLb*(code : INTEGER; labl : Mu.Label);
  PROCEDURE^ (os : PeFile)DefLabC*(l : Mu.Label; IN c : ARRAY OF CHAR);
  PROCEDURE^ (os : PeFile)Locals(),NEW;

  PROCEDURE^ MkMthDef(os  : PeFile;
                      xhr : BOOLEAN;
                      pTp : Ty.Procedure;
                      cls : Api.ClassDef;
                      str : RTS.NativeString)  : Api.MethodDef;

  PROCEDURE^ MkMthRef(os  : PeFile;
                      pTp : Ty.Procedure;
                      cls : Api.ClassRef;
                      str : RTS.NativeString) : Api.MethodRef;

  PROCEDURE^ (os : PeFile)mth(pId : Id.Procs)  : Api.Method,NEW;
  PROCEDURE^ (os : PeFile)fld(fId : Id.AbVar)  : Api.Field,NEW;
  PROCEDURE^ (os : PeFile)add(fId : Id.AbVar)  : Api.Method,NEW;
  PROCEDURE^ (os : PeFile)rem(fId : Id.AbVar)  : Api.Method,NEW;
  PROCEDURE^ (os : PeFile)asm(bId : Id.BlkId)  : Api.AssemblyRef,NEW;
  PROCEDURE^ (os : PeFile)dsc(bId : Id.BlkId)  : Api.Class,NEW;
  PROCEDURE^ (os : PeFile)cls(rTy : Ty.Record) : Api.Class,NEW;
  PROCEDURE^ (os : PeFile)new(rTy : Ty.Record) : Api.Method,NEW;
  PROCEDURE^ (os : PeFile)cpy(rTy : Ty.Record) : Api.Method,NEW;
  PROCEDURE^ (os : PeFile)typ(tTy : Sy.Type)   : Api.Type,NEW;
  PROCEDURE^ (os : PeFile)vDl(rTy : Ty.Record) : Api.Field,NEW;
  PROCEDURE^ (os : PeFile)dxt(pTy : Ty.Procedure) : DelXtn,NEW;
  PROCEDURE^ (os : PeFile)mcd() : Api.ClassRef,NEW;
  PROCEDURE^ (os : PeFile)rmv() : Api.MethodRef,NEW;
  PROCEDURE^ (os : PeFile)cmb() : Api.MethodRef,NEW;
(*
 *  PROCEDURE^ box(os : PeFile; rTy : Ty.Record) : Api.Class;
 *)
(* ============================================================ *)
(*                    Private Methods                        *)
(* ============================================================ *)

  PROCEDURE boxedName(typ : Ty.Record) : RTS.NativeString;
  BEGIN
    ASSERT(typ.xName # NIL);
    RETURN MKSTR(boxedObj^ + typ.xName^);
  END boxedName;

(* ============================================================ *)

  PROCEDURE nms(idD : Sy.Idnt) : RTS.NativeString;
  BEGIN
    RETURN MKSTR(Sy.getName.ChPtr(idD)^);
  END nms;

(* ============================================================ *)

  PROCEDURE toTypeAttr(attr : SET) : INTEGER;
    VAR result : INTEGER;
  BEGIN
    CASE ORD(attr * {0 .. 3}) OF
    | ORD(Asm.att_public)  : result := Api.TypeAttr.Public;
    | ORD(Asm.att_empty)   : result := Api.TypeAttr.Private;
    END;
    IF attr * Asm.att_sealed # {}    THEN 
      INC(result, Api.TypeAttr.Sealed);
    END;
    IF attr * Asm.att_abstract # {}  THEN 
      INC(result, Api.TypeAttr.Abstract);
    END;
    IF attr * Asm.att_interface # {} THEN 
      INC(result, Api.TypeAttr.Interface + Api.TypeAttr.Abstract);
    END;
(*
 *  what are "Import, AutoClass, UnicodeClass, *SpecialName" ? 
 *)
    RETURN result;
  END toTypeAttr;

  
(* ------------------------------------------------ *)
(*              New code for PERWAPI                *)
(* ------------------------------------------------ *)

  PROCEDURE getOrAddClass(mod : Api.ReferenceScope; 
                          nms : RTS.NativeString;
                          nam : RTS.NativeString) : Api.ClassRef;
    VAR cls : Api.Class;
  BEGIN
    cls := mod.GetClass(nms, nam);
    IF cls = NIL THEN cls := mod.AddClass(nms, nam) END;
    RETURN cls(Api.ClassRef);
  END getOrAddClass;

  PROCEDURE getOrAddValueClass(mod : Api.ReferenceScope; 
                               nms : RTS.NativeString;
                               nam : RTS.NativeString) : Api.ClassRef;
    VAR cls : Api.Class;
  BEGIN
    cls := mod.GetClass(nms, nam);
    IF cls = NIL THEN cls := mod.AddValueClass(nms, nam) END;
    RETURN cls(Api.ClassRef);
  END getOrAddValueClass;

  PROCEDURE getOrAddMethod(cls : Api.ClassRef; 
                           nam : RTS.NativeString;
                           ret : Api.Type;
                           prs : TypArr) : Api.MethodRef;
    VAR mth : Api.Method;
  BEGIN
    mth := cls.GetMethod(nam, prs);
    IF mth = NIL THEN mth := cls.AddMethod(nam, ret, prs) END;
    RETURN mth(Api.MethodRef);
  END getOrAddMethod;

  PROCEDURE getOrAddField(cls : Api.ClassRef; 
                          nam : RTS.NativeString;
                          typ : Api.Type) : Api.FieldRef;
    VAR fld : Api.FieldRef;
  BEGIN
    fld := cls.GetField(nam);
    IF fld = NIL THEN fld := cls.AddField(nam, typ) END;
    RETURN fld(Api.FieldRef);
  END getOrAddField;

(* ------------------------------------------------ *)

  PROCEDURE toMethAttr(attr : SET) : INTEGER;
    VAR result : INTEGER;
  BEGIN
    CASE ORD(attr * {0 .. 3}) OF
    | ORD(Asm.att_assembly)  : result := Api.MethAttr.Assembly;
    | ORD(Asm.att_public)    : result := Api.MethAttr.Public;
    | ORD(Asm.att_private)   : result := Api.MethAttr.Private;
    | ORD(Asm.att_protected) : result := Api.MethAttr.Family;
    END;
    IF  5 IN attr THEN INC(result, Api.MethAttr.Static) END;
    IF  6 IN attr THEN INC(result, Api.MethAttr.Final) END;
    IF  8 IN attr THEN INC(result, Api.MethAttr.Abstract) END;
    IF  9 IN attr THEN INC(result, Api.MethAttr.NewSlot) END;
    IF 13 IN attr THEN INC(result, Api.MethAttr.Virtual) END;
    RETURN result;
  END toMethAttr;

(* ------------------------------------------------ *)

  PROCEDURE toFieldAttr(attr : SET) : INTEGER;
    VAR result : INTEGER;
  BEGIN
    CASE ORD(attr * {0 .. 3}) OF
    | ORD(Asm.att_empty)     : result := Api.FieldAttr.Default;
    | ORD(Asm.att_assembly)  : result := Api.FieldAttr.Assembly;
    | ORD(Asm.att_public)    : result := Api.FieldAttr.Public;
    | ORD(Asm.att_private)   : result := Api.FieldAttr.Private;
    | ORD(Asm.att_protected) : result := Api.FieldAttr.Family;
    END;
    IF  5 IN attr THEN INC(result, Api.FieldAttr.Static) END;
   (* what about Initonly? *)
    RETURN result;
  END toFieldAttr;

(* ------------------------------------------------ *)

  PROCEDURE (os : PeFile)MkCodeBuffer(),NEW;
  BEGIN
    ASSERT((defSrc # NIL) & (os.pePI.mthD # NIL));
    os.pePI.code := os.pePI.mthD.CreateCodeBuffer();
    os.pePI.code.OpenScope();
    os.pePI.code.set_DefaultSourceFile(defSrc);
  END MkCodeBuffer;

(* ============================================================ *)
(*                    Exported Methods                  *)
(* ============================================================ *)

  PROCEDURE (os : PeFile)MethodDecl*(attr : SET; proc : Id.Procs);
    VAR prcT : Ty.Procedure; (* NOT NEEDED? *)
        prcD : Api.MethodDef;
  BEGIN
   (*
    *   Set the various attributes
    *)
    prcD := os.mth(proc)(Api.MethodDef);
    prcD.AddMethAttribute(toMethAttr(attr));
    prcD.AddImplAttribute(ilAtt);
    os.pePI.mthD := prcD;
    IF attr * Asm.att_abstract = {} THEN os.MkCodeBuffer() END;
  END MethodDecl;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)DoExtern(blk : Id.BlkId),NEW;
 (*
  *  Add references to all imported assemblies.
  *)
    VAR  asmRef  : Api.AssemblyRef;
         blkXtn  : BlkXtn;
   (* ----------------------------------------- *)
    PROCEDURE AsmName(bk : Id.BlkId) : Lv.CharOpen;
      VAR ix : INTEGER;
          ln : INTEGER;
          ch : CHAR;
          cp : Lv.CharOpen;
    BEGIN
      IF Sy.isFn IN bk.xAttr THEN
        ln := 0;
        FOR ix := LEN(bk.scopeNm) - 1 TO 1 BY -1 DO
          IF bk.scopeNm[ix] = "]" THEN ln := ix END;
        END;
        IF (ln = 0 ) OR (bk.scopeNm[0] # '[') THEN 
                          RTS.Throw("bad extern name "+bk.scopeNm^) END;
        NEW(cp, ln);
        FOR ix := 1 TO ln-1 DO cp[ix-1] := bk.scopeNm[ix] END;
        cp[ln-1] := 0X;
        RETURN cp;
      ELSE
        RETURN bk.xName;
      END;
    END AsmName;
   (* ----------------------------------------- *)
    PROCEDURE MkBytes(t1, t2 : INTEGER) : POINTER TO ARRAY OF UBYTE;
      VAR bIx : INTEGER;
          tok : POINTER TO ARRAY OF UBYTE;
    BEGIN [UNCHECKED_ARITHMETIC]
      NEW(tok, 8);
      FOR bIx := 3 TO 0 BY -1 DO
        tok[bIx] := USHORT(t1 MOD 256);
        t1 := t1 DIV 256;
      END;
      FOR bIx := 7 TO 4 BY -1 DO
        tok[bIx] := USHORT(t2 MOD 256);
        t2 := t2 DIV 256;
      END;
      RETURN tok;
    END MkBytes;
   (* ----------------------------------------- *)
  BEGIN
    IF blk.xName = NIL THEN Mu.MkBlkName(blk) END;
    asmRef := os.peFl.MakeExternAssembly(MKSTR(AsmName(blk)^));
    NEW(blkXtn);
    blk.tgXtn := blkXtn;
    blkXtn.asmD := asmRef;
    blkXtn.dscD := getOrAddClass(asmRef, 
                                 MKSTR(blk.pkgNm^), 
                                 MKSTR(blk.clsNm^));
    IF blk.verNm # NIL THEN
      asmRef.AddVersionInfo(blk.verNm[0], blk.verNm[1], 
                            blk.verNm[2], blk.verNm[3]);
      IF (blk.verNm[4] # 0) OR (blk.verNm[5] # 0) THEN
        asmRef.AddKeyToken(MkBytes(blk.verNm[4], blk.verNm[5]));
      END;
    END;
  END DoExtern;

(* ============================================================ *)

  PROCEDURE (os : PeFile)DoRtsMod(blk : Id.BlkId),NEW;
 (*
  *  Add references to all imported assemblies.
  *)
    VAR blkD : BlkXtn;
  BEGIN
    IF blk.xName = NIL THEN Mu.MkBlkName(blk) END;
    NEW(blkD);
    blkD.asmD := os.rts;
    blkD.dscD := os.rts.AddClass("", MKSTR(blk.clsNm^));
    blk.tgXtn := blkD;
  END DoRtsMod;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CheckNestedClass*(typ : Ty.Record;
                                           scp : Sy.Scope;
                                           str : Lv.CharOpen);
    VAR len : INTEGER;
        idx : INTEGER;
        jdx : INTEGER;
        kdx : INTEGER;
        hsh : INTEGER;
        tId : Sy.Idnt;
  BEGIN
   (* 
    *  Find last occurrence of '$', except at index 0  
    *  
    *  We seek the last occurrence because this method might
    *  be called recursively for a deeply nested class A$B$C.
    *)
    len := LEN(str$); (* LEN(x$) doen't count nul, therefore str[len] = 0X *)
    FOR idx := len TO 1 BY -1 DO
      IF str[idx] = '$' THEN (* a nested class *)
        str[idx] := 0X; (* terminate the string early *)
        hsh := NameHash.enterStr(str);
        tId := Sy.bind(hsh, scp);
        
        IF (tId = NIL) OR ~(tId IS Id.TypId) THEN 
          RTS.Throw(
             "Foreign Class <" + str^ + "> not found in <" + typ.extrnNm^ + ">"
          );
        ELSE
          typ.encCls := tId.type.boundRecTp();
          jdx := 0; kdx := idx+1;
          WHILE kdx <= len DO str[jdx] := str[kdx]; INC(kdx); INC(jdx) END;
        END;
        RETURN;
      END;
    END;
  END CheckNestedClass;

(* ============================================================ *)

  PROCEDURE (os : PeFile)ExternList*();
    VAR idx : INTEGER;
        blk : Id.BlkId;
  BEGIN
    FOR idx := 0 TO CSt.impSeq.tide-1 DO
      blk := CSt.impSeq.a[idx](Id.BlkId);
      IF (Sy.need IN blk.xAttr)  &
         (blk.tgXtn = NIL) THEN 
        IF ~(Sy.rtsMd IN blk.xAttr) THEN 
          os.DoExtern(blk);
        ELSE
          os.DoRtsMod(blk);
        END;
      END;
    END;
  END ExternList;

(* ============================================================ *)

  PROCEDURE (os : PeFile)DefLab*(l : Mu.Label);
  BEGIN
    os.pePI.code.CodeLabel(l(PeLab).labl);
  END DefLab;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)DefLabC*(l : Mu.Label; IN c : ARRAY OF CHAR);
  BEGIN
    os.pePI.code.CodeLabel(l(PeLab).labl);
  END DefLabC;

(* ============================================================ *)

  PROCEDURE (os : PeFile)Code*(code : INTEGER);
  BEGIN
    os.pePI.code.Inst(Asm.cd[code]);
    os.Adjust(Asm.dl[code]);
  END Code;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeF(code : INTEGER;
                               fld  : Api.Field), NEW;
  BEGIN
    os.pePI.code.FieldInst(Asm.cd[code], fld);
    os.Adjust(Asm.dl[code]);
  END CodeF;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeI*(code,int : INTEGER);
  BEGIN
    os.pePI.code.IntInst(Asm.cd[code],int);
    os.Adjust(Asm.dl[code]);
  END CodeI;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeT*(code : INTEGER; type : Sy.Type);
    VAR xtn : Api.Type;
  BEGIN
    xtn := os.typ(type);
    os.pePI.code.TypeInst(Asm.cd[code], xtn);
    os.Adjust(Asm.dl[code]);
  END CodeT;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeTn*(code : INTEGER; type : Sy.Type);
    VAR xtn : Api.Type;
  BEGIN
    xtn := os.typ(type);
    os.pePI.code.TypeInst(Asm.cd[code], xtn);
    os.Adjust(Asm.dl[code]);
  END CodeTn;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeL*(code : INTEGER; long : LONGINT);
  BEGIN
    ASSERT(code = Asm.opc_ldc_i8);
    os.pePI.code.ldc_i8(long);
    os.Adjust(1);
  END CodeL;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeR*(code : INTEGER; real : REAL);
  BEGIN
    IF code = Asm.opc_ldc_r8 THEN
      os.pePI.code.ldc_r8(real);
    ELSIF code = Asm.opc_ldc_r4 THEN
      os.pePI.code.ldc_r4(SHORT(real));
    ELSE
      ASSERT(FALSE);
    END;
    os.Adjust(1);
  END CodeR;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeLb*(code : INTEGER; labl : Mu.Label);
  BEGIN
    os.pePI.code.Branch(Asm.cd[code], labl(PeLab).labl);
  END CodeLb;

(* ============================================================ *)

  PROCEDURE (os : PeFile)getMethod(s : INTEGER) : Api.Method,NEW;
    VAR  mth : Api.MethodRef;
         cpr : Api.ClassRef;
         msc : Api.ClassRef;
         sys : Api.ClassRef;
  (* ----------------------------------- *)
    PROCEDURE p1(p : Api.Type) : TypArr;
      VAR a : TypArr;
    BEGIN
      NEW(a,1);
      a[0] := p;
      RETURN a;
    END p1;
  (* ----------------------------------- *)
    PROCEDURE p2(p,q : Api.Type) : TypArr;
      VAR a : TypArr;
    BEGIN
      NEW(a,2);
      a[0] := p;
      a[1] := q;
      RETURN a;
    END p2;
  (* ----------------------------------- *)
  BEGIN
   (*
    *  Lazy evaluation of array elements
    *)
    mth := rHelper[s];
    IF mth = NIL THEN
      cpr := os.cprts;
      CASE s OF
      | Mu.vStr2ChO  : mth := cpr.AddMethod("strToChO",charA,p1(strgD));
      | Mu.vStr2ChF  : mth := cpr.AddMethod("StrToChF",voidD,p2(charA,strgD));
      | Mu.aStrLen   : mth := cpr.AddMethod("chrArrLength",int4D,p1(charA));
      | Mu.aStrChk   : mth := cpr.AddMethod("ChrArrCheck",voidD,p1(charA));
      | Mu.aStrLp1   : mth := cpr.AddMethod("chrArrLplus1",int4D,p1(charA));
      | Mu.aaStrCmp  : mth := cpr.AddMethod("strCmp",int4D,p2(charA,charA));
      | Mu.aaStrCopy : mth := cpr.AddMethod("Stringify",voidD,p2(charA,charA));
      | Mu.CpModI    : mth := cpr.AddMethod("CpModI",int4D,p2(int4D,int4D));
      | Mu.CpDivI    : mth := cpr.AddMethod("CpDivI",int4D,p2(int4D,int4D));
      | Mu.CpModL    : mth := cpr.AddMethod("CpModL",int8D,p2(int8D,int8D));
      | Mu.CpDivL    : mth := cpr.AddMethod("CpDivL",int8D,p2(int8D,int8D));
      | Mu.caseMesg  : mth := cpr.AddMethod("caseMesg",strgD,p1(int4D));
      | Mu.withMesg  : mth := cpr.AddMethod("withMesg",strgD,p1(objtD));
      | Mu.chs2Str   : mth :=  cpr.AddMethod("mkStr",strgD,p1(charA));
      | Mu.CPJstrCatAA : mth := cpr.AddMethod("aaToStr",strgD,p2(charA,charA));
      | Mu.CPJstrCatSA : mth := cpr.AddMethod("saToStr",strgD,p2(strgD,charA));
      | Mu.CPJstrCatAS : mth := cpr.AddMethod("asToStr",strgD,p2(charA,strgD));
      | Mu.CPJstrCatSS : mth := cpr.AddMethod("ssToStr",strgD,p2(strgD,strgD));

      | Mu.toUpper   :  sys := getOrAddClass(corlib, "System", "Char");
                        mth := getOrAddMethod(sys,"ToUpper",charD,p1(charD));

      | Mu.sysExit   :  IF envrCls = NIL THEN
                          envrCls := 
                               getOrAddClass(corlib, "System", "Environment");
                        END;
                        mth := getOrAddMethod(envrCls,"Exit",voidD,p1(int4D));

      | Mu.mkExcept  :  IF excpCls = NIL THEN
                          IF CSt.ntvExc.tgXtn = NIL THEN
                            excpCls := 
                                  getOrAddClass(corlib, "System", "Exception");
                            CSt.ntvExc.tgXtn := excpCls;
                          ELSE
                            excpCls := CSt.ntvExc.tgXtn(Api.ClassRef);
                          END;
                        END;
                        sys := CSt.ntvExc.tgXtn(Api.ClassRef);
(*
 *                      mth := sys.AddMethod(ctorS,voidD,p1(strgD));
 *)
                        mth := getOrAddMethod(sys,ctorS,voidD,p1(strgD));
                        mth.AddCallConv(Api.CallConv.Instance);

      | Mu.getTpM    :  IF CSt.ntvTyp.tgXtn = NIL THEN
                          CSt.ntvTyp.tgXtn := 
                                  getOrAddClass(corlib, "System", "Type");
                        END;
                        sys := CSt.ntvTyp.tgXtn(Api.ClassRef);
                        mth := getOrAddMethod(sys,"GetType",sys,NIL);
                        mth.AddCallConv(Api.CallConv.Instance);

      | Mu.dFloor, Mu.dAbs, Mu.fAbs, Mu.iAbs, Mu.lAbs :
          IF mathCls = NIL THEN
            mathCls := getOrAddClass(corlib, "System", "Math");
          END;
          rHelper[Mu.dFloor] := getOrAddMethod(mathCls,"Floor",flt8D,p1(flt8D));
          rHelper[Mu.dAbs]   := getOrAddMethod(mathCls,"Abs",flt8D,p1(flt8D));
          rHelper[Mu.fAbs]   := getOrAddMethod(mathCls,"Abs",flt4D,p1(flt4D));
          rHelper[Mu.iAbs]   := getOrAddMethod(mathCls,"Abs",int4D,p1(int4D));
          rHelper[Mu.lAbs]   := getOrAddMethod(mathCls,"Abs",int8D,p1(int8D));
          mth := rHelper[s];
      END;
      rHelper[s] := mth;
    END;
    RETURN mth;
  END getMethod;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)StaticCall*(s : INTEGER; d : INTEGER);
    VAR mth : Api.Method;
  BEGIN
    mth := os.getMethod(s);
    os.pePI.code.MethInst(Asm.cd[Asm.opc_call], mth);
    os.Adjust(d);
  END StaticCall;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeS*(code : INTEGER; str : INTEGER);
    VAR mth : Api.Method;
  BEGIN
    mth := os.getMethod(str);
    os.pePI.code.MethInst(Asm.cd[code], mth);
  END CodeS;

(* ============================================================ *)

  PROCEDURE (os : PeFile)Try*();
    VAR retT : Sy.Type;
  BEGIN
    os.proc.exLb := os.newLabel();
    retT := os.proc.prId.type.returnType();
    IF retT # NIL THEN os.proc.rtLc := os.proc.newLocal(retT) END;
    os.pePI.code.StartBlock();
  END Try;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)Catch*(proc : Id.Procs);
  BEGIN
    os.pePI.tryB := os.pePI.code.EndTryBlock();
    os.pePI.code.StartBlock();
    os.Adjust(1);	(* allow for incoming exception reference *)
    os.StoreLocal(proc.except.varOrd);
  END Catch;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CloseCatch*();
  BEGIN
    IF excpCls = NIL THEN
      IF CSt.ntvExc.tgXtn = NIL THEN
        excpCls := getOrAddClass(corlib, "System", "Exception");
        CSt.ntvExc.tgXtn := excpCls;
      ELSE
        excpCls := CSt.ntvExc.tgXtn(Api.ClassRef);
      END;
    END;
    os.pePI.code.EndCatchBlock(excpCls, os.pePI.tryB);
  END CloseCatch;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CopyCall*(typ : Ty.Record);
  BEGIN
    os.pePI.code.MethInst(Asm.cd[Asm.opc_call], os.cpy(typ));
    os.Adjust(-2);
  END CopyCall;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)PushStr*(IN str : ARRAY OF CHAR);
  (* Use target quoting conventions for the literal string *)
  BEGIN
    (* os.pePI.code.ldstr(MKSTR(str)); *)
    os.pePI.code.ldstr(Sys.String.init(BOX(str), 0, LEN(str) - 1));
    os.Adjust(1);
  END PushStr;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CallIT*(code : INTEGER; 
                                 proc : Id.Procs; 
                                 type : Ty.Procedure);
    VAR xtn : Api.Method;
  BEGIN
    xtn := os.mth(proc);
    os.pePI.code.MethInst(Asm.cd[code], xtn);
    os.Adjust(type.retN - type.argN);        
  END CallIT;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CallCT*(proc : Id.Procs; 
                                 type : Ty.Procedure);
    VAR xtn : Api.Method;
  BEGIN
    ASSERT(proc.tgXtn # NIL);
    xtn := proc.tgXtn(Api.Method);
    os.pePI.code.MethInst(Asm.cd[Asm.opc_newobj], xtn);
    os.Adjust(-type.argN);        
  END CallCT;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CallDelegate*(typ : Ty.Procedure);
    VAR xtn : Api.Method;
  BEGIN
    ASSERT(typ.tgXtn # NIL);
(*
 *  xtn := typ.tgXtn(DelXtn).invD;
 *)
    xtn := os.dxt(typ).invD;
    os.pePI.code.MethInst(Asm.cd[Asm.opc_callvirt], xtn);
    os.Adjust(-typ.argN + typ.retN);        
  END CallDelegate;

(* ============================================================ *)

  PROCEDURE (os : PeFile)PutGetS*(code : INTEGER;
                                  blk  : Id.BlkId;
                                  fId  : Id.VarId);
  (* Emit putstatic and getstatic for static field *)
  BEGIN
    os.pePI.code.FieldInst(Asm.cd[code], os.fld(fId));
    os.Adjust(Asm.dl[code]);
  END PutGetS;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)GetValObj*(code : INTEGER; ptrT : Ty.Pointer);
    VAR rTp : Ty.Record;
  BEGIN
    rTp := ptrT.boundRecTp()(Ty.Record);
    os.pePI.code.FieldInst(Asm.cd[code], os.vDl(rTp));
    os.Adjust(Asm.dl[code]);
  END GetValObj;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)PutGetXhr*(code : INTEGER; 
                                    proc : Id.Procs; 
                                    locl : Id.LocId);
    VAR ix   : INTEGER;
        name : Lv.CharOpen;
        recT : Ty.Record;
        fldI : Id.FldId;
  BEGIN
    ix := 0;
    recT := proc.xhrType.boundRecTp()(Ty.Record);
    WHILE recT.fields.a[ix].hash # locl.hash DO INC(ix) END;;
    os.pePI.code.FieldInst(Asm.cd[code], os.fld(recT.fields.a[ix](Id.FldId)));
  END PutGetXhr;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)PutGetF*(code : INTEGER;
                                  fId  : Id.FldId);
  BEGIN
    os.pePI.code.FieldInst(Asm.cd[code], os.fld(fId));
    os.Adjust(Asm.dl[code]);
  END PutGetF;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE (os : PeFile)MkNewRecord*(typ : Ty.Record);
    CONST code = Asm.opc_newobj;
    VAR   name : Lv.CharOpen;
  BEGIN
   (*
    *  We need "newobj instance void <name>::.ctor()"
    *)
    os.pePI.code.MethInst(Asm.cd[code], os.new(typ));
    os.Adjust(1);
  END MkNewRecord;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE (os : PeFile)MkNewProcVal*(p : Sy.Idnt;   (* src Proc *)
                                       t : Sy.Type);  (* dst Type *)
    VAR ctor : Api.Method;
        ldfi : INTEGER;
        pTyp : Ty.Procedure;
        proc : Id.Procs;
  BEGIN
(*
 *  ctor := t.tgXtn(DelXtn).newD;
 *)
    proc := p(Id.Procs);
    pTyp := t(Ty.Procedure);
    ctor := os.dxt(pTyp).newD;
   (*
    *  We need "ldftn [instance] <retType> <procName>
    *)
    WITH p : Id.MthId DO
      IF p.bndType.isInterfaceType() THEN
        ldfi := Asm.opc_ldvirtftn;
      ELSIF p.mthAtt * Id.mask = Id.final THEN
        ldfi := Asm.opc_ldftn;
      ELSE
        ldfi := Asm.opc_ldvirtftn;
      END;
    ELSE
      ldfi := Asm.opc_ldftn;
    END;
   (*
    *  These next are needed for imported events
    *)
    Mu.MkProcName(proc, os);
    os.NumberParams(proc, pTyp);
   (*
    *   If this will be a virtual method call, then we
    *   must duplicate the receiver, since the call of
    *   ldvirtftn uses up one copy.
    *)
    IF ldfi = Asm.opc_ldvirtftn THEN os.Code(Asm.opc_dup) END;
    os.pePI.code.MethInst(Asm.cd[ldfi], os.mth(proc));
    os.Adjust(1);
   (*
    *  Now we need "newobj instance void <name>::.ctor(...)"
    *)
    os.pePI.code.MethInst(Asm.cd[Asm.opc_newobj], ctor);
    os.Adjust(-2);
  END MkNewProcVal;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CallSuper*(rTp : Ty.Record;
                                    prc : Id.PrcId);
    VAR pNm : INTEGER;
        spr : Api.Method;
  (* ---------------------------------------- *)
    PROCEDURE getSuperCtor(os  : PeFile; 
                           rTp : Ty.Record; 
                           prc : Id.Procs) : Api.Method;
      VAR bas : Ty.Record;
          pTp : Ty.Procedure;
          bcl : Api.Class;
          mth : Api.Method;
    BEGIN
      bas := rTp.superType();
      IF prc # NIL THEN
       (*
        *  This constructor has arguments.
        *  The super constructor is prc.basCll.sprCtor
        *)
        pTp := prc.type(Ty.Procedure);
        IF prc.tgXtn = NIL THEN
          bcl := os.cls(bas);
          WITH bcl : Api.ClassDef DO
              mth := MkMthDef(os, FALSE, pTp, bcl, ctorS);
              mth(Api.MethodDef).AddMethAttribute(ctAtt);
          | bcl : Api.ClassRef DO
              mth := MkMthRef(os, pTp, bcl, ctorS);
          END;
          mth.AddCallConv(Api.CallConv.Instance);
          prc.tgXtn := mth;
          RETURN mth;
        ELSE
          RETURN prc.tgXtn(Api.Method);
        END;
      ELSIF (bas # NIL) & (rTp.baseTp # Bi.anyRec) THEN
       (*
        * This is the explicit noarg constructor of the supertype.
        *)
        RETURN os.new(bas);
      ELSE
       (*
        *  This is System.Object::.ctor()
        *)
        RETURN newObjt;
      END;
    END getSuperCtor;
  (* ---------------------------------------- *)
  BEGIN
    IF prc # NIL THEN 
      pNm := prc.type(Ty.Procedure).formals.tide;
    ELSE 
      pNm := 0;
    END;
    spr := getSuperCtor(os, rTp, prc);
    os.pePI.code.MethInst(Asm.cd[Asm.opc_call], spr);
    os.Adjust(-(pNm+1));
  END CallSuper;

(* ============================================================ *)

  PROCEDURE (os : PeFile)InitHead*(rTp : Ty.Record;
                                   prc : Id.PrcId);
    VAR mDf : Api.MethodDef;
        cDf : Api.ClassDef;
  BEGIN
    cDf := os.cls(rTp)(Api.ClassDef);

    IF prc # NIL THEN
      mDf := prc.tgXtn(Api.MethodDef);
      mDf.AddMethAttribute(ctAtt);
    ELSE
      mDf := os.new(rTp)(Api.MethodDef);
    END;
    os.pePI.mthD := mDf;
    os.MkCodeBuffer();
    mDf.AddCallConv(Api.CallConv.Instance);
   (*
    *   Now we initialize the supertype;
    *)
    os.Code(Asm.opc_ldarg_0);
  END InitHead;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CopyHead*(typ : Ty.Record);
    VAR mDf : Api.MethodDef;
        cDf : Api.ClassDef;
        par : Id.ParId;
        prs : POINTER TO ARRAY OF Id.ParId;
  BEGIN
    cDf := os.cls(typ)(Api.ClassDef);
    mDf := os.cpy(typ)(Api.MethodDef);
    mDf.AddMethAttribute(Api.MethAttr.Public);
    mDf.AddImplAttribute(ilAtt);
    mDf.AddCallConv(Api.CallConv.Instance);
    os.pePI.mthD := mDf;
    os.MkCodeBuffer();
  END CopyHead;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MarkInterfaces*(IN seq : Sy.TypeSeq);
    VAR index  : INTEGER;
        tideX  : INTEGER;
        implT  : Ty.Record;
  BEGIN
    tideX := seq.tide-1;
    ASSERT(tideX >= 0);
    FOR index := 0 TO tideX DO
      implT := seq.a[index].boundRecTp()(Ty.Record);
      os.clsD.AddImplementedInterface(os.cls(implT));
    END;
  END MarkInterfaces;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MainHead*(xAtt : SET);
    VAR mthD : Api.MethodDef;

    VAR strA : Api.Type;
        list : Api.Field;
        pars : POINTER TO ARRAY OF Api.Param;
  BEGIN 
    NEW(pars, 1);
    strA := Api.ZeroBasedArray.init(strgD);
    pars[0] := Api.Param.init(0, "@args", strA);

    IF Sy.wMain IN xAtt THEN
      mthD := os.clsS.AddMethod(psAtt, ilAtt, ".WinMain", voidD, pars);
    ELSE (* Sy.cMain IN xAtt THEN *)
      mthD := os.clsS.AddMethod(psAtt, ilAtt, ".CPmain", voidD, pars);
    END;
    os.pePI.mthD := mthD;
    os.MkCodeBuffer();
    mthD.DeclareEntryPoint();
    IF CSt.debug THEN os.LineSpan(Scn.mkSpanT(CSt.thisMod.begTok)) END;
   (*
    *  Save the command-line arguments to the RTS.
    *)
    os.Code(Asm.opc_ldarg_0);
    os.CodeF(Asm.opc_stsfld, os.fld(CSt.argLst));
  END MainHead;

(* ============================================================ *)

  PROCEDURE (os : PeFile)SubSys*(xAtt : SET);
  BEGIN
    IF Sy.wMain IN xAtt THEN os.peFl.SetSubSystem(2) END;
  END SubSys;

(* ============================================================ *)

  PROCEDURE (os : PeFile)StartBoxClass*(rec : Ty.Record;
                                        att : SET;
                                        blk : Id.BlkId);
    VAR mthD : Api.MethodDef;
        sprC : Api.Method;
        boxC : Api.ClassDef;
  BEGIN
    boxC := rec.tgXtn(RecXtn).boxD(Api.ClassDef);
    boxC.AddAttribute(toTypeAttr(att));

   (*
    *   Emit the no-arg constructor
    *)
    os.MkNewProcInfo(blk);
    mthD := os.new(rec)(Api.MethodDef);
    os.pePI.mthD := mthD;
    os.MkCodeBuffer();
    mthD.AddCallConv(Api.CallConv.Instance);

    os.Code(Asm.opc_ldarg_0);
    sprC := newObjt;

    os.pePI.code.MethInst(Asm.cd[Asm.opc_call], sprC);
    os.InitHead(rec, NIL);
    os.CallSuper(rec, NIL);
    os.Code(Asm.opc_ret);
    os.Locals();
    os.InitTail(rec);
    os.pePI := NIL;
    os.proc := NIL;
   (*
    *   Copies of value classes are always done inline.
    *)
  END StartBoxClass;

(* ============================================================ *)

  PROCEDURE (os : PeFile)Tail(),NEW;
  BEGIN
    os.Locals();
    os.pePI.code.CloseScope();  (* Needed for PERWAPI pdb files *)
    os.pePI := NIL;
    os.proc := NIL;
  END Tail;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MainTail*();
  BEGIN os.Tail() END MainTail;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : PeFile)MethodTail*(id : Id.Procs);
  BEGIN os.Tail() END MethodTail;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : PeFile)ClinitTail*();
  BEGIN os.Tail() END ClinitTail;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : PeFile)CopyTail*();
  BEGIN os.Tail() END CopyTail;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : PeFile)InitTail*(typ : Ty.Record);
  BEGIN os.Tail() END InitTail;

(* ============================================================ *)

  PROCEDURE (os : PeFile)ClinitHead*();
    VAR mAtt : INTEGER;
  BEGIN
    mAtt := ctAtt + Api.MethAttr.Static;
    os.pePI.mthD := os.clsS.AddMethod(mAtt, ilAtt, ".cctor", voidD, NIL);
    os.MkCodeBuffer();
    IF CSt.debug THEN
      os.pePI.code.IntLine(CSt.thisMod.token.lin, 
                           CSt.thisMod.token.col, 
                           CSt.thisMod.token.lin,
                           CSt.thisMod.token.col + CSt.thisMod.token.len);
      os.Code(Asm.opc_nop);
    END; 
  END ClinitHead;

(* ============================================================ *)

  PROCEDURE (os : PeFile)EmitField*(id : Id.AbVar; att : SET);
    VAR fDf : Api.FieldDef;
  BEGIN
    fDf := os.fld(id)(Api.FieldDef);
    fDf.AddFieldAttr(toFieldAttr(att));
  END EmitField;

(* ============================================================ *)
(*           Start of Procedure Variable and Event Stuff            *)
(* ============================================================ *)

  PROCEDURE MkAddRem(os : PeFile; fId : Id.AbVar);
    VAR xtn : EvtXtn;
        fXt : Api.Field;
        clD : Api.Class;
        namS : Lv.CharOpen;
        typA : POINTER TO ARRAY OF Api.Type;
        parA : POINTER TO ARRAY OF Api.Param;
   (* -------------------------------- *)
    PROCEDURE GetClass(os : PeFile; 
                       id : Id.AbVar;
                   OUT cl : Api.Class;
                   OUT nm : Lv.CharOpen);
    BEGIN
      WITH id : Id.FldId DO
           cl := os.cls(id.recTyp(Ty.Record));
           nm := id.fldNm;
      | id : Id.VarId DO
           IF id.recTyp # NIL THEN cl:= os.cls(id.recTyp(Ty.Record));
           ELSE cl:= os.dsc(id.dfScp(Id.BlkId));
           END;
           nm := id.varNm;
      END;
    END GetClass;
   (* -------------------------------- *)
  BEGIN
   (*
    *  First, need to ensure that there is a field 
    *  descriptor created for this variable.
    *)
    IF fId.tgXtn = NIL THEN 
      fXt := os.fld(fId);
    ELSE 
      fXt := fId.tgXtn(Api.Field);
    END;
   (*
    *  Now allocate the Event Extension object.
    *)
    NEW(xtn);
    xtn.fldD := fXt;
   (*
    *  Now create the MethodRef or MethodDef descriptors
    *  for add_<fieldname>() and remove_<fieldname>()
    *)
    GetClass(os, fId, clD, namS);
    WITH clD : Api.ClassDef DO
          NEW(parA, 1);
          parA[0] := Api.Param.init(0, "ev", os.typ(fId.type)); 
          xtn.addD := clD.AddMethod(MKSTR(evtAdd^ + namS^), voidD, parA);
          xtn.remD := clD.AddMethod(MKSTR(evtRem^ + namS^), voidD, parA);
    | clD : Api.ClassRef DO
          NEW(typA, 1);
          typA[0] := os.typ(fId.type); 
          xtn.addD := clD.AddMethod(MKSTR(evtAdd^ + namS^), voidD, typA);
          xtn.remD := clD.AddMethod(MKSTR(evtRem^ + namS^), voidD, typA);
    END;
    fId.tgXtn := xtn;
  END MkAddRem;

(* ============================================================ *)

  PROCEDURE (os : PeFile)EmitEventMethods*(id : Id.AbVar);
    CONST att  = Api.MethAttr.Public + Api.MethAttr.SpecialName;
    VAR   eTp  : Ty.Event;
          evt  : Api.Event;
          addD  : Api.MethodDef;
          remD  : Api.MethodDef;
   (* ------------------------------------------------- *)
    PROCEDURE EmitEvtMth(os  : PeFile; 
                         id  : Id.AbVar;
                         add : BOOLEAN; 
                         mth : Api.MethodDef);
      VAR pFix : Lv.CharOpen;
          mStr : RTS.NativeString;
          mthD : Api.MethodDef;
          parA : POINTER TO ARRAY OF Api.Param;
    BEGIN
      os.MkNewProcInfo(NIL);
      WITH id : Id.FldId DO
          mth.AddMethAttribute(att);
          mth.AddCallConv(Api.CallConv.Instance);
          mth.AddImplAttribute(ilAtt + Api.ImplAttr.Synchronised);
          os.pePI.mthD := mth;
          os.MkCodeBuffer();
          os.Code(Asm.opc_ldarg_0);
          os.Code(Asm.opc_ldarg_0);
          os.PutGetF(Asm.opc_ldfld, id);
          os.Code(Asm.opc_ldarg_1);
          os.CallCombine(id.type, add);
          os.PutGetF(Asm.opc_stfld, id);
      | id : Id.VarId DO
          mth.AddMethAttribute(att + Api.MethAttr.Static);
          mth.AddImplAttribute(ilAtt + Api.ImplAttr.Synchronised);
          os.pePI.mthD := mth;
          os.MkCodeBuffer(); 
          os.PutGetS(Asm.opc_ldsfld, id.dfScp(Id.BlkId), id);
          os.Code(Asm.opc_ldarg_0);
          os.CallCombine(id.type, add);
          os.PutGetS(Asm.opc_stsfld, id.dfScp(Id.BlkId),id);
      END;
      os.Code(Asm.opc_ret);
      os.Tail();
    END EmitEvtMth;
   (* ------------------------------------------------- *)
  BEGIN
   (*
    *  Emit the "add_*" method
    *)
    addD := os.add(id)(Api.MethodDef);
    EmitEvtMth(os, id, TRUE, addD);
   (*
    *  Emit the "remove_*" method
    *)
    remD := os.rem(id)(Api.MethodDef);
    EmitEvtMth(os, id, FALSE, remD);
   (*
    *  Emit the .event declaration" 
    *)
    WITH id : Id.FldId DO
        evt := os.clsD.AddEvent(MKSTR(id.fldNm^), os.typ(id.type));
    | id : Id.VarId DO
        evt := os.clsD.AddEvent(MKSTR(id.varNm^), os.typ(id.type));
    END;
    evt.AddMethod(addD, Api.MethodType.AddOn);
    evt.AddMethod(remD, Api.MethodType.RemoveOn);
  END EmitEventMethods;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CallCombine(typ : Sy.Type;
                                     add : BOOLEAN),NEW;
    VAR xtn : Api.Method;
  BEGIN
    IF add THEN xtn := os.cmb() ELSE xtn := os.rmv() END; 
    os.pePI.code.MethInst(Asm.cd[Asm.opc_call], xtn);
    os.Adjust(-1);        
    os.CodeT(Asm.opc_castclass, typ);
  END CallCombine;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkAndLinkDelegate*(dl  : Sy.Idnt;
                                            id  : Sy.Idnt;
                                            ty  : Sy.Type;
                                            isA : BOOLEAN);
   (* --------------------------------------------------------- *)
    VAR rcv : INTEGER;
        mth : Api.Method;
   (* --------------------------------------------------------- *)
  BEGIN
    WITH id : Id.FldId DO
       (*
        *      <push handle>                  // ... already done
        *      <push receiver (or nil)>       // ... already done
        *      <make new proc value>          // ... still to do
        *      call      instance void A.B::add_fld(class tyName)
        *)
        os.MkNewProcVal(dl, ty);
        IF isA THEN mth := os.add(id) ELSE mth := os.rem(id) END;
        mth.AddCallConv(Api.CallConv.Instance);
        os.pePI.code.MethInst(Asm.cd[Asm.opc_call], mth);
    | id : Id.VarId DO
       (*
        *      <push receiver (or nil)>      // ... already done
        *      <make new proc value>            // ... still to do
        *      call      void A.B::add_fld(class tyName)
        *)
        os.MkNewProcVal(dl, ty);
        IF isA THEN mth := os.add(id) ELSE mth := os.rem(id) END;
        os.pePI.code.MethInst(Asm.cd[Asm.opc_call], mth);
    | id : Id.LocId DO
       (*
        *      <save receiver>      
        *      ldloc      'local'
        *      <restore receiver>      
        *      <make new proc value>            // ... still to do
        *      call      class D D::Combine(class D, class D)
        *)
        rcv := os.proc.newLocal(CSt.ntvObj);
        os.StoreLocal(rcv);
        os.GetLocal(id);
        os.PushLocal(rcv);
        os.MkNewProcVal(dl, ty);
        os.CallCombine(ty, isA);
        os.PutLocal(id); 
    END;
  END MkAndLinkDelegate;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE (os : PeFile)EmitPTypeBody*(tId : Id.TypId);
  BEGIN
    ASSERT(tId.tgXtn # NIL);
  END EmitPTypeBody;

(* ============================================================ *)
(*          End of Procedure Variable and Event Stuff           *)
(* ============================================================ *)

  PROCEDURE (os : PeFile)Line*(nm : INTEGER);
  BEGIN
    os.pePI.code.IntLine(nm,1,nm,100);
    (*IF CSt.debug THEN os.Code(Asm.opc_nop) END;*)
  END Line;
 
  PROCEDURE (os : PeFile)LinePlus*(lin, col : INTEGER); 
  BEGIN
    (*IF CSt.debug THEN os.Code(Asm.opc_nop) END;*)
    os.pePI.code.IntLine(lin,1,lin,col); 
  END LinePlus;
  
  PROCEDURE (os : PeFile)LineSpan*(s : Scn.Span);
  BEGIN
    IF s # NIL THEN 
         os.pePI.code.IntLine(s.sLin, s.sCol, s.eLin, s.eCol) END;     
  END LineSpan;

(* ============================================================ *)

  PROCEDURE (os : PeFile)Locals(),NEW;
  (** Declare the local of this method. *)
    VAR count : INTEGER;
        index : INTEGER;
        prcId : Sy.Scope;
        locId : Id.LocId;
        methD : Api.MethodDef;
        loclA : POINTER TO ARRAY OF Api.Local;
        boolA : POINTER TO ARRAY OF BOOLEAN;
        lBind : Api.LocalBinding;
  BEGIN
    methD := os.pePI.mthD;
   (*
    *   If dMax < 8, leave maxstack as default 
    *)
    IF os.proc.dMax > 8 THEN 
      methD.SetMaxStack(os.proc.dMax);
    ELSE
      methD.SetMaxStack(8);
    END;
    NEW(loclA, os.proc.tLst.tide);
    NEW(boolA, os.proc.tLst.tide);

    count := 0;
    IF os.proc.prId # NIL THEN 
      prcId := os.proc.prId;
      WITH prcId : Id.Procs DO
        IF Id.hasXHR IN prcId.pAttr THEN
          loclA[count] := Api.Local.init("", os.typ(prcId.xhrType)); 
          INC(count);
        END;
        FOR index := 0 TO prcId.locals.tide-1 DO
          locId := prcId.locals.a[index](Id.LocId);
          IF ~(locId IS Id.ParId) & (locId.varOrd # Id.xMark) THEN
            loclA[count] := Api.Local.init(nms(locId), os.typ(locId.type));
            IF CSt.debug THEN boolA[count] := TRUE END;
            INC(count);
          END;
        END;
      ELSE (* nothing for module blocks *)
      END;
    END;
    WHILE count < os.proc.tLst.tide DO 
      loclA[count] := Api.Local.init("", os.typ(os.proc.tLst.a[count])); 
      INC(count);
    END;
    IF count > 0 THEN methD.AddLocals(loclA, TRUE) END;
    FOR index := 0 TO count-1 DO
      IF boolA[index] THEN lBind := os.pePI.code.BindLocal(loclA[index]) END;
    END;
  END Locals;

(* ============================================================ *)

  PROCEDURE (os : PeFile)LoadType*(id : Sy.Idnt);
   (* ---------------------------------- *)
    PROCEDURE getLdTyp(os : PeFile) : Api.MethodRef;
      VAR typD : Api.ClassRef;
          rthA : POINTER TO ARRAY OF Api.Type;
    BEGIN
      IF loadTyp = NIL THEN
       (*
        *  Make params for the call
        *)
        NEW(rthA, 1);
        IF rtTpHdl = NIL THEN
          rtTpHdl := getOrAddValueClass(corlib, "System", "RuntimeTypeHandle");
        END;
        rthA[0] := rtTpHdl;
       (*
        *  Make receiver/result type descriptor
        *)
        IF CSt.ntvTyp.tgXtn = NIL THEN
          CSt.ntvTyp.tgXtn := getOrAddClass(corlib, "System", "Type");
        END;
        typD := CSt.ntvTyp.tgXtn(Api.ClassRef);
        loadTyp := getOrAddMethod(typD, "GetTypeFromHandle", typD, rthA);
      END;
      RETURN loadTyp;
    END getLdTyp;
   (* ---------------------------------- *)
  BEGIN
   (*
    *    ldtoken <Type>
    *    call class [mscorlib]System.Type 
    *           [mscorlib]System.Type::GetTypeFromHandle(
    *                     value class [mscorlib]System.RuntimeTypeHandle)
    *)
    os.CodeT(Asm.opc_ldtoken, id.type);
    os.pePI.code.MethInst(Asm.cd[Asm.opc_call], getLdTyp(os));
  END LoadType;

(* ============================================================ *)

  PROCEDURE (os : PeFile)Finish*();
   (*(* ------------------------------------ *)
    PROCEDURE MakeDebuggable(pef : Api.PEFile);
      VAR thisAssm : Api.Assembly;
          debugRef : Api.ClassRef;
          dbugCtor : Api.MethodRef;
          trueCnst : Api.BoolConst;
          twoBools : TypArr;
          dbugArgs : POINTER TO ARRAY OF Api.Constant;
    BEGIN
      thisAssm := pef.GetThisAssembly();
      debugRef := getOrAddClass(corlib, "System.Diagnostics", "DebuggableAttribute");
      NEW(twoBools, 2);
      NEW(dbugArgs, 2);
      twoBools[0] := Api.PrimitiveType.Boolean;
      twoBools[1] := Api.PrimitiveType.Boolean;
      dbugArgs[0] := Api.BoolConst.init(TRUE);
      dbugArgs[1] := Api.BoolConst.init(TRUE);
      dbugCtor := getOrAddMethod(debugRef, ctorS, voidD, twoBools)(Api.MethodRef);
      dbugCtor.AddCallConv(Api.CallConv.Instance);
      thisAssm.AddCustomAttribute(dbugCtor, dbugArgs);  
    END MakeDebuggable;
   (* ------------------------------------ *)*)
  BEGIN
    IF CSt.debug THEN os.peFl.MakeDebuggable(TRUE, TRUE) END; 
    (* bake the assembly ... *)
    os.peFl.WritePEFile(CSt.debug); 
  END Finish;

(* ============================================================ *)

  PROCEDURE (os : PeFile)RefRTS*();
    VAR i : INTEGER;
        xhrRc : Ty.Record;
        xhrNw : Api.Method;
        xhrXt : RecXtn;
        rtsXt : BlkXtn;
        recXt : RecXtn;
  BEGIN
   (*
    *  Reset the descriptor pool.
    *  Note that descriptors cannot persist between
    *  compilation unit, since the token sequence
    *  is reset in PEAPI.
    *)
    mathCls := NIL;
    envrCls := NIL;
    excpCls := NIL;
    rtTpHdl := NIL;
    loadTyp := NIL;
    FOR i := 0 TO Mu.rtsLen-1 DO rHelper[i] := NIL END;
   (*
    *  Now we need to create tgXtn fields
    *  for some of the system types.   All 
    *  others are only allocated on demand.
    *)
    corlib := os.peFl.MakeExternAssembly("mscorlib");
   (*
    *  Must put xtn markers on both the pointer AND the record
    *)
    NEW(recXt);
    CSt.ntvStr(Ty.Pointer).boundTp.tgXtn := recXt;      (* the record  *)
(*
 *  recXt.clsD := corlib.AddClass("System", "String");
 *)
(* -- start replacement -- *)
    recXt.clsD := getOrAddClass(corlib, "System", "String");
(* --- end replacement --- *)
    CSt.ntvStr.tgXtn := recXt.clsD;                     (* the pointer *)
   (*
    *  Must put xtn markers on both the pointer AND the record
    *)
    NEW(recXt);
    CSt.ntvObj(Ty.Pointer).boundTp.tgXtn := recXt;      (* the record  *)
(*
 *  recXt.clsD := corlib.AddClass("System", "Object");
 *)
(* -- start replacement -- *)
    recXt.clsD := getOrAddClass(corlib, "System", "Object");
(* --- end replacement --- *)
    CSt.ntvObj.tgXtn := recXt.clsD;                     (* the pointer *)
   (*
    *  CSt.ntvVal IS a record descriptor, not a pointer
    *)
    NEW(recXt);
    CSt.ntvVal.tgXtn := recXt;                          (* the record  *)
(*
 *  recXt.clsD := corlib.AddClass("System", "ValueType");
 *)
(* -- start replacement -- *)
    recXt.clsD := getOrAddClass(corlib, "System", "ValueType");
(* --- end replacement --- *)

    newObjt := getOrAddMethod(CSt.ntvObj.tgXtn(Api.ClassRef),ctorS,voidD,NIL);
    newObjt.AddCallConv(Api.CallConv.Instance);
   (*
    *  Create Api.AssemblyRef for "RTS"
    *  Create Api.ClassRef for "[RTS]RTS"
    *  Create Api.ClassRef for "[RTS]Cp_rts"
    *)
    IF CSt.rtsBlk.xName = NIL THEN Mu.MkBlkName(CSt.rtsBlk) END;
    os.rts   := os.peFl.MakeExternAssembly("RTS");
    NEW(rtsXt);
    rtsXt.asmD := os.rts;
    rtsXt.dscD := os.rts.AddClass("", "RTS");
    CSt.rtsBlk.tgXtn := rtsXt;
    os.cprts    := os.rts.AddClass("", "CP_rts");
   (*
    *  Create Api.AssemblyRef for "ProgArgs" (same as RTS)
    *  Create Api.ClassRef for "[RTS]ProgArgs"
    *)
    os.DoRtsMod(CSt.prgArg);
    os.progArgs := CSt.prgArg.tgXtn(BlkXtn).dscD(Api.ClassRef);
   (*
    *  Create Api.ClassRef for "[RTS]XHR"
    *  Create method "[RTS]XHR::.ctor()"
    *)
    xhrCl := os.rts.AddClass("", "XHR");
    xhrNw := xhrCl.AddMethod(ctorS, voidD, NIL);
    xhrNw.AddCallConv(Api.CallConv.Instance);
    xhrRc := CSt.rtsXHR.boundRecTp()(Ty.Record);
    NEW(xhrXt);
    xhrRc.tgXtn := xhrXt;
    xhrXt.clsD := xhrCl;
    xhrXt.newD := xhrNw;
  END RefRTS;

(* ============================================================ *)

  PROCEDURE (os : PeFile)StartNamespace*(nm : Lv.CharOpen);
  BEGIN
    os.nmSp := MKSTR(nm^);
  END StartNamespace;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkBodyClass*(mod : Id.BlkId);
   (*
    *   Instantiate a ClassDef object for the synthetic
    *   static class, and assign to the PeFile::clsS field.
    *   Of course, for the time being it is also the 
    *   "current class" held in the PeFile::clsD field.
    *)
    VAR namStr : RTS.NativeString;
        clsAtt : INTEGER;
        modXtn : BlkXtn;
  BEGIN
    defSrc := Api.SourceFile.GetSourceFile(
        MKSTR(CSt.srcNam), Sys.Guid.Empty, Sys.Guid.Empty, Sys.Guid.Empty);
    namStr  := MKSTR(mod.clsNm^);
    clsAtt  := toTypeAttr(Asm.modAttr);
    os.clsS := os.peFl.AddClass(clsAtt, os.nmSp, namStr);
    os.clsD := os.clsS; 
    NEW(modXtn);
    modXtn.asmD := NIL;
    modXtn.dscD := os.clsS;
    mod.tgXtn := modXtn;
  END MkBodyClass;

(* ============================================================ *)

  PROCEDURE (os : PeFile)ClassHead*(attSet : SET; 
                                    thisRc : Ty.Record;
                                    superT : Ty.Record);
    VAR clsAtt : INTEGER;
        clsDef : Api.ClassDef;
  BEGIN
    clsAtt := toTypeAttr(attSet);
    clsDef := os.cls(thisRc)(Api.ClassDef);
    clsDef.AddAttribute(clsAtt);
    os.clsD := clsDef;
  END ClassHead;

(* ============================================================ *)

  PROCEDURE (os : PeFile)ClassTail*();
  BEGIN
    os.clsD := NIL;
  END ClassTail;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkRecX*(t : Ty.Record; s : Sy.Scope);
   (* -------------------------------- *
    *  Create a ClassDef or a ClassRef for this type.
    *  The type attributes are set to a default value
    *  and are modified later for a ClassDef.
    * -------------------------------- *)
    VAR indx : INTEGER;
        valR : BOOLEAN;               (* is a value record  *)
        noNw : BOOLEAN;               (* no constructor...  *)
        base : Ty.Record;
        xAsm : Api.AssemblyRef;
        xCls : Api.ClassRef;
        cDef : Api.ClassDef;
        cRef : Api.ClassRef;
        nStr : RTS.NativeString;      (* record name string *)
        aStr : RTS.NativeString;      (* imported namespace *)
        recX : RecXtn;
   (* -------------------------------- *)
    PROCEDURE DoBoxDef(o : PeFile; t : Ty.Record);
      VAR nStr : RTS.NativeString;
          cDef : Api.ClassDef;
          cFld : Api.FieldDef;
          nMth : Api.MethodDef;
          tXtn : RecXtn;
    BEGIN
      nStr := boxedName(t);
      tXtn := t.tgXtn(RecXtn);
      cDef := o.peFl.AddClass(0, o.nmSp, nStr);
      cFld := cDef.AddField(vfldS, tXtn.clsD);
      nMth := cDef.AddMethod(ctAtt,ilAtt,ctorS,voidD,NIL);

      nMth.AddCallConv(Api.CallConv.Instance);
      cFld.AddFieldAttr(Api.FieldAttr.Public);

      tXtn.boxD := cDef;
      tXtn.newD := nMth;
      tXtn.vDlr := cFld;
    END DoBoxDef;
   (* -------------------------------- *)
    PROCEDURE DoBoxRef(o : PeFile; t : Ty.Record; c : Api.ClassRef);
      VAR cFld : Api.FieldRef;
          nMth : Api.MethodRef;
          tXtn : RecXtn;
    BEGIN
      tXtn := t.tgXtn(RecXtn);
      cFld := getOrAddField(c, vfldS, tXtn.clsD);
(*
 *    nMth := c.AddMethod(ctorS,voidD,NIL);
 *)
      nMth := getOrAddMethod(c, ctorS, voidD, NIL);
      nMth.AddCallConv(Api.CallConv.Instance);

      tXtn.boxD := c;
      tXtn.newD := nMth;
      tXtn.vDlr := cFld;
    END DoBoxRef;
   (* -------------------------------- *)
  BEGIN
    nStr := MKSTR(t.xName^);
    valR := Mu.isValRecord(t);
    NEW(recX);
    t.tgXtn := recX;
   (*
    *  No default no-arg constructor is defined if this
    *  is an abstract record, an interface, or extends a
    *  foreign record that does not export a no-arg ctor.
    *)
    noNw := t.isInterfaceType() OR (Sy.noNew IN t.xAttr);

    IF s.kind # Id.impId THEN (* this is a classDEF *)
      base := t.superType();  (* might return System.ValueType *)
      IF base = NIL THEN
        cDef := os.peFl.AddClass(0, os.nmSp, nStr);
      ELSIF valR THEN
        cDef := os.peFl.AddValueClass(0, os.nmSp, nStr);
      ELSE
        cDef := os.peFl.AddClass(0, os.nmSp, nStr, os.cls(base));
      END;
      recX.clsD := cDef; (* this field needed for MkFldName() *)
      IF valR THEN 
       (*
        *  Create the boxed version of this value record
        *  AND create a constructor for the boxed class
        *)
        DoBoxDef(os, t);
      ELSIF ~noNw THEN
       (*
        *  Create a constructor for this reference class.
        *)
        recX.newD := cDef.AddMethod(ctAtt, ilAtt, ctorS, voidD, NIL);
        recX.newD.AddCallConv(Api.CallConv.Instance);
      END;
      FOR indx := 0 TO t.fields.tide-1 DO
        Mu.MkFldName(t.fields.a[indx](Id.FldId), os);
      END;
    ELSE                      (* this is a classREF *)
      IF t.encCls # NIL THEN  (* ... a nested classREF *)
        base := t.encCls(Ty.Record);
        xCls := os.cls(base)(Api.ClassRef);
        cRef := xCls.AddNestedClass(nStr);
        recX.clsD := cRef;
      ELSE                    (* ... a normal classREF *)
        xAsm := os.asm(s(Id.BlkId));
        aStr := MKSTR(s(Id.BlkId).xName^);
        IF valR THEN
          cRef := getOrAddValueClass(xAsm, aStr, nStr);
        ELSE
          cRef := getOrAddClass(xAsm, aStr, nStr);
        END;
        recX.clsD := cRef;
        IF valR & ~(Sy.isFn IN t.xAttr) THEN
          DoBoxRef(os, t, xAsm.AddClass(aStr, boxedName(t)));
        END;
      END;

      IF ~noNw & ~valR THEN
        recX.newD := getOrAddMethod(cRef, ctorS, voidD, NIL);
        recX.newD.AddCallConv(Api.CallConv.Instance);
      END;
    END;
  END MkRecX;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkVecX*(t : Sy.Type; m : Id.BlkId);
    VAR xAsm : Api.AssemblyRef;
        recX : RecXtn;
        nStr : RTS.NativeString;      (* record name string *)
        aStr : RTS.NativeString;      (* imported namespace *)
        cRef : Api.ClassRef;
  BEGIN
    NEW(recX);
    t.tgXtn := recX;

    IF m.tgXtn = NIL THEN os.DoRtsMod(m) END;
    IF t.xName = NIL THEN Mu.MkTypeName(t, os) END;

    aStr := MKSTR(m.xName^);
    nStr := MKSTR(t.xName^);

    xAsm := os.asm(m);
    cRef := xAsm.AddClass(aStr, nStr);
    recX.clsD := cRef;
    recX.newD := cRef.AddMethod(ctorS, voidD, NIL);
    recX.newD.AddCallConv(Api.CallConv.Instance);
  END MkVecX;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkDelX(t : Ty.Procedure;
                                s : Sy.Scope),NEW;
   (* -------------------------------- *)
    CONST dAtt = Asm.att_public + Asm.att_sealed;
    VAR   xtn : DelXtn;             (* The created descriptor   *)
          str : RTS.NativeString;   (* The proc-type nameString *)
          att : Api.TypeAttr;       (* public,sealed (for Def)  *)
          asN : RTS.NativeString;   (* Assembly name (for Ref)  *)
          asR : Api.AssemblyRef;    (* Assembly ref  (for Ref)  *)
          rtT : Sy.Type;            (* AST return type of proc  *)
          rtD : Api.Type;           (* Api return type of del.  *)
          clD : Api.ClassDef;
          clR : Api.ClassRef;
          mtD : Api.MethodDef;
   (* -------------------------------- *)
    PROCEDURE t2() : POINTER TO ARRAY OF Api.Type;
      VAR a : POINTER TO ARRAY OF Api.Type;
    BEGIN 
      NEW(a,2); a[0] := objtD; a[1] := nIntD; RETURN a; 
    END t2;
   (* -------------------------------- *)
    PROCEDURE p2() : POINTER TO ARRAY OF Api.Param;
      VAR a : POINTER TO ARRAY OF Api.Param;
    BEGIN
      NEW(a,2);
      a[0] := Api.Param.init(0, "obj", objtD); 
      a[1] := Api.Param.init(0, "mth", nIntD); 
      RETURN a;
    END p2;
   (* -------------------------------- *)
    PROCEDURE tArr(t: Ty.Procedure; o: PeFile) : POINTER TO ARRAY OF Api.Type;
      VAR a : POINTER TO ARRAY OF Api.Type;
          i : INTEGER;
          p : Id.ParId;
          d : Api.Type;
    BEGIN
      NEW(a, t.formals.tide);
      FOR i := 0 TO t.formals.tide-1 DO
        p := t.formals.a[i];
        d := o.typ(p.type);
        IF Mu.takeAdrs(p) THEN 
          p.boxOrd := p.parMod;
          d := Api.ManagedPointer.init(d);
        END; 
        a[i] := d; 
      END;
      RETURN a;
    END tArr;
   (* -------------------------------- *)
    PROCEDURE pArr(t: Ty.Procedure; o: PeFile) : POINTER TO ARRAY OF Api.Param;
      VAR a : POINTER TO ARRAY OF Api.Param;
          i : INTEGER;
          p : Id.ParId;
          d : Api.Type;
    BEGIN
      NEW(a, t.formals.tide);
      FOR i := 0 TO t.formals.tide-1 DO
        p := t.formals.a[i];
        d := o.typ(p.type);
        IF Mu.takeAdrs(p) THEN 
          p.boxOrd := p.parMod;
          d := Api.ManagedPointer.init(d);
        END; 
        a[i] := Api.Param.init(0, nms(p), d); 
      END;
      RETURN a;
    END pArr;
   (* -------------------------------- *)
  BEGIN
    IF t.tgXtn # NIL THEN RETURN END;
    NEW(xtn);
    str := MKSTR(Sy.getName.ChPtr(t.idnt)^);
    rtT := t.retType;
    IF rtT = NIL THEN rtD := voidD ELSE rtD := os.typ(rtT) END;

    IF s.kind # Id.impId THEN (* this is a classDEF *)
      att := toTypeAttr(dAtt);
      clD := os.peFl.AddClass(att, os.nmSp, str, os.mcd());
      mtD := clD.AddMethod(ctorS, voidD, p2());
      mtD.AddMethAttribute(ctAtt);
      mtD.AddImplAttribute(rmAtt);
      xtn.newD := mtD;
      mtD := clD.AddMethod(invkS, rtD, pArr(t, os));
      mtD.AddMethAttribute(Api.MethAttr.Public);
      mtD.AddImplAttribute(rmAtt);
      xtn.invD := mtD;
      xtn.clsD := clD;
    ELSE                      (* this is a classREF *)
      asR := os.asm(s(Id.BlkId));
      asN := MKSTR(s(Id.BlkId).xName^);
      clR := getOrAddClass(asR, asN, str);
      xtn.newD := clR.AddMethod(ctorS, voidD, t2());
      xtn.invD := clR.AddMethod(invkS, rtD, tArr(t, os));
      xtn.clsD := clR;
    END;
    xtn.newD.AddCallConv(Api.CallConv.Instance);
    xtn.invD.AddCallConv(Api.CallConv.Instance);
    t.tgXtn := xtn;
    IF (t.idnt # NIL) & (t.idnt.tgXtn = NIL) THEN t.idnt.tgXtn := xtn END;
  END MkDelX;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkPtrX*(t : Ty.Pointer);
    VAR bTyp : Sy.Type;
        recX : RecXtn;
  BEGIN
    bTyp := t.boundTp;
    IF bTyp.tgXtn = NIL THEN Mu.MkTypeName(bTyp, os) END;
    WITH bTyp : Ty.Record DO
        recX := bTyp.tgXtn(RecXtn);
        IF recX.boxD # NIL THEN t.tgXtn := recX.boxD;
        ELSE t.tgXtn := recX.clsD;
        END;
    | bTyp : Ty.Array DO
        t.tgXtn := bTyp.tgXtn;
    END;
  END MkPtrX;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkArrX*(t : Ty.Array);
  BEGIN
    t.tgXtn := Api.ZeroBasedArray.init(os.typ(t.elemTp));
  END MkArrX;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkBasX*(t : Ty.Base);
  BEGIN
    CASE t.tpOrd OF
    | Ty.uBytN            : t.tgXtn := Api.PrimitiveType.UInt8;
    | Ty.byteN            : t.tgXtn := Api.PrimitiveType.Int8;
    | Ty.sIntN            : t.tgXtn := Api.PrimitiveType.Int16;
    | Ty.intN,Ty.setN     : t.tgXtn := Api.PrimitiveType.Int32;
    | Ty.lIntN            : t.tgXtn := Api.PrimitiveType.Int64;
    | Ty.boolN            : t.tgXtn := Api.PrimitiveType.Boolean;
    | Ty.charN,Ty.sChrN   : t.tgXtn := Api.PrimitiveType.Char;
    | Ty.realN            : t.tgXtn := Api.PrimitiveType.Float64;
    | Ty.sReaN            : t.tgXtn := Api.PrimitiveType.Float32;
    | Ty.anyRec,Ty.anyPtr : t.tgXtn := Api.PrimitiveType.Object;
    END;
  END MkBasX;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkEnuX*(t : Ty.Enum; s : Sy.Scope);
    VAR scNs : RTS.NativeString;
        enNm : RTS.NativeString;
  BEGIN
    ASSERT(s.kind = Id.impId);
    scNs := MKSTR(s(Id.BlkId).xName^);
    enNm := MKSTR(Sy.getName.ChPtr(t.idnt)^);
    t.tgXtn := getOrAddValueClass(os.asm(s(Id.BlkId)), scNs, enNm);
  END MkEnuX;

(* ============================================================ *)
(*
  PROCEDURE (os : PeFile)MkTyXtn*(t : Sy.Type; s : Sy.Scope);
  BEGIN
    IF t.tgXtn # NIL THEN RETURN END;
    WITH t : Ty.Record    DO os.MkRecX(t, s);
    |    t : Ty.Enum      DO os.MkEnuX(t, s);
    |    t : Ty.Procedure DO os.MkDelX(t, s);
    |    t : Ty.Base      DO os.MkBasX(t);
    |    t : Ty.Pointer   DO os.MkPtrX(t);
    |    t : Ty.Array     DO os.MkArrX(t);
    END;
  END MkTyXtn;
 *)
(* ============================================================ *)

  PROCEDURE MkMthDef(os  : PeFile;
                     xhr : BOOLEAN;
                     pTp : Ty.Procedure;
                     cls : Api.ClassDef;
                     str : RTS.NativeString)  : Api.MethodDef;
    VAR par : Id.ParId;
        prd : Api.Type;
        prs : POINTER TO ARRAY OF Api.Param;
        rtT : Sy.Type;
        rtd : Api.Type;
        pId : Sy.Idnt;

        idx : INTEGER;       (* index into formal array *)
        prX : INTEGER;       (* index into param. array *)
        prO : INTEGER;       (* runtime ordinal of arg. *)
        num : INTEGER;       (* length of formal array  *)
        len : INTEGER;       (* length of param array   *)
  BEGIN
    pId := pTp.idnt;
    IF (pId # NIL) & (pId IS Id.MthId) & (Id.covar IN pId(Id.MthId).mthAtt) THEN 
      rtT := pId(Id.MthId).retTypBound();
    ELSE
      rtT := pTp.retType;
    END;
    num := pTp.formals.tide;
    IF xhr THEN len := num + 1 ELSE len := num END;
    NEW(prs, len);
    IF rtT = NIL THEN rtd := voidD ELSE rtd := os.typ(rtT) END;

    prO := pTp.argN; (* count from 1 if xhr OR has this *)
    IF xhr THEN
      prs[0] := Api.Param.init(0, "", xhrCl); prX := 1;
    ELSE
      prX := 0;
    END;
    FOR idx := 0 TO num-1 DO
      par := pTp.formals.a[idx];
      par.varOrd := prO; 
      prd := os.typ(par.type);
      IF Mu.takeAdrs(par) THEN 
        par.boxOrd := par.parMod;
        prd := Api.ManagedPointer.init(prd);
        IF Id.uplevA IN par.locAtt THEN 
          par.boxOrd := Sy.val;
          ASSERT(Id.cpVarP IN par.locAtt);
        END;
      END; (* just mark *)
      prs[prX] := Api.Param.init(par.boxOrd, nms(par), prd); 
      INC(prX); INC(prO);
    END;
   (*
    *  Add attributes, Impl, Meth, CallConv in MethodDecl()
    *)
    RETURN cls.AddMethod(str, rtd, prs);
  END MkMthDef;

(* ============================================================ *)

  PROCEDURE MkMthRef(os  : PeFile;
                     pTp : Ty.Procedure;
                     cls : Api.ClassRef;
                     str : RTS.NativeString) : Api.MethodRef;
    VAR par : Id.ParId;
        tpD : Api.Type;
        prs : POINTER TO ARRAY OF Api.Type;
        rtT : Sy.Type;
        rtd : Api.Type;
        pId : Sy.Idnt;

        idx : INTEGER;       (* index into formal array *)
        prO : INTEGER;       (* runtime ordinal of arg. *)
        num : INTEGER;       (* length of formal array  *)
  BEGIN
    pId := pTp.idnt;
    IF (pId # NIL) & (pId IS Id.MthId) & (Id.covar IN pId(Id.MthId).mthAtt) THEN 
      rtT := pId(Id.MthId).retTypBound();
    ELSE
      rtT := pTp.retType;
    END;
    num := pTp.formals.tide;
    NEW(prs, num);
    IF rtT = NIL THEN rtd := voidD ELSE rtd := os.typ(rtT) END;

    prO := pTp.argN;
    FOR idx := 0 TO num-1 DO
      par := pTp.formals.a[idx];
      tpD := os.typ(par.type);
      par.varOrd := prO; (* if hasThis, then is (idx+1) *)
      IF Mu.takeAdrs(par) THEN 
        par.boxOrd := par.parMod;
        tpD := Api.ManagedPointer.init(tpD);
      END; (* just mark *)
      prs[idx] := tpD; INC(prO);
    END;
    RETURN getOrAddMethod(cls, str, rtd, prs);
  END MkMthRef;

(* ============================================================ *)

  PROCEDURE (os : PeFile)NumberParams*(pId : Id.Procs; 
                                       pTp : Ty.Procedure);
   (*
    *   (1) Generate signature information for this procedure
    *   (2) Generate the target extension Method(Def | Ref)
    *)
    VAR class : Api.Class;
        methD : Api.Method;
        namSt : RTS.NativeString;
        xhrMk : BOOLEAN;
        pLeng : INTEGER;
   (* ----------------- *)
    PROCEDURE classOf(os : PeFile; id : Id.Procs) : Api.Class;
      VAR scp : Sy.Scope;
    BEGIN
      scp := id.dfScp;
     (*
      *  Check for methods bound to explicit classes
      *)
      IF id.bndType # NIL THEN RETURN os.cls(id.bndType(Ty.Record)) END;
     (*
      *  Or associate static methods with the dummy class
      *)
      WITH scp : Id.BlkId DO
        RETURN os.dsc(scp);
      | scp : Id.Procs DO (* Nested procs take class from scope *)
        RETURN classOf(os, scp);
      END;
    END classOf;
   (* ----------------- *)
  BEGIN
    IF pId = NIL THEN 
      os.MkDelX(pTp, pTp.idnt.dfScp); RETURN;       (* PREMATURE RETURN HERE *)
    END;
    IF pId.tgXtn # NIL THEN RETURN END;             (* PREMATURE RETURN HERE *)

    class := classOf(os, pId);    
    namSt := MKSTR(pId.prcNm^);
    xhrMk := pId.lxDepth > 0;
   (*
    *  The incoming argN counts one for a receiver,
    *  and also counts one for nested procedures.
    *)
    IF pId IS Id.MthId THEN pLeng := pTp.argN-1 ELSE pLeng := pTp.argN END;
   (*
    *  Now create either a MethodDef or MethodRef
    *)
    WITH class : Api.ClassDef DO
        methD :=  MkMthDef(os, xhrMk, pTp, class, namSt);
    | class : Api.ClassRef DO
        methD :=  MkMthRef(os, pTp, class, namSt);
    END;
    INC(pTp.argN, pTp.formals.tide);
    IF pTp.retType # NIL THEN pTp.retN := 1 END;
    IF (pId.kind = Id.ctorP) OR
       (pId IS Id.MthId) THEN methD.AddCallConv(Api.CallConv.Instance) END;

    pId.tgXtn := methD;
    pTp.xName := cln2;  (* an arbitrary "done" marker *)

    IF (pId.kind = Id.fwdPrc) OR (pId.kind = Id.fwdMth) THEN
      pId.resolve.tgXtn := methD;
    END;
  END NumberParams;
 
(* ============================================================ *)

  PROCEDURE (os : PeFile)SwitchHead*(num : INTEGER);
  BEGIN
    switch.next := 0;
    NEW(switch.list, num);
  END SwitchHead;

  PROCEDURE (os : PeFile)SwitchTail*();
  BEGIN
    os.pePI.code.Switch(switch.list);
    switch.list := NIL;
  END SwitchTail;

  PROCEDURE (os : PeFile)LstLab*(l : Mu.Label);
  BEGIN
    WITH l : PeLab DO
      switch.list[switch.next] := l.labl; 
      INC(switch.next);
    END;
  END LstLab;

(* ============================================================ *)

  PROCEDURE (os : PeFile)mth(pId : Id.Procs) : Api.Method,NEW;
  BEGIN
    ASSERT(pId.tgXtn # NIL);
    RETURN pId.tgXtn(Api.Method);
  END mth;

(* -------------------------------- *)

  PROCEDURE (os : PeFile)fld(fId : Id.AbVar)  : Api.Field,NEW;
    VAR cDf : Api.Class;
        fNm : Lv.CharOpen;
        obj : ANYPTR;
   (* ---------------- *)
    PROCEDURE AddField(os : PeFile;
                       cl : Api.Class; 
                       fn : Lv.CharOpen; 
                       ty : Sy.Type) : Api.Field;
      VAR fs : RTS.NativeString;
    BEGIN
      fs := MKSTR(fn^);
      WITH cl : Api.ClassDef DO
        RETURN cl.AddField(fs, os.typ(ty));
      |    cl : Api.ClassRef DO
        RETURN getOrAddField(cl, fs, os.typ(ty));
      END;
    END AddField;
   (* ---------------- *)
  BEGIN
    IF fId.tgXtn = NIL THEN
      WITH fId : Id.VarId DO
          IF fId.varNm = NIL THEN Mu.MkVarName(fId,os) END;
          IF fId.recTyp = NIL THEN (* module variable *)
            cDf := os.dsc(fId.dfScp(Id.BlkId));
          ELSE                     (* static field    *)
            cDf := os.cls(fId.recTyp(Ty.Record));
          END;
          fNm := fId.varNm;
      | fId : Id.FldId DO
          IF fId.fldNm = NIL THEN Mu.MkFldName(fId,os) END;
          cDf := os.cls(fId.recTyp(Ty.Record));
          fNm := fId.fldNm;
      END;
      fId.tgXtn := AddField(os, cDf, fNm, fId.type);
    END;
    obj := fId.tgXtn;
    WITH obj : Api.Field DO RETURN obj;
    |    obj : EvtXtn    DO RETURN obj.fldD;
    END;
  END fld;

(* -------------------------------- *)

  PROCEDURE (os : PeFile)add(fId : Id.AbVar) : Api.Method,NEW;
  BEGIN (* returns the descriptor of add_<fieldname> *)
    IF (fId.tgXtn = NIL) OR ~(fId.tgXtn IS EvtXtn) THEN MkAddRem(os, fId) END;
    RETURN fId.tgXtn(EvtXtn).addD;
  END add;

(* -------------------------------- *)

  PROCEDURE (os : PeFile)rem(fId : Id.AbVar) : Api.Method,NEW;
  BEGIN (* returns the descriptor of remove_<fieldname> *)
    IF (fId.tgXtn = NIL) OR ~(fId.tgXtn IS EvtXtn) THEN MkAddRem(os, fId) END;
    RETURN fId.tgXtn(EvtXtn).remD;
  END rem;

(* -------------------------------- *)

  PROCEDURE (os : PeFile)asm(bId : Id.BlkId) : Api.AssemblyRef,NEW;
  BEGIN (* returns the assembly reference of this module *)
    IF bId.tgXtn = NIL THEN os.DoExtern(bId) END;
    RETURN bId.tgXtn(BlkXtn).asmD;
  END asm;

(* -------------------------------- *)

  PROCEDURE (os : PeFile)dsc(bId : Id.BlkId) : Api.Class,NEW;
  BEGIN (* returns descriptor of dummy static class of this module *)
    IF bId.tgXtn = NIL THEN os.DoExtern(bId) END;
    RETURN bId.tgXtn(BlkXtn).dscD;
  END dsc;

(* -------------------------------- *)

  PROCEDURE (os : PeFile)cls(rTy : Ty.Record) : Api.Class,NEW;
  BEGIN (* returns descriptor for this class *)
    IF rTy.tgXtn = NIL THEN Mu.MkRecName(rTy, os) END;
    RETURN rTy.tgXtn(RecXtn).clsD;
  END cls;

(* -------------------------------- *)
(*
 *  PROCEDURE (os : PeFile)box(rTy : Ty.Record) : Api.Class,NEW;
 *  BEGIN
 *    IF rTy.tgXtn = NIL THEN Mu.MkRecName(rTy, os) END;
 *    RETURN rTy.tgXtn(RecXtn).boxD;
 *  END box;
 *)
(* -------------------------------- *)

  PROCEDURE (os : PeFile)new(rTy : Ty.Record) : Api.Method,NEW;
  BEGIN (* returns the ctor for this reference class *)
    IF rTy.tgXtn = NIL THEN Mu.MkRecName(rTy, os) END;
    RETURN rTy.tgXtn(RecXtn).newD;
  END new;

(* -------------------------------- *)

  PROCEDURE (os : PeFile)dxt(pTy : Ty.Procedure) : DelXtn,NEW;
  BEGIN (* returns the DelXtn extension for this delegate type *)
    IF pTy.tgXtn = NIL THEN os.MkDelX(pTy, pTy.idnt.dfScp) END;
    RETURN pTy.tgXtn(DelXtn);
  END dxt;

(* -------------------------------- *)

  PROCEDURE mkCopyDef(cDf : Api.ClassDef; val : BOOLEAN) : Api.Method;
    VAR pra : POINTER TO ARRAY OF Api.Param;
        prd : Api.Type;
  BEGIN
    NEW(pra, 1);
    prd := cDf;
    IF val THEN prd := Api.ManagedPointer.init(prd) END;
    pra[0] := Api.Param.init(0, "src", prd);
    RETURN cDf.AddMethod(copyS, voidD, pra);
  END mkCopyDef;

(* -------------------------------- *)

  PROCEDURE (os : PeFile)cpy(rTy : Ty.Record) : Api.Method,NEW;
    VAR tXtn : RecXtn;
        tCls : Api.Class;
        mthX : Api.Method;
        typA : POINTER TO ARRAY OF Api.Type;
        valR : BOOLEAN;
  BEGIN
    tXtn := rTy.tgXtn(RecXtn);
    tCls := tXtn.clsD;
    IF tXtn.cpyD = NIL THEN
      valR := Mu.isValRecord(rTy);
      WITH tCls : Api.ClassDef DO
          mthX := mkCopyDef(tCls, valR);
      | tCls : Api.ClassRef DO
          NEW(typA, 1);
          IF valR THEN 
            typA[0] := Api.ManagedPointer.init(tCls);
          ELSE
            typA[0] := tCls;
          END;
          mthX := tCls.AddMethod(copyS, voidD, typA);
          mthX.AddCallConv(Api.CallConv.Instance);
      END;
      tXtn.cpyD := mthX;
    ELSE
      mthX := tXtn.cpyD;
    END;
    RETURN mthX;
  END cpy;

(* -------------------------------- *)

  PROCEDURE (os : PeFile)vDl(rTy : Ty.Record) : Api.Field,NEW;
  BEGIN (* returns descriptor of field "v$" for this boxed value type *)
    IF rTy.tgXtn = NIL THEN Mu.MkRecName(rTy, os) END;
    RETURN rTy.tgXtn(RecXtn).vDlr;
  END vDl;

(* -------------------------------- *)

  PROCEDURE (os : PeFile)RescueOpaque(tTy : Sy.Type),NEW;
    VAR blk : Id.BlkId;
        ext : BlkXtn;
  BEGIN
    blk := tTy.idnt.dfScp(Id.BlkId);
    os.DoExtern(blk);
    ext := blk.tgXtn(BlkXtn);
    (* Set tgXtn to a ClassRef *)
    tTy.tgXtn := getOrAddClass(ext.asmD, MKSTR(blk.xName^), MKSTR(Sy.getName.ChPtr(tTy.idnt)^));
  RESCUE (any)
    (* Just leave tgXtn = NIL *)
  END RescueOpaque;

(* -------------------------------- *)

  PROCEDURE (os : PeFile)typ(tTy : Sy.Type) : Api.Type,NEW;
    VAR xtn : ANYPTR;
  BEGIN (* returns Api.Type descriptor for this type *)
    IF tTy.tgXtn = NIL THEN Mu.MkTypeName(tTy, os) END;
    IF (tTy IS Ty.Opaque) & (tTy.tgXtn = NIL) THEN os.RescueOpaque(tTy(Ty.Opaque)) END;
    xtn := tTy.tgXtn;
    IF xtn = NIL THEN
      IF tTy.xName # NIL THEN tTy.TypeErrStr(236, tTy.xName);
      ELSE tTy.TypeError(236);
      END;
      RTS.Throw("Opaque Type Error");
    END;
    WITH xtn : Api.Type DO
        RETURN xtn;
    | xtn : RecXtn DO
        RETURN xtn.clsD;
    | xtn : DelXtn DO
        RETURN xtn.clsD;
    END;
  END typ;

(* ============================================================ *)

  PROCEDURE (os : PeFile)mcd() : Api.ClassRef,NEW;
  BEGIN (* returns System.MulticastDelegate *)
    IF multiCD = NIL THEN 
      multiCD := getOrAddClass(corlib, "System", "MulticastDelegate");
    END;
    RETURN multiCD;
  END mcd;

(* ============================================================ *)

  PROCEDURE (os : PeFile)del() : Api.ClassRef,NEW;
  BEGIN (* returns System.Delegate *)
    IF delegat = NIL THEN 
      delegat := getOrAddClass(corlib, "System", "Delegate");
    END;
    RETURN delegat;
  END del;

(* ============================================================ *)

  PROCEDURE (os : PeFile)rmv() : Api.MethodRef,NEW;
    VAR prs : POINTER TO ARRAY OF Api.Type;
        dlg : Api.ClassRef;
  BEGIN (* returns System.Delegate::Remove *)
    IF remove = NIL THEN 
      dlg := os.del();
      NEW(prs, 2);
      prs[0] := dlg; 
      prs[1] := dlg;
      remove := dlg.AddMethod("Remove", dlg, prs);
    END;
    RETURN remove;
  END rmv;

(* ============================================================ *)

  PROCEDURE (os : PeFile)cmb() : Api.MethodRef,NEW;
    VAR prs : POINTER TO ARRAY OF Api.Type;
        dlg : Api.ClassRef;
  BEGIN (* returns System.Delegate::Combine *)
    IF combine = NIL THEN 
      dlg := os.del();
      NEW(prs, 2);
      prs[0] := dlg; 
      prs[1] := dlg;
      combine := dlg.AddMethod("Combine", dlg, prs);
    END;
    RETURN combine;
  END cmb;

(* ============================================================ *)
(* ============================================================ *)
BEGIN
  evtAdd   := Lv.strToCharOpen("add_"); 
  evtRem   := Lv.strToCharOpen("remove_"); 
  cln2     := Lv.strToCharOpen("::"); 
  boxedObj := Lv.strToCharOpen("Boxed_"); 

  vfldS  := MKSTR("v$");
  ctorS  := MKSTR(".ctor");
  invkS  := MKSTR("Invoke");
  copyS  := MKSTR("__copy__");
END PeUtil.
(* ============================================================ *)
(* ============================================================ *)

