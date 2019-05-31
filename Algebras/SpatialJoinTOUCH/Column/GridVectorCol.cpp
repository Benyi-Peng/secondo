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

#include "GridVectorCol.h"
#include <algorithm>
#include "Algebras/RTree/RTreeAlgebra.h"
#include <memory>
#include "BinaryTuple.h"

using namespace mmrtreetouch;
using namespace std;

typedef long long int ullong;

GridVectorCol::GridVectorCol(
        nodeCol* node,
        double _xCellDim,
        double _yCellDim,
        int64_t _remainingMem
):
    box(node->box),
    minX((double) box.MinD(0)),
    maxX((double) box.MaxD(0)),
    minY((double) box.MinD(1)),
    maxY((double) box.MaxD(1)),
    //xCellDim(_xCellDim < 0.0001 ? 0.1: _xCellDim),
    xCellDim(_xCellDim == 0 ? 0.1: _xCellDim),
    //yCellDim(_yCellDim < 0.0001 ? 0.1: _yCellDim),
    yCellDim(_yCellDim == 0 ? 0.1: _yCellDim),
    xLength((maxX - minX) < 1 ? 1 : (maxX - minX)),
    yLength((maxY - minY) < 1 ? 1 : (maxY - minY)),
    numOfXCells((ullong) ceil(xLength / xCellDim)),
    numOfYCells((ullong) ceil(yLength / yCellDim)),
    gridVectorCol (
            numOfXCells,
            vector<vector<binaryTuple> >(numOfYCells, vector<binaryTuple >(0))),
    remainingMem(_remainingMem)
{
    xCellDim = xLength / numOfXCells;
    yCellDim = yLength / numOfYCells;

 }

GridVectorCol::~GridVectorCol() {};

int64_t GridVectorCol::calculateIndexX(double coord) {

    int64_t index;
    int64_t roundDown = (int64_t) floor((coord-minX) / xCellDim);

    index = roundDown;

    if (roundDown == numOfXCells) {
        index = roundDown - 1;
    }

    if (index < 0) {
        index = 0;
    } else if (index > numOfXCells) {
        index = numOfXCells - 1;
    }

    assert(index >= 0);
    assert(index < numOfXCells);

    return index;
}

int64_t GridVectorCol::calculateIndexY(double coord) {

    int64_t index;
    int64_t roundDown = (int64_t) floor((coord-minY) / yCellDim);

    index = roundDown;

    if (roundDown == numOfYCells) {
        index = roundDown - 1;
    }

    if (index < 0) {
        index = 0;
    } else if (index > numOfYCells) {
        index = numOfYCells - 1;
    }

    assert(index >= 0);
    assert(index < numOfYCells);

    return index;
}

pair<pair<int64_t, int64_t>, pair<int64_t, int64_t>>
GridVectorCol::getGridCoordinatesOf(
        binaryTuple bt
) {

    int tMinX = calculateIndexX(bt.xMin);
    int tMaxX = calculateIndexX(bt.xMax);

    int tMinY = calculateIndexY(bt.yMin);
    int tMaxY = calculateIndexY(bt.yMax);

    return make_pair(make_pair(tMinX, tMaxX), make_pair(tMinY, tMaxY));
}

vector<pair<binaryTuple, binaryTuple>> GridVectorCol::getMatchings() {
    return matchings;
}

void GridVectorCol::setMatchings(
        vector<pair<binaryTuple, binaryTuple>> _matchings
        ) {
    matchings = _matchings;
}

void GridVectorCol::getTuplesOverlappingWith(
        binaryTuple tbB
) {

    pair<pair<int, int>, pair<int, int>> indexes = getGridCoordinatesOf(
            tbB
    );

    pair<int, int> xPair = indexes.first;
    pair<int, int> yPair = indexes.second;

    for (int i = xPair.first; i <= xPair.second; i++) {
        for (int j = yPair.first; j <= yPair.second; j++) {

            vector<binaryTuple> temp = gridVectorCol[i][j];

            for (binaryTuple tbA: temp) {

                if (tuplesIntersectInCell(tbA, tbB, i, j)) {

                    pair<binaryTuple, binaryTuple> res  = make_pair(tbA, tbB);

                    matchings.push_back(res);
                }

                if (remainingMem <= 0) {
                    cout << "in GridVector" << endl;
                    return;
                }
            }

        }
    }
}

bool GridVectorCol::tuplesIntersectInCell(
        binaryTuple btA, binaryTuple btB, int64_t i, int64_t j) {

    double min[2];
    double max[2];
    min[0] = btA.xMin;
    min[1] = btA.yMin;
    max[0] = btA.xMax;
    max[1] = btA.yMax;

    if (remainingMem - 2*sizeof(Rectangle<2>) <= 0 ) {
        cout << "Memory is not enough 5" << endl;
        cout << "remainingMem: " << remainingMem << endl;
        cout << "Rectangle: " << sizeof(Rectangle<2>) << endl;
        remainingMem -= 2*sizeof(Rectangle<2>);
        return false;
    }

    Rectangle<2>* boxA = new Rectangle<2>(true, min, max);

    min[0] = btB.xMin;
    min[1] = btB.yMin;
    max[0] = btB.xMax;
    max[1] = btB.yMax;

    Rectangle<2>* boxB = new Rectangle<2>(true, min, max);

    remainingMem -= 2*sizeof(Rectangle<2>);

    // if the lower left edge of the intersection of the two boxes
    // is in the same cell then true, else false

    Rectangle<2> intersectionBox = boxA->Intersection(*boxB);

    bool boxesIntersect = boxA->Intersects(*boxB, 0);

    delete boxA;
    delete boxB;

    double minX = (double) intersectionBox.MinD(0);
    double minY = (double) intersectionBox.MinD(1);

    int64_t intersectionIndexX = calculateIndexX(minX);
    int64_t intersectionIndexY = calculateIndexY(minY);

    if (intersectionIndexX == i && intersectionIndexY == j) {
        if (boxesIntersect) {
            return true;
        }
    }

    return false;
}

void GridVectorCol::addTuple(binaryTuple bt) {

    int tMinX = calculateIndexX(bt.xMin);
    int tMaxX = calculateIndexX(bt.xMax);

    int tMinY = calculateIndexY(bt.yMin);
    int tMaxY = calculateIndexY(bt.yMax);

    for (int i = tMinX; i <= tMaxX; i++) {
        for (int j = tMinY; j <= tMaxY; j++) {
            gridVectorCol[i][j].push_back(bt);
        }
    }
}

