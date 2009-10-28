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
//[TOC] [\tableofcontents]

[1] Source File of the Transportation Mode-Bus Network Algebra

August, 2009 Jianqiu Xu

[TOC]

1 Overview

This source file essentially contains the necessary implementations for
queries moving objects with transportation modes.

2 Defines and includes

*/

#include "BusNetwork.h"

#ifdef graph_model
string BusNetwork::busrouteTypeInfo =
                    "(rel(tuple((Id int)(LineNo int)(Trip mpoint))))";
string BusNetwork::btreebusrouteTypeInfo =
                    "(btree(tuple((Id int)(LineNo int)(Trip mpoint))) int)";
#else
string BusNetwork::busrouteTypeInfo =
                    "(rel(tuple((Id int)(LineNo int)(Up bool)(Trip mpoint))))";
string BusNetwork::btreebusrouteTypeInfo =
              "(btree(tuple((Id int)(LineNo int)(Up bool)(Trip mpoint))) int)";
#endif

string BusNetwork::busstopTypeInfo = "(rel(tuple((Id int)(BusStop point))))";
string BusNetwork::btreebusstopTypeInfo =
                  "(btree(tuple((Id int)(BusStop point))) int)";
string BusNetwork::rtreebusstopTypeInfo =
                  "(rtree(tuple((Id int)(BusStop point))) BusStop FALSE)";
string s1 = "(rel(tuple((eid int)(v1 int)(v2 int)";
string s2 = "(def_t periods)(l line)(fee real)(rid int)(trip mpoint)";
string s3 = "(pid int)(p1 point)(p2 point))))";
string BusNetwork::busedgeTypeInfo = s1+s2+s3;

string bs1 = "(btree(tuple((eid int)(v1 int)(v2 int)";
string bs2 = "(def_t periods)(l line)(fee real)(rid int)(trip mpoint)";
string bs3 = "(pid int)(p1 point)(p2 point))) int)";
string BusNetwork::btreebusedgeTypeInfo = bs1+bs2+bs3;

string nbsti1 = "(rel(tuple((Id int)(BusStop point)(path int)(pos int)";
string nbsti2 = "(zval real)(atime real)(Busline int))))";
string BusNetwork::newbusstopTypeInfo = nbsti1 + nbsti2;


string ss1 = "(btree(tuple((Id int)(BusStop point)(path int)(pos int)";
string ss2 = "(zval real)(atime real)(Busline int))) int)";
string BusNetwork::newbtreebusstopTypeInfo1 = ss1 + ss2;

string bst1 = "(btree(tuple((Id int)(BusStop point)(path int)(pos int)";
string bst2 = "(zval real)(atime real)(Busline int))) real)";
string BusNetwork::newbtreebusstopTypeInfo2 = bst1 + bst2;

string BusNetwork::busstoptreeTypeInfo =
    "(rel(tuple((Bustid int)(Ltid int)(Rtid int))))";


ListExpr BusNetwork::BusNetworkProp()
{
  ListExpr examplelist = nl->TextAtom();
  nl->AppendText(examplelist,"thebusnetwork(<id>,<busroute-relation>)");

  return  (nl->TwoElemList(
          nl->TwoElemList(nl->StringAtom("Creation"),
                          nl->StringAtom("Example Creation")),
          nl->TwoElemList(examplelist,
          nl->StringAtom("(let citybus = thebusnetwork(id,busroutes))"))));
}

ListExpr BusNetwork::OutBusNetwork(ListExpr typeInfo,Word value)
{

  BusNetwork* p = (BusNetwork*)value.addr;
  return p->Out(typeInfo);
}

Word BusNetwork::CreateBusNetwork(const ListExpr typeInfo)
{
  return SetWord(new BusNetwork());
}


void BusNetwork::DeleteBusNetwork(const ListExpr typeInfo,Word& w)
{
  cout<<"DeleteBusNetwork"<<endl;
  BusNetwork* p = (BusNetwork*)w.addr;
  p->Destroy();
  delete p;
}

ListExpr BusNetwork::Out(ListExpr typeInfo)
{

  //relation for node
  ListExpr xBusStop = nl->TheEmptyList();
  ListExpr xLast = nl->TheEmptyList();
  ListExpr xNext = nl->TheEmptyList();

  bool bFirst = true;
#ifdef graph_model
  for(int i = 1;i <= bus_node->GetNoTuples();i++){
    Tuple* pCurrentBus_Stop = bus_node->GetTuple(i);

    CcInt* id = (CcInt*)pCurrentBus_Stop->GetAttribute(SID);
    int bus_stop_id = id->GetIntval();

    Point* p = (Point*)pCurrentBus_Stop->GetAttribute(LOC);
    ListExpr xPoint = OutPoint(nl->TheEmptyList(),SetWord(p));
    //build the list
    xNext = nl->TwoElemList(nl->IntAtom(bus_stop_id),xPoint);
    if(bFirst){
      xBusStop = nl->OneElemList(xNext);
      xLast = xBusStop;
      bFirst = false;
    }else
      xLast = nl->Append(xLast,xNext);
    pCurrentBus_Stop->DeleteIfAllowed();
  }
#else
  for(int i = 1;i <= bus_node_new->GetNoTuples();i++){
    Tuple* pCurrentBus_Stop = bus_node_new->GetTuple(i);

    CcInt* id = (CcInt*)pCurrentBus_Stop->GetAttribute(NEWSID);
    int bus_stop_id = id->GetIntval();

    CcInt* pathid = (CcInt*)pCurrentBus_Stop->GetAttribute(BUSPATH);
    CcInt* pos = (CcInt*)pCurrentBus_Stop->GetAttribute(POS);
    CcReal* zval = (CcReal*)pCurrentBus_Stop->GetAttribute(ZVAL);

    Point* p = (Point*)pCurrentBus_Stop->GetAttribute(NEWLOC);
    ListExpr xPoint = OutPoint(nl->TheEmptyList(),SetWord(p));
    //build the list
    xNext = nl->FiveElemList(nl->IntAtom(bus_stop_id),xPoint,
                  nl->IntAtom(pathid->GetIntval()),
                  nl->IntAtom(pos->GetIntval()),
                  nl->RealAtom(zval->GetRealval()));
    if(bFirst){
      xBusStop = nl->OneElemList(xNext);
      xLast = xBusStop;
      bFirst = false;
    }else
      xLast = nl->Append(xLast,xNext);
    pCurrentBus_Stop->DeleteIfAllowed();
  }
#endif

  return nl->TwoElemList(nl->IntAtom(busnet_id),xBusStop);
}

bool BusNetwork::OpenBusNetwork(SmiRecord& valueRecord,size_t& offset,
const ListExpr typeInfo,Word& value)
{
//  cout<<"openbusnetwork"<<endl;
  value.addr = BusNetwork::Open(valueRecord,offset,typeInfo);
  return value.addr != NULL;
}

BusNetwork* BusNetwork::Open(SmiRecord& valueRecord,size_t& offset,
                          const ListExpr typeInfo)
{
//  cout<<"open"<<endl;
  return new BusNetwork(valueRecord,offset,typeInfo);

}

bool BusNetwork::SaveBusNetwork(SmiRecord& valueRecord,size_t& offset,
const ListExpr typeInfo,Word& value)
{
//  cout<<"savebusnetwork"<<endl;
  BusNetwork* p = (BusNetwork*)value.addr;
  return p->Save(valueRecord,offset,typeInfo);
}


bool BusNetwork::Save(SmiRecord& in_xValueRecord,size_t& inout_iOffset,
const ListExpr in_xTypeInfo)
{
//  cout<<"save"<<endl;
  /********************Save id the busnetwork****************************/
  int iId = busnet_id;
  in_xValueRecord.Write(&iId,sizeof(int),inout_iOffset);
  inout_iOffset += sizeof(int);

  in_xValueRecord.Write(&maxspeed,sizeof(double),inout_iOffset);
  inout_iOffset += sizeof(double);

  ListExpr xType;
  ListExpr xNumericType;
  /************************save bus routes****************************/
  nl->ReadFromString(busrouteTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!bus_route->Save(in_xValueRecord,inout_iOffset,xNumericType))
      return false;
  /************************save b-tree on bus routes***********************/
  nl->ReadFromString(btreebusrouteTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!btree_bus_route->Save(in_xValueRecord,inout_iOffset,xNumericType))
      return false;

#ifdef graph_model

  /****************************save nodes*********************************/
  nl->ReadFromString(busstopTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!bus_node->Save(in_xValueRecord,inout_iOffset,xNumericType))
      return false;
  /********************Save b-tree for bus stop***************************/
  nl->ReadFromString(btreebusstopTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!btree_bus_node->Save(in_xValueRecord,inout_iOffset,xNumericType))
    return false;

  /*******************Save r-tree for bus stop**************************/
  if(!rtree_bus_node->Save(in_xValueRecord,inout_iOffset))
    return false;

  /********************Save edges*************************************/
  nl->ReadFromString(busedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!bus_edge->Save(in_xValueRecord,inout_iOffset,xNumericType))
      return false;

  /******************Save b-tree on edge (eid)*****************************/
  nl->ReadFromString(btreebusedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!btree_bus_edge->Save(in_xValueRecord,inout_iOffset,xNumericType))
    return false;

  /******************Save b-tree on edge start node id**********************/
  nl->ReadFromString(btreebusedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!btree_bus_edge_v1->Save(in_xValueRecord,inout_iOffset,xNumericType))
    return false;

  /******************Save b-tree on edge end node id**********************/
  nl->ReadFromString(btreebusedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!btree_bus_edge_v2->Save(in_xValueRecord,inout_iOffset,xNumericType))
    return false;

  /*******************Save b-tree on edge path id **********************/
  nl->ReadFromString(btreebusedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!btree_bus_edge_path->Save(in_xValueRecord,inout_iOffset,xNumericType))
    return false;

  ///////////////////////adjacency list /////////////////////////////////
  SmiFileId fileId = 0;
  adjacencylist_index.SaveToRecord(in_xValueRecord, inout_iOffset, fileId);
  adjacencylist.SaveToRecord(in_xValueRecord, inout_iOffset, fileId);

  ///////////////////////path adjacency list ///////////////////////////
//  adj_path_index.SaveToRecord(in_xValueRecord, inout_iOffset, fileId);
//  adj_path.SaveToRecord(in_xValueRecord, inout_iOffset, fileId);

#else

  nl->ReadFromString(busstopTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!bus_node->Save(in_xValueRecord,inout_iOffset,xNumericType))
      return false;

/****************************save new nodes*********************************/
  nl->ReadFromString(newbusstopTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!bus_node_new->Save(in_xValueRecord,inout_iOffset,xNumericType))
      return false;

    /********************Save b-tree for new bus stop************************/
  nl->ReadFromString(newbtreebusstopTypeInfo1,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!btree_bus_node_new1->Save(in_xValueRecord,inout_iOffset,xNumericType))
    return false;

  nl->ReadFromString(newbtreebusstopTypeInfo2,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!btree_bus_node_new2->Save(in_xValueRecord,inout_iOffset,xNumericType))
    return false;


  /********************Save edges from the new bus stop*****************/
  nl->ReadFromString(busedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!bus_edge_new->Save(in_xValueRecord,inout_iOffset,xNumericType))
      return false;

  /*********************Save b-tree for new edges***********************/
  nl->ReadFromString(btreebusedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!btree_bus_edge_path_new->Save(in_xValueRecord,inout_iOffset,xNumericType))
    return false;

  /********************bus node tree***********************************/
  nl->ReadFromString(busstoptreeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  if(!bus_node_tree->Save(in_xValueRecord,inout_iOffset,xNumericType))
    return false;

  SmiFileId fileId = 0;
  bus_tree.SaveToRecord(in_xValueRecord, inout_iOffset, fileId);
  ps_zval.SaveToRecord(in_xValueRecord, inout_iOffset, fileId);
#endif
  return true;
}

void BusNetwork::CloseBusNetwork(const ListExpr typeInfo,Word& w)
{
//  cout<<"CloseBusNetwork"<<endl;
  delete static_cast<BusNetwork*>(w.addr);

  w.addr = NULL;
}
BusNetwork:: ~BusNetwork()
{
//  cout<<"~BusNetwork"<<endl;
  //bus route
  if(bus_route != NULL)
   bus_route->Close();

  //b-tree on bus route
  if(btree_bus_route != NULL)
    delete btree_bus_route;

#ifdef graph_model
  //bus stop
  if(bus_node != NULL)
      bus_node->Close();

  //b-tree on bus stop
  if(btree_bus_node != NULL)
      delete btree_bus_node;
  //r-tree on bus stop
  if(rtree_bus_node != NULL)
      delete rtree_bus_node;

  //bus edge relation
  if(bus_edge != NULL)
     bus_edge->Close();

  //b-tree on edge id
  if(btree_bus_edge != NULL)
      delete btree_bus_edge;
  //b-tree on edge start node id
  if(btree_bus_edge_v1 != NULL)
      delete btree_bus_edge_v1;
  //b-tree on edge end node id
  if(btree_bus_edge_v2 != NULL)
     delete btree_bus_edge_v2;
  //b-tree on edge path id
  if(btree_bus_edge_path != NULL)
    delete btree_bus_edge_path;
  //b-tree on edge path id new

  adjacencylist_index.Clear();
  adjacencylist.Clear();
#else
    if(bus_node != NULL)
      bus_node->Close();
  //new bus node
  if(bus_node_new != NULL)
    bus_node_new->Close();

  //b-tree for new bus node
  if(btree_bus_node_new1 != NULL)
      delete btree_bus_node_new1;

  if(btree_bus_node_new2 != NULL)
      delete btree_bus_node_new2;

  //new bus edge relation
  if(bus_edge_new != NULL)
    bus_edge_new->Close();

  if(btree_bus_edge_path_new != NULL)
    delete btree_bus_edge_path_new;

  if(bus_node_tree != NULL)
    bus_node_tree->Close();

  bus_tree.Clear();
  ps_zval.Clear();
#endif
}

Word BusNetwork::CloneBusNetwork(const ListExpr,const Word&)
{
  return SetWord(Address(0));
}

void* BusNetwork::CastBusNetwork(void* addr)
{
  return NULL;
}
int BusNetwork::SizeOfBusNetwork()
{
  return 0;
}
bool BusNetwork::CheckBusNetwork(ListExpr type,ListExpr& errorInfo)
{
  return nl->IsEqual(type,"busnetwork");
}

Word BusNetwork::InBusNetwork(ListExpr in_xTypeInfo,ListExpr in_xValue,
int in_iErrorPos,ListExpr& inout_xErrorInfo,bool& inout_bCorrect)
{
  cout<<"inbusnetwork"<<endl;
  BusNetwork* p = new BusNetwork(in_xValue,in_iErrorPos, inout_xErrorInfo,
                                inout_bCorrect);
  if(inout_bCorrect)
    return SetWord(p);
  else{
    delete p;
    return SetWord(Address(0));
  }
}
/*
Create the relation for storing bus stops

*/
void BusNetwork::FillBusNode(const Relation* in_busRoute)
{
  cout<<"create bus stop....."<<endl;
  Points* ps = new Points(0);
  ps->StartBulkLoad();

  ListExpr xTypeInfoEdge;
  nl->ReadFromString(busrouteTypeInfo,xTypeInfoEdge);
  ListExpr xNumType2 =
                SecondoSystem::GetCatalog()->NumericType(xTypeInfoEdge);
  Relation* temp_bus_route = new Relation(xNumType2,true);


  for(int i = 1; i <=  in_busRoute->GetNoTuples();i++){
    Tuple* tuple = in_busRoute->GetTuple(i);

    temp_bus_route->AppendTuple(tuple);

    MPoint* trip = (MPoint*)tuple->GetAttribute(TRIP);
    const UPoint* up;
    for(int j = 0;j < trip->GetNoComponents();j++){
      trip->Get(j,up);
      Point p0 = up->p0;
      Point p1 = up->p1;
      if(j == 0 ){ //add the start location as a stop
        *ps += p0;
         continue;
      }
      if(j == trip->GetNoComponents() - 1){ //the end location
        *ps += p1;
        continue;
      }
      if(AlmostEqual(p0,p1)){
        *ps += p0;
      }
    }
    tuple->DeleteIfAllowed();
  }
  ps->EndBulkLoad(true,true);

  ListExpr xTypeInfo;
  nl->ReadFromString(busstopTypeInfo,xTypeInfo);
  ListExpr xNumType = SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
  Relation* temp_bus_node = new Relation(xNumType,true);
  for(int i = 0;i < ps->Size();i++){
    const Point* temp_p;
    ps->Get(i,temp_p);
    Tuple* tuple = new Tuple(nl->Second(xNumType));
    tuple->PutAttribute(SID,new CcInt(true,i+1));
    tuple->PutAttribute(LOC,new Point(*temp_p));
    temp_bus_node->AppendTuple(tuple);
    tuple->DeleteIfAllowed();
  }
  delete ps;
  /***************** bus route **************************/
  ostringstream xBusRoutePtrStream;
  xBusRoutePtrStream << (long)temp_bus_route;
  string strQuery = "(consume(sort(feed(" + busrouteTypeInfo +
                "(ptr " + xBusRoutePtrStream.str() + ")))))";
  Word xResult;
  int QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  bus_route = (Relation*)xResult.addr;
  temp_bus_route->Delete();
  cout<<"bus route is finished....."<<endl;

  /****************b-tree on bus route*****************/
  ostringstream xThisRoutePtrStream;
  xThisRoutePtrStream << (long)bus_route;
  strQuery = "(createbtree (" + busrouteTypeInfo +
             "(ptr " + xThisRoutePtrStream.str() + "))" + "Id)";
  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  btree_bus_route = (BTree*)xResult.addr;
  cout<<"b-tree on bus route is finished....."<<endl;


  /***********************bus stop relation***************************/
  ostringstream xBusStopPtrStream;
  xBusStopPtrStream << (long)temp_bus_node;
  strQuery = "(consume(sort(feed(" + busstopTypeInfo +
                "(ptr " + xBusStopPtrStream.str() + ")))))";

  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  bus_node = (Relation*)xResult.addr;
  temp_bus_node->Delete();
  cout<<"bus stop is finished....."<<endl;
  /************b-tree on bus stop****************************/
  cout<<"create b-tree on bus stop id...."<<endl;
  ostringstream xThisBusStopPtrStream;
  xThisBusStopPtrStream<<(long)bus_node;
  strQuery = "(createbtree (" + busstopTypeInfo +
             "(ptr " + xThisBusStopPtrStream.str() + "))" + "Id)";
  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  btree_bus_node = (BTree*)xResult.addr;
  cout<<"b-tree on bus stop id is finished...."<<endl;
  /***********r-tree on bus stop*****************************/
  cout<<"create r-tree on bus stop..."<<endl;
  ostringstream xBusStop;
  xBusStop <<(long)bus_node;
  strQuery = "(bulkloadrtree(sortby(addid(feed(" + busstopTypeInfo +
            "(ptr " + xBusStop.str() + "))))((BusStop asc))) BusStop)";
  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  rtree_bus_node = (R_Tree<2,TupleId>*)xResult.addr;
  cout<<"r-tree on bus stop is finished...."<<endl;
}

/*
Calculate the Z-order value for the input point

*/
inline double BusNetwork::ZValue(Point& p)
{
  bitset<20> b;
  double base = 2,exp = 20;
  int x = (int)p.GetX();
  int y = (int)p.GetY();
  assert (x < pow(base,exp));
  assert (y < pow(base,exp));
  bitset<10> b1(x);
  bitset<10> b2(y);
  bool val;
  b.reset();
  for(int j = 0; j < 10;j++){
      val = b1[j];
      b.set(2*j,val);
      val = b2[j];
      b.set(2*j+1,val);
  }
  return b.to_ulong();
}
/*
Create the relation for storing bus stops

*/
void BusNetwork::FillBusNode_New(const Relation* in_busRoute)
{
  cout<<"create new bus stop....."<<endl;

  ListExpr xTypeInfo;
  nl->ReadFromString(newbusstopTypeInfo,xTypeInfo);
  ListExpr xNumType = SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
  Relation* temp_bus_node = new Relation(xNumType,true);


  ListExpr xTypeInfoEdge;
  nl->ReadFromString(busrouteTypeInfo,xTypeInfoEdge);
  ListExpr xNumType2 =
                SecondoSystem::GetCatalog()->NumericType(xTypeInfoEdge);
  Relation* temp_bus_route = new Relation(xNumType2,true);

  Points* ps = new Points(0);
  ps->StartBulkLoad();

  int pointcounter = 1;
  int maxbusroute = in_busRoute->GetNoTuples();
  for(int i = 1; i <=  in_busRoute->GetNoTuples();i++){
    Tuple* tuple = in_busRoute->GetTuple(i);
    MPoint* trip = (MPoint*)tuple->GetAttribute(TRIP);

    temp_bus_route->AppendTuple(tuple);

///////////////////////
    int bus_line_no = 0;
#ifndef graph_model
    CcBool* up_down = (CcBool*)tuple->GetAttribute(UP);
    CcInt* busline = (CcInt*)tuple->GetAttribute(LINENO);
    if(up_down->GetBoolval()) bus_line_no = busline->GetIntval();
    else
      bus_line_no = busline->GetIntval() + maxbusroute;
#endif
/////////////////////////////

    const UPoint* up;
    int poscounter = 0;

    for(int j = 0;j < trip->GetNoComponents();j++){
      trip->Get(j,up);
      Point p0 = up->p0;
      Point p1 = up->p1;
      double atime = up->timeInterval.start.ToDouble();

      if(j == 0 ){ //add the start location as a stop
//////////////////
        Tuple* tuple1 = new Tuple(nl->Second(xNumType));
        tuple1->PutAttribute(NEWSID,new CcInt(true,pointcounter));
        tuple1->PutAttribute(NEWLOC,new Point(p0));
        tuple1->PutAttribute(BUSPATH,new CcInt(true,i));
        tuple1->PutAttribute(POS,new CcInt(true,poscounter));
        tuple1->PutAttribute(ZVAL,new CcReal(true,ZValue(p0)));
        tuple1->PutAttribute(ATIME,new CcReal(true,atime));
        tuple1->PutAttribute(BUSLINE,new CcInt(true,bus_line_no));
        temp_bus_node->AppendTuple(tuple1);
//        cout<<*tuple1<<endl;
        tuple1->DeleteIfAllowed();
        pointcounter++;
        poscounter++;
        *ps += p0;
////////////////
        continue;
      }
      if(j == trip->GetNoComponents() - 1){ //the end location
//////////////////
        Tuple* tuple2 = new Tuple(nl->Second(xNumType));
        tuple2->PutAttribute(NEWSID,new CcInt(true,pointcounter));
        tuple2->PutAttribute(NEWLOC,new Point(p1));
        tuple2->PutAttribute(BUSPATH,new CcInt(true,i));
        tuple2->PutAttribute(POS,new CcInt(true,poscounter));
        tuple2->PutAttribute(ZVAL,new CcReal(true,ZValue(p1)));
        tuple2->PutAttribute(ATIME,new CcReal(true,atime));
        tuple2->PutAttribute(BUSLINE,new CcInt(true,bus_line_no));
        temp_bus_node->AppendTuple(tuple2);
//        cout<<*tuple2<<endl;
        tuple2->DeleteIfAllowed();
        pointcounter++;
        poscounter++;
        *ps += p1;
////////////////
        continue;
      }
      if(AlmostEqual(p0,p1)){
//////////////////
        Tuple* tuple3 = new Tuple(nl->Second(xNumType));
        tuple3->PutAttribute(NEWSID,new CcInt(true,pointcounter));
        tuple3->PutAttribute(NEWLOC,new Point(p0));
        tuple3->PutAttribute(BUSPATH,new CcInt(true,i));
        tuple3->PutAttribute(POS,new CcInt(true,poscounter));
        tuple3->PutAttribute(ZVAL,new CcReal(true,ZValue(p0)));
        tuple3->PutAttribute(ATIME,new CcReal(true,atime));
        tuple3->PutAttribute(BUSLINE,new CcInt(true,bus_line_no));
        temp_bus_node->AppendTuple(tuple3);
//        cout<<*tuple3<<endl;
        tuple3->DeleteIfAllowed();
        pointcounter++;
        poscounter++;
        *ps += p0;
////////////////
      }
    }
    tuple->DeleteIfAllowed();
  }
  ps->EndBulkLoad(true,true);


  ListExpr xTypeInfo_old;
  nl->ReadFromString(busstopTypeInfo,xTypeInfo_old);
  ListExpr xNumType_old =
              SecondoSystem::GetCatalog()->NumericType(xTypeInfo_old);
  Relation* temp_bus_node_old = new Relation(xNumType_old,true);

  //map point to zval
  for(int i = 0;i < ps->Size();i++){
    const Point* elem_p;
    ps->Get(i,elem_p);
    ps_zval.Append(ZValue(*(const_cast<Point*>(elem_p))));

    Tuple* tuple = new Tuple(nl->Second(xNumType_old));
    tuple->PutAttribute(SID,new CcInt(true,i+1));
    tuple->PutAttribute(LOC,new Point(*elem_p));
    temp_bus_node_old->AppendTuple(tuple);
    tuple->DeleteIfAllowed();
  }
  ostringstream xBusStopPtrStream_old;
  xBusStopPtrStream_old << (long)temp_bus_node_old;
  string strQuery_old = "(consume(sort(feed(" + busstopTypeInfo +
                "(ptr " + xBusStopPtrStream_old.str() + ")))))";
  Word xResult_old;
  int QueryExecuted_old =
              QueryProcessor::ExecuteQuery(strQuery_old,xResult_old);
  assert(QueryExecuted_old);
  bus_node = (Relation*)xResult_old.addr;
  temp_bus_node_old->Delete();

/***************** bus route **************************/
  ostringstream xBusRoutePtrStream;
  xBusRoutePtrStream << (long)temp_bus_route;
  string strQuery = "(consume(sort(feed(" + busrouteTypeInfo +
                "(ptr " + xBusRoutePtrStream.str() + ")))))";
  Word xResult;
  int QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  bus_route = (Relation*)xResult.addr;
  temp_bus_route->Delete();
  cout<<"bus route is finished....."<<endl;

  /****************b-tree on bus route*****************/
  ostringstream xThisRoutePtrStream;
  xThisRoutePtrStream << (long)bus_route;
  strQuery = "(createbtree (" + busrouteTypeInfo +
             "(ptr " + xThisRoutePtrStream.str() + "))" + "Id)";
  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  btree_bus_route = (BTree*)xResult.addr;
  cout<<"b-tree on bus route is finished....."<<endl;

  //new bus stop relation
  ostringstream xBusStopPtrStream;
  xBusStopPtrStream << (long)temp_bus_node;
//  strQuery = "(consume(sort(feed(" + newbusstopTypeInfo +
//                "(ptr " + xBusStopPtrStream.str() + ")))))";

  //first sort by z-order, then sort by arrive time in increasing order
  strQuery = "(consume(sortby(feed(" + newbusstopTypeInfo +
         "(ptr " + xBusStopPtrStream.str() + ")))((zval asc)(atime asc))))";

//  string strQuery = "(consume(sortby(feed(" + busedgeTypeInfo +
//         "(ptr " + xBusEdgePtrStream.str() + ")))((v1 asc))))";

  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  bus_node_new = (Relation*)xResult.addr;
  temp_bus_node->Delete();
  cout<<"new bus stop is finished....."<<endl;

//  cout<<bus_node_new->GetNoTuples()<<endl;
  //b-tree on bus_node_new path

  cout<<"create b-tree on new bus stop ..."<<endl;
  ostringstream xThisBusStopPtrStream1;
  xThisBusStopPtrStream1<<(long)bus_node_new;
  strQuery = "(createbtree (" + newbusstopTypeInfo +
             "(ptr " + xThisBusStopPtrStream1.str() + "))" + "path)";
  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  btree_bus_node_new1 = (BTree*)xResult.addr;
  cout<<"b-tree on new bus stop id is finished...."<<endl;


  ostringstream xThisBusStopPtrStream2;
  xThisBusStopPtrStream2<<(long)bus_node_new;
  strQuery = "(createbtree (" + newbusstopTypeInfo +
             "(ptr " + xThisBusStopPtrStream2.str() + "))" + "zval)";
  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  btree_bus_node_new2 = (BTree*)xResult.addr;

}
/*
Create binary tree on bus stop

*/
void BusNetwork::ConstructBusNodeTree()
{
  cout<<"ConstructBusNodeTree()"<<endl;
  ofstream outfile("temp_result"); //record info for debug

  ListExpr xTypeInfo;
  nl->ReadFromString(busstoptreeTypeInfo,xTypeInfo);
  ListExpr xNumType = SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
  Relation* temp_bus_node_tree = new Relation(xNumType,true);

  vector<bool> node_mark;
  for(int i = 0;i <= bus_node_new->GetNoTuples() + 1;i++)
    node_mark.push_back(false);


  for(int i = 1;i <= bus_node_new->GetNoTuples();i++){
    Tuple* t = bus_node_new->GetTuple(i);
    Tuple* tuple = new Tuple(nl->Second(xNumType));
    tuple->PutAttribute(NODETID,new CcInt(true,t->GetTupleId()));

//    cout<<i<<" "<<t->GetTupleId()<<endl;
    Point* buspos = (Point*)t->GetAttribute(NEWLOC);
    ///////   Construct Left Child    /////////////
    CcInt* pathid = (CcInt*)t->GetAttribute(BUSPATH);
    CcInt* pos = (CcInt*)t->GetAttribute(POS);

    CcInt* id = new CcInt(*pathid);
    CcInt* busline = (CcInt*)t->GetAttribute(BUSLINE);

    BTreeIterator* btreeiter = btree_bus_node_new1->ExactMatch(id);
    int left_child = -1;
    while(btreeiter->Next()){
      Tuple* tuple1 = bus_node_new->GetTuple(btreeiter->GetId());
      CcInt* position = (CcInt*)tuple1->GetAttribute(POS);
      if(position->GetIntval() == pos->GetIntval() + 1){
        left_child = tuple1->GetTupleId();
        tuple1->DeleteIfAllowed();
        node_mark[left_child] = true; //has parent node

        break;
      }
      tuple1->DeleteIfAllowed();
    }
    delete id;
    delete btreeiter;
    tuple->PutAttribute(LEFTTID,new CcInt(true,left_child));

    /////    Construct Right Child   /////////////
    CcReal* atime = (CcReal*)t->GetAttribute(ATIME);
    CcReal* zval = (CcReal*)t->GetAttribute(ZVAL);

    CcReal* zid = new CcReal(*zval);
    btreeiter = btree_bus_node_new2->ExactMatch(zid);
    int right_child = -1;
    while(btreeiter->Next()){
      Tuple* tuple2 = bus_node_new->GetTuple(btreeiter->GetId());
      CcReal* arr_time = (CcReal*)tuple2->GetAttribute(ATIME);
      if(arr_time->GetRealval() > atime->GetRealval()){
        right_child = tuple2->GetTupleId();
        tuple2->DeleteIfAllowed();
        node_mark[right_child] = true; //has parent node
        break;
      }
      tuple2->DeleteIfAllowed();
    }
    delete zid;
    delete btreeiter;
    tuple->PutAttribute(RIGHTTID,new CcInt(true,right_child));
    temp_bus_node_tree->AppendTuple(tuple);

   ///////    find the number of different bus routes    ///////////////////
    zid = new CcReal(*zval);
    btreeiter = btree_bus_node_new2->ExactMatch(zid);
    vector<int> bus_no;
    while(btreeiter->Next()){
      Tuple* tuple2 = bus_node_new->GetTuple(btreeiter->GetId());
      CcInt* busline_no = (CcInt*)tuple2->GetAttribute(BUSLINE);
      if(busline_no->GetIntval() != busline->GetIntval())
         bus_no.push_back(busline_no->GetIntval());
      tuple2->DeleteIfAllowed();
    }
    delete zid;
    delete btreeiter;

//    cout<<"before "<<bus_no.size()<<endl;
//    for(int i = 0;i < bus_no.size();i++)cout<<bus_no[i]<<" ";
    sort(bus_no.begin(),bus_no.end());
    vector<int>::iterator end = unique(bus_no.begin(),bus_no.end());
    bus_no.erase(end,bus_no.end());
//    cout<<"after "<<bus_no.size()<<endl;
//    for(int i = 0;i < bus_no.size();i++)cout<<bus_no[i]<<" ";
//    cout<<endl;

    //////////////  DBArray structure /////////////////////
    Node_Tree node(i,pathid->GetIntval(),pos->GetIntval(),
      atime->GetRealval(),*buspos,zval->GetRealval(),
//      left_child,right_child);
      left_child,right_child,bus_no.size(),busline->GetIntval());
     //number of different bus routes

    bus_no.clear();
    bus_tree.Append(node);

//    cout<<"i "<<i<<" table tid "<<tuple->GetTupleId()<<endl;
//    outfile<<"tid "<<tuple->GetTupleId()<<" left "<<
//           left_child<<" right "<<right_child<<endl;

    tuple->DeleteIfAllowed();
    t->DeleteIfAllowed();
  }

//  cout<<temp_bus_node_tree->GetNoTuples()<<endl;
//  cout<<bus_tree.Size()<<endl;

/***************** bus node tree **************************/
  ostringstream xBusNodePtrStream;
  xBusNodePtrStream << (long)temp_bus_node_tree;
  string strQuery = "(consume(sort(feed(" + busstoptreeTypeInfo +
                "(ptr " + xBusNodePtrStream.str() + ")))))";
  Word xResult;
  int QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  bus_node_tree = (Relation*)xResult.addr;
  temp_bus_node_tree->Delete();


  bus_tree.TrimToSize();
  ps_zval.TrimToSize();

/*  for(int i = 1;i <= bus_node_new->GetNoTuples();i++){
    Tuple* t = bus_node_new->GetTuple(i);
    cout<<*t<<endl;
    const Node_Tree* node;
    bus_tree.Get(i-1,node);
    (const_cast<Node_Tree*>(node))->Print();
    cout<<endl;
    t->DeleteIfAllowed();
  }*/

// for each root node (without incoming edges), start depth-traversal tree
//  CreateNodeTreeIndex(node_mark);


}
/*
Create index table for bus node relation

*/
struct Index_item{
  int tid;
  int pre;
  int post;
  bool left;
  bool right;
  Index_item(){}
  Index_item(int id,int a,int b):tid(id),pre(a),post(b)
  {
    left = false;
    right = false;

  }
  Index_item(const Index_item& item):tid(item.tid),pre(item.pre),
        post(item.post),left(item.left),right(item.right){}
  Index_item& operator=(const Index_item& item)
  {
    tid = item.tid;
    pre = item.pre;
    post = item.post;
    left = item.left;
    right = item.right;
    return *this;
  }
  void Print()
  {
    cout<<"tid "<<tid<<" pre "<<pre<<" post "<<post<<endl;
  }
};
struct Tree_Node{
  int tid;
  int l_tid;
  int r_tid;
  int pre;
  int post;
  bool flag;
  Tree_Node(){}
  Tree_Node(int id,int l,int r,bool f):tid(id),l_tid(l),r_tid(r),flag(f){}
  Tree_Node(const Tree_Node& tn):tid(tn.tid),
      l_tid(tn.l_tid),r_tid(tn.r_tid),pre(tn.pre),post(tn.post),flag(tn.flag){}
  Tree_Node& operator=(const Tree_Node& tn)
  {
    tid = tn.tid;
    l_tid = tn.l_tid;
    r_tid = tn.r_tid;
    flag = tn.flag;
    pre = tn.pre;
    post = tn.post;
    return *this;
  }
};

void BusNetwork::CreateNodeTreeIndex(vector<bool>& node_mark)
{
  cout<<"CreateNodeTreeIndex()"<<endl;
  ofstream outfile("temp_result"); //record info for debug

//////////////////node with no parent///////////////////////////////////////
/*vector<int> root_node; //node without parent
  for(int i = 1;i <= bus_node_new->GetNoTuples();i++){
    if(node_mark[i] == false){
      cout<<"tid "<<i<<endl;
      root_node.push_back(i);
    }
  }
  //record whether the node is already visited
  vector<Tree_Node> visit_record;
  for(int i = 1;i <= bus_node_tree->GetNoTuples();i++){
    Tuple* node = bus_node_tree->GetTuple(i);
    CcInt* tid = (CcInt*)node->GetAttribute(NODETID);
    CcInt* left_tid = (CcInt*)node->GetAttribute(LEFTTID);
    CcInt* right_tid = (CcInt*)node->GetAttribute(RIGHTTID);
    visit_record.push_back(Tree_Node(tid->GetIntval(),left_tid->GetIntval(),
                          right_tid->GetIntval(),false));
    node->DeleteIfAllowed();
//    cout<<i<<" "<<tid->GetIntval()<<endl;
  }

  //traversal the tree and assign the pre and post order value
  int counter = 0;
  for(int i = 0;i < root_node.size();i++){
    Tuple* node = bus_node_tree->GetTuple(root_node[i]);
    stack<Index_item> traverse_tree;
    Index_item item(node->GetTupleId(),counter,-1);
//    Index_item item(161,counter,-1);

    counter++;
    traverse_tree.push(item);
//    cout<<"start elem ";
//    item.Print();


    //depth-traversal tree
    while(traverse_tree.empty() == false){
      Index_item& topelem = traverse_tree.top();
      if(topelem.left && topelem.right){
        traverse_tree.pop();
        topelem.post = counter;
        counter++;
//        cout<<" pop() ";
//        topelem.Print();

        visit_record[topelem.tid-1].flag = true; //mark it
        visit_record[topelem.tid-1].pre = topelem.pre; //mark it
        visit_record[topelem.tid-1].post = topelem.post; //mark it

        outfile<<topelem.tid<<" "<<topelem.pre<<" "<<topelem.post<<endl;

        continue;
      }

      Tuple* t = bus_node_tree->GetTuple(topelem.tid);
      while(traverse_tree.top().left == false){
        Index_item& top = traverse_tree.top();
        Tuple* tuple = bus_node_tree->GetTuple(top.tid);
        CcInt* left_child = (CcInt*)tuple->GetAttribute(LEFTTID);

        if(left_child->GetIntval() != -1){

          if(visit_record[left_child->GetIntval()-1].flag == false){
            Index_item item(left_child->GetIntval(),counter,-1);
            traverse_tree.push(item);
            counter++;
//            cout<<"left push ";
//            item.Print();
          }else{
            //get and mark all its children nodes

            int entry = left_child->GetIntval()-1;
            int delta = counter - visit_record[entry].pre;
            counter += visit_record[entry].post - visit_record[entry].pre;
            counter++; //later use

            stack<Tree_Node> sub_traverse_tree;
            sub_traverse_tree.push(visit_record[entry]);


            //preorder sub-tree
            while(sub_traverse_tree.empty() == false){
                Tree_Node topnode = sub_traverse_tree.top();
                sub_traverse_tree.pop();

                //output
                Index_item item;
                item.tid = topnode.tid;
                item.pre = topnode.pre + delta;
                item.post = topnode.post + delta;

//                item.Print();

                outfile<<item.tid<<" "<<item.pre<<" "<<item.post<<endl;
                if(topnode.r_tid != -1){
                  sub_traverse_tree.push(visit_record[topnode.r_tid-1]);

                }
                if(topnode.l_tid != -1)
                  sub_traverse_tree.push(visit_record[topnode.l_tid-1]);
            }

          }
        }
        top.left = true;
        tuple->DeleteIfAllowed();
      }

      if(traverse_tree.top().right == false){
        Index_item& top = traverse_tree.top();
        Tuple* tuple = bus_node_tree->GetTuple(top.tid);
        CcInt* right_child = (CcInt*)tuple->GetAttribute(RIGHTTID);
        if(right_child->GetIntval() != -1){
            if(visit_record[right_child->GetIntval()-1].flag == false){
                Index_item item(right_child->GetIntval(),counter,-1);
                traverse_tree.push(item);
                counter++;
//                cout<<"right push ";
//                item.Print();
            }else{
              //get and mark all its children nodes

               int entry = right_child->GetIntval()-1;
               int delta = counter - visit_record[entry].pre;
               counter += visit_record[entry].post - visit_record[entry].pre;
               counter++; //later use

               stack<Tree_Node> sub_traverse_tree;
               sub_traverse_tree.push(visit_record[entry]);

               //preorder sub-tree
               while(sub_traverse_tree.empty() == false){
                  Tree_Node topnode = sub_traverse_tree.top();
                  sub_traverse_tree.pop();

                  //output
                  Index_item item;
                  item.tid = topnode.tid;
                  item.pre = topnode.pre + delta;
                  item.post = topnode.post + delta;

//                  item.Print();
                  outfile<<item.tid<<" "<<item.pre<<" "<<item.post<<endl;

                  if(topnode.r_tid != -1){
                    sub_traverse_tree.push(visit_record[topnode.r_tid-1]);
                  }
                  if(topnode.l_tid != -1)
                    sub_traverse_tree.push(visit_record[topnode.l_tid-1]);
                }

            }
        }
        top.right = true;
        tuple->DeleteIfAllowed();
      }

      t->DeleteIfAllowed();

    } //end for while
    node->DeleteIfAllowed();
    break;
  }*/


}

/*
Get the node id of a bus stop

*/
TupleId BusNetwork::FindPointTid(Point& p)
{
  BBox<2> searchbox = p.BoundingBox();
  R_TreeLeafEntry<2,TupleId> e;
  if(rtree_bus_node->First(searchbox,e))
    return e.info;
  return 0;
}

/*
Get the node id of a bus stop (new)

*/
TupleId BusNetwork::FindPointTidNew(int pathid,Point& p)
{
    TupleId tid = 0;
    CcInt* id = new CcInt(true,pathid);
    BTreeIterator* btreeiter = btree_bus_node_new1->ExactMatch(id);
    while(btreeiter->Next()){
      Tuple* tuple = bus_node_new->GetTuple(btreeiter->GetId());
      Point* loc = (Point*)tuple->GetAttribute(NEWLOC);
      if(AlmostEqual(*loc,p)){
        tid = tuple->GetTupleId();
        tuple->DeleteIfAllowed();
        break;
      }
//      cout<<*tuple<<endl;
      tuple->DeleteIfAllowed();
    }
    delete id;
    delete btreeiter;
    return tid;
}


/*
Create the relation for storing edges

*/
void BusNetwork::FillBusEdge(const Relation* in_busRoute)
{
  cout<<"create edge relation..."<<endl;
  vector<Point> endpoints;//store start and end pointss
  vector<Point> connectpoints; //store points between two stops
  Interval<Instant> timeInterval;

  //relation description for edge

  ListExpr xTypeInfoEdge;
  nl->ReadFromString(busedgeTypeInfo,xTypeInfoEdge);
  ListExpr xNumType2 =
                SecondoSystem::GetCatalog()->NumericType(xTypeInfoEdge);
  Relation* temp_bus_edge = new Relation(xNumType2,true);

  int eid = 1;
  for(int i = 1; i <=  in_busRoute->GetNoTuples();i++){
    Tuple* tuple = in_busRoute->GetTuple(i);
    MPoint* trip = (MPoint*)tuple->GetAttribute(TRIP);
    const UPoint* up;
    CcInt* rid = (CcInt*)tuple->GetAttribute(LINENO);//bus line no
    int routeid = rid->GetIntval();
    CcInt* rpid = (CcInt*)tuple->GetAttribute(RID);//path id

    endpoints.clear();
    connectpoints.clear();
    /////generate cost fee /////
    srand(time(0));
    float costfee = (10+rand() % 20)/100.0;
//    cout<<costfee<<endl;
    srand(i);
    //////////
    int start_index=0;
    int end_index=0;
//    cout<<*trip<<endl;
    for(int j = 0;j < trip->GetNoComponents();j++){
      trip->Get(j,up);
      Point p0 = up->p0;
      Point p1 = up->p1;

      if(j == 0 ){ //add the start location as a stop
        endpoints.push_back(p0);
        connectpoints.push_back(p0);
        start_index = j;
        if(AlmostEqual(p0,p1))
          start_index++;
        if(!AlmostEqual(p0,p1))
          connectpoints.push_back(p1);

        continue;
      }

      if(AlmostEqual(p0,p1)){
        endpoints.push_back(p0);

        if(endpoints.size() == 2){ //extract edge
 //       cout<<i<<" Interval "<<timeInterval<<" "<<connectpoints.size()<<endl;
          //create line
          Line* line = new Line(0);
          line->StartBulkLoad();
          int edgeno = 0;
          HalfSegment hs;
          for(unsigned int k = 0;k < connectpoints.size() - 1;k++){
            Point start = connectpoints[k];
            Point end = connectpoints[k+1];
            hs.Set(true,start,end);
            hs.attr.edgeno = ++edgeno;
            *line += hs;
            hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
            *line += hs;
          }
          line->EndBulkLoad();
 ////////////////////    edge         ///////////////////////////////////////
          TupleId tid1 = FindPointTid(endpoints[0]);
          TupleId tid2 = FindPointTid(endpoints[1]);
//          cout<<id1<<" "<<id2<<endl;
          assert(tid1 != tid2 && tid1 != 0 && tid2 != 0);
//          cout<<endpoints[0]<<" "<<endpoints[1]<<endl;
          Tuple* tuple_p1 = bus_node->GetTuple(tid1);
          Tuple* tuple_p2 = bus_node->GetTuple(tid2);
//          cout<<*tuple_p1<<" "<<*tuple_p2<<endl;
          Point* p1 = (Point*)tuple_p1->GetAttribute(LOC);
          Point* p2 = (Point*)tuple_p2->GetAttribute(LOC);
          int nid1 = ((CcInt*)tuple_p1->GetAttribute(SID))->GetIntval();
          int nid2 = ((CcInt*)tuple_p2->GetAttribute(SID))->GetIntval();
          assert(AlmostEqual(*p1,endpoints[0])&&AlmostEqual(*p2,endpoints[1]));


          Tuple* edgetuple = new Tuple(nl->Second(xNumType2));

///////////////////////////////////////////////////////////////////////////
          end_index = j;

          trip->Get(start_index,up);
          timeInterval.start = up->timeInterval.start;
          trip->Get(end_index,up);
          timeInterval.end = up->timeInterval.start;
          Periods* peri = new Periods(1);
          peri->StartBulkLoad();
          peri->Add(timeInterval);
          peri->EndBulkLoad(true);

          MPoint* temp_mp = new MPoint(0);
          temp_mp->StartBulkLoad();
          for(;start_index < end_index;start_index++){//no static behavior
            trip->Get(start_index,up);
            temp_mp->Add(*up);
          }
          temp_mp->EndBulkLoad(true);

//          cout<<timeInterval<<endl;
//          cout<<*temp_mp<<endl;
/////////////////////////////////////////////////////////////////////////////
          edgetuple->PutAttribute(EID,new CcInt(true,eid));
          edgetuple->PutAttribute(V1, new CcInt(true,nid1));
          edgetuple->PutAttribute(V2, new CcInt(true,nid2));

          edgetuple->PutAttribute(DEF_T, peri);
          edgetuple->PutAttribute(LINE, new Line(*line));
          edgetuple->PutAttribute(FEE,new CcReal(true,costfee));
          edgetuple->PutAttribute(PID,new CcInt(true,routeid));
          edgetuple->PutAttribute(MOVE,temp_mp);
          edgetuple->PutAttribute(RPID,new CcInt(*rpid));
          edgetuple->PutAttribute(P1,new Point(*p1));
          edgetuple->PutAttribute(P2,new Point(*p2));
          temp_bus_edge->AppendTuple(edgetuple);
          tuple_p1->DeleteIfAllowed();
          tuple_p2->DeleteIfAllowed();
          edgetuple->DeleteIfAllowed();
          eid++;
          delete line;
//          temp_mp->Clear();
          start_index = j+1; //no static behavior
//////////////////////////////////////////////////////////////////////////
          endpoints.clear();
          connectpoints.clear();

          endpoints.push_back(p0);//next line
          connectpoints.push_back(p0);
        }else{
          //the prorgram should never come here
          assert(false);
        }
      }else{ //not AlmostEqual
        //add points
         connectpoints.push_back(p1);

         if(j == trip->GetNoComponents() - 1){ //add a trip without middle stop
          endpoints.push_back(p1);

          Line* line = new Line(0);
          line->StartBulkLoad();
          int edgeno = 0;
          HalfSegment hs;
          for(unsigned int k = 0;k < connectpoints.size() - 1;k++){
            Point start = connectpoints[k];
            Point end = connectpoints[k+1];
            hs.Set(true,start,end);
            hs.attr.edgeno = ++edgeno;
            *line += hs;
            hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
            *line += hs;
          }
          line->EndBulkLoad();

////////////////////    edge         ///////////////////////////////////////
          TupleId tid1 = FindPointTid(endpoints[0]);
          TupleId tid2 = FindPointTid(endpoints[1]);
//          cout<<id1<<" "<<id2<<endl;
          assert(tid1 != tid2);
//          cout<<endpoints[0]<<" "<<endpoints[1]<<endl;
          Tuple* tuple_p1 = bus_node->GetTuple(tid1);
          Tuple* tuple_p2 = bus_node->GetTuple(tid2);
//          cout<<*tuple_p1<<" "<<*tuple_p2<<endl;
          Point* p1 = (Point*)tuple_p1->GetAttribute(LOC);
          Point* p2 = (Point*)tuple_p2->GetAttribute(LOC);
          int nid1 = ((CcInt*)tuple_p1->GetAttribute(SID))->GetIntval();
          int nid2 = ((CcInt*)tuple_p2->GetAttribute(SID))->GetIntval();

          assert(AlmostEqual(*p1,endpoints[0])&&AlmostEqual(*p2,endpoints[1]));

    ///////////////////////////////////////////////////////////////////////////
          end_index = j;
//          cout<<start_index<<" "<<end_index<<endl;
          trip->Get(start_index,up);
          timeInterval.start = up->timeInterval.start;
          trip->Get(end_index,up);
          timeInterval.end = up->timeInterval.end;

          Periods* peri = new Periods(1);
          peri->StartBulkLoad();
          peri->Add(timeInterval);
          peri->EndBulkLoad(true);

          MPoint* temp_mp = new MPoint(0);
          temp_mp->StartBulkLoad();
          for(;start_index <= end_index;start_index++){//no static behavior
            trip->Get(start_index,up);
            temp_mp->Add(*up);
          }
          temp_mp->EndBulkLoad(true);
//          cout<<timeInterval<<endl;
//          cout<<*temp_mp<<endl;
////////////////////////////////////////////////////////////////////////////
          Tuple* edgetuple = new Tuple(nl->Second(xNumType2));
          edgetuple->PutAttribute(EID,new CcInt(true,eid));
          edgetuple->PutAttribute(V1, new CcInt(true,nid1));
          edgetuple->PutAttribute(V2, new CcInt(true,nid2));
          edgetuple->PutAttribute(DEF_T,peri);
          edgetuple->PutAttribute(LINE, new Line(*line));
          edgetuple->PutAttribute(FEE,new CcReal(true,costfee));
          edgetuple->PutAttribute(PID,new CcInt(true,routeid));
          edgetuple->PutAttribute(MOVE,temp_mp);
          edgetuple->PutAttribute(RPID,new CcInt(*rpid));
          edgetuple->PutAttribute(P1,new Point(*p1));
          edgetuple->PutAttribute(P2,new Point(*p2));

          temp_bus_edge->AppendTuple(edgetuple);
          tuple_p1->DeleteIfAllowed();
          tuple_p2->DeleteIfAllowed();
          edgetuple->DeleteIfAllowed();
          eid++;
          delete line;

          endpoints.clear();
          connectpoints.clear();
        }
      }
    }
    tuple->DeleteIfAllowed();
  }



///////////////////relation for edge///////////////////////////////////
//  cout<<temp_bus_edge->GetNoTuples()<<endl;
  ostringstream xBusEdgePtrStream;
  xBusEdgePtrStream << (long)temp_bus_edge;
//  string strQuery = "(consume(sort(feed(" + busedgeTypeInfo +
//                "(ptr " + xBusEdgePtrStream.str() + ")))))";

//  string strQuery = "(consume(sortby(feed(" + busedgeTypeInfo +
//         "(ptr " + xBusEdgePtrStream.str() + ")))((v1 asc))))";

  string sq1 = ")))((start_t(fun(tuple1 TUPLE)(minimum(attr tuple1 def_t))))))";
  string sq2 = "((v1 asc)(start_t desc)))(start_t)))";
  string strQuery = "(consume(remove(sortby(extend(feed(" + busedgeTypeInfo +
  "(ptr " + xBusEdgePtrStream.str() + sq1 + sq2;

/*(consume
        (remove
            (sortby
                (extend
                    (feed
                        (busedge berlintrains))
                    (
                        (start_t
                            (fun
                                (tuple1 TUPLE)
                                (minimum
                                    (attr tuple1 def_t))))))
                (
                    (v1 asc)
                    (start_t desc)))
            (start_t))) */

/*"(bulkloadrtree
                (sortby
                      (addid
                            (feed(" + busstopTypeInfo +
            "(ptr " + xBusStop.str() + ")
                                 )
                            )
                      )
                ((BusStop asc))
                )
               BusStop)";*/


//  cout<<strQuery<<endl;

  Word xResult;
  int QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  bus_edge = (Relation*)xResult.addr;
  temp_bus_edge->Delete();
  cout<<"edge relation is finished..."<<endl;
 /////////////////b-tree on edge///////////////////////////////////
  ostringstream xThisBusEdgePtrStream;
  xThisBusEdgePtrStream<<(long)bus_edge;
  strQuery = "(createbtree (" + busedgeTypeInfo +
             "(ptr " + xThisBusEdgePtrStream.str() + "))" + "eid)";
  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  btree_bus_edge = (BTree*)xResult.addr;
  cout<<"b-tree on edge id is built..."<<endl;
  /*********************b-tree on edge start node id******************/
  ostringstream xThisBusEdgeV1PtrStream;
  xThisBusEdgeV1PtrStream<<(long)bus_edge;
  strQuery = "(createbtree (" + busedgeTypeInfo +
             "(ptr " + xThisBusEdgeV1PtrStream.str() + "))" + "v1)";
  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  btree_bus_edge_v1 = (BTree*)xResult.addr;
  cout<<"b-tree on edge start node id is built..."<<endl;
  /*********************b-tree on edge end node id******************/
  ostringstream xThisBusEdgeV2PtrStream;
  xThisBusEdgeV2PtrStream<<(long)bus_edge;
  strQuery = "(createbtree (" + busedgeTypeInfo +
             "(ptr " + xThisBusEdgeV2PtrStream.str() + "))" + "v2)";
  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  btree_bus_edge_v2 = (BTree*)xResult.addr;
  cout<<"b-tree on edge end node id is built..."<<endl;
  /*******************b-tree one edge path id *******************/
  ostringstream xThisBusEdgePathPtrStream;
  xThisBusEdgePathPtrStream<<(long)bus_edge;
  strQuery = "(createbtree (" + busedgeTypeInfo +
             "(ptr " + xThisBusEdgePathPtrStream.str() + "))" + "pid)";
  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  btree_bus_edge_path = (BTree*)xResult.addr;
  cout<<"b-tree on edge path id is built..."<<endl;
}

void BusNetwork::FillBusEdge_New(const Relation* in_busRoute)
{
  cout<<"create new edge relation..."<<endl;
  vector<Point> endpoints;//store start and end pointss
  vector<Point> connectpoints; //store points between two stops
  Interval<Instant> timeInterval;

  //relation description for edge

  ListExpr xTypeInfoEdge;
  nl->ReadFromString(busedgeTypeInfo,xTypeInfoEdge);
  ListExpr xNumType2 =
                SecondoSystem::GetCatalog()->NumericType(xTypeInfoEdge);
  Relation* temp_bus_edge = new Relation(xNumType2,true);

  int eid = 1;
  for(int i = 1; i <=  in_busRoute->GetNoTuples();i++){
    Tuple* tuple = in_busRoute->GetTuple(i);
    MPoint* trip = (MPoint*)tuple->GetAttribute(TRIP);
    const UPoint* up;
    CcInt* rid = (CcInt*)tuple->GetAttribute(LINENO);//bus line no
    int routeid = rid->GetIntval();
    CcInt* rpid = (CcInt*)tuple->GetAttribute(RID);//path id

    endpoints.clear();
    connectpoints.clear();
    /////generate cost fee /////
    srand(time(0));
    float costfee = (10+rand() % 20)/100.0;
//    cout<<costfee<<endl;
    srand(i);
    //////////
    int start_index=0;
    int end_index=0;
//    cout<<*trip<<endl;
    for(int j = 0;j < trip->GetNoComponents();j++){
      trip->Get(j,up);
      Point p0 = up->p0;
      Point p1 = up->p1;

      if(j == 0 ){ //add the start location as a stop
        endpoints.push_back(p0);
        connectpoints.push_back(p0);
        start_index = j;
        if(AlmostEqual(p0,p1))
          start_index++;
        if(!AlmostEqual(p0,p1))
          connectpoints.push_back(p1);

        continue;
      }

      if(AlmostEqual(p0,p1)){
        endpoints.push_back(p0);

        if(endpoints.size() == 2){ //extract edge
 //       cout<<i<<" Interval "<<timeInterval<<" "<<connectpoints.size()<<endl;
          //create line
          Line* line = new Line(0);
          line->StartBulkLoad();
          int edgeno = 0;
          HalfSegment hs;
          for(unsigned int k = 0;k < connectpoints.size() - 1;k++){
            Point start = connectpoints[k];
            Point end = connectpoints[k+1];
            hs.Set(true,start,end);
            hs.attr.edgeno = ++edgeno;
            *line += hs;
            hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
            *line += hs;
          }
          line->EndBulkLoad();
 ////////////////////    edge         ///////////////////////////////////////
          TupleId tid1 = FindPointTidNew(i,endpoints[0]);
          TupleId tid2 = FindPointTidNew(i,endpoints[1]);
//          cout<<id1<<" "<<id2<<endl;
          assert(tid1 != tid2 && tid1 != 0 && tid2 != 0);
//          cout<<endpoints[0]<<" "<<endpoints[1]<<endl;
          Tuple* tuple_p1 = bus_node_new->GetTuple(tid1);
          Tuple* tuple_p2 = bus_node_new->GetTuple(tid2);
//          cout<<*tuple_p1<<" "<<*tuple_p2<<endl;
          Point* p1 = (Point*)tuple_p1->GetAttribute(NEWLOC);
          Point* p2 = (Point*)tuple_p2->GetAttribute(NEWLOC);
          int nid1 = ((CcInt*)tuple_p1->GetAttribute(NEWSID))->GetIntval();
          int nid2 = ((CcInt*)tuple_p2->GetAttribute(NEWSID))->GetIntval();
          assert(AlmostEqual(*p1,endpoints[0])&&AlmostEqual(*p2,endpoints[1]));

          Tuple* edgetuple = new Tuple(nl->Second(xNumType2));

///////////////////////////////////////////////////////////////////////////
          end_index = j;

          trip->Get(start_index,up);
          timeInterval.start = up->timeInterval.start;
          trip->Get(end_index,up);
          timeInterval.end = up->timeInterval.start;
          Periods* peri = new Periods(1);
          peri->StartBulkLoad();
          peri->Add(timeInterval);
          peri->EndBulkLoad(true);

          MPoint* temp_mp = new MPoint(0);
          temp_mp->StartBulkLoad();
          for(;start_index < end_index;start_index++){//no static behavior
            trip->Get(start_index,up);
            temp_mp->Add(*up);
          }
          temp_mp->EndBulkLoad(true);

//          cout<<timeInterval<<endl;
//          cout<<*temp_mp<<endl;
/////////////////////////////////////////////////////////////////////////////
          edgetuple->PutAttribute(EID,new CcInt(true,eid));
          edgetuple->PutAttribute(V1, new CcInt(true,nid1));
          edgetuple->PutAttribute(V2, new CcInt(true,nid2));

          edgetuple->PutAttribute(DEF_T, peri);
          edgetuple->PutAttribute(LINE, new Line(*line));
          edgetuple->PutAttribute(FEE,new CcReal(true,costfee));
          edgetuple->PutAttribute(PID,new CcInt(true,routeid));
          edgetuple->PutAttribute(MOVE,temp_mp);
          edgetuple->PutAttribute(RPID,new CcInt(*rpid));
          edgetuple->PutAttribute(P1,new Point(*p1));
          edgetuple->PutAttribute(P2,new Point(*p2));
          temp_bus_edge->AppendTuple(edgetuple);
          tuple_p1->DeleteIfAllowed();
          tuple_p2->DeleteIfAllowed();
          edgetuple->DeleteIfAllowed();
          eid++;
          delete line;
//          temp_mp->Clear();
          start_index = j+1; //no static behavior
//////////////////////////////////////////////////////////////////////////
          endpoints.clear();
          connectpoints.clear();

          endpoints.push_back(p0);//next line
          connectpoints.push_back(p0);
        }else{
          //the prorgram should never come here
          assert(false);
        }
      }else{ //not AlmostEqual
        //add points
         connectpoints.push_back(p1);

         if(j == trip->GetNoComponents() - 1){ //add a trip without middle stop
          endpoints.push_back(p1);

          Line* line = new Line(0);
          line->StartBulkLoad();
          int edgeno = 0;
          HalfSegment hs;
          for(unsigned int k = 0;k < connectpoints.size() - 1;k++){
            Point start = connectpoints[k];
            Point end = connectpoints[k+1];
            hs.Set(true,start,end);
            hs.attr.edgeno = ++edgeno;
            *line += hs;
            hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
            *line += hs;
          }
          line->EndBulkLoad();

////////////////////    edge         ///////////////////////////////////////
          TupleId tid1 = FindPointTidNew(i,endpoints[0]);
          TupleId tid2 = FindPointTidNew(i,endpoints[1]);
//          cout<<id1<<" "<<id2<<endl;
          assert(tid1 != tid2);
//          cout<<endpoints[0]<<" "<<endpoints[1]<<endl;
          Tuple* tuple_p1 = bus_node_new->GetTuple(tid1);
          Tuple* tuple_p2 = bus_node_new->GetTuple(tid2);
//          cout<<*tuple_p1<<" "<<*tuple_p2<<endl;
          Point* p1 = (Point*)tuple_p1->GetAttribute(NEWLOC);
          Point* p2 = (Point*)tuple_p2->GetAttribute(NEWLOC);
          int nid1 = ((CcInt*)tuple_p1->GetAttribute(NEWSID))->GetIntval();
          int nid2 = ((CcInt*)tuple_p2->GetAttribute(NEWSID))->GetIntval();

          assert(AlmostEqual(*p1,endpoints[0])&&AlmostEqual(*p2,endpoints[1]));

    ///////////////////////////////////////////////////////////////////////////
          end_index = j;
//          cout<<start_index<<" "<<end_index<<endl;
          trip->Get(start_index,up);
          timeInterval.start = up->timeInterval.start;
          trip->Get(end_index,up);
          timeInterval.end = up->timeInterval.end;

          Periods* peri = new Periods(1);
          peri->StartBulkLoad();
          peri->Add(timeInterval);
          peri->EndBulkLoad(true);

          MPoint* temp_mp = new MPoint(0);
          temp_mp->StartBulkLoad();
          for(;start_index <= end_index;start_index++){//no static behavior
            trip->Get(start_index,up);
            temp_mp->Add(*up);
          }
          temp_mp->EndBulkLoad(true);
//          cout<<timeInterval<<endl;
//          cout<<*temp_mp<<endl;
////////////////////////////////////////////////////////////////////////////
          Tuple* edgetuple = new Tuple(nl->Second(xNumType2));
          edgetuple->PutAttribute(EID,new CcInt(true,eid));
          edgetuple->PutAttribute(V1, new CcInt(true,nid1));
          edgetuple->PutAttribute(V2, new CcInt(true,nid2));
          edgetuple->PutAttribute(DEF_T,peri);
          edgetuple->PutAttribute(LINE, new Line(*line));
          edgetuple->PutAttribute(FEE,new CcReal(true,costfee));
          edgetuple->PutAttribute(PID,new CcInt(true,routeid));
          edgetuple->PutAttribute(MOVE,temp_mp);
          edgetuple->PutAttribute(RPID,new CcInt(*rpid));
          edgetuple->PutAttribute(P1,new Point(*p1));
          edgetuple->PutAttribute(P2,new Point(*p2));

          temp_bus_edge->AppendTuple(edgetuple);
          tuple_p1->DeleteIfAllowed();
          tuple_p2->DeleteIfAllowed();
          edgetuple->DeleteIfAllowed();
          eid++;
          delete line;

          endpoints.clear();
          connectpoints.clear();
        }
      }
    }
    tuple->DeleteIfAllowed();
  }


///////////////////relation for edge///////////////////////////////////
//  cout<<temp_bus_edge->GetNoTuples()<<endl;
  ostringstream xBusEdgePtrStream;
  xBusEdgePtrStream << (long)temp_bus_edge;
//  string strQuery = "(consume(sort(feed(" + busedgeTypeInfo +
//                "(ptr " + xBusEdgePtrStream.str() + ")))))";

//  string strQuery = "(consume(sortby(feed(" + busedgeTypeInfo +
//         "(ptr " + xBusEdgePtrStream.str() + ")))((v1 asc))))";

  string sq1 = ")))((start_t(fun(tuple1 TUPLE)(minimum(attr tuple1 def_t))))))";
  string sq2 = "((v1 asc)(start_t desc)))(start_t)))";
  string strQuery = "(consume(remove(sortby(extend(feed(" + busedgeTypeInfo +
  "(ptr " + xBusEdgePtrStream.str() + sq1 + sq2;

/*(consume
        (remove
            (sortby
                (extend
                    (feed
                        (busedge berlintrains))
                    (
                        (start_t
                            (fun
                                (tuple1 TUPLE)
                                (minimum
                                    (attr tuple1 def_t))))))
                (
                    (v1 asc)
                    (start_t desc)))
            (start_t))) */

/*"(bulkloadrtree
                (sortby
                      (addid
                            (feed(" + busstopTypeInfo +
            "(ptr " + xBusStop.str() + ")
                                 )
                            )
                      )
                ((BusStop asc))
                )
               BusStop)";*/

//  cout<<strQuery<<endl;

  Word xResult;
  int QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  bus_edge_new = (Relation*)xResult.addr;
  temp_bus_edge->Delete();
  cout<<"new edge relation is finished..."<<endl;
/*for(int i = 1; i <= bus_edge_new->GetNoTuples();i++){
    Tuple* tuple = bus_edge_new->GetTuple(i);
    cout<<*tuple<<endl;
    tuple->DeleteIfAllowed();
  }*/

//  cout<<bus_edge_new->GetNoTuples()<<endl;

  /*****************b-tree on edge path id**************************/

  ostringstream xThisBusEdgePathPtrStream;
  xThisBusEdgePathPtrStream<<(long)bus_edge_new;
  strQuery = "(createbtree (" + busedgeTypeInfo +
             "(ptr " + xThisBusEdgePathPtrStream.str() + "))" + "pid)";
  QueryExecuted = QueryProcessor::ExecuteQuery(strQuery,xResult);
  assert(QueryExecuted);
  btree_bus_edge_path_new = (BTree*)xResult.addr;
  cout<<"new b-tree on edge path id is built..."<<endl;

}
/*
store the adjacency list, for each edge it records which edge comes after it

*/
struct SmallEdge{
  int e_tid;
  Interval<Instant> interval;
  int pid;
  int rid;
  int end_node_id;
  SmallEdge(int tid,Interval<Instant> interv,int pathid,int routeid,int e_node)
  :e_tid(tid),interval(interv),pid(pathid),rid(routeid),end_node_id(e_node){}
  SmallEdge(const SmallEdge& se)
  :e_tid(se.e_tid),interval(se.interval),
   pid(se.pid),rid(se.rid),end_node_id(se.end_node_id){}
  SmallEdge& operator=(const SmallEdge& se)
  {
    e_tid = se.e_tid;
    interval = se.interval;
    pid = se.pid;
    rid = se.rid;
    end_node_id = se.end_node_id;
    return *this;
  }
};


void BusNetwork::FillAdjacency()
{
  cout<<"build adjacency list..."<<endl;
  for(int i = 1;i <= bus_edge->GetNoTuples();i++){
    Tuple* t = bus_edge->GetTuple(i);
//    cout<<"i "<<i<<" tuple id "<<t->GetTupleId()<<endl;
    CcInt* end_node = (CcInt*)t->GetAttribute(V2);
    Periods* def_t = (Periods*)t->GetAttribute(DEF_T);
    const Interval<Instant>* interval;
    def_t->Get(0,interval);

    BTreeIterator* btree_iter = btree_bus_edge_v1->ExactMatch(end_node);
    int start = adjacencylist.Size();
    int end = 0;
    vector<SmallEdge> temp;
    while(btree_iter->Next()){
      Tuple* edge_tuple = bus_edge->GetTuple(btree_iter->GetId());

      Periods* def_t_next = (Periods*)edge_tuple->GetAttribute(DEF_T);
      const Interval<Instant>* interval_next;
      def_t_next->Get(0,interval_next);
      CcInt* rid = (CcInt*)edge_tuple->GetAttribute(PID);
      CcInt* pathid = (CcInt*)edge_tuple->GetAttribute(RPID);
      CcInt* v2 = (CcInt*)edge_tuple->GetAttribute(V2);
      if(interval->end < interval_next->start){
//////////////////////////////////////////////////////////////////////////////
      SmallEdge smalledge(btree_iter->GetId(),*interval_next,
                          pathid->GetIntval(),rid->GetIntval(),v2->GetIntval());
        if(temp.size() == 0){
          temp.push_back(smalledge);
        }
        else{
            unsigned int i = 0;
            for(;i < temp.size();i++){
              if(temp[i].rid == smalledge.rid &&
                  temp[i].end_node_id == smalledge.end_node_id){
                if(temp[i].interval.start > smalledge.interval.start)
                  temp[i] = smalledge;
                break;
              }
            }
            if(i == temp.size())
              temp.push_back(smalledge);
          }
//////////////////////////////////////////////////////////////////////////////

//        adjacencylist.Append(btree_iter->GetId());
      }
      edge_tuple->DeleteIfAllowed();
    }//end while
  //////////////////////////////
      for(unsigned int i = 0;i < temp.size();i++)
            adjacencylist.Append(temp[i].e_tid);
    temp.clear();
  /////////////////////////////
    end = adjacencylist.Size();
//    cout<<"start "<<start<<" end "<<end<<endl;
    //[start,end)
    adjacencylist_index.Append(ListEntry(start,end)); //start from 0 = tid - 1

    delete btree_iter;
    t->DeleteIfAllowed();
  }
  cout<<"adjacency list is created"<<endl;
//  cout<<adjacencylist.Size()<<endl;

  adjacencylist_index.TrimToSize();
  adjacencylist.TrimToSize();
}
/*
store the adjacency list, for each path, it records which path it can swith to

*/
struct SubEdge{
  int e_tid1; //edge id1
  Interval<Instant> interval1;
  int e_tid2; //edge id2
  Interval<Instant> interval2;
  SubEdge(){}
  SubEdge(int id1, Interval<Instant> interv1,
  int id2, Interval<Instant> interv2):e_tid1(id1),interval1(interv1),
  e_tid2(id2),interval2(interv2){}
  SubEdge(const SubEdge& subedge):e_tid1(subedge.e_tid1),
  interval1(subedge.interval1),e_tid2(subedge.e_tid2),
  interval2(subedge.interval2){}
  SubEdge& operator=(const SubEdge& subedge)
  {
    e_tid1 = subedge.e_tid1;
    interval1 = subedge.interval1;
    e_tid2 = subedge.e_tid2;
    interval2 = subedge.interval2;
    return *this;
  }
  /*order edge by start time in descending order*/
  bool operator<(const SubEdge& subedge)const
  {
    if(interval2.start > subedge.interval2.start)
      return false;
    if(interval2.start == subedge.interval2.start)
      if(interval2.end > subedge.interval2.end)
        return false;
    return true;
  }
  void Print()
  {
    cout<<"eid1 "<<e_tid1<<" interval "<<interval1<<
        " eid2 "<<e_tid2<<" interval "<<interval2<<endl;
  }
};
void BusNetwork::FillAdjacencyPath()
{
    //DBArray<ListEntry> adj_path_index;
    //DBArray<SwithEntry> adj_path;
    for(int i = 1;i <= bus_route->GetNoTuples();i++){
        CcInt* id = new CcInt(true,i);
        BTreeIterator* btreeiter1 = btree_bus_edge_v2->ExactMatch(id);
        priority_queue<SubEdge> edges;
        while(btreeiter1->Next()){
          //get edge, time interval and end node
          Tuple* tuple1 = bus_edge->GetTuple(btreeiter1->GetId());
          Periods* peri1 = (Periods*)tuple1->GetAttribute(DEF_T);
          CcInt* end_node = (CcInt*)tuple1->GetAttribute(V2);
          CcInt* pathid1 = (CcInt*)tuple1->GetAttribute(RPID);
          const Interval<Instant>* interval1;
          peri1->Get(0,interval1);
//          edges.push(SubEdge(tuple->GetTupleId(),*interval));
          BTreeIterator* btreeiter2 = btree_bus_edge_v1->ExactMatch(end_node);
          while(btreeiter2->Next()){
            Tuple* tuple2 = bus_edge->GetTuple(btreeiter2->GetId());
            Periods* peri2 = (Periods*)tuple2->GetAttribute(DEF_T);
            const Interval<Instant>* interval2;
            peri2->Get(0,interval2);
            CcInt* pathid2 = (CcInt*)tuple2->GetAttribute(RPID);
            if(interval1->end < interval2->start &&
              pathid1->GetIntval() != pathid2->GetIntval()){
              edges.push(SubEdge(tuple1->GetTupleId(),*interval1,
                                 tuple2->GetTupleId(),*interval2));
            }
          }
          delete btreeiter2;

          tuple1->DeleteIfAllowed();
        }
//        int start = adj_path.Size();
//        int end = 0;
          cout<<"pathid "<<i<<" queue size "<<edges.size()<<endl;
          while(edges.empty() == false){
            SubEdge top = edges.top();
            edges.pop();
//            top.Print();
          }

        delete id;
        delete btreeiter1;

    }
}

void BusNetwork::Load(int in_iId,const Relation* in_busRoute)
{
  busnet_id = in_iId;
  bus_def = true;
  assert(in_busRoute != NULL);
#ifdef graph_model
  FillBusNode(in_busRoute);//for node
  FillBusEdge(in_busRoute);//for edge
  FillAdjacency();
#else
  FillBusNode_New(in_busRoute);
  FillBusEdge_New(in_busRoute);
  ConstructBusNodeTree();
#endif
  CalculateMaxSpeed();//calculate the maxspeed
}

/*
2 Type Constructors and Deconstructor

*/
BusNetwork::BusNetwork():
busnet_id(0),bus_def(false),bus_route(NULL),btree_bus_route(NULL),
bus_node(NULL),btree_bus_node(NULL),rtree_bus_node(NULL),bus_edge(NULL),
btree_bus_edge(NULL),btree_bus_edge_v1(NULL),btree_bus_edge_v2(NULL),
btree_bus_edge_path(NULL),maxspeed(0),
adjacencylist_index(0),adjacencylist(0),
bus_node_new(NULL),btree_bus_node_new1(NULL),btree_bus_node_new2(NULL),
bus_edge_new(NULL),btree_bus_edge_path_new(NULL), bus_node_tree(NULL),
bus_tree(0),ps_zval(0)
//adj_path_index(0),adj_path(0)
{

}

void BusNetwork::Destroy()
{
  cout<<"destroy"<<endl;
  //bus route
  if(bus_route != NULL){
    bus_route->Delete();
    bus_route = NULL;
  }
  //b-tree on bus route
  if(btree_bus_route != NULL){
    delete btree_bus_route;
    btree_bus_route = NULL;
  }

#ifdef graph_model
 //bus stop
  if(bus_node != NULL){
    bus_node->Delete();
    bus_node = NULL;
  }
  //b-tree on bus stop
  if(btree_bus_node != NULL){
    delete btree_bus_node;
    btree_bus_node = NULL;
  }
  //r-tree on bus stop
  if(rtree_bus_node != NULL){
    delete rtree_bus_node;
    rtree_bus_node = NULL;
  }

  //bus edge relation
  if(bus_edge != NULL){
    bus_edge->Delete();
    bus_edge = NULL;
  }
  //b-tree on bus edge id
  if(btree_bus_edge != NULL){
    delete btree_bus_edge;
    btree_bus_edge = NULL;
  }
  //b-tree on bus edge start node id
  if(btree_bus_edge_v1 != NULL){
    delete btree_bus_edge_v1;
    btree_bus_edge_v1 = NULL;
  }
  //b-tree on bus edge end node id
  if(btree_bus_edge_v2 != NULL){
    delete btree_bus_edge_v2;
    btree_bus_edge_v2 = NULL;
  }
  //b-tree on bus edge path id
  if(btree_bus_edge_path != NULL){
    delete btree_bus_edge_path;
    btree_bus_edge_path = NULL;
  }
#else
  if(bus_node != NULL){
    bus_node->Delete();
    bus_node = NULL;
  }
  //new bus node relation
  if(bus_node_new != NULL){
    bus_node_new->Delete();
    bus_node_new = NULL;
  }

  //btree for new bus node
  if(btree_bus_node_new1 != NULL){
    delete btree_bus_node_new1;
    btree_bus_node_new1 = NULL;
  }
  if(btree_bus_node_new2 != NULL){
    delete btree_bus_node_new2;
    btree_bus_node_new2 = NULL;
  }

  //new bus edge relation
  if(bus_edge_new != NULL){
    bus_edge_new->Delete();
    bus_edge_new = NULL;
  }
  //b-tree on bus edge path id (new)
  if(btree_bus_edge_path_new != NULL){
    delete btree_bus_edge_path_new;
    btree_bus_edge_path_new = NULL;
  }

  if(bus_node_tree != NULL){
    bus_node_tree->Delete();
    bus_node_tree = NULL;
  }

#endif

}

BusNetwork::BusNetwork(SmiRecord& in_xValueRecord,size_t& inout_iOffset,
const ListExpr in_xTypeInfo)
{
//  cout<<"BusNetwork() 2"<<endl;
  /***********************Read busnetwork id********************************/
  in_xValueRecord.Read(&busnet_id,sizeof(int),inout_iOffset);
  inout_iOffset += sizeof(int);
  in_xValueRecord.Read(&maxspeed,sizeof(double),inout_iOffset);
  inout_iOffset += sizeof(double);

  ListExpr xType;
  ListExpr xNumericType;
  /***********************Open relation for bus routes*********************/
//  cout<<"open bus_route"<<endl;
  nl->ReadFromString(busrouteTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  bus_route = Relation::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!bus_route) {
    return;
  }

  /*******************b-tree on bus routes********************************/
//  cout<<"open b-tree on bus route"<<endl;
  nl->ReadFromString(btreebusrouteTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  btree_bus_route = BTree::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!btree_bus_route) {
    bus_route->Delete();
    return;
  }

#ifdef graph_model
  /***************************Open relation for nodes*********************/
//  cout<<"open bus_node"<<endl;
  nl->ReadFromString(busstopTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  bus_node = Relation::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!bus_node) {
    bus_route->Delete();
    delete btree_bus_route;
    return;
  }

  /***********************Open btree for bus stop************************/
//  cout<<"open btree bus_node"<<endl;
  nl->ReadFromString(btreebusstopTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  btree_bus_node = BTree::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!btree_bus_node){
    bus_route->Delete();
    delete btree_bus_route;
    bus_node->Delete();
    return ;
  }
///////// Test B-tree on Nodes//////////////////////
/*  for(int i = 1;i <= bus_node->GetNoTuples();i++){
    CcInt* id = new CcInt(true,i);
    BTreeIterator* btreeiter = btree_bus_node->ExactMatch(id);
    while(btreeiter->Next()){
      Tuple* tuple = bus_node->GetTuple(btreeiter->GetId());
      cout<<*tuple<<endl;
      tuple->DeleteIfAllowed();
    }
    delete id;
    delete btreeiter;
  }*/
///////////////////////////////////////////////////

/***************************Open R-tree for bus stop************************/
//  cout<<"open rtree bus_node"<<endl;
  Word xValue;
  if(!(rtree_bus_node->Open(in_xValueRecord,inout_iOffset,
                          rtreebusstopTypeInfo,xValue))){
    bus_route->Delete();
    delete btree_bus_route;
    bus_node->Delete();
    delete btree_bus_node;
    return ;
  }
  rtree_bus_node = (R_Tree<2,TupleId>*)xValue.addr;
////////////////////Test R-tree ////////////////////////////////
//  cout<<rtree_bus_node->NodeCount()<<endl;
//  rtree_bus_node->BoundingBox().Print(cout);


  ////////////Test b-tree on new edges////////////////////////
/*for(int i = 1;i <= bus_route->GetNoTuples();i++){
    CcInt* id = new CcInt(true,i);
    BTreeIterator* btreeiter = btree_bus_edge_path_new->ExactMatch(id);
    while(btreeiter->Next()){
      Tuple* tuple = bus_edge_new->GetTuple(btreeiter->GetId());
      cout<<*tuple<<endl;
      tuple->DeleteIfAllowed();
    }
    delete id;
    delete btreeiter;
  }*/

  /****************************edges*********************************/
//  cout<<"open bus_edge"<<endl;
  nl->ReadFromString(busedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  bus_edge = Relation::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!bus_edge){
    bus_route->Delete();
    delete btree_bus_route;
    bus_node->Delete();
    delete btree_bus_node;
    delete rtree_bus_node;
    return ;
  }


  /*************************b-tree on edges (eid)****************************/
//  cout<<"open b-tree bus_edge id"<<endl;
  nl->ReadFromString(btreebusedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  btree_bus_edge = BTree::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!btree_bus_edge){
    bus_route->Delete();
    delete btree_bus_route;
    bus_node->Delete();
    delete btree_bus_node;
    delete rtree_bus_node;
    bus_edge->Delete();
    return ;
  }
///////// Test B-tree on Edge //////////////////////
/*  for(int i = 1;i <= bus_edge->GetNoTuples();i++){
    CcInt* id = new CcInt(true,i);
    BTreeIterator* btreeiter = btree_bus_edge->ExactMatch(id);
    while(btreeiter->Next()){
      Tuple* tuple = bus_edge->GetTuple(btreeiter->GetId());
      cout<<*tuple<<endl;
      tuple->DeleteIfAllowed();
    }
    delete id;
    delete btreeiter;
  }*/
  /***********************b-tree on edge start node id************************/
//  cout<<"open b-tree bus_edge nid1"<<endl;
  nl->ReadFromString(btreebusedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  btree_bus_edge_v1 = BTree::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!btree_bus_edge_v1){
    bus_route->Delete();
    delete btree_bus_route;
    bus_node->Delete();
    delete btree_bus_node;
    delete rtree_bus_node;
    bus_edge->Delete();
    delete btree_bus_edge;
    return ;
  }

///////// Test B-tree on edge start node id //////////////////////
/*  for(int i = 1;i <= bus_node->GetNoTuples();i++){
    CcInt* id = new CcInt(true,i);
    BTreeIterator* btreeiter = btree_bus_edge_v1->ExactMatch(id);
    while(btreeiter->Next()){
      Tuple* tuple = bus_edge->GetTuple(btreeiter->GetId());
      cout<<*tuple<<endl;
      tuple->DeleteIfAllowed();
    }
    delete id;
    delete btreeiter;
  }*/

  /***********************b-tree on edge end node id************************/
//  cout<<"open b-tree bus_edge nid2"<<endl;
  nl->ReadFromString(btreebusedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  btree_bus_edge_v2 = BTree::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!btree_bus_edge_v2){
    bus_route->Delete();
    delete btree_bus_route;
    bus_node->Delete();
    delete btree_bus_node;
    delete rtree_bus_node;
    bus_edge->Delete();
    delete btree_bus_edge;
    delete btree_bus_edge_v1;
    return;
  }
  ///////// Test B-tree on edge start node id //////////////////////
/*  for(int i = 1;i <= bus_node->GetNoTuples();i++){
    CcInt* id = new CcInt(true,i);
    BTreeIterator* btreeiter = btree_bus_edge_v2->ExactMatch(id);
    while(btreeiter->Next()){
      Tuple* tuple = bus_edge->GetTuple(btreeiter->GetId());
      cout<<*tuple<<endl;
      tuple->DeleteIfAllowed();
    }
    delete id;
    delete btreeiter;
  }*/

  /******************Open btree on edge path id***************************/
  nl->ReadFromString(btreebusedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  btree_bus_edge_path = BTree::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!btree_bus_edge_path){
    bus_route->Delete();
    delete btree_bus_route;
    bus_node->Delete();
    delete btree_bus_node;
    delete rtree_bus_node;
    bus_edge->Delete();
    delete btree_bus_edge;
    delete btree_bus_edge_v1;
    delete btree_bus_edge_v2;
    return ;
  }
  ///////// Test B-tree on edge path id //////////////////////
/*  for(int i = 1;i <= bus_route->GetNoTuples();i++){ //1-562
    CcInt* id = new CcInt(true,i);
    BTreeIterator* btreeiter = btree_bus_edge_path->ExactMatch(id);
    while(btreeiter->Next()){
      Tuple* tuple = bus_edge->GetTuple(btreeiter->GetId());
      cout<<*tuple<<endl;
      tuple->DeleteIfAllowed();
    }
    delete id;
    delete btreeiter;
  }*/

  ///////////////adjacency list ////////////////////////////////////
  adjacencylist_index.OpenFromRecord(in_xValueRecord, inout_iOffset);
  adjacencylist.OpenFromRecord(in_xValueRecord, inout_iOffset);
#else

  nl->ReadFromString(busstopTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  bus_node = Relation::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!bus_node) {
    bus_route->Delete();
    delete btree_bus_route;
    return;
  }

/*********************Open new bus node relation*************************/
  nl->ReadFromString(newbusstopTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  bus_node_new = Relation::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!bus_node_new) {
    bus_route->Delete();
    delete btree_bus_route;
    return;
  }

  /***********************Open btree for new bus stop************************/
//  cout<<"open btree bus_node"<<endl;
  nl->ReadFromString(newbtreebusstopTypeInfo1,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  btree_bus_node_new1 = BTree::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!btree_bus_node_new1){
    bus_route->Delete();
    delete btree_bus_route;
    bus_node_new->Delete();
    return ;
  }
//////////////////////Test b-tree on new bus stop//////////////////////////
/*for(int i = 1;i <= bus_route->GetNoTuples();i++){
    CcInt* id = new CcInt(true,i);
    BTreeIterator* btreeiter = btree_bus_node_new->ExactMatch(id);
    while(btreeiter->Next()){
      Tuple* tuple = bus_node_new->GetTuple(btreeiter->GetId());
      cout<<*tuple<<endl;
      tuple->DeleteIfAllowed();
    }
    delete id;
    delete btreeiter;
  }*/

  nl->ReadFromString(newbtreebusstopTypeInfo2,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  btree_bus_node_new2 = BTree::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!btree_bus_node_new2){
    bus_route->Delete();
    delete btree_bus_route;
    bus_node_new->Delete();
    return ;
  }

//////////////////////Test b-tree on new bus stop//////////////////////////
/* Points* ps = new Points(0);
  ps->StartBulkLoad();
  for(int i = 1;i <= bus_node_new->GetNoTuples();i++){
    Tuple* t = bus_node_new->GetTuple(i);
    Point* p = (Point*)t->GetAttribute(NEWLOC);
    *ps += *p;
    t->DeleteIfAllowed();
  }
  ps->EndBulkLoad(true,true);
  for(int i = 0;i < ps->Size();i++){
    const Point* p;
    ps->Get(i,p);
    double zval = ZValue(*(const_cast<Point*>(p)));
    CcReal* id = new CcReal(true,zval);
    BTreeIterator* btreeiter = btree_bus_node_new2->ExactMatch(id);
    while(btreeiter->Next()){
      Tuple* tuple = bus_node_new->GetTuple(btreeiter->GetId());
      cout<<*tuple<<endl;
      tuple->DeleteIfAllowed();
    }
    delete id;
    delete btreeiter;
  }
  delete ps; */


  nl->ReadFromString(busedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  bus_edge_new = Relation::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!bus_edge_new){
    bus_route->Delete();
    delete btree_bus_route;
    bus_node_new->Delete();
    delete btree_bus_node_new1;
    delete btree_bus_node_new2;
    return ;
  }


  /*******************b-tree on new edges******************************/
  nl->ReadFromString(btreebusedgeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  btree_bus_edge_path_new =
              BTree::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!btree_bus_edge_path_new){
    bus_route->Delete();
    delete btree_bus_route;
    bus_node_new->Delete();
    delete btree_bus_node_new1;
    delete btree_bus_node_new2;
    bus_edge_new->Delete();
    return ;
  }

  /******************relation bus node *****************************/
  nl->ReadFromString(busstoptreeTypeInfo,xType);
  xNumericType = SecondoSystem::GetCatalog()->NumericType(xType);
  bus_node_tree = Relation::Open(in_xValueRecord,inout_iOffset,xNumericType);
  if(!bus_node_tree){
    bus_route->Delete();
    delete btree_bus_route;
    bus_node_new->Delete();
    delete btree_bus_node_new1;
    delete btree_bus_node_new2;
    bus_edge_new->Delete();
    delete btree_bus_edge_path_new;
    return ;
  }

  bus_tree.OpenFromRecord(in_xValueRecord, inout_iOffset);
  ps_zval.OpenFromRecord(in_xValueRecord, inout_iOffset);
#endif

//  adj_path_index.OpenFromRecord(in_xValueRecord, inout_iOffset);
//  adj_path.OpenFromRecord(in_xValueRecord, inout_iOffset);
   bus_def = true;
//  cout<<"maxspeed "<<maxspeed<<endl;
}
void BusNetwork::CalculateMaxSpeed()
{
  maxspeed = numeric_limits<float>::min();
  for(int i = 1;i <= bus_route->GetNoTuples();i++){
    Tuple* t = bus_route->GetTuple(i);
    MPoint* trip = (MPoint*)t->GetAttribute(TRIP);
    for(int j = 0;j < trip->GetNoComponents();j++){
      const UPoint* up;
      trip->Get(j,up);
      double t = (up->timeInterval.end - up->timeInterval.start).ToDouble();
      double dist = up->p0.Distance(up->p1);
      if(dist/t > maxspeed)
        maxspeed = dist/t;
    }
    t->DeleteIfAllowed();
  }
//  cout<<"max speed "<<maxspeed<<endl;
}

BusNetwork::BusNetwork(ListExpr in_xValue,int in_iErrorPos,
ListExpr& inout_xErrorInfo,bool& inout_bCorrect):
busnet_id(0),bus_def(false),bus_route(NULL),btree_bus_route(NULL),
bus_node(NULL),btree_bus_node(NULL),rtree_bus_node(NULL),bus_edge(NULL),
btree_bus_edge(NULL),btree_bus_edge_v1(NULL),btree_bus_edge_v2(NULL),
btree_bus_edge_path(NULL),maxspeed(0),
adjacencylist_index(0),adjacencylist(0),
bus_node_new(NULL), btree_bus_node_new1(NULL),btree_bus_node_new2(NULL),
bus_edge_new(NULL),btree_bus_edge_path_new(NULL),bus_node_tree(NULL),
bus_tree(0),ps_zval(0)
{
  cout<<"BusNetwork() 3"<<endl;
  if(!(nl->ListLength(in_xValue) == 2)){
    string strErrorMessage = "BusNetwork(): List length must be 2";
    inout_xErrorInfo =
      nl->Append(inout_xErrorInfo,nl->StringAtom(strErrorMessage));
      inout_bCorrect = false;
      return;
  }
  //Get type-info fro temporary table

  //Split into the two parts
  ListExpr xIdList = nl->First(in_xValue);
//  ListExpr xRouteList = nl->Second(in_xValue);

  //Read Id
  if(!nl->IsAtom(xIdList) || nl->AtomType(xIdList) != IntType) {
    string strErrorMessage = "BusNetwork(): Id is missing)";
    inout_xErrorInfo = nl->Append(inout_xErrorInfo,
                       nl->StringAtom(strErrorMessage));
    inout_bCorrect = false;
    return;
  }
  busnet_id = nl->IntValue(xIdList);
  cout<<"bus id "<<busnet_id<<endl;
  bus_def = true;
}
/****************************Application Function***************************/

/*
Check whether it is reachable from a start node to an end node

*/
struct Elem{
  int edge_tuple_id;
  int pre_eid;
  double dist; //network distance to start node + Euclidan distance to end node
  Interval<Instant> interval;
  int s_node_id;
  int e_node_id;
  double delta_t;
  double delta_t_pre;
  int rid;
  int pre_edge_tid;
  int rpid;
  Elem(int id,Interval<Instant> interv,int snid):edge_tuple_id(id),
  interval(interv),s_node_id(snid),e_node_id(snid)
  {
    pre_eid=-1;
    dist=0.0;
    delta_t=0.0;
    delta_t_pre = 0.0;
    rid=0;
    pre_edge_tid = 0;
    rpid = 0;
  }
  Elem(const Elem& e):
  edge_tuple_id(e.edge_tuple_id),pre_eid(e.pre_eid),
  dist(e.dist),interval(e.interval),
  s_node_id(e.s_node_id),e_node_id(e.e_node_id),
  delta_t(e.delta_t),delta_t_pre(e.delta_t_pre),
  rid(e.rid),pre_edge_tid(e.pre_edge_tid),rpid(e.rpid){}
  Elem& operator=(const Elem& e)
  {
    edge_tuple_id = e.edge_tuple_id;
    pre_eid = e.pre_eid;
    dist = e.dist;
    interval = e.interval;
    s_node_id = e.s_node_id;
    e_node_id = e.e_node_id;
    delta_t = e.delta_t;
    delta_t_pre = e.delta_t_pre;
    rid = e.rid;
    pre_edge_tid = e.pre_edge_tid;
    rpid = e.rpid;
    return *this;
  }
  bool operator<(const Elem& e)const
  {
      if(delta_t < e.delta_t)
        return false;
      return true;

//    if(AlmostEqual(dist,e.dist)){
//      if(interval.start < e.interval.start)
//        return false;
//      return true;
//    }
//    if(dist < e.dist) return false;
//    return true;
  }
  void Print1()
  {
    cout<<"etid "<<edge_tuple_id<<" pre_eid "<<pre_eid<<
    " time "<<interval<<" delta_t "<<delta_t<<endl;
  }
  void Print2()
  {
    cout<<"etid "<<edge_tuple_id<<
    " time "<<interval<<"rid "<<rid<<" pid "<<rpid<<endl;
  }
};
/* use the time instant*/
class TimeCompare1{
public:
  bool operator()(const Elem& e1, const Elem& e2) const{
    if(e1.interval.start > e2.interval.start) return false;
    if(AlmostEqual(e1.interval.start.ToDouble(), e2.interval.start.ToDouble()))
      if(e1.interval.end > e2.interval.end)
        return false;
    return true;
  }
};

void BusNetwork::FindPath1(int id1,int id2,vector<int>& path,Instant* instant)
{
  ofstream outfile("temp_result"); //record info for debug

  if(id1 < 1 || id1 > bus_node->GetNoTuples()){
    cout<<"bus id is not valid"<<endl;
    return;
  }

  if(id2 < 1 || id2 > bus_node->GetNoTuples()){
    cout<<"bus id is not valid"<<endl;
    return;
  }
//  cout<<"start "<<id1<<" end "<<id2<<endl;
  //find all edges start from node id1, if time interval is given, filter
  //all edges start time earlier than it, no heuristic value

//  priority_queue<Elem,vector<Elem>,TimeCompare> q_list;
  priority_queue<Elem> q_list;
  vector<Elem> expansionlist;
  int expansion_count = 0;
  //Initialize list
  CcInt* start_id = new CcInt(true,id1);
  BTreeIterator* bt_iter_edge_v1 = btree_bus_edge_v1->ExactMatch(start_id);

  while(bt_iter_edge_v1->Next()){
    Tuple* t = bus_edge->GetTuple(bt_iter_edge_v1->GetId());
    CcInt* start_node = (CcInt*)t->GetAttribute(V1);
    CcInt* end_node = (CcInt*)t->GetAttribute(V2);
    Periods* peri = (Periods*)t->GetAttribute(DEF_T);
    const Interval<Instant>* interval;
    peri->Get(0,interval);
    if(path.size() != 0){ //middle stop
        TupleId edge_tid = path[path.size()-1];
        Tuple* edge_tuple = bus_edge->GetTuple(edge_tid);
        Periods* cur_def_t = (Periods*)edge_tuple->GetAttribute(DEF_T);
        const Interval<Instant>* interval_cur;
        cur_def_t->Get(0,interval_cur);
        //if user defined time instant, one more compare condition should be
        //considered
        //if ui1->lc == true

        if(interval->start > interval_cur->end){
            Elem e(bt_iter_edge_v1->GetId(),*interval,
                   start_node->GetIntval());
            e.delta_t = (interval->end-interval_cur->end).ToDouble();
            e.e_node_id = end_node->GetIntval();
            q_list.push(e);
        }
        edge_tuple->DeleteIfAllowed();
    }else{
//      if(interval->start > ui1->timeInterval.start){
      if(interval->start > *instant){
        Elem e(bt_iter_edge_v1->GetId(),*interval,
                  start_node->GetIntval());
//        e.delta_t = (interval->end-ui1->timeInterval.start).ToDouble();
        e.delta_t = (interval->end-*instant).ToDouble();
        e.e_node_id = end_node->GetIntval();
        q_list.push(e);
      }
    }
    t->DeleteIfAllowed();
  }

  delete bt_iter_edge_v1;
  delete start_id;
  cout<<"initialize size "<<q_list.size()<<endl;

//////////////////////////////////////////////////////////////////////////

  while(q_list.empty() == false){
    Elem top = q_list.top();
//    Tuple* edge_tuple = bus_edge->GetTuple(top.edge_tuple_id);
//    top.Print();

//    CcInt* start_node = (CcInt*)edge_tuple->GetAttribute(V1);
//    CcInt* end_node = (CcInt*)edge_tuple->GetAttribute(V2);
      CcInt* start_node = new CcInt(true,top.s_node_id);
      CcInt* end_node = new CcInt(true,top.e_node_id);

    if(end_node->GetIntval() == id2){//find the end
//      cout<<"find "<<endl;
      delete start_node;
      delete end_node;
      break;
    }

    expansionlist.push_back(top);
    expansion_count++;
    q_list.pop();

//    Periods* cur_def_t = (Periods*)edge_tuple->GetAttribute(DEF_T);
//    const Interval<Instant>* interval_cur;
//    cur_def_t->Get(0,interval_cur);

    Interval<Instant>* interval_cur = &top.interval;

//    cout<<"edge "<<edge_id->GetIntval()<<" v1 "<<start_node->GetIntval()
//        <<" v2 "<<end_node->GetIntval()<<" time "<<*interval_cur<<endl;

    /////////////get all edges from the same start node////////////////
    bt_iter_edge_v1 = btree_bus_edge_v1->ExactMatch(end_node);
    while(bt_iter_edge_v1->Next()){
     Tuple* t = bus_edge->GetTuple(bt_iter_edge_v1->GetId());

     CcInt* start_node_next = (CcInt*)t->GetAttribute(V1);//start node
     CcInt* end_node_next = (CcInt*)t->GetAttribute(V2);//end node
     Periods* next_def_t = (Periods*)t->GetAttribute(DEF_T);

     const Interval<Instant>* interval_next;
     next_def_t->Get(0,interval_next);
   //time instant of next edge should be late than cur edge
   //end node id of next edge should not be equal to start node id of cur edge
//     cout<<"next "<<*interval_next<<endl;
//     cout<<*trip<<endl;

      if(interval_next->start > interval_cur->end &&
           end_node_next->GetIntval() != start_node->GetIntval()){
            //store all edges from the same start node
            Elem e(bt_iter_edge_v1->GetId(),*interval_next,
                  start_node_next->GetIntval());
            e.pre_eid = expansion_count - 1;
            e.delta_t = top.delta_t +
                          (interval_next->end-interval_cur->end).ToDouble();
            e.e_node_id = end_node_next->GetIntval();
            q_list.push(e);
        }
        t->DeleteIfAllowed();
      }
      delete bt_iter_edge_v1;
    //////////////////////////////////////////////////////////////////////////
    delete start_node;
    delete end_node;
  }

//  cout<<expansionlist.size()<<endl;
  if(q_list.empty() == false){
    stack<Elem> temp_path;
    Elem top = q_list.top();
    temp_path.push(top);
//    cout<<top.edge_tuple_id<<endl;
    while(top.pre_eid != -1){
      int id = top.pre_eid;
//      cout<<"pre id "<<id<<endl;
      top = expansionlist[id];
//      cout<<top.edge_tuple_id<<endl;
      temp_path.push(top);
    }
//    cout<<temp_path.size()<<endl;
    while(temp_path.empty() == false){
      Elem top = temp_path.top();
      temp_path.pop();
      path.push_back(top.edge_tuple_id);
    }
  }

}

/* expand the graph by Dijkstra with minimum total time cost so far*/
void BusNetwork::FindPath_T_1(MPoint* mp,Relation* query,int attrpos,
Instant* instant)
{
//  cout<<"BusNetwork::Reachability"<<endl;
  if(query->GetNoTuples() < 2){
    cout<<"there is only start location, please give destination"<<endl;
    return;
  }

//  MPoint* mp = new MPoint(0);
  mp->Clear();
  mp->StartBulkLoad();

  vector<int> path; //record edge id
  for(int i = 1;i <= query->GetNoTuples() - 1;i++){
    Tuple* t1 = query->GetTuple(i);
    Tuple* t2 = query->GetTuple(i+1);
    CcInt* id1 = (CcInt*)t1->GetAttribute(attrpos);
    CcInt* id2 = (CcInt*)t2->GetAttribute(attrpos);
//    cout<<id1->GetIntval()<<" "<<id2->GetIntval()<<endl;

    FindPath1(id1->GetIntval(),id2->GetIntval(),path,instant);
    t1->DeleteIfAllowed();
    t2->DeleteIfAllowed();

  }
  /****************Construct the Reulst (MPoint)*************************/
  const UPoint* lastup = NULL;
  for(unsigned int i = 0;i < path.size();i++){
//    cout<<path[i]<<" ";
    Tuple* edge_tuple = bus_edge->GetTuple(path[i]);
    MPoint* temp_mp = (MPoint*)edge_tuple->GetAttribute(MOVE);
//    cout<<*temp_mp<<endl;
    for(int j = 0;j < temp_mp->GetNoComponents();j++){
      const UPoint* up;
      temp_mp->Get(j,up);
      //not the first trip
      if(lastup != NULL && i != 0 && j == 0){
        UPoint* insert_up = new UPoint(true);
//        cout<<"last "<<*lastup<<endl;
//        cout<<"cur "<<*up<<endl;
        insert_up->p0 = lastup->p1;
        insert_up->timeInterval.start = lastup->timeInterval.end;
        insert_up->p1 = up->p0;
        insert_up->timeInterval.end = up->timeInterval.start;
//        cout<<"insert "<<*insert_up<<endl;
        insert_up->timeInterval.lc = true;
        insert_up->timeInterval.rc = false;
        mp->Add(*insert_up);
        delete insert_up;
        delete lastup;
        lastup = NULL;
      }
      mp->Add(*up);
      if(j == temp_mp->GetNoComponents() - 1 && i != path.size() - 1)
        lastup = new UPoint(*up);
    }
    edge_tuple->DeleteIfAllowed();
  }
  mp->EndBulkLoad();
}

/*
due to periodic property, use the edge with earliest start time in their route

*/

void BusNetwork::Optimize1(priority_queue<Elem>& q_list,
priority_queue<Elem>& temp_list)
{
  vector<Elem> elem_pop;
  while(temp_list.empty() == false){
    Elem top = temp_list.top();
    temp_list.pop();
    if(elem_pop.size() == 0){
      q_list.push(top);
      elem_pop.push_back(top);
    }else{
      unsigned int i = 0;
      for(;i < elem_pop.size();i++)
       if(elem_pop[i].rid == top.rid && elem_pop[i].e_node_id == top.e_node_id)
          break;
//      cout<<"i "<<i<<" elem_pop size "<<elem_pop.size()<<endl;
      if(i == elem_pop.size()){
        q_list.push(top);
        elem_pop.push_back(top);
      }
    }
  }

}
/*use some optimization technique*/
void BusNetwork::FindPath2(int id1,int id2,vector<int>& path,Instant& instant)
{
  ofstream outfile("temp_result"); //record info for debug

  if(id1 < 1 || id1 > bus_node->GetNoTuples()){
    cout<<"bus id is not valid"<<endl;
    return;
  }

  if(id2 < 1 || id2 > bus_node->GetNoTuples()){
    cout<<"bus id is not valid"<<endl;
    return;
  }
//  cout<<"start "<<id1<<" end "<<id2<<endl;
  //find all edges start from node id1, if time interval is given, filter
  //all edges start time earlier than it, no heuristic value

  //to control that one edge is not expanded more than once
  vector<bool> edge_flag;
  for(int i = 0; i < bus_edge->GetNoTuples() + 1;i++)
    edge_flag.push_back(true);

 //priority_queue<Elem,vector<Elem>,TimeCompare> q_list;
  priority_queue<Elem> q_list;
  vector<Elem> expansionlist;

  priority_queue<Elem> tmp_list; //Optimize--1
  int expansion_count = 0;
  //Initialize list
  CcInt* start_id = new CcInt(true,id1);
  BTreeIterator* bt_iter_edge_v1 = btree_bus_edge_v1->ExactMatch(start_id);

  while(bt_iter_edge_v1->Next()){
    Tuple* t = bus_edge->GetTuple(bt_iter_edge_v1->GetId());
    CcInt* start_node = (CcInt*)t->GetAttribute(V1);
    CcInt* end_node = (CcInt*)t->GetAttribute(V2);
    Periods* peri = (Periods*)t->GetAttribute(DEF_T);
    CcInt* rid = (CcInt*)t->GetAttribute(PID);
    const Interval<Instant>* interval;
    peri->Get(0,interval);

    if(path.size() != 0){ //middle stop
        TupleId edge_tid = path[path.size()-1];
        Tuple* edge_tuple = bus_edge->GetTuple(edge_tid);
        Periods* cur_def_t = (Periods*)edge_tuple->GetAttribute(DEF_T);
        const Interval<Instant>* interval_cur;
        cur_def_t->Get(0,interval_cur);

        if(interval->start > interval_cur->end){
            Elem e(bt_iter_edge_v1->GetId(),*interval,
                   start_node->GetIntval());
            e.delta_t = (interval->end-interval_cur->end).ToDouble();
            e.e_node_id = end_node->GetIntval();//end node id
            e.pre_edge_tid = 0;
            e.rid = rid->GetIntval();
            if(edge_flag[bt_iter_edge_v1->GetId()]){
              tmp_list.push(e);
              edge_flag[bt_iter_edge_v1->GetId()] = false;
            }

        }
        edge_tuple->DeleteIfAllowed();
    }else{
//      if(interval->start > ui1->timeInterval.start){
      if(interval->start > instant){
        Elem e(bt_iter_edge_v1->GetId(),*interval,
                  start_node->GetIntval());
//        e.delta_t = (interval->end-ui1->timeInterval.start).ToDouble();
        e.delta_t = (interval->end-instant).ToDouble();
        e.e_node_id = end_node->GetIntval();//end node id
        e. pre_edge_tid = 0;
        e.rid = rid->GetIntval();
        if(edge_flag[bt_iter_edge_v1->GetId()]){
          tmp_list.push(e);
          edge_flag[bt_iter_edge_v1->GetId()] = false;
        }
      }
    }
    t->DeleteIfAllowed();
  }

  delete bt_iter_edge_v1;
  delete start_id;
  Optimize1(q_list,tmp_list);

  cout<<"initialize size "<<q_list.size()<<endl;
//////////////////////////////////////////////////////////////////////////

  while(q_list.empty() == false){
    Elem top = q_list.top();
//    Tuple* edge_tuple = bus_edge->GetTuple(top.edge_tuple_id);
//    CcInt* start_node = (CcInt*)edge_tuple->GetAttribute(V1);
//    CcInt* end_node = (CcInt*)edge_tuple->GetAttribute(V2);
    CcInt* start_node = new CcInt(true,top.s_node_id);
    CcInt* end_node = new CcInt(true,top.e_node_id);

    if(end_node->GetIntval() == id2){//find the end
      delete start_node;
      delete end_node;
      break;
    }

    expansionlist.push_back(top);
    expansion_count++;
    q_list.pop();

//    Periods* cur_def_t = (Periods*)edge_tuple->GetAttribute(DEF_T);
//    const Interval<Instant>* interval_cur;
//    cur_def_t->Get(0,interval_cur);

    Interval<Instant>* interval_cur = &top.interval;

//    outfile<<"edge "<<edge_id->GetIntval()<<" pre-edge tid "<<top.pre_edge_tid
//        <<" v1 "<<start_node->GetIntval()
//        <<" v2 "<<end_node->GetIntval()<<" time "<<*interval_cur
//        <<" delta_t "<<top.delta_t<<" rid "<<top.rid<<endl;

    /////////////get all edges from the same start node////////////////
    bt_iter_edge_v1 = btree_bus_edge_v1->ExactMatch(end_node);

    priority_queue<Elem> temp_list; //Optimize--1

    while(bt_iter_edge_v1->Next()){
     Tuple* t = bus_edge->GetTuple(bt_iter_edge_v1->GetId());

     CcInt* start_node_next = (CcInt*)t->GetAttribute(V1);//start node
     CcInt* end_node_next = (CcInt*)t->GetAttribute(V2);//end node
     Periods* next_def_t = (Periods*)t->GetAttribute(DEF_T);

     CcInt* rid = (CcInt*)t->GetAttribute(PID);
     const Interval<Instant>* interval_next;
     next_def_t->Get(0,interval_next);
   //time instant of next edge should be late than cur edge
   //end node id of next edge should not be equal to start node id of cur edge
//     cout<<"next "<<*interval_next<<endl;
//     cout<<*trip<<endl;

      if(interval_next->start > interval_cur->end &&
           end_node_next->GetIntval() != start_node->GetIntval()){
            //store all edges from the same start node
            Elem e(bt_iter_edge_v1->GetId(),*interval_next,
                  start_node_next->GetIntval());
            e.pre_eid = expansion_count - 1;
            e.delta_t = top.delta_t +
                          (interval_next->end-interval_cur->end).ToDouble();
            e.rid = rid->GetIntval();
            e.e_node_id = end_node_next->GetIntval();
            e.pre_edge_tid = top.edge_tuple_id;
//            cout<<e.delta_t<<endl;
//////////////////    Optimize - 1    ////////////////////////////////////////
            if(edge_flag[bt_iter_edge_v1->GetId()]){
              temp_list.push(e);
              edge_flag[bt_iter_edge_v1->GetId()] = false;
            }
            //q_list.push(e);
//////////////////////////////////////////////////////////////////////////////

        }
        t->DeleteIfAllowed();
      }
      delete bt_iter_edge_v1;

      Optimize1(q_list,temp_list);

    //////////////////////////////////////////////////////////////////////////
      delete start_node;
      delete end_node;
  }


  if(q_list.empty() == false){
    stack<Elem> temp_path;
    Elem top = q_list.top();
    temp_path.push(top);
//    cout<<top.edge_tuple_id<<endl;
    while(top.pre_eid != -1){
      int id = top.pre_eid;
//      cout<<"pre id "<<id<<endl;
      top = expansionlist[id];
//      cout<<top.edge_tuple_id<<endl;
      temp_path.push(top);
    }
//    cout<<temp_path.size()<<endl;
    while(temp_path.empty() == false){
      Elem top = temp_path.top();
      temp_path.pop();
      path.push_back(top.edge_tuple_id);
    }
  }
}

/*
expand the graph by Dijkstra with minimum total time cost so far
with some optimization techniques

*/
void BusNetwork::FindPath_T_2(MPoint* mp,Relation* query,int attrpos,
Instant& instant)
{
//  cout<<"BusNetwork::Reachability"<<endl;
  if(query->GetNoTuples() < 2){
    cout<<"there is only start location, please give destination"<<endl;
    return;
  }

//  MPoint* mp = new MPoint(0);
  mp->Clear();
  mp->StartBulkLoad();

  vector<int> path; //record edge id
  for(int i = 1;i <= query->GetNoTuples() - 1;i++){
    Tuple* t1 = query->GetTuple(i);
    Tuple* t2 = query->GetTuple(i+1);
    CcInt* id1 = (CcInt*)t1->GetAttribute(attrpos);
    CcInt* id2 = (CcInt*)t2->GetAttribute(attrpos);

    FindPath2(id1->GetIntval(),id2->GetIntval(),path,instant);
    t1->DeleteIfAllowed();
    t2->DeleteIfAllowed();
  }
  /****************Construct the Result (MPoint)*************************/
  const UPoint* lastup = NULL;
  for(unsigned int i = 0;i < path.size();i++){
//    cout<<path[i]<<" ";
    Tuple* edge_tuple = bus_edge->GetTuple(path[i]);
    MPoint* temp_mp = (MPoint*)edge_tuple->GetAttribute(MOVE);
//    cout<<*temp_mp<<endl;
    for(int j = 0;j < temp_mp->GetNoComponents();j++){
      const UPoint* up;
      temp_mp->Get(j,up);
      //not the first trip
      if(lastup != NULL && i != 0 && j == 0){
        UPoint* insert_up = new UPoint(true);
//        cout<<"last "<<*lastup<<endl;
//        cout<<"cur "<<*up<<endl;
        insert_up->p0 = lastup->p1;
        insert_up->timeInterval.start = lastup->timeInterval.end;
        insert_up->p1 = up->p0;
        insert_up->timeInterval.end = up->timeInterval.start;
//        cout<<"insert "<<*insert_up<<endl;
        insert_up->timeInterval.lc = true;
        insert_up->timeInterval.rc = false;
        mp->Add(*insert_up);
        delete insert_up;
        delete lastup;
        lastup = NULL;
      }
      mp->Add(*up);
      if(j == temp_mp->GetNoComponents() - 1 && i != path.size() - 1)
        lastup = new UPoint(*up);
    }
    edge_tuple->DeleteIfAllowed();
  }
  mp->EndBulkLoad();
}

/*
use some optimization technique, temporal property with middle stop
optimize-1
input relation and b-tree

*/
bool BusNetwork::FindPath3(int id1,int id2,vector<int>& path,
Relation* busedge,BTree* btree1,Instant& queryinstant,double wait_time)
{

//  struct timeb t1;
//  struct timeb t2;

  ofstream outfile("temp_result"); //record info for debug

  if(id1 < 1 || id1 > bus_node->GetNoTuples()){
    cout<<"bus id is not valid"<<endl;
    return false;
  }

  if(id2 < 1 || id2 > bus_node->GetNoTuples()){
    cout<<"bus id is not valid"<<endl;
    return false;
  }
  if(id1 == id2){
    cout<<"two locations are the same"<<endl;
    return false;
  }
  cout<<"start "<<id1<<" end "<<id2<<endl;
  //find all edges start from node id1, if time interval is given, filter
  //all edges start time earlier than it, no heuristic value

  //to control that one edge is not expanded more than once
  vector<bool> edge_flag;
  for(int i = 0; i < busedge->GetNoTuples() + 1;i++)
    edge_flag.push_back(true);

 //priority_queue<Elem,vector<Elem>,TimeCompare> q_list;
  priority_queue<Elem> q_list;
  vector<Elem> expansionlist;

  priority_queue<Elem> tmp_list; //Optimize--1
  int expansion_count = 0;
  //Initialize list
  CcInt* start_id = new CcInt(true,id1);
  BTreeIterator* bt_iter_edge_v1 = btree1->ExactMatch(start_id);

  while(bt_iter_edge_v1->Next()){
    Tuple* t = busedge->GetTuple(bt_iter_edge_v1->GetId());
    CcInt* start_node = (CcInt*)t->GetAttribute(V1);
    CcInt* end_node = (CcInt*)t->GetAttribute(V2);
    Periods* peri = (Periods*)t->GetAttribute(DEF_T);
    CcInt* rid = (CcInt*)t->GetAttribute(PID);
    const Interval<Instant>* interval;
    peri->Get(0,interval);
    if(path.size() != 0){ //middle stop
        TupleId edge_tid = path[path.size()-1];
        Tuple* edge_tuple = busedge->GetTuple(edge_tid);
        Periods* cur_def_t = (Periods*)edge_tuple->GetAttribute(DEF_T);
        const Interval<Instant>* interval_cur;
        cur_def_t->Get(0,interval_cur);

        //if user defined time instant, one more compare condition should be
        //considered
        //if ui1->lc == true
        Instant depart_time(instanttype);
        depart_time.ReadFrom(wait_time+interval_cur->end.ToDouble());

        //optimize-3, filter edge by start time
        if(interval->start < depart_time){
           edge_tuple->DeleteIfAllowed();
           t->DeleteIfAllowed();
           break;
        }

        if(interval->start > depart_time){
            Elem e(bt_iter_edge_v1->GetId(),*interval,
                   start_node->GetIntval());
            e.delta_t = (interval->end-interval_cur->end).ToDouble();
            e.e_node_id = end_node->GetIntval();//end node id
            e.pre_edge_tid = 0;
            e.rid = rid->GetIntval();
            if(edge_flag[bt_iter_edge_v1->GetId()]){
              tmp_list.push(e);
              edge_flag[bt_iter_edge_v1->GetId()] = false;
            }
        }
        edge_tuple->DeleteIfAllowed();
    }else{
        if(interval->start < queryinstant){
           t->DeleteIfAllowed();
           break;
        }

      if(interval->start > queryinstant){
        Elem e(bt_iter_edge_v1->GetId(),*interval,
                  start_node->GetIntval());

        e.delta_t = (interval->end-queryinstant).ToDouble();
        e.e_node_id = end_node->GetIntval();//end node id
        e. pre_edge_tid = 0;
        e.rid = rid->GetIntval();
        if(edge_flag[bt_iter_edge_v1->GetId()]){
          tmp_list.push(e);
          edge_flag[bt_iter_edge_v1->GetId()] = false;
        }
      }
    }
    t->DeleteIfAllowed();
  }

  delete bt_iter_edge_v1;
  delete start_id;

  Optimize1(q_list,tmp_list);

//  cout<<"initialize size "<<q_list.size()<<endl;
  if(q_list.empty() == true)
    return false;

//////////////////////////////////////////////////////////////////////////

  while(q_list.empty() == false){
    Elem top = q_list.top();

//    Tuple* edge_tuple = busedge->GetTuple(top.edge_tuple_id);
//    CcInt* start_node = (CcInt*)edge_tuple->GetAttribute(V1);
//    CcInt* end_node = (CcInt*)edge_tuple->GetAttribute(V2);

    CcInt* start_node = new CcInt(true,top.s_node_id);
    CcInt* end_node = new CcInt(true,top.e_node_id);

    if(end_node->GetIntval() == id2){//find the end
      delete start_node;
      delete end_node;
      break;
    }

    expansionlist.push_back(top);
    expansion_count++;
    q_list.pop();

//    Periods* cur_def_t = (Periods*)edge_tuple->GetAttribute(DEF_T);
//    const Interval<Instant>* interval_cur;
//    cur_def_t->Get(0,interval_cur);

     Interval<Instant>* interval_cur = &top.interval;

//    outfile<<"edge "<<edge_id->GetIntval()<<" pre-edge tid "<<top.pre_edge_tid
//        <<" v1 "<<start_node->GetIntval()
//        <<" v2 "<<end_node->GetIntval()<<" time "<<*interval_cur
//        <<" delta_t "<<top.delta_t<<" rid "<<top.rid<<endl;

    /////////////get all edges from the same start node////////////////
//    clock_t start_cpu = clock();
//    ftime(&t1);
    bt_iter_edge_v1 = btree1->ExactMatch(end_node);
    priority_queue<Elem> temp_list; //Optimize--1

    while(bt_iter_edge_v1->Next()){
     Tuple* t = busedge->GetTuple(bt_iter_edge_v1->GetId());

     CcInt* start_node_next = (CcInt*)t->GetAttribute(V1);//start node
     CcInt* end_node_next = (CcInt*)t->GetAttribute(V2);//end node
     Periods* next_def_t = (Periods*)t->GetAttribute(DEF_T);

     CcInt* rid = (CcInt*)t->GetAttribute(PID);
     const Interval<Instant>* interval_next;
     next_def_t->Get(0,interval_next);
   //time instant of next edge should be late than cur edge
   //end node id of next edge should not be equal to start node id of cur edge
//     cout<<"next "<<*interval_next<<endl;
//     cout<<*trip<<endl;
    //optimize-3
    if(interval_next->start < interval_cur->end){
        t->DeleteIfAllowed();
        break;
    }

      if(interval_next->start > interval_cur->end &&
           end_node_next->GetIntval() != start_node->GetIntval()){
            //store all edges from the same start node
            Elem e(bt_iter_edge_v1->GetId(),*interval_next,
                  start_node_next->GetIntval());
            e.pre_eid = expansion_count - 1;
            e.delta_t = top.delta_t +
                          (interval_next->end-interval_cur->end).ToDouble();
            e.rid = rid->GetIntval();
            e.e_node_id = end_node_next->GetIntval();
            e.pre_edge_tid = top.edge_tuple_id;
//            cout<<e.delta_t<<endl;
//////////////////    Optimize - 1    ////////////////////////////////////////
            if(edge_flag[bt_iter_edge_v1->GetId()]){
              temp_list.push(e);
              edge_flag[bt_iter_edge_v1->GetId()] = false;
            }
            //q_list.push(e);
//////////////////////////////////////////////////////////////////////////////

        }
        t->DeleteIfAllowed();
      }
      delete bt_iter_edge_v1;

//      ftime(&t2);
//      clock_t stop_cpu = clock();
//      cout<<"big searching 2 total "<<difftimeb(&t2,&t1)<<" ";
//      cout<<"big searching 2 CPU "<<
//                        ((double)(stop_cpu-start_cpu))/CLOCKS_PER_SEC<<endl;
      Optimize1(q_list,temp_list);

    //////////////////////////////////////////////////////////////////////////
    delete start_node;
    delete end_node;
  }

  if(q_list.empty() == false){
    stack<Elem> temp_path;
    Elem top = q_list.top();
    temp_path.push(top);
//    cout<<top.edge_tuple_id<<endl;
    while(top.pre_eid != -1){
      int id = top.pre_eid;
//      cout<<"pre id "<<id<<endl;
      top = expansionlist[id];
//      cout<<top.edge_tuple_id<<endl;
      temp_path.push(top);
    }
//    cout<<temp_path.size()<<endl;
    while(temp_path.empty() == false){
      Elem top = temp_path.top();
      temp_path.pop();
      path.push_back(top.edge_tuple_id);
    }
  }

  return true;
}

/*
expand the graph by Dijkstra with minimum total time cost so far
with some optimization techniques, supporting time duration for middle stop
optimize-1
input relation and b-tree

*/
void BusNetwork::FindPath_T_3(MPoint* mp,Relation* query,Relation* busedge,
BTree* btree1,int attrpos1,int attrpos2,Instant& instant)
{
  if(query->GetNoTuples() < 2){
    cout<<"there is only start location, please give destination"<<endl;
    return;
  }

  mp->Clear();
  mp->StartBulkLoad();

  vector<int> path; //record edge id

  //searching process

  for(int i = 1;i <= query->GetNoTuples() - 1;i++){
    Tuple* t1 = query->GetTuple(i);
    Tuple* t2 = query->GetTuple(i+1);
    CcInt* id1 = (CcInt*)t1->GetAttribute(attrpos1);
    CcInt* id2 = (CcInt*)t2->GetAttribute(attrpos1);
    DateTime* timestay = (DateTime*)t1->GetAttribute(attrpos2);
//    cout<<id1->GetIntval()<<" "<<id2->GetIntval()<<endl;
//    cout<<*timestay<<endl;
    double waittime = timestay->ToDouble();

   if(FindPath3(id1->GetIntval(),id2->GetIntval(),
              path,busedge,btree1,instant,waittime)==false){
        cout<<"such a route is not valid"<<endl;
        path.clear();
        break;
    }
    t1->DeleteIfAllowed();
    t2->DeleteIfAllowed();
  }
  /****************Construct the Reulst (MPoint)*************************/
  const UPoint* lastup = NULL;
  for(unsigned int i = 0;i < path.size();i++){
//    cout<<path[i]<<" ";
    Tuple* edge_tuple = busedge->GetTuple(path[i]);
    MPoint* temp_mp = (MPoint*)edge_tuple->GetAttribute(MOVE);
//    cout<<*temp_mp<<endl;
    for(int j = 0;j < temp_mp->GetNoComponents();j++){
      const UPoint* up;
      temp_mp->Get(j,up);
      //not the first trip
      if(lastup != NULL && i != 0 && j == 0){
        UPoint* insert_up = new UPoint(true);
//        cout<<"last "<<*lastup<<endl;
//        cout<<"cur "<<*up<<endl;
        insert_up->p0 = lastup->p1;
        insert_up->timeInterval.start = lastup->timeInterval.end;
        insert_up->p1 = up->p0;
        insert_up->timeInterval.end = up->timeInterval.start;
//        cout<<"insert "<<*insert_up<<endl;
        insert_up->timeInterval.lc = true;
        insert_up->timeInterval.rc = false;
        mp->Add(*insert_up);
        delete insert_up;
        delete lastup;
        lastup = NULL;
      }
      mp->Add(*up);
      if(j == temp_mp->GetNoComponents() - 1 && i != path.size() - 1)
        lastup = new UPoint(*up);
    }
    edge_tuple->DeleteIfAllowed();
  }
  mp->EndBulkLoad();
}

/*
use some optimization technique, temporal property with middle stop
optimize-1

optimize-3 filter edge by their start time
edge relation and a b-tree

*/
bool BusNetwork::FindPath4(int id1,int id2,vector<Elem>& path,
Instant& queryinstant,double& wait_time)
{
//  struct timeb t1;
//  struct timeb t2;

  ofstream outfile("temp_result"); //record info for debug

  if(id1 < 1 || id1 > bus_node->GetNoTuples()){
    cout<<"bus id is not valid"<<endl;
    return false;
  }

  if(id2 < 1 || id2 > bus_node->GetNoTuples()){
    cout<<"bus id is not valid"<<endl;
    return false;
  }
  if(id1 == id2){
    cout<<"two locations are the same"<<endl;
    return false;
  }
  cout<<"start "<<id1<<" end "<<id2<<endl;
  //find all edges start from node id1, if time interval is given, filter
  //all edges start time earlier than it, no heuristic value


//get the end point
  /*Point end_point;
  CcInt* end_node_id = new CcInt(true,id2);
  BTreeIterator* bt_iter_edge = btree_bus_edge_v2->ExactMatch(end_node_id);
  while(bt_iter_edge->Next()){
    Tuple* edge_tuple = bus_edge->GetTuple(bt_iter_edge->GetId());
    Point* end_p = (Point*)edge_tuple->GetAttribute(P2);
    end_point = *end_p;
    edge_tuple->DeleteIfAllowed();
    break;
  }
  delete end_node_id;
  delete bt_iter_edge;*/


  //to control that one edge is not expanded more than once
  vector<bool> edge_flag;
//  for(int i = 0; i < busedge->GetNoTuples() + 1;i++)
  for(int i = 0; i < bus_edge->GetNoTuples() + 1;i++)
    edge_flag.push_back(true);

 //priority_queue<Elem,vector<Elem>,TimeCompare> q_list;
  priority_queue<Elem> q_list;
  vector<Elem> expansionlist;

  priority_queue<Elem> tmp_list; //Optimize--1
  int expansion_count = 0;
  //Initialize list
  CcInt* start_id = new CcInt(true,id1);
//  BTreeIterator* bt_iter_edge_v1 = btree1->ExactMatch(start_id);

  BTreeIterator* bt_iter_edge_v1 = btree_bus_edge_v1->ExactMatch(start_id);

  while(bt_iter_edge_v1->Next()){
//    Tuple* t = busedge->GetTuple(bt_iter_edge_v1->GetId());
    Tuple* t = bus_edge->GetTuple(bt_iter_edge_v1->GetId());
    CcInt* start_node = (CcInt*)t->GetAttribute(V1);
    CcInt* end_node = (CcInt*)t->GetAttribute(V2);
    Periods* peri = (Periods*)t->GetAttribute(DEF_T);
    CcInt* rpid = (CcInt*)t->GetAttribute(RPID);
    CcInt* rid = (CcInt*)t->GetAttribute(PID);
    Point* endp = (Point*)t->GetAttribute(P2);

    const Interval<Instant>* interval;
    peri->Get(0,interval);
    if(path.size() != 0){ //middle stop
//        TupleId edge_tid = path[path.size()-1];
        TupleId edge_tid = path[path.size()-1].edge_tuple_id;
//        Tuple* edge_tuple = busedge->GetTuple(edge_tid);
        Tuple* edge_tuple = bus_edge->GetTuple(edge_tid);
        Periods* cur_def_t = (Periods*)edge_tuple->GetAttribute(DEF_T);
        const Interval<Instant>* interval_cur;
        cur_def_t->Get(0,interval_cur);

        //if user defined time instant, one more compare condition should be
        //considered
        //if ui1->lc == true
        Instant depart_time(instanttype);
        depart_time.ReadFrom(wait_time+interval_cur->end.ToDouble());

        //optimize-3, filter edge by start time
        if(interval->start < depart_time){
           edge_tuple->DeleteIfAllowed();
           t->DeleteIfAllowed();
           break;
        }

        if(interval->start > depart_time){
            Elem e(bt_iter_edge_v1->GetId(),*interval,
                   start_node->GetIntval());
            e.delta_t = (interval->end-interval_cur->end).ToDouble();
/////////////////////////
//            e.delta_t_pre = e.delta_t;
//            e.delta_t += endp->Distance(end_point)/maxspeed;
/////////////////
            e.e_node_id = end_node->GetIntval();//end node id
            e.pre_edge_tid = 0;
            e.rpid = rpid->GetIntval();
            e.rid = rid->GetIntval();
            if(edge_flag[bt_iter_edge_v1->GetId()]){
              tmp_list.push(e);
              edge_flag[bt_iter_edge_v1->GetId()] = false;
            }

        }
        edge_tuple->DeleteIfAllowed();
    }else{
        //optimize-3, filter edge by start time

        if(interval->start < queryinstant){
           t->DeleteIfAllowed();
           break;
        }

      if(interval->start > queryinstant){
        Elem e(bt_iter_edge_v1->GetId(),*interval,
                  start_node->GetIntval());

        e.delta_t = (interval->end-queryinstant).ToDouble();
///////////
//        e.delta_t_pre = e.delta_t;
//        e.delta_t += endp->Distance(end_point)/maxspeed;
/////////
        e.e_node_id = end_node->GetIntval();//end node id
        e.pre_edge_tid = 0;
        e.rpid = rpid->GetIntval();
        e.rid = rid->GetIntval();
        if(edge_flag[bt_iter_edge_v1->GetId()]){
          tmp_list.push(e);
          edge_flag[bt_iter_edge_v1->GetId()] = false;

        }
      }
    }
    t->DeleteIfAllowed();
  }

  delete bt_iter_edge_v1;
  delete start_id;

  Optimize1(q_list,tmp_list);


  if(q_list.empty() == true)
    return false;

//////////////////////////////////////////////////////////////////////////

  while(q_list.empty() == false){

    Elem top = q_list.top();
//    printf("top delta_t %.14f\n",top.delta_t);

    CcInt* start_node = new CcInt(true,top.s_node_id);
    CcInt* end_node = new CcInt(true,top.e_node_id);

    if(end_node->GetIntval() == id2){//find the end
      delete start_node;
      delete end_node;
      break;
    }

    expansionlist.push_back(top);
    expansion_count++;
    q_list.pop();

//    Periods* cur_def_t = (Periods*)edge_tuple->GetAttribute(DEF_T);
//    const Interval<Instant>* interval_cur;
//    cur_def_t->Get(0,interval_cur);
     Interval<Instant>* interval_cur = &top.interval;

//    outfile<<"edge "<<edge_id->GetIntval()<<" pre-edge tid "<<top.pre_edge_tid
//        <<" v1 "<<start_node->GetIntval()
//        <<" v2 "<<end_node->GetIntval()<<" time "<<*interval_cur
//        <<" delta_t "<<top.delta_t<<" rid "<<top.rid<<endl;

    /////////////get all edges from the same start node////////////////
//    clock_t start_cpu = clock();
//    ftime(&t1);
//    bt_iter_edge_v1 = btree1->ExactMatch(end_node);

    const ListEntry* listentry;
    adjacencylist_index.Get(top.edge_tuple_id-1,listentry);
    int start = listentry->low;
    int high = listentry->high;
    priority_queue<Elem> temp_list; //Optimize--1

    for(int i = start; i < high;i++){

//    bt_iter_edge_v1 = btree_bus_edge_v1->ExactMatch(end_node);

//    while(bt_iter_edge_v1->Next()){
      const int* tuple_id;
      adjacencylist.Get(i,tuple_id);

//      Tuple* t = bus_edge->GetTuple(bt_iter_edge_v1->GetId());
      Tuple* t = bus_edge->GetTuple(*tuple_id);

      CcInt* start_node_next = (CcInt*)t->GetAttribute(V1);//start node
      CcInt* end_node_next = (CcInt*)t->GetAttribute(V2);//end node
      Periods* next_def_t = (Periods*)t->GetAttribute(DEF_T);
      CcInt* rpid = (CcInt*)t->GetAttribute(RPID);
      CcInt* rid = (CcInt*)t->GetAttribute(PID);
      Point* endp = (Point*)t->GetAttribute(P2);

      const Interval<Instant>* interval_next;
      next_def_t->Get(0,interval_next);
     //optimize-3  filter edge by their start time
      if(interval_next->start < interval_cur->end){
        t->DeleteIfAllowed();
        break;
      }

      if(interval_next->start > interval_cur->end &&
           end_node_next->GetIntval() != start_node->GetIntval()){
            //store all edges from the same start node
//            Elem e(bt_iter_edge_v1->GetId(),*interval_next,
//                  start_node_next->GetIntval());
            Elem e(*tuple_id,*interval_next,start_node_next->GetIntval());

            e.pre_eid = expansion_count - 1;
            e.delta_t = top.delta_t +
                          (interval_next->end-interval_cur->end).ToDouble();

////////////////////////////////
//            e.delta_t = top.delta_t_pre +
//                         (interval_next->end-interval_cur->end).ToDouble();
//            e.delta_t_pre = e.delta_t;
//            e.delta_t += endp->Distance(end_point)/maxspeed;
////////////////////////////////////

            e.rid = rid->GetIntval();
            e.e_node_id = end_node_next->GetIntval();
            e.pre_edge_tid = top.edge_tuple_id;
            e.rpid = rpid->GetIntval();

//            if(edge_flag[bt_iter_edge_v1->GetId()]){
//              temp_list.push(e);
//              edge_flag[bt_iter_edge_v1->GetId()] = false;
//            }
            if(edge_flag[*tuple_id]){
              temp_list.push(e);
              edge_flag[*tuple_id] = false;
            }

        }
        t->DeleteIfAllowed();
      }

//      delete bt_iter_edge_v1;

//      ftime(&t2);
//      clock_t stop_cpu = clock();
//      cout<<"big searching 2 total "<<difftimeb(&t2,&t1)<<" ";
//      cout<<"big searching 2 CPU "<<
//                        ((double)(stop_cpu-start_cpu))/CLOCKS_PER_SEC<<endl;
      Optimize1(q_list,temp_list);

    //////////////////////////////////////////////////////////////////////////
    delete start_node;
    delete end_node;
//    cout<<"find_path_t_4 q_list size "<<q_list.size()<<endl;
  }

  if(q_list.empty() == false){
    stack<Elem> temp_path;
    Elem top = q_list.top();
    temp_path.push(top);
    while(top.pre_eid != -1){
      int id = top.pre_eid;
      top = expansionlist[id];
      temp_path.push(top);
    }
    while(temp_path.empty() == false){
      Elem top = temp_path.top();
      temp_path.pop();
//      path.push_back(top.edge_tuple_id);
      path.push_back(top);
    }
  }

  return true;
}

/*
expand the graph by Dijkstra with minimum total time cost so far
with some optimization techniques, supporting time duration for middle stop
optimize-1 filter the edge from the same route but comes later

optimize-3 filter edge by their start time
input edge relation and b-tree

*/
void BusNetwork::TestFunction(Relation* busedge, BTree* btree1)
{
  CcInt* id = new CcInt(true,1);
  BTreeIterator* bt_iter_edge_v1 = btree1->ExactMatch(id);
  while(bt_iter_edge_v1->Next()){
     Tuple* t = busedge->GetTuple(bt_iter_edge_v1->GetId());
     CcInt* eid = (CcInt*)t->GetAttribute(EID);//start node
     cout<<"eid "<<eid->GetIntval()<<endl;
     t->DeleteIfAllowed();
  }
  delete id;
  delete bt_iter_edge_v1;
}

void BusNetwork::FindPath_T_4(MPoint* mp,Relation* query,int attrpos1,
int attrpos2,Instant& queryinstant)
{
  if(query->GetNoTuples() < 2){
    cout<<"there is only start location, please give destination"<<endl;
    return;
  }

  mp->Clear();
  mp->StartBulkLoad();

  vector<Elem> path; //record edge id

  //searching process

//  TestFunction(busedge,btree1);
  for(int i = 1;i <= query->GetNoTuples() - 1;i++){
    Tuple* t1 = query->GetTuple(i);
    Tuple* t2 = query->GetTuple(i+1);
    CcInt* id1 = (CcInt*)t1->GetAttribute(attrpos1);
    CcInt* id2 = (CcInt*)t2->GetAttribute(attrpos1);

    DateTime* timestay = (DateTime*)t1->GetAttribute(attrpos2);
//    cout<<id1->GetIntval()<<" "<<id2->GetIntval()<<endl;
//    cout<<*timestay<<endl;
    double waittime = timestay->ToDouble();

    if(FindPath4(id1->GetIntval(),id2->GetIntval(),path,
           queryinstant,waittime)==false){
        cout<<"such a route is not valid"<<endl;
        path.clear();
        break;
    }
    t1->DeleteIfAllowed();
    t2->DeleteIfAllowed();
  }
  /****************Construct the Reulst (MPoint)*************************/
  const UPoint* lastup = NULL;
  for(unsigned int i = 0;i < path.size();i++){
//    cout<<path[i]<<" ";
//    Tuple* edge_tuple = bus_edge->GetTuple(path[i]);
    Tuple* edge_tuple = bus_edge->GetTuple(path[i].edge_tuple_id);
    MPoint* temp_mp = (MPoint*)edge_tuple->GetAttribute(MOVE);
//    cout<<*temp_mp<<endl;
    for(int j = 0;j < temp_mp->GetNoComponents();j++){
      const UPoint* up;
      temp_mp->Get(j,up);
      //not the first trip
      if(lastup != NULL && i != 0 && j == 0){
        UPoint* insert_up = new UPoint(true);
//        cout<<"last "<<*lastup<<endl;
//        cout<<"cur "<<*up<<endl;
        insert_up->p0 = lastup->p1;
        insert_up->timeInterval.start = lastup->timeInterval.end;
        insert_up->p1 = up->p0;
        insert_up->timeInterval.end = up->timeInterval.start;
//        cout<<"insert "<<*insert_up<<endl;
        insert_up->timeInterval.lc = true;
        insert_up->timeInterval.rc = false;
        mp->Add(*insert_up);
        delete insert_up;
        delete lastup;
        lastup = NULL;
      }
      mp->Add(*up);
      if(j == temp_mp->GetNoComponents() - 1 && i != path.size() - 1)
        lastup = new UPoint(*up);
    }
    edge_tuple->DeleteIfAllowed();
  }
  mp->EndBulkLoad();
}

/*
due to periodic property, use the edge with earliest start time in their route
and use the pre-defined path
optimize-2

*/

void BusNetwork::Optimize2(priority_queue<Elem>& q_list,
priority_queue<Elem>& temp_list,list<Elem>& end_node_edge,double& prune_time)
{
//  cout<<"prune_time "<<prune_time<<endl;
  vector<Elem> elem_pop;
  while(temp_list.empty() == false){
    Elem top = temp_list.top();
    temp_list.pop();

    if(top.delta_t > prune_time)
      break;

//    cout<<"top rpid "<<top.rpid<<endl;
//    top.Print2();
    if(elem_pop.size() == 0){
      if(top.delta_t <= prune_time){
//        cout<<"here 1"<<endl;
        q_list.push(top);
        elem_pop.push_back(top);

        //modify end_node_edge by time, reduce the size of queue
/*        while(top.interval.start > end_node_edge.front().interval.start)
          end_node_edge.pop_front();
        list<Elem>::iterator start = end_node_edge.begin();
        for(;start != end_node_edge.end();start++){
            if(start->rpid == top.rpid){
              double t = top.delta_t +
                (start->interval.end - top.interval.end).ToDouble();
              if(t < prune_time)
                prune_time = t;
              break;
            }
        }*/

      }
    }else{
      unsigned int i = 0;
      for(;i < elem_pop.size();i++)
       if(elem_pop[i].rid == top.rid && elem_pop[i].e_node_id == top.e_node_id)
          break;
//      cout<<"i "<<i<<" elem_pop size "<<elem_pop.size()<<endl;
      if(i == elem_pop.size()){
        if(top.delta_t <= prune_time){
//          cout<<"here 2"<<endl;
          q_list.push(top);
          elem_pop.push_back(top);

          //modify end_node_edge by time, reduce the size of queue
/*        while(top.interval.start > end_node_edge.front().interval.start)
            end_node_edge.pop_front();
            list<Elem>::iterator start = end_node_edge.begin();
            for(;start != end_node_edge.end();start++){
              if(start->rpid == top.rpid){
                double t = top.delta_t +
                  (start->interval.end - top.interval.end).ToDouble();
                if(t < prune_time)
                  prune_time = t;
                break;
              }
            }*/

          }
      }
    }
  }
//  cout<<"temp_list size 2 "<<temp_list.size()<<endl;
}

/*
use some optimization technique, temporal property with middle stop
optimize-1
optimize-2
optimize-3 filter edge by their start time
optimize-4 use A star algorithm, distance / maxspeed
edge relation and a b-tree

*/
bool BusNetwork::FindPath5(int id1,int id2,vector<Elem>& path,
Instant& queryinstant,double& wait_time)
{
//  struct timeb t1;
//  struct timeb t2;

  ofstream outfile("temp_result"); //record info for debug

  if(id1 < 1 || id1 > bus_node->GetNoTuples()){
    cout<<"bus id is not valid"<<endl;
    return false;
  }

  if(id2 < 1 || id2 > bus_node->GetNoTuples()){
    cout<<"bus id is not valid"<<endl;
    return false;
  }
  if(id1 == id2){
    cout<<"two locations are the same"<<endl;
    return false;
  }
//  cout<<"start "<<id1<<" end "<<id2<<endl;

//get the end point
  Point end_point;
  CcInt* end_node_id = new CcInt(true,id2);
//  BTreeIterator* bt_iter_edge = btree2->ExactMatch(end_node_id);
  BTreeIterator* bt_iter_edge = btree_bus_edge_v2->ExactMatch(end_node_id);
  while(bt_iter_edge->Next()){
//    Tuple* edge_tuple = busedge->GetTuple(bt_iter_edge->GetId());
    Tuple* edge_tuple = bus_edge->GetTuple(bt_iter_edge->GetId());
    Point* end_p = (Point*)edge_tuple->GetAttribute(P2);
    end_point = *end_p;
    edge_tuple->DeleteIfAllowed();
    break;
  }
//  cout<<"end point "<<end_point<<endl;
  delete end_node_id;
  delete bt_iter_edge;

  //find all edges start from node id1, if time interval is given, filter
  //all edges start time earlier than it

  //to control that one edge is not expanded more than once
  vector<bool> edge_flag;
//  for(int i = 0; i < busedge->GetNoTuples() + 1;i++)
  for(int i = 0; i < bus_edge->GetNoTuples() + 1;i++)
    edge_flag.push_back(true);

 //priority_queue<Elem,vector<Elem>,TimeCompare> q_list;
  priority_queue<Elem> q_list;
  vector<Elem> expansionlist;

  priority_queue<Elem> tmp_list; //Optimize--1
  int expansion_count = 0;
  vector<Elem> elemlist; //Optimize-1 faster
  //Initialize list
  CcInt* start_id = new CcInt(true,id1);
//  BTreeIterator* bt_iter_edge_v1 = btree1->ExactMatch(start_id);
  BTreeIterator* bt_iter_edge_v1 = btree_bus_edge_v1->ExactMatch(start_id);

  while(bt_iter_edge_v1->Next()){
//    Tuple* t = busedge->GetTuple(bt_iter_edge_v1->GetId());
    Tuple* t = bus_edge->GetTuple(bt_iter_edge_v1->GetId());
    CcInt* start_node = (CcInt*)t->GetAttribute(V1);
    CcInt* end_node = (CcInt*)t->GetAttribute(V2);
    Periods* peri = (Periods*)t->GetAttribute(DEF_T);
    CcInt* rpid = (CcInt*)t->GetAttribute(RPID);
    CcInt* rid = (CcInt*)t->GetAttribute(PID);
    Point* endp = (Point*)t->GetAttribute(P2);
    const Interval<Instant>* interval;
    peri->Get(0,interval);
//    cout<<"etid "<<t->GetTupleId()<<" v1 "<<start_node->GetIntval()
//        <<" v2 "<<end_node->GetIntval()<<" rid "<<rid->GetIntval()<<endl;

    if(path.size() != 0){ //middle stop
        const Interval<Instant>* interval_cur = &path[path.size()-1].interval;

        //if user defined time instant, one more compare condition should be
        //considered
        //if ui1->lc == true
        Instant depart_time(instanttype);
        depart_time.ReadFrom(wait_time+interval_cur->end.ToDouble());

        //optimize-3, filter edge by start time
        if(interval->start < depart_time){

           t->DeleteIfAllowed();
           break;
        }

        if(interval->start > depart_time){
            Elem e(bt_iter_edge_v1->GetId(),*interval,
                   start_node->GetIntval());
            e.delta_t = (interval->end-interval_cur->end).ToDouble();
            e.delta_t_pre = e.delta_t; //real cost time

            e.delta_t += endp->Distance(end_point)/maxspeed;

            e.e_node_id = end_node->GetIntval();//end node id
            e.pre_edge_tid = 0;
            e.rpid = rpid->GetIntval();
            e.rid = rid->GetIntval();
            if(edge_flag[bt_iter_edge_v1->GetId()]){
//              tmp_list.push(e);
              edge_flag[bt_iter_edge_v1->GetId()] = false;

        //////////////////////////////////////////////////
              if(elemlist.empty())
                elemlist.push_back(e);
              else{
                unsigned int i = 0;
                for(;i < elemlist.size();i++){
                  if(elemlist[i].rid == e.rid &&
                        elemlist[i].e_node_id == e.e_node_id &&
                        elemlist[i].interval.start > e.interval.start){
                    elemlist[i] = e;
                    break;
                  }
                }
                if(i == elemlist.size())
                  elemlist.push_back(e);
              }
        ///////////////////////////////////////////////////////////
            }
        }

    }else{
        //optimize-3, filter edge by start time
//        if(interval->start < ui1->timeInterval.start){
        if(interval->start < queryinstant){
           t->DeleteIfAllowed();
           break;
        }

//      if(interval->start > ui1->timeInterval.start){
        if(interval->start > queryinstant){
        Elem e(bt_iter_edge_v1->GetId(),*interval,
                  start_node->GetIntval());
//        e.delta_t = (interval->end-ui1->timeInterval.start).ToDouble();
        e.delta_t = (interval->end-queryinstant).ToDouble();

        e.delta_t_pre = e.delta_t; //real cost time
        e.delta_t += endp->Distance(end_point)/maxspeed;

        e.e_node_id = end_node->GetIntval();//end node id
        e.pre_edge_tid = 0;
        e.rpid = rpid->GetIntval();
        e.rid = rid->GetIntval();
        if(edge_flag[bt_iter_edge_v1->GetId()]){
 //         tmp_list.push(e);
          edge_flag[bt_iter_edge_v1->GetId()] = false;

              if(elemlist.empty())
                elemlist.push_back(e);
              else{
                unsigned int i = 0;
                for(;i < elemlist.size();i++){
                  if(elemlist[i].rid == e.rid &&
                        elemlist[i].e_node_id == e.e_node_id &&
                        elemlist[i].interval.start > e.interval.start){//update
                    elemlist[i] = e;
                    break;
                  }
                }
                if(i == elemlist.size())
                  elemlist.push_back(e);
              }
        }
      }
    }
    t->DeleteIfAllowed();
  }

  delete bt_iter_edge_v1;
  delete start_id;

//  cout<<"elemlist size "<<elemlist.size()<<endl;
  for(unsigned int i = 0;i < elemlist.size();i++)
    q_list.push(elemlist[i]);


  //////optimization techinique - 2  find all edges(path) end at end_node
//  ftime(&t1);
//  clock_t start_cpu = clock();
/*  double prune_time = numeric_limits<double>::max();

  list<Elem> end_node_edge;
  CcInt* end_node_id = new CcInt(true,id2);
  BTreeIterator* bt_iter_edge = btree2->ExactMatch(end_node_id);
  while(bt_iter_edge->Next()){
    Tuple* edge_tuple = busedge->GetTuple(bt_iter_edge->GetId());

    CcInt* start_node_id = (CcInt*)edge_tuple->GetAttribute(V1);
    CcInt* end_node_id = (CcInt*)edge_tuple->GetAttribute(V2);
    Periods* def_t = (Periods*)edge_tuple->GetAttribute(DEF_T);
    CcInt* rpid = (CcInt*)edge_tuple->GetAttribute(RPID);
    const Interval<Instant>* interval;
    def_t->Get(0,interval);
    //optimize-3, filter edge by start time
    if(interval->start < ui1->timeInterval.start){
        edge_tuple->DeleteIfAllowed();
        break;
    }
    if(interval->start >= ui1->timeInterval.start){
      Elem elem(bt_iter_edge->GetId(),*interval,start_node_id->GetIntval());
      elem.e_node_id = end_node_id->GetIntval();
      elem.rpid = rpid->GetIntval();
      end_node_edge.push_front(elem);
    }
    edge_tuple->DeleteIfAllowed();
  }
  delete end_node_id;
  delete bt_iter_edge;
  end_node_edge.sort(TimeCompare1());*/

  ///////////  Print ///////////////////////////
//  cout<<"end_node_edge size "<<end_node_edge.size()<<endl;
//  list<Elem>::iterator start = end_node_edge.begin();
//  for(;start != end_node_edge.end();start++)
//      start->Print2();

//   Optimize2(q_list,tmp_list,end_node_edge,prune_time);

//   ftime(&t2);
//   clock_t stop_cpu = clock();
//   cout<<"time 1 "<<difftimeb(&t2,&t1)<<" ";
//   cout<<"time 1 CPU "<<
//                        ((double)(stop_cpu-start_cpu))/CLOCKS_PER_SEC<<endl;
//  cout<<"initialize size "<<q_list.size()<<endl;
  if(q_list.empty() == true)
    return false;

//////////////////////////////////////////////////////////////////////////

  while(q_list.empty() == false){
//    cout<<"q_list size "<<q_list.size()<<endl;
    Elem top = q_list.top();

//    cout<<"top delta_t "<<top.delta_t<<endl;
//    cout<<"prune_time "<<prune_time<<endl;

//    Tuple* edge_tuple = busedge->GetTuple(top.edge_tuple_id);
//    CcInt* start_node = (CcInt*)edge_tuple->GetAttribute(V1);
//    CcInt* end_node = (CcInt*)edge_tuple->GetAttribute(V2);
    CcInt* start_node = new CcInt(true,top.s_node_id);
    CcInt* end_node = new CcInt(true,top.e_node_id);

    if(end_node->GetIntval() == id2){//find the end
      delete start_node;
      delete end_node;
      break;
    }

    expansionlist.push_back(top);
    expansion_count++;
    q_list.pop();

//    Periods* cur_def_t = (Periods*)edge_tuple->GetAttribute(DEF_T);
//    const Interval<Instant>* interval_cur;
//    cur_def_t->Get(0,interval_cur);
     Interval<Instant>* interval_cur = &top.interval;

//    outfile<<"edge "<<edge_id->GetIntval()<<" pre-edge tid "<<top.pre_edge_tid
//        <<" v1 "<<start_node->GetIntval()
//        <<" v2 "<<end_node->GetIntval()<<" time "<<*interval_cur
//        <<" delta_t "<<top.delta_t<<" rid "<<top.rid<<endl;

    /////////////get all edges from the same start node////////////////
//    clock_t start_cpu = clock();
//    ftime(&t1);
//    bt_iter_edge_v1 = btree1->ExactMatch(end_node);
    bt_iter_edge_v1 = btree_bus_edge_v1->ExactMatch(end_node);
    priority_queue<Elem> temp_list; //Optimize--1
    elemlist.clear();
    while(bt_iter_edge_v1->Next()){
//     Tuple* t = busedge->GetTuple(bt_iter_edge_v1->GetId());
     Tuple* t = bus_edge->GetTuple(bt_iter_edge_v1->GetId());

     CcInt* start_node_next = end_node;
     CcInt* end_node_next = (CcInt*)t->GetAttribute(V2);//end node
     Periods* next_def_t = (Periods*)t->GetAttribute(DEF_T);
     CcInt* rpid = (CcInt*)t->GetAttribute(RPID);
     CcInt* rid = (CcInt*)t->GetAttribute(PID);
     const Interval<Instant>* interval_next;
     next_def_t->Get(0,interval_next);
     Point* endp = (Point*)t->GetAttribute(P2);
     //optimize-3  filter edge by their start time
    if(interval_next->start < interval_cur->end){
        t->DeleteIfAllowed();
        break;
    }

   //time instant of next edge should be late than cur edge
   //end node id of next edge should not be equal to start node id of cur edge
//     cout<<"next "<<*interval_next<<endl;
//     cout<<*trip<<endl;

      if(interval_next->start > interval_cur->end &&
           end_node_next->GetIntval() != start_node->GetIntval()){
            //store all edges from the same start node
            Elem e(bt_iter_edge_v1->GetId(),*interval_next,
                  start_node_next->GetIntval());
            e.pre_eid = expansion_count - 1;
//            e.delta_t = top.delta_t +
//                          (interval_next->end-interval_cur->end).ToDouble();

            //real time cost
            e.delta_t = top.delta_t_pre +
                        (interval_next->end-interval_cur->end).ToDouble();
            e.delta_t_pre = e.delta_t;

            e.delta_t += endp->Distance(end_point)/maxspeed;
            e.rid = rid->GetIntval();
            e.e_node_id = end_node_next->GetIntval();
            e.pre_edge_tid = top.edge_tuple_id;
            e.rpid = rpid->GetIntval();
//            cout<<e.delta_t<<endl;
//////////////////    Optimize - 1    ////////////////////////////////////////
            if(edge_flag[bt_iter_edge_v1->GetId()]){
//              temp_list.push(e);
                edge_flag[bt_iter_edge_v1->GetId()] = false;

                if(elemlist.empty())
                  elemlist.push_back(e);
                else{
                  unsigned int i = 0;
                  for(;i < elemlist.size();i++){
                    if(elemlist[i].rid == e.rid &&
                        elemlist[i].e_node_id == e.e_node_id &&
                        elemlist[i].interval.start > e.interval.start){//update
                        elemlist[i] = e;
                        break;
                    }
                  }
                  if(i == elemlist.size())
                    elemlist.push_back(e);
                }
            }
//////////////////////////////////////////////////////////////////////////////

        }
        t->DeleteIfAllowed();
      }
      delete bt_iter_edge_v1;

//      ftime(&t2);
//      clock_t stop_cpu = clock();
//      cout<<"time 2 total "<<difftimeb(&t2,&t1)<<" ";
//      cout<<"time 2 CPU "<<
//                        ((double)(stop_cpu-start_cpu))/CLOCKS_PER_SEC<<endl;
//      cout<<"temp_list size 1 "<<temp_list.size()<<endl;
//      Optimize2(q_list,temp_list,end_node_edge,prune_time);

  for(unsigned int i = 0;i < elemlist.size();i++)
    q_list.push(elemlist[i]);

    //////////////////////////////////////////////////////////////////////////
    delete start_node;
    delete end_node;
//    cout<<"find_path_t_5 q_list size "<<q_list.size()<<endl;
  }

  if(q_list.empty() == false){
    stack<Elem> temp_path;
    Elem top = q_list.top();
    temp_path.push(top);
//    cout<<top.edge_tuple_id<<endl;
    while(top.pre_eid != -1){
      int id = top.pre_eid;
//      cout<<"pre id "<<id<<endl;
      top = expansionlist[id];
//      cout<<top.edge_tuple_id<<endl;
      temp_path.push(top);
    }
//    cout<<temp_path.size()<<endl;
    while(temp_path.empty() == false){
      Elem top = temp_path.top();
      temp_path.pop();
      path.push_back(top);
    }
  }

  return true;
}

/*
expand the graph by Dijkstra with minimum total time cost so far
with some optimization techniques, supporting time duration for middle stop
optimize-1 filter the edge from the same route but comes later
optimize-2 use pre-defined path to find end point
optimize-3 filter edge by their start time
optimize-4 use A star algorithm, heuristic-value = distance/maxspeed
input edge relation and b-tree

*/
void BusNetwork::FindPath_T_5(MPoint* mp,Relation* query,
int attrpos1,int attrpos2,Instant& queryinstant)
{
//  FillAdjacencyPath();

  if(query->GetNoTuples() < 2){
    cout<<"there is only start location, please give destination"<<endl;
    return;
  }

  mp->Clear();
  mp->StartBulkLoad();

//  vector<int> path; //record edge id
  vector<Elem> path;
  //searching process

//  TestFunction(busedge,btree1);
  for(int i = 1;i <= query->GetNoTuples() - 1;i++){
    Tuple* t1 = query->GetTuple(i);
    Tuple* t2 = query->GetTuple(i+1);
    CcInt* id1 = (CcInt*)t1->GetAttribute(attrpos1);
    CcInt* id2 = (CcInt*)t2->GetAttribute(attrpos1);

    DateTime* timestay = (DateTime*)t1->GetAttribute(attrpos2);
//    cout<<id1->GetIntval()<<" "<<id2->GetIntval()<<endl;
//    cout<<*timestay<<endl;
    double waittime = timestay->ToDouble();
//    cout<<waittime<<endl;

    if(FindPath5(id1->GetIntval(),id2->GetIntval(),
              path,queryinstant,waittime)==false){
        cout<<"such a route is not valid"<<endl;
        path.clear();
        break;
    }

  }
  /****************Construct the Reulst (MPoint)*************************/
  const UPoint* lastup = NULL;
  for(unsigned int i = 0;i < path.size();i++){
//    cout<<path[i]<<" ";
//    Tuple* edge_tuple = busedge->GetTuple(path[i].edge_tuple_id);
    Tuple* edge_tuple = bus_edge->GetTuple(path[i].edge_tuple_id);
    MPoint* temp_mp = (MPoint*)edge_tuple->GetAttribute(MOVE);

    for(int j = 0;j < temp_mp->GetNoComponents();j++){
      const UPoint* up;
      temp_mp->Get(j,up);
      //not the first trip
      if(lastup != NULL && i != 0 && j == 0){
        UPoint* insert_up = new UPoint(true);
//        cout<<"last "<<*lastup<<endl;
//        cout<<"cur "<<*up<<endl;
        insert_up->p0 = lastup->p1;
        insert_up->timeInterval.start = lastup->timeInterval.end;
        insert_up->p1 = up->p0;
        insert_up->timeInterval.end = up->timeInterval.start;
//        cout<<"insert "<<*insert_up<<endl;
        insert_up->timeInterval.lc = true;
        insert_up->timeInterval.rc = false;
        mp->Add(*insert_up);
        delete insert_up;
        delete lastup;
        lastup = NULL;
      }
      mp->Add(*up);
      if(j == temp_mp->GetNoComponents() - 1 && i != path.size() - 1)
        lastup = new UPoint(*up);
    }
    edge_tuple->DeleteIfAllowed();
  }
  mp->EndBulkLoad();
}
///////////////////////Bus Node Tree Representation /////////////////////////
struct E_Node_Tree:public Node_Tree{

  int parent;
  double cost;
  double e_cost;
  double last_zval; //zvalue of last node
  E_Node_Tree(){}
  E_Node_Tree(Node_Tree node):Node_Tree(node),parent(-1),
      cost(0),e_cost(0),last_zval(0){}
  E_Node_Tree(const E_Node_Tree& enode):Node_Tree(enode),
        parent(enode.parent),cost(enode.cost),
        e_cost(enode.e_cost),last_zval(enode.last_zval){}
  bool operator<(const E_Node_Tree& enode)const
  {
    if(cost < enode.cost)
      return false;
    return true;
  }
  E_Node_Tree& operator=(const E_Node_Tree& enode)
  {
    Node_Tree::operator=(enode);
    parent = enode.parent;
    cost = enode.cost;
    e_cost = enode.e_cost;
    last_zval = enode.last_zval;
    return *this;
  }
  void E_Print()
  {
    Print();
    cout<<"parent "<<parent<<" cost "<<cost<<endl;
  }

};

bool BusNetwork::Bus_Tree1(vector<int>& stops,vector<double>& duration,
vector<E_Node_Tree>& path,Instant& queryinstant)
{

  if(stops.size() < 2)
    return false;

  const double* pszvalue;
  vector<double> stops_zval; //zvalue for each bus stop
  for(unsigned int i = 0;i < stops.size();i++){
    ps_zval.Get(stops[i]-1,pszvalue);
    stops_zval.push_back(*pszvalue);
  }
  int start_index = 0;//entry for stops and duration
  int end_index = start_index + 1;//entry for stops and duration

//  for(int i = 0 ;i < stops.size();i++)
//    cout<<" stop "<<stops[i]<<" "<<" duration "
//        <<duration[i]<<" zval "<<stops_zval[i]<<endl;


  CcReal* start_tid = new CcReal(true,stops_zval[start_index]);
  BTreeIterator* bt_iter_v = btree_bus_node_new2->ExactMatch(start_tid);
  int start_node = -1;
  while(bt_iter_v->Next()){
    Tuple* node = bus_node_new->GetTuple(bt_iter_v->GetId());
    CcReal* atime = (CcReal*)node->GetAttribute(ATIME);

    if(queryinstant.ToDouble() < atime->GetRealval()){
      start_node = node->GetTupleId();
      node->DeleteIfAllowed();
      break;
    }
    node->DeleteIfAllowed();
  }
  delete start_tid;
  delete bt_iter_v;


  if(start_node == -1){
      cout<<"no valid start node"<<endl;
      return false;
  }

//  Tuple* t = bus_node_new->GetTuple(start_node);
//  cout<<*t<<endl;
//  t->DeleteIfAllowed()  ;

  vector<Point> endps;
  for(unsigned int j = end_index;j < stops_zval.size();j++){

    CcReal* end_tid = new CcReal(true,stops_zval[j]);
    bt_iter_v = btree_bus_node_new2->ExactMatch(end_tid);
    while(bt_iter_v->Next()){
      Tuple* node = bus_node_new->GetTuple(bt_iter_v->GetId());
      CcReal* zval = (CcReal*)node->GetAttribute(ZVAL);

      if(AlmostEqual(stops_zval[j],zval->GetRealval())){
        Point* endp = new Point(*((Point*)node->GetAttribute(NEWLOC)));
        endps.push_back(*endp);
        delete endp;
        node->DeleteIfAllowed();
        break;
      }
      node->DeleteIfAllowed();
    }
    delete end_tid;
    delete bt_iter_v;
  }

  if(endps.empty()){
    cout<<"end point is not valid"<<endl;
    return false;
  }
  unsigned int endps_index = 0; //start from 0

  const Node_Tree* start;
  bus_tree.Get(start_node-1,start);
//  (const_cast<Node_Tree*>(start))->Print();

  priority_queue<E_Node_Tree> q_list;
  E_Node_Tree e_node(*(const_cast<Node_Tree*>(start)));
  e_node.parent = -1;
  e_node.cost = start->t - queryinstant.ToDouble()+
                  start->p.Distance(endps[endps_index])/maxspeed;
  e_node.e_cost = start->t - queryinstant.ToDouble();
  e_node.last_zval = e_node.zorder;
  q_list.push(e_node);

//  double end_point_zval = ZValue(*endp);

  vector<E_Node_Tree> expansionlist;
  int expansion_counter = 0;
  int maxroute = bus_route->GetNoTuples();

  while(q_list.empty() == false){
    E_Node_Tree top = q_list.top();

//    cout<<"top element "<<endl;
//    top.Print();
    if(AlmostEqual(top.zorder,stops_zval[end_index])){
        ////////////  display    /////////////////////
        /*cout<<"display "<<endl;
        E_Node_Tree elem = top;
        elem.E_Print();
        while(elem.parent != -1){
          elem = expansionlist[elem.parent];
          elem.E_Print();
        }*/
        /////////////////////////////////////////////


//        cout<<"find "<<top.zorder<<endl;
        endps_index++;
        if(endps_index == endps.size())
          break;
        //deal with next shortest path neighbor

        start_index++;
        end_index++;
//        cout<<duration[start_index]<<endl;
        if(!AlmostEqual(duration[start_index],0.0)){ //duration time
           CcReal* start_tid = new CcReal(true,stops_zval[start_index]);
           BTreeIterator* bt_iter_v =
                              btree_bus_node_new2->ExactMatch(start_tid);
           int start_node = -1;
           while(bt_iter_v->Next()){
             Tuple* node = bus_node_new->GetTuple(bt_iter_v->GetId());
             CcReal* atime = (CcReal*)node->GetAttribute(ATIME);

             if(atime->GetRealval() >= (top.t + duration[start_index])){
                start_node = node->GetTupleId();
                node->DeleteIfAllowed();
                break;
             }
             node->DeleteIfAllowed();
           }
           delete start_tid;
           delete bt_iter_v;

            expansionlist.push_back(top);
            q_list.pop();

            assert(start_node != -1 && start_node != top.id);

            const Node_Tree* start;
            bus_tree.Get(start_node-1,start);
            E_Node_Tree e_node(*(const_cast<Node_Tree*>(start)));
            e_node.parent = expansion_counter;
            e_node.cost = top.cost + (start->t - top.t) +
                  start->p.Distance(endps[endps_index])/maxspeed;
            e_node.e_cost = top.cost + (start->t - top.t);

            while(q_list.empty() == false)
              q_list.pop();


            q_list.push(e_node);
            expansion_counter++;
//            cout<<"new start node "<<endl;
//            e_node.E_Print();
            continue;

        }
    }
    expansionlist.push_back(top);
    q_list.pop();

    /////////////    left   ///////////////////////////////
    int left = top.left;
    if(left != -1){
      const Node_Tree* l_left_node;
      bus_tree.Get(left-1,l_left_node);
      Node_Tree* left_node = const_cast<Node_Tree*>(l_left_node);

      E_Node_Tree l_e_node(*(const_cast<Node_Tree*>(left_node)));
//      l_e_node.parent = top.id;
      l_e_node.parent = expansion_counter;
      l_e_node.cost = top.cost + (left_node->t - top.t) +
                  left_node->p.Distance(endps[endps_index])/maxspeed;
      l_e_node.e_cost = l_e_node.cost + (left_node->t - top.t);
      l_e_node.last_zval = top.zorder;
      if(l_e_node.zorder != top.last_zval)
          q_list.push(l_e_node);
    }
    ///////////////   right   //////////////////////////////////////////////
    //////   for the node located in the same position in space   //////////
    /////////   go further only once    ///////////////////////////////////

    vector<int> bus_line;
//    bus_line.push_back(top.busline);

    int right = top.right;
/*    if(right != -1){
        const Node_Tree* r_right_node;
        bus_tree.Get(right-1,r_right_node);
        Node_Tree* right_node = const_cast<Node_Tree*>(r_right_node);

        E_Node_Tree r_e_node(*(const_cast<Node_Tree*>(right_node)));
        r_e_node.parent = top.id;
        r_e_node.cost = top.cost + (right_node->t - top.t) +
                  right_node->p.Distance(*endp)/maxspeed;
        r_e_node.e_cost = r_e_node.cost + (right_node->t - top.t);
        q_list.push(r_e_node);
    }*/

    while(right != -1 && bus_line.size() < top.diff_bus){
        const Node_Tree* r_right_node;
        bus_tree.Get(right-1,r_right_node);
        Node_Tree* right_node = const_cast<Node_Tree*>(r_right_node);

        E_Node_Tree r_e_node(*(const_cast<Node_Tree*>(right_node)));
        r_e_node.parent = expansion_counter;
        r_e_node.cost = top.cost + (right_node->t - top.t) +
                  right_node->p.Distance(endps[endps_index])/maxspeed;
        r_e_node.e_cost = r_e_node.cost + (right_node->t - top.t);
        r_e_node.last_zval = top.last_zval;
        right = r_e_node.right;
        r_e_node.right = -1;
        unsigned int j = 0;
        for(;j < bus_line.size();j++)
            if(bus_line[j] == r_e_node.busline ||
                top.busline ==  r_e_node.busline)
              break;
        if(j == bus_line.size()){
          if(r_e_node.busline < maxroute){
//              if(r_e_node.busline != top.busline + maxroute)
                if(r_e_node.busline != top.busline)
                    q_list.push(r_e_node);
          }
          else{
//              if(top.busline != maxroute + r_e_node.busline)
                if(r_e_node.busline != top.busline)
                    q_list.push(r_e_node);
          }
          if(r_e_node.busline != top.busline)
            bus_line.push_back(r_e_node.busline);
        }
    }
    expansion_counter++;

  }
//  delete endp;

  /// backward and construct the result ///
  stack<E_Node_Tree> temp_path;
  E_Node_Tree top_elem = q_list.top();
  temp_path.push(top_elem);
//  top_elem.E_Print();
  while(top_elem.parent != -1){
      top_elem = expansionlist[top_elem.parent];
      temp_path.push(top_elem);
//      top_elem.E_Print();
  }
  while(temp_path.empty() == false){
    E_Node_Tree elem = temp_path.top();
    temp_path.pop();
    path.push_back(elem);
  }

/*  for(int i = 0;i < path.size();i++)
    path[i].E_Print();*/

  return true;
}


void BusNetwork::FindPath_Bus_Tree1(MPoint* mp,Relation* query,
int attrpos1,int attrpos2,Instant& queryinstant)
{
  if(query->GetNoTuples() < 2){
    cout<<"there is only start location, please give destination"<<endl;
    return;
  }

  mp->Clear();
  mp->StartBulkLoad();

  vector<E_Node_Tree> path; //record edge id

  vector<int> stops;
  vector<double> duration;
  //searching process
//  TestFunction(busedge,btree1);
  for(int i = 1;i <= query->GetNoTuples();i++){
    Tuple* t1 = query->GetTuple(i);
    CcInt* id1 = (CcInt*)t1->GetAttribute(attrpos1);
    DateTime* timestay = (DateTime*)t1->GetAttribute(attrpos2);

//    cout<<*timestay<<endl;
    double waittime = timestay->ToDouble();
//    cout<<"time delay "<<waittime<<endl;

    stops.push_back(id1->GetIntval());
    duration.push_back(waittime);
    t1->DeleteIfAllowed();

  }

  if(Bus_Tree1(stops,duration,path,queryinstant)==false){
        cout<<"such a route is not valid"<<endl;
        return;
 }


  UPoint* last = NULL;
  bool first_path = true;
  for(unsigned int i = 0; i < path.size();){
//    path[i].E_Print();

    int buspath = path[i].pathid;
    int start_index = i;
    int end_index = i;
    while(path[i].pathid == buspath){
      end_index = i;
      i++;
    }

    CcInt* bus_path_id = new CcInt(true,buspath);
    BTreeIterator* bt_iter = btree_bus_route->ExactMatch(bus_path_id);
    while(bt_iter->Next()){
        Tuple* t = bus_route->GetTuple(bt_iter->GetId());
        MPoint* trip = (MPoint*)t->GetAttribute(TRIP);
        const UPoint* up1;
        const UPoint* up2;
        bool find1 = false;
        bool first_flag = false;
        for(int j = 0;j < trip->GetNoComponents();j++){
          trip->Get(j,up1);
          if(AlmostEqual(up1->p0,path[start_index].p))
            find1 = true;

          if(find1 == true){
//             (const_cast<UPoint*>(up1))->Print(cout);
              if(last!= NULL && first_flag == false && first_path == false){
                UPoint* insert_up = new UPoint(true);
                insert_up->p0 = last->p1;
                insert_up->p1 = up1->p0;
                insert_up->timeInterval.start = last->timeInterval.end;
                insert_up->timeInterval.end = up1->timeInterval.start;
                insert_up->timeInterval.lc = true;
                insert_up->timeInterval.rc = false;
//                cout<<*insert_up<<endl;
                if(!AlmostEqual(insert_up->timeInterval.start.ToDouble(),
                                insert_up->timeInterval.end.ToDouble()))
                  mp->Add(*insert_up);
                delete insert_up;
                first_flag = true;
              }
              mp->Add(*up1);
              last = const_cast<UPoint*>(up1); //last position
            }

          if(AlmostEqual(up1->p1,path[end_index].p) && find1){
//            cout<<*up1<<endl;
//            cout<<"break"<<endl;
            break;
          }
        }
        t->DeleteIfAllowed();
    }
    delete bus_path_id;
    delete bt_iter;
    first_path = false;
  }
  mp->EndBulkLoad();
}

