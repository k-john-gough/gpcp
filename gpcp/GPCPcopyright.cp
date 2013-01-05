MODULE GPCPcopyright;
  IMPORT Console;

  CONST
     (*	VERSION    = "0.1 of 26 December 1999"; *)
     (*	VERSION    = "0.2 of 01 March 2000"; *)
     (*	VERSION    = "0.3 of 22 April 2000"; *)
     (*	VERSION    = "0.4 of 01 May 2000"; *)
     (*	VERSION    = "0.5 of 10 June 2000"; *)
     (*	VERSION    = "0.6 of 17 June 2000"; *)
     (*	VERSION    = "0.7 of 18 June 2000"; *)
     (*	VERSION    = "0.8 of 18 July 2000"; *)
     (*	VERSION    = "0.9 of 27 August 2000"; *)
     (*	VERSION    = "0.95 of 26 October 2000"; *)
     (*	VERSION    = "0.96 of 18 November 2000"; *)
     (*	VERSION    = "1-d of 22 January 2001"; *)
     (*	VERSION    = "1.0 of 01 June 2001"; *)
     (*	VERSION    = "1.06 of 25 August 2001"; *)
     (*	VERSION    = "1.10 of 10 September 2001"; *)
     (*	VERSION    = "1.1.3 of 26 November 2001"; *)
     (*	VERSION    = "1.1.4 of 16 January 2002"; *)
     (*	VERSION    = "1.1.5x of 23 January 2002"; *)
     (*	VERSION    = "1.1.6 of 28 March 2002"; *)
     (*	VERSION    = "1.1.6a of 10 May 2002"; *)
     (*	VERSION    = "1.1.7e of 23 May 2002"; *)
     (*	VERSION    = "1.2.0 of 12 September 2002"; *)
     (*	VERSION    = "1.2.1 of 07 October 2002"; *)
     (*	VERSION    = "1.2.2 of 07 January 2003"; *)
     (*	VERSION    = "1.2.3 of 01 April 2003"; *)
     (* VERSION    = "1.2.3.1 of 16 April 2003"; *)
     (*	VERSION    = "1.2.3.2 of 20 July 2003"; *)
     (*	VERSION    = "1.2.3.3 of 16 September 2003"; *)
     (*	VERSION    = "1.2.4   of 26 February 2004"; *)
     (*	VERSION    = "1.2.x   of June+ 2004"; *)
     (*	VERSION    = "1.3.0 of 16 September 2004"; *)
     (*	VERSION    = "1.3.1 of 12 November 2004"; *)
     (*	VERSION    = "1.3.1.1 of 1 April 2005"; *)
     (* VERSION    = "1.3.1.2 of 10 April 2005"; *)
     (* VERSION    = "1.3.2.0 of 1 May 2005"; *)
     (* VERSION    = "1.3.4 of 20 August 2006"; *)
     (* VERSION    = "1.3.4e of 7 November 2006"; *)
     (* VERSION    = "1.3.6 of 1 September 2007"; *)
     (* VERSION    = "1.3.8 of 18 November 2007"; *)
     (* VERSION    = "1.3.9 of 15 January 2008"; *)
     (* VERSION    = "1.3.10 of 15 November 2010"; *)
     (* VERSION    = "1.3.12 of 17 November 2011"; *)
     (* VERSION    = "1.3.13 of 24 July 2012"; *)
     (* VERSION    = "1.3.14 of 05 September 2012"; *)
     (* VERSION    = "1.3.15 of 04 October 2012"; *)
        VERSION    = "1.3.16 of 01 January 2013"; 
	verStr*    = " version " + VERSION;

  CONST	prefix     = "#gpcp: ";
	millis     = "mSec";

(* ==================================================================== *)

  PROCEDURE V*() : POINTER TO ARRAY OF CHAR;
  BEGIN 
    RETURN BOX(VERSION) 
  END V; 

  PROCEDURE W(IN s : ARRAY OF CHAR);
  BEGIN Console.WriteString(s); Console.WriteLn END W;

  PROCEDURE Write*();
  BEGIN
    W("GARDENS POINT COMPONENT PASCAL");
    W("The files which import this module constitute a compiler");
    W("for the programming language Component Pascal.");
    W("Copyright (c) 1998 -- 2013 K John Gough.");
    W("Copyright (c) 2000 -- 2013 Queensland University of Technology.");
    Console.WriteLn;

    W("This program is free software; you can redistribute it and/or modify");
    W("it under the terms of the GPCP Copyright as included with this");
    W("distribution in the root directory.");
    W("See the file GPCPcopyright.rtf in the 'gpcp' directory for details.");
    Console.WriteLn;

    W("This program is distributed in the hope that it will be useful,");
    W("but WITHOUT ANY WARRANTY as is explained in the copyright notice.");
    Console.WriteLn;

    W("The authoritative version for this program, and all future upgrades");
    W("is at http://gpcp.codeplex.com. The project page on CodePlex allows");
    W("discussions, an issue tracker and source code repository");
    W("The program's news group is GPCP@yahoogroups.com.");
  END Write;

END GPCPcopyright.
