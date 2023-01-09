
(* ================================================= *) 
(*   This module provides symbol tables for Browse   *)
(*   Copyright (c) John Gough 2018                   *)
(* ================================================= *)

MODULE BrowseLookup;
  IMPORT
    RTS,
    Nh := NameHash;
    

(* ============================================================ *)

  TYPE  
    DescBase* = POINTER TO ABSTRACT RECORD name* : RTS.CharOpen END;

  TYPE  (* Symbol tables are implemented by a binary tree *)
    SymInfo = POINTER TO RECORD         (* private stuff  *)
                key : INTEGER;          (* hash key value *)
                val : DescBase;         (* descriptor obj *)
                lOp : SymInfo;          (* left child     *)
                rOp : SymInfo;          (* right child    *)
              END;

    SymbolTable* = RECORD
                     root : SymInfo;
                   END;

(* ============================================================ *)
(*  Private methods of the symbol-table info-blocks             *)
(* ============================================================ *)

  PROCEDURE mkSymInfo(h : INTEGER; d : DescBase) : SymInfo;
    VAR rtrn : SymInfo;
  BEGIN
    NEW(rtrn); rtrn.key := h; rtrn.val := d; RETURN rtrn;
  END mkSymInfo;

(* -------------------------------------------- *)

  PROCEDURE (i : SymInfo)lookup(key : INTEGER) : DescBase,NEW;
  BEGIN
    IF key < i.key THEN
      IF i.lOp = NIL THEN RETURN NIL ELSE RETURN i.lOp.lookup(key) END;
    ELSIF key > i.key THEN
      IF i.rOp = NIL THEN RETURN NIL ELSE RETURN i.rOp.lookup(key) END;
    ELSE (* key must equal i.key *)
      RETURN i.val;
    END;
  END lookup;

(* -------------------------------------------- *)

  PROCEDURE (i : SymInfo)enter(h : INTEGER; d : DescBase) : BOOLEAN,NEW;
  BEGIN
    IF h < i.key THEN
      IF i.lOp = NIL THEN i.lOp := mkSymInfo(h,d); RETURN TRUE;
      ELSE RETURN i.lOp.enter(h,d);
      END;
    ELSIF h > i.key THEN
      IF i.rOp = NIL THEN i.rOp := mkSymInfo(h,d); RETURN TRUE;
      ELSE RETURN i.rOp.enter(h,d);
      END;
    ELSE (* h must equal i.key *) RETURN FALSE;
    END;
  END enter;

(* -------------------------------------------- *)

  PROCEDURE (i : SymInfo)write(h : INTEGER; d : DescBase) : SymInfo,NEW;
    VAR rtrn : SymInfo;
  BEGIN
    rtrn := i;      (* default: return self *)
    IF    h < i.key THEN i.lOp := i.lOp.write(h,d);
    ELSIF h > i.key THEN i.rOp := i.rOp.write(h,d);
    ELSE  rtrn.val := d;
    END;
    RETURN rtrn;
  END write;

(* -------------------------------------------- *)
(*              Exported Procedures             *)
(* -------------------------------------------- *)

  PROCEDURE (IN tbl : SymbolTable)lookup*(str : RTS.CharOpen) : DescBase,NEW;
  BEGIN
    IF tbl.root = NIL THEN 
      RETURN NIL;
    ELSE 
      RETURN tbl.root.lookup(Nh.enterStr(str));
    END;
  END lookup;

(* -------------------------------------------- *)

  PROCEDURE (VAR tbl : SymbolTable)Overwrite*(str : RTS.CharOpen; new : DescBase),NEW;
  BEGIN
    tbl.root := tbl.root.write(Nh.enterStr(str), new);
  END Overwrite;

(* -------------------------------------------- *)

  PROCEDURE (VAR s : SymbolTable)enter*(str : RTS.CharOpen; v : DescBase) : BOOLEAN,NEW;
  (* Enter value in SymbolTable; Return value signals successful insertion. *)
  BEGIN
    IF s.root = NIL THEN
      s.root := mkSymInfo(Nh.enterStr(str), v); RETURN TRUE;
    ELSE
      RETURN s.root.enter(Nh.enterStr(str), v);
    END;
  END enter;

(* -------------------------------------------- *)
END BrowseLookup.
(* -------------------------------------------- *)

