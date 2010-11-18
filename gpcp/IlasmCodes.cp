(* ============================================================ *)
(*  IlasmCodes is the module which defines ilasm name ordinals. *)
(*  Name spelling is defined by the lexical rules of ILASM.	*)
(*  Copyright (c) John Gough 1999, 2000.			*)
(* ============================================================ *)

MODULE IlasmCodes;
IMPORT
        GPCPcopyright;

(* ============================================================ *)

  CONST
        dot_error*     = 0;
        dot_try*       = 1;
        dot_class*     = 2;
        dot_entrypoint*= 3;
        dot_field*     = 4;
        dot_implements*= 5;
        dot_interface* = 6;
        dot_locals*    = 7;
        dot_line*      = 8;
        dot_method*    = 9;
        dot_source*    = 10;
        dot_super*     = 11;
        dot_throws*    = 12;
        dot_var*       = 13;
        dot_assembly*  = 14;
        dot_namespace* = 15;
        dot_maxstack*  = 16;

  CONST
        att_empty*       = {};
        att_public*      = {0};
        att_private*     = {1};
        att_assembly*    = {2};
        att_protected*   = {3};
        att_value*       = {4};
        att_static*      = {5};
        att_final*       = {6};
        att_sealed*      = {7};
        att_abstract*    = {8};
        att_newslot*     = {9};
        att_interface*   = {10};
        att_synchronized*= {11};
        att_extern*      = {12};
        att_virtual*     = {13};
        att_instance*    = {14};
        att_volatile*    = {15};
	maxAttIndex*     = 15;

  CONST modAttr* = att_public + att_sealed (* + att_abstract *);

(* ============================================================ *)

  CONST
	opc_error*		= 0;		(* Start opcodes with *)
	opc_add*		= 1;		(* no arguments...    *)
	opc_add_ovf*		= 2;
	opc_add_ovf_un*		= 3;
	opc_and*		= 4;

	opc_arglist*		= 9;
	opc_break*		= 10;
	opc_ceq*		= 11;
	opc_cgt*		= 12;
	opc_cgt_un*		= 13;
	opc_ckfinite*		= 14;
	opc_clt*		= 15;
	opc_clt_un*		= 16;
	opc_conv_i*		= 17;
	opc_conv_i1*		= 18;
	opc_conv_i2*		= 19;
	opc_conv_i4*		= 20;
	opc_conv_i8*		= 21;
	opc_conv_ovf_i*		= 22;
	opc_conv_ovf_i_un*	= 23;
	opc_conv_ovf_i1*	= 24;
	opc_conv_ovf_i1_un*	= 25;
	opc_conv_ovf_i2*	= 26;
	opc_conv_ovf_i2_un*	= 27;
	opc_conv_ovf_i4*	= 28;
	opc_conv_ovf_i4_un*	= 29;
	opc_conv_ovf_i8*	= 30;
	opc_conv_ovf_i8_un*	= 31;
	opc_conv_ovf_u*		= 32;
	opc_conv_ovf_u_un*	= 32;
	opc_conv_ovf_u1*	= 34;
	opc_conv_ovf_u1_un*	= 35;
	opc_conv_ovf_u2*	= 36;
	opc_conv_ovf_u2_un*	= 37;
	opc_conv_ovf_u4*	= 38;
	opc_conv_ovf_u4_un*	= 39;
	opc_conv_ovf_u8*	= 40;
	opc_conv_ovf_u8_un*	= 41;
	opc_conv_r4*		= 42;
	opc_conv_r8*		= 43;
	opc_conv_u*		= 44;
	opc_conv_u1*		= 45;
	opc_conv_u2*		= 46;
	opc_conv_u4*		= 47;
	opc_conv_u8*		= 48;
	opc_cpblk*		= 49;
	opc_div*		= 50;
	opc_div_un*		= 51;
	opc_dup*		= 52;
	opc_endcatch*		= 53;
	opc_endfilter*		= 54;
	opc_endfinally*		= 55;
	opc_initblk*		= 56;
	opc_jmpi*		= 57;
	opc_ldarg_0*		= 58;
	opc_ldarg_1*		= 59;
	opc_ldarg_2*		= 60;
	opc_ldarg_3*		= 61;
	opc_ldc_i4_0*		= 62;
	opc_ldc_i4_1*		= 63;
	opc_ldc_i4_2*		= 64;
	opc_ldc_i4_3*		= 65;
	opc_ldc_i4_4*		= 66;
	opc_ldc_i4_5*		= 67;
	opc_ldc_i4_6*		= 68;
	opc_ldc_i4_7*		= 69;
	opc_ldc_i4_8*		= 70;
	opc_ldc_i4_M1*		= 71;
	opc_ldelem_i*		= 72;
	opc_ldelem_i1*		= 73;
	opc_ldelem_i2*		= 74;
	opc_ldelem_i4*		= 75;
	opc_ldelem_i8*		= 76;
	opc_ldelem_r4*		= 77;
	opc_ldelem_r8*		= 78;
	opc_ldelem_ref*		= 79;
	opc_ldelem_u*		= 80;
	opc_ldelem_u1*		= 81;
	opc_ldelem_u2*		= 82;
	opc_ldelem_u4*		= 83;
	opc_ldind_i*		= 84;
	opc_ldind_i1*		= 85;
	opc_ldind_i2*		= 86;
	opc_ldind_i4*		= 87;
	opc_ldind_i8*		= 88;
	opc_ldind_r4*		= 89;
	opc_ldind_r8*		= 90;
	opc_ldind_ref*		= 91;
	opc_ldind_u*		= 92;
	opc_ldind_u1*		= 93;
	opc_ldind_u2*		= 94;
	opc_ldind_u4*		= 95;
	opc_ldlen*		= 96;
	opc_ldloc_0*		= 97;
	opc_ldloc_1*		= 98;
	opc_ldloc_2*		= 99;
	opc_ldloc_3*		= 100;
	opc_ldnull*		= 101;
	opc_localloc*		= 102;
	opc_mul*		= 103;
	opc_mul_ovf*		= 104;
	opc_mul_ovf_un*		= 105;
	opc_neg*		= 106;
	opc_nop*		= 107;
	opc_not*		= 108;
	opc_or*			= 109;
	opc_pop*		= 110;
	opc_refanytype*		= 111;
	opc_rem*		= 112;
	opc_rem_un*		= 113;
	opc_ret*		= 114;
	opc_rethrow*		= 115;
	opc_shl*		= 116;
	opc_shr*		= 117;
	opc_shr_un*		= 118;
	opc_stelem_i*		= 119;
	opc_stelem_i1*		= 120;
	opc_stelem_i2*		= 121;
	opc_stelem_i4*		= 122;
	opc_stelem_i8*		= 123;
	opc_stelem_r4*		= 124;
	opc_stelem_r8*		= 125;
	opc_stelem_ref*		= 126;
	opc_stind_i*		= 127;
	opc_stind_i1*		= 128;
	opc_stind_i2*		= 129;
	opc_stind_i4*		= 130;
	opc_stind_i8*		= 131;
	opc_stind_r4*		= 132;
	opc_stind_r8*		= 133;
	opc_stind_ref*		= 134;
	opc_stloc_0*		= 135;
	opc_stloc_1*		= 136;
	opc_stloc_2*		= 137;
	opc_stloc_3*		= 138;
	opc_sub*		= 139;
	opc_sub_ovf*		= 140;
	opc_sub_ovf_un*		= 141;
	opc_tailcall*		= 142;
	opc_throw*		= 143;
	opc_volatile*		= 144;
	opc_xor*		= 145;
(*
 *)
	opc_ldarg*		= 151;
	opc_ldarg_s*		= 152;
	opc_ldarga*		= 153;
	opc_ldarga_s*		= 154;
	opc_starg*		= 155;
	opc_starg_s*		= 156;
	opc_ldloc*		= 225;		(* oops! *)
	opc_ldloc_s*		= 226;		(* oops! *)
	opc_ldloca*		= 227;		(* oops! *)
	opc_ldloca_s*		= 228;		(* oops! *)
	opc_stloc*		= 157;
	opc_stloc_s*		= 158;

	opc_ldc_i4*		= 159;		(* Opcodes with i4 arg. *)
	opc_unaligned_*		= 160;
	opc_ldc_i4_s*		= 161;
	opc_ldc_i8*		= 162;		(* Opcodes with i8 arg. *)
	opc_ldc_r4*		= 163;		(* Opcodes with flt/dbl *)
	opc_ldc_r8*		= 164;
(*
 *)
	opc_beq*		= 166;
	opc_beq_s*		= 167;
	opc_bge*		= 168;
	opc_bge_s*		= 169;
	opc_bge_un*		= 170;
	opc_bge_un_s*		= 171;
	opc_bgt*		= 172;
	opc_bgt_s*		= 173;
	opc_bgt_un*		= 174;
	opc_bgt_un_s*		= 175;
	opc_ble*		= 176;
	opc_ble_s*		= 177;
	opc_ble_un*		=   5;
	opc_ble_un_s*		=   6;
	opc_blt*		= 178;
	opc_blt_s*		= 179;
	opc_blt_un*		= 180;
	opc_blt_un_s*		= 181;
	opc_bne_un*		= 182;
	opc_bne_un_s*		= 183;
	opc_br*			= 184;
	opc_br_s*		= 185;
	opc_brfalse*		= 186;
	opc_brfalse_s*		= 187;
	opc_brtrue*		= 188;
	opc_brtrue_s*		= 189;
	opc_leave*		= 190;
(*
 *	opc_leave_s*		= 191;
 *
 *)
	opc_call*		= 194;
	opc_callvirt*		= 195;
	opc_jmp*		= 196;
	opc_ldftn*		= 197;
	opc_ldvirtftn*		= 198;
	opc_newobj*		= 199;

	opc_ldfld*		= 200;		(* Opcodes with fldNm args *)
	opc_ldflda*		= 201;
	opc_ldsfld*		= 202;
	opc_ldsflda*		= 203;
	opc_stfld*		= 204;
	opc_stsfld*		= 205;

	opc_box*		= 206;		(* Opcodes with type arg   *)
	opc_castclass*		= 207;
	opc_cpobj*		= 208;
	opc_initobj*		= 209;
	opc_isinst*		= 210;
	opc_ldelema*		= 211;
	opc_ldobj*		= 212;
	opc_mkrefany*		= 213;
	opc_newarr*		= 214;
	opc_refanyval*		= 215;
	opc_sizeof*		= 216;
	opc_stobj*		= 217;
	opc_unbox*		= 218;

	opc_ldstr*		= 219;		(* Miscellaneous	*)
	opc_calli*		= 220;
	opc_ldptr*		= 221;
	opc_ldtoken*		= 222;
(*
 *)
	opc_switch*		= 224;

(* ============================================================ *)

  TYPE
     OpName* = ARRAY 20 OF CHAR;

(* ============================================================ *)

  VAR	op* : ARRAY 232 OF OpName;
        cd* : ARRAY 232 OF INTEGER;
	dl* : ARRAY 232 OF INTEGER;

  VAR   dirStr* : ARRAY 18 OF OpName;
	access* : ARRAY 16 OF OpName;

(* ============================================================ *)

BEGIN
  (* ---------------------------------------------- *)

        dirStr[dot_error]       := ".ERROR";
        dirStr[dot_try]		:= "    .try";
        dirStr[dot_class]       := ".class";
        dirStr[dot_entrypoint]  := ".entrypoint";
        dirStr[dot_field]       := ".field";
        dirStr[dot_implements]  := "	implements";
        dirStr[dot_interface]   := ".interface";
        dirStr[dot_locals]      := ".locals";
        dirStr[dot_line]        := ".line";
        dirStr[dot_method]      := ".method";
        dirStr[dot_source]      := ".source";
        dirStr[dot_super]       := "	extends";
        dirStr[dot_throws]      := ".throws";
        dirStr[dot_var]         := ".var";
        dirStr[dot_assembly]    := ".assembly";
        dirStr[dot_namespace]   := ".namespace";
        dirStr[dot_maxstack]	:= ".maxstack";

  (* ---------------------------------------------- *)

	access[ 0]		:= "public";
	access[ 1]		:= "private";
	access[ 2]		:= "assembly";
	access[ 3]		:= "protected";
	access[ 4]		:= "value";
	access[ 5]		:= "static";
	access[ 6]		:= "final";
	access[ 7]		:= "sealed";
	access[ 8]		:= "abstract";
	access[ 9]		:= "newslot";
	access[10]		:= "interface";
	access[11]		:= "synchronized";
	access[12]		:= "extern";
	access[13]		:= "virtual";
	access[14]		:= "instance";
	access[15]		:= "volatile";

  (* ---------------------------------------------- *)

	op[opc_error]		:= "ERROR";
	op[opc_add]		:= "add";
	op[opc_add_ovf]		:= "add.ovf";
	op[opc_add_ovf_un]	:= "add.ovf.un";
	op[opc_and]		:= "and";

	op[opc_arglist]		:= "arglist";
	op[opc_break]		:= "break";
	op[opc_ceq]		:= "ceq";
	op[opc_cgt]		:= "cgt";
	op[opc_cgt_un]		:= "cgt.un";
	op[opc_ckfinite]	:= "ckfinite";
	op[opc_clt]		:= "clt";
	op[opc_clt_un]		:= "clt.un";
	op[opc_conv_i]		:= "conv.i";
	op[opc_conv_i1]		:= "conv.i1";
	op[opc_conv_i2]		:= "conv.i2";
	op[opc_conv_i4]		:= "conv.i4";
	op[opc_conv_i8]		:= "conv.i8";
	op[opc_conv_ovf_i]	:= "conv.ovf.i";
	op[opc_conv_ovf_i_un]	:= "conv.ovf.i.un";
	op[opc_conv_ovf_i1]	:= "conv.ovf.i1";
	op[opc_conv_ovf_i1_un]	:= "conv.ovf.i1.un";
	op[opc_conv_ovf_i2]	:= "conv.ovf.i2";
	op[opc_conv_ovf_i2_un]	:= "conv.ovf.i2.un";
	op[opc_conv_ovf_i4]	:= "conv.ovf.i4";
	op[opc_conv_ovf_i4_un]	:= "conv.ovf.i4.un";
	op[opc_conv_ovf_i8]	:= "conv.ovf.i8";
	op[opc_conv_ovf_i8_un]	:= "conv.ovf.i8.un";
	op[opc_conv_ovf_u]	:= "conv.ovf.u";
	op[opc_conv_ovf_u_un]	:= "conv.ovf.u.un";
	op[opc_conv_ovf_u1]	:= "conv.ovf.u1";
	op[opc_conv_ovf_u1_un]	:= "conv.ovf.u1.un";
	op[opc_conv_ovf_u2]	:= "conv.ovf.u2";
	op[opc_conv_ovf_u2_un]	:= "conv.ovf.u2.un";
	op[opc_conv_ovf_u4]	:= "conv.ovf.u4";
	op[opc_conv_ovf_u4_un]	:= "conv.ovf.u4.un";
	op[opc_conv_ovf_u8]	:= "conv.ovf.u8";
	op[opc_conv_ovf_u8_un]	:= "conv.ovf.u8.un";
	op[opc_conv_r4]		:= "conv.r4";
	op[opc_conv_r8]		:= "conv.r8";
	op[opc_conv_u]		:= "conv.u";
	op[opc_conv_u1]		:= "conv.u1";
	op[opc_conv_u2]		:= "conv.u2";
	op[opc_conv_u4]		:= "conv.u4";
	op[opc_conv_u8]		:= "conv.u8";
	op[opc_cpblk]		:= "cpblk";
	op[opc_div]		:= "div";
	op[opc_div_un]		:= "div.un";
	op[opc_dup]		:= "dup";
	op[opc_endcatch]	:= "endcatch";
	op[opc_endfilter]	:= "endfilter";
	op[opc_endfinally]	:= "endfinally";
	op[opc_initblk]		:= "initblk";
	op[opc_jmpi]		:= "jmpi";
	op[opc_ldarg_0]		:= "ldarg.0";
	op[opc_ldarg_1]		:= "ldarg.1";
	op[opc_ldarg_2]		:= "ldarg.2";
	op[opc_ldarg_3]		:= "ldarg.3";
	op[opc_ldc_i4_0]	:= "ldc.i4.0";
	op[opc_ldc_i4_1]	:= "ldc.i4.1";
	op[opc_ldc_i4_2]	:= "ldc.i4.2";
	op[opc_ldc_i4_3]	:= "ldc.i4.3";
	op[opc_ldc_i4_4]	:= "ldc.i4.4";
	op[opc_ldc_i4_5]	:= "ldc.i4.5";
	op[opc_ldc_i4_6]	:= "ldc.i4.6";
	op[opc_ldc_i4_7]	:= "ldc.i4.7";
	op[opc_ldc_i4_8]	:= "ldc.i4.8";
	op[opc_ldc_i4_M1]	:= "ldc.i4.M1";
	op[opc_ldelem_i]	:= "ldelem.i";
	op[opc_ldelem_i1]	:= "ldelem.i1";
	op[opc_ldelem_i2]	:= "ldelem.i2";
	op[opc_ldelem_i4]	:= "ldelem.i4";
	op[opc_ldelem_i8]	:= "ldelem.i8";
	op[opc_ldelem_r4]	:= "ldelem.r4";
	op[opc_ldelem_r8]	:= "ldelem.r8";
	op[opc_ldelem_ref]	:= "ldelem.ref";
	op[opc_ldelem_u]	:= "ldelem.u";
	op[opc_ldelem_u1]	:= "ldelem.u1";
	op[opc_ldelem_u2]	:= "ldelem.u2";
	op[opc_ldelem_u4]	:= "ldelem.u4";
	op[opc_ldind_i]		:= "ldind.i";
	op[opc_ldind_i1]	:= "ldind.i1";
	op[opc_ldind_i2]	:= "ldind.i2";
	op[opc_ldind_i4]	:= "ldind.i4";
	op[opc_ldind_i8]	:= "ldind.i8";
	op[opc_ldind_r4]	:= "ldind.r4";
	op[opc_ldind_r8]	:= "ldind.r8";
	op[opc_ldind_ref]	:= "ldind.ref";
	op[opc_ldind_u]		:= "ldind.u";
	op[opc_ldind_u1]	:= "ldind.u1";
	op[opc_ldind_u2]	:= "ldind.u2"; (* NOT ldind.u3! *)
	op[opc_ldind_u4]	:= "ldind.u4";
	op[opc_ldlen]		:= "ldlen";
	op[opc_ldloc_0]		:= "ldloc.0";
	op[opc_ldloc_1]		:= "ldloc.1";
	op[opc_ldloc_2]		:= "ldloc.2";
	op[opc_ldloc_3]		:= "ldloc.3";
	op[opc_ldnull]		:= "ldnull";
	op[opc_localloc]	:= "localloc";
	op[opc_mul]		:= "mul";
	op[opc_mul_ovf]		:= "mul.ovf";
	op[opc_mul_ovf_un]	:= "mul.ovf.un";
	op[opc_neg]		:= "neg";
	op[opc_nop]		:= "nop";
	op[opc_not]		:= "not";
	op[opc_or]		:= "or";
	op[opc_pop]		:= "pop";
	op[opc_refanytype]	:= "refanytype";
	op[opc_rem]		:= "rem";
	op[opc_rem_un]		:= "rem.un";
	op[opc_ret]		:= "ret";
	op[opc_rethrow]		:= "rethrow";
	op[opc_shl]		:= "shl";
	op[opc_shr]		:= "shr";
	op[opc_shr_un]		:= "shr.un";
	op[opc_stelem_i]	:= "stelem.i";
	op[opc_stelem_i1]	:= "stelem.i1";
	op[opc_stelem_i2]	:= "stelem.i2";
	op[opc_stelem_i4]	:= "stelem.i4";
	op[opc_stelem_i8]	:= "stelem.i8";
	op[opc_stelem_r4]	:= "stelem.r4";
	op[opc_stelem_r8]	:= "stelem.r8";
	op[opc_stelem_ref]	:= "stelem.ref";
	op[opc_stind_i]		:= "stind.i";
	op[opc_stind_i1]	:= "stind.i1";
	op[opc_stind_i2]	:= "stind.i2";
	op[opc_stind_i4]	:= "stind.i4";
	op[opc_stind_i8]	:= "stind.i8";
	op[opc_stind_r4]	:= "stind.r4";
	op[opc_stind_r8]	:= "stind.r8";
	op[opc_stind_ref]	:= "stind.ref";
	op[opc_stloc_0]		:= "stloc.0";
	op[opc_stloc_1]		:= "stloc.1";
	op[opc_stloc_2]		:= "stloc.2";
	op[opc_stloc_3]		:= "stloc.3";
	op[opc_sub]		:= "sub";
	op[opc_sub_ovf]		:= "sub.ovf";
	op[opc_sub_ovf_un]	:= "sub.ovf.un";
	op[opc_tailcall]	:= "tailcall";
	op[opc_throw]		:= "throw";
	op[opc_volatile]	:= "volatile";
	op[opc_xor]		:= "xor";

	op[opc_ldarg]		:= "ldarg";
	op[opc_ldarg_s]		:= "ldarg.s";
	op[opc_ldarga]		:= "ldarga";
	op[opc_ldarga_s]	:= "ldarga.s";
	op[opc_starg]		:= "starg";
	op[opc_starg_s]		:= "starg.s";
	op[opc_ldloc]		:= "ldloc";
	op[opc_ldloc_s]		:= "ldloc.s";
	op[opc_ldloca]		:= "ldloca";
	op[opc_ldloca_s]	:= "ldloca.s";
	op[opc_stloc]		:= "stloc";
	op[opc_stloc_s]		:= "stloc.s";

	op[opc_ldc_i4]		:= "ldc.i4";
	op[opc_unaligned_]	:= "unaligned.";
	op[opc_ldc_i4_s]	:= "ldc.i4.s";
	op[opc_ldc_i8]		:= "ldc.i8";
	op[opc_ldc_r4]		:= "ldc.r4";
	op[opc_ldc_r8]		:= "ldc.r8";

	op[opc_beq]		:= "beq";
	op[opc_beq_s]		:= "beq.s";
	op[opc_bge]		:= "bge";
	op[opc_bge_s]		:= "bge.s";
	op[opc_bge_un]		:= "bge.un";
	op[opc_bge_un_s]	:= "bge.un.s";
	op[opc_bgt]		:= "bgt";
	op[opc_bgt_s]		:= "bgt.s";
	op[opc_bgt_un]		:= "bgt.un";
	op[opc_bgt_un_s]	:= "bgt.un.s";
	op[opc_ble]		:= "ble";
	op[opc_ble_s]		:= "ble.s";
	op[opc_ble_un]		:= "ble.un";
	op[opc_ble_un_s]	:= "ble.un.s";
	op[opc_blt]		:= "blt";
	op[opc_blt_s]		:= "blt.s";
	op[opc_blt_un]		:= "blt.un";
	op[opc_blt_un_s]	:= "blt.un.s";
	op[opc_bne_un]		:= "bne.un";
	op[opc_bne_un_s]	:= "bne.un.s";
	op[opc_br]		:= "br";
	op[opc_br_s]		:= "br.s";
	op[opc_brfalse]		:= "brfalse";
	op[opc_brfalse_s]	:= "brfalse.s";
	op[opc_brtrue]		:= "brtrue";
	op[opc_brtrue_s]	:= "brtrue.s";
	op[opc_leave]		:= "leave";
(*
 *	op[opc_leave_s]		:= "leave.s";
 *)
	op[opc_call]		:= "call";
	op[opc_callvirt]	:= "callvirt";
	op[opc_jmp]		:= "jmp";
	op[opc_ldftn]		:= "ldftn";
	op[opc_ldvirtftn]	:= "ldvirtftn";
	op[opc_newobj]		:= "newobj";

	op[opc_ldfld]		:= "ldfld";
	op[opc_ldflda]		:= "ldflda";
	op[opc_ldsfld]		:= "ldsfld";
	op[opc_ldsflda]		:= "ldsflda";
	op[opc_stfld]		:= "stfld";
	op[opc_stsfld]		:= "stsfld";

	op[opc_box]		:= "box";
	op[opc_castclass]	:= "castclass";
	op[opc_cpobj]		:= "cpobj";
	op[opc_initobj]		:= "initobj";
	op[opc_isinst]		:= "isinst";
	op[opc_ldelema]		:= "ldelema";
	op[opc_ldobj]		:= "ldobj";
	op[opc_mkrefany]	:= "mkrefany";
	op[opc_newarr]		:= "newarr";
	op[opc_refanyval]	:= "refanyval";
	op[opc_sizeof]		:= "sizeof";
	op[opc_stobj]		:= "stobj";
	op[opc_unbox]		:= "unbox";

	op[opc_ldstr]		:= "ldstr";
	op[opc_calli]		:= "calli";
	op[opc_ldptr]		:= "ldptr";
	op[opc_ldtoken]		:= "ldtoken";
	op[opc_switch]		:= "switch";

  (* ---------------------------------------------- *)

	cd[opc_error]	        := -1;


	cd[opc_nop]		:= 0;
	cd[opc_break]		:= 1;
	cd[opc_ldarg_0]		:= 2;
	cd[opc_ldarg_1]		:= 3;
	cd[opc_ldarg_2]		:= 4;
	cd[opc_ldarg_3]		:= 5;
	cd[opc_ldloc_0]		:= 6;
	cd[opc_ldloc_1]		:= 7;
	cd[opc_ldloc_2]		:= 8;
	cd[opc_ldloc_3]		:= 9;
	cd[opc_stloc_0]		:= 10;
	cd[opc_stloc_1]		:= 11;
	cd[opc_stloc_2]		:= 12;
	cd[opc_stloc_3]		:= 13;

        cd[opc_ldarg_s]         := 0EH;
        cd[opc_ldarga_s]        := 0FH;
        cd[opc_starg_s]         := 10H;
        cd[opc_ldloc_s]         := 11H;
        cd[opc_ldloca_s]        := 12H;
        cd[opc_stloc_s]         := 13H;

	cd[opc_ldnull]		:= 14H;
	cd[opc_ldc_i4_M1]	:= 15H;
	cd[opc_ldc_i4_0]	:= 16H;
	cd[opc_ldc_i4_1]	:= 17H;
	cd[opc_ldc_i4_2]	:= 18H;
	cd[opc_ldc_i4_3]	:= 19H;
	cd[opc_ldc_i4_4]	:= 1AH;
	cd[opc_ldc_i4_5]	:= 1BH;
	cd[opc_ldc_i4_6]	:= 1CH;
	cd[opc_ldc_i4_7]	:= 1DH;
	cd[opc_ldc_i4_8]	:= 1EH;
        cd[opc_ldc_i4_s]        := 1FH; 
        cd[opc_ldc_i4]          := 20H; 
        cd[opc_ldc_i8]          := 21H; 
        cd[opc_ldc_r4]          := 22H; 
        cd[opc_ldc_r8]          := 23H; 

	cd[opc_dup]		:= 25H;
	cd[opc_pop]		:= 26H;
        cd[opc_jmp]             := 27H; 
        cd[opc_call]            := 28H;

	cd[opc_ret]		:= 2AH;
        cd[opc_br]              := 2BH; 
        cd[opc_brfalse]         := 2CH;
        cd[opc_brtrue]          := 2DH;
        cd[opc_beq]             := 2EH;
        cd[opc_bge]             := 2FH;
        cd[opc_bgt]             := 30H;
        cd[opc_ble]             := 31H;
        cd[opc_blt]             := 32H;
        cd[opc_bne_un]          := 33H;
        cd[opc_bge_un]          := 34H;
        cd[opc_bgt_un]          := 35H;
        cd[opc_ble_un]          := 36H;
        cd[opc_blt_un]          := 37H;

        cd[opc_ldind_i1]        := 46H;
        cd[opc_ldind_u1]        := 71;
        cd[opc_ldind_i2]        := 72;
        cd[opc_ldind_u2]        := 73;
        cd[opc_ldind_i4]        := 74;
        cd[opc_ldind_u4]        := 75;
        cd[opc_ldind_i8]        := 76;
        cd[opc_ldind_i]         := 77;
        cd[opc_ldind_r4]        := 78;
        cd[opc_ldind_r8]        := 79;
        cd[opc_ldind_ref]       := 80;
        cd[opc_stind_ref]       := 81;
        cd[opc_stind_i1]        := 82;
        cd[opc_stind_i2]        := 83;
        cd[opc_stind_i4]        := 84;
        cd[opc_stind_i8]        := 85;
        cd[opc_stind_r4]        := 86;
        cd[opc_stind_r8]        := 87;
        cd[opc_add]             := 88;
        cd[opc_sub]             := 89;
        cd[opc_mul]             := 90;
        cd[opc_div]             := 91;
        cd[opc_div_un]          := 92;
        cd[opc_rem]             := 93;
        cd[opc_rem_un]          := 94;
        cd[opc_and]             := 95;
        cd[opc_or]              := 96;
        cd[opc_xor]             := 97;
        cd[opc_shl]             := 98;
        cd[opc_shr]             := 99;
        cd[opc_shr_un]          := 100;
        cd[opc_neg]             := 101;
        cd[opc_not]             := 102;
        cd[opc_conv_i1]         := 103;
        cd[opc_conv_i2]         := 104;
        cd[opc_conv_i4]         := 105;
        cd[opc_conv_i8]         := 106;
        cd[opc_conv_r4]         := 107;
        cd[opc_conv_r8]         := 108;
        cd[opc_conv_u4]         := 109;
        cd[opc_conv_u8]         := 110;

        cd[opc_callvirt]        := 6FH; 
        cd[opc_cpobj]           := 70H; 
        cd[opc_ldobj]           := 71H;
        cd[opc_ldstr]           := 72H;
        cd[opc_newobj]          := 73H; 
        cd[opc_castclass]       := 74H; 
        cd[opc_isinst]          := 75H;
(*
 *      cd[opc_conv_r_un]       := 76H;
 *)
        cd[opc_unbox]           := 79H; 
        cd[opc_throw]           := 7AH;
        cd[opc_ldfld]           := 7BH; 
        cd[opc_ldflda]          := 7CH;
        cd[opc_stfld]           := 7DH;
        cd[opc_ldsfld]          := 7EH;
        cd[opc_ldsflda]         := 7FH;
        cd[opc_stsfld]          := 80H;
        cd[opc_stobj]           := 81H; 
        cd[opc_conv_ovf_i1_un]  := 82H;
        cd[opc_conv_ovf_i2_un]  := 83H;
        cd[opc_conv_ovf_i4_un]  := 84H;
        cd[opc_conv_ovf_i8_un]  := 85H;
        cd[opc_conv_ovf_u1_un]  := 86H;
        cd[opc_conv_ovf_u2_un]  := 87H;
        cd[opc_conv_ovf_u4_un]  := 88H;
        cd[opc_conv_ovf_u8_un]  := 89H;
        cd[opc_conv_ovf_i_un]   := 8AH;
        cd[opc_conv_ovf_u_un]   := 8BH;
        cd[opc_box]             := 8CH; 
        cd[opc_newarr]          := 8DH;
        cd[opc_ldlen]           := 8EH;
        cd[opc_ldelema]         := 8FH; 
        cd[opc_ldelem_i1]       := 90H;
        cd[opc_ldelem_u1]       := 91H;
        cd[opc_ldelem_i2]       := 92H;
        cd[opc_ldelem_u2]       := 93H;
        cd[opc_ldelem_i4]       := 94H;
        cd[opc_ldelem_u4]       := 95H;
        cd[opc_ldelem_i8]       := 96H;
        cd[opc_ldelem_i]        := 97H;
        cd[opc_ldelem_r4]       := 98H;
        cd[opc_ldelem_r8]       := 99H;
        cd[opc_ldelem_ref]      := 9AH;

        cd[opc_stelem_i]        := 9BH;
        cd[opc_stelem_i1]       := 9CH;
        cd[opc_stelem_i2]       := 9DH;
        cd[opc_stelem_i4]       := 9EH;
        cd[opc_stelem_i8]       := 9FH;
        cd[opc_stelem_r4]       := 0A0H;
        cd[opc_stelem_r8]       := 0A1H;
        cd[opc_stelem_ref]      := 0A2H;

        cd[opc_conv_ovf_i1]     := 0B3H;
        cd[opc_conv_ovf_u1]     := 0B4H;
        cd[opc_conv_ovf_i2]     := 0B5H;
        cd[opc_conv_ovf_u2]     := 0B6H;
        cd[opc_conv_ovf_i4]     := 0B7H;
        cd[opc_conv_ovf_u4]     := 0B8H;
        cd[opc_conv_ovf_i8]     := 0B9H;
        cd[opc_conv_ovf_u8]     := 0BAH;

        cd[opc_refanyval]       := 0C2H; 
        cd[opc_ckfinite]        := 0C3H;
        cd[opc_mkrefany]        := 0C6H; 

        cd[opc_ldtoken]         := 0D0H; 
        cd[opc_conv_u2]         := 0D1H;
        cd[opc_conv_u1]         := 0D2H;
        cd[opc_conv_i]          := 0D3H;
        cd[opc_conv_ovf_i]      := 0D4H;
        cd[opc_conv_ovf_u]      := 0D5H;
        cd[opc_add_ovf]         := 0D6H;
        cd[opc_add_ovf_un]      := 0D7H;
        cd[opc_mul_ovf]         := 0D8H;
        cd[opc_mul_ovf_un]      := 0D9H;
        cd[opc_sub_ovf]         := 0DAH;
        cd[opc_sub_ovf_un]      := 0DBH;
        cd[opc_endfinally]      := 0DCH; 

        cd[opc_leave]           := 0DEH;  (* actually leave.s *)
(*
 *      cd[opc_leave_s]         := 0DEH; 
 *)
        cd[opc_stind_i]         := 0DFH;
        cd[opc_conv_u]          := 0E0H;

        cd[opc_localloc]        := 0F1H;
        cd[opc_endfilter]       := 0F2H;
        cd[opc_volatile]        := 0F4H;
        cd[opc_tailcall]        := 0F5H;
        cd[opc_cpblk]           := 0F8H;
        cd[opc_initblk]         := 0F9H;

        cd[opc_arglist]         := 0FE00H;
        cd[opc_ceq]             := 0FE01H;
        cd[opc_cgt]             := 0FE02H;
        cd[opc_cgt_un]          := 0FE03H;
        cd[opc_clt]             := 0FE04H;
        cd[opc_clt_un]          := 0FE05H;
        cd[opc_ldftn]           := 0FE06H; 
        cd[opc_ldvirtftn]       := 0FE07H;
        cd[opc_ldarg]           := 0FE09H;
        cd[opc_ldarga]          := 0FE0AH;
        cd[opc_starg]           := 0FE0BH;
        cd[opc_ldloc]           := 0FE0CH;
        cd[opc_ldloca]          := 0FE0DH;
        cd[opc_stloc]           := 0FE0EH;
        cd[opc_unaligned_]      := 0FE12H;
        cd[opc_initobj]         := 0FE15H; 
        cd[opc_rethrow]         := 0FE1AH;
        cd[opc_sizeof]          := 0FE1CH; 
        cd[opc_refanytype]      := 0FE1DH;


 (* ---------------------------------------------- *)

	dl[opc_error]		:= 0;
	dl[opc_add]		:= -1;
	dl[opc_add_ovf]		:= -1;
	dl[opc_add_ovf_un]	:= -1;
	dl[opc_and]		:= -1;
	dl[opc_arglist]		:= 1;
	dl[opc_break]		:= 0;
	dl[opc_ceq]		:= -1;
	dl[opc_cgt]		:= -1;
	dl[opc_cgt_un]		:= -1;
	dl[opc_ckfinite]	:= -1;
	dl[opc_clt]		:= -1;
	dl[opc_clt_un]		:= -1;
	dl[opc_conv_i]		:= 0;
	dl[opc_conv_i1]		:= 0;
	dl[opc_conv_i2]		:= 0;
	dl[opc_conv_i4]		:= 0;
	dl[opc_conv_i8]		:= 0;
	dl[opc_conv_ovf_i]	:= 0;
	dl[opc_conv_ovf_i_un]	:= 0;
	dl[opc_conv_ovf_i1]	:= 0;
	dl[opc_conv_ovf_i1_un]	:= 0;
	dl[opc_conv_ovf_i2]	:= 0;
	dl[opc_conv_ovf_i2_un]	:= 0;
	dl[opc_conv_ovf_i4]	:= 0;
	dl[opc_conv_ovf_i4_un]	:= 0;
	dl[opc_conv_ovf_i8]	:= 0;
	dl[opc_conv_ovf_i8_un]	:= 0;
	dl[opc_conv_ovf_u]	:= 0;
	dl[opc_conv_ovf_u_un]	:= 0;
	dl[opc_conv_ovf_u1]	:= 0;
	dl[opc_conv_ovf_u1_un]	:= 0;
	dl[opc_conv_ovf_u2]	:= 0;
	dl[opc_conv_ovf_u2_un]	:= 0;
	dl[opc_conv_ovf_u4]	:= 0;
	dl[opc_conv_ovf_u4_un]	:= 0;
	dl[opc_conv_ovf_u8]	:= 0;
	dl[opc_conv_ovf_u8_un]	:= 0;
	dl[opc_conv_r4]		:= 0;
	dl[opc_conv_r8]		:= 0;
	dl[opc_conv_u]		:= 0;
	dl[opc_conv_u1]		:= 0;
	dl[opc_conv_u2]		:= 0;
	dl[opc_conv_u4]		:= 0;
	dl[opc_conv_u8]		:= 0;
	dl[opc_cpblk]		:= -3;
	dl[opc_div]		:= -1;
	dl[opc_div_un]		:= -1;
	dl[opc_dup]		:= 1;
	dl[opc_endcatch]	:= 0;
	dl[opc_endfilter]	:= -1;
	dl[opc_endfinally]	:= 0;
	dl[opc_initblk]		:= -3;
	dl[opc_ldarg_0]		:= 1;
	dl[opc_ldarg_1]		:= 1;
	dl[opc_ldarg_2]		:= 1;
	dl[opc_ldarg_3]		:= 1;
	dl[opc_ldc_i4_0]	:= 1;
	dl[opc_ldc_i4_1]	:= 1;
	dl[opc_ldc_i4_2]	:= 1;
	dl[opc_ldc_i4_3]	:= 1;
	dl[opc_ldc_i4_4]	:= 1;
	dl[opc_ldc_i4_5]	:= 1;
	dl[opc_ldc_i4_6]	:= 1;
	dl[opc_ldc_i4_7]	:= 1;
	dl[opc_ldc_i4_8]	:= 1;
	dl[opc_ldc_i4_M1]	:= 1;
	dl[opc_ldelem_i]	:= -1;
	dl[opc_ldelem_i1]	:= -1;
	dl[opc_ldelem_i2]	:= -1;
	dl[opc_ldelem_i4]	:= -1;
	dl[opc_ldelem_i8]	:= -1;
	dl[opc_ldelem_r4]	:= -1;
	dl[opc_ldelem_r8]	:= -1;
	dl[opc_ldelem_ref]	:= -1;
	dl[opc_ldelem_u]	:= -1;
	dl[opc_ldelem_u1]	:= -1;
	dl[opc_ldelem_u2]	:= -1;
	dl[opc_ldelem_u4]	:= -1;
	dl[opc_ldind_i]		:= 0;
	dl[opc_ldind_i1]	:= 0;
	dl[opc_ldind_i2]	:= 0;
	dl[opc_ldind_i4]	:= 0;
	dl[opc_ldind_i8]	:= 0;
	dl[opc_ldind_r4]	:= 0;
	dl[opc_ldind_r8]	:= 0;
	dl[opc_ldind_ref]	:= 0;
	dl[opc_ldind_u]		:= 0;
	dl[opc_ldind_u1]	:= 0;
	dl[opc_ldind_u2]	:= 0;
	dl[opc_ldind_u4]	:= 0;
	dl[opc_ldlen]		:= 0;
	dl[opc_ldloc_0]		:= 1;
	dl[opc_ldloc_1]		:= 1;
	dl[opc_ldloc_2]		:= 1;
	dl[opc_ldloc_3]		:= 1;
	dl[opc_ldnull]		:= 1;
	dl[opc_localloc]	:= 0;
	dl[opc_mul]		:= -1;
	dl[opc_mul_ovf]		:= -1;
	dl[opc_mul_ovf_un]	:= -1;
	dl[opc_neg]		:= 0;
	dl[opc_nop]		:= 0;
	dl[opc_not]		:= 0;
	dl[opc_or]		:= -1;
	dl[opc_pop]		:= -1;
	dl[opc_refanytype]	:= 0;
	dl[opc_rem]		:= -1;
	dl[opc_rem_un]		:= -1;
	dl[opc_ret]		:= 0;
	dl[opc_rethrow]		:= 0;
	dl[opc_shl]		:= -1;
	dl[opc_shr]		:= -1;
	dl[opc_shr_un]		:= -1;
	dl[opc_stelem_i]	:= -3;
	dl[opc_stelem_i1]	:= -3;
	dl[opc_stelem_i2]	:= -3;
	dl[opc_stelem_i4]	:= -3;
	dl[opc_stelem_i8]	:= -3;
	dl[opc_stelem_r4]	:= -3;
	dl[opc_stelem_r8]	:= -3;
	dl[opc_stelem_ref]	:= -3;
	dl[opc_stind_i]		:= -2;
	dl[opc_stind_i1]	:= -2;
	dl[opc_stind_i2]	:= -2;
	dl[opc_stind_i4]	:= -2;
	dl[opc_stind_i8]	:= -2;
	dl[opc_stind_r4]	:= -2;
	dl[opc_stind_r8]	:= -2;
	dl[opc_stind_ref]	:= -2;
	dl[opc_stloc_0]		:= -1;
	dl[opc_stloc_1]		:= -1;
	dl[opc_stloc_2]		:= -1;
	dl[opc_stloc_3]		:= -1;
	dl[opc_sub]		:= -1;
	dl[opc_sub_ovf]		:= -1;
	dl[opc_sub_ovf_un]	:= -1;
	dl[opc_tailcall]	:= 0;
	dl[opc_throw]		:= -1;
	dl[opc_volatile]	:= 0;
	dl[opc_xor]		:= -1;

	dl[opc_ldarg]		:= 1;
	dl[opc_ldarg_s]		:= 1;
	dl[opc_ldarga]		:= 1;
	dl[opc_ldarga_s]	:= 1;
	dl[opc_starg]		:= -1;
	dl[opc_starg_s]		:= -1;
	dl[opc_ldloc]		:= 1;
	dl[opc_ldloc_s]		:= 1;
	dl[opc_ldloca]		:= 1;
	dl[opc_ldloca_s]	:= 1;
	dl[opc_stloc]		:= -1;
	dl[opc_stloc_s]		:= -1;

	dl[opc_ldc_i4]		:= 1;
	dl[opc_unaligned_]	:= 0;
	dl[opc_ldc_i4_s]	:= 1;
	dl[opc_ldc_i8]		:= 1;
	dl[opc_ldc_r4]		:= 1;
	dl[opc_ldc_r8]		:= 1;

	dl[opc_beq]		:= -2;
	dl[opc_beq_s]		:= -2;
	dl[opc_bge]		:= -2;
	dl[opc_bge_s]		:= -2;
	dl[opc_bge_un]		:= -2;
	dl[opc_bge_un_s]	:= -2;
	dl[opc_bgt]		:= -2;
	dl[opc_bgt_s]		:= -2;
	dl[opc_bgt_un]		:= -2;
	dl[opc_bgt_un_s]	:= -2;
	dl[opc_ble]		:= -2;
	dl[opc_ble_s]		:= -2;
	dl[opc_ble_un]		:= -2;
	dl[opc_ble_un_s]	:= -2;
	dl[opc_blt]		:= -2;
	dl[opc_blt_s]		:= -2;
	dl[opc_blt_un]		:= -2;
	dl[opc_blt_un_s]	:= -2;
	dl[opc_bne_un]		:= -2;
	dl[opc_bne_un_s]	:= -2;
	dl[opc_br]		:= 0;
	dl[opc_br_s]		:= 0;
	dl[opc_brfalse]		:= -1;
	dl[opc_brfalse_s]	:= -1;
	dl[opc_brtrue]		:= -1;
	dl[opc_brtrue_s]	:= -1;
	dl[opc_leave]		:= 0;
(*
 *	dl[opc_leave_s]		:= 0;
 *)
	dl[opc_call]		:= 0;		(* variable *)
	dl[opc_callvirt]	:= 0;		(* variable *)
	dl[opc_jmp]		:= 0;
	dl[opc_jmpi]		:= -1;

	dl[opc_ldftn]		:= 1;
	dl[opc_ldvirtftn]	:= 1;
	dl[opc_newobj]		:= 0;		(* variable *)

	dl[opc_ldfld]		:= 0;
	dl[opc_ldflda]		:= 0;
	dl[opc_ldsfld]		:= 1;
	dl[opc_ldsflda]		:= 1;
	dl[opc_stfld]		:= -2;
	dl[opc_stsfld]		:= -1;

	dl[opc_box]		:= 0;
	dl[opc_castclass]	:= 0;
	dl[opc_cpobj]		:= -2;
	dl[opc_initobj]		:= -1;
	dl[opc_isinst]		:= 0;
	dl[opc_ldelema]		:= -1;
	dl[opc_ldobj]		:= 0;
	dl[opc_mkrefany]	:= 0;
	dl[opc_newarr]		:= 0;
	dl[opc_refanyval]	:= 0;
	dl[opc_sizeof]		:= 1;
	dl[opc_stobj]		:= -2;
	dl[opc_unbox]		:= 0;

	dl[opc_ldstr]		:= 1;
	dl[opc_calli]		:= 0;		(* variable *)
	dl[opc_ldptr]		:= 1;
	dl[opc_ldtoken]		:= 1;
	dl[opc_switch]		:= -1;

END IlasmCodes.
(* ============================================================ *)
