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

[2] Implementation with exakt dataype, 

Oktober 2014 - Maerz 2015, S. Schroeer for master thesis.

[TOC]

[2] Implementation with exakt dataype, 

April - November 2014, S. Schroer for master thesis.

1 Introduction

2 Defines and Includes

*/

#include "Point2D.h"
#include "Point3D.h"
#include "Point3DExt.h"
#include "Vector3D.h"
#include "Vector2D.h"
#include "Angle.h"

namespace temporalalgebra {
namespace mregionops2 {

ostream& operator <<(ostream& o, Point2D& p) {

    o << "(" << p.GetX() << ", " << p.GetY() << ")";

    return o;
}

ostream& operator <<(ostream& o, Point3D& p) {

    o << "(" << p.GetX() << ", " << p.GetY() << ", " << p.GetZ() << ")";

    return o;
}

ostream& operator <<(ostream& o, Point3DExt& p) {

    o << "(" << p.GetX() << ", " << p.GetY() << ", " << p.GetZ() << ")";

    return o;
}

ostream& operator <<(ostream& o, Vector3D& v) {

    o << "(" << v.GetX() << ", " << v.GetY() << ", " << v.GetZ() << ")";

    return o;
}

ostream& operator <<(ostream& o, Vector2D& v) {

    o << "(" << v.GetX() << ", " << v.GetY() << ")";
    return o;
}

ostream& operator <<(ostream& o, Angle& a) {

    if (a.IsInfinite())
      o  << "INFINITE";      
    else
      o  << a.GetAngle();

    return o;
}

void pmq(mpq_class val) {

    cout << val << endl;
}

}
}
