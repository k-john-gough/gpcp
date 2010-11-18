(* 
 *  Library module for GP Component Pascal.
 *  This module name is "magic" in the sense that its name is known
 *  to the compiler. If it is imported, the module will be compiled
 *  so that its body is named "main" with an arglist, rather than
 *  being in the static initializer <clinit>()V in JVM-speak.
 *
 *  Original : kjg November 1998
 *
 *  This is a dummy module, it exists only to cause the 
 *  generation of a corresponding symbol file: CPmain.cps
 *  when compiled with the -special flag.
 *)
SYSTEM MODULE CPmain;

  PROCEDURE ArgNumber*() : INTEGER; 
  PROCEDURE GetArg*(num : INTEGER; OUT arg : ARRAY OF CHAR); 

END CPmain.
