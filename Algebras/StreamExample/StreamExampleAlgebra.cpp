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

//paragraph [1] title: [{\Large \bf ]   [}]
//[->] [$\rightarrow$]



[1] Stream Example Algebra

July 2002 RHG

December 2005, Victor Almeida deleted the deprecated algebra levels
(~executable~, ~descriptive~, and ~hybrid~). Only the executable
level remains. Models are also removed from type constructors.

This little algebra demonstrates the use of streams and parameter functions
in algebra operators. It does not introduce any type constructors, but has
several operators to manipulate streams.


1 Preliminaries

1.1 Includes

*/

using namespace std;

#include "Algebra.h"
#include "NestedList.h"
#include "NList.h"
#include "QueryProcessor.h"
#include "AlgebraManager.h"
#include "StandardTypes.h"  //We need integers, for example
#include "Symbols.h"

#include <string>
#include <iostream>    //for testing

extern NestedList* nl;
extern QueryProcessor *qp;

using namespace symbols;

namespace ste {

/*

2 Creating Operators

2.1 Overview

This algebra provides the following operators:

  * intstream: int x int [->] (stream int)

    Creates a stream of integers containing all integers between the first and
the second argument. If the second argument is smaller than the first, the
stream will be empty.

  * count: (stream T) [->] int

    Returns the number of elements in an int stream.

  * printintstream: (stream int) [->] (stream int)

    Prints out all elements of the int stream

  * filter: (stream int) x (int [->] bool) [->] (stream int)

    Filters the elements of an int stream by a predicate.


2.2 Type Mapping Function

Checks whether the correct argument types are supplied for an operator; if so,
returns a list expression for the result type, otherwise the symbol
~typeerror~.


Type mapping for ~intstream~ is

----    (int int) -> (stream int)
----

*/
ListExpr
intstreamType( ListExpr args )
{  
  const string errMsg = "Type mapping function expects (int int) "
                        " but got " + nl->ToString(args);

  if ( nl->ListLength(args) != 2 )
    ErrorReporter::ReportError( errMsg );

  ListExpr arg1 = nl->First(args);
  ListExpr arg2 = nl->Second(args);
  
  if ( nl->IsEqual(arg1, INT) && nl->IsEqual(arg2, INT) )
    return nl->TwoElemList(nl->SymbolAtom(STREAM), nl->SymbolAtom(INT));

  ErrorReporter::ReportError( errMsg );
  return nl->TypeError();
}



/*
Type mapping for ~count~ is

----    ((stream int)) -> int
----

*/
ListExpr
countType( ListExpr args )
{
  ListExpr arg1 = nl->Empty();
  const string errMsg = "Operator count expects (stream int)";

  if ( nl->ListLength(args) == 1 )
  {
    arg1 = nl->First(args);

    if ( !nl->IsAtom(arg1) && nl->ListLength(arg1) == 2 )
    {
      if ( nl->IsEqual(nl->First(arg1), STREAM)
           && ( nl->IsEqual(nl->Second(arg1), INT))) 
      {
        return nl->SymbolAtom(INT);
      } 
      else
      {
        ErrorReporter::ReportError(errMsg);
      }
    }
  }
  ErrorReporter::ReportError(errMsg);
  return nl->TypeError();
}

/*
Type mapping for ~printintstream~ is

----    ((stream int)) -> (stream int)
----

*/
ListExpr
printintstreamType( ListExpr args )
{
  ListExpr arg11 = nl->Empty(), arg12 = nl->Empty();

  if ( nl->ListLength(args) == 1 )
  {     	  
    ListExpr arg = nl->First(args);
    if ( nl->ListLength(arg) == 2 ) 
    {    
      arg11 = nl->First(arg);
      arg12 = nl->Second(arg);

      if ( nl->IsEqual(arg11, STREAM) && nl->IsEqual(arg12, INT) )
        return nl->First(args);
    }
  }

  ErrorReporter::ReportError("Operator printintstream expects a "
                             "(stream int) as argument.");  
  return nl->TypeError();
}


/*
Type mapping for ~filter~ is

----    ((stream int) (map int bool)) -> (stream x)
----

*/
ListExpr
filterType( ListExpr args )
{
  ListExpr stream = nl->Empty();
  ListExpr map = nl->Empty();

  if ( nl->ListLength(args) == 2 )
  {
    stream = nl->First(args);
    map = nl->Second(args);

    // test first argument for stream(int)
    if ( nl->IsAtom(stream)
         || !(nl->ListLength(stream) == 2)
         || !nl->IsEqual(nl->First(stream), STREAM)
         || !nl->IsEqual(nl->Second(stream), INT) )
    {
      ErrorReporter::ReportError("Operator " + FILTER + 
		            "expects a (stream int) as its first argument.");

      return nl->TypeError();
    }

    // test second argument for map T' bool. T = T'
    if ( nl->IsAtom(map)
         || !nl->ListLength(map) == 3
         || !nl->IsEqual(nl->First(map), MAP)
         || !nl->IsEqual(nl->Second(map), INT)
         || !nl->IsEqual(nl->Third(map), BOOL) )
    {
      ErrorReporter::ReportError("Operator " + FILTER + " expects a "
                                 "(map int bool) as its second argument. ");
      return nl->TypeError();
    }
  }
  else 
  { // wrong number of arguments
    ErrorReporter::ReportError("Operator filter expects two arguments.");
    return nl->TypeError();
  }
  return stream; // return type of first argument
}

/*
4.2 Selection Function

Is used to select one of several evaluation functions for an overloaded
operator, based on the types of the arguments. In case of a non-overloaded
operator, we can use the simpleSelect function provided by the Operator class.

4.3 Value Mapping Function

*/

int
intstreamFun (Word* args, Word& result, int message, Word& local, Supplier s)
/*
Create integer stream. An example for creating a stream.

Note that for any operator that produces a stream its arguments are NOT
evaluated automatically. To get the argument value, the value mapping function
needs to use ~qp->Request~ to ask the query processor for evaluation explicitly.
This is illustrated in the value mapping functions below.

*/
{
  struct Range {  // an auxiliary record type
    int current;
    int last;

    Range(CcInt* i1, CcInt* i2) {

      if (i1->IsDefined() && i2->IsDefined()) 
      {	    
        current = i1->GetIntval();	    
        last = i2->GetIntval();	
      }
      else
      {
        current = 1;
        last = 0;
      }	
    }	    
  };
  
  Range* range = 0;
  CcInt* i1 = 0;
  CcInt* i2 = 0;
  CcInt* elem = 0;

  switch( message )
  {
    case OPEN: // initialize the local storage

      i1 = ((CcInt*)args[0].addr);
      i2 = ((CcInt*)args[1].addr);
      range = new Range(i1, i2);
      local.addr = range;

      return 0;

    case REQUEST: // return the next stream element

      range = ((Range*) local.addr);

      if ( range->current <= range->last )
      {
        elem = new CcInt(true, range->current++);
        result.addr = elem;
        return YIELD;
      }
      else 
      {
        result.addr = 0;	      
        return CANCEL;
      }	

    case CLOSE: // free the local storage

      range = ((Range*) local.addr);
      delete range;
      return 0;
  }
  /* should never happen */
  return -1;
}

/*
3 Value mapping for ~count~

Count the number of elements in a stream. An example for consuming a stream.

*/
int
countFun (Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word elem = SetWord(Address(0));
  int count = 0;

  qp->Open(args[0].addr);
  qp->Request(args[0].addr, elem);

  while ( qp->Received(args[0].addr) )
  {
    count++;
    ((Attribute*) elem.addr)->DeleteIfAllowed();// consume the stream objects
    qp->Request(args[0].addr, elem);
  }
  result = qp->ResultStorage(s);
  ((CcInt*) result.addr)->Set(true, count);

  qp->Close(args[0].addr);

  return 0;
}

/*
3 Value mapping ~printintstream~

The next function prints the elements of a "stream(int)". 
An example for a pure stream operator (input and output are streams).

*/

int
printintstreamFun (Word* args, Word& result, 
                   int message, Word& local, Supplier s)
{
  Word elem = SetWord(Address(0));

  switch( message )
  {
    case OPEN:

      qp->Open(args[0].addr);
      return 0;

    case REQUEST:

      qp->Request(args[0].addr, elem);
      if ( qp->Received(args[0].addr) )
      {
        cout << ((CcInt*) elem.addr)->GetIntval() << endl;
        result = elem;
        return YIELD;
      }
      else 
      {
	result = SetWord(Address(0));      
	return CANCEL;
      }	      

    case CLOSE:

      qp->Close(args[0].addr);
      return 0;
  }
  /* should not happen */
  return -1;
}

/*
3 Value mapping for ~filter~

Filter the elements of a stream by a predicate. An example for a stream
operator and also for one calling a parameter function.

*/
int
filterFun (Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word elem = SetWord(Address(0));
  Word funresult = SetWord(Address(0));
  ArgVectorPointer funargs = 0;

  switch( message )
  {
    case OPEN:

      qp->Open(args[0].addr);
      return 0;

    case REQUEST:

      // Get the argument vector for the parameter function.
      funargs = qp->Argument(args[1].addr);  

      // Loop over stream elements until the function yields true.
      qp->Request(args[0].addr, elem);
      while ( qp->Received(args[0].addr) )
      {
        // Supply the argument for the parameter function.
        (*funargs)[0] = elem;     

        // Instruct the parameter function to be evaluated.
        qp->Request(args[1].addr, funresult);
	CcBool* b = static_cast<CcBool*>( funresult.addr );

        bool funRes = b->IsDefined() && b->GetBoolval();

        if ( funRes )
        {
	  // TRUE: Element passes the filter condition	
          result = elem;
          return YIELD;
        }
	else
        {
           // FALSE: Element is rejected by the filter condition
	   
          // consume the stream object (allow deletion)
          static_cast<Attribute*>(elem.addr)->DeleteIfAllowed(); 

	  // Get next stream element
          qp->Request(args[0].addr, elem);
	}  
      }

      // End of Stream reached
      result = SetWord(Address(0));
      return CANCEL;

    case CLOSE:

      qp->Close(args[0].addr);
      return 0;
  }
  /* should never happen */
  return -1;
}

/*
4.4 Description of Operators

*/

struct intstreamInfo : OperatorInfo 
{
  intstreamInfo() : OperatorInfo()
  {
    name      = INTSTREAM; 
    signature = INT + " x " + INT + " -> stream(int)";
    syntax    = INTSTREAM + "(_ , _)";
    meaning   = "Creates a stream of integers containing the numbers "
                "between the first and the second argument.";
  }
};


struct countInfo :  OperatorInfo 
{
  countInfo() : OperatorInfo()
  {
    name      = COUNT; 
    signature = "stream(int)  -> " + INT;
    syntax    = "_" + COUNT;
    meaning   = "Counts the number of elements of an int stream.";
  }
};

struct printintSInfo :  OperatorInfo 
{
  printintSInfo() : OperatorInfo()
  {
    name      = PRINT_INTSTREAM; 
    signature = "stream(int)  -> stream(int)";
    syntax    = "_" + PRINT_INTSTREAM;
    meaning   = "Prints the elements int stream.";
  }
};

struct filterInfo :  OperatorInfo 
{
  filterInfo() : OperatorInfo()
  {
    name      = FILTER; 
    signature = "stream(int) x (int -> bool) -> stream(int)";
    syntax    = "_" + FILTER + "[ function ]";
    meaning   = "Filters the elements of an int stream by a predicate.";
  }
};


/*
5 Creating the Algebra

*/

class StreamExampleAlgebra : public Algebra
{
 public:
  StreamExampleAlgebra() : Algebra()
  {
    AddOperator( intstreamInfo(), intstreamFun, intstreamType );
    AddOperator( countInfo(), countFun, countType);
    AddOperator( printintSInfo(), printintstreamFun, printintstreamType );
    AddOperator( filterInfo(), filterFun, filterType );
  }
  ~StreamExampleAlgebra() {};
};

} // end of namespace ste

/*
6 Initialization

*/

extern "C"
Algebra*
InitializeStreamExampleAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  return (new ste::StreamExampleAlgebra);
}

