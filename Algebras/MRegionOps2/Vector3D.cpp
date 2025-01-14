/*
----
This file is part of SECONDO.

Copyright (C) 2008, University in Hagen, Department of Computer Science,
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
//[ue] [\"u]
//[ae] [\"a]
//[oe] [\"o]
//[x] [$\times $]
//[->] [$\rightarrow $]

[1] Implementation 

April - November 2008, M. H[oe]ger for bachelor thesis.

[2] Implementation with exakt dataype

Oktober 2014 - Maerz 2015, S. Schroeer for master thesis.

[TOC]

1 Introduction

2 Defines and Includes

*/

#include "Vector3D.h"
#include "Point3D.h"


namespace temporalalgebra {
namespace mregionops2 {

// calculate the crossprodukt from two vectors
Vector3D Vector3D::CrossProduct(Vector3D vec)
{
  Vector3D n(y * vec.GetZ() - z * vec.GetY(),
             z * vec.GetX() - x * vec.GetZ(),
             x * vec.GetY() - y * vec.GetX());
  return n;
}

Vector3D::Vector3D(Point3D p1, Point3D p2)
{
  x = p2.GetX() - p1.GetX();
  y = p2.GetY() - p1.GetY();
  z = p2.GetZ() - p1.GetZ();
}


}
}
