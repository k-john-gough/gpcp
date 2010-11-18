(* 
 *  Library module for GP Component Pascal.
 *  Low level reading and writing to the command-line console.
 *  Original : kjg November 1998
 *
 *
 *  This is a dummy module, it exists only to cause the 
 *  generation of a corresponding symbol file: StdIn.cps
 *  when compiled with the -special flag.
 *)
SYSTEM MODULE StdIn;

  PROCEDURE SkipLn*();  
  (* Read past next line marker *)

  PROCEDURE ReadLn*(OUT arr : ARRAY OF CHAR); 
  (* Read a line of text, discarding EOL *)

  PROCEDURE More*() : BOOLEAN; (* Return TRUE in gpcp v1.3! *)

  PROCEDURE Read*(OUT ch : CHAR); (* Get next character *)

END StdIn.
