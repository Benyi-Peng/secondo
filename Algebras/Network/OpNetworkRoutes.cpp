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

1.1 Implementation of operator routes


Mai-Oktober 2007 Martin Scheppokat

Parts of the source taken from Victor Almeida

Defines, includes, and constants

*/
#include "RelationAlgebra.h"
#include "BTreeAlgebra.h"
#include "DBArray.h"
#include "TupleIdentifier.h"

#include "SpatialAlgebra.h"
#include "StandardTypes.h"
#include "GPoint.h"
#include "Network.h"

#include "OpNetworkRoutes.h"

/*
Type Mapping of operator ~routes~

*/
ListExpr OpNetworkRoutes::TypeMap(ListExpr args)
{
  ListExpr arg1;
  if( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );
    if( nl->IsAtom( arg1 ) && 
        nl->AtomType( arg1 ) == SymbolType &&
        nl->SymbolValue( arg1 ) == "network" ) 
    {
      ListExpr xType;
      nl->ReadFromString(Network::routesTypeInfo, xType);
      return xType;
    }
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
Value mapping function of operator ~routes~

*/
int OpNetworkRoutes::ValueMapping( Word* args, Word& result, int message, 
                           Word& local, Supplier s )
{
  Network *network = (Network*)args[0].addr;
  result = SetWord( network->GetRoutes() );

  Relation *resultSt = (Relation*)qp->ResultStorage(s).addr;
  resultSt->Close();
  qp->ChangeResultStorage(s, result);

  return 0;
}

/*
Specification of operator ~routes~

*/
const string OpNetworkRoutes::Spec  = 
  "( ( \"Signature\" \"Syntax\" \"Meaning\" "
  "\"Example\" ) "
  "( <text>network -> rel" "</text--->"
  "<text>routes(_)</text--->"
  "<text>Return the routes of a network.</text--->"
  "<text>let r = routes(n)</text--->"
  ") )";
