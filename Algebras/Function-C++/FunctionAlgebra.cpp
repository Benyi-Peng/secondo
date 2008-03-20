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

1 Implementation Module Function Algebra

January 26, 2001 RHG

April 2002 Ulrich Telle Port to C++

December 2005, Victor Almeida deleted the deprecated algebra levels
(~executable~, ~descriptive~, and ~hibrid~). Only the executable
level remains. Models are also removed from type constructors.

The sole purpose of this little algebra is to provide a type constructor ~map~
which can be used to store the list expressions defining functions
(abstractions).

March 2006 M. Spiekermann, operators ~within~ and ~within2~ modified.

*/

using namespace std;

#include "Algebra.h"
#include "NestedList.h"
#include "NList.h"
#include "QueryProcessor.h"
#include "AlgebraManager.h"
#include "StandardTypes.h"

extern NestedList* nl;
extern QueryProcessor *qp;
extern AlgebraManager *am;

namespace FunctionAlgebra{

/*
2.1 Dummy Functions

The next function defines the type property of type constructor ~map~.

*/
ListExpr
FunctionProperty()
{
  ListExpr remarkslist = nl->TextAtom();
  nl->AppendText(remarkslist,"Stores list expressions defining functions "
  "(internal use).");

  return (nl->TwoElemList(
            nl->OneElemList(nl->StringAtom("Remarks")),
            nl->OneElemList(remarkslist)));
}

Word
NoSpace( const ListExpr typeInfo )
{
  return (SetWord( Address( 0 ) ));
}

void
DoNothing( const ListExpr typeInfo, Word& w )
{
  w.addr = 0;
}

Word
CloneNothing( const ListExpr typeInfo, const Word& w )
{
  return SetWord( Address(0) );
}

int
SizeOfNothing()
{
  return 0;
}

/*
2.2 Type Constructor ~map~

*/
Word
InMap( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct )
{
/*
We don't do any checks here; any list expression will be accepted.
Errors will be found when the function is used, i.e., sent to the
query processor.

*/
  correct = true;
  return (SetWord( instance ));
}

ListExpr
OutMap( ListExpr typeInfo, Word value )
{
  return (value.list);
}

void*
DummyCast( void* addr )
{
  return (0);
}

int
NullSize()
{
  return 0;
}

bool
CheckMap( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( nl->First( type ), "map" ));
}

TypeConstructor functionMap( "map",             FunctionProperty,
                             OutMap,            InMap,
                             0,                 0,
                             NoSpace,           DoNothing,
                             0,                 0,
                             DoNothing,         CloneNothing,
                             DummyCast,         NullSize, CheckMap );

/*
2.3 Type Operators ~ANY~ and ~ANY2~

Type operators are used only for inferring argument types of parameter
functions. They have a type mapping but no evaluation function.

2.3.1 Type mapping function of operator ~ANY~

The type operator ~ANY~ corresponds to the type of the first argument.

----    (t1 t2 ... tn)      -> t1
----

The type operator ~ANY2~ corresponds to the type of the second argument.

----    (t1 t2 ... tn)      -> t2
----

*/
ListExpr ANYTypeMap( ListExpr args )
{
  NList type(args);
  if(type.length() >= 1)
  {
    return type.first().listExpr();
  }
  return NList::typeError(
      "Type Map Operator 'ANY' expects a parameter list of length >=1");
}

const string ANYSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Remarks\" )"
   "( <text>(t1 t2 ... tn) -> t1</text--->"
   "<text>type operator</text--->"
   "<text>Simply returns the type of the first argument.</text--->"
   "<text></text---> ))";

Operator ANY (
      "ANY",
      ANYSpec,
      0,
      Operator::SimpleSelect,
      ANYTypeMap );


ListExpr ANY2TypeMap( ListExpr args )
{
  NList type(args);
  if(type.length() >= 2)
  {
    return type.second().listExpr();
  }
  return NList::typeError(
      "Type Map Operator 'ANY2' expects a parameter list of length >=2");
}

const string ANY2Spec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Remarks\" )"
   "( <text>(t1 t2 ... tn) -> t1 -> t2</text--->"
   "<text>type operator</text--->"
   "<text>Simply returns the type of the second argument.</text--->"
   "<text></text---> ))";

Operator ANY2 (
      "ANY2",
      ANY2Spec,
      0,
      Operator::SimpleSelect,
      ANY2TypeMap );



/*
2.4 Operator ~within~

2.4.1 Type mapping function of operator ~within~

Result type of within operation.

----    ( a (map a b) ) -> b
----    ( a b (map a b c) ) -> c

*/
ListExpr WithinTypeMap(ListExpr Args)
{
  NList args(Args);
  NList mapResult;

  static const string typeA = "(obj_a (map type_a type_b))";
  static const string typeB = "(obj_a obj_b (map type_a type_b type_c))";

  bool ok = false;

  try {
  switch (args.length())
  {
    case 2: {

      if (  args.second().hasLength(3) )
      {
        ok =      ( args.first() == args.second().second())
               && ((args.second().first()) == string("map") );
      }

      if (!ok) {
        return NList::typeError("Input list has not structure " + typeA + ".");
      }
      mapResult = args.second().third();
      break;
    }

    case 3: {

      if ( args.third().hasLength(4) )
      {
        ok = (args.third().first() == string("map"))
             && (args.first() == args.third().second())
             && (args.second() == args.third().third());
      }

      if (!ok) {
        return NList::typeError("Input list has not structure " + typeB + ".");
      }
      mapResult = args.third().fourth();
      break;
    }

    default:
    {
      return NList::typeError("Input list has not structure " + typeA +
                              " or " + typeB + ".");
    }

  }

  if ( !(args.first() == mapResult) )
  {
    NList::typeError(
      "Operator within expects that the first argument and the argument\n"
      "of the mapping function are equal, but gets\n"
      "First argument: " + args.first().convertToString() + "\n" +
      "Mapping argument: " +  mapResult.convertToString() + "."
    );
  }

  } catch ( NListErr e ) {
    return NList::typeError( e.msg() );
  }
  cout << ":::::::::::: " << mapResult << endl;
  return mapResult.listExpr();
}


/*
2.4.3 Value mapping function of operator ~within~

*/
int
Within_s(Word* args, Word& result, int message, Word& local, Supplier s)
{
  ArgVectorPointer funArgs;
  Word w;

  switch ( message )
  {
    case OPEN:
      funArgs = qp->Argument( args[1].addr );
      qp->Request( args[0].addr, w );
      (*funArgs)[0] = w;
      qp->Open( args[1].addr );
      return 0;

    case REQUEST:
      qp->Request( args[1].addr, result );
      if( qp->Received( args[1].addr ) )
        return YIELD;
      return CANCEL;

    case CLOSE:
      qp->Close( args[1].addr );
      return 0;
  }

  return 0;
}

int
Within2_s(Word* args, Word& result, int message, Word& local, Supplier s)
{
  ArgVectorPointer funArgs;
  Word w;

  switch ( message )
  {
    case OPEN:
      funArgs = qp->Argument( args[1].addr );
      qp->Request( args[0].addr, w );
      (*funArgs)[0] = w;
      qp->Open( args[1].addr );
      return 0;

    case REQUEST:
      qp->Request( args[1].addr, result );
      if( qp->Received( args[1].addr ) )
        return YIELD;
      return CANCEL;

    case CLOSE:
      qp->Close( args[1].addr );
      return 0;
  }

  return 0;
}


int
Within_o(Word* args, Word& result, int message, Word& local, Supplier s)
{
  ArgVectorPointer funArgs = qp->Argument( args[1].addr );
  (*funArgs)[0] = args[0];
  qp->Request( args[1].addr, result );

  return 0;
}

int
Within2_o(Word* args, Word& result, int message, Word& local, Supplier s)
{
  ArgVectorPointer funArgs = qp->Argument( args[2].addr );
  (*funArgs)[0] = args[0];
  (*funArgs)[1] = args[1];
  qp->Request( args[2].addr, result );

  return 0;
}

ValueMapping withinmap[]  = { Within_o,  Within_s  };
ValueMapping within2map[] = { Within2_o, Within2_s };

/*
2.4.2 Selection function of operator ~within~

Return values 0,2 are used for within and values 1,3 for operator within2

*/
int
WithinSelect( ListExpr Args )
{
  NList args(Args);

  TRACE("WithinSelect")
  SHOW(args)

  try {
    NList map;
    NList mapRes;

    if ( args.length() == 2)
    { // for within
      map = args.second();
      mapRes = map.third();
    }
    else
    { // for within2
      map = args.third();
      mapRes = map.fourth();
    }

    if( mapRes.isNoAtom() && mapRes.first().str() == "stream" )
    {
      return 1;
    }
    else
    {
      return 0;
    }

  } catch ( NListErr e ) {

    NList::typeError( e.msg() );
    return -1;
  }

  cmsg.error()
    << "Can't select value mapping function for"
    << " the overloaded operator 'within'.";
  cmsg.send();
  return -1;
}

/*

2.4.3 Specification of operator ~within~

*/
const string WithinSpec  = "( ( \"Signature\" \"Syntax\" \"Meaning\" "
                        "\"Example\" ) "
                        "( <text>a x (a -> stream(b)) -> stream(b)\n"
                        " a x ( a -> c) -> c</text--->"
                        "<text>_ within [ fun ]</text--->"
                        "<text>Calls the function passing as argument "
                        "its own first argument.</text--->"
                        "<text>query plz createbtree[Ort] "
                        "within[fun( index: ANY ) "
                        "Orte feed {o} loopjoin[index plz "
                        "exactmatch[.Ort_o]] consume]</text--->))";

const string Within2Spec  = "( ( \"Signature\" \"Syntax\" \"Meaning\" "
    "\"Example\" ) "
    "( <text>a x b x ( a x b -> c) -> c\n"
    "a x b x ( a x b -> stream(c)) -> stream(c)</text--->"
    "<text>_ _ within2[ fun ]</text--->"
    "<text>Calls the function passing as argument "
    "its own first and second argument.</text--->"
    "<text>(1+2) (2+3) within2[fun(I1: ANY, I2: ANY2) I1 + I2]</text--->))";

/*
2.4.3 Definition of operator ~within~, ~within2~

*/
Operator within (
         "within",               // name
         WithinSpec,             // specification
         2, 	                   // the number of overloaded functions
         withinmap,              // value mapping function array
         WithinSelect,           // the selection function
         WithinTypeMap           // type mapping
);

Operator within2 (
    "within2",               // name
     Within2Spec,            // specification
     2,                      // the number of overloaded functions
     within2map,             // value mapping function array
     WithinSelect,           // the selection function
     WithinTypeMap           // type mapping
                );

/*
2.5 Operator ~whiledo~

Performs some while-loop like iteration on an object.

*/


/*
2.5.1 Type Mapping for ~whiledo~

----
      T x (T --> bool) x (T --> T) --> stream(T)
      where T in kind DATA or T = tuple(X)
----

*/

ListExpr WhileDoTypeMap(ListExpr Args)
{
  cout << __PRETTY_FUNCTION__ << " called." << endl;

  NList args(Args);
  static const string typeA =
      "(T (map T bool) (map T T)), where T in DATA or T = tuple(X)";
  ListExpr errorInfo = nl->OneElemList(nl->SymbolAtom("ERROR"));

  if(    args.hasLength(3)
     // second argument
      && args.second().hasLength(3)
      && args.second().first()  == Symbols::MAP()
      && args.second().second() == args.first()
      && args.second().third()  == Symbols::BOOL()
     // third argument
      && args.third().hasLength(3)
      && args.third().first()  == Symbols::MAP()
      && args.third().second() == args.first()
      && args.third().third()  == args.first()
    )
  {
    if( // first argument
        SecondoSystem::GetAlgebraManager()
           ->CheckKind("DATA", args.first().listExpr(), errorInfo)
      )
    {  // case: T x (T --> bool) x (T --> T) --> T, where T in DATA
      NList restype(Symbols::STREAM(), args.first());
      return restype.listExpr();
    }
    else if(
               args.first().hasLength(2)
            && args.first().first() == Symbols::TUPLE()
            && (args.first().second().length() > 0)
            // Only the first attribute is checked!
            && args.first().second().first().hasLength(2)
            && args.first().second().first().first().isSymbol()
            && SecondoSystem::GetAlgebraManager()
                ->CheckKind("DATA",
                            args.first().second().first().first().listExpr(),
                            errorInfo
                           )
           )
    {  // case: T x (T --> bool) x (T --> T) --> T, where T = tuple(X)
      NList restype(Symbols::STREAM(), args.first());
      return restype.listExpr();
    }
    // else: error
  }
  // else: error
  return NList::typeError("Expected " + typeA + ".");
}

/*
2.5.2 Value Mapping for ~whiledo~

*/

struct WhileDoValueMapLocalInfo{
  Word lastInstance;
  Word pred;
  Word fun;
  bool finished;
};

int WhileDoValueMap(Word* args, Word& result,
                    int message, Word& local, Supplier s)
{
  Word              predResult, funResult;
  ArgVectorPointer  predArg,    funArg;
  WhileDoValueMapLocalInfo* sli = 0;

  switch (message)
  {
    case OPEN :
      sli = new WhileDoValueMapLocalInfo;
      sli->lastInstance = SetWord(((Attribute*)(args[0].addr))->Copy());
      sli->pred         = args[1];
      sli->fun          = args[2];
      sli->finished     = false;
      local = SetWord(sli);
      return 0;

    case REQUEST :

      if( local.addr == 0 )
      {
        result = SetWord(Address(0));
        return CANCEL;
      }
      sli = (WhileDoValueMapLocalInfo*)local.addr;

      if (sli->finished)
      {
        result = SetWord(Address(0));
        return CANCEL;
      }
      // For each REQUEST, we check if we have already finished,
      // If not, we pass the last instance to the parameter function
      // and evalute the latter.
      // If the new instance is the same as the last one. we have reached a
      // fixpoint set finished, and return the result (so that the user may
      // notice that a fixpoint has been reached).
      // Otherwise, the new instance is passed to the parameter predicate
      // function. If it evaluates to UNDEF or FALSE, we set finished,
      // otherwise not.
      predArg = qp->Argument(sli->pred.addr);   // set argument for the
      (*predArg)[0] = sli->lastInstance;        //   parameter predicate
      qp->Request(sli->pred.addr, predResult);  // call predicate function
      if( !(static_cast<CcBool*>(predResult.addr)->IsDefined()) )
      { // UNDEF pred result
        sli->finished = true;
        result = SetWord(Address(0));
        return CANCEL;
      }
      sli->finished = !(static_cast<CcBool*>(predResult.addr)->GetBoolval());
      if(sli->finished)
      { // Does predicate hold on last instance?
        result = SetWord(Address(0));
        return CANCEL;
      }
      // Create the next instance
      funArg  = qp->Argument(sli->fun.addr);    // set argument for the
      (*funArg)[0] = sli->lastInstance;         //   parameter function
      qp->Request(sli->fun.addr, funResult);    // call parameter function
      // copy result (and hand it over to the out-stream):
      result = SetWord(((Attribute*) (funResult.addr))->Clone());
      if (((Attribute*) (funResult.addr))
          ->Compare(((Attribute*) (sli->lastInstance.addr))) == 0
         )
      { // reached a fixpoint: changeing sli->lastInstance not required
        sli->finished = true;
        return YIELD;
      }
      ((Attribute*) (sli->lastInstance.addr))->DeleteIfAllowed();
      sli->lastInstance = SetWord(((Attribute*)(result.addr))->Copy());
      return YIELD;

    case CLOSE :

      if( local.addr != 0 )
      {
        sli = (WhileDoValueMapLocalInfo*)local.addr;
        ((Attribute*) (sli->lastInstance.addr))->DeleteIfAllowed();
        delete sli;
        local = SetWord(Address(0));
      }
      return 0;

  }  // end switch
  cout << "WhileDoValueMap received UNKNOWN COMMAND" << endl;
  return -1; // should not be reached
}
/*
2.5.1 Type Mapping for ~whiledo~

*/


const string WhileDoSpec  = "( ( \"Signature\" \"Syntax\" \"Meaning\" "
    "\"Example\" ) "
    "( <text>T x (T -> bool) x (T -> T) -> stream(T), where T in kind DATA\n"
    "tuple(X) x (tuple(X) -> bool) x (tuple(X) -> tuple(X)) -> stream(tuple(X))"
    "</text--->"
    "<text>obj whiledo[ pred ; func ]</text--->"
    "<text>Operates function 'func' iteratively on an object initialized with "
    "'obj' while 'pred' evaluates to TRUE on the currect instance of 'obj' "
    "and returns each instanciation of 'obj' within the result stream. If a "
    "fixpoint is reached during the evaluation ('obj' does not change), the "
    "processing is stopped. In this case, the two last results will be "
    "identical. If 'pred' evaluates to UNDEF, the iteration stops.</text--->"
    "<text>query 1 whiledo[ . < 10 ; . + 1] count"
    "</text--->))";


Operator whiledo (
    "whiledo",               // name
    WhileDoSpec,             // specification
    WhileDoValueMap,         // value mapping function array
    Operator::SimpleSelect,  // the selection function
    WhileDoTypeMap           // type mapping
);


/*
3 Creating the Algebra

*/
class FunctionAlgebra : public Algebra
{
 public:
  FunctionAlgebra() : Algebra()
  {
    AddTypeConstructor( &functionMap );
    AddOperator( &ANY );
    AddOperator( &ANY2 );
    AddOperator( &within );
    AddOperator( &within2 );
    AddOperator( &whiledo );
  }
  ~FunctionAlgebra() {};
};

FunctionAlgebra functionAlgebra;

/*
4 Initialization

Each algebra module needs an initialization function. The algebra manager
has a reference to this function if this algebra is included in the list
of required algebras, thus forcing the linker to include this module.

The algebra manager invokes this function to get a reference to the instance
of the algebra class and to provide references to the global nested list
container (used to store constructor, type, operator and object information)
and to the query processor.

The function has a C interface to make it possible to load the algebra
dynamically at runtime.

*/

extern "C"
Algebra*
InitializeFunctionAlgebra( NestedList* nlRef,
                           QueryProcessor* qpRef,
                           AlgebraManager* amRef )
{
  nl = nlRef;
  qp = qpRef;
  am = amRef;
  return (&functionAlgebra);
}

} // end of namespace FunctionAlgebra
