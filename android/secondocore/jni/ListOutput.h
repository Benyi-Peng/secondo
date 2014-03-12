/*
----
This file is part of SECONDO.

Copyright (C) 2004-2008, University in Hagen, Faculty of Mathematics and Computer Science,
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

1 The Implementation-Module SecondoInterface

September 1996 Claudia Freundorfer

December 23, 1996 RHG Added error code information.

January 17, 1998 RHG Connected Secondo Parser (= command level 1 available).

This module implements the module ~SecondoInterface~ by using the
modules ~NestedList~, ~SecondoCatalog~, ~QueryProcessor~ and
~StorageManager~.

May 15, 1998 RHG Added a command ~model value-expression~ which is
analogous to ~query value-expression~ but computes the result model for
a given query rather than the result value.

November 18, 1998 Stefan User commands ``abort transaction'' and
``commit transaction'' are implemented by calling SMI\_Abort() and
SMI\_Commit(), respectively.

April 2002 Ulrich Telle Port to C++

August 2002 Ulrich Telle Set the current algebra level for SecondoSystem.

September 2002 Ulrich Telle Close database after creation.

November 7, 2002 RHG Implemented the ~let~ command.

December 2002 M. Spiekermann Changes in Secondo(...) and NumTypeExpr(...).

February 3, 2003 RHG Added a ~list counters~ command.

April 29 2003 Hoffmann Added save and restore commands for single objects.

April 29, 2003 M. Spiekermann bug fix in LookUpTypeExpr(...).

April 30 2003 Hoffmann Changes syntax for the restore objects command.

September 2003 Hoffmann Extended section List-Commands for Secondo-Commands
~list algebras~ and ~list algebra <algebra name>~.

October 2003 M. Spiekermann made the command echo (printing out the command in
NL format) configurable.  This is useful for server configuration, since the
output of big lists consumes more time than processing the command.

May 2004, M. Spiekermann. Support of derived objects (for further Information
see DerivedObj.h) introduced.  A new command derive similar to let can be used
by the user to create objects which are derived from other objects via a more
or less complex value expression. The information about dependencies is stored
in two system tables (relation objects). The save database command omits to
save list expressions for those objects.  After restoring all saved objects the
derived objects are rebuild in the restore database command.

August 2004, M. Spiekermann. The complex nesting of function ~Secondo~ has been reduced.

Sept 2004, M. Spiekermann. A bug in the error handling of restore databases has been fixed.

Dec 2004, M. Spiekermann. The new command ~set~ was implemented to support
interactive changes of runtime parameters.

December 2005, Victor Almeida deleted the deprecated algebra levels
(~executable~, ~descriptive~, and ~hibrid~). Only the executable
level remains. Models are also removed from type constructors.

February 2006, M. Spiekermann. A proper handling of errors when commit or abort
transaction fails after command execution was implemented. Further, the scope of
variables in function Secondo was limited to a minimum, e.g. the declarations
were moved nearer to the usage. This gives more encapsulation and is easier to
understand and maintain.

April 2006, M. Spiekermann. Implementation of system tables SEC\_COUNTERS and SEC\_COMMANDS.

August 2006, M. Spiekermann. Bug fix for error messages of create or delete
database.  The error codes of the SMI module are now integrated into the error
reporting of the secondo interface. However, currently only a few SMI error
codes are mapped to strings.

September 2006, M. Spiekermann. System tables excluded into a separate file
called SystemTables.h.

April 2007, M. Spiekermann. Fixed bug concerning transaction management. Started
transactions of errorneous queries were not aborted.

\tableofcontents

*/

#include <string>
#include <iostream>
#include <stdlib.h>
#include "SecondoInterface.h"
#include "NestedList.h"
#include "NList.h"

#include <android/log.h>
#include <jni.h>

#ifndef LISTOUTPUT_H_
#define LISTOUTPUT_H_


class ListOutput {
public:
	ListOutput(NestedList *nl);
	virtual ~ListOutput();
	void resetZaehler();
	bool outputList(ListExpr li);
	NodeType GetBinaryType(ListExpr list);
	NestedList *nl;
	int zaehler;

private:
	static const byte BIN_LONGLIST = 0;
	static const byte BIN_INTEGER  = 1;
	static const byte BIN_REAL = 2;
	static const byte BIN_BOOLEAN = 3;
	static const byte BIN_LONGSTRING = 4;
	static const byte BIN_LONGSYMBOL = 5;
	static const byte BIN_LONGTEXT = 6;
	static const byte BIN_LIST = 10;
	static const byte BIN_SHORTLIST = 11;
	static const byte BIN_SHORTINT  = 12;
	static const byte BIN_BYTE = 13;
	static const byte BIN_STRING = 14;
	static const byte BIN_SHORTSTRING = 15;
	static const byte BIN_SYMBOL= 16;
	static const byte BIN_SHORTSYMBOL = 17;
	static const byte BIN_TEXT = 18;
	static const byte BIN_SHORTTEXT = 19;
	static const byte BIN_DOUBLE = 20;


};

#endif

