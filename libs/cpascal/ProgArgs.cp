(* 
 *  Library module for GP Component Pascal.
 *  This module allows access to the arguments in programs which
 *  import CPmain.  It is accessible from modules which do NOT
 *  import CPmain.
 *
 *  Original : kjg December 1999
 *
 *  This is a dummy module, it exists only to cause the 
 *  generation of a corresponding symbol file: ProgArgs.cps
 *  when compiled with the -nocode flag.
 *)
SYSTEM MODULE ProgArgs;

  PROCEDURE ArgNumber*() : INTEGER;

  PROCEDURE GetArg*(num : INTEGER; OUT arg : ARRAY OF CHAR); 

  PROCEDURE GetEnvVar*(IN name : ARRAY OF CHAR; OUT valu : ARRAY OF CHAR); 

END ProgArgs.
