(* ==================================================================== *)
(*									*)
(*  Error Module for the Gardens Point Component Pascal Compiler.	*)
(*	Copyright (c) John Gough 1999, 2000.				*)
(*									*)
(* ==================================================================== *)

MODULE CPascalErrors;

  IMPORT 
	GPCPcopyright,
	GPTextFiles,
	Console,
        FileNames,
	Scnr := CPascalS, 
	LitValue,
	GPText;

(* ============================================================ *)

  CONST
    consoleWidth = 80;
    listingWidth = 256;
    listingMax   = listingWidth-1;

  TYPE
    ParseHandler* = POINTER TO RECORD (Scnr.ErrorHandler)
		    END;
    SemanticHdlr* = POINTER TO RECORD (Scnr.ErrorHandler)
		    END;

  TYPE
      Message = LitValue.CharOpen;

      Err     = POINTER TO ErrDesc;
      ErrDesc = RECORD
		  num, lin, col: INTEGER;
		  msg: Message;
		END;
      ErrBuff = POINTER TO ARRAY OF Err;

  VAR
      parsHdlr : ParseHandler;
      semaHdlr : SemanticHdlr;
      eBuffer  : ErrBuff;	(* Invariant: eBuffer[eTide] = NIL *)
      eLimit   : INTEGER;	(* High index of dynamic array.    *)
      eTide    : INTEGER;	(* Next index for insertion in buf *)
      prompt*  : BOOLEAN;	(* Emit error message immediately  *)
      nowarn*  : BOOLEAN;	(* Don't store warning messages    *)
      srcNam   : FileNames.NameString;
      forVisualStudio* : BOOLEAN;
      xmlErrors* : BOOLEAN;

(* ============================================================ *)

  PROCEDURE StoreError (eNum, linN, colN : INTEGER; mesg: Message);
  (* Store an error message for later printing *)
    VAR
      nextErr: Err;

    (* -------------------------------------- *)

    PROCEDURE append(b : ErrBuff; n : Err) : ErrBuff;
      VAR s : ErrBuff;
	  i : INTEGER;
    BEGIN
      IF eTide = eLimit THEN (* must expand *)
	s := b;
        eLimit := eLimit * 2 + 1;
	NEW(b, eLimit+1);
	FOR i := 0 TO eTide DO b[i] := s[i] END;
      END;
      b[eTide] := n; INC(eTide); b[eTide] := NIL;
      RETURN b;
    END append;

    (* -------------------------------------- *)

  BEGIN
    NEW(nextErr);
    nextErr.num := eNum; 
    nextErr.msg := mesg;
    nextErr.col := colN;
    nextErr.lin := linN; 
    eBuffer := append(eBuffer, nextErr);
  END StoreError;

(* ============================================================ *)

  PROCEDURE QuickSort(min, max : INTEGER);
    VAR i,j : INTEGER;
        key : INTEGER;
	tmp : Err;
   (* -------------------------------------------------	*)
    PROCEDURE keyVal(i : INTEGER) : INTEGER;
    BEGIN 
      IF (eBuffer[i].col <= 0) OR (eBuffer[i].col >= listingWidth) THEN
        eBuffer[i].col := listingMax;
      END;
      RETURN eBuffer[i].lin * 256 + eBuffer[i].col;
    END keyVal;
   (* -------------------------------------------------	*)
  BEGIN
    i := min; j := max;
    key := keyVal((min+max) DIV 2);
    REPEAT
      WHILE keyVal(i) < key DO INC(i) END;
      WHILE keyVal(j) > key DO DEC(j) END;
      IF i <= j THEN
        tmp := eBuffer[i]; eBuffer[i] := eBuffer[j]; eBuffer[j] := tmp; 
	INC(i); DEC(j);
      END;
    UNTIL i > j;
    IF min < j THEN QuickSort(min,j) END;
    IF i < max THEN QuickSort(i,max) END;
  END QuickSort;

(* ============================================================ *)

  PROCEDURE (h : ParseHandler)Report*(num,lin,col : INTEGER); 
    VAR str : ARRAY 128 OF CHAR;
	msg : Message;
	idx : INTEGER;
	len : INTEGER;
  BEGIN
    CASE num OF
    |  0: str := "EOF expected";
    |  1: str := "ident expected";
    |  2: str := "integer expected";
    |  3: str := "real expected";
    |  4: str := "CharConstant expected";
    |  5: str := "string expected";
    |  6: str := "'*' expected";
    |  7: str := "'-' expected";
    |  8: str := "'!' expected";
    |  9: str := "'.' expected";
    | 10: str := "'=' expected";
    | 11: str := "'ARRAY' expected";
    | 12: str := "',' expected";
    | 13: str := "'OF' expected";
    | 14: str := "'ABSTRACT' expected";
    | 15: str := "'EXTENSIBLE' expected";
    | 16: str := "'LIMITED' expected";
    | 17: str := "'RECORD' expected";
    | 18: str := "'(' expected";
    | 19: str := "'+' expected";
    | 20: str := "')' expected";
    | 21: str := "'END' expected";
    | 22: str := "';' expected";
    | 23: str := "':' expected";
    | 24: str := "'POINTER' expected";
    | 25: str := "'TO' expected";
    | 26: str := "'PROCEDURE' expected";
    | 27: str := "'[' expected";
    | 28: str := "']' expected";
    | 29: str := "'^' expected";
    | 30: str := "'$' expected";
    | 31: str := "'#' expected";
    | 32: str := "'<' expected";
    | 33: str := "'<=' expected";
    | 34: str := "'>' expected";
    | 35: str := "'>=' expected";
    | 36: str := "'IN' expected";
    | 37: str := "'IS' expected";
    | 38: str := "'OR' expected";
    | 39: str := "'/' expected";
    | 40: str := "'DIV' expected";
    | 41: str := "'MOD' expected";
    | 42: str := "'&' expected";
    | 43: str := "'NIL' expected";
    | 44: str := "'~' expected";
    | 45: str := "'{' expected";
    | 46: str := "'}' expected";
    | 47: str := "'..' expected";
    | 48: str := "'EXIT' expected";
    | 49: str := "'RETURN' expected";
    | 50: str := "'NEW' expected";
    | 51: str := "':=' expected";
    | 52: str := "'IF' expected";
    | 53: str := "'THEN' expected";
    | 54: str := "'ELSIF' expected";
    | 55: str := "'ELSE' expected";
    | 56: str := "'CASE' expected";
    | 57: str := "'|' expected";
    | 58: str := "'WHILE' expected";
    | 59: str := "'DO' expected";
    | 60: str := "'REPEAT' expected";
    | 61: str := "'UNTIL' expected";
    | 62: str := "'FOR' expected";
    | 63: str := "'BY' expected";
    | 64: str := "'LOOP' expected";
    | 65: str := "'WITH' expected";
    | 66: str := "'EMPTY' expected";
    | 67: str := "'BEGIN' expected";
    | 68: str := "'CONST' expected";
    | 69: str := "'TYPE' expected";
    | 70: str := "'VAR' expected";
    | 71: str := "'OUT' expected";
    | 72: str := "'IMPORT' expected";
    | 73: str := "'MODULE' expected";
    | 74: str := "'CLOSE' expected";
    | 75: str := "'JAVACLASS' expected";
    | 76: str := "not expected";
    | 77: str := "error in OtherAtts";
    | 78: str := "error in MethAttributes";
    | 79: str := "error in ProcedureStuff";
    | 80: str := "this symbol not expected in StatementSequence";
    | 81: str := "this symbol not expected in StatementSequence";
    | 82: str := "error in IdentStatement";
    | 83: str := "error in MulOperator";
    | 84: str := "error in Factor";
    | 85: str := "error in AddOperator";
    | 86: str := "error in Relation";
    | 87: str := "error in OptAttr";
    | 88: str := "error in ProcedureType";
    | 89: str := "error in Type";
    | 90: str := "error in Module";
    | 91: str := "invalid lexical token";
    END;
    len := LEN(str$);
    NEW(msg, len+1);
    FOR idx := 0 TO len-1 DO
      msg[idx] := str[idx];
    END;
    msg[len] := 0X;
    StoreError(num,lin,col,msg); 
    INC(Scnr.errors);
  END Report;

(* ============================================================ *)

  PROCEDURE (h : ParseHandler)RepSt1*(num       : INTEGER;
				        IN s1   : ARRAY OF CHAR;
					lin,col : INTEGER),EMPTY; 
  PROCEDURE (h : ParseHandler)RepSt2*(num       : INTEGER;
				       IN s1,s2 : ARRAY OF CHAR;
					lin,col : INTEGER),EMPTY; 

(* ============================================================ *)

  PROCEDURE (h : SemanticHdlr)Report*(num,lin,col : INTEGER); 
    VAR str : ARRAY 128 OF CHAR;
	msg : Message;
	idx : INTEGER;
	len : INTEGER;
  BEGIN
    CASE num OF
    (* ======================= ERRORS =========================	*)
    |  -1: str := "invalid character";
    |   0: RETURN;			(* just a placeholder *)
    |   1: str := "Name after 'END' does not match";
    |   2: str := "Identifier not known in this scope";
    |   3: str := "Identifier not known in qualified scope";
    |   4: str := "This name already known in this scope";
    |   5: str := "This identifier is not a type name";
    |   6: str := "This fieldname clashes with previous fieldname";
    |   7: str := "Qualified identifier is not a type name";
    |   8: str := "Not a record type, so you cannot select a field";
    |   9: str := "Identifier is not a fieldname of the current type";

    |  10: str := "Not an array type, so you cannot index into it";
    |  11: str := "Too many indices for the dimension of the array";
    |  12: str := "Not a pointer type, so you cannot dereference it";
    |  13: str := "Not a procedure call or type guard";
    |  14: str := "Basetype is not record or pointer type";
    |  15: str := "Typename not a subtype of the current type";
    |  16: str := "Basetype was not declared ABSTRACT or EXTENSIBLE";
    |  17: str := "Not dynamically typed, so you cannot have type-guard";
    |  18: str := "The type-guard must be a record type here";
    |  19: str := "This constant token not known";

    |  20: str := "Name of formal is not unique";
    |  21: str := "Actual parameter is not compatible with formal type";
    |  22: str := "Too few actual parameters";
    |  23: str := "Too many actual parameters";
    |  24: str := "Attempt to use a proper procedure when function needed";
    |  25: str := "Expression is not constant";
    |  26: str := "Range of the numerical type exceeded";
    |  27: str := "String literal too long for destination type";
    |  28: str := "Low value of range not in SET base-type range";
    |  29: str := "High value of range not in SET base-type range";

    |  30: str := "Low value of range cannot be greater than high value";
    |  31: str := "Array index not of an integer type";
    |  32: str := "Literal array index is outside array bounds";
    |  33: str := "Literal value is not in SET base-type range";
    |  34: str := "Typename is not a subtype of the type of destination";
    |  35: str := "Expression is not of SET type";
    |  36: str := "Expression is not of BOOLEAN type";
    |  37: str := "Expression is not of an integer type";
    |  38: str := "Expression is not of a numeric type";
    |  39: str := "Overflow of negation of literal value";

    |  40: str := "Expression is not of ARRAY type";
    |  41: str := "Expression is not of character array type";
    |  42: str := "Expression is not a standard function";
    |  43: str := "Expression is not of character type";
    |  44: str := "Literal expression is not in CHAR range";
    |  45: str := "Expression is not of REAL type";
    |  46: str := "Optional param of LEN must be a positive integer constant";
    |  47: str := "LONG cannot be applied to this type";
    |  48: str := "Name is not the name of a basic type";
    |  49: str := "MAX and MIN not applicable to this type";

    |  50: str := "ORD only applies to SET and CHAR types";
    |  51: str := "SHORT cannot be applied to this type";
    |  52: str := "Both operands must be numeric, SET or CHAR types";
    |  53: str := "Character constant outside CHAR range";
    |  54: str := "Bad conversion type";
    |  55: str := "Numeric overflow in constant evaluation";
    |  56: str := "BITS only applies to expressions of type INTEGER";
    |  57: str := "Operands in '=' or '#' test are not type compatible";
    |  58: str := "EXIT is only permitted inside a LOOP";
    |  59: str := "BY expression must be a constant expression";

    |  60: str := "Case label is not an integer or character constant";
    |  61: str := "Method attributes don't apply to ordinary procedure";
    |  62: str := "Forward type-bound method elaborated as static procedure";
    |  63: str := "Forward static procedure elaborated as type-bound method";
    |  64: str := "Forward method had different receiver mode";
    |  65: str := "Forward procedure had non-matching formal types";
    |  66: str := "Forward method had different attributes";
    |  67: str := "Variable cannot have open array type";
    |  68: str := "Arrays must have at least one element";
    |  69: str := "Fixed array cannot have open array element type";

    |  70: str := "Forward procedure had different names for formals";
    |  71: str := "This imported type is LIMITED, and cannot be instantiated";
    |  72: str := "Forward procedure was not elaborated by end of block";
    |  73: str := "RETURN is not legal in a module body";
    |  74: str := "This is a proper procedure, it cannot return a value";
    |  75: str := "This is a function, it must return a value";
    |  76: str := "RETURN value not assign-compatible with function type";
    |  77: str := "Actual for VAR formal must be a writeable variable";
    |  78: str := "Functions cannot return record types";
    |  79: str := "Functions cannot return array types";

    |  80: str := "This designator is not the name of a proper procedure";
    |  81: str := "FOR loops cannot have zero step size";
    |  82: str := "This fieldname clashes with an inherited fieldname";
    |  83: str := "Expression not assign-compatible with destination";
    |  84: str := "FOR loop control variable must be of integer type";
    |  85: str := "Identifier is not the name of a variable";
    |  86: str := "Typename is not an extension of the variable type";
    |  87: str := "The selected identifier is not of dynamic type";
    |  88: str := "Case select expression is not of integer or CHAR type";
    |  89: str := "Case select value is duplicated for this statement";

    |  90: str := "Variables of ABSTRACT type cannot be instantiated";
    |  91: str := "Optional param of ASSERT must be an integer constant";
    |  92: str := "This is not a standard procedure";
    |  93: str := "The param of HALT must be a constant integer";
    |  94: str := "This variable is not of pointer or vector type";
    |  95: str := "NEW requires a length param for open arrays and vectors";
    |  96: str := "NEW only applies to pointers to records and arrays";
    |  97: str := "This call of NEW has too many lengths specified";
    |  98: str := "Length for an open array NEW must be an integer type";
    |  99: str := "Length only applies to open arrays and vectors";

    | 100: str := "This call of NEW needs more length params";
    | 101: str := "Numeric literal is too large, even for long type";
    | 102: str := "Only ABSTRACT basetypes can have abstract extensions";
    | 103: str := "This expression is read-only";
    | 104: str := "Receiver type must be a record, or pointer to record";
    | 105: str := "This method is not a redefinition, you must use NEW";
    | 106: str := "This method is a redefinition, you must not use NEW";
    | 107: str := "Receivers of record type must be VAR or IN mode";
    | 108: str := "Final method cannot be redefined";
    | 109: str := "Only ABSTRACT method can have ABSTRACT redefinition";

    | 110: str := "This type has ABSTRACT method, must be ABSTRACT";
    | 111: str := "Type has NEW,EMPTY method, must be ABSTRACT or EXTENSIBLE";
    | 112: str := "Only EMPTY or ABSTRACT method can be redefined EMPTY";
    | 113: str := "This redefinition of exported method must be exported";
    | 114: str := "This is an EMPTY method, and cannot have OUT parameters";
    | 115: str := "This is an EMPTY method, and cannot return a value";
    | 116: str := "Redefined method must have consistent return type";
    | 117: str := "Type has EXTENSIBLE method, must be ABSTRACT or EXTENSIBLE";
    | 118: str := "Empty or abstract methods cannot be called by super-call";
    | 119: str := "Super-call is invalid here";

    | 120: str := "There is no overridden method with this name";
    | 121: str := "Not all abstract methods were implemented";
    | 122: str := "This procedure is not at module scope, cannot be a method";
    | 123: str := "There is a cycle in the base-type declarations";
    | 124: str := "There is a cycle in the field-type declarations";
    | 125: str := "Cycle in typename equivalence declarations";
    | 126: str := "There is a cycle in the array element type declarations";
    | 127: str := "This is an implement-only method, and cannot be called";
    | 128: str := "Only declarations at module level can be exported";
    | 129: str := "Cannot open symbol file";

    | 130: str := "Bad magic number in symbol file";
    | 131: str := "This type is an INTERFACE, and cannot be instantiated";
    | 132: str := "Corrupted symbol file";
    | 133: str := "Inconsistent module keys";
    | 134: str := "Types can only be public or fully private";
    | 135: str := "This variable may be uninitialized";
    | 136: str := "Not all paths to END contain a RETURN statement";
    | 137: str := "This type tries to directly include itself";
    | 138: str := "Not all paths to END in RESCUE contain a RETURN statement";
    | 139: str := "Not all OUT parameters have been assigned to";

    | 140: str := "Pointer bound type can only be RECORD or ARRAY";
    | 141: str := "GPCP restriction: select expression cannot be LONGINT";
    | 142: str := "Cannot assign entire open array";
    | 143: str := "Cannot assign entire extensible or abstract record";
    | 144: str := "Foreign modules must be compiled with '-special'";
    | 145: str := "This type tries to indirectly include itself";
    | 146: str := "Constructors are declared without receiver";
    | 147: str := "Multiple supertype constructors match these parameters";
    | 148: str := "This type has another constructor with equal signature";
    | 149: str := "This procedure needs parameters";

    | 150: str := "Parameter types of exported procedures must be exported";
    | 151: str := "Return types of exported procedures must be exported";
    | 152: str := "Bound type of foreign reference type cannot be assigned";
    | 153: str := "Bound type of foreign reference type cannot be value param";
    | 154: str := "It is not possible to extend an interface type";
    | 155: str := "NEW illegal unless foreign supertype has no-arg constructor";
    | 156: str := "Interfaces can't extend anything. Leave blank or use ANYREC";
    | 157: str := "Only extensions of Foreign classes can implement interfaces";
    | 158: str := "Additional base types must be interface types";
    | 159: str := "Not all interface methods were implemented";

    | 160: str := "Inherited procedure had non-matching formal types";
    | 161: str := "Only foreign procs and fields can have protected mode";
    | 162: str := "This name only accessible in extensions of defining type";
    | 163: str := "Interface implementation has wrong export mode";
(**)| 164: str := "Non-locally accessed variable may be uninitialized";
    | 165: str := "This procedure cannot be used as a procedure value";
    | 166: str := "Super calls are only valid on the current receiver";
    | 167: str := "SIZE is not meaningful in this implementation";
    | 168: str := "Character literal outside SHORTCHAR range";
    | 169: str := "Module exporting this type is not imported";

    | 170: str := "This module has already been directly imported";
    | 171: str := "Invalid binary operation on these types";
    | 172: str := "Name clash in imported scope";
    | 173: str := "This module indirectly imported with different key";
    | 174: str := "Actual for IN formal must be record, array or string";
    | 175: str := "The module exporting this name has not been imported";
    | 176: str := "The current type is opaque and cannot be selected further";
    | 177: str := "File creation error";
    | 178: str := "This record field is read-only";
    | 179: str := "This IN parameter is read-only";

    | 180: str := "This variable is read-only";
    | 181: str := "This identifier is read-only";
    | 182: str := "Attempt to use a function when a proper procedure needed";
    | 183: str := "This record is private, you cannot export this field";
    | 184: str := "This record is readonly, this field cannot be public";
    | 185: str := "Static members can only be defined with -special";
    | 186: str := 'Ids with "$", "@" or "`" can only be defined with -special';
    | 187: str := "Idents escaped with ` must have length >= 2";
    | 188: str := "Methods of INTERFACE types must be ABSTRACT";
    | 189: str := "Non-local access to byref param of value type";

    | 190: str := "Temporary restriction: non-locals not allowed";
    | 191: str := "Temporary restriction: only name equivalence here";
    | 192: str := "Only '=' or ':' can go here";
    | 193: str := "THROW needs a string or native exception object";
    | 194: str := 'Only "UNCHECKED_ARITHMETIC" can go here';
    | 195: str := "NEW method cannot be exported if receiver type is private";
    | 196: str := "Only static fields can select on a type-name";
    | 197: str := "Only static methods can select on a type-name";
    | 198: str := "Static fields can only select on a type-name";
    | 199: str := "Static methods can only select on a type-name";

    | 200: str := "Constructors cannot be declared for imported types";
    | 201: str := "Constructors must return POINTER TO RECORD type";
    | 202: str := "Base type does not have a matching constructor";
    | 203: str := "Base type does not allow a no-arg constructor";
    | 204: str := "Constructors only allowed for extensions of foreign types";
    | 205: str := "Methods can only be declared for local record types";
    | 206: str := "Receivers of pointer type must have value mode";
    | 207: str := "Feature with this name already known in binding scope";
    | 208: str := "EVENT types only valid for .NET target";
    | 209: str := "Events must have a valid formal parameter list";

    | 210: str := "REGISTER expects an EVENT type here";
    | 211: str := "Only procedure literals allowed here";
    | 212: str := "Event types cannot be local to procedures";
    | 213: str := "Temporary restriction: no proc. variables with JVM";
    | 214: str := "Interface types cannot be anonymous";
    | 215: str := "Interface types must be exported";
    | 216: str := "Interface methods must be exported";
    | 217: str := "Covariant OUT parameters unsafe removed from language";
    | 218: str := "No procedure of this name with matching parameters";
    | 219: str := "Multiple overloaded procedure signatures match this call";

    | 220: RETURN;                                  (* BEWARE PREMATURE EXIT *)
    | 221: str := "Non-standard construct, not allowed with /strict";
    | 222: str := "This is not a value: thus cannot end with a type guard";
    | 223: str := "Override of imp-only in exported type must be imp-only";
    | 224: str := "This designator is not a procedure or a function call";
    | 225: str := "Non-empty constructors can only return SELF";
    | 226: str := "USHORT cannot be applied to this type";
    | 227: str := "Cannot import SYSTEM without /unsafe option";
    | 228: str := "Cannot import SYSTEM unless target=net";
    | 229: str := "Designator is not of VECTOR type";

    | 230: str := "Type is incompatible with element type";
    | 231: str := "Vectors are always one-dimensional only";
    | 232: str := 'Hex constant too big, use suffix "L" instead';
    | 233: str := "Literal constant too big, even for LONGINT";
    | 234: str := "Extension of LIMITED type must be limited";
    | 235: str := "LIMITED types can only be extended in the same module";
    | 236: str := "Cannot resolve CLR name of this type";
    | 237: str := "Invalid hex escape sequence in this string";
    | 238: str := "STA is illegal unless target is NET";

    | 298: str := "ILASM failed to assemble IL file";
    | 299: str := "Compiler raised an internal exception";
    (* ===================== END ERRORS =======================	*)
    (* ====================== WARNINGS ========================	*)
    | 300: str := "Warning: Super calls are deprecated";
    | 301: str := "Warning: Procedure variables are deprecated";
    | 302: str := "Warning: Non-local variable access here";
    | 303: str := "Warning: Numeric literal is not in the SET range [0 .. 31]";
    | 304: str := "Warning: This procedure is not exported, called or assigned";
    | 305: str := "Warning: Another constructor has an equal signature";
    | 306: str := "Warning: Covariant OUT parameters unsafe when aliassed";
    | 307: str := "Warning: Multiple overloaded procedure signatures match this call";
    | 308: str := "Warning: Default static class has name clash";
    | 309: str := "Warning: Looking for an automatically renamed module";

    | 310,
      311: str := "Warning: This variable is accessed from nested procedure";
    | 312,
      313: RETURN;                                  (* BEWARE PREMATURE EXIT *)
    | 314: str := "The anonymous record type is incomptible with all values";
    | 315: str := "The anonymous array type is incomptible with all values";
    | 316: str := "This pointer type may still have its default NIL value";
    | 317: str := "Empty CASE statement will trap if control reaches here";
    | 318: str := "Empty WITH statement will trap if control reaches here";
    | 319: str := "STA has no effect without CPmain or WinMain";
    (* ==================== END WARNINGS ====================== *)
    ELSE
      str := "Semantic error: " + LitValue.intToCharOpen(num)^;	
    END;
    len := LEN(str$);
    NEW(msg, len+1);
    FOR idx := 0 TO len-1 DO
      msg[idx] := str[idx];
    END;
    msg[len] := 0X;
    IF num < 300 THEN
      INC(Scnr.errors); 
      StoreError(num,lin,col,msg); 
    ELSIF ~nowarn THEN 
      INC(Scnr.warnings); 
      StoreError(num,lin,col,msg);
    END;

    IF prompt THEN
      IF num < 300 THEN 
        Console.WriteString("Error");
      ELSE 
        Console.WriteString("Warning");
      END;
      Console.WriteInt(num,0);
      Console.WriteString("@ line:");
      Console.WriteInt(lin,0);
      Console.WriteString(", col:");
      Console.WriteInt(col,0);
      Console.WriteLn;
      Console.WriteString(str);
      Console.WriteLn;
    END;

  END Report;

(* ============================================================ *)

  PROCEDURE (h : SemanticHdlr)RepSt1*(num     : INTEGER;
				      IN s1   : ARRAY OF CHAR;
				      lin,col : INTEGER); 
    VAR msg : Message;
  BEGIN
    CASE num OF
    |   0: msg := LitValue.strToCharOpen("Expected: END " + s1);
    |   1: msg := LitValue.strToCharOpen("Expected: " + s1);
    |  89: msg := LitValue.strToCharOpen("Duplicated selector values <" 
						+ s1 + ">");
    |   9,
      169: msg := LitValue.strToCharOpen("Current type was <" 
			+ s1 + '>');
    | 117: msg := LitValue.strToCharOpen("Type <" 
			+ s1 + "> must be extensible");
    | 121: msg := LitValue.strToCharOpen("Missing methods <" + s1 + '>');
    | 145: msg := LitValue.strToCharOpen("Types on cycle <" + s1 + '>');
    | 129, 
      130, 
      132: msg := LitValue.strToCharOpen("Filename <" + s1 + '>');
    | 133: msg := LitValue.strToCharOpen("Module <" 
			+ s1 + "> already imported with different key");
    | 138: msg := LitValue.strToCharOpen('<' 
			+ s1 + '> not assigned before "RETURN"');
    | 139: msg := LitValue.strToCharOpen('<' 
			+ s1 + '> not assigned before end of procedure');
    | 154: msg := LitValue.strToCharOpen('<' 
			+ s1 + "> is a Foreign interface type");
    | 157: msg := LitValue.strToCharOpen('<' 
			+ s1 + "> is not a Foreign type");
    | 158: msg := LitValue.strToCharOpen('<' 
			+ s1 + "> is not a foreign language interface type");
    | 159: msg := LitValue.strToCharOpen("Missing interface methods <" 
						+ s1 + '>');
    | 162: msg := LitValue.strToCharOpen('<' 
			+ s1 + "> is a protected, foreign-language feature");
    | 164: msg := LitValue.strToCharOpen('<' 
			+ s1 + "> not assigned before this call");
    | 172: msg := LitValue.strToCharOpen('Name <' 
			+ s1 + '> clashes in imported scope');
    | 175,
      176: msg := LitValue.strToCharOpen("Module " 
			+ '<' + s1 + "> is not imported");
    | 189: msg := LitValue.strToCharOpen('Non-local access to <' 
			+ s1 + '> cannot be verified on .NET');
    | 205,
      207: msg := LitValue.strToCharOpen(
		       "Binding scope of feature is record type <" + s1 + ">");
    | 236: msg := LitValue.strToCharOpen(
                       "Cannot resolve CLR name of type : " + s1);
    | 299: msg := LitValue.strToCharOpen("Exception: " + s1);
    | 308: msg := LitValue.strToCharOpen(
                       "Renaming static class to <" + s1 + ">");
    | 310: msg := LitValue.strToCharOpen('Access to <' 
                       + s1 + '> has copying not reference semantics');
    | 311: msg := LitValue.strToCharOpen('Access to variable <' 
		       + s1 + '> will be inefficient');
    | 220,
      312: msg := LitValue.strToCharOpen("Matches with - " + s1);
    | 313: msg := LitValue.strToCharOpen("Bound to - " + s1);
    END;
    IF ~nowarn OR                        (* If warnings are on OR  *)
       (num < 300) THEN                  (* this is an error then  *)
      StoreError(num,lin,0,msg);         (* (1) Store THIS message *)
      h.Report(num,lin,col);             (* (2) Generate other msg *)
    END;
(*
 *  IF (num # 251) & (num # 252) THEN 
 *    StoreError(num,lin,col,msg); 
 *    h.Report(num,lin,col);
 *  ELSIF ~nowarn THEN
 *    StoreError(num,lin,col,msg); 
 *  END;
 *)
  END RepSt1;

(* ============================================================ *)

  PROCEDURE (h : SemanticHdlr)RepSt2*(num      : INTEGER;
				      IN s1,s2 : ARRAY OF CHAR;
				      lin,col  : INTEGER); 
(*
 *  VAR str : ARRAY 128 OF CHAR;
 *      msg : Message;
 *      idx : INTEGER;
 *      len : INTEGER;
 *)
    VAR msg : Message;
  BEGIN
    CASE num OF
    |  21,
      217,
      306: msg := LitValue.strToCharOpen(
                  "Actual par-type was " + s1 + ", Formal type was " + s2);
    |  76: msg := LitValue.strToCharOpen(
                  "Expr-type was " + s2 + ", should be " + s1);
    |  57,
       83: msg := LitValue.strToCharOpen(
                  "LHS type was " + s1 + ", RHS type was " + s2);
    | 116: msg := LitValue.strToCharOpen(
                  "Inherited retType is " + s1 + ", this retType " + s2);
    | 131: msg := LitValue.strToCharOpen(
                  "Module name in file <" + s1 + ".cps> was <" + s2 + '>');
    | 172: msg := LitValue.strToCharOpen(
                  'Name <' + s1 + '> clashes in scope <' + s2 + '>');
    | 230: msg := LitValue.strToCharOpen(
                  "Expression type is " + s2 + ", element type is " + s1);
    | 309: msg := LitValue.strToCharOpen(
                  'Looking for module "' + s1 + '" in file <' + s2 + '>');
    END;
(*
 *  len := LEN(str$);
 *  NEW(msg, len+1);
 *  FOR idx := 0 TO len-1 DO
 *    msg[idx] := str[idx];
 *  END;
 *  msg[len] := 0X;
 *)
    StoreError(num,lin,col,msg); 
    h.Report(num,lin,col);
  END RepSt2;

(* ============================================================ *)

    PROCEDURE GetLine (VAR pos  : INTEGER;
                       OUT line : ARRAY OF CHAR;
                       OUT eof  : BOOLEAN);
    (** Read a source line. Return empty line if eof *)
      CONST
        cr  = 0DX;
        lf  = 0AX;
        tab = 09X;
      VAR
        ch: CHAR;
        i:  INTEGER;
      BEGIN
        ch := Scnr.charAt(pos); INC(pos);
        i := 0; 
	eof := FALSE;
        WHILE (ch # lf) & (ch # 0X) DO 
          IF ch = cr THEN (* skip *)
          ELSIF ch = tab THEN 
	    REPEAT line[MIN(i,listingMax)] := ' '; INC(i) UNTIL i MOD 8 = 0;
	  ELSE
            line[MIN(i,listingMax)] := ch; INC(i);
	  END;
          ch := Scnr.charAt(pos); INC(pos);
        END;
        eof := (i = 0) & (ch = 0X); line[MIN(i,listingMax)] := 0X;
      END GetLine;

(* ============================================================ *)

    PROCEDURE PrintErr(IN desc : ErrDesc);
    (** Print an error message *)
      VAR mLen : INTEGER;
	  indx : INTEGER;
    BEGIN
      GPText.WriteString(Scnr.lst, "**** ");
      mLen := LEN(desc.msg$);
      IF desc.col = listingMax THEN (* write field of width (col-2) *)
        GPText.WriteString(Scnr.lst, desc.msg);
      ELSIF mLen < desc.col-1 THEN (* write field of width (col-2) *)
	GPText.WriteFiller(Scnr.lst, desc.msg, "-", desc.col-1);
	GPText.Write(Scnr.lst, "^");
      ELSIF mLen + desc.col + 5 < consoleWidth THEN
	GPText.WriteFiller(Scnr.lst, "", "-", desc.col-1);
        GPText.WriteString(Scnr.lst, "^ ");
        GPText.WriteString(Scnr.lst, desc.msg);
      ELSE
	GPText.WriteFiller(Scnr.lst, "", "-", desc.col-1);
	GPText.Write(Scnr.lst, "^");
	GPText.WriteLn(Scnr.lst);
	GPText.WriteString(Scnr.lst, "**** ");
        GPText.WriteString(Scnr.lst, desc.msg);
      END; 
      GPText.WriteLn(Scnr.lst);
    END PrintErr;

(* ============================================================ *)

    PROCEDURE Display (IN desc : ErrDesc);
    (** Display an error message *)
      VAR mLen : INTEGER;
	  indx : INTEGER;
    BEGIN
      Console.WriteString("**** ");
      mLen := LEN(desc.msg$);
      IF desc.col = listingMax THEN
        Console.WriteString(desc.msg);
      ELSIF mLen < desc.col-1 THEN
	Console.WriteString(desc.msg);
	FOR indx := mLen TO desc.col-2 DO Console.Write("-") END;
	Console.Write("^");
      ELSIF mLen + desc.col + 5 < consoleWidth THEN
	FOR indx := 2 TO desc.col DO Console.Write("-") END;
        Console.WriteString("^ ");
        Console.WriteString(desc.msg);
      ELSE
	FOR indx := 2 TO desc.col DO Console.Write("-") END;
	Console.Write("^");
	Console.WriteLn;
	Console.WriteString("**** ");
        Console.WriteString(desc.msg);
      END; 
      Console.WriteLn;
    END Display;

(* ============================================================ *)

    PROCEDURE DisplayVS (IN desc : ErrDesc);
    (** Display an error message for Visual Studio *)
      VAR mLen : INTEGER;
	  indx : INTEGER;
    BEGIN
      Console.WriteString(srcNam);
      Console.Write("(");
      Console.WriteInt(desc.lin,1);
      Console.Write(",");
      Console.WriteInt(desc.col,1);
      Console.WriteString(") : ");
      IF desc.num < 300 THEN
        Console.WriteString("error : ");
      ELSE
        Console.WriteString("warning : ");
      END;
      Console.WriteString(desc.msg);
      Console.WriteLn; 
    END DisplayVS;

(* ============================================================ *)

    PROCEDURE DisplayXMLHeader ();
    BEGIN
      Console.WriteString('<?xml version="1.0"?>');
      Console.WriteLn;
      Console.WriteString('<compilererrors errorsContained="yes">');
      Console.WriteLn;
    END DisplayXMLHeader;

    PROCEDURE DisplayXMLEnd ();
    BEGIN
      Console.WriteString('</compilererrors>');
      Console.WriteLn;
    END DisplayXMLEnd;

    PROCEDURE DisplayXML (IN desc : ErrDesc);
    (** Display an error message in xml format (for eclipse) *)
    (*  <?xml version="1.0"?>
     *  <compilererrors errorsContained="yes">
     *    <error>
     *      <line> 1 </line>
     *      <position> 34 </position>
     *      <description> ; expected </description>
     *    </error>
     * ...
     *  </compilererrors>
     *)
          
      VAR mLen : INTEGER;
	  indx : INTEGER;
        isWarn : BOOLEAN;
    BEGIN
      isWarn := desc.num >= 300;
      IF isWarn THEN
        Console.WriteString("  <warning> "); 
      ELSE
        Console.WriteString("  <error> "); 
      END;
      Console.WriteLn; 
      Console.WriteString("    <line> ");  
      Console.WriteInt(desc.lin,1);
      Console.WriteString(" </line>"); Console.WriteLn; 
      Console.WriteString("    <position> ");  
      Console.WriteInt(desc.col,1);
      Console.WriteString(" </position>"); Console.WriteLn; 
      Console.WriteString("    <description> ");  
      IF isWarn THEN
        Console.WriteString("warning : ");
      ELSE
        Console.WriteString("error : ");
      END;
      Console.WriteString(desc.msg);
      Console.WriteString(" </description> ");  Console.WriteLn; 
      IF isWarn THEN
        Console.WriteString("  </warning> "); 
      ELSE
        Console.WriteString("  </error> "); 
      END;
      Console.WriteLn; 
    END DisplayXML;

(* ============================================================ *)

    PROCEDURE PrintLine(n : INTEGER; IN l : ARRAY OF CHAR);
    BEGIN
      GPText.WriteInt(Scnr.lst, n, 4); GPText.Write(Scnr.lst, " ");
      GPText.WriteString(Scnr.lst, l); GPText.WriteLn(Scnr.lst);
    END PrintLine;

(* ============================================================ *)

    PROCEDURE DisplayLn(n : INTEGER; IN l : ARRAY OF CHAR);
    BEGIN
      Console.WriteInt(n, 4); Console.Write(" ");
      Console.WriteString(l); Console.WriteLn;
    END DisplayLn;

(* ============================================================ *)

    PROCEDURE PrintListing*(list : BOOLEAN);
    (** Print a source listing with error messages *)
      VAR
        nextErr : Err;		(* next error descriptor *)
	nextLin : INTEGER;	(* line num of nextErr   *)
        eof     : BOOLEAN;	(* end of file found     *)
        lnr     : INTEGER;	(* current line number   *)
	errC    : INTEGER;	(* current error index   *)
        srcPos  : INTEGER;	(* postion in sourceFile *)
        line    : ARRAY listingWidth OF CHAR;
      BEGIN
        IF xmlErrors THEN DisplayXMLHeader(); END;
	nextLin := 0;
	IF eTide > 0 THEN QuickSort(0, eTide-1) END;
	IF list THEN
	  GPText.WriteString(Scnr.lst, "Listing:");
	  GPText.WriteLn(Scnr.lst); GPText.WriteLn(Scnr.lst);
	END;
        srcPos := 0; nextErr := eBuffer[0];
        GetLine(srcPos, line, eof); lnr := 1; errC := 0;
        WHILE ~ eof DO
	  IF nextErr # NIL THEN nextLin := nextErr.lin END;
	  IF list THEN PrintLine(lnr, line) END;
	  IF ~forVisualStudio & ~xmlErrors & (~list OR (lnr = nextLin)) THEN 
            DisplayLn(lnr, line) 
          END;
          WHILE (nextErr # NIL) & (nextErr.lin = lnr) DO
	    IF list THEN PrintErr(nextErr) END; 
            IF forVisualStudio THEN
              DisplayVS(nextErr);
            ELSIF xmlErrors THEN
              DisplayXML(nextErr);
            ELSE
	      Display(nextErr);
            END;
	    INC(errC);
            nextErr := eBuffer[errC];
          END;
          GetLine(srcPos, line, eof); INC(lnr);
        END;
        WHILE nextErr # NIL DO
          IF list THEN PrintErr(nextErr) END; 
          IF forVisualStudio THEN
            DisplayVS(nextErr);
          ELSE
	    Display(nextErr);
          END;
	  INC(errC);
          nextErr := eBuffer[errC];
        END;
(*
 *	IF list THEN
 *	  GPText.WriteLn(Scnr.lst);
 *     	  GPText.WriteInt(Scnr.lst, errC, 5); 
 *        GPText.WriteString(Scnr.lst, " error");
 *        IF errC # 1 THEN GPText.Write(Scnr.lst, "s") END;
 *        GPText.WriteLn(Scnr.lst); 
 *        GPText.WriteLn(Scnr.lst); 
 *        GPText.WriteLn(Scnr.lst);
 *	END;
 *)
        IF list THEN
	  GPText.WriteLn(Scnr.lst);
          GPText.WriteString(Scnr.lst, "There were: ");
          IF Scnr.errors = 0 THEN
            GPText.WriteString(Scnr.lst, "No errors");
          ELSE
       	    GPText.WriteInt(Scnr.lst, Scnr.errors, 0); 
            GPText.WriteString(Scnr.lst, " error");
            IF Scnr.errors # 1 THEN GPText.Write(Scnr.lst, "s") END;
          END;
          GPText.WriteString(Scnr.lst, ", and ");
          IF Scnr.warnings = 0 THEN
            GPText.WriteString(Scnr.lst, "No warnings");
          ELSE
       	    GPText.WriteInt(Scnr.lst, Scnr.warnings, 0); 
            GPText.WriteString(Scnr.lst, " warning");
            IF Scnr.warnings # 1 THEN GPText.Write(Scnr.lst, "s") END;
          END;
          GPText.WriteLn(Scnr.lst); 
          GPText.WriteLn(Scnr.lst); 
          GPText.WriteLn(Scnr.lst); 
        END;
        IF xmlErrors THEN DisplayXMLEnd(); END;
      END PrintListing;

      PROCEDURE ResetErrorList*();
      BEGIN
	eTide := 0;
	eBuffer[0] := NIL;
      END ResetErrorList;

(* ============================================================ *)

      PROCEDURE Init*; 
      BEGIN
	NEW(parsHdlr); Scnr.ParseErr := parsHdlr;
	NEW(semaHdlr); Scnr.SemError := semaHdlr;
      END Init;

(* ============================================================ *)

    PROCEDURE SetSrcNam* (IN nam : ARRAY OF CHAR);
    BEGIN
      GPText.Assign(nam,srcNam);
    END SetSrcNam;

(* ============================================================ *)
BEGIN
  NEW(eBuffer, 8); eBuffer[0] := NIL; eLimit := 7; eTide := 0;
  prompt := FALSE;
  nowarn := FALSE;
  forVisualStudio := FALSE;
END CPascalErrors.
(* ============================================================ *)
