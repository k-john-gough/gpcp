(* ============================================================ *)

MODULE BiStateHandler;

  IMPORT 
        RTS,
        Console,
        Error,
        GPFiles,
        GPBinFiles,
        ProgArgs,
        GPTextFiles,
        Btd := BiTypeDefs,
        Hwr := BiHtmlWriter;

(* ============================================================ *)

  TYPE
    FileList = 
        VECTOR OF Btd.FileDescriptor; (* all with same prefix *)

    PackageList =
        VECTOR OF FileList;

(* ============================================================ *)

  TYPE
    PackageInfo = RECORD key : Btd.CharOpen; val : FileList END;
               
    PrefixTable = RECORD table : VECTOR OF PackageInfo END;
                    
(* ============================================================ *)

  TYPE State* = 
        POINTER TO RECORD
          dstPath*    : Btd.CharOpen;
          prefixTab   : PrefixTable;
          unnamedList : FileList;
          verbose*    : BOOLEAN;
        END; 

(* ============================================================ *)

  VAR nilList     : FileList;

  VAR idx : INTEGER;
          arg : ARRAY 256 OF CHAR;
          dstPath : Btd.CharOpen;

(* ============================================================ *)

  PROCEDURE (IN tab : PrefixTable)lookup(IN key : ARRAY OF CHAR) : FileList,NEW;
    VAR ix : INTEGER;
  BEGIN
    FOR ix := 0 TO LEN(tab.table)-1 DO
      IF key = tab.table[ix].key^ THEN RETURN tab.table[ix].val END;
    END;
    RETURN nilList;
  END lookup;

(* ============================================================ *)

  PROCEDURE (s : State)InitPackageList*(),NEW; 
    VAR newList : FileList;
  BEGIN
    NEW(nilList,2);
   (*
    *  Create a FileList for the empty prefix!
    *  This FileList is NOT in the prefixTab.
    *)
    NEW(s.unnamedList, 8);
   (*
    *  Create an initially empty prefix lookup table.
    *)
    NEW(s.prefixTab.table, 8);
  END InitPackageList;

(* ============================================================ *)

 (* 
  *  For each FileList in prefixTab DO
  *  * Find the min pkgDepth of all the FileDescs on this list
  *  * Adjust pkgDepth for each FileDesc on the list
  *)
  PROCEDURE (s : State)AdjustPkgDepth*(),NEW;
    VAR pCount, fCount : INTEGER;
        thisInfo : PackageInfo;
        thisList : FileList;
        minDepth : INTEGER;
  BEGIN
    FOR pCount := 0 TO LEN(s.prefixTab.table) - 1 DO
      thisInfo := s.prefixTab.table[pCount];
      thisList := thisInfo.val;
      minDepth := 100;
      FOR fCount := 0 TO LEN(thisList) - 1 DO
        minDepth := MIN(minDepth, thisList[fCount].pkgDepth);
      END;
      FOR fCount := 0 TO LEN(thisList) - 1 DO
        DEC(thisList[fCount].pkgDepth, minDepth);
      END;
    END;
  END AdjustPkgDepth;

(* ============================================================ *)

  PROCEDURE (s : State)GetFileList(IN label : ARRAY OF CHAR) : FileList,NEW; 
    VAR result : FileList;
        pkgInf : PackageInfo;
  BEGIN
    result := s.prefixTab.lookup(label);
    IF LEN(result) = 0 THEN 
      NEW(result, 4);
      pkgInf.key := BOX(label);
      pkgInf.val := result;
      APPEND(s.prefixTab.table, pkgInf); (* value-copy of pkgInf *)
    END;
    RETURN result;
  END GetFileList;

(* ============================================================ *)
(*                 Name-manipulation Utilities                  *)
(* ============================================================ *)

  PROCEDURE GetPrefix(IN name : ARRAY OF CHAR) : Btd.CharOpen;
    VAR ix : INTEGER;
        jx : INTEGER;
        rz : Btd.CharOpen;
  BEGIN
    FOR ix := 0 TO LEN(name)-1 DO
      IF name[ix] = '_' THEN 
        NEW(rz, ix + 1);
        FOR jx := 0 TO ix-1 DO rz[jx] := name[jx] END;
        rz[ix] := 0X;
        RETURN rz;
      END;
    END;
    RETURN NIL;
  END GetPrefix;

  PROCEDURE TrimFileExt(IN name : ARRAY OF CHAR) : Btd.CharOpen;
    VAR ix : INTEGER;
        jx : INTEGER;
        rz : Btd.CharOpen;
  BEGIN
    FOR ix := LEN(name)-1 TO 0 BY -1 DO
      IF name[ix] = '.' THEN
        NEW(rz, ix+1);
        FOR jx := 0 TO ix-1 DO rz[jx] := name[jx] END;
        rz[ix] := 0X;
        RETURN rz;
      END;
    END; 
    RETURN NIL;   
  END TrimFileExt;

  PROCEDURE WarpCharOpen(name : Btd.CharOpen) : INTEGER;
    VAR ix : INTEGER;
        ll : INTEGER; (* number of low_lines *)
  BEGIN 
    ll := 0;
    FOR ix := 0 TO LEN(name)-1 DO
      IF name[ix] = '_' THEN name[ix] := '.'; INC(ll) END;
    END;
    RETURN ll;
  END WarpCharOpen;

(* ============================================================ *)

  PROCEDURE (s : State)ProcessFileName(IN name : ARRAY OF CHAR),NEW;
    VAR fileDsc : Btd.FileDescriptor;
        filLst : FileList;
  BEGIN
    NEW(fileDsc);
    fileDsc.name := BOX(name);
    fileDsc.dotNam := TrimFileExt(name);
    fileDsc.prefix := GetPrefix(name);
    fileDsc.pkgDepth := WarpCharOpen(fileDsc.dotNam);
    IF fileDsc.prefix = NIL THEN
      APPEND( s.unnamedList, fileDsc);
    ELSE
      filLst := s.GetFileList(fileDsc.prefix);
      APPEND( filLst, fileDsc );
    END;
  END ProcessFileName;

(* ============================================================ *)

  PROCEDURE (s : State)ListFiles*(),NEW;
    VAR rp, cp : INTEGER;
        fName  : RTS.NativeString; 
        files  : POINTER TO ARRAY OF RTS.CharOpen;
    (* ------------------- *)
    PROCEDURE EndsWith(name : RTS.CharOpen; 
                    IN extn : ARRAY OF CHAR) : BOOLEAN;
      VAR i, j : INTEGER;
    BEGIN
      i := LEN(name^); 
      IF i < LEN(extn) THEN RETURN FALSE END;
      FOR j := LEN(extn) - 1 TO 0 BY -1 DO
        DEC(i);
        IF CAP(name[i]) # CAP(extn[j]) THEN RETURN FALSE END;
      END;
      RETURN TRUE;
    END EndsWith;
    (* ------------------- *)
  BEGIN
    files := GPFiles.FileList(s.dstPath);
    IF files = NIL THEN 
      Error.WriteString("No Files for dst = " + s.dstPath^);
      Error.WriteLn;
    ELSE
      FOR rp := 0 TO LEN(files) - 1 DO
        IF EndsWith(files[rp], ".html") THEN 
          s.ProcessFileName(files[rp]);
        END;
      END;
    END;
  END ListFiles;

(* ============================================================ *)

  PROCEDURE (s : State)DiagnosticDump(),NEW;
    VAR pkgIx : INTEGER;
        pkgInfo : PackageInfo;
        fileList : FileList;
   (* -------------------------- *)
    PROCEDURE DumpList(fl : FileList);
      VAR modIx : INTEGER;
    BEGIN
      FOR modIx := 0 TO LEN(fl) - 1 DO
        Console.WriteInt(modIx + 1, 6); Console.Write(" ");
        Console.WriteString(fl[modIx].name); Console.WriteLn;
      END;
    END DumpList;
   (* -------------------------- *)
  BEGIN
    Console.WriteString("Index Diagnostic Dump"); Console.WriteLn;
    Console.WriteString("    Local CP Modules"); Console.WriteLn;
    DumpList(s.unnamedList);
    FOR pkgIx := 0 TO LEN(s.prefixTab.table) - 1 DO
      pkgInfo := s.prefixTab.table[pkgIx];
      Console.WriteString(" NameSpace " + pkgInfo.key^); Console.WriteLn;
      DumpList(pkgInfo.val);
    END;    
  END DiagnosticDump;

(* ============================================================ *)

  PROCEDURE (s : State)WriteHtml(),NEW;
    VAR pkgIx : INTEGER;
        pkgInfo : PackageInfo;
        fileList : FileList;
        writer : Hwr.FileWriter;
   (* -------------------------- *
    PROCEDURE WriteList(fl : FileList; wr : Hwr.FileWriter);
      VAR modIx : INTEGER;
    BEGIN
      FOR modIx := 0 TO LEN(fl) - 1 DO wr.WriteHref(fl[modIx]) END;
    END WriteList;
    * -------------------------- *)
  BEGIN
    s.AdjustPkgDepth();
    writer := Hwr.NewHtmlWriter("index.html", s.dstPath);
    writer.WriteHeader();
    writer.WriteUnnamedList(s.unnamedList);
    FOR pkgIx := 0 TO LEN(s.prefixTab.table) - 1 DO
      pkgInfo := s.prefixTab.table[pkgIx];
      writer.WriteNamespaceHeader(pkgInfo.key);
      writer.WriteList(pkgInfo.val);
    END;    
    writer.WriteFooter();    
  END WriteHtml;

(* ============================================================ *)

  PROCEDURE (s : State)WriteIndex*(),NEW;
  BEGIN
    IF s.verbose THEN s.DiagnosticDump() END;
    s.WriteHtml();
  END WriteIndex;

(* ============================================================ *)

BEGIN (* Static code of Module *)
END BiStateHandler.

(* ============================================================ *)
  
