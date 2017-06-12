(* ============================================================ *)
(*  PeUtil is the module which writes PE files using the        *)
(*  managed interface.                                          *)
(*  Copyright (c) John Gough 1999, 2000.                        *)
(* ============================================================ *)
(* ============================================================ *)
(*  THIS IS THE EMPTY VERSION, THAT IS REQUIRED TO BOOTSTRAP    *)
(*  THE JVM VERSION WITHOUT THE MSCORLIB ASSEMBLY AVAILABLE.    *)
(* ============================================================ *)
(* ============================================================ *)

MODULE PeUtil;

  IMPORT 
        GPCPcopyright,
        Mu  := MsilUtil,
        Id  := IdDesc,
        Lv  := LitValue,
        Sy  := Symbols,
        Ty  := TypeDesc;

(* ============================================================ *)

  TYPE PeFile*    = POINTER TO RECORD (Mu.MsilFile)
                 (*   Fields inherited from MsilFile *
                  *   srcS* : LitValue.CharOpen; (* source file name   *)
                  *   outN* : LitValue.CharOpen; (* output file name   *)
                  *   proc* : ProcInfo;
                  *)
                    END;

(* ============================================================ *)
(*                    Constructor Method                        *)
(* ============================================================ *)

  PROCEDURE newPeFile*(IN nam : ARRAY OF CHAR; isDll : BOOLEAN) : PeFile;
  BEGIN
    RETURN NIL;
  END newPeFile;

(* ============================================================ *)

  PROCEDURE (t : PeFile)fileOk*() : BOOLEAN;
  BEGIN
    RETURN FALSE;
  END fileOk;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkNewProcInfo*(proc : Sy.Scope);
  BEGIN
  END MkNewProcInfo;

(* ============================================================ *)

  PROCEDURE (os : PeFile)newLabel*() : Mu.Label;
  BEGIN
    RETURN NIL;
  END newLabel;

(* ============================================================ *)
(*                    Exported Methods                  *)
(* ============================================================ *)

  PROCEDURE (os : PeFile)MethodDecl*(attr : SET; proc : Id.Procs);
  END MethodDecl;

(* ============================================================ *)

  PROCEDURE (os : PeFile)ExternList*();
  END ExternList;

(* ============================================================ *)

  PROCEDURE (os : PeFile)DefLab*(l : Mu.Label);
  END DefLab;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)DefLabC*(l : Mu.Label; IN c : ARRAY OF CHAR);
  END DefLabC;

(* ============================================================ *)

  PROCEDURE (os : PeFile)Code*(code : INTEGER);
  END Code;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeI*(code,int : INTEGER);
  END CodeI;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeT*(code : INTEGER; type : Sy.Type);
  END CodeT;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeTn*(code : INTEGER; type : Sy.Type);
  END CodeTn;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeL*(code : INTEGER; long : LONGINT);
  END CodeL;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeR*(code : INTEGER; real : REAL);
  END CodeR;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeLb*(code : INTEGER; labl : Mu.Label);
  END CodeLb;

(* ============================================================ *)

  PROCEDURE (os : PeFile)StaticCall*(s : INTEGER; d : INTEGER);
  END StaticCall;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CodeS*(code : INTEGER; str : INTEGER);
  END CodeS;

(* ============================================================ *)

  PROCEDURE (os : PeFile)Try*();
  END Try;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)Catch*(proc : Id.Procs);
  END Catch;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CloseCatch*();
  END CloseCatch;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)CopyCall*(typ : Ty.Record);
  END CopyCall;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)PushStr*(IN str : ARRAY OF CHAR);
  END PushStr;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CallIT*(code : INTEGER; 
                                 proc : Id.Procs; 
                                 type : Ty.Procedure);
  END CallIT;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CallCT*(proc : Id.Procs; 
                                 type : Ty.Procedure);
  END CallCT;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CallDelegate*(typ : Ty.Procedure);
  END CallDelegate;

(* ============================================================ *)

  PROCEDURE (os : PeFile)PutGetS*(code : INTEGER;
                                  blk  : Id.BlkId;
                                  fId  : Id.VarId);
  END PutGetS;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)GetValObj*(code : INTEGER; ptrT : Ty.Pointer);
  END GetValObj;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)PutGetXhr*(code : INTEGER; 
                                    proc : Id.Procs; 
                                    locl : Id.LocId);
  END PutGetXhr;

(* -------------------------------------------- *)

  PROCEDURE (os : PeFile)PutGetF*(code : INTEGER;
                                  fId  : Id.FldId);
  END PutGetF;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE (os : PeFile)MkNewRecord*(typ : Ty.Record);
  END MkNewRecord;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE (os : PeFile)MkNewProcVal*(p : Sy.Idnt;   (* src Proc *)
                                       t : Sy.Type);  (* dst Type *)
  END MkNewProcVal;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CallSuper*(rTp : Ty.Record;
                                    prc : Id.PrcId);
  END CallSuper;

(* ============================================================ *)

  PROCEDURE (os : PeFile)InitHead*(rTp : Ty.Record;
                                   prc : Id.PrcId);
  END InitHead;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CopyHead*(typ : Ty.Record);
  END CopyHead;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MarkInterfaces*(IN seq : Sy.TypeSeq);
  END MarkInterfaces;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MainHead*(xAtt : SET);
  END MainHead;

  PROCEDURE (os : PeFile)SubSys*(xAtt : SET);
  END SubSys;

(* ============================================================ *)

  PROCEDURE (os : PeFile)StartBoxClass*(rec : Ty.Record;
                                        att : SET;
                                        blk : Id.BlkId);
  END StartBoxClass;


  PROCEDURE (os : PeFile)MainTail*();
  END MainTail;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : PeFile)MethodTail*(id : Id.Procs);
  END MethodTail;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : PeFile)ClinitTail*();
  END ClinitTail;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : PeFile)CopyTail*();
  END CopyTail;

(* ------------------------------------------------------------ *)

  PROCEDURE (os : PeFile)InitTail*(typ : Ty.Record);
  END InitTail;

(* ============================================================ *)

  PROCEDURE (os : PeFile)ClinitHead*();
  END ClinitHead;

(* ============================================================ *)

  PROCEDURE (os : PeFile)EmitField*(id : Id.AbVar; att : SET);
  END EmitField;

(* ============================================================ *)

  PROCEDURE (os : PeFile)EmitEventMethods*(id : Id.AbVar);
  END EmitEventMethods;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkAndLinkDelegate*(dl  : Sy.Idnt;
                                            id  : Sy.Idnt;
                                            ty  : Sy.Type;
                                            isA : BOOLEAN);
  END MkAndLinkDelegate;

(* ============================================================ *)
(* ============================================================ *)

  PROCEDURE (os : PeFile)EmitPTypeBody*(tId : Id.TypId);
  END EmitPTypeBody;

(* ============================================================ *)
(*          End of Procedure Variable and Event Stuff           *)
(* ============================================================ *)

  PROCEDURE (os : PeFile)Line*(nm : INTEGER),EMPTY;

(* ============================================================ *)

  PROCEDURE (os : PeFile)LoadType*(id : Sy.Idnt);
  END LoadType;

(* ============================================================ *)

  PROCEDURE (os : PeFile)Finish*();
  END Finish;

(* ============================================================ *)

  PROCEDURE (os : PeFile)RefRTS*();
  END RefRTS;

(* ============================================================ *)

  PROCEDURE (os : PeFile)StartNamespace*(nm : Lv.CharOpen);
  END StartNamespace;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkBodyClass*(mod : Id.BlkId);
  END MkBodyClass;

(* ============================================================ *)

  PROCEDURE (os : PeFile)ClassHead*(attSet : SET; 
                                    thisRc : Ty.Record;
                                    superT : Ty.Record);
  END ClassHead;

(* ============================================================ *)

  PROCEDURE (os : PeFile)CheckNestedClass*(typ : Ty.Record;
                                           scp : Sy.Scope;
                                           rNm : Lv.CharOpen);
  END CheckNestedClass;

(* ============================================================ *)

  PROCEDURE (os : PeFile)ClassTail*();
  END ClassTail;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkRecX*(t : Ty.Record; s : Sy.Scope);
  END MkRecX;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkPtrX*(t : Ty.Pointer);
  END MkPtrX;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkArrX*(t : Ty.Array);
  END MkArrX;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkBasX*(t : Ty.Base);
  END MkBasX;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkEnuX*(t : Ty.Enum; s : Sy.Scope);
  END MkEnuX;

(* ============================================================ *)

  PROCEDURE (os : PeFile)NumberParams*(pId : Id.Procs; 
                                       pTp : Ty.Procedure);
  END NumberParams;
 
(* ============================================================ *)

  PROCEDURE (os : PeFile)SwitchHead*(num : INTEGER);
  END SwitchHead;

  PROCEDURE (os : PeFile)SwitchTail*();
  END SwitchTail;

  PROCEDURE (os : PeFile)LstLab*(l : Mu.Label);
  END LstLab;

(* ============================================================ *)
(* ============================================================ *)
END PeUtil.
(* ============================================================ *)
(* ============================================================ *)

