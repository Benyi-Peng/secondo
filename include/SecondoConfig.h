/*
---- 
This file is part of SECONDO.

Copyright (C) 2004, University in Hagen, Department of Computer Science, 
Database Systems for New Applications.

SECONDO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

SECONDO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SECONDO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
----

//paragraph	[10]	title:		[{\Large \bf ] [}]
//paragraph	[21]	table1column:	[\begin{quote}\begin{tabular}{l}]	[\end{tabular}\end{quote}]
//paragraph	[22]	table2columns:	[\begin{quote}\begin{tabular}{ll}]	[\end{tabular}\end{quote}]
//paragraph	[23]	table3columns:	[\begin{quote}\begin{tabular}{lll}]	[\end{tabular}\end{quote}]
//paragraph	[24]	table4columns:	[\begin{quote}\begin{tabular}{llll}]	[\end{tabular}\end{quote}]
//[--------]	[\hline]
//characters    [1]     verbatim:       [$]         [$]
//characters    [2]     formula:        [$]         [$]
//characters    [3]     capital:        [\textsc{]  [}]
//characters    [4]     teletype:       [\texttt{]  [}]
//[ae] [\"a]
//[oe] [\"o]
//[ue] [\"u]
//[ss] [{\ss}]
//[<=] [\leq]
//[#]  [\neq]
//[tilde] [\verb|~|]

1 Header File: Secondo Configuration

January 2002 Ulrich Telle

1.1 Overview

The compilation of "Secondo"[3] has to take into account differences
in the architecture of the underlying operating system. Usually the
compiler defines symbols which can be used to identify the operating system
"Secondo"[3] is built for.

Depending on the actual operating system several constants and system
dependent macros are defined.

1.1 Imports, Types

*/

#ifndef SECONDO_CONFIG_H
#define SECONDO_CONFIG_H

#define SECONDO_VERSION            "2.0.1"
#define SECONDO_VERSION_MAJOR      2
#define SECONDO_VERSION_MINOR      0
#define SECONDO_VERSION_REVISION   1	

#define SECONDO_LITTLE_ENDIAN
/*
Define the preprocessor symbol "SECONDO\_LITTLE\_ENDIAN"[4] if your machine has a little
endian byte order architecture. Otherwise ~\#undef~ this symbol.

*TODO*: Detection of endianess should be done automatically.

*/

/*
1.1 Detect the platform

1.1.1 Windows

*/
#if (defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) \
  || defined(__MINGW32__)) && !defined(__CYGWIN__)
#  define SECONDO_WIN32
   // Define Windows version for WINVER and _WIN32_WINNT
   // 0x0400 = Windows NT 4.0
   // 0x0500 = Windows 2000 (NT based)
#ifndef WINVER 
#  define WINVER       0x0400
#endif
#ifndef _WIN32_WINNT
#  define _WIN32_WINNT WINVER 
#endif
#  define WIN32_LEAN_AND_MEAN
//#  include <windows.h>
/*
When creating or using shared libraries (i.e. DLLs = Dynamic Link Libraries)
on Windows platforms it is necessary to specify for C++ classes whether they
are imported from a DLL (when using a DLL) or exported from a DLL (when
creating a DLL).

Since the "Secondo"[3] system has separate components, namely the database kernel
and the storage management interface one must be able to differentiate between
these components.

Currently there are two components:

  * SDB  -- the "Secondo"[3] DataBase kernel.
Define SDB\_USE\_DLL if compiling modules which want to use a DLL for this component.
Define SDB\_CREATE\_DLL if compiling this component as a DLL.

  * SMI  -- the Storage Management Interface.
Define SMI\_USE\_DLL if compiling modules which want to use a DLL for this component.
Define SMI\_CREATE\_DLL if compiling this component as a DLL.

If none of these symbols is defined it is assumed that a static library is to be built.

*NOTE*: The GNU C++ compiler does not require these symbols to be present in
class definitions since all exportable symbols of a DLL are exported by default.
But to be compatible with compilers like Microsoft Visual C++ they should be
used.

*/
#  if defined(SDB_USE_DLL)
#    define SDB_EXPORT __declspec(dllimport)
#  elif defined(SECONDO_CREATE_DLL)
#    define SDB_EXPORT __declspec(dllexport)
#  else
#    define SDB_EXPORT
#  endif
#  if defined(SMI_USE_DLL)
#    define SMI_EXPORT __declspec(dllimport)
#  elif defined(SMI_CREATE_DLL)
#    define SMI_EXPORT __declspec(dllexport)
#  else
#    define SMI_EXPORT
#  endif
/*
1.1.1 Unix 

*/
#elif defined(__unix__) || defined(__APPLE__) || defined(sun)
#  define SECONDO_UNIX
/*
Creating shared libraries requires no special measures.

*/
#  define SDB_EXPORT
#  define SMI_EXPORT
/*

1.1.2 Linux 

*/
#if defined(__linux__)
#  define SECONDO_LINUX
/*
Creating shared libraries requires no special measures.

*/
#  define SDB_EXPORT
#  define SMI_EXPORT
#endif 
/*

1.1.3 Solaris

*/
#if defined(unix) && defined(sun)
#  define SECONDO_SOLARIS
/*
Creating shared libraries requires no special measures.

*/
#  define SDB_EXPORT
#  define SMI_EXPORT
#endif
/*

1.1.4 Mac OS X 

*/
#if defined(__APPLE__)
#  define SECONDO_MAC_OSX
/*
Creating shared libraries requires no special measures.

*/
#  define SDB_EXPORT
#  define SMI_EXPORT
#endif
 
#else
#  error Could not identify the operating system
#endif

/*
The following symbol must be defined in order to use the latest POSIX APIs:

*/
#ifdef __GNUC__
#  define _GNU_SOURCE 1
#endif

/*
Define separator character for pathnames in PATH environment variable:

*/
#ifdef SECONDO_WIN32
#  define PATH_SLASH "\\"
#  define PATH_SLASHCHAR '\\'
#  define PATH_SEP ";"
#  define PATH_SEPCHAR ';'
#else
#  define PATH_SLASH "/"
#  define PATH_SLASHCHAR '/'
#  define PATH_SEP ":"
#  define PATH_SEPCHAR ':'
#endif

/*
Number of elements in a static array:

*/
#define nelems(x) (sizeof((x))/sizeof(*(x)))

/*
Size of a structure member:

*/
#define sizeofm(struct_t,member) \
  ((size_t)(sizeof(((struct_t *)0)->member)))

/*
Default includes:

  * stdint.h -- defines standard integer types
(alternatively ~inttypes.h~ could be used where ~stdint.h~ is not available).

*/

#ifndef SECONDO_SOLARIS
#include <stdint.h>
#else
#include <inttypes.h>
#endif

#endif // SECONDO_CONFIG_H

