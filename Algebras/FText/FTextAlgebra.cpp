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

//[_] [\_]

//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]

[1] FText Algebra

March - April 2003 Lothar Sowada

December 2005, Victor Almeida deleted the deprecated algebra levels
(~executable~, ~descriptive~, and ~hibrid~). Only the executable
level remains. Models are also removed from type constructors.

April 2006, M. Spiekermann. Output format of type text changed!
Now it will be only a text atom instead of a one element list containing a text atom.

The algebra ~FText~ provides the type constructor ~text~ and two operators:

(i) ~contains~, which search text or string in a text.

(ii) ~length~ which give back the length of a text.


March 2008, Christian D[ue]ntgen added operators ~getcatalog~, $+$, ~substr~,
~subtext~, ~isempty~, $<$, $<=$, $=$, $>=$, $>$, \#, ~find~, ~evaluate~,
~getTypeNL~, ~getValueNL~, ~replace~, ~tostring~, ~totext~, ~tolower~,
~toupper~, ~chartext~.

October 2008, Christian D[ue]ntgen added operators ~sendtextUDP~ and
~receivetextUDP~

1 Preliminaries

1.1 Includes

*/


#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iterator>
#include <functional>

#include "FTextAlgebra.h"
#include "Attribute.h"
#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "AlgebraManager.h"
#include "StandardTypes.h"
#include "RelationAlgebra.h"
#include "SecondoInterface.h"
#include "SecondoInterface.h"
#include "DerivedObj.h"
#include "NList.h"
#include "DiceCoeff.h"
#include "SecParser.h"
#include "StopWatch.h"
#include "Symbols.h"
#include "SecondoSMI.h"
#include "SocketIO.h"
#include "Crypt.h"
#include "ListUtils.h"
#include "blowfish.h"
#include "md5.h"
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <limits>

#define LOGMSG_OFF
#include "LogMsg.h"
#undef LOGMSG
#define LOGMSG(a, b)


extern NestedList *nl;
extern QueryProcessor *qp;
extern AlgebraManager *am;

using namespace std;

//extern NestedList* nl;
//extern QueryProcessor* qp;
//extern AlgebraManager* am;

/*
2 Type Constructor ~text~

2.1 Data Structure - Class ~FText~

*/
size_t FText::HashValue() const
{
  if(traces)
    cout << '\n' << "Start HashValue" << '\n';

  if(!IsDefined())
    return 0;

  unsigned long h = 0;
  // Assuming text fits into memory
  char* s1 = Get();
  char*s = s1;
  while(*s != 0)
  {
    h = 5 * h + *s;
    s++;
  }
  delete[] s1;
  return size_t(h);
}

void FText::CopyFrom( const Attribute* right )
{
  if(traces)
    cout << '\n' << "Start CopyFrom" << '\n';

  const FText* r = (const FText*)right;
  string val = "";
  if(r->IsDefined())
  {
    val = r->GetValue();
  }
  Set( r->IsDefined(), val );
}

int FText::Compare( const Attribute *arg ) const
{
  if(traces)
    cout << '\n' << "Start Compare" << '\n';

  if(!IsDefined() && !arg->IsDefined())
    return 0;

  if(!IsDefined())
    return -1;

  if(!arg->IsDefined())
    return 1;

  const FText* f = (const FText* )(arg);

  if ( !f )
    return -2;

  char* s1 =  f->Get();
  char* s2 =  this->Get();

  int res = strcmp( s2, s1 );
  delete [] s1;
  delete [] s2;
  return res;
}

ostream& FText::Print(ostream &os) const
{
  if(!IsDefined())
  {
    return (os << "TEXT: UNDEFINED");
  }
  char* t = Get();
  string s(t);
  delete [] t;
  SHOW(theText)
  size_t len = theText.getSize();
  if(TextLength() > 65)
  {
    return (os << "TEXT: (" <<len <<") '" << s.substr(0,60) << " ... '" );
  }
  return (os << "TEXT: (" <<len << ") '" << s << "'" );
}


bool FText::Adjacent(const Attribute *arg) const
{
  if(traces)
    cout << '\n' << "Start Adjacent" << '\n';

  const FText* farg = static_cast<const FText*>(arg);

  char a[ theText.getSize() ];
  char b[ farg->theText.getSize() ];

  theText.read(a, theText.getSize() );
  farg->theText.read(b, farg->theText.getSize() );

  if( strcmp( a, b ) == 0 )
    return true;

  if( strlen( a ) == strlen( b ) )
  {
    if( strncmp( a, b, strlen( a ) - 1 ) == 0 )
    {
      char cha = (a)[strlen(a)-1],
           chb = (b)[strlen(b)-1];
      return( cha == chb + 1 || chb == cha + 1 );
    }
  }
  else if( strlen( a ) == strlen( b ) + 1 )
  {
    return( strncmp( a, b, strlen( b ) ) == 0 &&
            ( (a)[strlen(a)-1] == 'a' || (a)[strlen(a)-1] == 'A' ) );
  }
  else if( strlen( a ) + 1 == strlen( b ) )
  {
    return( strncmp( a, b, strlen( a ) ) == 0 &&
            ( (b)[strlen(b)-1] == 'a' || (b)[strlen(b)-1] == 'A' ) );
  }

  return false;
}

// This function writes the object value to a string ~dest~.
// The key must be shorter than SMI_MAX_KEYLEN and is 0-terminated
// Needed to create an index key
void FText::WriteTo( char *dest ) const
{
  string str = GetValue().substr(0, SMI_MAX_KEYLEN-2);
  string::size_type length = str.length();
  str.copy(dest,string::npos);
  dest[length+1] = 0;
}

// This function reads the object value from a string ~src~.
// Needed to create a text object from an index key
void FText::ReadFrom( const char *src )
{
  string myStr(src);
  Set( true, myStr);
}

// This function returns the number of bytes of the object's string
//   representation.
// Needed for transformation to/from an index key
SmiSize FText::SizeOfChars() const
{
  return (SmiSize) (GetValue().substr(0, SMI_MAX_KEYLEN-2).length()+1);
}


namespace ftext{

/*

2.3 Functions for using ~text~ in tuple definitions

The following Functions must be defined if we want to use ~text~ as an attribute type in tuple definitions.

*/

Word CreateFText( const ListExpr typeInfo )
{
  return (SetWord(new FText( false )));
}

void DeleteFText( const ListExpr typeInfo, Word& w )
{
  if(traces)
    cout << '\n' << "Start DeleteFText" << '\n';
  FText *f = (FText *)w.addr;

  f->Destroy();
  delete f;
  w.addr = 0;
}


/*
2.8 ~Open~-function

*/
bool OpenFText( SmiRecord& valueRecord,
                size_t& offset,
                const ListExpr typeInfo,
                Word& value )
{
  // This Open function is implemented in the Attribute class
  // and uses the same method of the Tuple manager to open objects
  FText *ft =
    (FText*)Attribute::Open( valueRecord, offset, typeInfo );
  value.setAddr( ft );
  return true;
}

/*
2.9 ~Save~-function

*/
bool SaveFText( SmiRecord& valueRecord,
                size_t& offset,
                const ListExpr typeInfo,
                Word& value )
{
  FText *ft = (FText *)value.addr;

  // This Save function is implemented in the Attribute class
  // and uses the same method of the Tuple manager to save objects
  Attribute::Save( valueRecord, offset, typeInfo, ft );
  return true;
}


void CloseFText( const ListExpr typeInfo, Word& w )
{
  if(traces)
    cout << '\n' << "Start CloseFText" << '\n';
  delete (FText*) w.addr;
  w.addr = 0;
}

Word CloneFText( const ListExpr typeInfo, const Word& w )
{
  if(traces)
    cout << '\n' << "Start CloneFText" << '\n';
  return SetWord( ((FText *)w.addr)->Clone() );
}

int SizeOfFText()
{
  return sizeof( FText );
}

void* CastFText( void* addr )
{
  if(traces)
    cout << '\n' << "Start CastFText" << '\n';
  return (new (addr) FText);
}


/*

2.4 ~In~ and ~Out~ Functions

*/

ListExpr OutFText( ListExpr typeInfo, Word value )
{
  if(traces)
    cout << '\n' << "Start OutFText" << '\n';
  FText* pftext;
  pftext = (FText*)(value.addr);

  if(traces)
    cout <<"pftext->Get()='"<< pftext->GetValue() <<"'\n";

  ListExpr res;
  if(pftext->IsDefined()){
     res=nl->TextAtom(pftext->GetValue());
  } else {
     res = nl->SymbolAtom("undef");
  }
  //nl->AppendText( TextAtomVar, pftext->Get() );

  if(traces)
    cout <<"End OutFText" << '\n';
  return res;
}


Word InFText( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct )
{
  if(traces)
    cout << '\n' << "Start InFText with ListLength "
         << nl->ListLength( instance );
  ListExpr First;
  if (nl->ListLength( instance ) == 1)
    First = nl->First(instance);
  else
    First = instance;

  if ( nl->IsAtom( First ) && nl->AtomType( First ) == SymbolType
       && nl->SymbolValue( First ) == "undef" )
  {
    string buffer = "";
    FText* newftext = new FText( false, buffer.c_str() );
    correct = true;

    if(traces)
      cout << "End InFText with undef Text '"<<buffer<<"'\n";
    return SetWord(newftext);
  }

  if ( nl->IsAtom(First) && nl->AtomType(First) == TextType)
  {
    string buffer;
    nl->Text2String(First, buffer);
    FText* newftext = new FText( true, buffer.c_str() );
    correct = true;

    if(traces)
      cout << "End InFText with Text '"<<buffer<<"'\n";
    return SetWord(newftext);
  }

  correct = false;
  if(traces)
    cout << "End InFText with errors!" << '\n';
  return SetWord(Address(0));
}


/*
2.5 Function describing the signature of the type constructor

*/

ListExpr FTextProperty()
{
  return
  (
  nl->TwoElemList
    (
      nl->FourElemList
      (
        nl->StringAtom("Signature"),
        nl->StringAtom("Example Type List"),
        nl->StringAtom("List Rep"),
        nl->StringAtom("Example List")
      ),
      nl->FourElemList
      (
        nl->StringAtom("-> DATA\n-> INDEXABLE"),
        nl->StringAtom("text"),
        nl->StringAtom("<text>writtentext</text--->"),
        nl->StringAtom("<text>This is an example.</text--->")
      )
    )
  );
}


ListExpr SVGProperty()
{
  return
  (
  nl->TwoElemList
    (
      nl->FourElemList
      (
        nl->StringAtom("Signature"),
        nl->StringAtom("Example Type List"),
        nl->StringAtom("List Rep"),
        nl->StringAtom("Example List")
      ),
      nl->FourElemList
      (
        nl->StringAtom("-> DATA"),
        nl->StringAtom("svg"),
        nl->StringAtom("<text>svg description</text--->"),
        nl->StringAtom("<text><svg> ... </svg></text--->")
      )
    )
  );
}


/*
2.6 Kind Checking Function

This function checks whether the type constructor is applied correctly.

*/

bool CheckFText( ListExpr type, ListExpr& errorInfo )
{
  if(traces)
    cout <<'\n'<<"Start CheckFText"<<'\n';
  bool bresult=(nl->IsEqual( type, typeName ));
  if(traces)
  {
    if(bresult)
      cout <<"End CheckFText with type=='"<<typeName<<"'"<<'\n';
    else
      cout <<"End CheckFText with type!='"<<typeName<<"'"<<'\n';
  }
  return bresult;
}


bool CheckSVG( ListExpr type, ListExpr& errorInfo )
{
  return nl->IsEqual( type, "svg");
}



/*

2.7 Creation of the type constructor instances

*/

TypeConstructor ftext(
  typeName,                     //name of the type
  FTextProperty,                //property function describing signature
  OutFText,    InFText,         //Out and In functions
  0,           0,               //SaveToList and RestoreFromList functions
  CreateFText, DeleteFText,     //object creation and deletion
  OpenFText, SaveFText,
  CloseFText, CloneFText,       //close, and clone
  CastFText,                    //cast function
  SizeOfFText,                  //sizeof function
  CheckFText );                 //kind checking function

TypeConstructor svg(
  "svg",                     //name of the type
  SVGProperty,                //property function describing signature
  OutFText,    InFText,         //Out and In functions
  0,           0,               //SaveToList and RestoreFromList functions
  CreateFText, DeleteFText,     //object creation and deletion
  OpenFText, SaveFText,
  CloseFText, CloneFText,       //close, and clone
  CastFText,                    //cast function
  SizeOfFText,                  //sizeof function
  CheckSVG );                 //kind checking function

/*

3 Creating Operators

3.1 Type Mapping Functions

Checks whether the correct argument types are supplied for an operator; if so,
returns a list expression for the result type, otherwise the symbol ~typeerror~.

*/

/*
Typemapping text x text [->] bool

*/
ListExpr TypeMap_Text_Text__Bool( ListExpr args )
{
  if(traces)
  {
    cout <<'\n'<<"Start TypeMap_Text_Text__Bool"<<'\n';
    nl->WriteToFile("/dev/tty",args);
  }
  ListExpr arg1, arg2;
  if ( nl->ListLength(args) == 2 )
  {
    arg1 = nl->First(args);
    arg2 = nl->Second(args);
    if ( nl->IsEqual(arg1, typeName) && nl->IsEqual(arg2, typeName) )
    {
      return nl->SymbolAtom("bool");
    }
  }

  if(traces)
    cout <<"End TypeMap_Text_Text__Bool with typeerror"<<'\n';
  return nl->SymbolAtom("typeerror");
}

/*
Typemapping {text|string} x {text|string} [->] bool

*/
ListExpr TypeMap_TextString_TextString__Bool( ListExpr args )
{
  if(nl->ListLength(args)!=2){
    return listutils::typeError("Expected 2 arguments.");
  }
  ListExpr arg1 = nl->First(args);
  if(   !listutils::isSymbol(arg1,symbols::TEXT)
     && !listutils::isSymbol(arg1,symbols::STRING)){
    return listutils::typeError("{text|string} x {text|string} expected.");
  }
  ListExpr arg2 = nl->Second(args);
  if(   !listutils::isSymbol(arg2,symbols::TEXT)
     && !listutils::isSymbol(arg2,symbols::STRING)){
      return listutils::typeError("{text|string} x {text|string} expected.");
    }
  return nl->SymbolAtom(symbols::BOOL);
}

/*
Typemapping: text x string [->] bool

*/
ListExpr TypeMap_Text_String__Bool( ListExpr args )
{
  if(traces)
    {
    cout <<'\n'<<"Start TypeMap_Text_String__Bool"<<'\n';
    nl->WriteToFile("/dev/tty",args);
    }
  ListExpr arg1, arg2;
  if ( nl->ListLength(args) == 2 )
  {
    arg1 = nl->First(args);
    arg2 = nl->Second(args);
    if ( nl->IsEqual(arg1, typeName) && nl->IsEqual(arg2, "string") )
    {
      return nl->SymbolAtom("bool");
    }
  }

  if(traces)
    cout <<"End TypeMap_Text_String__Bool with typeerror"<<'\n';
  return nl->SymbolAtom("typeerror");
}

/*
Typemapping text [->] int

*/

ListExpr TypeMap_Text__Int( ListExpr args )
{
  if(traces)
    cout << '\n' << "Start TypeMap_Text__Int" << '\n';
  if ( nl->ListLength(args) == 1 )
  {
    ListExpr arg1= nl->First(args);
    if ( nl->IsEqual(arg1, typeName))
    {
      if(traces)
        cout << '\n' << "Start TypeMap_Text__Int" << '\n';
      return nl->SymbolAtom("int");
    }
  }

  if(traces)
    cout <<"End TypeMap_Text__Int with typeerror"<<'\n';
  return nl->SymbolAtom("typeerror");
}

/*
Typemap: text [->] stream(string)

*/

ListExpr TypeMap_text__stringstream( ListExpr args ){
  ListExpr arg;
  string nlchrs;

  if ( nl->ListLength(args) == 1 )
  {
    arg = nl->First(args);
    if ( nl->IsEqual(arg, typeName) )
      return nl->TwoElemList( nl->SymbolAtom("stream"),
                              nl->SymbolAtom("string"));
  }
  return nl->SymbolAtom("typeerror");
}

/*
Typemap: text [->] stream(text)

*/
ListExpr TypeMap_text__textstream( ListExpr args ){
  ListExpr arg;

  if ( nl->ListLength(args) == 1 )
  {
    arg = nl->First(args);
    if ( nl->IsEqual(arg, typeName) )
      return nl->TwoElemList(nl->SymbolAtom("stream"), nl->SymbolAtom("text"));
  }
  return nl->SymbolAtom("typeerror");
}

/*
Typemap: int x text x text [->] real

*/
ListExpr TypeMap_int_text_text__real(ListExpr args){
  if(nl->ListLength(args)!=3){
       ErrorReporter::ReportError("three arguments required");
       return nl->SymbolAtom("typeerror");
  }
  if(!nl->IsEqual(nl->First(args),"int")){
     ErrorReporter::ReportError("first argument must be an integer");
     return nl->SymbolAtom("typeerror");
  }
  ListExpr arg2 = nl->Second(args);
  ListExpr arg3 = nl->Third(args);
  if(  (nl->AtomType(arg2)!=SymbolType ) ||
       (nl->AtomType(arg3)!=SymbolType)){
     ErrorReporter::ReportError("only simple types allowed");
     return nl->SymbolAtom("typeerror");
  }

  string t2 = nl->SymbolValue(arg2);
  string t3 = nl->SymbolValue(arg3);

  // first version, only texts, later extend to string
  if(t2!="text" || t3!="text"){
     ErrorReporter::ReportError("text as second and third argument expected");
     return nl->SymbolAtom("typeerror");
  }
  return nl->SymbolAtom("real");
}


/*

Type Mapping for operator ~getCatalog~

*/
ListExpr TypeGetCatalog( ListExpr args )
{
  NList type(args);

  if ( type.hasLength(0) ){
    NList resTupleType = NList(NList("ObjectName"),
                       NList(symbols::STRING)).enclose();
    resTupleType.append(NList(NList("Type"),NList(symbols::TEXT)));
    resTupleType.append(NList(NList("TypeExpr"),NList(symbols::TEXT)));
    NList resType =
        NList(NList(NList(symbols::STREAM),
            NList(NList(symbols::TUPLE),resTupleType)));
    return resType.listExpr();
  }
  return NList::typeError( "No argument expected!");
}

/*
Type Mapping for operators ~substr~: {text|string} x int x int [->] string

*/
ListExpr TypeFTextSubstr( ListExpr args )
{
  NList type(args);

  if ( !type.hasLength(3) ){
    return NList::typeError( "Three arguments expected");
  }
  if (    type.second() != NList(symbols::INT)
       || type.third()  != NList(symbols::INT)
     )
  {
    return NList::typeError( "Boundary arguments must be of type int.");
  }
  if ( type.first() == NList(symbols::TEXT) )
  {
    return NList(symbols::STRING).listExpr();
  }
  return NList::typeError( "Expected text as first argument type.");
}

/*

Type Mapping for operators ~subtext~: text x int x int [->] text

*/
ListExpr TypeFTextSubtext( ListExpr args )
{
  NList type(args);

  if ( !type.hasLength(3) ){
    return NList::typeError( "Three arguments expected");
  }
  if (    type.second() != NList(symbols::INT)
          || type.third()  != NList(symbols::INT)
     )
  {
    return NList::typeError( "Boundary arguments must be of type int.");
  }
  if ( type.first() == NList(symbols::TEXT) )
  {
    return NList(symbols::TEXT).listExpr();
  }
  return NList::typeError( "Expected text as first argument type.");
}

/*

Type Mapping for operator ~find~: {string|text} x {string|text} [->] stream(int)

*/
ListExpr TypeMap_textstring_textstring__intstream( ListExpr args )
{
  NList type(args);

  if ( type.hasLength(2) &&
       (
          (type == NList(symbols::STRING, symbols::STRING))
       || (type == NList(symbols::TEXT,   symbols::TEXT  ))
       || (type == NList(symbols::STRING, symbols::TEXT  ))
       || (type == NList(symbols::TEXT,   symbols::STRING))
       )
     )
  {
    return NList(symbols::STREAM, symbols::INT).listExpr();
  }
  return NList::typeError("Expected {text|string} x {text|string}.");
}


/*

Type Mapping: text [->] bool

*/
ListExpr TypeMap_text__bool( ListExpr args )
{
  NList type(args);

  if ( type.hasLength(1)
       &&( (type.first() == NList(symbols::TEXT)) )
     )
  {
    return NList(symbols::BOOL).listExpr();
  }
  return NList::typeError("Expected single text argument.");
}

/*

Type Mapping:  text [->] text

*/
ListExpr TypeMap_text__text( ListExpr args )
{
  if(nl->ListLength(args)!=1){
    return listutils::typeError("single text expected");
  }
  ListExpr arg = nl->First(args);
  if(!listutils::isSymbol(arg,symbols::TEXT)){
    return listutils::typeError("text expected");
  }
  return nl->SymbolAtom(symbols::TEXT);
}

/*

Type Mapping Function for operator ~+~

----
        text x {text | string} --> text
        {text | string} x text --> text
----

*/
ListExpr FTextTypeMapPlus( ListExpr args )
{
  NList type(args);

  if ( !type.hasLength(2) )
  {
    return NList::typeError("Expected two arguments.");
  }
  NList first = type.first();
  NList second = type.second();
  if(    (type == NList(symbols::STRING, symbols::TEXT  ))
      || (type == NList(symbols::TEXT,   symbols::TEXT  ))
      || (type == NList(symbols::TEXT,   symbols::STRING))
    )
  {
    return NList(symbols::TEXT).listExpr();
  }
  return NList::typeError("Expected (text x {text|string}) "
      "or ({text|string} x text).");
}



/*
Type Mapping Function for comparison predicates ~$<$, $<=$, $=$, $>=$, $>$, $\neq$~

----
    <, <=, =, >=, >, #: {string|text} x {string|text} --> bool
----

*/
ListExpr FTextTypeMapComparePred( ListExpr args )
{
  NList type(args);

  if ( !type.hasLength(2) )
  {
    return NList::typeError("Expected two arguments.");
  }
  NList first = type.first();
  NList second = type.second();
  if(    (type == NList(symbols::STRING, symbols::TEXT  ))
          || (type == NList(symbols::TEXT,   symbols::TEXT  ))
          || (type == NList(symbols::TEXT,   symbols::STRING))
    )
  {
    return NList(symbols::BOOL).listExpr();
  }
  return NList::typeError("Expected (text x {text|string}) "
      "or ({text|string} x text).");
}

/*
Type Mapping Function for operator ~evaluate~

----
    text -> stream(tuple((CmdStr text)       // copy of the evaluated command
                         (Success bool)      // TRUE iff execution succeded
                         (ResultType text)      // result type expression
                         (Result text)          // result as nested list expr
                         (ErrorMessage text)    // Error messages
                         (ElapsedTimeReal real) // The execution time in sec
                         (ElapsedTimeCPU real)  // The CPU time in sec
                        )
                  )
----

*/
ListExpr FTextTypeMapEvaluate( ListExpr args )
{
  NList type(args);
  NList st(symbols::STREAM);
  NList tu(symbols::TUPLE);
  NList resTupleType = NList(NList("CmdStr"),NList(symbols::TEXT)).enclose();
  resTupleType.append(NList(NList("Success"),NList(symbols::BOOL)));
  resTupleType.append(NList(NList("Correct"),NList(symbols::BOOL)));
  resTupleType.append(NList(NList("Evaluable"),NList(symbols::BOOL)));
  resTupleType.append(NList(NList("Defined"),NList(symbols::BOOL)));
  resTupleType.append(NList(NList("IsFunction"),NList(symbols::BOOL)));
  resTupleType.append(NList(NList("ResultType"),NList(symbols::TEXT)));
  resTupleType.append(NList(NList("Result"),NList(symbols::TEXT)));
  resTupleType.append(NList(NList("ErrorMessage"),NList(symbols::TEXT)));
  resTupleType.append(NList(NList("ElapsedTimeReal"),NList(symbols::REAL)));
  resTupleType.append(NList(NList("ElapsedTimeCPU"),NList(symbols::REAL)));

  NList resulttype(st,NList(tu,resTupleType));

  if (    type.hasLength(2)
       && (type.first()  == symbols::TEXT)
       && (type.second() == symbols::BOOL)
     )
  {
    return resulttype.listExpr();
  }
  else if(    type.hasLength(1)
           && (type.first() == symbols::TEXT)
         )
  {
    NList resType1 =
        NList( NList(symbols::APPEND),
               NList(false, false).enclose(), resulttype );
    return resType1.listExpr();
  }
  else
  {
    return NList::typeError("Expected 'text' as first, and 'bool' as "
        "optional second argument.");
  }
}

/*
Type Mapping Function for operator ~FTextTypeTextData\_Data~

----
      {text|string} x T --> T
----

*/

ListExpr FTextTypeTextData_Data( ListExpr args )
{
  NList type(args);
  ListExpr errorInfo = nl->OneElemList(nl->SymbolAtom("ErrorInfo"));

  if(     type.hasLength(2)
       && ((type.first()  == symbols::TEXT) ||
       (type.first()  == symbols::STRING))
       && (am->CheckKind("DATA",type.second().listExpr(),errorInfo))
    )
  {
    return type.second().listExpr();
  }
  // error
  return NList::typeError("Expected {text|string} x T, where T in DATA.");
}

/*
Type Mapping Function for operator ~replace~

----
      {text|string} x {text|string} x {text|string} --> text
      {text|string} x int    x int  x {text|string} --> text
----

*/

ListExpr FTextTypeReplace( ListExpr args )
{
  NList type(args);
  // {text|string} x {text|string} x {text|string} --> text
  if(     type.hasLength(3)
          && ((type.first()  == symbols::TEXT) ||
          (type.first()  == symbols::STRING))
          && ((type.second() == symbols::TEXT) ||
          (type.second() == symbols::STRING))
          && ((type.third()  == symbols::TEXT) ||
              (type.third()  == symbols::STRING))
    )
  {
    return NList(symbols::TEXT).listExpr();
  }
  // {text|string} x int    x int  x {text|string} --> text
  if(     type.hasLength(4)
          && ((type.first()  == symbols::TEXT) ||
          (type.first()  == symbols::STRING))
          && ((type.second() == symbols::INT ) ||
          (type.second() == symbols::INT   ))
          && ((type.third()  == symbols::INT ) ||
          (type.third()  == symbols::INT   ))
          && ((type.fourth() == symbols::TEXT) ||
          (type.fourth() == symbols::STRING))
    )
  {
    return NList(symbols::TEXT).listExpr();
  }
  // error
  return NList::typeError("Expected ({text|string} x {text|string} x "
      "{text|string}) or ({text|string} x int x int x {text|string}).");

}

/*
Type Mapping for ~isDBObject~:

---- string --> bool
----

*/
ListExpr TypeMap_string__bool( ListExpr args )
{
  NList type(args);
  if(type.hasLength(1) && (type.first()  == symbols::STRING))
  {

    NList restype(symbols::BOOL, false);
    return restype.listExpr();
  }
  return NList::typeError("Expected 'string' as single argument.");
}

/*
Type Mapping for ~getTypeNL~:

---- TypeExpr --> text @text
----

*/

ListExpr FTextTypeMapExpression2Text( ListExpr args )
{
  NList type(args);
  if(type.hasLength(1))
  {
    string firsttype = type.first().convertToString();
    NList firstType = NList(firsttype, true, true).enclose();
    NList append(symbols::APPEND);
    NList text(symbols::TEXT);
    NList restype(append,
                  firstType,
                  text
                 );
    return restype.listExpr();
  }
  return NList::typeError("Expected any Expression as single argument.");
}

/*
Type Mapping for ~getValueNL~:

----
     (stream TypeExpr) --> (stream text) @text
     Expr --> text @text
----

*/

ListExpr FTextTypeMapGetValueNL( ListExpr args )
{
  NList type(args);
  NList resulttype = NList(Symbols::TYPEERROR());
  ListExpr errorInfo = nl->OneElemList(nl->SymbolAtom("ErrorInfo"));

  if( !type.hasLength(1) )
  { // too many arguments
    return NList::typeError("Expected any Expression as single argument.");
  }
  type = type.first();
  if( IsStreamDescription(type.listExpr()) )
  { // tuplestream
    string myType    = type.second().convertToString();
    NList streamtype = NList(symbols::STREAM, symbols::TEXT);
    NList typeExpr   = NList(myType, true, true).enclose();
    resulttype = NList( NList(symbols::APPEND),
                        NList(myType, true, true).enclose(),
                        streamtype
                      );
  }
  else if (    type.hasLength(2)
            && (type.first() == symbols::STREAM)
            && am->CheckKind("DATA",type.second().listExpr(),errorInfo)
          )
  { // datastream
    string myType    = type.second().convertToString();
    NList streamtype = NList(symbols::STREAM, symbols::TEXT);
    NList typeExpr   = NList(myType, true, true).enclose();
    resulttype = NList( NList(symbols::APPEND),
                        NList(myType, true, true).enclose(),
                        streamtype
                      );
  }
  else
  { // non-stream expression
    string myType = type.convertToString();
    NList typeExpr = NList(myType, true, true).enclose();
    resulttype = NList( NList(symbols::APPEND),
                        NList(myType, true, true).enclose(),
                        NList(symbols::TEXT)
                      );
  }
//   cout << __PRETTY_FUNCTION__ << ": result = " << resulttype << endl;
  return resulttype.listExpr();
}

/*
Type Mapping Function for ~chartext~

----
     int --> text
----

*/

ListExpr TypeMap_int__text( ListExpr args )
{
  NList type(args);
  if( type.hasLength(1) && (type.first() == symbols::INT) )
  {
    return NList(symbols::TEXT).listExpr();
  }
  return NList::typeError("Expected 'int'.");
}


/*
Type Mapping Function for ~tostring~

----
     text --> string
----

*/

ListExpr TypeMap_text__string( ListExpr args )
{
  NList type(args);
  if( type.hasLength(1) && (type.first() == symbols::TEXT) )
  {
    return NList(symbols::STRING).listExpr();
  }
  return NList::typeError("Expected 'text'.");
}

/*
Type Mapping Function for ~totext~

----
      string --> text
----

*/

ListExpr TypeMap_string__text( ListExpr args )
{
  NList type(args);
  if( type.hasLength(1) && (type.first() == symbols::STRING) )
  {
    return NList(symbols::TEXT).listExpr();
  }
  return NList::typeError("Expected 'string'.");
}

/*
Type Mapping Function for ~sendtextUDP~

----
      {string|text}^n -> text, 3 <= n <= 5.
----

*/
ListExpr FTextTypeSendTextUDP( ListExpr args )
{
  NList type(args);
  int noargs = nl->ListLength(args);
  if(noargs < 3 || noargs > 5){
    return NList::typeError("Expected {string|text}^n, 3 <= n <= 5.");
  }

  for(int i = 1; i<=noargs; i++){
    string argtype;
    nl->WriteToString(argtype,nl->Nth(i,args));
    if((argtype != symbols::STRING) && (argtype != symbols::TEXT)){
      return NList::typeError("Expected {string|text}^n, 3 <= n <= 5.");
    }
  }
  return NList(symbols::TEXT).listExpr();
}

/*
Type Mapping Function for ~receivetextUDP~

----
     {string|text} x {string|text} x real ->
     {stream(tuple((Ok bool)
                   (Msg text)
                   (ErrMsg string)
                   (SenderIP string)
                   (SenderPort string)
                   (SenderIPversion string)
                  )
            )

----

*/
ListExpr FTextTypeReceiveTextUDP( ListExpr args )
{
  NList type(args);
  int noargs = nl->ListLength(args);
  if(    (noargs != 3)
      || (type.first()  != symbols::TEXT && type.first()  != symbols::STRING)
      || (type.second() != symbols::TEXT && type.second() != symbols::STRING)
      || (type.third()  != symbols::REAL) ){
    return NList::typeError("Expected {string|text} x {string|text} x real.");
  }
  NList resTupleType = NList(NList("Ok"),NList(symbols::BOOL)).enclose();
  resTupleType.append(NList(NList("Msg"),NList(symbols::TEXT)));
  resTupleType.append(NList(NList("ErrMsg"),NList(symbols::STRING)));
  resTupleType.append(NList(NList("SenderIP"),NList(symbols::STRING)));
  resTupleType.append(NList(NList("SenderPort"),NList(symbols::STRING)));
  resTupleType.append(NList(NList("SenderIPversion"),NList(symbols::STRING)));
  NList resType =
      NList(NList(NList(symbols::STREAM),
            NList(NList(symbols::TUPLE),resTupleType)));
  return resType.listExpr();
}

/*
Type Mapping Function for ~receivetextUDP~

----
{string|text} x {string|text} x real x real->
{stream(tuple((Ok bool)
                   (Msg text)
                   (ErrMsg string)
                   (SenderIP string)
                   (SenderPort string)
                   (SenderIPversion string)
                  )
            )

----

*/
ListExpr FTextTypeReceiveTextStreamUDP( ListExpr args )
{
  NList type(args);
  int noargs = nl->ListLength(args);
  if(    (noargs !=4 )
      || (type.first()  != symbols::TEXT && type.first()  != symbols::STRING)
      || (type.second() != symbols::TEXT && type.second() != symbols::STRING)
      || (type.third()  != symbols::REAL)
      || (type.fourth() != symbols::REAL) ) {
    return NList::typeError("Expected {string|text} x "
                            "{string|text} x real x real.");
  }
  NList resTupleType = NList(NList("Ok"),NList(symbols::BOOL)).enclose();
  resTupleType.append(NList(NList("Msg"),NList(symbols::TEXT)));
  resTupleType.append(NList(NList("ErrMsg"),NList(symbols::STRING)));
  resTupleType.append(NList(NList("SenderIP"),NList(symbols::STRING)));
  resTupleType.append(NList(NList("SenderPort"),NList(symbols::STRING)));
  resTupleType.append(NList(NList("SenderIPversion"),NList(symbols::STRING)));
  NList resType =
      NList(NList(NList(symbols::STREAM),
            NList(NList(symbols::TUPLE),resTupleType)));
  return resType.listExpr();
}


/*
2.50.1 ~TypeMap\_text\_\_svg~

*/
ListExpr TypeMap_text__svg(ListExpr args){
   if(nl->ListLength(args)!=1){
      ErrorReporter::ReportError("One argument expected");
      return nl->TypeError();
   }
   if(nl->IsEqual(nl->First(args),"text")){
      return nl->SymbolAtom("svg");
   }
   ErrorReporter::ReportError("text expected");
   return nl->TypeError();
}


/*
2.50.2 ~TypeMap\_svg\_\_text~

*/
ListExpr TypeMap_svg__text(ListExpr args){
   if(nl->ListLength(args)!=1){
      ErrorReporter::ReportError("One argument expected");
      return nl->TypeError();
   }
   if(nl->IsEqual(nl->First(args),"svg")){
      return nl->SymbolAtom("text");
   }
   ErrorReporter::ReportError("svg expected");
   return nl->TypeError();

}

/*
2.51 ~crypt~

 t1 [x t2] -> string , t1, t2 in {string, text}

*/

ListExpr cryptTM(ListExpr args){
int l = nl->ListLength(args);

string err = "t1 [x t2]  , t1, t2 in {string, text} expected";
if((l!=1) && (l!=2)){
  ErrorReporter::ReportError(err);
  return nl->TypeError();
}
while(!nl->IsEmpty(args)){
  ListExpr first = nl->First(args);
  args = nl->Rest(args);
  if(nl->AtomType(first)!=SymbolType){
     ErrorReporter::ReportError(err);
     return nl->TypeError();
  }
  string v = nl->SymbolValue(first);
  if( (v!="string") && (v!="text")){
     ErrorReporter::ReportError(err);
     return nl->TypeError();
  }
}
return nl->SymbolAtom("string");
}

/*
checkpw: {string | text} x {string | text} [->] bool

*/
ListExpr checkpwTM(ListExpr args){
  string err = "{string, text} x {string, text} expected";
  if(nl->ListLength(args)!=2){
     ErrorReporter::ReportError(err);
     return nl->TypeError();
  }
  while(!nl->IsEmpty(args)){
    ListExpr first = nl->First(args);
    args = nl->Rest(args);
    if(nl->AtomType(first)!=SymbolType){
       ErrorReporter::ReportError(err);
       return nl->TypeError();
    }
    string v = nl->SymbolValue(first);
    if( (v!="string") && (v!="text")){
       ErrorReporter::ReportError(err);
       return nl->TypeError();
    }
  }
  return nl->SymbolAtom("bool");
}

/*
2.53 ~md5~

 t1 [x t2] -> text , t1, t2 in {string, text}

*/

ListExpr md5TM(ListExpr args){
int l = nl->ListLength(args);

string err = "t1 [x t2]  , t1, t2 in {string, text} expected";
if((l!=1) && (l!=2)){
  ErrorReporter::ReportError(err);
  return nl->TypeError();
}
while(!nl->IsEmpty(args)){
  ListExpr first = nl->First(args);
  args = nl->Rest(args);
  if(nl->AtomType(first)!=SymbolType){
     ErrorReporter::ReportError(err);
     return nl->TypeError();
  }
  string v = nl->SymbolValue(first);
  if( (v!="string") && (v!="text")){
     ErrorReporter::ReportError(err);
     return nl->TypeError();
  }
}
return nl->SymbolAtom("string");
}


/*
2.54 ~blowfish~

 t1 x t2 -> text , t1, t2 in {string, text}

*/

ListExpr blowfish_encodeTM(ListExpr args){
int l = nl->ListLength(args);

string err = "t1 x t2, t1, t2 in {string, text} expected";
if(l!=2){
  ErrorReporter::ReportError(err);
  return nl->TypeError();
}
while(!nl->IsEmpty(args)){
  ListExpr first = nl->First(args);
  args = nl->Rest(args);
  if(nl->AtomType(first)!=SymbolType){
     ErrorReporter::ReportError(err);
     return nl->TypeError();
  }
  string v = nl->SymbolValue(first);
  if( (v!="string") && (v!="text")){
     ErrorReporter::ReportError(err);
     return nl->TypeError();
  }
}
return nl->SymbolAtom("text");
}

ListExpr blowfish_decodeTM(ListExpr args){
   return blowfish_encodeTM(args);
}

/*
2.54 ~letObject~

---- {string|text} x {string|text} x bool --> text
----

*/

ListExpr StringtypeStringtypeBool2TextTM(ListExpr args){
  NList type(args);
  int noargs = nl->ListLength(args);
  if(    (noargs  != 3)
      || ((type.first() != symbols::STRING) && (type.first() != symbols::TEXT))
      || ((type.second()!= symbols::STRING) && (type.second()!= symbols::TEXT))
      || ( type.third() != symbols::BOOL) ) {
    return NList::typeError("Expected {string|text} x {string|text} x bool.");
  }
  return NList(symbols::TEXT).listExpr();
}


/*
2.55 ~getDatabaseName~: () [->] string

*/

ListExpr TypeMap_empty__string(ListExpr args){
  NList type(args);
  int noargs = nl->ListLength(args);
  if( noargs != 0 ) {
    return NList::typeError("Expected no argument.");
  }
  return NList(symbols::STRING).listExpr();
}


/*
2.55 ~TypeMap\_textstring\_\_text~

---- {text | string} --> text
----

Used by ~deleteObject~, ~getObjectValueNL~, ~getObjectTypeNL~.

*/

ListExpr TypeMap_textstring__text(ListExpr args){
  NList type(args);
  int noargs = nl->ListLength(args);
  if(    (noargs != 1)
      || ((type.first()!=symbols::STRING) && (type.first() != symbols::TEXT))) {
    return NList::typeError("Expected {string|text}.");
  }
  return NList(symbols::TEXT).listExpr();
}



/*
2.56 TypeMap ~matchingOperatorsNames~

any -> stream(string)  

*/

ListExpr matchingOperatorNamesTM(ListExpr args){
    ListExpr res =  nl->TwoElemList(nl->SymbolAtom("stream"), 
                                    nl->SymbolAtom("string"));
    return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
               nl->OneElemList(nl->TextAtom(nl->ToString(args))),
               res);
}

/*
2.57 TypeMap ~matchingOperators~

  ANY -> stream(tuple(( OperatorName: string, 
                        AlgebraName : string,
                        ResultType : text,
                        Signature  : text,
                        Syntax : text,
                        Meaning : text,
                        Example : text,
                        Reamrk : text)))
                        
*/

ListExpr matchingOperatorsTM(ListExpr args){

    ListExpr attrList = nl->OneElemList(nl->TwoElemList(
                           nl->SymbolAtom("OperatorName"), 
                           nl->SymbolAtom(CcString::BasicType())));
    ListExpr last = attrList;
    last = nl->Append(last, nl->TwoElemList( 
                              nl->SymbolAtom("AlgebraName"), 
                              nl->SymbolAtom(CcString::BasicType())));
    last = nl->Append(last, nl->TwoElemList( 
                              nl->SymbolAtom("ResultType"), 
                              nl->SymbolAtom(FText::BasicType())));
    last = nl->Append(last, nl->TwoElemList( 
                              nl->SymbolAtom("Signature"), 
                              nl->SymbolAtom(FText::BasicType())));
    last = nl->Append(last, nl->TwoElemList( 
                              nl->SymbolAtom("Syntax"), 
                              nl->SymbolAtom(FText::BasicType())));
    last = nl->Append(last, nl->TwoElemList( 
                              nl->SymbolAtom("Meaning"), 
                              nl->SymbolAtom(FText::BasicType())));
    last = nl->Append(last, nl->TwoElemList( 
                              nl->SymbolAtom("Example"), 
                              nl->SymbolAtom(FText::BasicType())));
    last = nl->Append(last, nl->TwoElemList( 
                              nl->SymbolAtom("Remark"), 
                              nl->SymbolAtom(FText::BasicType())));
                                        
    ListExpr res = nl->TwoElemList(nl->SymbolAtom("stream"),
                     nl->TwoElemList( nl->SymbolAtom("tuple"),attrList));
    return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
                       nl->OneElemList(nl->TextAtom(nl->ToString(args))),
                             res);
}



/*
3.3 Value Mapping Functions

*/

/*
Value Mapping for the ~contains~ operator

*/
template<class T1, class T2>
int ValMapContains( Word* args, Word& result,
                    int message, Word& local, Supplier s)
{
  if(traces){
    cout <<'\n'<<"Start ValMapContains"<<'\n';
  }

  T1* text    = static_cast<T1*>(args[0].addr);
  T2* pattern = static_cast<T2*>(args[1].addr);
  result = qp->ResultStorage(s);
  CcBool* res = static_cast<CcBool*>(result.addr);

  if(!text->IsDefined() || !pattern->IsDefined()){
    res->SetDefined(false);
  } else {
    string t = text->GetValue();
    string p = pattern->GetValue();
    res->Set( true, (t.find(p) != string::npos) );
  }

  if(traces){
    cout <<"End ValMapContains"<<'\n';
  }
  return 0;
}

ValueMapping FText_VMMap_Contains[] =
{
  ValMapContains<CcString, FText>,    //  0
  ValMapContains<FText, CcString>,    //  1
  ValMapContains<FText, FText>,       //  2
  ValMapContains<CcString, CcString>  //  3
};


/*
Value Mapping for the ~length~ operator with a text and a string operator .

*/

int ValMapTextInt(Word* args, Word& result, int message,
                  Word& local, Supplier s)
{
  if(traces)
    cout <<'\n'<<"Start ValMapTextInt"<<'\n';
  FText* ftext1= ((FText*)args[0].addr);

  result = qp->ResultStorage(s); //query processor has provided
          //a CcBool instance to take the result
  ((CcInt*)result.addr)->Set(true, ftext1->TextLength());
          //the first argument says the boolean
          //value is defined, the second is the
          //length value)
  if(traces)
    cout <<"End ValMapTextInt"<<'\n';
  return 0;
}

/*
4.21 Value mapping function of operator ~keywords~

The following auxiliary function ~trim~ removes any kind of space
characters from the end of a string.

*/

int trimstr (char s[])
{
  int n;

  for(n = strlen(s) - 1; n >= 0; n--)
   if ( !isspace(s[n]) ) break;
  s[n+1] = '\0';
  return n;
}

// NonStopCharakters are alphanumeric characters, german umlauts and
// the hyphen/minus sign
bool IsNonStopCharacter(const char c)
{
  const unsigned char uc = static_cast<const unsigned char>(c);
  return ( isalnum(c)
      || c == '-' || (uc > 191 && uc <215)
      || (uc > 215 && uc <247) || (uc > 247));
}

int
ValMapkeywords (Word* args, Word& result, int message, Word& local, Supplier s)
/*
Creates a stream of strings of a text given as input parameter,
Recognized strings contains letters, digits and the '-' character.
The length of a string is three characters or more.

*/
{
  struct TheText {
     int start,
     nochr,
     strlength;
     char* subw;
  }* thetext;

  int textcursor, stringcursor, state;
  string tmpstr;
  STRING_T outstr;
  char c;
  CcString* mystring;

  switch( message )
  {
    case OPEN:
      //cout << "open ValMapkeywords" << endl;
      thetext = new TheText;
      thetext->start = 0;
      thetext->nochr = 0;
      thetext->subw = ((FText*)args[0].addr)->Get();
      thetext->strlength = ((FText*)args[0].addr)->TextLength();
      local.addr = thetext;
      return 0;

    case REQUEST:
      //cout << "request ValMapkeywords" << endl;
      thetext = ((TheText*) local.addr);
      textcursor = thetext->start;
      stringcursor = 0;
      state = 0;
      if(thetext->start>=thetext->strlength){
         return CANCEL;
      }
      while (true) {
        switch ( state ) {
          case 0 : c = thetext->subw[textcursor];
                   if ( IsNonStopCharacter( c ) )
                   {
                     outstr[stringcursor] = c;
                     stringcursor++;
                     state = 1;
                   }
                   else {
                     state = 2;
                     stringcursor = 0;
                   }
                   if ( c == '\0' ) { return CANCEL; }
                   textcursor++;
                   break;
          case 1 : c = thetext->subw[textcursor];
                   //cout << c << " state 1 " << endl;
                   if (IsNonStopCharacter( c ))
                   {
                     outstr[stringcursor] = c;
                     stringcursor++;
                     state = 3;
                   }
                   else {
                     state = 2;
                     stringcursor = 0;
                   }
                   if ( c == '\0' ) { return CANCEL; }
                   textcursor++;
                   break;
          case 2 : c = thetext->subw[textcursor];
                   //cout << c << " state 2 " << endl;
                   if (IsNonStopCharacter( c ))
                   {
                     outstr[stringcursor] = c;
                     stringcursor++;
                     state = 1;
                   }
                   else {
                     state = 2;
                     stringcursor = 0;
                   }
                   if ( c == '\0' ) { return CANCEL; }
                   textcursor++;
                   break;
          case 3 : c = thetext->subw[textcursor];
                   //cout << c << " state 3 " << endl;
                   if (IsNonStopCharacter( c ))
                   {
                     outstr[stringcursor] = c;
                     stringcursor++;
                     state = 4;
                   }
                   else {
                     state = 2;
                     stringcursor = 0;
                   }
                   if ( c == '\0' ) { return CANCEL; }
                   textcursor++;
                   break;
        case 4 : c = thetext->subw[textcursor];
                 //cout << c << " state 4 " << endl;
                 if ( IsNonStopCharacter( c ) &&
                      (stringcursor == MAX_STRINGSIZE) ) {
                  state = 5;
                  stringcursor = 0;
                 }
                 else if ( IsNonStopCharacter( c ) &&
                           (stringcursor < MAX_STRINGSIZE) ) {
                   outstr[stringcursor] = c;
                   stringcursor++;
                   state = 4;
                 }
                 else {
                   //if ( c == '\0' )
                   //{ outstr[stringcursor] = c; stringcursor ++; }
                   if ( textcursor == thetext->strlength )
                   { outstr[stringcursor] = c; stringcursor++; };
                   outstr[stringcursor] = '\0';
                   stringcursor = 0;
                   mystring = new CcString();
                   mystring->Set(true, &outstr);
                   result.setAddr(mystring);
                   thetext->start = ++textcursor;
                   local.addr = thetext;
                   return YIELD;
                 }
                 textcursor++;
                 break;
        case 5 : c = thetext->subw[textcursor];
                 //cout << c << " state 5 " << endl;
                 if ( isalnum(c) || c == '-' ) {
                   state = 5;
                   stringcursor = 0;
                 }
                 else {
                   state = 0;
                   stringcursor = 0;
                 }
                 if ( textcursor == thetext->strlength ) { return CANCEL; }
                 textcursor++;
                 break;

      }
    }

    case CLOSE:
      //cout << "close ValMapkeywords" << endl;
      if(local.addr)
      {
        thetext = ((TheText*) local.addr);
        delete [] thetext->subw;
        delete thetext;
        local.setAddr(0);
      }
      return 0;
  }
  /* should not happen */
  return -1;
}

int ValMapsentences (Word* args, Word& result, int message,
                     Word& local, Supplier s)
{
  struct TheText {int start, strlength; char* subw;}* thetext;
  int textcursor = 0, state = 0;
  string tmpstr = "";
  char c = 0;
  FText* returnsentence = 0;

  switch( message )
  {
    case OPEN:
      //cout << "open ValMapsentences" << endl;
      thetext = new TheText;
      thetext->start = 0;
      thetext->strlength = ((FText*)args[0].addr)->TextLength();
      thetext->subw = ((FText*)args[0].addr)->Get();
      local.addr = thetext;
      return 0;

    case REQUEST:
      //cout << "request ValMapsentences" << endl;
      thetext = ((TheText*) local.addr);
      textcursor = thetext->start;
      tmpstr = "";
      state = 0;

      while (true) {
        switch ( state ) {
          case 0 : c = thetext->subw[textcursor];
                   if ( (c == '\0') || (textcursor > thetext->strlength) )
                   { return CANCEL; }
                   if ( c == ',' || c == ';' || c ==':' || c ==' '
                                 || c == '\n' || c == '\t' )
                   {
                     state = 0;
                   }
                   else { if ( c == '.' || c == '!' || c =='?' )
                     {
                       tmpstr += c;
                       state = 3;
                     }
                     else  {
                       tmpstr += c;
                       state = 1;
                     }
                   }
                   textcursor++;
                   break;
          case 1 : c = thetext->subw[textcursor];
                   if ( (c == '\0') || (textcursor > thetext->strlength) )
                   { return CANCEL; }
                   if ( c == ',' || c == ';' || c ==':' )
                   {
                     tmpstr += c;
                     tmpstr += " ";
                     state = 0;
                   }
                   else { if ( c == ' ' || c == '\n' || c == '\t' )
                     state = 2;
                     else { if ( c == '.' || c == '!' || c =='?' )
                       {
                         tmpstr += c;
                         state = 3;
                       }
                       else {
                         tmpstr += c;
                         state = 1;
                       }
                     }
                   }
                   textcursor++;
                   break;
          case 2 : c = thetext->subw[textcursor];
                   if ( (c == '\0') || (textcursor > thetext->strlength) )
                   { return CANCEL; }
                   if ( c == ',' || c == ';' || c ==':' )
                   {
                     tmpstr += c;
                     tmpstr += " ";
                     state = 0;
                   }
                   else { if ( c == ' ' || c == '\n' || c == '\t' )
                     state = 2;
                     else { if ( c == '.' || c == '!' || c =='?' )
                       {
                         tmpstr += c;
                         state = 3;
                       }
                       else {
                         tmpstr += ' ';
                         tmpstr += c;
                         state = 1;
                       }
                     }
                   }
                   textcursor++;
                   break;
          case 3 : if ( (c == '\0') || (textcursor > thetext->strlength) )
                   { return CANCEL; }
                   returnsentence = new FText(true, (char*)tmpstr.c_str());
                   result.setAddr(returnsentence);
                   thetext->start = textcursor;
                   local.addr = thetext;
                   return YIELD;
        }
      }
    case CLOSE:
      //cout << "close ValMapsentences" << endl;
      if(local.addr)
      {
        thetext = ((TheText*) local.addr);
        delete [] thetext->subw;
        delete thetext;
        local.setAddr(0);
      }
      return 0;
  }
  /* should not happen */
  return -1;
}



int ValMapDice_t_t(Word* args, Word& result, int message,
                   Word& local, Supplier s){
  result = qp->ResultStorage(s);
  CcInt* arg1 = (CcInt*) args[0].addr;
  FText* arg2 = (FText*) args[1].addr;
  FText* arg3 = (FText*) args[2].addr;
  int n = arg1->GetIntval();
  if(n<=0){ // ensure a minimum of 1
     n = 1;
  }
  DiceTree* dt = new DiceTree(n);
  char* s1 = arg2->Get();
  char* s2 = arg3->Get();
  dt->appendText(s1,true);
  dt->appendText(s2,false);
  double res = dt->getCoeff();
  delete dt;
  delete[] s2;
  delete[] s1;
  ((CcReal*)result.addr)->Set(true,res);
  return 0;
}

/*
4.26 Operator ~getCatalog~


*/

struct GetCatalogLocalInfo{
  NList myCatalog;
  bool finished;
};

int ValMapGetCatalog( Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  GetCatalogLocalInfo* li   = 0;
  bool foundValidEntry      = true;
  Tuple *newTuple           = 0;
  CcString *objectNameValue = 0;
  FText *typeValue          = 0;
  FText *typeExprValue      = 0;
  TupleType *resultTupleType = 0;

  switch( message )
  {
    case OPEN:
      // cout << "open" << endl;
      li = new GetCatalogLocalInfo;
      li->finished = true;
      li->myCatalog = NList(SecondoSystem::GetCatalog()->ListObjects());
      if(!li->myCatalog.isEmpty() && li->myCatalog.isList())
      {
        li->myCatalog.rest(); // ignore 'OBJECTS'
        li->finished = false;
      }
      local.setAddr( li );
      return 0;

    case REQUEST:
      //  cout << "request" << endl;
      if (local.addr == 0)
        return CANCEL;
      li = (GetCatalogLocalInfo*) local.addr;
      if( li->finished )
        return CANCEL;
      if(li->myCatalog.isEmpty())
      {
        li->finished = true;
        return CANCEL;
      }
      foundValidEntry = false;
      while( !foundValidEntry )
      {
        // Get head of li->myCatalog
        NList currentEntry = li->myCatalog.first();
        if(    currentEntry.isList()
               && currentEntry.hasLength(4)
               && currentEntry.first().isSymbol("OBJECT")
               && currentEntry.second().isSymbol()
               && currentEntry.third().isList()
               && currentEntry.fourth().isList()
          )
        {
          currentEntry.rest(); // ignore 'OBJECT'
          objectNameValue =
              new CcString(true, currentEntry.first().convertToString());
          typeValue =
              new FText(true, currentEntry.second().isEmpty() ? "" :
              currentEntry.second().first().convertToString().c_str());
          typeExprValue =
              new FText(true, currentEntry.third().isEmpty() ? "" :
              currentEntry.third().first().convertToString().c_str());
          resultTupleType = new TupleType(nl->Second(GetTupleResultType(s)));
          newTuple = new Tuple( resultTupleType );
          newTuple->PutAttribute(  0,(Attribute*)objectNameValue);
          newTuple->PutAttribute(  1,(Attribute*)typeValue);
          newTuple->PutAttribute(  2,(Attribute*)typeExprValue);
          result.setAddr(newTuple);
          resultTupleType->DeleteIfAllowed();
          foundValidEntry = true;
        } else
        {
          cerr << __PRETTY_FUNCTION__<< "(" << __FILE__ << __LINE__
              << "): Malformed Catalog Entry passed:" << endl
              << "\tcurrentEntry.isList() = "
              << currentEntry.isList() << endl
              << "\tcurrentEntry.hasLength(4) = "
              << currentEntry.hasLength(4) << endl
              << "\tcurrentEntry.first().isSymbol(\"OBJECT\") = "
              << currentEntry.first().isSymbol("OBJECT") << endl
              << "\tcurrentEntry.second().isSymbol() "
              << currentEntry.second().isSymbol() << endl
              << "\tcurrentEntry.third().isList() = "
              << currentEntry.third().isList() << endl
              << "\tcurrentEntry.fourth().isList()"
              << currentEntry.fourth().isList() << endl
              << "\tcurrentEntry is: "
              << currentEntry.convertToString() << endl;
        }
        li->myCatalog.rest();
      }
      if(foundValidEntry)
        return YIELD;
      li->finished = true;
      return CANCEL;

    case CLOSE:
      // cout << "close" << endl;
      if (local.addr != 0){
        li = (GetCatalogLocalInfo*) local.addr;
        delete li;
        local.addr = 0;
      }
      return 0;
  }
  /* should not happen */
  return -1;
}


/*
4.27 Operator ~substr~


*/
int ValMapSubstr( Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  FText *Ctxt    = (FText*)args[0].addr;
  CcInt *Cbegin  = (CcInt*)args[1].addr;
  CcInt *Cend    = (CcInt*)args[2].addr;
  CcString *CRes = reinterpret_cast<CcString*>(result.addr);

  if ( !Ctxt->IsDefined() || !Cbegin->IsDefined() || !Cend->IsDefined() )
  {
    CRes->Set( false, string("") );
    return 0;
  }
  int begin  = Cbegin->GetIntval();
  int end    = Cend->GetIntval();
  int txtlen = Ctxt->TextLength();
  if( (begin < 1) || (begin > end) || (begin > txtlen) )
  {
    CRes->Set( false, string("") );
    return 0;
  }
  int n = min(min( (end-begin), (txtlen-begin) ),
              static_cast<int>(MAX_STRINGSIZE));
  string mytxt =  Ctxt->GetValue();
//   cout << "mytxt=\"" << mytxt << "\"" << endl;
  string mysubstring = mytxt.substr(begin-1, n+1);
//   cout << "mysubstring=\"" << mysubstring << "\"" << endl;
  CRes->Set( true, mysubstring );
  return 0;
}

/*
4.28 Operator ~subtext~


*/
int ValMapSubtext( Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  FText *CRes   = (FText*)(result.addr);
  FText *Ctxt   = (FText*)args[0].addr;
  CcInt *Cbegin = (CcInt*)args[1].addr;
  CcInt *Cend   = (CcInt*)args[2].addr;

  if ( !Ctxt->IsDefined() || !Cbegin->IsDefined() || !Cend->IsDefined() )
  {
    CRes->Set( false, string("") );
    return 0;
  }
  int begin   = Cbegin->GetIntval();
  int end     = Cend->GetIntval();
  string mTxt = Ctxt->GetValue();
  int txtlen  = mTxt.length();
  if( (begin < 1) || (begin > end) || (begin > txtlen) )
  {
    CRes->Set( false, string("") );
    return 0;
  }
  int n = min( (end - begin)+1, txtlen);
  string myRes = mTxt.substr(begin-1, n);
  CRes->Set( true, myRes );
  return 0;
}

/*
4.28 Operator ~crypt~

*/
template<class T>
int cryptVM(Word* args, Word& result, int message,
            Word& local, Supplier s ) {

  result = qp->ResultStorage(s);
  CcString* res = static_cast<CcString*>(result.addr);
  T* arg = static_cast<T*>(args[0].addr);
  if(!arg->IsDefined()){
     res->SetDefined(false);
     return 0;
  }
  string a = arg->GetValue();
  srand( (unsigned) time(0) ) ;

  char s1 = (rand() / ( RAND_MAX / 63 + 1 ))+46;
  char s2 = (rand() / ( RAND_MAX / 63 + 1 ))+46;

  if(s1>57){
    s1 += 7;
  }
  if(s1>90){
    s1 += 6;
  }
  if(s2>57){
    s2 += 7;
  }
  if(s2>90){
    s2 += 6;
  }
  char salt[] = {  s1, s2 };
  string b = (Crypt::crypt(a.c_str(),salt));
  res->Set(true,b);
  return 0;
}

template<class T1, class T2>
int cryptVM(Word* args, Word& result, int message,
            Word& local, Supplier s ) {

  result = qp->ResultStorage(s);
  CcString* res = static_cast<CcString*>(result.addr);
  T1* arg1 = static_cast<T1*>(args[0].addr);
  T2* arg2 = static_cast<T2*>(args[1].addr);
  if(!arg1->IsDefined() || !arg2->IsDefined()){
     res->SetDefined(false);
     return 0;
  }
  string a = arg1->GetValue();
  string b = arg2->GetValue();
  while(b.length()<2){
    b += "X";
  }

  string c (Crypt::crypt(a.c_str(),b.c_str()));
  res->Set(true,c);
  return 0;
}

 // value mapping array

ValueMapping cryptvm[] = {
      cryptVM<CcString>, cryptVM<FText>,
      cryptVM<CcString, CcString>, cryptVM<CcString, FText>,
      cryptVM<FText, CcString>, cryptVM<FText, FText>
  };

 // Selection function
 int cryptSelect(ListExpr args){
   int l = nl->ListLength(args);
   string s1 = nl->SymbolValue(nl->First(args));
   if(l==1){
     if(s1=="string") return 0;
     if(s1=="text") return 1;
   } else {
     string s2 = nl->SymbolValue(nl->Second(args));
     if(s1=="string" && s2=="string") return 2;
     if(s1=="string" && s2=="text") return 3;
     if(s1=="text" && s2=="string") return 4;
     if(s1=="text" && s2=="text") return 5;
   }
   return -1; // type mapping and selection are not compatible
 }

/*
4.27 Operator checkpw

*/
template<class T1, class T2>
int checkpwVM(Word* args, Word& result, int message,
              Word& local, Supplier s ) {

  result = qp->ResultStorage(s);
  CcBool* res = static_cast<CcBool*>(result.addr);
  T1* arg1 = static_cast<T1*>(args[0].addr);
  T2* arg2 = static_cast<T2*>(args[1].addr);
  if(!arg1->IsDefined() || !arg2->IsDefined()){
     res->SetDefined(false);
     return 0;
  }
  string a = arg1->GetValue();
  string b = arg2->GetValue();
  res->Set(true, Crypt::validate(a.c_str(),b.c_str()));
  return 0;
}

ValueMapping checkpwvm[] = {
   checkpwVM<CcString, CcString>,
   checkpwVM<CcString, FText>,
   checkpwVM<FText, CcString>,
   checkpwVM<FText, FText>
};


int checkpwSelect(ListExpr args){
  string s1 = nl->SymbolValue(nl->First(args));
  string s2 = nl->SymbolValue(nl->Second(args));
  if(s1=="string" && s2=="string") return 0;
  if(s1=="string" && s2=="text") return 1;
  if(s1=="text" && s2=="string") return 2;
  if(s1=="text" && s2=="text") return 3;
  return -1; // type mapping and selection are not compatible
 }



/*
4.28 Operator ~md5~

*/
template<class T>
int md5VM(Word* args, Word& result, int message,
          Word& local, Supplier s ) {

   result = qp->ResultStorage(s);
   CcString* res = static_cast<CcString*>(result.addr);
   T* arg = static_cast<T*>(args[0].addr);
   if(!arg->IsDefined()){
     res->SetDefined(false);
     return 0;
   }
   string a = arg->GetValue();
   unsigned char digest[16];
   MD5::md5(a.c_str(),digest);
   string r(MD5::toString(digest));
   res->Set(true, r);
   return 0;
}

template<class T1, class T2>
int md5VM(Word* args, Word& result, int message,
            Word& local, Supplier s ) {

  result = qp->ResultStorage(s);
  CcString* res = static_cast<CcString*>(result.addr);
  T1* arg1 = static_cast<T1*>(args[0].addr);
  T2* arg2 = static_cast<T2*>(args[1].addr);
  if(!arg1->IsDefined() || !arg2->IsDefined()){
     res->SetDefined(false);
     return 0;
  }
  string a = arg1->GetValue();
  string b = arg2->GetValue();
  while(b.length()<2){
    b += "X";
  }

  char*  c = (MD5::unix_encode(a.c_str(),b.c_str()));

  unsigned char *c1 = reinterpret_cast<unsigned char*>(c);

  res->Set(true,MD5::toString(c1));
  return 0;
}

 // value mapping array

ValueMapping md5vm[] = {
      md5VM<CcString>, md5VM<FText>,
      md5VM<CcString, CcString>, md5VM<CcString, FText>,
      md5VM<FText, CcString>, md5VM<FText, FText>
  };

 // Selection function
 int md5Select(ListExpr args){
   int l = nl->ListLength(args);
   string s1 = nl->SymbolValue(nl->First(args));
   if(l==1){
     if(s1=="string") return 0;
     if(s1=="text") return 1;
   } else {
     string s2 = nl->SymbolValue(nl->Second(args));
     if(s1=="string" && s2=="string") return 2;
     if(s1=="string" && s2=="text") return 3;
     if(s1=="text" && s2=="string") return 4;
     if(s1=="text" && s2=="text") return 5;
   }
   return -1; // type mapping and selection are not compatible
 }

template<class T1, class T2>
int blowfish_encodeVM(Word* args, Word& result, int message,
                      Word& local, Supplier s ) {

  result = qp->ResultStorage(s);
  FText* res = static_cast<FText*>(result.addr);
  T1* arg1 = static_cast<T1*>(args[0].addr);
  T2* arg2 = static_cast<T2*>(args[1].addr);
  if(!arg1->IsDefined() || !arg2->IsDefined()){
     res->SetDefined(false);
     return 0;
  }
  string a = arg1->GetValue();
  string b = arg2->GetValue();

  CBlowFish bf;
  unsigned char* passwd = (unsigned char*)(a.c_str());
  unsigned char* text = (unsigned char*)(b.c_str());

  bf.Initialize(passwd, a.length());
  int ol = bf.GetOutputLength(b.length());
  unsigned char out[ol];
  int l = bf.Encode(text, out, b.length());
  ostringstream ss;
  ss << std::hex;
  for(int i=0;i<l;i++){
    if(out[i]<16){
      ss << '0';
    }
    ss << (short)out[i];
  }
  res->Set(true, ss.str());
  return 0;
}


 // value mapping array

ValueMapping blowfish_encodevm[] = {
         blowfish_encodeVM<CcString, CcString>,
         blowfish_encodeVM<CcString, FText>,
         blowfish_encodeVM<FText, CcString>,
         blowfish_encodeVM<FText, FText>
};

 // Selection function
int blowfish_encodeSelect(ListExpr args){
   string s1 = nl->SymbolValue(nl->First(args));
   string s2 = nl->SymbolValue(nl->Second(args));
   if(s1=="string" && s2=="string") return 0;
   if(s1=="string" && s2=="text") return 1;
   if(s1=="text" && s2=="string") return 2;
   if(s1=="text" && s2=="text") return 3;
   return -1; // type mapping and selection are not compatible
}

/*

blowfish[_]decode

*/

int fromHex(unsigned char s){
  if(s>='0' && s<='9'){
     return s - '0';
  }
  if(s>='a' && s<='f'){
     return s -'a'+10;
  }
  if(s>='A' && s<='F'){
     return s -'A'+10;
  }
  return -1;
}

template<class T1, class T2>
int blowfish_decodeVM(Word* args, Word& result, int message,
                      Word& local, Supplier s ) {

  result = qp->ResultStorage(s);
  FText* res = static_cast<FText*>(result.addr);
  T1* arg1 = static_cast<T1*>(args[0].addr);
  T2* arg2 = static_cast<T2*>(args[1].addr);
  if(!arg1->IsDefined() || !arg2->IsDefined()){
     res->SetDefined(false);
     return 0;
  }
  string a = arg1->GetValue();
  string b = arg2->GetValue();

  CBlowFish bf;
  unsigned char* passwd = (unsigned char*)(a.c_str());
  unsigned char* text = (unsigned char*)(b.c_str());

  bf.Initialize(passwd, a.length());
  unsigned char orig[b.length()/2+1];
  // read the coded block from hex-text
  for(unsigned int i=0;i<b.length()-1;i+=2){
     int p1 = fromHex(text[i]);
     int p2 = fromHex(text[i+1]);
     if(p1<0 || p2<0){
       res->SetDefined(false);
       return 0;
     }
     orig[i/2] = (unsigned char)(p1*16+p2);
  }
  orig[b.length()/2]=0;
  bf.Decode(orig, orig, b.length()/2);
  res->Set(true,string((char*)orig));
  return 0;
}


 // value mapping array

ValueMapping blowfish_decodevm[] = {
         blowfish_decodeVM<CcString, CcString>,
         blowfish_decodeVM<CcString, FText>,
         blowfish_decodeVM<FText, CcString>,
         blowfish_decodeVM<FText, FText>
};

 // Selection function
int blowfish_decodeSelect(ListExpr args){
   return blowfish_encodeSelect(args);
}

/*
4.29 Operator ~find~


*/

struct ValMapFindLocalInfo{
  string text;
  string pattern;
  size_t textlen;
  size_t patternlen;
  size_t lastPosFound;
  bool finished;
};

template<class T1, class T2>
int FTextValMapFind( Word* args, Word& result, int message,
                     Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  T1 *Ctext = 0;
  T2 *Cpattern = 0;
  ValMapFindLocalInfo *li;

  switch( message )
  {
    case OPEN:

      Ctext    = static_cast<T1*>(args[0].addr);
      Cpattern = static_cast<T2*>(args[1].addr);
      li = new ValMapFindLocalInfo;
      li->finished = true;
      li->text = "";
      li->pattern = "";
      li->textlen = 0;
      li->patternlen = 0;
      li->lastPosFound = string::npos;
      if( Ctext->IsDefined() && Cpattern->IsDefined() )
      {
        li->text = Ctext->GetValue();
        li->pattern = Cpattern->GetValue();
        li->lastPosFound = 0;
        li->textlen      = li->text.length();
        li->patternlen   = li->pattern.length();
        li->finished = (    (li->patternlen > li->textlen)
                         || (li->patternlen == 0)
                       );
      }
      local.setAddr(li);
      return 0;

    case REQUEST:
      if(local.addr == 0)
      {
        result.addr = 0;
        return CANCEL;
      }
      li = (ValMapFindLocalInfo*) local.addr;
      if(     li->finished
          || (li->lastPosFound == string::npos)
          || (li->lastPosFound + li->patternlen > li->textlen)
        )
      {
        result.addr = 0;
        return CANCEL;
      }
      li->lastPosFound = li->text.find(li->pattern, li->lastPosFound);
      if(li->lastPosFound != string::npos)
      {
        CcInt* res = new CcInt(true, ++(li->lastPosFound));
        result.addr = res;
        return YIELD;
      }
      li->finished = false;
      result.addr = 0;
      return CANCEL;

    case CLOSE:
      if(local.addr != 0)
      {
        li = (ValMapFindLocalInfo*) local.addr;
        delete li;
        local.setAddr( Address(0) );
      }
      return 0;
  }
  return 0;
}

ValueMapping FText_VMMap_Find[] =
{
  FTextValMapFind<CcString, FText>,    //  0
  FTextValMapFind<FText, CcString>,    //  1
  FTextValMapFind<FText, FText>,       //  2
  FTextValMapFind<CcString, CcString>  //  3
};


int SVG2TEXTVM( Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
   FText* arg = static_cast<FText*>(args[0].addr);
   result = qp->ResultStorage(s);
   FText* res = static_cast<FText*>(result.addr);
   res->CopyFrom(arg);
   return 0;
}



int SelectFun_TextString_TextString( ListExpr args )
{
  NList type(args);
  if( (type.first() == NList(symbols::STRING)) &&
      (type.second() == NList(symbols::TEXT)) )
    return 0;
  if( (type.first() == NList(symbols::TEXT)) &&
      (type.second() == NList(symbols::STRING)) )
    return 1;
  if( (type.first() == NList(symbols::TEXT)) &&
      (type.second() == NList(symbols::TEXT)) )
    return 2;
  if( (type.first() == NList(symbols::STRING)) &&
      (type.second() == NList(symbols::STRING)) )
    return 3;
  // else: ERROR
  return -1;
}

/*
4.30 Operator ~isempty~


*/
int FTextValMapIsEmpty( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool *CRes = reinterpret_cast<CcBool*>(result.addr);
  FText *Ctxt    = (FText*)args[0].addr;
  CRes->Set( true, ( !Ctxt->IsDefined() || Ctxt->TextLength() == 0) );
  return 0;
}

/*
4.30 Operator ~trim~

Removes whitespaces from a text at the start and the end of the text.


*/
int FTextValMapTrim( Word* args, Word& result, int message,
                     Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  FText* res = static_cast<FText*>(result.addr);
  FText* src = static_cast<FText*>(args[0].addr);
  if(!src->IsDefined()){
     res->SetDefined(false);
  } else {
    string str = src->GetValue();
    string drop = " \t\n\r";
    str = str.erase(str.find_last_not_of(drop)+1);
    str = str.erase(0,str.find_first_not_of(drop));
    res->Set(true,str);
  }
  return 0;
}



/*
4.30 Operator ~+~

*/
template<class T1, class T2>
int FTextValMapPlus( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  FText *CRes = reinterpret_cast<FText*>(result.addr);

  T1 *Ctxt1    = (T1*)args[0].addr;
  T2 *Ctxt2    = (T2*)args[1].addr;

  if( !Ctxt1->IsDefined() || !Ctxt2->IsDefined() )
  {
    CRes->SetDefined( false );
    return 0;
  }

  string mTxt1 = string(Ctxt1->GetValue());
  string mTxt2 = string(Ctxt2->GetValue());
  string myRes = mTxt1 + mTxt2;

  CRes->Set( true, myRes );
  return 0;
}

ValueMapping FText_VMMap_Plus[] =
{
  FTextValMapPlus<CcString, FText>,    //  0
  FTextValMapPlus<FText, CcString>,    //  1
  FTextValMapPlus<FText, FText>        //  2
};

int FTextSelectFunPlus( ListExpr args )
{
  NList type(args);
  if( (type.first() == NList(symbols::STRING)) &&
      (type.second() == NList(symbols::TEXT)) )
    return 0;
  if( (type.first() == NList(symbols::TEXT)) &&
      (type.second() == NList(symbols::STRING)) )
    return 1;
  if( (type.first() == NList(symbols::TEXT)) &&
      (type.second() == NList(symbols::TEXT)) )
    return 2;
  // else: ERROR
  return -1;
}

/*
4.30 Operators  ~$<$, $<=$, $=$, $>=$, $>$, $\neq$~

----
    <, <=, =, >=, >, #: {string|text} x {string|text} --> bool
OP: 0,  1, 2,  3, 4, 5
----

*/
template<class T1, class T2, int OP>
int FTextValMapComparePred( Word* args, Word& result, int message,
                            Word& local, Supplier s )
{
  assert( (OP >=0) && (OP <=5) );

  result = qp->ResultStorage( s );
  CcBool *CRes = (CcBool*)(result.addr);
  bool res = false;
  T1 *Ctxt1 = (T1*)args[0].addr;
  T2 *Ctxt2 = (T2*)args[1].addr;
  if( !Ctxt1->IsDefined() || !Ctxt2->IsDefined() )
  {
    CRes->Set( false, false );
    return 0;
  }
  string mTxt1 = string(Ctxt1->GetValue());
  string mTxt2 = string(Ctxt2->GetValue());
  int cmp = mTxt1.compare(mTxt2);
  switch( OP )
  {
    case 0: res = (cmp <  0); break; // <
    case 1: res = (cmp <= 0); break; // <=
    case 2: res = (cmp == 0); break; //  =
    case 3: res = (cmp >= 0); break; // >=
    case 4: res = (cmp >  0); break; // >
    case 5: res = (cmp != 0); break; //  #
    default: assert( false);  break; // illegal mode of operation
  }
  CRes->Set( true, res );
  return 0;
}

ValueMapping FText_VMMap_Less[] =
{
  FTextValMapComparePred<CcString, FText, 0>,    //
  FTextValMapComparePred<FText, CcString, 0>,    //
  FTextValMapComparePred<FText, FText, 0>        //
};

ValueMapping FText_VMMap_LessEq[] =
{
  FTextValMapComparePred<CcString, FText, 1>,    //
  FTextValMapComparePred<FText, CcString, 1>,    //
  FTextValMapComparePred<FText, FText, 1>        //
};

ValueMapping FText_VMMap_Eq[] =
{
  FTextValMapComparePred<CcString, FText, 2>,    //
  FTextValMapComparePred<FText, CcString, 2>,    //
  FTextValMapComparePred<FText, FText, 2>        //
};

ValueMapping FText_VMMap_BiggerEq[] =
{
  FTextValMapComparePred<CcString, FText, 3>,    //
  FTextValMapComparePred<FText, CcString, 3>,    //
  FTextValMapComparePred<FText, FText, 3>        //
};

ValueMapping FText_VMMap_Bigger[] =
{
  FTextValMapComparePred<CcString, FText, 4>,    //
  FTextValMapComparePred<FText, CcString, 4>,    //
  FTextValMapComparePred<FText, FText, 4>        //
};

ValueMapping FText_VMMap_Neq[] =
{
  FTextValMapComparePred<CcString, FText, 5>,    //
  FTextValMapComparePred<FText, CcString, 5>,    //
  FTextValMapComparePred<FText, FText, 5>        //
};

int FTextSelectFunComparePred( ListExpr args )
{
  NList type(args);
  if( (type.first() == NList(symbols::STRING)) &&
      (type.second() == NList(symbols::TEXT)) )
    return 0;
  else if( (type.first() == NList(symbols::TEXT)) &&
           (type.second() == NList(symbols::STRING)) )
    return 1;
  else if( (type.first() == NList(symbols::TEXT)) &&
       (type.second() == NList(symbols::TEXT)) )
    return 2;
  return -1; // error
}

int FTextValueMapEvaluate( Word* args, Word& result, int message,
                          Word& local, Supplier s )
{
  FText* CCommand     = (FText*)(args[0].addr);
  CcBool* CcIsNL      = (CcBool*)(args[1].addr);
  bool *finished;
  string querystring  = "";
  string querystringParsed = "";
  string typestring   = "";
  string resultstring = "";
  string errorstring  = "";
  Word queryresultword;
  bool success        = false;
  bool correct        = false;
  bool evaluable      = false;
  bool defined        = false;
  bool isFunction     = false;
  StopWatch myTimer;
  double myTimeReal   = 0;
  double myTimeCPU    = 0;

  SecParser mySecParser;
  ListExpr parsedCommand;
  TupleType *resultTupleType = 0;
  Tuple *newTuple            = 0;

  FText  *CcCmdStr           = 0;
  CcBool *CcSuccess          = 0;
  CcBool *CcCorrect          = 0;
  CcBool *CcEvaluable        = 0;
  CcBool *CcDefined          = 0;
  CcBool *CcIsFunction       = 0;
  FText  *CcResultType       = 0;
  FText  *CcResult           = 0;
  FText  *CcErrorMessage     = 0;
  CcReal *CcElapsedTimeReal  = 0;
  CcReal *CcElapsedTimeCPU   = 0;

  switch(message)
  {
    case OPEN:
      finished = new bool(false);
      local.addr = finished;
      *finished = (!CCommand->IsDefined() || !CcIsNL->IsDefined());
      return 0;
    case REQUEST:
      if(local.addr == 0)
      {
        result.setAddr(0);
        return CANCEL;
      }
      finished = (bool*)(local.addr);
      if(*finished)
      {
        result.setAddr(0);
        return CANCEL;
      }
      if(!CCommand->IsDefined())
      {
        *finished = true;
        result.setAddr(0);
        return CANCEL;
      }

      correct = true;
      querystring = CCommand->GetValue();

      if( !CcIsNL->GetBoolval() ) // Command in SecondoExecutableLanguage
      {
        // call Parser: add "query" and transform expression
        //              to nested-list-string
        if(mySecParser.Text2List( "query " + querystring,
                                       querystringParsed ) != 0)
        {
          errorstring = "ERROR: Text does not contain a "
              "parsable query expression.";
          correct = false;
        }
      }
      else // Command is already a nested-list-string: just copy
      {
        querystringParsed = querystring;
      }
      if( correct)
      { // read nested list: transform nested-list-string to nested list
        if (!nl->ReadFromString(querystringParsed, parsedCommand) )
        {
          errorstring = "ERROR: Text does not produce a "
              "valid nested list expression.";
          correct = false;
          cout << "NLimport: " << errorstring << endl;
        }
      }
      if ( correct && !CcIsNL->GetBoolval() )
      {  // remove the "query" from the list
        if ( (nl->ListLength(parsedCommand) == 2) )
        {
          parsedCommand = nl->Second(parsedCommand);
          //string parsedCommandstr;
          //nl->WriteToString(parsedCommandstr, parsedCommand);
          //cout << "NLimport: OK. parsedCommand=" << parsedCommandstr  << endl;
        }
        else
        {
          errorstring = "ERROR: Text does not produce a "
              "valid nested query list expression.";
          correct = false;
          //cout << "NLimport: " << errorstring  << endl;
        }
      }
      if (correct)
      { // evaluate command
        myTimer.start();
        success =
            QueryProcessor::ExecuteQuery(parsedCommand,
                                         queryresultword,
                                         typestring,
                                         errorstring,
                                         correct,
                                         evaluable,
                                         defined,
                                         isFunction);
        myTimeReal = myTimer.diffSecondsReal();
        myTimeCPU  = myTimer.diffSecondsCPU();

        // handle result
        ListExpr queryResType;
        if ( !nl->ReadFromString( typestring, queryResType) )
        {
          errorstring = "ERROR: Invalid resulttype. " + errorstring;
        }
        else
        {
          if(   correct
                && evaluable
                && defined
                && ( typestring != "typeerror"  )
            )
          { // yielded a result (no typerror)
            ListExpr valueList = SecondoSystem::GetCatalog()
                ->OutObject(queryResType,queryresultword);

            if(! SecondoSystem::GetCatalog()->
                   DeleteObj(queryResType,queryresultword)){
              cerr << "problem in deleting queryresultword" << endl;

            }
            nl->WriteToString(resultstring,valueList);
          }
          // else: typeerror or nonevaluable query
          //       - Just return retrieved values and let user decide -
        }
      }
      // handle times
      // create result tuple
      if(traces){
        cout << "\n---------------------------------------------------" << endl;
        cout << "\tsuccess     =" << (success ? "yes" : "no"    ) << endl;
        cout << "\tevaluable   =" << (evaluable ? "yes" : "no"  ) << endl;
        cout << "\tdefined     =" << (defined ? "yes" : "no"    ) << endl;
        cout << "\tisFunction  =" << (isFunction ? "yes" : "no" ) << endl;
        cout << "\tquerystring =" << querystring  << endl;
        cout << "\ttypestring  =" << typestring   << endl;
        cout << "\tresultstring=" << resultstring << endl;
        cout << "\terrorstring =" << errorstring  << endl;
        cout << "---------------------------------------------------" << endl;
      }

      resultTupleType = new TupleType(nl->Second(GetTupleResultType(s)));
      newTuple = new Tuple( resultTupleType );

      CcCmdStr       = new FText(true, querystring);
      CcSuccess      = new CcBool(true, success);
      CcCorrect      = new CcBool(true, correct);
      CcEvaluable    = new CcBool(true, evaluable);
      CcDefined      = new CcBool(true, defined);
      CcIsFunction   = new CcBool(true, isFunction);
      CcResultType   = new FText(true, typestring);
      CcResult       = new FText(true, resultstring);
      CcErrorMessage = new FText(true, errorstring);
      CcElapsedTimeReal = new CcReal(true, myTimeReal);
      CcElapsedTimeCPU  = new CcReal(true, myTimeCPU);

      newTuple->PutAttribute(  0,(Attribute*)CcCmdStr);
      newTuple->PutAttribute(  1,(Attribute*)CcSuccess);
      newTuple->PutAttribute(  2,(Attribute*)CcCorrect);
      newTuple->PutAttribute(  3,(Attribute*)CcEvaluable);
      newTuple->PutAttribute(  4,(Attribute*)CcDefined);
      newTuple->PutAttribute(  5,(Attribute*)CcIsFunction);
      newTuple->PutAttribute(  6,(Attribute*)CcResultType);
      newTuple->PutAttribute(  7,(Attribute*)CcResult);
      newTuple->PutAttribute(  8,(Attribute*)CcErrorMessage);
      newTuple->PutAttribute(  9,(Attribute*)CcElapsedTimeReal);
      newTuple->PutAttribute( 10,(Attribute*)CcElapsedTimeCPU);

      result.setAddr(newTuple);
      resultTupleType->DeleteIfAllowed();
      *finished = true;
      return YIELD;

    case CLOSE:
      if(local.addr != 0)
      {
        finished = (bool*)(local.addr);
        delete finished;
        local.setAddr(0);
      }
      return 0;
  }
  return 0;
}

/*
Value Mapping Function for Operator ~replace~

*/

// {text|string} x {text|string} x {text|string} --> text
template<class T1, class T2, class T3>
int FTextValMapReplace( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  FText* Res = static_cast<FText*>(result.addr);

  T1* text       = static_cast<T1*>(args[0].addr);
  T2* patternOld = static_cast<T2*>(args[1].addr);
  T3* patternNew = static_cast<T3*>(args[2].addr);

  if(    !text->IsDefined()
      || !patternOld->IsDefined()
      || !patternNew->IsDefined()
    )
  {
    Res->Set(false, "");
    return 0;
  }
  string textStr       = text->GetValue();
  string patternOldStr = patternOld->GetValue();
  string patternNewStr = patternNew->GetValue();
  string textReplaced = "";
  textReplaced = replaceAll(textStr, patternOldStr, patternNewStr);
  Res->Set(true, textReplaced);
  return 0;
}

// {text|string} x int    x int  x {text|string} --> text
template<class T1, class T2>
int FTextValMapReplace2( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  FText* Res = static_cast<FText*>(result.addr);

  T1*    text    = static_cast<T1*>   (args[0].addr);
  CcInt* start   = static_cast<CcInt*>(args[1].addr);
  CcInt* end     = static_cast<CcInt*>(args[2].addr);
  T2* patternNew = static_cast<T2*>   (args[3].addr);

  if(    !text->IsDefined()
          || !start->IsDefined()
          || !end->IsDefined()
          || !patternNew->IsDefined()
    )
  {
    Res->Set(false, "");
    return 0;
  }
  string textStr        = text->GetValue();
  string patternNewStr  = patternNew->GetValue();
  int starti            = start->GetIntval();
  int endi              = end->GetIntval();
  unsigned int len      = (unsigned int)(endi-starti);
  if(    (starti > endi)                             // illegal end pos
      || (starti < 1)                                // illegal startpos
      || (unsigned int)starti > textStr.length()    // illegal startpos
    )
  { // nothing to do
    Res->Set(true, textStr);
    return 0;
  }
  string textReplaced = "";
  textReplaced = textStr.replace(starti-1, len+1, patternNewStr);
  Res->Set(true, textReplaced);
  return 0;
}


/*
ValueMappingArray for ~replace~

*/

ValueMapping FText_VMMap_Replace[] =
{
  // {text|string} x {text|string} x {text|string} --> text
  FTextValMapReplace<FText,    FText,    FText>,    //  0
  FTextValMapReplace<FText,    FText,    CcString>, //  1
  FTextValMapReplace<FText,    CcString, FText>,    //  2
  FTextValMapReplace<FText,    CcString, CcString>, //  3
  FTextValMapReplace<CcString, FText,    FText>,    //  4
  FTextValMapReplace<CcString, FText,    CcString>, //  5
  FTextValMapReplace<CcString, CcString, FText>,    //  6
  FTextValMapReplace<CcString, CcString, CcString>, //  7
  // {text|string} x int    x int  x {text|string} --> text
  FTextValMapReplace2<FText,    FText>,             //  8
  FTextValMapReplace2<FText,    CcString>,          //  9
  FTextValMapReplace2<CcString, FText>,             // 10
  FTextValMapReplace2<CcString, CcString>           // 11
};


/*
Selection function for ~replace~

*/

int FTextSelectFunReplace( ListExpr args )
{
  NList type(args);
  int result = 0;
  if(type.hasLength(3))
  { // {text|string} x {text|string} x {text|string} --> text
    result = 0;
    if( type.third() == NList(symbols::STRING) )
      result += 1;
    if( type.second() == NList(symbols::STRING) )
      result += 2;
    if( type.first() == NList(symbols::STRING) )
      result += 4;
  }
  else
  { // {text|string} x int x int x {text|string} --> text
    result = 8;
    if( type.fourth() == NList(symbols::STRING) )
      result += 1;
    if( type.first() == NList(symbols::STRING) )
      result += 2;
  }
  return result;
}

/*
Value Mapping Function for Operator ~isDBObject~

*/
int FTextValueMapIsDBObject( Word* args, Word& result, int message,
                            Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool* Res = static_cast<CcBool*>(result.addr);
  CcString* objName=static_cast<CcString*>(args[0].addr);
  if(!objName->IsDefined()){
     Res->Set(false,false);
  } else {
     string oname = objName->GetValue();
     SecondoCatalog* ctl = SecondoSystem::GetCatalog();
     Res->Set(true,ctl->IsObjectName(oname));
  }


  return 0;
}


/*
Value Mapping Function for Operator ~getTypeNL~

*/
int FTextValueMapGetTypeNL( Word* args, Word& result, int message,
                            Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  FText* Res = static_cast<FText*>(result.addr);
  FText* myType = static_cast<FText*>(args[1].addr);
  Res->CopyFrom((Attribute*)myType);
  return 0;
}

/*
Value Mapping Function for Operator ~getValueNL~

*/
// for single value
int FTextValueMapGetValueNL_single( Word* args, Word& result, int message,
                              Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  FText*   Res       = static_cast<FText*>(result.addr);
  FText*   myTypeFT  = static_cast<FText*>(args[1].addr);
  string   myTypeStr = "";
  string   valueStr  = "";
  ListExpr myTypeNL;

  if( !myTypeFT->IsDefined() )
  { // Error: undefined type description
    cerr << __PRETTY_FUNCTION__<< "(" << __FILE__ << __LINE__
        << "): ERROR: Undefined resulttype." << endl;
    Res->Set( false, "" );
    return 0;
  }
  if( args[0].addr == 0 )
  { // Error: NULL-pointer value
    cerr << __PRETTY_FUNCTION__<< "(" << __FILE__ << __LINE__
        << "): ERROR: NULL-pointer argument." << endl;
    Res->Set( false, "" );
    return 0;
  }
  myTypeStr = myTypeFT->GetValue();
  if ( !nl->ReadFromString( myTypeStr, myTypeNL) )
  { // Error: could not parse type description
    cerr << __PRETTY_FUNCTION__<< "(" << __FILE__ << __LINE__
        << "): ERROR: Invalid resulttype." << endl;
    Res->Set( false, "" );
    return 0;
  }
  else if( myTypeStr != "typeerror"  )
  {
    ListExpr valueNL =
        SecondoSystem::GetCatalog()->OutObject(myTypeNL,args[0]);
    nl->WriteToString(valueStr,valueNL);
    Res->Set( true, valueStr);
    return 0;
  }
  Res->Set( false, "" );
  return 0;
}

struct FTextValueMapGetValueNL_streamLocalInfo
{
  ListExpr myTypeNL;
  bool finished;
};

// for data streams
int FTextValueMapGetValueNL_stream( Word* args, Word& result, int message,
                             Word& local, Supplier s )
{
  result          = qp->ResultStorage( s );
  FText* Res      = 0;
  FText* myTypeFT = static_cast<FText*>(args[1].addr);
  FTextValueMapGetValueNL_streamLocalInfo *li;
  string   valueStr  = "";
  Word elem;

  switch( message )
  {
    case OPEN:
      li = new FTextValueMapGetValueNL_streamLocalInfo;
      li->finished = true;
      if( myTypeFT->IsDefined() )
      {
        string myTypeStr = myTypeFT->GetValue();
        if (    (myTypeStr != "typeerror")
             && nl->ReadFromString( myTypeStr, li->myTypeNL)
           )
        {
          li->finished = false;
        }
      }
      qp->Open(args[0].addr);
      local.setAddr( li );
      return 0;

    case REQUEST:

      if(local.addr)
        li = (FTextValueMapGetValueNL_streamLocalInfo*) local.addr;
      else
      {
        return CANCEL;
      }
      if(li->finished)
      {
        return CANCEL;
      }
      qp->Request(args[0].addr, elem);
      if ( qp->Received(args[0].addr) )
      {
        ListExpr valueNL =
              SecondoSystem::GetCatalog()->OutObject(li->myTypeNL,elem);
        nl->WriteToString(valueStr,valueNL);
        Res = new FText(true, valueStr);
        result.setAddr( Res );
        ((Attribute*) elem.addr)->DeleteIfAllowed();
        return YIELD;
      }
      // stream exhausted - we are finished
      result.addr = 0;
      li->finished = true;
      return CANCEL;

    case CLOSE:

      qp->Close(args[0].addr);
      if(local.addr)
      {
        li = (FTextValueMapGetValueNL_streamLocalInfo*) local.addr;
        delete li;
        local.setAddr(0);
      }
      return 0;
  }
  /* should not happen */
  return -1;
}

// for tuple-streams
int FTextValueMapGetValueNL_tuplestream( Word* args, Word& result, int message,
                             Word& local, Supplier s )
{
  result          = qp->ResultStorage( s );
  FText* Res      = 0;
  FText* myTypeFT = static_cast<FText*>(args[1].addr);
  FTextValueMapGetValueNL_streamLocalInfo *li;
  li = static_cast<FTextValueMapGetValueNL_streamLocalInfo*>(local.addr);
  Tuple*          myTuple = 0;
  string valueStr = "";
  Word elem;

  switch( message )
  {
    case OPEN:
      if(li){
        delete li;
      }
      li = new FTextValueMapGetValueNL_streamLocalInfo;
      li->finished = true;
      if( myTypeFT->IsDefined() )
      {
        string myTypeStr = myTypeFT->GetValue();
        if (    (myTypeStr != "typeerror")
             && nl->ReadFromString( myTypeStr, li->myTypeNL)
           )
        {
          li->myTypeNL = nl->OneElemList(li->myTypeNL);
          li->finished = false;
        }
      }
      qp->Open(args[0].addr);
      local.setAddr( li );
      return 0;

    case REQUEST:

      if(local.addr)
        li = (FTextValueMapGetValueNL_streamLocalInfo*) local.addr;
      else
      {
        return CANCEL;
      }
      if(li->finished)
      {
        return CANCEL;
      }
      qp->Request(args[0].addr, elem);
      if ( qp->Received(args[0].addr) )
      {
        myTuple = static_cast<Tuple*>(elem.addr);
        ListExpr valueNL = myTuple->Out( li->myTypeNL );
        nl->WriteToString(valueStr,valueNL);
        Res = new FText(true, valueStr);
        result.setAddr( Res );
        myTuple->DeleteIfAllowed();
        return YIELD;
      }
      // stream exhausted - we are finished
      result.addr = 0;
      li->finished = true;
      return CANCEL;

    case CLOSE:
      if(li){
         delete li;
      }
      qp->Close(args[0].addr);
      return 0;
  }
  /* should not happen */
  return -1;
}

/*
ValueMappingArray for ~replace~

*/

ValueMapping FText_VMMap_GetValueNL[] =
{
  FTextValueMapGetValueNL_tuplestream, // 0
  FTextValueMapGetValueNL_stream,      // 1
  FTextValueMapGetValueNL_single       // 2
};


/*
Selection function for ~replace~

*/

int FTextSelectFunGetValueNL( ListExpr args )
{
  NList type(args);
  if(    type.first().hasLength(2)
      && (type.first().first() == "stream")
      && (type.first().second().hasLength(2))
      && (type.first().second().first() == "tuple")
    )
  {
    return 0; // tuplestream
  }
  if(     type.first().hasLength(2)
       && (type.first().first() == "stream")
    )
  {
    return 1; // datastream
  }
  return 2;     // normal object
}


/*
Value Mapping Function for Operator ~toobject~

*/

// Auxiliary function to get AlgebraId and Type ID from a Type-ListExpr
void FTextGetIds(int& algebraId, int& typeId, const ListExpr typeInfo)
{
  if(nl->IsAtom(typeInfo)) {
    return;
  }

  ListExpr b1 = nl->First(typeInfo);
  if(nl->IsAtom(b1)) {
    //typeInfo = type = (algId ...)
    if (nl->ListLength(typeInfo)!=2) {
      return;
    }
      //list = (algid typeid)
    algebraId = nl->IntValue(nl->First(typeInfo));
    typeId = nl->IntValue(nl->Second(typeInfo));
  } else {
    //typeInfo = (type1 type2).
    //We only need type1 since a collection can only be of one type.
    //type1 is b1 (nl->First(typeInfo)), so b1 is (algId typeId).
    if (nl->ListLength(b1)!=2) {
      return;
    }
      //b1 = (algId typeId)
    algebraId = nl->IntValue(nl->First(b1));
    typeId = nl->IntValue(nl->Second(b1));
  }
}

template<class T>
int FTextValueMapToObject( Word* args, Word& result, int message,
                           Word& local, Supplier s )
{
  T* InText= static_cast<T*>(args[0].addr);
  result = qp->ResultStorage( s );
  Attribute* Res = static_cast<Attribute*>(result.addr);

  if(!InText->IsDefined())
  { // undefined text -> return undefined object
//     cout << __PRETTY_FUNCTION__ << " (" << __FILE__ << " line "
//          << __LINE__ << "): Text is undefined -> Undefined object."  << endl;
    Res->SetDefined(false);
    return 0;
  }
  // extract the text
  string myText = InText->GetValue();
  // read nested list: transform nested-list-string to nested list
  ListExpr myTextNL;
  if (!nl->ReadFromString(myText, myTextNL) ) // HIER SPEICHERLOCH!
  {
//     cout << __PRETTY_FUNCTION__ << " (" << __FILE__ << " line "
//         << __LINE__ << "): Text does not produce a "
//         "valid nested list expression -> Undefined object."  << endl;
    Res->SetDefined(false);
    return 0;
  }
  // get information on resulttype
  ListExpr myTypeNL = qp->GetType( s );
  // call InFunction
  Word myRes( Address(0) );
  int errorPos = 0;
  ListExpr& errorInfo = nl->GetErrorList();
  bool correct = true;
  myRes = SecondoSystem::GetCatalog()->InObject( myTypeNL,
                                                 myTextNL,
                                                 errorPos,
                                                 errorInfo,
                                                 correct
                                               );
  if(!correct)
  {
//     cout << __PRETTY_FUNCTION__ << " (" << __FILE__ << " line "
//          << __LINE__ << "): InFunction failed -> Undefined object."  << endl;
//     cout << "\tmyTypeNL  = " << nl->ToString(myTypeNL) << endl;
//     cout << "\tmyTextNL  = " << nl->ToString(myTextNL) << endl;
//     cout << "\terrorInfo = " << nl->ToString(errorInfo) << endl;
//     cout << "\tErrorPos  = " << errorPos << endl;
    Res->SetDefined(false);
    return 0;
  }
//   else
//   {
//     cout << __PRETTY_FUNCTION__ << " (" << __FILE__ << " line "
//         << __LINE__ << "): InFunction succeeded:"  << endl;
//     cout << "\tmyTypeNL  = " << nl->ToString(myTypeNL) << endl;
//     cout << "\tmyTextNL  = " << nl->ToString(myTextNL) << endl;
//     cout << "\tObject    = ";
//     ((Attribute*)(myRes.addr))->Print(cout); cout << endl;
//   }
  // Pass the Result
  qp->DeleteResultStorage(s);
  qp->ChangeResultStorage(s,myRes);
  result = qp->ResultStorage(s);
  return 0;
}

/*
ValueMappingArray for ~toobject~

*/

ValueMapping FText_VMMap_ToObject[] =
{
  FTextValueMapToObject<CcString>,   // 0
  FTextValueMapToObject<FText>,      // 1
};

/*
Selection function for ~toobject~

*/

int FTextSelectFunToObject( ListExpr args )
{
  NList type(args);
  if( type.first() == "string" )
    return 0;
  if ( type.first() == "text" )
    return 1;
  return -1; // should not happen!
}

/*
Value Mapping Function for ~charT~

*/

int FTextValueMapChartext( Word* args, Word& result, int message,
                           Word& local, Supplier s )
{
  CcInt* Cccode = static_cast<CcInt*>(args[0].addr);
  result = qp->ResultStorage( s );
  FText* res = static_cast<FText*>(result.addr);

  int code = 0;
  if ( !Cccode->IsDefined() )
    res->SetDefined( false );
  else{
    code = Cccode->GetIntval();
    if( (code >= 0) && (code <= 255) )
    {
      char ch = (char) code;
      ostringstream os;
      os << ch;
      string s = os.str();
      res->Set( true, s );
    }
    else
    { // illegal code --> return undef
      res->Set( false, "" );
    }
  }
  return 0;
}

/*
Value Mapping Function for ~toupper~, ~tolower~

*/
template<bool isToLower>
int FTextValueMapChangeCase( Word* args, Word& result, int message,
                             Word& local, Supplier s )
{
  FText* text = static_cast<FText*>(args[0].addr);
  result = qp->ResultStorage( s );
  FText* res = static_cast<FText*>(result.addr);

  if ( !text->IsDefined() ){
    res->Set( false, "" );
  }else{
    string str = text->GetValue();
    if(isToLower){
      std::transform(str.begin(),str.end(), str.begin(), (int(*)(int)) tolower);
    }else{
      std::transform(str.begin(),str.end(), str.begin(), (int(*)(int)) toupper);
    }
    res->Set( true, str );
  }
  return 0;
}


/*
Value Mapping Function for ~tostring~, ~totext~

*/

template<bool isTextToString, class T1, class T2>
int FTextValueMapConvert( Word* args, Word& result, int message,
                          Word& local, Supplier s )
{
  T1* inSec = static_cast<T1*>(args[0].addr);
  result = qp->ResultStorage( s );
  T2* res = static_cast<T2*>(result.addr);

  if( !inSec->IsDefined() )
  {
    res->Set( false, "" );
    return 0;
  }
  string inStr = inSec->GetValue();
  if(isTextToString)
  {
    string outStr = inStr.substr(0,MAX_STRINGSIZE);
    outStr = replaceAll(outStr, "\"", "'");
    res->Set( true, outStr );
  }
  else
  {
    res->Set( true, inStr );
  }
  return 0;
}

/*
Operator ~sendtextUDP~

Send the text to a given host:port

Any status and error messages from the session are appended to the result text

*/

template<class T1, class T2, class T3, class T4, class T5>
int FTextValueMapSendTextUDP( Word* args, Word& result, int message,
                              Word& local, Supplier s )
{
  result      = qp->ResultStorage( s );
  FText* Res  = static_cast<FText*>(result.addr);

  int no_args = qp->GetNoSons(s);
  ostringstream status;
  bool correct = true;

  // get message text (requiered)
  T1* CcMessage   = static_cast<T1*>(args[0].addr);
  string myMessage("");
  if(!CcMessage->IsDefined()){
    status << "ERROR: Message undefined.";
    correct = false;
  }else{
    myMessage = CcMessage->GetValue();
  }
  // get remote IP (requiered)
  T2* CcOtherIP   = static_cast<T2*>(args[1].addr);
  string OtherIP("");
  if (!CcOtherIP->IsDefined()){
    status << "ERROR: remoteIP undefined.";
    correct = false;
  }else{
    OtherIP = CcOtherIP->GetValue();
  }
  if(OtherIP == ""){
    status << "ERROR: remoteIP unspecified.";
    correct = false;
  }
  // get remote port number (requiered)
  T3* CcOtherPort = static_cast<T3*>(args[2].addr);
  string OtherPort("");
  if(!CcOtherPort->IsDefined()){
    status << "ERROR: remotePort undefined.";
    correct = false;
  }else{
    OtherPort = CcOtherPort->GetValue();
  }
  if(OtherPort == ""){
    status << "ERROR: remotePort unspecified.";
    correct = false;
  }
  // get sender IP (optional)
  string myIP("");
  if(no_args>=4){
    T4* CcMyIP      = static_cast<T4*>(args[3].addr);
    if (!CcMyIP->IsDefined()){
      status << "ERROR: localIP undefined.";
      correct = false;
    }else{
      myIP = CcMyIP->GetValue();
    }
  }
  // get sender port (requiered)
  string myPort("");
  if(no_args>=5){
    T5* CcMyPort    = static_cast<T5*>(args[4].addr);
    if(!CcMyPort->IsDefined()){
      status << "ERROR: localPort undefined.";
      correct = false;
    }else{
      myPort = CcMyPort->GetValue();
    }
  }
  if(myPort == ""){
    status << "ERROR: localPort unspecified.";
    correct = false;
  }
  // ensure diffrent sender and receiver address
  if( (myIP == OtherIP) && (myPort == OtherPort) ){
    status << "ERROR: sender and receiver address identical.";
    correct = false;
  }
  // return message on error due to any ERROR in parameters
  if(!correct){
    Res->Set( true, status.str() );
    return 0;
  }
  // define address struct for local socket:
  UDPaddress localAddress(myIP,myPort /*, AF_INET */);
  //cerr << "localAddress = " << localAddress << endl;
  if ( !localAddress.isOk() ) {
    status << "ERROR: Failed creating local address ("
           << localAddress.getErrorText() << ").";
    Res->Set( true, status.str() );
    return 0;
  }
  // define address struct for remote socket (destination)
  UDPaddress remoteAddress(OtherIP,OtherPort /*, AF_INET */);
  //cerr << "remoteAddress = " << remoteAddress << endl;
  if ( !remoteAddress.isOk() ) {
    status << "ERROR: Failed creating remote address ("
        << remoteAddress.getErrorText() << ").";
    Res->Set( true, status.str() );
    return 0;
  }
  // ensure diffrent sender and receiver address
  if(     ( localAddress.getIP() == remoteAddress.getIP() )
       && ( localAddress.getPort() == remoteAddress.getPort() ) ){
    status << "ERROR: sender and receiver address identical.";
    Res->Set( true, status.str() );
    return 0;
  }
  // create the local socket
  UDPsocket localSock(localAddress);
  if( !localSock.isOk() ){
    status << "ERROR: socket() failed ("
           <<  localSock.getErrorText() << ").";
    Res->Set( true, status.str() );
    return 0;
  }
  int msg_len = myMessage.length();
  cerr << "Trying to send " << msg_len << " bytes from IP "
       << localAddress.getIP() << " Port "
       << localAddress.getPort() << " to IP "
       << remoteAddress.getIP() << " Port "
       << remoteAddress.getPort() << endl;
  int sent_len = localSock.writeTo(remoteAddress,myMessage);
  if(sent_len < 0){
    status << "ERROR: sendto() failed (" << localSock.getErrorText() << ").";
  } else if (sent_len != msg_len){
    status << "WARNING: Message sent partially (" << sent_len << "/"
           <<msg_len<< " bytes).";
    Res->Set( true, status.str() );
    return 0;
  } else{
    status << "OK. " << sent_len << " bytes sent.";
  }
  // close socket and return status
  localSock.close();
  Res->Set( true, status.str() );
  return 0;
}

/*
ValueMappingArray for ~sendtext~

*/

ValueMapping FText_VMMap_MapSendTextUDP[] =
{
  FTextValueMapSendTextUDP<CcString,CcString,CcString,CcString,CcString>,  // 0
  FTextValueMapSendTextUDP<CcString,CcString,CcString,CcString,FText   >,  //
  FTextValueMapSendTextUDP<CcString,CcString,CcString,FText,   CcString>,  // 2
  FTextValueMapSendTextUDP<CcString,CcString,CcString,FText,   FText   >,  //
  FTextValueMapSendTextUDP<CcString,CcString,FText   ,CcString,CcString>,  // 4
  FTextValueMapSendTextUDP<CcString,CcString,FText   ,CcString,FText   >,  //
  FTextValueMapSendTextUDP<CcString,CcString,FText   ,FText,   CcString>,  // 6
  FTextValueMapSendTextUDP<CcString,CcString,FText   ,FText,   FText   >,  //
  FTextValueMapSendTextUDP<CcString,FText   ,CcString,CcString,CcString>,  // 8
  FTextValueMapSendTextUDP<CcString,FText   ,CcString,CcString,FText   >,  //
  FTextValueMapSendTextUDP<CcString,FText   ,CcString,FText,   CcString>,  // 10
  FTextValueMapSendTextUDP<CcString,FText   ,CcString,FText,   FText   >,  //
  FTextValueMapSendTextUDP<CcString,FText   ,FText   ,CcString,CcString>,  // 12
  FTextValueMapSendTextUDP<CcString,FText   ,FText   ,CcString,FText   >,  //
  FTextValueMapSendTextUDP<CcString,FText   ,FText   ,FText,   CcString>,  // 14
  FTextValueMapSendTextUDP<CcString,FText   ,FText   ,FText,   FText   >,  //
  FTextValueMapSendTextUDP<FText   ,CcString,CcString,CcString,CcString>,  // 16
  FTextValueMapSendTextUDP<FText   ,CcString,CcString,CcString,FText   >,  //
  FTextValueMapSendTextUDP<FText   ,CcString,CcString,FText,   CcString>,  // 18
  FTextValueMapSendTextUDP<FText   ,CcString,CcString,FText,   FText   >,  //
  FTextValueMapSendTextUDP<FText   ,CcString,FText   ,CcString,CcString>,  // 20
  FTextValueMapSendTextUDP<FText   ,CcString,FText   ,CcString,FText   >,  //
  FTextValueMapSendTextUDP<FText   ,CcString,FText   ,FText,   CcString>,  // 22
  FTextValueMapSendTextUDP<FText   ,CcString,FText   ,FText,   FText   >,  //
  FTextValueMapSendTextUDP<FText   ,FText   ,CcString,CcString,CcString>,  // 24
  FTextValueMapSendTextUDP<FText   ,FText   ,CcString,CcString,FText   >,  //
  FTextValueMapSendTextUDP<FText   ,FText   ,CcString,FText,   CcString>,  // 26
  FTextValueMapSendTextUDP<FText   ,FText   ,CcString,FText,   FText   >,  //
  FTextValueMapSendTextUDP<FText   ,FText   ,FText   ,CcString,CcString>,  // 28
  FTextValueMapSendTextUDP<FText   ,FText   ,FText   ,CcString,FText   >,  //
  FTextValueMapSendTextUDP<FText   ,FText   ,FText   ,FText,   CcString>,  // 30
  FTextValueMapSendTextUDP<FText   ,FText   ,FText   ,FText,   FText   >   // 31
};

/*
Selection function for ~sendtext~

*/

int FTextSelectSendTextUDP( ListExpr args )
{
  NList type(args);
  int noargs = nl->ListLength(args);
  int index = 0;
  if( noargs>=1 && type.first()=="text" )
    index += 16;
  if( noargs>=2 && type.second()=="text" )
    index += 8;
  if( noargs>=3 && type.third()=="text" )
    index += 4;
  if( noargs>=4 && type.fourth()=="text" )
    index += 2;
  if( noargs>=5 && type.fifth()=="text" )
    index += 1;
  return index;
}

/*
Operator ~receivetextUDP~

Receive text from a remote host

*/

// template to convert from string
template<typename T>
bool FromString( const std::string& str, T& result )
{
  std::istringstream is(str);
  is >> result;
  if( !is ) {
    return false;
  }
  return true;
}

template<class T1, class T2>
int FTextValueMapReceiveTextUDP( Word* args, Word& result, int message,
                                 Word& local, Supplier s )
{
  bool *finished             = 0;

  CcBool *ccOk               = 0;
  FText *ccMsg               = 0;
  CcString *ErrMsg           = 0;
  CcString *SenderIP         = 0;
  CcString *SenderPort       = 0;
  CcString *SenderIPversion  = 0;
  TupleType *resultTupleType = 0;
  Tuple *newTuple            = 0;

  bool   m_Ok              = true;
  string m_Msg             = "";
  string m_ErrMsg          = "";

  T1* CcMyIP         = static_cast<T1*>(args[0].addr);
  T2* CcMyPort       = static_cast<T2*>(args[1].addr);
  CcReal* CcRtimeout = static_cast<CcReal*>(args[2].addr);

  string myIP("");
  string myPort("");
  double timeoutSecs = 0.0;
  int iMyPort = 0;
  ostringstream status;

  switch( message )
  {
    case OPEN:{
      finished = new bool(false);
      local.setAddr( finished );
      return 0;
    }
    case REQUEST:{
      // check whether already finished
      if (local.addr == 0){
        return CANCEL;
      }
      finished = (bool*) local.addr;
      if( *finished ){
        return CANCEL;
      }
      // get arguments
      if (!CcMyIP->IsDefined()){ // get own IP
        status << "LocalIP undefined. ";
        m_Ok = false;
      }else{
        myIP = CcMyIP->GetValue();
      }
      if(!CcMyPort->IsDefined()){ // get own port
        status << "LocalPort undefined. ";
        m_Ok = false;
      }else{
        myPort = CcMyPort->GetValue();
      }
      if( (!FromString<int> (myPort,iMyPort))
                || (iMyPort < 1024)
                || (iMyPort > 65536)) {
        status << "LocalPort " << iMyPort
               << " is no valid port number. ";
        m_Ok = false;
      }
      if(CcRtimeout->IsDefined()){ // get timeout
        timeoutSecs = CcRtimeout->GetRealval();
      }
      if(timeoutSecs > 0.0){
        cout << "INFO: receivetextUDP: Timeout = " << timeoutSecs
             << " secs." << endl;
      } else {
        cout << "INFO: receivetextUDP: No timeout." << endl;
      }
      UDPaddress localAddress;
      UDPaddress senderAddress;
      // define address for local datagram socket:
      if(m_Ok){
        localAddress = UDPaddress(myIP,myPort);
        if ( !(localAddress.isOk()) ) {
          status << localAddress.getErrorText() << ".";
          m_Ok = false;
        }
      }
      if(m_Ok){
      // create the socket
        UDPsocket my_socket(localAddress);
        if(!my_socket.isOk()){
          status << my_socket.getErrorText() << ".";
          m_Ok = false;
        }
      // bind socket to local port
        if(m_Ok && !my_socket.bind()){
          status << my_socket.getErrorText() << ".";
          m_Ok = false;
        }
      // receive a message
        if(m_Ok){
          m_Msg = my_socket.readFrom(senderAddress,timeoutSecs);
          if( !(my_socket.isOk()) ){
            status << my_socket.getErrorText();
            m_Ok = false;
          }
        }
        if (!my_socket.close()){
          status << my_socket.getErrorText() << ".";
          m_Ok = false;
        }
      // create result tuple
      m_ErrMsg = status.str(); // get error messages

      ccOk            = new CcBool(true, m_Ok);
      ccMsg           = new FText((m_Msg.length() > 0), m_Msg);
      ErrMsg          = new CcString(true, m_ErrMsg);
      SenderIP        = new CcString(senderAddress.getIP() != "",
                                     senderAddress.getIP());
      SenderPort      = new CcString(senderAddress.getPort() != "",
                                     senderAddress.getPort());
      SenderIPversion = new CcString(senderAddress.getFamily() != "",
                                     senderAddress.getFamily());
      resultTupleType = new TupleType(nl->Second(GetTupleResultType(s)));
      newTuple        = new Tuple( resultTupleType );
      newTuple->PutAttribute(  0,(Attribute*)ccOk);
      newTuple->PutAttribute(  1,(Attribute*)ccMsg);
      newTuple->PutAttribute(  2,(Attribute*)ErrMsg);
      newTuple->PutAttribute(  3,(Attribute*)SenderIP);
      newTuple->PutAttribute(  4,(Attribute*)SenderPort);
      newTuple->PutAttribute(  5,(Attribute*)SenderIPversion);
      result.setAddr(newTuple);
      *finished = true;

      // free created objects
      resultTupleType->DeleteIfAllowed();
      }
      return YIELD;
    }
    case CLOSE:{
      if (local.addr != 0){
        finished = (bool*) local.addr;
        delete finished;
        local.addr = 0;
      }
      return 0;
    }
  }
  /* should not happen */
  return -1;
}

ValueMapping FText_VMMap_MapReceiveTextUDP[] =
{
  FTextValueMapReceiveTextUDP<CcString,CcString>,  // 0
  FTextValueMapReceiveTextUDP<CcString,FText   >,  // 1
  FTextValueMapReceiveTextUDP<FText,   CcString>,  // 2
  FTextValueMapReceiveTextUDP<FText,   FText   >   // 3
};

// {string|text}^2 x real
int FTextSelectReceiveTextUDP( ListExpr args )
{
  NList type(args);
  int index = 0;
  if( type.first()=="text" )
    index += 2;
  if( type.second()=="text" )
    index += 1;
  return index;
}


/*
Operator ~receivetextstreamUDP~

*/

struct FTextValueMapReceiveTextStreamUDPLocalInfo{
  bool       finished;
  double     localTimeout;
  double     globalTimeout;
  bool       hasGlobalTimeout;
  bool       hasLocalTimeout;
  double     initial;
  double     final;
  UDPaddress localAddress;
  UDPsocket  my_socket;
  TupleType *resultTupleType;
};

template<class T1, class T2>
int FTextValueMapReceiveTextStreamUDP( Word* args, Word& result, int message,
                                       Word& local, Supplier s )
{
  FTextValueMapReceiveTextStreamUDPLocalInfo *li;

  CcBool    *ccOk             = 0;
  FText     *ccMsg            = 0;
  CcString  *ErrMsg           = 0;
  CcString  *SenderIP         = 0;
  CcString  *SenderPort       = 0;
  CcString  *SenderIPversion  = 0;
  Tuple     *newTuple         = 0;

  bool   m_Ok              = true;
  string m_Msg             = "";
  string m_ErrMsg          = "";

  T1* CcMyIP          = static_cast<T1*>(args[0].addr);
  T2* CcMyPort        = static_cast<T2*>(args[1].addr);
  CcReal* CcRltimeout = static_cast<CcReal*>(args[2].addr);
  CcReal* CcRgtimeout = static_cast<CcReal*>(args[3].addr);

  string myIP("");
  string myPort("");
  int iMyPort = 0;
  ostringstream status;

  UDPaddress senderAddress;

  double timeoutSecs = 0.0;

  switch( message )
  {
    case OPEN:{
      li = new FTextValueMapReceiveTextStreamUDPLocalInfo;
      li->finished      = true;
      li->localTimeout  = 0.0;
      li->globalTimeout = 0.0;
      li->localTimeout  = 0.0;
      li->globalTimeout = 0.0;
      struct timeb tb;
      ftime(&tb);                        // get current time
      li->initial = tb.time;
      li->final   = tb.time;
      li->hasGlobalTimeout = false;
      li->hasLocalTimeout  = false;

      local.setAddr( li );

      li->resultTupleType = new TupleType(nl->Second(GetTupleResultType(s)));

      // get arguments
      if (!CcMyIP->IsDefined()){ // get own IP
        status << "LocalIP undefined. ";
        m_Ok = false;
      }else{
        myIP = CcMyIP->GetValue();
      }
      if(!CcMyPort->IsDefined()){ // get own port
        status << "LocalPort undefined. ";
        m_Ok = false;
      }else{
        myPort = CcMyPort->GetValue();
      }
      if( (!FromString<int> (myPort,iMyPort))
            || (iMyPort < 1024)
            || (iMyPort > 65536)) {
        status << "LocalPort " << iMyPort
            << " is no valid port number. ";
        m_Ok = false;
      }
      // local timeout
      if(CcRltimeout->IsDefined()){ // get timeout
        li->localTimeout = CcRltimeout->GetRealval();
      }
      if(li->localTimeout > 0.0){
        li->hasLocalTimeout = true;
        cout << "INFO: receivetextstreamUDP: Local Timeout = "
             << li->localTimeout
             << " secs." << endl;
      } else {
        li->hasLocalTimeout = false;
        cout << "INFO: receivetextstreamUDP: No local timeout." << endl;
      }
      // global timeout
      if(CcRgtimeout->IsDefined()){ // get timeout
        li->globalTimeout = CcRgtimeout->GetRealval();
      }
      if(li->globalTimeout > 0.0){
        li->hasGlobalTimeout = true;
        cout << "INFO: receivetextstreamUDP: Global Timeout = "
             << li->globalTimeout
             << " secs." << endl;
      } else {
        li->hasGlobalTimeout = false;
        cout << "INFO: receivetextstreamUDP: "
            << "No global timeout (Endless stream!)." << endl;
      }
      // correct timeout arguments
      li->localTimeout = min(li->localTimeout,li->globalTimeout);
      li->hasLocalTimeout = (li->localTimeout <= li->globalTimeout);
      if(li->hasGlobalTimeout){
        li->final = li->initial + li->globalTimeout;
      }
      // define address for local datagram socket:
      if(m_Ok){
        li->localAddress = UDPaddress(myIP,myPort);
        if ( !(li->localAddress.isOk()) ) {
          status << li->localAddress.getErrorText() << ".";
          m_Ok = false;
        }
      }
      if(m_Ok){
      // create the socket
        li->my_socket = UDPsocket(li->localAddress);
        if(!li->my_socket.isOk()){
          status << li->my_socket.getErrorText() << ".";
          m_Ok = false;
        }
      // bind socket to local port
        if(m_Ok && !li->my_socket.bind()){
          status << li->my_socket.getErrorText() << ".";
          m_Ok = false;
        }
      }
      li->finished = !m_Ok;
      if(!m_Ok){
        cerr << "ERROR: " << status.str() << endl;
      }
      return 0;
    }

    case REQUEST:{
      // check whether already finished
      if (local.addr == 0){
        return CANCEL;
      }
      li = (FTextValueMapReceiveTextStreamUDPLocalInfo*) local.addr;
      if( li->finished ){
        return CANCEL;
      }
      // handle global and local timeouts
      timeb now;
      ftime(&now);                      // get current time
      if(li->hasGlobalTimeout && (li->final <= now.time) ){
        status << "Global Timeout.";
        li->finished = true;
        m_Ok = false;
      } else if(!li->hasGlobalTimeout && !li->hasLocalTimeout){
        timeoutSecs = 0.0; // blocking - wait forever
      } else if(li->hasGlobalTimeout && !li->hasLocalTimeout){
        timeoutSecs = (li->final - now.time); // remainder of global timeout
      } else if(!li->hasGlobalTimeout && li->hasLocalTimeout){
          timeoutSecs = li->localTimeout; // set local timeout;
      } else if(li->hasGlobalTimeout && li->hasLocalTimeout){
        timeoutSecs = min( li->localTimeout, (li->final - now.time) );
      } else {
        status << "ERROR: Something's wrong.";
        m_Ok = false;
        li->finished = true;
      }
      // receive a message
      if(m_Ok){
        m_Msg = li->my_socket.readFrom(senderAddress,timeoutSecs);
        if( !(li->my_socket.isOk()) ){
          status << li->my_socket.getErrorText();
          m_Ok = false;
        }
      }
      // create result tuple
      m_ErrMsg = status.str(); // get error messages
      ccOk            = new CcBool(true, m_Ok);
      ccMsg           = new FText((m_Msg.length() > 0), m_Msg);
      ErrMsg          = new CcString(true, m_ErrMsg);
      SenderIP        = new CcString(senderAddress.getIP() != "",
                                     senderAddress.getIP());
      SenderPort      = new CcString(senderAddress.getPort() != "",
                                     senderAddress.getPort());
      SenderIPversion = new CcString(senderAddress.getFamily() != "",
                                     senderAddress.getFamily());
      newTuple        = new Tuple( li->resultTupleType );
      newTuple->PutAttribute(  0,(Attribute*)ccOk);
      newTuple->PutAttribute(  1,(Attribute*)ccMsg);
      newTuple->PutAttribute(  2,(Attribute*)ErrMsg);
      newTuple->PutAttribute(  3,(Attribute*)SenderIP);
      newTuple->PutAttribute(  4,(Attribute*)SenderPort);
      newTuple->PutAttribute(  5,(Attribute*)SenderIPversion);
      result.setAddr(newTuple);
      return YIELD;
    }

    case CLOSE:{
      if (local.addr != 0){
        li = (FTextValueMapReceiveTextStreamUDPLocalInfo*) local.addr;
        if (!li->my_socket.close()){
          cerr << "ERROR: " << li->my_socket.getErrorText() << "." << endl;
        }
        li->resultTupleType->DeleteIfAllowed();
        li->resultTupleType = 0;
        delete li;
        local.addr = 0;
      }
      return 0;
    }
  }
  /* should not happen */
  return -1;
}


ValueMapping FText_VMMap_MapReceiveTextStreamUDP[] =
{
  FTextValueMapReceiveTextStreamUDP<CcString,CcString>,  // 0
  FTextValueMapReceiveTextStreamUDP<CcString,FText   >,  // 1
  FTextValueMapReceiveTextStreamUDP<FText,   CcString>,  // 2
  FTextValueMapReceiveTextStreamUDP<FText,   FText   >   // 3
};

/*
Operator ~letObject~

*/
template <class T1, class T2>
int ftextletObjectVM( Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  result      = qp->ResultStorage( s );
  FText *Res  = reinterpret_cast<FText*>(result.addr);

  T1* CCObjName  = static_cast<T1*>(args[0].addr);
  T2* CCommand   = static_cast<T2*>(args[1].addr);
  CcBool* CcIsNL = static_cast<CcBool*>(args[2].addr);
  string querystring       = "";
  string querystringParsed = "";
  string typestring        = "";
  string errorstring       = "";
  string ObjNameString     = "";
  Word queryresultword;

  SecondoCatalog* ctlg = SecondoSystem::GetCatalog();

  SecParser mySecParser;
  ListExpr parsedCommand;

  // check definedness of all parameters
  if(    !CCObjName->IsDefined()
      || !CCommand->IsDefined()
      || !CcIsNL->IsDefined() ){
    Res->Set(false, "");
    return 0;
  }

  // check for a valid object name
  ObjNameString = CCObjName->GetValue();
  if( (ObjNameString == "") ){
    Res->Set(true, "ERROR: Empty object name.");
    return 0;
  } else if ( ctlg->IsSystemObject(ObjNameString) ) {
    Res->Set(true, "ERROR: Object name identifier is a reseverved identifier.");
    return 0;
  } else if (ctlg->IsObjectName(ObjNameString) ) {
    Res->Set(true, "ERROR: Object name identifier "
                      + ObjNameString + " is already used.");
    return 0;
  }

  // try to create the value from the valeNL/valueText
  querystring = CCommand->GetValue();

  if( !CcIsNL->GetBoolval() ){ // Command in SecondoExecutableLanguage
        // call Parser: add "query" and transform expression
        //              to nested-list-string
        if(mySecParser.Text2List( "query " + querystring,
                                             querystringParsed ) != 0)
        {
          Res->Set(true, "ERROR: Value text does not contain a "
              "parsable value expression.");
          return 0;
        }

  } else {// Command is already a nested-list-string: just copy
    querystringParsed = querystring;
  }
  // read nested list: transform nested-list-string to nested list
  if (!nl->ReadFromString(querystringParsed, parsedCommand) ) {
    Res->Set(true, "ERROR: Value text does not produce a "
              "valid nested list expression.");
    return 0;
  }
  if ( !CcIsNL->GetBoolval() ) {
    // remove the "query" from the list
    if ( (nl->ListLength(parsedCommand) == 2) ){
          parsedCommand = nl->Second(parsedCommand);
          //string parsedCommandstr;
          //nl->WriteToString(parsedCommandstr, parsedCommand);
          //cout << "NLimport: OK. parsedCommand=" << parsedCommandstr  << endl;
    } else {
      Res->Set(true, "ERROR: Value text does not produce a "
                     "valid nested list expression.");
      return 0;
    }
  }
  // evaluate command
  OpTree tree = 0;
  ListExpr resultType;
//   if( !SecondoSystem::BeginTransaction() ){
//     Res->Set(true, "ERROR: BeginTranscation failed!");
//     return 0;
//   };
  QueryProcessor *qpp = new QueryProcessor( nl, am );
  try{
    bool correct        = false;
    bool evaluable      = false;
    bool defined        = false;
    bool isFunction     = false;
    Word qresult;
    cerr << __PRETTY_FUNCTION__ << "Trying to build the operator tree" << endl;
    qpp->Construct( parsedCommand,
                   correct,
                   evaluable,
                   defined,
                   isFunction,
                   tree,
                   resultType );
    if ( !correct ){
      Res->Set(true, "ERROR: Value text yields a TYPEERROR.");
      // Do not need to destroy tree here!
      return 0;
    }
    typestring = nl->ToString(resultType);
    if ( evaluable || isFunction ){
      string typeName = "";
      ctlg->CreateObject(ObjNameString, typeName, resultType, 0);
    }
    if ( evaluable ){
      qpp->EvalS( tree, qresult, 1 );
      if( IsRootObject( tree ) && !IsConstantObject( tree ) ){
        ctlg->CloneObject( ObjNameString, qresult );
        qpp->Destroy( tree, true );
      } else {
        ctlg->UpdateObject( ObjNameString, qresult );
        qpp->Destroy( tree, false );
      }
    } else if ( isFunction ) { // abstraction or function object
      if ( nl->IsAtom( parsedCommand ) ) { // function object
        ListExpr functionList = ctlg->GetObjectValue(
                nl->SymbolValue( parsedCommand ) );
        ctlg->UpdateObject( ObjNameString, SetWord( functionList ) );
      } else {
        ctlg->UpdateObject( ObjNameString, SetWord( parsedCommand ) );
      }
      if( tree ) {
        qpp->Destroy( tree, true );
        tree = 0;
      }
    }
  } catch(SI_Error err) {
    if(tree) {
      qpp->Destroy( tree, true );
      tree = 0;
    }
    if( qpp ) {
      delete qpp;
      qpp = 0;
    }
    errorstring = "ERROR: " + SecondoInterface::GetErrorMessage(err);
//     if ( !SecondoSystem::AbortTransaction() ){
//       errorstring += ". AbortTransaction failed."
//     };
    Res->Set(true, errorstring);
    return 0;
  }
//   if( !SecondoSystem::CommitTransaction() ){
//     Res->Set(true, "ERROR: CommitTranscation failed!");
//     return 0;
//   };
  if( qpp ) {
    delete qpp;
    qpp = 0;
  }
  // Create object descriptor for the result FText
  string restring = "(OBJECT " + ObjNameString + " () (" + typestring + "))";
  Res->Set(true, restring);
  return 0;
}

// value mapping array
ValueMapping ftextletobject_vm[] = {
         ftextletObjectVM<CcString, CcString>,
         ftextletObjectVM<CcString, FText>,
         ftextletObjectVM<FText,    CcString>,
         ftextletObjectVM<FText,    FText> };

/*
Operator ~deleteObject~

*/

template<class T1>
int ftextdeleteObjectVM( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  result               = qp->ResultStorage( s );
  FText *Res           = reinterpret_cast<FText*>(result.addr);
  T1* CCObjName        = static_cast<T1*>(args[0].addr);
  SecondoCatalog* ctlg = SecondoSystem::GetCatalog();
  string objName       = CCObjName->GetValue();
  string typestring    = "";

  if ( !CCObjName->IsDefined() ){
    Res->Set(false, "");
    return 0;
  } else if ( (objName == "") ){
    Res->Set(true, "ERROR: Empty object name.");
    return 0;
  } else if ( ctlg->IsSystemObject(objName) ) {
      Res->Set(true, "ERROR: Cannot delete system object " + objName + ".");
      return 0;
  } else if ( !ctlg->IsObjectName(objName) ) {
    Res->Set(true, "ERROR: Object " + objName + " is unknown.");
    return 0;
  } else {
    ListExpr typeExpr = ctlg->GetObjectTypeExpr( objName );
    typestring = nl->ToString(typeExpr);
    if ( !ctlg->DeleteObject( objName ) ){
      Res->Set(true, "ERROR: Object " + objName + " is unknown.");
      return 0;
    } else {
      // also delete from derived objects table if necessary
      DerivedObj *derivedObjPtr = new DerivedObj();
      derivedObjPtr->deleteObj( objName );
      delete derivedObjPtr;
    }
  }
  Res->Set(true,"(OBJECT " + objName +" () (" + typestring + "))");
  return 0;
}

// value mapping array
ValueMapping ftextdeleteobject_vm[] = {
         ftextdeleteObjectVM<CcString>,
         ftextdeleteObjectVM<FText> };

// Selection function
int ftextdeleteobjectselect( ListExpr args )
{
  NList type(args);
  if( type.first()=="string" )
    return 0;
  if( type.first()=="text" )
    return 1;
  return -1;
}

/*
Operator ~createObject~

*/
template <class T1, class T2>
int ftextcreateObjectVM( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  result      = qp->ResultStorage( s );
  FText *Res  = reinterpret_cast<FText*>(result.addr);

  T1* CCObjName  = static_cast<T1*>(args[0].addr);
  T2* CCommand   = static_cast<T2*>(args[1].addr);
  CcBool* CcIsNL = static_cast<CcBool*>(args[2].addr);
  string querystring       = "";
  string querystringParsed = "";
  string typestring        = "";
  string errorstring       = "";
  string ObjNameString     = "";
  Word queryresultword;

  SecondoCatalog* ctlg = SecondoSystem::GetCatalog();

  SecParser mySecParser;
  ListExpr parsedCommand;

  // check definedness of all parameters
  if(    !CCObjName->IsDefined()
      || !CCommand->IsDefined()
      || !CcIsNL->IsDefined() ){
    Res->Set(false, "");
    return 0;
  }

  // check for a valid object name
  ObjNameString = CCObjName->GetValue();
  if( (ObjNameString == "") ){
    Res->Set(true, "ERROR: Empty object name.");
    return 0;
  } else if ( ctlg->IsSystemObject(ObjNameString) ) {
    Res->Set(true, "ERROR: Object name identifier is a reseverved identifier.");
    return 0;
  } else if (ctlg->IsObjectName(ObjNameString) ) {
    Res->Set(true, "ERROR: Object name identifier "
                      + ObjNameString + " is already used.");
    return 0;
  }

  // try to create the value from the valueNL/valueText
  querystring = CCommand->GetValue();

  if( !CcIsNL->GetBoolval() ){ // Command in SecondoExecutableLanguage
        // call Parser: add "query" and transform expression
        //              to nested-list-string
        if(mySecParser.Text2List( "query " + querystring,
                                             querystringParsed ) != 0)
        {
          Res->Set(true, "ERROR: Type text does not contain a "
              "parsable value expression.");
          return 0;
        }

  } else {// Command is already a nested-list-string: just copy
    querystringParsed = querystring;
  }
  // read nested list: transform nested-list-string to nested list
  if (!nl->ReadFromString(querystringParsed, parsedCommand) ) {
    Res->Set(true, "ERROR: Type text does not produce a "
              "valid nested list expression.");
    return 0;
  }
  if ( !CcIsNL->GetBoolval() ) {
    // remove the "query" from the list
    if ( (nl->ListLength(parsedCommand) == 2) ){
          parsedCommand = nl->Second(parsedCommand);
          //string parsedCommandstr;
          //nl->WriteToString(parsedCommandstr, parsedCommand);
          //cout << "NLimport: OK. parsedCommand=" << parsedCommandstr  << endl;
    } else {
      Res->Set(true, "ERROR: Type text does not produce a "
                     "valid nested list expression.");
      return 0;
    }
  }
  ListExpr typeExpr2 = ctlg->ExpandedType( parsedCommand );
  ListExpr errorList;
  string userDefTypeName = "";
  string typeExprString = nl->ToString(typeExpr2);
  if ( ctlg->KindCorrect( typeExpr2, errorList ) ) {
    if (    nl->IsAtom( parsedCommand )
         && ((nl->AtomType( parsedCommand ) == SymbolType))
       ) {
      userDefTypeName = nl->SymbolValue( parsedCommand );
      if ( !ctlg->MemberType( userDefTypeName ) ) { // not a user-defined type
          userDefTypeName = "";
      }
    }
    if ( !ctlg->CreateObject( ObjNameString, userDefTypeName, typeExpr2, 0 ) ){
      Res->Set(true, "ERROR: Object name identifier "
                        + ObjNameString + " is already used.");
      return 0;
    }
  } else { // Wrong type expression
      Res->Set(true, "ERROR: Invalid type expression.");
      return 0;
  }
  Res->Set(true, "(OBJECT " + ObjNameString +" () (" + typeExprString + "))");
  return -1;
}

// value mapping array
ValueMapping ftextcreateobject_vm[] = {
         ftextcreateObjectVM<CcString, CcString>,
         ftextcreateObjectVM<CcString, FText>,
         ftextcreateObjectVM<FText,    CcString>,
         ftextcreateObjectVM<FText,    FText>
};

/*
Operator ~getObjectTypeNL~

*/
template <class T1>
int ftextgetObjectTypeNL_VM( Word* args, Word& result, int message,
                                     Word& local, Supplier s )
{
  result      = qp->ResultStorage( s );
  FText *Res  = reinterpret_cast<FText*>(result.addr);

  T1* CCObjName  = static_cast<T1*>(args[0].addr);
  string ObjNameString     = "";
  string typestring        = "";
  SecondoCatalog* ctlg = SecondoSystem::GetCatalog();

  // check definedness of the parameters
  if( !CCObjName->IsDefined() ){
    Res->Set(false, "");
    return 0;
  }
  // check for a valid object name
  ObjNameString = CCObjName->GetValue();
  if( (ObjNameString == "") ){
    Res->Set(false, "");
    return 0;
  } else if ( !ctlg->IsObjectName(ObjNameString) ) {
    Res->Set(false, "");
    return 0;
  }
  // get the type expression
  typestring =
            nl->ToString(ctlg->GetObjectTypeExpr( ObjNameString ));
  // set result
  Res->Set(true, typestring);
  return 0;
}

// value mapping array
ValueMapping ftextgetObjectTypeNL_vm[] = {
         ftextgetObjectTypeNL_VM<CcString>,
         ftextgetObjectTypeNL_VM<FText>
};

/*
Operator ~getObjectValueNL~

*/
template <class T1>
int ftextgetObjectValueNL_VM( Word* args, Word& result, int message,
                              Word& local, Supplier s )
{
  result      = qp->ResultStorage( s );
  FText *Res  = reinterpret_cast<FText*>(result.addr);

  T1* CCObjName  = static_cast<T1*>(args[0].addr);
  string ObjNameString     = "";
  string valuestring       = "";
  SecondoCatalog* ctlg = SecondoSystem::GetCatalog();

  // check definedness of the parameters
  if( !CCObjName->IsDefined() ){
    Res->Set(false, "");
    return 0;
  }
  // check for a valid object name
  ObjNameString = CCObjName->GetValue();
  if( (ObjNameString == "") ){
    Res->Set(false, "");
    return 0;
  } else if ( !ctlg->IsObjectName(ObjNameString) ) {
    Res->Set(false, "");
    return 0;
  }
  // get the value expression
  valuestring = nl->ToString(ctlg->GetObjectValue( ObjNameString ));
  // set result
  Res->Set(true, valuestring);
  return 0;
}

// value mapping array
ValueMapping ftextgetObjectValueNL_vm[] = {
         ftextgetObjectValueNL_VM<CcString>,
         ftextgetObjectValueNL_VM<FText>
};

/*
Operator ~getDatabaseName~

*/

int getDatabaseName_VM( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result         = qp->ResultStorage( s );
  CcString *Res  = reinterpret_cast<CcString*>(result.addr);
  string dbname  = SecondoSystem::GetInstance()->GetDatabaseName();
  Res->Set(true, dbname);
  return 0;
}

class matchingOpsLocalInfo{
 public:
    matchingOpsLocalInfo(ListExpr argList, 
                         ListExpr attrList):pos(0),ops() {
       tupleType = new TupleType(attrList);
       ops = am->matchingOperators(argList);
    }

    matchingOpsLocalInfo(ListExpr argList):pos(0),ops(),tupleType(0) {
       ops = am->matchingOperators(argList);
    }

    CcString* nextName(){
       if(pos<ops.size()){
          CcString* r = new CcString(ops[pos].first.second->GetName());
          pos++;
          return r;
       }
       return 0;
    }

    Tuple* nextTuple(){

        if(pos>=ops.size()){
          return 0;
        } 
        pair< pair<int,Operator*>, ListExpr> op = ops[pos];
        pos++;
        Tuple* res = new Tuple(tupleType);
        // opName
        res->PutAttribute(0, new CcString(op.first.second->GetName())); 
        // algName
        res->PutAttribute(1, new CcString(am->GetAlgebraName(op.first.first)));
        // resType
        ListExpr resultList = op.second;
        if(nl->HasLength(resultList,3) &&
           listutils::isSymbol(nl->First(resultList),"APPEND")){
           resultList = nl->Third(resultList);
        }
        res->PutAttribute(2, new FText(true,nl->ToString(resultList)));
       

         OperatorInfo oi = op.first.second->GetOpInfo();
        // signature
        res->PutAttribute(3, new FText(true,oi.signature));
        // syntax
        res->PutAttribute(4, new FText(true,oi.syntax));

        // meaning
        res->PutAttribute(5, new FText(true,oi.meaning));
 
        // Example
        res->PutAttribute(6, new FText(true,oi.example));

        // Remark
        res->PutAttribute(7, new FText(true,oi.remark));
        return  res;
    }

    ~matchingOpsLocalInfo(){
      if(tupleType){
         tupleType->DeleteIfAllowed();
         tupleType = 0;
      }
      
    }



 private:
   size_t pos;
   vector< pair< pair<int,Operator*>, ListExpr> > ops;
   TupleType* tupleType;
};


template<bool onlyNames>
int matchingOperatorsVM( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
   switch(message){
     case OPEN: {
       if(local.addr){
         delete (matchingOpsLocalInfo*) local.addr;
         local.addr = 0;
       }
       int noSons = qp->GetNoSons(s);
       FText* t = (FText*) args[noSons-1].addr;
       ListExpr argList;
       nl->ReadFromString(t->GetValue(),argList);
       if(onlyNames){
          local.addr= new matchingOpsLocalInfo(argList);
       } else { 
          local.addr= new matchingOpsLocalInfo(argList,
                               nl->Second(GetTupleResultType(s)));
       }
       return 0;
    }
    case REQUEST:{
       if(!local.addr){
          return CANCEL;
       }
       if(onlyNames){
          result.addr = ((matchingOpsLocalInfo*) local.addr)->nextName();
       } else {
          result.addr = ((matchingOpsLocalInfo*) local.addr)->nextTuple();
       }
       return result.addr?YIELD:CANCEL;
    }
    case CLOSE: {
       if(local.addr){
         delete (matchingOpsLocalInfo*) local.addr;
         local.addr = 0;
       }
       return 0;
    }

  }
  return -1;

}




/*
3.4 Definition of Operators

*/

/*
Used to explain signature, syntax and meaning of the operators of the type ~text~.

*/

const string containsSpec =
  "( (\"Signature\" \"Syntax\" \"Meaning\" )"
    "("
    "<text>({text | string)} x {text |string) -> bool</text--->"
    "<text>_a_ contains _b_</text--->"
    "<text>Returns whether _a_ contains pattern _g_.</text--->"
    ")"
  ")";


const string lengthSpec =
  "( (\"Signature\" \"Syntax\" \"Meaning\" )"
    "("
    "<text>("+typeName+" ) -> int</text--->"
    "<text>length ( _ )</text--->"
    "<text>length returns the length of "+typeName+".</text--->"
    ")"
  ")";

const string keywordsSpec  = "( ( \"Signature\" \"Syntax\" \"Meaning\" "
                            "\"Example\" )"
                             "( <text>(text) -> (stream string)</text--->"
             "<text>_ keywords</text--->"
             "<text>Creates a stream of strings containing the single words"
             " of the origin text, on the assumption, that words in the text"
             " are separated by a space character.</text--->"
             "<text>let Keyword = documents feed "
             "extendstream[kword: .title keywords] consume</text--->"
             ") )";

const string sentencesSpec  = "( ( \"Signature\" \"Syntax\" \"Meaning\" "
                            "\"Example\" )"
                             "( <text>(text) -> (stream text)</text--->"
             "<text>_ sentences</text--->"
             "<text>Creates a stream of standardized texts containing "
             "complete sentences"
             " of the origin text, on the assumption, that sentences "
             "in the text"
             " are terminated by a ., ! or ? character.</text--->"
             "<text>let MySentences = documents feed "
             "projectextendstream[title; newattr: .content sentences] "
             "consume</text--->"
             ") )";

const string diceSpec  =
            "( ( \"Signature\" \"Syntax\" \"Meaning\" "
                 "\"Example\" )"
            "( <text> int x text x text -> real </text--->"
             "<text> dice( _ _ _)</text--->"
             "<text>Computes the dice coefficient between the text using"
             " n-grams where n is speciefied by the first argument."
             " Can be used to compare texts .</text--->"
             "<text> dice(3  text1  text2) "
             "</text--->"
             ") )";

const string getCatalogSpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text> -> stream(tuple((ObjectName string)(Type string)"
    "(TypeExpr ftext)))</text--->"
    "<text>getCatalog( )</text--->"
    "<text>Returns all object descriptions from the catalog of the currently "
    "opened database as a tuple stream.</text--->"
    "<text>query getCatalog( ) consume</text--->"
    ") )";

const string substrSpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text> text x int x int -> string</text--->"
    "<text>substr( s, b, e )</text--->"
    "<text>Returns a substring of a text value, beginning at position 'b' "
    "and ending at position 'e', where the first character's position is 1. "
    "if (e - b)>48, the result will be truncated to its starting 48 "
    "characters.</text--->"
    "<text>query substr('Hello world!', 1, 3 ) consume</text--->"
    ") )";

const string subtextSpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text> text x int x int -> text</text--->"
    "<text>subtext( s, b, e )</text--->"
    "<text>Returns a subtext of a text value, beginning at position 'b' "
    "and ending at position 'e', where the first character's position is 1. "
    "</text--->"
    "<text>query subtext('Hello world!', 1, 3 )</text--->"
    ") )";

const string FTextfindSpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text> {text | string} x {text | string}  -> stream(int)</text--->"
    "<text>find( s, p )</text--->"
    "<text>Returns a stream of integers giving the starting positions of all "
    "occurances of a pattern 'p' within a string or text 's'. The position of "
    "the first character in 's' is 1. For any malformed parameter combination, "
    "the result is an empty stream.</text--->"
    "<text>query find('Hello world!', 'l') count</text--->"
    ") )";

const string FTextisemptySpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text> {text | string} x {text | string}  -> stream(int)</text--->"
    "<text>isempty( t )</text--->"
    "<text>Returns TRUE, if text 't' is either undefined or empty.</text--->"
    "<text>query isempty('')</text--->"
    ") )";

const string FTexttrimSpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text> text -> text </text--->"
    "<text>trim( t )</text--->"
    "<text>Removes whitespaces at the start and and end of"
    " the argument</text--->"
    "<text>query trim('  hello  you    ')</text--->"
    ") )";

const string FTextplusSpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text> {text | string} x text -> text \n"
    " text x {text | string -> text}</text--->"
    "<text>t1 + t2</text--->"
    "<text>Returns the concatenation of a combination of text with another "
    "text or string value.</text--->"
    "<text>query ('Hello' + \" world\" + '!')</text--->"
    ") )";

// Compare predicates
const string FTextLessSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>text          x {text|string} -> bool\n"
    "{text|string} x text          -> bool</text--->"
    "<text>_ < _</text--->"
    "<text>Lexicographical ordering predicate 'less than'.</text--->"
    "<text>query 'TestA' < 'TestB'</text--->"
    ") )";

const string FTextLessEqSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>text          x {text|string} -> bool\n"
    "{text|string} x text          -> bool</text--->"
    "<text>_ <= _</text--->"
    "<text>Lexicographical ordering predicate 'less than or equal'.</text--->"
    "<text>query 'TestA' <= 'TestB'</text--->"
    ") )";

const string FTextEqSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>text          x {text|string} -> bool\n"
    "{text|string} x text          -> bool</text--->"
    "<text>_ = _</text--->"
    "<text>Lexicographical ordering predicate 'equals'.</text--->"
    "<text>query 'TestA' = 'TestB'</text--->"
    ") )";

const string FTextBiggerEqSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>text          x {text|string} -> bool\n"
    "{text|string} x text          -> bool</text--->"
    "<text>_ >= _</text--->"
    "<text>Lexicographical ordering predicate 'greater than or equal'."
    "</text--->"
    "<text>query 'TestA' >= 'TestB'</text--->"
    ") )";

const string FTextBiggerSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>text          x {text|string} -> bool\n"
    "{text|string} x text          -> bool</text--->"
    "<text>_ > _</text--->"
    "<text>Lexicographical ordering predicate 'greater than'.</text--->"
    "<text>query 'TestA' > 'TestB'</text--->"
    ") )";

const string FTextNeqSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>text          x {text|string} -> bool\n"
    "{text|string} x text          -> bool</text--->"
    "<text>_ # _</text--->"
    "<text>Lexicographical ordering predicate 'nonequal to'.</text--->"
    "<text>query 'TestA' # 'TestB'</text--->"
    ") )";

const string FTextEvaluateSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>text [ x bool ] -> stream(tuple((CmdStr text) (Success bool) "
    "(Evaluable bool) (Defined bool) (IsFunction bool)"
    "(ResultType text) (Result text) (ErrorMessage text) "
    "(ElapsedTimeReal real) (ElapsedTimeCPU real)))</text--->"
    "<text>evaluate( query , isNL )</text--->"
    "<text>Interprets the text argument 'query' as a Secondo Executable "
    "Language query expression and evaluates it. The calculated result "
    "returned as a nested list expression. Operator's result is a stream "
    "containing at most 1 tuple with a copy of the command, the result, "
    "errormessage, runtimes, and some more status information. If the optional "
    "second argument 'isNL' (default = FALSE) is set to TRUE, 'query' is "
    "expected to be in nested list format. </text--->"
    "<text>query evaluate('ten feed filter[.no > 5] count') "
    "filter[.Success] count > 1</text--->"
    ") )";

const string FTextReplaceSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>{text|string} x {text|string} x {text|string} -> text\n"
    "{text|string} x int    x int  x {text|string} -> text</text--->"
    "<text>replace( text , Pold, Pnew )\n replace( text, start, end, Pnew)"
    "</text--->"
    "<text>Within 'text', replace all occurencies of pattern 'Pold' by "
    "pattern 'Pnew'. Within 'text' replace characters starting at "
    "position 'start' and ending at position 'end' by pattern 'Pnew')</text--->"
    "<text>query replace('Fish! Fresh fish! Fresh fished fish!',"
    "'Fresh','Rotten')</text--->"
    ") )";

const string FTextIsDBObjectSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>string -> bool</text--->"
    "<text>isDBObject( Name )</text--->"
    "<text>Returns true iff Name is the name of a database object.</text--->"
    "<text>query isDBObject(\"blubb\")</text--->"
    ") )";

const string FTextGetTypeNLSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>Expression -> text</text--->"
    "<text>_ getTypeNL</text--->"
    "<text>Retrieves the argument's type as a nested list expression.</text--->"
    "<text>query int getTypeNL</text--->"
    ") )";

const string FTextGetValueNLSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>stream(DATA)   -> stream(text)\n"
    "stream(tuple(X)) -> stream(text)\n"
    "Expression       -> text</text--->"
    "<text>_ getValueNL</text--->"
    "<text>Returns the argument's nested list value expression as a text "
    "value. If the argument is a stream, a stream of text values with the "
    "textual representations for each stream element is produced.</text--->"
    "<text>query ten feed getValueNL</text--->"
    ") )";

const string FTextToObjectSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>{text|string} x T -> T, T in DATA>/text--->"
    "<text>toobject( list, type )</text--->"
    "<text>Creates an object of type T from a nested list expression 'list'. "
    "Argument 'type' is only needed for the type mapping, its value will be "
    "ignored. 'list' is a textual nested list value expression. If the value "
    "expression does not match the type of 'type', the result will be an "
    "undefined object of that type.</text--->"
    "<text>query toobject('3.141592653589793116', 1.0)</text--->"
    ") )";

const string FTextChartextSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>int -> text</text--->"
    "<text>chartext( code )</text--->"
    "<text>Creates atext of length 1 containing the character specified by "
    "ASCII symbol code 'code'.</text--->"
    "<text>query chartext(13)"
    "</text--->"
    ") )";

const string FTextToLowerSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>text -> text</text--->"
    "<text>tolower( text )</text--->"
    "<text>Creates a copy of 'text', where upper case characters are replaced "
    "by lower case characters.</text--->"
    "<text>query tolower('Hello World!')"
    "</text--->"
    ") )";

const string FTextToUpperSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>text -> text</text--->"
    "<text>toupper( text )</text--->"
    "<text>Creates a copy of 'text', where lower case characters are replaced "
    "by upper case characters.</text--->"
    "<text>query toupper('Hello World!')"
    "</text--->"
    ") )";

const string FTextTextToStringSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>text -> string</text--->"
    "<text>tostring( text )</text--->"
    "<text>Converts 'text' to a string value. One the first 48 characters are "
    "copied. Any contained doublequote (\") is replaced by a single quote ('). "
    "</text--->"
    "<text>query tostring('Hello World!')"
    "</text--->"
    ") )";

const string FTextStringToTextSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>string -> text</text--->"
    "<text>totext( string )</text--->"
    "<text>Converts 'string' to a text value.</text--->"
    "<text>query totext(\"Hello World!\")"
    "</text--->"
    ") )";

const string FTextSendTextUDPSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>{string|text}^n -> text, 3<=n<=5</text--->"
    "<text>sendtextUDP( message, remoteIP, remotePort [, myIP [, myPort] ] )"
    "</text--->"
    "<text>Sends 'message' to 'remotePort' to host 'remoteIP' using 'myPort' "
    "on 'myIP' as the sender address/port and returns a text with a send "
    "status report. If optional parameters are omitted or are empty, standard "
    "parameters will be used automatically. DNS is used to lookup for host"
    "names. Used the UDP connection-less protocol.</text--->"
    "<text>query sendtextUDP(\"Hello World!\", '127.0.0.0', '2626')</text--->"
    ") )";

const string FTextReceiveTextUDPSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>{string|text} x {string|text} x real -> stream(tuple("
    "(Ok bool)(Msg text)(ErrMsg string)(SenderIP string)(SenderPort string)"
    "(SenderIPversion string)))</text--->"
    "<text>receivetextUDP( myIP, myPort, timeout )"
    "</text--->"
    "<text>Tries to receive a UDP-message to 'myPort' under local address "
    "'myIP' for a duration up to 'timeout' seconds. Parameter 'myIP' "
    "is looked up automatically, being an empty string/text. DNS is used to "
    "lookup for host names. Negative 'timeout' values will deactive the "
    "timeout, possibly waiting forever. The result is a stream with a single "
    "tuple containing an OK-flag, the message, an error message, the "
    "sender's IP and port, and its IP-version.</text--->"
    "<text>query receivetextUDP(\"\",'2626',0.01) tconsume</text--->"
    ") )";

const string FTextReceiveTextStreamUDPSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>{string|text} x {string|text} x real x real -> stream(tuple("
    "(Ok bool)(Msg text)(ErrMsg string)(SenderIP string)(SenderPort string)"
    "(SenderIPversion string)))</text--->"
    "<text>receivetextstreamUDP( myIP, myPort, localTimeout, globalTimeout )"
    "</text--->"
    "<text>Tries to receive a series of UDP-messages to 'myPort' under local "
    "address 'myIP' for a duration up to 'localTimeout' seconds each. "
    "Terminates after a total time of 'globalTimeout'. Parameter 'myIP' "
    "is looked up automatically, being an empty string/text. DNS is used to "
    "lookup for host names. Negative 'timeout' values will deactive the "
    "timeout, possibly waiting forever. The result is a stream with a single "
    "tuple containing an OK-flag, the message, an error message, the "
    "sender's IP and port, and its IP-version. Can also be used to create a "
    "sequence of 'events' with a selected frequecy. You can use the 'cancel' "
    "operator to wait for 'magic messages' terminating the stream.</text--->"
    "<text>query receivetextstreamUDP(\"\",'2626',1.0, 10.0) tconsume</text--->"
    ") )";



const string svg2textSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>svg -> text</text--->"
    "<text>svg2text( svg )</text--->"
    "<text>Converts 'svg' to a text value.</text--->"
    "<text>query svg2text(...)"
    "</text--->"
    ") )";

const string text2svgSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>text -> svg</text--->"
    "<text>text2svg( text )</text--->"
    "<text>Converts 'text' to an svg  value.</text--->"
    "<text>query text2svg(...)"
    "</text--->"
    ") )";


const string cryptSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> t1 [x t2] -> string, t1,t2 in {string, text} </text--->"
    "<text>crypt( word [, salt] )</text--->"
    "<text>encrypt word using the DES crypt</text--->"
    "<text>query crypt(\"TopSecret\",\"TS\")"
    "</text--->"
    ") )";

const string checkpwSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> t1 x t2 -> bool, t1,t2 in {string, text} </text--->"
    "<text>checkpw( plain, encrypted )</text--->"
    "<text>checks whether encrypted is an encrypted version of "
    " plain using the crypt function </text--->"
    "<text>query checkpw(\"Secondo\",crypt(\"Secondo\"))"
    "</text--->"
    ") )";

const string md5Spec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {string, text} [x {string, text}]  -> string </text--->"
    "<text>md5( word [, salt]  )</text--->"
    "<text>encrypt word using the md5 encryption</text--->"
    "<text>query md5(\"TopSecret\")"
    "</text--->"
    ") )";

const string blowfish_encodeSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {string, text} x {string, text}  -> text </text--->"
    "<text>blowfish_encode( password, text )</text--->"
    "<text>encrypt text using the blowfish encryption</text--->"
    "<text>query blowfish_encode(\"TopSecret\",\"Secondo\")"
    "</text--->"
    ") )";

const string blowfish_decodeSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {string, text} x {string, text}  -> text </text--->"
    "<text>blowfish_decode( password, hex )</text--->"
    "<text>decrypt hex using the blowfish decryption</text--->"
    "<text>query blowfish_decode(\"TopSecret\",\"f27d7581d1aaaff\")"
    "</text--->"
    ") )";

const string ftextletObjectSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {string|text} x {string|text} x bool  -> text </text--->"
    "<text>letObject( objectName, valueNL, isNL )</text--->"
    "<text>Create a database object as a side effect. The object's name is "
    "objectName, and it is set to value valueNL. Parameter isNL indicates "
    "whether valueNL is given in NL syntax (TRUE) or text syntax (FALSE). "
    "The operator acts like the 'let object' command. The return value is a "
    "object descriptor (OBJECT () (<typeNL>) (<valueNL>)) is assignment was "
    "successful, an error message otherwise.</text--->"
    "<text>query letObject(\"MyThreeInt\",'3', FALSE)"
    "</text--->"
    ") )";

const string ftextdeleteObjectSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {string|text} -> text </text--->"
    "<text>deleteObject( objectName )</text--->"
    "<text>Deletes a database object as a side effect. The object is named "
    "objectName. You should NEVER try to delete an object, that is used by the "
    "query trying to delete it. If deletion fails, an error message is "
    "returned, otherwise a descriptor (OBJECT () (<typeNL>) (<valueNL>)) for "
    "the deleted object is returned.</text--->"
    "<text>query deleteObject(\"MyThreeInt\")"
    "</text--->"
    ") )";

const string ftextcreateObjectSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {string|text} x {string|text} x bool -> text </text--->"
    "<text>createObject( objectName, typeexpr, isNL )</text--->"
    "<text>Creates a database object as a side effect. The object is named "
    "objectName. It's type is given by typeexpr. The boolean parameter must be "
    "set to TRU, iff the typeexpr is given in NL-syntax, and FALSE when in "
    "text sxntax. If creation fails, an error message is returned, otherwise a "
    "descriptor (OBJECT () (<typeNL>) (<valueNL>)) for the created "
    "object.</text--->"
    "<text>query createObject(\"MyThreeInt\", 'int', TRUE)"
    "</text--->"
    ") )";

const string ftextgetObjectTypeNLSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {string|text} -> text </text--->"
    "<text>getObjectTypeNL( objectName )</text--->"
    "<text>Returns a text with the NL type expression associated with the "
    "database object with the name given as a text or string argument. "
    "If the according object does not exist, the result is undefined.</text--->"
    "<text>query 'getObjectTypeNL(\"MyThreeInt\")"
    "</text--->"
    ") )";

const string ftextgetObjectValueNLSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {string|text} -> text </text--->"
    "<text>getObjectValueNL( objectName )</text--->"
    "<text>Returns a text with the NL value expression associated with the "
    "database object with the name given as a text or string argument. "
    "If the according object does not exist, the result is undefined.</text--->"
    "<text>query 'getObjectValueNL(\"MyThreeInt\")"
    "</text--->"
    ") )";

const string getDatabaseNameSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>-> string </text--->"
    "<text>getDatabaseName()</text--->"
    "<text>Returns a string with the name of the current database.</text--->"
    "<text>query getDatabaseName()</text--->"
    ") )";


const string matchingOperatorNamesSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>  ANY -> stream(string) </text--->"
    "<text> matchingOperatorNames(arg1, arg2, ...) </text--->"
    "<text>Returns the names of operators which could be "
           "applied to the types coming from "
    "  evaluation of the arguments  </text--->"
    "<text>query matchingOperatorNamess(Trains) count</text--->"
    ") )";



const string matchingOperatorsSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>  ANY -> stream(tuple(( \n"
    "          OperatorName: string,\n" 
    "          AlgebraName : string,\n"
    "           ResultType : text,\n"
    "           Signature  : text,\n"
    "               Syntax : text,\n"
    "              Meaning : text,\n"
    "              Example : text,\n"
    "               Remark : text)))\n"
    " </text--->"
    "<text> query matchingOperators(arg1, arg2, ...) </text--->"
    "<text>Returns the operators which could be applied to the"
    " types coming from "
    "  evaluation of the arguments  </text--->"
    "<text>query matchingOperators(Trains) tconsume</text--->"
    ") )";

/*
Operator Definitions

*/
Operator contains
(
  "contains",           //name
  containsSpec,         //specification
  4,                    //no. of value mappings
  FText_VMMap_Contains, //value mapping
  SelectFun_TextString_TextString,   //selection function
  TypeMap_TextString_TextString__Bool   //type mapping
);

Operator length
(
  "length",             //name
  lengthSpec,           //specification
  ValMapTextInt,        //value mapping
  Operator::SimpleSelect,         //trivial selection function
  TypeMap_Text__Int        //type mapping
);

Operator getkeywords
(
  "keywords",            //name
  keywordsSpec,          //specification
  ValMapkeywords,        //value mapping
  Operator::SimpleSelect,          //trivial selection function
  TypeMap_text__stringstream        //type mapping
);

Operator getsentences
(
  "sentences",            //name
  sentencesSpec,          //specification
  ValMapsentences,        //value mapping
  Operator::SimpleSelect,           //trivial selection function
  TypeMap_text__textstream        //type mapping
);

Operator diceCoeff(
   "dice",
   diceSpec,
   ValMapDice_t_t,
   Operator::SimpleSelect,
   TypeMap_int_text_text__real
);

Operator ftextgetcatalog
(
    "getcatalog",
    getCatalogSpec,
    ValMapGetCatalog,
    Operator::SimpleSelect,
    TypeGetCatalog
);

Operator ftextsubstr
(
    "substr",
    substrSpec,
    ValMapSubstr,
    Operator::SimpleSelect,
    TypeFTextSubstr
);

Operator ftextsubtext
(
    "subtext",
    subtextSpec,
    ValMapSubtext,
    Operator::SimpleSelect,
    TypeFTextSubtext
);

Operator ftextfind
    (
    "find",
    FTextfindSpec,
    4,
    FText_VMMap_Find,
    SelectFun_TextString_TextString,
    TypeMap_textstring_textstring__intstream
    );

Operator ftextisempty
    (
    "isempty",
    FTextisemptySpec,
    FTextValMapIsEmpty,
    Operator::SimpleSelect,
    TypeMap_text__bool
    );

Operator ftexttrim
    (
    "trim",
    FTexttrimSpec,
    FTextValMapTrim,
    Operator::SimpleSelect,
    TypeMap_text__text
    );

Operator ftextplus
    (
    "+",
    FTextplusSpec,
    3,
    FText_VMMap_Plus,
    FTextSelectFunPlus,
    FTextTypeMapPlus
    );

Operator ftextless
    (
    "<",
    FTextLessSpec,
    3,
    FText_VMMap_Less,
    FTextSelectFunComparePred,
    FTextTypeMapComparePred
    );

Operator ftextlesseq
    (
    "<=",
    FTextLessEqSpec,
    3,
    FText_VMMap_LessEq,
    FTextSelectFunComparePred,
    FTextTypeMapComparePred
    );

Operator ftexteq
    (
    "=",
    FTextEqSpec,
    3,
    FText_VMMap_Eq,
    FTextSelectFunComparePred,
    FTextTypeMapComparePred
    );

Operator ftextbiggereq
    (
    ">=",
    FTextBiggerEqSpec,
    3,
    FText_VMMap_BiggerEq,
    FTextSelectFunComparePred,
    FTextTypeMapComparePred
    );

Operator ftextbigger
    (
    ">",
    FTextBiggerSpec,
    3,
    FText_VMMap_Bigger,
    FTextSelectFunComparePred,
    FTextTypeMapComparePred
    );

Operator ftextneq
    (
    "#",
    FTextNeqSpec,
    3,
    FText_VMMap_Neq,
    FTextSelectFunComparePred,
    FTextTypeMapComparePred
    );

Operator ftextevaluate
    (
    "evaluate",
    FTextEvaluateSpec,
    FTextValueMapEvaluate,
    Operator::SimpleSelect,
    FTextTypeMapEvaluate
    );

Operator ftextreplace
    (
    "replace",
    FTextReplaceSpec,
    12,
    FText_VMMap_Replace,
    FTextSelectFunReplace,
    FTextTypeReplace
    );

Operator isDBObject
    (
    "isDBObject",
    FTextIsDBObjectSpec,
    FTextValueMapIsDBObject,
    Operator::SimpleSelect,
    TypeMap_string__bool
    );

Operator getTypeNL
    (
    "getTypeNL",
    FTextGetTypeNLSpec,
    FTextValueMapGetTypeNL,
    Operator::SimpleSelect,
    FTextTypeMapExpression2Text
    );

Operator getValueNL
    (
    "getValueNL",
    FTextGetValueNLSpec,
    3,
    FText_VMMap_GetValueNL,
    FTextSelectFunGetValueNL,
    FTextTypeMapGetValueNL
    );

Operator ftexttoobject
    (
    "toobject",
    FTextToObjectSpec,
    2,
    FText_VMMap_ToObject,
    FTextSelectFunToObject,
    FTextTypeTextData_Data
    );

Operator chartext
    (
    "chartext",
    FTextChartextSpec,
    FTextValueMapChartext,
    Operator::SimpleSelect,
    TypeMap_int__text
    );

Operator ftexttoupper
    (
    "toupper",
    FTextToUpperSpec,
    FTextValueMapChangeCase<false>,
    Operator::SimpleSelect,
    TypeMap_text__text
    );

Operator ftexttolower
    (
    "tolower",
    FTextToLowerSpec,
    FTextValueMapChangeCase<true>,
    Operator::SimpleSelect,
    TypeMap_text__text
    );

Operator ftexttostring
    (
    "tostring",
    FTextTextToStringSpec,
    FTextValueMapConvert<true, FText, CcString>,
    Operator::SimpleSelect,
    TypeMap_text__string
    );

Operator ftexttotext
    (
    "totext",
    FTextStringToTextSpec,
    FTextValueMapConvert<false, CcString, FText>,
    Operator::SimpleSelect,
    TypeMap_string__text
    );

Operator ftsendtextUDP
    (
    "sendtextUDP",
    FTextSendTextUDPSpec,
    32,
    FText_VMMap_MapSendTextUDP,
    FTextSelectSendTextUDP,
    FTextTypeSendTextUDP
    );

Operator ftreceivetextUDP
    (
    "receivetextUDP",
    FTextReceiveTextUDPSpec,
    4,
    FText_VMMap_MapReceiveTextUDP,
    FTextSelectReceiveTextUDP,
    FTextTypeReceiveTextUDP
    );

Operator ftreceivetextstreamUDP
    (
    "receivetextstreamUDP",
    FTextReceiveTextStreamUDPSpec,
    4,
    FText_VMMap_MapReceiveTextStreamUDP,
    FTextSelectReceiveTextUDP,
    FTextTypeReceiveTextStreamUDP
    );

Operator svg2text
(
  "svg2text",           //name
  svg2textSpec,   //specification
  SVG2TEXTVM, //value mapping
  Operator::SimpleSelect,         //trivial selection function
  TypeMap_svg__text //type mapping
);

Operator text2svg
(
  "text2svg",           //name
  text2svgSpec,   //specification
  SVG2TEXTVM, //value mapping
  Operator::SimpleSelect,         //trivial selection function
  TypeMap_text__svg //type mapping
);

Operator crypt
    (
    "crypt",
    cryptSpec,
    6,
    cryptvm,
    cryptSelect,
    cryptTM
    );

Operator checkpw
    (
    "checkpw",
    checkpwSpec,
    4,
    checkpwvm,
    checkpwSelect,
    checkpwTM
    );

Operator md5
    (
    "md5",
    md5Spec,
    6,
    md5vm,
    md5Select,
    md5TM
    );

Operator blowfish_encode ( "blowfish_encode", blowfish_encodeSpec,
                           4, blowfish_encodevm, blowfish_encodeSelect,
                           blowfish_encodeTM
    );
Operator blowfish_decode ( "blowfish_decode", blowfish_decodeSpec,
                           4, blowfish_decodevm, blowfish_decodeSelect,
                           blowfish_decodeTM
    );

Operator ftextletObject ( "letObject", ftextletObjectSpec,
                           4, ftextletobject_vm, blowfish_encodeSelect,
                           StringtypeStringtypeBool2TextTM
    );

Operator ftextdeleteObject ( "deleteObject", ftextdeleteObjectSpec,
                           2, ftextdeleteobject_vm, ftextdeleteobjectselect,
                           TypeMap_textstring__text
    );

Operator ftextcreateObject ( "createObject", ftextcreateObjectSpec,
                           4, ftextcreateobject_vm, blowfish_encodeSelect,
                           StringtypeStringtypeBool2TextTM
    );

// Operator ftextupdateObject ( "updateObject", ftextupdateObjectSpec,
//                            4, ftextupdateobject_vm, blowfish_encodeSelect,
//                            StringtypeStringtypeBool2TextTM
//     );
//
// Operator ftextderiveObject ( "deriveObject", ftextderiveObjectSpec,
//                            4, ftextderiveobject_vm, blowfish_encodeSelect,
//                            StringtypeStringtypeBool2TextTM
//     );

Operator ftextgetObjectTypeNL ( "getObjectTypeNL", ftextgetObjectTypeNLSpec,
                           2, ftextgetObjectTypeNL_vm, ftextdeleteobjectselect,
                           TypeMap_textstring__text
    );

Operator ftextgetObjectValueNL ("getObjectValueNL",ftextgetObjectValueNLSpec,
                           2,ftextgetObjectValueNL_vm, ftextdeleteobjectselect,
                           TypeMap_textstring__text
    );

Operator getDatabaseName
(
   "getDatabaseName",          //name
   getDatabaseNameSpec,        //specification
   getDatabaseName_VM,         //value mapping
   Operator::SimpleSelect,     //trivial selection function
   TypeMap_empty__string              //type mapping
);


Operator matchingOperatorNames
(
   "matchingOperatorNames",          //name
   matchingOperatorNamesSpec,        //specification
   matchingOperatorsVM<true>,         //value mapping
   Operator::SimpleSelect,     //trivial selection function
   matchingOperatorNamesTM              //type mapping
);


Operator matchingOperators
(
   "matchingOperators",          //name
   matchingOperatorsSpec,        //specification
   matchingOperatorsVM<false>,         //value mapping
   Operator::SimpleSelect,     //trivial selection function
   matchingOperatorsTM              //type mapping
);




/*
5 Creating the algebra

*/

class FTextAlgebra : public Algebra
{
public:
  FTextAlgebra() : Algebra()
  {
    if(traces)
      cout <<'\n'<<"Start FTextAlgebra() : Algebra()"<<'\n';
    AddTypeConstructor( &ftext );
    AddTypeConstructor( &svg );
    ftext.AssociateKind("DATA");
    svg.AssociateKind("DATA");
    ftext.AssociateKind("INDEXABLE");
    ftext.AssociateKind("CSVIMPORTABLE");
    AddOperator( &contains );
    AddOperator( &length );
    AddOperator( &getkeywords );
    AddOperator( &getsentences );
    AddOperator( &diceCoeff);
    AddOperator( &ftextgetcatalog );
    AddOperator( &ftextsubstr );
    AddOperator( &ftextsubtext );
    AddOperator( &ftextfind );
    AddOperator( &ftextisempty );
    AddOperator( &ftexttrim );
    AddOperator( &ftextplus );
    AddOperator( &ftextless );
    AddOperator( &ftextlesseq );
    AddOperator( &ftexteq );
    AddOperator( &ftextbiggereq );
    AddOperator( &ftextbigger );
    AddOperator( &ftextneq );
    AddOperator( &ftextevaluate );
    AddOperator( &ftextreplace );
    AddOperator( &ftexttoupper );
    AddOperator( &ftexttolower );
    AddOperator( &ftexttostring );
    AddOperator( &ftexttotext );
    AddOperator( &isDBObject);
    AddOperator( &getTypeNL );
    AddOperator( &getValueNL );
    AddOperator( &ftexttoobject );
    AddOperator( &chartext );
    AddOperator( &ftsendtextUDP );
    AddOperator( &ftreceivetextUDP );
    AddOperator( &ftreceivetextstreamUDP );
    AddOperator( &svg2text );
    AddOperator( &text2svg );
    AddOperator( &crypt);
    AddOperator( &checkpw);
    AddOperator( &md5);
    AddOperator( &blowfish_encode);
    AddOperator( &blowfish_decode);
    AddOperator( &ftextletObject);
    AddOperator( &ftextdeleteObject);
    AddOperator( &ftextcreateObject);
//     AddOperator( &ftextderiveObject);
//     AddOperator( &ftextupdateObject);
    AddOperator( &ftextgetObjectTypeNL);
    AddOperator( &ftextgetObjectValueNL);
    AddOperator( &getDatabaseName);
    AddOperator( &matchingOperatorNames);
    AddOperator( &matchingOperators);

    LOGMSG( "FText:Trace",
      cout <<"End FTextAlgebra() : Algebra()"<<'\n';
    )
  }

  ~FTextAlgebra() {};
};

} // end namespace ftext

/*
6 Initialization

Each algebra module needs an initialization function. The algebra manager
has a reference to this function if this algebra is included in the list
of required algebras, thus forcing the linker to include this module.

The algebra manager invokes this function to get a reference to the instance
of the algebra class and to provide references to the global nested list
container (used to store constructor, type, operator and object information)
and to the query processor.

*/

extern "C"
Algebra*
InitializeFTextAlgebra( NestedList* nlRef,
                        QueryProcessor* qpRef,
                        AlgebraManager* amRef )
{
  if(traces)
    cout << '\n' <<"InitializeFTextAlgebra"<<'\n';
  ftext::FTextAlgebra* ptr = new ftext::FTextAlgebra();
  ptr->Init(nl, qp, am);
  return ptr;
}

