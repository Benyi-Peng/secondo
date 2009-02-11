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
//[_] [\_]

[1] Implementation of the Spatial Algebra

February, 2003. Victor Teixeira de Almeida

March-July, 2003. Zhiming Ding

January, 2005 Leonardo Guerreiro Azevedo

[TOC]

1 Overview

This implementation file essentially contains the implementation of the classes ~Point~,
~Points~, ~Line~, and ~Region~ used in the Spatial Algebra. These classes
respectively correspond to the memory representation for the type constructors
~point~, ~points~, ~line~, and ~region~.

For more detailed information see SpatialAlgebra.h.

2 Defines and Includes

*/


//#define __TRACE__ cout <<  __FILE__ << "::" << __LINE__;
// don't print out traces
#define __TRACE__

using namespace std;

#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include "SpatialAlgebra.h"
#include "SecondoConfig.h"
#include "AvlTree.h"
#include <vector>
#include <queue>
#include <stdexcept>
#include <iostream>
#include <string.h>
#include <string>
#include <cmath>
#include <algorithm>
#include <ctime>
#include <queue>
#include <iterator>
#include <sstream>
#include <limits>

#ifndef M_PI
const double M_PI = acos( -1.0 );
#endif

extern NestedList* nl;
extern QueryProcessor* qp;

/*
3 Type investigation auxiliaries

Within this algebra module, we have to handle with values of four different
types: ~point~ and ~points~, ~line~ and ~region~.

Later on we will
examine nested list type descriptions. In particular, we
are going to check whether they describe one of the four types just introduced.
In order to simplify dealing with list expressions describing these types, we
declare an enumeration, ~SpatialType~, containing the four types, and a function,
~SpatialTypeOfSymbol~, taking a nested list as argument and returning the
corresponding ~SpatialType~ type name.

*/
enum SpatialType { stpoint, stpoints, stline, stregion,
                   stbox, sterror, stsline };

SpatialType
SpatialTypeOfSymbol( ListExpr symbol )
{
  if ( nl->AtomType( symbol ) == SymbolType )
  {
    string s = nl->SymbolValue( symbol );
    if ( s == "point"  ) return (stpoint);
    if ( s == "points" ) return (stpoints);
    if ( s == "line"   ) return (stline);
    if ( s == "region" ) return (stregion);
    if ( s == "rect"   ) return (stbox);
    if ( s == "sline"  ) return (stsline);
  }
  return (sterror);
}

/*
9 Object Traversal functions

These functions are utilities useful for traversing objects.  They are basic functions
to be called by the operations defined below.

There are 6 combinations, pp, pl, pr, ll, lr, rr

*/

enum object {none, first, second, both};
enum status {endnone, endfirst, endsecond, endboth};

void SelectFirst_pp( const Points& P1, const Points& P2,
                     object& obj, status& stat )
{
  P1.SelectFirst();
  P2.SelectFirst();

  const Point *p1, *p2;
  bool gotP1 = P1.GetPt( p1 ),
       gotP2 = P2.GetPt( p2 );

  if( !gotP1 && !gotP2 )
  {
    obj = none;
    stat = endboth;
  }
  else if( !gotP1 )
  {
    obj = second;
    stat = endfirst;
  }
  else if( !gotP2 )
  {
    obj = first;
    stat = endsecond;
  }
  else //both defined
  {
    stat = endnone;
    if( *p1 < *p2 )
      obj = first;
    else if( *p1 > *p2 )
      obj = second;
    else
      obj = both;
  }
}

void SelectNext_pp( const Points& P1, const Points& P2,
                    object& obj, status& stat )
{
  // 1. get the current elements
  const Point *p1, *p2;
  bool gotP1 = P1.GetPt( p1 ),
       gotP2 = P2.GetPt( p2 );

  //2. move the pointers
  if( !gotP1 && !gotP2 )
  {
    //do nothing
  }
  else if( !gotP1 )
  {
    P2.SelectNext();
    gotP2 = P2.GetPt( p2 );
  }
  else if( !gotP2 )
  {
    P1.SelectNext();
    gotP1 = P1.GetPt( p1 );
  }
  else //both currently defined
  {
    if( *p1 < *p2 ) //then hs1 is the last output
    {
      P1.SelectNext();
      gotP1 = P1.GetPt( p1 );
    }
    else if( *p1 > *p2 )
    {
      P2.SelectNext();
      gotP2 = P2.GetPt( p2 );
    }
    else
    {
      P1.SelectNext();
      gotP1 = P1.GetPt( p1 );
      P2.SelectNext();
      gotP2 = P2.GetPt( p2 );
    }
  }

  //3. generate the outputs
  if( !gotP1 && !gotP2 )
  {
    obj = none;
    stat = endboth;
  }
  else if( !gotP1 )
  {
    obj = second;
    stat = endfirst;
  }
  else if( !gotP2 )
  {
    obj = first;
    stat = endsecond;
  }
  else //both defined
  {
    stat = endnone;
    if( *p1 < *p2 )
      obj = first;
    else if( *p1 > *p2 )
      obj = second;
    else
      obj = both;
  }
}

void SelectFirst_pl( const Points& P, const Line& L,
                     object& obj, status& stat )
{
  P.SelectFirst();
  L.SelectFirst();

  const Point *p1;
  Point p2;
  const HalfSegment *hs;

  bool gotPt = P.GetPt( p1 ),
       gotHs = L.GetHs( hs );

  if( gotHs )
    p2 = hs->GetDomPoint();

  if( !gotPt && !gotHs )
  {
    obj = none;
    stat = endboth;
  }
  else if( !gotPt )
  {
    obj = second;
    stat = endfirst;
  }
  else if( !gotHs )
  {
    obj = first;
    stat = endsecond;
  }
  else //both defined
  {
    stat = endnone;
    if( *p1 < p2 )
      obj = first;
    else if( *p1 > p2 )
      obj = second;
    else
      obj=both;
  }
}

void SelectNext_pl( const Points& P, const Line& L,
                    object& obj, status& stat )
{
  // 1. get the current elements
  const Point *p1;
  Point p2;
  const HalfSegment *hs;

  bool gotPt = P.GetPt( p1 ),
       gotHs = L.GetHs( hs );

  if( gotHs )
    p2 = hs->GetDomPoint();

  //2. move the pointers
  if( !gotPt && !gotHs )
  {
    //do nothing
  }
  else if( !gotPt )
  {
    L.SelectNext();
    gotHs = L.GetHs( hs );
    if( gotHs )
      p2 = hs->GetDomPoint();
  }
  else if( !gotHs )
  {
    P.SelectNext();
    gotPt = P.GetPt( p1 );
  }
  else //both currently defined
  {
    if( *p1 < p2 ) //then hs1 is the last output
    {
      P.SelectNext();
      gotPt = P.GetPt( p1 );
    }
    else if( *p1 > p2 )
    {
      L.SelectNext();
      gotHs = L.GetHs( hs );
      if( gotHs )
        p2 = hs->GetDomPoint();
    }
    else
    {
      P.SelectNext();
      gotPt = P.GetPt( p1 );
      L.SelectNext();
      gotHs = L.GetHs( hs );
      if( gotHs )
        p2 = hs->GetDomPoint();
    }
  }

  //3. generate the outputs
  if( !gotPt && !gotHs )
  {
    obj = none;
    stat = endboth;
  }
  else if( !gotPt )
  {
    obj = second;
    stat = endfirst;
  }
  else if( !gotHs )
  {
    obj = first;
    stat = endsecond;
  }
  else //both defined
  {
    stat=endnone;
    if( *p1 < p2 )
      obj = first;
    else if( *p1 > p2 )
      obj = second;
    else
      obj = both;
  }
}

void SelectFirst_pr( const Points& P, const Region& R,
                     object& obj, status& stat )
{
  P.SelectFirst();
  R.SelectFirst();

  const Point *p1;
  Point p2;
  const HalfSegment *hs;

  bool gotPt = P.GetPt( p1 ),
       gotHs = R.GetHs( hs );
  if( gotHs )
    p2 = hs->GetDomPoint();

  if( !gotPt && !gotHs )
  {
    obj = none;
    stat = endboth;
  }
  else if( !gotPt )
  {
    obj = second;
    stat = endfirst;
  }
  else if( !gotHs )
  {
    obj = first;
    stat = endsecond;
  }
  else //both defined
  {
    stat = endnone;
    if( *p1 < p2 )
      obj = first;
    else if( *p1 > p2 )
      obj = second;
    else
      obj=both;
  }
}

void SelectNext_pr( const Points& P, const Region& R,
                    object& obj, status& stat )
{
  // 1. get the current elements
  const Point *p1;
  Point p2;
  const HalfSegment *hs;

  bool gotPt = P.GetPt( p1 ),
       gotHs = R.GetHs( hs );
  if( gotHs )
    p2 = hs->GetDomPoint();

  //2. move the pointers
  if( !gotPt && !gotHs )
  {
    //do nothing
  }
  else if( !gotPt )
  {
    R.SelectNext();
    gotHs = R.GetHs( hs );
    if( gotHs )
      p2 = hs->GetDomPoint();
  }
  else if( !gotHs )
  {
    P.SelectNext();
    gotPt = P.GetPt( p1 );
  }
  else //both currently defined
  {
    if( *p1 < p2 ) //then hs1 is the last output
    {
      P.SelectNext();
      gotPt = P.GetPt( p1 );
    }
    else if( *p1 > p2 )
    {
      R.SelectNext();
      gotHs = R.GetHs( hs );
      if( gotHs )
        p2 = hs->GetDomPoint();
    }
    else
    {
      P.SelectNext();
      gotPt = P.GetPt( p1 );
      R.SelectNext();
      gotHs = R.GetHs( hs );
      if( gotHs )
        p2 = hs->GetDomPoint();
    }
  }

  //3. generate the outputs
  if( !gotPt && !gotHs )
  {
    obj = none;
    stat = endboth;
  }
  else if( !gotPt )
  {
    obj = second;
    stat = endfirst;
  }
  else if( !gotHs )
  {
    obj = first;
    stat = endsecond;
  }
  else //both defined
  {
    stat = endnone;
    if( *p1 < p2 )
      obj = first;
    else if( *p1 > p2 )
      obj = second;
    else
      obj = both;
  }
}

void SelectFirst_ll( const Line& L1, const Line& L2,
                     object& obj, status& stat )
{
  L1.SelectFirst();
  L2.SelectFirst();

  const HalfSegment *hs1, *hs2;
  bool gotHs1 = L1.GetHs( hs1 ),
       gotHs2 = L2.GetHs( hs2 );

  if( !gotHs1 && !gotHs2 )
  {
    obj = none;
    stat = endboth;
  }
  else if( !gotHs1 )
  {
    obj = second;
    stat = endfirst;
  }
  else if( !gotHs2 )
  {
    obj = first;
    stat = endsecond;
  }
  else //both defined
  {
    stat = endnone;
    if( *hs1 < *hs2 )
      obj = first;
    else if( *hs1 > *hs2 )
      obj = second;
    else obj = both;
  }
}

void SelectNext_ll( const Line& L1, const Line& L2,
                    object& obj, status& stat )
{
  // 1. get the current elements
  const HalfSegment *hs1, *hs2;
  bool gotHs1 = L1.GetHs( hs1 ),
       gotHs2 = L2.GetHs( hs2 );

  //2. move the pointers
  if( !gotHs1 && !gotHs2 )
  {
    //do nothing
  }
  else if( !gotHs1 )
  {
    L2.SelectNext();
    gotHs2 = L2.GetHs( hs2 );
  }
  else if( !gotHs2 )
  {
    L1.SelectNext();
    gotHs1 = L1.GetHs( hs1 );
  }
  else //both currently defined
  {
    if( *hs1 < *hs2 ) //then hs1 is the last output
    {
      L1.SelectNext();
      gotHs1 = L1.GetHs( hs1 );
    }
    else if( *hs1 > *hs2 )
    {
      L2.SelectNext();
      gotHs2 = L2.GetHs( hs2 );
    }
    else
    {
      L1.SelectNext();
      gotHs1 = L1.GetHs( hs1 );
      L2.SelectNext();
      gotHs2 = L2.GetHs( hs2 );
    }
  }

  //3. generate the outputs
  if( !gotHs1 && !gotHs2 )
  {
    obj = none;
    stat = endboth;
  }
  else if( !gotHs1 )
  {
    obj = second;
    stat = endfirst;
  }
  else if( !gotHs2 )
  {
    obj = first;
    stat = endsecond;
  }
  else //both defined
  {
    stat = endnone;
    if( *hs1 < *hs2 )
      obj = first;
    else if( *hs1 > *hs2 )
      obj = second;
    else
      obj = both;
  }
}

void SelectFirst_lr( const Line& L, const Region& R,
                     object& obj, status& stat )
{
  L.SelectFirst();
  R.SelectFirst();

  const HalfSegment *hs1, *hs2;
  bool gotHs1 = L.GetHs( hs1 ),
       gotHs2 = R.GetHs( hs2 );

  if( !gotHs1 && !gotHs2 )
  {
    obj = none;
    stat = endboth;
  }
  else if( !gotHs1 )
  {
    obj = second;
    stat = endfirst;
  }
  else if( !gotHs2 )
  {
    obj = first;
    stat = endsecond;
  }
  else //both defined
  {
    stat = endnone;
    if( *hs1 < *hs2 )
      obj = first;
    else if( *hs1 > *hs2 )
      obj = second;
    else
      obj = both;
  }
}

void SelectNext_lr( const Line& L, const Region& R,
                    object& obj, status& stat )
{
  // 1. get the current elements
  const HalfSegment *hs1, *hs2;
  bool gotHs1 = L.GetHs( hs1 ),
       gotHs2 = R.GetHs( hs2 );

  //2. move the pointers
  if( !gotHs1 && !gotHs2 )
  {
    //do nothing
  }
  else if( !gotHs1 )
  {
    R.SelectNext();
    gotHs2 = R.GetHs( hs2 );
  }
  else if( !gotHs2 )
  {
    L.SelectNext();
    gotHs1 = L.GetHs( hs1 );
  }
  else //both currently defined
  {
    if( *hs1 < *hs2 ) //then hs1 is the last output
    {
      L.SelectNext();
      gotHs1 = L.GetHs( hs1 );
    }
    else if( *hs1 > *hs2 )
    {
      R.SelectNext();
      gotHs2 = R.GetHs( hs2 );
    }
    else
    {
      L.SelectNext();
      gotHs1 = L.GetHs( hs1 );
      R.SelectNext();
      gotHs2 = R.GetHs( hs2 );
    }
  }

  //3. generate the outputs
  if( !gotHs1 && gotHs2 )
  {
    obj = none;
    stat = endboth;
  }
  else if( !gotHs1 )
  {
    obj = second;
    stat = endfirst;
  }
  else if( !gotHs2 )
  {
    obj = first;
    stat = endsecond;
  }
  else //both defined
  {
    stat = endnone;
    if( *hs1 < *hs2 )
      obj = first;
    else if( *hs1 > *hs2 )
      obj = second;
    else
      obj=both;
  }
}

void SelectFirst_rr( const Region& R1, const Region& R2,
                     object& obj, status& stat )
{
  R1.SelectFirst();
  R2.SelectFirst();

  const HalfSegment *hs1, *hs2;
  bool gotHs1 = R1.GetHs( hs1 ),
       gotHs2 = R2.GetHs( hs2 );

  if( !gotHs1 && !gotHs2 )
  {
    obj = none;
    stat = endboth;
  }
  else if( !gotHs1 )
  {
    obj = second;
    stat = endfirst;
  }
  else if( !gotHs2 )
  {
    obj = first;
    stat = endsecond;
  }
  else //both defined
  {
    stat = endnone;
    if( *hs1 < *hs2 )
      obj = first;
    else if( *hs1 > *hs2 )
      obj = second;
    else
      obj=both;
  }
}

void SelectNext_rr( const Region& R1, const Region& R2,
                    object& obj, status& stat )
{
  // 1. get the current elements
  const HalfSegment *hs1, *hs2;
  bool gotHs1 = R1.GetHs( hs1 ),
       gotHs2 = R2.GetHs( hs2 );

  //2. move the pointers
  if( !gotHs1 && !gotHs2 )
  {
    //do nothing
  }
  else if( !gotHs1 )
  {
    R2.SelectNext();
    gotHs2 = R2.GetHs( hs2 );
  }
  else if( !gotHs2 )
  {
    R1.SelectNext();
    gotHs1 = R1.GetHs( hs1 );
  }
  else //both currently defined
  {
    if( *hs1 < *hs2 ) //then hs1 is the last output
    {
      R1.SelectNext();
      gotHs1 = R1.GetHs( hs1 );
    }
    else if( *hs1 > *hs2 )
    {
      R2.SelectNext();
      gotHs2 = R2.GetHs( hs2 );
    }
    else
    {
      R1.SelectNext();
      gotHs1 = R1.GetHs( hs1 );
      R2.SelectNext();
      gotHs2 = R2.GetHs( hs2 );
    }
  }

  //3. generate the outputs
  if( !gotHs1 && !gotHs2 )
  {
    obj = none;
    stat = endboth;
  }
  else if( !gotHs1 )
  {
    obj = second;
    stat = endfirst;
  }
  else if( !gotHs2 )
  {
    obj = first;
    stat = endsecond;
  }
  else //both defined
  {
    stat = endnone;
    if( *hs1 < *hs2 )
      obj = first;
    else if( *hs1 > *hs2 )
      obj = second;
    else
      obj = both;
  }
}


/*
3 Auxiliary classes

*/

// Generic print-function object for printing with STL::for_each
template<class T> struct print : public unary_function<T, void>
{
  print(ostream& out) : os(out) {}
  void operator() (T x) { os << "\t" << x << "\n"; }
  ostream& os;
};


/*
4 Type Constructor ~point~

A value of type ~point~ represents a point in the Euclidean plane or is undefined.

4.1 Implementation of the class ~Point~

*/
ostream& operator<<( ostream& o, const Point& p )
{
  if( p.IsDefined() )
    o << "(" << p.GetX() << ", " << p.GetY() << ")";
  else
    o << "undef";
  return o;
}

ostream& Point::Print( ostream &os ) const
{
  return os << *this;
}

bool Point::Inside( const Rectangle<2>& r ) const
{
  assert( r.IsDefined() );
  if( x < r.MinD(0) || x > r.MaxD(0) )
    return false;
  else if( y < r.MinD(1) || y > r.MaxD(1) )
    return false;
  return true;
}

double Point::Distance( const Point& p ) const
{
  double dx = p.x - x,
         dy = p.y - y;

  return sqrt( pow( dx, 2 ) + pow( dy, 2 ) );
}

double Point::Distance( const Rectangle<2>& r ) const
{
  assert( IsDefined() && r.IsDefined() );
  double rxmin = r.MinD(0), rxmax = r.MaxD(0),
         rymin = r.MinD(1), rymax = r.MaxD(1);
  double dx =
        (   (x > rxmin || AlmostEqual(x,rxmin))
         && (x < rxmax || AlmostEqual(x,rxmax))) ? (0.0) :
        (min(abs(x-rxmin),abs(x-rxmax)));
  double dy =
        (   (y > rymin || AlmostEqual(y,rymin))
         && (y < rymax || AlmostEqual(y,rymax))) ? (0.0) :
        (min(abs(y-rymin),abs(y-rymax)));

  return sqrt( pow( dx, 2 ) + pow( dy, 2 ) );
}

double Point::Direction( const Point& p ) const
{
  assert( !AlmostEqual( *this, p ) );

  Coord x1 = x,
        y1 = y,
        x2 = p.x,
        y2 = p.y;

  if( AlmostEqual( x1, x2 ) )
  {
    if( y2 > y1 )
      return 90.0;
    return 270.0;
  }

  if( AlmostEqual( y1, y2 ) )
  {
    if( x2 > x1 )
      return 0.0;
    return 180.0;
  }

  double k= (y2 - y1) / (x2 - x1),
         direction = atan( k ) * 180 / M_PI;

  if( x2 < x1 && y2 > y1 )
    direction = 180 + direction;
  else if( x2 < x1 && y2 < y1 )
    direction = 180 + direction;
  else if( x2 > x1 && y2 < y1 )
    direction = 360 + direction;

  return direction;
}

void Point::Rotate(const Coord& x, const Coord& y,
                          const double alpha, Point& res) const{

  if(!IsDefined()){
     res.SetDefined(false);
     return;
  }

  double s = sin(alpha);
  double c = cos(alpha);

  double m00 = c;
  double m01 = -s;
  double m02 = x - x*c + y*s;
  double m10 = s;
  double m11 = c;
  double m12 = y - x*s-y*c;

  res.SetDefined(true);
  res.x = m00*this->x + m01*this->y + m02;
  res.y = m10*this->x + m11*this->y + m12;

}


/*
4.2 List Representation

The list representation of a point is

----  (x y)
----

4.3 ~Out~-function

*/
ListExpr
OutPoint( ListExpr typeInfo, Word value )
{
  Point* point = (Point*)(value.addr);
  if( point->IsDefined() )
    return nl->TwoElemList(
               nl->RealAtom( point->GetX() ),
               nl->RealAtom( point->GetY() ) );
  else
    return nl->SymbolAtom( "undef" );
}

/*
4.4 ~In~-function

*/
Word
InPoint( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct )
{
  if( nl->ListLength( instance ) == 2 )
  {
    ListExpr first = nl->First(instance),
             second = nl->Second(instance);

    Coord x, y;

    //1. processing the first data item
    correct = true;
    if( nl->IsAtom(first) )
    {
      if( nl->AtomType(first) == IntType )
        x = nl->IntValue(first);
      else if( nl->AtomType(first) == RealType )
        x = nl->RealValue(first);
      else
        correct = false;
    }
    else
      correct = false;

    //2. processing the second data item
    if( correct && nl->IsAtom(second) )
    {
      if( nl->AtomType(second) == IntType )
        y = nl->IntValue(second);
      else if( nl->AtomType(second) == RealType )
        y = nl->RealValue(second);
      else
        correct = false;
    }
    else
      correct = false;

    //3. create the class object
    if( correct )
      return SetWord( new Point( true, x, y ) );
  }
  else if( nl->IsEqual( instance, "undef" ) )
  {
    correct = true;
    return SetWord( new Point( false ) );
  }

  correct = false;
  return SetWord( Address(0) );
}

/*
4.5 ~Create~-function

*/
Word
CreatePoint( const ListExpr typeInfo )
{
  return SetWord( new Point( false ) );
}

/*
4.6 ~Delete~-function

*/
void
DeletePoint( const ListExpr typeInfo,
             Word& w )
{
  ((Point *)w.addr)->DeleteIfAllowed();
  w.addr = 0;
}

/*
4.7 ~Close~-function

*/
void
ClosePoint( const ListExpr typeInfo,
            Word& w )
{
  ((Point *)w.addr)->DeleteIfAllowed();
  w.addr = 0;
}

/*
4.8 ~Clone~-function

*/
Word
ClonePoint( const ListExpr typeInfo,
            const Word& w )
{
  return SetWord( new Point( *((Point *)w.addr) ) );
}

/*
4.8 ~SizeOf~-function

*/
int
SizeOfPoint()
{
  return sizeof(Point);
}

/*
4.9 Function describing the signature of the type constructor

*/
ListExpr
PointProperty()
{
  return nl->TwoElemList(
           nl->FourElemList(
             nl->StringAtom("Signature"),
             nl->StringAtom("Example Type List"),
             nl->StringAtom("List Rep"),
             nl->StringAtom("Example List")),
           nl->FourElemList(
             nl->StringAtom("-> DATA"),
             nl->StringAtom("point"),
             nl->StringAtom("(x y)"),
             nl->StringAtom("(10 5)")));
}

/*
4.10 Kind Checking Function

This function checks whether the type constructor is applied correctly. Since
type constructor ~point~ does not have arguments, this is trivial.

*/
bool
CheckPoint( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "point" ));
}

/*
4.11 ~Cast~-function

*/
void* CastPoint(void* addr)
{
  return (new (addr) Point);
}

/*
4.12 Creation of the type constructor instance

*/
TypeConstructor point(
  "point",                    //name
  PointProperty,              //property function describing signature
  OutPoint,      InPoint,     //Out and In functions
  0,             0,           //SaveToList and RestoreFromList functions
  CreatePoint,   DeletePoint, //object creation and deletion
  OpenAttribute<Point>,
  SaveAttribute<Point>,  // object open and save
  ClosePoint,    ClonePoint,  //object close, and clone
  CastPoint,                  //cast function
  SizeOfPoint,                //sizeof function
  CheckPoint );               //kind checking function

/*
5 Type Constructor ~points~

A ~points~ value is a finite set of points.

5.1 Implementation of the class ~Points~

*/
bool Points::Find( const Point& p, int& pos, const bool& exact ) const
{
  assert( IsOrdered() );
  if (exact)
    return points.Find( &p, PointCompare, pos );
  return points.Find( &p, PointCompareAlmost, pos );
}

Points& Points::operator=( const Points& ps )
{
  assert( ps.IsOrdered() );

  points.Clear();

  if( !ps.IsEmpty() )
  {
    points.Resize( ps.Size() );
    for( int i = 0; i < ps.Size(); i++ )
    {
      const Point *p;
      ps.Get( i, p );
      points.Put( i, *p );
    }
  }
  bbox = ps.BoundingBox();
  ordered = true;
  del.refs=1;
  del.isDelete=true;
  return *this;
}

void Points::StartBulkLoad()
{
  ordered = false;
}

void Points::EndBulkLoad( bool sort, bool remDup )
{
  if( sort ){
    Sort();
  }
  else{
    ordered = true;
  }
  if( remDup ){
    RemoveDuplicates();
  }
  points.TrimToSize();
}

bool Points::operator==( const Points& ps ) const
{
  assert( IsOrdered() && ps.IsOrdered() );

  if( Size() != ps.Size() )
    return false;

  if( IsEmpty() && ps.IsEmpty() )
    return true;

  if( bbox != ps.bbox )
    return false;

  object obj;
  status stat;
  SelectFirst_pp( *this, ps, obj, stat );

  while( obj == both || obj == none )
  {
    if( stat == endboth )
      return true;

    SelectNext_pp( *this, ps, obj, stat );
  }
  return false;
}

bool Points::operator!=( const Points& ps ) const
{
  return !( *this == ps );
}

Points& Points::operator+=( const Point& p )
{
  if( !IsOrdered() )
  { // DBArray is unsorted
    if( IsEmpty() )
      bbox = p.BoundingBox();
    else
      bbox = bbox.Union( p.BoundingBox() );
    points.Append(p);
  }
  else
  { // DBArray is sorted
    int pos;
    if( !Find( p, pos, false ) )
    { // no AlmostEqual point contained
      if( IsEmpty() )
        bbox = p.BoundingBox();
      else
        bbox = bbox.Union( p.BoundingBox() );
      Find( p, pos, true ); // find exact insertion position
      for( int i = points.Size() - 1; i >= pos; i++ )
      {
        const Point *auxp;
        points.Get( i, auxp );
        points.Put( i+1, *auxp );
      }
      points.Put( pos, p );
    } // else: do not insert
  }
  return *this;
}

Points& Points::operator+=( const Points& ps )
{
  if( IsEmpty() )
    bbox = ps.BoundingBox();
  else
    bbox = bbox.Union( ps.BoundingBox() );

  if( !IsOrdered() )
  {
    points.Resize( Size() + ps.Size() );
    for( int i = 0; i < ps.Size(); i++ )
    {
      const Point *p;
      ps.Get( i, p );
      points.Put( points.Size(), *p );
    }
  }
  else
  {
    Points newPs( Size() + ps.Size() );
    Union( ps, newPs );
    *this = newPs;
  }
  return *this;
}

Points& Points::operator-=( const Point& p )
{
  assert( IsOrdered() );

  int pos, posLow, posHigh;
  if( Find( p, pos, false ) )
  { // found an AlmostEqual point
    const Point *auxp;
    posLow = pos;
    while(posLow > 0)
    { // find first pos with AlmostEqual point
      points.Get( posLow-1, auxp );
      if( AlmostEqual(p, *auxp) )
      { posLow--; }
      else
      { break; }
    }
    posHigh = pos;
    while(posHigh < Size())
    { // find last pos with AlmostEqual point
      points.Get( posHigh+1, auxp );
      if( AlmostEqual(p, *auxp) )
      { posHigh++; }
      else
      { break; }
    }
    for( int i = 0; i < Size()-posHigh; i++ )
    { // keep smaller and move down bigger points
      points.Get( posHigh+i, auxp );
      points.Put( posLow+i, *auxp );
    }
    points.Resize( Size()-(1+posHigh-posLow) );

    // Naive way to redo the bounding box.
    if( IsEmpty() )
      bbox.SetDefined( false );
    int i = 0;
    points.Get( i++, auxp );
    bbox = auxp->BoundingBox();
    for( ; i < Size(); i++ )
    {
      points.Get( i, auxp );
      bbox = bbox.Union( auxp->BoundingBox() );
    }
  }
  return *this;
}

ostream& operator<<( ostream& o, const Points& ps )
{
  o << "<";
  for( int i = 0; i < ps.Size(); i++ )
  {
    const Point *p;
    ps.Get( i, p );
    o << " " << *p;
  }
  o << ">";

  return o;
}

// use this when adding and sorting the DBArray
int PointCompare( const void *a, const void *b )
{
  const Point *pa = (const Point*)a,
              *pb = (const Point*)b;

  if( *pa == *pb )
    return 0;

  if( *pa < *pb )
    return -1;

  return 1;
}

// use this when testing for containment or removing duplicates
// in the DBArray
int PointCompareAlmost( const void *a, const void *b )
{
  const Point *pa = (const Point*)a,
              *pb = (const Point*)b;

  if( AlmostEqual( *pa, *pb ) )
    return 0;

  if( *pa < *pb )
    return -1;

  return 1;
}

// Old implementation, should be replaced by the following one
// to avoid problems when sorting and removing duplicates
int PointHalfSegmentCompare( const void *a, const void *b )
{
  const Point *pa = (const Point *)a;
  const HalfSegment *hsb = (const HalfSegment *)b;

  if( AlmostEqual( *pa, hsb->GetDomPoint() ) )
    return 0;

  if( *pa < hsb->GetDomPoint() )
    return -1;

  return 1;
}

// // use this when adding and sorting the DBArray
// int PointHalfSegmentCompare( const void *a, const void *b )
// {
//   const Point *pa = (const Point *)a;
//   const HalfSegment *hsb = (const HalfSegment *)b;
//
//   if( *pa == hsb->GetDomPoint() )
//     return 0;
//
//   if( *pa < hsb->GetDomPoint() )
//     return -1;
//
//   return 1;
// }

// use this when testing for containment or removing duplicates
// in the DBArray
int PointHalfSegmentCompareAlmost( const void *a, const void *b )
{
  const Point *pa = (const Point *)a;
  const HalfSegment *hsb = (const HalfSegment *)b;

  if( AlmostEqual( *pa, hsb->GetDomPoint() ) )
    return 0;

  if( *pa < hsb->GetDomPoint() )
    return -1;

  return 1;
}

void Points::Sort(const bool exact /*= true*/)
{
  assert( !IsOrdered() );
  if(exact){
      points.Sort( PointCompare );
  } else{
      points.Sort(PointCompareAlmost);
  }
  ordered = true;
}


/*
Function supporting the RemoveDuplicates function.
This function checks whether in an array of points
a point exists which is AlmostEqual to the given one.
The search is restricted to the range in array given
by the indices __min__ and __max__.

*/
bool AlmostContains( const DBArray<Point>& points, const Point& p,
                     int min, int max, int size){

  if(min>max){
     return false;
  }
  const Point* pa;
  if(min==max){ // search around the position found
     // search left of min
     int pos = min;
     double x = p.GetX();
     points.Get(pos,pa);
     while(pos>=0 && AlmostEqual(pa->GetX(),x)){
        if(AlmostEqual(*pa,p)){
           return true;
        }
        pos--;
        if(pos>=0){
          points.Get(pos,pa);
        }
     }
     // search right of min
     pos=min+1;
     if(pos<size){
        points.Get(pos,pa);
     }
     while(pos<size &&AlmostEqual(pa->GetX(),x)){
        if(AlmostEqual(*pa,p)){
          return  true;
        }
        pos++;
        if(pos<size){
           points.Get(pos,pa);
        }
    }
    return false; // no matching point found
  } else {
      int mid = (min+max)/2;
      points.Get(mid,pa);
      double x = pa->GetX();
      double cx = p.GetX();
      if(AlmostEqual(x,cx)){
         return AlmostContains(points,p,mid,mid,size);
      } else if(cx<x){
         return AlmostContains(points,p,min,mid-1,size);
      }else {
         return AlmostContains(points,p,mid+1,max,size);
      }
  }
}



void Points::RemoveDuplicates()
{
 assert(IsOrdered());
 //Point allPoints[points.Size()];
 DBArray<Point> allPoints(points.Size());
 const Point* pp;
 Point p;
 for(int i=0;i<points.Size();i++){
    points.Get(i,pp);
    p = *pp;
    bool found = AlmostContains(allPoints,p,0,
                                allPoints.Size()-1,
                                allPoints.Size());
    if(!found){
      allPoints.Append(p);
    }
 }
 if(allPoints.Size()!=Size()){
     points.Clear();
     for(int i=0;i < allPoints.Size(); i++){
        allPoints.Get(i,pp);
        points.Append(*pp);
     }
 }
 allPoints.Destroy();
}

bool Points::Contains( const Point& p ) const
{
  assert( IsOrdered() );

  if( IsEmpty() )
    return false;

  if( !p.Inside( bbox ) )
    return false;

  int pos;
  return Find( p, pos, false ); // find using AlmostEqual
}

bool Points::Contains( const Points& ps ) const
{
  assert( IsOrdered() && ps.IsOrdered() );

  if( IsEmpty() && ps.IsEmpty() )
    return true;

  if( IsEmpty() || ps.IsEmpty() )
    return false;

  if( !bbox.Contains( ps.BoundingBox() ) )
    return false;

  object obj;
  status stat;
  SelectFirst_pp( *this, ps, obj, stat );

  while( stat != endsecond && stat != endboth )
  {
    if( obj == second || stat == endfirst )
      return false;

    SelectNext_pp( *this, ps, obj, stat );
  }
  return true;
}

bool Points::Inside( const Points& ps ) const
{
  assert( IsOrdered() && ps.IsOrdered() );
  return ps.Contains( *this );
}

bool Points::Inside( const Line& l ) const
{
  assert( IsOrdered() && l.IsOrdered() );

  if( IsEmpty() && l.IsEmpty() )
    return true;

  if( IsEmpty() || l.IsEmpty() )
    return false;

  if( !l.BoundingBox().Contains( bbox ) )
    return false;

  const Point *p;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, p );
    if( !l.Contains( *p ) )
      return false;
  }
  return true;
}

bool Points::Inside( const Region& r ) const
{
  assert( IsOrdered() && r.IsOrdered() );

  if( IsEmpty() && r.IsEmpty() )
    return true;

  if( IsEmpty() || r.IsEmpty() )
    return false;

  if( !r.BoundingBox().Contains( bbox ) )
    return false;

  const Point *p;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, p );
    if( !r.Contains( *p ) )
      return false;
  }
  return true;
}

bool Points::Intersects( const Points& ps ) const
{
  assert( IsOrdered() && ps.IsOrdered() );

  if( IsEmpty() && ps.IsEmpty() )
    return true;

  if( IsEmpty() || ps.IsEmpty() )
    return false;

  if( !bbox.Intersects( ps.BoundingBox() ) )
    return false;

  object obj;
  status stat;
  SelectFirst_pp( *this, ps, obj, stat );

  while( stat != endboth )
  {
    if( obj == both )
      return true;
    SelectNext_pp( *this, ps, obj, stat );
  }
  return false;
}

bool Points::Intersects( const Line& l ) const
{
  assert( IsOrdered() && l.IsOrdered() );

  if( IsEmpty() && l.IsEmpty() )
    return true;

  if( IsEmpty() || l.IsEmpty() )
    return false;

  if( !BoundingBox().Intersects( l.BoundingBox() ) )
    return false;

  const Point *p;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, p );
    if( l.Contains( *p ) )
      return true;
  }

  return false;
}

bool Points::Intersects( const Region& r ) const
{
  assert( IsOrdered() && r.IsOrdered() );

  if( IsEmpty() && r.IsEmpty() )
    return true;

  if( IsEmpty() || r.IsEmpty() )
    return false;

  if( !BoundingBox().Intersects( r.BoundingBox() ) )
    return false;

  const Point *p;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, p );
    if( r.Contains( *p ) )
      return true;
  }
  return false;
}

bool Points::Adjacent( const Region& r ) const
{
  assert( IsOrdered() && r.IsOrdered() );

  if( IsEmpty() || r.IsEmpty() )
    return false;

  if( !BoundingBox().Intersects( r.BoundingBox() ) )
    return false;

  const Point *p;
  bool found = false;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, p );

    if( !r.Contains( *p ) )
      continue;

    // At least one point is contained in the region
    // If it is not inside the region, then the
    // function will return true.
    found = true;
    if( r.InnerContains( *p ) )
      return false;
  }
  return found;
}

void Points::Intersection( const Points& ps, Points& result ) const
{
  assert( ordered && ps.ordered );
  result.Clear();

  object obj;
  status stat;
  const Point *p;
  SelectFirst_pp( *this, ps, obj, stat );

  result.StartBulkLoad();
  while( stat != endboth )
  {
    if( obj == both )
    {
      int GotPt = ps.GetPt( p );
      assert( GotPt );
      result += *p;
    }
    SelectNext_pp( *this, ps, obj, stat );
  }
  result.EndBulkLoad( false, false );
}

void Points::Intersection( const Line& l, Points& result ) const
{
  assert( IsOrdered() && l.IsOrdered() );
  result.Clear();

  if( IsEmpty() || l.IsEmpty() )
    return;

  const Point *p;
  result.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, p );
    if( l.Contains( *p ) )
      result += *p;
  }
  result.EndBulkLoad( false, false );
}

void Points::Intersection( const Region& r, Points& result ) const
{
  assert( IsOrdered() && r.IsOrdered() );
  result.Clear();

  if( IsEmpty() || r.IsEmpty() )
    return;

  const Point *p;
  result.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, p );
    if( r.Contains( *p ) )
      result += *p;
  }
  result.EndBulkLoad( false, false );
}

void Points::Minus( const Point& p, Points& ps ) const
{
  assert( p.IsDefined() && ordered );
  ps.Clear();

  ps.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    const Point *pi;
    Get( i, pi );

    if( !AlmostEqual(*pi, p) )
      ps += *pi;
  }
  ps.EndBulkLoad( false, false );
}

void Points::Minus( const Points& ps, Points& result ) const
{
  assert( ordered && ps.ordered );
  result.Clear();

  result.StartBulkLoad();
  int size1 = this->Size();
  int size2 = ps.Size();
  int pos1 = 0, pos2 = 0;
  const Point *p1, *p2;

  while(pos1<size1 && pos2<size2)
  {
    this->Get(pos1, p1);
    ps.Get(pos2, p2);
    if( AlmostEqual(*p1, *p2) )
    {
      pos1++;
    }
    else if (*p1 < *p2)
    {
      result += *p1;
      pos1++;
    }
    else
    { // *p1 > *p2
      pos2++;
    }
  }
  while(pos1<size1)
  {
    this->Get(pos1, p1);
    result += *p1;
    pos1++;
  }
  result.EndBulkLoad( false, false );
}

void Points::Minus( const Line& l, Points& result ) const
{
  assert( IsOrdered() && l.IsOrdered() );
  result.Clear();

  const Point *p;
  result.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, p );
    if( !l.Contains( *p ) )
      result += *p;
  }
  result.EndBulkLoad( false, false );
}

void Points::Minus( const Region& r, Points& result ) const
{
  assert( IsOrdered() && r.IsOrdered() );
  result.Clear();

  const Point *p;
  result.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, p );
    if( !r.Contains( *p ) )
      result += *p;
  }
  result.EndBulkLoad( false, false );
}

void Points::Union( const Point& p, Points& result ) const
{
  assert( p.IsDefined() && ordered );
  result.Clear();

  result.StartBulkLoad();
  const Point *pi;
  bool inserted = false;
  for( int i = 0; i < Size(); i++)
  {
    Get( i, pi );

    if( !inserted && *pi == p )
      inserted = true;

    if( !inserted && *pi > p )
    {
      result += p;
      inserted = true;
    }
    result += *pi;
  }
  if( !inserted )
    result += p;

  result.EndBulkLoad( false, true );
}

void Points::Union( const Points& ps, Points& result ) const
{
  assert( ordered && ps.ordered );
  result.Clear();

  object obj;
  status stat;
  SelectFirst_pp( *this, ps, obj, stat );
  const Point *p;

  result.StartBulkLoad();
  while( stat != endboth )
  {
    if( obj == first || obj == both )
    {
      int GotPt = GetPt( p );
      assert( GotPt );
    }
    else if( obj == second )
    {
      int GotPt = ps.GetPt( p );
      assert( GotPt );
    }
    result += *p;
    SelectNext_pp( *this, ps, obj, stat );
  }
  result.EndBulkLoad( false, true );
}


double Points::Distance( const Point& p ) const
{
  assert( p.IsDefined() && !IsEmpty() );

  double result = numeric_limits<double>::max();
  for( int i = 0; i < Size(); i++ )
  {
    const Point *pi;
    Get( i, pi );

    if( AlmostEqual( p, *pi ) )
      return 0.0;

    result = MIN( result, pi->Distance( p ) );
  }
  return result;
}

double Points::Distance( const Points& ps ) const
{
  assert( !IsEmpty() && !ps.IsEmpty() );

  double result = numeric_limits<double>::max();
  const Point *pi, *pj;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, pi );

    for( int j = 0; j < ps.Size(); j++ )
    {
      ps.Get( j, pj );

      if( AlmostEqual( *pi, *pj ) )
        return 0.0;

      result = MIN( result, pi->Distance( *pj ) );
    }
  }
  return result;
}

double Points::Distance( const Rectangle<2>& r ) const
{
  assert( IsDefined() && !IsEmpty() && r.IsDefined() );
  double result = numeric_limits<double>::max();
  const Point *pi;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, pi );
    result = MIN( result, pi->Distance( r ) );
  }
  return result;
}


void Points::Translate( const Coord& x, const Coord& y, Points& result ) const
{
  assert( result.IsEmpty() );
  assert( ordered );

  result.StartBulkLoad();
  const Point *p;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, p );
    result += p->Translate( x, y );
  }
  result.EndBulkLoad( false, false );
}


void Points::Rotate( const Coord& x, const Coord& y,
                     const double alpha,
                     Points& result ) const
{
  result.Clear();
  if(!IsDefined()){
     result.SetDefined(false);
     return;
  }
  result.Resize(Size());
  result.SetDefined(true);

  double s = sin(alpha);
  double c = cos(alpha);

  double m00 = c;
  double m01 = -s;
  double m02 = x - x*c + y*s;
  double m10 = s;
  double m11 = c;
  double m12 = y - x*s-y*c;


  result.StartBulkLoad();
  const Point *p;
  Point rot(true,0,0);

  for( int i = 0; i < Size(); i++ )
  {
    Get( i, p );
    rot.Set( m00*p->GetX() + m01*p->GetY() + m02,
             m10*p->GetX() + m11*p->GetY() + m12);
    result += rot;
  }
  result.EndBulkLoad( true, false );

}

Point Points::theCenter() const{
   Point res(true,0,0);
   if(!IsDefined() || (Size()==0)){
     res.SetDefined(false);
   } else {
     int size = Size();
     const Point* p;
     double x = 0.0;
     double y = 0.0;
     for(int i=0;i<size;i++){
         Get(i,p);
         x += p->GetX();
         y += p->GetY();
     }
     res.Set(x/size,y/size);
   }
   return res;
}



size_t Points::HashValue() const
{
  if( IsEmpty() )
  return 0;

  size_t h = 0;

  const Point *p;
  for( int i = 0; i < Size() && i < 5; i++ )
  {
    Get( i, p );
    h = h + (size_t)(5 * p->GetX() + p->GetY());
  }
  return h;
}

bool Points::IsValid() const
{
  if( IsEmpty() )
    return true;

  const Point *auxp1, *p2;
  Get( 0, auxp1 );

  Point p1 = *auxp1;
  for( int i = 1; i < Size(); i++ )
  {
    Get( i, p2 );
    if( AlmostEqual( p1, *p2 ) )
      return false;
    p1 = *p2;
  }
  return true;
}

void Points::Clear()
{
  points.Clear();
  pos = -1;
  ordered = true;
  bbox.SetDefined( false );
}

void Points::CopyFrom( const StandardAttribute* right )
{
  const Points *ps = (const Points*)right;
  assert( ps->IsOrdered() );
  *this = *ps;
}

int Points::Compare( const Attribute* arg ) const
{
  const Points* ps = (const Points*)arg;

  if( !ps )
    return (-2);

  if( IsEmpty() && ps->IsEmpty() )
    return 0;

  if( IsEmpty() )
    return -1;

  if( ps->IsEmpty() )
    return 1;

  if( Size() > ps->Size() )
    return 1;

  if( Size() < ps->Size() )
    return -1;

  for( int i = 0; i < Size(); i++ )
  {
    const Point *p1, *p2;
    Get( i, p1);
    ps->Get( i, p2 );

    if( *p1 > *p2 )
      return 1;

    if( *p1 < *p2 )
      return -1;
  }
  return 0;
}

int Points::CompareAlmost( const Attribute* arg ) const
{
  const Points* ps = (const Points*)arg;

  if( !ps )
    return (-2);

  if( IsEmpty() && ps->IsEmpty() )
    return 0;

  if( IsEmpty() )
    return -1;

  if( ps->IsEmpty() )
    return 1;

  if( Size() > ps->Size() )
    return 1;

  if( Size() < ps->Size() )
    return -1;

  for( int i = 0; i < Size(); i++ )
  {
    const Point *p1, *p2;
    Get( i, p1);
    ps->Get( i, p2 );

    if( !AlmostEqual(*p1, *p2) )
    {
      if( *p1 > *p2 )
        return 1;
      if( *p1 < *p2 )
        return -1;
    }
  }
  return 0;
}


bool Points::Adjacent( const Attribute* arg ) const
{
  return 0;
  // for points which takes double values, we can not decide whether they are
  //adjacent or not.
}

Points* Points::Clone() const
{
  return new Points( *this );
}

ostream& Points::Print( ostream &os ) const
{
  return os << *this;
}

/*
5.2 List Representation

The list representation of a point is

----  (x y)
----

5.3 ~Out~-function

*/
ListExpr
OutPoints( ListExpr typeInfo, Word value )
{
  Points* points = (Points*)(value.addr);
  if( points->IsEmpty() )
    return nl->TheEmptyList();

  const Point *p;
  points->Get( 0, p );
  ListExpr result =
    nl->OneElemList( OutPoint( nl->TheEmptyList(), SetWord( (void*)p ) ) );
  ListExpr last = result;

  for( int i = 1; i < points->Size(); i++ )
  {
    points->Get( i, p );
    last = nl->Append( last,
                       OutPoint( nl->TheEmptyList(), SetWord( (void*)p ) ) );
  }
  return result;
}

/*
5.4 ~In~-function

*/
Word
InPoints( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct )
{
  Points* points = new Points( nl->ListLength( instance ) );
  points->StartBulkLoad();

  ListExpr rest = instance;
  while( !nl->IsEmpty( rest ) )
  {
    ListExpr first = nl->First( rest );
    rest = nl->Rest( rest );

    Point *p = (Point*)InPoint( nl->TheEmptyList(),
                                first, 0, errorInfo, correct ).addr;
    if( correct )
    {
      (*points) += (*p);
      delete p;
    }
    else
    {
      delete p;
      return SetWord( Address(0) );
    }
  }
  points->EndBulkLoad();

  if( points->IsValid() )
  {
    correct = true;
    return SetWord( points );
  }
  delete points;
  correct = false;
  return SetWord( Address(0) );
}

/*
5.5 ~Create~-function

*/
Word
CreatePoints( const ListExpr typeInfo )
{
  return SetWord( new Points( 0 ) );
}

/*
5.6 ~Delete~-function

*/
void
DeletePoints( const ListExpr typeInfo, Word& w )
{
  Points *ps = (Points *)w.addr;
  ps->Destroy();
  ps->DeleteIfAllowed();
  w.addr = 0;
}

/*
5.7 ~Close~-function

*/
void
ClosePoints( const ListExpr typeInfo, Word& w )
{
  ((Points *)w.addr)->DeleteIfAllowed();
  w.addr = 0;
}

/*
5.8 ~Clone~-function

*/
Word
ClonePoints( const ListExpr typeInfo, const Word& w )
{
  return SetWord( new Points( *((Points *)w.addr) ) );
}

/*
7.8 ~Open~-function

*/
bool
OpenPoints( SmiRecord& valueRecord,
            size_t& offset,
            const ListExpr typeInfo,
            Word& value )
{
  Points *ps = (Points*)Attribute::Open( valueRecord, offset, typeInfo );
  value = SetWord( ps );
  return true;
}

/*
7.8 ~Save~-function

*/
bool
SavePoints( SmiRecord& valueRecord,
            size_t& offset,
            const ListExpr typeInfo,
            Word& value )
{
  Points *ps = (Points*)value.addr;
  Attribute::Save( valueRecord, offset, typeInfo, ps );
  return true;
}

/*
5.8 ~SizeOf~-function

*/
int
SizeOfPoints()
{
  return sizeof(Points);
}

/*
5.11 Function describing the signature of the type constructor

*/
ListExpr
PointsProperty()
{
  return (nl->TwoElemList(
            nl->FourElemList(nl->StringAtom("Signature"),
                       nl->StringAtom("Example Type List"),
           nl->StringAtom("List Rep"),
           nl->StringAtom("Example List")),
            nl->FourElemList(nl->StringAtom("-> DATA"),
                       nl->StringAtom("points"),
           nl->StringAtom("(<point>*) where point is (<x><y>)"),
           nl->StringAtom("( (10 1)(4 5) )"))));
}

/*
5.12 Kind checking function

This function checks whether the type constructor is applied correctly. Since
type constructor ~point~ does not have arguments, this is trivial.

*/
bool
CheckPoints( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "points" ));
}

/*
5.13 ~Cast~-function

*/
void* CastPoints(void* addr)
{
  return (new (addr) Points);
}

/*
5.14 Creation of the type constructor instance

*/
TypeConstructor points(
        "points",                     //name
        PointsProperty,               //property function describing signature
        OutPoints,      InPoints,     //Out and In functions
        0,              0,            //SaveTo and RestoreFrom List functions
        CreatePoints,   DeletePoints, //object creation and deletion
        OpenPoints,     SavePoints,   // object open and save
        ClosePoints,    ClonePoints,  //object close and clone
        CastPoints,                   //cast function
        SizeOfPoints,                 //sizeof function
        CheckPoints );                //kind checking function

/*
6 Type Constructor ~halfsegment~

A ~halfsegment~ value is a pair of points, with a boolean flag indicating the dominating point .

6.1 Implementation of the class ~halfsegment~

*/
void HalfSegment::Translate( const Coord& x, const Coord& y )
{
  lp.Translate( x, y );
  rp.Translate( x, y );
}


void HalfSegment::Set( bool ldp, const Point& lp, const Point& rp )
{
  assert( !AlmostEqual( lp, rp ) );

  this->ldp = ldp;
  if( lp < rp )
  {
    this->lp = lp;
    this->rp = rp;
  }
  else // rp > lp
  {
    this->lp = rp;
    this->rp = lp;
  }
}

int HalfSegment::Compare( const HalfSegment& hs ) const
{
  const Point& dp = GetDomPoint(),
                   sp = GetSecPoint(),
                   DP = hs.GetDomPoint(),
                   SP = hs.GetSecPoint();

  if( dp < DP )
    return -1;
  else if( dp > DP )
    return 1;

  if( ldp != hs.ldp )
  {
    if( ldp == false )
      return -1;
    return 1;
  }
  else
  {
    bool v1 = IsVertical();
    bool v2 = hs.IsVertical();
    if( v1 && v2 ) // both are vertical
    {
      if(   (     (CompareDouble(sp.GetY(),dp.GetY())>0)
               && ( CompareDouble(SP.GetY(),DP.GetY())>0)
            )
          ||
            (     (CompareDouble(sp.GetY(),dp.GetY())<0)
               && (CompareDouble(SP.GetY(),DP.GetY())<0) ) )
      {
        if( sp < SP )
          return -1;
        if( sp > SP )
          return 1;
        return 0;
      }
      else if( CompareDouble(sp.GetY(),dp.GetY())>0)
      {
        if( ldp == true )
          return 1;
        return -1;
      }
      else
      {
        if( ldp == true )
          return -1;
        return 1;
      }
    }
    else if( AlmostEqual(dp.GetX(),sp.GetX()) )
    {
      if( CompareDouble(sp.GetY(), dp.GetY())>0 )
      {
        if( ldp == true )
          return 1;
        return -1;
      }
      else if( CompareDouble(sp.GetY(),dp.GetY())<0 )
      {
        if( ldp == true )
          return -1;
        return 1;
      }
    }
    else if( AlmostEqual(DP.GetX(), SP.GetX()) )
    {
      if( CompareDouble(SP.GetY() , DP.GetY())>0 )
      {
        if( ldp == true )
          return -1;
        return 1;
      }
      else if( CompareDouble(SP.GetY() , DP.GetY())<0 )
      {
        if( ldp == true )
          return 1;
        return -1;
      }
    }
    else
    {
      Coord xd = dp.GetX(), yd = dp.GetY(),
            xs = sp.GetX(), ys = sp.GetY(),
            Xd = DP.GetX(), Yd = DP.GetY(),
            Xs = SP.GetX(), Ys = SP.GetY();
      double k = (yd - ys) / (xd - xs),
             K= (Yd -Ys) / (Xd - Xs);

      if( CompareDouble(k , K) <0 )
        return -1;
      if( CompareDouble( k,  K) > 0)
        return 1;

      if( sp < SP )
        return -1;
      if( sp > SP )
        return 1;
      return 0;
    }
  }
  assert( true ); // This code should never be reached
  return 0;
}

HalfSegment& HalfSegment::operator=( const HalfSegment& hs )
{
  ldp = hs.ldp;
  lp = hs.lp;
  rp = hs.rp;
  attr = hs.attr;
  return *this;
}

bool HalfSegment::operator==( const HalfSegment& hs ) const
{
  return Compare(hs) == 0;
}

bool HalfSegment::operator!=( const HalfSegment& hs ) const
{
  return !( *this == hs );
}

bool HalfSegment::operator<( const HalfSegment& hs ) const
{
  return Compare(hs) == -1;
}

bool HalfSegment::operator>( const HalfSegment& hs ) const
{
  return Compare(hs) == 1;
}

int HalfSegment::LogicCompare( const HalfSegment& hs ) const
{
  if( attr.faceno < hs.attr.faceno )
    return -1;

  if( attr.faceno > hs.attr.faceno )
    return 1;

  if( attr.cycleno < hs.attr.cycleno )
    return -1;

  if( attr.cycleno > hs.attr.cycleno )
    return 1;

  if( attr.edgeno < hs.attr.edgeno )
    return -1;

  if( attr.edgeno > hs.attr.edgeno )
    return 1;

  return 0;
}

int HalfSegmentCompare(const void *a, const void *b)
{
  const HalfSegment *hsa = (const HalfSegment *)a,
                    *hsb = (const HalfSegment *)b;
  return hsa->Compare( *hsb );
}

int HalfSegmentLogicCompare(const void *a, const void *b)
{
  const HalfSegment *hsa = (const HalfSegment *)a,
                    *hsb = (const HalfSegment *)b;

  return hsa->LogicCompare( *hsb );
}

int LRSCompare( const void *a, const void *b )
{
  const LRS *lrsa = (const LRS *)a,
            *lrsb = (const LRS *)b;

  if( lrsa->lrsPos < lrsb->lrsPos )
    return -1;
  if( lrsa->lrsPos > lrsb->lrsPos )
    return 1;

  return 0;
}

ostream& operator<<(ostream &os, const HalfSegment& hs)
{
  return os << "("
             <<"F("<< hs.attr.faceno
             <<") C("<<  hs.attr.cycleno
             <<") E(" << hs.attr.edgeno<<") DP("
             <<  (hs.IsLeftDomPoint()? "L":"R")
             <<") IA("<< (hs.attr.insideAbove? "A":"U")
             <<") Co("<<hs.attr.coverageno
             <<") PNo("<<hs.attr.partnerno
             <<") ("<< hs.GetLeftPoint() << " "<< hs.GetRightPoint() <<") ";
}

bool HalfSegment::Intersects( const HalfSegment& hs ) const
{
  double k, a, K, A;

  if( !BoundingBox().Intersects( hs.BoundingBox() ) )
    return false;

  Coord xl = lp.GetX(),
        yl = lp.GetY(),
        xr = rp.GetX(),
        yr = rp.GetY(),
        Xl = hs.GetLeftPoint().GetX(),
        Yl = hs.GetLeftPoint().GetY(),
        Xr = hs.GetRightPoint().GetX(),
        Yr = hs.GetRightPoint().GetY();

  if( AlmostEqual( xl, xr ) &&
      AlmostEqual( Xl, Xr ) )
    // both segments are vertical
  {
    if( AlmostEqual( xl, Xl ) &&
        ( AlmostEqual( yl, Yl ) || AlmostEqual( yl, Yr ) ||
          AlmostEqual( yr, Yl ) || AlmostEqual( yr, Yr ) ||
          ( yl > Yl && yl < Yr ) || ( yr > Yl && yr < Yr ) ||
          ( Yl > yl && Yl < yr ) || ( Yr > yl && Yr < yr ) ) )
      return true;
    return false;
  }

  if( !AlmostEqual( xl, xr ) )
    // this segment is not vertical
  {
    k = (yr - yl) / (xr - xl);
    a = yl - k * xl;
  }


  if( !AlmostEqual( Xl, Xr ) )
    // hs is not vertical
  {
    K = (Yr - Yl) / (Xr - Xl);
    A = Yl - K * Xl;
  }

  if( AlmostEqual( Xl, Xr ) )
    //only hs is vertical
  {
    Coord y0 = k * Xl + a;

    if( ( Xl > xl || AlmostEqual( Xl, xl ) ) &&
        ( Xl < xr || AlmostEqual( Xl, xr ) ) )
    {
      if( ( ( y0 > Yl || AlmostEqual( y0, Yl ) ) &&
            ( y0 < Yr || AlmostEqual( y0, Yr ) ) ) ||
          ( ( y0 > Yr || AlmostEqual( y0, Yr ) ) &&
            ( y0 < Yl || AlmostEqual( y0, Yl ) ) ) )
        // (Xl, y0) is the intersection point
        return true;
    }
    return false;
  }

  if( AlmostEqual( xl, xr ) )
    // only this segment is vertical
  {
    Coord Y0 = K * xl + A;

    if( ( xl > Xl || AlmostEqual( xl, Xl ) ) &&
        ( xl < Xr || AlmostEqual( xl, Xr ) ) )
    {
      if( ( ( Y0 > yl || AlmostEqual( Y0, yl ) ) &&
            ( Y0 < yr || AlmostEqual( Y0, yr ) ) ) ||
          ( ( Y0 > yr || AlmostEqual( Y0, yr ) ) &&
            ( Y0 < yl || AlmostEqual( Y0, yl ) ) ) )
        // (xl, Y0) is the intersection point
        return true;
    }
    return false;
  }

  // both segments are non-vertical

  if( AlmostEqual( k, K ) )
    // both segments have the same inclination
  {
    if( AlmostEqual( A, a ) &&
        ( ( xl > Xl || AlmostEqual( xl, Xl ) ) &&
          ( xl < Xr || AlmostEqual( xl, Xr ) ) ) ||
        ( ( Xl > xl || AlmostEqual( xl, Xl ) ) &&
          ( Xl < xr || AlmostEqual( xr, Xl ) ) ) )
      // the segments are in the same straight line
      return true;
  }
  else
  {
    Coord x0 = (A - a) / (k - K);
    // y0 = x0 * k + a;

    if( ( x0 > xl || AlmostEqual( x0, xl ) ) &&
        ( x0 < xr || AlmostEqual( x0, xr ) ) &&
        ( x0 > Xl || AlmostEqual( x0, Xl ) ) &&
        ( x0 < Xr || AlmostEqual( x0, Xr ) ) )
      // the segments intersect at (x0, y0)
      return true;
  }
  return false;
}

bool HalfSegment::InnerIntersects( const HalfSegment& hs ) const
{
  double k, a, K, A;
  Coord x0; //, y0;  (x0, y0) is the intersection

  Coord xl = lp.GetX(), yl = lp.GetY(),
        xr = rp.GetX(), yr = rp.GetY();

  if( xl != xr )
  {
    k = (yr - yl) / (xr - xl);
    a = yl - k * xl;
  }

  Coord Xl = hs.GetLeftPoint().GetX(),
        Yl = hs.GetLeftPoint().GetY(),
        Xr = hs.GetRightPoint().GetX(),
        Yr = hs.GetRightPoint().GetY();

  if( Xl != Xr )
  {
    K = (Yr - Yl) / (Xr - Xl);
    A = Yl - K * Xl;
  }

  if( xl == xr && Xl == Xr ) //both l and L are vertical lines
  {
    if( xl != Xl )
      return false;

    Coord ylow, yup, Ylow, Yup;
    if( yl < yr )
    {
      ylow = yl;
      yup = yr;
    }
    else
    {
      ylow = yr;
      yup = yl;
    }

    if( Yl < Yr)
    {
      Ylow = Yl;
      Yup = Yr;
    }
    else
    {
      Ylow = Yr;
      Yup = Yl;
    }

    if( ylow >= Yup || yup <= Ylow )
      return false;
    return true;
  }

  if( Xl == Xr )    //only L is vertical
  {
    double y0 = k * Xl + a;
    Coord yy = y0;

    //(Xl, y0) is the intersection of l and L
    if( Xl >= xl && Xl <= xr )
    {
      if( ( yy > Yl && yy < Yr ) ||
          ( yy > Yr && yy < Yl ) )
        return true;
      return false;
    }
    else return false;
  }

  if( xl == xr )    //only l is vertical
  {
    double Y0 = K * xl + A;
    Coord YY = Y0;

    //(xl, Y0) is the intersection of l and L
    if( xl > Xl && xl < Xr )
    {
      if( ( YY >= yl && YY <= yr ) ||
          ( YY >= yr && YY <= yl ) )
        return true;
      return false;
    }
    else return false;
  }

  //otherwise: both segments are non-vertical
  if( k == K )
  {
    if( A != a ) //Parallel lines
      return false;

    //they are in the same straight line
    if( xr <= Xl || xl >= Xr )
      return false;
    return true;
  }
  else
  {
    x0 = (A - a) / (k - K);  // y0=x0*k+a;
    Coord xx = x0;
    if( xx >= xl && xx <= xr && xx > Xl && xx < Xr )
      return true;
    return false;
  }
}

bool HalfSegment::Intersection( const HalfSegment& hs, Point& resp ) const
{
  double k, a, K, A;

  if( !BoundingBox().Intersects( hs.BoundingBox() ) )
    return false;

  Coord xl = lp.GetX(),
        yl = lp.GetY(),
        xr = rp.GetX(),
        yr = rp.GetY(),
        Xl = hs.GetLeftPoint().GetX(),
        Yl = hs.GetLeftPoint().GetY(),
        Xr = hs.GetRightPoint().GetX(),
        Yr = hs.GetRightPoint().GetY();

  if( AlmostEqual( xl, xr ) &&
      AlmostEqual( Xl, Xr ) )
    // both segments are vertical
  {
    if( AlmostEqual( yr, Yl ) )
    {
      resp.Set( xl, yr );
      return true;
    }
    if( AlmostEqual( yl, Yr ) )
    {
      resp.Set( xl, yl );
      return true;
    }
    return false;
  }

  if( !AlmostEqual( xl, xr ) )
    // this segment is not vertical
  {
    k = (yr - yl) / (xr - xl);
    a = yl - k * xl;
  }

  if( !AlmostEqual( Xl, Xr ) )
    // hs is not vertical
  {
    K = (Yr - Yl) / (Xr - Xl);
    A = Yl - K * Xl;
  }

  if( AlmostEqual( Xl, Xr ) )
    //only hs is vertical
  {
    Coord y0 = k * Xl + a;

    if( ( Xl > xl || AlmostEqual( Xl, xl ) ) &&
        ( Xl < xr || AlmostEqual( Xl, xr ) ) )
    {
      if( ( ( y0 > Yl || AlmostEqual( y0, Yl ) ) &&
            ( y0 < Yr || AlmostEqual( y0, Yr ) ) ) ||
          ( ( y0 > Yr || AlmostEqual( y0, Yr ) ) &&
            ( y0 < Yl || AlmostEqual( y0, Yl ) ) ) )
        // (Xl, y0) is the intersection point
      {
        resp.Set( Xl, y0 );
        return true;
      }
    }
    return false;
  }

  if( AlmostEqual( xl, xr ) )
    // only this segment is vertical
  {
    Coord Y0 = K * xl + A;

    if( ( xl > Xl || AlmostEqual( xl, Xl ) ) &&
        ( xl < Xr || AlmostEqual( xl, Xr ) ) )
    {
      if( ( ( Y0 > yl || AlmostEqual( Y0, yl ) ) &&
            ( Y0 < yr || AlmostEqual( Y0, yr ) ) ) ||
          ( ( Y0 > yr || AlmostEqual( Y0, yr ) ) &&
            ( Y0 < yl || AlmostEqual( Y0, yl ) ) ) )
        // (xl, Y0) is the intersection point
      {
        resp.Set( xl, Y0 );
        return true;
      }
    }
    return false;
  }

  // both segments are non-vertical

  if( AlmostEqual( k, K ) )
    // both segments have the same inclination
  {
    if( AlmostEqual( rp, hs.lp ) )
    {
      resp = rp;
      return true;
    }
    if( AlmostEqual( lp, hs.rp ) )
    {
      resp = lp;
      return true;
    }
    return false;
  }

  Coord x0 = (A - a) / (k - K),
        y0 = x0 * k + a;

  if( ( x0 > xl || AlmostEqual( x0, xl ) ) &&
      ( x0 < xr || AlmostEqual( x0, xr ) ) &&
      ( x0 > Xl || AlmostEqual( x0, Xl ) ) &&
      ( x0 < Xr || AlmostEqual( x0, Xr ) ) )
    // the segments intersect at (x0, y0)
  {
    resp.Set( x0, y0 );
    return true;
  }

  return false;
}

bool HalfSegment::Intersection( const HalfSegment& hs,
                                HalfSegment& reshs ) const
{
  double k, a, K, A;

  if( !BoundingBox().Intersects( hs.BoundingBox() ) )
    return false;

  if( AlmostEqual( *this, hs ) )
  {
    reshs = hs;
    return true;
  }

  Coord xl = lp.GetX(),
        yl = lp.GetY(),
        xr = rp.GetX(),
        yr = rp.GetY(),
        Xl = hs.GetLeftPoint().GetX(),
        Yl = hs.GetLeftPoint().GetY(),
        Xr = hs.GetRightPoint().GetX(),
        Yr = hs.GetRightPoint().GetY();

  if( AlmostEqual( xl, xr ) &&
      AlmostEqual( Xl, Xr ) )
    //both l and L are vertical lines
  {
    Coord ylow, yup, Ylow, Yup;
    if( yl < yr )
    {
      ylow = yl;
      yup = yr;
    }
    else
    {
      ylow = yr;
      yup = yl;
    }

    if( Yl < Yr )
    {
      Ylow = Yl;
      Yup = Yr;
    }
    else
    {
      Ylow = Yr;
      Yup = Yl;
    }

    if( yup > Ylow && ylow < Yup &&
        !AlmostEqual( yup, Ylow ) &&
        !AlmostEqual( ylow, Yup ) )
    {
      Point p1, p2;
      if( ylow > Ylow )
        p1.Set( xl, ylow );
      else
        p1.Set( xl, Ylow );

      if( yup < Yup )
        p2.Set( xl, yup );
      else
        p2.Set( xl, Yup );

      reshs.Set( true, p1, p2 );
      return true;
    }
    else return false;
  }

  if( AlmostEqual( Xl, Xr ) ||
      AlmostEqual( xl, xr ) )
    //only L or l is vertical
    return false;

  //otherwise: both *this and *arg are non-vertical lines

  k = (yr - yl) / (xr - xl);
  a = yl - k * xl;

  K = (Yr - Yl) / (Xr - Xl);
  A = Yl - K * Xl;

  if( AlmostEqual( k, K ) &&
      AlmostEqual( A, a ) )
  {
    if( xr > Xl && xl < Xr &&
        !AlmostEqual( xr, Xl ) &&
        !AlmostEqual( xl, Xr ) )
    {
      Point p1, p2;
      if( xl > Xl )
        p1.Set( xl, yl );
      else
        p1.Set( Xl, Yl );

      if( xr < Xr )
        p2.Set( xr, yr );
      else
        p2.Set( Xr, Yr );

      reshs.Set( true, p1, p2 );
      return true;
    }
  }

  return false;
}

bool HalfSegment::Crosses( const HalfSegment& hs ) const
{
  double k, a, K, A;

  if( !BoundingBox().Intersects( hs.BoundingBox() ) )
    return false;

  Coord xl = lp.GetX(),
        yl = lp.GetY(),
        xr = rp.GetX(),
        yr = rp.GetY(),
        Xl = hs.GetLeftPoint().GetX(),
        Yl = hs.GetLeftPoint().GetY(),
        Xr = hs.GetRightPoint().GetX(),
        Yr = hs.GetRightPoint().GetY();

  if( AlmostEqual( xl, xr ) &&
      AlmostEqual( Xl, Xr ) )
    // both segments are vertical
    return false;

  if( !AlmostEqual( xl, xr ) )
    // this segment is not vertical
  {
    k = (yr - yl) / (xr - xl);
    a = yl - k * xl;
  }

  if( !AlmostEqual( Xl, Xr ) )
    // hs is not vertical
  {
    K = (Yr - Yl) / (Xr - Xl);
    A = Yl - K * Xl;
  }

  if( AlmostEqual( Xl, Xr ) )
    //only hs is vertical
  {
    double y0 = k * Xl + a;

    if( Xl > xl && !AlmostEqual( Xl, xl ) &&
        Xl < xr && !AlmostEqual( Xl, xr ) )
    {
      if( ( y0 > Yl && !AlmostEqual( y0, Yl ) &&
            y0 < Yr && !AlmostEqual( y0, Yr ) ) ||
          ( y0 > Yr && !AlmostEqual( y0, Yr ) &&
            y0 < Yl && !AlmostEqual( y0, Yl ) ) )
        // (Xl, y0) is the intersection point
        return true;
    }
    return false;
  }

  if( AlmostEqual( xl, xr ) )
    // only this segment is vertical
  {
    double Y0 = K * xl + A;

    if( ( xl > Xl && !AlmostEqual( xl, Xl ) ) &&
        ( xl < Xr && !AlmostEqual( xl, Xr ) ) )
    {
      if( ( Y0 > yl && !AlmostEqual( Y0, yl ) &&
            Y0 < yr && !AlmostEqual( Y0, yr ) ) ||
          ( Y0 > yr && !AlmostEqual( Y0, yr ) &&
            Y0 < yl && !AlmostEqual( Y0, yl ) ) )
        // (xl, Y0) is the intersection point
        return true;
    }
    return false;
  }

  // both segments are non-vertical

  if( AlmostEqual( k, K ) )
    // both segments have the same inclination
    return false;

  double x0 = (A - a) / (k - K);
  // y0 = x0 * k + a;

  if( x0 > xl && !AlmostEqual( x0, xl ) &&
      x0 < xr && !AlmostEqual( x0, xr ) &&
      x0 > Xl && !AlmostEqual( x0, Xl ) &&
      x0 < Xr && !AlmostEqual( x0, Xr ) )
    // the segments intersect at (x0, y0)
    return true;

  return false;
}

bool HalfSegment::Inside( const HalfSegment& hs ) const
{
  return hs.Contains( GetLeftPoint() ) &&
         hs.Contains( GetRightPoint() );
}

bool HalfSegment::Contains( const Point& p ) const
{
  if( AlmostEqual( p, lp ) ||
      AlmostEqual( p, rp ) )
    return true;

  Coord xl = lp.GetX(), yl = lp.GetY(),
        xr = rp.GetX(), yr = rp.GetY(),
        x = p.GetX(), y = p.GetY();

  if( xl != xr && xl != x )
    // the segment is not vertical
  {
    double k1 = (y - yl) / (x - xl),
           k2 = (yr - yl) / (xr - xl);

    if( AlmostEqual( k1, k2 ) )
    {
      if( ( x > xl || AlmostEqual( x, xl ) ) &&
          ( x < xr || AlmostEqual( x, xr ) ) )
        // we check only this possibility because lp < rp and
        // therefore, in this case, xl < xr
        return true;
    }
  }
  else if( AlmostEqual( xl, xr ) &&
           AlmostEqual( xl, x ) )
    // the segment is vertical and the point is also in the
    // same x-position. In this case we just have to check
    // whether the point is inside the y-interval
  {
    if( ( y > yl || AlmostEqual( y, yl ) ) &&
        ( y < yr || AlmostEqual( y, yr ) ) ||
        ( y < yl || AlmostEqual( y, yl ) ) &&
        ( y > yr || AlmostEqual( y, yr ) ) )
      // Here we check both possibilities because we do not
      // know wheter yl < yr, given that we used the
      // AlmostEqual function in the previous condition
      return true;
  }

  return false;
}

double HalfSegment::Distance( const Point& p ) const
{
  Coord xl = GetLeftPoint().GetX(),
        yl = GetLeftPoint().GetY(),
        xr = GetRightPoint().GetX(),
        yr = GetRightPoint().GetY(),
        X = p.GetX(),
        Y = p.GetY();

  double result, auxresult;

  if( xl == xr || yl == yr )
  {
    if( xl == xr) //hs is vertical
    {
      if( (yl <= Y && Y <= yr) || (yr <= Y && Y <= yl) )
        result = fabs( X - xl );
      else
      {
        result = p.Distance( GetLeftPoint() );
        auxresult = p.Distance( GetRightPoint() );
        if( result > auxresult )
          result = auxresult;
      }
    }
    else         //hs is horizontal line: (yl==yr)
    {
      if( xl <= X && X <= xr )
        result = fabs( Y - yl );
      else
      {
        result = p.Distance( GetLeftPoint() );
        auxresult = p.Distance( GetRightPoint() );
        if( result > auxresult )
          result = auxresult;
      }
    }
  }
  else
  {
    double k = (yr - yl) / (xr - xl),
           a = yl - k * xl,
           xx = (k * (Y - a) + X) / (k * k + 1),
           yy = k * xx + a;
    Coord XX = xx,
          YY = yy;
    Point PP( true, XX, YY );
    if( xl <= XX && XX <= xr )
      result = p.Distance( PP );
    else
    {
      result = p.Distance( GetLeftPoint() );
      auxresult = p.Distance( GetRightPoint() );
      if( result > auxresult )
        result = auxresult;
    }
  }
  return result;
}

double HalfSegment::Distance( const HalfSegment& hs ) const
{
  if( Intersects( hs ) )
    return 0.0;

  return MIN( Distance( hs.GetLeftPoint() ),
              Distance( hs.GetRightPoint() ) );
}


double HalfSegment::Distance(const Rectangle<2>& rect) const{

  
  if(rect.Contains(lp.BoundingBox()) || rect.Contains(rp.BoundingBox()) ){
    return 0.0;
  }
  // both endpoints are outside the rectangle
  double x0(rect.MinD(0));
  double y0(rect.MinD(1));
  double x1(rect.MaxD(0));
  double y1(rect.MaxD(1));
  Point p0(true,x0,y0);
  Point p1(true,x1,y0);
  Point p2(true,x1,y1);
  Point p3(true,x0,y1);
  HalfSegment hs(true,p0,p1);
  double dist = this->Distance(hs);
  if(dist<FACTOR){
    return dist;
  }
  hs.Set(true,p1,p2);
  dist = MIN( dist, this->Distance(hs));
  if(dist<FACTOR){
    return dist;
  }
  hs.Set(true,p2,p3);
  dist = MIN( dist, this->Distance(hs));
  if(dist<FACTOR){
    return dist;
  }
  hs.Set(true,p3,p0);
  dist = MIN( dist, this->Distance(hs));
  return dist;
}



bool HalfSegment::RayAbove( const Point& p, double &abovey0 ) const
{
  const Coord& x = p.GetX(), y = p.GetY(),
               xl = GetLeftPoint().GetX(),
               yl = GetLeftPoint().GetY(),
               xr = GetRightPoint().GetX(),
               yr = GetRightPoint().GetY();

  if (xl!=xr)
  {
    if( x == xl && yl > y )
    {
      abovey0 = yl;
      return true;
    }
    else if( xl < x && x <= xr )
    {
      double k = (yr - yl) / (xr - xl),
             a = (yl - k * xl),
             y0 = k * x + a;
      Coord yy = y0;
      if( yy > y )
      {
        abovey0 = y0;
        return true;
      }
    }
  }
  return false;
}

typedef unsigned int outcode;
enum { TOP = 0x1, BOTTOM = 0x2, RIGHT = 0x4, LEFT = 0x8 };

outcode CompOutCode( double x, double y, double xmin,
                     double xmax, double ymin, double ymax)
{
  outcode code = 0;
  if (y > ymax)
    code |=TOP;
  else
    if (y < ymin)
      code |= BOTTOM;
  if ( x > xmax)
    code |= RIGHT;
  else
    if ( x < xmin)
      code |= LEFT;
  return code;
}

void HalfSegment::CohenSutherlandLineClipping( const Rectangle<2> &window,
                                               double &x0, double &y0,
                                               double &x1, double &y1,
                                               bool &accept ) const
{
  // Outcodes for P0, P1, and whatever point lies outside the clip rectangle*/
  outcode outcode0, outcode1, outcodeOut;
  double xmin = window.MinD(0)  , xmax = window.MaxD(0),
         ymin = window.MinD(1), ymax = window.MaxD(1);
  bool done = false;
  accept = false;

  outcode0 = CompOutCode( x0, y0, xmin, xmax, ymin, ymax);
  outcode1 = CompOutCode( x1, y1, xmin, xmax, ymin, ymax);

  do
  {
    if ( !(outcode0 | outcode1) )
    {
      //"Trivial accept and exit"<<endl;
      accept = true;
      done = true;
    }
    else if (outcode0 & outcode1)
    {
      done = true;
        //"Logical and is true, so trivial reject and exit"<<endl;
    }
    else
    {
      //Failed both tests, so calculate the line segment to clip:
      //from an outside point to an instersection with clip edge.
      double x,y;
      // At least one endpoint is outside the clip rectangle; pick it.
      outcodeOut = outcode0 ? outcode0 : outcode1;
      //Now find the intersection point;
      //use formulas y = y0 + slope * (x - x0), x = x0 + (1 /slope) * (y-y0).

      if (outcodeOut & TOP)
        //Divide the line at top of clip rectangle
      {
        x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
        y = ymax;
      }
      else if (outcodeOut & BOTTOM)
        //Divide line at bottom edge of clip rectangle
      {
        x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
        y = ymin;
      }
      else if (outcodeOut & RIGHT)
        //Divide line at right edge of clip rectangle
      {
        y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
        x = xmax;
      }
      else // divide lene at left edge of clip rectangle
      {
        y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
        x = xmin;
      }

      //Now we move outside point to intersection point to clip
      //and get ready for next pass
      if (outcodeOut == outcode0)
      {
        x0 = x;
        y0 = y;
        outcode0 = CompOutCode(x0, y0, xmin, xmax, ymin, ymax);
      }
      else
      {
        x1 = x;
        y1 = y;
        outcode1 = CompOutCode(x1, y1, xmin, xmax, ymin, ymax);
      }
    }
  }
  while ( done == false);
}

void HalfSegment::WindowClippingIn( const Rectangle<2> &window,
                                    HalfSegment &hsInside, bool &inside,
                                    bool &isIntersectionPoint,
                                    Point &intersectionPoint) const
{
  double x0 = GetLeftPoint().GetX(),
         y0 = GetLeftPoint().GetY(),
         x1 = GetRightPoint().GetX(),
         y1 = GetRightPoint().GetY();

  CohenSutherlandLineClipping(window, x0, y0, x1, y1, inside);
  isIntersectionPoint=false;

  if (inside)
  {
    Point lp, rp;
    lp.Set( x0, y0 );
    rp.Set( x1, y1 );

    if (lp==rp)
    {
      isIntersectionPoint = true;
      intersectionPoint=lp;
    }
    else
    {
      AttrType attr=this->GetAttr();
      hsInside.Set(true, rp, lp);
      hsInside.SetAttr(attr);
    }
  }
}

/*
6.2 List Representation

The list representation of a HalfSegment is

----  ( bool (lp rp))
----

where the bool value indicate whether the dominating point is the left point.

6.3 ~In~ and ~Out~ Functions

*/
ListExpr
OutHalfSegment( ListExpr typeInfo, Word value )
{
  HalfSegment* hs;
  hs = (HalfSegment*)(value.addr);

  Point lp = hs->GetLeftPoint(),
            rp = hs->GetRightPoint();

  return nl->TwoElemList(
           nl-> BoolAtom(hs->IsLeftDomPoint() ),
           nl->TwoElemList(
             OutPoint( nl->TheEmptyList(), SetWord( &lp ) ),
             OutPoint( nl->TheEmptyList(), SetWord( &rp) ) ) );
}

Word
InHalfSegment( const ListExpr typeInfo, const ListExpr instance,
               const int errorPos, ListExpr& errorInfo, bool& correct )
{
  if ( nl->ListLength( instance ) == 2 )
  {
    ListExpr first = nl->First(instance),
             second = nl->Second(instance),
             firstP, secondP;
    bool ldp;

    if( nl->IsAtom(first) && nl->AtomType(first) == BoolType )
    {
      ldp = nl->BoolValue(first);

      if( nl->ListLength(second) == 2 )
      {
        firstP = nl->First(second);
        secondP = nl->Second(second);
      }

      correct=true;
      Point *lp = (Point*)InPoint(nl->TheEmptyList(),
                                  firstP, 0, errorInfo, correct ).addr;
      if( correct && lp->IsDefined() )
      {
        Point *rp = (Point*)InPoint(nl->TheEmptyList(),
                                    secondP, 0, errorInfo, correct ).addr;
        if( correct && rp->IsDefined() && *lp != *rp )
        {
          HalfSegment *hs = new HalfSegment( ldp, *lp, *rp );
          delete lp;
          delete rp;
          return SetWord( hs );
        }
        delete rp;
      }
      delete lp;
    }
  }

  correct = false;
  return SetWord( Address(0) );
}

/*
7 Type Constructor ~line~

A ~line~ value is a set of halfsegments. In the external (nestlist) representation, a line value is
expressed as a set of segments. However, in the internal (class) representation, it is expressed
as a set of sorted halfsegments, which are stored as a PArray.

7.1 Implementation of the class ~line~

*/
void Line::StartBulkLoad()
{
  ordered = false;
}

/*
~EndBulkLoad~

Finishs the bulkload for a line. If this function is called,
both HalfSegments assigned to a segment of the line must be part
of this line.

The parameter ~sort~ can be set to __false__ if the Halfsegments are
already ordered using the HalfSegment order.

The parameter ~realminize~ can be set to __false__ if the line is
already realminized, meaning each pair of different Segments has
at most a common endpoint. Furthermore, the two halgsegments belonging
to a segment must have the same edge number. The edge numbers mut be
in Range [0..Size()-1]. HalfSegments belonging to different segments
must have different edge numbers.

Only change one of the parameters if you exacly know what you do.
Changing such parameters without fulifilling the conditions stated
above may construct invalid line representations which again may
produce a system crash within some operators.

*/

void Line::EndBulkLoad( bool sort /* = true */,
                        bool realminize /* = true */
                      ){
  if(sort){
    Sort();
  }
  if(Size()>0){
     if(realminize){
       DBArray<HalfSegment>* line2 = ::Realminize(line);
       const HalfSegment* hs;
       line2->Sort(HalfSegmentCompare);
       line.Clear();
       for(int i=0;i<line2->Size();i++){
         line2->Get(i,hs);
         line.Append(*hs);
       }
       line2->Destroy();
       delete line2;
     }
     SetPartnerNo();
  }
  computeComponents();
  TrimToSize();
}

Line& Line::operator=( const Line& l )
{
  assert( l.ordered );
  line.Clear();
  if( !l.IsEmpty() )
  {
    line.Resize( l.Size() );
    const HalfSegment *hs;
    for( int i = 0; i < l.Size(); i++ )
    {
      l.line.Get( i, hs );
      line.Append(*hs );
    }
  }

  bbox = l.bbox;
  length = l.length;
  noComponents = l.noComponents;
  ordered = true;
  currentHS = l.currentHS;

  return *this;
}

bool Line::operator==( const Line& l ) const
{
  assert( ordered && l.ordered );

  if( Size() != l.Size() )
    return false;

  if( IsEmpty() && l.IsEmpty() )
    return true;

  if( bbox != l.bbox )
    return false;

  object obj;
  status stat;
  SelectFirst_ll( *this, l, obj, stat );

  while( obj == both || obj == none )
  {
    if( stat == endboth )
      return true;

    SelectNext_ll( *this, l, obj, stat );
  }
  return false;
}

bool Line::operator!=( const Line& l ) const
{
  return !( *this == l);
}

Line& Line::operator+=( const HalfSegment& hs )
{
  if( IsEmpty() )
    bbox = hs.BoundingBox();
  else
    bbox = bbox.Union( hs.BoundingBox() );

  if( !IsOrdered() )
    line.Append( hs );
  else
  {
    int pos;
    if( !Find( hs, pos ) )
    {
      for( int i = line.Size() - 1; i >= pos; i++ )
      {
        const HalfSegment *auxhs;
        line.Get( i, auxhs );
        line.Put( i+1, *auxhs );
      }
      line.Put( pos, hs );
    }
  }
  return *this;
}

Line& Line::operator-=( const HalfSegment& hs )
{
  assert( IsOrdered() );

  int pos;
  const HalfSegment *auxhs;
  if( Find( hs, pos ) )
  {
    for( int i = pos; i < Size(); i++ )
    {
      line.Get( i+1, auxhs );
      line.Put( i, *auxhs );
    }
  }

  // Naive way to redo the bounding box.
  if( IsEmpty() )
    bbox.SetDefined( false );
  int i = 0;
  line.Get( i++, auxhs );
  bbox = auxhs->BoundingBox();
  for( ; i < Size(); i++ )
  {
    line.Get( i, auxhs );
    bbox = bbox.Union( auxhs->BoundingBox() );
  }

  return *this;
}

bool Line::Contains( const Point& p ) const
{
  if( IsEmpty() )
    return false;

  int pos;
  if( Find( p, pos ) )
    return true;

  if( pos >= Size() )
    return false;

  const HalfSegment *hs;
  for( ; pos >= 0; pos-- )
  {
    Get( pos, hs );
    if( hs->IsLeftDomPoint() )
    {
      if( hs->Contains( p ) )
        return true;
    }
  }
  return false;
}

bool Line::Intersects( const Line& l ) const
{
  assert( IsOrdered() && l.IsOrdered() );

  if( IsEmpty() && l.IsEmpty() )
    return true;

  if( IsEmpty() || l.IsEmpty() )
    return false;

  if( !BoundingBox().Intersects( l.BoundingBox() ) )
    return false;

  const HalfSegment *hs1, *hs2;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );
    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < l.Size(); j++ )
      {
        l.Get( j, hs2 );
        if (hs2->IsLeftDomPoint())
        {
          if( hs1->Intersects( *hs2 ) )
            return true;
        }
      }
    }
  }
  return false;
}

bool Line::Intersects( const Region& r ) const
{
  assert( IsOrdered() && r.IsOrdered() );

  if( IsEmpty() && r.IsEmpty() )
    return true;

  if( IsEmpty() || r.IsEmpty() )
    return false;

  if( !BoundingBox().Intersects( r.BoundingBox() ) )
    return false;

  const HalfSegment *hsl, *hsr;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hsl );
    if( hsl->IsLeftDomPoint() )
    {
      for( int j = 0; j < r.Size(); j++ )
      {
        r.Get( j, hsr );
        if( hsr->IsLeftDomPoint() )
        {
          if( hsl->Intersects( *hsr ) )
            return true;
        }
      }

      if( r.Contains( hsl->GetLeftPoint() ) ||
          r.Contains( hsl->GetRightPoint() ) )
        return true;
    }
  }
  return false;
}

bool Line::Inside( const Line& l ) const
{
  assert( IsOrdered() && l.IsOrdered() );

  if( IsEmpty() && l.IsEmpty() )
    return true;

  if( IsEmpty() || l.IsEmpty() )
    return false;

  if( !l.BoundingBox().Contains( bbox ) )
    return false;

  const HalfSegment *hs1, *hs2;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );
    if( hs1->IsLeftDomPoint() )
    {
      bool found = false;
      for( int j = 0; j < l.Size() && !found; j++ )
      {
        l.Get( j, hs2 );
        if( hs2->IsLeftDomPoint() && hs1->Inside( *hs2 ) )
          found = true;
      }
      if( !found )
        return false;
    }
  }
  return true;
}

bool Line::Inside( const Region& r ) const
{
  assert( IsOrdered() && r.IsOrdered() );

  if( IsEmpty() && r.IsEmpty() )
    return true;

  if( IsEmpty() || r.IsEmpty() )
    return false;

  if( !r.BoundingBox().Contains( bbox ) )
    return false;

  const HalfSegment *hsl;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hsl );
    if( hsl->IsLeftDomPoint() )
    {
      if( !r.Contains( *hsl ) )
        return false;
    }
  }
  return true;
}

bool Line::Adjacent( const Region& r ) const
{
  assert( IsOrdered() && r.IsOrdered() );

  if( IsEmpty() || r.IsEmpty() )
    return false;

  if( !BoundingBox().Intersects( r.BoundingBox() ) )
    return false;

  const HalfSegment *hsl, *hsr;
  bool found = false;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hsl );
    if( hsl->IsLeftDomPoint() )
    {
      if( r.InnerContains( hsl->GetLeftPoint() ) ||
          r.InnerContains( hsl->GetRightPoint() ) )
        return false;

      for( int j = 0; j < r.Size(); j++ )
      {
        r.Get( j, hsr );
        if( hsr->IsLeftDomPoint() )
        {
          if( !hsr->Intersects( *hsl ) )
            continue;

          found = true;

          if( hsr->Crosses( *hsl ) )
            return false;
        }
      }
    }
  }
  return found;
}

void Line::Intersection( const Line& l, Line& result ) const
{
  assert( IsOrdered() && l.IsOrdered() );
  result.Clear();

  if( IsEmpty() || l.IsEmpty() )
    return;

  const HalfSegment *hs1, *hs2;
  HalfSegment hs;
  result.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );

    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < l.Size(); j++ )
      {
        l.Get( j, hs2 );

        if( hs2->IsLeftDomPoint() && hs1->Intersection( *hs2, hs ) )
        {
          result += hs;
          hs.SetLeftDomPoint( !hs.IsLeftDomPoint() );
          result += hs;
        }
      }
    }
  }
  result.EndBulkLoad();
}

void Line::Crossings( const Line& l, Points& result ) const
{
  assert( IsOrdered() && l.IsOrdered() );
  result.Clear();

  if( IsEmpty() || l.IsEmpty() )
    return;

  const HalfSegment *hs1, *hs2;
  Point p;

  result.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );

    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < l.Size(); j++ )
      {
        l.Get( j, hs2 );

        if( hs2->IsLeftDomPoint() )
        {
          if( hs1->Intersection( *hs2, p ) )
            result += p;
        }
      }
    }
  }
  result.EndBulkLoad(true, true); // sort and remove duplicates
}

double Line::Distance( const Point& p ) const
{
  assert( IsOrdered() && !IsEmpty() && p.IsDefined() );

  const HalfSegment *hs;
  double result = numeric_limits<double>::max();

  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs );

    if( hs->IsLeftDomPoint() )
    {
      if( hs->Contains( p ) )
        return 0.0;

      result = MIN( result, hs->Distance( p ) );
    }
  }
  return result;
}

double Line::Distance( const Points& ps ) const
{
  assert( IsOrdered() && !IsEmpty() &&
          ps.IsOrdered() && !ps.IsEmpty() );

  const HalfSegment *hs;
  const Point *p;
  double result = numeric_limits<double>::max();

  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs );

    if( hs->IsLeftDomPoint() )
    {
      for( int j = 0; j < ps.Size(); j++ )
      {
        ps.Get( j, p );
        if( hs->Contains( *p ) )
          return 0;
        result = MIN( result, hs->Distance( *p ) );
      }
    }
  }
  return result;
}

double Line::Distance( const Line& l ) const
{
  assert( IsOrdered() && !IsEmpty() &&
          l.IsOrdered() && !l.IsEmpty() );

  const HalfSegment *hs1, *hs2;
  double result = numeric_limits<double>::max();

  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );

    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < l.Size(); j++ )
      {
        l.Get( j, hs2 );

        if( hs1->Intersects( *hs2 ) )
          return 0.0;

        result = MIN( result, hs1->Distance( *hs2 ) );
      }
    }
  }
  return result;
}

double Line::Distance( const Rectangle<2>& r ) const {
  if(!IsDefined() || IsEmpty() || !r.IsDefined()){
    return -1;
  }
  const HalfSegment* hs;
  double dist = numeric_limits<double>::max();
  for(int i=0; i < line.Size() && dist>0; i++){
     line.Get(i,hs);
     if(hs->IsLeftDomPoint()){
       double d = hs->Distance(r);
       if(d<dist){
          dist = d;
       }
     }
  }
  return dist;
}


int Line::NoComponents() const
{
  return noComponents;
}

void Line::Translate( const Coord& x, const Coord& y, Line& result ) const
{
  assert( IsOrdered() );

  result = *this;

  const HalfSegment *hs;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs );

    HalfSegment auxhs( *hs );
    auxhs.Translate( x, y );
    result.Put(i,auxhs);
  }
}

void Line::Rotate( const Coord& x, const Coord& y,
                   const double alpha,
                   Line& result ) const
{
  result.Clear();
  if(!IsDefined()){
     result.SetDefined(false);
     return;
  }
  result.Resize(Size());
  result.SetDefined(true);

  double s = sin(alpha);
  double c = cos(alpha);

  double m00 = c;
  double m01 = -s;
  double m02 = x - x*c + y*s;
  double m10 = s;
  double m11 = c;
  double m12 = y - x*s-y*c;


  result.StartBulkLoad();
  const HalfSegment *hso;
  Point p1;
  Point p2;

  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hso );
    p1.Set( m00*hso->GetLeftPoint().GetX()
            + m01*hso->GetLeftPoint().GetY() + m02,
            m10*hso->GetLeftPoint().GetX()
           + m11*hso->GetLeftPoint().GetY() + m12);
    p2.Set( m00*hso->GetRightPoint().GetX()
             + m01*hso->GetRightPoint().GetY() + m02,
             m10*hso->GetRightPoint().GetX()
             + m11*hso->GetRightPoint().GetY() + m12);

    HalfSegment hsr(*hso); // ensure to copy attr;
    hsr.Set(hso->IsLeftDomPoint(),p1,p2);
    result += hsr;
  }
  result.EndBulkLoad(); // reordering may be required

}

void Line::Transform( Region& result ) const
{
  assert( IsOrdered() );

  const HalfSegment *hs;
  result.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs );

    HalfSegment auxhs( *hs );
    result += auxhs;
  }
  result.SetNoComponents( NoComponents() );
  //result.EndBulkLoad( false, false, false, false );
  result.EndBulkLoad();
}



/*
Simple function marking some elements between min and  max as used to avoid
segments degenerated to single points.

*/
static double maxDist(vector<Point>& orig, // points
                      int min, int max, // range
                      int& index){  // resultindex

  // search the point with the largest distance to the segment between
  // orig[min] and orig[max]
  double maxdist = 0.0;
  int maxindex = -1;

  try{
    if(!AlmostEqual(orig.at(min),orig.at(max))){
      HalfSegment hs(true,orig.at(min),orig.at(max));
      for(int i=min+1; i<max; i++){
        double dist = hs.Distance(orig.at(i));
        if(dist>maxdist){
          maxdist = dist;
          maxindex = i;
        }
      }
   } else { // special case of a cycle
      Point p = orig[min];
      for(int i=min+1; i<max; i++){
        double dist = p.Distance(orig.at(i));
        if(dist>maxdist){
          maxdist = dist;
          maxindex = i;
        }
      }
    }
    index = maxindex;
    return maxdist;
  } catch (out_of_range){
      cerr << "min=" << min << " max=" << max << " size="
           << orig.size() << endl;
      assert(false);
  }
}


/*
Implementation of the Douglas Peucker algorithm.

The return indoicates whether between min and max further points was
used.

*/

static bool douglas_peucker(vector<Point>& orig, // original line
                       const double epsilon, // maximum derivation
                       bool* use, // result
                       int min, int max,
                       bool force = false){ // current range
// always use the endpoints
use[min] = true;
use[max] = true;
if(min+1>=max){ // no inner points, nothing to do
  return false;
}
 int index;
 double maxdist = maxDist(orig,min,max,index);
 bool cycle = AlmostEqual(orig[min], orig[max]);
 if((maxdist<=epsilon) &&  // line closed enough
     !cycle &&  // no degenerated segment
     !force){
       return false; // all ok, stop recursion
   } else {
     bool ins = douglas_peucker(orig,epsilon,use,min,index,cycle);
     if(index>=0){
        douglas_peucker(orig,epsilon,use,index,max,cycle && !ins);
     }
     return true;
   }
}



static void  douglas_peucker(vector<Point>& orig, // original chain of points
                             const double epsilon, // maximum derivation
                             bool* use){ // result
  for(unsigned int i=0;i<orig.size();i++){
     use[i] = false;
  }
  // call the recursive implementation
  douglas_peucker(orig,epsilon, use, 0, orig.size()-1);
}



void Line::Simplify(Line& result, const double epsilon,
                    const Points& importantPoints /*= Points(0)*/ ) const{
   result.Clear(); // remove old stuff

   if(!IsDefined()){ // this is not defined
      result.SetDefined(false);
      return;
   }
   result.SetDefined(true);
   if(epsilon<=0){ // maximum derivation reached in each case
       result.CopyFrom(this);
       return;
   }
   // at least one immediate point is required to simplify
   // the line, thus at leat 4 halfsegments are needed.
   if(Size()<4){
      result.CopyFrom(this);
      return;
   }
   // an array with the used halfsegments
   bool used[Size()];
   for(int i=0;i<Size();i++){
      used[i] = false;
   }

   vector<Point> forward;
   vector<Point> backward;
   vector<Point> complete;

   const HalfSegment* hs; // current halfsegment

   result.StartBulkLoad();
   int pos = 0;
   int size = Size();
   int egdeno=0;
   while(pos<size){
    // skip all halfsegments in prior runs
    while(pos<size && used[pos]){
      pos++;
    }

    if(pos<size){
       // unused halfsegment found
       forward.clear();
       backward.clear();
       int next = pos;
       // trace the polyline until it ends or critical point is reached
       bool done = false;
       while(!done){
          used[next]= true;
          Get(next,hs);
          forward.push_back(hs->GetDomPoint());
          int partner = hs->attr.partnerno;
          used[partner] = true;
          Get(partner,hs);
          Point p = hs->GetDomPoint();
          if(importantPoints.Contains(p)){
            done = true;
          }else {
             int s = max(0,partner-2);
             int e = min(partner+3,Size());
             int count =0;
             const HalfSegment* tmp;

             // search around partner for segments with an
             // equal dominating point.
            for(int k=s; (k<e) && (count < 2); k++){
               if(k!=partner){
                  Get(k,tmp);
                  Point p2 = tmp->GetDomPoint();
                  if(AlmostEqual(p,p2)){
                     count++;
                     next = k;
                  }
               }
             }
             done = (count != 1) || used[next];
          }
       }
       forward.push_back(hs->GetDomPoint());

       // check possible enlargement into the other direction
       next = pos; // start again at pos
       done = false;
       do{
          Get(next,hs);
          Point p = hs->GetDomPoint();
          // check whether next has an unique connected segment
          int s = max(0, next-2);
          int e = min(next+3,Size());
          int count = 0;
          const HalfSegment* tmp;
          int partner = 0;
          // search around next
          if(importantPoints.Contains(p)){
            done = true;
          } else {
             for(int k=s; (k<e) && (count <2); k++){
                if(k!=next){
                   Get(k,tmp);
                   Point p2 = tmp->GetDomPoint();
                   if(AlmostEqual(p,p2)){
                     count++;
                     partner = k;
                   }
                }
             }
             done = (count!=1) || used[partner];
             if(!done){ // extension found
                 used[partner] = true;
                 Get(partner,hs);
                 next = hs->attr.partnerno;
                 used[next] = true;
                 Get(next,hs);
                 backward.push_back(hs->GetDomPoint());
             }
         }
       } while(!done);
       // connect backward and forward into complete
       complete.clear();
       for(int i=backward.size()-1; i>=0; i--){
         complete.push_back(backward[i]);
       }
       for(unsigned int i=0;i<forward.size(); i++){
         complete.push_back(forward[i]);
       }


       // determine all segments to use and copy them into the result
       bool use[complete.size()];
       douglas_peucker(complete,epsilon,use);
       int size = complete.size();
       Point last = complete[0];
       for(int i=1;i<size;i++){
          if(use[i]){
             Point p = complete[i];
             HalfSegment nhs(true,last,p);
             nhs.attr.edgeno=egdeno;
             result += nhs;
             nhs.SetLeftDomPoint(false);
             result += nhs;
             egdeno++;
             last = p;
          }
       }

     }
   }
   result.EndBulkLoad();
}


void Line::Realminize(){
   // special case: empty line
  if(!IsDefined()){
    line.Clear();
     return;
  }

  Line tmp(0);
  Realminize2(*this,tmp);
  *this = tmp;
}

void Line::Vertices( Points* result ) const
{
  assert( IsOrdered() );
  result->Clear();

  if(!IsDefined()){
    result->SetDefined(false);
    return;
  }
  result->SetDefined(true);
  if( IsEmpty() ){
    return;
  }

  const HalfSegment* hs;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs );
    *result += hs->GetDomPoint();
  }
  result->EndBulkLoad( false, true );
}

void Line::Boundary(Points* result) const{
  // we assume that the interior of each halfsegment has no
  // common point with any part of another halfsegment
  result->Clear();
  if(!IsDefined()){
     result->SetDefined(false);
     return;
  }
  if(IsEmpty()){
    return;
  }
  const HalfSegment* hs;
  const HalfSegment* hs_n; // neighbooring halfsegment
  Point p;
  int size = Size();
  result->StartBulkLoad();
  for(int i=0;i<size;i++){
     bool common = false;
     Get(i,hs);
     p=hs->GetDomPoint();
     if(i>0){
       Get(i-1,hs_n);
       if(p==hs_n->GetDomPoint()){
          common=true;
       }
     }
     if(i<size-1){
       Get(i+1,hs_n);
       if(p==hs_n->GetDomPoint()){
          common=true;
       }
     }
     if(!common){
        *result += p;
     }
  }
  result->EndBulkLoad(false,false);

}


bool Line::Find( const HalfSegment& hs, int& pos, const bool& exact ) const
{
  assert( IsOrdered() );
  if( exact )
    return line.Find( &hs, HalfSegmentCompare, pos );
  return line.Find( &hs, HalfSegmentCompare, pos );
}

bool Line::Find( const Point& p, int& pos, const bool& exact ) const
{
  assert( IsOrdered() );
  if( exact )
    return line.Find( &p, PointHalfSegmentCompare, pos );
  return line.Find( &p, PointHalfSegmentCompareAlmost, pos );
}

void Line::SetPartnerNo() {
 if(line.Size()==0){
   return;
 }
 DBArray<int> TMP((line.Size()+1)/2);

 const HalfSegment* hs1;
 const HalfSegment* hs2;
 for(int i=0; i<line.Size(); i++){
    line.Get(i,hs1);
    if(hs1->IsLeftDomPoint()){
      TMP.Put(hs1->attr.edgeno, i);
    } else {
      const int* lpp;
      TMP.Get(hs1->attr.edgeno,lpp);
      int leftpos = *lpp;
      HalfSegment right = *hs1;
      right.attr.partnerno = leftpos;
      right.attr.insideAbove = false;
      right.attr.coverageno = 0;
      right.attr.cycleno = 0;
      right.attr.faceno = 0;
      line.Get(leftpos,hs2);
      HalfSegment left = *hs2;
      left.attr.partnerno = i;
      left.attr.insideAbove = false;
      left.attr.coverageno = 0;
      left.attr.cycleno = 0;
      left.attr.faceno = 0;
      line.Put(i,right);
      line.Put(leftpos,left);
     }
    }
    TMP.Destroy();
}

bool Line::GetNextSegment( const int poshs, const HalfSegment& hs,
                           int& posnexths, const HalfSegment*& nexths )
{
  if( poshs > 0 )
  {
    Get( posnexths = poshs - 1, nexths );
    if( hs.GetDomPoint() == nexths->GetDomPoint() )
      return true;
  }

  if( poshs < Size() - 1 )
  {
    Get( posnexths = poshs + 1, nexths );
    if( hs.GetDomPoint() == nexths->GetDomPoint() )
      return true;
  }

  return false;
}

bool Line::GetNextSegments( const int poshs, const HalfSegment& hs,
                            vector<bool>& visited,
                            int& posnexths, const HalfSegment*& nexths,
                            stack< pair<int, const HalfSegment*> >& nexthss )
{
  bool first = true;
  const HalfSegment *aux;

  int auxposhs = poshs;
  while( auxposhs > 0 )
  {
    auxposhs--;
    if( !visited[auxposhs] )
    {
      Get( auxposhs, aux );
      if( hs.GetDomPoint() == aux->GetDomPoint() )
      {
        if( first )
        {
          first = false;
          nexths = aux;
          posnexths = auxposhs;
        }
        else
          nexthss.push( make_pair( auxposhs, aux ) );
      }
      else
        break;
    }
  }

  auxposhs = poshs;
  while( auxposhs < Size() - 1 )
  {
    auxposhs++;
    if( !visited[auxposhs] )
    {
      Get( auxposhs, aux );
      if( hs.GetDomPoint() == aux->GetDomPoint() )
      {
        if( first )
        {
          first = false;
          nexths = aux;
          posnexths = auxposhs;
        }
        else
          nexthss.push( make_pair( auxposhs, aux ) );
      }
      else
        break;
    }
  }

  return !first;
}

/*
~computeComponents~

Computes FaceNo, edgeno of each halfsegment.
Sets length,noComponents, and bbox of the line.


*/

int Line::getUnusedExtension(int startPos,const DBArray<bool>& used)const{
  const HalfSegment* hs;
  line.Get(startPos,hs);
  Point p = hs->GetDomPoint();
  int pos = startPos-1;
  bool done = false;
  const bool* u;
  // search on the left side
  while(pos>=0 && !done){
     line.Get(pos,hs);
     Point p2 = hs->GetDomPoint();
     if(!AlmostEqual(p,p2)){
       done = true;
     }else {
       used.Get(pos,u);
       if(!*u){
         return pos;
       } else {
         pos--;
       }
     }
  }
  // search on the right side
  done = false;
  pos = startPos+1;
  int size = line.Size();
  while(!done && pos<size){
     line.Get(pos,hs);
     Point p2 = hs->GetDomPoint();
     if(!AlmostEqual(p,p2)){
       done = true;
     } else {
       used.Get(pos,u);
       if(!*u){
         return pos;
       } else {
        pos++;
       }
     }
  }
  return -1;
}

void Line::collectFace(int faceno, int startPos, DBArray<bool>& used){
  set<int> extensionPos;

  used.Put(startPos,true);
  const HalfSegment* hs1;
  const HalfSegment* hs2;

  int pos = startPos;
  line.Get(startPos,hs1);
  HalfSegment Hs1 = *hs1;
  int edgeno = 0;
  Hs1.attr.insideAbove=false;
  Hs1.attr.coverageno = 0;
  Hs1.attr.cycleno=0;
  Hs1.attr.faceno=faceno;
  Hs1.attr.edgeno = edgeno;
  line.Put(pos,Hs1);
  used.Put(pos,true);

  // get and Set the Partner
  int partner = Hs1.attr.partnerno;
  line.Get(partner,hs2);
  HalfSegment Hs2 = *hs2;
  Hs2.attr.insideAbove=false;
  Hs2.attr.coverageno = 0;
  Hs2.attr.cycleno=0;
  Hs2.attr.faceno=faceno;
  Hs2.attr.edgeno = edgeno;
  used.Put(partner,true);
  line.Put(partner,Hs2);

  if(!bbox.IsDefined()){
    bbox = hs1->BoundingBox();
  } else {
    bbox = bbox.Union(hs1->BoundingBox());
  }
  length += hs1->Length();

  if(getUnusedExtension(pos,used)>=0){
     extensionPos.insert(pos);
  }
  if(getUnusedExtension(partner,used)>=0){
     extensionPos.insert(partner);
  }

  edgeno++;
  while(!extensionPos.empty()){

    int spos =  *(extensionPos.begin());
    pos = getUnusedExtension(spos,used);
    if(pos < 0){
      extensionPos.erase(spos);
    } else { // extension found at position pos
      line.Get(pos,hs1);
      Hs1 = (*hs1);
      Hs1.attr.insideAbove=false;
      Hs1.attr.coverageno = 0;
      Hs1.attr.cycleno=0;
      Hs1.attr.faceno=faceno;
      Hs1.attr.edgeno = edgeno;
      used.Put(pos,true);
      line.Put(pos,Hs1);

      partner = Hs1.attr.partnerno;
      line.Get(partner,hs2);
      Hs2 = (*hs2);
      Hs2.attr.insideAbove=false;
      Hs2.attr.coverageno = 0;
      Hs2.attr.cycleno=0;
      Hs2.attr.faceno=faceno;
      Hs2.attr.edgeno = edgeno;
      used.Put(partner,true);
      line.Put(partner,Hs2);
      if(getUnusedExtension(partner,used)>=0){
        extensionPos.insert(partner);
      }
      length += hs1->Length();
      bbox = bbox.Union(hs1->BoundingBox());
      edgeno++;
    }
  }
}

/*
~ComputeComponents~

Computes the length of this lines as well as its bounding box and the number
of components of this line. Each Halfsegment is assigned to a face number
(the component) and an egde number within this face.



*/

void Line::computeComponents() {
  length = 0.0;
  noComponents = 0;
  bbox.SetDefined(false);

  if(Size()==0){
    return;
  }

  DBArray<bool> used(line.Size());

  for(int i=0;i<line.Size();i++){
    used.Append(false);
  }

  int faceno = 0;

  for(int i=0;i<line.Size();i++){
    const bool* u;
    used.Get(i,u);
    if(!(*u)){ // an unused halfsegment
      collectFace(faceno,i,used);
      faceno++;
    }
  }
  noComponents = faceno;
}

void Line::Sort()
{
  assert( !IsOrdered() );
  line.Sort( HalfSegmentCompare );
  ordered = true;
}

void Line::RemoveDuplicates()
{
  assert( IsOrdered() );
  int size = line.Size();
  if( size == 0 ){
   // nothing to do
     return;
  }

  int newEdgeNumbers[size]; // mapping oldnumber -> newnumber
  for(int i=0;i<size;i++){
     newEdgeNumbers[i] = -1;
  }
  int newedge = 0;  // next unused edge number

  HalfSegment tmp;
  HalfSegment last;
  const HalfSegment* ptmp;
  int pos = 0;
  line.Get( 0, ptmp );
  last = *ptmp;

  newEdgeNumbers[last.attr.edgeno] = newedge;
  last.attr.edgeno = newedge;
  line.Put(0,last);
  newedge++;

  for( int i = 1; i < size; i++ )
  {
    line.Get( i, ptmp );
    tmp = *ptmp;
    int edge = tmp.attr.edgeno;
    if( last != tmp )
      // new segment found
    {
      pos++;
      if(newEdgeNumbers[edge]<0){ // first
         newEdgeNumbers[edge] = newedge;
         tmp.attr.edgeno = newedge;
         newedge++;
      } else {
         tmp.attr.edgeno = newEdgeNumbers[edge];
      }
      line.Put( pos, tmp );
      last = tmp;
    } else { // duplicate found
      newEdgeNumbers[edge] = newedge-1; // use the last edge number
    }
  }
  if( pos + 1 != size ){
    // duplicates found
    line.Resize( pos + 1 );
  }
}

void Line::WindowClippingIn( const Rectangle<2> &window,
                             Line &clippedLine,
                             bool &inside ) const
{
  inside = false;
  clippedLine.StartBulkLoad();
  for (int i=0; i < Size();i++)
  {
    const HalfSegment *hs;
    HalfSegment hsInside;
    bool insidehs = false,
         isIntersectionPoint = false;
    Get( i, hs );

    if( hs->IsLeftDomPoint() )
    {
      Point intersectionPoint;
      hs->WindowClippingIn( window, hsInside, insidehs,
                            isIntersectionPoint,
                            intersectionPoint );
      if( insidehs && !isIntersectionPoint )
      {
        clippedLine += hsInside;
        hsInside.SetLeftDomPoint( !hsInside.IsLeftDomPoint() );
        clippedLine += hsInside;
        inside = true;
      }
    }
  }
  clippedLine.EndBulkLoad();
}

void Line::WindowClippingOut( const Rectangle<2> &window,
                              Line &clippedLine,
                              bool &outside ) const
{
  outside = false;
  clippedLine.StartBulkLoad();
  for (int i=0; i < Size();i++)
  {
    const HalfSegment *hs;
    HalfSegment hsInside;
    bool outsidehs = false,
         isIntersectionPoint = false;
    Get( i, hs );

    if( hs->IsLeftDomPoint() )
    {
      Point intersectionPoint;
      hs->WindowClippingIn( window, hsInside, outsidehs,
                            isIntersectionPoint,
                            intersectionPoint );
      if( outsidehs && !isIntersectionPoint )
      {
        if( hs->GetLeftPoint() != hsInside.GetLeftPoint() )
        {//Add the part of the half segment composed by the left
         // point of hs and the left point of hsInside.
          HalfSegment hsLeft( true,
                              hs->GetLeftPoint(),
                              hsInside.GetLeftPoint() );
          AttrType attr = hs->GetAttr();
          hsLeft.SetAttr(attr);
          clippedLine += hsLeft;
          hsLeft.SetLeftDomPoint( !hsLeft.IsLeftDomPoint() );
          clippedLine += hsLeft;
          outside = true;
        }
        if( hs->GetRightPoint() != hsInside.GetRightPoint() )
        {//Add the part of the half segment composed by the left
         // point of hs and the left point of hsInside.
          HalfSegment hsRight( true,
                               hs->GetRightPoint(),
                               hsInside.GetRightPoint() );
          AttrType attr=hs->GetAttr();
          hsRight.SetAttr(attr);
          clippedLine += hsRight;
          hsRight.SetLeftDomPoint( !hsRight.IsLeftDomPoint() );
          clippedLine += hsRight;
          outside = true;
        }
      }
      else
      {
        HalfSegment auxhs = *hs;
        clippedLine += auxhs;
        auxhs.SetLeftDomPoint( !auxhs.IsLeftDomPoint() );
        clippedLine += auxhs;
        outside = true;
      }
    }
  }
  clippedLine.EndBulkLoad();
}


ostream& operator<<( ostream& os, const Line& cl )
{
  os << "<";
  for( int i = 0; i < cl.Size(); i++ )
  {
    const HalfSegment *hs;
    cl.Get( i, hs );
    os << " " << *hs << endl;
  }
  os << ">";
  return os;
}

size_t Line::HashValue() const
{
  if( IsEmpty() )
    return 0;

  size_t h = 0;

  const HalfSegment *hs;
  Coord x1, y1, x2, y2;

  for( int i = 0; i < Size() && i < 5; i++ )
  {
    Get( i, hs );
    x1 = hs->GetLeftPoint().GetX();
    y1 = hs->GetLeftPoint().GetY();
    x2 = hs->GetRightPoint().GetX();
    y2 = hs->GetRightPoint().GetY();

    h += (size_t)( (5 * x1 + y1) + (5 * x2 + y2) );
  }
  return h;
}

void Line::Clear()
{
  line.Clear();
  pos = -1;
  ordered = true;
  bbox.SetDefined( false );
}

void Line::CopyFrom( const StandardAttribute* right )
{
  Clear();
  *this = *((const Line *)right);
}

int Line::Compare( const Attribute *arg ) const
{
  const Line &l = *((const Line*)arg);

  if( Size() > l.Size() )
    return 1;
  if( Size() < l.Size() )
    return -1;

  object obj;
  status stat;
  SelectFirst_ll( *this, l, obj, stat );

  while( stat == endnone )
  {
    if( obj == first )
      return -1;
    if( obj == second )
      return 1;

    SelectNext_ll( *this, l, obj, stat );
  }
  return 0;
}

Line* Line::Clone() const
{
  return new Line( *this );
}

ostream& Line::Print( ostream &os ) const
{
  ios_base::fmtflags oldOptions = os.flags();
  os.setf(ios_base::fixed,ios_base::floatfield);
  os.precision(8);
  os << *this;
  os.flags(oldOptions);
  return os;

}

/*
7.2 List Representation

The list representation of a line is

----  ((x1 y1 x2 y2) (x1 y1 x2 y2) ....)
----

7.3 ~Out~-function

*/
ListExpr
OutLine( ListExpr typeInfo, Word value )
{
  ListExpr result, last;
  const HalfSegment *hs;
  ListExpr halfseg, halfpoints, flatseg;
  Line* l = (Line*)(value.addr);

  if( l->IsEmpty() )
    return nl->TheEmptyList();

  result = nl->TheEmptyList();
  last = result;
  bool first = true;

  for( int i = 0; i < l->Size(); i++ )
  {
    l->Get( i, hs );
    if( hs->IsLeftDomPoint() == true )
    {
      halfseg = OutHalfSegment( nl->TheEmptyList(), SetWord( (void*)hs ) );
      halfpoints = nl->Second( halfseg );
      flatseg = nl->FourElemList(
                  nl->First( nl->First( halfpoints ) ),
                  nl->Second( nl->First( halfpoints ) ),
                  nl->First( nl->Second( halfpoints ) ),
                  nl->Second( nl->Second( halfpoints ) ) );
      if( first == true )
      {
        result = nl->OneElemList( flatseg );
        last = result;
        first = false;
      }
      else
        last = nl->Append( last, flatseg );
    }
  }
  return result;
}

/*
7.4 ~In~-function

*/
Word
InLine( const ListExpr typeInfo, const ListExpr instance,
        const int errorPos, ListExpr& errorInfo, bool& correct )
{
  Line* l = new Line( 0 );
  HalfSegment * hs;
  l->StartBulkLoad();
  ListExpr first, halfseg, halfpoint;
  ListExpr rest = instance;
  int edgeno = 0;

  if( !nl->IsAtom( instance ) )
  {
    while( !nl->IsEmpty( rest ) )
    {
      first = nl->First( rest );
      rest = nl->Rest( rest );

      if( nl->ListLength( first ) == 4 )
      {
        halfpoint = nl->TwoElemList(
                      nl->TwoElemList(
                        nl->First(first),
                        nl->Second(first)),
                      nl->TwoElemList(
                        nl->Third(first),
                        nl->Fourth(first)));
      } else { // wrong list representation
         delete l;
         correct = false;
         return SetWord( Address(0) );
      }
      halfseg = nl->TwoElemList(nl->BoolAtom(true), halfpoint);
      hs = (HalfSegment*)InHalfSegment( nl->TheEmptyList(), halfseg,
                                        0, errorInfo, correct ).addr;
      if( correct )
      {
        hs->attr.edgeno = edgeno++;
        *l += *hs;
        hs->SetLeftDomPoint( !hs->IsLeftDomPoint() );
        *l += *hs;
      }
      delete hs;
    }
    l->EndBulkLoad();

    correct = true;
    return SetWord( l );
  }
  delete l;
  correct = false;
  return SetWord( Address(0) );
}

/*
7.5 ~Create~-function

*/
Word
CreateLine( const ListExpr typeInfo )
{
  return SetWord( new Line( 0 ) );
}

/*
7.6 ~Delete~-function

*/
void
DeleteLine( const ListExpr typeInfo, Word& w )
{
  Line *l = (Line *)w.addr;
  l->Destroy();
  l->DeleteIfAllowed();
  w.addr = 0;
}

/*
7.7 ~Close~-function

*/
void
CloseLine( const ListExpr typeInfo, Word& w )
{
  ((Line *)w.addr)->DeleteIfAllowed();
  w.addr = 0;
}

/*
7.8 ~Clone~-function

*/
Word
CloneLine( const ListExpr typeInfo, const Word& w )
{
  return SetWord( new Line( *((Line *)w.addr) ) );
}

/*
7.8 ~Open~-function

*/
bool
OpenLine( SmiRecord& valueRecord,
          size_t& offset,
          const ListExpr typeInfo,
          Word& value )
{
  Line *l = (Line*)Attribute::Open( valueRecord, offset, typeInfo );
  value = SetWord( l );
  return true;
}

/*
7.8 ~Save~-function

*/
bool
SaveLine( SmiRecord& valueRecord,
          size_t& offset,
          const ListExpr typeInfo,
          Word& value )
{
  Line *l = (Line *)value.addr;
  Attribute::Save( valueRecord, offset, typeInfo, l );
  return true;
}

/*
7.9 ~SizeOf~-function

*/
int SizeOfLine()
{
  return sizeof(Line);
}

/*
7.11 Function describing the signature of the type constructor

*/
ListExpr
LineProperty()
{
  return nl->TwoElemList(
           nl->FourElemList(
             nl->StringAtom("Signature"),
             nl->StringAtom("Example Type List"),
             nl->StringAtom("List Rep"),
             nl->StringAtom("Example List")),
           nl->FourElemList(
             nl->StringAtom("-> DATA"),
             nl->StringAtom("line"),
             nl->StringAtom("(<segment>*) where segment is "
               "(<x1><y1><x2><y2>)"),
             nl->StringAtom("( (1 1 2 2)(3 3 4 4) )")));
}

/*
7.12 Kind checking function

This function checks whether the type constructor is applied correctly. Since
type constructor ~line~ does not have arguments, this is trivial.

*/
bool
CheckLine( ListExpr type, ListExpr& errorInfo )
{
  return nl->IsEqual( type, "line" );
}

/*
7.13 ~Cast~-function

*/
void* CastLine(void* addr)
{
  return Line::Cast(addr);
}

/*
7.14 Creation of the type constructor instance

*/
TypeConstructor line(
        "line",                         //name
        LineProperty,                   //describing signature
        OutLine,        InLine,         //Out and In functions
        0,              0,              //SaveTo and RestoreFrom List functions
        CreateLine,     DeleteLine,     //object creation and deletion
        OpenLine,       SaveLine,       // object open and save
        CloseLine,      CloneLine,      //object close and clone
        CastLine,                       //cast function
        SizeOfLine,                     //sizeof function
        CheckLine );                    //kind checking function


/*
7 The type SimpleLine

7.1 Constructor

This constructor coinstructs a simple line from ~src~

If ~src~ is not simple, the simple line will be invalidated, i.e.
the defined flag is set to false;

*/
SimpleLine::SimpleLine(const Line& src):segments(0),lrsArray(0){
  fromLine(src);
  del.refs = 1;
  del.isDelete = true;
}

/*
7.2 Bulk Loading Functions

*/
void SimpleLine::StartBulkLoad(){
  isOrdered = false;
  SetDefined( true );
}

/*
~Operator +=~

Appends an HalfSegment during BulkLoading.

*/
SimpleLine& SimpleLine::operator+=(const HalfSegment& hs){
  assert(!isOrdered && IsDefined());
  segments.Append(hs);
  return *this;
}

bool SimpleLine::EndBulkLoad(){
  // Sort the segments
  Sort();
  // Realminize the segments
  DBArray<HalfSegment>* tmp;
  tmp = Realminize(segments);
  segments.Clear();
  //segments = *tmp; // does not work correctly
  const HalfSegment* hs;
  for(int i=0;i<tmp->Size();i++){
     tmp->Get(i,hs);
     segments.Append(*hs);
  }
  //tmp->Destroy();
  delete tmp;
  SetPartnerNo();

  // recompute Bounding box;
  if(segments.Size()>0){
    const HalfSegment* hs;
    segments.Get(0,hs);
    bbox = hs->BoundingBox();
    for(int i=1; i< segments.Size();i++){
      segments.Get(i,hs);
      bbox = bbox.Union(hs->BoundingBox());
    }
  }else{
    bbox.SetDefined(false);
  }

  if(!computePolyline()){
     segments.Clear();
     lrsArray.Clear();
     SetDefined(false);
     return false;
  } else {
     TrimToSize();
     return true;
  }
}

/*
~StartPoint~

Determines the startPoint of this simple line.

*/
Point SimpleLine::StartPoint( bool startsSmaller ) const {
  if( IsEmpty() || !IsDefined() ){
    return Point( false );
  }

  // Find out which end should be the start of the line. This
  // depends on the orientation of the curve and the parameter
  // to this function.
  bool startPointSmaller = startsSmaller && this->startSmaller;
  int pos;
  if( startPointSmaller ) {
    // Start is at the smaller end of the array
    pos = 0;
   } else {
    // Start is at the bigger end of the array
    pos = lrsArray.Size()-1;
   }

   // Read entry from linear referencing system.
   const LRS *lrs;
   lrsArray.Get( pos, lrs );
   // Get half-segment
   const HalfSegment* hs;
   segments.Get( lrs->hsPos, hs );

   // Return one end of the half-segment depending
   // on the start.
   return startPointSmaller ?  hs->GetDomPoint() : hs->GetSecPoint();
}

/*
~EndPoint~

Returns the endpoint of this simple Line.

*/
Point SimpleLine::EndPoint( bool startsSmaller ) const {
  // The end is opposite to the start.
  return StartPoint(!startsSmaller);
}

bool SimpleLine::Contains( const Point& p ) const {
 if( IsEmpty()  || !IsDefined() ){
   return false;
 }
 int pos;
 if( segments.Find( &p, PointHalfSegmentCompareAlmost, pos )){
   // p is a dominating point of a line
   return true;
 }
 if( pos >= Size() ){
    return false;
 }
 const HalfSegment *hs;
 for( ; pos >= 0; pos-- ){
   segments.Get( pos, hs );
   if( hs->IsLeftDomPoint() ) {
     if( hs->Contains( p ) ){
       return true;
     }
   }
 }
 return false;
}

double SimpleLine::Distance(const Point& p)const {
  const HalfSegment *hs;
  double result = numeric_limits<double>::max();
  for( int i = 0; i < Size(); i++ ) {
    Get( i, hs );
    if( hs->IsLeftDomPoint() ) {
      if( hs->Contains( p ) ){
        return 0.0;
      }
      result = MIN( result, hs->Distance( p ) );
    }
  }
  return result;
}


double SimpleLine::Distance(const Points& ps)const{
  const HalfSegment *hs;
  const Point *p;
  double result = numeric_limits<double>::max();

  for( int i = 0; i < Size(); i++ ) {
    Get( i, hs );

    if( hs->IsLeftDomPoint() ) {
      for( int j = 0; j < ps.Size(); j++ ) {
        ps.Get( j, p );
        if( hs->Contains( *p ) ){
          return 0;
        }
        result = MIN( result, hs->Distance( *p ) );
      }
    }
  }
  return result;
}

double SimpleLine::Distance(const SimpleLine& sl)const{
  const HalfSegment *hs1, *hs2;
  double result = numeric_limits<double>::max();

  for( int i = 0; i < Size(); i++ ) {
    Get( i, hs1 );

    if( hs1->IsLeftDomPoint() ) {
      for( int j = 0; j < sl.Size(); j++ ) {
        sl.Get( j, hs2 );

        if( hs1->Intersects( *hs2 ) ){
          return 0.0;
        }
        result = MIN( result, hs1->Distance( *hs2 ) );
      }
    }
  }
  return result;
}

double SimpleLine::Distance(const Rectangle<2>& r)const{
  Line sll(0);
  toLine(sll);
  return sll.Distance( r );
}

bool SimpleLine::AtPosition( double pos, bool startsSmaller, Point& p ) const {
 if(!IsDefined()){
    return false;
 }
 if( startsSmaller != this->startSmaller ){
   pos = length - pos;
 }
 LRS lrs( pos, 0 );
 int lrsPos;
 if( !Find( lrs,lrsPos ) ){
    return false;
 }

 const LRS *lrs2;
 lrsArray.Get( lrsPos, lrs2 );

 const HalfSegment *hs;
 segments.Get( lrs2->hsPos, hs );

 p = hs->AtPosition( pos - lrs2->lrsPos );
 return true;
}

/*
~AtPoint~

*/
bool SimpleLine::AtPoint( const Point& p,
                          bool startsSmaller,
                          double& result ) const {
 if( IsEmpty() || !IsDefined() ){
   return false;
 }

 bool found = false;
 const HalfSegment *hs;
 int pos;
 if( Find( p, pos ) ) {
   found = true;
   segments.Get( pos, hs );
  } else if( pos < Size() ) {
   for( ; pos >= 0; pos-- ) {
     segments.Get( pos, hs );
     if( hs->IsLeftDomPoint() && hs->Contains( p ) ) {
       found = true;
       break;
     }
   }
  }

  if( found ){
    const LRS *lrs;
    lrsArray.Get( hs->attr.edgeno, lrs );
    segments.Get( lrs->hsPos, hs );
    result = lrs->lrsPos + p.Distance( hs->GetDomPoint() );

    if( startsSmaller != this->startSmaller ){
      result = length - result;
    }

    if( AlmostEqual( result, 0.0 ) ){
      result = 0;
    } else if( AlmostEqual( result, length ) ){
      result = length;
    }

    assert( result >= 0.0 && result <= length );

    return true;
  }
  return false;
}

void SimpleLine::SubLine( double pos1, double pos2,
                         bool startsSmaller, SimpleLine& l ) const {
  if( IsEmpty() || !IsDefined() ){
     return;
  }

  if( pos1 < 0 ){
    pos1 = 0;
  } else if( pos1 > length ){
    pos1 = length;
  }

  if( pos2 < 0 ){
    pos2 = 0;
  } else if( pos2 > length ){
    pos2 = length;
  }

  if( AlmostEqual( pos1, pos2 ) || pos1 > pos2 ){
    return;
  }

  if( startsSmaller != this->startSmaller ) {
    double aux = length - pos1;
    pos1 = length - pos2;
    pos2 = aux;
  }

  // First search for the first half segment
  LRS lrs( pos1, 0 );
  int lrsPos;
  Find( lrs, lrsPos );

  const LRS *lrs2;
  lrsArray.Get( lrsPos, lrs2 );

  const HalfSegment *hs;
  segments.Get( lrs2->hsPos, hs );

  l.Clear();
  l.StartBulkLoad();
  int edgeno = 0;

  HalfSegment auxHs;
  if( hs->SubHalfSegment( pos1 - lrs2->lrsPos, pos2 - lrs2->lrsPos, auxHs ) ) {
     auxHs.attr.edgeno = ++edgeno;
     l += auxHs;
     auxHs.SetLeftDomPoint( !auxHs.IsLeftDomPoint() );
     l += auxHs;
   }

   while( lrsPos < lrsArray.Size() - 1 &&
          ( lrs2->lrsPos + hs->Length() < pos2 ||
            AlmostEqual( lrs2->lrsPos + hs->Length(), pos2 ) ) ) {
     // Get the next half segment in the sequence
     lrsArray.Get( ++lrsPos, lrs2 );
     segments.Get( lrs2->hsPos, hs );

     if( hs->SubHalfSegment( pos1 - lrs2->lrsPos, pos2 - lrs2->lrsPos, auxHs)){
       auxHs.attr.edgeno = ++edgeno;
       l += auxHs;
       auxHs.SetLeftDomPoint( !auxHs.IsLeftDomPoint() );
       l += auxHs;
     }
   }

   l.EndBulkLoad();
}


void SimpleLine::Crossings( const SimpleLine& l, Points& result ) const
{
  assert( IsOrdered() && l.IsOrdered() );
  result.Clear();

  if( IsEmpty() || l.IsEmpty() )
    return;

  const HalfSegment *hs1, *hs2;
  Point p;

  result.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );

    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < l.Size(); j++ )
      {
        l.Get( j, hs2 );

        if( hs2->IsLeftDomPoint() )
        {
          if( hs1->Intersection( *hs2, p ) )
            result += p;
        }
      }
    }
  }
  result.EndBulkLoad(true, true); // sort and remove duplicates
}

bool SimpleLine::Intersects(const SimpleLine& l) const{
   if(!IsDefined() || ! l.IsDefined()){
      return false;
   }
   if(IsEmpty() || l.IsEmpty()){
      return false;
   }

   if(!BoundingBox().Intersects(l.BoundingBox())){
      return false;
   }

  const HalfSegment *hs1, *hs2;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );
    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < l.Size(); j++ )
      {
        l.Get( j, hs2 );
        if (hs2->IsLeftDomPoint())
        {
          if( hs1->Intersects( *hs2 ) )
            return true;
        }
      }
    }
  }
  return false;
}



bool SimpleLine::SelectInitialSegment( const Point &startPoint,
                                       const double tolerance ){
  assert( IsDefined() );
  assert( startPoint.IsDefined() );
  if(isCycle ){
     return false;
  }
  bool success = Find(startPoint, currentHS, false);
  if ( !success || currentHS < 0 || currentHS >= Size() ){
     currentHS = -1;
     if (tolerance > 0.0) {
       // try to find the point with minimum distance to startPoint,
       // where the distance is smaller than tolerance
       double minDist = tolerance; // currentHS is -1
       double distance = 0.0;
       for(int pos=0; pos<Size(); pos++) {
         // scan all dominating point, save the index of the HalfSegment with
         // the currently smallest distance to startPoint to currentHS and the
         // current minimum distance to minDist
         const HalfSegment* hs;
         segments.Get( pos, hs );
         distance = hs->GetDomPoint().Distance(startPoint);
         if (distance <= minDist) {
           minDist   = distance;
           currentHS = pos;
         }
       }
       if (currentHS != -1) {
         return true;
       }
     }
     return false;
   }
   return true;
}

bool SimpleLine::SelectSubsequentSegment() {
  assert( IsDefined() );

  const HalfSegment* hs;

  if( isCycle || currentHS < 0 ){
     return false;
  }
  segments.Get(currentHS, hs);
  int partner = hs->attr.partnerno;
  const HalfSegment* nexths;

  // look at position before currentHS's partner
  if( partner>0 ) {
    currentHS = partner - 1;
    segments.Get(currentHS, nexths);
    if ( AlmostEqual(nexths->GetDomPoint(), hs->GetSecPoint()) ) {
       return true;
    }
  }
  // look at position after currentHS's partner
  if( partner < Size()-1 ) {
     currentHS = partner + 1;
     segments.Get(currentHS, nexths);
     if ( AlmostEqual(nexths->GetDomPoint(), hs->GetSecPoint()) ) {
       return true;
     }
   }
   // No subsequent HalfSegment found:
   currentHS = -1;
   return false;
}


bool SimpleLine::getWaypoint( Point &destination ) const{
   assert( IsDefined() );
   if( isCycle || currentHS < 0 || currentHS >= Size() ) {
     destination.SetDefined( false );
     return false;
   }
   const HalfSegment* hs;
   segments.Get(currentHS, hs);
   destination = hs->GetSecPoint();
   destination.SetDefined( true );
   return true;
}

void SimpleLine::fromLine(const Line& src){
  Clear(); // remove all old segments
  if(!src.IsDefined()){
     SetDefined(false);
     return;
  }
  SetDefined(true);
  if(src.IsEmpty()){
    return;
  }
  StartBulkLoad();
  int edgeno = 0;
  const HalfSegment* hs;
  for(int i=0;i<src.Size();i++){
    src.Get(i,hs);
    if(hs->IsLeftDomPoint()){
       HalfSegment hs2 = *hs;
       hs2.attr.edgeno = edgeno;
       edgeno++;
       (*this) += hs2;
       hs2.SetLeftDomPoint(false);
       (*this) += hs2;
    }
  }
  EndBulkLoad();
}


void SimpleLine::toLine(Line& result)const{
  result.Clear();
  if(!IsDefined()){
    result.SetDefined(false);
    return;
  }
  result.SetDefined(true);
  const HalfSegment* hs;
  result.StartBulkLoad();
  for(int i=0;i<segments.Size();i++){
    segments.Get(i,hs);
    result +=  *hs;
  }
  result.EndBulkLoad();
}


void SimpleLine::SetPartnerNo(){
  if(segments.Size()==0){
    return;
  }
  DBArray<int> TMP((segments.Size()+1)/2);

  const HalfSegment* hs1;
  const HalfSegment* hs2;
  for(int i=0; i<segments.Size(); i++){
     segments.Get(i,hs1);
     if(hs1->IsLeftDomPoint()){
        TMP.Put(hs1->attr.edgeno, i);
      } else {
        const int* lpp;
        TMP.Get(hs1->attr.edgeno,lpp);
        int leftpos = *lpp;
        HalfSegment right = *hs1;
        right.attr.partnerno = leftpos;
        right.attr.insideAbove = false;
        right.attr.coverageno = 0;
        right.attr.cycleno = 0;
        right.attr.faceno = 0;
        segments.Get(leftpos,hs2);
        HalfSegment left = *hs2;
        left.attr.partnerno = i;
        left.attr.insideAbove = false;
        left.attr.coverageno = 0;
        left.attr.cycleno = 0;
        left.attr.faceno = 0;
        segments.Put(i,right);
        segments.Put(leftpos,left);
      }
   }
   TMP.Destroy();
 }

 bool SimpleLine::computePolyline(){
  lrsArray.Clear();
  isCycle = false;
  length = 0;
  if(segments.Size()==0){ // an empty line
     return true;
  }

  // the halfsegment array has to be sorted, realminized and
  // the partnernumber must be set correctly

  // step 1: try to find the start of the polyline and check for branches
  int size = segments.Size();
  int start = -1;
  int end = -1;
  int count = 0;
  int pos = 0;
  const HalfSegment* hs;
  Point p1;
  Point p2;
  while(pos<size){
    count = 1;
    segments.Get(pos,hs);
    p1 = hs->GetDomPoint();
    pos++;
    if(pos<size){
      segments.Get(pos,hs);
      p2 = hs->GetDomPoint();
    }
    while(pos<size && AlmostEqual(p1,p2)){
      count++;
      if(count>2){  // branch detected
        return false;
      } else {
        pos++;
        p1 = p2;
        if(pos<size){
           segments.Get(pos,hs);
           p2 = hs->GetDomPoint();
        }
      }
    }
    if(count==1){
       if(start<0){
         start = pos - 1;
       } else if(end < 0){
         end = pos - 1;
       } else { // third end detected
         return false;
       }
    }
  }

  if(start<0 && end>=0){ // loop detected
      return false;
  }

  pos = 0;
  if(start<0){ // line is a cycle
    isCycle=true;
  } else {
    isCycle = false;
    pos = start;
  }

  // the line has two or zero endpoints, may be several components
  vector<bool> used(size,false);
  int noUnused = size;
  const HalfSegment* hs1;
  const HalfSegment* hs2;
  lrsArray.Resize(segments.Size()/2 + 1);
  double lrsPos = 0.0;
  int hsPos = pos;
  int edge = 0;
  while(noUnused > 0){
    segments.Get(hsPos,hs1);
    used[hsPos]=true; // segment is used
    noUnused--;
    int partnerpos = hs1->attr.partnerno;
    segments.Get(partnerpos,hs2);
    used[partnerpos] = true; // partner is used
    noUnused--;
    // store edgenumber
    HalfSegment HS1 = *hs1;
    HalfSegment HS2 = *hs2;
    HS1.attr.edgeno = edge;
    HS2.attr.edgeno = edge;
    edge++;
    segments.Put(hsPos,HS1);
    segments.Put(partnerpos,HS2);

    lrsArray.Append(LRS(lrsPos,hsPos));
    lrsPos += hs1->Length();
    Point p1 = hs2->GetDomPoint();
    if(noUnused > 0){
       bool found = false;
       if(partnerpos > 0 && !used[partnerpos-1]){ // check left side
         segments.Get(partnerpos-1,hs2);
         Point p2 = hs2->GetDomPoint();
         if(AlmostEqual(p1,p2)){ // extension found
           found = true;
           hsPos = partnerpos-1;
         }
       }
       if(!found  && (partnerpos < (size-1) && !used[partnerpos+1])){
           segments.Get(partnerpos+1,hs2);
           Point p2 = hs2->GetDomPoint();
           if(AlmostEqual(p1,p2)){
              found = true;
              hsPos = partnerpos+1;
           }
       }
       if(!found){  // no extension found
         return false;
       }
    }
  }
  lrsArray.Append(LRS(lrsPos,hsPos));
  length = lrsPos;
  return true;
}


int SimpleLine::Compare(const Attribute* arg)const{
  const SimpleLine* line = static_cast<const SimpleLine*>(arg);
  if(!IsDefined() && !line->IsDefined()){
     return true;
  }
  if(!IsDefined()){
    return -1;
  }
  if(!line->IsDefined()){
     return 1;
  }
  if(segments.Size() < line->segments.Size()){
    return -1;
  }
  if(segments.Size() > line->segments.Size()){
    return 1;
  }
  int cmp;
  const HalfSegment* hs1;
  const HalfSegment* hs2;
  for(int i=0;i<segments.Size();i++){
     segments.Get(i,hs1);
     line->segments.Get(i,hs2);
     if( (cmp = hs1->Compare(*hs2)) !=0){
       return cmp;
     }
  }
  return 0;
}

ostream& SimpleLine::Print(ostream& o)const{
  o << "SimpleLine def =" << IsDefined()
    << " size = " << Size() << endl;
  return o;
}

ostream& operator<<(ostream& o, const SimpleLine& cl){
   cl.Print(o);
   return o;
}

Word
 InSimpleLine( const ListExpr typeInfo, const ListExpr instance,
               const int errorPos, ListExpr& errorInfo, bool& correct ){

 correct = true;
 if(nl->AtomType(instance)!=NoAtom){
    correct=false;
    return SetWord(Address(0));
 }
 HalfSegment* hs;
 SimpleLine* line= new SimpleLine(10);
 int edgeno = 0;
 ListExpr rest = instance;
 line->StartBulkLoad();
 while(!nl->IsEmpty(rest)){
   ListExpr segment = nl->First(rest);
   if(!nl->HasLength(segment,4)){
      correct=false;
      delete line;
      return SetWord(Address(0));
   }
   ListExpr halfSegment = nl->TwoElemList(
                                 nl->BoolAtom(true),
                                 nl->TwoElemList(
                                    nl->TwoElemList(nl->First(segment),
                                                    nl->Second(segment)),
                                    nl->TwoElemList(nl->Third(segment),
                                                    nl->Fourth(segment))));
   hs = static_cast<HalfSegment*>(
           InHalfSegment( nl->TheEmptyList(), halfSegment,
                          0, errorInfo, correct ).addr);
   if(!correct){
      delete hs;
      return SetWord(Address(0));
   }
   hs->attr.edgeno = edgeno++;
   *line += *hs;
   hs->SetLeftDomPoint( !hs->IsLeftDomPoint() );
   *line += *hs;
   delete hs;
   rest = nl->Rest(rest);
 }
 if(!line->EndBulkLoad()){
   delete line;
   return SetWord(Address(0));
 }else{
   return SetWord(line);
 }
}


 ListExpr OutSimpleLine( ListExpr typeInfo, Word value ) {
   ListExpr result, last;
   const HalfSegment *hs;
   ListExpr halfseg, halfpoints, flatseg;
   SimpleLine* l = static_cast<SimpleLine*>(value.addr);

   if( l->IsEmpty() ){
     return nl->TheEmptyList();
   }

   result = nl->TheEmptyList();
   last = result;
   bool first = true;

   for( int i = 0; i < l->Size(); i++ ) {
      l->Get( i, hs );
      if( hs->IsLeftDomPoint() ){
        halfseg = OutHalfSegment( nl->TheEmptyList(), SetWord( (void*)hs ) );
        halfpoints = nl->Second( halfseg );
        flatseg = nl->FourElemList(
                    nl->First( nl->First( halfpoints ) ),
                    nl->Second( nl->First( halfpoints ) ),
                    nl->First( nl->Second( halfpoints ) ),
                    nl->Second( nl->Second( halfpoints ) ) );
        if( first == true ) {
          result = nl->OneElemList( flatseg );
          last = result;
          first = false;
        } else {
          last = nl->Append( last, flatseg );
        }
      }
   }
   return result;
}

 Word CreateSimpleLine( const ListExpr typeInfo ) {
   return SetWord( new SimpleLine( 0 ) );
 }

void DeleteSimpleLine( const ListExpr typeInfo, Word& w ) {
  SimpleLine *l = static_cast<SimpleLine*>(w.addr);
  l->Destroy();
  l->DeleteIfAllowed();
  w.addr = 0;
}


void CloseSimpleLine( const ListExpr typeInfo, Word& w ) {
 (static_cast<SimpleLine*>(w.addr))->DeleteIfAllowed();
 w.addr = 0;
}

Word CloneSimpleLine( const ListExpr typeInfo, const Word& w ) {
  return SetWord( new SimpleLine( *((SimpleLine *)w.addr) ) );
}

int SizeOfSimpleLine() {
   return sizeof(SimpleLine);
}

 ListExpr SimpleLineProperty() {
   return nl->TwoElemList(
            nl->FourElemList(
              nl->StringAtom("Signature"),
              nl->StringAtom("Example Type List"),
              nl->StringAtom("List Rep"),
              nl->StringAtom("Example List")),
            nl->FourElemList(
              nl->StringAtom("-> DATA"),
              nl->StringAtom("line"),
              nl->StringAtom("(<segment>*) where segment is "
                             "(<x1><y1><x2><y2>)"),
              nl->StringAtom("( (1 1 2 2)(2 2 1 4) )")));
}

bool CheckSimpleLine( ListExpr type, ListExpr& errorInfo ){
   return nl->IsEqual( type, "sline" );
}

void* CastSimpleLine(void* addr) {
  return SimpleLine::Cast(addr);;
}

TypeConstructor sline(
     "sline",                         //name
     SimpleLineProperty,                   //describing signature
     OutSimpleLine,  InSimpleLine,         //Out and In functions
     0,              0,              //SaveTo and RestoreFrom List functions
     CreateSimpleLine, DeleteSimpleLine,     //object creation and deletion
     OpenAttribute<SimpleLine>,
     SaveAttribute<SimpleLine>,       // object open and save
     CloseSimpleLine, CloneSimpleLine,      //object close and clone
     CastSimpleLine,                       //cast function
     SizeOfSimpleLine,                     //sizeof function
     CheckSimpleLine );


/*
8 Type Constructor ~region~

A ~region~ value is a set of halfsegments. In the external (nestlist) representation, a region value is
expressed as a set of faces, and each face is composed of a set of cycles.  However, in the internal
(class) representation, it is expressed as a set of sorted halfsegments, which are stored as a PArray.

8.1 Implementation of the class ~region~

*/
Region::Region( const Region& cr, bool onlyLeft ) :
region( cr.Size() ),
bbox( cr.bbox ),
noComponents( cr.noComponents ),
ordered( true )
{
  assert( cr.IsOrdered() );
  StartBulkLoad();
  const HalfSegment *hs;
  if( !onlyLeft )
  {
    for( int i = 0; i < cr.Size(); i++ )
    {
      cr.Get( i, hs );
      region.Put( i, *hs );
    }
  }
  else
  {
    int j=0;
    for( int i = 0; i < cr.Size(); i++ )
    {
      cr.Get( i, hs );
      if (hs->IsLeftDomPoint())
      {
        region.Put( j, *hs );
        j++;
      }
    }
  }
  EndBulkLoad( false, false, false, false );
  del.refs=1;
  del.isDelete=true;
  del.isDefined = cr.del.isDefined;
}

Region::Region( const Rectangle<2>& r ):region(8)
{
    SetDefined( true);
    if(  r.IsDefined() )
    {
      HalfSegment hs;
      int partnerno = 0;
      double min0 = r.MinD(0), max0 = r.MaxD(0),
      min1 = r.MinD(1), max1 = r.MaxD(1);

      Point v1(true, max0, min1),
      v2(true, max0, max1),
      v3(true, min0, max1),
      v4(true, min0, min1);

      if( AlmostEqual(v1, v2) ||
          AlmostEqual(v2, v3) ||
          AlmostEqual(v3, v4) ||
          AlmostEqual(v4, v1) )
      { // one interval is (almost) empty, so will be the region
        SetDefined( true );
        return;
      }

      StartBulkLoad();

      hs.Set(true, v1, v2);
      hs.attr.faceno = 0;         // only one face
      hs.attr.cycleno = 0;        // only one cycle
      hs.attr.edgeno = partnerno;
      hs.attr.partnerno = partnerno++;
      hs.attr.insideAbove = (hs.GetLeftPoint() == v1);
      *this += hs;
      hs.SetLeftDomPoint( !hs.IsLeftDomPoint() );
      *this += hs;

      hs.Set(true, v2, v3);
      hs.attr.faceno = 0;         // only one face
      hs.attr.cycleno = 0;        // only one cycle
      hs.attr.edgeno = partnerno;
      hs.attr.partnerno = partnerno++;
      hs.attr.insideAbove = (hs.GetLeftPoint() == v2);
      *this += hs;
      hs.SetLeftDomPoint( !hs.IsLeftDomPoint() );
      *this += hs;

      hs.Set(true, v3, v4);
      hs.attr.faceno = 0;         // only one face
      hs.attr.cycleno = 0;        // only one cycle
      hs.attr.edgeno = partnerno;
      hs.attr.partnerno = partnerno++;
      hs.attr.insideAbove = (hs.GetLeftPoint() == v3);
      *this += hs;
      hs.SetLeftDomPoint( !hs.IsLeftDomPoint() );
      *this += hs;

      hs.Set(true, v4, v1);
      hs.attr.faceno = 0;         // only one face
      hs.attr.cycleno = 0;        // only one cycle
      hs.attr.edgeno = partnerno;
      hs.attr.partnerno = partnerno++;
      hs.attr.insideAbove = (hs.GetLeftPoint() == v4);
      *this += hs;
      hs.SetLeftDomPoint( !hs.IsLeftDomPoint() );
      *this += hs;

      EndBulkLoad();
      SetDefined( true );
    }
    else
      SetDefined( false );
}

void Region::StartBulkLoad()
{
  ordered = false;
}

void Region::EndBulkLoad( bool sort, bool setCoverageNo,
                          bool setPartnerNo, bool computeRegion )
{
  if( sort )
    Sort();

  if( setCoverageNo )
  {
    const HalfSegment *hs;
    int currCoverageNo = 0;

    for( int i = 0; i < this->Size(); i++ )
    {
      Get( i, hs );

      if( hs->IsLeftDomPoint() )
        currCoverageNo++;
      else
        currCoverageNo--;

      HalfSegment auxhs( *hs );
      auxhs.attr.coverageno = currCoverageNo;

      region.Put( i, auxhs );
    }
  }

  if( setPartnerNo )
    SetPartnerNo();

  if( computeRegion )
    ComputeRegion();

  region.TrimToSize();
  ordered = true;
}

bool Region::Contains( const Point& p ) const
{
  assert( IsOrdered() );

  if( IsEmpty() )
    return false;

  if( !p.Inside(bbox) )
    return false;

  map<int, int> faceISN;
  const HalfSegment *hs;

  int coverno=0;
  int startpos=0;
  double y0;

  //1. find the right place by binary search
  if( Find( p, startpos ) )
    return true;

  if ( startpos == 0 )   //p is smallest
    return false;
  else if ( startpos == Size() )  //p is largest
    return false;

  int i = startpos - 1;

  //2. deal with equal-x hs's
  region.Get( i, hs );
  while( i > 0 &&
         hs->GetDomPoint().GetX() == p.GetX() )
  {
    if( hs->Contains(p) )
      return true;

    if( hs->IsLeftDomPoint() && hs->RayAbove( p, y0 ) )
    {
      if( faceISN.find( hs->attr.faceno ) != faceISN.end() )
        faceISN[ hs->attr.faceno ]++;
      else
        faceISN[ hs->attr.faceno ] = 1;
    }
    region.Get( --i, hs );
  }

  // at this point, i is pointing to the last hs whose dp.x != p.x

  //3. get the coverage value
  coverno = hs->attr.coverageno;

  //4. search the region value for coverageno steps
  int touchedNo = 0;
  while( i >= 0 && touchedNo < coverno )
  {
    this->Get(i, hs);

    if( hs->Contains(p) )
      return true;

    if( hs->IsLeftDomPoint() &&
        hs->GetLeftPoint().GetX() <= p.GetX() &&
        p.GetX() <= hs->GetRightPoint().GetX() )
      touchedNo++;

    if( hs->IsLeftDomPoint() && hs->RayAbove( p, y0 ) )
    {
      if( faceISN.find( hs->attr.faceno ) != faceISN.end() )
        faceISN[ hs->attr.faceno ]++;
      else
        faceISN[ hs->attr.faceno ] = 1;
    }

    i--;  //the iterator
  }

  for( map<int, int>::iterator iter = faceISN.begin();
       iter != faceISN.end();
       iter++ )
  {
    if( iter->second % 2 != 0 )
      return true;
  }
  return false;
}

bool Region::InnerContains( const Point& p ) const
{
  assert( IsOrdered() );

  if( IsEmpty() )
    return false;

  if( !p.Inside(bbox) )
    return false;

  map<int, int> faceISN;
  const HalfSegment *hs;

  int coverno = 0,
      startpos = 0;
  double y0;

  //1. find the right place by binary search
  if( Find( p, startpos ) )
    return false;

  if ( startpos == 0 )   //p is smallest
    return false;
  else if ( startpos == Size() )  //p is largest
    return false;

  int i = startpos - 1;

  //2. deal with equal-x hs's
  region.Get( i, hs );
  while( i > 0 &&
         hs->GetDomPoint().GetX() == p.GetX() )
  {
    if( hs->Contains(p) )
      return false;

    if( hs->IsLeftDomPoint() && hs->RayAbove( p, y0 ) )
    {
      if( faceISN.find( hs->attr.faceno ) != faceISN.end() )
        faceISN[ hs->attr.faceno ]++;
      else
        faceISN[ hs->attr.faceno ] = 1;
    }
    region.Get( --i, hs );
  }

  // at this point, i is pointing to the last hs whose dp.x != p.x

  //3. get the coverage value
  coverno = hs->attr.coverageno;

  //4. search the region value for coverageno steps
  int touchedNo = 0;
  while( i >= 0 && touchedNo < coverno )
  {
    this->Get(i, hs);

    if( hs->Contains(p) )
      return false;

    if( hs->IsLeftDomPoint() &&
        hs->GetLeftPoint().GetX() <= p.GetX() &&
        p.GetX() <= hs->GetRightPoint().GetX() )
      touchedNo++;

    if( hs->IsLeftDomPoint() && hs->RayAbove( p, y0 ) )
    {
      if( faceISN.find( hs->attr.faceno ) != faceISN.end() )
        faceISN[ hs->attr.faceno ]++;
      else
        faceISN[ hs->attr.faceno ] = 1;
    }

    i--;  //the iterator
  }

  for( map<int, int>::iterator iter = faceISN.begin();
       iter != faceISN.end();
       iter++ )
  {
    if( iter->second % 2 != 0 )
      return true;
  }
  return false;
}

bool Region::Intersects( const HalfSegment& inHs ) const
{
  if( bbox.Intersects( inHs.BoundingBox() ) )
  {
    if( Contains( inHs.GetLeftPoint() ) ||
        Contains( inHs.GetRightPoint() ) )
      return true;

    const HalfSegment *hs;
    for( int i = 0; i < Size(); i++ )
    {
      Get( i, hs );
      if( hs->Intersects( inHs ) )
        return true;
    }
  }
  return false;
}

bool Region::Contains( const HalfSegment& hs ) const
{
  if( IsEmpty() )
    return false;

  if( !hs.GetLeftPoint().Inside(bbox) ||
      !hs.GetRightPoint().Inside(bbox) )
    return false;

  if( !Contains(hs.GetLeftPoint()) ||
      !Contains(hs.GetRightPoint()) )
    return false;

  const HalfSegment *auxhs;
  bool checkMidPoint = false;

  //now we know that both endpoints of hs is inside region
  for( int i = 0; i < Size(); i++ )
  {
    Get(i, auxhs);
    if( auxhs->IsLeftDomPoint() )
    {
      if( hs.Crosses(*auxhs) )
        return false;
      else if( hs.Inside(*auxhs) )
       //hs is part of the border
        return true;
      else if( hs.Intersects(*auxhs) &&
               ( auxhs->Contains(hs.GetLeftPoint()) ||
                 auxhs->Contains(hs.GetRightPoint()) ) );
        checkMidPoint = true;
    }
  }

  if( checkMidPoint )
  {
    Point midp( true,
                ( hs.GetLeftPoint().GetX() + hs.GetRightPoint().GetX() ) / 2,
                ( hs.GetLeftPoint().GetY() + hs.GetRightPoint().GetY() ) / 2 );
    if( !Contains( midp ) )
      return false;
  }
  return true;
}

bool Region::InnerContains( const HalfSegment& hs ) const
{
  if( IsEmpty() )
    return false;

  if( !hs.GetLeftPoint().Inside(bbox) ||
      !hs.GetRightPoint().Inside(bbox) )
    return false;

  if( !InnerContains(hs.GetLeftPoint()) ||
      !InnerContains(hs.GetRightPoint()) )
    return false;

  const HalfSegment *auxhs;

  //now we know that both endpoints of hs are completely inside the region
  for( int i = 0; i < Size(); i++ )
  {
    Get(i, auxhs);
    if( auxhs->IsLeftDomPoint() )
    {
      if( hs.Intersects( *auxhs ) )
        return false;
    }
  }

  return true;
}

bool Region::HoleEdgeContain( const HalfSegment& hs ) const
{
  const HalfSegment *auxhs;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, auxhs );
    if( auxhs->IsLeftDomPoint() &&
        auxhs->attr.cycleno > 0 &&
        hs.Inside( *auxhs ) )
      return true;
  }
  return false;
}

/*
4.4.7 Operation ~intersects~

*/
bool Region::Intersects( const Region &r ) const
{
  assert( IsOrdered() && r.IsOrdered() );

  if( IsEmpty() && r.IsEmpty() )
    return true;

  if( IsEmpty() || r.IsEmpty() )
    return false;

  if( !BoundingBox().Intersects( r.BoundingBox() ) )
    return false;

  if( Inside( r ) || r.Inside( *this ) )
    return true;

  const HalfSegment *hs1, *hs2;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );
    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < r.Size(); j++ )
      {
        r.Get( j, hs2 );
        if( hs2->IsLeftDomPoint() &&
            hs1->Intersects( *hs2 ) )
          return true;
      }
    }
  }

  return false;
}

bool Region::Inside( const Region& r ) const
{
  assert( IsOrdered() && r.IsOrdered() );

  if( IsEmpty() && r.IsEmpty() )
    return true;

  if( IsEmpty() || r.IsEmpty() )
    return false;

  if( !r.BoundingBox().Contains( bbox ) )
    return false;

  const HalfSegment *hs1, *hs2;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );

    if( hs1->IsLeftDomPoint() )
    {
      if( !r.Contains( *hs1 ) )
        return false;
    }
  }

  bool existhole = false,
       allholeedgeinside = true;

  for( int j = 0; j < r.Size(); j++ )
  {
    r.Get( j, hs2 );

    if( hs2->IsLeftDomPoint() &&
        hs2->attr.cycleno > 0 )
    //&& (hs2 is not masked by another face of region2)
    {
      if( !HoleEdgeContain( *hs2 ) )
      {
        existhole=true;
        if( !Contains( *hs2 ) )
          allholeedgeinside=false;
      }
    }
  }

  if( existhole && allholeedgeinside )
    return false;

  return true;
}

bool Region::Adjacent( const Region& r ) const
{
  assert( IsOrdered() && r.IsOrdered() );

  if( IsEmpty() || r.IsEmpty() )
    return false;

  if( !BoundingBox().Intersects( r.BoundingBox() ) )
    return false;

  if( Inside( r ) || r.Inside( *this ) )
    return false;

  const HalfSegment *hs1, *hs2;
  bool found = false;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );
    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < r.Size(); j++ )
      {
        r.Get( j, hs2 );
        if( hs2->IsLeftDomPoint() && hs1->Intersects( *hs2 ) )
        {
          if( hs1->Crosses( *hs2 ) )
            return false;
          found = true;
        }
      }
    }
  }
  return found;
}

bool Region::Overlaps( const Region& r ) const
{
  assert( IsOrdered() && r.IsOrdered() );

  if( IsEmpty() || r.IsEmpty() )
    return false;

  if( !BoundingBox().Intersects( r.BoundingBox() ) )
    return false;

  if( Inside( r ) || r.Inside( *this ) )
    return true;

  const HalfSegment *hs1, *hs2;
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );
    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < r.Size(); j++ )
      {
        r.Get( j, hs2 );
        if( hs2->IsLeftDomPoint() )
        {
          if( hs2->Crosses( *hs1 ) )
            return true;
        }
      }
    }
  }
  return false;
}

bool Region::OnBorder( const Point& p ) const
{
  int pos;
  if( Find( p, pos ) )
    // the point is found on one half segment
    return true;

  if( pos == 0 ||
      pos == Size() )
    // the point is smaller or bigger than all
    // segments
    return false;

  const HalfSegment *hs;
  pos--;
  while( pos >= 0 )
  {
    Get( pos--, hs );
    if( hs->IsLeftDomPoint() &&
        hs->Contains( p ) )
      return true;
  }
  return false;
}

bool Region::InInterior( const Point& p ) const
{
  return InnerContains( p );
}

double Region::Distance( const Point& p ) const
{
  assert( IsDefined() && p.IsDefined() && IsOrdered() && !IsEmpty() );

  if( Contains( p ) )
    return 0.0;

  const HalfSegment *hs;
  double result = numeric_limits<double>::max();

  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs );

    if( hs->IsLeftDomPoint() )
      result = MIN( result, hs->Distance( p ) );
  }
  return result;
}

double Region::Distance( const Rectangle<2>& r ) const
{
  assert( IsDefined() && r.IsDefined() && IsOrdered() && !IsEmpty() );
  Region rr( r );
  return Distance( rr );
}

double Region::Area() const
{
  assert( IsDefined() );
  int n = Size();
  double area = 0.0,
         x0 = 0.0, y0 = 0.0,
         x1 = 0.0, y1 = 0.0;

  // get minimum with respect to Y-dimension
  double minY = MIN(BoundingBox().MinD(1), +0.0);

  for(int i=0; i<n; i++)
  {
    const HalfSegment *hs;
    Get( i, hs );
    if( hs->IsLeftDomPoint() )
    { // use only one halfsegment
      x0 = hs->GetLeftPoint().GetX();
      x1 = hs->GetRightPoint().GetX();
      // y0, y1 must be >= 0, so we correct them
      y0 = hs->GetLeftPoint().GetY() - minY;
      y1 = hs->GetRightPoint().GetY() - minY;
//       cout << "HSegment #" << i
//           << ": ( (" << x0 << "," << y0
//           << ") (" << x1 << "," << y1 << ") )"
//           << hs->attr.insideAbove << endl;
//       double dx = (x1-x0);
//       double ay = (y1+y0) * 0.5;
      double a = (x1-x0) * ((y1+y0) * 0.5);
      if ( hs->attr.insideAbove )
//    if ( (hs->attr.insideAbove && (hs->attr.cycleno == 0 )) ||
//          (!hs->attr.insideAbove && (hs->attr.cycleno > 0 ))   )
        a = -a;
//       cout << "  dx=" << dx << " ay=" << ay << " a=" << a << endl;
      area += a;
    }
  }

  return area;
}

double Region::Distance( const Points& ps ) const
{
  assert( IsOrdered() && !IsEmpty() &&
          ps.IsOrdered() && !ps.IsEmpty() );

  double result = numeric_limits<double>::max();
  const Point *p;

  for( int i = 0; i < ps.Size(); i++ )
  {
    ps.Get( i, p );

    if( Contains( *p ) )
      return 0.0;

    const HalfSegment *hs;

    for( int j = 0; j < Size(); j++ )
    {
      Get( j, hs );

      if( hs->IsLeftDomPoint() )
        result = MIN( result, hs->Distance( *p ) );
    }
  }
  return result;
}

double Region::Distance( const Region &r ) const
{
  assert( IsOrdered() && r.IsOrdered() &&
          !IsEmpty() && !r.IsEmpty() );

  if( Inside( r ) || r.Inside( *this ) )
    return 0.0;

  double result = numeric_limits<double>::max();
  const HalfSegment *hs1, *hs2;

  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );
    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < r.Size(); j++ )
      {
        r.Get( j, hs2 );
        if( hs2->IsLeftDomPoint() )
        {
          if( hs1->Intersects( *hs2 ) )
            return 0.0;

          result = MIN( result, hs1->Distance( *hs2 ) );
        }
      }
    }
  }

  return result;
}

void Region::Components( vector<Region*>& components )
{
  Region *copy = new Region( *this );
  copy->LogicSort();

  map<int,int> edgeno,
               cycleno,
               faceno;

  for( int i = 0; i < Size(); i++ )
  {
    const HalfSegment *hs;
    copy->Get( i, hs );
    Region *r;
    HalfSegment aux( *hs );
    if( faceno.find( hs->attr.faceno ) == faceno.end() )
    {
      r = new Region( 1 );
      r->StartBulkLoad();
      components.push_back( r );
      aux.attr.faceno = faceno.size();
      faceno.insert( make_pair( hs->attr.faceno, aux.attr.faceno ) );
    }
    else
    {
      aux.attr.faceno = faceno[ hs->attr.faceno ];
      r = components[ aux.attr.faceno ];
    }

    if( cycleno.find( hs->attr.cycleno ) == cycleno.end() )
    {
      aux.attr.cycleno = cycleno.size();
      cycleno.insert( make_pair( hs->attr.cycleno, aux.attr.cycleno ) );
    }
    else
      aux.attr.cycleno = cycleno[ hs->attr.cycleno ];

    if( edgeno.find( hs->attr.edgeno ) == edgeno.end() )
    {
      aux.attr.edgeno = edgeno.size();
      edgeno.insert( make_pair( hs->attr.edgeno, aux.attr.edgeno ) );
    }
    else
      aux.attr.edgeno = edgeno[ hs->attr.edgeno ];

    *r += aux;
  }

  for( size_t i = 0; i < components.size(); i++ )
    components[i]->EndBulkLoad();
}

void Region::Translate( const Coord& x, const Coord& y, Region& result ) const
{
  assert( IsOrdered() );

  const HalfSegment *hs;
  result.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs );

    HalfSegment auxhs( *hs );
    auxhs.Translate( x, y );
    result += auxhs;
  }
  result.SetNoComponents( NoComponents() );
  result.EndBulkLoad( false, false, false, false );
}

void Region::Rotate( const Coord& x, const Coord& y,
                   const double alpha,
                   Region& result ) const
{
  result.Clear();
  if(!IsDefined()){
     result.SetDefined(false);
     return;
  }
  result.Resize(Size());
  result.SetDefined(true);

  double s = sin(alpha);
  double c = cos(alpha);

  double m00 = c;
  double m01 = -s;
  double m02 = x - x*c + y*s;
  double m10 = s;
  double m11 = c;
  double m12 = y - x*s-y*c;

  result.StartBulkLoad();
  const HalfSegment *hso;
  Point p1;
  Point p2;
  Point p1o;
  Point p2o;

  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hso );
    p1o = hso->GetLeftPoint();
    p2o = hso->GetRightPoint();
    p1.Set( m00*p1o.GetX()
            + m01*p1o.GetY() + m02,
            m10*p1o.GetX()
           + m11*p1o.GetY() + m12);
    p2.Set( m00*p2o.GetX()
             + m01*p2o.GetY() + m02,
             m10*p2o.GetX()
             + m11*p2o.GetY() + m12);

    HalfSegment hsr(*hso); // ensure to copy attr;
    hsr.Set(hso->IsLeftDomPoint(),p1,p2);
    bool above = hso->attr.insideAbove;

    if(p1>p2)  {
       above = !above;
    }
  
    hsr.attr.insideAbove = above;
    result += hsr;
  }
  result.EndBulkLoad(true,true,true,false); // reordering may be required

}

void Region::TouchPoints( const Line& l, Points& result ) const
{
  assert( IsOrdered() && l.IsOrdered() );
  result.Clear();

  if( IsEmpty() || l.IsEmpty() )
    return;

  const HalfSegment *hs1, *hs2;
  Point p;

  result.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );

    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < l.Size(); j++ )
      {
        l.Get( j, hs2 );

        if( hs2->IsLeftDomPoint() )
        {
          if( hs1->Intersection( *hs2, p ) )
            result += p;
        }
      }
    }
  }
  result.EndBulkLoad( true, true );
}

void Region::TouchPoints( const Region& r, Points& result ) const
{
  assert( IsOrdered() && r.IsOrdered() );
  result.Clear();

  if( IsEmpty() || r.IsEmpty() )
    return;

  const HalfSegment *hs1, *hs2;
  Point p;

  result.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );

    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < r.Size(); j++ )
      {
        r.Get( j, hs2 );

        if( hs2->IsLeftDomPoint() )
        {
          if( hs1->Intersection( *hs2, p ) )
            result += p;
        }
      }
    }
  }
  result.EndBulkLoad( true, false );
}

void Region::CommonBorder( const Region& r, Line& result ) const
{
  assert( IsOrdered() && r.IsOrdered() );
  result.Clear();

  if( IsEmpty() || r.IsEmpty() )
    return;

  const HalfSegment *hs1, *hs2;
  HalfSegment reshs;
  int edgeno = 0;
  Point p;

  result.StartBulkLoad();
  for( int i = 0; i < Size(); i++ )
  {
    Get( i, hs1 );

    if( hs1->IsLeftDomPoint() )
    {
      for( int j = 0; j < r.Size(); j++ )
      {
        r.Get( j, hs2 );

        if( hs2->IsLeftDomPoint() )
        {
          if( hs1->Intersection( *hs2, reshs ) )
          {
            reshs.attr.edgeno = edgeno++;
            result += reshs;
            reshs.SetLeftDomPoint( !reshs.IsLeftDomPoint() );
            result += reshs;
          }
        }
      }
    }
  }
  result.EndBulkLoad();
}

int Region::NoComponents() const
{
  return noComponents;
}

void Region::Vertices( Points* result ) const
{
  assert( IsOrdered() );

  result->Clear();
  if(!IsDefined()){
      result->SetDefined(false);
      return;
  }
  result->SetDefined(true);

  if( IsEmpty() ){
    return;
  }

  const HalfSegment* hs;
  int size = Size();
  for( int i = 0; i < size; i++ )
  {
    Get( i, hs );
    Point p = hs->GetDomPoint();
    *result += p;
  }
  result->EndBulkLoad( false, true );
}


void Region::Boundary( Line* result ) const
{
  assert( IsOrdered() );

  result->Clear();
  if(!IsDefined()){
      result->SetDefined(false);
      return;
  }

  if( IsEmpty() ){
    return;
  }
  const HalfSegment* hs;
  result->StartBulkLoad();
  int size = Size();
  for( int i = 0; i < size; i++ )
  {
    Get( i, hs );
    if(hs->IsLeftDomPoint()){
       HalfSegment h = *hs;
       h.attr.edgeno = i;
       *result += h;
       h.SetLeftDomPoint(false);
       *result += h;
    }
  }
  result->EndBulkLoad();
}



Region& Region::operator=( const Region& r )
{
  assert( r.IsOrdered() );

  region.Clear();
  if( !r.IsEmpty() )
  {
    region.Resize( r.Size() );
    for( int i = 0; i < r.Size(); i++ )
    {
      const HalfSegment *hs;
      r.Get( i, hs );
      region.Put( i, *hs );
    }
  }

  bbox = r.bbox;
  noComponents = r.noComponents;
  del.refs=1;
  del.isDelete=true;
  return *this;
}

bool Region::operator==( const Region& r ) const
{
  assert( ordered && r.ordered );

  if( Size() != r.Size() )
    return false;

  if( IsEmpty() && r.IsEmpty() )
    return true;

  if( bbox != r.bbox )
    return false;

  object obj;
  status stat;
  SelectFirst_rr( *this, r, obj, stat );

  while( obj == both || obj == none )
  {
    if( stat == endboth )
      return true;

    SelectNext_rr( *this, r, obj, stat );
  }
  return false;
}

bool Region::operator!=( const Region &cr) const
{
  return !(*this==cr);
}

Region& Region::operator+=( const HalfSegment& hs )
{
  if( IsEmpty() )
    bbox = hs.BoundingBox();
  else
    bbox = bbox.Union( hs.BoundingBox() );

  if( !IsOrdered() )
  {
    region.Put( region.Size(), hs);
  }
  else
  {
    int pos;
    if( !Find( hs, pos ) )
    {
      for( int i = region.Size() - 1; i >= pos; i++ )
      {
        const HalfSegment *auxhs;
        region.Get( i, auxhs );
        region.Put( i+1, *auxhs );
      }
      region.Put( pos, hs );
    }
  }
  return *this;
}

Region& Region::operator-=( const HalfSegment& hs )
{
  assert( IsOrdered() );

  int pos;
  if( Find( hs, pos ) )
  {
    for( int i = pos; i < Size(); i++ )
    {
      const HalfSegment *auxhs;
      region.Get( i+1, auxhs );
      region.Put( i, *auxhs );
    }
  }

  // Naive way to redo the bounding box.
  if( IsEmpty() )
    bbox.SetDefined( false );
  int i = 0;
  const HalfSegment *auxhs;
  region.Get( i++, auxhs );
  bbox = auxhs->BoundingBox();
  for( ; i < Size(); i++ )
  {
    region.Get( i, auxhs );
    bbox = bbox.Union( auxhs->BoundingBox() );
  }

  return *this;
}

bool Region::Find( const HalfSegment& hs, int& pos ) const
{
  assert( IsOrdered() );
  return region.Find( &hs, HalfSegmentCompare, pos );
}

bool Region::Find( const Point& p, int& pos ) const
{
  assert( IsOrdered() );
  return region.Find( &p, PointHalfSegmentCompare, pos );
}

void Region::Sort()
{
  assert( !IsOrdered() );

  region.Sort( HalfSegmentCompare );

  ordered = true;
}

void Region::LogicSort()
{
  region.Sort( HalfSegmentLogicCompare );

  ordered = true;
}

ostream& operator<<( ostream& os, const Region& cr )
{
  os << "<"<<endl;
  for( int i = 0; i < cr.Size(); i++ )
  {
    const HalfSegment *hs;
    cr.Get( i, hs );
    os << " " << *hs << endl;
  }
  os << ">";
  return os;
}

void Region::SetPartnerNo()
{
  int size = Size();
  int tmp[size/2];
  const HalfSegment* hs;
  for( int i = 0; i < size; i++ )
  {
    Get( i, hs );
    if( hs->IsLeftDomPoint() )
    {
      tmp[hs->attr.edgeno] = i;
    }
    else
    {
      int p = tmp[hs->attr.edgeno];
      HalfSegment hs1( *hs );
      hs1.attr.partnerno = p;
      Put( i, hs1 );
      Get( p, hs );
      hs1 = *hs;
      hs1.attr.partnerno = i;
      Put( p, hs1 );
    }
  }
}


double VectorSize(const Point &p1, const Point &p2)
{
  double size = pow( (p1.GetX() - p2.GetX()),2)
                + pow( (p1.GetY() - p2.GetY()),2);
  size = sqrt(size);
  return size;
}

//The angle function returns the angle of VP1P2
// P1 is the point on the window's edge
double Angle(const Point &v, const Point &p1,const Point &p2)
{
  double coss;

  //If P1P2 is vertical and the window's edge
  // been tested is horizontal , then
  //the angle VP1P2 is equal to 90 degrees. On the
  //other hand, if P1P2 is vertical
  //and the window's edge been tested is vertical, then
  // the angle is 90 degrees.
  //Similar tests are applied when P1P2 is horizontal.

  if (p1.GetX() == p2.GetX()) //the segment is vertical
    if (v.GetY()==p1.GetY()) return PI/2; //horizontal edge
    else return 0;
  if (p1.GetY() == p2.GetY()) //the segment is horizontal
    if (v.GetY()==p1.GetY()) return 0; //horizontal edge
    else return PI/2;

  coss = double( ( (v.GetX() - p1.GetX()) * (p2.GetX() - p1.GetX()) ) +
                 ( (v.GetY() - p1.GetY()) * (p2.GetY() - p1.GetY()) ) ) /
                 (VectorSize(v,p1) * VectorSize(p2,p1));
  //cout<<endl<<"Coss"<<coss;
  //coss = abs(coss);
  //cout<<endl<<"Coss"<<coss;
  return acos(coss);
}


ostream& operator<<( ostream& o, const EdgePoint& p )
{
  o << "(" << p.GetX() << ", " << p.GetY() << ")"
    <<" D("<<(p.direction ? "LEFT/DOWN" : "RIGHT/UP")<<")"
    <<" R("<<(p.rejected ? "Rejected" : "Accepted")<<")";
  return o;
}

EdgePoint* EdgePoint::GetEdgePoint( const Point &p,
                                    const Point &p2,
                                    bool insideAbove,
                                    const Point &v,
                                    const bool reject )
{
  //The point p2 must be outside the window
  bool direction;

  //window's vertical edge
  if (v.GetX()==p.GetX())
  {
    if (insideAbove)
      direction =  false; //UP
    else
      direction =  true; //DOWN
  }
  else  //Horizontal edge
  {
    if (insideAbove)
    {
      if ( (p.GetX()-p2.GetX())>0 )
        //p2.GetX() is located to the left of p.GetX()
        direction =  false; //RIGHT
      else
        direction =  true; //LEFT
    }
    else
    {
      if ( (p.GetX()-p2.GetX())>0 )
        //p2.GetX() is located to the right of p.GetX()
        direction =  true; //LEFT
      else
        direction =  false; //RIGHT
    }
  }
  return new EdgePoint(p,direction,reject);

}

void AddPointToEdgeArray( const Point &p,
                          const HalfSegment &hs,
                          const Rectangle<2> &window,
                          vector<EdgePoint> pointsOnEdge[4] )
{
  EdgePoint *dp;
  Point v;
  AttrType attr;
  attr = hs.GetAttr();
  Point p2;
  //If the left and right edges are been tested then
  //it is not need to check the angle
  //between the half segment and the edge. If the attribute
  //inside above is true, then
  //the direction is up (false), otherwise it is down (true).
  if (p.GetX() == window.MinD(0))
  {
    dp = new EdgePoint(p,!attr.insideAbove,false);
    pointsOnEdge[WLEFT].push_back(*dp);
  }
  else
    if (p.GetX() == window.MaxD(0))
    {
      dp = new EdgePoint(p,!attr.insideAbove,false);
      pointsOnEdge[WRIGHT].push_back(*dp);
    }
  if (p.GetY() == window.MinD(1))
  {
    v.Set(window.MinD(0), window.MinD(1));
    //In this case we don't know which point is outside the window,
    //so it is need to test both half segment's poinst. Moreover,
    //in order to use the same comparison that is used for
    //Top edge, it is need to choose the half segment point that
    //is over the bottom edge.
    if (hs.GetLeftPoint().GetY()>window.MinD(1))
      dp = EdgePoint::GetEdgePoint(p,hs.GetLeftPoint(),
                                   attr.insideAbove,v,false);
    else
      dp = EdgePoint::GetEdgePoint(p,hs.GetRightPoint(),
                                   attr.insideAbove,v,false);
    pointsOnEdge[WBOTTOM].push_back(*dp);
  }
  else
    if (p.GetY() == window.MaxD(1))
    {
      v.Set(window.MinD(0), window.MaxD(1));
    //In this case we don't know which point is outside the window,
    //so it is need to test
    if (hs.GetLeftPoint().GetY()>window.MaxD(1))
      dp = EdgePoint::GetEdgePoint(p,hs.GetLeftPoint(),
                                   attr.insideAbove,v,false);
    else
      dp = EdgePoint::GetEdgePoint(p,hs.GetRightPoint(),
                                   attr.insideAbove,v,false);
      pointsOnEdge[WTOP].push_back(*dp);
    }
}

bool GetAcceptedPoint( vector <EdgePoint>pointsOnEdge,
                       int &i, const int end,
                       EdgePoint &ep )
{
  //id is the indice of the current point in the scan
  //ep is the correct edge point that will be returned.
  ep = pointsOnEdge[i];
  //discard all rejected points
  while (ep.rejected && i<=end)
  {
    i++;
    if (i>end)
      return false;
    EdgePoint epAux = pointsOnEdge[i];
    //Discard all the points that was accepted but has a
    // corresponding rejection point.
    //In other words, point that has the same
    // coordinates and direction on the edge.
    if (!epAux.rejected && (epAux.direction==ep.direction) &&
         (epAux.GetX() == ep.GetX()) && (epAux.GetY() == ep.GetY()) )
    {
      while ( (i<=end) && (epAux.direction==ep.direction) &&
         (epAux.GetX() == ep.GetX()) && (epAux.GetY() == ep.GetY()) )
      {
        i++;
        if (i>end)
          return false;
        epAux = pointsOnEdge[i];
      }
    }
    ep = epAux;
  }
  return true;
}

void Region::CreateNewSegments(vector <EdgePoint>pointsOnEdge, Region &cr,
                                const Point &bPoint,const Point &ePoint,
                               WindowEdge edge,int &partnerno,
                               bool inside)
//The inside attribute indicates if the points on edge will originate
//segments that are inside the window (its values is true), or outside
//the window (its value is false)
{
  int begin, end, i;
  HalfSegment *hs;
  AttrType attr;
  EdgePoint dp,dpAux;

  if (pointsOnEdge.size()==0) return;
/*
  for (int j=0;j<pointsOnEdge.size();j++)
    cout<<endl<<j<<": "<<pointsOnEdge[j];

*/
  sort(pointsOnEdge.begin(),pointsOnEdge.end());

/*
  for (int j=0;j<pointsOnEdge.size();j++)
    cout<<endl<<j<<": "<<pointsOnEdge[j];

*/
  begin = 0;
  end = pointsOnEdge.size()-1;

  dp = pointsOnEdge[begin];
  if ( dp.direction)//dp points to left or down
  {

    if (!dp.rejected)
    {
      //If dp is a rejected point then it must not be considered
      //as point to be connected to the window edge
      hs = new HalfSegment(true, bPoint, dp);

      attr.partnerno = partnerno;
      partnerno++;
      if ( (edge == WTOP) || (edge == WLEFT) )
        attr.insideAbove = !inside;
        //If inside == true, then insideAbove attribute of the top and left
        //half segments must be set to false, otherwise its value must be true.
        //In other words, the insideAbove atribute value is the opposite of the
        //parameter inside's value.
      else
        if ( (edge == WRIGHT) || (edge == WBOTTOM))
          attr.insideAbove = inside;
        //If inside == true, then insideAbove attribute of the right and bottom
        //half segments must be set to true, otherwise its value must be false.
        //In other words, the insideAbove atribute value is the same of the
        //parameter inside's value.
      hs->SetAttr(attr);
      cr+=(*hs);
      hs->SetLeftDomPoint( !hs->IsLeftDomPoint() );
      cr+=(*hs);
      delete hs;
    }
    begin++;
    //The variable ~begin~ must be incremented until exists
    // points with the same coordinates
    //and directions as dp
    while (begin<=end)
    {
      dpAux = pointsOnEdge[begin];
      if (!( (dpAux.GetX() == dp.GetX()) && (dpAux.GetY() == dp.GetY())
            && (dpAux.direction==dp.direction) ) )
        break;
      begin++;
    }
  }


  dp = pointsOnEdge[end];
  if ( !dp.direction) //dp points to right or up
  {
    bool rejectEndPoint=dp.rejected;
    end--;

    while ( end >= begin )
    {
      dpAux = pointsOnEdge[end];
      if ( !( (dpAux.GetX() == dp.GetX() ) && ( dpAux.GetY() == dp.GetY() ) &&
              (dpAux.direction==dp.direction) ) )
         break;

      //when a rejected point is found the rejectEndPoint
      //does not change anymore.

      end--;
    }

    if (!rejectEndPoint)
    {
      hs = new HalfSegment(true, dp, ePoint);
      attr.partnerno = partnerno;
      if ( (edge == WTOP) || (edge == WLEFT) )
        attr.insideAbove = !inside;
      else
        if ( (edge == WRIGHT) || (edge == WBOTTOM))
          attr.insideAbove = inside;
      partnerno++;
      hs->SetAttr(attr);
      cr+=(*hs);
      hs->SetLeftDomPoint( !hs->IsLeftDomPoint() );
      cr+=(*hs);

      delete hs;
    }
  }

  i = begin;
  while (i < end)
  {
    EdgePoint ep1,ep2;
    if ( GetAcceptedPoint(pointsOnEdge,i,end,ep1) )
    {
      i++;
      if (GetAcceptedPoint(pointsOnEdge,i,end, ep2) )
        i++;
      else
        break;
    }
    else
      break;
    if ( ! ( (ep1.GetX() == ep2.GetX()) && (ep1.GetY() == ep2.GetY()) ) )
    {  //discard degenerated edges
      hs = new HalfSegment(true, ep1, ep2);
      attr.partnerno = partnerno;
      partnerno++;
      if ( (edge == WTOP) || (edge == WLEFT) )
        attr.insideAbove = !inside;
      else
        if ( (edge == WRIGHT) || (edge == WBOTTOM))
          attr.insideAbove = inside;
      hs->SetAttr(attr);
      cr+=(*hs);
      hs->SetLeftDomPoint( !hs->IsLeftDomPoint() );
      cr+=(*hs);
      delete hs;
    }
  }
}

void Region::CreateNewSegmentsWindowVertices(const Rectangle<2> &window,
                                vector<EdgePoint> pointsOnEdge[4],Region &cr,
                                int &partnerno,bool inside) const
//The inside attribute indicates if the points on edge will originate
//segments that are inside the window (its values is true), or outside
//the window (its value is false)
{
  Point tlPoint(true,window.MinD(0),window.MaxD(1)),
        trPoint(true,window.MaxD(0),window.MaxD(1)),
        blPoint(true,window.MinD(0),window.MinD(1)),
        brPoint(true,window.MaxD(0),window.MinD(1));
   bool tl=false, tr=false, bl=false, br=false;

  /*
  cout<<endl<<"interno"<<endl;
  cout<<"Left   :"<<window.MinD(0)<<endl;
  cout<<"Top    :"<<window.MaxD(1)<<endl;
  cout<<"Right  :"<<window.MaxD(0)<<endl;
  cout<<"Bottom :"<<window.MinD(1)<<endl;

  cout<<"Points"<<endl;
  cout<<"tlPoint: "<<tlPoint<<endl;
  cout<<"trPoint: "<<trPoint<<endl;
  cout<<"blPoint: "<<blPoint<<endl;
  cout<<"brPoint: "<<brPoint<<endl;

  */

  AttrType attr;

  if ( ( (pointsOnEdge[WTOP].size()==0) ||
         (pointsOnEdge[WLEFT].size()==0) )
     && ( this->Contains(tlPoint) ) )
      tl = true;

  if ( ( (pointsOnEdge[WTOP].size()==0) ||
         (pointsOnEdge[WRIGHT].size()==0)  )
       && ( this->Contains(trPoint) ) )
      tr = true;

  if ( ( (pointsOnEdge[WBOTTOM].size()==0) ||
         (pointsOnEdge[WLEFT].size()==0)  )
       && ( this->Contains(blPoint) ) )
      bl = true;
  if ( ( (pointsOnEdge[WBOTTOM].size()==0) ||
         (pointsOnEdge[WRIGHT].size()==0)  )
         && ( this->Contains(brPoint) ) )
      br = true;


  //Create top edge
  if (tl && tr && (pointsOnEdge[WTOP].size()==0))
  {
    HalfSegment *hs;
    hs = new HalfSegment(true, tlPoint, trPoint);
    //If inside == true, then insideAbove attribute of the top and left
    //half segments must be set to false, otherwise its value must be true.
    //In other words, the insideAbove atribute value is the opposite of the
    //inside function's parameter value.
    attr.insideAbove = !inside;
    attr.partnerno = partnerno;
    partnerno++;

    hs->SetAttr(attr);
    cr+=(*hs);
    hs->SetLeftDomPoint( !hs->IsLeftDomPoint() );
    cr+=(*hs);
    delete hs;
  }
  //Create left edge
  if (tl && bl && (pointsOnEdge[WLEFT].size()==0))
  {
    HalfSegment *hs;
    hs = new HalfSegment(true, tlPoint, blPoint);
    //If inside == true, then insideAbove attribute of the top and left
    //half segments must be set to false, otherwise its value must be true.
    //In other words, the insideAbove atribute value is the opposite of the
    //parameter inside's value.
    attr.insideAbove = !inside;
    attr.partnerno = partnerno;
    partnerno++;

    hs->SetAttr(attr);
    cr+=(*hs);
    hs->SetLeftDomPoint( !hs->IsLeftDomPoint() );
    cr+=(*hs);
    delete hs;
  }
  //Create right edge
  if (tr && br && (pointsOnEdge[WRIGHT].size()==0))
  {
    HalfSegment *hs;
    hs = new HalfSegment(true, trPoint, brPoint);
    //If inside == true, then insideAbove attribute of the right and bottom
    //half segments must be set to true, otherwise its value must be false.
    //In other words, the insideAbove atribute value is the same of the
    //parameter inside's value.
    attr.insideAbove = inside;
    attr.partnerno = partnerno;
    partnerno++;

    hs->SetAttr(attr);
    cr+=(*hs);
    hs->SetLeftDomPoint( !hs->IsLeftDomPoint() );
    cr+=(*hs);
    delete hs;
  }
  //Create bottom edge
  if (bl && br && (pointsOnEdge[WBOTTOM].size()==0))
  {
    HalfSegment *hs;
    hs = new HalfSegment(true, blPoint, brPoint);
    //If inside == true, then insideAbove attribute of the right and bottom
    //half segments must be set to true, otherwise its value must be false.
    //In other words, the insideAbove atribute value is the same of the
    //parameter inside's value.
    attr.insideAbove = inside;
    attr.partnerno = partnerno;
    partnerno++;

    hs->SetAttr(attr);
    cr+=(*hs);
    hs->SetLeftDomPoint( !hs->IsLeftDomPoint() );
    cr+=(*hs);
    delete hs;
  }
}

bool Region::ClippedHSOnEdge(const Rectangle<2> &window,const HalfSegment &hs,
                             bool clippingIn,vector<EdgePoint> pointsOnEdge[4])
{
//This function returns true if the segment lies on one of the window's edge.
// The clipped half segments that lie on the edges must be rejected according to
// the kind of clipping (returning the portion of the region that is inside the
// region or the portion that is outside).

  EdgePoint ep1,ep2;
  AttrType attr=hs.GetAttr();
  bool reject = false,
       result = false;
  //Returns true if the clipped hs was treated as a segment on edge
  if ( hs.GetLeftPoint().GetY() == hs.GetRightPoint().GetY() )
  { //horizontal edge
    if (( hs.GetLeftPoint().GetY() == window.MaxD(1) ) )
    { //top edge
  // If the half segment lies on the upper edge and
  // the insideAbove attribute's value
  // is true then the region's area is outside the window,
  // and the half segment mustn't
  // be included in the clipped region (Reject).
  // However, its end points maybe will have to be
  // connected to the vertices of the window.
  // It happens only when the vertice of the
  // window is inside the region and the end point
  // is the first point on the window's
  // edge (for the upper-left vertice) or the last
  // point on the window's vertice (for
  // the upper right edge).
      if ( clippingIn && attr.insideAbove )
        reject = true;
      else
        if ( !clippingIn && !attr.insideAbove )
          reject = true;
      ep1.Set(hs.GetLeftPoint(),false,reject); //--> right
      ep2.Set(hs.GetRightPoint(),true,reject);  //<-- left
      pointsOnEdge[WTOP].push_back(ep1);
      pointsOnEdge[WTOP].push_back(ep2);
      result = true;
    }
    else //bottom edge
      if (( hs.GetLeftPoint().GetY() == window.MinD(1) ) )
      {
        if ( clippingIn && !attr.insideAbove )
           reject = true;
        else
          if ( !clippingIn && attr.insideAbove )
            reject = true;
        ep1.Set(hs.GetLeftPoint(),false,reject); //--> right
        ep2.Set(hs.GetRightPoint(),true,reject);  //<-- left
        pointsOnEdge[WBOTTOM].push_back(ep1);
        pointsOnEdge[WBOTTOM].push_back(ep2);
        result = true;
      }
  }
  else //Vertical edges
    if ( hs.GetLeftPoint().GetX() == hs.GetRightPoint().GetX() )
    {
      if ( hs.GetLeftPoint().GetX() == window.MinD(0) ) //Left edge
      {
        if ( clippingIn && attr.insideAbove )
          reject = true;
        else
          if (!clippingIn && !attr.insideAbove )
            reject = true;
        ep1.Set(hs.GetLeftPoint(),false,reject); //^ up
        ep2.Set(hs.GetRightPoint(),true,reject);  //v dowb
        pointsOnEdge[WLEFT].push_back(ep1);
        pointsOnEdge[WLEFT].push_back(ep2);
        result = true;
      }
      else
        if ( hs.GetLeftPoint().GetX() == window.MaxD(0) ) //Right edge
        {
          if ( clippingIn && !attr.insideAbove )
            reject = true;
          else
            if ( !clippingIn && attr.insideAbove )
              reject = true;
          ep1.Set(hs.GetLeftPoint(),false,reject); //^ up
          ep2.Set(hs.GetRightPoint(),true,reject);  //v dowb
          pointsOnEdge[WRIGHT].push_back(ep1);
          pointsOnEdge[WRIGHT].push_back(ep2);
          result = true;
        }
    }
  return result;
}

bool Region::GetCycleDirection( const Point &pA,
                                const Point &pP,
                                const Point &pB )
{
  double m_p_a,m_p_b;
  if (pA.GetX() == pP.GetX())//A --> P is a vertical segment
    if (pA.GetY() > pP.GetY() ) //A --> P directed downwards (case 1)
      return false; //Counterclockwise
    else //upwards (case 2)
      return true; // Clockwise
  if (pB.GetX() == pP.GetX()) //P --> B is a vertical segment
    if ( pP.GetY() > pB.GetY()) //downwords (case 3)
      return false; //Conterclockwise
    else //upwards
      return true; //Clockwise

  //compute the slopes of P-->A and P-->B
  m_p_a = ( pA.GetY() - pP.GetY() ) / ( pA.GetX() - pP.GetX() );
  m_p_b = ( pB.GetY() - pP.GetY() ) / ( pB.GetX() - pP.GetX() );
  if (m_p_a > m_p_b) //case 5
    return false;//counterclockwise
  else  //case 6
    return true; //clockwise
}

bool Region::GetCycleDirection() const
{
/*
Preconditions:
* The region must represent just one cycle!!!!
* It is need that the edgeno stores the order that the half segments were typed, and
the half segments must be sorted in the half segment order. In other words if
hs1.attr.edgeno is less than hs2.attr.edgeno then hs1 was typed first than hs2.

This function has the purpose of choosing the A, P, and B points in order to call the
function that really computes the cycle direction.
As the point P is leftmost point then it is the left point of hs1 or the left point
of hs2 because in the half segment order these two points are equal.
Now the problem is to decide which of the right points are A and B. At the first sight
we could say that the point A is the right point of the half segment with lowest
partner number. However it is not true ever because the APB connected points may be go over the
bound of the pointlist. This will be the case if the cycle is in the form P,B,..,A
and B,...,A,P. Nevertheless the segments are ordered in the half segment order, and when the
last half segment is been considered for choosing the APB connected points, the point A will be
always the right point of the last segment.

*/
  Point pA, pP, pB;
  const HalfSegment *hs1, *hs2;
  this->Get(0,hs1);
  this->Get(1,hs2);
  assert( hs1->GetLeftPoint()==hs2->GetLeftPoint() );
  pP = hs1->GetLeftPoint();
  // If we have the last half segment connected to the first half
  // segment, the difference //between their partner numbers is
  // more than one.
  if (abs(hs1->attr.edgeno - hs2->attr.edgeno)>1)
  {
    if (hs1->attr.edgeno > hs2->attr.edgeno)
    {
      pA = hs1->GetRightPoint();
      pB = hs2->GetRightPoint();
    }
    else
    {
      pA = hs2->GetRightPoint();
      pB = hs1->GetRightPoint();
    }
  }
  else
    if (hs1->attr.edgeno < hs2->attr.edgeno)
    {
      pA = hs1->GetRightPoint();
      pB = hs2->GetRightPoint();
    }
    else
    {
      pA = hs2->GetRightPoint();
      pB = hs1->GetRightPoint();
    }
  return GetCycleDirection(pA,pP,pB);
}

   //cycleDirection: true (cycle is clockwise) / false
  // (cycle is counterclockwise)
  //It is need that the attribute insideAbove of the half segments represents
  //the order that  their points were typed: true (left point, right point) /
  //false (right point, left point).




void Region::GetClippedHSIn(const Rectangle<2> &window,
                            Region &clippedRegion,
                            vector<EdgePoint> pointsOnEdge[4],
                            int &partnerno) const
{
  const HalfSegment *hs;
  HalfSegment hsInside;
  bool inside, isIntersectionPoint;

  SelectFirst();
  for(int i=0; i < Size(); i++)
  {
    GetHs( hs );
    if (hs->IsLeftDomPoint())
    {
      Point intersectionPoint;
      hs->WindowClippingIn(window, hsInside, inside,
                           isIntersectionPoint,intersectionPoint);
      if (isIntersectionPoint)
         AddPointToEdgeArray(intersectionPoint,*hs,window, pointsOnEdge);
      else
        if ( inside )
        {
          bool hsOnEdge = ClippedHSOnEdge(window,hsInside,true,pointsOnEdge);
          if (!hsOnEdge)
          {
            //Add the clipped segment to the new region if it was not rejected
            hsInside.attr.partnerno=partnerno;
            partnerno++;
            hsInside.SetAttr(hsInside.attr);
            clippedRegion += hsInside;
            hsInside.SetLeftDomPoint( !hsInside.IsLeftDomPoint() );
            clippedRegion += hsInside;

            //Add the points to the array of the points that lie on
            //some of the window's edges
            const Point& lp = hsInside.GetLeftPoint(),
                             rp = hsInside.GetRightPoint();

            //If the point lies on one edge it must be added to the
            //corresponding vector.
            AddPointToEdgeArray(lp,*hs,window, pointsOnEdge);
            AddPointToEdgeArray(rp, *hs,window, pointsOnEdge);
          }
        }
    }
    SelectNext();
  }
}

void Region::AddClippedHS( const Point &pl,
                           const Point &pr,
                           AttrType &attr,
                           int &partnerno )
{
  HalfSegment hs(true,pl,pr);
  attr.partnerno = partnerno;
  partnerno++;
  hs.SetAttr(attr);
  (*this)+=hs;
  hs.SetLeftDomPoint( !hs.IsLeftDomPoint() );
  (*this)+=hs;
}

void Region::GetClippedHSOut(const Rectangle<2> &window,
                             Region &clippedRegion,
                             vector<EdgePoint> pointsOnEdge[4],
                             int &partnerno) const
{
  for (int i=0; i < Size();i++)
  {
    const HalfSegment *hs;
    HalfSegment hsInside;
    bool inside=false,isIntersectionPoint=false;
    Get(i,hs);

    if (hs->IsLeftDomPoint())
    {
      Point intersectionPoint;
      hs->WindowClippingIn(window,hsInside, inside,
                           isIntersectionPoint,intersectionPoint);
      if (inside)
      {
        bool hsOnEdge=false;
        if (isIntersectionPoint)
        {
          HalfSegment aux( *hs );
          if (hs->GetLeftPoint()!=intersectionPoint)
            clippedRegion.AddClippedHS(aux.GetLeftPoint(),
                                       intersectionPoint,
                                       aux.attr,partnerno) ;
          if (hs->GetRightPoint()!=intersectionPoint)
            clippedRegion.AddClippedHS(intersectionPoint,
                                       aux.GetRightPoint(),
                                       aux.attr,partnerno);
          AddPointToEdgeArray(intersectionPoint,aux,
                              window, pointsOnEdge);
        }
        else
        {
          hsOnEdge = ClippedHSOnEdge(window, hsInside,
                                     false, pointsOnEdge);
          if (!hsOnEdge)
          {
            HalfSegment aux( *hs );
            if (hs->GetLeftPoint()!=hsInside.GetLeftPoint())
             //Add the part of the half segment composed by the left
             //point of hs and the left point of hsInside.
              clippedRegion.AddClippedHS(aux.GetLeftPoint(),
                                         hsInside.GetLeftPoint(),
                                         aux.attr,partnerno) ;
            AddPointToEdgeArray(hsInside.GetLeftPoint(),aux,
                                window, pointsOnEdge);
            if (hs->GetRightPoint()!=hsInside.GetRightPoint())
             //Add the part of the half segment composed by the right
             //point of hs and the right point of hsInside.
              clippedRegion.AddClippedHS(hsInside.GetRightPoint(),
                                         aux.GetRightPoint(),
                                         aux.attr,partnerno);

            AddPointToEdgeArray(hsInside.GetRightPoint(),aux,
                                window, pointsOnEdge);
          }
        }
      }
      else
      {
        HalfSegment aux( *hs );
        clippedRegion.AddClippedHS(aux.GetLeftPoint(),
                                   aux.GetRightPoint(),
                                   aux.attr,partnerno);
      }
    }
    SelectNext();
  }
}

void Region::GetClippedHS(const Rectangle<2> &window,
                          Region &clippedRegion,
                          bool inside) const
{
  vector<EdgePoint> pointsOnEdge[4];
    //upper edge, right edge, bottom, left
  int partnerno=0;

  clippedRegion.StartBulkLoad();

  if (inside)
    GetClippedHSIn(window,clippedRegion,pointsOnEdge,partnerno);
  else
    GetClippedHSOut(window,clippedRegion,pointsOnEdge,partnerno);


  Point bPoint,ePoint;
  bPoint.Set(window.MinD(0),window.MaxD(1)); //left-top
  ePoint.Set(window.MaxD(0),window.MaxD(1)); //right-top
  CreateNewSegments(pointsOnEdge[WTOP],clippedRegion,bPoint,ePoint,
                    WTOP,partnerno,inside);
  bPoint.Set(window.MinD(0),window.MinD(1)); //left-bottom
  ePoint.Set(window.MaxD(0),window.MinD(1)); //right-bottom
  CreateNewSegments(pointsOnEdge[WBOTTOM],clippedRegion,bPoint,ePoint,
                    WBOTTOM,partnerno,inside);
  bPoint.Set(window.MinD(0),window.MinD(1)); //left-bottom
  ePoint.Set(window.MinD(0),window.MaxD(1)); //left-top
  CreateNewSegments(pointsOnEdge[WLEFT],clippedRegion,bPoint,ePoint,
                    WLEFT,partnerno,inside);
  bPoint.Set(window.MaxD(0),window.MinD(1)); //right-bottom
  ePoint.Set(window.MaxD(0),window.MaxD(1)); //right-top
  CreateNewSegments(pointsOnEdge[WRIGHT],clippedRegion,bPoint,ePoint,
                     WRIGHT,partnerno,inside);

  CreateNewSegmentsWindowVertices(window, pointsOnEdge,clippedRegion,
                                  partnerno,inside);

  clippedRegion.EndBulkLoad();
}

bool Region::IsCriticalPoint( const Point &adjacentPoint,
                              const int hsPosition ) const
{
  int adjPosition = hsPosition,
      adjacencyNo = 0,
      step = 1;
  do
  {
    const HalfSegment *adjCHS;
    adjPosition+=step;
    if ( adjPosition<0 || adjPosition>=this->Size())
      break;
    Get(adjPosition,adjCHS);
    if (!adjCHS->IsLeftDomPoint())
      continue;
    AttrType attr = adjCHS->GetAttr();
    //When looking for critical points, the partner of
    //the adjacent half segment found
    //cannot be consired.
    if (attr.partnerno == hsPosition)
      continue;
    if ( ( adjacentPoint==adjCHS->GetLeftPoint() ) ||
         ( adjacentPoint==adjCHS->GetRightPoint() ) )
      adjacencyNo++;
    else
    {
      if (step==-1)
        return false;
      step=-1;
      adjPosition=hsPosition;
    }
  }
  while (adjacencyNo<2);

  return (adjacencyNo>1);
}

bool Region::GetAdjacentHS( const HalfSegment &hs,
                            const int hsPosition,
                            int &position,
                            const int partnerno,
                            const int partnernoP,
                            HalfSegment const*& adjacentCHS,
                            const Point &adjacentPoint,
                            Point &newAdjacentPoint,
                            bool *cycle,
                            int step) const
{
  bool adjacencyFound=false;
  do
  {
    position+=step;
    if ( position<0 || position>=this->Size())
      break;

    Get(position,adjacentCHS);
    if (partnernoP == position)
      continue;
    if ( adjacentPoint==adjacentCHS->GetLeftPoint() )
    {
      if (!cycle[position])
      {
        newAdjacentPoint = adjacentCHS->GetRightPoint();
        adjacencyFound = true;
      }
    }
    else
      if  ( adjacentPoint==adjacentCHS->GetRightPoint() )
      {
         if (!cycle[position])
        {
          newAdjacentPoint = adjacentCHS->GetLeftPoint();
          adjacencyFound = true;
        }
      }
      else
        break;
  }
  while (!adjacencyFound);
  return adjacencyFound;
}
/*
The parameter ~hasCriticalPoint~ indicates that the cycle that
is been computed has a critical point.

*/

void Region::ComputeCycle( HalfSegment &hs,
                           int faceno,
                           int cycleno,
                           int &edgeno,
                           bool *cycle )
{
  Point nextPoint = hs.GetLeftPoint(),
            lastPoint = hs.GetRightPoint(),
            previousPoint, *currentCriticalPoint=NULL;
  AttrType attr, attrP;
  const HalfSegment *hsP;
  vector<SCycle> sCycleVector;
  SCycle *s=NULL;

  do
  {
     if (s==NULL)
     {
       //Update attributes
       attr = hs.GetAttr();

       Get(attr.partnerno,hsP);
       attrP = hsP->GetAttr();

       attr.faceno=faceno;
       attr.cycleno=cycleno;
       attr.edgeno=edgeno;

       UpdateAttr(attrP.partnerno,attr);

       attrP.faceno=faceno;
       attrP.cycleno=cycleno;
       attrP.edgeno=edgeno;
       UpdateAttr(attr.partnerno,attrP);

       edgeno++;

       cycle[attr.partnerno]=true;
       cycle[attrP.partnerno]=true;

       if (this->IsCriticalPoint(nextPoint,attrP.partnerno))
         currentCriticalPoint = new Point(nextPoint);

       s = new SCycle(hs,attr.partnerno,*hsP,attrP.partnerno,
                      currentCriticalPoint,nextPoint);
     }
     const HalfSegment *adjacentCHS;
     Point adjacentPoint;
     bool adjacentPointFound=false;
     previousPoint = nextPoint;
     if (s->goToCHS1Right)
     {
       s->goToCHS1Right=GetAdjacentHS(s->hs1,
                                      s->hs2Partnerno,
                                      s->hs1PosRight,
                                      s->hs1Partnerno,
                                      s->hs2Partnerno,adjacentCHS,
                                      previousPoint,
                                      nextPoint,
                                      cycle,
                                      1);
       adjacentPointFound=s->goToCHS1Right;
     }
     if ( !adjacentPointFound && s->goToCHS1Left )
     {
       s->goToCHS1Left=GetAdjacentHS(s->hs1,
                                     s->hs2Partnerno,
                                     s->hs1PosLeft,
                                     s->hs1Partnerno,
                                     s->hs2Partnerno,
                                     adjacentCHS,
                                     previousPoint,
                                     nextPoint,
                                     cycle,
                                     -1);
       adjacentPointFound=s->goToCHS1Left;
     }
     if (!adjacentPointFound && s->goToCHS2Right)
     {
       s->goToCHS2Right=GetAdjacentHS(s->hs2,
                                      s->hs1Partnerno,
                                      s->hs2PosRight,
                                      s->hs2Partnerno,
                                      s->hs1Partnerno,
                                      adjacentCHS,
                                      previousPoint,
                                      nextPoint,
                                      cycle,
                                      1);
       adjacentPointFound=s->goToCHS2Right;
     }
     if (!adjacentPointFound && s->goToCHS2Left)
     {
       s->goToCHS2Left=GetAdjacentHS(s->hs2,
                                     s->hs1Partnerno,
                                     s->hs2PosLeft,
                                     s->hs2Partnerno,
                                     s->hs1Partnerno,
                                     adjacentCHS,
                                     previousPoint,
                                     nextPoint,
                                     cycle,
                                     -1);
       adjacentPointFound = s->goToCHS2Left;
     }

     if(!adjacentPointFound){
         cerr << "Problem in rebuilding cycle in a region " << endl;
         cerr << "no adjacent point found" << endl;
         cerr << "Halfsegments : ---------------     " << endl;
         const HalfSegment* hs;
         for(int i=0;i<Size();i++){
            Get(i,hs);
            cerr << i << " : " << (*hs) << endl;
         }
         assert(adjacentPointFound); // assert(false)
     }
     sCycleVector.push_back(*s);

     if ( (currentCriticalPoint!=NULL) && (*currentCriticalPoint==nextPoint) )
     {
       //The critical point defines a cycle, so it is need to
       //remove the segments
       //from the vector, and set the segment as not visited in thei
       // cycle array.
       //FirsAux is the first half segment with the critical point equals to
       //criticalPoint.
       SCycle sAux,firstSCycle;

       do
       {
          sAux=sCycleVector.back();
          sCycleVector.pop_back();
          firstSCycle=sCycleVector.back();
          if (firstSCycle.criticalPoint==NULL)
            break;
          if (*firstSCycle.criticalPoint!=*currentCriticalPoint)
            break;
          cycle[sAux.hs1Partnerno]=false;
          cycle[sAux.hs2Partnerno]=false;
          edgeno--;
       }while(sCycleVector.size()>1);
       delete s; //when s is deleted, the critical point is also deleted.
       s = 0;
       if (sCycleVector.size()==1)
       {
         sCycleVector.pop_back();
         if(s){
           delete s;
         }
         s = new SCycle(firstSCycle);
       }
       else{
         if(s){
           delete s;
         }
         s= new SCycle(sAux);
       }
       hs = s->hs1;
       currentCriticalPoint=s->criticalPoint;
       nextPoint=s->nextPoint;
       continue;
     }

     if ( nextPoint==lastPoint )
     {
       //Update attributes
       attr = adjacentCHS->GetAttr();

       Get(attr.partnerno,hsP);
       attrP = hsP->GetAttr();

       attr.faceno=faceno;
       attr.cycleno=cycleno;
       attr.edgeno=edgeno;

       UpdateAttr(attrP.partnerno,attr);

       attrP.faceno=faceno;
       attrP.cycleno=cycleno;
       attrP.edgeno=edgeno;
       UpdateAttr(attr.partnerno,attrP);

       edgeno++;

       cycle[attr.partnerno]=true;
       cycle[attrP.partnerno]=true;

       break;
     }
     hs = *adjacentCHS;
     delete s;
     s=NULL;
  }
  while(1);
  if(s){
    delete s;
    s = 0;
  }

}

//This function returns the value of the atribute inside above of
//the first half segment under the half segment hsS.
int Region::GetNewFaceNo(HalfSegment &hsS, bool *cycle)
{
  int coverno=0;
  int startpos=0;
  double y0;
  AttrType attr;
  vector<HalfSegment> v;

  //1. find the right place by binary search
  Find( hsS, startpos );

  int hsVisiteds=0;

  //2. deal with equal-x hs's
  //To verify if it is need to deal with this

  attr = hsS.GetAttr();
  coverno = attr.coverageno;

  //search the region value for coverageno steps
  int touchedNo=0;
  const HalfSegment *hs;
  const Point& p = hsS.GetLeftPoint();

  int i=startpos;
  while (( i>=0)&&(touchedNo<coverno))
  {
    this->Get(i, hs);
    hsVisiteds++;

    if ( (cycle[i]) && (hs->IsLeftDomPoint()) &&
         ( (hs->GetLeftPoint().GetX() <= p.GetX()) &&
         (p.GetX() <= hs->GetRightPoint().GetX()) ))
    {
      touchedNo++;
      if (!hs->RayAbove(p, y0))
        v.push_back(*hs);
    }
    i--;  //the iterator
  }
  if (v.size()==0)
    return -1; //the new face number will be the last face number +1
  else
  {
    sort(v.begin(),v.end());
    //The first half segment is the next half segment above hsS
    HalfSegment hs = v[v.size()-1];
    attr = hs.GetAttr();
    if (attr.insideAbove)
      return attr.faceno; //the new cycle is a cycle of the face ~attr.faceno~
    else
      return -1; //new face
  }
}

int Region::GetNewFaceNo(const HalfSegment& hsIn, const int startpos) const {
    
    // Precondition: 
    // hsIn is the smallest (in halfsegment-order) segment of a cycle.
    // startpos is the index of hsIn in the DBArray.
    
    if (hsIn.GetAttr().insideAbove) {
        
        // hsIn belongs to a new face:
        return -1;
    }
    
    // Now we know hsIn belongs to a new hole and we
    // have to encounter the enclosing face.
    // This is done by searching the next halfsegment maxHS 'under' hsIn.
    // Since we go downwards, the facenumber of maxHS must be already known
    // and is equal to the facenumber of hsIn.
    
    double y0;
    double maxY0;
    const HalfSegment *hs = 0;
    const HalfSegment *maxHS = 0;
    const Point& p = hsIn.GetLeftPoint();
    const int coverno = hsIn.GetAttr().coverageno;
    int touchedNo = 0;
    int i = startpos - 1;
    bool first = true;
    
    while (i >=0 && touchedNo < coverno) {
        
        Get(i, hs);
        
        if (!hs->IsLeftDomPoint()) {
            
            i--;
            continue;
        }

        if (hs->GetLeftPoint().GetX() <= p.GetX() &&
            p.GetX() <= hs->GetRightPoint().GetX()) {

            touchedNo++;
        }

        if (!AlmostEqual(hs->GetRightPoint().GetX(), p.GetX()) && 
            hs->RayDown(p, y0)) { 
            
            if (first || 
                y0 > maxY0 || 
                (AlmostEqual(y0, maxY0) && *hs > *maxHS)) {

                // To find the first halfsegment 'under' hsIn
                // we compare them as follows:
                // (1) y-value of the intersection point between a ray down 
                //     from the left point of hsIn and hs.
                // (2) halfsegment order.

                maxY0 = y0;
                maxHS = hs;
                first = false;
            }
        }
        
        i--;
    }

    if (maxHS == 0) {
        
        cerr << "Problem in rebuilding cycle in a region " << endl;
        cerr << "No outer cycle found" << endl;
        cerr << "hsIn: " << hsIn << endl;
        cerr << "Halfsegments : ---------------     " << endl;
        const HalfSegment* hs;

        for(int i=0;i<Size();i++) {

            Get(i,hs);
            cerr << i << " : " << (*hs) << endl;
        }

        assert(false);
    }
    
    //the new cycle is a holecycle of the face ~maxHS.attr.faceno~
    return maxHS->GetAttr().faceno;
}

bool HalfSegment::RayDown( const Point& p, double &yIntersection ) const
{
    if (this->IsVertical())
          return false;
    
    const Coord& x = p.GetX(), y = p.GetY(),
                 xl = GetLeftPoint().GetX(),
                 yl = GetLeftPoint().GetY(),
                 xr = GetRightPoint().GetX(),
                 yr = GetRightPoint().GetY();
  
    // between is true, iff xl <= x <= xr.
    const bool between = CompareDouble(x, xl) != -1 &&
                         CompareDouble(x, xr) != 1;
    
    if (!between)
        return false;
    
    const double k = (yr - yl) / (xr - xl);
    const double a = (yl - k * xl);
    const double y0 = k * x + a;
    
    if (CompareDouble(y0, y) == 1) // y0 > y: this is above p.
        return false;
    
    // y0 <= p: p is above or on this.

    yIntersection = y0;
    
    return true;
}



void Region::ComputeRegion()
{
  //array that stores in position i the last cycle number of the face i
  vector<int> face;
  //array that stores in the position ~i~ if the half
  //segment hi had already the face
  //number, the cycle number and the edge number
  //attributes set properly, in other words,
  //it means that hi is already part of a cycle
  bool *cycle;
  int lastfaceno=0,
      faceno=0,
      cycleno = 0,
      edgeno = 0;
  bool isFirstCHS=true;
  const HalfSegment *hs;

  if (Size()==0)
    return;
   //Insert in the vector the first cycle of the first face
  face.push_back(0);
  cycle = new bool[Size()];
#ifdef SECONDO_MAC_OSX
  // something goes wrong at mac osx and the memset function
  int size = Size();
  for(int i=0;i<size;i++){
    cycle[i] = false;
  }
#else
  memset( cycle, false, Size() );
#endif
  for ( int i=0; i<Size(); i++)
  {
    Get(i,hs);
    HalfSegment aux(*hs);
    if ( aux.IsLeftDomPoint() && !cycle[i])
    {
      if(!isFirstCHS)
      {
        int facenoAux = GetNewFaceNo(aux,i);
        if (facenoAux==-1)
        {/*The lhs half segment will start a new face*/
          lastfaceno++;
          faceno = lastfaceno;
           /*to store the first cycle number of the face lastFace*/
          face.push_back(0);
          cycleno = 0;
          edgeno = 0;
        }
        else
        { /*The half segment ~hs~ belongs to an existing face*/
          faceno = facenoAux;
          face[faceno]++;
          cycleno = face[faceno];
          edgeno = 0;
        }
      }
      else
      {
        isFirstCHS = false;
      }
      ComputeCycle(aux, faceno,cycleno, edgeno, cycle);
    }
  }
  delete [] cycle;
  noComponents = lastfaceno + 1;
}

void Region::WindowClippingIn(const Rectangle<2> &window,
                              Region &clippedRegion) const
{
  //cout<<endl<<"Original: "<<*this<<endl;
  if (!this->bbox.Intersects(window))
    return;
  //If the bounding box of the region is inside the window,
  //then the clippedRegion
  //is equal to the region been clipped.

  if (window.Contains(this->bbox))
    clippedRegion = *this;
  else
  {
    //cout<<endl<<this->bbox<<endl;
    this->GetClippedHS(window,clippedRegion,true);
    //cout<<endl<<"Clipped HS:"<<endl<<clippedRegion;
    clippedRegion.ComputeRegion();
  }
  //cout<<endl<<"Clipped;"<<clippedRegion;
}

void Region::WindowClippingOut(const Rectangle<2> &window,
                               Region &clippedRegion) const
{
  //If the bounding box of the region is inside the window,
  //then the clipped region is empty
  //cout<<"region: "<<*this<<endl;
  if (window.Contains(this->bbox))
    return;
  if (!window.Intersects(this->bbox))
    clippedRegion = *this;
  else
  {
    this->GetClippedHS(window,clippedRegion,false);
 //   cout<<endl<<"Clipped HS:"<<endl<<clippedRegion;
    clippedRegion.ComputeRegion();
  }
 // cout<<endl<<"clippedRegion: "<<clippedRegion;
}

size_t   Region::HashValue() const
{
  //cout<<"cregion hashvalue1*******"<<endl;
  if(IsEmpty())
    return 0;

  unsigned long h=0;
  const HalfSegment *hs;
  Coord x1, y1;
  Coord x2, y2;

  for( int i = 0; ((i < Size())&&(i<5)); i++ )
  {
    Get( i, hs );
    x1=hs->GetLeftPoint().GetX();
    y1=hs->GetLeftPoint().GetY();

    x2=hs->GetRightPoint().GetX();
    y2=hs->GetRightPoint().GetY();
    h=h+(unsigned long)((5*x1 + y1)+ (5*x2 + y2));
  }
  return size_t(h);
}

void Region::Clear()
{
  region.Clear();
  pos = -1;
  ordered = true;
  bbox.SetDefined(false);
}

void Region::SetEmpty()
{
  region.SetEmpty();
  pos = -1;
  ordered = true;
  bbox.SetDefined(false);
}


void Region::CopyFrom( const StandardAttribute* right )
{
  *this = *(const Region *)right;
}

int Region::Compare( const Attribute* arg ) const
{
  int res=0;
  Region* cr = (Region* )(arg);
  if ( !cr )
    return -2;

  if (IsEmpty() && (cr->IsEmpty()))
    res=0;
  else if (IsEmpty())
    res=-1;
  else  if ((cr->IsEmpty()))
    res=1;
  else
  {
    if (Size() > cr->Size())
      res=1;
    else if (Size() < cr->Size())
      res=-1;
    else  //their sizes are equal
    {
      int bboxCmp = bbox.Compare( &cr->bbox );
      if( bboxCmp == 0 )
      {
        bool decided = false;
        for( int i = 0; ((i < Size())&&(!decided)); i++ )
        {
          const HalfSegment *hs1, *hs2;
          Get( i, hs1);
          cr->Get( i, hs2 );

          if (*hs1 >*hs2)
          {
            res=1;
            decided=true;
          }
          else if (*hs1 < *hs2)
          {
            res=-1;
            decided=true;
          }
        }
        if (!decided)
          res=0;
      }
      else
        res = bboxCmp;
    }
  }
  return res;
}

ostream& Region::Print( ostream &os ) const
{
  os << "<";
  for( int i = 0; i < Size(); i++ )
  {
    const HalfSegment *hs;
    Get( i, hs );
    os << " " << *hs;
  }
  os << ">";
  return os;
}

Region *Region::Clone() const
{
  return new Region( *this );
}

bool Region::InsertOk( const HalfSegment& hs ) const
{
  const HalfSegment *auxhs;
  double dummyy0;

  return true;
    //the check is closed temporarily to import data.

  int prevcycleMeet[50];

  int prevcyclenum=0;
  for( int i = 0; i < 50; i++ )
    prevcycleMeet[i]=0;

  for( int i = 0; i<= region.Size()-1; i++ )
  {
    region.Get( i, auxhs );

    if (auxhs->IsLeftDomPoint())
    {
      if (hs.Intersects(*auxhs))
      {
        if ((hs.attr.faceno!=auxhs->attr.faceno)||
            (hs.attr.cycleno!=auxhs->attr.cycleno))
        {
          cout<<"two cycles intersect with the ";
          cout<<"following edges:";
          cout<<*auxhs<<" :: "<<hs<<endl;
          return false;
        }
        else
        {
          if ((auxhs->GetLeftPoint()!=hs.GetLeftPoint()) &&
              (auxhs->GetLeftPoint()!=hs.GetRightPoint()) &&
              (auxhs->GetRightPoint()!=hs.GetLeftPoint()) &&
              (auxhs->GetRightPoint()!=hs.GetRightPoint()))
          {
            cout<<"two edges: " <<*auxhs<<" :: "<< hs
                <<" of the same cycle intersect in middle!"
                <<endl;
            return false;
          }
        }
      }
      else
      {
        if ((hs.attr.cycleno>0) &&
            (auxhs->attr.faceno==hs.attr.faceno) &&
            (auxhs->attr.cycleno!=hs.attr.cycleno))
        {
          if (auxhs->RayAbove(hs.GetLeftPoint(), dummyy0))
          {
            prevcycleMeet[auxhs->attr.cycleno]++;
            if (prevcyclenum < auxhs->attr.cycleno)
              prevcyclenum=auxhs->attr.cycleno;
          }
        }
      }
    }
  }

  if ((hs.attr.cycleno>0))
  {
    if  (prevcycleMeet[0] % 2 ==0)
    {
      cout<<"hole(s) is not inside the outer cycle! "<<endl;
      return false;
    }
    for (int i=1; i<=prevcyclenum; i++)
    {
      if (prevcycleMeet[i] % 2 !=0)
      {
        cout<<"one hole is inside another! "<<endl;
        return false;
      }
    }
  }
/*
Now we know that the new half segment is not inside any other previous holes of the
same face. However, whether this new hole contains any previous hole of the same
face is not clear. In the following we do this kind of check.

*/

  if (((hs.attr.faceno>0) || (hs.attr.cycleno>2)))
  {
    const HalfSegment *hsHoleNEnd, *hsHoleNStart;

    if (region.Size() ==0) return true;

    int holeNEnd=region.Size()-1;
    region.Get(holeNEnd, hsHoleNEnd );

    if  ((hsHoleNEnd->attr.cycleno>1) &&
         ((hs.attr.faceno!=hsHoleNEnd->attr.faceno)||
         (hs.attr.cycleno!=hsHoleNEnd->attr.cycleno)))
    {
      if (hsHoleNEnd->attr.cycleno>1)
      {
        int holeNStart=holeNEnd - 1;
        region.Get(holeNStart, hsHoleNStart );

        while ((hsHoleNStart->attr.faceno==hsHoleNEnd->attr.faceno) &&
               (hsHoleNStart->attr.cycleno==hsHoleNEnd->attr.cycleno)&&
               (holeNStart>0))
        {
          holeNStart--;
          region.Get(holeNStart, hsHoleNStart );
        }
        holeNStart++;

        int prevHolePnt=holeNStart-1;
        const HalfSegment *hsPrevHole, *hsLastHole;

        bool stillPrevHole = true;
        while ((stillPrevHole) && (prevHolePnt>=0))
        {
          region.Get(prevHolePnt, hsPrevHole );
          prevHolePnt--;

          if ((hsPrevHole->attr.faceno!= hsHoleNEnd->attr.faceno)||
              (hsPrevHole->attr.cycleno<=0))
          {
            stillPrevHole=false;
          }

          if (hsPrevHole->IsLeftDomPoint())
          {
            int holeNMeent=0;
            for (int i=holeNStart; i<=holeNEnd; i++)
            {
              region.Get(i, hsLastHole );
              if ((hsLastHole->IsLeftDomPoint())&&
                  (hsLastHole->RayAbove
                  (hsPrevHole->GetLeftPoint(), dummyy0)))
                holeNMeent++;
            }
            if  (holeNMeent % 2 !=0)
            {
              cout<<"one hole is inside another!!! "<<endl;
              return false;
            }
          }
        }
      }
    }
  }
  return true;
}


/*
~Shift~ Operator for ~ownertype~

*/

ostream& avlseg::operator<<(ostream& o, const avlseg::ownertype& owner){
   switch(owner){
      case avlseg::none   : o << "none" ; break;
      case avlseg::first  : o << "first"; break;
      case avlseg::second : o << "second"; break;
      case avlseg::both   : o << "both"; break;
      default     : assert(false);
   }
   return o;
}


/*
3 Implementation of ~AVLSegment~

*/


/*
3.1 Constructors

~Standard Constructor~

*/
  avlseg::AVLSegment::AVLSegment(){
    x1 = 0;
    x2 = 0;
    y1 = 0;
    y2 = 0;
    owner = none;
    insideAbove_first = false;
    insideAbove_second = false;
    con_below = 0;
    con_above = 0;
  }


/*
~Constructor~

This constructor creates a new segment from the given HalfSegment.
As owner only __first__ and __second__ are the allowed values.

*/

  avlseg::AVLSegment::AVLSegment(const HalfSegment* hs, ownertype owner){
     x1 = hs->GetLeftPoint().GetX();
     y1 = hs->GetLeftPoint().GetY();
     x2 = hs->GetRightPoint().GetX();
     y2 = hs->GetRightPoint().GetY();
     if( (AlmostEqual(x1,x2) && (y2<y2) ) || (x2<x2) ){// swap the entries
        double tmp = x1;
        x1 = x2;
        x2 = tmp;
        tmp = y1;
        y1 = y2;
        y2 = tmp;
     }
     this->owner = owner;
     switch(owner){
        case first: {
             insideAbove_first = hs->GetAttr().insideAbove;
             insideAbove_second = false;
             break;
        } case second: {
             insideAbove_second = hs->GetAttr().insideAbove;
             insideAbove_first = false;
             break;
        } default: {
             assert(false);
        }
     }
     con_below = 0;
     con_above = 0;
  }

/*
~Constructor~

Create a Segment only consisting of a single point.

*/

  avlseg::AVLSegment::AVLSegment(const Point* p, ownertype owner){
      x1 = p->GetX();
      x2 = x1;
      y1 = p->GetY();
      y2 = y1;
      this->owner = owner;
      insideAbove_first = false;
      insideAbove_second = false;
      con_below = 0;
      con_above = 0;
  }


/*
~Copy Constructor~

*/
   avlseg::AVLSegment::AVLSegment(const AVLSegment& src){
      Equalize(src);
   }



/*
3.3 Operators

*/

  avlseg::AVLSegment& avlseg::AVLSegment::operator=(
                                         const avlseg::AVLSegment& src){
    Equalize(src);
    return *this;
  }

  bool avlseg::AVLSegment::operator==(const avlseg::AVLSegment& s) const{
    return compareTo(s)==0;
  }

  bool avlseg::AVLSegment::operator<(const avlseg::AVLSegment& s) const{
     return compareTo(s)<0;
  }

  bool avlseg::AVLSegment::operator>(const avlseg::AVLSegment& s) const{
     return compareTo(s)>0;
  }

/*
3.3 Further Needful Functions

~Print~

This function writes this segment to __out__.

*/
  void avlseg::AVLSegment::Print(ostream& out)const{
    out << "Segment("<<x1<<", " << y1 << ") -> (" << x2 << ", " << y2 <<") "
        << owner << " [ " << insideAbove_first << ", "
        << insideAbove_second << "] con("
        << con_below << ", " << con_above << ")";

  }

/*

~Equalize~

The value of this segment is taken from the argument.

*/

  void avlseg::AVLSegment::Equalize( const avlseg::AVLSegment& src){
     x1 = src.x1;
     x2 = src.x2;
     y1 = src.y1;
     y2 = src.y2;
     owner = src.owner;
     insideAbove_first = src.insideAbove_first;
     insideAbove_second = src.insideAbove_second;
     con_below = src.con_below;
     con_above = src.con_above;
  }




/*
3.5 Geometric Functions

~crosses~

Checks whether this segment and __s__ have an intersection point of their
interiors.

*/
 bool avlseg::AVLSegment::crosses(const avlseg::AVLSegment& s) const{
   double x,y;
   return crosses(s,x,y);
 }

/*
~crosses~

This function checks whether the interiors of the related
segments are crossing. If this function returns true,
the parameters ~x~ and ~y~ are set to the intersection point.

*/
 bool avlseg::AVLSegment::crosses(const avlseg::AVLSegment& s,
                                  double& x, double& y) const{
    if(isPoint() || s.isPoint()){
      return false;
    }

    if(!xOverlaps(s)){
       return false;
    }
    if(overlaps(s)){ // a common line
       return false;
    }
    if(compareSlopes(s)==0){ // parallel or disjoint lines
       return false;
    }

    if(isVertical()){
        x = x1; // compute y for s
        y =  s.y1 + ((x-s.x1)/(s.x2-s.x1))*(s.y2 - s.y1);
        return !AlmostEqual(y1,y) && !AlmostEqual(y2,y) &&
               (y>y1)  && (y<y2)
               && !AlmostEqual(s.x1,x) && !AlmostEqual(s.x2,x) ;
    }

    if(s.isVertical()){
       x = s.x1;
       y = y1 + ((x-x1)/(x2-x1))*(y2-y1);
       return !AlmostEqual(y,s.y1) && !AlmostEqual(y,s.y2) &&
              (y>s.y1) && (y<s.y2) &&
              !AlmostEqual(x1,x) && !AlmostEqual(x2,x);
    }
    // avoid problems with rounding errors during computation of
    // the intersection point
    if(pointEqual(x1,y1,s.x1,s.y1)){
      return false;
    }
    if(pointEqual(x2,y2,s.x1,s.y1)){
      return false;
    }
    if(pointEqual(x1,y1,s.x2,s.y2)){
      return false;
    }
    if(pointEqual(x2,y2,s.x2,s.y2)){
      return false;
    }


    // both segments are non vertical
    double m1 = (y2-y1)/(x2-x1);
    double m2 = (s.y2-s.y1)/(s.x2-s.x1);
    double c1 = y1 - m1*x1;
    double c2 = s.y1 - m2*s.x1;
    double xs = (c2-c1) / (m1-m2);  // x coordinate of the intersection point

    x = xs;
    y = y1 + ((x-x1)/(x2-x1))*(y2-y1);

    return !AlmostEqual(x1,xs) && !AlmostEqual(x2,xs) && // not an endpoint
           !AlmostEqual(s.x1,xs) && !AlmostEqual(s.x2,xs) && // of any segment
           (x1<xs) && (xs<x2) && (s.x1<xs) && (xs<s.x2);
}

/*
~extends~

This function returns true, iff this segment is an extension of
the argument, i.e. if the right point of ~s~ is the left point of ~this~
and the slopes are equal.

*/
  bool avlseg::AVLSegment::extends(const avlseg::AVLSegment& s)const{
     return pointEqual(x1,y1,s.x2,s.y2) &&
            compareSlopes(s)==0;
  }

/*
~exactEqualsTo~

This function checks if s has the same geometry like this segment, i.e.
if both endpoints are equal.

*/
bool avlseg::AVLSegment::exactEqualsTo(const avlseg::AVLSegment& s)const{
  return pointEqual(x1,y1,s.x1,s.y1) &&
         pointEqual(x2,y2,s.x2,s.y2);
}

/*
~isVertical~

Checks whether this segment is vertical.

*/

 bool avlseg::AVLSegment::isVertical() const{
     return AlmostEqual(x1,x2);
 }

/*
~isPoint~

Checks if this segment consists only of a single point.

*/
  bool avlseg::AVLSegment::isPoint() const{
     return AlmostEqual(x1,x2) && AlmostEqual(y1,y2);
  }

/*
~length~

Returns the length of this segment.

*/
  double avlseg::AVLSegment::length(){
    return sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
  }


/*
~InnerDisjoint~

This function checks whether this segment and s have at most a
common endpoint.

*/

  bool avlseg::AVLSegment::innerDisjoint(const avlseg::AVLSegment& s)const{
      if(pointEqual(x1,y1,s.x2,s.y2)){ // common endpoint
        return true;
      }
      if(pointEqual(s.x1,s.y1,x2,y2)){ // common endpoint
        return true;
      }
      if(overlaps(s)){ // a common line
         return false;
      }
      if(compareSlopes(s)==0){ // parallel or disjoint lines
         return true;
      }
      if(ininterior(s.x1,s.y1)){
         return false;
      }
      if(ininterior(s.x2,s.y2)){
         return false;
      }
      if(s.ininterior(x1,y1)){
        return false;
      }
      if(s.ininterior(x2,y2)){
        return false;
      }
      if(crosses(s)){
         return false;
      }
      return true;

  }
/*
~Intersects~

This function checks whether this segment and ~s~ have at least a
common point.

*/

  bool avlseg::AVLSegment::intersects(const avlseg::AVLSegment& s)const{
      if(pointEqual(x1,y1,s.x2,s.y2)){ // common endpoint
        return true;
      }
      if(pointEqual(s.x1,s.y1,x2,y2)){ // common endpoint
        return true;
      }
      if(overlaps(s)){ // a common line
         return true;
      }
      if(compareSlopes(s)==0){ // parallel or disjoint lines
         return false;
      }

      if(isVertical()){
        double x = x1; // compute y for s
        double y =  s.y1 + ((x-s.x1)/(s.x2-s.x1))*(s.y2 - s.y1);
        return  ( (contains(x,y) && s.contains(x,y) ) );

      }
      if(s.isVertical()){
         double x = s.x1;
         double y = y1 + ((x-x1)/(x2-x1))*(y2-y1);
         return ((contains(x,y) && s.contains(x,y)));
      }

      // both segments are non vertical
      double m1 = (y2-y1)/(x2-x1);
      double m2 = (s.y2-s.y1)/(s.x2-s.x1);
      double c1 = y1 - m1*x1;
      double c2 = s.y1 - m2*s.x1;
      double x = (c2-c1) / (m1-m2);  // x coordinate of the intersection point
      double y = y1 + ((x-x1)/(x2-x1))*(y2-y1);
      return ( (contains(x,y) && s.contains(x,y) ) );
  }

/*
~overlaps~

Checks whether this segment and ~s~ have a common segment.

*/
   bool avlseg::AVLSegment::overlaps(const avlseg::AVLSegment& s) const{
      if(isPoint() || s.isPoint()){
         return false;
      }

      if(compareSlopes(s)!=0){
          return false;
      }
      // one segment is an extension of the other one
      if(pointEqual(x1,y1,s.x2,s.y2)){
          return false;
      }
      if(pointEqual(x2,y2,s.x1,s.y1)){
         return false;
      }
      return contains(s.x1,s.y1) || contains(s.x2,s.y2);
   }

/*
~ininterior~

This function checks whether the point defined by (x,y) is
part of the interior of this segment.

*/
   bool avlseg::AVLSegment::ininterior(const double x,const  double y)const{
     if(isPoint()){ // a point has no interior
       return false;
     }

     if(pointEqual(x,y,x1,y1) || pointEqual(x,y,x2,y2)){ // an endpoint
        return false;
     }

     if(!AlmostEqual(x,x1) && x < x1){ // (x,y) left of this
         return false;
     }
     if(!AlmostEqual(x,x2) && x > x2){ // (X,Y) right of this
        return false;
     }
     if(isVertical()){
       return (!AlmostEqual(y,y1) && (y>y1) &&
               !AlmostEqual(y,y2) && (y<y2));
     }
     double ys = getY(x);
     return AlmostEqual(y,ys);
   }


/*
~contains~

Checks whether the point defined by (x,y) is located anywhere on this
segment.

*/
   bool avlseg::AVLSegment::contains(const double x,const  double y)const{
     if(pointEqual(x,y,x1,y1) || pointEqual(x,y,x2,y2)){
        return true;
     }
     if(isPoint()){
       return false;
     }
     if(AlmostEqual(x1,x2)){ // vertical segment
        return (y>=y1) && (y <= y2);
     }
     // check if (x,y) is located on the line
     double res1 = (x-x1)*(y2-y1);
     double res2 = (y-y1)*(x2-x1);
     if(!AlmostEqual(res1,res2)){
         return false;
     }

     return ((x>x1) && (x<x2)) ||
            AlmostEqual(x,x1) ||
            AlmostEqual(x,x2);
   }

/*
3.6 Comparison

Compares this with s. The x intervals must overlap.

*/

 int avlseg::AVLSegment::compareTo(const avlseg::AVLSegment& s) const{

    if(!xOverlaps(s)){
      cerr << "Warning: compare AVLSegments with disjoint x intervals" << endl;
      cerr << "This may be a problem of roundig errors!" << endl;
      cerr << "*this = " << *this << endl;
      cerr << " s    = " << s << endl;
    }

    if(isPoint()){
      if(s.isPoint()){
        return comparePoints(x1,y1,s.x1,s.y1);
      } else {
        if(s.contains(x1,y1)){
           return 0;
        } else {
           double y = s.getY(x1);
           if(y1<y){
             return -1;
           } else {
             return 1;
           }
        }
      }
    }
    if(s.isPoint()){
      if(contains(s.x1,s.y1)){
        return 0;
      } else {
        double y = getY(s.x1);
        if(y<s.y1){
          return -1;
        } else {
          return 1;
        }
      }
    }


   if(overlaps(s)){
     return 0;
   }

    bool v1 = isVertical();
    bool v2 = s.isVertical();

    if(!v1 && !v2){
       double x = max(x1,s.x1); // the right one of the left coordinates
       double y_this = getY(x);
       double y_s = s.getY(x);
       if(!AlmostEqual(y_this,y_s)){
          if(y_this<y_s){
            return -1;
          } else  {
            return 1;
          }
       } else {
         int cmp = compareSlopes(s);
         if(cmp!=0){
           return cmp;
         }
         // if the segments are connected, the left segment
         // is the smaller one
         if(AlmostEqual(x2,s.x1)){
             return -1;
         }
         if(AlmostEqual(s.x2,x1)){
             return 1;
         }
         // the segments have an proper overlap
         return 0;
       }
   } else if(v1 && v2){ // both are vertical
      if(AlmostEqual(y1,s.y2) || (y1>s.y2)){ // this is above s
        return 1;
      }
      if(AlmostEqual(s.y1,y2) || (s.y1>y2)){ // s above this
        return 1;
      }
      // proper overlapping part
      return 0;
  } else { // one segment is vertical

    double x = v1? x1 : s.x1; // x coordinate of the vertical segment
    double y1 = getY(x);
    double y2 = s.getY(x);
    if(AlmostEqual(y1,y2)){
        return v1?1:-1; // vertical segments have the greatest slope
    } else if(y1<y2){
       return -1;
    } else {
       return 1;
    }
  }
 }


/*
~SetOwner~

This function changes the owner of this segment.

*/
  void avlseg::AVLSegment::setOwner(avlseg::ownertype o){
    this->owner = o;
  }

/*
3.7 Some ~Get~ Functions

~getInsideAbove~

Returns the insideAbove value for such segments for which this value is unique,
e.g. for segments having owner __first__ or __second__.

*/
  bool avlseg::AVLSegment::getInsideAbove() const{
      switch(owner){
        case first : return insideAbove_first;
        case second: return insideAbove_second;
        default : assert(false);
      }
  }

/*
3.8 Split Functions

~split~

This function splits two overlapping segments.
Preconditions:

1) this segment and ~s~ have to overlap.

2) the owner of this and ~s~ must be different

~left~, ~common~ and ~right~ will contain the
explicitely left part, a common part, and
an explecitely right part. The left and/or right part
my be empty. The existence can be checked using the return
value of this function. Let ret the return value. It holds:

  __ret | LEFT__: the left part exists

  __ret | COMMON__: the common part exist (always true)

  __ret | RIGHT__: the right part exists


The constants LEFT, COMMON, and RIGHT have been defined
earlier.

*/

  int avlseg::AVLSegment::split(const avlseg::AVLSegment& s,
                               avlseg::AVLSegment& left,
                               avlseg::AVLSegment& common,
                               avlseg::AVLSegment& right,
                               const bool checkOwner/* = true*/) const{

     assert(overlaps(s));
     if(checkOwner){
       assert( (this->owner==first && s.owner==second) ||
               (this->owner==second && s.owner==first));
     }


     int result = 0;



     int cmp = comparePoints(x1,y1,s.x1,s.y1);
     if(cmp==0){
        left.x1 = x1;
        left.y1 = y1;
        left.x2 = x1;
        left.y2 = y1;
     } else { // there is a left part
       result = result | avlseg::LEFT;
       if(cmp<0){ // this is smaller
         left.x1 = x1;
         left.y1 = y1;
         left.x2 = s.x1;
         left.y2 = s.y1;
         left.owner = this->owner;
         left.con_above = this->con_above;
         left.con_below = this->con_below;
         left.insideAbove_first = this->insideAbove_first;
         left.insideAbove_second = this->insideAbove_second;
       } else { // s is smaller than this
         left.x1 = s.x1;
         left.y1 = s.y1;
         left.x2 = this->x1;
         left.y2 = this->y1;
         left.owner = s.owner;
         left.con_above = s.con_above;
         left.con_below = s.con_below;
         left.insideAbove_first = s.insideAbove_first;
         left.insideAbove_second = s.insideAbove_second;
       }
     }

    // there is an overlapping part
    result = result | COMMON;
    cmp = comparePoints(x2,y2,s.x2,s.y2);
    common.owner = both;
    common.x1 = left.x2;
    common.y1 = left.y2;
    if(this->owner==first){
      common.insideAbove_first  = insideAbove_first;
      common.insideAbove_second = s.insideAbove_second;
    } else {
      common.insideAbove_first = s.insideAbove_first;
      common.insideAbove_second = insideAbove_second;
    }
    common.con_above = this->con_above;
    common.con_below = this->con_below;

    if(cmp<0){
       common.x2 = x2;
       common.y2 = y2;
    } else {
       common.x2 = s.x2;
       common.y2 = s.y2;
    }
    if(cmp==0){ // common right endpoint
        return result;
    }

    result = result | avlseg::RIGHT;
    right.x1 = common.x2;
    right.y1 = common.y2;
    if(cmp<0){ // right part comes from s
       right.owner = s.owner;
       right.x2 = s.x2;
       right.y2 = s.y2;
       right.insideAbove_first = s.insideAbove_first;
       right.insideAbove_second = s.insideAbove_second;
       right.con_below = s.con_below;
       right.con_above = s.con_above;
    }  else { // right part comes from this
       right.owner = this->owner;
       right.x2 = this->x2;
       right.y2 = this->y2;
       right.insideAbove_first = this->insideAbove_first;
       right.insideAbove_second = this->insideAbove_second;
       right.con_below = this->con_below;
       right.con_above = this->con_above;
    }
   return result;


  }

/*
~splitAt~

This function divides a segment into two parts at the point
provided by (x, y). The point must be on the interior of this segment.

*/

  void avlseg::AVLSegment::splitAt(const double x, const double y,
               avlseg::AVLSegment& left,
               avlseg::AVLSegment& right)const{

  /*
    // debug::start
    if(!ininterior(x,y)){
         cout << "ininterior check failed (may be an effect"
              << " of rounding errors !!!" << endl;
         cout << "The segment is " << *this << endl;
         cout << "The point is (" <<  x << " , " << y << ")" << endl;
     }
     // debug::end
   */

     left.x1=x1;
     left.y1=y1;
     left.x2 = x;
     left.y2 = y;
     left.owner = owner;
     left.insideAbove_first = insideAbove_first;
     left.insideAbove_second = insideAbove_second;
     left.con_below = con_below;
     left.con_above = con_above;

     right.x1=x;
     right.y1=y;
     right.x2 = x2;
     right.y2 = y2;
     right.owner = owner;
     right.insideAbove_first = insideAbove_first;
     right.insideAbove_second = insideAbove_second;
     right.con_below = con_below;
     right.con_above = con_above;

  }

/*
~splitCross~

Splits two crossing segments into the 4 corresponding parts.
Both segments have to cross each other.

*/
void avlseg::AVLSegment::splitCross(const avlseg::AVLSegment& s,
                                          avlseg::AVLSegment& left1,
                                          avlseg::AVLSegment& right1,
                                          avlseg::AVLSegment& left2,
                                          avlseg::AVLSegment& right2) const{

    double x,y;
    bool cross = crosses(s,x,y);
    assert(cross);
    splitAt(x, y, left1, right1);
    s.splitAt(x, y, left2, right2);
}

/*
3.9 Converting Functions

~ConvertToHs~

This functions creates a ~HalfSegment~ from this segment.
The owner must be __first__ or __second__.

*/
HalfSegment avlseg::AVLSegment::convertToHs(bool lpd,
                            avlseg::ownertype owner/* = both*/)const{
   assert( owner!=both || this->owner==first || this->owner==second);
   assert( owner==both || owner==first || owner==second);

   bool insideAbove;
   if(owner==both){
      insideAbove = this->owner==first?insideAbove_first
                                  :insideAbove_second;
   } else {
      insideAbove = owner==first?insideAbove_first
                                  :insideAbove_second;
   }
   HalfSegment hs(lpd, Point(true,x1,y1), Point(true,x2,y2));
   hs.attr.insideAbove = insideAbove;
   return hs;
}

/*
~pointequal~

This function checks if the points defined by (x1, y1) and
(x2,y2) are equals using the ~AlmostEqual~ function.

*/
  bool avlseg::AVLSegment::pointEqual(const double x1, const double y1,
                         const double x2, const double y2){
    return AlmostEqual(x1,x2) && AlmostEqual(y1,y2);
  }

/*
~pointSmaller~

This function checks if the point defined by (x1, y1) is
smaller than the point defined by (x2, y2).

*/

 bool avlseg::AVLSegment::pointSmaller(const double x1, const double y1,
                          const double x2, const double y2){

    return comparePoints(x1,y1,x2,y2) < 0;
 }


/*
~comparePoints~

*/
  int avlseg::AVLSegment::comparePoints(const double x1,const  double y1,
                            const double x2,const double y2){
     if(AlmostEqual(x1,x2)){
       if(AlmostEqual(y1,y2)){
          return 0;
       } else if(y1<y2){
          return -1;
       } else {
          return 1;
       }
     } else if(x1<x2){
       return -1;
     } else {
       return 1;
     }
  }

/*
~compareSlopes~

compares the slopes of __this__ and __s__. The slope of a vertical
segment is greater than all other slopes.

*/
   int avlseg::AVLSegment::compareSlopes(const avlseg::AVLSegment& s) const{
      assert(!isPoint() && !s.isPoint());
      bool v1 = AlmostEqual(x1,x2);
      bool v2 = AlmostEqual(s.x1,s.x2);
      if(v1 && v2){ // both segments are vertical
        return 0;
      }
      if(v1){
        return 1;  // this is vertical, s not
      }
      if(v2){
        return -1; // s is vertical
      }

      // both segments are non-vertical
      double res1 = (y2-y1)/(x2-x1);
      double res2 = (s.y2-s.y1)/(s.x2-s.x1);
      int result = -3;
      if( AlmostEqual(res1,res2)){
         result = 0;
      } else if(res1<res2){
         result =  -1;
      } else { // res1>res2
         result = 1;
      }
      return result;
   }

/*
~XOverlaps~

Checks whether the x interval of this segment overlaps the
x interval of ~s~.

*/

  bool avlseg::AVLSegment::xOverlaps(const avlseg::AVLSegment& s) const{
    if(!AlmostEqual(x1,s.x2) && x1 > s.x2){ // left of s
        return false;
    }
    if(!AlmostEqual(x2,s.x1) && x2 < s.x1){ // right of s
        return false;
    }
    return true;
  }

/*
~XContains~

Checks if the x coordinate provided by the parameter __x__ is contained
in the x interval of this segment;

*/
  bool avlseg::AVLSegment::xContains(const double x) const{
    if(!AlmostEqual(x1,x) && x1>x){
      return false;
    }
    if(!AlmostEqual(x2,x) && x2<x){
      return false;
    }
    return true;
  }

/*
~GetY~

Computes the y value for the specified  __x__.
__x__ must be contained in the x-interval of this segment.
If the segment is vertical, the minimum y value of this
segment is returned.

*/
  double avlseg::AVLSegment::getY(const double x) const{

     if(!xContains(x)){
       cerr << "Warning: compute y value for a x outside the x interval!"
            << endl;
       double diff1 = x1 - x;
       double diff2 = x - x2;
       double diff = (diff1>diff2?diff1:diff2);
       cerr << "difference to x is " << diff << endl;
       cerr << "The segment is " << *this << endl;
       //assert(diff < 1.0);
     }
     if(isVertical()){
        return y1;
     }
     double d = (x-x1)/(x2-x1);
     return y1 + d*(y2-y1);
  }


/*
3.12 Shift Operator

*/
ostream& avlseg::operator<<(ostream& o, const avlseg::AVLSegment& s){
    s.Print(o);
    return o;
}



/*
~insertEvents~

Creates events for the ~AVLSegment~ and insert them into ~q1~ and/ or ~q1~.
The target queue(s) is (are) determined by the owner of ~seg~.
The flags ~createLeft~ and ~createRight~ determine
whether the left and / or the right events should be created.

*/

void insertEvents(const avlseg::AVLSegment& seg,
                  const bool createLeft,
                  const bool createRight,
                  priority_queue<HalfSegment,
                                 vector<HalfSegment>,
                                 greater<HalfSegment> >& q1,
                  priority_queue<HalfSegment,
                                 vector<HalfSegment>,
                                 greater<HalfSegment> >& q2){
   if(seg.isPoint()){
     return;
   }
   switch(seg.getOwner()){
      case avlseg::first: {
           if(createLeft){
              q1.push(seg.convertToHs(true, avlseg::first));
           }
           if(createRight){
              q1.push(seg.convertToHs(false, avlseg::first));
           }
           break;
      } case avlseg::second:{
           if(createLeft){
              q2.push(seg.convertToHs(true, avlseg::second));
           }
           if(createRight){
              q2.push(seg.convertToHs(false, avlseg::second));
           }
           break;
      } case avlseg::both : {
           if(createLeft){
              q1.push(seg.convertToHs(true, avlseg::first));
              q2.push(seg.convertToHs(true, avlseg::second));
           }
           if(createRight){
              q1.push(seg.convertToHs(false, avlseg::first));
              q2.push(seg.convertToHs(false, avlseg::second));
           }
           break;
      } default: {
           assert(false);
      }
   }
}


/*
~splitByNeighbour~


~neighbour~ has to be an neighbour from ~current~ within ~sss~.

The return value is true, if current was changed.


*/

bool splitByNeighbour(avltree::AVLTree<avlseg::AVLSegment>& sss,
                      avlseg::AVLSegment& current,
                      avlseg::AVLSegment const*& neighbour,
                      priority_queue<HalfSegment,
                                     vector<HalfSegment>,
                                     greater<HalfSegment> >& q1,
                      priority_queue<HalfSegment,
                                     vector<HalfSegment>,
                                     greater<HalfSegment> >& q2){
    avlseg::AVLSegment left1, right1, left2, right2;

    if(neighbour && !neighbour->innerDisjoint(current)){
       if(neighbour->ininterior(current.getX1(),current.getY1())){
          neighbour->splitAt(current.getX1(),current.getY1(),left1,right1);
          sss.remove(*neighbour);
          if(!left1.isPoint()){
            neighbour = sss.insert2(left1);
            insertEvents(left1,false,true,q1,q2);
          }
          insertEvents(right1,true,true,q1,q2);
          return false;
       } else if(neighbour->ininterior(current.getX2(),current.getY2())){
          neighbour->splitAt(current.getX2(),current.getY2(),left1,right1);
          sss.remove(*neighbour);
          if(!left1.isPoint()){
            neighbour = sss.insert2(left1);
            insertEvents(left1,false,true,q1,q2);
          }
          insertEvents(right1,true,true,q1,q2);
          return false;
       } else if(current.ininterior(neighbour->getX2(),neighbour->getY2())){
          current.splitAt(neighbour->getX2(),neighbour->getY2(),left1,right1);
          current = left1;
          insertEvents(left1,false,true,q1,q2);
          insertEvents(right1,true,true,q1,q2);
          return true;
       } else if(current.crosses(*neighbour)){
          neighbour->splitCross(current,left1,right1,left2,right2);
          sss.remove(*neighbour);
          if(!left1.isPoint()){
            neighbour = sss.insert2(left1);
          }
          current = left2;
          insertEvents(left1,false,true,q1,q2);
          insertEvents(right1,true,true,q1,q2);
          insertEvents(left2,false,true,q1,q2);
          insertEvents(right2,true,true,q1,q2);
          return true;
       } else {  // forgotten case or wrong order of halfsegments
          cerr.precision(16);
          cerr << "Warning wrong order in halfsegment array detected" << endl;

          cerr << "current" << current << endl
               << "neighbour " << (*neighbour) << endl;
          if(current.overlaps(*neighbour)){ // a common line
              cerr << "1 : The segments overlaps" << endl;
           }
           if(neighbour->ininterior(current.getX1(),current.getY1())){
              cerr << "2 : neighbour->ininterior(current.x1,current.y1)"
                   << endl;
           }
           if(neighbour->ininterior(current.getX2(),current.getY2())){
              cerr << "3 : neighbour->ininterior(current.getX2()"
                   << ",current.getY2()" << endl;
           }
          if(current.ininterior(neighbour->getX1(),neighbour->getY1())){
             cerr << " case 4 : current.ininterior(neighbour->getX1(),"
                  << "neighbour.getY1()" << endl;
             cerr << "may be an effect of rounding errors" << endl;

             cerr << "remove left part from current" << endl;
             current.splitAt(neighbour->getX1(),neighbour->getY1(),
                             left1,right1);
             cerr << "removed part is " << left1 << endl;
             current = right1;
             insertEvents(current,false,true,q1,q2);
             return true;

          }
          if(current.ininterior(neighbour->getX2(),neighbour->getY2())){
            cerr << " 5 : current.ininterior(neighbour->getX2(),"
                 << "neighbour->getY2())" << endl;
          }
          if(current.crosses(*neighbour)){
             cerr << "6 : crosses" << endl;
          }
          assert(false);
          return true;
       }
    } else {
      return false;
    }
}


/*
~splitNeighbours~

Checks if the left and the right neighbour are intersecting in their
interiors and performs the required actions.


*/

void splitNeighbours(avltree::AVLTree<avlseg::AVLSegment>& sss,
                     avlseg::AVLSegment const*& leftN,
                     avlseg::AVLSegment const*& rightN,
                     priority_queue<HalfSegment,
                                    vector<HalfSegment>,
                                    greater<HalfSegment> >& q1,
                     priority_queue<HalfSegment,
                                    vector<HalfSegment>,
                                    greater<HalfSegment> >& q2){
  if(leftN && rightN && !leftN->innerDisjoint(*rightN)){
    avlseg::AVLSegment left1, right1, left2, right2;
    if(leftN->ininterior(rightN->getX2(),rightN->getY2())){
       leftN->splitAt(rightN->getX2(),rightN->getY2(),left1,right1);
       sss.remove(*leftN);
       if(!left1.isPoint()){
         leftN = sss.insert2(left1);
         insertEvents(left1,false,true,q1,q2);
       }
       insertEvents(right1,true,true,q1,q2);
    } else if(rightN->ininterior(leftN->getX2(),leftN->getY2())){
       rightN->splitAt(leftN->getX2(),leftN->getY2(),left1,right1);
       sss.remove(*rightN);
       if(!left1.isPoint()){
         rightN = sss.insert2(left1);
         insertEvents(left1,false,true,q1,q2);
       }
       insertEvents(right1,true,true,q1,q2);
    } else if (rightN->crosses(*leftN)){
         leftN->splitCross(*rightN,left1,right1,left2,right2);
         sss.remove(*leftN);
         sss.remove(*rightN);
         if(!left1.isPoint()) {
           leftN = sss.insert2(left1);
         }
         if(!left2.isPoint()){
            rightN = sss.insert2(left2);
         }
         insertEvents(left1,false,true,q1,q2);
         insertEvents(left2,false,true,q1,q2);
         insertEvents(right1,true,true,q1,q2);
         insertEvents(right2,true,true,q1,q2);
    } else { // forgotten case or overlapping segments (rounding errors)
       if(leftN->overlaps(*rightN)){
         cerr << "Overlapping neighbours found" << endl;
         cout << "leftN = " << *leftN << endl;
         cout << "rightN = " << *rightN << endl;
         avlseg::AVLSegment left;
         avlseg::AVLSegment common;
         avlseg::AVLSegment right;
         int parts = leftN->split(*rightN, left,common,right,false);
         sss.remove(*leftN);
         sss.remove(*rightN);
         if(parts & avlseg::LEFT){
           if(!left.isPoint()){
             cout << "insert left part" << left << endl;
             leftN = sss.insert2(left);
             insertEvents(left,false,true,q1,q2);
           }
         }
         if(parts & avlseg::COMMON){
           if(!common.isPoint()){
             cout << "insert common part" << common << endl;
             rightN = sss.insert2(common);
             insertEvents(common,false,true,q1,q2);
           }
         }
         if(parts & avlseg::RIGHT){
           if(!right.isPoint()){
             cout << "insert events for the right part" << right << endl;;
             insertEvents(right,true,true,q1,q2);
           }
         }

       } else {
          assert(false);
       }
    }
  } // intersecting neighbours
}
/*

~selectNext~

Selects the minimum halfsegment from ~v~1, ~v~2, ~q~1, and ~q~2.
If no values are available, the return value will be __none__.
In this case, __result__ remains unchanged. Otherwise, __result__
is set to the minimum value found. In this case, the return value
will be ~first~ or ~second~.
If some halfsegments are equal, the one
from  ~v~1 is selected.
Note: ~pos~1 and ~pos~2 are increased automatically. In the same way,
      the topmost element of the selected queue is deleted.

The template parameter can be instantiated with ~Region~ or ~Line~

*/
template<class T1, class T2>
avlseg::ownertype selectNext(T1 const* const v1,
                     int& pos1,
                     T2 const* const v2,
                     int& pos2,
                     priority_queue<HalfSegment,
                                    vector<HalfSegment>,
                                    greater<HalfSegment> >& q1,
                     priority_queue<HalfSegment,
                                    vector<HalfSegment>,
                                    greater<HalfSegment> >& q2,
                     HalfSegment& result,
                     int& src = 0
                    ){


  const HalfSegment* values[4];
  int number = 0; // number of available values
  // read the available elements
  if(pos1<v1->Size()){
     v1->Get(pos1,values[0]);
     number++;
  }  else {
     values[0] = 0;
  }
  if(q1.empty()){
    values[1] = 0;
  } else {
    values[1] = &q1.top();
    number++;
  }
  if(pos2<v2->Size()){
     v2->Get(pos2,values[2]);
     number++;
  }  else {
     values[2] = 0;
  }
  if(q2.empty()){
    values[3] = 0;
  } else {
    values[3] = &q2.top();
    number++;
  }
  // no halfsegments found

  if(number == 0){
     return avlseg::none;
  }
  // search for the minimum.
  int index = -1;
  for(int i=0;i<4;i++){
    if(values[i]){
       if(index<0 || (result > *values[i])){
          result = *values[i];
          index = i;
       }
    }
  }
  src = index +  1;
  switch(index){
    case 0: pos1++; return avlseg::first;
    case 1: q1.pop();  return avlseg::first;
    case 2: pos2++;  return avlseg::second;
    case 3: q2.pop();  return avlseg::second;
    default: assert(false);
  }
  return avlseg::none;
}

/*
Instantiation of the ~selectNext~ Function.

*/

avlseg::ownertype selectNext(Region const* const reg1,
                     int& pos1,
                     Region const* const reg2,
                     int& pos2,
                     priority_queue<HalfSegment,
                                    vector<HalfSegment>,
                                    greater<HalfSegment> >& q1,
                     priority_queue<HalfSegment,
                                    vector<HalfSegment>,
                                    greater<HalfSegment> >& q2,
                     HalfSegment& result,
                     int& src // for debugging only
                    ){
   return selectNext<Region,Region>(reg1,pos1,reg2,pos2,q1,q2,result,src);
}

/*
7.8 ~line~ [x] ~line~


Instantiation of the ~selectNext~ function.

*/
avlseg::ownertype selectNext(Line const* const line1,
                     int& pos1,
                     Line const* const line2,
                     int& pos2,
                     priority_queue<HalfSegment,
                                    vector<HalfSegment>,
                                    greater<HalfSegment> >& q1,
                     priority_queue<HalfSegment,
                                    vector<HalfSegment>,
                                    greater<HalfSegment> >& q2,
                     HalfSegment& result,
                     int& src
                    ){

   return selectNext<Line,Line>(line1,pos1,line2,pos2,q1,q2,result, src);
}

/*
7.9 ~line~ [x] ~region~


~selectNext~

Instantiation of the ~selectNext~ function for ~line~ [x] ~region~.

*/

avlseg::ownertype selectNext(Line const* const line,
                     int& pos1,
                     Region const* const region,
                     int& pos2,
                     priority_queue<HalfSegment,
                                    vector<HalfSegment>,
                                    greater<HalfSegment> >& q1,
                     priority_queue<HalfSegment,
                                    vector<HalfSegment>,
                                    greater<HalfSegment> >& q2,
                     HalfSegment& result,
                     int& src
                    ){

   return selectNext<Line,Region>(line,pos1,region,pos2,q1,q2,result,src);
}

/*
~SelectNext~ line [x] point

This function looks which event from the line or from the point
is smaller. The line is divided into the original line part
at position ~posLine~ and possible splitted segments stored
in ~q~. The return value of the function will be ~first~ if the
next event comes from the line value and ~second~ if the next
event comes from the point value. Depending of the return value,
one of the arguments ~resHs~ or ~resPoint~ is set the the value of
this event.
The positions are increased automatically by this function.

*/
avlseg::ownertype selectNext( Line const* const line,
                      priority_queue<HalfSegment,
                                     vector<HalfSegment>,
                                     greater<HalfSegment> >& q,
                      int& posLine,
                      Point const* const point,
                      int& posPoint, // >0: point already used
                      HalfSegment& resHs,
                      Point& resPoint){


   int size = line->Size();
   const HalfSegment* hsl = 0;
   const HalfSegment* hsq = 0;
   const HalfSegment* hsmin = 0;
   HalfSegment hstmp;
   int src = 0;
   if(posLine < size){
      line->Get(posLine,hsl);
   }
   if(!q.empty()){
       hstmp = q.top();
       hsq = &hstmp;
   }
   if(hsl){
      src = 1;
      hsmin = hsl;
   }
   if(hsq){
     if(!hsl || (*hsq < *hsl)){
       src = 2;
       hsmin = hsq;
     }
   }

   if(posPoint==0){  // point not already used
     if(!hsmin){
       src = 3;
     } else {
       Point p = hsmin->GetDomPoint();
       if(*point < p){
            src = 3;
        }
     }
   }

   switch(src){
    case 0: return avlseg::none;
    case 1: posLine++;
            resHs = *hsmin;
            return avlseg::first;
    case 2: q.pop();
            resHs = *hsmin;
            return avlseg::first;
    case 3: resPoint = *point;
            posPoint++;
            return avlseg::second;
    default: assert(false);
             return avlseg::none;
   }
}

/*
~selectNext~ line [x] points

This function works like the function above but instead for a point, a
points value is used.

*/


avlseg::ownertype selectNext( Line const* const line,
                      priority_queue<HalfSegment,
                                     vector<HalfSegment>,
                                     greater<HalfSegment> >& q,
                      int& posLine,
                      Points const* const point,
                      int& posPoint,
                      HalfSegment& resHs,
                      Point& resPoint){

   int sizeP = point->Size();
   int sizeL = line->Size();


   const HalfSegment* hsl = 0;
   const HalfSegment* hsq = 0;
   const HalfSegment* hsmin = 0;
   HalfSegment hstmp;
   int src = 0;
   if(posLine < sizeL){
      line->Get(posLine,hsl);
   }
   if(!q.empty()){
       hstmp = q.top();
       hsq = &hstmp;
   }
   if(hsl){
      src = 1;
      hsmin = hsl;
   }
   if(hsq){
     if(!hsl || (*hsq < *hsl)){
       src = 2;
       hsmin = hsq;
     }
   }

   const Point * cp;
   if(posPoint<sizeP){  // point not already used
     point->Get(posPoint,cp);
     if(!hsmin){
       src = 3;
     } else {
       Point p = hsmin->GetDomPoint();
       if(*cp < p){
            src = 3;
        }
     }
   }

   switch(src){
    case 0: return avlseg::none;
    case 1: posLine++;
            resHs = *hsmin;
            return avlseg::first;
    case 2: q.pop();
            resHs = *hsmin;
            return avlseg::first;
    case 3: resPoint = *cp;
            posPoint++;
            return avlseg::second;
    default: assert(false);
             return avlseg::none;
   }

}


/*
9 Set Operations (union, intersection, difference)


The following functions implement the operations ~union~,
~intersection~ and ~difference~ for some combinations of spatial types.


*/
/*
8 ~Realminize~

This function converts  a line given as ~src~ into a realminized version
stored in ~result~.

*/
avlseg::ownertype selectNext(const Line& src, int& pos,
                            priority_queue<HalfSegment,
                                           vector<HalfSegment>,
                                           greater<HalfSegment> >& q,
                            HalfSegment& result){

 int size = src.Size();
 if(size<=pos){
    if(q.empty()){
      return avlseg::none;
    } else {
      result = q.top();
      q.pop();
      return avlseg::first;
    }
 } else {
   const HalfSegment* hs;
   src.Get(pos,hs);
   if(q.empty()){
      result = *hs;
      pos++;
      return avlseg::first;
   } else{
      HalfSegment hsq = q.top();
      if(hsq<*hs){
         result = hsq;
         q.pop();
         return avlseg::first;
      } else {
         pos++;
         result = *hs;
         return avlseg::first;
      }
   }
 }
}


void Realminize2(const Line& src, Line& result){

  result.Clear();
  if(!src.IsDefined()){
     result.SetDefined(false);
     return;
  }
  result.SetDefined(true);
  if(src.Size()==0){ // empty line, nothing to realminize
    return;
  }

  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q;
  avltree::AVLTree<avlseg::AVLSegment> sss;

  int pos = 0;

  HalfSegment nextHS;
  const avlseg::AVLSegment* member=0;
  const avlseg::AVLSegment* leftN  = 0;
  const avlseg::AVLSegment* rightN = 0;

  avlseg::AVLSegment left1, right1,left2,right2;

  result.StartBulkLoad();
  int edgeno = 0;
  avlseg::AVLSegment tmpL,tmpR;


  while(selectNext(src,pos,q,nextHS)!=avlseg::none) {
      avlseg::AVLSegment current(&nextHS,avlseg::first);
      member = sss.getMember(current,leftN,rightN);
      if(leftN){
         tmpL = *leftN;
         leftN = &tmpL;
      }
      if(rightN){
         tmpR = *rightN;
         rightN = &tmpR;
      }
      if(nextHS.IsLeftDomPoint()){
         if(member){ // overlapping segment found in sss
            double xm = member->getX2();
            double xc = current.getX2();
            if(!AlmostEqual(xm,xc) && (xm<xc)){ // current extends member
               current.splitAt(xm,member->getY2(),left1,right1);
               insertEvents(right1,true,true,q,q);
            }
         } else { // no overlapping segment found
            splitByNeighbour(sss,current,leftN,q,q);
            splitByNeighbour(sss,current,rightN,q,q);
            if(!current.isPoint()){
              sss.insert(current);
              insertEvents(current,false,true,q,q);
            }
         }
      } else {  // nextHS rightDomPoint
          if(member && member->exactEqualsTo(current)){
             // insert the halfsegments
             HalfSegment hs1 = current.convertToHs(true);
             HalfSegment hs2 = current.convertToHs(false);
             hs1.attr.edgeno = edgeno;
             hs2.attr.edgeno = edgeno;
             result += hs1;
             result += hs2;
             splitNeighbours(sss,leftN,rightN,q,q);
             edgeno++;
             sss.remove(*member);
          }
      }
  }
  result.EndBulkLoad();
} // Realminize2


avlseg::ownertype selectNext(const DBArray<HalfSegment>& src, int& pos,
                     priority_queue<HalfSegment,
                     vector<HalfSegment>,
                     greater<HalfSegment> >& q,
                     HalfSegment& result){

 int size = src.Size();
 if(size<=pos){
    if(q.empty()){
      return avlseg::none;
    } else {
      result = q.top();
      q.pop();
      return avlseg::first;
    }
 } else {
   const HalfSegment* hs;
   src.Get(pos,hs);
   if(q.empty()){
      result = *hs;
      pos++;
      return avlseg::first;
   } else{
      HalfSegment hsq = q.top();
      if(hsq<*hs){
         result = hsq;
         q.pop();
         return avlseg::first;
      } else {
         pos++;
         result = *hs;
         return avlseg::first;
      }
   }
 }
}





DBArray<HalfSegment>* Realminize(const DBArray<HalfSegment>& segments){

  DBArray<HalfSegment>* res = new DBArray<HalfSegment>(0);

  if(segments.Size()==0){ // no halfsegments, nothing to realminize
    res->TrimToSize();
    return res;
  }

  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q1;
  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q2;
  avltree::AVLTree<avlseg::AVLSegment> sss;

  int pos = 0;

  HalfSegment nextHS;
  const avlseg::AVLSegment* member=0;
  const avlseg::AVLSegment* leftN  = 0;
  const avlseg::AVLSegment* rightN = 0;

  avlseg::AVLSegment left1, right1,left2,right2;

  int edgeno = 0;
  avlseg::AVLSegment tmpL,tmpR,tmpM;
  while(selectNext(segments,pos,q1,nextHS)!=avlseg::none) {
      avlseg::AVLSegment current(&nextHS,avlseg::first);
      member = sss.getMember(current,leftN,rightN);
      if(leftN){
         tmpL = *leftN;
         leftN = &tmpL;
      }
      if(rightN){
         tmpR = *rightN;
         rightN = &tmpR;
      }
      if(member){
         tmpM = *member;
         member = &tmpM;
      }
      if(nextHS.IsLeftDomPoint()){
         if(member){ // overlapping segment found in sss
            double xm = member->getX2();
            double xc = current.getX2();
            if(!AlmostEqual(xm,xc) && (xm<xc)){ // current extends member
               current.splitAt(xm,member->getY2(),left1,right1);
               insertEvents(right1,true,true,q1,q2);
            } else if(AlmostEqual(xm,xc) && current.isVertical()){
               double ym = member->getY2();
               double yc = current.getY2();
               if(!AlmostEqual(ym,yc) && (ym < yc)){
                  current.splitAt(xc,yc,left1,right1);
                  insertEvents(right1,true,true,q1,q2);
               }
            }
         } else { // no overlapping segment found
            if(splitByNeighbour(sss,current,leftN,q1,q2)){
               insertEvents(current,true,true,q1,q2);
            } else if(splitByNeighbour(sss,current,rightN,q1,q2)) {
               insertEvents(current,true,true,q1,q2);
            } else {
               sss.insert(current);
               insertEvents(current,false,true,q1,q2);
            }
         }
      } else {  // nextHS rightDomPoint
          if(member && member->exactEqualsTo(current)){
             // insert the halfsegments
             HalfSegment hs1 = current.convertToHs(true);
             HalfSegment hs2 = current.convertToHs(false);
             hs1.attr.edgeno = edgeno;
             hs2.attr.edgeno = edgeno;
             res->Append(hs1);
             res->Append(hs2);
             splitNeighbours(sss,leftN,rightN,q1,q2);
             edgeno++;
             sss.remove(*member);
          }
      }
  }
  res->Sort(HalfSegmentCompare);
  res->TrimToSize();
  return res;
}

/*
~Split~

This function works similar to the realminize function. The difference is,
that overlapping parts of segments are keept, instead to remove them.
But at all crossing points and so on, the segments will be split.

*/
DBArray<HalfSegment>* Split(const DBArray<HalfSegment>& segments){

  DBArray<HalfSegment>* res = new DBArray<HalfSegment>(0);

  if(segments.Size()==0){ // no halfsegments, nothing to split
    res->TrimToSize();
    return res;
  }

  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q;
  avltree::AVLTree<avlseg::AVLSegment> sss;

  int pos = 0;

  HalfSegment nextHS;
  const avlseg::AVLSegment* member=0;
  const avlseg::AVLSegment* leftN  = 0;
  const avlseg::AVLSegment* rightN = 0;

  avlseg::AVLSegment left1, right1,left2,right2;

  int edgeno = 0;
  avlseg::AVLSegment tmpL,tmpR;

  while(selectNext(segments,pos,q,nextHS)!=avlseg::none) {
      avlseg::AVLSegment current(&nextHS,avlseg::first);
      member = sss.getMember(current,leftN,rightN);
      if(leftN){
         tmpL = *leftN;
         leftN = &tmpL;
      }
      if(rightN){
         tmpR = *rightN;
         rightN = &tmpR;
      }
      if(nextHS.IsLeftDomPoint()){
         if(member){ // overlapping segment found in sss
            // insert the common part into res
            avlseg::AVLSegment tmp_left, tmp_common, tmp_right;
            int sr = member->split(current,tmp_left,tmp_common,tmp_right,false);

            tmp_left.setOwner(avlseg::first);
            tmp_common.setOwner(avlseg::first);
            tmp_right.setOwner(avlseg::first);
            Point pl(true,tmp_common.getX1(),tmp_common.getY1());
            Point pr(true,tmp_common.getX2(),tmp_common.getY2());

            HalfSegment hs1 = tmp_common.convertToHs(true);
            HalfSegment hs2 = tmp_common.convertToHs(false);
            hs1.attr.edgeno = edgeno;
            hs2.attr.edgeno = edgeno;
            res->Append(hs1);
            res->Append(hs2);
            edgeno++;


            sss.remove(*member);
            if(sr & avlseg::LEFT){
              if(!tmp_left.isPoint()){
                sss.insert(tmp_left);
                insertEvents(tmp_left,false,true,q,q);
              }
            }
            insertEvents(tmp_common,true,true,q,q);
            if(sr & avlseg::RIGHT){
              insertEvents(tmp_right,true,true,q,q);
            }
         } else { // no overlapping segment found
            splitByNeighbour(sss,current,leftN,q,q);
            splitByNeighbour(sss,current,rightN,q,q);
            if(!current.isPoint()){
              sss.insert(current);
              insertEvents(current,false,true,q,q);
            }
         }
      } else {  // nextHS rightDomPoint
          if(member && member->exactEqualsTo(current)){
             // insert the halfsegments
             HalfSegment hs1 = current.convertToHs(true);
             HalfSegment hs2 = current.convertToHs(false);
             hs1.attr.edgeno = edgeno;
             hs2.attr.edgeno = edgeno;
             res->Append(hs1);
             res->Append(hs2);
             splitNeighbours(sss,leftN,rightN,q,q);
             edgeno++;
             sss.remove(*member);
          }
      }
  }
  res->Sort(HalfSegmentCompare);
  res->TrimToSize();
  // work around because problems with overlapping segments
  if(hasOverlaps(*res,true)){
    DBArray<HalfSegment>* tmp = Split(*res);
    delete res;
    res = tmp;
  }

  return res;
}

bool hasOverlaps(const DBArray<HalfSegment>& segments,
                 const bool ignoreEqual){
  if(segments.Size()<2){ // no overlaps possible
    return false;
  }

  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q;
  avltree::AVLTree<avlseg::AVLSegment> sss;

  int pos = 0;

  HalfSegment nextHS;
  const avlseg::AVLSegment* member=0;
  const avlseg::AVLSegment* leftN  = 0;
  const avlseg::AVLSegment* rightN = 0;

  avlseg::AVLSegment left1, right1,left2,right2;

  avlseg::AVLSegment tmpL,tmpR;

  while(selectNext(segments,pos,q,nextHS)!=avlseg::none) {
      avlseg::AVLSegment current(&nextHS,avlseg::first);
      member = sss.getMember(current,leftN,rightN);
      if(leftN){
         tmpL = *leftN;
         leftN = &tmpL;
      }
      if(rightN){
         tmpR = *rightN;
         rightN = &tmpR;
      }
      if(nextHS.IsLeftDomPoint()){
         if(member){ // overlapping segment found in sss
            if(!ignoreEqual || !member->exactEqualsTo(current)){
              return true;
            }
            double xm = member->getX2();
            double xc = current.getX2();
            if(!AlmostEqual(xm,xc) && (xm<xc)){ // current extends member
               current.splitAt(xm,member->getY2(),left1,right1);
               insertEvents(right1,true,true,q,q);
            }
         } else { // no overlapping segment found
            splitByNeighbour(sss,current,leftN,q,q);
            splitByNeighbour(sss,current,rightN,q,q);
            if(!current.isPoint()){
              sss.insert(current);
              insertEvents(current,false,true,q,q);
            }
         }
      } else {  // nextHS rightDomPoint
          if(member && member->exactEqualsTo(current)){
             // insert the halfsegments
             splitNeighbours(sss,leftN,rightN,q,q);
             sss.remove(*member);
          }
      }
  }
  return false;
}





/*
9.2 ~line~ [x] ~line~ [->] ~line~

This combination can be used for all possible set operations.


*/

void SetOp(const Line& line1,
           const Line& line2,
           Line& result,
           avlseg::SetOperation op){

   result.Clear();
   if(!line1.IsDefined() || !line2.IsDefined()){
       result.SetDefined(false);
       return;
   }
   result.SetDefined(true);
   if(line1.Size()==0){
       switch(op){
         case avlseg::union_op : result = line2;
                         return;
         case avlseg::intersection_op : return; // empty line
         case avlseg::difference_op : return; // empty line
         default : assert(false);
       }
   }
   if(line2.Size()==0){
      switch(op){
         case avlseg::union_op: result = line1;
                        return;
         case avlseg::intersection_op: return;
         case avlseg::difference_op: result = line1;
                             return;
         default : assert(false);
      }
   }


  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q1;
  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q2;
  avltree::AVLTree<avlseg::AVLSegment> sss;
  avlseg::ownertype owner;
  int pos1 = 0;
  int pos2 = 0;
  HalfSegment nextHs;
  int src = 0;

  const avlseg::AVLSegment* member=0;
  const avlseg::AVLSegment* leftN = 0;
  const avlseg::AVLSegment* rightN = 0;

  avlseg::AVLSegment left1,right1,common1,
             left2,right2;

  int edgeno =0;
  avlseg::AVLSegment tmpL,tmpR;

  result.StartBulkLoad();
  while( (owner=selectNext(&line1,pos1,
                           &line2,pos2,
                           q1,q2,nextHs,src))!=avlseg::none){
       avlseg::AVLSegment current(&nextHs,owner);
       member = sss.getMember(current,leftN,rightN);
       if(leftN){
         tmpL = *leftN;
         leftN = &tmpL;
       }
       if(rightN){
         tmpR = *rightN;
         rightN = &tmpR;
       }
       if(nextHs.IsLeftDomPoint()){
          if(member){ // found an overlapping segment
             if(member->getOwner()==current.getOwner() ||
                member->getOwner()==avlseg::both){ // same source
                 double xm = member->getX2();
                 double xc = current.getX2();
                 if(!AlmostEqual(xm,xc) && (xm<xc)){ // current extends member
                    current.splitAt(xm,member->getY2(),left1,right1);
                    insertEvents(right1,true,true,q1,q2);
                 }
             }  else { // member and current come from different sources
                 int parts = member->split(current,left1,common1,right1);
                 sss.remove(*member);
                 member = &common1;
                 if(parts & avlseg::LEFT){
                     if(!left1.isPoint()){
                       sss.insert(left1);
                       insertEvents(left1,false,true,q1,q2);
                     }
                 }
                 assert(parts & avlseg::COMMON);
                 if(!common1.isPoint()){
                   sss.insert(common1);
                   insertEvents(common1,false,true,q1,q2);
                 }
                 if(parts & avlseg::RIGHT){
                    insertEvents(right1,true,true,q1,q2);
                 }
             }
          } else { // no overlapping segment found
            splitByNeighbour(sss,current,leftN,q1,q2);
            splitByNeighbour(sss,current,rightN,q1,q2);
            if(!current.isPoint()){
              sss.insert(current);
              insertEvents(current,false,true,q1,q2);
            }
          }
       } else { // nextHS rightDomPoint
         if(member && member->exactEqualsTo(current)){
             // insert the segments into the result
             switch(op){
                case avlseg::union_op : {
                     HalfSegment hs1 = member->convertToHs(true,avlseg::first);
                     hs1.attr.edgeno = edgeno;
                     result += hs1;
                     hs1.SetLeftDomPoint(false);
                     result += hs1;
                     edgeno++;
                     break;
                } case avlseg::intersection_op : {
                     if(member->getOwner()==avlseg::both){
                        HalfSegment hs1 =
                           member->convertToHs(true,avlseg::first);
                        hs1.attr.edgeno = edgeno;
                        result += hs1;
                        hs1.SetLeftDomPoint(false);
                        result += hs1;
                        edgeno++;
                      }
                      break;
                } case avlseg::difference_op :{
                      if(member->getOwner()==avlseg::first){
                        HalfSegment hs1 =
                            member->convertToHs(true,avlseg::first);
                        hs1.attr.edgeno = edgeno;
                        result += hs1;
                        hs1.SetLeftDomPoint(false);
                        result += hs1;
                        edgeno++;
                      }
                      break;
                } default : {
                      assert(false);
                }
             }
             sss.remove(*member);
             splitNeighbours(sss,leftN,rightN,q1,q2);
         }
       }
  }
  result.EndBulkLoad(true,false);
} // setop line x line -> line


Line* SetOp(const Line& line1, const Line& line2, avlseg::SetOperation op){
  Line* result = new Line(1);
  SetOp(line1,line2,*result,op);
  return result;
}


/*

9.3 ~region~ [x] ~region~ [->] ~region~

*/

void SetOp(const Region& reg1,
           const Region& reg2,
           Region& result,
           avlseg::SetOperation op){

   result.Clear();
   if(!reg1.IsDefined() || !reg2.IsDefined()){
       result.SetDefined(false);
       return;
   }
   result.SetDefined(true);
   if(reg1.Size()==0){
       switch(op){
         case avlseg::union_op : result = reg2;
                         return;
         case avlseg::intersection_op : return; // empty region
         case avlseg::difference_op : return; // empty region
         default : assert(false);
       }
   }
   if(reg2.Size()==0){
      switch(op){
         case avlseg::union_op: result = reg1;
                        return;
         case avlseg::intersection_op: return;
         case avlseg::difference_op: result = reg1;
                             return;
         default : assert(false);
      }
   }

   if(!reg1.BoundingBox().Intersects(reg2.BoundingBox())){
      switch(op){
        case avlseg::union_op: {
          result.StartBulkLoad();
          int edgeno=0;
          int s = reg1.Size();
          const HalfSegment* hs;
          for(int i=0;i<s;i++){
              reg1.Get(i,hs);
              if(hs->IsLeftDomPoint()){
                 HalfSegment HS(*hs);
                 HS.attr.edgeno = edgeno;
                 result += HS;
                 HS.SetLeftDomPoint(false);
                 result += HS;
                 edgeno++;
              }
          }
          s = reg2.Size();
          for(int i=0;i<s;i++){
              reg2.Get(i,hs);
              if(hs->IsLeftDomPoint()){
                 HalfSegment HS(*hs);
                 HS.attr.edgeno = edgeno;
                 result += HS;
                 HS.SetLeftDomPoint(false);
                 result += HS;
                 edgeno++;
              }
          }
          result.EndBulkLoad();
          return;
        } case avlseg::difference_op: {
           result = reg1;
           return;
        } case avlseg::intersection_op:{
           return;
        } default: assert(false);
      }
   }

  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q1;
  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q2;
  avltree::AVLTree<avlseg::AVLSegment> sss;
  avlseg::ownertype owner;
  int pos1 = 0;
  int pos2 = 0;
  HalfSegment nextHs;
  int src = 0;

  const avlseg::AVLSegment* member=0;
  const avlseg::AVLSegment* leftN = 0;
  const avlseg::AVLSegment* rightN = 0;

  avlseg::AVLSegment left1,right1,common1,
             left2,right2;

  int edgeno =0;
  avlseg::AVLSegment tmpL,tmpR;

  result.StartBulkLoad();
  while( (owner=selectNext(&reg1,pos1,
                           &reg2,pos2,
                           q1,q2,nextHs,src))!=avlseg::none){
       avlseg::AVLSegment current(&nextHs,owner);
       member = sss.getMember(current,leftN,rightN);
       if(leftN){
          tmpL = *leftN;
          leftN = &tmpL;
       }
       if(rightN){
          tmpR = *rightN;
          rightN = &tmpR;
       }
       if(nextHs.IsLeftDomPoint()){
          if(member){ // overlapping segment found
            if((member->getOwner()==avlseg::both) ||
               (member->getOwner()==owner)){
               cerr << "overlapping segments detected within a single region"
                    << endl;
               cerr << "the argument is "
                    << (owner==avlseg::first?"first":"second")
                    << endl;
               cerr.precision(16);
               cerr << "stored is " << *member << endl;
               cerr << "current = " << current << endl;
               avlseg::AVLSegment tmp_left, tmp_common, tmp_right;
               member->split(current,tmp_left, tmp_common, tmp_right, false);
               cout << "The common part is " << tmp_common << endl;
               cout << "The lenth = " << tmp_common.length() << endl;
               assert(false);
            }
            int parts = member->split(current,left1,common1,right1);
            sss.remove(*member);
            if(parts & avlseg::LEFT){
              if(!left1.isPoint()){
                sss.insert(left1);
                insertEvents(left1,false,true,q1,q2);
              }
            }
            assert(parts & avlseg::COMMON);
            // update coverage numbers
            if(current.getInsideAbove()){
               common1.con_above++;
            }  else {
               common1.con_above--;
            }
            if(!common1.isPoint()){
              sss.insert(common1);
              insertEvents(common1,false,true,q1,q2);
            }
            if(parts & avlseg::RIGHT){
               insertEvents(right1,true,true,q1,q2);
            }
          } else { // there is no overlapping segment
            // try to split segments if required
            splitByNeighbour(sss,current,leftN,q1,q2);
            splitByNeighbour(sss,current,rightN,q1,q2);


            // update coverage numbers
            bool iac = current.getOwner()==avlseg::first
                            ?current.getInsideAbove_first()
                            :current.getInsideAbove_second();

            iac = current.getOwner()==avlseg::first
                                           ?current.getInsideAbove_first()
                                           :current.getInsideAbove_second();

            if(leftN && current.extends(*leftN)){
              current.con_below = leftN->con_below;
              current.con_above = leftN->con_above;
            }else{
              if(leftN && leftN->isVertical()){
                 current.con_below = leftN->con_below;
              } else if(leftN){
                 current.con_below = leftN->con_above;
              } else {
                 current.con_below = 0;
              }
              if(iac){
                 current.con_above = current.con_below+1;
              } else {
                 current.con_above = current.con_below-1;
              }
            }
            // insert element
            if(!current.isPoint()){
              sss.insert(current);
              insertEvents(current,false,true,q1,q2);
            }
          }
       } else {  // nextHs.IsRightDomPoint
          if(member && member->exactEqualsTo(current)){
              switch(op){
                case avlseg::union_op :{
                   if( (member->con_above==0) || (member->con_below==0)) {
                      HalfSegment hs1 = member->getOwner()==avlseg::both
                                      ?member->convertToHs(true,avlseg::first)
                                      :member->convertToHs(true);
                      hs1.attr.edgeno = edgeno;
                      result += hs1;
                      hs1.SetLeftDomPoint(false);
                      result += hs1;
                      edgeno++;
                   }
                   break;
                }
                case avlseg::intersection_op: {
                  if(member->con_above==2 || member->con_below==2){
                      HalfSegment hs1 = member->getOwner()==avlseg::both
                                      ?member->convertToHs(true,avlseg::first)
                                      :member->convertToHs(true);
                      hs1.attr.edgeno = edgeno;
                      hs1.attr.insideAbove = (member->con_above==2);
                      result += hs1;
                      hs1.SetLeftDomPoint(false);
                      result += hs1;
                      edgeno++;
                  }
                  break;
                }
                case avlseg::difference_op : {
                  switch(member->getOwner()){
                    case avlseg::first:{
                      if(member->con_above + member->con_below == 1){
                         HalfSegment hs1 = member->getOwner()==avlseg::both
                                      ?member->convertToHs(true,avlseg::first)
                                      :member->convertToHs(true);
                         hs1.attr.edgeno = edgeno;
                         result += hs1;
                         hs1.SetLeftDomPoint(false);
                         result += hs1;
                         edgeno++;
                      }
                      break;
                    }
                    case avlseg::second:{
                      if(member->con_above + member->con_below == 3){
                         HalfSegment hs1 = member->getOwner()==avlseg::both
                                      ?member->convertToHs(true,avlseg::second)
                                      :member->convertToHs(true);
                         hs1.attr.insideAbove = ! hs1.attr.insideAbove;
                         hs1.attr.edgeno = edgeno;
                         result += hs1;
                         hs1.SetLeftDomPoint(false);
                         result += hs1;
                         edgeno++;
                      }
                      break;
                    }
                    case avlseg::both: {
                      if((member->con_above==1) && (member->con_below== 1)){
                         HalfSegment hs1 = member->getOwner()==avlseg::both
                                      ?member->convertToHs(true,avlseg::first)
                                      :member->convertToHs(true);
                         hs1.attr.insideAbove = member->getInsideAbove_first();
                         hs1.attr.edgeno = edgeno;
                         result += hs1;
                         hs1.SetLeftDomPoint(false);
                         result += hs1;
                         edgeno++;
                      }
                      break;
                    }
                    default : assert(false);
                  } // switch member->getOwner
                  break;
                } // case difference
                default : assert(false);
              } // end of switch
              sss.remove(*member);
              splitNeighbours(sss,leftN,rightN,q1,q2);
          } // current found in sss
       } // right endpoint
  }
  result.EndBulkLoad();
} // setOP region x region -> region

Region* SetOp(const Region& reg1, const Region& reg2, avlseg::SetOperation op){
  Region* result = new Region(1);
  SetOp(reg1,reg2,*result,op);
  return result;
}



/*
9.4 ~region~ [x] ~line~ [->] ~region~

This combination can only be used for the operations
~union~ and ~difference~. In both cases, the result will be
the original region value.

*/

void SetOp(const Region& region,
           const Line& line,
           Region& result,
           avlseg::SetOperation op){

   assert(op == avlseg::union_op || op == avlseg::difference_op);
   result.Clear();
   if(!line.IsDefined() || !region.IsDefined()){
      result.SetDefined(false);
      return;
   }
   result.SetDefined(true);
   result.CopyFrom(&region);
}

/*
9.5  ~line~ [x] ~region~ [->] ~line~

Here, only the ~difference~ and ~intersection~ operation are applicable.


*/
void SetOp(const Line& line,
           const Region& region,
           Line& result,
           avlseg::SetOperation op){

  assert(op==avlseg::intersection_op || op == avlseg::difference_op);

  result.Clear();
  if(!line.IsDefined() || !region.IsDefined()){
       result.SetDefined(false);
       return;
   }
   result.SetDefined(true);
   if(line.Size()==0){ // empty line -> empty result
       switch(op){
         case avlseg::intersection_op : return; // empty region
         case avlseg::difference_op : return; // empty region
         default : assert(false);
       }
   }
   if(region.Size()==0){
      switch(op){
         case avlseg::intersection_op: return;
         case avlseg::difference_op: result = line;
                             return;
         default : assert(false);
      }
   }


  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q1;
  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q2;
  avltree::AVLTree<avlseg::AVLSegment> sss;
  avlseg::ownertype owner;
  int pos1 = 0;
  int pos2 = 0;
  int size1= line.Size();
  HalfSegment nextHs;
  int src = 0;

  const avlseg::AVLSegment* member=0;
  const avlseg::AVLSegment* leftN = 0;
  const avlseg::AVLSegment* rightN = 0;

  avlseg::AVLSegment left1,right1,common1,
             left2,right2;

  int edgeno =0;
  avlseg::AVLSegment tmpL,tmpR;
  bool done = false;

  result.StartBulkLoad();
  // perform a planesweeo
  while( ((owner=selectNext(&line,pos1,
                            &region,pos2,
                            q1,q2,nextHs,src))!=avlseg::none)
         && ! done){
     avlseg::AVLSegment current(&nextHs,owner);
     member = sss.getMember(current,leftN,rightN);
     if(leftN){
        tmpL = *leftN;
        leftN = &tmpL;
     }
     if(rightN){
        tmpR = *rightN;
        rightN = &tmpR;
     }
     if(nextHs.IsLeftDomPoint()){
        if(member){ // there is an overlapping segment in sss
           if(member->getOwner()==owner ||
              member->getOwner()==avlseg::both     ){
              if(current.ininterior(member->getX2(),member->getY2())){
                 current.splitAt(member->getX2(),member->getY2(),left1,right1);
                 insertEvents(right1,true,true,q1,q2);
              }
           } else { // member and source come from difference sources
             int parts = member->split(current,left1,common1,right1);
             sss.remove(*member);
             member = &common1;
             if(parts & avlseg::LEFT){
                if(!left1.isPoint()){
                  sss.insert(left1);
                  insertEvents(left1,false,true,q1,q2);
                }
             }
             assert(parts & avlseg::COMMON);
             if(owner==avlseg::second) {  // the region
               if(current.getInsideAbove()){
                  common1.con_above++;
               } else {
                  common1.con_above--;
               }
             } // for a line is nothing to do
             if(!common1.isPoint()){
               sss.insert(common1);
               insertEvents(common1,false,true,q1,q2);
             }
             if(parts & avlseg::RIGHT){
                 insertEvents(right1,true,true,q1,q2);
             }
           }
        } else { // no overlapping segment in sss found
          splitByNeighbour(sss,current,leftN,q1,q2);
          splitByNeighbour(sss,current,rightN,q1,q2);
          // update coverage numbers
          if(owner==avlseg::second){ // the region
            bool iac = current.getInsideAbove();
            if(leftN && current.extends(*leftN)){
              current.con_below = leftN->con_below;
              current.con_above = leftN->con_above;
            }else{
              if(leftN && leftN->isVertical()){
                 current.con_below = leftN->con_below;
              } else if(leftN){
                 current.con_below = leftN->con_above;
              } else {
                 current.con_below = 0;
              }
              if(iac){
                 current.con_above = current.con_below+1;
              } else {
                 current.con_above = current.con_below-1;
              }
            }
          } else { // the line
            if(leftN){
               if(leftN->isVertical()){
                  current.con_below = leftN->con_below;
               } else {
                  current.con_below = leftN->con_above;
               }
            }
            current.con_above = current.con_below;
          }
          // insert element
          if(!current.isPoint()){
            sss.insert(current);
            insertEvents(current,false,true,q1,q2);
          }
        }
     } else { // nextHs.IsRightDomPoint()
       if(member && member->exactEqualsTo(current)){

          switch(op){
              case avlseg::intersection_op: {
                if( (member->getOwner()==avlseg::both) ||
                    (member->getOwner()==avlseg::first && member->con_above>0)){
                    HalfSegment hs1 = member->convertToHs(true,avlseg::first);
                    hs1.attr.edgeno = edgeno;
                    result += hs1;
                    hs1.SetLeftDomPoint(false);
                    result += hs1;
                    edgeno++;
                }
                break;
              }
              case avlseg::difference_op: {
                if( (member->getOwner()==avlseg::first) &&
                    (member->con_above==0)){
                    HalfSegment hs1 = member->convertToHs(true,avlseg::first);
                    hs1.attr.edgeno = edgeno;
                    result += hs1;
                    hs1.SetLeftDomPoint(false);
                    result += hs1;
                    edgeno++;
                }
                break;
              }
              default : assert(false);
          }
          sss.remove(*member);
          splitNeighbours(sss,leftN,rightN,q1,q2);
       }
       if(pos1>=size1 && q1.empty()){ // line is processed
          done = true;
       }
     }
  }
  result.EndBulkLoad();
} // setOP(line x region -> line)


/*
9  ~CommonBorder~

Signature: ~region~ [x] ~region~ [->] ~line~

*/

void CommonBorder(
           const Region& reg1,
           const Region& reg2,
           Line& result){

   result.Clear();
   if(!reg1.IsDefined() || !reg2.IsDefined()){
       result.SetDefined(false);
       return;
   }
   result.SetDefined(true);
   if(reg1.Size()==0 || reg2.Size()==0){
       // a region is empty -> the common border is also empty
       return;
   }
   if(!reg1.BoundingBox().Intersects(reg2.BoundingBox())){
      // no common border possible
      return;
   }

  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q1;
  priority_queue<HalfSegment,  vector<HalfSegment>, greater<HalfSegment> > q2;
  avltree::AVLTree<avlseg::AVLSegment> sss;
  avlseg::ownertype owner;
  int pos1 = 0;
  int pos2 = 0;
  HalfSegment nextHs;
  int src = 0;

  const avlseg::AVLSegment* member=0;
  const avlseg::AVLSegment* leftN = 0;
  const avlseg::AVLSegment* rightN = 0;

  avlseg::AVLSegment left1,right1,common1,
             left2,right2;

  int edgeno =0;
  avlseg::AVLSegment tmpL,tmpR;

  result.StartBulkLoad();
  bool done = false;
  int size1 = reg1.Size();
  int size2 = reg2.Size();

  while( ((owner=selectNext(&reg1,pos1,
                            &reg2,pos2,
                            q1,q2,nextHs,src))!=avlseg::none)
         && !done  ){
       avlseg::AVLSegment current(&nextHs,owner);
       member = sss.getMember(current,leftN,rightN);
       if(leftN){
          tmpL = *leftN;
          leftN = &tmpL;
       }
       if(rightN){
          tmpR = *rightN;
          rightN = &tmpR;
       }
       if(nextHs.IsLeftDomPoint()){
          if(member){ // overlapping segment found
            assert(member->getOwner()!=avlseg::both);
            assert(member->getOwner()!=owner);
            int parts = member->split(current,left1,common1,right1);
            sss.remove(*member);
            if(parts & avlseg::LEFT){
              if(!left1.isPoint()){
                sss.insert(left1);
                insertEvents(left1,false,true,q1,q2);
              }
            }
            assert(parts & avlseg::COMMON);
            if(!common1.isPoint()){
              sss.insert(common1);
              insertEvents(common1,false,true,q1,q2);
            }
            if(parts & avlseg::RIGHT){
               insertEvents(right1,true,true,q1,q2);
            }
          } else { // there is no overlapping segment
            // try to split segments if required
            splitByNeighbour(sss,current,leftN,q1,q2);
            splitByNeighbour(sss,current,rightN,q1,q2);
            if(!current.isPoint()){
              sss.insert(current);
              insertEvents(current,false,true,q1,q2);
            }
          }
       } else {  // nextHs.IsRightDomPoint
          if(member && member->exactEqualsTo(current)){
              if(member->getOwner()==avlseg::both){
                 HalfSegment hs = member->convertToHs(true,avlseg::first);
                 hs.attr.edgeno = edgeno;
                 result += hs;
                 hs.SetLeftDomPoint(false);
                 result += hs;
                 edgeno++;
              }
              sss.remove(*member);
              splitNeighbours(sss,leftN,rightN,q1,q2);
          } // current found in sss
          if(((pos1 >= size1) && q1.empty())  ||
             ((pos2 >= size2) && q2.empty())){
             done = true;
          }
       } // right endpoint
  }
  result.EndBulkLoad();
} // commonborder

/*
~IsSpatialType~

This function checks whether the type given as a ListExpr is one of
~point~, ~points~, ~line~, or ~region~.

*/

bool IsSpatialType(ListExpr type){
   if(!nl->IsAtom(type)){
      return false;
   }
   if(nl->AtomType(type)!=SymbolType){
      return false;
   }
   string t = nl->SymbolValue(type);
   if(t=="point") return true;
   if(t=="points") return true;
   if(t=="line") return true;
   if(t=="region") return true;
   return false;
}



/*
~getDir~

This fucntion will return true iff the cycle given as a
vector of points is in clockwise order.


*/

bool getDir(const vector<Point>& vp){
  // determine the direction of cycle
  int min = 0;
  for(unsigned int i=1;i<vp.size();i++){
    if(vp[i] < vp[min]){
       min = i;
    }
  }

  bool cw;
  int s = vp.size();
  if(AlmostEqual(vp[0],vp[vp.size()-1])){
    s--;
  }

  Point a = vp[ (min - 1 + s ) % s ];
  Point p = vp[min];
  Point b = vp[ (min+1) % s];
  if(AlmostEqual(a.GetX(),p.GetX())){ // a -> p vertical
    if(a.GetY()>p.GetY()){
       cw = false;
    } else {
       cw = true;
    }
  } else if(AlmostEqual(p.GetX(), b.GetX())){ //p -> b vertical
    if(p.GetY()>b.GetY()){
       cw = false;
    } else {
       cw = true;
    }
  } else { // both segments are non-vertical
    double m_p_a = (a.GetY() - p.GetY()) / (a.GetX() - p.GetX());
    double m_p_b = (b.GetY() - p.GetY()) / (b.GetX() - p.GetX());
    if(m_p_a > m_p_b){
        cw = false;
    } else {
        cw = true;
    }
  }
  return cw;
}


/*
This function check whether a region value is valid after thr insertion of a new half segment.
Whenever a half segment is about to be inserted, the state of the region is checked.
A valid region must satisfy the following conditions:

1)  any two cycles of the same region must be disconnect, which means that no edges
of different cycles can intersect each other;

2) edges of the same cycle can only intersect with their endpoints, but no their middle points;

3)  For a certain face, the holes must be inside the outer cycle;

4)  For a certain face, any two holes can not contain each other;

5)  Faces must have the outer cycle, but they can have no holes;

6)  for a certain cycle, any two vertex can not be the same;

7)  any cycle must be made up of at least 3 edges;

8)  It is allowed that one face is inside another provided that their edges do not intersect.

*/

static vector<Point> getCycle(const bool isHole,
                              const vector<HalfSegment>& vhs){

  // first extract the cycle
  bool used[vhs.size()];
  for(unsigned int i=0;i<vhs.size();i++){
    used[i] = false;
  }
  if(vhs.size() < 3 ){
    assert(false);
  }
  // trivial n^2 implementation
  HalfSegment hs = vhs[0];
  Point sp = hs.GetDomPoint();
  Point cp = hs.GetSecPoint();
  vector<Point> vp;

  vp.push_back(sp);
  vp.push_back(cp);

  used[0] = true;
  for(unsigned int i=1; i< vhs.size();i++){
    for(unsigned int j=1; j<vhs.size(); j++){
       if(!used[j]){
          Point p0 = vhs[j].GetDomPoint();
          Point p1 = vhs[j].GetSecPoint();
          if(AlmostEqual(p0,cp)){
             used[j] = true;
             cp = p1;
             vp.push_back(cp);
          } else if(AlmostEqual(p1,cp)){
             used[j] = true;
             cp = p0;
             vp.push_back(cp);
          }
       }
    }
  }
  vp.push_back(cp);
  // debugging only
  for(unsigned int i=0;i<vhs.size();i++){
     if(!used[i]){
        cerr << "Unused halfsegment found" << endl;
     }
  }


  bool cw = getDir(vp);

  if(!(( isHole && cw ) || (!isHole && !cw))){
    vector<Point> vp2;
    for(int i= vp.size()-1; i>=0; i--){
       vp2.push_back(vp[i]);
    }
    return vp2;
  } else {
    return vp;
  }
}


static vector< vector <Point> > getCycles(const Region& reg){
      // first step , map halsfsegment according to faceno and cycleno

      map< pair<int, int> , vector<HalfSegment> > m;

      const HalfSegment* hs;
      for(int i=0;i<reg.Size(); i++){
         reg.Get(i,hs);
         if(hs->IsLeftDomPoint()){
            int faceno = hs->attr.faceno;
            int cycleno = hs->attr.cycleno;
            m[make_pair(faceno, cycleno)].push_back(*hs);
         }
      }

      vector< vector <Point> > result;
      map< pair<int, int> , vector<HalfSegment> >::iterator it;
      for(it=m.begin(); it!=m.end(); it++){
        pair< pair<int, int> , vector<HalfSegment> > cycleDesc = *it;
        bool isHole = cycleDesc.first.second > 0;
        result.push_back(getCycle(isHole, cycleDesc.second));
      }
      return result;

   }


   void Region::saveShape(ostream& o, uint32_t RecNo) const{
     // first, write the record header
     WinUnix::writeBigEndian(o,RecNo);
     // an empty region
     if(!IsDefined() || IsEmpty()){
        uint32_t length = 2;
        WinUnix::writeBigEndian(o,length);
        uint32_t type = 0;
        WinUnix::writeLittleEndian(o,type);
     } else {

        vector<vector < Point> > cycles = getCycles(*this);

        uint32_t numParts = cycles.size();


        uint32_t  numPoints = 0;

        vector<vector < Point> >::iterator it;
        for(it = cycles.begin(); it!=cycles.end(); it++){
           numPoints += it->size();
        }

        uint32_t  numBytes = 44 + 4 * numParts + 16*numPoints;

        uint32_t length = numBytes / 2;


        WinUnix::writeBigEndian(o,length);
        WinUnix::writeLittleEndian(o,getshpType()); // 4
        double minX = getMinX();
        double maxX = getMaxX();
        double minY = getMinY();
        double maxY = getMaxY();
        // write the boundig box
        WinUnix::writeLittle64(o,minX);        // 8 * 4
        WinUnix::writeLittle64(o,minY);
        WinUnix::writeLittle64(o,maxX);
        WinUnix::writeLittle64(o,maxY);

        WinUnix::writeLittleEndian(o,numParts);
        WinUnix::writeLittleEndian(o,numPoints);

        // write the parts
        uint32_t pos = 0;
        for(unsigned int i=0; i<numParts; i++){
           WinUnix::writeLittleEndian(o,pos);
           pos += cycles[i].size();
        }
        // write the points
        for(unsigned int i=0;i<numParts; i++){
           vector<Point> cycle = cycles[i];
           for(unsigned int j=0;j<cycle.size(); j++){
              Point p = cycle[j];
              double x = p.GetX();
              double y = p.GetY();
              WinUnix::writeLittle64(o,x);
              WinUnix::writeLittle64(o,y);
           }
        }



     }
  }




/*

8.2 List Representation

The list representation of a region is

----  (face1  face2  face3 ... )
                 where facei=(outercycle, holecycle1, holecycle2....)

  cyclei= (vertex1, vertex2,  .....)
                where each vertex is a point.
----






8.3 ~Out~-function

*/
ListExpr
OutRegion( ListExpr typeInfo, Word value )
{
  Region* cr = (Region*)(value.addr);
  if( cr->IsEmpty() )
  {
    return (nl->TheEmptyList());
  }
  else
  {
    Region *RCopy=new Region(*cr, true); // in memory

    RCopy->LogicSort();

    const HalfSegment *hs, *hsnext;

    ListExpr regionNL = nl->TheEmptyList();
    ListExpr regionNLLast = regionNL;

    ListExpr faceNL = nl->TheEmptyList();
    ListExpr faceNLLast = faceNL;

    ListExpr cycleNL = nl->TheEmptyList();
    ListExpr cycleNLLast = cycleNL;

    ListExpr pointNL;

    int currFace, currCycle;
    Point outputP, leftoverP;

    for( int i = 0; i < RCopy->Size(); i++ )
    {
      RCopy->Get( i, hs );
      if (i==0)
      {
        currFace = hs->attr.faceno;
        currCycle = hs->attr.cycleno;
        RCopy->Get( i+1, hsnext );

        if ((hs->GetLeftPoint() == hsnext->GetLeftPoint()) ||
            ((hs->GetLeftPoint() == hsnext->GetRightPoint())))
        {
          outputP = hs->GetRightPoint();
          leftoverP = hs->GetLeftPoint();
        }
        else if ((hs->GetRightPoint() == hsnext->GetLeftPoint()) ||
                 ((hs->GetRightPoint() == hsnext->GetRightPoint())))
        {
          outputP = hs->GetLeftPoint();
          leftoverP = hs->GetRightPoint();
        }
        else
        {
          cout<<"wrong data format!"<<endl;
          return nl->TheEmptyList();
        }

        pointNL = OutPoint( nl->TheEmptyList(), SetWord(&outputP) );
        if (cycleNL == nl->TheEmptyList())
        {
          cycleNL = nl->OneElemList(pointNL);
          cycleNLLast = cycleNL;
        }
        else
        {
          cycleNLLast = nl->Append( cycleNLLast, pointNL );
        }
      }
      else
      {
        if (hs->attr.faceno == currFace)
        {
          if (hs->attr.cycleno == currCycle)
          {
            outputP=leftoverP;

            if (hs->GetLeftPoint() == leftoverP)
              leftoverP = hs->GetRightPoint();
            else if (hs->GetRightPoint() == leftoverP)
            {
              leftoverP = hs->GetLeftPoint();
            }
            else
            {
              cout<<"wrong data format!"<<endl;
              return nl->TheEmptyList();
            }

            pointNL=OutPoint( nl->TheEmptyList(),
                              SetWord( &outputP) );
            if (cycleNL == nl->TheEmptyList())
            {
              cycleNL=nl->OneElemList(pointNL);
              cycleNLLast = cycleNL;
            }
            else
            {
              cycleNLLast = nl->Append(cycleNLLast, pointNL);
            }
          }
          else
          {
            if (faceNL == nl->TheEmptyList())
            {
              faceNL = nl->OneElemList(cycleNL);
              faceNLLast = faceNL;
            }
            else
            {
              faceNLLast = nl->Append(faceNLLast, cycleNL);
            }
            cycleNL = nl->TheEmptyList();
            currCycle = hs->attr.cycleno;


            RCopy->Get( i+1, hsnext );
            if ((hs->GetLeftPoint() == hsnext->GetLeftPoint()) ||
                ((hs->GetLeftPoint() == hsnext->GetRightPoint())))
            {
              outputP = hs->GetRightPoint();
              leftoverP = hs->GetLeftPoint();
            }
            else if ((hs->GetRightPoint() == hsnext->GetLeftPoint()) ||
                     ((hs->GetRightPoint() == hsnext->GetRightPoint())))
            {
              outputP = hs->GetLeftPoint();
              leftoverP = hs->GetRightPoint();
            }
            else
            {
              cout<<"wrong data format!"<<endl;
              return nl->TheEmptyList();
            }

            pointNL = OutPoint( nl->TheEmptyList(),
                                SetWord(&outputP) );
            if (cycleNL == nl->TheEmptyList())
            {
              cycleNL = nl->OneElemList(pointNL);
              cycleNLLast = cycleNL;
            }
            else
            {
              cycleNLLast = nl->Append(cycleNLLast, pointNL);
            }
          }
        }
        else
        {
          if (faceNL == nl->TheEmptyList())
          {
            faceNL = nl->OneElemList(cycleNL);
            faceNLLast = faceNL;
          }
          else
          {
            faceNLLast = nl->Append(faceNLLast, cycleNL);
          }
          cycleNL = nl->TheEmptyList();


          if (regionNL == nl->TheEmptyList())
          {
            regionNL = nl->OneElemList(faceNL);
            regionNLLast = regionNL;
          }
          else
          {
            regionNLLast = nl->Append(regionNLLast, faceNL);
          }
          faceNL = nl->TheEmptyList();

          currFace = hs->attr.faceno;
          currCycle = hs->attr.cycleno;


          RCopy->Get( i+1, hsnext );
          if ((hs->GetLeftPoint() == hsnext->GetLeftPoint()) ||
             ((hs->GetLeftPoint() == hsnext->GetRightPoint())))
          {
            outputP = hs->GetRightPoint();
            leftoverP = hs->GetLeftPoint();
          }
          else if ((hs->GetRightPoint() == hsnext->GetLeftPoint()) ||
                  ((hs->GetRightPoint() == hsnext->GetRightPoint())))
          {
            outputP = hs->GetLeftPoint();
            leftoverP = hs->GetRightPoint();
          }
          else
          {
            cout<<"wrong data format!"<<endl;
            return nl->TheEmptyList();
          }

          pointNL = OutPoint(nl->TheEmptyList(), SetWord(&outputP));
          if (cycleNL == nl->TheEmptyList())
          {
            cycleNL = nl->OneElemList(pointNL);
            cycleNLLast = cycleNL;
          }
          else
          {
            cycleNLLast = nl->Append(cycleNLLast, pointNL);
          }
        }
      }
    }

    if (faceNL == nl->TheEmptyList())
    {
      faceNL = nl->OneElemList(cycleNL);
      faceNLLast = faceNL;
    }
    else
    {
      faceNLLast = nl->Append(faceNLLast, cycleNL);
    }
    cycleNL = nl->TheEmptyList();


    if (regionNL == nl->TheEmptyList())
    {
      regionNL = nl->OneElemList(faceNL);
      regionNLLast = regionNL;
    }
    else
    {
      regionNLLast = nl->Append(regionNLLast, faceNL);
    }
    faceNL = nl->TheEmptyList();

    delete RCopy;
    return regionNL;
  }
}

/*
8.4 ~In~-function

*/
Word
InRegion( const ListExpr typeInfo, const ListExpr instance,
          const int errorPos, ListExpr& errorInfo, bool& correct )
{
  Region* cr = new Region( 0 );

  cr->StartBulkLoad();

  ListExpr RegionNL = instance;
  ListExpr FaceNL, CycleNL;
  int fcno=-1;
  int ccno=-1;
  int edno=-1;
  int partnerno = 0;

  if (!nl->IsAtom(instance))
  {
    while( !nl->IsEmpty( RegionNL ) )
    {
      FaceNL = nl->First( RegionNL );
      RegionNL = nl->Rest( RegionNL);
      bool isCycle = true;

      //A face is composed by 1 cycle, and can have holes.
      //All the holes must be inside the face. (TO BE IMPLEMENTED0)
      //Region *faceCycle;

      fcno++;
      ccno=-1;
      edno=-1;

      if (nl->IsAtom( FaceNL ))
      {
        correct=false;
        return SetWord( Address(0) );
      }

      while (!nl->IsEmpty( FaceNL) )
      {
        CycleNL = nl->First( FaceNL );
        FaceNL = nl->Rest( FaceNL );

        ccno++;
        edno=-1;

        if (nl->IsAtom( CycleNL ))
        {
          correct=false;
          return SetWord( Address(0) );
        }

        if (nl->ListLength( CycleNL) <3)
        {
          cout<<"a cycle must have at least 3 edges!"<<endl;
          correct=false;
          return SetWord( Address(0) );
        }
        else
        {
          ListExpr firstPoint = nl->First( CycleNL );
          ListExpr prevPoint = nl->First( CycleNL );
          ListExpr flagedSeg, currPoint;
          CycleNL = nl->Rest( CycleNL );

          //Starting to compute a new cycle

          Points *cyclepoints= new Points( 0 ); // in memory

          Point *currvertex,p1,p2,firstP;

          //This function has the goal to store the half segments of
          //the cycle that is been treated. When the cycle's computation
          //is terminated the region rDir will be used to compute the
          //insideAbove
          //attribute of the half segments of this cycle.
          Region *rDir = new Region(0);
          rDir->StartBulkLoad();


          currvertex = (Point*) InPoint ( nl->TheEmptyList(),
              firstPoint, 0, errorInfo, correct ).addr;
          if (!correct) return SetWord( Address(0) );
          cyclepoints->StartBulkLoad();
          (*cyclepoints) += (*currvertex);
          p1 = *currvertex;
          firstP = p1;
          cyclepoints->EndBulkLoad();
          delete currvertex;

          while ( !nl->IsEmpty( CycleNL) )
          {
            currPoint = nl->First( CycleNL );
            CycleNL = nl->Rest( CycleNL );

            currvertex = (Point*) InPoint( nl->TheEmptyList(),
                  currPoint, 0, errorInfo, correct ).addr;

            if (!correct) return SetWord( Address(0) );

            if (cyclepoints->Contains(*currvertex))
            {
              cout<<"the same vertex: "<<(*currvertex)
              <<" repeated in the cycle!"<<endl;
              correct=false;
              return SetWord( Address(0) );
            }
            else
            {
              p2 = *currvertex;
              cyclepoints->StartBulkLoad();
              (*cyclepoints) += (*currvertex);
              cyclepoints->EndBulkLoad();
            }
            delete currvertex;

            flagedSeg = nl->TwoElemList
            (nl-> BoolAtom(true),
             nl->TwoElemList(prevPoint, currPoint));
            prevPoint=currPoint;
            edno++;
            //Create left dominating half segment
            HalfSegment * hs = (HalfSegment*)InHalfSegment
                      ( nl->TheEmptyList(), flagedSeg,
                       0, errorInfo, correct ).addr;
            hs->attr.faceno=fcno;
            hs->attr.cycleno=ccno;
            hs->attr.edgeno=edno;
            hs->attr.partnerno=partnerno;
            partnerno++;
            hs->attr.insideAbove = (hs->GetLeftPoint() == p1);
              //true (L-->R ),false (R--L)
            p1 = p2;

            if (( correct )&&( cr->InsertOk(*hs) ))
            {
              (*cr) += (*hs);
              if( hs->IsLeftDomPoint() )
              {
                (*rDir) += (*hs);
                hs->SetLeftDomPoint( false );
              }
              else
              {
                hs->SetLeftDomPoint( true );
                (*rDir) += (*hs);
              }
              (*cr) += (*hs);
              delete hs;
            }
            else
            {
              correct=false;
              return SetWord( Address(0) );
            }

          }
          delete cyclepoints;

          edno++;
          flagedSeg= nl->TwoElemList
            (nl-> BoolAtom(true),
             nl->TwoElemList(firstPoint, currPoint));
          HalfSegment * hs = (HalfSegment*)InHalfSegment
                  ( nl->TheEmptyList(), flagedSeg,
                    0, errorInfo, correct ).addr;
          hs->attr.faceno=fcno;
          hs->attr.cycleno=ccno;
          hs->attr.edgeno=edno;
          hs->attr.partnerno=partnerno;
          hs->attr.insideAbove = (hs->GetRightPoint() == firstP);
          //true (L-->R ),false (R--L),
          //the order of typing is last point than first point.
          partnerno++;

          //The last half segment of the region
          if (( correct )&&( cr->InsertOk(*hs) ))
          {
            (*cr) += (*hs);
            if( hs->IsLeftDomPoint() )
            {
              (*rDir) += (*hs);
              hs->SetLeftDomPoint( false );
            }
            else
            {
              hs->SetLeftDomPoint( true );
              (*rDir) += (*hs);
            }
            (*cr) += (*hs);
            delete hs;
            rDir->EndBulkLoad(true, false, false, false);
            //To calculate the inside above attribute
            bool direction = rDir->GetCycleDirection();
            int h = cr->Size() - ( rDir->Size() * 2 );
            while ( h < cr->Size())
            {
              //after each left half segment of the region is its
              //correspondig right half segment
              const HalfSegment *hsIA;
              bool insideAbove;
              cr->Get(h,hsIA);
              /*
                The test for adjusting the inside above can be described
                as above, but was implemented in a different way that
                produces the same result.
                if ( (direction  && hsIA->attr.insideAbove) ||
                     (!direction && !hsIA->attr.insideAbove) )
                {
                  //clockwise and l-->r or
                  //counterclockwise and r-->l
                  hsIA->attr.insideAbove=false;
                }
                else
                  //clockwise and r-->r or
                  //counterclockwise and l-->r
                  true;

              */
              if (direction == hsIA->attr.insideAbove)
                insideAbove = false;
              else
                insideAbove = true;
              if (!isCycle)
                insideAbove = !insideAbove;
              HalfSegment auxhsIA( *hsIA );
              auxhsIA.attr.insideAbove = insideAbove;
              cr->UpdateAttr(h,auxhsIA.attr);
              //Get right half segment
              cr->Get(h+1,hsIA);
              auxhsIA = *hsIA;
              auxhsIA.attr.insideAbove = insideAbove;
              cr->UpdateAttr(h+1,auxhsIA.attr);
              h+=2;
            }

            //After the first face's cycle read the faceCycle variable is set.
            //Afterwards
            //it is tested if all the new cycles are inside the faceCycle.
            /*
            if (isCycle)
              faceCycle = new Region(rDir,false);
            else
              //To implement the test
            */
            delete rDir;
            //After the end of the first cycle of the face,
            //all the following cycles are
            //holes, then isCycle is set to false.
            isCycle = false;

          }
          else
          {
            correct=false;
            return SetWord( Address(0) );
          }
        }
      }
    }

    cr->SetNoComponents( fcno+1 );
    cr->EndBulkLoad( true, true, true, false );

    correct = true;
    return SetWord( cr );
  }
  else
  {
    correct=false;
    return SetWord( Address(0) );
  }
}

/*
8.5 ~Create~-function

*/
Word
CreateRegion( const ListExpr typeInfo )
{
  //cout << "CreateRegion" << endl;

  return (SetWord( new Region( 0 ) ));
}

/*
8.6 ~Delete~-function

*/
void
DeleteRegion( const ListExpr typeInfo, Word& w )
{
  //cout << "DeleteRegion" << endl;

  Region *cr = (Region *)w.addr;
  cr->Destroy();
  cr->DeleteIfAllowed();
  w.addr = 0;
}

/*
8.7 ~Close~-function

*/
void
CloseRegion( const ListExpr typeInfo, Word& w )
{
  //cout << "CloseRegion" << endl;

  ((Region *)w.addr)->DeleteIfAllowed();
  w.addr = 0;
}

/*
8.8 ~Clone~-function

*/
Word
CloneRegion( const ListExpr typeInfo, const Word& w )
{
  //cout << "CloneRegion" << endl;

  Region *cr = new Region( *((Region *)w.addr) );
  return SetWord( cr );
}

/*
7.8 ~Open~-function

*/
bool
OpenRegion( SmiRecord& valueRecord,
            size_t& offset,
            const ListExpr typeInfo,
            Word& value )
{
  Region *r = (Region*)Attribute::Open( valueRecord, offset, typeInfo );
  value = SetWord( r );
  return true;
}

/*
7.8 ~Save~-function

*/
bool
SaveRegion( SmiRecord& valueRecord,
            size_t& offset,
            const ListExpr typeInfo,
            Word& value )
{
  Region *r = (Region *)value.addr;
  Attribute::Save( valueRecord, offset, typeInfo, r );
  return true;
}

/*
8.9 ~SizeOf~-function

*/
int SizeOfRegion()
{
  return sizeof(Region);
}

/*
8.11 Function describing the signature of the type constructor

*/
ListExpr
RegionProperty()
{
  ListExpr listreplist = nl->TextAtom();
  nl->AppendText(listreplist,"(<face>*) where face is"
                             " (<outercycle><holecycle>*); "
  "<outercycle> and <holecycle> are <points>*");
  ListExpr examplelist = nl->TextAtom();
  nl->AppendText(examplelist,"(((3 0)(10 1)(3 1))((3.1 0.1)"
           "(3.1 0.9)(6 0.8)))");
  ListExpr remarkslist = nl->TextAtom();
  nl->AppendText(remarkslist,"all <holecycle> must be completely within "
  "<outercycle>.");

  return (nl->TwoElemList(
            nl->FiveElemList(nl->StringAtom("Signature"),
                       nl->StringAtom("Example Type List"),
           nl->StringAtom("List Rep"),
           nl->StringAtom("Example List"),
           nl->StringAtom("Remarks")),
            nl->FiveElemList(nl->StringAtom("-> DATA"),
                       nl->StringAtom("region"),
           listreplist,
           examplelist,
           remarkslist)));
}

/*
8.12 Kind checking function

This function checks whether the type constructor is applied correctly. Since
type constructor ~point~ does not have arguments, this is trivial.

*/
bool
CheckRegion( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "region" ));
}

/*
8.13 ~Cast~-function

*/
void* CastRegion(void* addr)
{
  return (new (addr) Region);
}

/*
8.14 Creation of the type constructor instance

*/
TypeConstructor region(
        "region",                       //name
        RegionProperty,                 //describing signature
        OutRegion,      InRegion,       //Out and In functions
        0,              0,              //SaveTo and RestoreFrom List functions
        CreateRegion,   DeleteRegion,   //object creation and deletion
        OpenRegion,     SaveRegion,     // object open and save
        CloseRegion,    CloneRegion,    //object close and clone
        CastRegion,                     //cast function
        SizeOfRegion,                   //sizeof function
        CheckRegion );                  //kind checking function

/*
10 Operators

Definition of operators is similar to definition of type constructors. An
operator is defined by creating an instance of class ~Operator~. Again we
have to define some functions before we are able to create an ~Operator~
instance.

10.1 Type mapping functions

A type mapping function takes a nested list as argument. Its contents are
type descriptions of an operator's input parameters. A nested list describing
the output type of the operator is returned.

10.1.1 Type mapping function SpatialTypeMapBool

It is for the compare operators which have ~bool~ as resulttype, like =, !=, <,
<=, >, >=.

*/
ListExpr
SpatialTypeMapBool( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stpoint)
      return (nl->SymbolAtom( "bool" ));
    if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
         SpatialTypeOfSymbol( arg2 ) == stpoints)
      return (nl->SymbolAtom( "bool" ));
    if ( SpatialTypeOfSymbol( arg1 ) == stline &&
         SpatialTypeOfSymbol( arg2 ) == stline)
      return (nl->SymbolAtom( "bool" ));
    if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
         SpatialTypeOfSymbol( arg2 ) == stregion)
      return (nl->SymbolAtom( "bool" ));
    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stpoints)
      return (nl->SymbolAtom( "bool" ));
    if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
         SpatialTypeOfSymbol( arg2 ) == stpoint)
      return (nl->SymbolAtom( "bool" ));
    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stline)
      return (nl->SymbolAtom( "bool" ));
    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stregion)
      return (nl->SymbolAtom( "bool" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}


ListExpr SpatialTypeMapCompare(ListExpr args){
   string err = " st x st expected, with"
                " st in {point, points, line, region, sline)";
   if(!nl->HasLength(args,2)){
     ErrorReporter::ReportError(err);
     return nl->TypeError();
   }
   ListExpr arg1 = nl->First(args);
   ListExpr arg2 = nl->Second(args);
   if(!nl->Equal(arg1,arg2)){
     ErrorReporter::ReportError(err);
     return nl->TypeError();
   }
   SpatialType st = SpatialTypeOfSymbol(arg1);
   if( st ==  stpoint ||
       st == stpoints ||
       st == stline   ||
       st == stregion ||
       st == stsline){
     return nl->SymbolAtom("bool");
   }
   ErrorReporter::ReportError(err);
   return nl->TypeError();
}

/*
10.1.2 Type mapping function GeoGeoMapBool

It is for the binary operators which have ~bool~ as result type, such as interscets,
inside, onborder, ininterior, etc.

*/

ListExpr
GeoGeoMapBool( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if (((SpatialTypeOfSymbol( arg1 ) == stpoint)  ||
         (SpatialTypeOfSymbol( arg1 ) == stpoints) ||
         (SpatialTypeOfSymbol( arg1 ) == stline)     ||
         (SpatialTypeOfSymbol( arg1 ) == stregion)) &&
        ((SpatialTypeOfSymbol( arg2 ) == stpoint)  ||
         (SpatialTypeOfSymbol( arg2 ) == stpoints) ||
         (SpatialTypeOfSymbol( arg2 ) == stline)     ||
         (SpatialTypeOfSymbol( arg2 ) == stregion)))
      return (nl->SymbolAtom( "bool" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.2 Type Mapping setsetmapbool

It is for all combinations of spatial types which are represented as
sets, .i.e all types except point.

*/

ListExpr
IntersectsTM( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    SpatialType st1 = SpatialTypeOfSymbol( arg1 );
    SpatialType st2 = SpatialTypeOfSymbol( arg2 );
    if ( ((st1 == stpoints) || (st1 == stline) || (st1 == stregion)) &&
         ((st2 == stpoints) || (st2 == stline) || (st2 == stregion)))
    {
      return (nl->SymbolAtom( "bool" ));
    }
    else if(st1==stsline && st2==stsline)
    {
      return (nl->SymbolAtom( "bool" ));
    }
    else
    {
      ErrorReporter::ReportError(" t_1 x t_2 expected,"
                                 " with t_1, t_2 in {points,line,region}");
    }
  } else {
      ErrorReporter::ReportError("two arguments expected");
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.2 PointsRegionMapBool

*/
ListExpr
PointsRegionMapBool( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if (((SpatialTypeOfSymbol( arg1 ) == stpoints) ) &&
        ((SpatialTypeOfSymbol( arg2 ) == stregion))){
      return (nl->SymbolAtom( "bool" ));
    } else {
      ErrorReporter::ReportError("points x region expected");
    }
  } else {
      ErrorReporter::ReportError("Two arguments expected");
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.2 RegionRegionMapBool

*/
ListExpr
RegionRegionMapBool( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if (((SpatialTypeOfSymbol( arg1 ) == stregion) ) &&
        ((SpatialTypeOfSymbol( arg2 ) == stregion)))
      return (nl->SymbolAtom( "bool" ));
    else {
      ErrorReporter::ReportError("region x region expected");
    }
  } else{
      ErrorReporter::ReportError("two arguments expected");
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.2 PointRegionMapBool

*/
ListExpr
PointRegionMapBool( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if (((SpatialTypeOfSymbol( arg1 ) == stpoint) ) &&
        ((SpatialTypeOfSymbol( arg2 ) == stregion)))
      return (nl->SymbolAtom( "bool" ));
    else {
      ErrorReporter::ReportError("points x region expected");
    }
  } else {
      ErrorReporter::ReportError("two arguments expected");
  }
  return (nl->SymbolAtom( "typeerror" ));
}


/*
10.1.2 AdjacentTypeMap

*/
ListExpr
AdjacentTypeMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if(
      ((SpatialTypeOfSymbol(arg1)==stpoints) &&
       (SpatialTypeOfSymbol(arg2)==stregion)) ||
      ((SpatialTypeOfSymbol(arg1)==stline) &&
       (SpatialTypeOfSymbol(arg2)==stregion)) ||
      ((SpatialTypeOfSymbol(arg1)==stregion) &&
       (SpatialTypeOfSymbol(arg2)==stpoints)) ||
      ((SpatialTypeOfSymbol(arg1)==stregion) &&
       (SpatialTypeOfSymbol(arg2)==stline)) ||
      ((SpatialTypeOfSymbol(arg1)==stregion) &&
       (SpatialTypeOfSymbol(arg2)==stregion))){
      return nl->SymbolAtom("bool");
    } else {
      ErrorReporter::ReportError("points x region expected");
    }
  } else {
      ErrorReporter::ReportError("two arguments expected");
  }
  return (nl->SymbolAtom( "typeerror" ));
}


/*
10.1.2 InsideTypeMap

*/
ListExpr
InsideTypeMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if(
      ((SpatialTypeOfSymbol(arg1)==stpoint) &&
       (SpatialTypeOfSymbol(arg2)==stpoints)) ||

      ((SpatialTypeOfSymbol(arg1)==stpoint) &&
       (SpatialTypeOfSymbol(arg2)==stline)) ||

      ((SpatialTypeOfSymbol(arg1)==stpoint) &&
       (SpatialTypeOfSymbol(arg2)==stregion)) ||

      ((SpatialTypeOfSymbol(arg1)==stpoints) &&
       (SpatialTypeOfSymbol(arg2)==stpoints)) ||

      ((SpatialTypeOfSymbol(arg1)==stpoints) &&
       (SpatialTypeOfSymbol(arg2)==stline))||

      ((SpatialTypeOfSymbol(arg1)==stpoints) &&
       (SpatialTypeOfSymbol(arg2)==stregion)) ||

      ((SpatialTypeOfSymbol(arg1)==stline) &&
       (SpatialTypeOfSymbol(arg2)==stline)) ||

      ((SpatialTypeOfSymbol(arg1)==stline) &&
       (SpatialTypeOfSymbol(arg2)==stregion)) ||

      ((SpatialTypeOfSymbol(arg1)==stregion) &&
       (SpatialTypeOfSymbol(arg2)==stregion)) ){
      return nl->SymbolAtom("bool");
    } else {
      ErrorReporter::ReportError("points x region expected");
    }
  } else {
      ErrorReporter::ReportError("two arguments expected");
  }
  return (nl->SymbolAtom( "typeerror" ));
}


/*
10.1.3 Type mapping function SpatialTypeMapBool1

It is for the operator ~isempty~ which have ~point~, ~points~, ~line~, and ~region~ as input and ~bool~ resulttype.

*/

ListExpr
SpatialTypeMapBool1( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );
    if ( SpatialTypeOfSymbol( arg1 ) == stpoint )
      return (nl->SymbolAtom( "bool" ));
    if ( SpatialTypeOfSymbol( arg1 ) == stpoints )
      return (nl->SymbolAtom( "bool" ));
    if ( SpatialTypeOfSymbol( arg1 ) == stline )
      return (nl->SymbolAtom( "bool" ));
    if ( SpatialTypeOfSymbol( arg1 ) == stregion )
      return (nl->SymbolAtom( "bool" ));
    if ( SpatialTypeOfSymbol( arg1 ) == stsline )
      return (nl->SymbolAtom( "bool" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.4 Type mapping function for operator ~intersection~

This type mapping function is the one for ~intersection~ operator. This is a SET operation
so that the result type is a set such as points, line, or region.

*/

ListExpr
SpatialIntersectionMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stpoint )
      return (nl->SymbolAtom( "point" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stpoints )
      return (nl->SymbolAtom( "point" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
         SpatialTypeOfSymbol( arg2 ) == stpoint )
      return (nl->SymbolAtom( "point" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stline )
      return (nl->SymbolAtom( "point" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stline &&
         SpatialTypeOfSymbol( arg2 ) == stpoint )
      return (nl->SymbolAtom( "point" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stregion )
      return (nl->SymbolAtom( "point" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
         SpatialTypeOfSymbol( arg2 ) == stpoint )
      return (nl->SymbolAtom( "point" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
         SpatialTypeOfSymbol( arg2 ) == stpoints )
      return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
         SpatialTypeOfSymbol( arg2 ) == stline )
      return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stline &&
         SpatialTypeOfSymbol( arg2 ) == stpoints )
      return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
         SpatialTypeOfSymbol( arg2 ) == stregion )
      return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
         SpatialTypeOfSymbol( arg2 ) == stpoints )
      return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stline &&
         SpatialTypeOfSymbol( arg2 ) == stline )
      return (nl->SymbolAtom( "line" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stline &&
         SpatialTypeOfSymbol( arg2 ) == stregion )
      return (nl->SymbolAtom( "line" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
         SpatialTypeOfSymbol( arg2 ) == stline )
      return (nl->SymbolAtom( "line" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
         SpatialTypeOfSymbol( arg2 ) == stregion )
      return (nl->SymbolAtom( "region" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}


/*
10.1.5 Type mapping function for operator ~minus~

This type mapping function is the one for ~minus~ operator. This is a SET operation
so that the result type is a set such as points, line, or region.

*/
ListExpr
SpatialMinusMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stpoint )
        return (nl->SymbolAtom( "point" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stpoints )
        return (nl->SymbolAtom( "point" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stline )
        return (nl->SymbolAtom( "point" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stregion )
        return (nl->SymbolAtom( "point" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
         SpatialTypeOfSymbol( arg2 ) == stpoint )
        return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
         SpatialTypeOfSymbol( arg2 ) == stpoints )
        return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
         SpatialTypeOfSymbol( arg2 ) == stline )
        return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
         SpatialTypeOfSymbol( arg2 ) == stregion )
        return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stline &&
         SpatialTypeOfSymbol( arg2 ) == stpoint )
        return (nl->SymbolAtom( "line" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stline &&
         SpatialTypeOfSymbol( arg2 ) == stpoints )
        return (nl->SymbolAtom( "line" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stline &&
         SpatialTypeOfSymbol( arg2 ) == stline )
        return (nl->SymbolAtom( "line" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
         SpatialTypeOfSymbol( arg2 ) == stpoint )
        return (nl->SymbolAtom( "region" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
         SpatialTypeOfSymbol( arg2 ) == stpoints )
        return (nl->SymbolAtom( "region" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
         SpatialTypeOfSymbol( arg2 ) == stline )
        return (nl->SymbolAtom( "region" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.6 Type mapping function for operator ~union~

This type mapping function is the one for ~union~ operator. This is a SET operation
so that the result type is a set such as points, line, or region.

*/
ListExpr
SpatialUnionMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stpoints )
      return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
         SpatialTypeOfSymbol( arg2 ) == stpoint )
      return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
         SpatialTypeOfSymbol( arg2 ) == stpoints )
      return (nl->SymbolAtom( "points" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.7 Type mapping function for operator ~crossings~

This type mapping function is the one for ~crossings~ operator. This operator
compute the crossing point of two lines so that the result type is a set of points.

*/
ListExpr
SpatialCrossingsTM( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stline &&
         SpatialTypeOfSymbol( arg2 ) == stline )
      return (nl->SymbolAtom( "points" ));
    if ( SpatialTypeOfSymbol( arg1 ) == stsline &&
         SpatialTypeOfSymbol( arg2 ) == stsline )
      return (nl->SymbolAtom( "points" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.8 Type mapping function for operator ~single~

This type mapping function is used for the ~single~ operator. This
operator transform a single-element points value to a point.

*/
ListExpr
SpatialSingleMap( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );

    if (SpatialTypeOfSymbol( arg1 ) == stpoints)
      return (nl->SymbolAtom( "point" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.9 Type mapping function for operator ~distance~

This type mapping function is used for the ~distance~ operator. This
operator computes the distance between two spatial objects.

*/
ListExpr
SpatialDistanceMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    SpatialType t1 = SpatialTypeOfSymbol(arg1);
    SpatialType t2 = SpatialTypeOfSymbol(arg2);
    ListExpr erg = nl->SymbolAtom("real");

    if ( (t1 == stpoint) && (t2 == stpoint) ){
      return erg;
    }else if ( (t1  == stpoint) && (t2 == stpoints) ){
      return erg;
    }else if ( (t1 == stpoints) && (t2 == stpoint )) {
      return erg;
    } else if ( (t1 == stpoint) && (t2 == stline ) ){
      return erg;
    } else if( (t1 == stline) && ( t2 ==  stpoint )){
      return erg;
    } else if( (t1  == stpoint) && (t2 == stregion )) {
      return erg;
    } else if ( (t1  == stregion) && ( t2 == stpoint )){
      return erg;
    } else if ( ( t1 == stpoints) && ( t2 == stpoints )){
      return erg;
    } else if ( (t1 == stpoints) && (t2 == stline )){
      return erg;
    } else  if ( ( t1 == stline )&&( t2  == stpoints )){
      return erg;
    } else if ( (t1 == stpoints) && ( t2  == stregion )){
      return erg;
    } else if ( (t1  == stregion) && (t2 == stpoints )){
      return erg;
    } else if ( (t1 == stline) && (t2 == stline )){
      return erg;
    } else if ( ( t1 == stregion) && ( t2 == stregion )){
      return erg;
    } else if ( (t1 == stsline) && (t2 == stpoint)){
      return erg;
    } else if( ( t1 == stpoint) && (t2 == stsline)){
      return erg;
    } else if(( t1 == stsline) && ( t2 == stpoints)){
      return erg;
    } else if((t1==stpoints) && (t2==stsline)){
      return erg;
    } else if((t1==stsline) && (t2==stsline)){
      return erg;
    } else if((t1==stbox) && (t2==stpoint)){
      return erg;
    } else if((t1==stpoint) && (t2==stbox)){
      return erg;
    } else if((t1==stbox) && (t2==stpoints)){
      return erg;
    } else if((t1==stpoints) && (t2==stbox)){
      return erg;
    } else if((t1==stbox) && (t2==stline)){
      return erg;
    } else if((t1==stline) && (t2==stbox)){
      return erg;
    } else if((t1==stbox) && (t2==stregion)){
      return erg;
    } else if((t1==stregion) && (t2==stbox)){
      return erg;
    } else if((t1==stbox) && (t2==stsline)){
      return erg;
    } else if((t1==stsline) && (t2==stbox)){
      return erg;
    }
  }
  return nl->TypeError();
}

/*
10.1.10 Type mapping function for operator ~direction~

This type mapping function is used for the ~direction~ operator. This
operator computes the direction from the first point to the second point.

*/
ListExpr
SpatialDirectionMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stpoint )
      return (nl->SymbolAtom( "real" ));
  }

  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.11 Type mapping function for operator ~no\_compoents~

This type mapping function is used for the ~no\_components~ operator. This
operator computes the number of components of a spatial object. For poins,
this function returns the number of points contained in the point set.
For regions, this function returns the faces of the region.

*/
ListExpr
SpatialNoComponentsMap( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );

    if ((SpatialTypeOfSymbol( arg1 ) == stpoints)||
        (SpatialTypeOfSymbol( arg1 ) == stline)||
        (SpatialTypeOfSymbol( arg1 ) == stregion))
      return (nl->SymbolAtom( "int" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.11 Type mapping function for operator ~no\_segments~

This type mapping function is used for the ~no\_segments~ operator. This
operator computes the number of segments of a spatial object (lines and
regions only).

*/
ListExpr
SpatialNoSegmentsMap( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );

    if ((SpatialTypeOfSymbol( arg1 ) == stline)||
        (SpatialTypeOfSymbol( arg1 ) == stregion)||
        (SpatialTypeOfSymbol(arg1) == stsline))
        return (nl->SymbolAtom( "int" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.12 Type mapping function for operator ~size~

This type mapping function is used for the ~size~ operator. This operator
computes the size of the spatial object. For line, the size is the totle length
of the line segments.

*/
ListExpr
SpatialSizeMap( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );
    SpatialType st = SpatialTypeOfSymbol(arg1);
    if(st==stregion || st==stline || st==stsline){
      return (nl->SymbolAtom( "real" ));
    }
  }

  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.13 Type mapping function for operator ~touchpoints~

This type mapping function is used for the ~touchpoints~ operator. This operator
computes the touchpoints of a region and another region or a line.

*/
ListExpr
SpatialTouchPointsMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
         SpatialTypeOfSymbol( arg2 ) == stregion )
      return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
         SpatialTypeOfSymbol( arg2 ) == stline )
      return (nl->SymbolAtom( "points" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stline &&
         SpatialTypeOfSymbol( arg2 ) == stregion )
      return (nl->SymbolAtom( "points" ));
  }

  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.17 Type mapping function for operator ~components~

This type mapping function is used for the ~components~ operator.

*/
ListExpr SpatialComponentsMap( ListExpr args )
{
  if( nl->ListLength( args ) == 1 )
  {
    if( SpatialTypeOfSymbol( nl->First( args ) ) == stpoints )
      return nl->TwoElemList( nl->SymbolAtom("stream"),
                              nl->SymbolAtom("point") );

    if( SpatialTypeOfSymbol( nl->First( args ) ) == stregion )
      return nl->TwoElemList( nl->SymbolAtom("stream"),
                              nl->SymbolAtom("region") );

    if( SpatialTypeOfSymbol( nl->First( args ) ) == stline )
      return nl->TwoElemList( nl->SymbolAtom("stream"),
                              nl->SymbolAtom("line") );

    ErrorReporter::ReportError("point, line or region expected");
    return nl->TypeError();
  }
  ErrorReporter::ReportError("Wrong number of arguments");
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.18 Type Mapping function for operator ~vertices~

*/
ListExpr SpatialVerticesMap(ListExpr args)
{
  if(nl->ListLength(args)!=1)
  {
    ErrorReporter::ReportError("one argument expected");
    return nl->SymbolAtom("typeerror");
  }
  if( (nl->IsEqual(nl->First(args),"region")) ||
      (nl->IsEqual(nl->First(args),"line")) ){
    return nl->SymbolAtom("points");
  }

  ErrorReporter::ReportError("region or line required");
  return nl->SymbolAtom("typeerror");
}

/*
10.1.18 Type Mapping function for operator ~boundary~

*/
ListExpr SpatialBoundaryMap(ListExpr args)
{
  if(nl->ListLength(args)!=1)
  {
    ErrorReporter::ReportError("invalid number of arguments");
    return nl->SymbolAtom("typeerror");
  }
  if( SpatialTypeOfSymbol( nl->First( args ) ) == stregion ){
     return nl->SymbolAtom("line");
  }

  if( SpatialTypeOfSymbol( nl->First( args ) ) == stline ){
      return nl->SymbolAtom("points");
  }
  ErrorReporter::ReportError("region or line required");
  return nl->SymbolAtom("typeerror");
}

/*
10.1.14 Type mapping function for operator ~commonborder~

This type mapping function is used for the ~commonborder~ operator. This operator
computes the commonborder of two regions.

*/
ListExpr
SpatialCommonBorderMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
         SpatialTypeOfSymbol( arg2 ) == stregion )
      return (nl->SymbolAtom( "line" ));
  }

  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.15 Type mapping function for operator ~bbox~

This type mapping function is used for the ~bbox~ operator. This operator
computes the bbox of a region, which is a ~rect~ (see RectangleAlgebra).

*/
ListExpr
SpatialBBoxMap( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stregion ||
         SpatialTypeOfSymbol( arg1 ) == stpoint ||
         SpatialTypeOfSymbol( arg1 ) == stline ||
         SpatialTypeOfSymbol( arg1 ) == stpoints ||
         SpatialTypeOfSymbol( arg1 ) == stsline )
      return (nl->SymbolAtom( "rect" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.17 Type mapping function for operator ~translate~

This type mapping function is used for the ~translate~ operator. This operator
moves a region parallelly to another place and gets another region.

*/
ListExpr
SpatialTranslateMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if( SpatialTypeOfSymbol( arg1 ) == stregion &&
        nl->IsEqual(nl->First( arg2 ), "real") &&
        nl->IsEqual(nl->Second( arg2 ), "real"))
      return (nl->SymbolAtom( "region" ));

    if( SpatialTypeOfSymbol( arg1 ) == stline &&
        nl->IsEqual(nl->First( arg2 ), "real") &&
        nl->IsEqual(nl->Second( arg2 ), "real"))
      return (nl->SymbolAtom( "line" ));

    if( SpatialTypeOfSymbol( arg1 ) == stpoints &&
        nl->IsEqual(nl->First( arg2 ), "real") &&
        nl->IsEqual(nl->Second( arg2 ), "real"))
      return (nl->SymbolAtom( "points" ));

    if( SpatialTypeOfSymbol( arg1 ) == stpoint &&
        nl->IsEqual(nl->First( arg2 ), "real") &&
        nl->IsEqual(nl->Second( arg2 ), "real"))
      return (nl->SymbolAtom( "point" ));
  }

  return nl->SymbolAtom( "typeerror" );
}

/*
10.1.17 Type mapping function for operator ~rotate~

This type mapping function is used for the ~rotate~ operator.
The mamp is spatialtype x real x real x real -> spatialtype

*/
ListExpr
SpatialRotateMap( ListExpr args )
{
  if ( nl->ListLength( args ) != 4 )
  { ErrorReporter::ReportError("wrong number of arguments (4 expected)");
    return nl->TypeError();
  }
  ListExpr arg1 = nl->First(args);
  ListExpr arg2 = nl->Second(args);
  ListExpr arg3 = nl->Third(args);
  ListExpr arg4 = nl->Fourth(args);

  if( !nl->IsEqual(arg2,"real") ||
      !nl->IsEqual(arg3,"real") ||
      !nl->IsEqual(arg4,"real")){
    ErrorReporter::ReportError("spatial x real x real x real expected");
    return nl->TypeError();
  }

  if(!nl->AtomType(arg1)==SymbolType){
    ErrorReporter::ReportError("spatial x real x real x real expected");
    return nl->TypeError();
  }
  string st = nl->SymbolValue(arg1);
  if( st!="point" && st!="points" && st!="line" && st!="region"){
    ErrorReporter::ReportError("spatial x real x real x real expected");
    return nl->TypeError();
  }
  return nl->SymbolAtom(st);

}

/*
10.1.16 Type Mapping function for center.

The signature is points -> point.

*/

ListExpr SpatialCenterMap(ListExpr args){

  if( (nl->ListLength(args)==1) &&
      (nl->IsEqual(nl->First(args),"points")) ){
      return nl->SymbolAtom("point");
  }

  ErrorReporter::ReportError("points expected");
  return nl->TypeError();

}


/*
10.1.17 Type Mapping function for convexhull.

The signature is points -> region.

*/

ListExpr SpatialConvexhullMap(ListExpr args){

  if( (nl->ListLength(args)==1) &&
      (nl->IsEqual(nl->First(args),"points")) ){
      return nl->SymbolAtom("region");
  }

  ErrorReporter::ReportError("points expected");
  return nl->TypeError();

}


/*
10.1.17 Type mapping function for operator ~windowclipping~

This type mapping function is used for the ~windowclipping~ operators. There are
two kind of operators, one that computes the part of the object that is inside
the window (windowclippingin), and another one that computes the part that is
outside of it (windowclippingout).

*/
ListExpr
SpatialWindowClippingMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stline)
        return (nl->SymbolAtom( "line" ));

    if ( SpatialTypeOfSymbol( arg1 ) == stregion )
        return (nl->SymbolAtom( "region" ));
  }

  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.18 Type mapping function for operator ~scale~

This type mapping function is used for the ~scale~ operator. This operator
scales a spatial object by a given factor.

*/
ListExpr SpatialScaleMap(ListExpr args)
{
   if(nl->ListLength(args)!=2)
   {
      ErrorReporter::ReportError("operator scale requires two arguments");
      return nl->SymbolAtom( "typeerror" );
   }

   ListExpr arg1 = nl->First(args);
   ListExpr arg2 = nl->Second(args);
   if(!(nl->IsEqual(arg2 , "real")))
   {
      ErrorReporter::ReportError("the second "
                                 "argument has to be of type real");
      return nl->SymbolAtom("typeerror");
   }

   if(nl->IsEqual(arg1,"region"))
     return nl->SymbolAtom("region");
   if(nl->IsEqual(arg1,"line"))
     return nl->SymbolAtom("line");
   if(nl->IsEqual(arg1,"point"))
     return nl->SymbolAtom("point");
   if(nl->IsEqual(arg1,"points"))
     return nl->SymbolAtom("points");

   ErrorReporter::ReportError("First argument has to be in "
                              "{region, line, points, points}");
   return nl->SymbolAtom( "typeerror" );
}

/*
10.1.6 Type mapping function for operator ~atpoint~

This type mapping function is the one for ~atpoint~ operator. This operator
receives a line and a point and returns the relative position of the point
in the line as a real.

*/
ListExpr
SpatialAtPointMap( ListExpr args )
{
  ListExpr arg1, arg2, arg3;
  if ( nl->ListLength( args ) == 3 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    arg3 = nl->Third( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stsline &&
         SpatialTypeOfSymbol( arg2 ) == stpoint &&
         nl->IsEqual( arg3, "bool" ) )
      return (nl->SymbolAtom( "real" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.6 Type mapping function for operator ~atposition~

This type mapping function is the one for ~atposition~ operator. This operator
receives a line and a relative position and returns the corresponding point.

*/
ListExpr
SpatialAtPositionMap( ListExpr args )
{
  ListExpr arg1, arg2, arg3;
  if ( nl->ListLength( args ) == 3 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    arg3 = nl->Third( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stsline &&
         nl->IsEqual( arg2, "real" ) &&
         nl->IsEqual( arg3, "bool" ) )
      return (nl->SymbolAtom( "point" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.6 Type mapping function for operator ~subline~

This type mapping function is the one for ~subline~ operator. This operator
receives a line and two relative positions and returns the corresponding
sub-line.

*/
ListExpr
SpatialSubLineMap( ListExpr args )
{
  ListExpr arg1, arg2, arg3, arg4;
  if ( nl->ListLength( args ) == 4 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    arg3 = nl->Third( args );
    arg4 = nl->Fourth( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stsline &&
         nl->IsEqual( arg2, "real" ) &&
         nl->IsEqual( arg3, "real" ) &&
         nl->IsEqual( arg4, "bool" ) )
      return (nl->SymbolAtom( "sline" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.6 Type mapping function for operator ~add~

This type mapping function is the one for the ~add~ operator.
The result type is a point.

*/
ListExpr
SpatialAddTypeMap( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
         SpatialTypeOfSymbol( arg2 ) == stpoint )
      return (nl->SymbolAtom( "point" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.7 Type mapping function for the operators ~getx~ and ~gety~

This type mapping function is the one for the ~getx and ~gety operator.
The result type is a real.

*/
ListExpr
SpatialGetXYMap( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stpoint )
      return (nl->SymbolAtom( "real" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.7 Type mapping function for the operators ~line2region~

This type mapping function is the one for the ~line2region~ operator.
The result type is a region.

*/
ListExpr
SpatialLine2RegionMap( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stline )
      return (nl->SymbolAtom( "region" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.7 Type mapping function for the operator ~rect2region~

This type mapping function is the one for the ~rect2region~ operator.
The result type is a region.

*/
ListExpr
    SpatialRect2RegionMap( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stbox )
      return (nl->SymbolAtom( "region" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
10.1.8 Type mapping function for the operator ~area~

This type mapping function is the one for the ~area~ operator.
The result type is a real.

*/
ListExpr
    SpatialAreaMap( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );

    if ( SpatialTypeOfSymbol( arg1 ) == stregion )
      return (nl->SymbolAtom( "real" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}


/*
10.1.9 Type mapping for the ~polylines~ operator

The ~polylines~ operator takes a complex line and creates
a set of simple polylines from it. Thus, the signature of
this operator is line -> stream(line)

*/
ListExpr PolylinesMap(ListExpr args){
  int len = nl->ListLength(args);
  if( (len!=2) &&len!=3){
     ErrorReporter::ReportError("line x bool [x points] expected");
     return nl->TypeError();
  }
  if(!nl->IsEqual(nl->First(args),"line") ||
     !nl->IsEqual(nl->Second(args),"bool")){
     ErrorReporter::ReportError("line  x bool expected");
     return nl->TypeError();
  }
  if(len==3){
     if(!nl->IsEqual(nl->Third(args),"points")){
       ErrorReporter::ReportError("line x bool [x points] expected");
       return nl->TypeError();
     }
  }
  return nl->TwoElemList(nl->SymbolAtom("stream"),
                         nl->SymbolAtom("line"));
}


/*
10.1.10 Type mapping for the ~simplify~ operator

*/
ListExpr SimplifyTypeMap(ListExpr args){
   int len = nl->ListLength(args);
   if((len!=2) && (len!=3)){
      ErrorReporter::ReportError("invalid number of"
                                 " arguments (has to be 2 or 3 )");
      return nl->TypeError();
   }
   if(!nl->IsEqual(nl->First(args),"line") ||
      !nl->IsEqual(nl->Second(args),"real")){
      ErrorReporter::ReportError("line x real [x points] expected");
      return nl->TypeError();
   }
   if( (len==3) &&
       !(nl->IsEqual(nl->Third(args),"points"))){
       ErrorReporter::ReportError("line x real [ x points] expected");
       return nl->TypeError();
   }
   return nl->SymbolAtom("line");
}



/*
10.1.11 Type Mapping for the ~segments~ operator

*/
ListExpr SegmentsTypeMap(ListExpr args){
  if(nl->ListLength(args)!=1){
     ErrorReporter::ReportError("Invalid number of arguments");
     return nl->TypeError();
  }
  if(!nl->IsEqual(nl->First(args),"line")){
     ErrorReporter::ReportError("line expected");
     return nl->TypeError();
  }
  return nl->TwoElemList(
               nl->SymbolAtom("stream"),
               nl->SymbolAtom("line")
         );
}

/*
10.1.12 Type Mapping for the ~get~ operator

Signatur is points x int -> point

*/
ListExpr GetTypeMap(ListExpr args){
   if( (nl->ListLength(args)==2) &&
       (nl->IsEqual(nl->First(args),"points")) &&
       (nl->IsEqual(nl->Second(args),"int"))){
      return nl->SymbolAtom("point");
   }
   ErrorReporter::ReportError("points x int expected");
   return nl->TypeError();
}

/*
10.1.13 Type Mapping for the ~realminize~ operator

Signatur is line -> line

*/
ListExpr RealminizeTypeMap(ListExpr args){
  if( (nl->ListLength(args)==1) &&
       (nl->IsEqual(nl->First(args),"line"))){
    return nl->SymbolAtom("line");
       }
       ErrorReporter::ReportError("line expected");
       return nl->TypeError();
}

/*
10.1.14 Type Mapping for the ~makeline~ operator

Signature is point x point -> line

*/
ListExpr MakeLineTypeMap(ListExpr args){
  int len;
  if((len = nl->ListLength(args))!=2){
    ErrorReporter::ReportError("two arguments expected, but got " + len);
    return nl->TypeError();
  }
  if(nl->IsEqual(nl->First(args),"point") &&
     nl->IsEqual(nl->Second(args),"point")){
     return nl->SymbolAtom("line");
  } else {
    ErrorReporter::ReportError("point x point expected");
    return nl->TypeError();
  }
}

/*
~Union2TypeMap~

Signatures:

  line [x] line [->] line

  line [x] region [->] region

  region [x] line [->] region

  region [x] region [->] region

*/


ListExpr Union2TypeMap(ListExpr args){

   if(nl->ListLength(args)!=2){
     ErrorReporter::ReportError("2 arguments expected.");
     return nl->TypeError();
   }
   ListExpr arg1 = nl->First(args);
   ListExpr arg2 = nl->Second(args);

   if(!IsSpatialType(arg1) || !IsSpatialType(arg2)){
     ErrorReporter::ReportError("two spatial type expected");
     return nl->TypeError();
   }

   if(nl->IsEqual(arg1,"line") && nl->IsEqual(arg2,"line")){
     return nl->SymbolAtom("line");
   }
   if(nl->IsEqual(arg1,"region") && nl->IsEqual(arg2,"region")){
     return nl->SymbolAtom("region");
   }

   if(nl->IsEqual(arg1,"line") && nl->IsEqual(arg2,"region")){
     return nl->SymbolAtom("region");
   }

   if(nl->IsEqual(arg1,"region") && nl->IsEqual(arg2,"line")){
     return nl->SymbolAtom("region");
   }
   ErrorReporter::ReportError("combination not implemented yet");
   return nl->TypeError();
}

/*
~Intersection2TypeMap~

Signatures:

  line [x] line [->] line

  line [x] region [->] line

  region [x] line [->] line

  region [x] region [->] region

*/
ListExpr Intersection2TypeMap(ListExpr args){

   if(nl->ListLength(args)!=2){
     ErrorReporter::ReportError("2 arguments expected.");
     return nl->TypeError();
   }
   ListExpr arg1 = nl->First(args);
   ListExpr arg2 = nl->Second(args);

   if(!IsSpatialType(arg1) || !IsSpatialType(arg2)){
     ErrorReporter::ReportError("two spatial type expected");
     return nl->TypeError();
   }

   if(nl->IsEqual(arg1,"line") && nl->IsEqual(arg2,"line")){
     return nl->SymbolAtom("line");
   }
   if(nl->IsEqual(arg1,"region") && nl->IsEqual(arg2,"region")){
     return nl->SymbolAtom("region");
   }

   if(nl->IsEqual(arg1,"line") && nl->IsEqual(arg2,"region")){
     return nl->SymbolAtom("line");
   }

   if(nl->IsEqual(arg1,"region") && nl->IsEqual(arg2,"line")){
     return nl->SymbolAtom("line");
   }
   ErrorReporter::ReportError("combination not implemented yet");
   return nl->TypeError();
}


/*
~Difference2TypeMap~

Signatures:

  line [x] line [->] line

  line [x] region [->] line

  region [x] line [->] region

  region [x] region [->] region

*/

ListExpr Difference2TypeMap(ListExpr args){

   if(nl->ListLength(args)!=2){
     ErrorReporter::ReportError("2 arguments expected.");
     return nl->TypeError();
   }
   ListExpr arg1 = nl->First(args);
   ListExpr arg2 = nl->Second(args);

   if(!IsSpatialType(arg1) || !IsSpatialType(arg2)){
     ErrorReporter::ReportError("two spatial type expected");
     return nl->TypeError();
   }

   if(nl->IsEqual(arg1,"line") && nl->IsEqual(arg2,"line")){
     return nl->SymbolAtom("line");
   }
   if(nl->IsEqual(arg1,"region") && nl->IsEqual(arg2,"region")){
     return nl->SymbolAtom("region");
   }

   if(nl->IsEqual(arg1,"line") && nl->IsEqual(arg2,"region")){
     return nl->SymbolAtom("line");
   }

   if(nl->IsEqual(arg1,"region") && nl->IsEqual(arg2,"line")){
     return nl->SymbolAtom("region");
   }
   ErrorReporter::ReportError("combination not implemented yet");
   return nl->TypeError();
}


/*
~CommonBorder2TypeMap~

Signature: ~region~ [x] ~region~ [->] ~line~

*/

ListExpr CommonBorder2TypeMap(ListExpr args){

  if(nl->ListLength(args)!=2){
     ErrorReporter::ReportError("Wrong number of arguments,"
                                " region x region expected");
     return nl->TypeError();
  }
  if(nl->IsEqual(nl->First(args),"region") &&
     nl->IsEqual(nl->Second(args),"region")){
     return nl->SymbolAtom("line");
  }
  ErrorReporter::ReportError(" region x region expected");
  return nl->TypeError();
}

/*
~toLineTypeMap~

Signature is :  sline [->] line

*/

ListExpr toLineTypeMap(ListExpr args){
  const string err = "sline expected";
  if(nl->ListLength(args)!=1){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  if(nl->IsEqual(nl->First(args),"sline")){
    return nl->SymbolAtom("line");
  }
  ErrorReporter::ReportError(err);
  return nl->TypeError();
}


/*
~fromLineTypeMap~

Signature is :  line [->] sline

*/

ListExpr fromLineTypeMap(ListExpr args){
  const string err = "line expected";
  if(nl->ListLength(args)!=1){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  if(nl->IsEqual(nl->First(args),"line")){
    return nl->SymbolAtom("sline");
  }
  ErrorReporter::ReportError(err);
  return nl->TypeError();
}

/*
~isCycleTypeMap~

Signature is :  sline [->] bool

*/

ListExpr isCycleTypeMap(ListExpr args){
  const string err = "sline expected";
  if(nl->ListLength(args)!=1){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  if(nl->IsEqual(nl->First(args),"sline")){
    return nl->SymbolAtom("bool");
  }
  ErrorReporter::ReportError(err);
  return nl->TypeError();
}


/*
10.3 Selection functions

A selection function is quite similar to a type mapping function. The only
difference is that it doesn't return a type but the index of a value
mapping function being able to deal with the respective combination of
input parameter types.

Note that a selection function does not need to check the correctness of
argument types; it has already been checked by the type mapping function that it
is applied to correct arguments.

10.3.2 Selection function ~SpatialSelectIsEmpty~

It is used for the ~isempty~ operator

*/
int
SpatialSelectIsEmpty( ListExpr args )
{
  ListExpr arg1 = nl->First( args );

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint )
    return 0;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints )
    return 1;

  if ( SpatialTypeOfSymbol( arg1 ) == stline )
    return 2;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion )
    return 3;

  if(nl->IsEqual(arg1,"sline")){
    return 4;
  }

  return -1; // This point should never be reached
}

/*
10.3.3 Selection function ~SpatialSelectCompare~

It is used for compare operators ($=$, $\neq$, $<$, $>$, $\geq$, $\leq$)

*/
int
SpatialSelectCompare( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stpoint )
    return 0;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 1;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 2;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 3;

  if ( SpatialTypeOfSymbol( arg1 ) == stsline &&
       SpatialTypeOfSymbol( arg2 ) == stsline )
    return 4;

  return -1; // This point should never be reached
}

/*
10.3.4 Selection function ~SpatialSelectIntersects~

It is used for the operator ~intersects~

*/
int
SpatialSelectIntersects( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 0;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 1;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 2;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 3;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 4;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 5;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 6;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 7;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 8;

  if ( SpatialTypeOfSymbol( arg1 ) == stsline &&
       SpatialTypeOfSymbol( arg2 ) == stsline )
    return 9;

  return -1; // This point should never be reached
}

/*
10.3.5 Selection function ~SpatialSelectInside~

This select function is used for the ~inside~ operator.

*/
int
SpatialSelectInside( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 0;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 1;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 2;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 3;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 4;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 5;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 6;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 7;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 8;

  return -1; // This point should never be reached
}

/*
10.3.6 Selection function ~SpatialSelectTopology~

This select function is used for the ~attached~ , and ~overlaps~  operator.

*/
int
SpatialSelectTopology( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 0;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 1;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 2;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 3;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 4;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 5;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 6;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 7;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 8;

  return -1; // This point should never be reached
}

/*
10.3.6 Selection function ~SpatialSelectAdjacent~

This select function is used for the ~adjacent~ operator.

*/
int
SpatialSelectAdjacent( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 0;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 1;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 2;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 3;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 4;

  return -1; // This point should never be reached
}

/*
10.3.8 Selection function ~SpatialSelectIntersection~

This select function is used for the ~intersection~ operator.

*/
int
SpatialSelectIntersection( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stpoint )
    return 0;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 1;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 2;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 3;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stpoint )
    return 4;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 5;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 6;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 7;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stpoint )
    return 8;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 9;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 10;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stpoint )
    return 11;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 12;

  return -1; // This point should never be reached
}

/*
10.3.9 Selection function ~SpatialSelectMinus~

This select function is used for the ~minus~ operator.

*/
int
SpatialSelectMinus( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stpoint )
    return 0;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 1;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 2;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 3;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stpoint )
    return 4;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 5;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 6;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 7;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stpoint )
    return 8;

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 9;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stpoint )
    return 10;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 11;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 12;

  return -1; // This point should never be reached
}

/*
10.3.10 Selection function ~SpatialSelectUnion~

This select function is used for the ~union~ operator.

*/
int
SpatialSelectUnion( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );

  if ( SpatialTypeOfSymbol( arg1 ) == stpoint &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 0;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stpoint )
    return 1;

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints &&
       SpatialTypeOfSymbol( arg2 ) == stpoints )
    return 2;

  return -1; // This point should never be reached
}

/*
10.3.13 Selection function ~SpatialSelectDistance~

This select function is used for the ~distance~ operator.

*/
int
SpatialSelectDistance( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );

  SpatialType st1 = SpatialTypeOfSymbol( arg1 );
  SpatialType st2 = SpatialTypeOfSymbol( arg2 );

  if ( st1 == stpoint && st2 == stpoint ) return 0;

  if ( st1  == stpoint && st2 == stpoints ) return 1;

  if ( st1 == stpoint && st2 == stline ) return 2;

  if ( st1 == stpoint && st2 == stregion ) return 3;

  if ( st1 == stpoints && st2 == stpoint ) return 4;

  if ( st1 == stpoints && st2 == stpoints ) return 5;

  if ( st1 == stpoints && st2 == stline ) return 6;

  if ( st1 == stpoints && st2 == stregion ) return 7;

  if ( st1 == stline && st2 == stpoint ) return 8;

  if ( st1 == stline && st2 == stpoints ) return 9;

  if ( st1 == stline && st2 == stline ) return 10;

  if ( st1 == stregion && st2 == stpoint ) return 11;

  if ( st1 == stregion && st2 == stpoints ) return 12;

  if ( st1 == stregion && st2 == stregion ) return 13;

  if( st1 == stsline && st2 == stpoint ) return 14;

  if( st1 == stpoint && st2 == stsline) return 15;

  if( st1 == stsline && st2 == stpoints) return 16;

  if( st1 == stpoints && st2 == stsline ) return 17;

  if( st1 == stsline && st2 == stsline) return  18;

  if( st1 == stbox && st2 == stpoint ) return  19;

  if( st1 == stpoint && st2 == stbox ) return  20;

  if( st1 == stbox && st2 == stpoints ) return  21;

  if( st1 == stpoints && st2 == stbox ) return  22;

  if( st1 == stbox && st2 == stline ) return  23;

  if( st1 == stline && st2 == stbox ) return  24;

  if( st1 == stbox && st2 == stregion ) return  25;

  if( st1 == stregion && st2 == stbox ) return  26;

  if( st1 == stbox && st2 == stsline ) return  27;

  if( st1 == stsline && st2 == stbox ) return  28;

  // (rect2 x rect2) has already been implemented in the RectangleAlgebra!

  return -1; // This point should never be reached
}

/*
10.3.15 Selection function ~SpatialSelectNoComponents~

This select function is used for the ~no\_components~ operator.

*/
int
SpatialSelectNoComponents( ListExpr args )
{
  ListExpr arg1 = nl->First( args );

  if (SpatialTypeOfSymbol( arg1 ) == stpoints)
    return 0;

  if (SpatialTypeOfSymbol( arg1 ) == stline)
    return 1;

  if (SpatialTypeOfSymbol( arg1 ) == stregion)
    return 2;

  return -1; // This point should never be reached
}

/*
10.3.16 Selection function ~SpatialSelectNoSegments~

This select function is used for the ~no\_segments~ operator.

*/
int
SpatialSelectNoSegments( ListExpr args )
{
  ListExpr arg1 = nl->First( args );

  if (SpatialTypeOfSymbol( arg1 ) == stline)
    return 0;

  if (SpatialTypeOfSymbol( arg1 ) == stregion)
    return 1;

  if (SpatialTypeOfSymbol( arg1 ) == stsline)
    return 2;

  return -1; // This point should never be reached
}

/*
10.3.16 Selection function ~SpatialSelectBBox~

This select function is used for the ~bbox~ operator.

*/
int
SpatialSelectBBox( ListExpr args )
{
  ListExpr arg1 = nl->First( args );

  if (SpatialTypeOfSymbol( arg1 ) == stpoint)
    return 0;

  if (SpatialTypeOfSymbol( arg1 ) == stpoints)
    return 1;

  if (SpatialTypeOfSymbol( arg1 ) == stline)
    return 2;

  if (SpatialTypeOfSymbol( arg1 ) == stregion)
    return 3;

  if (SpatialTypeOfSymbol( arg1 ) == stsline)
    return 4;

  return -1; // This point should never be reached
}

/*
10.3.17 Selection function ~SpatialSelectTouchPoints~

This select function is used for the ~touchpoints~ operator.

*/
int
SpatialSelectTouchPoints( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );

  if ( SpatialTypeOfSymbol( arg1 ) == stline &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 0;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stline )
    return 1;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion &&
       SpatialTypeOfSymbol( arg2 ) == stregion )
    return 2;

  return -1; // This point should never be reached
}

/*
10.3.17 Selection function ~SpatialComponentsSelect~

This select function is used for the ~components~ operator.

*/
int
SpatialComponentsSelect( ListExpr args )
{
  ListExpr arg1 = nl->First( args );

  if ( SpatialTypeOfSymbol( arg1 ) == stpoints )
    return 0;

  if ( SpatialTypeOfSymbol( arg1 ) == stregion )
    return 1;

  if ( SpatialTypeOfSymbol( arg1 ) == stline )
    return 2;
  return -1; // This point should never be reached
}

/*
10.3.19 Selection function ~SpatialSelectTranslate~

This select function is used for the ~translate~, rotate,
and ~scale~ operators.

*/
int
SpatialSelectTranslate( ListExpr args )
{
  ListExpr arg1 = nl->First( args );

  if (SpatialTypeOfSymbol( arg1 ) == stpoint)
    return 0;

  if (SpatialTypeOfSymbol( arg1 ) == stpoints)
    return 1;

  if (SpatialTypeOfSymbol( arg1 ) == stline)
    return 2;

  if (SpatialTypeOfSymbol( arg1 ) == stregion)
    return 3;

  return -1; // This point should never be reached
}

/*
10.3.19 Selection function ~SpatialSelectWindowClipping~

This select function is used for the ~windowclipping(in)(out)~ operators.

*/
int
SpatialSelectWindowClipping( ListExpr args )
{
  ListExpr arg1 = nl->First( args );

  if (SpatialTypeOfSymbol( arg1 ) == stline)
    return 0;

  if (SpatialTypeOfSymbol( arg1 ) == stregion)
    return 1;

  return -1; // This point should never be reached
}

/*
10.3.19 Selection function ~SpatialVerticesSelect~

This select function is used for the ~vertices~ operator.

*/
int SpatialVerticesSelect( ListExpr args )
{
  if( nl->IsEqual(nl->First(args), "line") )
    return 0;

  if( nl->IsEqual(nl->First(args), "region") )
    return 1;

  return -1; // This point should never be reached
}

/*
10.3.19 Selection function ~SpatialBoundarySelect~

This select function is used for the ~vertices~ operator.

*/
int SpatialBoundarySelect( ListExpr args )
{
  if( nl->IsEqual(nl->First(args), "region") )
    return 0;

  if( nl->IsEqual(nl->First(args), "line") )
    return 1;

  return -1; // This point should never be reached
}

/*
10.3.20 Selection function for the simplify operator

*/
int SpatialSimplifySelect(ListExpr args){
   if(nl->ListLength(args)==2){ // line x real
      return 0;
   } else {
      return 1; // line x real x points
   }

}

static int SetOpSelect(ListExpr args){
   string type1 = nl->SymbolValue(nl->First(args));
   string type2 = nl->SymbolValue(nl->Second(args));

  if( (type1=="line") && (type2=="line")){
      return 0;
   }
  if( (type1=="region") && (type2=="region")){
      return 1;
   }
   if( (type1=="region") && (type2=="line")){
      return 2;
   }
   if( (type1=="line") && (type2=="region")){
      return 3;
   }
   return -1;
}

static int SpatialSelectSize(ListExpr args){
   SpatialType st = SpatialTypeOfSymbol(nl->First(args));
   if(st==stline){
     return 0;
   }
   if(st==stregion){
     return 1;
   }
   if(st==stsline){
     return 2;
   }
   return -1;
}

static int SpatialSelectCrossings(ListExpr args){
   SpatialType st = SpatialTypeOfSymbol(nl->First(args));
   if(st==stline){
     return 0;
   }
   if(st==stsline){
     return 1;
   }
   return -1;
}


/*
10.4 Value mapping functions

A value mapping function implements an operator's main functionality: it takes
input arguments and computes the result. Each operator consists of at least
one value mapping function. In the case of overloaded operators there are
several value mapping functions, one for each possible combination of input
parameter types.

10.4.1 Value mapping functions of operator ~isempty~

*/
template<class T>
int SpatialIsEmpty(Word* args, Word& result, int message,
                   Word& local, Supplier s ){
  result = qp->ResultStorage( s );
  (static_cast<CcBool*>(result.addr))->Set(true,
              static_cast<T*>(args[0].addr)->IsEmpty());
  return 0;
}


/*
10.4.2 Value mapping functions of operator ~$=$~

*/
template<class T>
int
SpatialEqual( Word* args, Word& result, int message,
              Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool* res = static_cast<CcBool*>(result.addr);
  T* a1 = static_cast<T*>(args[0].addr);
  T* a2 = static_cast<T*>(args[1].addr);
  if(!a1->IsDefined() || !a2->IsDefined()){
     res->Set(false,false);
     return 0;
  }
  bool e = (*a1) == (*a2);
  res->Set(true,e);
  return 0;
}


/*
10.4.3 Value mapping functions of operator ~$\neq$~

*/
template<class T>
int
SpatialNotEqual( Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  T* a1 = static_cast<T*>(args[0].addr);
  T* a2 = static_cast<T*>(args[1].addr);
  CcBool* res = static_cast<CcBool*>(result.addr);
  if(!a1->IsDefined() || !a2->IsDefined()){
     res->Set(false,false);
     return 0;
  }
  res->Set(true, (*a1) != (*a2));
  return 0;
}

/*
10.4.8 Value mapping functions of operator ~intersects~

*/
template<class A, class B,bool symm>
int SpatialIntersectsVM( Word* args, Word& result, int message,
                       Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool* res = static_cast<CcBool*>(result.addr);
  A* a;
  B* b;
  if(symm){
    a = static_cast<A*>(args[1].addr);
    b = static_cast<B*>(args[0].addr);
  } else {
    a = static_cast<A*>(args[0].addr);
    b = static_cast<B*>(args[1].addr);
  }

  if(!a->IsDefined() || !b->IsDefined()){
     res->Set(false,false);
  } else {
     res->Set(true,a->Intersects(*b));
  }
  return 0;
}


/*
10.4.9 Value mapping functions of operator ~inside~

*/
int
SpatialInside_pps( Word* args, Word& result, int message,
                   Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = ((Point*)args[0].addr);
  if ( p->IsDefined() )
    ((CcBool *)result.addr)->
      Set( true, ((Points*)args[1].addr)->Contains( *p ) );
  else
    ((CcBool *)result.addr)->Set( false, false );
  return 0;
}

int
SpatialInside_pl( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = ((Point*)args[0].addr);
  if( p->IsDefined() )
    ((CcBool *)result.addr)->Set( true, ((Line*)args[1].addr)->Contains( *p ) );
  else
    ((CcBool *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialInside_pr( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = ((Point*)args[0].addr);
  if( p->IsDefined() )
    ((CcBool *)result.addr)->
      Set( true, ((Region*)args[1].addr)->Contains( *p ) );
  else
    ((CcBool *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialInside_psps( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true,
         ((Points*)args[0].addr)->Inside( *((Points*)args[1].addr) ) );
  return 0;
}

int
SpatialInside_psl( Word* args, Word& result, int message,
                   Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true,
         ((Points*)args[0].addr)->Inside( *((Line*)args[1].addr) ) );
  return 0;
}

int
SpatialInside_psr( Word* args, Word& result, int message,
                   Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true,
         ((Points*)args[0].addr)->Inside( *((Region*)args[1].addr) ) );
  return 0;
}

int
SpatialInside_ll( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true,
         ((Line*)args[0].addr)->Inside( *((Line*)args[1].addr) ) );
  return 0;
}

int
SpatialInside_lr( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true,
         ((Line*)args[0].addr)->Inside( *((Region*)args[1].addr) ) );
  return 0;
}

int
SpatialInside_rr( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true,
         ((Region*)args[0].addr)->Inside( *((Region*)args[1].addr) ) );
  return 0;
}

/*
10.4.10 Value mapping functions of operator ~adjacent~

*/
int
SpatialAdjacent_psr( Word* args, Word& result, int message,
                     Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true,
         ((Points*)args[0].addr)->Adjacent( *((Region*)args[1].addr) ) );
  return 0;
}

int
SpatialAdjacent_rps( Word* args, Word& result, int message,
                     Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true,
         ((Points*)args[1].addr)->Adjacent( *((Region*)args[0].addr) ) );
  return 0;
}

int
SpatialAdjacent_lr( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true,
         ((Line*)args[0].addr)->Adjacent( *((Region*)args[1].addr) ) );
  return 0;
}

int
SpatialAdjacent_rl( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true,
         ((Line*)args[1].addr)->Adjacent( *((Region*)args[0].addr) ) );
  return 0;
}

int
SpatialAdjacent_rr( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true,
         ((Region*)args[0].addr)->Adjacent( *((Region*)args[1].addr) ) );
  return 0;
}

/*
10.4.12 Value mapping functions of operator ~overlaps~

*/
int
SpatialOverlaps_rr( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true,
         ((Region*)args[0].addr)->Overlaps( *((Region*)args[1].addr) ) );
  return 0;
}

/*
10.4.13 Value mapping functions of operator ~onborder~

*/
int
SpatialOnBorder_pr( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = (Point*)args[0].addr;

  if( p->IsDefined() )
    ((CcBool *)result.addr)->
      Set( true,
           ((Region*)args[1].addr)->OnBorder( *((Point*)args[0].addr) ) );
  else
    ((CcBool *)result.addr)->Set( false, false );

  return 0;
}

/*
10.4.14 Value mapping functions of operator ~ininterior~

*/
int
SpatialInInterior_pr( Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = (Point*)args[0].addr;

  if( p->IsDefined() )
    ((CcBool *)result.addr)->
      Set( true,
           ((Region*)args[1].addr)->InInterior( *((Point*)args[0].addr) ) );
  else
    ((CcBool *)result.addr)->Set( false, false );

  return 0;
}

/*
10.4.15 Value mapping functions of operator ~intersection~

*/
int
SpatialIntersection_pp( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p1 = ((Point*)args[0].addr);
  Point *p2 = ((Point*)args[1].addr);
  if ( p1->IsDefined() && p2->IsDefined() && *p1 == *p2 )
    *((Point *)result.addr) = *p1;
  else
    ((Point *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialIntersection_pps( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = (Point*)args[0].addr;
  if( p->IsDefined() && ((Points*)args[1].addr)->Contains( *p ) )
    *((Point *)result.addr) = *p;
  else
    ((Point *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialIntersection_psp( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = (Point*)args[1].addr;
  if( p->IsDefined() && ((Points*)args[0].addr)->Contains(*p) )
    *((Point *)result.addr) = *p;
  else
    ((Point *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialIntersection_pl( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = ((Point*)args[0].addr);
  if( p->IsDefined() && ((Line*)args[1].addr)->Contains( *p ) )
    *((Point *)result.addr) = *p;
  else
    ((Point *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialIntersection_lp( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = ((Point*)args[1].addr);
  if( p->IsDefined() && ((Line*)args[0].addr)->Contains( *p ) )
    *((Point *)result.addr) = *p;
  else
    ((Point *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialIntersection_pr( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p=((Point*)args[0].addr);
  if( p->IsDefined() && ((Region*)args[1].addr)->Contains( *p ) )
    *((Point *)result.addr) = *p;
  else
    ((Point *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialIntersection_rp( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p=((Point*)args[1].addr);
  if( p->IsDefined() && ((Region*)args[0].addr)->Contains( *p ) )
    *((Point *)result.addr) = *p;
  else
    ((Point *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialIntersection_psps( Word* args, Word& result, int message,
                          Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Points *ps1 = ((Points*)args[0].addr),
         *ps2 = ((Points*)args[1].addr);
  ps1->Intersection( *ps2, *((Points *)result.addr) );
  return 0;
}

int
SpatialIntersection_psl( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Points *ps = ((Points*)args[0].addr);
  Line *l = ((Line*)args[1].addr);
  ps->Intersection( *l, *((Points *)result.addr) );
  return 0;
}

int
SpatialIntersection_lps( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Points *ps = ((Points*)args[1].addr);
  Line *l = ((Line*)args[0].addr);
  ps->Intersection( *l, *((Points *)result.addr) );
  return 0;
}

int
SpatialIntersection_psr( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((Points*)args[0].addr)->
    Intersection( *((Region*)args[1].addr), *((Points *)result.addr) );
  return 0;
}

int
SpatialIntersection_rps( Word* args, Word& result, int message,
                         Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((Points*)args[1].addr)->
    Intersection( *((Region*)args[0].addr), *((Points *)result.addr) );
  return 0;
}

int
SpatialIntersection_ll( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((Line*)args[0].addr)->
    Intersection( *((Line*)args[1].addr), *((Line *)result.addr) );
  return 0;
}

/*
10.4.16 Value mapping functions of operator ~minus~

*/
int
SpatialMinus_pp( Word* args, Word& result, int message,
                 Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p1 = (Point*)args[0].addr,
        *p2 = (Point*)args[1].addr;
  if( p1->IsDefined() && p2->IsDefined() && *p1 == *p2 )
    *((Point *)result.addr) = *p1;
  else
    ((Point *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialMinus_pps( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = (Point*)args[0].addr;
  if( p->IsDefined() && ((Points*)args[1].addr)->Contains( *p ) )
    *((Point *)result.addr) = *p;
  else
    ((Point *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialMinus_pl( Word* args, Word& result, int message,
                 Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = (Point*)args[0].addr;
  if( p->IsDefined() && ((Line*)args[1].addr)->Contains( *p ) )
    *((Point *)result.addr) = *p;
  else
    ((Point *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialMinus_pr( Word* args, Word& result, int message,
                 Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = (Point*)args[0].addr;
  if( p->IsDefined() && ((Region*)args[1].addr)->Contains( *p ) )
    *((Point *)result.addr) = *p;
  else
    ((Point *)result.addr)->SetDefined( false );
  return 0;
}

int
SpatialMinus_psp( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p = (Point*)args[1].addr;
  if( p->IsDefined() )
    ((Points*)args[0].addr)->Minus( *p, *((Points *)result.addr) );
  else
    *((Points *)result.addr) = *((Points*)args[0].addr);
  return 0;
}

int
SpatialMinus_lp( Word* args, Word& result, int message,
                 Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  *((Line*)result.addr) = *((Line*)args[0].addr);
  return 0;
}

int
SpatialMinus_rp( Word* args, Word& result, int message,
                 Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  *(Region*)result.addr = *(Region *)args[0].addr;
  return 0;
}

int
SpatialMinus_psps( Word* args, Word& result, int message,
                   Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Points *ps1 = ((Points*)args[0].addr),
         *ps2=((Points*)args[1].addr);

  ps1->Minus( *ps2, *((Points *)result.addr) );

  return 0;
}

int
SpatialMinus_psl( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Points *ps = ((Points*)args[0].addr);
  Line *l = ((Line*)args[1].addr);

  ps->Minus( *l, *((Points *)result.addr) );

  return 0;
}

int
SpatialMinus_psr( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Points *ps = ((Points*)args[0].addr);
  Region *r = ((Region*)args[1].addr);

  ps->Minus( *r, *((Points *)result.addr) );

  return 0;
}

int
SpatialMinus_lps( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  *(Line*)result.addr = *(Line*)args[0].addr;
  return 0;
}

int
SpatialMinus_rps( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  *(Region*)result.addr = *(Region *)args[0].addr;
  return 0;
}

int
SpatialMinus_rl( Word* args, Word& result, int message,
                 Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  *(Region*)result.addr = *(Region *)args[0].addr;
  return 0;
}

/*
10.4.17 Value mapping functions of operator ~union~

*/
int
SpatialUnion_pps( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Point *p = ((Point*)args[0].addr);
  Points *ps = ((Points*)args[1].addr);

  if( p->IsDefined() )
    ps->Union( *p, *((Points *)result.addr) );
  else
    *((Points *)result.addr) = *ps;

  return 0;
}

int
SpatialUnion_psp( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Point *p = ((Point*)args[1].addr);
  Points *ps = ((Points*)args[0].addr);

  if( p->IsDefined() )
    ps->Union( *p, *((Points *)result.addr) );
  else
    *((Points *)result.addr) = *ps;

  return 0;
}

int
SpatialUnion_psps( Word* args, Word& result, int message,
                   Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Points *ps1 = ((Points*)args[1].addr),
         *ps2 = ((Points*)args[0].addr);

  ps1->Union( *ps2, *((Points *)result.addr) );

  return 0;
}

/*
10.4.18 Value mapping functions of operator ~crossings~

*/
template<class Ltype>
int
SpatialCrossings( Word* args, Word& result, int message,
                     Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Ltype *cl1=((Ltype*)args[0].addr),
       *cl2=((Ltype*)args[1].addr);
  cl1->Crossings( *cl2, *(Points*)result.addr );
  return 0;
}

/*
10.4.19 Value mapping functions of operator ~single~

*/
int
SpatialSingle_ps( Word* args, Word& result, int message,
                  Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Points *ps = ((Points*)args[0].addr);

  const Point *p;
  if( ps->Size() == 1 )
  {
    ps->Get( 0, p );
    *(Point*)result.addr = *p;
  }
  else
    ((Point *)result.addr)->SetDefined( false );

  return 0;
}

/*
10.4.20 Value mapping functions of operator ~distance~

*/
template<class A, class B,bool symm>
int SpatialDistance( Word* args, Word& result, int message,
                     Word& local, Supplier s ){

   result = qp->ResultStorage( s );
   CcReal* res = static_cast<CcReal*>(result.addr);
   A* arg1=0;
   B* arg2=0;
   if(symm){
     arg1 = static_cast<A*>(args[1].addr);
     arg2 = static_cast<B*>(args[0].addr);
   } else {
     arg1 = static_cast<A*>(args[0].addr);
     arg2 = static_cast<B*>(args[1].addr);
   }
   if(!arg1->IsDefined() || !arg2->IsDefined() ||
      arg1->IsEmpty() || arg2->IsEmpty()){
      res->SetDefined(false);
   } else {
     double dist = arg1->Distance(*arg2);
     res->Set(true,dist);
   }
   return 0;
}


/*
10.4.21 Value mapping functions of operator ~direction~

*/
int
SpatialDirection_pp( Word* args, Word& result, int message,
                     Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Point *p1 = ((Point*)args[0].addr),
        *p2 = ((Point*)args[1].addr);

  if( p1->IsDefined() && p2->IsDefined() && !AlmostEqual( *p1, *p2 ) )
    ((CcReal *)result.addr)->Set( true, p1->Direction( *p2 ) );
  else
    ((CcReal *)result.addr)->SetDefined( false );

  return 0;
}

/*
10.4.22 Value mapping functions of operator ~nocomponents~

*/

int
SpatialNoComponents_ps( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcInt *)result.addr)->Set( true, ((Points*)args[0].addr)->Size() );
  return 0;
}

int
SpatialNoComponents_l( Word* args, Word& result, int message,
                       Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcInt *)result.addr)->Set( true, ((Line*)args[0].addr)->NoComponents() );
  return 0;
}

int
SpatialNoComponents_r( Word* args, Word& result, int message,
                       Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcInt *)result.addr)->Set( true, ((Region*)args[0].addr)->NoComponents() );
  return 0;
}

/*
10.4.22 Value mapping functions of operator ~no\_segments~

*/
template<class T>
int
SpatialNoSegments( Word* args, Word& result, int message,
                     Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  T *cl=((T*)args[0].addr);
  assert( cl->Size() % 2 == 0 );
  ((CcInt *)result.addr)->Set( true, cl->Size() / 2 );
  return 0;
}


/*
10.4.22 Value mapping functions of operator ~bbox~

*/
template<class T>
int SpatialBBox(Word* args, Word& result, int message,
            Word& local, Supplier s ){

  result = qp->ResultStorage( s );
  Rectangle<2>* box = static_cast<Rectangle<2>* >(result.addr);
  T* arg = static_cast<T*>(args[0].addr);
  if(!arg->IsDefined()){
    box->SetDefined(false);
  } else {
    (*box) = arg->BoundingBox();
  }
  return 0;
}


/*
10.4.23 Value mapping functions of operator ~size~

*/
template<class T>
int
SpatialSize( Word* args, Word& result, int message,
               Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcReal* res = static_cast<CcReal*>(result.addr);
  T* a = (static_cast<T*>(args[0].addr));
  if(!a->IsDefined()){
    res->SetDefined(false);
  } else {
    res->Set(true,a->SpatialSize());
  }
  return 0;
}

/*
10.4.24 Value mapping functions of operator ~touchpoints~

*/
int
SpatialTouchPoints_lr( Word* args, Word& result, int message,
                       Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Line *l = ((Line*)args[0].addr);
  Region *r = ((Region*)args[1].addr);
  r->TouchPoints( *l, *((Points *)result.addr) );
  return 0;
}

int
SpatialTouchPoints_rl( Word* args, Word& result, int message,
                       Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Line *l = ((Line*)args[1].addr);
  Region *r = ((Region*)args[0].addr);
  r->TouchPoints( *l, *((Points *)result.addr) );
  return 0;
}

int
SpatialTouchPoints_rr( Word* args, Word& result, int message,
                       Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Region *r1 = ((Region*)args[0].addr),
         *r2 = ((Region*)args[1].addr);
  r1->TouchPoints( *r2, *((Points *)result.addr) );
  return 0;
}

/*
Classes suppoorting the computation of the convex hull of
an pointset.


*/
class SimplePoint;
ostream& operator<<(ostream& o,const SimplePoint& p);

class SimplePoint{
  public:
     SimplePoint(const Point* p){
       this->x = p->GetX();
       this->y = p->GetY();
     }

     SimplePoint(){
        x = 0;
        y = 0;
     }

     SimplePoint(double x, double y){
       this->x = x;
       this->y = y;
     }

     SimplePoint(const SimplePoint& p){
        this->x = p.x;
        this->y = p.y;
     }

     SimplePoint& operator=(const SimplePoint& p){
       this->x = p.x;
       this->y = p.y;
       return *this;
     }

     ~SimplePoint(){}

     SimplePoint relTo(const SimplePoint& p) const{
        return SimplePoint(this->x - p.x, this->y-p.y);
     }

     void makeRelTo(const SimplePoint& p){
        this->x -= p.x;
        this->y -= p.y;
     }

     SimplePoint moved(const double x0, const double y0)const{
        return SimplePoint(x+x0, y+y0);
     }

     SimplePoint reversed()const{
        return SimplePoint(-x,-y);
     }

     bool isLower(const SimplePoint& p)const{
        if(!AlmostEqual(y,p.y)){
           return y < p.y;
        }
        if(AlmostEqual(x,p.x)){ // equal points
           return false;
        }
        return x < p.x;
     }

     double mdist()const{ // manhatten distance to (0,0)
       return abs(x) + abs(y);
     }

     double mdist(const SimplePoint p)const{
        return abs(x-p.x) + abs(y-p.y);
     }

     bool isFurther(const SimplePoint& p)const{
        return mdist() > p.mdist();
     }

     bool isBetween(const SimplePoint& p0, const SimplePoint p1) const{
        return p0.mdist(p1) >= mdist(p0)+mdist(p1);
     }

     double cross(const SimplePoint& p)const{
        return x*p.y - p.x*y;
     }

     bool isLess(const SimplePoint& p) const{
        double f = cross(p);
        bool res;
        if(AlmostEqual(f,0.0)){
          res = isFurther(p);
        } else {
          res = f>0;
        }
        return  res;
     }

     bool operator<(const SimplePoint& p) const{
         return isLess(p);
     }

     bool operator==(const SimplePoint& p) const{
         return AlmostEqual(x,p.x)&& AlmostEqual(y,p.y);
     }

    bool operator>(const SimplePoint& p) const{
         return !(AlmostEqual(x,p.x) && AlmostEqual(y,p.y)) && !isLess(p);
     }

     double area2(const SimplePoint& p0, const SimplePoint& p1) const{
        return p0.relTo(*this).cross(p1.relTo(*this));
     }

     bool isConvex(const SimplePoint& p0, const SimplePoint& p1) const {
        double f = area2(p0,p1);
        if(AlmostEqual(f,0.0)){
           bool between = isBetween(p0,p1);
           return !between;
        }
        return f<0;
     }

     Point getPoint()const {
        return Point(true,x,y);
     }

     double getX()const{ return x;}
     double getY()const{ return y;}


  private:
     double x;
     double y;

}; // end of class SimplePoint



ostream& operator<<(ostream& o,const SimplePoint& p) {
        o << "(" << p.getX() << ", " << p.getY() << ")";
        return o;
}


class GrahamScan{
public:
  static void convexHull(const Points* ps, Region* result){
     result->Clear();
     if(!ps->IsDefined() ){
         result->SetDefined(false);
         return;
     }
     if(ps->Size()<3){
        result->SetDefined(false);
        return;
     }
     GrahamScan scan(ps);
     int size = scan.computeHull();
     if(size<3){ // points was on a single line
        result->SetDefined(false);
        return;
     }


     result->SetDefined(true);
     result->StartBulkLoad();
     for(int i=0;i<size-1; i++){
        SimplePoint p1(scan.p[i]);
        SimplePoint p2(scan.p[i+1]);
        // build the halfsegment
        HalfSegment hs1(true,p1.getPoint(),p2.getPoint());
        HalfSegment hs2(false,p1.getPoint(),p2.getPoint());
        hs1.attr.edgeno = i;
        hs2.attr.edgeno = i;
        bool ia = isInsideAbove(p1,p2);
        hs1.attr.insideAbove = ia;
        hs2.attr.insideAbove = ia;
        (*result) += hs1;
        (*result) += hs2;
     }
     // close the polygon
     SimplePoint p1(scan.p[size-1]);
     SimplePoint p2(scan.p[0]);
     // build the halfsegment
     HalfSegment hs1(true,p1.getPoint(),p2.getPoint());
     HalfSegment hs2(false,p1.getPoint(),p2.getPoint());
     hs1.attr.edgeno = size-1;
     hs2.attr.edgeno = size-1;
     bool ia = isInsideAbove(p1,p2);
     hs1.attr.insideAbove = ia;
     hs2.attr.insideAbove = ia;
     (*result) += hs1;
     (*result) += hs2;
     result->EndBulkLoad();
  }


private:
   vector<SimplePoint> p;
   int n;
   int h;


  GrahamScan(const Points* ps){
     n = ps->Size();
     const Point* pt;
     for(int i=0;i<n;i++){
        ps->Get(i,pt);
        p.push_back(SimplePoint(pt));
     }
  }

   int computeHull(){
    if(n<3){
      return n;
    }
    h = 0;
    grahamScan();
    return h;
  }
   void grahamScan(){
     int min = indexOfLowestPoint();

     exchange(0,min);



     SimplePoint pl(p[0]);
     makeRelTo(pl);
     sort();
     makeRelTo(pl.reversed());

     int i=3;
     int k=3;
     while(k<n){
        exchange(i,k);
        while(!isConvex(i-1)){
           exchange(i-1,i);
           i--;
        }
        k++;
        i++;
     }
     // remove a possibly last 180 degree angle
     if(i>=3){
        if(!p[i-1].isConvex(p[i-2],p[0])){
            i--;
        }
     }

     h = i;
   }

   static bool isInsideAbove(const SimplePoint& p1, const SimplePoint& p2){
     double diffx = p2.getX()-p1.getX();
     double diffy = p2.getY()-p1.getY();

     if(AlmostEqual(diffx,0.0)){
        return diffy < 0;
     }
     if(AlmostEqual(diffy,0.0)){
        return diffx > 0;
     }

     bool sx = diffx>0;
    // bool sy = diffy>0;
    // return sx == sy;
    return sx;
   }


   void exchange(const int i, const int j){
      SimplePoint t(p[i]);
      p[i] = p[j];
      p[j] = t;
   }

   void makeRelTo(const SimplePoint& p0){
       SimplePoint p1(p0);
       for(int i=0;i<n;i++){
          p[i].makeRelTo(p1);
       }
   }

   int indexOfLowestPoint()const{
     unsigned min = 0;
     for(unsigned int i=1; i<p.size(); i++){
        if(p[i].isLower(p[min])){
           min = i;
        }
     }
     return min;
   }

   bool isConvex(const int i){
     return p[i].isConvex(p[i-1],p[i+1]);
   }

   void sort(){
     vector<SimplePoint>::iterator it= p.begin();
     it++;
     std::sort(it,p.end()); // without the first point
   }


}; // end of class GrahamScan



/*
10.4.25 Value mapping functions of operator ~commomborder~

*/

int
SpatialCommonBorder_rr( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Line *l = (Line*)result.addr;

  Region *cr1 = ((Region*)args[0].addr),
         *cr2 = ((Region*)args[1].addr);

  cr1->CommonBorder( *cr2, *l );

  return 0;
}

/*
10.4.26 Value mapping functions of operator ~translate~

*/
int
SpatialTranslate_p( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  const Point *p= (Point*)args[0].addr;

  Supplier son = qp->GetSupplier( args[1].addr, 0 );
  Word t;
  qp->Request( son, t );
  const CcReal *tx = ((CcReal *)t.addr);

  son = qp->GetSupplier( args[1].addr, 1 );
  qp->Request( son, t );
  const CcReal *ty = ((CcReal *)t.addr);

  if( p->IsDefined())
    if( tx->IsDefined() && ty->IsDefined() )
      *((Point*)result.addr) = p->Translate( tx->GetRealval(),
                                             ty->GetRealval() );
    else
      *((Point*)result.addr) = *p;
  else
    ((Point*)result.addr)->SetDefined( false );

  return 0;
}

int
SpatialTranslate_ps( Word* args, Word& result, int message,
                     Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  const Points *ps = (Points*)args[0].addr;

  Supplier son = qp->GetSupplier( args[1].addr, 0 );
  Word t;
  qp->Request( son, t );
  const CcReal *tx = ((CcReal *)t.addr);

  son = qp->GetSupplier( args[1].addr, 1 );
  qp->Request( son, t );
  const CcReal *ty = ((CcReal *)t.addr);

  if( ps->IsDefined() )
    if( tx->IsDefined() && ty->IsDefined() )
      ps->Translate( tx->GetRealval(),
                     ty->GetRealval(),
                     *((Points*)result.addr) );
    else
      *((Points*)result.addr) = *ps;
  else
    ((Points*)result.addr)->SetDefined( false );

  return 0;
}

int
SpatialTranslate_l( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Line *cl = (Line *)args[0].addr,
        *pResult = (Line *)result.addr;

  Supplier son = qp->GetSupplier( args[1].addr, 0 );
  Word t;
  qp->Request( son, t );
  const CcReal *tx = ((CcReal *)t.addr);

  son = qp->GetSupplier( args[1].addr, 1 );
  qp->Request( son, t );
  const CcReal *ty = ((CcReal *)t.addr);

  if(  !cl->IsEmpty() )
    if( tx->IsDefined() && ty->IsDefined() ) {
      const Coord txval = (Coord)(tx->GetRealval()),
                  tyval = (Coord)(ty->GetRealval());
      cl->Translate( txval, tyval, *pResult );
    }
    else
      *((Line*)result.addr) = *cl;
  else
    ((Line*)result.addr)->SetDefined( false );

  return 0;
}

template<class T>
int SpatialRotate( Word* args, Word& result, int message,
                    Word& local, Supplier s ){
  result = qp->ResultStorage(s);
  T* res = static_cast<T*>(result.addr);
  T* st = static_cast<T*>(args[0].addr);
  CcReal* x = static_cast<CcReal*>(args[1].addr);
  CcReal* y = static_cast<CcReal*>(args[2].addr);
  CcReal* a = static_cast<CcReal*>(args[3].addr);
  if(!st->IsDefined() || !x->IsDefined() || !y->IsDefined()
     || !a->IsDefined()){
      res->SetDefined(false);
      return 0;
  }
  double angle = a->GetRealval() * PI / 180;

  st->Rotate(x->GetRealval(),y->GetRealval(),angle,*res);
  return 0;
}


int SpatialCenter( Word* args, Word& result, int message,
                   Word& local, Supplier s ){
   result = qp->ResultStorage(s);
   Points* ps = static_cast<Points*>(args[0].addr);
   Point* res = static_cast<Point*>(result.addr);

   *res = ps->theCenter();
   return 0;
}

int SpatialConvexhull( Word* args, Word& result, int message,
                   Word& local, Supplier s ){
   result = qp->ResultStorage(s);
   Points* ps = static_cast<Points*>(args[0].addr);
   Region* res = static_cast<Region*>(result.addr);
   GrahamScan::convexHull(ps,res);
   return 0;
}

int
SpatialLine2Region( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Line *cl = (Line *)args[0].addr;
  Region *pResult = (Region *)result.addr;

  pResult->Clear();

  if(  !cl->IsEmpty() )
    cl->Transform( *pResult );

  else
    ((Region*)result.addr)->SetDefined( false );

  return 0;
}

/*
10.4.29 Value mapping function of operator ~rect2region~

*/

int SpatialRect2Region( Word* args, Word& result, int message,
                        Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Rectangle<2> *rect = (Rectangle<2> *)args[0].addr;
  Region *res = (Region *)result.addr;
  res->SetDefined(true);
  res->Clear();
  if(  rect->IsDefined() )
  *res = Region( *rect );
  return 0;
}

int SpatialArea( Word* args, Word& result, int message,
                 Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Region *reg = (Region *)args[0].addr;
  CcReal *res = (CcReal *)result.addr;

  if(  reg->IsDefined() )
  {
    res->Set( true, reg->Area() );
  }
  else
    res->Set( false, 0.0 );

  return 0;
}

int
SpatialTranslate_r( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Region *cr = (Region *)args[0].addr,
          *pResult = (Region *)result.addr;

  pResult->Clear();


  Supplier son = qp->GetSupplier( args[1].addr, 0 );
  Word t;
  qp->Request( son, t );
  const CcReal *tx = ((CcReal *)t.addr);

  son = qp->GetSupplier( args[1].addr, 1 );
  qp->Request( son, t );
  const CcReal *ty = ((CcReal *)t.addr);

  if(  !cr->IsEmpty() )
    if( tx->IsDefined() && ty->IsDefined() ) {
      const Coord txval = (Coord)(tx->GetRealval()),
                  tyval = (Coord)(ty->GetRealval());
      cr->Translate( txval, tyval, *pResult );
    }
    else
      *((Region*)result.addr) = *cr;
  else
    ((Region*)result.addr)->SetDefined( false );

  return 0;
}

/*
10.4.26 Value mapping functions of operator ~windowclippingin~

*/
int
SpatialWindowClippingIn_l( Word* args, Word& result, int message,
                           Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Line *l = ((Line *)args[0].addr);
  Line *clippedLine = (Line*)result.addr;
  clippedLine->Clear();
  bool inside;
  Rectangle<2> *window = ((Rectangle<2>*)args[1].addr);

  l->WindowClippingIn(*window,*clippedLine,inside);

  return 0;
}

int
SpatialWindowClippingIn_r( Word* args, Word& result, int message,
                           Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Region *r = ((Region *)args[0].addr);
  Region *clippedRegion=(Region*)result.addr;
  clippedRegion->Clear();

  Rectangle<2> *window = ((Rectangle<2>*)args[1].addr);

  r->WindowClippingIn(*window,*clippedRegion);

  return 0;

}

/*
10.4.26 Value mapping functions of operator ~windowclippingout~

*/
int
SpatialWindowClippingOut_l( Word* args, Word& result, int message,
                            Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Line *l = ((Line *)args[0].addr);
  Line *clippedLine = (Line*)result.addr;
  clippedLine->Clear();
  bool outside;
  Rectangle<2> *window = ((Rectangle<2>*)args[1].addr);

  l->WindowClippingOut(*window,*clippedLine,outside);
  return 0;
}

int
SpatialWindowClippingOut_r( Word* args, Word& result, int message,
                            Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  Region *r = ((Region *)args[0].addr);
  Region *clippedRegion=(Region*)result.addr;
  clippedRegion->Clear();

  Rectangle<2> *window = ((Rectangle<2>*)args[1].addr);

  r->WindowClippingOut(*window,*clippedRegion);

  return 0;
}

/*
10.4.27 Value Mapping functions of the Operator Scale

*/
int SpatialScale_p( Word* args, Word& result, int message,
                    Word& local, Supplier s ){
  result = qp->ResultStorage(s);
  Point* p = (Point*) args[0].addr;
  CcReal*  factor = (CcReal*) args[1].addr;
  Point* res = (Point*) result.addr;
  if ( !p->IsDefined() || !factor->IsDefined() )
  {
    res->SetDefined(false);
  }
  else
  {
    res->SetDefined(true);
    res->Set(p->GetX(),p->GetY());
    double f = factor->GetRealval();
    res->Scale(f);
  }
  return 0;
}

int SpatialScale_ps( Word* args, Word& result, int message,
                     Word& local, Supplier s ){
  result = qp->ResultStorage(s);
  Points* p = (Points*) args[0].addr;
  CcReal*  factor = (CcReal*) args[1].addr;
  Points* res = (Points*) result.addr;
  if( !p->IsDefined() || !factor->IsDefined() )
  {
    res->SetDefined(false);
  }
  else
  {
    res->SetDefined(true);
    double f = factor->GetRealval();
    // make res empty if it is not already
    if(!res->IsEmpty()){
       Points P(0);
       (*res) = P;
    }
    if(!p->IsEmpty()){
       res->StartBulkLoad();
       int size = p->Size();
       const Point *PTemp;
       for(int i=0;i<size;i++){
           p->Get(i,PTemp);
           Point aux( *PTemp );
           aux.Scale(f);
           (*res) += aux;
        }
        res->EndBulkLoad();
    }
  }
  return 0;
}

int SpatialScale_l( Word* args, Word& result, int message,
                    Word& local, Supplier s ){
  result = qp->ResultStorage(s);
  Line* L = (Line*) args[0].addr;
  CcReal* factor = (CcReal*) args[1].addr;
  Line* res = (Line*) result.addr;
  if( !L->IsDefined() || !factor->IsDefined() )
  {
    res->SetDefined(false);
  }
  else
  {
    res->SetDefined(true);
    double f = factor->GetRealval();
    // delete result if not empty
    if(!res->IsEmpty()){
       Line Lempty(0);
       (*res) = Lempty;
    }
    if(!L->IsEmpty()){
       res->StartBulkLoad();
       int size = L->Size();
       const HalfSegment *hs;
       for(int i=0;i<size;i++){
         L->Get(i,hs);
         HalfSegment aux( *hs );
         aux.Scale(f);
         (*res) += aux;
       }
       res->EndBulkLoad();
    }
  }
  return 0;
}

int SpatialScale_r( Word* args, Word& result, int message,
                    Word& local, Supplier s ){
  result = qp->ResultStorage(s);
  Region* R = (Region*) args[0].addr;
  CcReal* factor = (CcReal*) args[1].addr;
  Region* res = (Region*) result.addr;
  if( !R->IsDefined() || !factor->IsDefined() )
  {
    res->SetDefined(false);
  }
  else
  {
    res->SetDefined(true);
    double f = factor->GetRealval();
    // delete result if not empty
    if(!res->IsEmpty()){
       Region Rempty(0);
       (*res) = Rempty;
    }
    if(!R->IsEmpty()){
       res->StartBulkLoad();
       int size = R->Size();
       const HalfSegment *hs;
       for(int i=0;i<size;i++){
         R->Get(i,hs);
         HalfSegment aux( *hs );
         aux.Scale(f);
         (*res) += aux;
       }
      res->EndBulkLoad();
    }
  }
  return 0;
}

/*
10.4.27 Value mapping functions of operator ~components~

*/
struct ComponentsLocalInfo
{
  vector<Region*> components;
  vector<Region*>::iterator iter;
};

int
SpatialComponents_r( Word* args, Word& result, int message,
                     Word& local, Supplier s )
{
  ComponentsLocalInfo *localInfo;

  switch( message )
  {
    case OPEN:
      localInfo = new ComponentsLocalInfo();
      ((Region*)args[0].addr)->Components( localInfo->components );
      localInfo->iter = localInfo->components.begin();
      local.setAddr( localInfo );
      return 0;

    case REQUEST:

      localInfo = (ComponentsLocalInfo*)local.addr;
      if( localInfo->iter == localInfo->components.end() )
        return CANCEL;
      result.setAddr( *localInfo->iter++ );
      return YIELD;

    case CLOSE:

      if(local.addr)
      {
        localInfo = (ComponentsLocalInfo*)local.addr;
        while( localInfo->iter != localInfo->components.end() )
        {
          delete *localInfo->iter++;
        }
        delete localInfo;
        local.setAddr(0);
      }
      return 0;
  }
  return 0;
}

int
SpatialComponents_ps( Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  Points *localInfo;

  switch( message )
  {
    case OPEN:
      localInfo = new Points(*((Points*)args[0].addr));
      localInfo->SelectFirst();
      local.setAddr(localInfo);
      return 0;

    case REQUEST:
    {
      localInfo = (Points*)local.addr;
      if(localInfo->EndOfPt() ){
        return CANCEL;}

      const Point *p;
      localInfo->GetPt( p );
      result.addr = new Point( *p );
      localInfo->SelectNext();
      local.setAddr(localInfo);
      return YIELD;
    }

    case CLOSE:
      if(local.addr)
      {
        localInfo = (Points*)local.addr;
        delete localInfo;
        local.setAddr(0);
      }
      return 0;
  }
  return 0;
}


class LineComponentsLi{
public:
  LineComponentsLi(Line* theLine){
    this->theLine = (Line*)theLine->Copy();
    size = theLine->Size();
    used = new bool[size];
    for(int i=0;i<size;i++){
       used[i] = false;
    }
    pos = 0;
  }

  ~LineComponentsLi(){
     theLine->DeleteIfAllowed();
     delete[] used;
   }

  Line* NextLine(){
     // search next unused part
     while(pos<size && used[pos]){
         pos++;
     }
     // all segments are used already
     if(pos>=size){
        return 0;
     }
     Line* result = new Line(size-pos);
     result->StartBulkLoad();
     int edgeno = 0;
     // pos points to an unused segments
     stack<int> criticalPoints;
     bool done=false;
     const HalfSegment* hs=0;
     int hspos = pos;
     const HalfSegment* hsp=0;
     int hsppos = -1;
     criticalPoints.push(pos); // mark to search an extension here
     while(!done){
        theLine->Get(hspos,hs);
        hsppos = hs->attr.partnerno;
        theLine->Get(hsppos,hsp);
        used[hspos]=true;
        used[hsppos]=true;
        HalfSegment hs1 = *hs;
        hs1.attr.edgeno = edgeno;
        hs1.SetLeftDomPoint(false);
        (*result) += hs1;
        hs1.SetLeftDomPoint(true);
        (*result) += hs1;
        edgeno++;
        // search an extension of result
        Point p = hsp->GetDomPoint();
        criticalPoints.push(hsppos);

        // search within the stack
        bool found = false;
        while(!criticalPoints.empty() && !found){
          int k = criticalPoints.top();
          const HalfSegment* tmp;
          theLine->Get(k,tmp);
          Point p = tmp->GetDomPoint();
          // search left of k
          int m = k-1;
          while(m>0 && isDomPoint(p,m) && !found){
             found = !used[m];
             if(!found) m--;
          }
          if(found){
              hspos=m;
          } else { // search right of k
             m = k+1;
             while(m<size && isDomPoint(p,m) &&!found){
                found = !used[m];
                if(!found) m++;
             }
             if(found){
               hspos = m;
             }
          }

          if(!found){
             criticalPoints.pop();
          }
        }
        done = !found; // no extension found
     }
     result->EndBulkLoad();
     return result;
  }


private:
   bool isDomPoint(const Point& p,int pos){
     const HalfSegment* hs;
     theLine->Get(pos,hs);
     return AlmostEqual(p,hs->GetDomPoint());
   }


   Line* theLine;
   int pos;
   int size;
   bool* used;
};


int
SpatialComponents_l( Word* args, Word& result, int message,
                      Word& local, Supplier s ){

   switch(message){
     case OPEN:{
       local.addr = new LineComponentsLi((Line*) args[0].addr);
       return 0;
     }

     case REQUEST:{
         LineComponentsLi* li = (LineComponentsLi*) local.addr;
         Line* res = li->NextLine();
         if(res==0){
            return CANCEL;
         } else {
            result.addr = res;
            return YIELD;
         }
     }
     case CLOSE:{
       if(local.addr){
          LineComponentsLi* li = (LineComponentsLi*) local.addr;
          delete li;
          local.setAddr(0);
       }
       return 0;
     }
   }
   return 0;
}


/*
10.4.28 Value mapping functions of operator ~vertices~

*/
int SpatialVertices_r(Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  result = qp->ResultStorage(s);
  Region* reg = (Region*)args[0].addr;
  Points* res = (Points*)result.addr;
  reg->Vertices(res);
  return 0;
}

int SpatialVertices_l(Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  result = qp->ResultStorage(s);
  ((Line*)args[0].addr)->Vertices( (Points*) result.addr );
  return 0;
}


/*
10.4.28 Value mapping functions of operator ~boundary~

*/

int SpatialBoundary_r(Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  result = qp->ResultStorage(s);
  Region* reg = (Region*)args[0].addr;
  Line* res = (Line*) result.addr;
  reg->Boundary(res);
  return 0;
}
int SpatialBoundary_l(Word* args, Word& result, int message,
                      Word& local, Supplier s )
{
  result = qp->ResultStorage(s);
  Line* line = (Line*)args[0].addr;
  Points* res = (Points*) result.addr;
  line->Boundary(res);
  return 0;
}

/*
10.4.23 Value mapping functions of operator ~atpoint~

*/
int
SpatialAtPoint( Word* args, Word& result, int message,
                Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  SimpleLine *l = (SimpleLine*)args[0].addr;
  Point *p = (Point*)args[1].addr;
  CcBool *startsSmaller = (CcBool*)args[2].addr;
  double res;

  if( p->IsDefined() &&
      startsSmaller->IsDefined() &&
      l->AtPoint( *p, startsSmaller->GetBoolval(), res ) )
    ((CcReal*)result.addr)->Set( true, res );
  else
    ((CcReal*)result.addr)->SetDefined( false );
  return 0;
}

/*
10.4.23 Value mapping functions of operator ~atposition~

*/
int
SpatialAtPosition( Word* args, Word& result, int message,
                   Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  SimpleLine *l = (SimpleLine*)args[0].addr;
  CcReal *pos = (CcReal*)args[1].addr;
  CcBool *startsSmaller = (CcBool*)args[2].addr;
  Point *p = (Point*)result.addr;

  if( !pos->IsDefined() ||
      !startsSmaller->IsDefined() ||
      !l->AtPosition( pos->GetRealval(),
                      startsSmaller->GetBoolval(),
                      *p ) )
    p->SetDefined( false );

  return 0;
}

/*
10.4.23 Value mapping functions of operator ~subline~

*/
int
SpatialSubLine( Word* args, Word& result, int message,
                Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  SimpleLine *l = (SimpleLine*)args[0].addr;
  CcReal *pos1 = (CcReal*)args[1].addr,
         *pos2 = (CcReal*)args[2].addr;
  CcBool *startsSmaller = (CcBool*)args[3].addr;
  SimpleLine *rLine = (SimpleLine*)result.addr;

  rLine->Clear();

  if( pos1->IsDefined() &&
      pos2->IsDefined() &&
      startsSmaller->IsDefined() )
    l->SubLine( pos1->GetRealval(),
                pos2->GetRealval(),
                startsSmaller->GetBoolval(),
                *rLine );

  return 0;
}

/*
10.4.26 Value mapping function of operator ~add~

*/
int
SpatialAdd_p( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  const Point *p1= (Point*)args[0].addr;
  const Point *p2= (Point*)args[1].addr;

  if( p1->IsDefined() && p2->IsDefined() )
    *((Point*)result.addr) = *p1 + *p2 ;
  else
    ((Point*)result.addr)->SetDefined( false );

  return 0;
}

/*
10.4.27 Value mapping function of operator ~getx~

*/
int
SpatialGetX_p( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  const Point *p = (Point*)args[0].addr;

  if( p->IsDefined() )
    ((CcReal*)result.addr)->Set( true, p->GetX() ) ;
  else
    ((CcReal*)result.addr)->Set( false, 0.0 );

  return 0;
}

/*
10.4.28 Value mapping function of operator ~gety~

*/
int
SpatialGetY_p( Word* args, Word& result, int message,
                    Word& local, Supplier s )
{
  result = qp->ResultStorage( s );

  const Point *p = (Point*)args[0].addr;

  if( p->IsDefined() )
    ((CcReal*)result.addr)->Set( true, p->GetY() ) ;
  else
    ((CcReal*)result.addr)->Set( false, 0.0 );

  return 0;
}


/*
10.4.29 Value mapping function for the ~polylines~ operator

*/

class LineSplitter{
public:
/*
~Constructor~

Creates a LineSplitter from the given line.

*/
   LineSplitter(Line* line, bool ignoreCriticalPoints, bool allowCycles,
                Points* points = 0){
        this->theLine = line;
        size = line->Size();
        lastPos =0;
        this->points = points;
        used = new bool[size];
        memset(used,false,size);
        this->ignoreCriticalPoints = ignoreCriticalPoints;
        this->allowCycles = allowCycles;
   }

/*
~Destroys~ the lineSplitter

*/

   ~LineSplitter(){
      delete [] used;
      points = 0;
    }

/*
~NextLine~

This function extracts the next simple part of the line.
If the line is processed completely, the result will be
0. This function creates a new line instance via the new
operator. The caller of this function has to ensure the
deletion of this object.

*/
    Line* NextLine(){
      // go to the first unused halfsegment
      while(lastPos<size && used[lastPos]){
        lastPos++;
      }
      if(lastPos>=size){
         return 0;
      }
      // unused segment found,  construct a new Line
      int maxSize = max(1,size - lastPos);
      Line* result = new Line(maxSize);
      set<Point> pointset;
      int pos = lastPos;
      bool done = false;
      result->Clear();
      result->StartBulkLoad();
      const HalfSegment* hs1;
      const HalfSegment* hs2; // partner of hs1
      int edgeno = 0;
      bool seconddir = false;
      theLine->Get(pos,hs1);
      Point firstPoint = hs1->GetDomPoint();
      bool isCycle = false;


      while(!done){ // extension possible
        theLine->Get(pos,hs1);
        int partnerpos = hs1->GetAttr().partnerno;
        theLine->Get(partnerpos, hs2);
        Point p1 = hs1->GetDomPoint();
        pointset.insert(p1);
        Point p = hs2->GetDomPoint();
        pointset.insert(p);
        // add the halfsegments to the result
        HalfSegment Hs1 = *hs1;
        HalfSegment Hs2 = *hs2;
        AttrType attr1 = Hs1.GetAttr();
        attr1.edgeno = edgeno;
        Hs1.SetAttr(attr1);
        AttrType attr2 = Hs2.GetAttr();
        attr2.edgeno=edgeno;
        Hs2.SetAttr(attr2);
        edgeno++;
        (*result) += (Hs1);
        (*result) += (Hs2);
        // mark as used
        used[pos] = true;
        used[partnerpos] = true;

        if(isCycle){
           done = true;
        } else {
           bool found = false;
           int sp = partnerpos-1;

           if(points==0 || !points->Contains(p)){//no forced split

             // search for extension of the polyline
             // search left of partnerpos for an extension
             while(sp>0 && !found){
               const HalfSegment* hs3;
               if(!used[sp]){
                 theLine->Get(sp,hs3);
                 if(AlmostEqual(p,hs3->GetDomPoint())){
                   Point p3 = hs3->GetSecPoint(); // cycles?
                   if(pointset.find(p3)==pointset.end() ||
                     (allowCycles && AlmostEqual(p3,firstPoint))){
                     if(AlmostEqual(p3,firstPoint)){
                       isCycle = true;
                     }
                     found = true;
                   } else {
                     sp--;
                   }
                 } else {
                   sp = -1; // stop searching
                 }
               } else {
                 sp --; // search next
               }
             }
             // search on the right side
             if(!found){
                sp = partnerpos + 1;
                while(sp<size && !found){
                  if(!used[sp]){
                    const HalfSegment* hs3;
                    theLine->Get(sp,hs3);
                    if(AlmostEqual(p,hs3->GetDomPoint())){
                      Point p3 = hs3->GetSecPoint(); // avoid cycles
                      if(pointset.find(p3)==pointset.end() ||
                         (allowCycles && AlmostEqual(p3,firstPoint))){
                        if(AlmostEqual(p3,firstPoint)){
                          isCycle = true;
                        }
                        found = true;
                      } else {
                        sp++;
                      }
                    } else {
                        sp = size; // stop searching
                    }
                  } else {
                    sp ++; // search next
                  }
                }
             }
        }

        if(found){ // sp is a potential extension of the line
          if(ignoreCriticalPoints || !isCriticalPoint(partnerpos)){
            pos = sp;
          } else {
            done = true;
          }
        }  else { // no extension found
          done = true;
        }

        if(done && !seconddir && (lastPos < (size-1)) &&
           (points==0 || !points->Contains(firstPoint)) &&
           (!isCycle)){
           // done means at this point, the line can't be extended
           // in the direction start from the first selected halfsegment.
           // but is is possible the extend the line by going into the
           // reverse direction
           seconddir = true;
           const HalfSegment* hs;
           theLine->Get(lastPos,hs);
           Point p = hs->GetDomPoint();
           while(lastPos<size && used[lastPos]){
             lastPos ++;
           }
           if(lastPos <size){
             theLine->Get(lastPos,hs);
             Point p2 = hs->GetDomPoint();
             if(AlmostEqual(p,p2)){
               if(pointset.find(hs->GetSecPoint())==pointset.end()){
                 if(ignoreCriticalPoints || !isCriticalPoint(lastPos)){
                   pos = lastPos;
                   done = false;
                 }
               }
             }
           }
          }
        } // isCycle
      } // while
      result->EndBulkLoad();
      return result;
    }
private:
/*
~isCriticalPoint~

Checks whether the dominating point of the halfsegment at
position index is a critical one meaning a junction within the
line.

*/
   bool isCriticalPoint(int index){
      // check for critical point
      const HalfSegment* hs;
      theLine->Get(index,hs);
      Point  cpoint = hs->GetDomPoint();
      int count = 0;
      for(int i=index-2; i<= index+2 ; i++){
           if(i>=0 && i<size){
              theLine->Get(i,hs);
              if(AlmostEqual(cpoint, hs->GetDomPoint())){
                  count++;
              }
           }
       }
       return count>2;
   }
  /*
   bool AlmostEqual2(const Point& p1, const Point& p2 ){
      double z1 = abs(p1.GetX());
      double z2 = abs(p1.GetY());
      double z3 = abs(p2.GetX());
      double z4 = abs(p2.GetY());
      double Min = min(min(z1,z2) , min(z3,z4));
      double eps = max(FACTOR, FACTOR*Min);
      if(abs(z1-z3)>eps) return false;
      if(abs(z2-z4)>eps) return false;
      return true;
   }
  */
   bool* used;
   Line* theLine;
   int lastPos;
   int size;
   bool ignoreCriticalPoints;
   Points* points;
   bool allowCycles;
};


template<bool allowCycles>
int SpatialPolylines(Word* args, Word& result, int message,
                    Word& local, Supplier s){

   LineSplitter* localinfo;
   Line* res;

   result = qp->ResultStorage(s);
   switch (message){
      case OPEN:
          if(qp->GetNoSons(s)==2){
             local.setAddr(new LineSplitter((Line*)args[0].addr,
                                ((CcBool*)args[1].addr)->GetBoolval(),
                                allowCycles));
          } else {
            local.setAddr(new LineSplitter((Line*)args[0].addr,
                            ((CcBool*)args[1].addr)->GetBoolval(),
                            allowCycles,
                            ((Points*)args[2].addr)));
          }
          return 0;
      case REQUEST:
           localinfo = (LineSplitter*) local.addr;
           res = localinfo->NextLine();
           if(res==0){
              return CANCEL;
           } else {
              result.setAddr(res);
              return YIELD;
           }
      case CLOSE:
           localinfo = (LineSplitter*) local.addr;
           if(localinfo!=0){
              delete localinfo;
              local.setAddr(0);
           }
           return 0;
   }
   return 0; // ignore unknown message
}




/*
10.4.30 Value Mapping for the ~segments~ operator

*/
class SegmentsInfo{
  public:
    SegmentsInfo(Line* line){
       this->theLine =(Line*) line->Copy(); // increase the ref counter of line
       this->position = 0;
       this->size = line->Size();
    }
    ~SegmentsInfo(){
      if(theLine!=0){
         theLine->DeleteIfAllowed(); // mark as free'd
      }
    }
    Line* NextSegment(){
       const HalfSegment* hs;
       // search for a segment with left dominating point
       bool found = false;
       while((position<size) && !found){
             theLine->Get(position,hs);
             if(hs->IsLeftDomPoint()){
                found=true;
             } else {
                position++;
             }
       }
       position++; // for the next run
       if(!found){ // no more segments available
          return 0;
       } else {
          Line* res = new Line(2);
          HalfSegment hs1 = *hs;
          res->StartBulkLoad();
          hs1.attr.edgeno = 0;
          (*res) += hs1;
          hs1.SetLeftDomPoint(false);
          (*res) += hs1;
          res->EndBulkLoad();
          return res;
       }
    }
  private:
     int position;
     int size;
     Line* theLine;
};


int SpatialSegments(Word* args, Word& result, int message,
                    Word& local, Supplier s){

 SegmentsInfo* si=0;
 Line* res =0;
 switch(message){
    case OPEN:
      local.addr = new SegmentsInfo((Line*)args[0].addr);
      return 0;

   case REQUEST:
      si = (SegmentsInfo*) local.addr;
      res = si->NextSegment();
      if(res){
         result.setAddr(res);
         return YIELD;
      } else {
         return CANCEL;
      }

    case CLOSE:
      if(local.addr)
      {
        si = (SegmentsInfo*) local.addr;
        delete si;
        local.setAddr(0);
      }
      return 0;
 }
 return 0;
}




/*
10.4.31 Value Mappings for the simplify operator

*/

int SpatialSimplify_LReal(Word* args, Word& result, int message,
                    Word& local, Supplier s){
   result = qp->ResultStorage(s);
   Line* line = (Line*) args[0].addr;
   double epsilon = ((CcReal*)args[1].addr)->GetRealval();
   line->Simplify(* ((Line*)result.addr),epsilon);
   return 0;
}

int SpatialSimplify_LRealPs(Word* args, Word& result, int message,
                    Word& local, Supplier s){
   result = qp->ResultStorage(s);
   Line* line = (Line*) args[0].addr;
   double epsilon = ((CcReal*)args[1].addr)->GetRealval();
   Points* ps = (Points*) args[2].addr;
   line->Simplify(* ((Line*)result.addr),epsilon,*ps);
   return 0;
}


/*
10.4.32 Value Mapping for the ~get~ operator

*/
int SpatialGet(Word* args, Word& result, int message,
               Word& local, Supplier s){

   result = qp->ResultStorage(s);
   Points* ps = (Points*) args[0].addr;
   CcInt*  index = (CcInt*) args[1].addr;
   if(!ps->IsDefined() || !index->IsDefined()){
      ((Point*)result.addr)->SetDefined(false);
      return 0;
   }
   int i = index->GetIntval();
   if(i<0 || i >= ps->Size()){
      ((Point*)result.addr)->SetDefined(false);
      return 0;
   }

   const Point* p;
   ps->Get(i,p);
   ((Point*)result.addr)->CopyFrom(p);
   return 0;
}


/*
10.4.34 Value Mapping for the ~makeline~ operator

*/
int SpatialMakeLine(Word* args, Word& result, int message,
                    Word& local, Supplier s){

  result = qp->ResultStorage(s);
  Point* p1 = (Point*) args[0].addr;
  Point* p2 = (Point*) args[1].addr;
  Line* res = (Line*) result.addr;
  res->Clear();
  if(!p1->IsDefined() || !p2->IsDefined()){
       res->SetDefined(false);
       return 0;
  }
  if(AlmostEqual(*p1,*p2)){
     return 0;
  }
  res->StartBulkLoad();
  HalfSegment h(true, *p1, *p2);
  h.attr.edgeno = 0;
  (*res) += h;
  h.SetLeftDomPoint(false);
  (*res) += h;
  res->EndBulkLoad();
  return 0;
}
/*
~Realminize~

*/
int RealminizeVM(Word* args, Word& result, int message,
                Word& local, Supplier s){

   Line* arg = static_cast<Line*>(args[0].addr);
   result = qp->ResultStorage(s);
   Line* res = static_cast<Line*>(result.addr);
   Realminize2(*arg,*res);
   return 0;
}


/*
Value mapping for set operations

*/

template<class t1, class t2, class tres, avlseg::SetOperation op>
int SetOpVM(Word* args, Word& result, int message,
            Word& local, Supplier s){
   result = qp->ResultStorage(s);
   t1* arg1 = static_cast<t1*>(args[0].addr);
   t2* arg2 = static_cast<t2*>(args[1].addr);
   tres* res = static_cast<tres*>(result.addr);
   SetOp(*arg1,*arg2,*res,op);
   return 0;
}

template<class t1, class t2, class tres, avlseg::SetOperation op>
int SetOpVMSym(Word* args, Word& result, int message,
            Word& local, Supplier s){
   result = qp->ResultStorage(s);
   t1* arg1 = static_cast<t1*>(args[0].addr);
   t2* arg2 = static_cast<t2*>(args[1].addr);
   tres* res = static_cast<tres*>(result.addr);
   SetOp(*arg2,*arg1,*res,op);
   return 0;
}


/*
Value Mapping for ~CommonBorder2~

*/
int CommonBorder2VM(Word* args, Word& result, int message,
            Word& local, Supplier s){
   result = qp->ResultStorage(s);
   Region* arg1 = static_cast<Region*>(args[0].addr);
   Region* arg2 = static_cast<Region*>(args[1].addr);
   Line* res = static_cast<Line*>(result.addr);
   CommonBorder(*arg2,*arg1,*res);
   return 0;
}

/*
Value Mapping for ~toLine~

*/
int toLineVM(Word* args, Word& result, int message,
            Word& local, Supplier s){
   result = qp->ResultStorage(s);
   SimpleLine* sline = static_cast<SimpleLine*>(args[0].addr);
   Line* res = static_cast<Line*>(result.addr);
   sline->toLine(*res);
   return 0;
}


/*
Value Mapping for ~fromLine~

*/
int fromLineVM(Word* args, Word& result, int message,
            Word& local, Supplier s){
   result = qp->ResultStorage(s);
   Line* line = static_cast<Line*>(args[0].addr);
   SimpleLine* res = static_cast<SimpleLine*>(result.addr);
   res->fromLine(*line);
   return 0;
}


/*
Value Mapping for ~isCycle~

*/
int isCycleVM(Word* args, Word& result, int message,
            Word& local, Supplier s){
   result = qp->ResultStorage(s);
   SimpleLine* sline = static_cast<SimpleLine*>(args[0].addr);
   CcBool* res = static_cast<CcBool*>(result.addr);
   res->Set(true,sline->IsCycle());
   return 0;
}


/*
10.5 Definition of operators

Definition of operators is done in a way similar to definition of
type constructors: an instance of class ~Operator~ is defined.

Because almost all operators are overloaded, we have first do define an array of value
mapping functions for each operator. For nonoverloaded operators there is also such and array
defined, so it easier to make them overloaded.

10.5.1 Definition of value mapping vectors

*/
ValueMapping spatialisemptymap[] = {
  SpatialIsEmpty<Point>,
  SpatialIsEmpty<Points>,
  SpatialIsEmpty<Line>,
  SpatialIsEmpty<Region>,
  SpatialIsEmpty<SimpleLine> };

ValueMapping spatialequalmap[] = {
  SpatialEqual<Point>,
  SpatialEqual<Points>,
  SpatialEqual<Line>,
  SpatialEqual<Region>,
  SpatialEqual<SimpleLine>};

ValueMapping spatialnotequalmap[] = {
  SpatialNotEqual<Point>,
  SpatialNotEqual<Points>,
  SpatialNotEqual<Line>,
  SpatialNotEqual<Region>,
  SpatialNotEqual<SimpleLine> };

ValueMapping spatialintersectsmap[] = {
  SpatialIntersectsVM<Points,Points,false>,
  SpatialIntersectsVM<Points,Line,false>,
  SpatialIntersectsVM<Points,Region,false>,
  SpatialIntersectsVM<Points,Line,true>,
  SpatialIntersectsVM<Line,Line,false>,
  SpatialIntersectsVM<Line,Region,false>,
  SpatialIntersectsVM<Points,Region,true>,
  SpatialIntersectsVM<Line,Region,true>,
  SpatialIntersectsVM<Region,Region,false>,
  SpatialIntersectsVM<SimpleLine,SimpleLine,false>
};

ValueMapping spatialinsidemap[] = {
  SpatialInside_pps,
  SpatialInside_pl,
  SpatialInside_pr,
  SpatialInside_psps,
  SpatialInside_psl,
  SpatialInside_psr,
  SpatialInside_ll,
  SpatialInside_lr,
  SpatialInside_rr };

ValueMapping spatialadjacentmap[] = {
  SpatialAdjacent_psr,
  SpatialAdjacent_lr,
  SpatialAdjacent_rps,
  SpatialAdjacent_rl,
  SpatialAdjacent_rr };

ValueMapping spatialintersectionmap[] = {
  SpatialIntersection_pp,
  SpatialIntersection_pps,
  SpatialIntersection_pl,
  SpatialIntersection_pr,
  SpatialIntersection_psp,
  SpatialIntersection_psps,
  SpatialIntersection_psl,
  SpatialIntersection_psr,
  SpatialIntersection_lp,
  SpatialIntersection_lps,
  SpatialIntersection_ll,
  SpatialIntersection_rp,
  SpatialIntersection_rps };

ValueMapping spatialminusmap[] = {
  SpatialMinus_pp,
  SpatialMinus_pps,
  SpatialMinus_pl,
  SpatialMinus_pr,
  SpatialMinus_psp,
  SpatialMinus_psps,
  SpatialMinus_psl,
  SpatialMinus_psr,
  SpatialMinus_lp,
  SpatialMinus_lps,
  SpatialMinus_rp,
  SpatialMinus_rps,
  SpatialMinus_rl };

ValueMapping spatialunionmap[] = {
  SpatialUnion_pps,
  SpatialUnion_psp,
  SpatialUnion_psps };

ValueMapping spatialdistancemap[] = {
  SpatialDistance<Point,Point,false>,
  SpatialDistance<Points,Point,true>,
  SpatialDistance<Line,Point,true>,
  SpatialDistance<Region,Point,true>,
  SpatialDistance<Points,Point,false>,
  SpatialDistance<Points,Points,false>,
  SpatialDistance<Line,Points,true>,
  SpatialDistance<Region,Points,true>,
  SpatialDistance<Line,Point,false>,
  SpatialDistance<Line,Points,false>,
  SpatialDistance<Line,Line,false>,
  SpatialDistance<Region,Point,false>,
  SpatialDistance<Region,Points,false>,
  SpatialDistance<Region,Region,false>,
  SpatialDistance<SimpleLine,Point,false>,
  SpatialDistance<SimpleLine,Point, true>,
  SpatialDistance<SimpleLine, Points, false>,
  SpatialDistance<SimpleLine, Points, true>,
  SpatialDistance<SimpleLine, SimpleLine, false>,
  SpatialDistance<Point, Rectangle<2>, true>,
  SpatialDistance<Point, Rectangle<2>, false>,
  SpatialDistance<Points, Rectangle<2>, true>,
  SpatialDistance<Points, Rectangle<2>, false>,
  SpatialDistance<Line, Rectangle<2>, true>,
  SpatialDistance<Line, Rectangle<2>, false>,
  SpatialDistance<Region, Rectangle<2>, true>,
  SpatialDistance<Region, Rectangle<2>, false>,
  SpatialDistance<SimpleLine, Rectangle<2>, true>,
  SpatialDistance<SimpleLine, Rectangle<2>, false>
};

ValueMapping spatialnocomponentsmap[] = {
  SpatialNoComponents_ps,
  SpatialNoComponents_l,
  SpatialNoComponents_r };

ValueMapping spatialnosegmentsmap[] = {
  SpatialNoSegments<Line>,
  SpatialNoSegments<Region>,
  SpatialNoSegments<SimpleLine> };

ValueMapping spatialbboxmap[] = {
  SpatialBBox<Point>,
  SpatialBBox<Points>,
  SpatialBBox<Line>,
  SpatialBBox<Region>,
  SpatialBBox<SimpleLine> };

ValueMapping spatialtouchpointsmap[] = {
  SpatialTouchPoints_lr,
  SpatialTouchPoints_rl,
  SpatialTouchPoints_rr };

ValueMapping spatialtranslatemap[] = {
  SpatialTranslate_p,
  SpatialTranslate_ps,
  SpatialTranslate_l,
  SpatialTranslate_r };

ValueMapping spatialrotatemap[] = {
  SpatialRotate<Point>,
  SpatialRotate<Points>,
  SpatialRotate<Line>,
  SpatialRotate<Region>};

ValueMapping spatialwindowclippinginmap[] = {
  SpatialWindowClippingIn_l,
  SpatialWindowClippingIn_r };

ValueMapping spatialwindowclippingoutmap[] = {
  SpatialWindowClippingOut_l,
  SpatialWindowClippingOut_r };

ValueMapping spatialscalemap[] = {
  SpatialScale_p,
  SpatialScale_ps,
  SpatialScale_l,
  SpatialScale_r };

ValueMapping spatialcomponentsmap[] = {
  SpatialComponents_ps,
  SpatialComponents_r,
  SpatialComponents_l };

ValueMapping spatialverticesmap[] = {
  SpatialVertices_l,
  SpatialVertices_r };

ValueMapping spatialboundarymap[] = {
  SpatialBoundary_r,
  SpatialBoundary_l};

ValueMapping spatialaddmap[] = { SpatialAdd_p };

ValueMapping spatialgetxmap[] = { SpatialGetX_p };

ValueMapping spatialgetymap[] = { SpatialGetY_p };

ValueMapping spatialsimplifymap[] = { SpatialSimplify_LReal,
                                      SpatialSimplify_LRealPs };

ValueMapping spatialsizemap[] = {
      SpatialSize<Line>,
      SpatialSize<Region>,
      SpatialSize<SimpleLine>
  };


ValueMapping Union2Map[] = {
       SetOpVM<Line,Line,Line,avlseg::union_op>,
       SetOpVM<Region,Region,Region,avlseg::union_op>,
       SetOpVM<Region,Line,Region,avlseg::union_op>,
       SetOpVMSym<Line,Region,Region,avlseg::union_op>
       };

ValueMapping Intersection2Map[] = {
       SetOpVM<Line,Line,Line,avlseg::intersection_op>,
       SetOpVM<Region,Region,Region,avlseg::intersection_op>,
       SetOpVMSym<Region,Line,Line,avlseg::intersection_op>,
       SetOpVM<Line,Region,Line,avlseg::intersection_op>
      };

ValueMapping Difference2Map[] = {
       SetOpVM<Line,Line,Line,avlseg::difference_op>,
       SetOpVM<Region,Region,Region,avlseg::difference_op>,
       SetOpVM<Region,Line,Region,avlseg::difference_op>,
       SetOpVM<Line,Region,Line,avlseg::difference_op>
      };

ValueMapping SpatialCrossingsMap[] = {
          SpatialCrossings<Line>,
          SpatialCrossings<SimpleLine>
      };


/*
10.5.2 Definition of specification strings

*/
const string SpatialSpecIsEmpty  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
     "( <text>point -> bool, points -> bool, line -> bool,"
       "region -> bool, sline -> bool</text---> "
       "<text>isempty ( _ )</text--->"
       "<text>Returns whether the value is defined or not.</text--->"
       "<text>query isempty ( line1 )</text--->"
       ") )";

const string SpatialSpecEqual  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
  "( <text>(point point) -> bool, (points points) -> bool, "
  "(line line) -> bool, (region region) -> bool,"
  " (sline sline) -> bool </text--->"
  "<text>_ = _</text--->"
  "<text>Equal.</text--->"
  "<text>query point1 = point2</text--->"
  ") )";

const string SpatialSpecNotEqual  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
  "( <text>(point point) -> bool, (points points) -> bool, "
  "(line line) -> bool, (region region) -> bool,"
  "(sline sline) -> bool</text--->"
  "<text>_ # _</text--->"
  "<text>Not equal.</text--->"
  "<text>query point1 # point2</text--->"
  ") )";

const string SpatialSpecIntersects  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
  "( <text>(points||line||region x points||line||region) -> bool ||"
  " sline x sline -> bool  </text--->"
  "<text>_ intersects _</text--->"
  "<text>Intersects.</text--->"
  "<text>query region1 intersects region2</text--->"
  ") )";

const string SpatialSpecInside  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
  "( <text>(points||line||region x points||line||region) "
  "-> bool</text--->"
  "<text>_ inside _</text--->"
  "<text>Inside.</text--->"
  "<text>query point1 inside line1</text--->"
  ") )";

const string SpatialSpecAdjacent  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
  "( <text>(points||line||region x points||line||region) -> bool</text--->"
  "<text>_ adjacent _</text--->"
  "<text>two regions are adjacent.</text--->"
  "<text>query r1 adjacent r2</text--->"
  ") )";

const string SpatialSpecOverlaps  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
  "( <text>(region x region) -> bool</text--->"
  "<text>_ overlaps _</text--->"
  "<text>two spatial objects overlap each other.</text--->"
  "<text>query line overlap region</text--->"
  ") )";

const string SpatialSpecOnBorder  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(point x region) -> bool</text--->"
  "<text>_ onborder _</text--->"
  "<text>on endpoints or on border edges.</text--->"
  "<text>query point onborder line</text--->"
  ") )";

const string SpatialSpecInInterior  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(point x region) -> bool</text--->"
  "<text>_ ininterior _</text--->"
  "<text>in interior of a line or region.</text--->"
  "<text>query point ininterior region</text--->"
  ") )";

const string SpatialSpecIntersection  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text> (point||points||line||region x point||points||line||region)"
  "-> points||line||region</text--->"
  "<text>_intersection_</text--->"
  "<text>intersection of two sets.</text--->"
  "<text>query points intersection region</text--->"
  ") )";

const string SpatialSpecMinus  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(point||points||line||region x point||points||line||region)"
  " -> point||points||line||region</text--->"
  "<text>_ minus _</text--->"
  "<text>minus of two sets.</text--->"
  "<text>query points minus point</text--->"
  ") )";

const string SpatialSpecUnion  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(point x points) -> points\n"
  "(points x point) -> points\n"
  "(points x points) -> points</text--->"
  "<text>_union_</text--->"
  "<text>union of two sets. Also see: 'union_new'.</text--->"
  "<text>query points union point</text--->"
  ") )";

const string SpatialSpecCrossings  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(line x line) -> points || sline x sline -> points</text--->"
  "<text>crossings(_,_)</text--->"
  "<text>crossing points of two lines.</text--->"
  "<text>query crossings(line1, line2)</text--->"
  ") )";

const string SpatialSpecSingle  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(points) -> point</text--->"
  "<text> single(_)</text--->"
  "<text>transform a single-element points value to point value.</text--->"
  "<text>query single(points)</text--->"
  ") )";

const string SpatialSpecDistance  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(point||points||line||sline||rect x "
  "point||points||line||sline||rect) -> real</text--->"
  "<text>distance(_, _)</text--->"
  "<text>compute distance between two spatial objects.</text--->"
  "<text>query distance(point, line)</text--->"
  ") )";

const string SpatialSpecDirection  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(point x point) -> real</text--->"
  "<text>direction(_, _)</text--->"
  "<text>compute the direction (0 - 360 degree) from one point to "
  "another point.</text--->"
  "<text>query direction(p1, p2)</text--->"
  ") )";

const string SpatialSpecNocomponents  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(points||line||region) -> int</text--->"
  "<text> no_components( _ )</text--->"
  "<text>return the number of components of a spatial object.</text--->"
  "<text>query no_components(region)</text--->"
  ") )";

const string SpatialSpecNoSegments  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>{line, region, sline} -> int</text--->"
  "<text> no_segments( _ )</text--->"
  "<text>return the number of half segments of a region.</text--->"
  "<text>query no_segments(region)</text--->"
  ") )";

const string SpatialSpecBbox  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(point||points||line||region||sline) -> rect</text--->"
  "<text> bbox( _ )</text--->"
  "<text>return the bounding box of a spatial type.</text--->"
  "<text>query bbox(region)</text--->"
  ") )";

const string SpatialSpecSize  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(line, region, sline) -> real</text--->"
  "<text> size( _ )</text--->"
  "<text> return the size (length, area) of a spatial object.</text--->"
  "<text> query size(line)</text--->"
  ") )";

const string SpatialSpecTouchpoints  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(line||region x region) -> points</text--->"
  "<text> touchpoints(_, _) </text--->"
  "<text> return the touch points of a region and another "
  "region or line.</text--->"
  "<text> query touchpoints(line, region)</text--->"
  ") )";

const string SpatialSpecCommonborder  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(region x region) -> line</text--->"
  "<text> commonborder(_, _ )</text--->"
  "<text> return the common border of two regions.</text--->"
  "<text> query commonborder(region1, region2)</text--->"
  ") )";

const string SpatialSpecTranslate  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(point||points||line||region x real x real) -> "
  "point||points||line||region</text--->"
  "<text> _ translate[list]</text--->"
  "<text> move the object parallely for some distance.</text--->"
  "<text> query region1 translate[3.5, 15.1]</text--->"
  ") )";

const string SpatialSpecRotate  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(point||points||line||region x real x real x real) -> "
  "point||points||line||region</text--->"
  "<text> _ translate[ x y theta]</text--->"
  "<text> rotates the spatial object around (x,y) theta degree</text--->"
  "<text> query region1 rotate[3.5, 15.1, 10.0]</text--->"
  ") )";

const string SpatialSpecCenter  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text> points -> point </text--->"
  "<text> center(_) </text--->"
  "<text> computes the center of the points value</text--->"
  "<text> query center(vertices(tiergarten))</text--->"
  ") )";

const string SpatialSpecConvexhull  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text> points -> region </text--->"
  "<text> convexhull(_) </text--->"
  "<text> computes the convex hull of the points value</text--->"
  "<text> query convexhull(vertices(tiergarten))</text--->"
  ") )";

const string SpatialSpecAdd  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>point x point -> point</text--->"
  "<text> _ + _</text--->"
  "<text> Returns the vector addition for two points.</text--->"
  "<text> query [const point value (0.0 -1.2)] + "
  "[const point value (-5.0 1.2)] </text--->"
  ") )";

const string SpatialSpecWindowClippingIn  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(line x rect) -> line, (region x rect) --> region</text--->"
  "<text> windowclippingin( _, _ ) </text--->"
  "<text> computes the part of the object that is inside the window.</text--->"
  "<text> query windowclippingin(line1, window)</text--->"
  ") )";

const string SpatialSpecWindowClippingOut  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>(line x rect) -> line, (region x rect) --> region</text--->"
  "<text> windowclippingout( _, _ ) </text--->"
  "<text> computes the part of the object that is outside the window.</text--->"
  "<text> query windowclippingout(line1, rect)</text--->"
  ") )";

const string SpatialSpecScale  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>for T in {point, points, line, region}: "
  "T x real -> T</text--->"
  "<text> _ scale [ _ ] </text--->"
  "<text> scales an object by the given factor.</text--->"
  "<text> query region1 scale[1000.0]</text--->"
  ") )";

const string SpatialSpecComponents  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
  "( <text>points -> stream(point), region -> "
  "stream(region), line -> stream(line)</text--->"
  "<text>components( _ )</text--->"
  "<text>Returns the components of a points or region "
  "object.</text--->"
  "<text>query components(r1) count;</text--->"
  ") )";

const string SpatialSpecVertices  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
  "( <text>(region -> points) or (line -> points)</text--->"
  "<text>vertices(_)</text--->"
  "<text>Returns the vertices of a region or line.</text--->"
  "<text>query vertices(r1)</text--->"
  ") )";

const string SpatialSpecBoundary  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
  "( <text>(region -> line) or (line -> points)</text--->"
  "<text>boundary(_)</text--->"
  "<text>Returns the boundary of a region or a line.</text--->"
  "<text>query boundary(thecenter)</text--->"
  ") )";

const string SpatialSpecAtPoint  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
  "( <text>sline x point x bool -> real</text--->"
  "<text>atpoint(_, _, _)</text--->"
  "<text>Returns the relative position of the point in the line."
  "The boolean flag indicates where the positions start, i.e. "
  "from the smaller point (TRUE) or from the bigger one (FALSE)."
  "</text---><text>query atpoint(l, p, TRUE)</text--->"
  ") )";

const string SpatialSpecAtPosition  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
  "( <text>sline x real x bool -> point</text--->"
  "<text>atposition(_, _, _)</text--->"
  "<text>Returns the point at a relative position in the line."
  "The boolean flag indicates where the positions start, i.e. "
  "from the smaller point (TRUE) or from the bigger one (FALSE)."
  "</text---><text>query atposition(l, 0.0, TRUE)</text--->"
  ") )";

const string SpatialSpecSubLine  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
  "( <text>sline x real x real x bool -> line</text--->"
  "<text>subline(_, _, _, _)</text--->"
  "<text>Returns the sub-line inside the two relative positions."
  "The boolean flag indicates where the positions start, i.e. "
  "from the smaller point (TRUE) or from the bigger one (FALSE)."
  "</text---><text>query subline(l, 0.0, size(.l), TRUE)</text--->"
  ") )";

const string SpatialSpecGetX  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>point -> real</text--->"
  "<text>getx( _ )</text--->"
  "<text>Extracts the x-component of a point.</text--->"
  "<text> query getx([const point value (0.0 -1.2)])</text--->"
  ") )";

const string SpatialSpecGetY  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>point -> real</text--->"
  "<text>gety( _ )</text--->"
  "<text>Extracts the y-component of a point.</text--->"
  "<text> query gety([const point value (0.0 -1.2)])</text--->"
  ") )";

const string SpatialSpecLine2Region  =
  "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
  "( <text>line -> region</text--->"
  "<text>_ line2region</text--->"
  "<text>Converts a line object to a region object.</text--->"
  "<text> query gety([const point value (0.0 -1.2)])</text--->"
  ") )";

const string SpatialSpecRect2Region  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>rect -> region</text--->"
    "<text>_ rect2region</text--->"
    "<text>Converts a rect object to a region object.</text--->"
    "<text> query </text--->"
    ") )";

const string SpatialSpecArea  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>region -> real</text--->"
    "<text>area( _ )</text--->"
    "<text>Returns the area of a region object as a real value.</text--->"
    "<text> query area( tiergarten )</text--->"
    ") )";

const string SpatialSpecPolylines  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>line  x bool [ x points] -> stream( line ) </text--->"
    "<text> _  polylines [ _ , _ ] </text--->"
    "<text>Returns a stream of simple line objects "
    " whose union is the original line. The boolean parameter"
    "indicates to ignore critical points as splitpoints.</text--->"
    "<text> query trajectory(train1) polylines [TRUE]  count</text--->"
    ") )";

const string SpatialSpecPolylinesC  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>line  x bool [ x points] -> stream( line ) </text--->"
    "<text> _  polylinesC [ _ , _ ] </text--->"
    "<text>Returns a stream of simple line objects "
    " whose union is the original line. The boolean parameter"
    "indicates to ignore critical points (branches) as splitpoints."
    " Some of the  resulting polylines may build a cycle.</text--->"
    "<text> query trajectoryC(train1) polylines [TRUE]  count</text--->"
    ") )";

const string SpatialSpecSimplify  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>line  x real [x points] -> line </text--->"
    "<text> simplify(_,_) </text--->"
    "<text>Simplifies a line value, the points value marks"
    " important points </text--->"
    "<text> query simplify(trajectory(train1),10.0) count</text--->"
    ") )";

const string SpatialSpecSegments  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>line  -> stream( line ) </text--->"
    "<text> segments( _ ) </text--->"
    "<text>Returns a stream of segments of the line.</text--->"
    "<text>query  (segments(PotsdamLine) count) = "
                 "(no_segments(PotsdamLine)) </text--->"
    ") )";

const string SpatialSpecGet  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>points x int -> point </text--->"
    "<text> _ get _  </text--->"
    "<text>Returns a point from a points value.</text--->"
    "<text>query  vertices(BGrenzenLine) get [1]    </text--->"
    ") )";


const string SpatialSpecMakeLine  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>point x point  -> line </text--->"
    "<text> makeline( _, _ )  </text--->"
    "<text>Create a segment from the arguments.</text--->"
    "<text>query makeline([const point value (0 0)],"
           " [ const point value (100 40)])</text--->"
    ") )";

const string Union2Spec =
 "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
 " ( <text> t1 x t2 -> tx, ti in {line, region}  </text--->"
 " \"  _ union2 _ \" "
 "  \" computes the union of two spatial values \" "
  "  \" query l1 union2 l2 \" ))";

const string Intersection2Spec =
 "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
 " ( <text> {line,region} x {line,region} -> {line,region} </text--->"
 " \"  _ intersection2 _ \" "
 "  <text> computes the intersection of two spatial values </text---> "
  "  \" query l1 intersection2 l2 \" ))";

const string Difference2Spec =
 "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
 " ( <text> {line,region} x {line,region} -> {line,region} </text--->"
 " \"  _ difference2 _ \" "
 "  \" computes the difference of two spatial values \" "
  "  \" query l1 difference2 l2 \" ))";

const string CommonBorder2Spec =
 "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
 " ( <text> region x region -> line </text--->"
 " \"  _ commonborder2  _ \" "
 "  <text> computes the common part of the"
 " boundaries of the arguments </text---> "
  "  \" query r1 commonborder2 r2 \" ))";


const string SpatialSpecRealminize  =
     "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
     "( <text>line -> line </text--->"
     "<text> realminize( _ )  </text--->"
     "<text>Returns the realminized argument.</text--->"
     "<text>query realminize(train7sections)</text--->"
     ") )";

const string SpatialSpecToLine  =
     "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
     "( <text>sline -> line </text--->"
     "<text> toline( _ )  </text--->"
     "<text>Converts an sline into a line</text--->"
     "<text>query toline(fromline(trajectory(train7))</text--->"
     ") )";

const string SpatialSpecFromLine  =
     "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
     "( <text>line -> sline </text--->"
     "<text> fromline( _ )  </text--->"
     "<text>Converts a line into an sline</text--->"
     "<text>query toline(fromline(trajectory(train7))</text--->"
     ") )";

const string SpatialSpecIsCycle  =
     "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
     "( <text>sline -> bool </text--->"
     "<text> iscycle( _ )  </text--->"
     "<text>check for cycle</text--->"
     "<text>query iscycle(fromline(trajectory(train7))</text--->"
     ") )";


/*
10.5.3 Definition of the operators

*/
Operator spatialisempty (
  "isempty",
  SpatialSpecIsEmpty,
  5,
  spatialisemptymap,
  SpatialSelectIsEmpty,
  SpatialTypeMapBool1 );

Operator spatialequal (
  "=",
  SpatialSpecEqual,
  5,
  spatialequalmap,
  SpatialSelectCompare,
  SpatialTypeMapCompare );

Operator spatialnotequal (
  "#",
  SpatialSpecNotEqual,
  5,
  spatialnotequalmap,
  SpatialSelectCompare,
  SpatialTypeMapCompare );

Operator spatialintersects (
  "intersects",
  SpatialSpecIntersects,
  10,
  spatialintersectsmap,
  SpatialSelectIntersects,
  IntersectsTM );

Operator spatialinside (
  "inside",
  SpatialSpecInside,
  9,
  spatialinsidemap,
  SpatialSelectInside,
  InsideTypeMap );

Operator spatialadjacent (
  "adjacent",
  SpatialSpecAdjacent,
  5,
  spatialadjacentmap,
  SpatialSelectAdjacent,
  AdjacentTypeMap );

Operator spatialoverlaps (
  "overlaps",
  SpatialSpecOverlaps,
  SpatialOverlaps_rr,
  Operator::SimpleSelect,
  RegionRegionMapBool );

Operator spatialonborder (
  "onborder",
  SpatialSpecOnBorder,
  SpatialOnBorder_pr,
  Operator::SimpleSelect,
  PointRegionMapBool );

Operator spatialininterior (
  "ininterior",
  SpatialSpecOnBorder,
  SpatialInInterior_pr,
  Operator::SimpleSelect,
  PointRegionMapBool );

Operator spatialintersection (
  "intersection",
  SpatialSpecIntersection,
  13,
  spatialintersectionmap,
  SpatialSelectIntersection,
  SpatialIntersectionMap );

Operator spatialminus (
  "minus",
  SpatialSpecMinus,
  13,
  spatialminusmap,
  SpatialSelectMinus,
  SpatialMinusMap );

Operator spatialunion (
  "union",
  SpatialSpecUnion,
  3,
  spatialunionmap,
  SpatialSelectUnion,
  SpatialUnionMap );

Operator spatialcrossings (
  "crossings",
  SpatialSpecCrossings,
  2,
  SpatialCrossingsMap,
  SpatialSelectCrossings,
  SpatialCrossingsTM );

Operator spatialsingle (
  "single",
  SpatialSpecSingle,
  SpatialSingle_ps,
  Operator::SimpleSelect,
  SpatialSingleMap );

Operator spatialdistance (
  "distance",
  SpatialSpecDistance,
  29,
  spatialdistancemap,
  SpatialSelectDistance,
  SpatialDistanceMap );

Operator spatialdirection (
  "direction",
  SpatialSpecDirection,
  SpatialDirection_pp,
  Operator::SimpleSelect,
  SpatialDirectionMap );

Operator spatialnocomponents (
  "no_components",
  SpatialSpecNocomponents,
  3,
  spatialnocomponentsmap,
  SpatialSelectNoComponents,
  SpatialNoComponentsMap );

Operator spatialnosegments (
  "no_segments",
  SpatialSpecNoSegments,
  3,
  spatialnosegmentsmap,
  SpatialSelectNoSegments,
  SpatialNoSegmentsMap );

Operator spatialbbox (
  "bbox",
  SpatialSpecBbox,
  5,
  spatialbboxmap,
  SpatialSelectBBox,
  SpatialBBoxMap );

Operator spatialsize (
  "size",
  SpatialSpecSize,
  3,
  spatialsizemap,
  SpatialSelectSize,
  SpatialSizeMap );

Operator spatialtouchpoints (
  "touchpoints",
  SpatialSpecTouchpoints,
  3,
  spatialtouchpointsmap,
  SpatialSelectTouchPoints,
  SpatialTouchPointsMap );

Operator spatialcommonborder (
  "commonborder",
  SpatialSpecCommonborder,
  SpatialCommonBorder_rr,
  Operator::SimpleSelect,
  SpatialCommonBorderMap );

Operator spatialtranslate (
  "translate",
  SpatialSpecTranslate,
  4,
  spatialtranslatemap,
  SpatialSelectTranslate,
  SpatialTranslateMap );

Operator spatialrotate (
  "rotate",
  SpatialSpecRotate,
  4,
  spatialrotatemap,
  SpatialSelectTranslate,
  SpatialRotateMap );


Operator spatialcenter (
  "center",
  SpatialSpecCenter,
  SpatialCenter,
  Operator::SimpleSelect,
  SpatialCenterMap );

Operator spatialconvexhull (
  "convexhull",
  SpatialSpecConvexhull,
  SpatialConvexhull,
  Operator::SimpleSelect,
  SpatialConvexhullMap );

Operator spatialadd (
  "+",
  SpatialSpecAdd,
  1,
  spatialaddmap,
  Operator::SimpleSelect,
  SpatialAddTypeMap );

Operator spatialwindowclippingin (
  "windowclippingin",
  SpatialSpecWindowClippingIn,
  4,
  spatialwindowclippinginmap,
  SpatialSelectWindowClipping,
  SpatialWindowClippingMap );

Operator spatialwindowclippingout (
  "windowclippingout",
  SpatialSpecWindowClippingOut,
  4,
  spatialwindowclippingoutmap,
  SpatialSelectWindowClipping,
  SpatialWindowClippingMap );

Operator spatialscale (
  "scale",
  SpatialSpecScale,
  4,
  spatialscalemap,
  SpatialSelectTranslate,
  SpatialScaleMap );

Operator spatialcomponents (
  "components",
  SpatialSpecComponents,
  3,
  spatialcomponentsmap,
  SpatialComponentsSelect,
  SpatialComponentsMap );

Operator spatialvertices (
  "vertices",
  SpatialSpecVertices,
  2,
  spatialverticesmap,
  SpatialVerticesSelect,
  SpatialVerticesMap);

Operator spatialboundary (
  "boundary",
  SpatialSpecBoundary,
  2,
  spatialboundarymap,
  SpatialBoundarySelect,
  SpatialBoundaryMap);

Operator spatialsimplify (
  "simplify",
  SpatialSpecSimplify,
  2,
  spatialsimplifymap,
  SpatialSimplifySelect,
  SimplifyTypeMap );

Operator spatialatpoint (
  "atpoint",
  SpatialSpecAtPoint,
  SpatialAtPoint,
  Operator::SimpleSelect,
  SpatialAtPointMap );

Operator spatialatposition (
  "atposition",
  SpatialSpecAtPosition,
  SpatialAtPosition,
  Operator::SimpleSelect,
  SpatialAtPositionMap );

Operator spatialsubline (
  "subline",
  SpatialSpecSubLine,
  SpatialSubLine,
  Operator::SimpleSelect,
  SpatialSubLineMap );

Operator spatialgetx (
  "getx",
  SpatialSpecGetX,
  1,
  spatialgetxmap,
  Operator::SimpleSelect,
  SpatialGetXYMap );

Operator spatialgety (
  "gety",
  SpatialSpecGetY,
  1,
  spatialgetymap,
  Operator::SimpleSelect,
  SpatialGetXYMap );

Operator spatialline2region (
  "line2region",
  SpatialSpecLine2Region,
  SpatialLine2Region,
  Operator::SimpleSelect,
  SpatialLine2RegionMap );

Operator spatialrect2region (
    "rect2region",
  SpatialSpecRect2Region,
  SpatialRect2Region,
  Operator::SimpleSelect,
  SpatialRect2RegionMap );

Operator spatialarea (
    "area",
  SpatialSpecArea,
  SpatialArea,
  Operator::SimpleSelect,
  SpatialAreaMap );


Operator spatialpolylines (
  "polylines",
  SpatialSpecPolylines,
  SpatialPolylines<false>,
  Operator::SimpleSelect,
  PolylinesMap );

Operator spatialpolylinesC (
  "polylinesC",
  SpatialSpecPolylinesC,
  SpatialPolylines<true>,
  Operator::SimpleSelect,
  PolylinesMap );

Operator spatialsegments (
  "segments",
  SpatialSpecSegments,
  SpatialSegments,
  Operator::SimpleSelect,
  SegmentsTypeMap );

Operator spatialget (
  "get",
  SpatialSpecGet,
  SpatialGet,
  Operator::SimpleSelect,
  GetTypeMap );

Operator makeline (
    "makeline",
  SpatialSpecMakeLine,
  SpatialMakeLine,
  Operator::SimpleSelect,
  MakeLineTypeMap );

Operator realminize(
     "realminize",           //name
     SpatialSpecRealminize,   //specification
     RealminizeVM, //value mapping
     Operator::SimpleSelect,         //trivial selection function
     RealminizeTypeMap //type mapping
);


Operator union2(
        "union2",     // name
         Union2Spec,   // specification
         sizeof(Union2Map)/sizeof(ValueMapping),  // number of functions
         Union2Map,    // array of value mappings
         SetOpSelect,
         Union2TypeMap
         );


Operator intersection2(
        "intersection2",     // name
         Intersection2Spec,   // specification
         sizeof(Intersection2Map)/sizeof(ValueMapping),  // number of functions
         Intersection2Map,    // array of value mappings
         SetOpSelect,
         Intersection2TypeMap
         );

Operator difference2(
        "difference2",     // name
         Difference2Spec,   // specification
         sizeof(Difference2Map)/sizeof(ValueMapping),  // number of functions
         Difference2Map,    // array of value mappings
         SetOpSelect,
         Difference2TypeMap
         );


Operator commonborder2(
         "commonborder2",           //name
          CommonBorder2Spec,   //specification
          CommonBorder2VM, //value mapping
          Operator::SimpleSelect,         //trivial selection function
          CommonBorder2TypeMap //type mapping
);


Operator spatialtoline (
  "toline",
  SpatialSpecToLine,
  toLineVM,
  Operator::SimpleSelect,
  toLineTypeMap );

Operator spatialfromline (
  "fromline",
  SpatialSpecFromLine,
  fromLineVM,
  Operator::SimpleSelect,
  fromLineTypeMap );

Operator spatialiscycle (
  "iscycle",
  SpatialSpecIsCycle,
  isCycleVM,
  Operator::SimpleSelect,
  isCycleTypeMap );

/*
11 Creating the Algebra

*/

class SpatialAlgebra : public Algebra
{
 public:
  SpatialAlgebra() : Algebra()
  {
    AddTypeConstructor( &point );
    AddTypeConstructor( &points );
    AddTypeConstructor( &line );
    AddTypeConstructor( &region );

    AddTypeConstructor( &sline);

    point.AssociateKind("DATA");
    points.AssociateKind("DATA");
    line.AssociateKind("DATA");
    region.AssociateKind("DATA");
    sline.AssociateKind("DATA");

    point.AssociateKind("SPATIAL2D");
    points.AssociateKind("SPATIAL2D");
    line.AssociateKind("SPATIAL2D");
    region.AssociateKind("SPATIAL2D");
    sline.AssociateKind("SPATIAL2D");


    point.AssociateKind("SHPEXPORTABLE");
    points.AssociateKind("SHPEXPORTABLE");
    line.AssociateKind("SHPEXPORTABLE");
    region.AssociateKind("SHPEXPORTABLE");

    AddOperator( &spatialisempty );
    AddOperator( &spatialequal );
    AddOperator( &spatialnotequal );
    AddOperator( &spatialintersects );
    AddOperator( &spatialinside );
    AddOperator( &spatialadjacent );
    AddOperator( &spatialoverlaps );
    AddOperator( &spatialonborder );
    AddOperator( &spatialininterior );
    AddOperator( &spatialintersection );
    AddOperator( &spatialminus );
    AddOperator( &spatialunion );
    AddOperator( &spatialcrossings );
    AddOperator( &spatialtouchpoints);
    AddOperator( &spatialcommonborder);
    AddOperator( &spatialsingle );
    AddOperator( &spatialdistance );
    AddOperator( &spatialdirection );
    AddOperator( &spatialnocomponents );
    AddOperator( &spatialnosegments );
    AddOperator( &spatialsize );
    AddOperator( &spatialbbox);
    AddOperator( &spatialtranslate );
    AddOperator( &spatialrotate );
    AddOperator( &spatialcenter );
    AddOperator( &spatialconvexhull );
    AddOperator( &spatialwindowclippingin );
    AddOperator( &spatialwindowclippingout );
    AddOperator( &spatialcomponents );
    AddOperator( &spatialvertices );
    AddOperator( &spatialboundary );
    AddOperator( &spatialscale );
    AddOperator( &spatialatpoint );
    AddOperator( &spatialatposition );
    AddOperator( &spatialsubline );
    AddOperator( &spatialadd );
    AddOperator( &spatialgetx );
    AddOperator( &spatialgety );
    AddOperator( &spatialline2region );
    AddOperator( &spatialrect2region );
    AddOperator( &spatialarea );
    AddOperator( &spatialpolylines);
    AddOperator( &spatialpolylinesC);
    AddOperator( &spatialsegments );
    AddOperator( &spatialget );
    AddOperator( &spatialsimplify);
    AddOperator( &realminize);
    AddOperator( &makeline);
    AddOperator(&union2);
    AddOperator(&intersection2);
    AddOperator(&difference2);
    AddOperator(&commonborder2);
    AddOperator(&spatialtoline);
    AddOperator(&spatialfromline);
    AddOperator(&spatialiscycle);
  }
  ~SpatialAlgebra() {};
};


/*
12 Initialization

Each algebra module needs an initialization function. The algebra manager
has a reference to this function if this algebra is included in the list
of required algebras, thus forcing the linker to include this module.

The algebra manager invokes this function to get a reference to the instance
of the algebra class and to provide references to the global nested list
container (used to store constructor, type, operator and object information)
and to the query processor.

The function has a C interface to make it possible to load the algebra
dynamically at runtime.

*/

extern "C"
Algebra*
InitializeSpatialAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return (new SpatialAlgebra());
}




