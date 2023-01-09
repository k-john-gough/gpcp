
(* ============================================================ *)
(*  RefEmitUtil is the module which writes PE files using the   *)
(*  System.Reflection.Emit library                              *)
(*  Copyright (c) John Gough 2018.                              *)
(* ============================================================ *)
(* ============================================================ *)

MODULE RefEmitUtil;

  IMPORT 
        GPCPcopyright,
        Mu  := MsilUtil,
        Id  := IdDesc,
        Lv  := LitValue,
        Sy  := Symbols,
        Ty  := TypeDesc,
        Cs  := CompState,

        RflHlp := RefEmitHelpers,

        Sys    := "[mscorlib]System",
        SysRfl := "[mscorlib]System.Reflection",
        RflEmt := "[mscorlib]System.Reflection.Emit";

(* ============================================================ *)

  TYPE PeFile*    = POINTER TO RECORD (Mu.MsilFile)
                 (*   Fields inherited from MsilFile *
                  *   srcS* : LitValue.CharOpen; (* source file name   *)
                  *   outN* : LitValue.CharOpen; (* output file name   *)
                  *   proc* : ProcInfo;
                  *)
                      mBldr  : RflEmt.ModuleBuilder;   (* Private  *)
                      aBldr- : RflEmt.AssemblyBuilder; (* ReadOnly *)
                      cBldr- : RflEmt.TypeBuilder;     (* ReadOnly *)
                      ilGen- : RflEmt.ILGenerator;     (* ReadOnly *)
                    END;

(* ============================================================ *)

  TYPE PeLabel*   = POINTER TO RECORD (Mu.Label)
                      labl : RflEmt.Label;
                    END;

(* ============================================================ *)
(*                    Constructor Method                        *)
(* ============================================================ *)

  PROCEDURE newPeFile*(IN nam : ARRAY OF CHAR; isDll : BOOLEAN) : PeFile;
    VAR bldr : RflEmt.AssemblyBuilder;
        asNm : SysRfl.AssemblyName;
        name : Sys.String;          (* simple dst name *)
        bnDr : Sys.String;          (* binDir, or NIL  *)
        rslt : PeFile;
  BEGIN
   (*
    *   FIXME Maybe need to check if tgXtn reference need to be zapped?
    *   FIXME Maybe need to initialize RTS types?
    *)
    NEW(rslt);
    name := MKSTR(nam);
   (* 
    *  Make the destination filename and AssemblyName
    *)
    IF isDll THEN
      rslt.outN := BOX(nam + ".dll");
    ELSE
      rslt.outN := BOX(nam + ".exe");
    END;
    asNm := SysRfl.AssemblyName.init(name);
   (*
    *  Create the destination directory 
    *  name as native string - or NIL.
    *)
    bnDr := RflHlp.binDir();
   (*
    *  Now the AssemblyBuilder and ModuleBuilder
    *)
    rslt.aBldr := Sys.AppDomain.get_CurrentDomain().DefineDynamicAssembly(
                            asNm, RflEmt.AssemblyBuilderAccess.Save, bnDr);
    rslt.mBldr := rslt.aBldr.DefineDynamicModule(
                            name, MKSTR(rslt.outN^), Cs.debug);
    RETURN rslt;
  END newPeFile;

(* ============================================================ *)

  PROCEDURE (t : PeFile)fileOk*() : BOOLEAN;
  BEGIN
   (* For this emitter file is created at end. *)
    RETURN TRUE;
  END fileOk;

(* ============================================================ *)

  PROCEDURE (os : PeFile)MkNewProcInfo*(proc : Sy.Scope);
  BEGIN
    NEW(os.proc);
    Mu.InitProcInfo(os.proc, proc);
  END MkNewProcInfo;

(* ============================================================ *)

  PROCEDURE (os : PeFile)newLabel*() : Mu.Label;
    VAR rslt : PeLabel;
        ilLb : RflEmt.Label;
  BEGIN
    RETURN NIL;
    (*
    ASSERT(os.ilGen # NIL);
    NEW(rslt);
    ilLb := os.ilGen.DefineLabel();
    *)
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
END RefEmitUtil.
(* ============================================================ *)
(* ============================================================ *)
