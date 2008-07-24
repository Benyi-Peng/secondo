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

//paragraph [1] title: [{\Large \bf ]  [}]


[1] TupleIdentifier Algebra

March 2005 Matthias Zielke

December 2005, Victor Almeida deleted the deprecated algebra levels
(~executable~, ~descriptive~, and ~hibrid~). Only the executable
level remains. Models are also removed from type constructors.

The only purpose of this algebra is to provide a typeconstructor 'tid' so that the tupleidentifiers
of tuples from relations can be stored as attribute-values in different tuples. This feature is needed
for the implementation of operators to update relations.

1 Preliminaries

1.1 Includes

*/

using namespace std;

#include "RelationAlgebra.h"
#include "TupleIdentifier.h"

extern NestedList* nl;
extern QueryProcessor *qp;


/*

2 Type Constructor ~tid~

*/

void TupleIdentifier::CopyFrom(const StandardAttribute* attr)
{
  const TupleIdentifier* tupleI = (const TupleIdentifier*) attr;
  defined = tupleI->IsDefined();
  tid = tupleI->GetTid();
}

bool TupleIdentifier::Adjacent( const Attribute* arg ) const
{
  TupleId argTid = ((const TupleIdentifier *)arg)->GetTid();

  return( tid == argTid -1 || tid == argTid + 1 );
}

TupleIdentifier::TupleIdentifier(bool DEFINED, TupleId TID)
{
  defined = DEFINED;
  tid = TID;
  del.refs = 1;
  del.isDelete = true;
}

TupleIdentifier::TupleIdentifier(const TupleIdentifier& source){
   defined = source.defined;
   tid = source.tid;
   del.refs = 1;
   del.isDelete=true;
}


TupleIdentifier::~TupleIdentifier() {}

TupleId TupleIdentifier::GetTid() const {return tid;}

void TupleIdentifier::SetTid(TupleId TID)
{tid = TID; defined = true;}

TupleIdentifier* TupleIdentifier::Clone() const
{ return new TupleIdentifier( *this ); }


ostream& TupleIdentifier::Print(ostream& out) const
{
  out << tid;
  return  out;
}


/*
2.2 List Representation

The list representation of a TupleIdentifier is

----    (tid)
----

2.3 ~In~ and ~Out~ Functions

*/

ListExpr
OutTupleIdentifier( ListExpr typeInfo, Word value )
{
  TupleIdentifier* tupleI = (TupleIdentifier*)(value.addr);
  return nl->IntAtom(tupleI->GetTid());
}

Word
InTupleIdentifier( const ListExpr typeInfo, const ListExpr instance,
          const int errorPos, ListExpr& errorInfo, bool& correct )
{
  if ( nl->IsAtom(instance) && nl->AtomType(instance) == IntType)
  {
    correct = true;
    TupleIdentifier* newTid = new TupleIdentifier(true, nl->IntValue(instance));
    return SetWord(newTid);
  }
  correct = false;
  return SetWord(Address(0));
}

/*
2.4 Functions Describing the Signature of the Type Constructors

This one works for type constructor ~tid~.

*/
ListExpr
TupleIdentifierProperty()
{
  return (nl->TwoElemList(
            nl->FiveElemList(nl->StringAtom("Signature"),
              nl->StringAtom("Example Type List"),
                  nl->StringAtom("List Rep"),
                  nl->StringAtom("Example List"),
                  nl->StringAtom("Remarks")),
            nl->FiveElemList(nl->StringAtom("-> DATA"),
              nl->StringAtom("tid"),
                  nl->StringAtom("(<tid>)"),
                  nl->StringAtom("(50060)"),
                  nl->StringAtom("The tupleidentifier is a long."))));
}
Word
CreateTupleIdentifier( const ListExpr typeInfo )
{
  return (SetWord( new TupleIdentifier(true, 0) ));
}

void
DeleteTupleIdentifier( const ListExpr typeInfo, Word& w )
{
  delete (TupleIdentifier *)w.addr;
  w.addr = 0;
}

void
CloseTupleIdentifier( const ListExpr typeInfo, Word& w )
{
  delete (TupleIdentifier *)w.addr;
  w.addr = 0;
}

Word
CloneTupleIdentifier( const ListExpr typeInfo, const Word& w )
{
  return SetWord( ((TupleIdentifier *)w.addr)->Clone() );
}

int
SizeOfTupleIdentifier()
{
  return sizeof(TupleIdentifier);
}

/*
2.5 Kind Checking Function

This function checks whether the type constructor is applied correctly. Since
type constructor ~TupleIdentifier~ does not have arguments, this is trivial.

*/
bool
CheckTupleIdentifier( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "tid" ));
}

/*
2.10 ~Cast~-Function

*/
void* CastTupleIdentifier( void* addr )
{
  return new (addr) TupleIdentifier;
}
/*
2.6 Creation of the Type Constructor Instance

*/
TypeConstructor tupleIdentifier
(
 "tid",
   //name
 TupleIdentifierProperty,
   //property function describing signature
 OutTupleIdentifier, InTupleIdentifier,
   //Out and In functions
 0, 0,
   //SaveToList and RestoreFromList functions
 CreateTupleIdentifier, DeleteTupleIdentifier,
   //object creation and deletion
 0, 0, CloseTupleIdentifier, CloneTupleIdentifier,
   //object open,save,close,clone
 CastTupleIdentifier,
   //cast function
 SizeOfTupleIdentifier,
   //sizeof function
 CheckTupleIdentifier );
   //kind checking function

/*
3 Operators

3.1 Operator ~tupleid~

Returns the tuple identifier.

3.1.1 Type mapping function of operator ~tupleid~

Operator ~tupleid~ accepts a tuple and returns an integer.

----    (tuple x)           -> tid
----

*/
ListExpr
TupleIdTypeMap(ListExpr args)
{
  ListExpr first;
  string argstr;

  CHECK_COND(nl->ListLength(args) == 1,
  "Operator tupleid expects a list of length one.");

  first = nl->First(args);

  nl->WriteToString(argstr, first);
  CHECK_COND(  nl->ListLength(first) == 2 &&
               TypeOfRelAlgSymbol(nl->First(first)) == tuple &&
               IsTupleDescription(nl->Second(first)),
  "Operator tupleid expects a list with structure "
  "(tuple ((a1 t1)...(an tn)))\n"
  "Operator tupleid gets a list with structure '" + argstr + "'.");

  return nl->SymbolAtom("tid");
}

/*
3.1.2 Value mapping function of operator ~tupleid~

*/
int
TIDTupleId(Word* args, Word& result, int message, Word& local, Supplier s)
{
  Tuple* t = (Tuple*)args[0].addr;
  result = qp->ResultStorage(s);
  ((TupleIdentifier *) result.addr)->SetTid( t->GetTupleId() );
  return 0;
}

/*
3.1.3 Specification of operator ~tupleid~

*/
const string TupleIdSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" \"Comment\" \" \" \" \" ) "
  "( <text>(tuple x) -> int</text--->"
  "<text>tupleid( _ )</text--->"
  "<text>Returns the identification of the tuple.</text--->"
  "<text>query cities feed filter[ tupleid(.) < 100 ]"
  " consume</text--->"
  "<text>Apply tupleid(_) directly after a feed, because </text--->"
  "<text>other operators my corrupt the tid </text--->"
  "<text>(in-memory tuples all have tid=0).</text--->"
  ") )";

/*
3.1.4 Definition of operator ~tupleid~

*/
Operator tidtupleid (
         "tupleid",             // name
         TupleIdSpec,           // specification
         TIDTupleId,            // value mapping
         Operator::SimpleSelect,         // trivial selection function
         TupleIdTypeMap         // type mapping
);

/*
3.2 Operator ~addtupleid~

Appends the tuple identifier as an attribute in the stream of tuples.

3.2.1 Type mapping function of operator ~addtupleid~

Operator ~addtupleid~ accepts a stream of tuples and returns the same stream
with the tuple identifier attribute in the end.

----    (stream (tuple ((x1 t1) ... (xn tn))))   ->
        (stream (tuple ((x1 t1) ... (xn tn) (id tid))))
----

*/
ListExpr
AddTupleIdTypeMap(ListExpr args)
{
  ListExpr first;
  string argstr;

  CHECK_COND(nl->ListLength(args) == 1,
  "Operator addtupleid expects a list of length one.");

  first = nl->First(args);

  nl->WriteToString(argstr, first);
  CHECK_COND(  nl->ListLength(first) == 2 &&
               TypeOfRelAlgSymbol(nl->First(first)) == stream &&
               TypeOfRelAlgSymbol(nl->First(nl->Second(first))) == tuple &&
               IsTupleDescription(nl->Second(nl->Second(first))),
  "Operator addtupleid expects a list with structure "
  "(stream (tuple ((a1 t1)...(an tn))))\n"
  "Operator addtupleid gets a list with structure '" + argstr + "'.");

  ListExpr rest = nl->Second(nl->Second(first));
  ListExpr newAttrList = nl->TheEmptyList();
  ListExpr lastNewAttrList = nl->TheEmptyList();
  bool firstcall = true;

  while (!nl->IsEmpty(rest))
  {
    first = nl->First(rest);
    rest = nl->Rest(rest);

    if (firstcall)
    {
      firstcall = false;
      newAttrList = nl->OneElemList(first);
      lastNewAttrList = newAttrList;
    }
    else
    {
      lastNewAttrList = nl->Append(lastNewAttrList, first);
    }
  }
  lastNewAttrList = nl->Append(lastNewAttrList,
                               nl->TwoElemList(
                                 nl->SymbolAtom("id"),
                                 nl->SymbolAtom("tid")));

  return nl->TwoElemList(
           nl->SymbolAtom("stream"),
           nl->TwoElemList(
             nl->SymbolAtom("tuple"),
             newAttrList));
}

/*
3.2.2 Value mapping function of operator ~addtupleid~

*/
int
TIDAddTupleId(Word* args, Word& result, int message, Word& local, Supplier s)
{
  TupleType *resultTupleType;
  ListExpr resultType;
  Word t;

  switch (message)
    {
    case OPEN :

      qp->Open(args[0].addr);
      resultType = GetTupleResultType( s );
      resultTupleType = new TupleType( nl->Second( resultType ) );
      local = SetWord( resultTupleType );
      return 0;

    case REQUEST :

      resultTupleType = (TupleType *)local.addr;
      qp->Request(args[0].addr,t);
      if (qp->Received(args[0].addr))
      {
        Tuple *tup = (Tuple*)t.addr;
        Tuple *newTuple = new Tuple( resultTupleType );
        assert( newTuple->GetNoAttributes() == tup->GetNoAttributes() + 1 );
        for( int i = 0; i < tup->GetNoAttributes(); i++ )
          newTuple->PutAttribute( i, tup->GetAttribute( i )->Clone() );
        newTuple->PutAttribute( newTuple->GetNoAttributes() - 1,
                          new TupleIdentifier(true,tup->GetTupleId()));

        tup->DeleteIfAllowed();
        result = SetWord(newTuple);
        return YIELD;
      }
      else
        return CANCEL;

    case CLOSE :
      qp->Close(args[0].addr);
      if(local.addr)
      {
        ((TupleType *)local.addr)->DeleteIfAllowed();
        local = SetWord(Address(0));
      }
      return 0;
  }
  return 0;
}


/*
3.2.3 Specification of operator ~addtupleid~

*/
const string AddTupleIdSpec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" \"Comment\" ) "
  "( <text>(stream (tuple ((x1 t1) ... (xn tn)))) ->"
  "(stream (tuple ((x1 t1) ... (xn tn) (id tid))))</text--->"
  "<text>_ addtupleid</text--->"
  "<text>Appends the tuple identifier in the tuple type</text--->"
  "<text>query cities feed addtupleid consume</text--->"
  "<text>Apply addtupleid directly after a feed, because other "
  "operators my corrupt the tid. All in-memory tuples all have tid=0."
  "</text--->"
  ") )";


/*
3.2.4 Definition of operator ~addtupleid~

*/
Operator tidaddtupleid (
         "addtupleid",             // name
         AddTupleIdSpec,           // specification
         TIDAddTupleId,            // value mapping
         Operator::SimpleSelect,         // trivial selection function
         AddTupleIdTypeMap         // type mapping
);


/*
3.3 Operator ~=~

Compares two TupleIdentifiers and returns TRUE, iff they are equal.

3.3.1 Type mapping function of operator ~=~

----    (tid tid) -> bool
----

*/

ListExpr
EqualTupleIdTypeMap(ListExpr args)
{
  ListExpr first;
  ListExpr second;
  string argstr;

  CHECK_COND(nl->ListLength(args) == 2,
  "Operator '=' expects a list of length two.");

  first = nl->First(args);
  second = nl->Second(args);

  nl->WriteToString(argstr, args);
  CHECK_COND(  nl->AtomType( first ) == SymbolType &&
               nl->SymbolValue( first ) == "tid" &&
               nl->AtomType( second ) == SymbolType &&
               nl->SymbolValue( second ) == "tid",
               "Operators '=' and '#' expect a list with structure "
               "(tid tid)\n"
               "Operator '='/'#' gets a list with structure '" + argstr + "'.");

  return nl->SymbolAtom("bool");
}


/*
3.3.2 Value mapping function of operators ~=, \#, $<$, $>$, $\leq$, $\geq$~

Comparison operators

----
  Operator   op
  <          0
  <=         1
  =          2
  >=         3
  >          4
  #          5
----

*/

template<int op>
int
TIDCompareTupleId( Word* args, Word& result,
                   int message, Word& local, Supplier s )
{
  assert((op >= 0) && (op <=5));
  result = qp->ResultStorage( s );
  const TupleIdentifier* a = static_cast<const TupleIdentifier*>(args[0].addr);
  const TupleIdentifier* b = static_cast<const TupleIdentifier*>(args[1].addr);

  //int cmp = a->Compare(b);
  switch (op)
  {
    case 0: // <
      ((CcBool *)result.addr)->Set( true, a->Compare(b) < 0 );
      return (0);
    case 1: // <=
      ((CcBool *)result.addr)->Set( true, a->Compare(b) <= 0 );
      return (0);
    case 2: // =
      ((CcBool *)result.addr)->Set( true, a->Compare(b) == 0 );
      return (0);
    case 3: // >=
      ((CcBool *)result.addr)->Set( true, a->Compare(b) >= 0 );
      return (0);
    case 4: // >
      ((CcBool *)result.addr)->Set( true, a->Compare(b) > 0 );
      return (0);
    case 5: // #
      ((CcBool *)result.addr)->Set( true, a->Compare(b) != 0 );
      return (0);
  }
  // ERROR:
  ((CcBool *)result.addr)->Set( false, false );
  return (0);
}

/*
3.3.3 Specification of operators ~=, \#, $<$, $>$, $\leq$, $\geq$~

*/

const string EqualTupleIdSpec =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" \"Result\" \"Comment\" ) "
  "( <text>(tid tid) -> bool</text--->"
  "<text>_ = _</text--->"
  "<text>Returns TRUE, iff both tuple identifiers are equal (i.e "
  "both refer to the same tuple).</text--->"
  "<text>query plz feed head[4] loopsel[plz_Ort exactmatchS[.Ort]] {A}\n"
  "plz feed head[4] loopsel[plz_Ort  exactmatchS[.Ort]] {B}\n"
  "symmjoin[.id_A = ..id_B] count</text--->"
  "<text>2336</text--->"
  "<text>Caution: Only compare TIDs referring to the same relation! "
  "All in-memory tuples have tid=0.</text--->"
  ") )";

const string NequalTupleIdSpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" "
    "\"Example\" \"Result\" \"Comment\" ) "
    "( <text>(tid tid) -> bool</text--->"
    "<text>_ # _</text--->"
    "<text>Returns TRUE, iff both tuple identifiers are different (i.e "
    "both refer to different tuples).</text--->"
    "<text>query plz feed head[4] loopsel[plz_Ort exactmatchS[.Ort]] {A}\n"
    "plz feed head[4] loopsel[plz_Ort  exactmatchS[.Ort]] {B}\n"
    "symmjoin[.id_A # ..id_B] count</text--->"
    "<text>338720</text--->"
    "<text>Caution: Only compare TIDs referring to the same relation! "
    "All in-memory tuples have tid=0.</text--->"
    ") )";

const string LessTupleIdSpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" "
    "\"Example\" \"Result\" \"Comment\" ) "
    "( <text>(tid tid) -> bool</text--->"
    "<text>_ < _</text--->"
    "<text>Returns TRUE, iff the first tuple identifier is less than "
    "the second one.</text--->"
    "<text>query plz feed head[4] loopsel[plz_Ort exactmatchS[.Ort]] {A}\n"
    "plz feed head[4] loopsel[plz_Ort  exactmatchS[.Ort]] {B}\n"
    "symmjoin[.id_A < ..id_B] count</text--->"
    "<text>169360</text--->"
    "<text>Caution: Only compare TIDs referring to the same relation! "
    "All in-memory tuples have tid=0.</text--->"
    ") )";

const string GreaterTupleIdSpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" "
    "\"Example\" \"Result\" \"Comment\" ) "
    "( <text>(tid tid) -> bool</text--->"
    "<text>_ > _</text--->"
    "<text>Returns TRUE, iff the first tuple identifier is greater than "
    "the second one.</text--->"
    "<text>query plz feed head[4] loopsel[plz_Ort exactmatchS[.Ort]] {A}\n"
    "plz feed head[4] loopsel[plz_Ort  exactmatchS[.Ort]] {B}\n"
    "symmjoin[.id_A > ..id_B] count</text--->"
    "<text>169360</text--->"
    "<text>Caution: Only compare TIDs referring to the same relation! "
    "All in-memory tuples have tid=0.</text--->"
    ") )";

const string LeqTupleIdSpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" "
    "\"Example\" \"Result\" \"Comment\" ) "
    "( <text>(tid tid) -> bool</text--->"
    "<text>_ <= _</text--->"
    "<text>Returns TRUE, iff the first tuple identifier is less or equal than "
    "the second one.</text--->"
    "<text>query plz feed head[4] loopsel[plz_Ort exactmatchS[.Ort]] {A}\n"
    "plz feed head[4] loopsel[plz_Ort  exactmatchS[.Ort]] {B}\n"
    "symmjoin[.id_A <= ..id_B] count</text--->"
    "<text>171696</text--->"
    "<text>Caution: Only compare TIDs referring to the same relation! "
    "All in-memory tuples have tid=0.</text--->"
    ") )";

const string GeqTupleIdSpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" "
    "\"Example\" \"Result\" \"Comment\" ) "
    "( <text>(tid tid) -> bool</text--->"
    "<text>_ < _</text--->"
    "<text>Returns TRUE, iff the first tuple identifier is greater or equal "
    "than the second one.</text--->"
    "<text>query plz feed head[4] loopsel[plz_Ort exactmatchS[.Ort]] {A}\n"
    "plz feed head[4] loopsel[plz_Ort  exactmatchS[.Ort]] {B}\n"
    "symmjoin[.id_A >= ..id_B] count</text--->"
    "<text>171696</text--->"
    "<text>Caution: Only compare TIDs referring to the same relation! "
    "All in-memory tuples have tid=0.</text--->"
    ") )";

/*
3.3.4 Definition of operators ~=, \#, $<$, $>$, $\leq$, $\geq$~

*/

Operator tidless (
         "<",                      // name
         LessTupleIdSpec,          // specification
         TIDCompareTupleId<0>,     // value mapping
         Operator::SimpleSelect,   // trivial selection function
         EqualTupleIdTypeMap       // type mapping
                 );

Operator tidleq (
         "<=",                     // name
         LeqTupleIdSpec,           // specification
         TIDCompareTupleId<1>,     // value mapping
         Operator::SimpleSelect,   // trivial selection function
         EqualTupleIdTypeMap       // type mapping
                );

Operator tidequal (
         "=",                      // name
         EqualTupleIdSpec,         // specification
         TIDCompareTupleId<2>,     // value mapping
         Operator::SimpleSelect,   // trivial selection function
         EqualTupleIdTypeMap       // type mapping
);

Operator tidgeq (
         ">=",                      // name
         GeqTupleIdSpec,           // specification
         TIDCompareTupleId<3>,     // value mapping
         Operator::SimpleSelect,   // trivial selection function
         EqualTupleIdTypeMap       // type mapping
                  );

Operator tidgreater (
         ">",                      // name
         GreaterTupleIdSpec,       // specification
         TIDCompareTupleId<4>,     // value mapping
         Operator::SimpleSelect,   // trivial selection function
         EqualTupleIdTypeMap       // type mapping
                  );

Operator tidnequal (
         "#",                      // name
         NequalTupleIdSpec,        // specification
         TIDCompareTupleId<5>,     // value mapping
         Operator::SimpleSelect,   // trivial selection function
         EqualTupleIdTypeMap       // type mapping
                  );


/*
5 Creating the Algebra

*/

class TupleIdentifierAlgebra : public Algebra
{
 public:
  TupleIdentifierAlgebra() : Algebra()
  {
    AddTypeConstructor( &tupleIdentifier );
    tupleIdentifier.AssociateKind( "DATA" );

    AddOperator( &tidtupleid );
    AddOperator( &tidaddtupleid );
    AddOperator( &tidequal );
    AddOperator( &tidnequal );
    AddOperator( &tidless );
    AddOperator( &tidleq );
    AddOperator( &tidgreater );
    AddOperator( &tidgeq );

  }
  ~TupleIdentifierAlgebra() {};
};

/*
6 Initialization

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
InitializeTupleIdentifierAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return (new TupleIdentifierAlgebra());
}


