(* 
 *  Library module for GP Component Pascal.
 *  Low level reading and writing to the command-line console.
 *  Original : kjg November 1998
 *
 *
 *  This is a dummy module, it exists only to cause the 
 *  generation of a corresponding symbol file: Console.cps
 *  when compiled with the -special flag.
 *)
SYSTEM MODULE Console;

  PROCEDURE WriteLn*();

  PROCEDURE Write*(ch : CHAR);

  PROCEDURE WriteString*(IN str : ARRAY OF CHAR);

  PROCEDURE WriteInt*(val : INTEGER; width : INTEGER); 

  PROCEDURE WriteHex*(val : INTEGER; width : INTEGER);

END Console.
