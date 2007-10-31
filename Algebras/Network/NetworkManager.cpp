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

1.1 Implementation of a Class Managing Networks


Mai-July 2007 Martin Scheppokat

Defines, includes, and constants

*/



#include "TupleIdentifier.h"
#include "NestedList.h"
#include "BTreeAlgebra.h"
#include "DBArray.h"
#include "GPoint.h"
#include "RelationAlgebra.h"
#include "SpatialAlgebra.h"
#include "Network.h"
#include "NetworkManager.h"

extern NestedList* nl;


/*
Loads a network with a given id.

*/
Network* NetworkManager::GetNetwork(int in_iNetworkId)
{
  // Search in all objects in the current database for a network
  ListExpr xObjectList = SecondoSystem::GetCatalog()->ListObjects();
  xObjectList = nl->Rest(xObjectList);
  while(!nl->IsEmpty(xObjectList))
  {
    // Next element in list
    ListExpr xCurrent = nl->First(xObjectList);
    xObjectList = nl->Rest(xObjectList);
    
    // Type of object is at fourth position in list
    ListExpr xObjectType = nl->First(nl->Fourth(xCurrent));
    if(nl->IsAtom(xObjectType) &&
       nl->SymbolValue(xObjectType) == "network")
    {
      // Get name of the network
      ListExpr xObjectName = nl->Second(xCurrent);
      string strObjectName = nl->SymbolValue(xObjectName);
      
      // Load object to find out the id of the network. Normally their
      // won't be to much networks in one database giving us a good
      // chance to load only the wanted network.
      Word xValue;
      bool bDefined;
      bool bOk = SecondoSystem::GetCatalog()->GetObject(strObjectName,
                                                        xValue,
                                                        bDefined);
      if(!bDefined || !bOk)
      {
        // Undefined network
        continue;
      }
  
      Network* pNetwork = (Network*)xValue.addr;
      if(pNetwork->GetId() == in_iNetworkId)
      {
        // This is the network we have been looking for
        return pNetwork;
      }
      
      SecondoSystem::GetCatalog()->CloseObject(nl->SymbolAtom("network"),
                                               xValue);
    } 
  }
  return 0;
}

/*
Closes a network retrived via getNetwork.

*/
void NetworkManager::CloseNetwork(Network* in_pNetwork)
{
  Word xValue;
  xValue.addr = in_pNetwork;
  SecondoSystem::GetCatalog()->CloseObject(nl->SymbolAtom( "network" ),
                                           xValue);
}
