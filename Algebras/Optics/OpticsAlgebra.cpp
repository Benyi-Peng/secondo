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
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SECONDO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
----

//[_] [\_]
//characters   [1]  verbatim:  [$]  [$]
//characters   [2]  formula:  [$]  [$]
//characters   [3]  capital:  [\textsc{] [}]
//characters   [4]  teletype:  [\texttt{] [}]

1 Source file "OpticsAlgebra.cpp"[4]

March-October 2014, Marius Haug

1.1 Overview

This file contains the implementation of the OpticsAlgebra.

1.2 Includes

*/
#include "NestedList.h"
#include "QueryProcessor.h"
#include "RelationAlgebra.h"
#include "LogMsg.h"
#include "Stream.h"
#include "StandardTypes.h"
#include "SpatialAlgebra.h"
#include "OpticsR.h"
#include "OpticsM.h"
#include "DistFunction.h"

#include "OpticsGen.h"

#include "Symbols.h"

#include <limits.h>
#include <float.h>
#include <iostream>
#include <string>
#include <algorithm>

extern NestedList* nl;
extern QueryProcessor* qp;

namespace clusteropticsalg
{
/* 
Struct ~opticsResult~ to save the ordering of algorithm.

*/
 struct opticsResult
 {
  TupleBuffer* buffer; 
  list<TupleId>* tupleIds;
  list<TupleId>::iterator it;
  bool init;

  opticsResult(TupleBuffer* buf)
  {
   buffer = buf;
   tupleIds = new list<TupleId>;
   init = false;
  }

  ~opticsResult() { delete tupleIds; }

  void initialize() { it = tupleIds->begin(); init = true; }

  TupleId next() { TupleId tid = *it; *it++;  return tid; }
   
  bool hasNext() { return init && (it != tupleIds->end()); }
 };
/*
Type mapping method ~opticsRTM~

*/
 ListExpr opticsRTM( ListExpr args )
 {    
  if(nl->ListLength(args)!=2)
  {
   ErrorReporter::ReportError("two element expected");
   return nl->TypeError();
  }

  ListExpr stream = nl->First(args);

  if(!Stream<Tuple>::checkType(nl->First(args)))
  {
     return listutils::typeError("stream(Tuple) expected");
  }

  //Check the arguments
  ListExpr arguments = nl->Second(args);

  if(nl->ListLength(arguments)!=3)
  {
   ErrorReporter::ReportError("non conform list (three arguments expected)");
   return nl->TypeError();
  }
  
  if(!CcReal::checkType(nl->Second(arguments)))
  {
     return listutils::typeError("arg2 is not a real (Eps)");
  }
  
  if(!CcInt::checkType(nl->Third(arguments)))
  {
     return listutils::typeError("arg3 is not an int (MinPts)");
  }
  
  //Check the attribute name, is it in the tuple list
  ListExpr attrList = nl->Second(nl->Second(stream));
  ListExpr attrType;
  string attrName = nl->SymbolValue(nl->First(nl->Second(args)));
  int found = FindAttribute(attrList, attrName, attrType);
  if(found == 0)
  {
   ErrorReporter::ReportError("Attribute "
    + attrName + " is not a member of the tuple");
   return nl->TypeError();
  }
  
  if( !Rectangle<2>::checkType(attrType)
   && !Rectangle<3>::checkType(attrType)
   && !Rectangle<4>::checkType(attrType)
   && !Rectangle<8>::checkType(attrType) )
  {
   return listutils::typeError("Attribute " + attrName + " not of type " 
    + Rectangle<2>::BasicType() + ", " 
    + Rectangle<3>::BasicType() + ", " 
    + Rectangle<4>::BasicType() + " or " 
    + Rectangle<8>::BasicType() );
  }
  
  //Copy attrlist to newattrlist
  attrList             = nl->Second(nl->Second(stream));
  ListExpr newAttrList = nl->OneElemList(nl->First(attrList));
  ListExpr lastlistn   = newAttrList;
  
  attrList = nl->Rest(attrList);
  
  while(!(nl->IsEmpty(attrList)))
  {
   lastlistn = nl->Append(lastlistn,nl->First(attrList));
   attrList = nl->Rest(attrList);
  }

  lastlistn = nl->Append(lastlistn
   ,nl->TwoElemList(nl->SymbolAtom("CoreDist")
    ,nl->SymbolAtom(CcReal::BasicType())));
  lastlistn = nl->Append(lastlistn
   ,nl->TwoElemList(nl->SymbolAtom("ReachDist")
    ,nl->SymbolAtom(CcReal::BasicType())));
  lastlistn = nl->Append(lastlistn
   ,nl->TwoElemList(nl->SymbolAtom("Processed")
    ,nl->SymbolAtom(CcBool::BasicType())));
  lastlistn = nl->Append(lastlistn
   ,nl->TwoElemList(nl->SymbolAtom("Eps")
    ,nl->SymbolAtom(CcReal::BasicType())));

  return nl->ThreeElemList(nl->SymbolAtom(Symbol::APPEND())
   ,nl->OneElemList(nl->IntAtom(found-1))
   ,nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM())
   ,nl->TwoElemList(nl->SymbolAtom(Tuple::BasicType())
     ,newAttrList)));
 }
/*
Value mapping method ~opticsRVM~

*/
 template <int dim>
 int opticsRVM(Word* args, Word& result, int message, Word& local, Supplier s)
 {
  opticsResult* info = (opticsResult*) local.addr;

  switch (message)
  {
   case OPEN :
   {
    //Default values
    double defEps = 0.0;
    int defMinPts = 0;
    int attrCnt   = 0;
    int idxData   = -1;
    int minLeafs  = 2;
    int maxLeafs  = 8;
        
    //OpticsR instance
    OpticsR<dim> optics;

    Tuple* tup;
    TupleType* resultTupleType;
    ListExpr resultType;
    TupleBuffer* tp;
    size_t maxMem;
    Word argument;
    Supplier son;
    
    qp->Open(args[0].addr);
    
    //set the result type of the tuple
    resultType = GetTupleResultType(s);
    resultTupleType = new TupleType(nl->Second(resultType));
    Stream<Tuple> stream(args[0]);
    
    //set the given eps
    son = qp->GetSupplier(args[1].addr, 1);
    qp->Request(son, argument);
    defEps = ((CcReal*)argument.addr)->GetRealval();
    
    //set the given minPts
    son = qp->GetSupplier(args[1].addr, 2);
    qp->Request(son, argument);
    defMinPts = ((CcInt*)argument.addr)->GetIntval();
    
    //set the index of the attribute in the tuple
    idxData = static_cast<CcInt*>(args[2].addr)->GetIntval();
    
    stream.open();
    
    maxMem = qp->GetMemorySize(s);
    tp = new TupleBuffer(maxMem*1024*1024);
    
    while((tup = stream.request()) != 0)
    {
     Tuple *newTuple = new Tuple(resultTupleType);
     
     //Copy data from given tuple to the new tuple
     attrCnt = tup->GetNoAttributes();
     for( int i = 0; i < attrCnt; i++ ) 
     {
      newTuple->CopyAttribute( i, tup, i);
     }
 
     //Initialize the result tuple with default values
     CcReal coreDist(-1.0);
     newTuple->PutAttribute( attrCnt, ((Attribute*) &coreDist)->Clone());
     CcReal reachDist(-1.0);
     newTuple->PutAttribute( attrCnt+1, ((Attribute*) &reachDist)->Clone());
     CcBool processed(true, false);
     newTuple->PutAttribute( attrCnt+2, ((Attribute*) &processed)->Clone());
     CcReal eps(defEps);
     newTuple->PutAttribute( attrCnt+3, ((Attribute*) &eps)->Clone());
     
     tp->AppendTuple(newTuple);
     tup->DeleteIfAllowed();
     newTuple->DeleteIfAllowed();
    }
    stream.close();
    
    if(info)
    {
     if(info->buffer) delete info->buffer;
     delete info;
    }
    
    info = new opticsResult(tp);
    
    mmrtree::RtreeT<dim, TupleId> rtree(minLeafs, maxLeafs);
        
    GenericRelationIterator* relIter = tp->MakeScan();
    Tuple* obj;
    
    while((obj = relIter->GetNextTuple()))
    {
     TupleId objId = relIter->GetTupleId();
     Rectangle<dim>* attr = (Rectangle<dim>*) obj->GetAttribute(idxData);
     
     if(attr->IsDefined())
     {
      rtree.insert(*attr, objId);
     }
     obj->DeleteIfAllowed();
    }
    delete relIter;
   

 
    optics.initialize(&rtree, tp, idxData, attrCnt, attrCnt+1, attrCnt+2);

    


    //Start the optics ordering
    optics.order(defEps, defMinPts, info->tupleIds);
    
    
    info->initialize();
    local.setAddr(info);
    resultTupleType->DeleteIfAllowed(); 
    return 0;
   }
   case REQUEST :
   {
    if(info->hasNext())
    {
     TupleId tid = info->next();
     Tuple* outTup = info->buffer->GetTuple(tid, false);
     result = SetWord(outTup);
     return YIELD;
    }
    else
    {
     return CANCEL;
    }
   }
   case CLOSE :
   {
    if(info){
       delete info->buffer;
       delete info;
       local.addr=0;
    }
    return 0;
   }
  }
  
  return 0;
 }


 template <int dim>
 int opticsR2VM1(Word* args, Word& result, int message, Word& local, Supplier s)
 {

  typedef OpticsGen<Rectangle<dim>,  
                    SetOfObjectsR<RectDist<dim>, dim>,
                    RectDist<dim> > opticsclass;

   opticsclass* info = (opticsclass*) local.addr;

  switch (message)
  {
   case OPEN :
   {
    if(info){
      delete info;
      local.addr=0;
    }
        
    //set the result type of the tuple
    ListExpr resultType = nl->Second(GetTupleResultType(s));
    //set the given eps
    Supplier son = qp->GetSupplier(args[1].addr, 1);
    Word argument; 
    qp->Request(son, argument);
    CcReal* Eps = ((CcReal*) argument.addr);
    if(!Eps->IsDefined()){
      return 0;
    }
    double eps = Eps->GetValue();
    if(eps <= 0) {
       return 0;
    }
    
    //set the given minPts
    son = qp->GetSupplier(args[1].addr, 2);
    qp->Request(son, argument);
    CcInt* MinPts  = ((CcInt*)argument.addr);
    if(!MinPts->IsDefined()){
      return 0;
    }
    int minPts = MinPts->GetValue();
    if(minPts < 1){
       return 0;
    }
    //set the index of the attribute in the tuple
    int attrPos = static_cast<CcInt*>(args[2].addr)->GetIntval();
    size_t maxMem = qp->GetMemorySize(s)*1024*1024; 
    double UNDEFINED = -1.0;

    RectDist<dim> df;
    local.addr = new opticsclass(
                          args[0], attrPos, eps, minPts, 
                          UNDEFINED, resultType, maxMem, df);
    return 0;
   } 
   case REQUEST : {
     result.addr = info?info->next():0;
     return result.addr?YIELD:CANCEL;
   }
   case CLOSE : {
    if(info){
      delete info;
      local.addr = 0;
    }
    return 0;
   }
  }
  return 0;
}





/*
Selection method for value mapping array ~opticsRRecSL~

*/
 int opticsRRecSL(ListExpr args)
 {
  ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
  ListExpr attrType;
  string attrName = nl->SymbolValue(nl->First(nl->Second(args)));
  int found = FindAttribute(attrList, attrName, attrType);
  assert(found > 0);
   
  if(Rectangle<2>::checkType(attrType))
  {
   return 0;
  }
  else if(Rectangle<3>::checkType(attrType))
  {
   return 1;
  }
  else if(Rectangle<4>::checkType(attrType))
  {
   return 2;
  }
  else if(Rectangle<8>::checkType(attrType))
  {
   return 3;
  }
   
  return -1;
 }
/*
Value mapping array ~opticsRRecVM[]~

*/
 ValueMapping opticsRRecVM[] = 
 {
  opticsRVM<2>
 ,opticsRVM<3>
 ,opticsRVM<4>
 ,opticsRVM<8>
 };


 ValueMapping opticsR2VM[] = 
 {
  opticsR2VM1<2>,
  opticsR2VM1<3>,
  opticsR2VM1<4>,
  opticsR2VM1<8>
 };





/*
Type mapping method ~opticsMTM~

*/
 ListExpr opticsMTM( ListExpr args )
 {    
  if(nl->ListLength(args)!=2)
  {
   ErrorReporter::ReportError("two element expected");
   return nl->TypeError();
  }

  ListExpr stream = nl->First(args);

  if(!Stream<Tuple>::checkType(nl->First(args)))
  {
     return listutils::typeError("stream(Tuple) expected");
  }

  //Check the arguments
  ListExpr arguments = nl->Second(args);

  if(nl->ListLength(arguments)!=3)
  {
   ErrorReporter::ReportError("non conform list (three arguments expected)");
   return nl->TypeError();
  }
  
  if(!CcReal::checkType(nl->Second(arguments)))
  {
     return listutils::typeError("arg2 is not a real (Eps)");
  }
  
  if(!CcInt::checkType(nl->Third(arguments)))
  {
     return listutils::typeError("arg3 is not an int (MinPts)");
  }
  
  //Check the attribute name, is it in the tuple list
  ListExpr attrList = nl->Second(nl->Second(stream));
  ListExpr attrType;
  string attrName = nl->SymbolValue(nl->First(nl->Second(args)));
  int found = FindAttribute(attrList, attrName, attrType);
  if(found == 0)
  {
   ErrorReporter::ReportError("Attribute "
    + attrName + " is not a member of the tuple");
   return nl->TypeError();
  }
  
  if( !CcInt::checkType(attrType)
   && !CcReal::checkType(attrType)
   && !Point::checkType(attrType)
   && !CcString::checkType(attrType)
   && !Picture::checkType(attrType) )
  {
   return listutils::typeError("Attribute " + attrName + " not of type " 
    + CcInt::BasicType() + ", " 
    + CcReal::BasicType() + ", " 
    + Point::BasicType() + " , " 
    + CcString::BasicType() + " or " 
    + Picture::BasicType() );
  }
  
  //Copy attrlist to newattrlist
  attrList             = nl->Second(nl->Second(stream));
  ListExpr newAttrList = nl->OneElemList(nl->First(attrList));
  ListExpr lastlistn   = newAttrList;
  
  attrList = nl->Rest(attrList);
  
  while(!(nl->IsEmpty(attrList)))
  {
     lastlistn = nl->Append(lastlistn,nl->First(attrList));
     attrList = nl->Rest(attrList);
  }

  lastlistn = nl->Append(lastlistn
   ,nl->TwoElemList(nl->SymbolAtom("CoreDist")
    ,nl->SymbolAtom(CcReal::BasicType())));
  lastlistn = nl->Append(lastlistn
   ,nl->TwoElemList(nl->SymbolAtom("ReachDist")
    ,nl->SymbolAtom(CcReal::BasicType())));
  lastlistn = nl->Append(lastlistn
   ,nl->TwoElemList(nl->SymbolAtom("Processed")
    ,nl->SymbolAtom(CcBool::BasicType())));
  lastlistn = nl->Append(lastlistn
   ,nl->TwoElemList(nl->SymbolAtom("Eps")
    ,nl->SymbolAtom(CcReal::BasicType())));
    
   return nl->ThreeElemList(nl->SymbolAtom(Symbol::APPEND())
   ,nl->OneElemList(nl->IntAtom(found-1))
   ,nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM())
   ,nl->TwoElemList(nl->SymbolAtom(Tuple::BasicType())
     ,newAttrList)));
 }
/*
Value mapping method ~opticsMVM~

*/
 template <class T, class DistComp>
 int opticsMVM(Word* args, Word& result, int message, Word& local, Supplier s)
 {
  opticsResult* info = (opticsResult*) local.addr;

  switch (message)
  {
   case OPEN :
   {
    //Default values
    double defEps = 0.0;
    int defMinPts = 0;
    int attrCnt   = 0;
    int idxData   = -1;
    int minLeafs = 2;
    int maxLeafs = 8;
        
    //OpticsM instance
    OpticsM<T, DistComp> optics;

    Tuple* tup;
    TupleType* resultTupleType;
    ListExpr resultType;
    TupleBuffer* tp;
    size_t maxMem;
    Word argument;
    Supplier son;
    MMMTree<pair<T, TupleId>, DistComp >* mtree;
    
    qp->Open(args[0].addr);
    
    //set the result type of the tuple
    resultType = GetTupleResultType(s);
    resultTupleType = new TupleType(nl->Second(resultType));
    Stream<Tuple> stream(args[0]);
    
    //set the given eps
    son = qp->GetSupplier(args[1].addr, 1);
    qp->Request(son, argument);
    defEps = ((CcReal*)argument.addr)->GetRealval();
    
    //set the given minPts
    son = qp->GetSupplier(args[1].addr, 2);
    qp->Request(son, argument);
    defMinPts = ((CcInt*)argument.addr)->GetIntval();
    
    //set the index of the attribute in the tuple
    idxData = static_cast<CcInt*>(args[2].addr)->GetIntval();
    
    stream.open();
    
    maxMem = qp->GetMemorySize(s);
    tp = new TupleBuffer(maxMem*1024*1024);
    int cou = 0;
    while((tup = stream.request()) != 0)
    {
     Tuple *newTuple = new Tuple(resultTupleType);
     
     //Copy data from given tuple to the new tuple
     attrCnt = tup->GetNoAttributes();
     for( int i = 0; i < attrCnt; i++ ) 
     {
      newTuple->CopyAttribute( i, tup, i);
     }
 
     //Initialize the result tuple with default values
     CcReal coreDist(-1.0);
     newTuple->PutAttribute( attrCnt, ((Attribute*) &coreDist)->Clone());
     CcReal reachDist(-1.0);
     newTuple->PutAttribute( attrCnt+1, ((Attribute*) &reachDist)->Clone());
     CcBool processed(true, false);
     newTuple->PutAttribute( attrCnt+2, ((Attribute*) &processed)->Clone());
     CcReal eps(defEps);
     newTuple->PutAttribute( attrCnt+3, ((Attribute*) &eps)->Clone());
     cou++;
     tp->AppendTuple(newTuple);
     tup->DeleteIfAllowed();
     newTuple->DeleteIfAllowed();
    }
    
    stream.close();
    
    if(info)
    { 
     delete info->buffer;
     delete info;
    }
    
    info = new opticsResult(tp);
    
    DistComp dc;
    mtree = new MMMTree<pair<T, TupleId>, DistComp >(minLeafs, maxLeafs, dc);
    
    GenericRelationIterator* relIter = tp->MakeScan();
    Tuple* obj;
    
    while((obj = relIter->GetNextTuple()))
    {
     TupleId objId = relIter->GetTupleId();
     T attr = (T) obj->GetAttribute(idxData);
     
     pair<T, TupleId> p(attr, objId);
     
     mtree->insert(p);
     obj->DeleteIfAllowed();
    }
    delete relIter;     
 
    optics.initialize(mtree, tp, idxData, attrCnt, attrCnt+1, attrCnt+2, dc);
    
    //Start the optics ordering
    optics.order(defEps, defMinPts, info->tupleIds);
    
    info->initialize();
    local.setAddr(info);
    delete mtree; 
    resultTupleType->DeleteIfAllowed();
    return 0;
   }
   case REQUEST :
   {
    if(info->hasNext())
    {
     TupleId tid = info->next();
     Tuple* outTup = info->buffer->GetTuple(tid, false);
     result = SetWord(outTup);
     return YIELD;
    }
    else
    {
     return CANCEL;
    }
   }
   case CLOSE :
   {
    if(info)
    { 
     delete info->buffer;
     delete info;
     local.addr=0;
    }
    return 0;
   }
  }
  
  return 0;
 }
/*
Selection method for value mapping array ~opticsMDisSL~

*/
 int opticsMDisSL(ListExpr args)
 {
  ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
  ListExpr attrType;
  string attrName = nl->SymbolValue(nl->First(nl->Second(args)));
  int found = FindAttribute(attrList, attrName, attrType);
  assert(found > 0);
  
  if(CcInt::checkType(attrType))
  {
   return 0;
  }
  else if(CcReal::checkType(attrType))
  {
   return 1;
  }
  else if(Point::checkType(attrType))
  {
   return 2;
  }
  else if(CcString::checkType(attrType))
  {
   return 3;
  }
  else if(Picture::checkType(attrType))
  {
   return 4;
  }
  
  return -1; 
 };
/*
Value mapping array ~opticsMDisVM[]~

*/
 ValueMapping opticsMDisVM[] = 
 {
  opticsMVM<CcInt*, IntDist>
 ,opticsMVM<CcReal*, RealDist>
 ,opticsMVM<Point*, PointDist>
 ,opticsMVM<CcString*, StringDist>
 ,opticsMVM<Picture*, PictureDist>
 };
/*
Type mapping method ~opticsFTM~

*/
 ListExpr opticsFTM( ListExpr args )
 {    
  if(nl->ListLength(args)!=2)
  {
   ErrorReporter::ReportError("two element expected");
   return nl->TypeError();
  }

  ListExpr stream = nl->First(args);

  if(!Stream<Tuple>::checkType(nl->First(args)))
  {
     return listutils::typeError("stream(Tuple) expected");
  }

  //Check the arguments
  ListExpr arguments = nl->Second(args);

  if(nl->ListLength(arguments)!=4)
  {
   ErrorReporter::ReportError("non conform list (four arguments expected)");
   return nl->TypeError();
  }
  
  if(!CcReal::checkType(nl->Second(arguments)))
  {
   return listutils::typeError("arg2 is not a real (Eps)");
  }
  
  if(!CcInt::checkType(nl->Third(arguments)))
  {
   return listutils::typeError("arg3 is not an int (MinPts)");
  }
  
  if(!listutils::isMap<2>(nl->Fourth(arguments)))
  {
   return listutils::typeError("arg4 is not a map");
  }
  ListExpr funres = nl->Fourth(nl->Fourth(arguments));
  if(!CcInt::checkType(funres) &&
     !CcReal::checkType(funres)){
    return listutils::typeError("function reault not of type int or real");
  }

  
  
  //Check the attribute name, is it in the tuple list
  ListExpr attrType;
  ListExpr attrList = nl->Second(nl->Second(stream));
  string attrName = nl->SymbolValue(nl->First(nl->Second(args)));
  int found = FindAttribute(attrList, attrName, attrType);
  if(found == 0)
  {
   ErrorReporter::ReportError("Attribute "
    + attrName + " is not a member of the tuple");
   return nl->TypeError();
  }
  
  if(!nl->Equal(attrType, nl->Second(nl->Fourth(arguments))))
  {
   return listutils::typeError("Clustervalue type and function value type"
    "different");
  }
  
  if(!nl->Equal(attrType, nl->Third(nl->Fourth(arguments))))
  {
   return listutils::typeError("Clustervalue type and function value type"
    "different");
  }
  
  
  //Copy attrlist to newattrlist
  attrList             = nl->Second(nl->Second(stream));
  ListExpr newAttrList = nl->OneElemList(nl->First(attrList));
  ListExpr lastlistn   = newAttrList;
  
  attrList = nl->Rest(attrList);
  
  while(!(nl->IsEmpty(attrList)))
  {
     lastlistn = nl->Append(lastlistn,nl->First(attrList));
     attrList = nl->Rest(attrList);
  }

  lastlistn = nl->Append(lastlistn
   ,nl->TwoElemList(nl->SymbolAtom("CoreDist")
    ,nl->SymbolAtom(CcReal::BasicType())));
  lastlistn = nl->Append(lastlistn
   ,nl->TwoElemList(nl->SymbolAtom("ReachDist")
    ,nl->SymbolAtom(CcReal::BasicType())));
  lastlistn = nl->Append(lastlistn
   ,nl->TwoElemList(nl->SymbolAtom("Processed")
    ,nl->SymbolAtom(CcBool::BasicType())));
  lastlistn = nl->Append(lastlistn
   ,nl->TwoElemList(nl->SymbolAtom("Eps")
    ,nl->SymbolAtom(CcReal::BasicType())));
    
   return nl->ThreeElemList(nl->SymbolAtom(Symbol::APPEND())
   ,nl->OneElemList(nl->IntAtom(found-1))
   ,nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM())
   ,nl->TwoElemList(nl->SymbolAtom(Tuple::BasicType())
     ,newAttrList)));
 } 
/*
Value mapping method ~opticsFVM~

*/
 template <class T, class DistComp>
 int opticsFVM(Word* args, Word& result, int message, Word& local, Supplier s)
 {
  opticsResult* info = (opticsResult*) local.addr;

  switch (message)
  {
   case OPEN :
   {
    //Default values
    double defEps = 0.0;
    int defMinPts = 0;
    int attrCnt   = 0;
    int idxData   = -1;
    int minLeafs  = 2;
    int maxLeafs  = 8;    

    Tuple* tup;
    TupleType* resultTupleType;
    ListExpr resultType;
    TupleBuffer* tp;
    size_t maxMem;
    Word argument;
    Supplier son;

    //OpticsM instance
    OpticsM<T, DistComp> optics;

    MMMTree<pair<T, TupleId>, DistComp>* mtree;
    
    qp->Open(args[0].addr);
    
    //set the result type of the tuple
    resultType = GetTupleResultType(s);
    resultTupleType = new TupleType(nl->Second(resultType));
    Stream<Tuple> stream(args[0]);
    
    //set the given eps
    son = qp->GetSupplier(args[1].addr, 1);
    qp->Request(son, argument);
    defEps = ((CcReal*)argument.addr)->GetRealval();
    
    //set the given minPts
    son = qp->GetSupplier(args[1].addr, 2);
    qp->Request(son, argument);
    defMinPts = ((CcInt*)argument.addr)->GetIntval();
    
    //set the given function
    son = qp->GetSupplier(args[1].addr, 3);

    //set the index of the attribute in the tuple
    idxData = static_cast<CcInt*>(args[2].addr)->GetIntval();
    
    stream.open();
    
    maxMem = qp->GetMemorySize(s);
    tp = new TupleBuffer(maxMem * 1024*1024);
    int cou = 0;
    while((tup = stream.request()) != 0)
    {
     Tuple *newTuple = new Tuple(resultTupleType);
     
     //Copy data from given tuple to the new tuple
     attrCnt = tup->GetNoAttributes();
     for( int i = 0; i < attrCnt; i++ ) 
     {
      newTuple->CopyAttribute( i, tup, i);
     }
 
     //Initialize the result tuple with default values
     CcReal coreDist(-1.0);
     newTuple->PutAttribute( attrCnt, ((Attribute*) &coreDist)->Clone());
     CcReal reachDist(-1.0);
     newTuple->PutAttribute( attrCnt+1, ((Attribute*) &reachDist)->Clone());
     CcBool processed(true, false);
     newTuple->PutAttribute( attrCnt+2, ((Attribute*) &processed)->Clone());
     CcReal eps(defEps);
     newTuple->PutAttribute( attrCnt+3, ((Attribute*) &eps)->Clone());
     cou++;
     tp->AppendTuple(newTuple);
     tup->DeleteIfAllowed();
     newTuple->DeleteIfAllowed();
    }
    
    stream.close();
    
    if(info)
    {
     delete info->buffer;
     delete info;
    }
    
    info = new opticsResult(tp);
    
    DistComp dc;
    dc.initialize(qp, son);
    mtree = new MMMTree<pair<T, TupleId>, DistComp>(minLeafs, maxLeafs, dc);
    
    GenericRelationIterator* relIter = tp->MakeScan();
    Tuple* obj;
    
    while((obj = relIter->GetNextTuple()))
    {
     TupleId objId = relIter->GetTupleId();
     T attr = (T) obj->GetAttribute(idxData);
     
     pair<T, TupleId> p(attr, objId);
     
     mtree->insert(p);
     
     obj->DeleteIfAllowed();
    }
    delete relIter;
    
    optics.initialize(mtree, tp, idxData, attrCnt, attrCnt+1, attrCnt+2, dc);
    
    //Start the optics ordering
    optics.order(defEps, defMinPts, info->tupleIds);
    
    info->initialize();
    local.setAddr(info);
    resultTupleType->DeleteIfAllowed();
    delete mtree;
    
    return 0;
   }
   case REQUEST :
   {
    if(info->hasNext())
    {
     TupleId tid = info->next();
     Tuple* outTup = info->buffer->GetTuple(tid, false);
     result = SetWord(outTup);
     return YIELD;
    }
    else
    {
     return CANCEL;
    }
   }
   case CLOSE :
   {
    if(info){
      delete info->buffer;
      delete info;
      local.addr=0;
    }
    return 0;
   }
  }
  
  return 0;
 }
/*
Selection method for value mapping array ~opticsFDisSL~

*/
 int opticsFDisSL(ListExpr args)
 {
  ListExpr funResType = nl->Fourth(nl->Fourth(nl->Second(args)));
  if(CcInt::checkType(funResType)) return 0;
  if(CcReal::checkType(funResType)) return 1;
  
  return -1; 
 };
/*
Value mapping array ~opticsFDisVM[]~

*/
 ValueMapping opticsFDisVM[] = 
 {
  opticsFVM<Attribute*, CustomDist<Attribute*, CcInt> >,
  opticsFVM<Attribute*, CustomDist<Attribute*, CcReal> >
 };
/*
Struct ~opticsInfoR~

*/
 struct opticsInfoR : OperatorInfo
 {
  opticsInfoR() : OperatorInfo()
  {
   name      = "opticsR";
   signature = "stream(Tuple) -> stream(Tuple)";
   syntax    = "_ opticsR [_, _, _]";
   meaning   = "This operator will ordering data to identify the cluster "
               "structure. The operator uses the MMRTree index structure. The "
               "first paramater has to be a stream of tuple, the second is the "
               "attribute for clustering, the third is eps and the fourth is "
               "MinPts. The return value is a stream of tuples."
               "The supported type to cluster is the bbox.";
   example   = "query Kneipen feed extend[B : bbox(.GeoData)]" 
               "opticsR[B, 1000.0, 5] consume";
  }
 };


 struct opticsInfoR2 : OperatorInfo
 {
  opticsInfoR2() : OperatorInfo()
  {
   name      = "opticsR2";
   signature = "stream(Tuple) x attrName x double x int -> stream(Tuple)";
   syntax    = "_ opticsR2[_, _, _]";
   meaning   = "This operator will ordering data to identify the cluster "
               "structure. The operator uses the MMRTree index structure. The "
               "first paramater has to be a stream of tuple, the second is the "
               "attribute for clustering, the third is eps and the fourth is "
               "MinPts. The return value is a stream of tuples."
               "The supported type to cluster is the bbox.";
   example   = "query Kneipen feed extend[B : bbox(.GeoData)]" 
               "opticsR2[B, 1000.0, 5] consume";
  }
 };

/*
Struct ~opticsInfoM~

*/
 struct opticsInfoM : OperatorInfo
 {
  opticsInfoM() : OperatorInfo()
  {
   name      = "opticsM";
   signature = "stream(Tuple) -> stream(Tuple)";
   syntax    = "_ opticsM [_, _, _]";
   meaning   = "This operator will ordering data to identify the cluster "
               "structure. The operator uses the MMMTree index structure. The "
               "first paramater has to be a stream of tuple, the second is the "
               "attribute for clustering, the third is eps and the fourth is "
               "MinPts. The return value is a stream of tuples."
               "The supported types to cluster are point, picture, int, real "
               "and string.";
   example   = "query Kneipen feed opticsM[Name, 10.0, 5] consume";
  }
 };
/*
Struct ~opticsInfoF~

*/
 struct opticsInfoF : OperatorInfo
 {
  opticsInfoF() : OperatorInfo()
  {
   name      = "opticsF";
   signature = "stream(Tuple) -> stream(Tuple)";
   syntax    = "_ opticsF [_, _, _, fun]";
   meaning   = "This operator will ordering data to identify the cluster "
               "structure. The operator uses the MMMTree index structure. The "
               "first paramater has to be a stream of tuple, the second is the "
               "attribute for clustering, the third is eps, the fourth is "
               "MinPts and the sixth is the distance function. The return "
               "value is a stream of tuples."
               "The supported types to cluster are point, picture, int, real "
               "and string.";
   example   = "query plz feed opticsF[PLZ, 10.0, 5, fun(i1: int, i2: int)"
               "i1 - i2] consume";
   }
 };



/*
Operator ~extractDbScan~

*/
ListExpr extractDbScanTM(ListExpr args){

  if(!nl->HasLength(args,2)){
     return listutils::typeError("two arguments expected");
  }
  ListExpr stream = nl->First(args);
  if(!Stream<Tuple>::checkType(stream)){
     return listutils::typeError("first argument must be a tuple stream");
  }
  if(!CcReal::checkType(nl->Second(args))){
     return listutils::typeError("first argument must be a real");
  }
  ListExpr attrList = nl->Second(nl->Second(stream));
  ListExpr type;
  int coreDistPos = listutils::findAttribute(attrList, "CoreDist", type); 
  if(!coreDistPos){
     return listutils::typeError("Attribute CoreDist not member of the stream");
  }
  if(!CcReal::checkType(type)){
     return listutils::typeError("Attribute CoreDist not of type real");
  }
  int reachDistPos = listutils::findAttribute(attrList, "ReachDist", type); 
  if(!reachDistPos){
     return listutils::typeError("Attribute reachDist not "
                                  "member of the stream");
  }
  if(!CcReal::checkType(type)){
     return listutils::typeError("Attribute reachDist not of type real");
  }
  
  int EpsPos = listutils::findAttribute(attrList, "Eps", type); 
  if(!EpsPos){
     return listutils::typeError("Attribute Eps not member of the stream");
  }
  if(!CcReal::checkType(type)){
     return listutils::typeError("Attribute Eps not of type real");
  }
  int cidPos = listutils::findAttribute(attrList,"Cid", type);
  if(cidPos){
     return listutils::typeError("Attribute Cid already "
                                 "part of the attributes");
  }  
 


  ListExpr newAttr = nl->OneElemList( 
               nl->TwoElemList( nl->SymbolAtom("Cid"),
                                listutils::basicSymbol<CcInt>()));
  ListExpr newAttrList = listutils::concat(attrList, newAttr);
  ListExpr appendList = nl->ThreeElemList( nl->IntAtom(coreDistPos-1), 
                                           nl->IntAtom(reachDistPos-1), 
                                           nl->IntAtom(EpsPos-1));
  return nl->ThreeElemList(
                 nl->SymbolAtom(Symbols::APPEND()),
                 appendList,
                 nl->TwoElemList( listutils::basicSymbol<Stream<Tuple> >(),
                                  nl->TwoElemList( 
                                         listutils::basicSymbol<Tuple>(),
                                          newAttrList)));
}


class extractLocal{

  public:
     extractLocal(Word _stream, double _eps, int _coreDistPos, 
                  int _reachDistPos, int _epsPos, ListExpr type):
       stream(_stream), eps(_eps), coreDistPos(_coreDistPos), 
       reachDistPos(_reachDistPos), epsPos(_epsPos), id(0){
       tt = new TupleType(type);
       stream.open();
     }

     ~extractLocal(){
         stream.close();
         tt->DeleteIfAllowed();
      }

      Tuple* next(){
         Tuple* inTuple = stream.request();
         if(!inTuple){
            return 0;
         }
         Tuple* resTuple = new Tuple(tt);
         int attrCnt = inTuple->GetNoAttributes();
         // copy attributes to resTuple
         for(int i=0;i<attrCnt; i++){
            resTuple->CopyAttribute(i,inTuple,i);
         }
         CcReal* Eps = (CcReal*) inTuple->GetAttribute(epsPos);
         CcReal* ReachDist = (CcReal*) inTuple->GetAttribute(reachDistPos);
         CcReal* CoreDist = (CcReal*) inTuple->GetAttribute(coreDistPos);
         // optics never creates undefined attributes.
         // undefined is simulated by a value < 0
         if(    !Eps->IsDefined() || !ReachDist->IsDefined()
             || !CoreDist->IsDefined()){
            resTuple->PutAttribute(attrCnt, new CcInt(false,0));
            return resTuple;
         }
         double oldEps = Eps->GetValue();
         if(eps > oldEps){  // check condition
            resTuple->PutAttribute(attrCnt, new CcInt(false,0));
            return resTuple;
         }
         double reachDist = ReachDist->GetValue();
         double coreDist = CoreDist->GetValue();
         if((reachDist > eps) || (reachDist < 0)){
            if((coreDist <= eps) && !(coreDist < 0)){
               // start new cluster
               id++;
               resTuple->PutAttribute(attrCnt, new CcInt(true,id));  
            } else {
               // mark as noise
               resTuple->PutAttribute(attrCnt, new CcInt(true, -2));
            }
         } else {
            resTuple->PutAttribute(attrCnt, new CcInt(true,id));  
         }
         return resTuple; 
      }


  private:
     Stream<Tuple> stream;
     double eps;
     int coreDistPos;
     int reachDistPos;
     int epsPos;
     TupleType* tt; 
     int id;
};



 int extractDbScanVM(Word* args, Word& result, 
                     int message, Word& local, Supplier s) {
   extractLocal* info = (extractLocal*) local.addr;
   switch(message){
      case OPEN: {
            if(info) {
               delete info;
               local.addr=0;
            }
            int coreDistPos = ((CcInt*)args[2].addr)->GetValue();
            int reachDistPos = ((CcInt*)args[3].addr)->GetValue();
            int epsPos = ((CcInt*)args[4].addr)->GetValue();
            ListExpr type = nl->Second(GetTupleResultType(s));
            CcReal* Eps = (CcReal*) args[1].addr;
            if(!Eps->IsDefined()){
                return 0;
            }
            double eps = Eps->GetValue();
            if(eps<=0){
               return 0;
            }
            info = new extractLocal(args[0], eps, coreDistPos, 
                                    reachDistPos, epsPos, type);
            local.addr = info;
            return 0;
         }
      case REQUEST: {
         result.addr = info?info->next():0;
         return result.addr?YIELD:CANCEL;
      }
      case CLOSE: {
           if(info){
              delete info;
              local.addr = 0;
           }
           return 0;
      }
   }
   return -1;
 }

 OperatorSpec extractDbScanSpec(
  "stream x real -> stream",
  "_ extractDbScan",
  " Extract cluster from a stream processed via optics.",
  "query Kneipen feed extend[B : bbox(.GeoData)] "
    "opticsR[B, 2000.0, 10] extractDbScan[500.0] consume"
);


Operator extractDbScanOP(
  "extractDbScan",
  extractDbScanSpec.getStr(),
  extractDbScanVM,
  Operator::SimpleSelect,
  extractDbScanTM
);




/*
Algebra class ~ClusterOpticsAlgebra~

*/
 class ClusterOpticsAlgebra : public Algebra
 {
  public:
   ClusterOpticsAlgebra() : Algebra()
   {
    Operator* opr = 
        AddOperator(opticsInfoR(), opticsRRecVM, opticsRRecSL, opticsRTM);
    opr->SetUsesMemory();
    
    Operator* opr2 = 
        AddOperator(opticsInfoR2(), opticsR2VM, opticsRRecSL, opticsRTM);
    opr2->SetUsesMemory();

    Operator* opm = 
        AddOperator(opticsInfoM(), opticsMDisVM, opticsMDisSL, opticsMTM);
    opm->SetUsesMemory();
    Operator* opf = 
        AddOperator(opticsInfoF(), opticsFDisVM, opticsFDisSL, opticsFTM);
    opf->SetUsesMemory();

    AddOperator(&extractDbScanOP);

   }

   ~ClusterOpticsAlgebra() {};
 };
}

extern "C"
 Algebra* InitializeOpticsAlgebra( NestedList* nlRef, QueryProcessor* qpRef)
 {
  nl = nlRef;
  qp = qpRef;

  return (new clusteropticsalg::ClusterOpticsAlgebra());
 }



