/*
//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]

[1] FText Algebra

March - April 2003 Lothar Sowada

The algebra ~FText~ provides the type constructor ~text~ and two operators:

(i) ~search~, which checks if a text is in another text.

(ii) ~length~ which give back the length of a text.

1 Preliminaries

1.1 Includes

*/

using namespace std;

#include "StandardAttribute.h"
#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "StandardTypes.h" //needed because we return a CcBool in an op.
#include <string>
#include <iostream.h>

static NestedList* nl;
static QueryProcessor* qp;



/*
2 Type Constructor ~text~

2.1 Data Structure - Class ~FText~

*/

const string typeName="text";
const bool traces=false;
typedef string* textType;



class FText: public StandardAttribute
{
public:

  FText(bool newDefined, textType newText);
  FText();
  ~FText();

  bool  Search(textType inString);
  void  SetFText(textType inString);
  int   TextLength();
  textType GetFText();

/*************************************************************************

  The following virtual functions:
  IsDefined, SetDefined, GetValue, HashValue, CopyFrom, Compare, Sizeof, Clone, Print, Adjacent
  need to be defined if we want to use ~text~ as an attribute type in tuple definitions.

*************************************************************************/

  bool     IsDefined();
  void     SetDefined(bool newDefined);
  void*    GetValue();
  size_t   HashValue();
  void     CopyFrom(StandardAttribute* right);
  int      Compare(Attribute * arg);
  int      Sizeof() ;
  FText*   Clone() ;
  ostream& Print(ostream &os);
  int      Adjacent(Attribute * arg);

private:
  textType theText;
  bool defined;
};


FText::FText(bool newDefined, textType newText)
{
  if(traces)
    cout << '\n' <<"Start FText(bool newDefined, textType newText)"<<'\n';
  defined=newDefined;
  theText= new string;
  theText=newText;
  if(traces)
    cout <<"End FText(bool newDefined, textType newText)"<<'\n';
}


FText::FText()
{
  if(traces)
    cout << '\n' <<"Start FText()"<<'\n';
  theText= new string;
  if(traces)
    cout <<"End FText()"<<'\n';
}


FText::~FText()
{
  if(traces)
    cout << '\n' <<"Start ~FText()"<<'\n';
  delete theText;
  if(traces)
    cout <<"End ~FText()"<<'\n';
}


bool  FText::Search(textType subString)
{
  int ipos=theText->find(*subString);
  return (ipos>-1);
}


void FText::SetFText(textType newString)
{
  if(traces)
    cout << '\n' << "Start SetFText with *newString='"<<*newString<<"'\n";
    *theText=*newString;
  if(traces)
    cout <<"End SetFText"<<'\n';
}


int FText::TextLength()
{
  return theText->length();
}


textType FText::GetFText()
{
  return theText;
}

/*

2.2 Class methods for using ~text~ in tuple definitions

In the following, we give the definitions of the virtual functions which are needed
if we want to use ~text~ as an attribute type in tuple definitions.

*/

bool FText::IsDefined()
{
  return (defined);
}

void FText::SetDefined(bool newDefined)
{
  if(traces)
    cout << '\n' << "Start SetDefined" << '\n';
  defined = newDefined;
}


void*  FText::GetValue()
{
  if(traces)
    cout << '\n' << "Start GetValue" << '\n';
  return ((void *)theText);
}

size_t FText::HashValue()
{
  if(traces)
    cout << '\n' << "Start HashValue" << '\n';
  if(!defined)
    return 0;

  unsigned long h = 0;
  char* s = (char*)theText->c_str();
  while(*s != 0)
  {
    h = 5 * h + *s;
    s++;
  }
  return size_t(h);
}

void FText::CopyFrom(StandardAttribute* right)
{
  if(traces)
    cout << '\n' << "Start CopyFrom" << '\n';
  FText* r = (FText*)right;
  defined = r->defined;
  SetFText(r->GetFText());
}


int FText::Compare(Attribute * arg)
{
  if(traces)
    cout << '\n' << "Start Compare" << '\n';
  if(!IsDefined() && !arg->IsDefined())
    return 0;

  if(!IsDefined())
    return -1;

  if(!arg->IsDefined())
    return 1;

  FText* f = (FText* )(arg);
  if ( !f )
    return -2;

  textType pstr=f->GetFText();
  if ( *theText==*pstr)
    return 0;
  else
    return 0;
}


int  FText::Sizeof()
{
  if(traces)
    cout << '\n' << "Start Sizeof" << '\n';
  return this->TextLength();
}


FText*  FText::Clone()
{
  return (new FText(*this));
}


ostream& FText::Print(ostream &os)
{
  return (os << theText);
}


int FText::Adjacent(Attribute *arg)
{
  if(traces)
    cout << '\n' << "Start Adjacent" << '\n';

  textType atT = (textType)GetValue(),
    btT = (textType)(((FText *)arg)->GetValue());

  const char *a = atT->c_str(),
    *b = btT->c_str();


  if( strcmp( a, b ) == 0 )
    return 1;

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

  return 0;
}


/*

2.3 Functions for using ~text~ in tuple definitions

The following Functions must be defined if we want to use ~ftext~ as an attribute type in tuple definitions.

*/

static Word
CreateFText( const ListExpr typeInfo )
{
  if(traces)
    cout << '\n' << "Start CreateFText" << '\n';
  return (SetWord( new FText()));
}

static void
DeleteFText( Word& w )
{
  if(traces)
    cout << '\n' << "Start DeleteFText" << '\n';
  delete (FText*) w.addr;
  w.addr = 0;
}

static void
CloseFText( Word& w )
{
  if(traces)
    cout << '\n' << "Start CloseFText" << '\n';
  delete (FText*) w.addr;
  w.addr = 0;
}

static Word
CloneFText( const Word& w )
{
  if(traces)
    cout << '\n' << "Start CloneFText" << '\n';
  return SetWord( ((FText *)w.addr)->Clone() );
}

static void*
CastFText( void* addr )
{
  if(traces)
    cout << '\n' << "Start CastFText" << '\n';
  return (new (addr) FText);
}


/*

2.4 ~In~ and ~Out~ Functions

*/

static ListExpr
OutFText( ListExpr typeInfo, Word value )
{
  if(traces)
    cout << '\n' << "Start OutFText" << '\n';
  FText* pftext;
  pftext = (FText*)(value.addr);
  ListExpr TextAtomVar=nl->TextAtom();

  if(traces)
    cout <<"pftext->GetFText()='"<<*pftext->GetFText()<<"'\n";
  nl->AppendText(TextAtomVar, *pftext->GetFText());

  if(traces)
    cout <<"End OutFText" << '\n';
  return nl->OneElemList(TextAtomVar);
}


static Word
InFText( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct )
{
  if(traces)
    cout << '\n' << "Start InFText with ListLength "<<nl->ListLength( instance );
  ListExpr First;
  if (nl->ListLength( instance ) == 1 || nl->ListLength( instance ) == 4 )
    First = nl->First(instance);
  else
    First =instance;
  if ( nl->IsAtom(First) && nl->AtomType(First) == TextType)
  {
    string buffer;
    TextScan textScan = nl->CreateTextScan (First);
    nl->GetText (textScan, nl->TextLength( First ), buffer);
    nl->DestroyTextScan (textScan);

    FText* newftext;
    newftext = new FText();
    buffer=buffer;
    newftext->SetFText(&buffer);
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

static ListExpr
FTextProperty()
{
  ListExpr examplelist = nl->TextAtom();
  nl->AppendText(examplelist,"<text>A text, which can be longer "
  "than 48 chararcters<text--->");

  return (nl->TwoElemList(
         nl->FourElemList(nl->StringAtom("Signature"), 
	                  nl->StringAtom("Example Type List"), 
			  nl->StringAtom("List Rep"), 
			  nl->StringAtom("Example List")),
         nl->FourElemList(nl->StringAtom("-> DATA"), 
	                  nl->StringAtom("text"), 
			  nl->StringAtom("(<textvalue>)"), 
			  examplelist)));
}

/*
2.6 Kind Checking Function

This function checks whether the type constructor is applied correctly.

*/

static bool
CheckFText( ListExpr type, ListExpr& errorInfo )
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


/*

2.7 Creation of the type constructor instance

*/

TypeConstructor ftext(
  typeName,       //name of the type
  FTextProperty,    //property function describing signature
  OutFText,    InFText, //Out and In functions
  CreateFText, DeleteFText,  //object creation and deletion
  0, 0, CloseFText, CloneFText,     //object open, save, close, and clone
  CastFText,        //cast function
  CheckFText,                 //kind checking function
  0,             //predef. pers. function for model
  TypeConstructor::DummyInModel,
  TypeConstructor::DummyOutModel,
  TypeConstructor::DummyValueToModel,
  TypeConstructor::DummyValueListToModel );


/*

3 Creating Operators

3.1 Type Mapping Functions

Checks whether the correct argument types are supplied for an operator; if so,
returns a list expression for the result type, otherwise the symbol ~typeerror~.

*/

static ListExpr
FTextFTextBool( ListExpr args )
{
  if(traces)
    cout <<'\n'<<"Start FTextFTextBool"<<'\n';
  nl->WriteToFile("/dev/tty",args);
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
    cout <<"End FTextFTextBool with typeerror"<<'\n';
  return nl->SymbolAtom("typeerror");
}


static ListExpr
FTextInt( ListExpr args )
{
  if(traces)
    cout << '\n' << "Start FTextInt" << '\n';
  if ( nl->ListLength(args) == 1 )
  {
    ListExpr arg1= nl->First(args);
    if ( nl->IsEqual(arg1, typeName))
    {
      if(traces)
        cout << '\n' << "Start FTextInt" << '\n';
      return nl->SymbolAtom("int");
    }
  }

  if(traces)
    cout <<"End FTextInt with typeerror"<<'\n';
  return nl->SymbolAtom("typeerror");
}


/*

3.2 Selection Function

Is used to select one of several evaluation functions for an overloaded
operator, based on the types of the arguments. In case of a non-overloaded
operator, we just have to return 0.

*/

static int
simpleSelect (ListExpr args )
{
  return 0;
}


/*
3.3 Value Mapping Function

*/

static int
searchFun (Word* args, Word& result, int message, Word& local, Supplier s)
/*
Search predicate for two texts.

*/

{
  if(traces)
    cout <<'\n'<<"Start searchFun"<<'\n';
  FText* ftext1= ((FText*)args[0].addr);
  FText* ftext2= ((FText*)args[1].addr);

  result = qp->ResultStorage(s); //query processor has provided
          //a CcBool instance to take the result
  ((CcBool*)result.addr)->Set(true, ftext1->Search(ftext2->GetFText()));
          //the first argument says the boolean
          //value is defined, the second is the
          //real boolean value)

  if(traces)
    cout <<"End searchFun"<<'\n';
  return 0;
}


static int
lengthFun (Word* args, Word& result, int message, Word& local, Supplier s)
/*
length predicate for a text.

*/

{
  if(traces)
    cout <<'\n'<<"Start lengthFun"<<'\n';
  FText* ftext1= ((FText*)args[0].addr);

  result = qp->ResultStorage(s); //query processor has provided
          //a CcBool instance to take the result
  ((CcInt*)result.addr)->Set(true, ftext1->TextLength());
          //the first argument says the boolean
          //value is defined, the second is the
          //length value)
  if(traces)
    cout <<"End lengthFun"<<'\n';
  return 0;
}


/*
3.4 Definition of Operators

*/

const string searchSpec  = "( ( \"Signature\" \"Syntax\" \"Meaning\" "
                           "\"Example\" )"
                           "( <text>(text text) -> "
			   "bool</text--->"
			   "<text>_ search _</text--->"
			   "<text>Search the second text in the first "
			   "text.</text--->"
			   "<text>text1 search text2</text--->"
			   ") )";

const string lengthSpec  = "( ( \"Signature\" \"Syntax\" \"Meaning\" "
                           "\"Example\" )"
                           "( <text>( text ) -> int</text--->"
			   "<text>length ( _ )</text--->"
			   "<text>length returns the length of "
			   "text.</text--->"
			   "<text>length ( text1 )</text--->"
			   ") )";

/*
Used to explain the signature and the meaning of the ~search~ and ~length~ operators.

*/

Operator ftextsearch (
  "search",     //name
  searchSpec,         //specification
  searchFun,    //value mapping
  Operator::DummyModel,   //dummy model mapping, defined in Algebra.h
  simpleSelect,     //trivial selection function
  FTextFTextBool      //type mapping
);

Operator ftextlength (
  "length",     //name
  lengthSpec,         //specification
  lengthFun,    //value mapping
  Operator::DummyModel,   //dummy model mapping, defined in Algebra.h
  simpleSelect,     //trivial selection function
  FTextInt      //type mapping
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
    ftext.AssociateKind("DATA");
    AddOperator( &ftextsearch );
    AddOperator( &ftextlength );
    //cout <<"End FTextAlgebra() : Algebra()"<<'\n';
  }

  ~FTextAlgebra() {};
};

FTextAlgebra FTextAlgebra;

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
InitializeFTextAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  if(traces)
    cout << '\n' <<"InitializeFTextAlgebra"<<'\n';
  nl = nlRef;
  qp = qpRef;
  return (&FTextAlgebra);
}

