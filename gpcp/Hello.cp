
MODULE Hello;
  IMPORT CPmain, Console, 
  BF := GPBinFiles,
  RTS;

  CONST greet = "Hello ASM World";
  VAR   file : BF.FILE;
        indx : INTEGER;
        pLen : INTEGER;
        path : POINTER TO ARRAY OF CHAR;
        char : CHAR;
        jStr : RTS.NativeString;

  PROCEDURE WriteArray(IN a : ARRAY OF CHAR);
    VAR indx : INTEGER;
        char : CHAR;
  BEGIN
    Console.Write(char);
    FOR indx := 0 TO LEN(a) - 1 DO
      char := a[indx];
      Console.WriteInt(indx, 2);
      Console.WriteInt(ORD(char), 0);
      IF char # 0X THEN
        Console.Write(" ");
        Console.Write(char);
      END;
      Console.WriteLn;
    END;
  END WriteArray;

BEGIN
  Console.WriteString(greet);
  Console.WriteLn;
  NEW(path, 4);
  path[0] := "A"; path[1] := "B"; path[2] := "C";
  WriteArray(path);
  Console.WriteString(MKSTR(path^)); Console.WriteLn;
  Console.WriteString(path); Console.WriteLn;
  Console.WriteString(path^ + " !"); Console.WriteLn;
  path[3] := "D";
  WriteArray(path);
  Console.WriteString(MKSTR(path^)); Console.WriteLn;
  Console.WriteString(path); Console.WriteLn;
  Console.WriteString(path^ + " !"); Console.WriteLn;

(*
  file := BF.findOnPath("CPSYM", "rts.cps");
  IF file # NIL THEN
    path := BF.getFullPathName(file);
    jStr := MKSTR(path^);
    pLen := LEN(path);
    Console.WriteString("path length =");
    Console.WriteInt(pLen, 0); Console.WriteLn;
    WriteArray(path);
    Console.WriteString(path^); Console.WriteLn;
    Console.WriteString(jStr); Console.WriteLn;
    Console.WriteString(path^ + " was found"); Console.WriteLn;
    Console.WriteString(jStr + " was found"); Console.WriteLn;
    path := BOX("foobar");
    jStr := MKSTR(path^);
    pLen := LEN(path);
    Console.WriteString("path length =");
    Console.WriteInt(pLen, 0); Console.WriteLn;
    WriteArray(path);
    Console.WriteString(path^); Console.WriteLn;
    Console.WriteString(jStr); Console.WriteLn;
    Console.WriteString(path^ + " was found"); Console.WriteLn;
    Console.WriteString(jStr + " was found"); Console.WriteLn;
  ELSE
    Console.WriteString("File not found"); Console.WriteLn;
  END;
 *)
END Hello.

