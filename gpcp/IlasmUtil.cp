(* ============================================================ *)
(*  MsilUtil is the module which writes ILASM file structures   *)
(*  Copyright (c) John Gough 1999, 2000.                        *)
(* ============================================================ *)

MODULE IlasmUtil;

  IMPORT 
        GPCPcopyright,
        RTS, 
        ASCII,
        Console,
        GPText,
        GPBinFiles,
        GPTextFiles,
     
        Lv := LitValue,
        Cs := CompState,
        Sy := Symbols,
        Mu := MsilUtil,
        Bi := Builtin,
        Id  := IdDesc,
        Ty  := TypeDesc,
        Scn := CPascalS,
        Asm := IlasmCodes;

(* ============================================================ *)

   CONST
        (* various ILASM-specific runtime name strings *)
        initPrefix  = "instance void ";
        initSuffix  = ".ctor() ";
        initString  = ".ctor";
        managedStr  = "il managed";
        specialStr  = "public specialname rtspecialname ";
        cctorStr    = "static void .cctor() ";
        objectInit  = "instance void $o::.ctor() ";
        mainString  = "public static void '.CPmain'($S[]) il managed";
        winString   = "public static void '.WinMain'($S[]) il managed";
        subSysStr   = "  .subsystem 0x00000002";
        copyHead    = "public void __copy__(";

   CONST
        putArgStr   = "$S[] [RTS]ProgArgs::argList";
        catchStr    = "      catch [mscorlib]System.Exception";

(* ============================================================ *)
(* ============================================================ *)

  TYPE IlasmFile* = POINTER TO RECORD (Mu.MsilFile)
                 (* Fields inherited from MsilFile *
                  *   srcS* : Lv.CharOpen; (* source file name   *)
                  *   outN* : Lv.CharOpen;
                  *   proc* : ProcInfo;
                  *)
                      nxtLb : INTEGER;
                      clsN* : Lv.CharOpen; (* current class name *)
                      file* : GPBinFiles.FILE;
                    END;

(* ============================================================ *)

  TYPE ILabel*    = POINTER TO RECORD (Mu.Label) 
                      labl : INTEGER;
                    END;

(* ============================================================ *)

  TYPE UByteArrayPtr* = POINTER TO ARRAY OF UBYTE;

(* ============================================================ *)

  VAR   nmArray   : Lv.CharOpenSeq;
        rts       : ARRAY Mu.rtsLen OF Lv.CharOpen;

  VAR   vals,                           (* "value"  *)
        clss,                           (* "class"  *)
        cln2,                           (* "::"     *)
        brks,                           (* "[]"     *)
        vStr,                           (* "void "  *)
        ouMk,                           (* "[out]"  *)
        lPar, rPar,                     (* ( )      *)
        rfMk,                           (* "&"      *)
        cmma,                           (* ","      *)
        brsz,                           (* "{} etc" *)
        vFld,                           (* "v$"     *)
        inVd : Lv.CharOpen;             (* "in.v "  *)

  VAR   evtAdd, evtRem : Lv.CharOpen;
        pVarSuffix     : Lv.CharOpen;
        xhrMk          : Lv.CharOpen;
  
  VAR   boxedObj       : Lv.CharOpen;

(* ============================================================ *)
(*                 Utility Coercion Method                      *)
(* ============================================================ *)

  PROCEDURE UBytesOf(IN str : ARRAY OF CHAR) : UByteArrayPtr;
    VAR result : UByteArrayPtr;
        index  : INTEGER;
        length : INTEGER;
  BEGIN
    length := LEN(str$);
    NEW(result, length);
    FOR index := 0 TO length-1 DO
      result[index] := USHORT(ORD(str[index]) MOD 256);
    END;
    RETURN result;
  END UBytesOf;

(* ============================================================ *)
(*                    Constructor Method                        *)
(* ============================================================ *)

  PROCEDURE newIlasmFile*(IN nam : ARRAY OF CHAR) : IlasmFile;
    VAR f : IlasmFile;
  BEGIN
    NEW(f);
    f.outN := BOX(nam + ".il");
    f.file := GPBinFiles.createFile(f.outN);
    RETURN f;
  END newIlasmFile;

(* ============================================================ *)

  PROCEDURE (t : IlasmFile)fileOk*() : BOOLEAN;
  BEGIN
    RETURN t.file # NIL;
  END fileOk;

(* ============================================================ *)
(*                    Some static utilities                     *)
(* ============================================================ *)

  PROCEDURE^ (os : IlasmFile)Locals(),NEW;
  PROCEDURE^ (os : IlasmFile)TypeName(typ : Sy.Type),NEW;
  PROCEDURE^ (os : IlasmFile)CallCombine(typ : Sy.Type; add : BOOLEAN),NEW;
  PROCEDURE^ (os : IlasmFile)Comment*(IN s : ARRAY OF CHAR);
  PROCEDURE^ (os : IlasmFile)CodeLb*(code : INTEGER; i2 : Mu.Label);
  PROCEDURE^ (os : IlasmFile)DefLabC*(l : Mu.Label; IN c : ARRAY OF CHAR);

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)MkNewProcInfo*(proc : Sy.Scope);
  BEGIN
    NEW(os.proc);
    Mu.InitProcInfo(os.proc, proc);
  END MkNewProcInfo;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)newLabel*() : Mu.Label;
    VAR label : ILabel;
  BEGIN
    NEW(label); 
    INC(os.nxtLb); 
    label.labl := os.nxtLb;
    RETURN label;
  END newLabel;

(* ============================================================ *)
(*   Signature handling for this version                        *)
(* ============================================================ *)

  PROCEDURE (os : IlasmFile)NumberParams*(pIdn : Id.Procs; 
                                          pTyp : Ty.Procedure);
    VAR parId : Id.ParId;
        index : INTEGER;
        count : INTEGER;
        first : BOOLEAN;
        fmArray  : Lv.CharOpenSeq;
   (* ----------------------------------------- *)
    PROCEDURE AppendTypeName(VAR lst : Lv.CharOpenSeq; 
                                 typ : Sy.Type;
                                 slf : IlasmFile);
    BEGIN
     (*
      *   We append type names, which must be lexically
      *   equivalent to the names used in the declaration
      *   in MethodDecl.
      *)
      IF typ.xName = NIL THEN Mu.MkTypeName(typ, slf) END;
      WITH typ : Ty.Base DO
          Lv.AppendCharOpen(lst, typ.xName);
      | typ : Ty.Vector DO
          Lv.AppendCharOpen(lst, clss);
          Lv.AppendCharOpen(lst, typ.xName);
      | typ : Ty.Array DO
          AppendTypeName(lst, typ.elemTp, slf);
          Lv.AppendCharOpen(lst, brks);
      | typ : Ty.Record DO
          IF ~(Sy.clsTp IN typ.xAttr) THEN Lv.AppendCharOpen(lst,vals) END;
          IF ~(Sy.spshl IN typ.xAttr) THEN Lv.AppendCharOpen(lst,clss) END;
          Lv.AppendCharOpen(lst, typ.scopeNm);
      | typ : Ty.Pointer DO
         (* 
          *  This is a pointer to a value class, which has a
          *  runtime representation as a boxed-class reference.
          *)
          IF Mu.isValRecord(typ.boundTp) THEN
            Lv.AppendCharOpen(lst, clss);
            Lv.AppendCharOpen(lst, typ.xName);
          ELSE
            AppendTypeName(lst, typ.boundTp, slf);
          END;
      | typ : Ty.Opaque DO
          Lv.AppendCharOpen(lst, clss);
          Lv.AppendCharOpen(lst, typ.xName);
      | typ : Ty.Enum DO
          Lv.AppendCharOpen(lst, vals);
          Lv.AppendCharOpen(lst, clss);
          Lv.AppendCharOpen(lst, typ.xName);
      | typ : Ty.Procedure DO
          Lv.AppendCharOpen(lst, clss);
          Lv.AppendCharOpen(lst, typ.tName);
      END;
    END AppendTypeName;
   (* ----------------------------------------- *)
  BEGIN
    first := TRUE;
    count := pTyp.argN;
    Lv.InitCharOpenSeq(fmArray, 4);
    Lv.AppendCharOpen(fmArray, lPar);
    IF (pIdn # NIL) & (pIdn.lxDepth > 0) THEN
      Lv.AppendCharOpen(fmArray, xhrMk); first := FALSE;
    END;
    FOR index := 0 TO pTyp.formals.tide-1 DO
      IF ~first THEN Lv.AppendCharOpen(fmArray, cmma) END;
      parId := pTyp.formals.a[index];
      parId.varOrd := count; INC(count);
      AppendTypeName(fmArray, parId.type, os);
      IF Mu.takeAdrs(parId) THEN 
        parId.boxOrd := parId.parMod;
        Lv.AppendCharOpen(fmArray, rfMk);
        IF Id.uplevA IN parId.locAtt THEN 
          parId.boxOrd := Sy.val;
          ASSERT(Id.cpVarP IN parId.locAtt);
        END;
      END; (* just mark *)
      first := FALSE;
    END;
    Lv.AppendCharOpen(fmArray, rPar);
    pTyp.xName := Lv.arrayCat(fmArray);
   (*
    *   The current info.lNum (before the locals
    *   have been added) is the argsize.
    *)
    pTyp.argN := count;
    IF pTyp.retType # NIL THEN pTyp.retN := 1 END;
  END NumberParams;
 
(* ============================================================ *)
(*                    Private Methods                           *)
(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CatChar(chr : CHAR),NEW;
  BEGIN
    GPBinFiles.WriteByte(os.file, ORD(chr) MOD 256);
  END CatChar;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CatStr(IN str : ARRAY OF CHAR),NEW;
  BEGIN
    GPBinFiles.WriteNBytes(os.file, UBytesOf(str), LEN(str$));
  END CatStr;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CatInt(val : INTEGER),NEW;
    VAR arr : ARRAY 16 OF CHAR;
  BEGIN
    GPText.IntToStr(val, arr); os.CatStr(arr);
  END CatInt;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CatLong(val : LONGINT),NEW;
    VAR arr : ARRAY 32 OF CHAR;
  BEGIN
    GPText.LongToStr(val, arr); os.CatStr(arr);
  END CatLong;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CatEOL(),NEW;
  BEGIN
    os.CatStr(RTS.eol);
  END CatEOL;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)WriteHex(int : INTEGER),NEW;
    VAR ord : INTEGER;
  BEGIN
    IF int <= 9 THEN ord := ORD('0') + int ELSE ord := (ORD('A')-10)+int END;
    os.CatChar(CHR(ord));
  END WriteHex;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)WriteHexByte(int : INTEGER),NEW;
  BEGIN
    os.WriteHex(int DIV 16);
    os.WriteHex(int MOD 16);
    os.CatChar(' ');
  END WriteHexByte;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Tstring(IN str : ARRAY OF CHAR),NEW;
   (* TAB, then string *)
  BEGIN
    os.CatChar(ASCII.HT); os.CatStr(str);
  END Tstring;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Bstring(IN str : ARRAY OF CHAR),NEW;
   (* BLANK, then string *)
  BEGIN
    os.CatChar(" "); os.CatStr(str);
  END Bstring;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Tint(int : INTEGER),NEW;
   (* TAB, then int *)
  BEGIN
    os.CatChar(ASCII.HT); os.CatInt(int);
  END Tint;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Tlong(long : LONGINT),NEW;
   (* TAB, then long *)
  BEGIN
    os.CatChar(ASCII.HT); os.CatLong(long);
  END Tlong;

(* ============================================================ *)
  
  PROCEDURE (os : IlasmFile)QuoteStr(IN str : ARRAY OF CHAR),NEW;
   (* ------------------------ *)
    PROCEDURE EmitQuotedString(os : IlasmFile; IN str : ARRAY OF CHAR);
      VAR chr : CHAR;
          idx : INTEGER;
          ord : INTEGER;
    BEGIN
      os.CatChar('"');
      FOR idx := 0 TO LEN(str) - 2 DO
        chr := str[idx];
        CASE chr OF
        | "\",'"' : os.CatChar("\");
                    os.CatChar(chr);
        | 9X      : os.CatChar("\");
                    os.CatChar("t");
        | 0AX     : os.CatChar("\");
                    os.CatChar("n");
        ELSE
          IF chr > 07EX THEN
            ord := ORD(chr);
            os.CatChar('\');
            os.CatChar(CHR(ord DIV 64 + ORD('0')));
            os.CatChar(CHR(ord MOD 64 DIV 8 + ORD('0')));
            os.CatChar(CHR(ord MOD 8 + ORD('0')));
          ELSE
            os.CatChar(chr);
          END
        END;
      END;
      os.CatChar('"');
    END EmitQuotedString;
   (* ------------------------ *)
    PROCEDURE EmitByteArray(os : IlasmFile; IN str : ARRAY OF CHAR);
      VAR idx : INTEGER;
          ord : INTEGER;
    BEGIN
      os.CatStr("bytearray (");
      FOR idx := 0 TO LEN(str) - 2 DO
        ord := ORD(str[idx]);
        os.WriteHexByte(ord MOD 256);
        os.WriteHexByte(ord DIV 256);
      END;
      os.CatStr(")");
    END EmitByteArray;
   (* ------------------------ *)
    PROCEDURE NotASCIIZ(IN str : ARRAY OF CHAR) : BOOLEAN;
      VAR idx : INTEGER;
          ord : INTEGER;
    BEGIN
      FOR idx := 0 TO LEN(str) - 2 DO
        ord := ORD(str[idx]);
        IF (ord = 0) OR (ord > 0FFH) THEN RETURN TRUE END;
      END;
      RETURN FALSE;
    END NotASCIIZ;
   (* ------------------------ *)
  BEGIN
    IF NotASCIIZ(str) THEN
      EmitByteArray(os, str);
    ELSE
      EmitQuotedString(os, str);
    END;
  END QuoteStr;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Prefix(code : INTEGER),NEW;
  BEGIN
    os.CatChar(ASCII.HT); os.CatStr(Asm.op[code]);
  END Prefix;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)IAdjust*(delta : INTEGER),NEW; 
  BEGIN
    os.Adjust(delta);
    IF Cs.verbose THEN
      os.CatStr(" // ");
      os.CatInt(os.proc.dNum);
      os.CatChar(",");
      os.CatInt(os.proc.dMax);
    END;
    os.CatEOL();
  END IAdjust;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Suffix(code : INTEGER),NEW;
  BEGIN
    os.Adjust(Asm.dl[code]);
    IF Cs.verbose THEN
      os.CatStr(" // ");
      os.CatInt(os.proc.dNum);
      os.CatChar(",");
      os.CatInt(os.proc.dMax);
    END;
    os.CatEOL();
  END Suffix;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Access*(acc : SET),NEW;
    VAR att : INTEGER;
  BEGIN
    os.CatChar(" ");
    FOR att := 0 TO Asm.maxAttIndex DO
      IF att IN acc THEN
        os.CatStr(Asm.access[att]);
        os.CatChar(' ');
      END;
    END;
  END Access;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)RefLab*(l : Mu.Label),NEW;
  BEGIN
    os.CatChar(ASCII.HT);
    os.CatChar("l");
    os.CatChar("b");
    os.CatInt(l(ILabel).labl);
  END RefLab;

(* ------------------------------------ *)

  PROCEDURE (os : IlasmFile)LstLab*(l : Mu.Label);
  BEGIN
    os.CatChar(",");
    os.CatEOL();
    os.Tstring("       lb");
    os.CatInt(l(ILabel).labl);
  END LstLab;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)SwitchHead*(n : INTEGER);
   (* n is table length, ignored here *)
  BEGIN
    os.Prefix(Asm.opc_switch);
    os.Tstring("( // dispatch table: ");
    os.CatInt(n);
  END SwitchHead;

(* ------------------------------------ *)

  PROCEDURE (os : IlasmFile)SwitchTail*();
  BEGIN
    os.CatChar(")");
    os.IAdjust(-1);
  END SwitchTail;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Idnt(idD : Sy.Idnt),NEW;
  BEGIN
    os.CatChar("'");
    os.CatStr(Sy.getName.ChPtr(idD));
    os.CatChar("'");
  END Idnt;

(* ------------------------------------ *)

  PROCEDURE (os : IlasmFile)SQuote(str : Lv.CharOpen),NEW;
  BEGIN
    os.CatChar("'");
    os.CatStr(str);
    os.CatChar("'");
  END SQuote;

(* ------------------------------------ *)

  PROCEDURE (os : IlasmFile)TsQuote(str : Lv.CharOpen),NEW;
  BEGIN
    os.CatChar(ASCII.HT); os.SQuote(str);
  END TsQuote;

(* ------------------------------------ *)

  PROCEDURE (os : IlasmFile)Tidnt(idD : Sy.Idnt),NEW;
  BEGIN
    os.CatChar(ASCII.HT); os.Idnt(idD);
  END Tidnt;

(* ------------------------------------ *)

  PROCEDURE (os : IlasmFile)Bidnt(idD : Sy.Idnt),NEW;
  BEGIN
    os.CatChar(" "); os.Idnt(idD);
  END Bidnt;

(* ------------------------------------ *)

  PROCEDURE (os : IlasmFile)PIdnt(idD : Id.Procs),NEW;
   (* Write out procedure identifier name *)
    VAR fullNm : Lv.CharOpen;
  BEGIN
    IF idD.scopeNm = NIL THEN Mu.MkProcName(idD, os) END;
    os.CatChar(" ");
    WITH idD : Id.PrcId DO 
        IF idD.bndType = NIL THEN
          fullNm := Mu.cat2(idD.scopeNm, idD.clsNm);
        ELSE (* beware of special cases for object and string! *)
          fullNm := idD.bndType(Ty.Record).scopeNm;
        END;
    | idD : Id.MthId DO 
        IF Id.boxRcv IN idD.mthAtt THEN
          fullNm := Mu.cat3(idD.scopeNm, boxedObj, idD.bndType.xName);
        ELSE (* beware of special cases for object and string! *)
          fullNm := idD.bndType(Ty.Record).scopeNm;
        END;
    END;
    os.CatStr(fullNm);
    os.CatStr(cln2);
    os.SQuote(idD.prcNm);
  END PIdnt;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)TypeTag(typ : Sy.Type),NEW;
  BEGIN
    IF typ.xName = NIL THEN Mu.MkTypeName(typ,os) END;
    WITH typ : Ty.Base DO
        os.CatStr(typ.xName);
    | typ : Ty.Vector DO
        os.CatStr(clss);
        os.CatStr(typ.xName);
    | typ : Ty.Array DO
        os.TypeTag(typ.elemTp);
        os.CatStr(brks);
    | typ : Ty.Record DO
        IF ~(Sy.clsTp IN typ.xAttr) THEN os.CatStr(vals) END;
        IF ~(Sy.spshl IN typ.xAttr) THEN os.CatStr(clss) END;
        os.CatStr(typ.scopeNm);
    | typ : Ty.Procedure DO (* and also Event! *)
        os.CatStr(clss);
        os.CatStr(typ.tName);
    | typ : Ty.Pointer DO
        IF Mu.isValRecord(typ.boundTp) THEN
         (* 
          *  This is a pointer to a value class, which has a
          *  runtime representation as a boxed-class reference.
          *)
          os.CatStr(clss);
          os.CatStr(typ.xName);
        ELSE
          os.TypeTag(typ.boundTp);
        END;
    | typ : Ty.Opaque DO
        os.CatStr(clss);
        os.CatStr(typ.xName);
    | typ : Ty.Enum DO
        os.CatStr(vals);
        os.CatStr(clss);
        os.CatStr(typ.xName);
    END;
  END TypeTag;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Translate(IN str : ARRAY OF CHAR),NEW;
    VAR ix : INTEGER;
        ch : CHAR;
  BEGIN
    ix := 0; ch := str[0];
    WHILE ch # 0X DO
      IF ch = '$' THEN
        INC(ix); ch := str[ix];
        IF    ch = "s" THEN os.TypeName(Cs.ntvStr);
        ELSIF ch = "o" THEN os.TypeName(Cs.ntvObj);
        ELSIF ch = "S" THEN os.TypeTag(Cs.ntvStr);
        ELSIF ch = "O" THEN os.TypeTag(Cs.ntvObj);
        END;
      ELSE
        os.CatChar(ch);
      END;
      INC(ix); ch := str[ix];
    END;
  END Translate;

  PROCEDURE (os : IlasmFile)TTranslate(IN str : ARRAY OF CHAR),NEW;
  BEGIN
    os.CatChar(ASCII.HT);
    os.Translate(str);
  END TTranslate;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)TtypeTag(typ : Sy.Type),NEW;
  BEGIN
    os.CatChar(ASCII.HT);
    os.TypeTag(typ);
  END TtypeTag;

(* ------------------------------------ *)

  PROCEDURE (os : IlasmFile)TtypeNam(typ : Sy.Type),NEW;
  BEGIN
    os.Tstring(Mu.typeName(typ, os));
  END TtypeNam;

(* ------------------------------------ *)

  PROCEDURE (os : IlasmFile)TypeName(typ : Sy.Type),NEW;
  BEGIN
    os.CatStr(Mu.typeName(typ, os));
  END TypeName;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)RetType(typ : Ty.Procedure; pId : Id.Procs),NEW;
  BEGIN
    IF typ.retType = NIL THEN
      os.CatStr(vStr);
    ELSIF (pId # NIL) & (pId IS Id.MthId) & 
        (Id.covar IN pId(Id.MthId).mthAtt) THEN
     (*
      *  This is a method with a covariant return type. We must
      *  erase the declared type, substituting the non-covariant
      *  upper-bound. Calls will cast the result to the real type.
      *)
      os.TypeTag(pId.retTypBound());
    ELSE
      os.TypeTag(typ.retType);
    END;
  END RetType;

(* ============================================================ *)
(*                    Exported Methods                          *)
(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Blank*();
  BEGIN
    os.CatEOL();
  END Blank;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Separator(c : CHAR; i : INTEGER),NEW;
  BEGIN
    os.CatChar(c);
    os.CatEOL();
    WHILE i > 0 DO os.CatChar(ASCII.HT); DEC(i) END;
  END Separator;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)OpenBrace*(i : INTEGER);
  BEGIN
    WHILE i > 0 DO os.CatChar(" "); DEC(i) END;
    os.CatChar("{");
    os.CatEOL();
  END OpenBrace;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CloseBrace*(i : INTEGER);
  BEGIN
    WHILE i > 0 DO os.CatChar(" "); DEC(i) END;
    os.CatChar("}");
    os.CatEOL();
  END CloseBrace;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Directive*(dir : INTEGER),NEW;
  BEGIN
    os.CatStr(Asm.dirStr[dir]);
    os.CatEOL();
  END Directive;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)DirectiveS*(dir : INTEGER;
                             IN str : ARRAY OF CHAR),NEW;
  BEGIN
    os.CatStr(Asm.dirStr[dir]);
    os.Bstring(str);
    os.CatEOL();
  END DirectiveS;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)DirectiveIS*(dir : INTEGER;
                                  att : SET;
                               IN str : ARRAY OF CHAR),NEW;
  BEGIN
    os.CatStr(Asm.dirStr[dir]);
    os.Access(att);
    os.CatStr(str);
    os.CatEOL();
  END DirectiveIS;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)DirectiveISS*(dir : INTEGER;
                                    att : SET;
                                 IN s1  : ARRAY OF CHAR;
                                 IN s2  : ARRAY OF CHAR),NEW;
  BEGIN
    os.CatStr(Asm.dirStr[dir]);
    os.Access(att);
    os.CatStr(s1);
    os.CatStr(s2);
    IF dir = Asm.dot_method THEN os.Bstring(managedStr) END;
    os.CatEOL();
  END DirectiveISS;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Finish*();
  BEGIN
    GPBinFiles.CloseFile(os.file);
  END Finish;
 
(* ------------------------------------------------- *)

  PROCEDURE (os : IlasmFile)MkBodyClass*(mod : Id.BlkId);
  BEGIN
    os.clsN := mod.clsNm;
    os.DirectiveIS(Asm.dot_class, Asm.modAttr, os.clsN);
  END MkBodyClass;

(* ------------------------------------------------- *)

  PROCEDURE (os : IlasmFile)ClassHead*(attSet : SET; 
                                       thisRc : Ty.Record;
                                       superT : Ty.Record);
  BEGIN
    os.clsN := thisRc.xName;
    os.DirectiveIS(Asm.dot_class, attSet, os.clsN);
    IF superT # NIL THEN
      IF superT.xName = NIL THEN Mu.MkRecName(superT, os) END;
      os.DirectiveS(Asm.dot_super, superT.scopeNm);
    END;
  END ClassHead;

(* ------------------------------------------------- *)

  PROCEDURE (os : IlasmFile)StartNamespace*(name : Lv.CharOpen);
  BEGIN
    os.DirectiveS(Asm.dot_namespace, name);
  END StartNamespace;

(* ------------------------------------------------- *)

  PROCEDURE (os : IlasmFile)AsmDef*(IN pkNm : ARRAY OF CHAR);
  BEGIN
    os.DirectiveIS(Asm.dot_assembly, {}, "'" + pkNm + "' {}");
  END AsmDef;

(* ------------------------------------------------- *)

  PROCEDURE (os : IlasmFile)RefRTS*();
  BEGIN
    os.DirectiveIS(Asm.dot_assembly, Asm.att_extern, "RTS {}");
    IF Cs.netRel = Cs.netV2_0 THEN
      os.DirectiveIS(Asm.dot_assembly, Asm.att_extern, "mscorlib {}");
    END;
  END RefRTS;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)SignatureDecl(prcT : Ty.Procedure),NEW;
    VAR indx : INTEGER;
        parD : Id.ParId;
        long : BOOLEAN;
        nest : BOOLEAN;
        frst : BOOLEAN;
  BEGIN
    frst := TRUE;
    indx := prcT.formals.tide;
    nest := (prcT.idnt IS Id.Procs) & (prcT.idnt(Id.Procs).lxDepth > 0);
    long := (indx > 1) OR (nest & (indx > 0));

    os.CatChar("(");
    IF long THEN os.Separator(' ', 2) END;
    IF nest THEN os.CatStr(xhrMk); frst := FALSE END;
    FOR indx := 0 TO prcT.formals.tide-1 DO
      parD := prcT.formals.a[indx];
      IF long THEN
        IF ~frst THEN os.Separator(',', 2) END;
        IF parD.boxOrd = Sy.out THEN os.CatStr(ouMk) END;
        os.TypeTag(parD.type);
        IF (parD.boxOrd # Sy.val) OR
           (Id.cpVarP IN parD.locAtt) THEN os.CatStr(rfMk) END;
        os.Tidnt(parD); 
      ELSE
        IF ~frst THEN os.CatStr(cmma) END;
        IF parD.boxOrd = Sy.out THEN os.CatStr(ouMk) END;
        os.TypeTag(parD.type);
        IF (parD.boxOrd # Sy.val) OR
           (Id.cpVarP IN parD.locAtt) THEN os.CatStr(rfMk) END;
        os.Bidnt(parD);
      END;
      frst := FALSE;
    END;
    os.CatChar(")");
  END SignatureDecl;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)MethodDecl*(attr : SET; proc : Id.Procs);
    VAR prcT : Ty.Procedure;
  BEGIN
    prcT := proc.type(Ty.Procedure);
    os.CatStr(Asm.dirStr[Asm.dot_method]);
    os.Access(attr);
    os.RetType(prcT, proc);
    os.CatChar(" ");
    os.CatChar("'");
    os.CatStr(proc.prcNm);
    os.CatChar("'");
    os.SignatureDecl(prcT);
    os.Bstring(managedStr);
    os.CatEOL();
    IF Asm.att_abstract * attr # {} THEN 
      os.Tstring(brsz); os.CatEOL();
    END;
  END MethodDecl;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : IlasmFile)CheckNestedClass*(typ : Ty.Record;
                                              scp : Sy.Scope;
                                              str : Lv.CharOpen);
    VAR i, len: INTEGER;
  BEGIN
   (* 
    *  scan str with all occurences of 
    *  '$' replaced by '/', except at index 0  
    *)
    len := LEN(str); 
    FOR i := 1 TO len-1 DO
      IF str[i] = '$' THEN str[i] := '/' END;
    END; (* FOR *)
  END CheckNestedClass;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : IlasmFile)ExternList*();
    VAR idx : INTEGER;
        blk : Id.BlkId;
   (* ----------------------------------------- *)
    PROCEDURE Assembly(fl : IlasmFile; bk : Id.BlkId);
      VAR ix : INTEGER;
          ch : CHAR;
    BEGIN
      IF Sy.isFn IN bk.xAttr THEN
        IF bk.scopeNm[0] # '[' THEN 
                          RTS.Throw("bad extern name "+bk.scopeNm^) END;
        ix := 1;
        ch := bk.scopeNm[ix];
        WHILE (ch # 0X) & (ch # ']') DO
          fl.CatChar(ch);
          INC(ix);
          ch := bk.scopeNm[ix];
        END;
      ELSE
        fl.CatStr(bk.xName);
      END;
    END Assembly;
   (* ----------------------------------------- *)
    PROCEDURE WriteHex(fl : IlasmFile; int : INTEGER);
      VAR ord : INTEGER;
    BEGIN
      IF int <= 9 THEN ord := ORD('0') + int ELSE ord := (ORD('A')-10)+int END;
      fl.CatChar(CHR(ord));
    END WriteHex;
   (* ----------------------------------------- *)
    PROCEDURE WriteHexByte(fl : IlasmFile; int : INTEGER);
    BEGIN
      WriteHex(fl, int DIV 16);
      WriteHex(fl, int MOD 16);
      fl.CatChar(' ');
    END WriteHexByte;
   (* ----------------------------------------- *)
    PROCEDURE WriteBytes(fl : IlasmFile; int : INTEGER);
    BEGIN
      WriteHexByte(fl, int DIV 1000000H MOD 100H);
      WriteHexByte(fl, int DIV 10000H MOD 100H);
      WriteHexByte(fl, int DIV 100H MOD 100H);
      WriteHexByte(fl, int      MOD 100H);
    END WriteBytes;
   (* ----------------------------------------- *)
    PROCEDURE WriteVersionName(os : IlasmFile; IN nam : ARRAY OF INTEGER);
    BEGIN
      os.CatStr(" {"); os.CatEOL();

      IF (nam[4] # 0) OR (nam[5] # 0) THEN
        os.CatStr("    .publickeytoken = ("); 
(*
 *      IF Cs.netRel = Cs.beta2 THEN
 *        os.CatStr("    .publickeytoken = ("); 
 *      ELSE
 *        os.CatStr("    .originator = ("); 
 *      END;
 *)
        WriteBytes(os, nam[4]);
        WriteBytes(os, nam[5]);
        os.CatChar(")"); 
        os.CatEOL(); 
      END;

      os.CatStr("    .ver "); 
      os.CatInt(nam[0]); 
      os.CatChar(":"); 
      os.CatInt(nam[1]); 
      os.CatChar(":"); 
      os.CatInt(nam[2]); 
      os.CatChar(":"); 
      os.CatInt(nam[3]); 
      os.CatEOL(); 
      os.CatChar("}"); 
    END WriteVersionName;
   (* ----------------------------------------- *)
  BEGIN
   (*
    *   It is empirically established that all the
    *   .assembly extern declarations must come at
    *   the beginning of the ILASM file, at once.
    *)
    FOR idx := 0 TO Cs.impSeq.tide-1 DO
      blk := Cs.impSeq.a[idx](Id.BlkId);
      IF ~(Sy.rtsMd IN blk.xAttr) & (Sy.need IN blk.xAttr) THEN
        Mu.MkBlkName(blk);
        os.CatStr(Asm.dirStr[Asm.dot_assembly]);
        os.Access(Asm.att_extern);
        Assembly(os, blk);
        IF blk.verNm = NIL THEN
          os.CatStr(" {}");
        ELSE
          WriteVersionName(os, blk.verNm);
        END;
        os.CatEOL();
      END;
    END;
  END ExternList;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)Comment*(IN s : ARRAY OF CHAR);
  BEGIN
    os.CatStr("// ");
    os.CatStr(s);
    os.CatEOL();
  END Comment;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)CommentT*(IN s : ARRAY OF CHAR);
  BEGIN
    os.CatStr("//        ");
    os.CatStr(s);
    os.CatEOL();
  END CommentT;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)DefLab*(l : Mu.Label);
  BEGIN
    os.CatChar("l");
    os.CatChar("b");
    os.CatInt(l(ILabel).labl);
    os.CatChar(":");
    os.CatEOL();
  END DefLab;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)DefLabC*(l : Mu.Label; IN c : ARRAY OF CHAR);
  BEGIN
    os.CatChar("l");
    os.CatChar("b");
    os.CatInt(l(ILabel).labl);
    os.CatChar(":");
    os.CatChar(ASCII.HT);
    os.Comment(c);
  END DefLabC;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Code*(code : INTEGER);
  BEGIN
    os.Prefix(code);
    os.Suffix(code);
  END Code;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)CodeI*(code,int : INTEGER);
  BEGIN
    os.Prefix(code);
    os.Tint(int);
    os.Suffix(code);
  END CodeI;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)CodeT*(code : INTEGER; type : Sy.Type);
  BEGIN
    os.Prefix(code);
    os.TtypeTag(type);
    os.Suffix(code);
  END CodeT;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)CodeTn*(code : INTEGER; type : Sy.Type);
  BEGIN
    os.Prefix(code);
    os.TtypeNam(type);
    os.Suffix(code);
  END CodeTn;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)CodeL*(code : INTEGER; long : LONGINT);
  BEGIN
    os.Prefix(code);
    os.Tlong(long);
    os.Suffix(code);
  END CodeL;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)CodeR*(code : INTEGER; real : REAL);
    VAR nam : ARRAY 64 OF CHAR;
  BEGIN
    os.Prefix(code);
    RTS.RealToStrInvar(real, nam);
    os.Tstring(nam$);
    os.Suffix(code);
  END CodeR;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)CodeLb*(code : INTEGER; i2 : Mu.Label);
  BEGIN
    os.Prefix(code);
    os.RefLab(i2);
    os.Suffix(code);
  END CodeLb;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)CodeStr(code : INTEGER; 
                                 IN str  : ARRAY OF CHAR),NEW;
  BEGIN
    os.Prefix(code);
    os.TTranslate(str);
    os.Suffix(code);
  END CodeStr;

  PROCEDURE (os : IlasmFile)CodeS*(code : INTEGER; str : INTEGER);
  BEGIN
    os.CodeStr(code, rts[str]);
  END CodeS;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)StaticCall*(s : INTEGER; d : INTEGER);
  BEGIN
    os.Prefix(Asm.opc_call);
    os.TTranslate(rts[s]);
    os.IAdjust(d);
  END StaticCall;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)Try*();
    VAR retT : Sy.Type;
  BEGIN
    retT := os.proc.prId.type.returnType();
    os.Directive(Asm.dot_try);
    os.OpenBrace(4);
    os.proc.exLb := os.newLabel();
    IF retT # NIL THEN os.proc.rtLc := os.proc.newLocal(retT) END;
  END Try;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)Catch*(proc : Id.Procs);
  BEGIN
    os.CloseBrace(4);
    os.CatStr(catchStr);
    os.CatEOL();
    os.OpenBrace(4);
    os.Adjust(1);	(* allow for incoming exception reference *)
    os.StoreLocal(proc.except.varOrd);
  END Catch;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)CloseCatch*();
  BEGIN
    os.CloseBrace(4);
  END CloseCatch;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)CopyCall*(typ : Ty.Record);
  BEGIN
    os.Prefix(Asm.opc_call);
    os.Tstring(initPrefix);
    os.Bstring(typ.scopeNm);
    os.CatStr("::__copy__(");
    os.TypeTag(typ);
    os.CatChar(")");
    os.IAdjust(-2);
  END CopyCall;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)PushStr*(IN str : ARRAY OF CHAR);
  (* Use target quoting conventions for the literal string *)
  BEGIN
    os.Prefix(Asm.opc_ldstr);
    os.CatChar(ASCII.HT);
    os.QuoteStr(str);
    os.IAdjust(1);
  END PushStr;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CallIT*(code : INTEGER; 
                              proc : Id.Procs; 
                              type : Ty.Procedure);
  BEGIN
    os.Prefix(code);
    os.CatChar(ASCII.HT);
   (*
    *   For static calls to procedures we want
    *            call  <ret-type> <idnt>(<signature>)
    *   for static calls to final or super methods, we need
    *            call  instance <ret-type> <idnt>(<signature>)
    *   for calls to type-bound methods that are not final
    *            callvirt  instance <ret-type> <idnt>(<signature>)
    *)
    IF proc IS Id.MthId THEN os.CatStr("instance ") END;
    os.RetType(type, proc);
    os.PIdnt(proc);
    os.CatStr(type.xName);
    os.IAdjust(type.retN - type.argN);        
  END CallIT;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CallCT*(proc : Id.Procs; 
                                    type : Ty.Procedure);
  BEGIN
    os.Prefix(Asm.opc_newobj);
    os.Tstring(initPrefix);
    os.PIdnt(proc);
    os.CatStr(type.xName);
    os.IAdjust(type.retN - type.argN);        
  END CallCT;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CallDelegate*(typ : Ty.Procedure);
  BEGIN
    os.Prefix(Asm.opc_callvirt);
    os.Tstring("instance ");
    os.RetType(typ, NIL);
    os.Bstring(typ.tName);
    os.CatStr("::Invoke");
    os.CatStr(typ.xName);
    os.IAdjust(typ.retN - typ.argN);        
  END CallDelegate;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)PutGetS*(code : INTEGER;
                                     blk  : Id.BlkId;
                                     fld  : Id.VarId);
    VAR size : INTEGER;
  (* Emit putstatic and getstatic for static field *)
  BEGIN
    os.Prefix(code);
    os.TtypeTag(fld.type);
    os.CatChar(ASCII.HT);
    IF blk.xName = NIL THEN Mu.MkBlkName(blk) END;
    IF fld.varNm = NIL THEN Mu.MkVarName(fld, os) END;
    os.CatStr(blk.scopeNm);
    os.CatStr(fld.clsNm);
    os.CatStr(cln2);
    os.SQuote(fld.varNm);
    os.Suffix(code);
  END PutGetS;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)PutGetFld(code : INTEGER;
                                      fTyp : Sy.Type;
                                      rTyp : Sy.Type;
                                      name : Lv.CharOpen),NEW;
  BEGIN
    os.Prefix(code);
    os.TtypeTag(fTyp);
    os.TtypeNam(rTyp);
    os.CatStr(cln2);
    os.SQuote(name);
    os.Suffix(code);
  END PutGetFld;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)GetValObj*(code : INTEGER; 
                                       ptrT : Ty.Pointer);
  BEGIN
    os.PutGetFld(code, ptrT.boundTp, ptrT, vFld);
  END GetValObj;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)PutGetXhr*(code : INTEGER;
                                       proc : Id.Procs;
                                       locD : Id.LocId);
    VAR name : Lv.CharOpen;
  BEGIN
    name := Sy.getName.ChPtr(locD);
    os.PutGetFld(code, locD.type, proc.xhrType, name);
  END PutGetXhr;

(* -------------------------------------------- *)

  PROCEDURE (os : IlasmFile)PutGetF*(code : INTEGER;
                                     fld  : Id.FldId);
    VAR recT : Ty.Record;
  (* Emit putfield and getfield for record field *)
  BEGIN
    recT := fld.recTyp(Ty.Record);
    os.Prefix(code);
    os.TtypeTag(fld.type);
    os.CatChar(ASCII.HT);
    IF fld.fldNm = NIL THEN fld.fldNm := Sy.getName.ChPtr(fld) END;
   (*
    *  Note the difference here. JVM needs the
    *  static type of the variable, VOS wants
    *  the name of the record with the field.
    *)
    os.CatStr(recT.scopeNm);
    os.CatStr(cln2);
    os.SQuote(fld.fldNm);
    os.Suffix(code);
  END PutGetF;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE (os : IlasmFile)MkNewRecord*(typ : Ty.Record);
    VAR name : Lv.CharOpen;
  BEGIN
   (*
    *  We need "newobj instance void <name>::.ctor()"
    *)
    IF typ.xName = NIL THEN Mu.MkRecName(typ, os) END;
    IF Sy.clsTp IN typ.xAttr THEN
      name := typ.scopeNm;
    ELSE
      name := Mu.boxedName(typ, os);
    END;
    os.Prefix(Asm.opc_newobj);
    os.Tstring(initPrefix);
    os.Bstring(name);
    os.CatStr(cln2);
    os.CatStr(initSuffix);
    os.IAdjust(1);
  END MkNewRecord;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE (os : IlasmFile)MkNewProcVal*(p : Sy.Idnt; t : Sy.Type);
    VAR name : Lv.CharOpen;
        proc : Id.Procs;
        type : Ty.Procedure;
        code : INTEGER;
  BEGIN
    proc := p(Id.Procs);
    type := t(Ty.Procedure);
   (*
    *  We need "ldftn [instance] <retType> <procName>
    *)
    IF type.xName = NIL THEN Mu.MkPTypeName(type, os) END;
    WITH p : Id.MthId DO
      IF p.bndType.isInterfaceType() THEN
        code := Asm.opc_ldvirtftn;
      ELSIF p.mthAtt * Id.mask = Id.final THEN
        code := Asm.opc_ldftn;
      ELSE
        code := Asm.opc_ldvirtftn;
      END;
    ELSE
      code := Asm.opc_ldftn;
    END;
   (*
    *   If this will be a virtual method call, then we
    *   must duplicate the receiver, since the call of
    *   ldvirtftn uses up one copy.
    *)
    IF code = Asm.opc_ldvirtftn THEN os.Code(Asm.opc_dup) END;
    os.Prefix(code);
    os.CatChar(ASCII.HT);
    IF p IS Id.MthId THEN os.CatStr("instance ") END;
    os.RetType(type, NIL);
    os.PIdnt(proc);
    os.CatStr(type.xName);
    os.IAdjust(1);
   (*
    *  We need "newobj instance void <name>::.ctor(...)"
    *)
    os.Prefix(Asm.opc_newobj);
    os.Tstring(initPrefix);
    os.Bstring(type.tName);
    os.CatStr(cln2);
    os.Translate(pVarSuffix);
    os.IAdjust(-2);
  END MkNewProcVal;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)InitHead*(typ : Ty.Record;
                                      prc : Id.PrcId);
    VAR pTp : Ty.Procedure;
  BEGIN
   (*
    *  Get the procedure type, if any ...
    *)
    IF prc # NIL THEN 
      pTp := prc.type(Ty.Procedure);
    ELSE 
      pTp := NIL;
    END;

    os.CatStr(Asm.dirStr[Asm.dot_method]);
    os.Bstring(specialStr);
    os.Bstring(initPrefix);
    IF prc = NIL THEN
      os.Bstring(initSuffix);
    ELSE
      os.Bstring(initString);
      os.SignatureDecl(pTp);
    END;
    os.CatStr(managedStr);
    os.CatEOL();
    os.CatStr("    {");
    os.CatEOL();
   (*
    *   Now we begin to initialize the supertype;
    *)
    os.CommentT("Call supertype constructor");
    os.Code(Asm.opc_ldarg_0);
  END InitHead;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CallSuper*(typ : Ty.Record;
                                       prc : Id.PrcId);
    VAR pTp : Ty.Procedure;
        pNm : INTEGER;
  BEGIN
   (*
    *  Get the procedure type, if any ...
    *)
    IF prc # NIL THEN 
      pTp := prc.type(Ty.Procedure);
      pNm := pTp.formals.tide;
    ELSE 
      pTp := NIL;
      pNm := 0;
    END;
    os.Prefix(Asm.opc_call);
    IF  (typ # NIL) & 
        (typ.baseTp # NIL) & 
        (typ.baseTp # Bi.anyRec) THEN
      os.Tstring(initPrefix);
      os.Bstring(typ.baseTp(Ty.Record).scopeNm);
      os.CatStr(cln2);
      IF pTp # NIL THEN
        os.Bstring(initString);
        os.CatStr(pTp.xName);
      ELSE
        os.CatStr(initSuffix);
      END;
    ELSE
      os.TTranslate(objectInit);
    END;
    os.IAdjust(-(pNm+1));
  END CallSuper;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)InitTail*(typ : Ty.Record);
  BEGIN
    os.Locals();
    os.CatStr("    } // end of method '");
    IF typ # NIL THEN 
      os.CatStr(typ.xName);
    END;
    os.CatStr("::.ctor'");
    os.CatEOL();
  END InitTail;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CopyHead*(typ : Ty.Record);
  BEGIN
    os.Comment("standard record copy method");
    os.CatStr(Asm.dirStr[Asm.dot_method]);
    os.Tstring(copyHead);
(* FIX FOR BOXED CLASS COPY *)
    IF ~(Sy.clsTp IN typ.xAttr) THEN os.CatStr(vals) END;
    os.CatStr(clss);
    os.CatStr(typ.scopeNm);
    IF ~(Sy.clsTp IN typ.xAttr) THEN os.CatStr(rfMk) END;
    os.CatStr(") ");
    os.CatStr(managedStr);
    os.CatEOL();
    os.CatStr("    {");
    os.CatEOL();
  END CopyHead;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CopyTail*();
  BEGIN
    os.Locals();
    os.CatStr("    } // end of __copy__ method '");
    os.CatEOL();
  END CopyTail;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)MarkInterfaces*(IN seq : Sy.TypeSeq);
    VAR index  : INTEGER;
        tideX  : INTEGER;
        implT  : Ty.Record;
  BEGIN
    tideX := seq.tide-1;
    ASSERT(tideX >= 0);
    os.CatStr(Asm.dirStr[Asm.dot_implements]); 
    FOR index := 0 TO tideX DO
      implT := seq.a[index].boundRecTp()(Ty.Record);
      IF implT.xName = NIL THEN Mu.MkRecName(implT, os) END;
      os.Bstring(implT.scopeNm);
      IF index < tideX THEN os.CatChar(",") END;
      os.CatEOL();
      IF index < tideX THEN os.CatStr("                 ") END;
    END;
  END MarkInterfaces;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)MainHead*(xAtt : SET);
  BEGIN
    os.Comment("Main entry point");
    os.CatStr(Asm.dirStr[Asm.dot_method]);
    IF Sy.wMain IN xAtt THEN
      os.TTranslate(winString);
    ELSE
      os.TTranslate(mainString);
    END;
    os.CatEOL();
    os.OpenBrace(4);
    os.Directive(Asm.dot_entrypoint);
    IF Cs.debug & ~(Sy.sta IN xAtt) THEN 
      os.LineSpan(Scn.mkSpanT(Cs.thisMod.begTok));
    END;
   (*
    *  Save the command-line arguments to the RTS.
    *)
    os.Code(Asm.opc_ldarg_0);
    os.CodeStr(Asm.opc_stsfld, putArgStr);
  END MainHead;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)SubSys*(xAtt : SET);
  BEGIN
    IF Sy.wMain IN xAtt THEN
      os.TTranslate(subSysStr);
      os.Comment("WinMain entry");
      os.CatEOL();
    ELSIF Sy.cMain IN xAtt THEN
      os.Comment("CPmain entry");
    END;
  END SubSys;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)StartBoxClass*(rec : Ty.Record;
                                   att : SET;
                                   blk : Id.BlkId);
    VAR name : Lv.CharOpen;
        bNam : Lv.CharOpen;
  BEGIN
    os.CatEOL();
    name := Mu.cat2(boxedObj, os.clsN);
    os.DirectiveIS(Asm.dot_class, att, name);
    os.OpenBrace(2);
    os.CatStr(Asm.dirStr[Asm.dot_field]);
    os.Access(Asm.att_public);
    os.TtypeTag(rec);
    os.Bstring(vFld);
    os.CatEOL();
   (*
    *   Emit the no-arg constructor
    *)
    os.CatEOL();
    os.MkNewProcInfo(blk);
    os.InitHead(rec, NIL);
    os.CallSuper(rec, NIL);
    os.Code(Asm.opc_ret);
    os.InitTail(rec);
   (*
    *   Copies of value classes are always done inline.
    *)
  END StartBoxClass;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Tail(IN s : ARRAY OF CHAR),NEW;
  BEGIN
    os.Locals();
    os.CatStr(s);
    os.CatEOL();
  END Tail;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : IlasmFile)MainTail*();
  BEGIN os.Tail("    } // end of method .CPmain") END MainTail;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : IlasmFile)ClinitTail*();
  BEGIN os.Tail("    } // end of .cctor method") END ClinitTail;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : IlasmFile)MethodTail*(id : Id.Procs);
  BEGIN os.Tail("    } // end of method " + id.prcNm^) END MethodTail;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)ClinitHead*();
  BEGIN
    os.CatStr(Asm.dirStr[Asm.dot_method]);
    os.Tstring(specialStr);
    os.CatStr(cctorStr);
    os.CatStr(managedStr);
    os.CatEOL();
    os.CatStr("    {");
    os.CatEOL();
  END ClinitHead;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)EmitField*(id : Id.AbVar; att : SET);
    VAR nm : Lv.CharOpen;
  BEGIN
    WITH id : Id.FldId DO 
         IF id.fldNm = NIL THEN Mu.MkFldName(id, os) END;
         nm := id.fldNm;
    | id : Id.VarId DO
         IF id.varNm = NIL THEN Mu.MkVarName(id, os) END;
         nm := id.varNm;
    END;
    os.CatStr(Asm.dirStr[Asm.dot_field]);
    os.Access(att);
    os.TtypeTag(id.type);
    os.TsQuote(nm);
    os.CatEOL();
  END EmitField;

(* ============================================================ *)
(*           Start of Procedure Variable and Event Stuff        *)
(* ============================================================ *)

  PROCEDURE (os : IlasmFile)EmitEventMethods*(id : Id.AbVar);
    VAR eTp  : Ty.Event;
   (* ------------------------------------------------- *)
    PROCEDURE Head(os : IlasmFile; fn, tn : Lv.CharOpen; add : BOOLEAN);
    BEGIN
      os.CatEOL(); 
      os.CatChar(ASCII.HT);
      IF add THEN os.CatStr(evtAdd) ELSE os.CatStr(evtRem) END; 
      os.CatStr(fn); os.CatStr("(class "); os.CatStr(tn); 
      os.CatStr(") il managed synchronized {"); 
      os.CatEOL(); 
    END Head;
   (* ------------------------------------------------- *)
    PROCEDURE EmitEvtMth(os : IlasmFile; add : BOOLEAN; fld : Id.AbVar);
    BEGIN
      os.MkNewProcInfo(NIL);
      WITH fld : Id.FldId DO
          os.CatStr(".method public specialname instance void");
          Head(os, fld.fldNm, fld.type(Ty.Event).tName, add);
          os.Code(Asm.opc_ldarg_0);
          os.Code(Asm.opc_ldarg_0);
          os.PutGetF(Asm.opc_ldfld, fld);
          os.Code(Asm.opc_ldarg_1);
          os.CallCombine(fld.type, add);
          os.PutGetF(Asm.opc_stfld, fld);
      | fld : Id.VarId DO
          os.CatStr(".method public specialname static void");
          Head(os, fld.varNm, fld.type(Ty.Event).tName, add);
          os.PutGetS(Asm.opc_ldsfld, fld.dfScp(Id.BlkId), fld);
          os.Code(Asm.opc_ldarg_0);
          os.CallCombine(fld.type, add);
          os.PutGetS(Asm.opc_stsfld, fld.dfScp(Id.BlkId), fld);
      END;
      os.Code(Asm.opc_ret);
      os.CloseBrace(4);
      os.CatEOL(); 
    END EmitEvtMth;
   (* ------------------------------------------------- *)
    PROCEDURE Decl(os : IlasmFile; cv, cl, fn, tn : Lv.CharOpen);
    BEGIN
      os.CatStr(".event ");
      os.Tstring(tn);
      os.CatChar(' '); 
      os.SQuote(fn);        (* field name *)
      os.CatEOL(); os.OpenBrace(4);

      os.Tstring(".addon "); os.CatStr(cv); os.CatStr(cl); os.CatStr(cln2);
      os.CatStr(evtAdd); os.CatStr(fn); os.CatStr("(class "); 
      os.CatStr(tn); os.CatChar(')'); 
      os.CatEOL(); 

      os.Tstring(".removeon "); os.CatStr(cv); os.CatStr(cl); os.CatStr(cln2);
      os.CatStr(evtRem); os.CatStr(fn); os.CatStr("(class "); 
      os.CatStr(tn); os.CatChar(')'); 
      os.CatEOL(); 
    END Decl;
   (* ------------------------------------------------- *)
  BEGIN
    eTp := id.type(Ty.Event);
    os.CatEOL(); 
   (*
    *  Emit the "add_*" method
    *)
    EmitEvtMth(os, TRUE, id);
   (*
    *  Emit the "remove_*" method
    *)
    EmitEvtMth(os, FALSE, id);
   (*
    *  Emit the .event declaration" 
    *)
    WITH id : Id.FldId DO
        Decl(os, inVd, id.recTyp(Ty.Record).scopeNm, id.fldNm, eTp.tName); 
    | id : Id.VarId DO
        Decl(os, vStr, 
              Mu.cat2(id.dfScp(Id.BlkId).scopeNm, id.clsNm), 
              id.varNm, eTp.tName); 
    END;
    os.CloseBrace(4);
    os.CatEOL(); 
  END EmitEventMethods;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)CallCombine(typ : Sy.Type;
                                        add : BOOLEAN),NEW;
  BEGIN
    os.CatStr("        call        class [mscorlib]System.Delegate ");
    os.CatEOL(); 
    os.CatStr("                       [mscorlib]System.Delegate::");
    IF add THEN os.CatStr("Combine(") ELSE os.CatStr("Remove(") END; 
    os.CatEOL(); 
    os.CatStr(
"                               class [mscorlib]System.Delegate,");
    os.CatEOL(); 
    os.CatStr(
"                               class [mscorlib]System.Delegate)");
    os.CatEOL(); 
    os.CodeT(Asm.opc_castclass, typ);
  END CallCombine;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE (os : IlasmFile)MkAndLinkDelegate*(dl  : Sy.Idnt;
                                               id  : Sy.Idnt;
                                               ty  : Sy.Type;
                                               add : BOOLEAN);
    VAR rcv : INTEGER;
   (* --------------------------------------------------------- *)
    PROCEDURE Head(os : IlasmFile; 
                   id : Sy.Idnt; 
                   cc : Lv.CharOpen); 
    BEGIN
      Mu.MkIdName(id, os);
      os.Prefix(Asm.opc_call);
      os.Tstring(cc);
    END Head;
   (* --------------------------------------------------------- *)
    PROCEDURE Tail(os  : IlasmFile; 
                   ty  : Sy.Type; 
                 nm  : Lv.CharOpen; 
                   add : BOOLEAN);
    BEGIN
      os.CatStr(cln2);
      IF add THEN os.CatStr(evtAdd) ELSE os.CatStr(evtRem) END; 
      os.CatStr(nm);  os.CatChar("(");
      os.TypeTag(ty); os.CatChar(")");
      os.Suffix(Asm.opc_call);
    END Tail;
   (* --------------------------------------------------------- *)
  BEGIN
    WITH id : Id.FldId DO
       (*
        *      <push handle>                 // ... already done
        *      <push receiver (or nil)>      // ... already done
        *      <make new proc value>         // ... still to do
        *      call      instance void A.B::add_fld(class tyName)
        *)
        os.MkNewProcVal(dl, ty);
        Head(os, id, inVd);
        os.CatStr(id.recTyp(Ty.Record).scopeNm);
        Tail(os, ty, id.fldNm, add);
    | id : Id.VarId DO
       (*
        *      <push receiver (or nil)>      // ... already done
        *      <make new proc value>         // ... still to do
        *      call      void A.B::add_fld(class tyName)
        *)
        os.MkNewProcVal(dl, ty);
        Head(os, id, vStr);
        Mu.MkBlkName(id.dfScp(Id.BlkId));
        os.CatStr(id.dfScp.scopeNm);
        os.CatStr(id.clsNm);
        Tail(os, ty, id.varNm, add);
    | id : Id.LocId DO
       (*
        *      <save receiver>      
        *      ldloc      'local'
        *      <restore receiver>      
        *      <make new proc value>            // ... still to do
        *      call      class D D::Combine(class D, class D)
        *)
        rcv := os.proc.newLocal(Cs.ntvObj);
        os.StoreLocal(rcv);
        os.GetLocal(id);
        os.PushLocal(rcv);
        os.MkNewProcVal(dl, ty);
        os.CallCombine(ty, add);
        os.PutLocal(id); 
    END;
  END MkAndLinkDelegate;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE (os : IlasmFile)EmitPTypeBody*(tId : Id.TypId);
    VAR pTp : Ty.Procedure;
  BEGIN
    pTp := tId.type(Ty.Procedure);
    os.CatEOL(); 
    os.CatStr(".class public auto sealed "); os.CatStr(tId.type.name());
    os.CatEOL(); 
   (*
    *  From Beta-2, all delegates derive from MulticastDelegate
    *)
    os.Tstring("extends [mscorlib]System.MulticastDelegate {");
    os.CatEOL(); 
    os.CatStr(".method public specialname rtspecialname instance void");
    os.CatEOL(); 
    os.Translate(pVarSuffix);
    os.CatStr(" runtime managed { }");
    os.CatEOL(); 
    os.CatStr(".method public virtual instance ");
    os.RetType(pTp, NIL);
    os.CatEOL(); 
    os.Tstring("Invoke"); 

    os.SignatureDecl(pTp);

    os.CatStr(" runtime managed { }"); os.CatEOL(); 
    os.CloseBrace(2);
    os.CatEOL(); 
  END EmitPTypeBody;

(* ============================================================ *)
(*            End of Procedure Variable and Event Stuff         *)
(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Line*(nm : INTEGER);
  BEGIN
    os.CatStr(Asm.dirStr[Asm.dot_line]); 
    os.Tint(nm);
    os.Tstring(os.srcS);
    os.CatEOL();
  END Line;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)LinePlus*(l,w : INTEGER);
  BEGIN
    os.CatStr(Asm.dirStr[Asm.dot_line]); 
    os.Tint(l);
    os.CatChar(":");
    os.CatInt(w);
    os.Tstring(os.srcS);
    os.CatEOL();
  END LinePlus;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)LineSpan*(s : Scn.Span);
  BEGIN
    IF s = NIL THEN RETURN END;
    os.CatStr(Asm.dirStr[Asm.dot_line]); 
    os.Tint(s.sLin);
    os.CatChar(",");
    os.CatInt(s.eLin);
    os.CatChar(":");
    os.CatInt(s.sCol);
    os.CatChar(",");
    os.CatInt(s.eCol);
    os.Bstring(os.srcS);
    os.CatEOL();
  END LineSpan;
  
(* ============================================================ *)

  PROCEDURE (os : IlasmFile)Locals(),NEW;
  (** Declare the local of this method. *)
    VAR count : INTEGER;
        index : INTEGER;
        prcId : Sy.Scope;
        locId : Id.LocId;
  BEGIN
    count := 0;
   (* if dMax < 8, leave maxstack as default *)
    IF os.proc.dMax < 8 THEN os.CatStr("//") END;
    os.CatChar(ASCII.HT);
    os.CatStr(Asm.dirStr[Asm.dot_maxstack]); 
    os.Tint(os.proc.dMax);
    os.CatEOL();

    os.CatChar(ASCII.HT);
    os.CatStr(Asm.dirStr[Asm.dot_locals]); 
    IF os.proc.tLst.tide > 0 THEN os.CatStr(" init ") END; 
    IF os.proc.tLst.tide > 1 THEN 
      os.Separator("(",2);
    ELSE
      os.CatChar("(");
    END;
    IF os.proc.prId # NIL THEN 
      prcId := os.proc.prId;
      WITH prcId : Id.Procs DO
        IF Id.hasXHR IN prcId.pAttr THEN
          os.TypeTag(prcId.xhrType);
          INC(count);
        END;
        FOR index := 0 TO prcId.locals.tide-1 DO
          locId := prcId.locals.a[index](Id.LocId);
          IF ~(locId IS Id.ParId) & (locId.varOrd # Id.xMark) THEN
            IF count > 0 THEN os.Separator(',', 2) END;
            os.TypeTag(locId.type);
            os.Tidnt(locId);
            INC(count);
          END;
        END;
      ELSE (* nothing for module blocks *)
      END;
    END;
    WHILE count < os.proc.tLst.tide DO 
      IF count > 0 THEN os.Separator(',', 2) END;
      os.TypeTag(os.proc.tLst.a[count]);
      INC(count);
    END;
    os.CatChar(")");
    os.CatEOL();
  END Locals;

(* ============================================================ *)

  PROCEDURE (os : IlasmFile)LoadType*(id : Sy.Idnt);
  BEGIN
   (*
    *    ldtoken <Type>
    *    call class [mscorlib]System.Type 
    *           [mscorlib]System.Type::GetTypeFromHandle(
    *                     value class [mscorlib]System.RuntimeTypeHandle)
    *)
    os.CodeT(Asm.opc_ldtoken, id.type);
    os.CatStr("        call    class [mscorlib]System.Type");
            os.CatEOL(); 
    os.CatStr("            [mscorlib]System.Type::GetTypeFromHandle(");
            os.CatEOL(); 
    os.CatStr("            value class [mscorlib]System.RuntimeTypeHandle)");
            os.CatEOL(); 
  END LoadType;

(* ============================================================ *)
(* ============================================================ *)
BEGIN
  rts[Mu.vStr2ChO] := BOX("wchar[] [RTS]CP_rts::strToChO($S)");
  rts[Mu.vStr2ChF] := BOX("void [RTS]CP_rts::StrToChF(wchar[], $S)");
  rts[Mu.sysExit]  := BOX("void    [mscorlib]System.Environment::Exit(int32)");
  rts[Mu.toUpper]  := BOX("wchar   [mscorlib]System.Char::ToUpper(wchar) ");
  rts[Mu.dFloor]   := BOX("float64 [mscorlib]System.Math::Floor(float64) ");
  rts[Mu.dAbs]     := BOX("float64 [mscorlib]System.Math::Abs(float64) ");
  rts[Mu.fAbs]     := BOX("float32 [mscorlib]System.Math::Abs(float32) ");
  rts[Mu.iAbs]     := BOX("int32   [mscorlib]System.Math::Abs(int32) ");
  rts[Mu.lAbs]     := BOX("int64   [mscorlib]System.Math::Abs(int64) ");
  rts[Mu.getTpM]   := BOX("instance class [mscorlib]System.Type $o::GetType()");
  rts[Mu.CpModI]   := BOX("int32 [RTS]CP_rts::CpModI(int32, int32)");
  rts[Mu.CpDivI]   := BOX("int32 [RTS]CP_rts::CpDivI(int32, int32)");
  rts[Mu.CpModL]   := BOX("int64 [RTS]CP_rts::CpModL(int64, int64)");
  rts[Mu.CpDivL]   := BOX("int64 [RTS]CP_rts::CpDivL(int64, int64)");
  rts[Mu.aStrLen]  := BOX("int32 [RTS]CP_rts::chrArrLength(wchar[])");
  rts[Mu.aStrChk]  := BOX("void  [RTS]CP_rts::ChrArrCheck(wchar[])");
  rts[Mu.aStrLp1]  := BOX("int32 [RTS]CP_rts::chrArrLplus1(wchar[])");
  rts[Mu.aaStrCmp] := BOX("int32 [RTS]CP_rts::strCmp(wchar[],wchar[])");
  rts[Mu.aaStrCopy]:= BOX("void  [RTS]CP_rts::Stringify(wchar[],wchar[])");
  rts[Mu.caseMesg] := BOX("$S [RTS]CP_rts::caseMesg(int32)");
  rts[Mu.withMesg] := BOX("$S [RTS]CP_rts::withMesg($O)");
  rts[Mu.chs2Str]  :=  BOX("$S [RTS]CP_rts::mkStr(wchar[])");
  rts[Mu.CPJstrCatAA] := BOX("$S [RTS]CP_rts::aaToStr(wchar[],wchar[])");
  rts[Mu.CPJstrCatSA] := BOX("$S [RTS]CP_rts::saToStr($S, wchar[])");
  rts[Mu.CPJstrCatAS] := BOX("$S [RTS]CP_rts::asToStr(wchar[], $S)");
  rts[Mu.CPJstrCatSS] := BOX("$S [RTS]CP_rts::ssToStr($S, $S)");
  rts[Mu.mkExcept]    := BOX(
            "instance void [mscorlib]System.Exception::.ctor($S)");

(* ============================================================ *)

  Lv.InitCharOpenSeq(nmArray, 8);

  evtAdd := Lv.strToCharOpen("add_"); 
  evtRem := Lv.strToCharOpen("remove_"); 

  cln2 := Lv.strToCharOpen("::"); 
  brks := Lv.strToCharOpen("[]"); 
  cmma := Lv.strToCharOpen(","); 
  lPar := Lv.strToCharOpen("("); 
  rPar := Lv.strToCharOpen(")"); 
  rfMk := Lv.strToCharOpen("&"); 
  vFld := Lv.strToCharOpen("v$"); 
  ouMk := Lv.strToCharOpen("[out] ");
  clss := Lv.strToCharOpen("class "); 
  vals := Lv.strToCharOpen("value "); 
  vStr := Lv.strToCharOpen("void "); 
  inVd := Lv.strToCharOpen("instance void "); 
  brsz := Lv.strToCharOpen("    {} // abstract method"); 
  xhrMk := Lv.strToCharOpen("class [RTS]XHR"); 
  boxedObj := Lv.strToCharOpen("Boxed_"); 
  pVarSuffix := Lv.strToCharOpen(".ctor($O, native int) ");
END IlasmUtil.
(* ============================================================ *)
(* ============================================================ *)

