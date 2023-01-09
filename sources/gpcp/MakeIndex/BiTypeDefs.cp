
MODULE BiTypeDefs;
  IMPORT RTS;

(* ============================================================ *)

  TYPE
    CharOpen* = POINTER TO ARRAY OF CHAR;

(* ============================================================ *)

  TYPE
    FileDescriptor* = 
        POINTER TO RECORD
          name* : CharOpen;
          prefix* : CharOpen;
          dotNam* : CharOpen;
          pkgDepth* : INTEGER;
        END;

(* ============================================================ *)

END BiTypeDefs.

