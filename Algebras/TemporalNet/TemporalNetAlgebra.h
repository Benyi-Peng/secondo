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
//[TOC] [\tableofcontents]

1 Declarations needed by Algebra TemporalNet

Mai-Oktober 2007 Martin Scheppokat

February 2008 - June 2008 Simone Jandt

1.2 Defines, includes, and constants

*/

#ifndef _TEMPORAL_NET_ALGEBRA_H_
#define _TEMPORAL_NET_ALGEBRA_H_

#include <iostream>
#include <sstream>
#include <string>
#include "NestedList.h"
#include "QueryProcessor.h"

#ifndef __NETWORK_ALGEBRA_H__
#error NetworkAlgebra.h is needed by TemporalNetAlgebra.h. \
Please include in *.cpp-File.
#endif

#ifndef _TEMPORAL_ALGEBRA_H_
#error TemporalAlgebra.h is needed by MGPoint.h. \
Please include in *.cpp-File.
#endif



using namespace datetime;

/*
Instant

This class represents a time instant, or a point in time. It will be
used in the ~instant~ type constructor.

*/
typedef DateTime Instant;

/*
1.3 UGPoint

This class will be used in the ~ugpoint~ type constructor, i.e., the type
constructor for the temporal unit of gpoint values.

*/
class UGPoint : public SpatialTemporalUnit<GPoint, 3>
{
  public:
  UGPoint() {};

  UGPoint(bool is_defined):
    SpatialTemporalUnit<GPoint, 3>(is_defined) {};

  UGPoint( const Interval<Instant>& interval,
           const int in_NetworkID,
           const int in_RouteID,
           const Side in_Side,
           const double in_Position0,
           const double in_Position1 ):
    SpatialTemporalUnit<GPoint, 3>( interval ),
    p0( true,        // defined
        in_NetworkID,    // NetworkID
        in_RouteID,      // RouteID
        in_Position0,    // d
        in_Side),      // Side
    p1( true,        // defined
        in_NetworkID,    // NetworkID
        in_RouteID,      // RouteID
        in_Position1,    // d
        in_Side)       // Side
    {}

  UGPoint( const Interval<Instant>& interval,
           const GPoint& p0,
           const GPoint& p1 ):
    SpatialTemporalUnit<GPoint, 3>( interval ),
    p0( p0 ),
    p1( p1 )
    {}

/*
Operator redefinitions

Returns ~true~ if this temporal unit is equal to the temporal unit ~i~ and ~false~ if they are different.

*/

  virtual UGPoint& operator=( const UGPoint& i )
  {
    *((TemporalUnit<GPoint>*)this) = *((TemporalUnit<GPoint>*)&i);
    p0 = i.p0;
    p1 = i.p1;

    return *this;
  }

/*
Returns ~true~ if this temporal unit is different to the temporal unit ~i~ and ~false~ if they are equal.

*/

    virtual bool operator!=( const UGPoint& i ) const
  {
    return !( *this == i );
  }

/*
Functions to be part of relations

*/

  inline virtual size_t Sizeof() const
  {
    return sizeof( *this );
  }

  inline virtual UGPoint* Clone() const
  {
    UGPoint *res;
    res = new UGPoint( );
    res->defined = TemporalUnit<GPoint>::defined;
    return res;
  }

  inline virtual void CopyFrom( const StandardAttribute* right )
  {
    const UGPoint* i = (const UGPoint*)right;

    TemporalUnit<GPoint>::defined = i->defined;
    if(i->defined)
      {
        timeInterval.CopyFrom( i->timeInterval );
        p0 = i->p0;
        p1 = i->p1;
      }
    else
      {
        timeInterval = Interval<Instant>();
        p0 = GPoint( false, 0, 0, 0.0, None);
        p1 = GPoint( false, 0, 0, 0.0, None);
      }
  }

  virtual const Rectangle<3> BoundingBox() const
  {
    if (this->IsDefined())
      return Rectangle<3> (true,
                         (double) p0.GetRouteId(),
                         (double) p0.GetRouteId(),
                         min(p0.GetPosition(),p1.GetPosition()),
                         max(p0.GetPosition(),p1.GetPosition()),
                         timeInterval.start.ToDouble(),
                         timeInterval.end.ToDouble());
    else
      return Rectangle<3>();
  }

  inline const Rectangle<2> BoundingBox2d() const
  {
    if (this->IsDefined())
      return Rectangle<2> (true,
                           (double) p0.GetRouteId(),
                           (double) p0.GetRouteId(),
                          min(p0.GetPosition(), p1.GetPosition()),
                          max(p0.GetPosition(), p1.GetPosition()));
    else
      return Rectangle<2>();
  }

  virtual void TemporalFunction( const Instant& t,
                                 GPoint& result,
                                 bool ignoreLimits = false ) const;
  virtual bool Passes( const GPoint& val ) const;
  virtual bool At( const GPoint& val, TemporalUnit<GPoint>& result ) const;

    static ListExpr Property();

    static bool Check(ListExpr type,
                      ListExpr& errorInfo );

    static ListExpr Out(ListExpr typeInfo,
                        Word value );

    static Word In(const ListExpr typeInfo,
                   const ListExpr instance,
                   const int errorPos,
                   ListExpr& errorInfo,
                   bool& correct );

    static Word Create(const ListExpr typeInfo );

    static void Delete(const ListExpr typeInfo,
                       Word& w );

    static void Close(const ListExpr typeInfo,
                      Word& w );

    static Word Clone(const ListExpr typeInfo,
                      const Word& w );

    static int SizeOf();

    static void* Cast(void* addr);

    int GetUnitRid();

    double GetUnitStartPos();

    double GetUnitEndPos();

    double GetUnitStartTime();

    double GetUnitEndTime();

    GPoint p0, p1;
};

/*
1.4 MGPoint

*/

class MGPoint : public Mapping< UGPoint, GPoint >
{
  public:
/*
The simple constructor should not be used.

*/
    MGPoint();

    MGPoint( const int n );

    static ListExpr Property();

    static bool Check(ListExpr type,
                      ListExpr& errorInfo);

};


#endif // _TEMPORAL_NET_ALGEBRA_H_
