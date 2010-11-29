(* 
 *  Library module for GP Component Pascal.
 *  This module name is "magic" in the sense that its name is known
 *  to the compiler. If it is imported, the module will be compiled
 *  so that its body is named "WinMain" with no arglist. 
 *
 *  Original : kjg CPmain November 1998
 *  Modified : kjg WinMain February 2004
 *
 *  This is a dummy module, it exists only to cause the 
 *  generation of a corresponding symbol file: WinMain.cps
 *  when compiled with the -special flag.
 *)
SYSTEM MODULE WinMain;
END WinMain.
