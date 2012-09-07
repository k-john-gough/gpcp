(* ============================================================ *)
(*  JsmnUtil is the module which writes jasmin file structures  *)
(*  Copyright (c) John Gough 1999, 2000.			*)
(* ============================================================ *)

MODULE JsmnUtil;

  IMPORT 
	GPCPcopyright,
	RTS, ASCII,
	Console,
	GPText,
	LitValue,
        FileNames,
	GPTextFiles,
	CompState,
        J := JavaUtil,
	D := Symbols,
	G := Builtin,
	Id := IdDesc,
	Ty := TypeDesc,
	Jvm := JVMcodes;

(* ============================================================ *)

   CONST
	classPrefix = "CP";
        pubStat     = Jvm.att_public + Jvm.att_static;
        modAttrib   = Jvm.att_public + Jvm.att_final;

   CONST
	(* various Java-specific runtime name strings *)
	initStr     = "<init>";
	initSuffix* = "/<init>()V";
	object*     = "java/lang/Object";
	objectInit* = "java/lang/Object/<init>()V";
	mainStr*    = "main([Ljava/lang/String;)V";
	jlExcept*   = "java/lang/Exception";
(*
 *	jlError*    = "java/lang/Error";
 *)
	jlError*    = jlExcept;
	mkExcept*   = "java/lang/Exception/<init>(Ljava/lang/String;)V";
(*
 *	mkError*    = "java/lang/Error/<init>(Ljava/lang/String;)V";
 *)
	mkError*    = mkExcept;
	putArgStr*  = "CP/CPmain/CPmain/PutArgs([Ljava/lang/String;)V";

(* ============================================================ *)
(* ============================================================ *)

  TYPE ProcInfo*  = POINTER TO RECORD
		      prId- : D.Scope;  (* mth., prc. or mod.	*)
		      lMax  : INTEGER;	(* max locals for proc  *)
		      lNum  : INTEGER;	(* current locals proc  *)
		      dMax  : INTEGER;	(* max depth for proc.  *)
		      dNum  : INTEGER;  (* current depth proc.  *)
                      attr  : SET;      (* access attributes    *)
		      exLb  : J.Label;
		      hnLb  : J.Label;
		    END;

(* ============================================================ *)

  TYPE JsmnFile*  = POINTER TO RECORD (J.JavaFile)
                      file* : GPTextFiles.FILE;
		      proc* : ProcInfo;
		      nxtLb : INTEGER;
                    END;

(* ============================================================ *)

  TYPE  TypeNameString = ARRAY 12 OF CHAR;
        ProcNameString = ARRAY 90 OF CHAR;

(* ============================================================ *)

  VAR   typeName  : ARRAY 15 OF TypeNameString;	(* base type names *)
	typeChar  : ARRAY 15 OF CHAR;		(* base type chars *)
        rtsProcs  : ARRAY 24 OF ProcNameString; 

(* ============================================================ *)
(*			Constructor Method			*)
(* ============================================================ *)

  PROCEDURE newJsmnFile*(fileName : ARRAY OF CHAR) : JsmnFile;
    VAR f : JsmnFile;
  BEGIN
    NEW(f);
    f.file := GPTextFiles.createFile(fileName);
    IF f.file = NIL THEN RETURN NIL; END;
    RETURN f;
  END newJsmnFile;

(* ============================================================ *)

  PROCEDURE^ (os : JsmnFile)Directive(dir : INTEGER),NEW;
  PROCEDURE^ (os : JsmnFile)DirectiveS(dir : INTEGER;
				   IN str : ARRAY OF CHAR),NEW;
  PROCEDURE^ (os : JsmnFile)DirectiveIS(dir : INTEGER; att : SET;
				     IN str : ARRAY OF CHAR),NEW;
  PROCEDURE^ (os : JsmnFile)DirectiveISS(dir : INTEGER; att : SET;
				      IN s1  : ARRAY OF CHAR;
				      IN s2  : ARRAY OF CHAR),NEW;
  PROCEDURE^ (os : JsmnFile)Call2*(code : INTEGER; 
				IN st1 : ARRAY OF CHAR;
				IN st2 : ARRAY OF CHAR;
				argL,retL : INTEGER),NEW;

(* ============================================================ *)
(* ============================================================ *)
(*			ProcInfo Methods			*)
(* ============================================================ *)

  PROCEDURE newProcInfo*(proc : D.Scope) : ProcInfo;
    VAR p : ProcInfo;
  BEGIN
    NEW(p);
    p.prId := proc;
    WITH proc : Id.Procs DO
      p.lNum := proc.rtsFram;
      p.lMax := MAX(proc.rtsFram, 1);
    ELSE	(* Id.BlkId *)
      p.lNum := 0;
      p.lMax := 1;
    END;
    p.dNum := 0;
    p.dMax := 0;
    p.attr := {};
    RETURN p;
  END newProcInfo;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : JsmnFile)StartProc* (proc : Id.Procs);
  VAR
    attr : SET;
    method : Id.MthId;
    procName : FileNames.NameString;
  BEGIN
    os.proc := newProcInfo(proc);
    os.Comment("PROCEDURE " + D.getName.ChPtr(proc)^);
   (*
    *  Compute the method attributes
    *)
    IF proc.kind = Id.conMth THEN
      method := proc(Id.MthId);
      attr := {};
      IF method.mthAtt * Id.mask = {} THEN attr := Jvm.att_final END;
      IF method.mthAtt * Id.mask = Id.isAbs THEN 
        attr := attr + Jvm.att_abstract; 
      END;
      IF Id.widen IN method.mthAtt THEN attr := attr + Jvm.att_public END; 
    ELSE
      attr := Jvm.att_static;
    END;
(*
 *  The following code fails for "implement-only" methods
 *  since the JVM places the "override method" in a different 
 *  slot! We must thus live with the insecurity of public mode.
 *
 *  IF proc.vMod = D.pubMode THEN	(* explicitly public *)
 *)
    IF (proc.vMod = D.pubMode) OR	(* explicitly public *)
       (proc.vMod = D.rdoMode) THEN     (* "implement only"  *)
      attr := attr + Jvm.att_public;
    ELSIF proc.dfScp IS Id.PrcId THEN	(* nested procedure  *)
      attr := attr + Jvm.att_private;
    END;
    FileNames.StripUpToLast("/", proc.prcNm, procName);
    os.DirectiveISS(Jvm.dot_method, attr, procName$, proc.type.xName);
    os.proc.attr := attr
  END StartProc;

(* ------------------------------------------------------------ *)

  PROCEDURE^ (os : JsmnFile)Locals(),NEW;
  PROCEDURE^ (os : JsmnFile)Stack(),NEW;
  PROCEDURE^ (os : JsmnFile)Blank(),NEW;

  PROCEDURE (os : JsmnFile)EndProc*();
  BEGIN
    IF (os.proc.attr * Jvm.att_abstract # {}) THEN
      os.Comment("Abstract method");
    ELSE
      os.Locals();
      os.Stack();
    END;
    os.Directive(Jvm.dot_end);
    os.Blank();
  END EndProc;

  PROCEDURE (os : JsmnFile)isAbstract*() : BOOLEAN;
  BEGIN
    RETURN (os.proc.attr * Jvm.att_abstract # {});
  END isAbstract;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : JsmnFile)getScope*() : D.Scope;
  BEGIN
    RETURN os.proc.prId; 
  END getScope;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : JsmnFile)newLocal*() : INTEGER;
    VAR ord : INTEGER;
        info : ProcInfo;
  BEGIN
    info := os.proc; 
    ord := info.lNum;
    INC(info.lNum);
    IF info.lNum > info.lMax THEN info.lMax := info.lNum END;
    RETURN ord;
  END newLocal;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : JsmnFile)ReleaseLocal*(i : INTEGER);
  BEGIN
   (*
    *  If you try to release not in LIFO order, the 
    *  location will not be made free again. This is safe!
    *)
    IF i+1 = os.proc.lNum THEN DEC(os.proc.lNum) END;
  END ReleaseLocal;

(* ------------------------------------------------------------ *)

  PROCEDURE (info : ProcInfo)numLocals*() : INTEGER,NEW;
  BEGIN
    IF info.lNum = 0 THEN RETURN 1 ELSE RETURN info.lNum END;
  END numLocals;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : JsmnFile)markTop*() : INTEGER;
  BEGIN
    RETURN os.proc.lNum;
  END markTop;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : JsmnFile)ReleaseAll*(m : INTEGER);
  BEGIN
    os.proc.lNum := m;
  END ReleaseAll;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : JsmnFile)getDepth*() : INTEGER;
  BEGIN RETURN os.proc.dNum END getDepth;

  (* ------------------------------------------ *)

  PROCEDURE (os : JsmnFile)setDepth*(i : INTEGER);
  BEGIN os.proc.dNum := i END setDepth;

(* ============================================================ *)
(*			Init Methods				*)
(* ============================================================ *)

  PROCEDURE (os : JsmnFile) ClinitHead*();
  BEGIN
    os.proc := newProcInfo(NIL);
    os.Comment("Class initializer");
    os.DirectiveIS(Jvm.dot_method, pubStat, "<clinit>()V");
  END ClinitHead;

(* ============================================================ *)

  PROCEDURE (os: JsmnFile)VoidTail*();
  BEGIN
    os.Code(Jvm.opc_return);
    os.Locals();
    os.Stack();
    os.Directive(Jvm.dot_end);
    os.Blank();
  END VoidTail;

(* ============================================================ *)

  PROCEDURE^ (os : JsmnFile)CallS*(code : INTEGER; IN str : ARRAY OF CHAR;
				   argL,retL : INTEGER),NEW;

  PROCEDURE (os : JsmnFile)MainHead*();
  BEGIN
    os.proc := newProcInfo(NIL);
    os.Comment("Main entry point");
    os.DirectiveIS(Jvm.dot_method, pubStat, mainStr);
   (*
    *  Save the command-line arguments to the RTS.
    *)
    os.Code(Jvm.opc_aload_0);
    os.CallS(Jvm.opc_invokestatic, putArgStr, 1, 0);
  END MainHead;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)ModNoArgInit*();
  BEGIN
    os.Blank();
    os.proc := newProcInfo(NIL);
    os.Comment("Standard no-arg constructor");
    os.DirectiveIS(Jvm.dot_method, Jvm.att_public, "<init>()V");
    os.Code(Jvm.opc_aload_0);
    os.CallS(Jvm.opc_invokespecial, objectInit, 1, 0);
    os.Code(Jvm.opc_return);
    os.Stack();
    os.Directive(Jvm.dot_end);
    os.Blank();
  END ModNoArgInit;

(* ---------------------------------------------------- *)

  PROCEDURE (os : JsmnFile)RecMakeInit*(rec : Ty.Record;
					prc : Id.PrcId);
    VAR	pTp : Ty.Procedure;
  BEGIN
    os.Blank();
    IF prc = NIL THEN
      IF D.noNew IN rec.xAttr THEN 
        os.Comment("There is no no-arg constructor for this class");
        os.Blank();
        RETURN;					(* PREMATURE RETURN HERE *)
      ELSIF D.xCtor IN rec.xAttr THEN 
        os.Comment("There is an explicit no-arg constructor for this class");
        os.Blank();
        RETURN;					(* PREMATURE RETURN HERE *)
      END;
    END;
    os.proc := newProcInfo(prc);
   (*
    *  Get the procedure type, if any.
    *)
    IF prc # NIL THEN 
      pTp := prc.type(Ty.Procedure);
      J.MkCallAttr(prc, pTp);
      os.DirectiveISS(Jvm.dot_method, Jvm.att_public, initStr, pTp.xName);
    ELSE
      os.Comment("Standard no-arg constructor");
      pTp := NIL;
      os.DirectiveIS(Jvm.dot_method, Jvm.att_public, "<init>()V");
    END;
    os.Code(Jvm.opc_aload_0);
  END RecMakeInit;

(*
    IF pTp # NIL THEN
     (*
      *  Copy the args to the super-constructor
      *)
      FOR idx := 0 TO pNm-1 DO os.GetLocal(pTp.formals.a[idx]) END;

    END;
 *)

  PROCEDURE (os : JsmnFile)CallSuperCtor*(rec : Ty.Record;
                                          pTy : Ty.Procedure);
    VAR	idx : INTEGER;
	fld : D.Idnt;
	pNm : INTEGER;
	string2 : LitValue.CharOpen;
  BEGIN
   (*
    *    Initialize the embedded superclass object.
    *)
    IF (rec.baseTp # NIL) & (rec.baseTp # G.anyRec) THEN
      IF pTy # NIL THEN
        string2 := LitValue.strToCharOpen("/" + initStr + pTy.xName^);
        pNm := pTy.formals.tide;
      ELSE
        string2 := LitValue.strToCharOpen(initSuffix);
        pNm := 0;
      END;
      os.Call2(Jvm.opc_invokespecial,
		 rec.baseTp(Ty.Record).xName, string2, pNm+1, 0);
    ELSE
      os.CallS(Jvm.opc_invokespecial, objectInit, 1, 0);
    END;
   (*
    *    Initialize fields, as necessary.
    *)
    FOR idx := 0 TO rec.fields.tide-1 DO
      fld := rec.fields.a[idx];
      IF (fld.type IS Ty.Record) OR (fld.type IS Ty.Array) THEN
        os.Comment("Initialize embedded object");
	os.Code(Jvm.opc_aload_0);
	os.VarInit(fld);
        os.PutGetF(Jvm.opc_putfield, rec, fld(Id.FldId));
      END;
    END;
(*
 *  os.Code(Jvm.opc_return);
 *  os.Stack();
 *  os.Directive(Jvm.dot_end);
 *  os.Blank();
 *)
  END CallSuperCtor;

(* ---------------------------------------------------- *)

  PROCEDURE (os : JsmnFile)CopyProcHead*(rec : Ty.Record);
  BEGIN
    os.proc := newProcInfo(NIL);
    os.Comment("standard record copy method");
    os.DirectiveIS(Jvm.dot_method, Jvm.att_public, 
				"__copy__(" + rec.scopeNm^ + ")V");
  END CopyProcHead;

(* ============================================================ *)
(*			Private Methods				*)
(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Mark(),NEW;
  BEGIN
    GPTextFiles.WriteChar(os.file, ";");
    GPText.WriteInt(os.file, os.proc.dNum, 3);
    GPText.WriteInt(os.file, os.proc.dMax, 3);
    GPTextFiles.WriteEOL(os.file);
  END Mark;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)CatStr(IN str : ARRAY OF CHAR),NEW;
  BEGIN
    GPTextFiles.WriteNChars(os.file, str, LEN(str$));
  END CatStr;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Tstring(IN str : ARRAY OF CHAR),NEW;
  BEGIN
    GPTextFiles.WriteChar(os.file, ASCII.HT);
    GPTextFiles.WriteNChars(os.file, str, LEN(str$));
  END Tstring;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Tint(int : INTEGER),NEW;
  BEGIN
    GPTextFiles.WriteChar(os.file, ASCII.HT);
    GPText.WriteInt(os.file, int, 1);
  END Tint;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Tlong(long : LONGINT),NEW;
  BEGIN
    GPTextFiles.WriteChar(os.file, ASCII.HT);
    GPText.WriteLong(os.file, long, 1);
  END Tlong;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)QuoteStr(IN str : ARRAY OF CHAR),NEW;
    VAR ix : INTEGER;
	ch : CHAR;
  BEGIN
    ix := 0;
    ch := str[0];
    GPTextFiles.WriteChar(os.file, '"');
    WHILE ch # 0X DO
      CASE ch OF
      | "\",'"' : GPTextFiles.WriteChar(os.file, "\");
		  GPTextFiles.WriteChar(os.file, ch);
      | 9X	: GPTextFiles.WriteChar(os.file, "\");
		  GPTextFiles.WriteChar(os.file, "t");
      | 0AX	: GPTextFiles.WriteChar(os.file, "\");
		  GPTextFiles.WriteChar(os.file, "n");
      ELSE
	GPTextFiles.WriteChar(os.file, ch);
      END;
      INC(ix);
      ch := str[ix];
    END;
    GPTextFiles.WriteChar(os.file, '"');
  END QuoteStr;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Prefix(code : INTEGER),NEW;
  BEGIN
    GPTextFiles.WriteChar(os.file, ASCII.HT);
    GPTextFiles.WriteNChars(os.file,Jvm.op[code],LEN(Jvm.op[code]$));
  END Prefix;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Suffix(code : INTEGER),NEW;
  BEGIN
    GPTextFiles.WriteEOL(os.file);
    INC(os.proc.dNum, Jvm.dl[code]);
    IF os.proc.dNum > os.proc.dMax THEN os.proc.dMax := os.proc.dNum END;
    IF CompState.verbose THEN os.Mark() END;
  END Suffix;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Access(acc : SET),NEW;
    VAR att : INTEGER;
  BEGIN
    FOR att := 0 TO 10 DO
      IF att IN acc THEN
	GPText.WriteString(os.file, Jvm.access[att]);
	GPTextFiles.WriteChar(os.file, ' ');
      END;
    END;
  END Access;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)RefLab(l : J.Label),NEW;
  BEGIN
    GPTextFiles.WriteChar(os.file, ASCII.HT);
    GPTextFiles.WriteChar(os.file, "l");
    GPTextFiles.WriteChar(os.file, "b");
    GPText.WriteInt(os.file, l.defIx, 1);
  END RefLab;

  PROCEDURE (os : JsmnFile)AddSwitchLab*(l : J.Label; pos : INTEGER);
  BEGIN
    os.RefLab(l);
    GPTextFiles.WriteEOL(os.file);
  END AddSwitchLab;

  PROCEDURE (os : JsmnFile)LstDef*(l : J.Label);
  BEGIN
    GPText.WriteString(os.file, "default:");
    os.RefLab(l);
    GPTextFiles.WriteEOL(os.file);
  END LstDef;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Idnt(idD : D.Idnt),NEW;
  BEGIN
    GPText.WriteString(os.file, D.getName.ChPtr(idD));
  END Idnt;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Type(typ : D.Type),NEW;
  BEGIN
    WITH typ : Ty.Base DO
	GPText.WriteString(os.file, typ.xName);
    | typ : Ty.Vector DO
	IF typ.xName = NIL THEN J.MkVecName(typ) END;
	GPText.WriteString(os.file, typ.xName);
	| typ : Ty.Procedure DO
        IF typ.xName = NIL THEN J.MkProcTypeName(typ) END;
        GPText.WriteString(os.file, typ.hostClass.scopeNm);
    | typ : Ty.Array DO
	GPTextFiles.WriteChar(os.file, "[");
	os.Type(typ.elemTp);
    | typ : Ty.Record DO
	IF typ.xName = NIL THEN J.MkRecName(typ) END;
	GPText.WriteString(os.file, typ.scopeNm);
    | typ : Ty.Enum DO
	GPText.WriteString(os.file, G.intTp.xName);
    | typ : Ty.Pointer DO
	os.Type(typ.boundTp);
    | typ : Ty.Opaque DO
	IF typ.xName = NIL THEN J.MkAliasName(typ) END;
	GPText.WriteString(os.file, typ.scopeNm);
    END;
  END Type;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)TypeTag(typ : D.Type),NEW;
  BEGIN
    WITH typ : Ty.Base DO
	GPText.WriteString(os.file, typ.xName);
    | typ : Ty.Array DO
	GPTextFiles.WriteChar(os.file, "[");
	os.TypeTag(typ.elemTp);
    | typ : Ty.Record DO
	IF typ.xName = NIL THEN J.MkRecName(typ) END;
	GPText.WriteString(os.file, typ.xName);
    | typ : Ty.Pointer DO
	os.TypeTag(typ.boundTp);
    | typ : Ty.Opaque DO
	IF typ.xName = NIL THEN J.MkAliasName(typ) END;
	GPText.WriteString(os.file, typ.xName);
    END;
  END TypeTag;

(* ============================================================ *)
(*			Exported Methods			*)
(* ============================================================ *)

  PROCEDURE (os : JsmnFile)newLabel*() : J.Label;
  VAR
    lab : J.Label;
  BEGIN
    NEW(lab);
    INC(os.nxtLb); 
    lab.defIx := os.nxtLb;
    RETURN lab;
  END newLabel;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)getLabelRange*(VAR labs : ARRAY OF J.Label);
    VAR labNo : INTEGER;
        count : INTEGER;
        i     : INTEGER;
        
  BEGIN
    count := LEN(labs); 
    labNo := os.nxtLb + 1;
    INC(os.nxtLb, count); 
    FOR i := 0 TO count-1 DO
      NEW(labs[i]);
      labs[i].defIx := labNo;
      INC(labNo);
    END;
  END getLabelRange;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Blank*(),NEW;
  BEGIN
    GPTextFiles.WriteEOL(os.file);
  END Blank;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Directive(dir : INTEGER),NEW;
  BEGIN
    os.CatStr(Jvm.dirStr[dir]);
    GPTextFiles.WriteEOL(os.file);
  END Directive;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)DirectiveS(dir : INTEGER;
				   IN str : ARRAY OF CHAR),NEW;
  BEGIN
    os.CatStr(Jvm.dirStr[dir]);
    GPTextFiles.WriteChar(os.file, " ");
    GPTextFiles.WriteNChars(os.file, str, LEN(str$));
    GPTextFiles.WriteEOL(os.file);
  END DirectiveS;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)DirectiveIS(dir : INTEGER;
				        att : SET;
				     IN str : ARRAY OF CHAR),NEW;
  BEGIN
    os.CatStr(Jvm.dirStr[dir]);
    GPTextFiles.WriteChar(os.file, " ");
    os.Access(att);
    GPTextFiles.WriteNChars(os.file, str, LEN(str$));
    GPTextFiles.WriteEOL(os.file);
  END DirectiveIS;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)DirectiveISS(dir : INTEGER;
				         att : SET;
				      IN s1  : ARRAY OF CHAR;
				      IN s2  : ARRAY OF CHAR),NEW;
  BEGIN
    os.CatStr(Jvm.dirStr[dir]);
    GPTextFiles.WriteChar(os.file, " ");
    os.Access(att);
    GPTextFiles.WriteNChars(os.file, s1, LEN(s1$));
    GPTextFiles.WriteNChars(os.file, s2, LEN(s2$));
    GPTextFiles.WriteEOL(os.file);
  END DirectiveISS;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)Comment*(IN s : ARRAY OF CHAR);
  BEGIN
    GPTextFiles.WriteChar(os.file, ";");
    GPTextFiles.WriteChar(os.file, " ");
    GPTextFiles.WriteNChars(os.file, s, LEN(s$));
    GPTextFiles.WriteEOL(os.file);
  END Comment;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)DefLab*(l : J.Label);
  BEGIN
    GPTextFiles.WriteChar(os.file, "l");
    GPTextFiles.WriteChar(os.file, "b");
    GPText.WriteInt(os.file, l.defIx, 1);
    GPTextFiles.WriteChar(os.file, ":");
    GPTextFiles.WriteEOL(os.file);
  END DefLab;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)DefLabC*(l : J.Label; IN c : ARRAY OF CHAR);
  BEGIN
    GPTextFiles.WriteChar(os.file, "l");
    GPTextFiles.WriteChar(os.file, "b");
    GPText.WriteInt(os.file, l.defIx, 1);
    GPTextFiles.WriteChar(os.file, ":");
    GPTextFiles.WriteChar(os.file, ASCII.HT);
    os.Comment(c);
  END DefLabC;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Code*(code : INTEGER);
  BEGIN
    os.Prefix(code);
    os.Suffix(code);
  END Code;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)CodeI*(code,int : INTEGER);
  BEGIN
    os.Prefix(code);
    os.Tint(int);
    os.Suffix(code);
  END CodeI;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)CodeT*(code : INTEGER; type : D.Type);
  BEGIN
    os.Prefix(code);
    GPTextFiles.WriteChar(os.file, ASCII.HT);
    os.TypeTag(type);
    os.Suffix(code);
  END CodeT;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)CodeL*(code : INTEGER; long : LONGINT);
  BEGIN
    os.Prefix(code);
    os.Tlong(long);
    os.Suffix(code);
  END CodeL;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)CodeR*(code : INTEGER; real : REAL; short : BOOLEAN);
    VAR nam : ARRAY 64 OF CHAR;
  BEGIN
    os.Prefix(code);
    RTS.RealToStr(real, nam);
    os.Tstring(nam$);
    os.Suffix(code);
  END CodeR;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)CodeLb*(code : INTEGER; i2 : J.Label);
  BEGIN
    os.Prefix(code);
    os.RefLab(i2);
    os.Suffix(code);
  END CodeLb;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)CodeII*(code,i1,i2 : INTEGER),NEW;
  BEGIN
    os.Prefix(code);
    os.Tint(i1);
    os.Tint(i2);
    os.Suffix(code);
  END CodeII;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)CodeInc*(localIx, incVal : INTEGER);
  BEGIN
    os.CodeII(Jvm.opc_iinc, localIx, incVal);
  END CodeInc;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)CodeS*(code : INTEGER; IN str : ARRAY OF CHAR),NEW;
  BEGIN
    os.Prefix(code);
    os.Tstring(str);
    os.Suffix(code);
  END CodeS;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)CodeC*(code : INTEGER; IN str : ARRAY OF CHAR);
  BEGIN
    os.Prefix(code);
    GPTextFiles.WriteNChars(os.file, str, LEN(str$));
    os.Suffix(code);
  END CodeC;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)CodeSwitch*(loIx,hiIx : INTEGER; dfLb : J.Label);
  BEGIN
    os.CodeII(Jvm.opc_tableswitch,loIx,hiIx);
  END CodeSwitch;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)PushStr*(IN str : LitValue.CharOpen);
  (* Use target quoting conventions for the literal string *)
  BEGIN
    os.Prefix(Jvm.opc_ldc);
    GPTextFiles.WriteChar(os.file, ASCII.HT);
    os.QuoteStr(str^);
    os.Suffix(Jvm.opc_ldc);
  END PushStr;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)CallS*(code : INTEGER; 
				IN str : ARRAY OF CHAR;
				argL,retL : INTEGER),NEW;
  BEGIN
    os.Prefix(code);
    os.Tstring(str);
    IF code = Jvm.opc_invokeinterface THEN os.Tint(argL) END;
    GPTextFiles.WriteEOL(os.file);
    INC(os.proc.dNum, retL-argL);
    IF os.proc.dNum > os.proc.dMax THEN os.proc.dMax := os.proc.dNum END;
    IF CompState.verbose THEN os.Mark() END;
  END CallS;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)CallIT*(code : INTEGER; 
				   proc : Id.Procs; 
				   type : Ty.Procedure);
    VAR argL, retL : INTEGER;
	clsNam : LitValue.CharOpen;
  BEGIN
    os.Prefix(code);
    IF proc.scopeNm = NIL THEN J.MkProcName(proc) END;
    os.Tstring(proc.scopeNm);
    GPTextFiles.WriteChar(os.file, "/");
    WITH proc : Id.PrcId DO clsNam := proc.clsNm;
    |    proc : Id.MthId DO clsNam := proc.bndType(Ty.Record).extrnNm;
    END;
    os.CatStr(clsNam);
    GPTextFiles.WriteChar(os.file, "/");
    os.CatStr(proc.prcNm);
    os.CatStr(type.xName);
    argL := type.argN;
    retL := type.retN;
    IF code = Jvm.opc_invokeinterface THEN os.Tint(type.argN) END;
    GPTextFiles.WriteEOL(os.file);
    INC(os.proc.dNum, retL-argL);
    IF os.proc.dNum > os.proc.dMax THEN os.proc.dMax := os.proc.dNum END;
    IF CompState.verbose THEN os.Mark() END;
  END CallIT;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Call2*(code : INTEGER; 
				IN st1 : ARRAY OF CHAR;
				IN st2 : ARRAY OF CHAR;
				argL,retL : INTEGER),NEW;
  BEGIN
    os.Prefix(code);
    os.Tstring(st1);
    os.CatStr(st2);
    IF code = Jvm.opc_invokeinterface THEN os.Tint(argL) END;
    GPTextFiles.WriteEOL(os.file);
    INC(os.proc.dNum, retL-argL);
    IF os.proc.dNum > os.proc.dMax THEN os.proc.dMax := os.proc.dNum END;
    IF CompState.verbose THEN os.Mark() END;
  END Call2;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)MultiNew*(elT : D.Type;
				     dms : INTEGER),NEW;
   (* dsc is the array descriptor, dms the number of dimensions *)
    VAR i : INTEGER;
  BEGIN
    os.Prefix(Jvm.opc_multianewarray);
    GPTextFiles.WriteChar(os.file, ASCII.HT);
    FOR i := 1 TO dms DO GPTextFiles.WriteChar(os.file, "[") END;
    os.TypeTag(elT);
    os.Tint(dms);
    GPTextFiles.WriteEOL(os.file);
    DEC(os.proc.dNum, dms-1);
  END MultiNew;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)PutGetS*(code : INTEGER;
				    blk  : Id.BlkId;
				    fld  : Id.VarId);
    VAR size : INTEGER;
  (* Emit putstatic and getstatic for static field *)
  BEGIN
    os.Prefix(code);
    IF blk.xName = NIL THEN J.MkBlkName(blk) END;
    IF fld.varNm = NIL THEN J.MkVarName(fld) END;
    os.Tstring(blk.scopeNm);
    GPTextFiles.WriteChar(os.file, "/");
    os.CatStr(fld.clsNm);
    GPTextFiles.WriteChar(os.file, "/");
    os.CatStr(fld.varNm);
    GPTextFiles.WriteChar(os.file, " ");
    os.Type(fld.type);
    GPTextFiles.WriteEOL(os.file);
    size := J.jvmSize(fld.type);
    IF    code = Jvm.opc_getstatic THEN INC(os.proc.dNum, size);
    ELSIF code = Jvm.opc_putstatic THEN DEC(os.proc.dNum, size);
    END;
    IF os.proc.dNum > os.proc.dMax THEN os.proc.dMax := os.proc.dNum END;
    IF CompState.verbose THEN os.Mark() END;
  END PutGetS;

(* -------------------------------------------- *)

  PROCEDURE (os : JsmnFile)PutGetF*(code : INTEGER;
				    rec  : Ty.Record;
				    fld  : Id.AbVar);
(*
				    fld  : Id.FldId);
 *)
    VAR size : INTEGER;
  (* Emit putfield and getfield for record field *)
  BEGIN
    os.Prefix(code);
    GPTextFiles.WriteChar(os.file, ASCII.HT);
    os.TypeTag(rec);
    GPTextFiles.WriteChar(os.file, "/");
    os.Idnt(fld);
    GPTextFiles.WriteChar(os.file, " ");
    os.Type(fld.type);
    GPTextFiles.WriteEOL(os.file);
    size := J.jvmSize(fld.type);
    IF    code = Jvm.opc_getfield THEN INC(os.proc.dNum, size-1);
    ELSIF code = Jvm.opc_putfield THEN DEC(os.proc.dNum, size+1);
    END;
    IF os.proc.dNum > os.proc.dMax THEN os.proc.dMax := os.proc.dNum END;
    IF CompState.verbose THEN os.Mark() END;
  END PutGetF;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Alloc1d*(elTp : D.Type);
  BEGIN
    WITH elTp : Ty.Base DO
      IF (elTp.tpOrd < Ty.anyRec) THEN
        os.CodeS(Jvm.opc_newarray, typeName[elTp.tpOrd]);
      ELSE
        os.Prefix(Jvm.opc_anewarray);
        os.Tstring(object);
        os.Suffix(Jvm.opc_anewarray);
      END;
    ELSE
      os.Prefix(Jvm.opc_anewarray);
      GPTextFiles.WriteChar(os.file, ASCII.HT);
      os.TypeTag(elTp);
      os.Suffix(Jvm.opc_anewarray);
    END;
  END Alloc1d;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)MkNewRecord*(typ : Ty.Record);
  BEGIN
    os.CodeT(Jvm.opc_new, typ);
    os.Code(Jvm.opc_dup);
    os.Prefix(Jvm.opc_invokespecial);
    os.Tstring(typ.xName);
    os.CatStr(initSuffix);
    os.Suffix(Jvm.opc_invokespecial);
  END MkNewRecord;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)MkNewFixedArray*(topE : D.Type; len0 : INTEGER);
    VAR dims : INTEGER;
	arTp : Ty.Array;
	elTp : D.Type;
  BEGIN
    (*
    //  Fixed-size, possibly multi-dimensional arrays.
    //  The code relies on the semantic property in CP
    //  that the element-type of a fixed array type cannot
    //  be an open array. This simplifies the code somewhat.
    *)
    os.PushInt(len0);
    dims := 1;
    elTp := topE;
   (*
    *  Find the number of dimensions ...
    *)
    LOOP
      WITH elTp : Ty.Array DO arTp := elTp ELSE EXIT END;
      elTp := arTp.elemTp;
      os.PushInt(arTp.length);
      INC(dims);
    END;
    IF dims = 1 THEN
      os.Alloc1d(elTp);
     (*
      *  Stack is (top) len0, ref...
      *)
      IF elTp.kind = Ty.recTp THEN os.Init1dArray(elTp, len0) END;
    ELSE
     (*
      *  Allocate the array headers for all dimensions.
      *  Stack is (top) lenN, ... len0, ref...
      *)
      os.MultiNew(elTp, dims);
     (*
      *  Stack is (top) ref...
      *)
      IF elTp.kind = Ty.recTp THEN os.InitNdArray(topE, elTp) END;
    END;
  END MkNewFixedArray;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)MkNewOpenArray*(arrT : Ty.Array; dims : INTEGER);
    VAR elTp : D.Type;
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
      os.Alloc1d(elTp);
     (*
      *  Stack is now (top) ref ...
      *  and we _might_ need to initialize the elements.
      *)
      IF (elTp.kind = Ty.recTp) OR 
	 (elTp.kind = Ty.arrTp) THEN 
	os.Init1dArray(elTp, 0);
      END;
    ELSE
      os.MultiNew(elTp, dims);
     (*
      *    Stack is now (top) ref ...
      *    Now we _might_ need to initialize the elements.
      *)
      IF (elTp.kind = Ty.recTp) OR 
	 (elTp.kind = Ty.arrTp) THEN 
	os.InitNdArray(arrT.elemTp, elTp);
      END;
    END;
  END MkNewOpenArray;


(* ============================================================ *)

  PROCEDURE (os : JsmnFile)MkArrayCopy*(arrT : Ty.Array);
    VAR dims : INTEGER;
        elTp : D.Type;
  BEGIN
   (*
    *	Assert: we must find the lengths from the runtime 
    *   descriptors.  Find the number of dimensions.  The 
    *   array to copy is on the top of stack, which reads -
    *		(top) aRef, ...
    *)
    elTp := arrT.elemTp;
    IF elTp.kind # Ty.arrTp THEN
      os.Code(Jvm.opc_arraylength);     (* (top) len0, aRef,...	*)
      os.Alloc1d(elTp);			     (* (top) aRef, ...		*)
      IF elTp.kind = Ty.recTp THEN os.Init1dArray(elTp, 0) END; (*0 ==> open*)
    ELSE
      dims := 1;
      REPEAT
       (* 
        *  Invariant: an array reference is on the top of
        *  of the stack, which reads:
        *		(top) [arRf, lengths,] arRf ...
	*)
	INC(dims);
        elTp := elTp(Ty.Array).elemTp;
        os.Code(Jvm.opc_dup);	     (*          arRf, arRf,... *)
        os.Code(Jvm.opc_arraylength);   (*    len0, arRf, arRf,... *)
        os.Code(Jvm.opc_swap);	     (*    arRf, len0, arRf,... *)
        os.Code(Jvm.opc_iconst_0);	     (* 0, arRf, len0, arRf,... *)
        os.Code(Jvm.opc_aaload);	     (*    arRf, len0, arRf,... *)
       (* 
        *  Stack reads:	(top) arRf, lenN, [lengths,] arRf ...
	*)
      UNTIL  elTp.kind # Ty.arrTp;
     (*
      *  Now get the final length...
      *)
      os.Code(Jvm.opc_arraylength);  
     (* 
      *   Stack reads:	(top) lenM, lenN, [lengths,] arRf ...
      *   Allocate the array headers for all dimensions.
      *)
      os.MultiNew(elTp, dims);
     (*
      *  Stack is (top) ref...
      *)
      IF elTp.kind = Ty.recTp THEN os.InitNdArray(arrT.elemTp, elTp) END;
    END;
  END MkArrayCopy;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)VarInit*(var : D.Idnt);
    VAR typ : D.Type;
  BEGIN
   (*
    *  Precondition: var is of a type that needs initialization
    *)
    typ := var.type;
    WITH typ : Ty.Record DO
	os.MkNewRecord(typ);
    | typ : Ty.Array DO
	os.MkNewFixedArray(typ.elemTp, typ.length);
    ELSE
      os.Code(Jvm.opc_aconst_null);
    END;
  END VarInit;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)ValRecCopy*(typ : Ty.Record);
    VAR nam : LitValue.CharOpen;
  BEGIN
   (*
    *     Stack at entry is (top) srcRef, dstRef...
    *)
    IF typ.xName = NIL THEN J.MkRecName(typ) END;
    nam := typ.xName;
    os.CallS(Jvm.opc_invokevirtual, 
			nam^ + "/__copy__(L" + nam^ + ";)V", 2, 0);
  END ValRecCopy;


(* ============================================================ *)

  PROCEDURE (os : JsmnFile)CallRTS*(ix,args,ret : INTEGER);
  BEGIN
    os.CallS(Jvm.opc_invokestatic, rtsProcs[ix], args, ret);
  END CallRTS;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)CallGetClass*();
  BEGIN
    os.CallS(Jvm.opc_invokevirtual, rtsProcs[J.GetTpM], 1, 1);
  END CallGetClass;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Trap*(IN str : ARRAY OF CHAR);
  BEGIN
    os.CodeS(Jvm.opc_new, jlError);
    os.Code(Jvm.opc_dup);
(* Do we need the quotes? *)
    os.PushStr(LitValue.strToCharOpen('"' + str + '"'));
    os.CallS(Jvm.opc_invokespecial, mkError,2,0);
    os.Code(Jvm.opc_athrow);
  END Trap;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)CaseTrap*(i : INTEGER);
  BEGIN
    os.CodeS(Jvm.opc_new, jlError);
    os.Code(Jvm.opc_dup);
    os.LoadLocal(i, G.intTp);
    os.CallS(Jvm.opc_invokestatic, 
		"CP/CPJrts/CPJrts/CaseMesg(I)Ljava/lang/String;",1,1);
    os.CallS(Jvm.opc_invokespecial, mkError,2,0);
    os.Code(Jvm.opc_athrow);
  END CaseTrap;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)WithTrap*(id : D.Idnt);
  BEGIN
    os.CodeS(Jvm.opc_new, jlError);
    os.Code(Jvm.opc_dup);
    os.GetVar(id);
    os.CallS(Jvm.opc_invokestatic, 
	"CP/CPJrts/CPJrts/WithMesg(Ljava/lang/Object;)Ljava/lang/String;",1,1);
    os.CallS(Jvm.opc_invokespecial, mkError,2,0);
    os.Code(Jvm.opc_athrow);
  END WithTrap;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Header*(IN str : ARRAY OF CHAR);
    VAR date : ARRAY 64 OF CHAR;
  BEGIN
    RTS.GetDateString(date);
    os.Comment("Jasmin output produced by CPascal compiler (" + 
					RTS.defaultTarget + " version)");
    os.Comment("at date: " + date);
    os.Comment("from source file <" + str + '>');
  END Header;

(* ============================================================ *)
 
  PROCEDURE (os : JsmnFile)StartRecClass*(rec : Ty.Record);
  VAR
    baseT  : D.Type;
    attSet : SET;
    clsId  : D.Idnt;
    impRec : D.Type;
    index  : INTEGER;
  BEGIN
    os.Blank();
    os.DirectiveS(Jvm.dot_source, CompState.srcNam);
   (*
    *   Account for the record attributes.
    *)
    CASE rec.recAtt OF
    | Ty.noAtt : attSet := Jvm.att_final;
    | Ty.isAbs : attSet := Jvm.att_abstract;
    | Ty.limit : attSet := Jvm.att_empty;
    | Ty.extns : attSet := Jvm.att_empty;
    END;
   (*
    *   Get the pointer IdDesc, if this is anonymous.
    *)
    IF rec.bindTp # NIL THEN 
      clsId := rec.bindTp.idnt;
    ELSE 
      clsId := rec.idnt;
    END;
   (*
    *   Account for the identifier visibility.
    *)
    IF clsId # NIL THEN
      IF clsId.vMod = D.pubMode THEN
	attSet := attSet + Jvm.att_public;
      ELSIF clsId.vMod = D.prvMode THEN
	attSet := attSet + Jvm.att_private;
      END;
    END;
    os.DirectiveIS(Jvm.dot_class, attSet, rec.xName);
   (*
    *   Compute the super class attribute.
    *)
    baseT := rec.baseTp;
    WITH baseT : Ty.Record DO
      IF baseT.xName = NIL THEN J.MkRecName(baseT) END;
      os.DirectiveS(Jvm.dot_super, baseT.xName);
    ELSE
      os.DirectiveS(Jvm.dot_super, object);
    END;
   (*
    *   Emit interface declarations (if any)
    *)
    IF rec.interfaces.tide > 0 THEN
      FOR index := 0 TO rec.interfaces.tide-1 DO
	impRec := rec.interfaces.a[index];
	baseT  := impRec.boundRecTp();
        IF baseT.xName = NIL THEN J.MkRecName(baseT(Ty.Record)) END;
	os.DirectiveS(Jvm.dot_implements, baseT.xName);
      END;
    END;
    os.Blank();
  END StartRecClass;

  PROCEDURE (os : JsmnFile)StartModClass*(mod : Id.BlkId);
  BEGIN
    IF mod.main THEN os.Comment("This module implements CPmain") END;
    os.Blank();
    os.DirectiveS(Jvm.dot_source, CompState.srcNam);
    IF mod.scopeNm[0] = 0X THEN
      os.DirectiveIS(Jvm.dot_class, modAttrib, mod.xName);
    ELSE
      os.DirectiveISS(Jvm.dot_class, modAttrib, mod.scopeNm^ + '/', mod.xName);
    END;
    os.DirectiveS(Jvm.dot_super, object);
    os.Blank();
  END StartModClass;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)EmitField*(id : Id.AbVar);
  VAR
    att : SET;
  BEGIN
    IF id IS Id.FldId THEN att := Jvm.att_empty; 
    ELSE att := Jvm.att_static; END;
    IF id.vMod # D.prvMode THEN (* any export ==> public in JVM *)
      att := att + Jvm.att_public;
    END;
    os.CatStr(Jvm.dirStr[Jvm.dot_field]);
    GPTextFiles.WriteChar(os.file, " ");
    os.Access(att);
    GPTextFiles.WriteChar(os.file, " ");
    os.Idnt(id);
    GPTextFiles.WriteChar(os.file, " ");
    os.Type(id.type);
    GPTextFiles.WriteEOL(os.file);
  END EmitField;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Line*(nm : INTEGER);
  BEGIN
    os.CatStr(Jvm.dirStr[Jvm.dot_line]); 
    os.Tint(nm);
    GPTextFiles.WriteEOL(os.file);
  END Line;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Locals(),NEW;
  BEGIN
    os.CatStr(Jvm.dirStr[Jvm.dot_limit]); 
    os.CatStr(" locals");
    os.Tint(os.proc.lMax);
    GPTextFiles.WriteEOL(os.file);
  END Locals;

(* ============================================================ *)

  PROCEDURE (os : JsmnFile)Stack(),NEW;
  BEGIN
    os.CatStr(Jvm.dirStr[Jvm.dot_limit]); 
    os.CatStr(" stack");
    os.Tint(os.proc.dMax);
    GPTextFiles.WriteEOL(os.file);
  END Stack;

(* ============================================================ *)
(*			Namehandling Methods			*)
(* ============================================================ *)

  PROCEDURE (os : JsmnFile)LoadConst*(num : INTEGER);
  BEGIN
    IF (num >= MIN(SHORTINT)) & (num <= MAX(SHORTINT)) THEN
      os.CodeI(Jvm.opc_sipush, num);
    ELSE
      os.CodeI(Jvm.opc_ldc, num);
    END;
  END LoadConst;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : JsmnFile)Try*();
    VAR start : J.Label;
  BEGIN
    start := os.newLabel();
    os.proc.exLb := os.newLabel();
    os.proc.hnLb := os.newLabel();
    os.CatStr(Jvm.dirStr[Jvm.dot_catch]);
    os.CatStr(" java/lang/Exception from lb");
    GPText.WriteInt(os.file, start.defIx, 1);
    os.CatStr(" to lb");
    GPText.WriteInt(os.file, os.proc.exLb.defIx, 1);
    os.CatStr(" using lb");
    GPText.WriteInt(os.file, os.proc.hnLb.defIx, 1);
    GPTextFiles.WriteEOL(os.file);
    os.DefLab(start);
  END Try;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : JsmnFile)MkNewException*();
  BEGIN
    os.CodeS(Jvm.opc_new, jlExcept);
  END MkNewException;

  PROCEDURE (os : JsmnFile)InitException*();
  BEGIN
    os.CallS(Jvm.opc_invokespecial, mkExcept, 2,0);
  END InitException;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : JsmnFile)Catch*(prc : Id.Procs);
  BEGIN
    os.DefLab(os.proc.exLb);
    os.DefLab(os.proc.hnLb);
    os.StoreLocal(prc.except.varOrd, NIL);
   (*
    *  Now make sure that the overall stack
    *  depth computation is correctly initialized
    *)
    IF os.proc.dMax < 1 THEN os.proc.dMax := 1 END;
    os.proc.dNum := 0;
  END Catch;

(* ============================================================ *)

  PROCEDURE (jf : JsmnFile)Dump*();
  BEGIN
    jf.Blank();
    jf.Comment("end output produced by CPascal");
    GPTextFiles.CloseFile(jf.file);
  END Dump;

(* ============================================================ *)
(* ============================================================ *)
BEGIN

  typeChar[       0] := "?";		
  typeChar[ Ty.boolN] := "Z";
  typeChar[ Ty.sChrN] := "C";
  typeChar[ Ty.charN] := "C";
  typeChar[ Ty.byteN] := "B";
  typeChar[ Ty.sIntN] := "S";
  typeChar[  Ty.intN] := "I";
  typeChar[ Ty.lIntN] := "J";
  typeChar[ Ty.sReaN] := "F";
  typeChar[ Ty.realN] := "D";
  typeChar[  Ty.setN] := "I";
  typeChar[Ty.anyRec] := "?";
  typeChar[Ty.anyPtr] := "?";
  typeChar[  Ty.strN] := "?";
  typeChar[ Ty.sStrN] := "?";

  typeName[     0] := "";		
  typeName[ Ty.boolN] := "boolean";
  typeName[ Ty.sChrN] := "char";
  typeName[ Ty.charN] := "char";
  typeName[ Ty.byteN] := "byte";
  typeName[ Ty.sIntN] := "short";
  typeName[  Ty.intN] := "int";
  typeName[ Ty.lIntN] := "long";
  typeName[ Ty.sReaN] := "float";
  typeName[ Ty.realN] := "double";
  typeName[  Ty.setN] := "int";
  typeName[Ty.anyRec] := "";
  typeName[Ty.anyPtr] := "";
  typeName[  Ty.strN] := "";
  typeName[ Ty.sStrN] := "";

  rtsProcs[J.StrCmp] := "CP/CPJrts/CPJrts/strCmp([C[C)I";
  rtsProcs[J.StrToChrOpen] := 
	"CP/CPJrts/CPJrts/JavaStrToChrOpen(Ljava/lang/String;)[C";
  rtsProcs[J.StrToChrs] := 
	"CP/CPJrts/CPJrts/JavaStrToFixChr([CLjava/lang/String;)V";
  rtsProcs[J.ChrsToStr] := 
	"CP/CPJrts/CPJrts/FixChToJavaStr([C)Ljava/lang/String;";
  rtsProcs[J.StrCheck] := "CP/CPJrts/CPJrts/ChrArrCheck([C)V";
  rtsProcs[J.StrLen] := "CP/CPJrts/CPJrts/ChrArrLength([C)I";
  rtsProcs[J.ToUpper] := "java/lang/Character/toUpperCase(C)C";
  rtsProcs[J.DFloor] := "java/lang/Math/floor(D)D";
  rtsProcs[J.ModI] := "CP/CPJrts/CPJrts/CpModI(II)I";
  rtsProcs[J.ModL] := "CP/CPJrts/CPJrts/CpModL(JJ)J";
  rtsProcs[J.DivI] := "CP/CPJrts/CPJrts/CpDivI(II)I";
  rtsProcs[J.DivL] := "CP/CPJrts/CPJrts/CpDivL(JJ)J";
  rtsProcs[J.StrCatAA] := 
	"CP/CPJrts/CPJrts/ArrArrToString([C[C)Ljava/lang/String;";
  rtsProcs[J.StrCatSA] := 
"CP/CPJrts/CPJrts/StrArrToString(Ljava/lang/String;[C)Ljava/lang/String;";
  rtsProcs[J.StrCatAS] := 
"CP/CPJrts/CPJrts/ArrStrToString([CLjava/lang/String;)Ljava/lang/String;";
  rtsProcs[J.StrCatSS] := "CP/CPJrts/CPJrts/StrStrToString(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;";
  rtsProcs[J.StrLP1] := "CP/CPJrts/CPJrts/ChrArrLplus1([C)I";
  rtsProcs[J.StrVal] := "CP/CPJrts/CPJrts/ChrArrStrCopy([C[C)V";
  rtsProcs[J.SysExit] := "java/lang/System/exit(I)V";
  rtsProcs[J.LoadTp1] := "CP/CPJrts/CPJrts/getClassByOrd(I)Ljava/lang/Class;";
  rtsProcs[J.LoadTp2] := 
	"CP/CPJrts/CPJrts/getClassByName(Ljava/lang/String;)Ljava/lang/Class;";
  rtsProcs[J.GetTpM] := "java/lang/Object/getClass()Ljava/lang/Class;";
END JsmnUtil.
(* ============================================================ *)
(* ============================================================ *)

