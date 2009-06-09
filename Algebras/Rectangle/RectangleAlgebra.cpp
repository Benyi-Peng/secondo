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

//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]

[1] Implementation of the Rectangle Algebra

October, 2003. Victor Teixeira de Almeida

December 2005, Victor Almeida deleted the deprecated algebra levels
(~executable~, ~descriptive~, and ~hibrid~). Only the executable
level remains. Models are also removed from type constructors.

1 Overview

This implementation file essentially contains the implementation of the
struct ~Rectangle~, and the definitions of the type constructur
~rect~, ~rect3~, and ~rect4~ with its associated operations.

2 Defines and Includes

*/
using namespace std;

#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "SpatialAlgebra.h"
#include "RectangleAlgebra.h"
#include "StandardTypes.h"
#include "RelationAlgebra.h"

extern NestedList* nl;
extern QueryProcessor* qp;

/*
3 Type Constructor ~rect~

A value of type ~rect~ represents a 2-dimensional rectangle alligned with
the axes x and y. A rectangle in such a way can be represented by four
numbers, the upper and lower values for the two dimensions.

3.1 List Representation

The list representation of a 2D rectangle is

----    (x1 x2 y1 y2)
----

3.3 ~Out~-function

See RectangleAlgebra.h

3.4 ~In~-function

See RectangleAlgebra.h

3.9 Function describing the signature of the type constructor

*/
ListExpr
Rectangle2Property()
{
  return (nl->TwoElemList(
            nl->FourElemList(nl->StringAtom("Signature"),
                             nl->StringAtom("Example Type List"),
                             nl->StringAtom("List Rep"),
                             nl->StringAtom("Example List")),
            nl->FourElemList(nl->StringAtom("-> DATA"),
                             nl->StringAtom("rect"),
                             nl->StringAtom("(<left> <right> <bottom> <top>)"),
                             nl->StringAtom("(0.0 1.0 0.0 1.0)"))));
}

/*
3.10 Kind Checking Function

This function checks whether the type constructor is applied correctly. Since
type constructor ~rect~ does not have arguments, this is trivial.

*/
bool
CheckRectangle2( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "rect" ));
}

/*
3.12 Creation of the type constructor instance

*/
TypeConstructor rect(
        "rect",                                  //name
        Rectangle2Property,                      //property function
                                                 //describing signature
        OutRectangle<2>,     InRectangle<2>,     //Out and In functions
        0,                   0,                  //SaveToList and
                                                 //RestoreFromList functions
        CreateRectangle<2>,  DeleteRectangle<2>, //object creation and deletion
        OpenAttribute<Rectangle<2> >,
        SaveAttribute<Rectangle<2> >,            //open and save functions
        CloseRectangle<2>,   CloneRectangle<2>,  //object close, and clone
        CastRectangle<2>,                        //cast function
        SizeOfRectangle<2>,                      //sizeof function
        CheckRectangle2 );                       //kind checking function

/*
3 Type Constructor ~rect3~

A value of type ~rect3~ represents a 3-dimensional rectangle alligned with
the axes x, y and z. A rectangle in such a way can be represented by six
numbers, the upper and lower values for the three dimensions.

3.1 List Representation

The list representation of a 3D rectangle is

----    (x1 x2 y1 y2 z1 z2)
----

3.3 ~Out~-function

See RectangleAlgeba.h

3.4 ~In~-function

See RectangleAlgebra.h

3.9 Function describing the signature of the type constructor

*/
ListExpr
Rectangle3Property()
{
  return (nl->TwoElemList(
            nl->FourElemList(nl->StringAtom("Signature"),
                             nl->StringAtom("Example Type List"),
                             nl->StringAtom("List Rep"),
                             nl->StringAtom("Example List")),
            nl->FourElemList(nl->StringAtom("-> DATA, RECT34"),
                             nl->StringAtom("rect3"),
                             nl->StringAtom(
                             "(list of six <value>). l/r for 3 dimensions."),
                             nl->StringAtom("(0.0 1.0 10.0 11.0 20.0 21.0)"))));
}

/*
3.10 Kind Checking Function

This function checks whether the type constructor is applied correctly. Since
type constructor ~rect3~ does not have arguments, this is trivial.

*/
bool
CheckRectangle3( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "rect3" ));
}

/*
3.12 Creation of the type constructor instance

*/
TypeConstructor rect3(
        "rect3",                                 //name
        Rectangle3Property,                      //property function
                                                 //describing signature
        OutRectangle<3>,     InRectangle<3>,     //Out and In functions
        0,                   0,                  //SaveToList and
                                                 //RestoreFromList functions
        CreateRectangle<3>,  DeleteRectangle<3>, //object creation and deletion
        OpenAttribute<Rectangle<3> >,
        SaveAttribute<Rectangle<3> >,            //open and save functions
        CloseRectangle<3>,   CloneRectangle<3>,  //object close, and clone
        CastRectangle<3>,                        //cast function
        SizeOfRectangle<3>,                      //sizeof function
        CheckRectangle3 );                       //kind checking function

/*
3 Type Constructor ~rect4~

A value of type ~rect4~ represents a 4-dimensional rectangle alligned with
the axes w, x, y and z. A rectangle in such a way can be represented by eight
numbers, the upper and lower values for the four dimensions.

3.1 List Representation

The list representation of a 4D rectangle is

----    (w1 w2 x1 x2 y1 y2 z1 z2)
----

3.3 ~Out~-function

See RectangleAlgebra.h

3.4 ~In~-function

See RectangleAlgebra.h

3.9 Function describing the signature of the type constructor

*/
ListExpr
Rectangle4Property()
{
  return (nl->TwoElemList(
            nl->FourElemList(nl->StringAtom("Signature"),
                             nl->StringAtom("Example Type List"),
                             nl->StringAtom("List Rep"),
                             nl->StringAtom("Example List")),
            nl->FourElemList(nl->StringAtom("-> DATA"),
                             nl->StringAtom("rect4"),
                             nl->StringAtom(
                             "(list of eight <value>). l/r for 4 dimensions."),
                             nl->StringAtom(
                             "(0.0 1.0 10.0 11.0 20.0 21.0 0.0 0.4)"))));
}

/*
3.10 Kind Checking Function

This function checks whether the type constructor is applied correctly. Since
type constructor ~rect4~ does not have arguments, this is trivial.

*/
bool
CheckRectangle4( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "rect4" ));
}

/*
3.12 Creation of the type constructor instance

*/
TypeConstructor rect4(
        "rect4",                                 //name
        Rectangle4Property,                      //property function
                                                 //describing signature
        OutRectangle<4>,     InRectangle<4>,     //Out and In functions
        0,                   0,                  //SaveToList and
                                                 //RestoreFromList functions
        CreateRectangle<4>,  DeleteRectangle<4>, //object creation and deletion
        OpenAttribute<Rectangle<4> >,
        SaveAttribute<Rectangle<4> >,            //open and save functions
        CloseRectangle<4>,   CloneRectangle<4>,  //object close, and clone
        CastRectangle<4>,                        //cast function
        SizeOfRectangle<4>,                      //sizeof function
        CheckRectangle4 );                       //kind checking function

/*
3 Type Constructor ~rect8~

A value of type ~rect8~ represents a 8-dimensional rectangle alligned with
the axes. A rectangle in such a way can be represented by sixteen
numbers, the upper and lower values for the eight dimensions.

3.1 List Representation

The list representation of a 8D rectangle is

----    (x1 x2 ... x15 x16)
----

3.3 ~Out~-function

See RectangleAlgebra.h

3.4 ~In~-function

See RectangleAlgebra.h

3.9 Function describing the signature of the type constructor

*/
ListExpr
Rectangle8Property()
{
  return (nl->TwoElemList(
            nl->FourElemList(nl->StringAtom("Signature"),
                             nl->StringAtom("Example Type List"),
                             nl->StringAtom("List Rep"),
                             nl->StringAtom("Example List")),
            nl->FourElemList(nl->StringAtom("-> DATA"),
                             nl->StringAtom("rect8"),
                             nl->StringAtom(
                             "(list of sixteen <value>). l/r for "
                             "8 dimensions."),
                             nl->StringAtom(
                             "(0 1 10 11 20 21 0 4 4"
                             " 5 6 7 1 3 4 6)"))));
}

/*
3.10 Kind Checking Function

This function checks whether the type constructor is applied correctly. Since
type constructor ~rect4~ does not have arguments, this is trivial.

*/
bool
CheckRectangle8( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "rect8" ));
}

/*
3.12 Creation of the type constructor instance

*/
TypeConstructor rect8(
        "rect8",                                 //name
        Rectangle8Property,                      //property function
                                                 //describing signature
        OutRectangle<8>,     InRectangle<8>,     //Out and In functions
        0,                   0,                  //SaveToList and
                                                 //RestoreFromList functions
        CreateRectangle<8>,  DeleteRectangle<8>, //object creation and deletion
        OpenAttribute<Rectangle<8> >,
        SaveAttribute<Rectangle<8> >,            //open and save functions
        CloseRectangle<8>,   CloneRectangle<8>,  //object close, and clone
        CastRectangle<8>,                        //cast function
        SizeOfRectangle<8>,                      //sizeof function
        CheckRectangle8 );                       //kind checking function

/*
4 Operators

Definition of operators is similar to definition of type constructors. An
operator is defined by creating an instance of class ~Operator~. Again we
have to define some functions before we are able to create an ~Operator~
instance.

4.1 Type mapping functions

A type mapping function takes a nested list as argument. Its contents are
type descriptions of an operator's input parameters. A nested list describing
the output type of the operator is returned.


4.1.2 Type mapping function ~RectTypeMapBool~

It is for the operator ~isempty~ which have ~rect~, ~rect3~, or ~rect4~
as input and ~bool~ as result type.

*/
ListExpr
RectTypeMapBool( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );
    if( nl->IsEqual( arg1, "rect" ) ||
        nl->IsEqual( arg1, "rect3" ) ||
        nl->IsEqual( arg1, "rect4" ) ||
        nl->IsEqual( arg1, "rect8" ))
      return (nl->SymbolAtom( "bool" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}


/*
4.1.2 Type mapping function ~RectTypeMapReal~

It is for the operator ~isempty~ which have ~rect~, ~rect3~, or ~rect4~
as input and ~real~ as result type.

*/
ListExpr
RectTypeMapReal( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );
    if( nl->IsEqual( arg1, "rect" ) ||
        nl->IsEqual( arg1, "rect3" ) ||
        nl->IsEqual( arg1, "rect4" ) ||
        nl->IsEqual( arg1, "rect8" ))
      return (nl->SymbolAtom( "real" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
4.1.3 Type mapping function ~RectRectTypeMapRect~

It is for the operator ~union~ and ~intersection~, which takes
two rectangles as arguments and return a rectangle.

*/
ListExpr
RectRectTypeMapRect( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if( nl->IsEqual( arg1, "rect" ) && nl->IsEqual( arg2, "rect" ) )
      return nl->SymbolAtom( "rect" );
    if( nl->IsEqual( arg1, "rect3" ) && nl->IsEqual( arg2, "rect3" ) )
      return nl->SymbolAtom( "rect3" );
    if( nl->IsEqual( arg1, "rect4" ) && nl->IsEqual( arg2, "rect4" ) )
      return nl->SymbolAtom( "rect4" );
    if( nl->IsEqual( arg1, "rect8" ) && nl->IsEqual( arg2, "rect8" ) )
      return nl->SymbolAtom( "rect8" );
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
4.1.3 Type mapping function ~RectRectTypeMapBool~

It is for the operator like ~equal~, which takes
two rectangles as arguments and return a bool.

*/
ListExpr
RectRectTypeMapBool( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if( nl->IsEqual( arg1, "rect" ) && nl->IsEqual( arg2, "rect" ) )
      return nl->SymbolAtom( "bool" );
    if( nl->IsEqual( arg1, "rect3" ) && nl->IsEqual( arg2, "rect3" ) )
      return nl->SymbolAtom( "bool" );
    if( nl->IsEqual( arg1, "rect4" ) && nl->IsEqual( arg2, "rect4" ) )
      return nl->SymbolAtom( "bool" );
    if( nl->IsEqual( arg1, "rect8" ) && nl->IsEqual( arg2, "rect8" ) )
      return nl->SymbolAtom( "bool" );
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
4.1.4 Type mapping function ~TranslateTypeMap~

It is used for the ~translate~ operator.

*/
ListExpr TranslateTypeMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if( nl->IsEqual( arg1, "rect" ) &&
        nl->ListLength( arg2 ) == 2 &&
        nl->IsEqual( nl->First( arg2 ), "real" ) &&
        nl->IsEqual( nl->Second( arg2 ), "real" ) )
      return nl->SymbolAtom( "rect" );

    if( nl->IsEqual( arg1, "rect3" ) &&
        nl->ListLength( arg2 ) == 3 &&
        nl->IsEqual( nl->First( arg2 ), "real" ) &&
        nl->IsEqual( nl->Second( arg2 ), "real" ) &&
        nl->IsEqual( nl->Third( arg2 ), "real" ) )
      return nl->SymbolAtom( "rect3" );

    if( nl->IsEqual( arg1, "rect4" ) &&
        nl->ListLength( arg2 ) == 4 &&
        nl->IsEqual( nl->First( arg2 ), "real" ) &&
        nl->IsEqual( nl->Second( arg2 ), "real" ) &&
        nl->IsEqual( nl->Third( arg2 ), "real" ) &&
        nl->IsEqual( nl->Fourth( arg2 ), "real" ) )
      return nl->SymbolAtom( "rect4" );
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
4.1.5 Type mapping function ~rectanglex~

It is used for the ~rectanglex~ operator.

*/
template< int dim>
ListExpr RectangleTypeMap( ListExpr args )
{
  ListExpr arg[2*dim];
  bool checkint = true, checkreal = true;

  if( (nl->ListLength( args ) == 2*dim) )
  {
    for(unsigned int i = 1; i <= 2*dim; i++) {
      arg[i-1] = nl->Nth(i,args);
    }

    for(int j = 0; j < 2*dim; j++) {
      if( !(nl->IsEqual( arg[j], "int" )) ) { checkint = false; break; }
    }

    for(int k = 0; k < 2*dim; k++) {
     if( !(nl->IsEqual( arg[k], "real" )) ) { checkreal = false; break; }
   }

   if( checkint ||  checkreal )
     switch(dim) {
       case 2: return nl->SymbolAtom( "rect" );
       case 3: return nl->SymbolAtom( "rect3" );
       case 4: return nl->SymbolAtom( "rect4" );
       default: return nl->SymbolAtom( "typeerror" );
     }
    else ErrorReporter::ReportError("All argument types must be either"
                                    " int or real!");
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
4.1.6 Type mapping function ~rectangle8size~

It is used for the ~rectangle8size~ operator.

*/
template< int dim>
ListExpr Rectangle8TypeMap( ListExpr args )
{
  ListExpr arg[dim+1];
  bool checkint = true, checkreal = true;

  if( (nl->ListLength( args ) == dim+1) )
  {
    for(unsigned int i = 1; i <= dim+1; i++) {
      arg[i-1] = nl->Nth(i,args);
    }

    for(int j = 0; j < dim; j++) {
      if( !(nl->IsEqual( arg[j], "int" )) ) { checkint = false; break; }
    }

    for(int k = 0; k < dim; k++) {
     if( !(nl->IsEqual( arg[k], "real" )) ) { checkreal = false; break; }
   }

   if ( (checkint ||  checkreal) && nl->IsEqual( arg[dim], "real" ) )
     return nl->SymbolAtom( "rect8" );
   else ErrorReporter::ReportError("All argument types must be either"
                                   " int or real!");
  }
  return nl->SymbolAtom( "typeerror" );
}

/*
4.1.7 Type mapping function ~RectRectTypeMapReal~

Used for ~distance~.

*/
ListExpr
RectRectTypeMapReal( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if( ( nl->IsEqual( arg1,  "rect" ) && nl->IsEqual( arg2,  "rect" ) ) ||
        ( nl->IsEqual( arg1, "rect3" ) && nl->IsEqual( arg2, "rect3" ) ) ||
        ( nl->IsEqual( arg1, "rect4" ) && nl->IsEqual( arg2, "rect4" ) )   )
    return nl->SymbolAtom( "real" );
  }
  return nl->SymbolAtom( "typeerror" );
}


/*
4.1.7 Type mapping function ~RectProjectTypeMap~

Used for ~rectproject~.

*/
ListExpr
RectProjectTypeMap( ListExpr args )
{
  ListExpr arg1, arg2, arg3;
  string argstr;
  int dim = -1;
  if ( !( nl->ListLength( args ) == 3 ) )
  {
    nl->WriteToString (argstr, args);
    ErrorReporter::ReportError("operator rectproject expects a list "
        "a list of length 3, but gets '" + argstr + "'.");
    return nl->SymbolAtom("typeerror");\
  }
  arg1 = nl->First( args );
  arg2 = nl->Second( args );
  arg3 = nl->Third( args );
  if( nl->IsEqual( arg1,  "rect" ) ) dim = 2;
  else if( nl->IsEqual( arg1, "rect3" ) ) dim = 3;
  else if( nl->IsEqual( arg1, "rect4" ) ) dim = 4;
  else if( nl->IsEqual( arg1, "rect8" ) ) dim = 8;
  else dim = -1;
  if ( !(dim > 0) ||
       !nl->IsEqual( arg2,  "int" ) ||
       !nl->IsEqual( arg3,  "int" )
     )
  {
    nl->WriteToString (argstr, args);
    ErrorReporter::ReportError("operator rectproject expects a list "
        "'(rect<D> int int)' as input, but gets a list '" + argstr + "'.");
    return nl->SymbolAtom("typeerror");\
  }

  return nl->SymbolAtom( "rect" );
}

/*
4.1.7 Type mapping function ~RectangleMinMaxTypeMap~

Used for ~minD~, ~maxD~.

*/
ListExpr
    RectangleMinMaxTypeMap( ListExpr args )
{
  ListExpr arg1, arg2;
  string argstr;
  int dim = -1;
  AlgebraManager* am = SecondoSystem::GetAlgebraManager();
  ListExpr errorInfo = nl->OneElemList( nl->SymbolAtom( "ERRORS" ) );

  if ( !( nl->ListLength( args ) == 2 ) )
  {
    nl->WriteToString (argstr, args);
    ErrorReporter::ReportError("operator minD/MaxD expects a list "
        "a list of length 2, but gets '" + argstr + "'.");
    return nl->SymbolAtom("typeerror");\
  }
  arg1 = nl->First( args );
  arg2 = nl->Second( args );
  if( nl->IsEqual( arg1,  "rect" ) ||
      am->CheckKind("SPATIAL2D", arg1, errorInfo) ) dim = 2;
  else if( nl->IsEqual( arg1, "rect3" ) ||
           am->CheckKind("SPATIAL2D", arg1, errorInfo) ) dim = 3;
  else if( nl->IsEqual( arg1, "rect4" ) ||
           am->CheckKind("SPATIAL2D", arg1, errorInfo) ) dim = 4;
  else if( nl->IsEqual( arg1, "rect8" ) ||
           am->CheckKind("SPATIAL2D", arg1, errorInfo)) dim = 8;
  else dim = -1;
  if ( !(dim > 0) ||
         !nl->IsEqual( arg2,  "int" )
     )
  {
    nl->WriteToString (argstr, args);
    ErrorReporter::ReportError("operator minD/maxD expects a list "
        "'(T int)' as input, where T in {rect<D>, SPATIAL<D>D}, "
        "but gets a list '" + argstr + "'.");
    return nl->SymbolAtom("typeerror");\
  }

  return nl->SymbolAtom( "real" );
}

ListExpr  RectangleTypeMapRectRect( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );
    if( nl->IsEqual( arg1, "rect" )  ||
        nl->IsEqual( arg1, "rect3" ) ||
        nl->IsEqual( arg1, "rect4" ) ||
        nl->IsEqual( arg1, "rect8" )
      )
      return (arg1);
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
4.1.8 Type mapping function ~RectangleTypeMapEnlargeRect~

It is used for the ~enlargeRect~ operator.

*/
ListExpr RectangleTypeMapEnlargeRect( ListExpr args )
{
  bool ok = true;
  unsigned int noArgs = nl->ListLength( args );
  switch(noArgs) {
    case 3:  ok &= nl->IsEqual( nl->First(args), "rect" );  break;
    case 4:  ok &= nl->IsEqual( nl->First(args), "rect3" ); break;
    case 5:  ok &= nl->IsEqual( nl->First(args), "rect4" ); break;
    case 9:  ok &= nl->IsEqual( nl->First(args), "rect8" ); break;
    default: ok = false;
  }
  if(ok){
    for(unsigned int i = 2; i <= noArgs; i++) {
        if ( !nl->IsEqual(nl->Nth(i,args), "real") ) {
          ok = false;
          break;
        }
    }
  }
  if(ok){
    return nl->First(args);
  }
  ErrorReporter::ReportError("Expected rect<Dim> x real^<Dim>.");
  return (nl->SymbolAtom( "typeerror" ));
}

/*
4.2 Selection functions

A selection function is quite similar to a type mapping function. The only
difference is that it doesn't return a type but the index of a value
mapping function being able to deal with the respective combination of
input parameter types.

Note that a selection function does not need to check the correctness of
argument types; it has already been checked by the type mapping function that it
is applied to correct arguments.

4.3.1 Selection function ~RectangleUnarySelect~

Is used for all unary operators.

*/
int
RectangleUnarySelect( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  if( nl->IsEqual( arg1, "rect" ) )
    return 0;

  if( nl->IsEqual( arg1, "rect3" ) )
    return 1;

  if( nl->IsEqual( arg1, "rect4" ) )
    return 2;

  if( nl->IsEqual( arg1, "rect8" ) )
    return 3;

  return -1; // should never occur
}

/*
4.3.1 Selection function ~RectangleBinarySelect~

Is used for all binary operators.

*/
int
RectangleBinarySelect( ListExpr args )
{
  ListExpr arg1 = nl->First( args ),
           arg2 = nl->Second( args );

  if( nl->IsEqual( arg1, "rect" ) && nl->IsEqual( arg2, "rect" ) )
    return 0;

  if( nl->IsEqual( arg1, "rect3" ) && nl->IsEqual( arg2, "rect3" ) )
    return 1;

  if( nl->IsEqual( arg1, "rect4" ) && nl->IsEqual( arg2, "rect4" ) )
    return 2;

  if( nl->IsEqual( arg1, "rect8" ) && nl->IsEqual( arg2, "rect8" ) )
    return 3;

  return -1; // should never occur
}

/*

4.3.2 Selection function ~RectangleSelect~

Is used for the ~rectanglex~ operator.

*/
template< int dim>
int RectangleSelect( ListExpr args )
{
  ListExpr arg[2*dim];
  bool checkint = true, checkreal = true;

  for(unsigned int i = 1; i <= 2*dim; i++)
  {
    arg[i-1] = nl->Nth(i,args);
  }

  for(int j = 0; j < 2*dim; j++)
  {
    if( !(nl->IsEqual( arg[j], "int" )) ) { checkint = false; break; }
  }

   for(int k = 0; k < 2*dim; k++)
  {
    if( !(nl->IsEqual( arg[k], "real" )) ) { checkreal = false; break; }
  }

  if( checkint ) return 0;

  if( checkreal ) return 1;

  return -1; // should never occur
}

/*

4.3.2 Selection function ~Rectangle8Select~

Is used for the ~rectangle8size~ operator.

*/
template< int dim>
int Rectangle8Select( ListExpr args )
{
  ListExpr arg[dim];
  bool checkint = true, checkreal = true;

  for(unsigned int i = 1; i <= dim; i++)
  {
    arg[i-1] = nl->Nth(i,args);
  }

  for(int j = 0; j < dim; j++)
  {
    if( !(nl->IsEqual( arg[j], "int" )) ) { checkint = false; break; }
  }

   for(int k = 0; k < dim; k++)
  {
    if( !(nl->IsEqual( arg[k], "real" )) ) { checkreal = false; break; }
  }

  if( checkint ) return 0;

  if( checkreal ) return 1;

  return -1; // should never occur
}


/*

4.3.3 Selection function ~RectangleMinMaxDSelect~

Is used for the ~minD~ and ~maxD~ operators.

*/
int RectangleMinMaxDSelect( ListExpr args )
{
  ListExpr arg1 = nl->First(args);
  AlgebraManager* am = SecondoSystem::GetAlgebraManager();
  ListExpr errorInfo = nl->OneElemList( nl->SymbolAtom( "ERRORS" ) );
  if(nl->IsEqual(arg1, "rect" ) ||
     am->CheckKind("SPATIAL2D", arg1, errorInfo) ) return 0;
  if(nl->IsEqual(arg1, "rect3") ||
     am->CheckKind("SPATIAL3D", arg1, errorInfo) ) return 1;
  if(nl->IsEqual(arg1, "rect4") ||
     am->CheckKind("SPATIAL4D", arg1, errorInfo) ) return 2;
  if(nl->IsEqual(arg1, "rect8") ||
     am->CheckKind("SPATIAL8D", arg1, errorInfo) ) return 3;

  return -1; // should never occur
}

/*
4.4 Value mapping functions

A value mapping function implements an operator's main functionality: it takes
input arguments and computes the result. Each operator consists of at least
one value mapping function. In the case of overloaded operators there are
several value mapping functions, one for each possible combination of input
parameter types.

4.4.1 Value mapping functions of operator ~isempty~

*/
template <unsigned dim>
int RectangleIsEmpty( Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if( ((Rectangle<dim>*)args[0].addr)->IsDefined() )
  {
    ((CcBool*)result.addr)->Set( true, false );
  }
  else
  {
    ((CcBool *)result.addr)->Set( true, true );
  }
  return (0);
}

/*
4.4.2 Value mapping functions of operator ~$=$~

*/
template <unsigned dim>
int RectangleEqual( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if ( ((Rectangle<dim>*)args[0].addr)->IsDefined() &&
       ((Rectangle<dim>*)args[1].addr)->IsDefined() )
  {
    ((CcBool *)result.addr)->
      Set( true, *((Rectangle<dim>*)args[0].addr) ==
      *((Rectangle<dim>*)args[1].addr) );
  }
  else
  {
    ((CcBool *)result.addr)->Set( false, false );
  }
  return (0);
}

/*
4.4.3 Value mapping functions of operator ~$\neq$~

*/
template <unsigned dim>
int RectangleNotEqual( Word* args, Word& result, int message,
                       Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if ( ((Rectangle<dim>*)args[0].addr)->IsDefined() &&
       ((Rectangle<dim>*)args[1].addr)->IsDefined() )
  {
    ((CcBool *)result.addr)->
      Set( true, *((Rectangle<dim>*)args[0].addr) !=
      *((Rectangle<dim>*)args[1].addr) );
  }
  else
  {
    ((CcBool *)result.addr)->Set( false, false );
  }
  return (0);
}

/*
4.4.4 Value mapping functions of operator ~intersects~

*/
template <unsigned dim>
int RectangleIntersects( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  if ( ((Rectangle<dim>*)args[0].addr)->IsDefined() &&
       ((Rectangle<dim>*)args[1].addr)->IsDefined() )
  {
    ((CcBool *)result.addr)->
      Set( true, ((Rectangle<dim>*)args[0].addr)->
      Intersects( *((Rectangle<dim>*)args[1].addr) ) );
  }
  else
  {
    ((CcBool *)result.addr)->Set( false, false );
  }
  return (0);
}

/*
4.4.5 Value mapping functions of operator ~inside~

*/
template <unsigned dim>
int RectangleInside( Word* args, Word& result, int message,
                     Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if ( ((Rectangle<dim>*)args[0].addr)->IsDefined() &&
       ((Rectangle<dim>*)args[1].addr)->IsDefined() )
  { // A inside B <=> B contains A
    ((CcBool *)result.addr)->
      Set( true, ((Rectangle<dim>*)args[1].addr)->
      Contains( *((Rectangle<dim>*)args[0].addr) ) );
  }
  else
  {
    ((CcBool *)result.addr)->Set( false, false );
  }
  return (0);
}

/*
4.4.6 Value mapping functions of operator ~union~

*/
template <unsigned dim>
int RectangleUnion( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  *((Rectangle<dim> *)result.addr) =
      ((Rectangle<dim>*)args[1].addr)->
      Union( *((Rectangle<dim>*)args[0].addr) );
  return (0);
}

/*
4.4.7 Value mapping functions of operator ~intersection~

*/
template <unsigned dim>
int RectangleIntersection( Word* args, Word& result, int message,
                           Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  *((Rectangle<dim> *)result.addr) =
      ((Rectangle<dim>*)args[1].addr)->
      Intersection( *((Rectangle<dim>*)args[0].addr) );
  return (0);
}

/*
10.4.26 Value mapping functions of operator ~translate~

*/
template <unsigned dim>
int RectangleTranslate( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Rectangle<dim> *pResult = (Rectangle<dim> *)result.addr;
  Rectangle<dim> *r = ((Rectangle<dim>*)args[0].addr);
  double t[dim];
  Supplier son;
  Word arg;

  for( unsigned d = 0; d < dim; d++ )
  {
    son = qp->GetSon( args[1].addr, d );
    qp->Request( son, arg );
    t[d] = ((CcReal *)arg.addr)->GetRealval();
  }

  if( r->IsDefined() )
  {
    *pResult = *r;
    pResult->Translate( t );
    return (0);
  }
  else
  {
    pResult->SetDefined( false );
    return (0);
  }
}

/*
4.4.5 Value mapping functions of operator ~rectanglex~

*/
template<class T, unsigned int dim>
int RectangleValueMap( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  double min[dim];
  double max[dim];
  bool alldefined = true, checkminmax = true;

  result = qp->ResultStorage( s );

  for(unsigned int i=0; i < dim*2-1; i++) {
    if ( !(((T*)(args[i].addr))->IsDefined()) ) alldefined = false;
  }

  if ( alldefined )
  {
    for(unsigned int j=0; j < dim; j++) {
      min[j] = (double)(((T*)args[2*j].addr)->GetValue());
      max[j] = (double)(((T*)args[2*j+1].addr)->GetValue());
    }

    for(unsigned int k=0; k < dim; k++) {
      if ( !(min[k] <= max[k]) ) checkminmax =false;
    }

    if ( checkminmax )
    {
      ((Rectangle<dim> *)result.addr)->Set( true, min, max );
      return 0;
    }
  }
  ((Rectangle<dim> *)result.addr)->SetDefined( false );
  return (0);
}

/*
4.4.5 Value mapping functions of operator ~rectangle8size~

*/
template<class T, unsigned int dim>
int Rectangle8ValueMap( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  double min[dim+1];
  double max[dim+1];
  bool alldefined = true;

  result = qp->ResultStorage( s );
  for(unsigned int i=0; i <= dim; i++) {
    if ( !(((T*)(args[i].addr))->IsDefined()) ) alldefined = false;
  }

  if ( alldefined )
  {
    for(unsigned int j=0; j < dim; j++) {
      min[j] = (double)(((T*)args[j].addr)->GetValue());
      max[j] = ((double)(((T*)args[j].addr)->GetValue())) +
               ((double)(((CcReal*)args[dim].addr)->GetValue()));
    }
    ((Rectangle<dim> *)result.addr)->Set( true, min, max );
  }
  else
  {
    ((Rectangle<dim> *)result.addr)->Set( false, min, max );
  }
  return (0);
}

/*
4.4.6 Value mapping functions of operator ~distance~

*/
template<unsigned int dim>
int RectangleDistanceValueMap( Word* args, Word& result, int message,
                               Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcReal *res = (CcReal*) result.addr;
  Rectangle<dim> *r1, *r2;
  r1 = (Rectangle<dim> *) args[0].addr;
  r2 = (Rectangle<dim> *) args[1].addr;

  if( !r1->IsDefined() || !r2->IsDefined() )
    res->Set( false, 0.0 );
  else
  {
     res->Set( true, r1->Distance(*r2) );
  }
  return 0;
}

/*
4.4.7 Value mapping functions of operator ~rectproject~

*/
template<unsigned int dim>
int RectangleRectprojectValueMap( Word* args, Word& result, int message,
                                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Rectangle<2> *res = (Rectangle<2> *) result.addr;
  Rectangle<dim> *r;
  CcInt *ci1, *ci2;

  r = (Rectangle<dim> *) args[0].addr;
  ci1 = (CcInt*) args[1].addr;
  ci2 = (CcInt*) args[2].addr;

  if( !r->IsDefined() || !ci1->IsDefined() || !ci1->IsDefined())
    res->SetDefined( false );
  else
  {
     unsigned int i1 = ci1->GetIntval();
     unsigned int i2 = ci2->GetIntval();

     if( i1 <= dim && i1>0 && i2 <= dim && i2>0 )
     {
       double min[2] = {r->MinD(i1-1), r->MinD(i2-1)};
       double max[2] = {r->MaxD(i1-1), r->MaxD(i2-1)};
       res->Set( true, min, max );
     }
     else
     {
       ErrorReporter::ReportError("operator rectproject: dimension index "
           "out of bound!\n");
       res->SetDefined( false );
     }
  }
  return 0;
}

/*
4.4.8 Value mapping functions of operator ~minD~

*/
template<unsigned int dim>
    int RectangleMinDValueMap( Word* args, Word& result, int message,
                                      Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcReal *res = (CcReal *) result.addr;
  StandardSpatialAttribute<dim> *r
      = (StandardSpatialAttribute<dim> *) args[0].addr;
  CcInt *ci1 = (CcInt*) args[1].addr;

  if( !r->IsDefined() || !ci1->IsDefined() )
    res->SetDefined( false );
  unsigned int i1 = ci1->GetIntval();
  if ( i1 <= dim && i1>0 )
  {
    Rectangle<dim> bbx = r->BoundingBox();
    res->Set( true, bbx.MinD(i1-1) );
  }
  else
  {
    ErrorReporter::ReportError("operator minD: dimension index "
        "out of bounds!\n");
    res->SetDefined( false );
  }
  return 0;
}

/*
4.4.9 Value mapping functions of operator ~maxD~

*/
template<unsigned int dim>
    int RectangleMaxDValueMap( Word* args, Word& result, int message,
                                      Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcReal *res = (CcReal *) result.addr;
  StandardSpatialAttribute<dim> *r
      = (StandardSpatialAttribute<dim> *) args[0].addr;
  CcInt *ci1 = (CcInt*) args[1].addr;

  if( !r->IsDefined() || !ci1->IsDefined() )
    res->SetDefined( false );
  unsigned int i1 = ci1->GetIntval();
  if ( i1 <= dim && i1>0 )
  {
    Rectangle<dim> bbx = r->BoundingBox();
    res->Set( true, bbx.MaxD(i1-1) );
  }
  else
  {
    ErrorReporter::ReportError("operator maxD: dimension index "
        "out of bounds!\n");
    res->SetDefined( false );
  }
  return 0;
}

/*
4.4.10 Value mapping functions of operator ~bbox~

*/
template<unsigned int dim>
    int RectangleBboxValueMap( Word* args, Word& result, int message,
                               Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Rectangle<dim> *arg = static_cast<Rectangle<dim> *>(args[0].addr);
  Rectangle<dim> *res = static_cast<Rectangle<dim> *>(result.addr);
  *res = arg->BoundingBox();
  return 0;
}

/*
4.4.11 Value mapping functions of operator ~enlargeRect~

*/
template<unsigned int dim>
    int RectangleEnlargeRectValueMap( Word* args, Word& result, int message,
                                     Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Rectangle<dim> *arg = static_cast<Rectangle<dim> *>(args[0].addr);
  Rectangle<dim> *res = static_cast<Rectangle<dim> *>(result.addr);
  if(!arg->IsDefined()){
    res->SetDefined(false);
    return 0;
  }
  double newMin[dim], newMax[dim];
  for(unsigned int i = 0; i<dim; i++){
    newMin[i] = 0.0;
    newMax[i] = 0.0;
  }
  for(unsigned int i = 0; i<dim; i++){
    CcReal* deltaC = static_cast<CcReal*>(args[i+1].addr);
    if(!deltaC->IsDefined()){ // return undef result
      res->SetDefined(false);
      return 0;
    }
    double deltaD = deltaC->GetRealval();
    double minD = arg->MinD(i);
    double maxD = arg->MaxD(i);
    if( (deltaD < 0) && ((maxD-minD+2*deltaD) <= 0) ){
      // interval too small, set result to UNDEF
      res->SetDefined(false);
      return 0;
    } else {
      newMin[i] = minD - deltaD;
      newMax[i] = maxD + deltaD;
    }
  }
  res->Set(true, newMin, newMax);
  return 0;
}

/*
4.4.10 Value mapping functions of operator ~size~

*/
template<unsigned int dim>
    int RectangleSizeValueMap( Word* args, Word& result, int message,
                                     Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Rectangle<dim> *arg = static_cast<Rectangle<dim> *>(args[0].addr);
  CcReal *res = static_cast<CcReal*>(result.addr);
  double resval = arg->Size();
  res->Set( (resval >= -0.1), resval);
  return 0;
}

/*
4.5 Definition of operators

Definition of operators is done in a way similar to definition of
type constructors: an instance of class ~Operator~ is defined.

Because almost all operators are overloaded, we have first do define an array of value
mapping functions for each operator. For nonoverloaded operators there is also such and array
defined, so it easier to make them overloaded.

4.5.1 Definition of value mapping vectors

*/
ValueMapping rectangleisemptymap[] = { RectangleIsEmpty<2>,
                                       RectangleIsEmpty<3>,
                                       RectangleIsEmpty<4>,
                                       RectangleIsEmpty<8> };

ValueMapping rectangleequalmap[] = { RectangleEqual<2>,
                                     RectangleEqual<3>,
                                     RectangleEqual<4>,
                                     RectangleEqual<8> };

ValueMapping rectanglenotequalmap[] = { RectangleNotEqual<2>,
                                        RectangleNotEqual<3>,
                                        RectangleNotEqual<4>,
                                        RectangleNotEqual<8>};

ValueMapping rectangleintersectsmap[] = { RectangleIntersects<2>,
                                          RectangleIntersects<3>,
                                          RectangleIntersects<4>,
                                          RectangleIntersects<8> };

ValueMapping rectangleinsidemap[] = { RectangleInside<2>,
                                      RectangleInside<3>,
                                      RectangleInside<4>,
                                      RectangleInside<8> };

ValueMapping rectangletranslatemap[] = { RectangleTranslate<2>,
                                         RectangleTranslate<3>,
                                         RectangleTranslate<4> };

ValueMapping rectangleunionmap[] = { RectangleUnion<2>,
                                     RectangleUnion<3>,
                                     RectangleUnion<4>,
                                     RectangleUnion<8> };

ValueMapping rectangleintersectionmap[] = { RectangleIntersection<2>,
                                            RectangleIntersection<3>,
                                            RectangleIntersection<4>,
                                            RectangleIntersection<4> };

ValueMapping rectanglerectangle2map[] = { RectangleValueMap<CcInt, 2>,
                                          RectangleValueMap<CcReal, 2> };

ValueMapping rectanglerectangle3map[] = { RectangleValueMap<CcInt, 3>,
                                          RectangleValueMap<CcReal, 3> };

ValueMapping rectanglerectangle4map[] = { RectangleValueMap<CcInt, 4>,
                                          RectangleValueMap<CcReal, 4> };

ValueMapping rectanglerectangle8map[] = { Rectangle8ValueMap<CcInt, 8>,
                                          Rectangle8ValueMap<CcReal, 8> };

ValueMapping rectangledistancemap[] = { RectangleDistanceValueMap<2>,
                                        RectangleDistanceValueMap<3>,
                                        RectangleDistanceValueMap<4>,
                                        RectangleDistanceValueMap<8> };

ValueMapping rectanglerectprojectmap[] =
{
  RectangleRectprojectValueMap<2>,
  RectangleRectprojectValueMap<3>,
  RectangleRectprojectValueMap<4>,
  RectangleRectprojectValueMap<8>};

ValueMapping rectangleMaxDmap[] =
{
  RectangleMaxDValueMap<2>,
  RectangleMaxDValueMap<3>,
  RectangleMaxDValueMap<4>,
  RectangleMaxDValueMap<8>
};

ValueMapping rectangleMinDmap[] =
{
  RectangleMinDValueMap<2>,
  RectangleMinDValueMap<3>,
  RectangleMinDValueMap<4>,
  RectangleMinDValueMap<8>
};

ValueMapping rectangleBboxmap[] =
{
  RectangleBboxValueMap<2>,
  RectangleBboxValueMap<3>,
  RectangleBboxValueMap<4>,
  RectangleBboxValueMap<8>
};

ValueMapping rectangleEnlargeRectmap[] =
{
  RectangleEnlargeRectValueMap<2>,
  RectangleEnlargeRectValueMap<3>,
  RectangleEnlargeRectValueMap<4>,
  RectangleEnlargeRectValueMap<8>
};

ValueMapping rectangleSizemap[] =
{
  RectangleSizeValueMap<2>,
  RectangleSizeValueMap<3>,
  RectangleSizeValueMap<4>,
  RectangleSizeValueMap<8>
};

/*
4.5.2 Definition of specification strings

*/
const string RectangleSpecIsEmpty  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
           "( <text>rect<d> -> bool</text---> <text>isempty ( _ )</text--->"
             "<text>Returns whether the value is defined or not.</text--->"
             "<text>query isempty ( rect1 )</text--->"
             ") )";

const string RectangleSpecEqual  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
        "( <text>(rect<d> x rect<d>) -> bool</text--->"
        "<text>_ = _</text--->"
        "<text>Equal.</text--->"
        "<text>query rect1 = rect</text--->"
        ") )";

const string RectangleSpecNotEqual  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
        "( <text>(rect<d> x rect<d>) -> bool</text--->"
        "<text>_ # _</text--->"
        "<text>Not equal.</text--->"
        "<text>query rect1 # rect</text--->"
        ") )";

const string RectangleSpecIntersects  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
        "( <text>(rect<d> x rect<d>) -> bool </text--->"
        "<text>_ intersects _</text--->"
        "<text>Intersects.</text--->"
        "<text>query rect1 intersects rect</text--->"
        ") )";

const string RectangleSpecInside  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
        "( <text>(rect<d> x rect<d>) -> bool</text--->"
        "<text>_ inside _</text--->"
        "<text>Inside.</text--->"
        "<text>query rect1 inside rect</text--->"
        ") )";

const string RectangleSpecUnion  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
        "( <text>(rect<d> x rect<d>) -> rect<d></text--->"
        "<text>_ union _</text--->"
        "<text>return the union of two rectangles.</text--->"
        "<text>query rect1 union rect2</text--->"
        ") )";

const string RectangleSpecIntersection  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
        "( <text>(rect<d> x rect<d>) -> rect<d></text--->"
        "<text>_ intersection _</text--->"
        "<text>return the intersection of two rectangles.</text--->"
        "<text>query rect1 intersection rect2</text--->"
        ") )";

const string RectangleSpecTranslate  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(rect (real real)) -> rect "
  "(rect3 (real real real)) -> rect3 "
  "(rect4 (real real real real)) -> rect4</text--->"
  "<text> _ translate[list]</text--->"
  "<text> move the rectangle parallely for some distance.</text--->"
  "<text> query rect1 translate[3.5, 15.1]</text--->"
  ") )";

const string RectangleSpecRectangle2  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" \"Remarks\")"
        "( <text>(int x int x int x int -> rect) or"
        " (real x real x real x real -> rect)</text--->"
        "<text>rectangle2( xmin, xmax, ymin, ymax)</text--->"
        "<text>creates a rect from the given parameters.</text--->"
        "<text>query rectangle2(17.0, 24.3, 12.0, 13.1)</text--->"
        "<text>The sequence of parameters must be "
        "(minx, maxx, miny, maxy) with (minx < maxx) and"
        " (miny < maxy).</text--->"
        ") )";

const string RectangleSpecRectangle3  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" \"Remarks\")"
        "( <text>(int x int x int x int x int x int -> rect3) or"
        " (real x real x real x real x real x real -> rect3)</text--->"
        "<text>rectangle3( xmin, xmax, ymin, ymax, zmin, zmax)</text--->"
        "<text>creates a rect3 from the given parameters.</text--->"
        "<text>query rectangle3(17.0, 24.0, 12.0, 13.1, 4.41, 6.18)</text--->"
        "<text>The sequence of parameters must be "
        "(minx, maxx, miny, maxy, minz, maxz) with (minx < maxx) and"
        " (miny < maxy) and (minz < maxz).</text--->"
        ") )";

const string RectangleSpecRectangle4  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" \"Remarks\")"
        "( <text>(int^8 -> rect4) or (real^8 -> rect4)"
        "</text--->"
        "<text>rectangle4( d1min,d1max,d2min,d2max,d3min,d3max,d4min,d4max)"
        "</text--->"
        "<text>creates a rect4 from the given parameters.</text--->"
        "<text>query rectangle3(17.0, 24.0, 12.0, 13.1, "
        "4.41, 6.18, 2.3, 3.74)</text--->"
        "<text>The sequence of parameters must be "
        "(minx, maxx, miny, maxy, minz, maxz, min4d, max4d)"
        " with (minx < maxx) and (miny < maxy) and (minz < maxz)"
        " and (min4d < max4d).</text--->"
        ") )";

  /*
const string RectangleSpecRectangle8  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" \"Remarks\")"
        "( <text>(int^16 -> rect8) or (real^16 -> rect8)</text--->"
        "<text>rectangle8( d1min,d1max,d2min,d2max,d3min,d3max,d4min,d4max, "
        "d5min,d5max,d6min,d6max,d7min,d7max,d8min,d8max)</text--->"
        "<text>creates a rect8 from the given parameters.</text--->"
        "<text>query rectangle8(1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8)</text--->"
        "<text>The sequence of parameters must be "
        "(d1min,d1max,d2min,d2max,d3min,d3max,d4min,d4max, d5min,d5max,d6min,"
        "d6max,d7min,d7max,d8min,d8max).</text--->"
        ") )";
  */

const string RectangleSpecRectangle8  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" \"Remarks\")"
        "( <text>(int x int x int x int x int x int x int x int x real -> "
        "rect8) or (real x real x real x real x real x real x real x "
        "real x real -> rect8)</text--->"
        "<text>rectangle8( min1d,min2d,min3d,min4d,min5d,min6d,min7d,min8d, "
        "size )</text--->"
        "<text>creates a rect8 from the given parameters with minxd as lower "
        "bound and size computing the upper bound (minxd+size) of the "
        "respective axis intervals.</text--->"
        "<text>query rectangle8(1,2,3,4,5,6,7,8, 1.2)</text--->"
        "<text>The sequence of parameters must be "
        "(min1d, min2d, min3d, min4d, min5d, min6d, min7d, min8d, size)"
        " with minxd as lower bound and size computing the upper bound "
        "(minxd+size) of the respective axis intervals.</text--->"
        ") )";

const string RectangleSpecDistance  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" \"Remarks\")"
        "( <text>(rect<d> x rect<d>) -> real</text--->"
        "<text>distance( _ , _ )</text--->"
        "<text>return the euclidean distance of two rectangles.</text--->"
        "<text>query distance(rect1, rect2)</text--->"
        "<text></text--->"
        ") )";

const string RectangleSpecRectproject  =
        "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" \"Remarks\")"
        "( <text>(rect<d> x int x int) -> rect</text--->"
        "<text>rectproject( rect , d1 , d2 )</text--->"
        "<text>projects the rectangle 'rect' to its dimensions "
        "'d1' and 'd2'. 1 <= d1, d2 <= D.</text--->"
        "<text>query rectproject([const rect3 value "
        "(1.0 2.0 3.0 4.0 5.0 6.0) ], 1, 3)</text--->"
        "<text></text--->"
        ") )";

const string RectangleSpecMinD  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" \"Remarks\")"
    "( <text>(rect<d> x int) -> real</text--->"
    "<text>minD( rect , d )</text--->"
    "<text>Return the minimum value of the object's bounding box with "
    "respect to the Dth dimension. "
    "1 <= d <= D.</text--->"
    "<text>query minD([const rect3 value "
    "(1.0 2.0 3.0 4.0 5.0 6.0) ], 2)</text--->"
    "<text></text--->"
    ") )";

const string RectangleSpecMaxD  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" \"Remarks\")"
    "( <text>(rect<d> x int) -> real</text--->"
    "<text>maxD( rect , d )</text--->"
    "<text>Return the maximum value of the value's bounding box "
    "with respect to the Dth dimension. "
    "1 <= d <= D.</text--->"
    "<text>query maxD([const rect3 value "
    "(1.0 2.0 3.0 4.0 5.0 6.0) ], 2)</text--->"
    "<text></text--->"
    ") )";

const string RectangleSpecBbox  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" \"Remarks\")"
    "( <text>rect<d> -> rect<d></text--->"
    "<text>bbox( rect )</text--->"
    "<text>Returns the minimum bounding box of the n-dimensional rectangle "
    "'rect', which is a clone of the argument.</text--->"
    "<text>query bbox([const rect3 value "
    "(1.0 2.0 3.0 4.0 5.0 6.0) ], 2)</text--->"
    "<text></text--->"
    ") )";

const string RectangleSpecEnlargeRect  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" \"Remarks\")"
    "( <text>rect<d> x real^d -> rect<d></text--->"
    "<text>enlargeRect( rect, m1, ... , md )</text--->"
    "<text>Enlarges (shrinks) the rectangle by a value mi to each side "
    "in each dimension.</text--->"
    "<text>query enlargeRect([const rect3 value "
    "(1.0 2.0 3.0 4.0 5.0 6.0) ], 2.0, 3.0, -1.0)</text--->"
    "<text></text--->"
    ") )";

const string RectangleSpecSize  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" \"Remarks\")"
    "( <text>rect<d> -> real</text--->"
    "<text>size( rect )</text--->"
    "<text>Caculates the rectangle's area (resp. D-dimensional volume)."
    "</text--->"
    "<text>query size([const rect3 value "
    "(1.0 2.0 3.0 4.0 5.0 6.0) ], 2.0, 3.0, -1.0)</text--->"
    "<text></text--->"
    ") )";

/*
4.5.3 Definition of the operators

*/
Operator rectangleisempty( "isempty",
                           RectangleSpecIsEmpty,
                           4,
                           rectangleisemptymap,
                           RectangleUnarySelect,
                           RectTypeMapBool );

Operator rectangleequal( "=",
                         RectangleSpecEqual,
                         4,
                         rectangleequalmap,
                         RectangleBinarySelect,
                         RectRectTypeMapBool );

Operator rectanglenotequal( "#",
                            RectangleSpecNotEqual,
                            4,
                            rectanglenotequalmap,
                            RectangleBinarySelect,
                            RectRectTypeMapBool );

Operator rectangleintersects( "intersects",
                              RectangleSpecIntersects,
                              4,
                              rectangleintersectsmap,
                              RectangleBinarySelect,
                              RectRectTypeMapBool );

Operator rectangleinside( "inside",
                          RectangleSpecInside,
                          4,
                          rectangleinsidemap,
                          RectangleBinarySelect,
                          RectRectTypeMapBool );

Operator rectangleunion( "union",
                          RectangleSpecUnion,
                          4,
                          rectangleunionmap,
                          RectangleBinarySelect,
                          RectRectTypeMapRect );

Operator rectangleintersection( "intersection",
                                RectangleSpecIntersection,
                                4,
                                rectangleintersectionmap,
                                RectangleBinarySelect,
                                RectRectTypeMapRect );

Operator rectangletranslate( "translate",
                             RectangleSpecTranslate,
                             3,
                             rectangletranslatemap,
                             RectangleUnarySelect,
                             TranslateTypeMap );

Operator rectanglerectangle2( "rectangle2",
                             RectangleSpecRectangle2,
                             2,
                             rectanglerectangle2map,
                             RectangleSelect<2>,
                             RectangleTypeMap<2> );

Operator rectanglerectangle3( "rectangle3",
                             RectangleSpecRectangle3,
                             2,
                             rectanglerectangle3map,
                             RectangleSelect<3>,
                             RectangleTypeMap<3> );

Operator rectanglerectangle4( "rectangle4",
                             RectangleSpecRectangle4,
                             2,
                             rectanglerectangle4map,
                             RectangleSelect<4>,
                             RectangleTypeMap<4> );

Operator rectanglerectangle8( "rectangle8",
                             RectangleSpecRectangle8,
                             2,
                             rectanglerectangle8map,
                             Rectangle8Select<8>,
                             Rectangle8TypeMap<8> );

Operator rectangledistance( "distance",
                          RectangleSpecDistance,
                          4,
                          rectangledistancemap,
                          RectangleBinarySelect,
                          RectRectTypeMapReal );

Operator rectanglerectproject( "rectproject",
                          RectangleSpecRectproject,
                          4,
                          rectanglerectprojectmap,
                          RectangleUnarySelect,
                          RectProjectTypeMap );

Operator rectangleminD( "minD",
                         RectangleSpecMinD,
                         4,
                         rectangleMinDmap,
                         RectangleMinMaxDSelect,
                         RectangleMinMaxTypeMap );

Operator rectanglemaxD( "maxD",
                        RectangleSpecMaxD,
                        4,
                        rectangleMaxDmap,
                        RectangleMinMaxDSelect,
                        RectangleMinMaxTypeMap );

Operator rectanglebbox( "bbox",
                        RectangleSpecBbox,
                        4,
                        rectangleBboxmap,
                        RectangleUnarySelect,
                        RectangleTypeMapRectRect );

Operator rectangleextendrect( "enlargeRect",
                        RectangleSpecEnlargeRect,
                        4,
                        rectangleEnlargeRectmap,
                        RectangleUnarySelect,
                        RectangleTypeMapEnlargeRect );

Operator rectanglesize( "size",
                        RectangleSpecSize,
                        4,
                        rectangleSizemap,
                        RectangleUnarySelect,
                        RectTypeMapReal );

/*
5 Creating the Algebra

*/

class RectangleAlgebra : public Algebra
{
 public:
  RectangleAlgebra() : Algebra()
  {
    AddTypeConstructor( &rect );
    AddTypeConstructor( &rect3 );
    AddTypeConstructor( &rect4 );
    AddTypeConstructor( &rect8 );

    rect.AssociateKind("DATA");
    rect3.AssociateKind("DATA");
    rect4.AssociateKind("DATA");
    rect8.AssociateKind("DATA");

    AddOperator( &rectangleisempty );
    AddOperator( &rectangleequal );
    AddOperator( &rectanglenotequal );
    AddOperator( &rectangleintersects );
    AddOperator( &rectangleinside );
    AddOperator( &rectangleunion );
    AddOperator( &rectangleintersection );
    AddOperator( &rectangletranslate );
    AddOperator( &rectangledistance );
    AddOperator( &rectanglerectangle2 );
    AddOperator( &rectanglerectangle3 );
    AddOperator( &rectanglerectangle4 );
    AddOperator( &rectanglerectangle8 );
    AddOperator( &rectanglerectproject );
    AddOperator( &rectangleminD );
    AddOperator( &rectanglemaxD );
    AddOperator( &rectanglebbox );
    AddOperator( &rectangleextendrect );
    AddOperator( &rectangleextendrect );
    AddOperator( &rectanglesize );
  }
  ~RectangleAlgebra() {};
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
InitializeRectangleAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return (new RectangleAlgebra());
}


