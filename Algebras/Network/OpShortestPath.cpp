/*
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

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]

1.1 Implementation of Operator ShorestPath

Mai-Oktober 2007 Martin Scheppokat

This file contains the implementation of the operator ShortestPath. It uses
a modified version of diskstras algorithm keeping edges instead of nodes
in Q.


Includes

*/
#include "RelationAlgebra.h"
#include "BTreeAlgebra.h"
#include "DBArray.h"
#include "TupleIdentifier.h"

#include "SpatialAlgebra.h"
#include "StandardTypes.h"
#include "GLine.h"
#include "GPoint.h"
#include "Network.h"
#include "NetworkManager.h"

#include "OpShortestPath.h"
#include "Messages.h"
#include <time.h>

/*
TypeMap: Type mapping function of the operator

*/
ListExpr OpShortestPath::TypeMap(ListExpr in_xArgs)
{
  if( nl->ListLength(in_xArgs) != 2 )
    return (nl->SymbolAtom( "typeerror" ));

  ListExpr xGPoint1Desc = nl->First(in_xArgs);
  ListExpr xGPoint2Desc = nl->Second(in_xArgs);

  if( (!nl->IsAtom( xGPoint1Desc )) ||
      nl->AtomType( xGPoint1Desc ) != SymbolType ||
      nl->SymbolValue( xGPoint1Desc ) != "gpoint" )
  {
    return (nl->SymbolAtom( "typeerror" ));
  }

  if( (!nl->IsAtom( xGPoint2Desc )) ||
      nl->AtomType( xGPoint2Desc ) != SymbolType ||
      nl->SymbolValue( xGPoint2Desc ) != "gpoint" )
  {
    return (nl->SymbolAtom( "typeerror" ));
  }

  return nl->SymbolAtom( "gline" );
}

/*
ValueMapping: Value mapping function of the operator

*/
int OpShortestPath::ValueMapping( Word* args,
                                  Word& result,
                                  int message,
                                  Word& local,
                                  Supplier in_xSupplier )
{
  // Get values
  GLine* pGLine = (GLine*)qp->ResultStorage(in_xSupplier).addr;
  result = SetWord( pGLine );

  GPoint* pFromGPoint = (GPoint*)args[0].addr;
  GPoint* pToGPoint = (GPoint*)args[1].addr;

  if (pFromGPoint == 0 || pToGPoint == 0 || !pFromGPoint->IsDefined() ||
      !pToGPoint->IsDefined()) {
     sendMessage("Both gpoints must exist and be defined.");
     return 0;
  }
  // Check wether both points belong to the same network
  if(pFromGPoint->GetNetworkId() != pToGPoint->GetNetworkId())
  {
    sendMessage("Both gpoints belong to different networks.");
    return 0;
  }

  // Load the network
  Network* pNetwork = NetworkManager::GetNetwork(pFromGPoint->GetNetworkId());
  if(pNetwork == 0)
  {
    ostringstream xStream;
    xStream << pFromGPoint->GetNetworkId() << char(0);
    string strMessage = "Network with id '" +
                        xStream.str() +
                        "' does not exist on this database.";
    sendMessage(strMessage);
    return 0;
  }

  // Get sections where the path should start or end
  Tuple* pFromSection = pNetwork->GetSectionOnRoute(pFromGPoint);
  if (pFromSection == 0) {
    sendMessage("Starting GPoint not found in network.");
    pFromSection->DeleteIfAllowed();
    NetworkManager::CloseNetwork(pNetwork);
    return 0;
  }
  Tuple* pToSection = pNetwork->GetSectionOnRoute(pToGPoint);
  if (pToSection == 0) {
    sendMessage("End GPoint not found in network.");
    pFromSection->DeleteIfAllowed();
    pToSection->DeleteIfAllowed();
    NetworkManager::CloseNetwork(pNetwork);
    return 0;
  }
  Point* pToPoint = pNetwork->GetPointOnRoute(pToGPoint);

  pGLine->SetNetworkId(pNetwork->GetId());

  // Calculate the shortest path
  Dijkstra(pNetwork,
           pFromSection->GetTupleId(),
           pFromGPoint,
           pToSection->GetTupleId(),
           pToGPoint,
           pToPoint,
           pGLine);

  // Cleanup and return
  delete pToPoint;
  pFromSection->DeleteIfAllowed();
  pToSection->DeleteIfAllowed();
  NetworkManager::CloseNetwork(pNetwork);
  return 0;
}

/*
Dijkstra: Modified Version of Dijkstras Algorithm.

Modified version of dikstras algorithm calculating shortest path in graphs.

Whereas sedgewick's version of disktra operates on the nodes of the graph this
version will process an array of edges. This change is necessary to take into
account that not all transitions between connecting edges are possible. The
preceding edge has to be known when looking for the next one.

*/
void OpShortestPath::Dijkstra(Network* in_pNetwork,
                              int in_iStartSegmentId,
                              GPoint* in_pFromGPoint,
                              int in_iEndSegmentId,
                              GPoint* in_pToGPoint,
                              Point* in_pToPoint,
                              GLine* in_pGLine)
{
 // clock_t xEnterTime = clock();
  // Specialized data structure with all edges of the graph. This
  // structure will not only support the access to the remaining edges
  // with the fewest weight but also to all edges by their index.
  PriorityQueue xQ;

  /////////////////////////////////////////
  //
  // InitializeSingleSource
  //
  // Get all Sections and fill them into Q. All weights but
  // the one for the starting segment are set to infinity
  Relation* pSections = in_pNetwork->GetSectionsInternal();
  GenericRelationIterator* pSectionsIt = pSections->MakeScan();
  Tuple* pSection;
  while( (pSection = pSectionsIt->GetNextTuple()) != 0 )
  {
    // Get values
    int iSegmentId = pSectionsIt->GetTupleId();
    CcInt* xRouteId = (CcInt*)pSection->GetAttribute(SECTION_RID);
    int iRouteId = xRouteId->GetIntval();
    CcReal* xMeas1 = (CcReal*)pSection->GetAttribute(SECTION_MEAS1);
    double dMeas1 = xMeas1->GetRealval();
    CcReal* xMeas2 = (CcReal*)pSection->GetAttribute(SECTION_MEAS2);
    double dMeas2 = xMeas2->GetRealval();
    Line* pLine = (Line*)pSection->GetAttribute(SECTION_CURVE);
    Point* pPoint = new Point(false);
    pLine->AtPosition(0, true, *pPoint);

    double dx = pPoint->GetX() - in_pToPoint->GetX();
    double dy = pPoint->GetY() - in_pToPoint->GetY();
    delete pPoint;
    double dHeuristicDistanceToEnd = sqrt( pow( dx, 2 ) + pow( dy, 2 ) );

//    // Use Dijkstra instead of A*
//    fHeuristicDistanceToEnd = 0;

    // Put one struct for each direction in the map (up and down)
    bool bStartSegment = iSegmentId == in_iStartSegmentId &&
                         (in_pFromGPoint->GetSide() == None ||
                          in_pFromGPoint->GetSide() == Up);
    xQ.push(new DijkstraStruct(iSegmentId,
                               true,
                               iRouteId,
                               dMeas1,
                               dMeas2,
                               bStartSegment ? 0 : double(1e29),
                               dHeuristicDistanceToEnd,
                               -1,
                               true));

    bStartSegment = iSegmentId == in_iStartSegmentId &&
                         (in_pFromGPoint->GetSide() == None ||
                         in_pFromGPoint->GetSide() == Down);
    xQ.push(new DijkstraStruct(iSegmentId,
                               false,
                               iRouteId,
                               dMeas1,
                               dMeas2,
                               bStartSegment ? 0 : double(1e29),
                               dHeuristicDistanceToEnd,
                               -1,
                               true));

    pSection->DeleteIfAllowed();
  }
  delete pSectionsIt;
//       << "InitializeSingleSource Time: "
//       << (clock() - xEnterTime) << " / " << CLOCKS_PER_SEC << " Sekunden."
//       << endl;
  // End InitializeSingleSource

  /////////////////////////////////
  //
  // Now dikstras algorithm will handle each node in
  // the queue recalculating their weights.
  //
  while(! xQ.isEmtpy())
  {
    // Extract-Min
    DijkstraStruct* pCurrent = xQ.pop();

    // Abbruchbedingung pr�fen
    if(pCurrent->m_iSectionTid == in_iEndSegmentId &&
       (
         in_pToGPoint->GetSide() == None ||
         (
           (
             in_pToGPoint->GetSide() == Up &&
             pCurrent->m_bUpDownFlag
           ) ||
           (
             in_pToGPoint->GetSide() == Down &&
             !pCurrent->m_bUpDownFlag
           )
         )
       )
      )
    {
      break;
    }

    // Get values
    int iCurrentSectionTid = pCurrent->m_iSectionTid;
    bool bCurrentUpDownFlag = pCurrent->m_bUpDownFlag;

    // Get all adjacent sections
    vector<DirectedSection> xAdjacentSections;
    xAdjacentSections.clear();
    in_pNetwork->GetAdjacentSections(iCurrentSectionTid,
                                     bCurrentUpDownFlag,
                                     xAdjacentSections);

    // Iterate over adjacent sections
    for(size_t i = 0;  i < xAdjacentSections.size(); i++)
    {
      // Load the structure belonging to the adjacent section
      DirectedSection xAdjacentSection = xAdjacentSections[i];
      int iAdjacentSectionTid = xAdjacentSection.getSectionTid();
      bool bAdjacentUpDownFlag = xAdjacentSection.getUpDownFlag();
      DijkstraStruct* pAdjacent = xQ.get(iAdjacentSectionTid,
                                         bAdjacentUpDownFlag);

      // Relax the weight if a shorter path has been found
      if(pAdjacent->m_dD > pCurrent->m_dD + pCurrent->Length())
      {
        // Calculate new distance
        pAdjacent->m_dD = pCurrent->m_dD + pCurrent->Length();
        // Set current as predecessor of adjacent section
        pAdjacent->m_iPiSectionTid = pCurrent->m_iSectionTid;
        pAdjacent->m_bPiUpDownFlag = pCurrent->m_bUpDownFlag;
      }
    }
  }
  // Now all weights and all predecessors have been found.

  // Find the route starting at the end looking at the pi-entries pointing
  // at a predecessor.
  DijkstraStruct* pStruct;
  if(in_pToGPoint->GetSide() == Up)
  {
    pStruct = xQ.get(in_iEndSegmentId,
                     true);
  }
  else if(in_pToGPoint->GetSide() == Down)
  {
    pStruct = xQ.get(in_iEndSegmentId,
                     false);
  }
  else
  {
    // GPoint lies on an undirected section. We first get the distance
    // for both sides
    double dUpDistance = xQ.get(in_iEndSegmentId, true)->m_dD;
    double dDownDistance = xQ.get(in_iEndSegmentId, false)->m_dD;
    // Now we get the directed section for the shorter Distance
    pStruct = xQ.get(in_iEndSegmentId,
                     dUpDistance < dDownDistance);
  }

  if(pStruct->m_iPiSectionTid == -1)
  {
    // End not reachable from start. Either the graph consists of two
    // parts or the ConnectivityCode prevents driving on all path from
    // start to end.
    // Return empty GLine
    string strMessage = "Destination is not reachable from the start. Either "
                        "the points are located in disjoint parts of the "
                        "network or the ConnectityCode prevents reaching the "
                        "destination.";
    sendMessage(strMessage);
    return;
  }

  // Add last interval of the route
  double dPos1 = pStruct->m_dMeas1;
  double dPos2 = pStruct->m_dMeas2;
  if(pStruct->m_bUpDownFlag)
  {
    dPos2 = in_pToGPoint->GetPosition();
  }
  else
  {
    dPos1 = in_pToGPoint->GetPosition();
  }
  in_pGLine->AddRouteInterval(pStruct->m_iRouteId,
                              dPos1,
                              dPos2);
  // Add all predecessors until the start is reached.
  while(pStruct->m_iSectionTid != in_iStartSegmentId)
  {
    pStruct = xQ.get(pStruct->m_iPiSectionTid,
                     pStruct->m_bPiUpDownFlag);
    double dPos1 = pStruct->m_dMeas1;
    double dPos2 = pStruct->m_dMeas2;

    if(pStruct->m_iSectionTid == in_iStartSegmentId)
    {
      // First segment reached. We only need a part of this segment
      if(pStruct->m_bUpDownFlag)
      {
        dPos1 = in_pFromGPoint->GetPosition();
      }
      else
      {
        dPos2 = in_pFromGPoint->GetPosition();
      }
    }

    in_pGLine->AddRouteInterval(pStruct->m_iRouteId,
                                dPos1,
                                dPos2);
  }

//       << "DijkstraTime: "
//       << (clock() - xEnterTime) << " / " << CLOCKS_PER_SEC << " Sekunden."
//       << endl;
}



/*
Specification of the operator

*/
const string OpShortestPath::Spec  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" ) "
  "( <text>gpoint x gpoint -> gline" "</text--->"
  "<text>shortest_path(_, _)</text--->"
  "<text>Calculates the shortest path between two gpoints.</text--->"
  "<text>let n = shortestpath(x, y)</text--->"
  ") )";


/*
sendMessage: Sending a message via the message-center

*/
void OpShortestPath::sendMessage(string in_strMessage)
{
  // Get message-center and initialize message-list
  static MessageCenter* xMessageCenter= MessageCenter::GetInstance();
  NList xMessage;
  xMessage.append(NList("error"));

  xMessage.append(NList().textAtom(in_strMessage));

  xMessageCenter->Send(xMessage);
}
