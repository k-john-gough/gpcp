
MODULE BiHtmlWriter;
  IMPORT 
    Console,
    Error,
    GPFiles,
    GPTextFiles,
    Btd := BiTypeDefs,
    RTS;

(* ============================================================ *)

  TYPE FileWriter* = POINTER TO RECORD file : GPTextFiles.FILE END;

(* ============================================================ *)

  PROCEDURE (w : FileWriter)EOL(),NEW;
  BEGIN
    GPTextFiles.WriteEOL(w.file); 
  END EOL;

  PROCEDURE (w : FileWriter)Text(IN tag : ARRAY OF CHAR),NEW;
  BEGIN
    GPTextFiles.WriteNChars(w.file, tag, LEN(tag)-1); 
  END Text;

  PROCEDURE (w : FileWriter)StartTag(IN tag : ARRAY OF CHAR),NEW;
  BEGIN
    GPTextFiles.WriteChar(w.file, '<'); 
    GPTextFiles.WriteNChars(w.file, tag, LEN(tag)-1); 
    GPTextFiles.WriteChar(w.file, '>');
  END StartTag;

  PROCEDURE (w : FileWriter)EndTag(IN tag : ARRAY OF CHAR),NEW;
  BEGIN
    GPTextFiles.WriteChar(w.file, '<'); 
    GPTextFiles.WriteChar(w.file, '/'); 
    GPTextFiles.WriteNChars(w.file, tag, LEN(tag)-1); 
    GPTextFiles.WriteChar(w.file, '>');
  END EndTag;

  PROCEDURE (w : FileWriter)EntireTag(IN tag : ARRAY OF CHAR),NEW;
  BEGIN
    GPTextFiles.WriteChar(w.file, '<');
    GPTextFiles.WriteNChars(w.file, tag, LEN(tag)-1); 
    GPTextFiles.WriteChar(w.file, '/');
    GPTextFiles.WriteChar(w.file, '>');
  END EntireTag;

  PROCEDURE (w : FileWriter)Heading(levl : INTEGER;
                                 IN text : ARRAY OF CHAR),NEW;
    VAR hx : ARRAY 3 OF CHAR;
  BEGIN
    hx[0] := 'h'; hx[1] := CHR((levl MOD 5) + ORD('0'));
    w.StartTag(hx); w.Text(text); w.EndTag(hx); w.EOL();
  END Heading;

  PROCEDURE (w : FileWriter)WriteHref(ref : Btd.FileDescriptor),NEW;
    VAR i : INTEGER;
  BEGIN
    FOR i := 0 TO ref.pkgDepth - 1 DO w.Text("|   ") END; 
    w.StartTag('a href="' + ref.name^ + '"');
    w.Text(ref.dotNam);
    w.EndTag('a');
    w.EOL();
  END WriteHref;

(* ============================================================ *)

  PROCEDURE NewHtmlWriter*(IN name : ARRAY OF CHAR;
                           IN dstP : ARRAY OF CHAR) : FileWriter;
    VAR wrtr : FileWriter;
        fSep : ARRAY 2 OF CHAR;
  BEGIN
    fSep[0] := GPFiles.fileSep;
    NEW(wrtr);
    wrtr.file := GPTextFiles.createPath(dstP + fSep + name);
    IF wrtr.file = NIL THEN
      Error.WriteString("Could not create file <");
      Error.WriteString(dstP + fSep + name + ">, HALTING");
      Error.WriteLn;
      HALT(1);
    END;
    RETURN wrtr;
  END NewHtmlWriter;

  PROCEDURE (wrtr : FileWriter)WriteHeader*(),NEW;
  BEGIN
    wrtr.StartTag("!DOCTYPE html");
    wrtr.StartTag("html");
    wrtr.StartTag("head");
    wrtr.StartTag("title");
    wrtr.Text("Html-Index");
    wrtr.EndTag("title");
    wrtr.EndTag("head");
    wrtr.EOL();
    wrtr.StartTag('body bgcolor="white"');
    wrtr.EntireTag("hr");
    wrtr.Heading(1, "Browser File Index");
    wrtr.EntireTag("hr");
  END WriteHeader;

  PROCEDURE (wrtr : FileWriter)WriteNamespaceHeader*(prefix : Btd.CharOpen),NEW;
  BEGIN
    wrtr.Heading(2, "Namespace " + prefix^);
    wrtr.EntireTag("hr");
  END WriteNamespaceHeader;

  PROCEDURE (wrtr : FileWriter)WriteList*(list : VECTOR OF Btd.FileDescriptor),NEW;
    VAR modIx : INTEGER;
  BEGIN
    wrtr.StartTag("pre");
    FOR modIx := 0 TO LEN(list) - 1 DO wrtr.WriteHref(list[modIx]) END;
    wrtr.EndTag("pre");
    wrtr.EntireTag("hr");
  END WriteList;

  PROCEDURE (wrtr : FileWriter)WriteUnnamedList*(list : VECTOR OF Btd.FileDescriptor),NEW;
  BEGIN
    wrtr.Heading(2, "Component Pascal Modules (Native CP)");
    wrtr.WriteList(list);
  END WriteUnnamedList;

  PROCEDURE (wrtr : FileWriter)WriteFooter*(),NEW;
  BEGIN
    wrtr.EntireTag("hr");
    wrtr.EndTag("body");
    wrtr.EndTag("html");
    GPTextFiles.CloseFile(wrtr.file);
  END WriteFooter;

(* ============================================================ *)

BEGIN
END BiHtmlWriter.


