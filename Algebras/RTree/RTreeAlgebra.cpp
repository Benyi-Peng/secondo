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

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[TOC] [\tableofcontents]

[1] Implementation of R-Tree Algebra

July 2003, Victor Almeida

October 2004, Herbert Schoenhammer, tested and divided in Header-File
and Implementation-File. Also R-Trees with three and four dimensions
are created.

December 2005, Victor Almeida deleted the deprecated algebra levels
(~executable~, ~descriptive~, and ~hibrid~). Only the executable
level remains. Models are also removed from type constructors.

[TOC]

0 Overview

This Algebra implements the creation of two-, three- and four-dimensional
R-Trees.

First the the type constructor ~rtree~, ~rtree3~ and ~rtree4~ are defined.

Second the operator ~creatertree~ to build the trees is defined. This operator
expects an attribute in the relation that implements the kind ~SPATIAL2D~,
~SPATIAL3D~, or ~SPATIAL4D~. Only the bounding boxes of the values of this
attribute are indexed in the R-Tree.

Finally, the operator ~windowintersects~ retrieves the tuples of the original
relation which bounding boxes of the indexed attribute intersect the window
(of type ~rect~, ~rect3~, or ~rect4~) given as argument to the operator.

1 Defines and Includes

*/
#include <iostream>
#include <stack>

using namespace std;

#include "SpatialAlgebra.h"
#include "RelationAlgebra.h"
#include "TemporalAlgebra.h"
#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include "RectangleAlgebra.h"
#include "RTreeAlgebra.h"
#include "CPUTimeMeasurer.h"
#include "TupleIdentifier.h"

extern NestedList* nl;
extern QueryProcessor* qp;

#define BBox Rectangle

#define CHECK_COND(cond, msg) \
  if(!(cond)) \
  {\
    ErrorReporter::ReportError(msg);\
    return nl->SymbolAtom("typeerror");\
  };

/*
Implementation of Functions and Procedures

*/
int myCompare( const void* a, const void* b )
{
  if( ((SortedArrayItem *) a)->pri < ((SortedArrayItem *) b)->pri )
    return -1;
  else if( ((SortedArrayItem *) a)->pri > 
           ((SortedArrayItem *) b)->pri )
    return 1;
  else
    return 0;
}

/*
1 Type constructor ~rtree~

1.1 Type property of type constructor ~rtree~

*/
ListExpr RTree2Prop()
{
  ListExpr examplelist = nl->TextAtom();
  nl->AppendText(examplelist, 
    "<relation> creatertree [<attrname>]"
    " where <attrname> is the key of type rect");

  return
    (nl->TwoElemList(
         nl->TwoElemList(nl->StringAtom("Creation"),
                         nl->StringAtom("Example Creation")),
         nl->TwoElemList(examplelist,
                         nl->StringAtom("(let myrtree = countries"
                         " creatertree [boundary])"))));
}

/*
1.8 ~Check~-function of type constructor ~rtree~

*/
bool CheckRTree2(ListExpr type, ListExpr& errorInfo)
{
  AlgebraManager* algMgr;

  if((!nl->IsAtom(type))
    && (nl->ListLength(type) == 4)
    && nl->Equal(nl->First(type), nl->SymbolAtom("rtree")))
  {
    algMgr = SecondoSystem::GetAlgebraManager();
    return
      algMgr->CheckKind("TUPLE", nl->Second(type), errorInfo) &&
      (nl->IsEqual(nl->Third(type), "rect") ||
      algMgr->CheckKind("SPATIAL2D", nl->Third(type), errorInfo)) &&
      nl->IsAtom(nl->Fourth(type)) && 
      nl->AtomType(nl->Fourth(type)) == BoolType;
  }
  else
  {
    errorInfo = nl->Append(errorInfo,
      nl->ThreeElemList(
        nl->IntAtom(60), 
        nl->SymbolAtom("RTREE2"), 
        type));
    return false;
  }
  return true;
}

/*
1.12 Type Constructor object for type constructor ~rtree3~

*/
TypeConstructor rtree( "rtree",              
                        RTree2Prop,
                        OutRTree<2>, 
                        InRTree<2>,
                        0,                
                        0,
                        CreateRTree<2>,
                        DeleteRTree<2>,
                        OpenRTree<2>,
                        SaveRTree<2>,
                        CloseRTree<2>,
                        CloneRTree<2>,
                        CastRTree<2>,
                        SizeOfRTree<2>,
                        CheckRTree2 );

/*
2 Type constructor ~rtree3~

2.1 Type property of type constructor ~rtree3~

*/
ListExpr RTree3Prop()
{
  ListExpr examplelist = nl->TextAtom();
  nl->AppendText(examplelist, 
    "<relation> creatertree [<attrname>]"
    " where <attrname> is the key of type rect3");

  return
    (nl->TwoElemList(
         nl->TwoElemList(nl->StringAtom("Creation"),
                         nl->StringAtom("Example Creation")),
         nl->TwoElemList(examplelist,
                         nl->StringAtom("(let myrtree = countries"
                         " creatrtree [boundary])"))));
}

/*
2.8 ~Check~-function of type constructor ~rtree3~

*/
bool CheckRTree3(ListExpr type, ListExpr& errorInfo)
{
  AlgebraManager* algMgr;

  if((!nl->IsAtom(type))
    && (nl->ListLength(type) == 3)
    && nl->Equal(nl->First(type), nl->SymbolAtom("rtree3")))
  {
    algMgr = SecondoSystem::GetAlgebraManager();
    return
      algMgr->CheckKind("TUPLE", nl->Second(type), errorInfo) &&
      algMgr->CheckKind("SPATIAL3D", nl->Third(type), errorInfo);
  }
  else
  {
    errorInfo = nl->Append(errorInfo,
      nl->ThreeElemList(
        nl->IntAtom(60), 
        nl->SymbolAtom("RTREE3"), 
        type));
    return false;
  }
  return true;
}

/*
1.12 Type Constructor object for type constructor ~rtree3~

*/
TypeConstructor rtree3( "rtree3",
                        RTree3Prop,
                        OutRTree<3>,
                        InRTree<3>,
                        0,
                        0,
                        CreateRTree<3>,
                        DeleteRTree<3>,
                        OpenRTree<3>,
                        SaveRTree<3>,
                        CloseRTree<3>,
                        CloneRTree<3>,
                        CastRTree<3>,
                        SizeOfRTree<3>,
                        CheckRTree3 );

/*
3 Type constructor ~rtree4~

3.1 Type property of type constructor ~rtree4~

*/
ListExpr RTree4Prop()
{
  ListExpr examplelist = nl->TextAtom();
  nl->AppendText(examplelist,
    "<relation> creatertree [<attrname>]"
    " where <attrname> is the key of type rect4");

  return (nl->TwoElemList(
            nl->TwoElemList(
              nl->StringAtom("Creation"),
              nl->StringAtom("Example Creation")),
            nl->TwoElemList(
              examplelist,
              nl->StringAtom("(let myrtree = countries"
                             " creatertree [boundary])"))));
}

/*
3.8 ~Check~-function of type constructor ~rtree4~

*/
bool CheckRTree4(ListExpr type, ListExpr& errorInfo)
{
  AlgebraManager* algMgr;

  if((!nl->IsAtom(type))
    && (nl->ListLength(type) == 3)
    && nl->Equal(nl->First(type), nl->SymbolAtom("rtree4")))
  {
    algMgr = SecondoSystem::GetAlgebraManager();
    return
      algMgr->CheckKind("TUPLE", nl->Second(type), errorInfo)
      &&nl->Equal(nl->Third(type), nl->SymbolAtom("rect4"));
  }
  else
  {
    errorInfo = nl->Append(errorInfo,
      nl->ThreeElemList(
        nl->IntAtom(60), 
        nl->SymbolAtom("RTREE4"), 
        type));
    return false;
  }
  return true;
}

/*
3.12 Type Constructor object for type constructor ~rtree~

*/
TypeConstructor rtree4( "rtree4",
                        RTree4Prop,
                        OutRTree<4>,
                        InRTree<4>,
                        0,
                        0,
                        CreateRTree<4>,
                        DeleteRTree<4>,
                        OpenRTree<4>,
                        SaveRTree<4>,
                        CloseRTree<4>,
                        CloneRTree<4>,
                        CastRTree<4>,
                        SizeOfRTree<4>,
                        CheckRTree4 );


/*
7 Operators of the RTree Algebra

7.1 Operator ~creatertree~

The operator ~creatrtree~ creates a R-Tree for a given Relation. The exact type
of the desired R-Tree is defind in RTreeAlgebra.h. The variables ~do\_linear\_split~,
~do\_quadratic\_split~ or ~do\_axis\_split~ are defining, whether a R-Tree (Guttman 84) or
a R[*]-Tree is generated.

The following operator ~creatertree~ accepts relations with tuples of (at least) one
attribute of kind ~SPATIAL2D~, ~SPATIAL3D~ or ~SPATIAL4D~ or ~rect~, ~rect3~, and ~rect4~.
The attribute name is specified as argument of the operator.

7.1.1 Type Mapping of operator ~creatertree~

*/
ListExpr CreateRTreeTypeMap(ListExpr args)
{
  string attrName, relDescriptionStr, argstr;
  string errmsg = "Incorrect input for operator creatertree.";
  int attrIndex;
  ListExpr attrType;
  AlgebraManager* algMgr = SecondoSystem::GetAlgebraManager();
  ListExpr errorInfo = nl->OneElemList( nl->SymbolAtom( "ERRORS" ) );

  CHECK_COND(!nl->IsEmpty(args) &&
    nl->ListLength(args) == 2,
    errmsg + "\nOperator creatertree expects two arguments.");

  ListExpr relDescription = nl->First(args),
           attrNameLE = nl->Second(args);

  CHECK_COND(nl->IsAtom(attrNameLE) &&
    nl->AtomType(attrNameLE) == SymbolType,
    errmsg + "\nThe second argument must be the name of "
    "the attribute to index.");
  attrName = nl->SymbolValue(attrNameLE);

  nl->WriteToString (relDescriptionStr, relDescription);
  CHECK_COND(!nl->IsEmpty(relDescription) &&
    nl->ListLength(relDescription) == 2,
    errmsg + 
    "\nOperator creatertree expects a first argument with structure "
    "(rel (tuple ((a1 t1)...(an tn)))) or "
    "(stream (tuple ((a1 t1)...(an tn))))\n"
    "but gets it with structure '" + relDescriptionStr + "'.");

  ListExpr tupleDescription = nl->Second(relDescription);

  CHECK_COND((nl->IsEqual(nl->First(relDescription), "rel") ||
    nl->IsEqual(nl->First(relDescription), "stream")) &&
    nl->ListLength(tupleDescription) == 2,
    errmsg + 
    "\nOperator creatertree expects a first argument with structure "
    "(rel (tuple ((a1 t1)...(an tn)))) or "
    "(stream (tuple ((a1 t1)...(an tn))))\n"
    "but gets it with structure '" + relDescriptionStr + "'.");

  ListExpr attrList = nl->Second(tupleDescription);
  CHECK_COND(nl->IsEqual(nl->First(tupleDescription), "tuple") &&
    IsTupleDescription(attrList),
    errmsg + 
    "\nOperator creatertree expects a first argument with structure "
    "(rel (tuple ((a1 t1)...(an tn)))) or "
    "(stream (tuple ((a1 t1)...(an tn))))\n"
    "but gets it with structure '" + relDescriptionStr + "'.");

  CHECK_COND(
    (attrIndex = FindAttribute(attrList, attrName, attrType)) > 0,
    errmsg + 
    "\nOperator creatertree expects that the attribute " + 
    attrName + "\npassed as second argument to be part of "
    "the relation or stream description\n'" + 
    relDescriptionStr + "'.");

  CHECK_COND(algMgr->CheckKind("SPATIAL2D", attrType, errorInfo)||
    algMgr->CheckKind("SPATIAL3D", attrType, errorInfo)||
    algMgr->CheckKind("SPATIAL4D", attrType, errorInfo)||
    nl->IsEqual(attrType, "rect")||
    nl->IsEqual(attrType, "rect3")||
    nl->IsEqual(attrType, "rect4"),
    errmsg + 
    "\nOperator creatertree expects that attribute "+attrName+"\n"
    "belongs to kinds SPATIAL2D, SPATIAL3D, or SPATIAL4D\n"
    "or rect, rect3, and rect4.");

  string rtreetype;

  if ( algMgr->CheckKind("SPATIAL2D", attrType, errorInfo) ||
       nl->IsEqual( attrType, "rect" ) )
    rtreetype = "rtree";
  else if ( algMgr->CheckKind("SPATIAL3D", attrType, errorInfo) ||
            nl->IsEqual( attrType, "rect3" ) )
    rtreetype = "rtree3";
  else if ( algMgr->CheckKind("SPATIAL4D", attrType, errorInfo) ||
       nl->IsEqual( attrType, "rect4" ) )
    rtreetype = "rtree4";

  if( nl->IsEqual(nl->First(relDescription), "rel") )
  {
    return
      nl->ThreeElemList(
        nl->SymbolAtom("APPEND"),
        nl->OneElemList(
          nl->IntAtom(attrIndex)),
        nl->FourElemList(
          nl->SymbolAtom(rtreetype),
          tupleDescription,
          attrType,
          nl->BoolAtom(false)));
  }
  else // nl->IsEqual(nl->First(relDescription), "stream")
  {
/*
Here we can have two possibilities:

- multi-entry indexing, or
- double indexing

For multi-entry indexing, one and only one of the attributes
must be a tuple identifier. In the latter, together with
a tuple identifier, the last two attributes must be of
integer type (~int~).
 
In the first case, a standard R-Tree is created possibly
containing several entries to the same tuple identifier, and 
in the latter, a double index R-Tree is created using as low 
and high parameters these two last integer numbers.

*/
    ListExpr first, rest, newAttrList, lastNewAttrList;
    int tidIndex = 0;
    string type;
    bool firstcall = true,
         doubleIndex = false;

    int nAttrs = nl->ListLength( attrList );
    rest = attrList;
    int j = 1;
    while (!nl->IsEmpty(rest))
    {
      first = nl->First(rest);
      rest = nl->Rest(rest);

      type = nl->SymbolValue(nl->Second(first));
      if (type == "tid")
      {
        CHECK_COND( tidIndex == 0,
          "Operator creatertree expects as first argument a stream "
          "with\none and only one attribute of type 'tid'\n'"
          "but gets\n'" + relDescriptionStr + "'.");

        tidIndex = j;
      }
      else if( j == nAttrs - 1 && type == "int" && 
               nl->SymbolValue(
                 nl->Second(nl->First(rest))) == "int" )
      { // the last two attributes are integers
        doubleIndex = true;
      }
      else
      {
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
      j++;
    }
    CHECK_COND( tidIndex != 0,
      "Operator creatertree expects as first argument a stream "
      "with\none and only one attribute of type 'tid'\n'"
      "but gets\n'" + relDescriptionStr + "'.");

    return
      nl->ThreeElemList(
        nl->SymbolAtom("APPEND"),
        nl->TwoElemList(
          nl->IntAtom(attrIndex),
          nl->IntAtom(tidIndex)),
        nl->FourElemList(
          nl->SymbolAtom(rtreetype),
          nl->TwoElemList(
            nl->SymbolAtom("tuple"),
            newAttrList),
          attrType,
          nl->BoolAtom(doubleIndex)));
  }
}

/*
4.1.2 Selection function of operator ~creatertree~

*/
int
CreateRTreeSelect (ListExpr args)
{
  ListExpr relDescription = nl->First(args),
           attrNameLE = nl->Second(args),
           tupleDescription = nl->Second(relDescription),
           attrList = nl->Second(tupleDescription);
  string attrName = nl->SymbolValue(attrNameLE);

  int attrIndex;
  ListExpr attrType;
  attrIndex = FindAttribute(attrList, attrName, attrType);

  AlgebraManager* algMgr = SecondoSystem::GetAlgebraManager();
  ListExpr errorInfo = nl->OneElemList( nl->SymbolAtom( "ERRORS" ) );

  int result;
  if ( algMgr->CheckKind("SPATIAL2D", attrType, errorInfo) )
    result = 0;
  else if ( algMgr->CheckKind("SPATIAL3D", attrType, errorInfo) )
    result = 1;
  else if ( algMgr->CheckKind("SPATIAL4D", attrType, errorInfo) )
    result = 2;
  else if( nl->SymbolValue(attrType) == "rect" )
    result = 3;
  else if( nl->SymbolValue(attrType) == "rect3" )
    result = 4;
  else if( nl->SymbolValue(attrType) == "rect4" )
    result = 5;
  else
    return -1; /* should not happen */

  if( nl->SymbolValue(nl->First(relDescription)) == "rel")
    return result;
  if( nl->SymbolValue(nl->First(relDescription)) == "stream")
  {
    ListExpr first, 
             rest = attrList;
    while (!nl->IsEmpty(rest))
    {
      first = nl->First(rest);
      rest = nl->Rest(rest);
    }
    if( nl->IsEqual( nl->Second( first ), "int" ) )
      // Double indexing
      return result + 12;
    else 
      // Multi-entry indexing
      return result + 6;
  }

  return -1;
}

/*
4.1.3 Value mapping function of operator ~creatertree~

*/
template<unsigned dim>
int CreateRTreeRelSpatial(Word* args, Word& result, int message, 
                          Word& local, Supplier s)
{
  Relation* relation;
  int attrIndex;
  RelationIterator* iter;
  Tuple* tuple;

  R_Tree<dim, TupleId> *rtree = 
    (R_Tree<dim, TupleId>*)qp->ResultStorage(s).addr;
  result = SetWord( rtree );

  relation = (Relation*)args[0].addr;
  attrIndex = ((CcInt*)args[2].addr)->GetIntval() - 1;

  iter = relation->MakeScan();
  while( (tuple = iter->GetNextTuple()) != 0 )
  {
    if( ((StandardSpatialAttribute<dim>*)tuple->
          GetAttribute(attrIndex))->IsDefined() )
    {
      BBox<dim> box = ((StandardSpatialAttribute<dim>*)tuple->
                        GetAttribute(attrIndex))->BoundingBox();
      R_TreeLeafEntry<dim, TupleId> e( box, tuple->GetTupleId() );
      rtree->Insert( e );
    }
    tuple->DeleteIfAllowed();
  }
  delete iter;

  return 0;
}

template<unsigned dim>
int CreateRTreeStreamSpatial(Word* args, Word& result, int message, 
                             Word& local, Supplier s)
{
  Word wTuple;
  R_Tree<dim, TupleId> *rtree = 
    (R_Tree<dim, TupleId>*)qp->ResultStorage(s).addr;
  result = SetWord( rtree );

  int attrIndex = ((CcInt*)args[2].addr)->GetIntval() - 1,
      tidIndex = ((CcInt*)args[3].addr)->GetIntval() - 1;

  qp->Open(args[0].addr);
  qp->Request(args[0].addr, wTuple);
  while (qp->Received(args[0].addr))
  {
    Tuple* tuple = (Tuple*)wTuple.addr;

    if( ((StandardSpatialAttribute<dim>*)tuple->
          GetAttribute(attrIndex))->IsDefined() &&
        ((TupleIdentifier *)tuple->GetAttribute(tidIndex))->
          IsDefined() )
    {
      BBox<dim> box = ((StandardSpatialAttribute<dim>*)tuple->
                        GetAttribute(attrIndex))->BoundingBox();
      R_TreeLeafEntry<dim, TupleId> 
        e( box, 
           ((TupleIdentifier *)tuple->
             GetAttribute(tidIndex))->GetTid() );
      rtree->Insert( e );
    }
    tuple->DeleteIfAllowed();
    qp->Request(args[0].addr, wTuple);
  }
  qp->Close(args[0].addr);

  return 0;
}

template<unsigned dim>
int CreateRTreeRelRect(Word* args, Word& result, int message, 
                       Word& local, Supplier s)
{
  Relation* relation;
  int attrIndex;
  RelationIterator* iter;
  Tuple* tuple;

  R_Tree<dim, TupleId> *rtree = 
    (R_Tree<dim, TupleId>*)qp->ResultStorage(s).addr;
  result = SetWord( rtree );

  relation = (Relation*)args[0].addr;
  attrIndex = ((CcInt*)args[2].addr)->GetIntval() - 1;

  iter = relation->MakeScan();
  while( (tuple = iter->GetNextTuple()) != 0 )
  {
    BBox<dim> *box = (BBox<dim>*)tuple->GetAttribute(attrIndex);
    if( box->IsDefined() )
    {
      R_TreeLeafEntry<dim, TupleId> e( *box, tuple->GetTupleId() );
      rtree->Insert( e );
    }
    tuple->DeleteIfAllowed();
  }
  delete iter;

  return 0;
}

template<unsigned dim>
int CreateRTreeStreamRect(Word* args, Word& result, int message, 
                          Word& local, Supplier s)
{
  Word wTuple;
  R_Tree<dim, TupleId> *rtree = 
    (R_Tree<dim, TupleId>*)qp->ResultStorage(s).addr;
  result = SetWord( rtree );

  int attrIndex = ((CcInt*)args[2].addr)->GetIntval() - 1,
      tidIndex = ((CcInt*)args[3].addr)->GetIntval() - 1;

  qp->Open(args[0].addr);
  qp->Request(args[0].addr, wTuple);
  while (qp->Received(args[0].addr))
  {
    Tuple* tuple = (Tuple*)wTuple.addr;

    if( ((StandardSpatialAttribute<dim>*)tuple->
          GetAttribute(attrIndex))->IsDefined() &&
        ((TupleIdentifier *)tuple->GetAttribute(tidIndex))->
          IsDefined() )
    {
      BBox<dim> *box = (BBox<dim>*)tuple->GetAttribute(attrIndex);
      if( box->IsDefined() )
      {
        R_TreeLeafEntry<dim, TupleId> 
          e( *box, 
             ((TupleIdentifier *)tuple->
               GetAttribute(tidIndex))->GetTid() );
        rtree->Insert( e );
      }
    }
    tuple->DeleteIfAllowed();
    qp->Request(args[0].addr, wTuple);
  }
  qp->Close(args[0].addr);

  return 0;
}

template<unsigned dim>
int CreateRTree2LSpatial(Word* args, Word& result, int message, 
                         Word& local, Supplier s)
{
  Word wTuple;
  R_Tree<dim, TwoLayerLeafInfo> *rtree = 
    (R_Tree<dim, TwoLayerLeafInfo>*)qp->ResultStorage(s).addr;
  result = SetWord( rtree );

  int attrIndex = ((CcInt*)args[2].addr)->GetIntval() - 1,
      tidIndex = ((CcInt*)args[3].addr)->GetIntval() - 1;

  qp->Open(args[0].addr);
  qp->Request(args[0].addr, wTuple);
  while (qp->Received(args[0].addr))
  {
    Tuple* tuple = (Tuple*)wTuple.addr;

    if( ((StandardSpatialAttribute<dim>*)tuple->
          GetAttribute(attrIndex))->IsDefined() &&
        ((TupleIdentifier *)tuple->GetAttribute(tidIndex))->
          IsDefined() &&
        ((CcInt*)tuple->GetAttribute(tuple->GetNoAttributes()-2))->
          IsDefined() &&
        ((CcInt*)tuple->GetAttribute(tuple->GetNoAttributes()-1))->
          IsDefined() )
    {
      BBox<dim> box = 
        ((StandardSpatialAttribute<dim>*)tuple->
          GetAttribute(attrIndex))->BoundingBox();
      R_TreeLeafEntry<dim, TwoLayerLeafInfo> e(
        box,
        TwoLayerLeafInfo( 
          ((TupleIdentifier *)tuple->
            GetAttribute(tidIndex))->GetTid(),
          ((CcInt*)tuple->
            GetAttribute(tuple->GetNoAttributes()-2))->GetIntval(),
          ((CcInt*)tuple->
            GetAttribute(tuple->GetNoAttributes()-1))->GetIntval()));
      rtree->Insert( e );
    }
    tuple->DeleteIfAllowed();
    qp->Request(args[0].addr, wTuple);
  }
  qp->Close(args[0].addr);

  return 0;
}

template<unsigned dim>
int CreateRTree2LRect(Word* args, Word& result, int message, 
                      Word& local, Supplier s)
{
  Word wTuple;
  R_Tree<dim, TwoLayerLeafInfo> *rtree = 
    (R_Tree<dim, TwoLayerLeafInfo>*)qp->ResultStorage(s).addr;
  result = SetWord( rtree );

  int attrIndex = ((CcInt*)args[2].addr)->GetIntval() - 1,
      tidIndex = ((CcInt*)args[3].addr)->GetIntval() - 1;

  qp->Open(args[0].addr);
  qp->Request(args[0].addr, wTuple);
  while (qp->Received(args[0].addr))
  {
    Tuple* tuple = (Tuple*)wTuple.addr;

    if( ((StandardSpatialAttribute<dim>*)tuple->
          GetAttribute(attrIndex))->IsDefined() &&
        ((TupleIdentifier *)tuple->GetAttribute(tidIndex))->
          IsDefined() &&
        ((CcInt*)tuple->GetAttribute(tuple->GetNoAttributes()-2))->
          IsDefined() &&
        ((CcInt*)tuple->GetAttribute(tuple->GetNoAttributes()-1))->
          IsDefined() )
    {
      BBox<dim> *box = (BBox<dim>*)tuple->GetAttribute(attrIndex);
      if( box->IsDefined() )
      {
        R_TreeLeafEntry<dim, TwoLayerLeafInfo>
          e( *box,
             TwoLayerLeafInfo( 
               ((TupleIdentifier *)tuple->
                 GetAttribute(tidIndex))->GetTid(),
               ((CcInt*)tuple->GetAttribute(
                 tuple->GetNoAttributes()-2))->GetIntval(),
               ((CcInt*)tuple->GetAttribute(
                 tuple->GetNoAttributes()-1))->GetIntval() ) );
        rtree->Insert( e );
      }
    }
    tuple->DeleteIfAllowed();
    qp->Request(args[0].addr, wTuple);
  }
  qp->Close(args[0].addr);

  return 0;
}

/*
4.1.5 Definition of value mapping vectors

*/
ValueMapping rtreecreatertreemap [] = { CreateRTreeRelSpatial<2>,
                                        CreateRTreeRelSpatial<3>,
                                        CreateRTreeRelSpatial<4>,
                                        CreateRTreeRelRect<2>,
                                        CreateRTreeRelRect<3>,
                                        CreateRTreeRelRect<4>,
                                        CreateRTreeStreamSpatial<2>,
                                        CreateRTreeStreamSpatial<3>,
                                        CreateRTreeStreamSpatial<4>,
                                        CreateRTreeStreamRect<2>,
                                        CreateRTreeStreamRect<3>,
                                        CreateRTreeStreamRect<4>,
                                        CreateRTree2LSpatial<2>,
                                        CreateRTree2LSpatial<3>,
                                        CreateRTree2LSpatial<4>,
                                        CreateRTree2LRect<2>,
                                        CreateRTree2LRect<3>,
                                        CreateRTree2LRect<4> };

/*
4.1.6 Specification of operator ~creatertree~

*/

const string CreateRTreeSpec  = 
  "( ( \"1st Signature\" \"2nd Signature\" "
  "\"3rd Signature\" \"Syntax\" \"Meaning\" "
  "\"1st Example\" \"2nd Example\" "
  "\"3rd Example\" ) "
  "( <text>((rel (tuple (x1 t1)...(xn tn)))) xi)"
  " -> (rtree<d> (tuple ((x1 t1)...(xn tn))) ti false)</text--->"
  "<text>((stream (tuple (x1 t1)...(xn tn) (id tid))) xi)"
  " -> (rtree<d> (tuple ((x1 t1)...(xn tn))) ti false)</text--->"
  "<text>((stream (tuple (x1 t1)...(xn tn) "
  "(id tid)(low int)(high int))) xi)"
  " -> (rtree<d> (tuple ((x1 t1)...(xn tn))) ti true)</text--->"
  "<text>_ creatertree [ _ ]</text--->"
  "<text>Creates an rtree<d>. The key type ti must"
  " be of kind SPATIAL2D, SPATIAL3D or SPATIAL4D.</text--->"
  "<text>let myrtree = Kreis creatertree [Gebiet]</text--->"
  "<text>let myrtree = Kreis feed extend[id: tupleid(.)] "
  "creatertree[Gebiet]</text--->"
  "<text>let myrtree = Kreis feed extend[id: tupleid(.)] "
  "extend[low: 0, high: 0] creatertree[Gebiet]</text--->"
  ") )";

/*
4.1.7 Definition of operator ~creatertree~

*/
Operator creatertree (
          "creatertree",       // name
          CreateRTreeSpec,     // specification
          18,                  //Number of overloaded functions
          rtreecreatertreemap, // value mapping
          CreateRTreeSelect,   // trivial selection function
          CreateRTreeTypeMap   // type mapping
);

/*
7.2 Operator ~windowintersects~

7.2.1 Type mapping function of operator ~windowintersects~

*/
ListExpr WindowIntersectsTypeMap(ListExpr args)
{
  AlgebraManager *algMgr = SecondoSystem::GetAlgebraManager();
  ListExpr errorInfo = nl->OneElemList( nl->SymbolAtom( "ERRORS" ) );

  char* errmsg = "Incorrect input for operator windowintersects.";
  string rtreeDescriptionStr, relDescriptionStr;

  CHECK_COND(!nl->IsEmpty(args), errmsg);
  CHECK_COND(!nl->IsAtom(args), errmsg);
  CHECK_COND(nl->ListLength(args) == 3, errmsg);

  /* Split argument in three parts */
  ListExpr rtreeDescription = nl->First(args);
  ListExpr relDescription = nl->Second(args);
  ListExpr searchWindow = nl->Third(args);

  /* Query window: find out type of key */
  CHECK_COND(nl->IsAtom(searchWindow) &&
    nl->AtomType(searchWindow) == SymbolType &&
    (algMgr->CheckKind("SPATIAL2D", searchWindow, errorInfo)||
     algMgr->CheckKind("SPATIAL3D", searchWindow, errorInfo)||
     algMgr->CheckKind("SPATIAL4D", searchWindow, errorInfo)||
     nl->SymbolValue(searchWindow) == "rect"  ||
     nl->SymbolValue(searchWindow) == "rect3" ||
     nl->SymbolValue(searchWindow) == "rect4"),
    "Operator windowintersects expects that the search window\n"
    "is of TYPE rect, rect3, rect4 or "
    "of kind SPATIAL2D, SPATIAL3D, and SPATIAL4D.");

  /* handle rtree part of argument */
  nl->WriteToString (rtreeDescriptionStr, rtreeDescription); 
  CHECK_COND(!nl->IsEmpty(rtreeDescription) &&
    !nl->IsAtom(rtreeDescription) &&
    nl->ListLength(rtreeDescription) == 4,
    "Operator windowintersects expects a R-Tree with structure "
    "(rtree||rtree3||rtree4 (tuple ((a1 t1)...(an tn))) attrtype "
    "bool)\nbut gets a R-Tree list with structure '"
    +rtreeDescriptionStr+"'.");

  ListExpr rtreeSymbol = nl->First(rtreeDescription),
           rtreeTupleDescription = nl->Second(rtreeDescription),
           rtreeKeyType = nl->Third(rtreeDescription),
           rtreeTwoLayer = nl->Fourth(rtreeDescription);

  CHECK_COND(nl->IsAtom(rtreeKeyType) &&
    nl->AtomType(rtreeKeyType) == SymbolType &&
    (algMgr->CheckKind("SPATIAL2D", rtreeKeyType, errorInfo)||
     algMgr->CheckKind("SPATIAL3D", rtreeKeyType, errorInfo)||
     algMgr->CheckKind("SPATIAL4D", rtreeKeyType, errorInfo)||
     nl->IsEqual(rtreeKeyType, "rect")||
     nl->IsEqual(rtreeKeyType, "rect3")||
     nl->IsEqual(rtreeKeyType, "rect4")),
   "Operator windowintersects expects a R-Tree with key type\n"
   "of kind SPATIAL2D, SPATIAL3D, and SPATIAL4D\n"
   "or rect, rect3, and rect4.");

  /* handle rtree type constructor */
  CHECK_COND(nl->IsAtom(rtreeSymbol) &&
    nl->AtomType(rtreeSymbol) == SymbolType &&
    (nl->SymbolValue(rtreeSymbol) == "rtree"  ||
     nl->SymbolValue(rtreeSymbol) == "rtree3" ||
     nl->SymbolValue(rtreeSymbol) == "rtree4") ,
   "Operator windowintersects expects a R-Tree \n"
   "of type rtree, rtree3 or rtree4.");

  CHECK_COND(!nl->IsEmpty(rtreeTupleDescription) &&
    !nl->IsAtom(rtreeTupleDescription) &&
    nl->ListLength(rtreeTupleDescription) == 2,
    "Operator windowintersects expects a R-Tree with structure "
    "(rtree||rtree3||rtree4 (tuple ((a1 t1)...(an tn))) attrtype "
    "bool)\nbut gets a first list with wrong tuple description in "
    "structure \n'"+rtreeDescriptionStr+"'.");

  ListExpr rtreeTupleSymbol = nl->First(rtreeTupleDescription);
  ListExpr rtreeAttrList = nl->Second(rtreeTupleDescription);

  CHECK_COND(nl->IsAtom(rtreeTupleSymbol) &&
    nl->AtomType(rtreeTupleSymbol) == SymbolType &&
    nl->SymbolValue(rtreeTupleSymbol) == "tuple" &&
    IsTupleDescription(rtreeAttrList),
    "Operator windowintersects expects a R-Tree with structure "
    "(rtree||rtree3||rtree4 (tuple ((a1 t1)...(an tn))) attrtype "
    "bool)\nbut gets a first list with wrong tuple description in "
    "structure \n'"+rtreeDescriptionStr+"'.");

  CHECK_COND(nl->IsAtom(rtreeTwoLayer) &&
    nl->AtomType(rtreeTwoLayer) == BoolType,
   "Operator windowintersects expects a R-Tree with structure "
   "(rtree||rtree3||rtree4 (tuple ((a1 t1)...(an tn))) attrtype "
   "bool)\nbut gets a first list with wrong tuple description in "
   "structure \n'"+rtreeDescriptionStr+"'.");

  /* handle rel part of argument */
  nl->WriteToString (relDescriptionStr, relDescription); 
  CHECK_COND(!nl->IsEmpty(relDescription), errmsg);
  CHECK_COND(!nl->IsAtom(relDescription), errmsg);
  CHECK_COND(nl->ListLength(relDescription) == 2, errmsg);

  ListExpr relSymbol = nl->First(relDescription);;
  ListExpr tupleDescription = nl->Second(relDescription);

  CHECK_COND(nl->IsAtom(relSymbol) &&
    nl->AtomType(relSymbol) == SymbolType &&
    nl->SymbolValue(relSymbol) == "rel" &&
    !nl->IsEmpty(tupleDescription) &&
    !nl->IsAtom(tupleDescription) &&
    nl->ListLength(tupleDescription) == 2,
    "Operator windowintersects expects a R-Tree with structure "
    "(rel (tuple ((a1 t1)...(an tn)))) as relation description\n"
    "but gets a relation list with structure \n"
    "'"+relDescriptionStr+"'.");

  ListExpr tupleSymbol = nl->First(tupleDescription);
  ListExpr attrList = nl->Second(tupleDescription);

  CHECK_COND(nl->IsAtom(tupleSymbol) &&
    nl->AtomType(tupleSymbol) == SymbolType &&
    nl->SymbolValue(tupleSymbol) == "tuple" &&
    IsTupleDescription(attrList),
    "Operator windowintersects expects a R-Tree with structure "
    "(rel (tuple ((a1 t1)...(an tn)))) as relation description\n"
    "but gets a relation list with structure \n"
    "'"+relDescriptionStr+"'.");

  /* check that rtree and rel have the same associated tuple type */
  CHECK_COND(nl->Equal(attrList, rtreeAttrList),
   "Operator windowintersects: The tuple type of the R-tree\n"
   "differs from the tuple type of the relation.");

  string attrTypeRtree_str, attrTypeWindow_str;
  nl->WriteToString (attrTypeRtree_str, rtreeKeyType);
  nl->WriteToString (attrTypeWindow_str, searchWindow);

  CHECK_COND( 
    ( algMgr->CheckKind("SPATIAL2D", rtreeKeyType, errorInfo) &&
      nl->IsEqual(searchWindow, "rect") ) ||
    ( algMgr->CheckKind("SPATIAL2D", rtreeKeyType, errorInfo) &&
      algMgr->CheckKind("SPATIAL2D", searchWindow, errorInfo) ) ||
    ( nl->IsEqual(rtreeKeyType, "rect") &&
      nl->IsEqual(searchWindow, "rect") ) ||
    ( algMgr->CheckKind("SPATIAL3D", rtreeKeyType, errorInfo) &&
      nl->IsEqual(searchWindow, "rect3") ) ||
    ( algMgr->CheckKind("SPATIAL3D", rtreeKeyType, errorInfo) &&
      algMgr->CheckKind("SPATIAL3D", searchWindow, errorInfo) ) ||
    ( nl->IsEqual(rtreeKeyType, "rect3") &&
      nl->IsEqual(searchWindow, "rect3") ) ||
    ( algMgr->CheckKind("SPATIAL4D", rtreeKeyType, errorInfo) &&
      nl->IsEqual(searchWindow, "rect4") ) ||
    ( algMgr->CheckKind("SPATIAL4D", rtreeKeyType, errorInfo) &&
      algMgr->CheckKind("SPATIAL4D", searchWindow, errorInfo) ) ||
    ( nl->IsEqual(rtreeKeyType, "rect4") &&
      nl->IsEqual(searchWindow, "rect4") ),
    "Operator windowintersects expects joining attributes of same "
    "dimension.\nBut gets "+attrTypeRtree_str+
    " as left type and "+attrTypeWindow_str+" as right type.\n");
  
  return
    nl->TwoElemList(
      nl->SymbolAtom("stream"),
      tupleDescription);
}

/*
5.1.2 Selection function of operator ~windowintersects~

*/
int
WindowIntersectsSelection( ListExpr args )
{
  AlgebraManager *algMgr = SecondoSystem::GetAlgebraManager();
  ListExpr searchWindow = nl->Third(args),
           errorInfo = nl->OneElemList( nl->SymbolAtom( "ERRORS" ) );

  if (nl->SymbolValue(searchWindow) == "rect" ||
      algMgr->CheckKind("SPATIAL2D", searchWindow, errorInfo))
    return 0;
  else if (nl->SymbolValue(searchWindow) == "rect3" ||
           algMgr->CheckKind("SPATIAL3D", searchWindow, errorInfo))
    return 1;
  else if (nl->SymbolValue(searchWindow) == "rect4" ||
           algMgr->CheckKind("SPATIAL4D", searchWindow, errorInfo))
    return 2;

  return -1; /* should not happen */
}

/*
5.1.3 Value mapping function of operator ~windowintersects~

*/

template <unsigned dim>
struct WindowIntersectsLocalInfo
{
  Relation* relation;
  R_Tree<dim, TupleId>* rtree;
  BBox<dim> *searchBox;
  bool first;
};

template <unsigned dim>
int WindowIntersects( Word* args, Word& result,
                      int message, Word& local,
                      Supplier s )
{
  WindowIntersectsLocalInfo<dim> *localInfo;

  switch (message)
  {
    case OPEN :
    {
      Word rtreeWord, relWord, boxWord;
      qp->Request(args[0].addr, rtreeWord);
      qp->Request(args[1].addr, relWord);
      qp->Request(args[2].addr, boxWord);

      localInfo = new WindowIntersectsLocalInfo<dim>;
      localInfo->rtree = (R_Tree<dim, TupleId>*)rtreeWord.addr;
      localInfo->relation = (Relation*)relWord.addr;
      localInfo->first = true;
      localInfo->searchBox = 
        new BBox<dim> ( 
          (((StandardSpatialAttribute<dim> *)boxWord.addr)->
            BoundingBox()) );

      assert(localInfo->rtree != 0);
      assert(localInfo->relation != 0);
      local = SetWord(localInfo);
      return 0;
    }

    case REQUEST :
    {
      localInfo = (WindowIntersectsLocalInfo<dim>*)local.addr;
      R_TreeLeafEntry<dim, TupleId> e;

      if(localInfo->first)
      {
        localInfo->first = false;
        if( localInfo->rtree->First( *localInfo->searchBox, e ) )
        {
          Tuple *tuple = localInfo->relation->GetTuple(e.info);
          result = SetWord(tuple);
          return YIELD;
        }
        else
          return CANCEL;
      }
      else
      {
        if( localInfo->rtree->Next( e ) )
        {
          Tuple *tuple = localInfo->relation->GetTuple(e.info);
          result = SetWord(tuple);
          return YIELD;
        }
        else
          return CANCEL;
      }
    }

    case CLOSE :
    {
      localInfo = (WindowIntersectsLocalInfo<dim>*)local.addr;
      delete localInfo->searchBox;
      delete localInfo;
      return 0;
    }
  }
  return 0;
}

/*
5.1.4 Definition of value mapping vectors

*/
ValueMapping rtreewindowintersectsmap [] = { WindowIntersects<2>,
                                             WindowIntersects<3>,
                                             WindowIntersects<4> };


/*
5.1.5 Specification of operator ~windowintersects~

*/
const string windowintersectsSpec  =
      "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
      "( <text>((rtree (tuple ((x1 t1)...(xn tn)))"
      " ti)(rel (tuple ((x1 t1)...(xn tn)))) rect||rect3||rect4) ->"
      " (stream (tuple ((x1 t1)...(xn tn))))"
      "</text--->"
      "<text>_ _ windowintersects [ _ ]</text--->"
      "<text>Uses the given rtree to find all tuples"
      " in the given relation with .xi intersects the "
      " argument value's bounding box.</text--->"
      "<text>query citiesInd cities windowintersects"
      " [r] consume; where citiesInd "
      "is e.g. created with 'let citiesInd = "
      "cities creatertree [pos]'</text--->"
      ") )";

/*
5.1.6 Definition of operator ~windowintersects~

*/
Operator windowintersects (
         "windowintersects",        // name
         windowintersectsSpec,      // specification
         3,                         //number of overloaded functions
         rtreewindowintersectsmap,  // value mapping
         WindowIntersectsSelection, // trivial selection function
         WindowIntersectsTypeMap    // type mapping
);


/*
7.2 Operator ~windowintersectsS~

7.2.1 Type mapping function of operator ~windowintersectsS~

*/
ListExpr WindowIntersectsSTypeMap(ListExpr args)
{
  AlgebraManager *algMgr;
  algMgr = SecondoSystem::GetAlgebraManager();
  ListExpr errorInfo = nl->OneElemList( nl->SymbolAtom( "ERRORS" ) );

  char* errmsg = "Incorrect input for operator windowintersectsS.";
  string rtreeDescriptionStr;

  CHECK_COND(!nl->IsEmpty(args), errmsg);
  CHECK_COND(!nl->IsAtom(args), errmsg);
  CHECK_COND(nl->ListLength(args) == 2, errmsg);

  /* Split argument into two parts */
  ListExpr rtreeDescription = nl->First(args),
           searchWindow = nl->Second(args);

  /* Query window: find out type of key */
  CHECK_COND(nl->IsAtom(searchWindow) &&
    nl->AtomType(searchWindow) == SymbolType &&
    (algMgr->CheckKind("SPATIAL2D", searchWindow, errorInfo)||
     algMgr->CheckKind("SPATIAL3D", searchWindow, errorInfo)||
     algMgr->CheckKind("SPATIAL4D", searchWindow, errorInfo)||
     nl->SymbolValue(searchWindow) == "rect"  ||
     nl->SymbolValue(searchWindow) == "rect3" ||
     nl->SymbolValue(searchWindow) == "rect4"),
    "Operator windowintersects expects that the search window\n"
    "is of TYPE rect, rect3, rect4 or "
    "of kind SPATIAL2D, SPATIAL3D, and SPATIAL4D.");

  /* handle rtree part of argument */
  nl->WriteToString (rtreeDescriptionStr, rtreeDescription); 
  CHECK_COND(!nl->IsEmpty(rtreeDescription) &&
             !nl->IsAtom(rtreeDescription) &&
             nl->ListLength(rtreeDescription) == 4,
    "Operator windowintersects expects a R-Tree with structure "
    "(rtree||rtree3||rtree4 (tuple ((a1 t1)...(an tn))))\n"
    "but gets a R-Tree list with structure '"+
    rtreeDescriptionStr+"'.");

  ListExpr rtreeSymbol = nl->First(rtreeDescription),
           rtreeTupleDescription = nl->Second(rtreeDescription),
           rtreeKeyType = nl->Third(rtreeDescription),
           rtreeTwoLayer = nl->Fourth(rtreeDescription);
           
  CHECK_COND(nl->IsAtom(rtreeKeyType) &&
    nl->AtomType(rtreeKeyType) == SymbolType &&
    (algMgr->CheckKind("SPATIAL2D", rtreeKeyType, errorInfo)||
     algMgr->CheckKind("SPATIAL3D", rtreeKeyType, errorInfo)||
     algMgr->CheckKind("SPATIAL4D", rtreeKeyType, errorInfo)||
     nl->IsEqual(rtreeKeyType, "rect")||
     nl->IsEqual(rtreeKeyType, "rect3")||
     nl->IsEqual(rtreeKeyType, "rect4")),
   "Operator windowintersects expects a R-Tree with key type\n"
   "of kind SPATIAL2D, SPATIAL3D, and SPATIAL4D\n"
   "or rect, rect3, and rect4.");

  /* handle rtree type constructor */
  CHECK_COND(nl->IsAtom(rtreeSymbol) &&
    nl->AtomType(rtreeSymbol) == SymbolType &&
    (nl->SymbolValue(rtreeSymbol) == "rtree"  ||
     nl->SymbolValue(rtreeSymbol) == "rtree3" ||
     nl->SymbolValue(rtreeSymbol) == "rtree4") ,
   "Operator windowintersects expects a R-Tree \n"
   "of type rtree, rtree3 or rtree4.");

  CHECK_COND(!nl->IsEmpty(rtreeTupleDescription) &&
    !nl->IsAtom(rtreeTupleDescription) &&
    nl->ListLength(rtreeTupleDescription) == 2,
    "Operator windowintersects expects a R-Tree with structure "
    "(rtree||rtree3||rtree4 (tuple ((a1 t1)...(an tn))))\n"
    "but gets a first list with wrong tuple description in "
    "structure \n'"+rtreeDescriptionStr+"'.");

  ListExpr rtreeTupleSymbol = nl->First(rtreeTupleDescription);
  ListExpr rtreeAttrList = nl->Second(rtreeTupleDescription);

  CHECK_COND(nl->IsAtom(rtreeTupleSymbol) &&
    nl->AtomType(rtreeTupleSymbol) == SymbolType &&
    nl->SymbolValue(rtreeTupleSymbol) == "tuple" &&
    IsTupleDescription(rtreeAttrList),
    "Operator windowintersects expects a R-Tree with structure "
    "(rtree||rtree3||rtree4 (tuple ((a1 t1)...(an tn))))\n"
    "but gets a first list with wrong tuple description in "
    "structure \n'"+rtreeDescriptionStr+"'.");

  string attrTypeRtree_str, attrTypeWindow_str;
  nl->WriteToString (attrTypeRtree_str, rtreeKeyType);
  nl->WriteToString (attrTypeWindow_str, searchWindow);

  CHECK_COND( 
    ( algMgr->CheckKind("SPATIAL2D", rtreeKeyType, errorInfo) &&
      nl->IsEqual(searchWindow, "rect") ) ||
    ( algMgr->CheckKind("SPATIAL2D", rtreeKeyType, errorInfo) &&
      algMgr->CheckKind("SPATIAL2D", searchWindow, errorInfo) ) ||
    ( nl->IsEqual(rtreeKeyType, "rect") &&
      nl->IsEqual(searchWindow, "rect") ) ||
    ( algMgr->CheckKind("SPATIAL3D", rtreeKeyType, errorInfo) &&
      nl->IsEqual(searchWindow, "rect3") ) ||
    ( algMgr->CheckKind("SPATIAL3D", rtreeKeyType, errorInfo) &&
      algMgr->CheckKind("SPATIAL3D", searchWindow, errorInfo) ) ||
    ( nl->IsEqual(rtreeKeyType, "rect3") &&
      nl->IsEqual(searchWindow, "rect3") ) ||
    ( algMgr->CheckKind("SPATIAL4D", rtreeKeyType, errorInfo) &&
      nl->IsEqual(searchWindow, "rect4") ) ||
    ( algMgr->CheckKind("SPATIAL4D", rtreeKeyType, errorInfo) &&
      algMgr->CheckKind("SPATIAL4D", searchWindow, errorInfo) ) ||
    ( nl->IsEqual(rtreeKeyType, "rect4") &&
      nl->IsEqual(searchWindow, "rect4") ),
    "Operator windowintersects expects joining attributes of "
    "same dimension.\nBut gets "+attrTypeRtree_str+
    " as left type and "+attrTypeWindow_str+" as right type.\n");

  CHECK_COND(nl->IsAtom(rtreeTwoLayer) &&
             nl->AtomType(rtreeTwoLayer) == BoolType,
   "Operator windowintersects expects a R-Tree with structure "
    "(rtree||rtree3||rtree4 (tuple ((a1 t1)...(an tn))) attrtype "
    "bool)\nbut gets a first list with wrong tuple description in "
    "structure \n'"+rtreeDescriptionStr+"'.");

  if( nl->BoolValue(rtreeTwoLayer) == true )
    return
      nl->TwoElemList(
        nl->SymbolAtom("stream"),
        nl->TwoElemList(
          nl->SymbolAtom("tuple"), 
          nl->ThreeElemList(
            nl->TwoElemList(
              nl->SymbolAtom("id"),
              nl->SymbolAtom("tid")),
            nl->TwoElemList(
              nl->SymbolAtom("low"),
              nl->SymbolAtom("int")),
            nl->TwoElemList(
              nl->SymbolAtom("high"),
              nl->SymbolAtom("int")))));
  else
    return
      nl->TwoElemList(
        nl->SymbolAtom("stream"),
        nl->TwoElemList(
          nl->SymbolAtom("tuple"), 
          nl->OneElemList(
            nl->TwoElemList(
              nl->SymbolAtom("id"),
              nl->SymbolAtom("tid")))));
}  
 
/*
5.1.2 Selection function of operator ~windowintersectsS~

*/
int
WindowIntersectsSSelection( ListExpr args )
{
  AlgebraManager *algMgr = SecondoSystem::GetAlgebraManager();
  ListExpr searchWindow = nl->Second(args),
           errorInfo = nl->OneElemList( nl->SymbolAtom( "ERRORS" ) );
  bool doubleIndex = nl->BoolValue(nl->Fourth(nl->First(args)));

  if (nl->SymbolValue(searchWindow) == "rect" ||
      algMgr->CheckKind("SPATIAL2D", searchWindow, errorInfo))
    return 0 + (!doubleIndex ? 0 : 3);
  else if (nl->SymbolValue(searchWindow) == "rect3" ||
           algMgr->CheckKind("SPATIAL3D", searchWindow, errorInfo))
    return 1 + (!doubleIndex ? 0 : 3);
  else if (nl->SymbolValue(searchWindow) == "rect4" ||
           algMgr->CheckKind("SPATIAL4D", searchWindow, errorInfo))
    return 2 + (!doubleIndex ? 0 : 3);

  return -1; /* should not happen */
}

/*
5.1.3 Value mapping function of operator ~windowintersectsB~

*/
template <unsigned dim, class LeafInfo>
struct WindowIntersectsSLocalInfo
{
  R_Tree<dim, LeafInfo>* rtree;
  BBox<dim> *searchBox;
  TupleType *resultTupleType;
  bool first;
};

template <unsigned dim>
int WindowIntersectsSStandard( Word* args, Word& result,
                               int message, Word& local,
                               Supplier s )
{
  switch (message)
  {
    case OPEN :
    {
      Word rtreeWord, boxWord;
      qp->Request(args[0].addr, rtreeWord);
      qp->Request(args[1].addr, boxWord);

      WindowIntersectsSLocalInfo<dim, TupleId> *localInfo = 
        new WindowIntersectsSLocalInfo<dim, TupleId>();
      localInfo->rtree = (R_Tree<dim, TupleId>*)rtreeWord.addr;
      localInfo->first = true;
      localInfo->searchBox = 
        new BBox<dim> ( 
          (((StandardSpatialAttribute<dim> *)boxWord.addr)->
            BoundingBox()) );
      localInfo->resultTupleType = 
        new TupleType(nl->Second(GetTupleResultType(s)));
      local = SetWord(localInfo);
      return 0;
    }

    case REQUEST :
    {
      WindowIntersectsSLocalInfo<dim, TupleId> *localInfo = 
        (WindowIntersectsSLocalInfo<dim, TupleId>*)local.addr;
      R_TreeLeafEntry<dim, TupleId> e;

      if(localInfo->first)
      {
        localInfo->first = false;
        if( localInfo->rtree->First( *localInfo->searchBox, e ) )
        {
          Tuple *tuple = new Tuple( localInfo->resultTupleType );
          tuple->PutAttribute(0, new TupleIdentifier(true, e.info));
          result = SetWord(tuple);
          return YIELD;
        }
        else
          return CANCEL;
      }
      else
      {
        if( localInfo->rtree->Next( e ) )
        {
          Tuple *tuple = new Tuple( localInfo->resultTupleType );
          tuple->PutAttribute(0, new TupleIdentifier(true, e.info));
          result = SetWord(tuple);
          return YIELD;
        }
        else
          return CANCEL;
      }
    }

    case CLOSE :
    {
      WindowIntersectsSLocalInfo<dim, TupleId>* localInfo = 
        (WindowIntersectsSLocalInfo<dim, TupleId>*)local.addr;
      delete localInfo->searchBox;
      localInfo->resultTupleType->DeleteIfAllowed();
      delete localInfo;
      return 0;
    }
  }
  return 0;
}

template <unsigned dim>
int WindowIntersectsSDoubleLayer( Word* args, Word& result,
                                  int message, Word& local,
                                  Supplier s )
{
  switch (message)
  {
    case OPEN :
    {
      Word rtreeWord, boxWord;
      qp->Request(args[0].addr, rtreeWord);
      qp->Request(args[1].addr, boxWord);

      WindowIntersectsSLocalInfo<dim, TwoLayerLeafInfo> *localInfo = 
        new WindowIntersectsSLocalInfo<dim, TwoLayerLeafInfo>();
      localInfo->rtree = 
        (R_Tree<dim, TwoLayerLeafInfo>*)rtreeWord.addr;
      localInfo->first = true;
      localInfo->searchBox = 
        new BBox<dim> ( 
          (((StandardSpatialAttribute<dim> *)boxWord.addr)->
            BoundingBox()) );
      localInfo->resultTupleType = 
        new TupleType(nl->Second(GetTupleResultType(s)));
      local = SetWord(localInfo);
      return 0;
    }

    case REQUEST :
    {
      WindowIntersectsSLocalInfo<dim, TwoLayerLeafInfo> *localInfo = 
        (WindowIntersectsSLocalInfo<dim, TwoLayerLeafInfo>*)
        local.addr;
      R_TreeLeafEntry<dim, TwoLayerLeafInfo> e;

      if(localInfo->first)
      {
        localInfo->first = false;
        if( localInfo->rtree->First( *localInfo->searchBox, e ) )
        {
          Tuple *tuple = new Tuple( localInfo->resultTupleType );
          tuple->PutAttribute( 
            0, new TupleIdentifier( true, e.info.tupleId ) );
          tuple->PutAttribute( 1, new CcInt( true, e.info.low ) );
          tuple->PutAttribute( 2, new CcInt( true, e.info.high ) );
          result = SetWord(tuple);
          return YIELD;
        }
        else
          return CANCEL;
      }
      else
      {
        if( localInfo->rtree->Next( e ) )
        {
          Tuple *tuple = new Tuple( localInfo->resultTupleType );
          tuple->PutAttribute( 
            0, new TupleIdentifier( true, e.info.tupleId ) );
          tuple->PutAttribute( 1, new CcInt( true, e.info.low ) );
          tuple->PutAttribute( 2, new CcInt( true, e.info.high ) );
          result = SetWord(tuple);
          return YIELD;
        }
        else
          return CANCEL;
      }
    } 
    case CLOSE :
    {
      WindowIntersectsSLocalInfo<dim, TwoLayerLeafInfo> *localInfo = 
        (WindowIntersectsSLocalInfo<dim, TwoLayerLeafInfo>*)
        local.addr;
      delete localInfo->searchBox;
      localInfo->resultTupleType->DeleteIfAllowed();
      delete localInfo;
      return 0;
    }
  }
  return 0;
}

/*
5.1.4 Definition of value mapping vectors

*/
ValueMapping rtreewindowintersectsSmap [] = 
{ WindowIntersectsSStandard<2>,
  WindowIntersectsSStandard<3>,
  WindowIntersectsSStandard<4>,
  WindowIntersectsSDoubleLayer<2>,
  WindowIntersectsSDoubleLayer<3>,
  WindowIntersectsSDoubleLayer<4> };
 
 
/*
5.1.5 Specification of operator ~windowintersects~

*/
const string windowintersectsSSpec  =
      "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
      "( <text>((rtree (tuple ((x1 t1)...(xn tn)))"
      " ti) rect<d> ->"
      " (stream (tuple ((key rect<d>)(id tid))))"
      "</text--->"
      "<text>_ windowintersectsS [ _ ]</text--->"
      "<text>Uses the given rtree to find all entries"
      " that intersects the "
      " argument value's bounding box.</text--->"
      "<text>query citiesInd windowintersects"
      " [r] consume; where citiesInd "
      "is e.g. created with 'let citiesInd = "
      "cities creatertree [pos]'</text--->"
      ") )";

/*
5.1.6 Definition of operator ~windowintersects~

*/
Operator windowintersectsS (
         "windowintersectsS",        // name
         windowintersectsSSpec,      // specification
         6,                         //number of overloaded functions
         rtreewindowintersectsSmap,  // value mapping
         WindowIntersectsSSelection, // trivial selection function
         WindowIntersectsSTypeMap    // type mapping
);

/*
7.2 Operator ~gettuples~

7.2.1 Type mapping function of operator ~gettuples~

*/
ListExpr GetTuplesTypeMap(ListExpr args)
{
  string errmsg = "Incorrect input for operator gettuples.";
  AlgebraManager *algMgr;
  algMgr = SecondoSystem::GetAlgebraManager();

  string argStr;
  nl->WriteToString(argStr, args);

  CHECK_COND(!nl->IsEmpty(args) &&
             !nl->IsAtom(args) &&
             nl->ListLength(args) == 2, 
    errmsg + 
    "\nOperator gettuples expects two arguments, but gets '" + 
    argStr + "'.");

  // Split arguments into two parts 
  ListExpr streamDescription = nl->First(args),
           relDescription = nl->Second(args);
  string streamDescriptionStr, relDescriptionStr;

  // Handle the stream part of arguments
  nl->WriteToString (streamDescriptionStr, streamDescription);
  CHECK_COND(IsStreamDescription(streamDescription),
    errmsg + 
    "\nOperator gettuples expects a first argument with structure "
    "(stream (tuple ((id tid) (a1 t1)...(an tn))))\n"
    "but gets it with structure '" + streamDescriptionStr + "'.");

  // Handle the rel part of arguments
  nl->WriteToString (relDescriptionStr, relDescription);
  CHECK_COND(IsRelDescription(relDescription),
    errmsg + 
    "\nOperator gettuples expects a second argument with structure "
    "(rel (tuple ((a1 t1)...(an tn))))\n"
    "but gets it with structure '" + relDescriptionStr + "'.");

  ListExpr sTupleDescription = nl->Second(streamDescription),
           sAttrList = nl->Second(sTupleDescription),
           rTupleDescription = nl->Second(relDescription),
           rAttrList = nl->Second(rTupleDescription);

  // Find the attribute with type tid
  ListExpr first, rest, newAttrList, lastNewAttrList;
  int j, tidIndex = 0;
  string type;
  bool firstcall = true;

  rest = sAttrList;
  j = 1;
  while (!nl->IsEmpty(rest))
  {
    first = nl->First(rest);
    rest = nl->Rest(rest);

    type = nl->SymbolValue(nl->Second(first));
    if (type == "tid")
    {
      CHECK_COND(tidIndex == 0,
       "Operator gettuples expects as first argument a stream with\n"
       "one and only one attribute of type tid but gets\n'" + 
       streamDescriptionStr + "'.");
      tidIndex = j;
    }
    else
    {
      if (firstcall)
      {
        firstcall = false;
        newAttrList = nl->OneElemList(first);
        lastNewAttrList = newAttrList;
      }
      else
        lastNewAttrList = nl->Append(lastNewAttrList, first);
    }
    j++;
  }

  rest = rAttrList;
  while(!nl->IsEmpty(rest))
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
      lastNewAttrList = nl->Append(lastNewAttrList, first);
  }

  CHECK_COND( tidIndex != 0,
    "Operator gettuples expects as first argument a stream with\n"
    "one and only one attribute of type tid but gets\n'" + 
    streamDescriptionStr + "'.");

  return
    nl->ThreeElemList(
      nl->SymbolAtom("APPEND"),
      nl->OneElemList(
        nl->IntAtom(tidIndex)),
      nl->TwoElemList(
        nl->SymbolAtom("stream"),
        nl->TwoElemList(
          nl->SymbolAtom("tuple"),
          newAttrList)));
}

/*
5.1.3 Value mapping function of operator ~gettuples~

*/
struct GetTuplesLocalInfo
{
  Relation *relation;
  int tidIndex;
  TupleType *resultTupleType;
};

int GetTuples( Word* args, Word& result, int message, 
               Word& local, Supplier s )
{
  GetTuplesLocalInfo *localInfo;

  switch (message)
  {
    case OPEN :
    {
      Word relWord, tidWord;
      qp->Open(args[0].addr);
      qp->Request(args[1].addr, relWord);
      qp->Request(args[2].addr, tidWord);

      localInfo = new GetTuplesLocalInfo();
      localInfo->relation = (Relation*)relWord.addr;
      localInfo->resultTupleType = 
        new TupleType(nl->Second(GetTupleResultType(s)));
      localInfo->tidIndex = ((CcInt*)tidWord.addr)->GetIntval() - 1;
      local = SetWord(localInfo);
      return 0;
    }

    case REQUEST :
    {
      localInfo = (GetTuplesLocalInfo*)local.addr;

      Word wTuple;
      qp->Request(args[0].addr, wTuple);
      if( qp->Received(args[0].addr) )
      {
        Tuple *sTuple = (Tuple*)wTuple.addr,
              *resultTuple = new Tuple( localInfo->resultTupleType ),
              *relTuple = localInfo->relation->
                GetTuple(((TupleIdentifier *)sTuple->
                  GetAttribute(localInfo->tidIndex))->GetTid());
         
        int j = 0;

        // Copy the attributes from the stream tuple
        for( int i = 0; i < sTuple->GetNoAttributes(); i++ )
        {
          if( i != localInfo->tidIndex )
            resultTuple->CopyAttribute( j++, sTuple, i );
        }
        sTuple->DeleteIfAllowed();

        for( int i = 0; i < relTuple->GetNoAttributes(); i++ )
          resultTuple->CopyAttribute( j++, relTuple, i );
        relTuple->DeleteIfAllowed();

        result = SetWord( resultTuple );
        return YIELD;
      }
      else
        return CANCEL;
    }

    case CLOSE :
    {
      qp->Close(args[0].addr);
      localInfo = (GetTuplesLocalInfo*)local.addr;
      localInfo->resultTupleType->DeleteIfAllowed();
      delete localInfo;
      return 0;
    }
  }
  return 0;
}

/*
5.1.5 Specification of operator ~gettuples~

*/
const string gettuplesSpec  =
      "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
      "( <text>(stream (tuple ((id tid) (x1 t1)...(xn tn)))) x"
      " (rel (tuple ((y1 t1)...(yn tn)))) ->"
      " (stream (tuple ((x1 t1)...(xn tn) (y1 t1)...(yn tn))))"
      "</text--->"
      "<text>_ _ gettuples</text--->"
      "<text>Retrieves the tuples in the relation in the second "
      "argument given by the tuple id in first argument stream. "
      "The result tuple type is a concatenation of both types "
      "without the tid attribute.</text--->"
      "<text>query citiesInd windowintersectsS[r] cities gettuples; "
      "where citiesInd is e.g. created with 'let citiesInd = "
      "cities creatertree [pos]'</text--->"
      ") )";

/*
5.1.6 Definition of operator ~gettuples~

*/
Operator gettuples (
         "gettuples",            // name
         gettuplesSpec,          // specification
         GetTuples,              // value mapping
         Operator::SimpleSelect, // trivial selection function
         GetTuplesTypeMap        // type mapping
);

/*
7.2 Operator ~gettuplesdbl~

7.2.1 Type mapping function of operator ~gettuplesdbl~

*/
ListExpr GetTuplesDblTypeMap(ListExpr args)
{
  string errmsg = "Incorrect input for operator gettuplesdbl.";
  AlgebraManager *algMgr;
  algMgr = SecondoSystem::GetAlgebraManager();

  string argStr;
  nl->WriteToString(argStr, args);

  CHECK_COND(!nl->IsEmpty(args) &&
             !nl->IsAtom(args) &&
             nl->ListLength(args) == 3,
    errmsg +
    "\nOperator gettuplesdbl expects three arguments, but gets '" +
    argStr + "'.");

  // Split arguments into three parts
  ListExpr streamDescription = nl->First(args),
           relDescription = nl->Second(args),
           attrnameDescription = nl->Third(args);
  string streamDescriptionStr, relDescriptionStr, attrnameDescriptionStr;

  // Handle the stream part of arguments
  nl->WriteToString (streamDescriptionStr, streamDescription);

  CHECK_COND(IsStreamDescription(streamDescription),
    errmsg +
    "\nOperator gettuplesdbl expects a first argument with structure "
    "(stream (tuple ((id tid) (a1 t1)...(an tn))))\n"
    "but gets it with structure '" + streamDescriptionStr + "'.");

  // Handle the rel part of arguments
  nl->WriteToString (relDescriptionStr, relDescription);

  CHECK_COND(IsRelDescription(relDescription),
    errmsg +
    "\nOperator gettuplesdbl expects a second argument with structure "
    "(rel (tuple ((a1 t1)...(an tn))))\n"
    "but gets it with structure '" + relDescriptionStr + "'.");

  ListExpr sTupleDescription = nl->Second(streamDescription),
           sAttrList = nl->Second(sTupleDescription),
           rTupleDescription = nl->Second(relDescription),
           rAttrList = nl->Second(rTupleDescription);

  nl->WriteToString (attrnameDescriptionStr, attrnameDescription);

  CHECK_COND(nl->IsAtom(attrnameDescription) &&
    nl->AtomType(attrnameDescription) == SymbolType,
    errmsg + "\nOperator gettuplesdbl expects as third argument the name "
    "of the indexed attribute but gets '" + attrnameDescriptionStr + "'" );

  string attrName = nl->SymbolValue(attrnameDescription);

  int attrIndex;
  ListExpr attrType;

  CHECK_COND(
    (attrIndex = FindAttribute(rAttrList, attrName, attrType)) > 0,
    errmsg +
    "\nOperator gettuplesdbl expects that the attribute " +
    attrName + "\npassed as third argument to be part of "
    "the relation passed as second\n'" +
    relDescriptionStr + "'.");

  // Find the attribute with type tid
  ListExpr first, rest, newAttrList, lastNewAttrList;
  int j, tidIndex = 0;
  string type;
  bool firstcall = true,
       dblIdxFirst = false,
       dblIndex = false;

  rest = sAttrList;
  j = 1;
  while (!nl->IsEmpty(rest))
  {
    first = nl->First(rest);
    rest = nl->Rest(rest);

    type = nl->SymbolValue(nl->Second(first));
    if (type == "tid")
    {
      CHECK_COND(tidIndex == 0,
       "Operator gettuplesdbl expects as first argument a stream with\n"
       "one and only one attribute of type tid but gets\n'" +
       streamDescriptionStr + "'.");
      tidIndex = j;
    }
    else if( type == "int" &&
             tidIndex == j-1 )
    {
      dblIdxFirst = true;
    }
    else if( type == "int" &&
             dblIdxFirst &&
             tidIndex == j-2 )
    {
      dblIndex = true;
    }
    else
    {
      if (firstcall)
      {
        firstcall = false;
        newAttrList = nl->OneElemList(first);
        lastNewAttrList = newAttrList;
      }
      else
        lastNewAttrList = nl->Append(lastNewAttrList, first);
    }
    j++;
  }

  rest = rAttrList;
  while(!nl->IsEmpty(rest))
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
      lastNewAttrList = nl->Append(lastNewAttrList, first);
  }

  CHECK_COND( tidIndex != 0,
    "Operator gettuplesdbl expects as first argument a stream with\n"
    "one and only one attribute of type tid but gets\n'" +
    streamDescriptionStr + "'.");

  return
    nl->ThreeElemList(
      nl->SymbolAtom("APPEND"),
      nl->TwoElemList(
        nl->IntAtom(attrIndex),
        nl->IntAtom(tidIndex)),
      nl->TwoElemList(
        nl->SymbolAtom("stream"),
        nl->TwoElemList(
          nl->SymbolAtom("tuple"),
          newAttrList)));
}
                                                       
/*
5.1.3 Value mapping functions of operator ~gettuplesdbl~

*/
struct GetTuplesDblLocalInfo
{
  Relation *relation;
  int attrIndex;
  int tidIndex;
  Tuple *lastTuple;
  vector< pair<int, int> > intervals;
  TupleType *resultTupleType;
};

int GetTuplesDbl( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  GetTuplesDblLocalInfo *localInfo;

  switch (message)
  {
    case OPEN :
    {
      Word relWord, attrWord, tidWord;
      qp->Open(args[0].addr);
      qp->Request(args[1].addr, relWord);
      // We jump argument 2 which is the name of the attribute
      qp->Request(args[3].addr, attrWord);
      qp->Request(args[4].addr, tidWord);

      localInfo = new GetTuplesDblLocalInfo();
      localInfo->relation = (Relation*)relWord.addr;
      localInfo->resultTupleType =
        new TupleType(nl->Second(GetTupleResultType(s)));
      localInfo->attrIndex = ((CcInt*)attrWord.addr)->GetIntval() - 1;
      localInfo->tidIndex = ((CcInt*)tidWord.addr)->GetIntval() - 1;
      localInfo->lastTuple = 0;
      local = SetWord(localInfo);
      return 0;
    }

    case REQUEST :
    {
      localInfo = (GetTuplesDblLocalInfo*)local.addr;

      Word wTuple;
      qp->Request(args[0].addr, wTuple);
      while( qp->Received(args[0].addr) )
      {
        Tuple *sTuple = (Tuple*)wTuple.addr;

        if( localInfo->lastTuple == 0 )
        {
          localInfo->lastTuple = sTuple;
          localInfo->intervals.push_back( make_pair( 
            ((CcInt*)sTuple->GetAttribute( localInfo->tidIndex+1 ))->
              GetIntval(),
            ((CcInt*)sTuple->GetAttribute( localInfo->tidIndex+2 ))->
              GetIntval() ) );
        }
        else if( sTuple->GetAttribute( localInfo->tidIndex )->
                   Compare( localInfo->lastTuple->
                     GetAttribute( localInfo->tidIndex ) ) == 0 )
        {
          localInfo->intervals.push_back( make_pair( 
            ((CcInt*)sTuple->GetAttribute( localInfo->tidIndex+1 ))->
              GetIntval(),
            ((CcInt*)sTuple->GetAttribute( localInfo->tidIndex+2 ))->
              GetIntval() ) );
          sTuple->DeleteIfAllowed();
        }
        else
        {
          Tuple *resultTuple = new Tuple( localInfo->resultTupleType ),
                *relTuple = localInfo->relation->
                  GetTuple( ((TupleIdentifier *)localInfo->lastTuple->
                              GetAttribute(localInfo->tidIndex))->GetTid(),
                            localInfo->attrIndex,
                            localInfo->intervals );
          localInfo->intervals.clear();
          localInfo->intervals.push_back( make_pair( 
            ((CcInt*)sTuple->GetAttribute( localInfo->tidIndex+1 ))->
              GetIntval(),
            ((CcInt*)sTuple->GetAttribute( localInfo->tidIndex+2 ))->
              GetIntval() ) );

          // Copy the attributes from the stream tuple
          int j = 0;
          for( int i = 0; i < localInfo->lastTuple->GetNoAttributes(); i++ )
          {
            if( i < localInfo->tidIndex &&
                i > localInfo->tidIndex + 2 )
              resultTuple->CopyAttribute( j++, localInfo->lastTuple, i );
          }
          localInfo->lastTuple->DeleteIfAllowed();
          localInfo->lastTuple = sTuple;

          for( int i = 0; i < relTuple->GetNoAttributes(); i++ )
            resultTuple->CopyAttribute( j++, relTuple, i );
          relTuple->DeleteIfAllowed();

          result = SetWord( resultTuple );
          return YIELD;
        }
        qp->Request(args[0].addr, wTuple);
      }

      if( localInfo->lastTuple != 0 )
      {
        Tuple *resultTuple = new Tuple( localInfo->resultTupleType ),
              *relTuple = localInfo->relation->
                GetTuple(((TupleIdentifier *)localInfo->lastTuple->
                  GetAttribute(localInfo->tidIndex))->GetTid(),
                localInfo->attrIndex,
                localInfo->intervals );

        // Copy the attributes from the stream tuple
        int j = 0;
        for( int i = 0; i < localInfo->lastTuple->GetNoAttributes(); i++ )
        {
          if( i < localInfo->tidIndex &&
              i > localInfo->tidIndex + 2 )
            resultTuple->CopyAttribute( j++, localInfo->lastTuple, i );
        }
        localInfo->lastTuple->DeleteIfAllowed();
        localInfo->lastTuple = 0;

        for( int i = 0; i < relTuple->GetNoAttributes(); i++ )
          resultTuple->CopyAttribute( j++, relTuple, i );
        relTuple->DeleteIfAllowed();

        result = SetWord( resultTuple );
        return YIELD;
      }
      else
        return CANCEL;
    }

    case CLOSE :
    {
      qp->Close(args[0].addr);
      localInfo = (GetTuplesDblLocalInfo*)local.addr;
      localInfo->resultTupleType->DeleteIfAllowed();
      delete localInfo;
      return 0;
    }
  }
  return 0;
}

/*
5.1.5 Specification of operator ~gettuplesdbl~

*/
const string GetTuplesDblSpec  =
      "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
      "( <text>(stream (tuple ((id tid) (x1 t1)...(xn tn)))) x"
      " (rel (tuple ((y1 t1)...(yn tn)))) ->"
      " (stream (tuple ((x1 t1)...(xn tn) (y1 t1)...(yn tn))))"
      "</text--->"
      "<text>_ _ gettuplesdbl</text--->"
      "<text>Retrieves the tuples in the relation in the second "
      "argument given by the tuple id in first argument stream. "
      "The result tuple type is a concatenation of both types "
      "without the tid attribute.</text--->"
      "<text>query citiesInd windowintersectsS[r] cities gettuplesdbl; "
      "where citiesInd is e.g. created with 'let citiesInd = "
      "cities creatertree [pos]'</text--->"
      ") )";

/*
5.1.6 Definition of operator ~gettuplesdbl~

*/
Operator gettuplesdbl (
         "gettuplesdbl",            // name
         GetTuplesDblSpec,          // specification
         GetTuplesDbl,              // value mapping
         Operator::SimpleSelect,    // selection function
         GetTuplesDblTypeMap        // type mapping
);



/*
6 Definition and initialization of RTree Algebra

*/
class RTreeAlgebra : public Algebra
{
 public:
  RTreeAlgebra() : Algebra()
  {
    AddTypeConstructor( &rtree );
    AddTypeConstructor( &rtree3 );
    AddTypeConstructor( &rtree4 );

    AddOperator( &creatertree );
    AddOperator( &windowintersects );
    AddOperator( &windowintersectsS );
    AddOperator( &gettuples );
    AddOperator( &gettuplesdbl );
  }
  ~RTreeAlgebra() {};
};

RTreeAlgebra rtreeAlgebra;


extern "C"
Algebra*
InitializeRTreeAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return (&rtreeAlgebra);
}

