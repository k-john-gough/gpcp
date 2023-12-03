

(* ============================================================ *)
(*  AsmFrames is the module which provides all the methods      *)
(*  associated with the MethodFrame and FrameElement types.     *)
(*  Copyright (c) John Gough 2016-2017.                         *)
(* ============================================================ *)

(* ============================================================ *
 *
 *  About stack frame emission.
 *  At various points in the code, for Java SE 7+ a stack frame
 *  marker is emitted to allow the new Prolog-based verifier to
 *  run efficiently. The stack frames are emitted in a rather
 *  tricky compressed form which, when expanded, gives the 
 *  evaluation stack and local variable state at the corresponding 
 *  code point.
 *
 *  Each element in the stack and locals is given a "verification
 *  type". These are values from a type-lattice.
 *
 *  The way that this compiler generates the stack frames is to
 *  maintain a shadow stack which is updated after the emission
 *  of each instruction. At control-flow merge points the type
 *  is the join-type of all the paths merging at that point.
 *
 *  The initial state of the shadow stack is determined from the
 *  type and mode of the formal parameters. The initial state of
 *  the locals requires that even allocated locals have type
 *  "TOP" until a definite assignment is made to the local slot.
 *  This requirement is met by storing the future type of the 
 *  slot and copying to the locals stack on encountering a store
 *  to that location.
 *
 *  It is a requirement that stack frames cannot be at the
 *  beginning of method code, nor can they be adjacent without
 *  at least one intervening instruction. The implementation of
 *  this requirement is clunky, since GPCP generates code that
 *  may have multiple target labels that are adjacent (when 
 *  statements are deeply nested, for example). The clunky 
 *  algorithm for this is that each label carries with it the 
 *  verification types of its jump-off locations. As each 
 *  label-definition is processed the merged state is updated 
 *  but no frame is emitted. Only when the emission of a
 *  non-label "instruction" is encountered is the latest merged 
 *  state emitted in a stack frame.
 *
 * ============================================================ *)

MODULE AsmFrames;
  IMPORT 
    RTS,
    Jvm := JVMcodes,
    Sym := Symbols,
    Blt := Builtin,
    CSt := CompState,
    Id  := IdDesc,
    Ty  := TypeDesc,
    Ju  := JavaUtil,
    Lv  := LitValue,

    JL  := java_lang,

    Hlp := AsmHelpers,
    Acs := AsmCodeSets,
    Def := AsmDefinitions,
    ASM := org_objectweb_asm;

(* ================================================= *)
(*     Types used for manipulation of stack frames   *)
(* ================================================= *)

 (* ----------------------------------------------- *)
 (* Element type of the locals frame vector --      *)
 (* ----------------------------------------------- *)
 (* 'state' asserts the state of the local:         *)
 (*   TOP  - unknown type of value                  *)
 (*   some java/lang/Integer denoting a basic type  *)
 (*   some java/lang/String denoting a typename.    *) 
 (*   NULL - value known to be nil                  *)
 (* 'dclTp' denotes the CP declared static typename *)
 (*   for a local of some reference type, after the *)
 (*   local has had a definite assignment the dclTp *)
 (*   is copied to the state field.                 *)
 (* ----------------------------------------------- *)
  TYPE FrameElement* = 
    RECORD 
      state* : RTS.NativeObject;
      dclTp* : RTS.NativeObject;  (* "state" in waiting *)
    END;

  TYPE FrameVector* = VECTOR OF FrameElement;

  TYPE FrameSave* = POINTER TO RECORD
                      lDclArr : Def.JloArr;
                      loclArr : Def.JloArr;
                      evalArr : Def.JloArr;
                    END;

 (* ----------------------------------------------- *)
 (* Element type of the eval stack vector --        *)
 (* ----------------------------------------------- *)
 (* Each item on the stack notes the stack state    *)
 (* of the eval-stack element at that position.     *)
 (* ----------------------------------------------- *)
  TYPE StackVector* = VECTOR OF RTS.NativeObject;

 (* ----------------------------------------------- *)
 (* MethodFrames simulate locals and eval-stack     *)
 (* ----------------------------------------------- *)
 (* localStack holds declared local variables,      *)
 (* initially with their statically declared types  *)
 (* memorized in the dclTp field. State fields of   *)
 (* all live elements of localStack are emitted by  *)
 (* every mv.visitFrame() call.                     *)
 (* ----------------------------------------------- *)
 (* evalStack tracks the current contents of the    *)
 (* evaluation stack of the JVM at each program     *)
 (* point. Live elements of the evalStack are       *)
 (* emitted by every mv.visitFrame() call.          *)
 (* ----------------------------------------------- *)
 (* Type-2 elements (LONG and DOUBLE) take up two   *)
 (* slots in the localStack array, with the second  *)
 (* being a dummy to preserve the mapping to local  *)
 (* variable ordinal number. These dummies are      *)
 (* skipped over in the creation of object arrays   *)
 (* used in the visitFrame instruction.             *)
 (* ----------------------------------------------- *)
  TYPE MethodFrame* = 
    POINTER TO RECORD
      procName-   : Lv.CharOpen;
      descriptor- : Id.Procs;
      signature-  : RTS.NativeString;
      localStack* : FrameVector;
      evalStack*  : StackVector;
      maxEval-    : INTEGER;
      maxLocal-   : INTEGER;
      sigMax-     : INTEGER; (* top of initialized locals *)
      undefEval   : BOOLEAN;
    END;

  VAR dummyElement- : FrameElement; (* {TOP,NIL} *)

(* ================================================ *)
(* ================================================ *)
(*          Forward Procedure Declarations          *)
(* ================================================ *)
(* ================================================ *)

  PROCEDURE^ ( mFrm : MethodFrame )EvLn*() : INTEGER,NEW;
  PROCEDURE^ ( mFrm : MethodFrame )EvHi*() : INTEGER,NEW;
  PROCEDURE^ ( mFrm : MethodFrame )LcLn*() : INTEGER,NEW;
  PROCEDURE^ ( mFrm : MethodFrame )LcHi*() : INTEGER,NEW;
  PROCEDURE^ ( mFrm : MethodFrame )LcCount*() : INTEGER,NEW;
  PROCEDURE^ ( mFrm : MethodFrame )EvCount*() : INTEGER,NEW;
  PROCEDURE^ ( mFrm : MethodFrame )AddLocal*(type : Sym.Type),NEW;
  PROCEDURE^ ( mFrm : MethodFrame )AddValParam*(type : Sym.Type),NEW;
  PROCEDURE^ ( mFrm : MethodFrame )AddRefParam*(str : Lv.CharOpen),NEW;
(*
 *PROCEDURE^ ( mFrm : MethodFrame )DiagFrame*( ),NEW;
 *PROCEDURE^ ( mFrm : MethodFrame )DiagEvalStack*(),NEW;
 *PROCEDURE^ ( mFrm : MethodFrame )Diag*(code : INTEGER),NEW;
 *PROCEDURE^ ( fs : FrameSave )Diag*( ),NEW;
 *)
  PROCEDURE^ ( mFrm : MethodFrame )GetLocalArrStr*() : RTS.NativeString,NEW;
  PROCEDURE^ ( mFrm : MethodFrame )GetDclTpArrStr*() : RTS.NativeString,NEW;
  PROCEDURE^ ( mFrm : MethodFrame )GetEvalArrStr*() : RTS.NativeString,NEW;
  PROCEDURE^ ( fs : FrameSave )EvLn*( ) : INTEGER,NEW;
  PROCEDURE^ ( fs : FrameSave )EvHi*( ) : INTEGER,NEW;
  PROCEDURE^ ( fs : FrameSave )LcLn*( ) : INTEGER,NEW;
  PROCEDURE^ ( fs : FrameSave )LcHi*( ) : INTEGER,NEW;

(* ================================================= *)
(* ================================================= *)
(*        Static procedures for MethodFrames         *)
(* ================================================= *)
(* ================================================= *)

 (* ----------------------------------------------- *)
 (*   Parse a signature and initialize the Frame    *)
 (* ----------------------------------------------- *)
  PROCEDURE ParseSig( frm : MethodFrame; sig : Lv.CharOpen );
    VAR cIdx : INTEGER;
        cVal : CHAR;
        cVec : Lv.CharVector;
  BEGIN
    NEW( cVec, 32 );
    cIdx := 0; cVal := sig[cIdx];
    LOOP 
      CASE cVal OF
      | '(' : (* skip *);
      | ')' : RETURN;

      | 'I', 'B', 'C', 'S', 'Z' : 
             (* 
              *  All 32-bit ordinals get integer frame state 
              *)
              frm.AddValParam( Blt.intTp );  (* int      *)
      | 'J' : frm.AddValParam( Blt.lIntTp ); (* long int *)
      | 'F' : frm.AddValParam( Blt.sReaTp ); (* float    *)
      | 'D' : frm.AddValParam( Blt.realTp ); (* double   *)

      | 'L' : (* binary class name *)
              CUT(cVec, 0);
              APPEND( cVec, cVal );
              REPEAT
                INC( cIdx ); cVal := sig[cIdx]; (* Get Next *)
                APPEND( cVec, cVal );
              UNTIL cVal = ';';
              frm.AddRefParam( Lv.chrVecToCharOpen( cVec ) );

      | '[' : (* binary array name *)
              CUT(cVec, 0);
              WHILE cVal = '[' DO
                APPEND( cVec, cVal );
                INC( cIdx ); cVal := sig[cIdx]; (* Get Next *)
              END;
              APPEND( cVec, cVal ); 
             (* if this was an 'L' continue until ';' *)
              IF cVal = 'L' THEN
                REPEAT
                  INC( cIdx ); cVal := sig[cIdx]; (* Get Next *)
                  APPEND( cVec, cVal );
                UNTIL cVal = ';';
              END;
              frm.AddRefParam( Lv.chrVecToCharOpen( cVec ) );

      (* else CASEtrap *)
      END;
      INC( cIdx ); cVal := sig[cIdx]; (* Get Next *)
    END; (* LOOP *)
  END ParseSig; 

 (* ----------------------------------------------- *)
 (*             Add Proxies to the frame            *)
 (* ----------------------------------------------- *)
  PROCEDURE AddProxies( frm : MethodFrame; prc : Id.Procs );
    VAR idx  : INTEGER;
        pars : Id.ParSeq;
        parX : Id.ParId;
  BEGIN
   (* ------------------ *
    *  Allocate a method frame slot for the XHR if this is needed.  
    * ------------------ *)
    IF Id.hasXHR IN prc.pAttr THEN
        frm.AddLocal( prc.xhrType );
    END; 
    
    pars := prc.type(Ty.Procedure).formals;
    FOR idx := 0 TO pars.tide-1 DO
      parX := pars.a[idx];
      IF parX.varOrd > frm.LcHi() THEN
        frm.AddLocal( parX.type );
      END;
    END;
  END AddProxies;
 
 (* ----------------------------------------------- *)
 (*             Add locals to the frame             *)
 (* ----------------------------------------------- *)
  PROCEDURE AddLocals( frm : MethodFrame; prc : Id.Procs );
    VAR idx  : INTEGER;
        locs : Sym.IdSeq;
        locX : Sym.Idnt;
  BEGIN
    locs := prc.locals;
    FOR idx := 0 TO locs.tide-1 DO
      locX := locs.a[idx];
      WITH locX : Id.ParId DO (* ignore *)
      | locX : Id.LocId DO
          frm.AddLocal( locX.type );
      END;
    END;
  END AddLocals;
 
 (* ----------------------------------------------- *)
 (*  Create a new MethodFrame for procedure procId  *)
 (* ----------------------------------------------- *)
  PROCEDURE NewMethodFrame*( procId : Id.Procs) : MethodFrame;
    VAR rslt : MethodFrame;
        sigL : Lv.CharOpen;
  BEGIN
    sigL := procId.type(Ty.Procedure).xName;
    NEW( rslt );
    NEW( rslt.localStack , 8 );
    NEW( rslt.evalStack , 8 );
    rslt.procName   := procId.prcNm;
    rslt.descriptor := procId;
    rslt.signature  := MKSTR( sigL^ );
    rslt.maxEval    := 0;
    rslt.maxLocal   := procId.rtsFram;
    rslt.undefEval  := FALSE;
   (*
    *  Allocate slot 0 for the receiver, if any
    *)
    IF procId IS Id.MthId THEN
      rslt.AddRefParam( procId(Id.MthId).bndType.xName );
    END;
   (*
    *  Now load up the frame with the param info.
    *  All of these are initialized frame elements.
    *)
    ParseSig( rslt, sigL );
    rslt.sigMax := LEN(rslt.localStack);
   (*
    *  And now with the local variable info
    *  All of these are uninitialized frame elements.
    *)
    AddProxies( rslt, procId );
    AddLocals( rslt, procId );

    RETURN rslt;
  END NewMethodFrame;

 (* ----------------------------------------------- *)
 (*   Create a MethodFrame for a known procedure    *)
 (* ----------------------------------------------- *)
  PROCEDURE SigFrame*( sigL, name, recv : Lv.CharOpen ) : MethodFrame;
    VAR rslt : MethodFrame;
  BEGIN
    NEW( rslt );
    NEW( rslt.localStack , 8 );
    NEW( rslt.evalStack , 8 );
    rslt.maxEval := 0;
    rslt.maxLocal := 1;
    rslt.procName := name;
    IF recv # NIL THEN
      rslt.AddRefParam( recv );
    END;
    ParseSig( rslt, sigL );
    RETURN rslt;
  END SigFrame;

 (* ----------------------------------------------- *
  *  FrameElements get initialized in several contexts:  
  *   (1) uninitialized locals of any type
  *        This case should get dclTp <== sigStr or INTEGER etc.,
  *        and state <== ASM.Opcodes.TOP.
  *   (2) initialized params of reference type
  *        This case should get dclTp <== sigStr,
  *        and state <== sigStr. This should be
  *        true even in the case of out params.
  *   (3) locals of basic, value types
  *        This case should get dclTp <== ASM.Opcodes.INTEGER
  *        etc. and state <== ASM.Opcodes.TOP
  * ----------------------------------------------- *)

 (* ----------------------------------------------- *)
 (*  Create a FrameElement, reflecting the given    *)
 (*  type. Determine is a dummy second slot needed. *)
 (* ----------------------------------------------- *)
  PROCEDURE Uninitialized( type : Sym.Type; 
                       OUT twoE : BOOLEAN;
                       OUT elem : FrameElement );
  BEGIN
    twoE := FALSE;
    elem.state := ASM.Opcodes.TOP; 
    Hlp.EnsureTypName( type );
    IF (type.kind = Ty.basTp) OR (type.kind = Ty.enuTp) THEN
      CASE (type(Ty.Base).tpOrd) OF
      | Ty.boolN .. Ty.intN, Ty.setN, Ty.uBytN :
          elem.dclTp := ASM.Opcodes.INTEGER;
      | Ty.sReaN :
          elem.dclTp := ASM.Opcodes.FLOAT;
      | Ty.anyRec .. Ty.sStrN :
          elem.dclTp := ASM.Opcodes.TOP;  (* ???? *)
      | Ty.lIntN :
          elem.dclTp := ASM.Opcodes.LONG; twoE := TRUE; 
      | Ty.realN :
          elem.dclTp := ASM.Opcodes.DOUBLE; twoE := TRUE;
      END;
    ELSE
      elem.dclTp := MKSTR( type.xName^ );
    END;
  END Uninitialized;

 (* ----------------------------------------------- *)
 (*  Create a single-slot FrameElement with given   *)
 (*  signature. This is marked as initialized.      *)
 (* ----------------------------------------------- *)
  PROCEDURE ParamRefElement( sig  : Lv.CharOpen;
                         OUT elem : FrameElement);
  BEGIN
    elem.dclTp := MKSTR( sig^ );
    elem.state := elem.dclTp;
  END ParamRefElement;

 (* ----------------------------------------------- *)
  PROCEDURE MkFrameSave( loclLen, evalLen : INTEGER ) : FrameSave;
    VAR rslt : FrameSave;
  BEGIN
    NEW( rslt );
    IF loclLen > 0 THEN 
      NEW( rslt.loclArr, loclLen );
      NEW( rslt.lDclArr, loclLen );
    ELSE
      rslt.loclArr := NIL;
      rslt.lDclArr := NIL;
    END;
    IF evalLen > 0 THEN 
      NEW( rslt.evalArr, evalLen );
    ELSE
      rslt.evalArr := NIL;
    END;
    RETURN rslt;
  END MkFrameSave;

  PROCEDURE ( old : FrameSave)Shorten( newLen : INTEGER ) : FrameSave,NEW;
    VAR new : FrameSave;
        int : INTEGER;
  BEGIN
    ASSERT( LEN( old.loclArr ) >= newLen );
    NEW( new );
    new.evalArr := old.evalArr;
    NEW( new.loclArr, newLen );
    NEW( new.lDclArr, newLen );
    FOR int := 0 TO newLen - 1 DO
      new.loclArr[int] := old.loclArr[int];
      new.lDclArr[int] := old.lDclArr[int];
    END;
    RETURN new;
  END Shorten;

(* ================================================ *)
(* ================================================ *)
(*       Typebound methods for MethodFrames         *)
(* ================================================ *)
(* ================================================ *)

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)isDummyElem(ix:INTEGER) : BOOLEAN,NEW;
  BEGIN
   (*
    * (dclTp = NIL) ==> 2nd element of type-1 element.
    * However if state of previous element is TOP
    * then visitFrame will need to emit TOP, TOP.
    *)
    RETURN (mFrm.localStack[ix].dclTp = NIL) &
           (mFrm.localStack[ix - 1].state # ASM.Opcodes.TOP);
  END isDummyElem;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)LcHi*() : INTEGER,NEW;
  BEGIN
    RETURN LEN( mFrm.localStack ) - 1;
  END LcHi;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)LcLn*() : INTEGER,NEW;
  BEGIN
    RETURN LEN( mFrm.localStack );
  END LcLn;

 (* --------------------------------------------- *)
  PROCEDURE ( fs : FrameSave )LcLn( ) : INTEGER,NEW;
  BEGIN
    IF fs.loclArr = NIL THEN RETURN 0 END;
    RETURN LEN( fs.loclArr );
  END LcLn;

 (* --------------------------------------------- *)
  PROCEDURE ( fs : FrameSave )LcHi( ) : INTEGER,NEW;
  BEGIN
    IF fs.loclArr = NIL THEN RETURN -1 END;
    RETURN LEN( fs.loclArr ) - 1;
  END LcHi;

 (* --------------------------------------------- *)
 (* LcLn gives the *length* of the local array    *)
 (* LcHi gives the *hi* index of the local array  *)
 (* LcCount gives the number of logical elements  *)
 (* However, uninitialized type-2s count as two.  *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)LcCount*() : INTEGER,NEW;
    VAR count, index : INTEGER;
  BEGIN
    count := 0;
    FOR index := 0 TO mFrm.LcHi() DO 
      IF ~mFrm.isDummyElem(index) THEN INC(count) END;
    END; 
    RETURN count;
  END LcCount;

 (* --------------------------------------------- *)
  PROCEDURE ( fs : FrameSave )EvLn( ) : INTEGER,NEW;
  BEGIN
    IF fs.evalArr = NIL THEN RETURN 0 END;
    RETURN LEN( fs.evalArr );
  END EvLn;

 (* --------------------------------------------- *)
  PROCEDURE ( fs : FrameSave )EvHi( ) : INTEGER,NEW;
  BEGIN
    IF fs.evalArr = NIL THEN RETURN -1 END;
    RETURN LEN( fs.evalArr ) - 1;
  END EvHi;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)EvHi*() : INTEGER,NEW;
  BEGIN
    RETURN LEN( mFrm.evalStack ) - 1;
  END EvHi;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)EvLn*() : INTEGER,NEW;
  BEGIN
    RETURN LEN( mFrm.evalStack );
  END EvLn;

 (* --------------------------------------------- *)
 (* EvLn gives the *length* of the eval array     *)
 (* EvHi gives the *hi* index of the eval array   *)
 (* EvCount gives the number of logical elements  *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)EvCount*() : INTEGER,NEW;
    VAR count, index, evHi : INTEGER;
        elem : RTS.NativeObject;
  BEGIN
    count := 0;
    index := 0;
    evHi  := mFrm.EvHi();
    WHILE index <= evHi DO
      INC(count);
      elem := mFrm.evalStack[index];
      IF (elem = ASM.Opcodes.LONG) OR
         (elem = ASM.Opcodes.DOUBLE) THEN
        INC(index);
      END;
      INC(index); 
    END;
    RETURN count;
  END EvCount;

 (* --------------------------------------------- *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)setDepth*( i : INTEGER ),NEW;
    VAR len : INTEGER;
  BEGIN
    len := LEN( mFrm.evalStack );
    IF len > i THEN
      CUT( mFrm.evalStack, i );
    ELSE
      WHILE len < i DO 
        APPEND( mFrm.evalStack, NIL ); INC(len);
      END;
    END;
  END setDepth;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)InvalidStack*() : BOOLEAN,NEW;
  BEGIN
    RETURN mFrm.undefEval;
  END InvalidStack;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)InvalidateEvalStack*(),NEW;
  BEGIN
    mFrm.undefEval := TRUE;
  END InvalidateEvalStack;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)ValidateEvalStack*(),NEW;
  BEGIN
    mFrm.undefEval := FALSE;
  END ValidateEvalStack;

 (* --------------------------------------------- *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)DeltaEvalDepth*( d : INTEGER ),NEW;
    VAR new : INTEGER;
  BEGIN
    CASE d OF
      0 : (* skip *)
    | 1 : APPEND( mFrm.evalStack, NIL );
    | 2 : APPEND( mFrm.evalStack, NIL ); 
          APPEND( mFrm.evalStack, NIL );
    | 3 : APPEND( mFrm.evalStack, NIL );
          APPEND( mFrm.evalStack, NIL );
          APPEND( mFrm.evalStack, NIL );
    ELSE
      IF d > 0 THEN 
        CSt.Message( "Delta too big: " + Ju.i2CO(d)^ );
      ELSIF (mFrm.EvLn()) < -d THEN
        CSt.Message( "Delta too neg: " + Ju.i2CO(d)^ +
                     " current count: " + Ju.i2CO(mFrm.EvLn() )^);
      END;

      ASSERT(d<0);
      ASSERT((mFrm.EvLn() + d) >= 0);
      CUT( mFrm.evalStack, mFrm.EvLn() + d );
    END;
    IF d > 0 THEN 
      mFrm.maxEval := MAX( mFrm.maxEval, mFrm.EvLn() );
    END;
  END DeltaEvalDepth;

 (* --------------------------------------------- *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)SetTosType*( t : Sym.Type ),NEW;
    VAR tIx : INTEGER;
        obj : RTS.NativeObject;
  BEGIN
    tIx := mFrm.EvHi();
    obj := Hlp.TypeToObj( t );

    IF t.isLongType() THEN
      ASSERT( tIx >= 1 );
      mFrm.evalStack[tIx - 1] := obj;
      mFrm.evalStack[tIx] := ASM.Opcodes.TOP;
    ELSE
      ASSERT( tIx >= 0 );
      mFrm.evalStack[tIx] := obj;
    END;
  END SetTosType;

 (* --------------------------------------------- *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)SetTosSig*( sig : RTS.NativeString ),NEW;
    VAR tIx : INTEGER;
  BEGIN
    tIx := mFrm.EvHi();
    ASSERT( tIx >= 0 );
    mFrm.evalStack[tIx] := Hlp.SigToObj( sig );
  END SetTosSig;

 (* --------------------------------------------- *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)SetTosState*( bSig : Lv.CharOpen ),NEW;
  BEGIN
    mFrm.SetTosSig( MKSTR( bSig^ ) );
  END SetTosState;

 (* --------------------------------------------- *)
 (*  Adjust the eval stack depth and/or state.    *)
 (*  Most instructions can use this, but a few,   *)
 (*  such as the invoke* and multianewarray need  *)
 (*  varying extra data, and are treated uniquely *)
 (*  Also, long values (long, double) excluded.   *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)FixEvalSig*(
                  code : INTEGER; 
                  sig  : RTS.NativeString ),NEW;
  BEGIN
    ASSERT( ~Acs.badFix( code ) ); (* ==> fixed delta *)
    IF code = Jvm.opc_goto THEN 
      mFrm.InvalidateEvalStack(); 
    ELSE
      mFrm.DeltaEvalDepth( Jvm.dl[code] );
      IF sig # NIL THEN 
        mFrm.SetTosSig( sig );
      END;
    END;
  END FixEvalSig;

  (* 
   *  Used after various typed *load instructions,
   *  typed *store instructions, ldc variants, jumps,
   *  new, newarray, anewarray.
   *)
  PROCEDURE (mFrm : MethodFrame)FixEvalStack*(
                  code : INTEGER; 
                  type : Sym.Type ),NEW;
  BEGIN
    ASSERT( ~Acs.badFix( code ) ); (* ==> fixed delta *)
    IF code = Jvm.opc_goto THEN 
      mFrm.InvalidateEvalStack(); 
    ELSE
      mFrm.DeltaEvalDepth( Jvm.dl[code] );
      IF type # NIL THEN 
        mFrm.SetTosType( type );
      END;
    END;
  END FixEvalStack;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)PutGetFix*(
                  code : INTEGER; type : Sym.Type ),NEW;
    VAR jvmSize : INTEGER;
  BEGIN

    jvmSize := Ju.jvmSize(type);
    CASE code OF
    | Jvm.opc_getfield  :                    (* t1 or t2 *) 
        mFrm.DeltaEvalDepth( jvmSize - 1 );  (*  0 or 1  *)
        mFrm.SetTosType( type );
    | Jvm.opc_putfield  :
        mFrm.DeltaEvalDepth( -jvmSize - 1 ); (* -2 or -3 *)
    | Jvm.opc_getstatic :
        mFrm.DeltaEvalDepth( jvmSize );      (*  1 or 2  *)
        mFrm.SetTosType( type );
    | Jvm.opc_putstatic :
        mFrm.DeltaEvalDepth( -jvmSize );     (* -1 or -2 *)
    END;
  END PutGetFix;

 (* --------------------------------------------- *)
 (*    Infer new TOS and mutate evalStack state   *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)MutateEvalStack*( code : INTEGER ),NEW;
    VAR delta : INTEGER;
        tosHi : INTEGER;
        tmpOb : RTS.NativeObject;
        tmpSt : RTS.NativeString;
        tempS : RTS.NativeObject;
  BEGIN
    delta := Jvm.dl[code];
    mFrm.DeltaEvalDepth( delta );
    tosHi := mFrm.EvHi();

    CASE code OF 

    | Jvm.opc_nop,       (* All of these discard the TOS, or    *)
      Jvm.opc_pop,       (* leave the TOS state equal to that   *) 
      Jvm.opc_pop2,      (* of (one of) the incoming operand(s) *)
      Jvm.opc_iastore,
      Jvm.opc_lastore,
      Jvm.opc_fastore,
      Jvm.opc_dastore,
      Jvm.opc_aastore,
      Jvm.opc_bastore,
      Jvm.opc_castore,
      Jvm.opc_sastore,
      Jvm.opc_lneg,
      Jvm.opc_ineg,
      Jvm.opc_fneg,
      Jvm.opc_dneg:
         (* skip *)

    | Jvm.opc_goto,
      Jvm.opc_ireturn,
      Jvm.opc_lreturn,
      Jvm.opc_freturn,
      Jvm.opc_dreturn,
      Jvm.opc_areturn,
      Jvm.opc_return,
      Jvm.opc_athrow :
          mFrm.undefEval := TRUE;

    | ASM.Opcodes.ACONST_NULL :
          mFrm.evalStack[tosHi] := ASM.Opcodes.NULL;

    | ASM.Opcodes.ICONST_M1 ,
      ASM.Opcodes.ICONST_0 ,
      ASM.Opcodes.ICONST_1 ,
      ASM.Opcodes.ICONST_2 ,
      ASM.Opcodes.ICONST_3 ,
      ASM.Opcodes.ICONST_4 ,
      ASM.Opcodes.ICONST_5 ,
      ASM.Opcodes.IALOAD ,
      ASM.Opcodes.BALOAD ,
      ASM.Opcodes.CALOAD ,
      ASM.Opcodes.SALOAD ,
      ASM.Opcodes.IADD ,

      ASM.Opcodes.ISUB,
      ASM.Opcodes.IMUL,
      ASM.Opcodes.IDIV,

      ASM.Opcodes.IREM ,
      ASM.Opcodes.ISHL ,
      ASM.Opcodes.ISHR ,
      ASM.Opcodes.IUSHR ,
      ASM.Opcodes.IAND ,
      ASM.Opcodes.IOR ,
      ASM.Opcodes.IXOR ,
      ASM.Opcodes.L2I ,
      ASM.Opcodes.F2I ,
      ASM.Opcodes.D2I ,
      ASM.Opcodes.I2B ,
      ASM.Opcodes.I2C ,
      ASM.Opcodes.I2S ,
      ASM.Opcodes.LCMP, 
      ASM.Opcodes.FCMPL, 
      ASM.Opcodes.FCMPG, 
      ASM.Opcodes.DCMPL, 
      ASM.Opcodes.DCMPG, 
      ASM.Opcodes.BIPUSH,
      ASM.Opcodes.SIPUSH,
      ASM.Opcodes.ARRAYLENGTH :
          mFrm.evalStack[tosHi] := ASM.Opcodes.`INTEGER;

    | ASM.Opcodes.LCONST_0 ,
      ASM.Opcodes.LCONST_1 ,
      ASM.Opcodes.LALOAD ,
      ASM.Opcodes.LADD ,

      ASM.Opcodes.LSUB,
      ASM.Opcodes.LMUL,
      ASM.Opcodes.LDIV,

      ASM.Opcodes.LREM ,
      ASM.Opcodes.LSHL ,
      ASM.Opcodes.LSHR ,
      ASM.Opcodes.LUSHR ,
      ASM.Opcodes.LAND ,
      ASM.Opcodes.LOR ,
      ASM.Opcodes.LXOR ,
      ASM.Opcodes.I2L ,
      ASM.Opcodes.F2L ,
      ASM.Opcodes.D2L :
          mFrm.evalStack[tosHi] := ASM.Opcodes.TOP;
          mFrm.evalStack[tosHi-1] := ASM.Opcodes.LONG;

    | ASM.Opcodes.FCONST_0 ,
      ASM.Opcodes.FCONST_1 ,
      ASM.Opcodes.FCONST_2 ,
      ASM.Opcodes.FALOAD ,
      ASM.Opcodes.FADD ,

      ASM.Opcodes.FSUB,
      ASM.Opcodes.FMUL,
      ASM.Opcodes.FDIV,

      ASM.Opcodes.FREM ,
      ASM.Opcodes.I2F ,
      ASM.Opcodes.L2F ,
      ASM.Opcodes.D2F :
          mFrm.evalStack[tosHi] := ASM.Opcodes.FLOAT;

    | ASM.Opcodes.DCONST_0 ,
      ASM.Opcodes.DCONST_1 ,
      ASM.Opcodes.DALOAD ,
      ASM.Opcodes.DADD ,

      ASM.Opcodes.DMUL,
      ASM.Opcodes.DDIV,
      ASM.Opcodes.DSUB,

      ASM.Opcodes.DREM ,
      ASM.Opcodes.I2D ,
      ASM.Opcodes.L2D ,
      ASM.Opcodes.F2D :
          mFrm.evalStack[tosHi] := ASM.Opcodes.TOP;
          mFrm.evalStack[tosHi] := ASM.Opcodes.DOUBLE;

    | ASM.Opcodes.AALOAD : 
         (* 
          *  The new TOS location will still hold the
          *  signature of the array type. Strip one '['
          *  and assign the signature.
          *)
          tmpSt := mFrm.evalStack[tosHi](RTS.NativeString);
          ASSERT( tmpSt.charAt(0) = '[' );
          mFrm.evalStack[tosHi] := tmpSt.substring(1);

    | ASM.Opcodes.DUP :
          mFrm.evalStack[tosHi] := mFrm.evalStack[tosHi-1];

    | ASM.Opcodes.DUP_X1 : 
          mFrm.evalStack[tosHi] := mFrm.evalStack[tosHi-1];
          mFrm.evalStack[tosHi-1] := mFrm.evalStack[tosHi-2];
          mFrm.evalStack[tosHi-2] := mFrm.evalStack[tosHi];

    | ASM.Opcodes.DUP_X2 :
          mFrm.evalStack[tosHi] := mFrm.evalStack[tosHi-1];
          mFrm.evalStack[tosHi-1] := mFrm.evalStack[tosHi-2];
          mFrm.evalStack[tosHi-2] := mFrm.evalStack[tosHi-3];
          mFrm.evalStack[tosHi-3] := mFrm.evalStack[tosHi];

    | ASM.Opcodes.DUP2 :
          mFrm.evalStack[tosHi] := mFrm.evalStack[tosHi-2];
          mFrm.evalStack[tosHi-1] := mFrm.evalStack[tosHi-3];

    | ASM.Opcodes.DUP2_X1 : 
          mFrm.evalStack[tosHi] := mFrm.evalStack[tosHi-2];
          mFrm.evalStack[tosHi-1] := mFrm.evalStack[tosHi-3];
          mFrm.evalStack[tosHi-2] := mFrm.evalStack[tosHi-4];
          mFrm.evalStack[tosHi-3] := mFrm.evalStack[tosHi];
          mFrm.evalStack[tosHi-4] := mFrm.evalStack[tosHi-1];

    | ASM.Opcodes.DUP2_X2 : 
          mFrm.evalStack[tosHi] := mFrm.evalStack[tosHi-2];
          mFrm.evalStack[tosHi-1] := mFrm.evalStack[tosHi-3];
          mFrm.evalStack[tosHi-2] := mFrm.evalStack[tosHi-4];
          mFrm.evalStack[tosHi-3] := mFrm.evalStack[tosHi-5];
          mFrm.evalStack[tosHi-4] := mFrm.evalStack[tosHi];
          mFrm.evalStack[tosHi-5] := mFrm.evalStack[tosHi-1];

    | ASM.Opcodes.SWAP :
          tmpOb := mFrm.evalStack[tosHi];
          mFrm.evalStack[tosHi] := mFrm.evalStack[tosHi-1];
          mFrm.evalStack[tosHi-1] := tmpOb;
   (* --------------------------------- *)
   (*       ELSE take a CASE TRAP       *)
   (* --------------------------------- *)
    END; 
  END MutateEvalStack;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)AddValParam*( 
                                type : Sym.Type ),NEW;
    VAR newFE : FrameElement;
        need2 : BOOLEAN;
  BEGIN
    need2 := FALSE;
    CASE (type(Ty.Base).tpOrd) OF
      | Ty.boolN .. Ty.intN, Ty.setN, Ty.uBytN :
          newFE.dclTp := ASM.Opcodes.INTEGER;
      | Ty.sReaN :
          newFE.dclTp := ASM.Opcodes.FLOAT;
      | Ty.anyRec .. Ty.sStrN :
          newFE.dclTp := ASM.Opcodes.TOP;  (* ???? *)
      | Ty.lIntN :
          newFE.dclTp := ASM.Opcodes.LONG; need2 := TRUE;
      | Ty.realN :
          newFE.dclTp := ASM.Opcodes.DOUBLE; need2 := TRUE;
    END;
    newFE.state := newFE.dclTp; (* ==> initialized *)
    APPEND( mFrm.localStack, newFE );
    IF need2 THEN
      APPEND( mFrm.localStack, dummyElement );
    END;
    mFrm.maxLocal := MAX( mFrm.maxLocal, mFrm.LcLn() );
  END AddValParam;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)AddLocal*( 
                                type : Sym.Type ),NEW;
    VAR newFE : FrameElement;
        need2 : BOOLEAN;
        preHi : INTEGER;
  BEGIN
    Uninitialized( type, need2, newFE );
    APPEND( mFrm.localStack, newFE );
    IF need2 THEN APPEND( mFrm.localStack, dummyElement ) END;
    mFrm.maxLocal := MAX( mFrm.maxLocal, mFrm.LcLn() );
  END AddLocal;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)TrackStore*( ord : INTEGER ),NEW;
  BEGIN
    IF mFrm.localStack[ord].state = ASM.Opcodes.TOP THEN
      mFrm.localStack[ord].state := mFrm.localStack[ord].dclTp;
    END;
  END TrackStore;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)PopLocal1*(),NEW;
    VAR preHi : INTEGER;
  BEGIN
    CUT( mFrm.localStack, mFrm.LcHi() );
(*
 *    Hlp.IMsg( "=================================" );
 *    Hlp.IMsg( "PopLocal start" );
 *    mFrm.Diag(0);
 *    preHi := mFrm.LcHi();
 *    CUT( mFrm.localStack, mFrm.LcHi() );
 *    Hlp.IMsg3( "PopLocal", Ju.i2CO(preHi), Ju.i2CO(mFrm.LcHi()) );
 *    mFrm.Diag(0);
 *    Hlp.IMsg( "PopLocal end" );
 *    Hlp.IMsg( "=================================" );
 *)
  END PopLocal1;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)PopLocal2*(),NEW;
    VAR preHi : INTEGER;
  BEGIN
    CUT( mFrm.localStack, mFrm.LcHi() - 1 );
  END PopLocal2;

 (* --------------------------------------------- *)
 (*     Invalidate locals from sigMax to top      *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)InvalidateLocals*(),NEW;
    VAR ix : INTEGER;
  BEGIN
    FOR ix := mFrm.sigMax TO mFrm.LcHi() DO
      mFrm.localStack[ix].state := ASM.Opcodes.TOP;
    END;
  END InvalidateLocals;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)ReleaseTo*( mark : INTEGER),NEW;
  BEGIN
    CUT( mFrm.localStack, mark );
  END ReleaseTo;

 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)AddRefParam*( 
                                str : Lv.CharOpen ),NEW;
    VAR newFE : FrameElement;
  BEGIN
    ParamRefElement( str, newFE );
    APPEND( mFrm.localStack, newFE );
    mFrm.maxLocal := MAX( mFrm.maxLocal, mFrm.LcLn() );
  END AddRefParam;

 (* ================================================ *)
  PROCEDURE remSigS( obj : RTS.NativeObject ): RTS.NativeObject;
  VAR sig : RTS.NativeString;
  BEGIN
    IF obj IS RTS.NativeString THEN
      sig := obj(RTS.NativeString);
      IF sig.startsWith("L") & sig.endsWith(";") THEN
        RETURN sig.substring( 1, sig.length()-1 );
      END;
    END; 
    RETURN obj
  END remSigS;

  PROCEDURE ( mFrm : MethodFrame )GetLocalArr*() : Def.JloArr,NEW;
    VAR indx, count : INTEGER;
        rslt : POINTER TO ARRAY OF RTS.NativeObject;
  BEGIN
    rslt := NIL;
    IF mFrm.LcLn() > 0 THEN
      NEW( rslt, mFrm.LcCount() );
      count := 0;
      FOR indx := 0 TO mFrm.LcHi() DO
        IF mFrm.isDummyElem( indx ) THEN (* skip *)
        ELSE
          rslt[count] := remSigS( mFrm.localStack[indx].state ); (* XXX *)
          INC(count);
        END;
      END;
    END;
    RETURN rslt;
  END GetLocalArr;

 (* --------------------------------------------- *)

  PROCEDURE ( mFrm : MethodFrame )GetDclTpArr*() : Def.JloArr,NEW;
    VAR indx, count : INTEGER;
        rslt : POINTER TO ARRAY OF RTS.NativeObject;
  BEGIN
    rslt := NIL;
    IF mFrm.LcLn() > 0 THEN
      NEW( rslt, mFrm.LcCount() );
      count := 0;
      FOR indx := 0 TO mFrm.LcHi() DO
        IF mFrm.isDummyElem( indx ) THEN (* skip *)
        ELSE
          rslt[count] := remSigS( mFrm.localStack[indx].dclTp ); (* XXX *)
          INC(count);
        END;
      END;
    END;
    RETURN rslt;
  END GetDclTpArr;

 (* --------------------------------------------- *)

  PROCEDURE ( mFrm : MethodFrame )GetEvalArr*() : Def.JloArr,NEW;
    VAR index, count : INTEGER;
        rslt : POINTER TO ARRAY OF RTS.NativeObject;
        elem : RTS.NativeObject;
  BEGIN
    rslt := NIL;
(*
 Hlp.Msg("GetEvalArr: count " + Ju.i2CO(mFrm.EvCount())^ + ", hi " + Ju.i2CO(mFrm.EvHi())^ );
 *)
    IF mFrm.EvLn() > 0 THEN
      NEW( rslt, mFrm.EvCount() );
      index := 0;
      count := 0;
      WHILE index <= mFrm.EvHi() DO
        elem := mFrm.evalStack[index]; 
(*
 Hlp.Msg2("  elem " + Ju.i2CO(index)^, Hlp.objToStr(elem) );
 *)
        rslt[count] := remSigS( elem ); INC(index); INC(count); (* XXX *)
        IF (elem = ASM.Opcodes.DOUBLE) OR
           (elem = ASM.Opcodes.LONG) THEN INC(index) ;
(*
 Hlp.Msg("  skipping next element " );
 *)
        END;
      END;
    END;
    RETURN rslt;
  END GetEvalArr;

 (* --------------------------------------------- *)
 (*  Make a shallow copy of the evaluation stack  *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)GetFrameSave*( 
             save : FrameSave ) : FrameSave,NEW;
    VAR int,min : INTEGER;
        eMF, lMF : INTEGER;
        eFS, lFS : INTEGER;
        rslt : FrameSave;
  BEGIN
   (* ----------- *)
    IF save = NIL THEN 
      eMF := mFrm.EvLn();
      lMF := mFrm.LcLn();
      rslt := MkFrameSave( lMF, eMF );
      FOR int := 0 TO eMF - 1 DO
        rslt.evalArr[int] := mFrm.evalStack[int];
      END;
      FOR int := 0 TO lMF - 1 DO
        rslt.loclArr[int] := mFrm.localStack[int].state;
        rslt.lDclArr[int] := mFrm.localStack[int].dclTp;
      END;
    ELSE
      eMF := mFrm.EvLn();  (* Eval length in method frame  *)
      lMF := mFrm.LcLn();  (* Local length in method frame *)
      eFS := save.EvLn();  (* Eval length in FrameSave     *)
      lFS := save.LcLn();  (* Local length in FrameSave    *)
      ASSERT( eFS = eMF );
      min := MIN( lFS, lMF );
      IF min < lFS THEN 
        rslt := save.Shorten( min );
      ELSE 
        rslt := save;
      END;
      FOR int := 0 TO min-1 DO
       (*
        *  The following assertion is not true in general. For
        *  example, several branches to an exit label may each 
        *  have a temporary variable at the point of branching 
        *  that are of different types.
        *    ASSERT( rslt.lDclArr[int] = mFrm.localStack[int].dclTp );
        *  At the label's definition point all of the temporaries will
        *  be out of scope, and the merged list will be truncated.
        *)
        IF rslt.loclArr[int] # mFrm.localStack[int].state THEN
          rslt.loclArr[int] := ASM.Opcodes.TOP;
        END; 
      END;
    END;
    RETURN rslt;
  END  GetFrameSave;

 (* --------------------------------------------- *)
 (*        Copy the stored evaluation stack       *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)OverrideLocalState*( save : FrameSave ),NEW;
    VAR elem : FrameElement;
        sHi : INTEGER; (* High index of saved local state *)
        idx : INTEGER;
  BEGIN
   (*
    *  Frame state is invalid, therefore construct
    *  new state using only the saved state.
    *)
    CUT( mFrm.localStack, 0 );
    sHi := save.LcHi();
    FOR idx := 0 TO sHi DO
      elem.state := save.loclArr[idx];
      elem.dclTp := save.lDclArr[idx];
      APPEND( mFrm.localStack, elem );
    END;
  END OverrideLocalState;

 (* --------------------------------------------- *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)MergeLocalState*( save : FrameSave ),NEW;
    VAR elem : FrameElement;
        lLn : INTEGER; (* length of local state *)
        sLn : INTEGER;
        idx : INTEGER;
  BEGIN
   (*
    *  Frame state is valid, therefore construct
    *  new state merging save and mFrm.localStack.
    *)
    sLn := save.LcLn();
    lLn := MIN( mFrm.LcLn(), sLn );
    FOR idx := 0 TO lLn - 1 DO
      ASSERT( save.lDclArr[idx] = mFrm.localStack[idx].dclTp );
      IF save.loclArr[idx] # mFrm.localStack[idx].state THEN
        mFrm.localStack[idx].state := ASM.Opcodes.TOP;
      END;
      CUT( mFrm.localStack, lLn );
    END;
  END MergeLocalState;

 (* --------------------------------------------- *)
 (* --------------------------------------------- *)
  PROCEDURE (mFrm : MethodFrame)RestoreFrameState*( save : FrameSave ),NEW;
    VAR len, int, eL, lL : INTEGER;
        definedAtEntry : BOOLEAN;
        ePreState, lPreState : RTS.NativeString;
  BEGIN
    IF save = NIL THEN
      mFrm.setDepth( 0 ); RETURN;
    END;
    definedAtEntry := ~mFrm.undefEval;
    ePreState := NIL; (* Avoid "no initialization warning *)
    lPreState := NIL; (* Avoid "no initialization warning *)
    mFrm.undefEval := FALSE;
   (*
    *  First, fix the evaluation stack.
    *)
    len := save.EvLn();
    mFrm.setDepth( 0 );
    FOR int := 0 TO len-1 DO
      APPEND( mFrm.evalStack, save.evalArr[int] );
    END;
   (*
    *  Next, fix the frame state.
    *  Note that there are two cases here:
    *  (1) There is a fall-through into the current label 
    *      which is the case if mFrm.undefEval is FALSE.
    *      In this case the saved localStack is copied
    *      up to the current LcHi, since the jump may 
    *      have originated from a scope with more locals,
    *      and the EvLn must match.
    *  (2) mFrm.undefEval is TRUE: ==> no fall-through.
    *      In this case the entire saved localStack is 
    *      copied, as is the evalStack.
    *)
    IF definedAtEntry THEN
      mFrm.MergeLocalState( save );
    ELSE
      mFrm.OverrideLocalState( save );
    END;
  END RestoreFrameState;

 (* --------------------------------------------- *)

  PROCEDURE ( mFrm : MethodFrame )GetLocalArrStr*() : 
                                RTS.NativeString,NEW;
    VAR objArr : Def.JloArr;
  BEGIN
    objArr := mFrm.GetLocalArr();
    RETURN Hlp.ObjArrToStr( objArr );
  END GetLocalArrStr;

 (* --------------------------------------------- *)

  PROCEDURE ( mFrm : MethodFrame )GetDclTpArrStr*() : 
                                RTS.NativeString,NEW;
    VAR objArr : Def.JloArr;
  BEGIN
    objArr := mFrm.GetDclTpArr();
    RETURN Hlp.ObjArrToStr( objArr );
  END GetDclTpArrStr;

 (* --------------------------------------------- *)

  PROCEDURE ( mFrm : MethodFrame )GetEvalArrStr*() :
                               RTS.NativeString,NEW;
    VAR objArr : Def.JloArr;
  BEGIN
    IF mFrm.undefEval THEN RETURN Hlp.invalid;
    ELSE 
      objArr := mFrm.GetEvalArr();
      RETURN Hlp.ObjArrToStr( objArr );
    END;
  END GetEvalArrStr;


(* ================================================ *)
BEGIN
  dummyElement.state := ASM.Opcodes.TOP;
  dummyElement.dclTp := NIL;
END AsmFrames.
(* ================================================ *)

