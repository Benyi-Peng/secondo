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
 
 01590 Fachpraktikum "Erweiterbare Datenbanksysteme" 
 WS 2014 / 2015

<our names here>

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[TOC] [\tableofcontents]
 
[1] Implementation of a Spatial3D algebra

[TOC]

1 Includes and Defines

*/

#ifndef _SPATIAL3DGEOMETRIC_ALGORITHM__INTERSECTION_LINE_PLANE_H
#define _SPATIAL3DGEOMETRIC_ALGORITHM_H

namespace spatial3d_geometric
{
  class IntersectionPointResult {
   
  public:

    bool segmentIntersects();
    bool rayIntersects();
    bool lineIntersects();
    
    bool hasIntersectionPoint();
    
    // must not be called if hasIntersectionPoint() returns false
    double getIntersectionParameter();
    // must not be called if hasIntersectionPoint() returns false
    SimplePoint3d getIntersectionPoint();
    // must not be called if hasIntersectionPoint() returns false
    bool isIntersectionOnTriangleEdge();

    enum TriangleLineIntersection { NONE, EDGE, INNER, ON_PLANE };

    IntersectionPointResult(TriangleLineIntersection _resultType,
                            double _intersectionParameter,
                            const SimplePoint3d& _intersectionPoint);

    TriangleLineIntersection resultType;
    double intersectionParameter;

  private:

    
    SimplePoint3d intersectionPoint;    

    friend IntersectionPointResult intersection(const SimplePoint3d& p0,
                                        const Vector3d& segmentVector,
                                        const Triangle& triangle);
    
    friend IntersectionPointResult intersection(const SimplePoint3d& p0,
                                                const SimplePoint3d& p1,
                                                const Plane3d& plane);
  
    friend IntersectionPointResult intersection(const SimplePoint3d& p0,
                                                const Vector3d& segmentVector,
                                                const Plane3d& plane);
  
    friend IntersectionPointResult intersection(const SimplePoint3d& p0,
                                                const SimplePoint3d& p1,
                                                const Triangle& triangle);
 
  };
  
  IntersectionPointResult intersection(const SimplePoint3d& p0,
                                       const SimplePoint3d& p1,
                                       const Plane3d& plane);
  
  IntersectionPointResult intersection(const SimplePoint3d& p0,
                                       const Vector3d& segmentVector,
                                       const Plane3d& plane);
  
  IntersectionPointResult intersection(const SimplePoint3d& p0,
                                       const SimplePoint3d& p1,
                                       const Triangle& triangle);

  IntersectionPointResult intersection(const SimplePoint3d& p0,
                                       const Vector3d& segmentVector,
                                       const Triangle& triangle);
  
}

#endif
