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

*/

#ifndef SECONDO_SPATIALJOINMLOCALINFO_H
#define SECONDO_SPATIALJOINMLOCALINFO_H

#include "Algebra.h"
#include "NestedList.h"
#include "NList.h"
#include "QueryProcessor.h"
#include "AlgebraManager.h"
#include "StandardTypes.h"
#include "Symbols.h"
#include "Algebras/Stream/Stream.h"
#include "ListUtils.h"
#include "Algebras/Relation-C++/RelationAlgebra.h"
#include <string>

#include <iostream>
#include <typeinfo>
//#include <deque>
#include "../RTreeTouch.h"
#include "../STR.h"

#include "Algebras/MainMemory2/MainMemoryExt.h"

extern NestedList* nl;
extern QueryProcessor *qp;

class SpatialJoinMLocalInfo
{
private:
    TupleType* tt;
    std::vector<Tuple*> vLeft;
    std::vector<Tuple*> vRight;

    mmrtreetouch::RTreeTouch * rtree;

    bool turnOn;

public:

    SpatialJoinMLocalInfo(
            std::vector<Tuple*> vLeftIn,
            std::vector<Tuple*> vRightIn,
            int vLeftIndexIn,
            int vRightIndexIn,
            TupleType* ttIn,
            int fanoutIn,
            int numOfItemsInBucket,
            int cellFactor
    );

    ~SpatialJoinMLocalInfo();

    mm2algebra::MemoryRelObject* getMatchings(std::vector<Tuple*> vRightIn);

};


#endif //SECONDO_SPATIALJOINMLOCALINFO_H