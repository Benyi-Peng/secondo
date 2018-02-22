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

Started March 2012, Fabio Vald\'{e}s

[TOC]

\section{Overview}
This is the implementation of the Symbolic Trajectory Algebra.

\section{Defines and Includes}

*/

#include "Algorithms.h"
#include "Algebras/Raster2/sint.h"
#include "Algebras/Hash/HashAlgebra.h"
#include "Algebras/NestedRelation/NestedRelationAlgebra.h"

namespace stj {

/*
\section{class ~MaxspeedRasterCreator~}

*/
class MaxspeedRasterCreator {
 public:
  MaxspeedRasterCreator(raster2::sint *_hgt, NestedRelation *_nrel, 
                        RTree2TID *_rtree)
    : hgt(_hgt), nrel(_nrel), rtree(_rtree) {
    grid = hgt->getGrid();
    primary = nrel->getPrimary();
    subrel = nrel->getSubRel("WayInfo");
    sc = SecondoSystem::GetCatalog();
  }

  int getMaxspeedFromRoadInfo(std::string roadInfo1, std::string roadInfo2);
  int getMaxspeedFromLeaf(TupleId leafinfo);
  int getMaxspeed(const raster2::RasterIndex<2> pos);
  
 private:
  raster2::sint *hgt;
  NestedRelation *nrel;
  RTree2TID *rtree;
  raster2::grid2 grid;
  Relation *primary;
  SubRelation *subrel;
  SecondoCatalog *sc;
};
  
enum DirectionNum {DIR_ERROR = -1, EAST, NORTHEAST, NORTH, NORTHWEST, WEST,
                     SOUTHWEST, SOUTH, SOUTHEAST};

/*
\section{class ~RestoreTrajLI~}

Applied for the operator ~restoreTraj~.

*/
struct Tile {
  Tile(const int newX, const int newY) : x(newX), y(newY) {path.clear();}
  
  Tile(const int newX, const int newY, const vector<NewPair<int, int> > newPath)
       : x(newX), y(newY), path(newPath) {
    path.push_back(NewPair<int, int>(x, y));
  }
  
  bool operator<(const Tile& tile) const {
    if (x == tile.x) {
      if (y == tile.y) {
        if (path.size() == tile.path.size()) {
          for (unsigned int i = 0; i < path.size(); i++) {
            if (!(path[i] == tile.path[i])) {
              return path[i] < tile.path[i];
            }
          }
          return false;
        }
        return path.size() < tile.path.size();
      }
      return (y < tile.y);
    }
    return (x < tile.x);
  }
  
  bool operator==(const Tile& tile) const {
    if (x == tile.x && y == tile.y) {
      for (unsigned int i = 0; i < path.size(); i++) {
        if (!(path[i] == tile.path[i])) {
          return false;
        }
      }
      return true;
    }
    return false;
  }
  
  void set(const int newX, const int newY) {
    path.push_back(NewPair<int, int>(this->x, this->y));
    x = newX;
    y = newY;
  }
  
  Tile moveTo(const DirectionNum dir) {
    Tile result(this->x, this->y, path);
    switch (dir) {
      case EAST: {
        result.x++;
        break;
      }
      case NORTHEAST: {
        result.x++;
        result.y--;
        break;
      }
      case NORTH: {
        result.y--;
        break;
      }
      case NORTHWEST: {
        result.x--;
        result.y--;
        break;
      }
      case WEST: {
        result.x--;
        break;
      }
      case SOUTHWEST: {
        result.x--;
        result.y++;
        break;
      }
      case SOUTH: {
        result.y++;
        break;
      }
      case SOUTHEAST: {
        result.x++;
        result.y++;
        break;
      }
      default: {
        break;
      }
    }
    return result;
  }
  
  int x, y;
  std::vector<NewPair<int, int> > path;
};

class RestoreTrajLI {
 public:
  RestoreTrajLI(Relation *e, BTree *ht, RTree2TID *st, raster2::sint *r,
                Hash *rh, MLabel *h, MLabel *d, MLabel *s);
  
  RestoreTrajLI() {}
  
  bool retrieveSequel(const int startPos, std::set<Tile>& tiles);
  void processNeighbors(Tile origin, const Instant& inst,
                        const int height, std::set<Tile>& result);
  std::vector<Tile> getNeighbors(Tile origin, const DirectionNum dirNum);
  void retrieveTilesFromHeight(const int pos, std::set<Tile>& result);
  void updateCoords(const DirectionNum dir, int& x, int& y);
  DirectionNum dirLabelToNum(const Label& dirLabel);
  MLabel* nextCandidate();
  
 private:
  Relation *edgesRel;
  BTree *heightBtree;
  RTree2TID *segmentsRtree;
  raster2::sint *raster;
  Hash *rhash;
  MLabel *height;
  MLabel *direction;
  MLabel *speed;
  
//   std::vector<std::vector<NewPair<int> > > tileSequences;
};

}
