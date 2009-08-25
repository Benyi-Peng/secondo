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

[1] Header File of the Spatiotemporal Pattern Algebra

August, 2009 Jianqiu Xu

[TOC]

1 Overview

2 Defines and includes

*/

#ifndef BusNetwork_H
#define BusNetwork_H


#include "Algebra.h"

#include "NestedList.h"

#include "QueryProcessor.h"
#include "RTreeAlgebra.h"
#include "BTreeAlgebra.h"
#include "TemporalAlgebra.h"
#include "StandardTypes.h"
#include "LogMsg.h"
#include "NList.h"
#include "RelationAlgebra.h"

/*
Subclass for manage bus network

*/
class BusNetwork{
public:
/*data*/
  static string busrouteTypeInfo;//relation description for pre-defined paths
  enum BusRouteInfo{RID=0,TRIP};
  static string busstopTypeInfo; //relation description for bus stop
  enum BusStopInfo{SID=0,LOC};

/*function for type constructor*/
  static ListExpr BusNetworkProp();
  static ListExpr OutBusNetwork(ListExpr,Word);
  static Word InBusNetwork(ListExpr,ListExpr,int,ListExpr&,bool&);
  static Word CreateBusNetwork(const ListExpr);
  static void DeleteBusNetwork(const ListExpr,Word&);
  static bool OpenBusNetwork(SmiRecord&,size_t&,const ListExpr,Word&);
  static bool SaveBusNetwork(SmiRecord&,size_t&,const ListExpr,Word&);
  static void CloseBusNetwork(const ListExpr,Word&);
  static Word CloneBusNetwork(const ListExpr,const Word&);
  static void* CastBusNetwork(void* addr);
  static int SizeOfBusNetwork();
  static bool CheckBusNetwork(ListExpr,ListExpr&);
  ListExpr Out(ListExpr typeInfo);
  void Load(int in_iId,const Relation* in_busRoute);
  static BusNetwork* Open(SmiRecord& valueRecord,size_t& offset,
                          const ListExpr typeInfo);
  bool Save(SmiRecord&,size_t&,const ListExpr);
/*Type Constructor and Deconstructor*/
  BusNetwork(ListExpr in_xValue,int in_iErrorPos,ListExpr& inout_xErrorInfo,
             bool& inout_bCorrect);
  BusNetwork();
  BusNetwork(SmiRecord&,size_t&,const ListExpr);
  void FillBusNode(const Relation*);
  void  Destory();
/*Interface function*/
  Relation* GetRelBus_Node(){return bus_node;}

private:
  int bus_id;
  bool bus_def;
  Relation* bus_node; //relation storing bus stops
  Relation* bus_weight; //relation storing weight function for each edge
  Relation* bus_edge;//relation storing edge
};


#endif