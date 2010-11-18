(** This is the dummy module that sits in the runtime system, 
  * or will do if it ever gets a body.
  *
  * Version:	19 May      2001 (kjg).
  *)

SYSTEM MODULE ASCII;

  CONST
    NUL* = 00X;
    SOH* = 01X;
    STX* = 02X;
    ETX* = 03X;
    EOT* = 04X;
    ENQ* = 05X;
    ACK* = 06X;
    BEL* = 07X;
    BS*  = 08X;		(* backspace character *)
    HT*  = 09X;		(* horizontal tab character *)
    LF*  = 0AX;		(* line feed character *)
    VT*  = 0BX;
    FF*  = 0CX;
    CR*  = 0DX; 	(* carriage return character *)
    SO*  = 0EX;
    SI*  = 0FX;
    DLE* = 10X;
    DC1* = 11X;
    DC2* = 12X;
    DC3* = 13X;
    DC4* = 14X;
    NAK* = 15X;
    SYN* = 16X;
    ETB* = 17X;
    CAN* = 18X;
    EM*  = 19X;
    SUB* = 1AX;
    ESC* = 1BX;		(* escape character *)
    FS*  = 1CX;
    GS*  = 1DX;
    RS*  = 1EX;
    US*  = 1FX;
    SP*  = 20X;		(* space character  *)
    DEL* = 7FX;		(* delete character *)

END ASCII.
