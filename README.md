# Gardens Point Component Pascal (_GPCP_)

## Getting Started
* [Download the latest release](https://github.com/k-john-gough/gpcp/releases)

## Project Description
Gardens Point Component Pascal is an implementation of the Component Pascal Language (CP).  There are implementations for both the CLR and the JVM.  

Component Pascal is an object oriented language in the Pascal family.  Its closest relatives are Oberon-2 and Oberon.

Gardens Point Component Pascal (_gpcp_) is an implementation of the language which was created for both the .NET and JVM.  Both versions are here.

_gpcp_ for .NET was one of the language implementations created for the .NET platform as part of Microsoft's "Project-7" which demonstrated the cross-language capability of .NET.  The first release of the language was in mid-2000, at the Professional Developer's Conference, where .NET was announced.  The language was also used as the running example in the book "Compiling for the .NET Common Language Runtime" (Prentice-Hall 2002).

_gpcp_ provides a number of extensions to the standard Component Pascal language.  These extensions are provided to allow programs to access the rich capabilities of the base class libraries of the .NET system.  Thus types defined in _gpcp_ programs are able to declare that they implement interfaces, or provide methods that override protected methods defined in other languages.  Although Component Pascal does not allow the definition of overloaded methods, overloaded methods may be called from Component Pascal code.  All such language extensions may be turned off by a command line option.

As well as the compiler, the distribution contains a number of other utilities
* The program _CPMake_ which performs a minimal consistent compilation of a set of Component Pascal modules, respecting the module dependencies explicitly declared in the source files.
* The program _PeToCps_ which creates a _gpcp_ symbol file from a nominated "foreign language" PE file.  Such symbol files are used by _gpcp_ to perform type-safe separate compilation of Component Pascal modules that import foreign language libraries.
* The program _Browse_ produces a readable, hyperlinked representation of _gpcp_ symbol files.
