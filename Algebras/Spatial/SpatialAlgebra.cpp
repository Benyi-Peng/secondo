/*
//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]

[1] Implementation of the Spatial Algebra

February, 2003. Victor Teixeira de Almeida

March, 2003. Zhiming Ding

1 Overview

This implementation file essentially contains the implementation of the classes ~Point~, 
~Points~, ~Line~, and ~Region~ used in the Spatial Algebra. These classes
respectively correspond to the memory representation for the type constructors
~point~, ~points~, ~line~, and ~region~.  

For more detailed information see SpatialAlgebra.h.

2 Defines and Includes

*/

using namespace std;

#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include "SpatialAlgebra.h"

static NestedList* nl;
static QueryProcessor* qp;

/*
3 Type investigation auxiliaries

Within this algebra module, we have to handle with values of four different
types: ~point~ and ~points~, ~line~ and ~region~.

Later on we will
examine nested list type descriptions. In particular, we
are going to check whether they describe one of the four types just introduced.
In order to simplify dealing with list expressions describing these types, we
declare an enumeration, ~SpatialType~, containing the four types, and a function,
~TypeOfSymbol~, taking a nested list as argument and returning the
corresponding ~SpatialType~ type name.

*/

enum SpatialType { stpoint, stpoints, stline, stregion, sterror };

static SpatialType
TypeOfSymbol( ListExpr symbol )
{
  if ( nl->AtomType( symbol ) == SymbolType )
  {
    string s = nl->SymbolValue( symbol );
    if ( s == "point"  ) return (stpoint);
    if ( s == "points" ) return (stpoints);
    if ( s == "line"   ) return (stline);
    if ( s == "region" ) return (stregion);
  }
  return (sterror);
}

/*
4 Type Constructor ~point~

A value of type ~point~ represents a point in the Euclidean plane or is undefined.

4.1 Implementation of the class ~Point~

*/
Point::Point( const bool d, const Coord& x, const Coord& y ) :
  x( x ),
  y( y ),
  defined( d )
{}

Point::Point( const Point& p ) :
  defined( p.IsDefined() )
{
  if( defined )
  {
    x = p.GetX();
    y = p.GetY();
  }
}

Point::~Point() 
{}

const Coord& Point::GetX() const 
{
  assert( IsDefined() );
  return x;
}

const Coord& Point::GetY() const
{
  assert( IsDefined() );
  return y;
}

const bool Point::IsDefined() const
{
  return defined;
}

Point& Point::operator=( const Point& p )
{
  assert( p.IsDefined() );
  defined = true;
  x = p.GetX();
  y = p.GetY();

  return *this;
}

void Point::SetDefined( const bool d )
{
  defined = d;
}

int Point::operator==( const Point& p ) const
{
  assert( IsDefined() && p.IsDefined() );
  return x == p.GetX() && y == p.GetY();
}

int Point::operator!=( const Point& p ) const
{
  assert( IsDefined() && p.IsDefined() );
  return x != p.GetX() || y == p.GetY();
}

int Point::operator<=( const Point& p ) const
{
  assert( IsDefined() && p.IsDefined() );
  if( x < p.GetX() )
    return 1;
  else if( x == p.GetX() && y <= p.GetY() )
    return 1;
  return 0;
}

int Point::operator<( const Point& p ) const
{
  assert( IsDefined() && p.IsDefined() );
  if( x < p.GetX() )
    return 1;
  else if( x == p.GetX() && y < p.GetY() )
    return 1;
  return 0;
}

int Point::operator>=( const Point& p ) const
{
  assert( IsDefined() && p.IsDefined() );
  if( x > p.GetX() )
    return 1;
  else if( x == p.GetX() && y >= p.GetY() )
    return 1;
  return 0;
}

int Point::operator>( const Point& p ) const
{
  assert( IsDefined() && p.IsDefined() );
  if( x > p.GetX() )
    return 1;
  else if( x == p.GetX() && y > p.GetY() )
    return 1;
  return 0;
}

ostream& operator<<( ostream& o, const Point& p )
{
  if( p.IsDefined() )
    o << "(" << p.GetX() << ", " << p.GetY() << ")";
  else
    o << "undef";

  return o;
}

const bool Point::Inside( const Points& ps ) const
{
  assert( IsDefined() && ps.IsOrdered() );

  return ps.Contains( *this );
}

void Point::Intersection( const Point& p, Point& result ) const
{
  assert( IsDefined() && p.IsDefined() );

  if( *this == p )
    result = *this;
  else
    result.SetDefined( false );
}

void Point::Intersection( const Points& ps, Point& result ) const
{
  assert( IsDefined() );

  if( this->Inside( ps ) )
    result = *this;
  else
    result.SetDefined( false );
}

void Point::Minus( const Point& p, Point& result ) const
{
  assert( IsDefined() && p.IsDefined() );

  if( *this == p )
    result.SetDefined( false );
  else
    result = *this;
}

void Point::Minus( const Points& ps, Point& result ) const
{
  assert( IsDefined() );

  if( this->Inside( ps ) )
    result.SetDefined( false );
  else
    result = *this;
}


/*
4.2 List Representation

The list representation of a point is

----	(x y)
----

4.3 ~Out~-function

*/
static ListExpr
OutPoint( ListExpr typeInfo, Word value )
{
  Point* point = (Point*)(value.addr);
  if( point->IsDefined() )
  {
    return nl->TwoElemList(
             point->GetX().IsInteger() ? nl->IntAtom( point->GetX().IntValue() ) : nl->RealAtom( point->GetX().Value() ), 
             point->GetY().IsInteger() ? nl->IntAtom( point->GetY().IntValue() ) : nl->RealAtom( point->GetY().Value() ) );
  }
  else
  {
    return (nl->SymbolAtom("undef"));
  }
}

/*
4.4 ~In~-function

*/
static Word
InPoint( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct )
{
  Point* newpoint;

  if ( nl->ListLength( instance ) == 2 )
  { 
    ListExpr First = nl->First(instance); 
    ListExpr Second = nl->Second(instance);

    if ( nl->IsAtom(First) && nl->IsAtom(Second) )
    {
      Coord x, y;

      correct = true;
      if( nl->AtomType(First) == IntType )
        x = nl->IntValue(First);
      else if( nl->AtomType(First) == RealType )
        x = nl->RealValue(First);
      else
        correct = false; 

      if( nl->AtomType(Second) == IntType )
        y = nl->IntValue(Second);
      else if( nl->AtomType(Second) == RealType )
        y = nl->RealValue(Second);
      else
        correct = false; 

      if( correct )
      {
        newpoint = new Point(true, x, y);
        return SetWord(newpoint);
      }
    }
  }
  correct = false;
  return SetWord(Address(0));
}

/*
4.5 ~Create~-function

*/
static Word
CreatePoint( const ListExpr typeInfo ) 
{
  return (SetWord( new Point() ));
}

/*
4.6 ~Delete~-function

*/
static void
DeletePoint( Word& w ) 
{
  delete (Point *)w.addr;
  w.addr = 0;
}
 
/*
4.7 ~Close~-function

*/
static void
ClosePoint( Word& w ) 
{
  delete (Point *)w.addr;
  w.addr = 0;
}
 
/*
4.8 ~Clone~-function

*/
static Word
ClonePoint( const Word& w ) 
{
  assert( ((Point *)w.addr)->IsDefined() ); 
  Point *p = new Point( *((Point *)w.addr) );
  return SetWord( p );
}
 
/*
4.9 Function describing the signature of the type constructor

*/
static ListExpr
PointProperty()
{
  return (nl->TwoElemList(
		nl->TheEmptyList(),
		nl->SymbolAtom("SPATIAL") ));
}

/*
4.10 Kind Checking Function

This function checks whether the type constructor is applied correctly. Since
type constructor ~point~ does not have arguments, this is trivial.

*/
static bool
CheckPoint( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "point" ));
}

/*
4.11 ~Cast~-function

*/
void* CastPoint(void* addr)
{
  return ( 0 );
}

/*
4.12 Creation of the type constructor instance

*/
TypeConstructor point(
	"point",			//name		
	PointProperty,			//property function describing signature
        OutPoint,   	InPoint,	//Out and In functions
	CreatePoint,	DeletePoint,	//object creation and deletion
        0, 0, ClosePoint, ClonePoint,   //object open, save, close, and clone
	CastPoint,			//cast function
	CheckPoint,	                //kind checking function
	0, 				//predef. pers. function for model
        TypeConstructor::DummyInModel, 	
        TypeConstructor::DummyOutModel,
        TypeConstructor::DummyValueToModel,
        TypeConstructor::DummyValueListToModel );


/*
5 Type Constructor ~points~

A ~points~ value is a finite set of points. 

5.1 Implementation of the class ~Points~

*/
Points::Points( SmiRecordFile *recordFile ) :
  points( new PArray<Point>( recordFile ) ),
  ordered( true )
{}

Points::Points( SmiRecordFile *recordFile, const Points& ps ) :
  points( new PArray<Point>( recordFile ) ), 
  ordered( true )
{
  assert( ps.IsOrdered() );

  for( int i = 0; i < ps.Size(); i++ )
  {
    Point p;
    ps.Get( i, p );
    points->Put( i, p ); 
  }
}

Points::Points( SmiRecordFile *recordFile, const SmiRecordId recordId, bool update ):
  points( new PArray<Point>( recordFile, recordId, update ) ),
  ordered( true )
  {}

void Points::Destroy()
{
  points->MarkDelete();
}

Points::~Points() 
{
  delete points;
}

void Points::Get( const int i, Point& p ) const
{
  assert( i >= 0 && i < Size() );

  points->Get( i, p );
}

const int Points::Size() const 
{
  return points->Size();
}

const bool Points::IsEmpty() const
{
  return points->Size() == 0;
}

/*
The following 5 functions, SelectFirst(), SelectNext(), EndOfPt(), GetPt(), InsertPt() are added by DZM
as a basis for object traversal operations 
  
*/

void Points::SelectFirst()
{
    if (IsEmpty()) pos=-1;
    else pos=0;
}

void Points::SelectNext()
{
    if ((pos>=0) && (pos<Size()-1)) pos++;
    else pos=-1;
}

bool Points::EndOfPt()
{
    return (pos==-1);
}

void Points::GetPt( Point& p )
{
    if (( pos>=0) && (pos<=Size()-1)) points->Get( pos, p);
    else p.SetDefined(false);
}
 
void Points::InsertPt( Point& p )
{
    assert(p.IsDefined());

    if( !IsOrdered() )
    {
	pos=points->Size();
	points->Put( points->Size(), p );
    }
    else
    {
	int insertpos = Position( p );
	if( insertpos != -1 )
	{
	    for( int i = points->Size() - 1; i >= insertpos; i++ )
	    {
		Point auxp;
		points->Get( i, auxp );
		points->Put( i+1, auxp );
	    }
	    points->Put( insertpos, p );
	    pos=insertpos;
	}
    }
}

const int Points::Position( const Point& p ) const
{
  assert( IsOrdered() && p.IsDefined() );

  int first = 0, last = Size();

  while (first <= last) 
  {
    int mid = ( first + last ) / 2;  
    Point midPoint;
    points->Get( mid, midPoint );
    if( p > midPoint ) 
      first = mid + 1;  
    else if( p < midPoint ) 
      last = mid - 1; 
    else
      return mid;
   }
   return -1;
}

const SmiRecordId Points::GetPointsRecordId() const
{
  return points->Id();
}

Points& Points::operator=( const Points& ps )
{
  assert( ps.IsOrdered() );

  points->MarkDelete();
  delete points;
  points = new PArray<Point>(SecondoSystem::GetLobFile());  //$$$$$$$$$$$$$$$$$$$$
  for( int i = 0; i < ps.Size(); i++ )
  {
    Point p;
    ps.Get( i, p );
    points->Put( i, p );
  }
  ordered = true;
  return *this;
}

void Points::StartBulkLoad()
{
  assert( IsOrdered() );
  ordered = false;
}

void Points::EndBulkLoad()
{
  assert( !IsOrdered() );
  cout << "Before sorting: " << *this << endl;
  Sort();
  cout << "After sorting: " << *this << endl;
  ordered = true;
}

const bool Points::IsOrdered() const
{
  return ordered;
}

int Points::operator==( const Points& ps ) const
{
  assert( IsOrdered() && ps.IsOrdered() );

  if( Size() != ps.Size() )
    return 0;

  for( int i = 0; i < Size(); i++ )
  {
    Point p1, p2;
    points->Get( i, p1 );
    ps.Get( i, p2 );
    if( p1 != p2 )
      return 0;
  }
  return 1;
}

int Points::operator!=( const Points& ps ) const
{
  assert( IsOrdered() && ps.IsOrdered() );

  return !( *this == ps );
}

Points& Points::operator+=(const Point& p)
{
  assert( p.IsDefined() );

  if( !IsOrdered() )
  {
    points->Put( points->Size(), p );
  }
  else
  {
    int pos = Position( p );
    if( pos != -1 )
    {
      for( int i = points->Size() - 1; i >= pos; i++ )
      {
        Point auxp;
        points->Get( i, auxp );
        points->Put( i+1, auxp );
      }
      points->Put( pos, p );
    }
  }
  return *this;
}

Points& Points::operator+=(const Points& ps)
{
  for( int i = 0; i < ps.Size(); i++ )
  {
    Point p;
    ps.Get( i, p );
    points->Put( points->Size(), p );
  }
  if( IsOrdered() )
  {
    ordered = false;
    Sort();
  }

  return *this;
}

Points& Points::operator-=(const Point& p)
{
  assert( IsOrdered() && p.IsDefined() );

  int pos = Position( p );
  if( pos != -1 )
  {
    for( int i = pos; i < Size(); i++ )
    {
      Point auxp;
      points->Get( i+1, auxp );
      points->Put( i, auxp );
    }
  }
  return *this;
}

ostream& operator<<( ostream& o, const Points& ps )
{
  o << "(" << ps.GetPointsRecordId() << ") <";
  for( int i = 0; i < ps.Size(); i++ )
  {
    Point p;
    ps.Get( i, p );
    o << " " << p; 
  }
  o << " >";

  return o;
}

void Points::Sort()
{
  assert( !IsOrdered() );

  if( Size() > 1 ) 
  {
    int low = 0, high = Size() - 1;
    QuickSortRecursive( low, high );
  }
}

void Points::QuickSortRecursive( const int low, const int high )
{
  int i = high, j = low;
  Point p, pj, pi;

  Get( (int)( (low + high) / 2 ), p );

  do 
  {
    Get( j, pj );
    while( pj < p ) 
      Get( ++j, pj );

    Get( i, pi );
    while( pi > p ) 
      Get( --i, pi );

    if( i >= j ) 
    {
      if ( i != j ) 
      {
        points->Put( i, pj );
        points->Put( j, pi );
      }

      i--;
      j++;
    }
  } while( j <= i );

  if( low < i ) 
    QuickSortRecursive( low, i );
  if( j < high ) 
    QuickSortRecursive( j, high );
}

const bool Points::Contains( const Point& p ) const
{
  assert( IsOrdered() && p.IsDefined() );

  if( IsEmpty() )
    return false;

  int first = 0, last = Size() - 1;

  while (first <= last) 
  {
    int mid = ( first + last ) / 2;  
    Point midPoint;
    points->Get( mid, midPoint );
    if( p > midPoint ) 
      first = mid + 1;  
    else if( p < midPoint ) 
      last = mid - 1; 
    else
      return true;
   }
   return false;
}

const bool Points::Contains( const Points& ps ) const
{
  assert( IsOrdered() && ps.IsOrdered() );

  if( ps.IsEmpty() )
    return true;
  if( IsEmpty() )
    return false;

  Point p1, p2;
  int i = 0, j = 0;

  Get( i, p1 );
  ps.Get( j, p2 );
  while( true )
  {
    if( p1 == p2 )
    {
      if( ++j == ps.Size() )
        return true;
      ps.Get( j, p2 );
      if( ++i == Size() )
        return false;
      Get( i, p1 );
    }
    else if( p1 < p2 )
    {
      if( ++i == Size() )
        return false;
      Get( i, p1 );
    }
    else // p1 > p2 
    {
      return false;
    }
  }
  // This part of the code should never be reached.LineSeg
  assert( true );
  return true;
}

const bool Points::Inside( const Points& ps ) const
{
  assert( IsOrdered() && ps.IsOrdered() );

  return ps.Contains( *this );
}

const bool Points::Intersects( const Points& ps ) const
{
  assert( IsOrdered() && ps.IsOrdered() );

  if( IsEmpty() || ps.IsEmpty() )
    return false;

  Point p1, p2;
  int i = 0, j = 0;

  Get( i, p1 );
  ps.Get( j, p2 );

  while( 1 )
  {
    if( p1 == p2 )
      return true;
    if( p1 < p2 )
    {
      if( ++i == Size() )
        return false;
      Get( i, p1 );
    }
    else // p1 > p2 
    {
      if( ++j == ps.Size() )
        return false;
      ps.Get( j, p2 );
    }
  }
  // this part of the code should never be reached
  assert( false );
  return false;
}

/*
5.2 List Representation

The list representation of a point is

----	(x y)
----

5.3 ~Out~-function

*/
static ListExpr
OutPoints( ListExpr typeInfo, Word value )
{
  cout << "OutPoints" << endl;

  Points* points = (Points*)(value.addr);
  if( points->IsEmpty() )
  {
    return (nl->TheEmptyList());
  }
  else
  {
    Point p;
    points->Get( 0, p );
    ListExpr result = nl->OneElemList( OutPoint( nl->TheEmptyList(), SetWord( &p ) ) );
    ListExpr last = result;

    for( int i = 1; i < points->Size(); i++ )
    {
      points->Get( i, p );
      last = nl->Append( last,
                         OutPoint( nl->TheEmptyList(), SetWord( &p ) ) );
    }  

    return result;
  }
}

/*
5.4 ~In~-function

*/
static Word
InPoints( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct )
{
  cout << "InPoints" << endl;

  Points* points = new Points( SecondoSystem::GetLobFile() );
  points->StartBulkLoad();

  ListExpr rest = instance;
  while( !nl->IsEmpty( rest ) )
  {
    ListExpr first = nl->First( rest );
    rest = nl->Rest( rest );

    Point *p = (Point*)InPoint( nl->TheEmptyList(), first, 0, errorInfo, correct ).addr;
    if( correct ) 
    {
      (*points) += (*p);
      delete p;
    }
    else
    {
      return SetWord( Address(0) );
    }
  }
  points->EndBulkLoad();
  correct = true;
  return SetWord( points );
}

/*
5.5 ~Create~-function

*/
static Word
CreatePoints( const ListExpr typeInfo ) 
{
  cout << "CreatePoints" << endl;

  return (SetWord( new Points( SecondoSystem::GetLobFile() ) ));
}

/*
5.6 ~Delete~-function

*/
static void
DeletePoints( Word& w ) 
{
  cout << "DeletePoints" << endl;

  Points *ps = (Points *)w.addr;
  ps->Destroy();
  delete ps;
  w.addr = 0;
}
 
/*
5.7 ~Close~-function

*/
static void
ClosePoints( Word& w ) 
{
  cout << "ClosePoints" << endl;

  delete (Points *)w.addr;
  w.addr = 0;
}
 
/*
5.8 ~Clone~-function

*/
static Word
ClonePoints( const Word& w ) 
{
  cout << "ClonePoints" << endl;

  Points *p = new Points(  SecondoSystem::GetLobFile(), *((Points *)w.addr) );
  return SetWord( p );
}
 
/*
5.9 ~Open~-function

*/
bool
OpenPoints( SmiRecord& valueRecord,
            const ListExpr typeInfo,
            Word& value )
{
  cout << "OpenPoints" << endl;

  SmiRecordId recordId;

  valueRecord.Read( &recordId, sizeof( SmiRecordId ), 0 );
  Points *points = new Points( SecondoSystem::GetLobFile(), recordId );
  value = SetWord( points );

  cout << "OpenPoints: " << *points << endl;
  return (true);
}

/*
5.10 ~Save~-function

*/
bool
SavePoints( SmiRecord& valueRecord,
            const ListExpr typeInfo,
            Word& value )
{
  cout << "SavePoints" << endl;

  Points *points = (Points*)value.addr;

  cout << "SavePoints: " << *points << endl;

  SmiRecordId recordId = points->GetPointsRecordId();

  valueRecord.Write( &recordId, sizeof( SmiRecordId ), 0 );

  return (true);
}

/*
5.11 Function describing the signature of the type constructor

*/
static ListExpr
PointsProperty()
{
  return (nl->TwoElemList(
		nl->TheEmptyList(),
		nl->SymbolAtom("SPATIAL") ));
}

/*
5.12 Kind checking function

This function checks whether the type constructor is applied correctly. Since
type constructor ~point~ does not have arguments, this is trivial.

*/
static bool
CheckPoints( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "points" ));
}

/*
5.13 ~Cast~-function

*/
void* CastPoints(void* addr)
{
  return ( 0 );
}

/*
5.14 Creation of the type constructor instance

*/
TypeConstructor points(
	"points",			//name		
	PointsProperty, 		//property function describing signature
        OutPoints,   	InPoints,	//Out and In functions
	CreatePoints,	DeletePoints,	//object creation and deletion
        OpenPoints, 	SavePoints,    	// object open and save
        ClosePoints, 	ClonePoints,   	//object close and clone
	CastPoints,			//cast function
	CheckPoints,	                //kind checking function
	0, 				//predef. pers. function for model
        TypeConstructor::DummyInModel, 	
        TypeConstructor::DummyOutModel,
        TypeConstructor::DummyValueToModel,
        TypeConstructor::DummyValueListToModel );

/*
6 Type Constructor ~halfsegment~

A ~halfsegment~ value is a pair of points, with a boolean flag indicating the dominating point .

6.1 Implementation of the class ~halfsegment~

*/

CHalfSegment::CHalfSegment(bool Defined, bool LDP, Point& P1, Point& P2)
{
   defined = Defined;
   ldp = LDP;
   if (P1<P2) 
   {
       lp = P1;
       rp = P2;
   }
   else if (P1>P2) 
   {
       lp = P2;
       rp = P1;
   }
   else cout <<"incorrect segment value!"<<endl;
}


CHalfSegment::CHalfSegment( const CHalfSegment& chs )
{
   defined = chs.IsDefined();
   ldp = chs.GetLDP();
   lp = chs.GetLP();
   rp = chs.GetRP();
   attr=chs.GetAttr();
}

CHalfSegment::CHalfSegment()
{
}

CHalfSegment::~CHalfSegment() 
{
}

const bool CHalfSegment::IsDefined() const  
{
    return (defined); 
}
  
const Point&  CHalfSegment::GetLP() const 
{
    return lp;
}

const Point&  CHalfSegment::GetRP() const 
{
    return rp;
}

const Point&  CHalfSegment::GetDPoint() const 
{
    assert(IsDefined());
    if (ldp==true) return lp;
    else return rp;
}

const Point&  CHalfSegment::GetSPoint() const 
{
    assert(IsDefined());
    if (ldp==true) return rp;
    else return lp;
}

const bool CHalfSegment::GetLDP() const
{
    assert(IsDefined());
    return ldp;
}

void     CHalfSegment::SetLDP(bool LDP)
{
    assert(IsDefined());
    ldp=LDP;
}

const attrtype&  CHalfSegment::GetAttr() const
{
    assert(IsDefined());
    return attr;
}
	
void CHalfSegment::SetDefined(bool Defined) 
{
    defined = Defined;
}

void  CHalfSegment::Set(bool Defined,  bool LDP, Point& P1, Point& P2) 
{
    defined = Defined;
    ldp = LDP;
    if (P1<P2) 
    {
	lp = P1;
	rp = P2;
    }
    else if (P1>P2) 
    {
	lp = P2;
	rp = P1;
    }
    else cout <<"incorrect segment value!"<<endl;
}

void    CHalfSegment::SetAttr(attrtype& ATTR)
{
    attr=ATTR;
}

CHalfSegment& CHalfSegment::operator=(const CHalfSegment& chs)
{
  assert( chs.IsDefined() );
  defined = true;
  ldp = chs.GetLDP();
  lp = chs.GetLP();
  rp = chs.GetRP();
  attr = chs.GetAttr();
  return *this;
}

/*
The following function compares two halfsegments, a and b.  if a \verb+<+ b, then -1 is returned;
if a \verb+>+ b then 1 is returned, if a=b then 0 is returned. The order of halfsegments follows the
rules given in the paper about the ROSE algebra implementation. 

*/

int CHalfSegment::chscmp(const CHalfSegment& chs) const
{ 
    //cout<<"Comparing!!!"<<*this<<" and "<<chs<<endl; 
    if (!IsDefined() && !(chs.IsDefined()))  return 0;
    else if (!IsDefined())  return -1;
    else  if (!(chs.IsDefined())) return 1;
    else  // both defined
    {
	Point dp, sp, DP, SP;
	dp=GetDPoint(); sp=GetSPoint();
	DP=chs.GetDPoint(); SP=chs.GetSPoint();
	
	if (dp < DP) return -1;
	else if (dp > DP) return 1;
	else 	
	{   //Dominating Points are equal. then compare the flag
	    if (ldp!=chs.GetLDP())
	    {           //if flages are not equal
		if (ldp==false) return -1; 
		else return 1;
	    }
	    else  	//flags are equal : compute the rotation. 
	    {      	
		if ((dp.GetX()==sp.GetX()) && (DP.GetX()==SP.GetX()))
		{  //both lines are vertical lines
		    if (((sp.GetY()>dp.GetY()) && (SP.GetY()>DP.GetY())) ||
		        ((sp.GetY()<dp.GetY()) && (SP.GetY()<DP.GetY())))
		    {
			if (GetSPoint()<chs.GetSPoint()) return -1;
			else if (GetSPoint()>chs.GetSPoint()) return 1;
			else return 0;
		    }
		    else if (sp.GetY()>dp.GetY()) 
		    {
			if (ldp==true) return 1;
			else return -1;
		    }
		    else
		    {
			if (ldp==true) return -1;
			else return 1;
		    }
		}
		else if (dp.GetX()==sp.GetX())
		{  //only this is vertical
		    if (sp.GetY()>dp.GetY()) 
		    {
			if (ldp==true) return 1;
			else return -1;
		    }
		    else if  (sp.GetY()<dp.GetY()) 
		    {
			if (ldp==true) return -1;
			else return 1;
		    }
		    else cout<<"two end points are identical!";
		}
		else if (DP.GetX()==SP.GetX())
		{   //only arg is vertical
    		    if (SP.GetY()>DP.GetY()) 
		    {
			if (ldp==true) return -1;
			else return 1;
		    }
		    else if  (SP.GetY()<DP.GetY()) 
		    {
			if (ldp==true) return 1;
			else return -1;
		    }
		    else cout<<"two end points are identical!";
		}
		else  //both are none-vertical
		{
		    Coord xd,yd,xs,ys;
		    Coord Xd,Yd,Xs,Ys;
		    xd=dp.GetX();  yd=dp.GetY(); 
		    xs=sp.GetX();  ys=sp.GetY();
		    Xd=DP.GetX();  Yd=DP.GetY(); 
		    Xs=SP.GetX();  Ys=SP.GetY();    
		    double k=
		    ((yd.IsInteger()? yd.IntValue():yd.Value()) -
		     (ys.IsInteger()? ys.IntValue():ys.Value())) / 
		    ((xd.IsInteger()? xd.IntValue():xd.Value()) -
		     (xs.IsInteger()? xs.IntValue():xs.Value())) ;
		    double K=
		    ((Yd.IsInteger()? Yd.IntValue():Yd.Value()) -
		     (Ys.IsInteger()? Ys.IntValue():Ys.Value())) / 
		    ((Xd.IsInteger()? Xd.IntValue():Xd.Value()) -
		     (Xs.IsInteger()? Xs.IntValue():Xs.Value())); 
		if (k<K) return -1;
		else if (k>K) return 1;
		else //k==K then compare the secondary point
		{
		    if (GetSPoint()<chs.GetSPoint()) return -1;
		    else if (GetSPoint()>chs.GetSPoint()) return 1;
		    else return 0;
		}
	             }
	    }
	}
    }
    cout<<"it shouldn't reach here!"<<endl;
    return -2;
}

int CHalfSegment::operator==(const CHalfSegment& chs) const
{
    return (chscmp(chs)==0);
}

int CHalfSegment::operator<(const CHalfSegment& chs) const
{
    return (chscmp(chs)==-1);
}

int CHalfSegment::operator>(const CHalfSegment& chs) const
{
    return (chscmp(chs)==1);
}

ostream& operator<<(ostream &os, const CHalfSegment& chs)
{
    if( chs.IsDefined())
	return (os << "(" << (chs.GetLDP()? "L":"R") << 
	            ", ("<< chs.GetLP() << ","<< chs.GetRP() <<"))");
    else
         return (os << "undef");
}

const bool CHalfSegment::Intersects( const CHalfSegment& chs ) const
{   //To judge whether two halfsegments intersect
    Coord xl,yl,xr,yr;
    Coord Xl,Yl,Xr,Yr;
    double k, a, K, A; 
    double x0; //, y0;  (x0, y0) is the intersection

    xl=lp.GetX();  yl=lp.GetY();
    xr=rp.GetX();  yr=rp.GetY();
    if (xl!=xr) 
    {   	//k=(yr-yl) / (xr-xl);  a=yl - k*yl;
	k=((yr.IsInteger()? yr.IntValue():yr.Value()) -
	      (yl.IsInteger()? yl.IntValue():yl.Value())) / 
	     ((xr.IsInteger()? xr.IntValue():xr.Value()) -
	      (xl.IsInteger()? xl.IntValue():xl.Value())); 
	a=(yl.IsInteger()? yl.IntValue():yl.Value()) -
	     k*(xl.IsInteger()? xl.IntValue():xl.Value());
    }
    
    Xl=chs.GetLP().GetX();  Yl=chs.GetLP().GetY();
    Xr=chs.GetRP().GetX();  Yr=chs.GetRP().GetY();
    if (Xl!=Xr)
    {    	//K=(Yr-Yl) / (Xr-Xl);  A=Yl - K*Xl;
	K=  ((Yr.IsInteger()? Yr.IntValue():Yr.Value()) -
	        (Yl.IsInteger()? Yl.IntValue():Yl.Value())) / 
 	       ((Xr.IsInteger()? Xr.IntValue():Xr.Value()) -
	        (Xl.IsInteger()? Xl.IntValue():Xl.Value())); 
	A = (Yl.IsInteger()? Yl.IntValue():Yl.Value()) -
	       K*(Xl.IsInteger()? Xl.IntValue():Xl.Value());
    }

    if ((xl==xr) && (Xl==Xr)) //both *this and *arg are vertical lines
      {
	  if (xl!=Xl) return false;
	  else   if  (((yl>=Yl) && (yl<=Yr)) ||
		  ((yr>=Yl) && (yr<=Yr))||
		  ((Yl>=yl) && (Yl<=yr)) ||
		  ((Yr>=yl) && (Yr<=yr)))
	                  return true;
 	             else return false;
      }
    
    if (Xl==Xr)    //only *arg is vertical
    {
	double y0=k*(Xl.IsInteger()? Xl.IntValue():Xl.Value())+a; 
	//(xl, y0) is the intersection of l and L
	if (((y0>=(Yl.IsInteger()? Yl.IntValue():Yl.Value())) && 
	      (y0<=(Yr.IsInteger()? Yr.IntValue():Yr.Value()))) || 
	     ((y0>=(Yr.IsInteger()? Yr.IntValue():Yr.Value())) && 
	      (y0<=(Yl.IsInteger()? Yl.IntValue():Yl.Value())))) 
	         return true;
	else return false;
    }

    if (xl==xr)    //only *this is vertical
    {
	double Y0=K*(xl.IsInteger()? xl.IntValue():xl.Value())+A; 
	//(Xl, Y0) is the intersection of l and L
	if (((Y0>=(yl.IsInteger()? yl.IntValue():yl.Value())) && 
	      (Y0<=(yr.IsInteger()? yr.IntValue():yr.Value()))) || 
  	     ((Y0>=(yr.IsInteger()? yr.IntValue():yr.Value())) && 
	      (Y0<=(yl.IsInteger()? yl.IntValue():yl.Value())))) 
	         return true;
	else return false;
    }
    
    //otherwise: both *this and *arg are non-vertical lines
    if (k==K) 
      {
	  if  (A!=a)  return false; //Parallel lines
	  else //they are in the same straight line
	  {
	      if (((xl>=Xl)&&(xl<=Xr)) || ((Xl>=xl) && (Xl<=xr)))
	              return true;
	      else return false;
	  }
      }
      else
      {
	  x0=(A-a) / (k-K);	 // y0=x0*k+a;
	  if ((x0>=(xl.IsInteger()?xl.IntValue():xl.Value()))   && 
	      (x0<=(xr.IsInteger()?xr.IntValue():xr.Value()))  && 
	      (x0>=(Xl.IsInteger()?Xl.IntValue():Xl.Value())) &&
	      (x0 <=(Xr.IsInteger()?Xr.IntValue():Xr.Value())))
	      return true;
	  else return false;
      }
}

const bool CHalfSegment::Inside(const CHalfSegment& chs) const 
{ //to judge whether *this is part of *arg.  eg. (1 1) (2 2) is inside of (0 0) (5 5)
  assert( IsDefined() && chs.IsDefined() );
  
  Coord xl,yl,xr,yr;
  Coord Xl,Yl,Xr,Yr;
  double k, a, K, A; 
  
  xl=lp.GetX();  yl=lp.GetY();
  xr=rp.GetX();  yr=rp.GetY();
  if (xl!=xr) 
  {   // k=(yr-yl) / (xr-xl);  a=yl - k*yl;
      k=((yr.IsInteger()? yr.IntValue():yr.Value()) -
           (yl.IsInteger()? yl.IntValue():yl.Value())) / 
          ((xr.IsInteger()? xr.IntValue():xr.Value()) -
           (xl.IsInteger()? xl.IntValue():xl.Value())); 
      a=(yl.IsInteger()? yl.IntValue():yl.Value()) -
           k*(xl.IsInteger()? xl.IntValue():xl.Value());
  }
    
  Xl=chs.GetLP().GetX();  Yl=chs.GetLP().GetY();
  Xr=chs.GetRP().GetX();  Yr=chs.GetRP().GetY();
  if (Xl!=Xr)
  {   // K=(Yr-Yl) / (Xr-Xl);  A=Yl - K*Xl;
      K=  ((Yr.IsInteger()? Yr.IntValue():Yr.Value()) -
              (Yl.IsInteger()? Yl.IntValue():Yl.Value())) /
             ((Xr.IsInteger()? Xr.IntValue():Xr.Value()) -
              (Xl.IsInteger()? Xl.IntValue():Xl.Value())); 
      A = (Yl.IsInteger()? Yl.IntValue():Yl.Value()) -
              K*(Xl.IsInteger()? Xl.IntValue():Xl.Value());
    }
  
  if ((Xl==Xr) && (xl==xr))  //both are vertical lines
  {  
      if (xl==Xl) // then they are in a straight line
      {
	  if  (((yl>=Yl) && (yl<=Yr)) && ((yr>=Yl) && (yr<=Yr)))
	          return true;
	  else return false; 
      }
  }
  else if ((Xl!=Xr) && (xl!=xr) && (K==k) && (A==a)) //both are non-vertical lines and they are in the same straight line
               {
	  if ((xl>=Xl) && (xr<=Xr)) return true;
               }
  
  return false;
}

const bool CHalfSegment::Contains( const Point& p ) const
{
  assert( p.IsDefined() );

  if( !IsDefined() )
    return false;

  Coord xl,yl,xr,yr;
  Coord X,Y;
    
  xl=lp.GetX();  yl=lp.GetY();
  xr=rp.GetX();  yr=rp.GetY();

  X=p.GetX(); Y=p.GetY();
  
  if (((Y-yl)*(xr-xl) ==(yr-yl)*(X-xl)) && (X>=xl) && (X <=xr))
             return true;
  else    return false;
}

/*

6.2 List Representation

The list representation of a HalfSegment is

----	( bool, (Point1 Point2))  for instance: ( true ((1 1) (2 2)) )
----

where the bool value indicate whether the dominating point is the left point.

6.3 ~In~ and ~Out~ Functions

*/

static ListExpr
OutHalfSegment( ListExpr typeInfo, Word value ) 
{
  CHalfSegment* chs;
  chs = (CHalfSegment*)(value.addr);
  if (chs->IsDefined())
  {
    //cout<<"OUT Function"<<*ls<<endl;
    Point LP, RP;
    LP = chs->GetLP();
    RP = chs->GetRP();
    
    return (nl->TwoElemList( nl-> BoolAtom(chs->GetLDP()), 
                nl->TwoElemList( OutPoint( nl->TheEmptyList(), SetWord( &LP)),
                OutPoint( nl->TheEmptyList(), SetWord( &RP)))));
  }
  else
  {
    return (nl->SymbolAtom("undef"));
  }
}

static Word
InHalfSegment( const ListExpr typeInfo, const ListExpr instance, const int errorPos, ListExpr& errorInfo, bool& correct )
{
    CHalfSegment* chs;
    ListExpr First, Second, FirstP, SecondP;
    Point *LP, *RP;
    bool LDP;
  
    if (nl->IsAtom(instance))
    {
	if ( nl->AtomType(instance)==SymbolType && nl->SymbolValue(instance)=="undef")
	{
	    correct = true;
	    chs = new CHalfSegment();
	    chs->SetDefined(false);
	    return SetWord(chs);
	}
	else 
	{
	    correct = false;
	    return SetWord(Address(0));
	}
    }
  
    if ( nl->ListLength( instance ) == 2 )  
    {	//in the format of (t/f  ((x1 y1) (x2 y2)) )
	First=nl->First(instance);
	Second=nl->Second(instance);
      
	if (nl->IsAtom(First) && nl->AtomType(First)==BoolType)
	    LDP =  nl->BoolValue(First);
	else       
	{
		correct = false;
		return SetWord(Address(0));
	}
	  
	if (nl->ListLength(Second)==2)
	{
		FirstP = nl->First(Second);
		SecondP = nl->Second(Second);
	}
	else
	{
		correct = false;
		return SetWord(Address(0));
	}
      
	correct=true;
	LP = (Point*)InPoint(nl->TheEmptyList(), FirstP, 0, errorInfo, correct ).addr;
	if (correct)  RP = (Point*)InPoint( nl->TheEmptyList(), SecondP, 0, errorInfo, correct ).addr;
      
	if (correct)
	{
	    if (*LP==*RP)
	    {
		cout <<">>>invalid data!<<<"<<endl;
		correct=false;
		return SetWord(Address(0));
	    }	    
	    
	    chs = new CHalfSegment(true, LDP, *LP, *RP);
        delete LP;
        delete RP;
	    //cout<<"IN Function"<<*ls<<endl;
	    return SetWord(chs);
	}
	else return SetWord(Address(0));
    }
    correct=false;
    return SetWord(Address(0));
}

/*
6.4 ~Create~-function

*/

static Word
CreateHalfSegment( const ListExpr typeInfo )
{
    CHalfSegment* chs = new CHalfSegment();
    chs->SetDefined(false);
    return (SetWord(chs));
}

/*
6.5 ~Delete~-function

*/

static void
DeleteHalfSegment( Word& w )
{
  delete (CHalfSegment*) w.addr;
  w.addr = 0;
}

/*
6.6 ~Close~-function

*/

static void
CloseHalfSegment( Word& w )
{
  delete (CHalfSegment*) w.addr;
  w.addr = 0;
}

/*
6.7 ~Clone~-function

*/

static Word
CloneHalfSegment( const Word& w )
{
  return SetWord( ((CHalfSegment*)w.addr)->Clone());
}

/*
6.8 ~Cast~-function

*/

static void*
CastHalfSegment( void* addr )
{
  return (new (addr) CHalfSegment);
}

/*
  
6.9 Function Describing the Signature of the Type Constructor

*/

static ListExpr
HalfSegmentProperty()
{
  return (nl->TwoElemList(nl->TheEmptyList(), nl->SymbolAtom("SPATIAL")));
}

/*
  
6.10 Kind Checking Function

*/

static bool
CheckHalfSegment( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual(type, "halfsegment" ));
}

/*
  
6.11 Creation of the Type Constructor Instance

*/

TypeConstructor halfsegment(
	"halfsegment",		       		//name
	HalfSegmentProperty, 			//Describing signature
	OutHalfSegment,  InHalfSegment,	    	//Out and In functions
	CreateHalfSegment, DeleteHalfSegment,	//object creation and deletion
	0, 0, CloseHalfSegment, CloneHalfSegment, //open, save, close, clone
	CastHalfSegment, 			//cast function
	CheckHalfSegment,			//kind checking function
	0, 				    	//predef. pers. function for model
	TypeConstructor::DummyInModel,
	TypeConstructor::DummyOutModel,
	TypeConstructor::DummyValueToModel,
	TypeConstructor::DummyValueListToModel );

/*
7 Type Constructor ~line~

A ~line~ value is a set of halfsegments. In the external (nestlist) representation, a line value is 
expressed as a set of segments. However, in the internal (class) representation, it is expressed
as a set of sorted halfsegments, which are stored as a PArray.

7.1 Implementation of the class ~line~

*/


CLine::CLine(SmiRecordFile *recordFile) : 
	line( new PArray<CHalfSegment>(recordFile) ), ordered( true )
{}

CLine::CLine(SmiRecordFile *recordFile, const CLine& cl ) :
	line( new PArray<CHalfSegment>(recordFile) ), ordered( true )
{
  assert( cl.IsOrdered());

  for( int i = 0; i < cl.Size(); i++ )
  {
    CHalfSegment chs;
    cl.Get( i, chs );
    line->Put( i, chs ); 
  }
}

CLine::CLine(SmiRecordFile *recordFile, const SmiRecordId recordId, bool update = true):
	 line(new PArray<CHalfSegment>(recordFile, recordId, update ) ),ordered(true)
{}

void CLine::Destroy()
{
  line->MarkDelete();
}

CLine::~CLine() 
{
  delete line;
}

const bool CLine::IsOrdered() const
{
  return ordered;
}

void CLine::StartBulkLoad()
{
  assert( IsOrdered() );
  ordered = false;
}

void CLine::EndBulkLoad()
{
  assert( !IsOrdered());
//  cout << "Before sorting: " << *this << endl;
  Sort();
//  cout << "After sorting: " << *this << endl;
  ordered = true;
}

const bool CLine::IsEmpty() const
{
  return (line->Size() == 0);
}

const int CLine::Size() const 
{
  return (line->Size());
}

void CLine::Get( const int i, CHalfSegment& chs ) const
{
  assert( i >= 0 && i < Size());
  line->Get( i, chs);
}

const SmiRecordId CLine::GetLineRecordId() const
{
  return line->Id();
}

CLine& CLine::operator=(const CLine& cl)
{
  assert( cl.IsOrdered() );

  line->MarkDelete();
  delete line;
  line = new PArray<CHalfSegment>(SecondoSystem::GetLobFile());  //$$$$$$$$$$$$$$$$$  
  for( int i = 0; i < cl.Size(); i++ )
  {
    CHalfSegment chs;
    cl.Get( i, chs );
    line->Put( i, chs );
  }
  ordered = true;
  return *this;
}

int CLine::operator==(const CLine& cl) const
{
  assert( IsOrdered() && cl.IsOrdered() );

  if( Size() != cl.Size() )    return 0;

  for( int i = 0; i < Size(); i++ )
  {
    CHalfSegment chs1, chs2;
    line->Get( i, chs1 );
    cl.Get( i, chs2 );
    if( chs1 != chs2 )
      return 0;
  }
  return 1;
}	     
	     
CLine& CLine::operator+=(const CHalfSegment& chs)
{ 
  assert(chs.IsDefined());

  if( !IsOrdered() )
  {
    line->Put( line->Size(), chs);
  }
  else
  {
    int pos = Position( chs );
    if( pos != -1 )
    {
      for( int i = line->Size() - 1; i >= pos; i++ )
      {
        CHalfSegment auxchs;
        line->Get( i, auxchs );
        line->Put( i+1, auxchs );
      }
      line->Put( pos, chs );
    }
  }
  return *this;
}

CLine& CLine::operator-=(const CHalfSegment& chs)
{
  assert( IsOrdered() && chs.IsDefined() );

  int pos = Position( chs );
  if( pos != -1 )
  {
    for( int i = pos; i < Size(); i++ )
    {
      CHalfSegment auxchs;
      line->Get( i+1, auxchs );
      line->Put( i, auxchs );
    }
  }
  return *this;
}		 

void CLine::SelectFirst()
{
    if (IsEmpty()) pos=-1;
    else pos=0;
}

void CLine::SelectNext()
{
    if ((pos>=0) && (pos<Size()-1)) pos++;
    else pos=-1;
}

bool CLine::EndOfHs()
{
    return (pos==-1);
}

void CLine::GetHs( CHalfSegment& chs )
{
    if (( pos>=0) && (pos<=Size()-1))    line->Get( pos, chs);
    else chs.SetDefined(false);
}
 
void CLine::InsertHs( CHalfSegment& chs )
{
    assert(chs.IsDefined());

    if( !IsOrdered())
    {
	pos=line->Size();
	line->Put( line->Size(), chs);
    }
    else
    {
	int insertpos = Position( chs );
	if( insertpos != -1 )
	{
	    for( int i = line->Size() - 1; i >= insertpos; i++ )
	    {
		CHalfSegment auxchs;
		line->Get( i, auxchs );
		line->Put( i+1, auxchs );
	    }
	    line->Put( insertpos, chs );
	    pos=insertpos;
	}
    }
}

const int CLine::Position( const CHalfSegment& chs) const
{
  assert( IsOrdered() && chs.IsDefined() );

  int first = 0, last = Size();

  while (first <= last) 
  {
    int mid = ( first + last ) / 2;  
    CHalfSegment midchs;
    line->Get( mid, midchs);
    if (chs > midchs )   first = mid + 1;  
    else if ( chs < midchs)  last = mid - 1; 
            else  return mid;
   }
   return -1;
}

void CLine::Sort()
{
  assert( !IsOrdered() );

  if( Size() > 1 ) 
  {
    int low = 0, high = Size() - 1;
    QuickSortRecursive( low, high );
  }
}

void CLine::QuickSortRecursive( const int low, const int high )
{
  int i = high, j = low;
  CHalfSegment chs, chsj, chsi;

  Get( (int)( (low + high) / 2 ), chs );

  do 
  {
    Get( j, chsj );
    while( chsj < chs )  Get( ++j, chsj );

    Get( i,chsi );
    while( chsi > chs ) Get( --i, chsi );

    if( i >= j ) 
    {
      if ( i != j ) 
      {
        line->Put( i, chsj );
        line->Put( j, chsi );
      }
      i--;
      j++;
    }
  } while( j <= i );

  if( low < i ) 
    QuickSortRecursive( low, i );
  if( j < high ) 
    QuickSortRecursive( j, high );
}

ostream& operator<<( ostream& os, const CLine& cl )
{
  os << "(" << cl.GetLineRecordId() << ") <";
  for( int i = 0; i < cl.Size(); i++ )
  {
    CHalfSegment chs;
    cl.Get( i, chs );
    os << " " << chs; 
  }
  os << " >";
  return os;
}

/*
7.2 List Representation

The list representation of a line is

----	((x1 y1 x2 y2) (x1 y1 x2 y2) ....)
----

7.3 ~Out~-function

*/

static ListExpr
OutLine( ListExpr typeInfo, Word value )
{
  cout<<"OUTLINE"<<endl;
  ListExpr result, last;
  CHalfSegment chs;
  ListExpr halfseg, halfpoints, flatseg;
	  
  CLine* cl = (CLine*)(value.addr);
  if( cl->IsEmpty())
  {
    return (nl->TheEmptyList());
  }
  else
  {
    result = nl->TheEmptyList(); 
    last = result;
    bool firstitem=true;
    
    for( int i = 0; i < cl->Size(); i++ )
    { 
      cl->Get( i, chs );
      if ((chs.IsDefined())&&(chs.GetLDP()==true))  //only output LEFT halfsegments
      {      
	  halfseg = OutHalfSegment( nl->TheEmptyList(), SetWord( &chs ) );
	  //the result is (true ( (x1 y1) (x2 y2) ) )
	  halfpoints=nl->Second( halfseg );
	  flatseg = nl->FourElemList(nl->First(nl->First( halfpoints )),nl->Second(nl->First( halfpoints )),
				  nl->First(nl->Second( halfpoints )),nl->Second(nl->Second( halfpoints )));
	  //the result is (x1 y1 x2 y2)
	  if (firstitem==true)
	  {
	      result=nl->OneElemList( flatseg );
	      last = result;
	      firstitem=false;
	  }
	  else
	  {
	      last = nl->Append( last, flatseg );
	  }
      }
    }
    return result;
  }
}

/*
7.4 ~In~-function

*/
static Word
InLine( const ListExpr typeInfo, const ListExpr instance, const int errorPos, ListExpr& errorInfo, bool& correct )
{ //cout<<"INLINE"<<endl;
  CLine* cl = new CLine( SecondoSystem::GetLobFile());
  CHalfSegment * chs;
  cl->StartBulkLoad();
  ListExpr first, halfseg, halfpoint;
  ListExpr rest = instance;
  
  if (!nl->IsAtom(instance))
  {
      while( !nl->IsEmpty( rest ) )
      {
	  first = nl->First( rest );
	  rest = nl->Rest( rest );
	  //first is a segment expressed as (x1 y1 x2 y2)
	  if (nl->ListLength( first ) != 4)
	  {
	      correct=false;
	      return SetWord( Address(0) );
	  }
	  else
	  {   //change it to the format of ((x1 y1) (x2 y2))
	      halfpoint=nl->TwoElemList(nl->TwoElemList(nl->First(first), nl->Second(first)),
				       nl->TwoElemList(nl->Third(first), nl->Fourth(first)));
	  }
	  halfseg = nl->TwoElemList(nl-> BoolAtom(true),halfpoint);
	  chs = (CHalfSegment*)InHalfSegment( nl->TheEmptyList(), halfseg, 0, errorInfo, correct ).addr;
	  if( correct ) 
	  {   //every point is added twice
	      (*cl) += (*chs);
	      chs->SetLDP(false);
	      (*cl) += (*chs);
	      delete chs;
	  }
	  else
	  {
	      return SetWord( Address(0) );
	  }
      }
      cl->EndBulkLoad();
      correct = true;
//      cout<<"INLINE: About to Returned from InLine!"<<endl;
      return SetWord( cl );
  }
  else  
  {
      correct=false;
      return SetWord( Address(0) );
  }
}

/*
7.5 ~Create~-function

*/
static Word
CreateLine( const ListExpr typeInfo )
{
//  cout << "CreateLine" << endl;

  return (SetWord( new CLine(SecondoSystem::GetLobFile() ) ));
}

/*
7.6 ~Delete~-function

*/
static void
DeleteLine( Word& w )
{
//  cout << "DeleteLine" << endl;

  CLine *cl = (CLine *)w.addr;
  cl->Destroy();
  delete cl;
  w.addr = 0;
}
 
/*
7.7 ~Close~-function

*/
static void
CloseLine( Word& w ) 
{
//  cout << "CloseLine" << endl;

  delete (CLine *)w.addr;
  w.addr = 0;
}
 
/*
7.8 ~Clone~-function

*/
static Word
CloneLine( const Word& w ) 
{
//  cout << "CloneLine" << endl;

  CLine *cl = new CLine(SecondoSystem::GetLobFile(), *((CLine *)w.addr) );
  return SetWord( cl );
}
 
/*
7.9 ~Open~-function

*/
bool
OpenLine( SmiRecord& valueRecord, const ListExpr typeInfo, Word& value )
{
  SmiRecordId recordId;
  valueRecord.Read( &recordId, sizeof( SmiRecordId ), 0 );
  CLine *cl = new CLine( SecondoSystem::GetLobFile(), recordId );
  value = SetWord( cl );
  //cout << "OpenLine: " << *cl << endl;
  return (true);
}

/*
7.10 ~Save~-function

*/
bool
SaveLine( SmiRecord& valueRecord, const ListExpr typeInfo, Word& value )
{
//  cout << "SaveLine" << endl;

  CLine *cl = (CLine*)value.addr;

//  cout << "SaveLine: " << *cl << endl;

  SmiRecordId recordId = cl->GetLineRecordId();

  valueRecord.Write( &recordId, sizeof( SmiRecordId ), 0 );

  return (true);
}

/*
7.11 Function describing the signature of the type constructor

*/
static ListExpr
LineProperty()
{
  return (nl->TwoElemList(
		nl->TheEmptyList(),
		nl->SymbolAtom("SPATIAL") ));
}

/*
7.12 Kind checking function

This function checks whether the type constructor is applied correctly. Since
type constructor ~line~ does not have arguments, this is trivial.

*/

static bool
CheckLine( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "line" ));
}

/*
7.13 ~Cast~-function

*/
void* CastLine(void* addr)
{
  return ( 0 );
}

/*
7.14 Creation of the type constructor instance

*/
TypeConstructor line(
	"line",				//name		
	LineProperty,	 		//describing signature
	OutLine,   	InLine,		//Out and In functions
	CreateLine,	DeleteLine,	//object creation and deletion
	OpenLine, 	SaveLine,    	// object open and save
	CloseLine, 	CloneLine,   	//object close and clone
	CastLine,			//cast function
	CheckLine,			//kind checking function
	0, 				//predef. pers. function for model
	TypeConstructor::DummyInModel, 	
	TypeConstructor::DummyOutModel,
	TypeConstructor::DummyValueToModel,
	TypeConstructor::DummyValueListToModel );
 
/*
8 Type Constructor ~region~

A ~region~ value is a set of halfsegments. In the external (nestlist) representation, a region value is 
expressed as a set of segments. However, in the internal (class) representation, it is expressed
as a set of sorted halfsegments, which are stored as a PArray.

The system will do the basic check on the validity of the region data. For instance, the region should have 
at least 3 edges, and every vertex should have even-numbered edges associated.

In the following step, I will change the design of the region data type. The internal representation of a region
is still a set of  sorted halfsegments, but the external representation (nestedlist) will be changed. It will use
the concept of cycles and faces.

8.1 Implementation of the class ~region~

*/

CRegion::CRegion(SmiRecordFile *recordFile) : 
	region( new PArray<CHalfSegment>(recordFile) ), ordered( true )
{}

CRegion::CRegion(SmiRecordFile *recordFile, const CRegion& cr ) : 
	region( new PArray<CHalfSegment>(recordFile) ), ordered( true )
{
  assert( cr.IsOrdered());

  for( int i = 0; i < cr.Size(); i++ )
  {
    CHalfSegment chs;
    cr.Get( i, chs );
    region->Put( i, chs ); 
  }
}

CRegion::CRegion(SmiRecordFile *recordFile, const SmiRecordId recordId, bool update = true):
	       region(new PArray<CHalfSegment>( recordFile, recordId, update ) ),ordered(true)
{}

void CRegion::Destroy()
{
  region->MarkDelete();
}

CRegion::~CRegion() 
{
  delete region;
}

const bool CRegion::IsOrdered() const
{
  return ordered;
}

void CRegion::StartBulkLoad()
{
  assert( IsOrdered() );
  ordered = false;
}

void CRegion::EndBulkLoad()
{
  assert( !IsOrdered());
  cout << "Before sorting: " << *this << endl;
  Sort();
  cout << "After sorting: " << *this << endl;
  ordered = true;
}

const bool CRegion::IsEmpty() const
{
  return (region->Size() == 0);
}

const bool CRegion::IsValid() const
{
    //1. check whether the number of edges is equal or greater than 3
    if (Size() /2 < 3) 
    {
	cout<<"a region should have at least 3 edges!"<<endl;
	return false;
    }
    
    //2. check whether the degree of every vertex is even number
    Points auxpoints(SecondoSystem::GetLobFile()); //$$$$$$$$$$$$$$$
    CHalfSegment auxhs;
    
    auxpoints.StartBulkLoad();
    for (int i=0; i<Size(); i++)
    {
	region->Get(i,auxhs);
	if (auxhs.GetLDP())  //only process the left halfseg
	{
	    auxpoints += auxhs.GetDPoint();
	    auxpoints += auxhs.GetSPoint();
	}
    }
    auxpoints.EndBulkLoad();

    Point lastp, currp;
    auxpoints.Get(0, lastp);    
    int j=1; //the number of the same point
    for (int i=1; i<auxpoints.Size(); i++)
    {
	    auxpoints.Get(i, currp);    
	    if (currp==lastp) j++;
	    else 
	    {
		if ((j%2)==0) 
		{
		    j=1;  //j is even number and it is ok. j return to 1
		    lastp=currp;
		}
		else 
		{
		    cout<<"every vertex of a region should be associated with even-numbered edges!"<<endl;
		    return false;
		}
	    }
    }
    if ((j%2)!=0)  //the last check when the vertex list is finished
    {
	    cout<<"every vertex of a region should be associated with even-numbered edges!"<<endl;
	    return false;  //check the last point
    }
    
    return true;
}

const int CRegion::Size() const 
{
  return (region->Size());
}

void CRegion::Get( const int i, CHalfSegment& chs ) const
{
  assert( i >= 0 && i < Size());
  region->Get( i, chs);
}

const SmiRecordId CRegion::GetRegionRecordId() const
{
  return region->Id();
}

CRegion& CRegion::operator=(const CRegion& cr)
{
  assert( cr.IsOrdered() );

  region->MarkDelete();
  delete region;
  region = new PArray<CHalfSegment>(SecondoSystem::GetLobFile());  //$$$$$$$$$$$$$$$$$$
  for( int i = 0; i < cr.Size(); i++ )
  {
    CHalfSegment chs;
    cr.Get( i, chs );
    region->Put( i, chs );
  }
  ordered = true;
  return *this;
}

int CRegion::operator==(const CRegion& cr) const
{
  assert( IsOrdered() && cr.IsOrdered() );

  if( Size() != cr.Size() )    return 0;

  for( int i = 0; i < Size(); i++ )
  {
    CHalfSegment chs1, chs2;
    region->Get( i, chs1 );
    cr.Get( i, chs2 );
    if( chs1 != chs2 )
      return 0;
  }
  return 1;
}	     
	     
CRegion& CRegion::operator+=(const CHalfSegment& chs)
{ 
  assert(chs.IsDefined());

  if( !IsOrdered() )
  {
    region->Put( region->Size(), chs);
  }
  else
  {
    int pos = Position( chs );
    if( pos != -1 )
    {
      for( int i = region->Size() - 1; i >= pos; i++ )
      {
        CHalfSegment auxchs;
        region->Get( i, auxchs );
        region->Put( i+1, auxchs );
      }
      region->Put( pos, chs );
    }
  }
  return *this;
}

CRegion& CRegion::operator-=(const CHalfSegment& chs)
{
  assert( IsOrdered() && chs.IsDefined() );

  int pos = Position( chs );
  if( pos != -1 )
  {
    for( int i = pos; i < Size(); i++ )
    {
      CHalfSegment auxchs;
      region->Get( i+1, auxchs );
      region->Put( i, auxchs );
    }
  }
  return *this;
}		 

void CRegion::SelectFirst()
{
    if (IsEmpty()) pos=-1;
    else pos=0;
}

void CRegion::SelectNext()
{
    if ((pos>=0) && (pos<Size()-1)) pos++;
    else pos=-1;
}

bool CRegion::EndOfHs()
{
    return (pos==-1);
}

void CRegion::GetHs( CHalfSegment& chs )
{
    if (( pos>=0) && (pos<=Size()-1))  region->Get( pos, chs);
    else chs.SetDefined(false);
}
 
void CRegion::InsertHs( CHalfSegment& chs )
{
    assert(chs.IsDefined());
    
    //chs.SetAttr(NULL);
    if( !IsOrdered())
    {
	pos=region->Size();
	region->Put( region->Size(), chs);
    }
    else
    {
	int insertpos = Position( chs );
	if( insertpos != -1 )
	{
	    for( int i = region->Size() - 1; i >= insertpos; i++ )
	    {
		CHalfSegment auxchs;
		region->Get( i, auxchs );
		region->Put( i+1, auxchs );
	    }
	    region->Put( insertpos, chs );
	    pos=insertpos;
	}
    }
}

const attrtype& CRegion::GetAttr()
{
    assert(( pos>=0) && (pos<=Size()-1));
    CHalfSegment chs;
    region->Get( pos, chs);
    return chs.GetAttr();
}

void CRegion::UpdateAttr( attrtype& ATTR )
{
    if (( pos>=0) && (pos<=Size()-1))
    {
	CHalfSegment chs;
	region->Get( pos, chs);
	chs.SetAttr(ATTR);
	region->Put( pos, chs);
    }
}

const int CRegion::Position( const CHalfSegment& chs) const
{
  assert( IsOrdered() && chs.IsDefined() );

  int first = 0, last = Size();

  while (first <= last) 
  {
    int mid = ( first + last ) / 2;  
    CHalfSegment midchs;
    region->Get( mid, midchs);
    if (chs > midchs )   first = mid + 1;  
    else if ( chs < midchs)  last = mid - 1; 
            else  return mid;
   }
   return -1;
}

void CRegion::Sort()
{
  assert( !IsOrdered() );

  if( Size() > 1 ) 
  {
    int low = 0, high = Size() - 1;
    QuickSortRecursive( low, high );
  }
}

void CRegion::QuickSortRecursive( const int low, const int high )
{
  int i = high, j = low;
  CHalfSegment chs, chsj, chsi;

  Get( (int)( (low + high) / 2 ), chs );

  do 
  {
    Get( j, chsj );
    while( chsj < chs )  Get( ++j, chsj );

    Get( i,chsi );
    while( chsi > chs ) Get( --i, chsi );

    if( i >= j ) 
    {
      if ( i != j ) 
      {
        region->Put( i, chsj );
        region->Put( j, chsi );
      }
      i--;
      j++;
    }
  } while( j <= i );

  if( low < i ) 
    QuickSortRecursive( low, i );
  if( j < high ) 
    QuickSortRecursive( j, high );
}

ostream& operator<<( ostream& os, const CRegion& cr )
{
  os << "(" << cr.GetRegionRecordId() << ") <";
  for( int i = 0; i < cr.Size(); i++ )
  {
    CHalfSegment chs;
    cr.Get( i, chs );
    os << " " << chs; 
  }
  os << " >";
  return os;
}

/*
8.2 List Representation

The list representation of a region is

----	(hs1  hs2  hs3 ... )
----

8.3 ~Out~-function

*/

static ListExpr
OutRegion( ListExpr typeInfo, Word value )
{
  CRegion* cr = (CRegion*)(value.addr);
  if( cr->IsEmpty() )
  {
    return (nl->TheEmptyList());
  }
  else
  {
    CHalfSegment chs;
//  cr->Get( 0, chs );
    ListExpr result = nl->TheEmptyList();
    ListExpr last = result;

    for( int i = 0; i < cr->Size(); i++ )
    {
      cr->Get( i, chs );
//    last = nl->Append( last, OutHalfSegment( nl->TheEmptyList(), SetWord( &chs ) ) );
      if (chs.GetLDP()==true)  //only output LEFT halfsegments
      {      
	  ListExpr HSlist = OutHalfSegment( nl->TheEmptyList(), SetWord( &chs ) );
	  if (result==nl->TheEmptyList())
	  {
	      result=nl->OneElemList( nl->Second( HSlist) );
	      last = result;
	  }
	  else
	  {
	      if ( nl->ListLength( HSlist ) == 2)  last = nl->Append( last, nl->Second( HSlist) );
	  }
      }
    }  
    return result;
  }
}

/*
8.4 ~In~-function

*/

/*
int 
InCycle( const ListExpr cycleNL, CRegion *cr, bool& correct )
{

  ListExpr rest = cycleNL;
  if (!nl->IsAtom(faceNL))
  {
      while( !nl->IsEmpty( rest ) )
      {
	  ListExpr first = nl->First( rest );
	  rest = nl->Rest( rest );
	  //every point is added twice
	  ListExpr flagfirst = nl->TwoElemList(nl-> BoolAtom(true),first);
	  CHalfSegment * chs = (CHalfSegment*)InHalfSegment( nl->TheEmptyList(), flagfirst, 0, errorInfo, correct ).addr;
	  if( correct ) 
	  {
	      (*cr) += (*chs);
      	      chs->SetLDP(false);
	      (*cr) += (*chs);
	      delete chs;
	  }
	  else
	  {
	      return SetWord( Address(0) );
	  }
      }
      cr->EndBulkLoad();
      if (!(cr->IsValid()))
      {
	  correct=false; 
	  return SetWord( Address(0) );
      }
      correct = true;
      return SetWord( cr );
  }
  else  
  {
      correct=false;
      return SetWord( Address(0) );
  } 
}

int 
InFace( const ListExpr faceNL, CRegion *cr, bool& correct )
{

  ListExpr rest = faceNL;
  if (!nl->IsAtom(faceNL))
  {
      while( !nl->IsEmpty( rest ) )
      {
	  ListExpr first = nl->First( rest );
	  rest = nl->Rest( rest );
	  //every point is added twice
	  ListExpr flagfirst = nl->TwoElemList(nl-> BoolAtom(true),first);
	  CHalfSegment * chs = (CHalfSegment*)InHalfSegment( nl->TheEmptyList(), flagfirst, 0, errorInfo, correct ).addr;
	  if( correct ) 
	  {
	      (*cr) += (*chs);
      	      chs->SetLDP(false);
	      (*cr) += (*chs);
	      delete chs;
	  }
	  else
	  {
	      return SetWord( Address(0) );
	  }
      }
      cr->EndBulkLoad();
      if (!(cr->IsValid()))
      {
	  correct=false; 
	  return SetWord( Address(0) );
      }
      correct = true;
      return SetWord( cr );
  }
  else  
  {
      correct=false;
      return SetWord( Address(0) );
  } 
}
*/
static Word
InRegion( const ListExpr typeInfo, const ListExpr instance, const int errorPos, ListExpr& errorInfo, bool& correct )
{
  CRegion* cr = new CRegion(SecondoSystem::GetLobFile());
  cr->StartBulkLoad();

  ListExpr rest = instance;
  if (!nl->IsAtom(instance))
  {
      while( !nl->IsEmpty( rest ) )
      {
	  ListExpr first = nl->First( rest );
	  rest = nl->Rest( rest );
	  //every point is added twice
	  ListExpr flagfirst = nl->TwoElemList(nl-> BoolAtom(true),first);
	  CHalfSegment * chs = (CHalfSegment*)InHalfSegment( nl->TheEmptyList(), flagfirst, 0, errorInfo, correct ).addr;
	  if( correct ) 
	  {
	      (*cr) += (*chs);
      	      chs->SetLDP(false);
	      (*cr) += (*chs);
	      delete chs;
	  }
	  else
	  {
	      return SetWord( Address(0) );
	  }
      }
      cr->EndBulkLoad();
      if (!(cr->IsValid()))
      {
	  correct=false; 
	  return SetWord( Address(0) );
      }
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
static Word
CreateRegion( const ListExpr typeInfo )
{
  cout << "CreateRegion" << endl;

  return (SetWord( new CRegion(SecondoSystem::GetLobFile() ) ));
}

/*
8.6 ~Delete~-function

*/
static void
DeleteRegion( Word& w )
{
  cout << "DeleteRegion" << endl;

  CRegion *cr = (CRegion *)w.addr;
  cr->Destroy();
  delete cr;
  w.addr = 0;
}
 
/*
8.7 ~Close~-function

*/
static void
CloseRegion( Word& w ) 
{
  cout << "CloseRegion" << endl;

  delete (CRegion *)w.addr;
  w.addr = 0;
}
 
/*
8.8 ~Clone~-function

*/
static Word
CloneRegion( const Word& w ) 
{
  cout << "CloneRegion" << endl;

  CRegion *cr = new CRegion( SecondoSystem::GetLobFile(), *((CRegion *)w.addr) );
  return SetWord( cr );
}
 
/*
8.9 ~Open~-function

*/
bool
OpenRegion( SmiRecord& valueRecord, const ListExpr typeInfo, Word& value )
{
  SmiRecordId recordId;
  valueRecord.Read( &recordId, sizeof( SmiRecordId ), 0 );
  CRegion *cr = new CRegion(SecondoSystem::GetLobFile(), recordId );
  value = SetWord( cr );
  //cout << "OpenLine: " << *cl << endl;
  return (true);
}

/*
8.10 ~Save~-function

*/
bool
SaveRegion( SmiRecord& valueRecord, const ListExpr typeInfo, Word& value )
{
  //cout << "SaveLine" << endl;

  CRegion *cr = (CRegion*)value.addr;

  cout << "SaveRegion: " << *cr << endl;

  SmiRecordId recordId = cr->GetRegionRecordId();

  valueRecord.Write( &recordId, sizeof( SmiRecordId ), 0 );

  return (true);
}

/*
8.11 Function describing the signature of the type constructor

*/
static ListExpr
RegionProperty()
{
  return (nl->TwoElemList(
		nl->TheEmptyList(),
		nl->SymbolAtom("SPATIAL") ));
}

/*
8.12 Kind checking function

This function checks whether the type constructor is applied correctly. Since
type constructor ~point~ does not have arguments, this is trivial.

*/
static bool
CheckRegion( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "region" ));
}

/*
8.13 ~Cast~-function

*/
void* CastRegion(void* addr)
{
  return ( 0 );
}

/*
8.14 Creation of the type constructor instance

*/
TypeConstructor region(
	"region",			//name		
	RegionProperty,	 		//describing signature
	OutRegion,   	InRegion,	//Out and In functions
	CreateRegion,	DeleteRegion,	//object creation and deletion
	OpenRegion, 	SaveRegion,    	// object open and save
	CloseRegion, 	CloneRegion,   	//object close and clone
	CastRegion,			//cast function
	CheckRegion,			//kind checking function
	0, 				//predef. pers. function for model
	TypeConstructor::DummyInModel, 	
	TypeConstructor::DummyOutModel,
	TypeConstructor::DummyValueToModel,
	TypeConstructor::DummyValueListToModel );

/*
9 Operators

Definition of operators is similar to definition of type constructors. An
operator is defined by creating an instance of class ~Operator~. Again we
have to define some functions before we are able to create an ~Operator~
instance.

9.1 Type mapping function

A type mapping function takes a nested list as argument. Its contents are
type descriptions of an operator's input parameters. A nested list describing
the output type of the operator is returned.

9.1.1 Type mapping function SpatialTypeMapBool

It is for the compare operators which have ~bool~ as resulttype, like =, !=, <, 
<=, >, >=.

*/
static ListExpr
SpatialTypeMapBool( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 )
  {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if ( TypeOfSymbol( arg1 ) == stpoint && TypeOfSymbol( arg2 ) == stpoint)
      return (nl->SymbolAtom( "bool" ));
    if ( TypeOfSymbol( arg1 ) == stpoints && TypeOfSymbol( arg2 ) == stpoints)
      return (nl->SymbolAtom( "bool" ));
    if ( TypeOfSymbol( arg1 ) == stline && TypeOfSymbol( arg2 ) == stline)
      return (nl->SymbolAtom( "bool" ));
    if ( TypeOfSymbol( arg1 ) == stregion && TypeOfSymbol( arg2 ) == stregion)
      return (nl->SymbolAtom( "bool" ));
    if ( TypeOfSymbol( arg1 ) == stpoint && TypeOfSymbol( arg2 ) == stpoints)
      return (nl->SymbolAtom( "bool" ));
    if ( TypeOfSymbol( arg1 ) == stpoints && TypeOfSymbol( arg2 ) == stpoint)
      return (nl->SymbolAtom( "bool" ));
    if ( TypeOfSymbol( arg1 ) == stpoint && TypeOfSymbol( arg2 ) == stline)
      return (nl->SymbolAtom( "bool" ));
    if ( TypeOfSymbol( arg1 ) == stpoint && TypeOfSymbol( arg2 ) == stregion)
      return (nl->SymbolAtom( "bool" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
9.1.2 Type mapping function SpatialTypeMapBool1

It is for the operator ~isempty~ which have ~point~, ~points~, ~line~, and ~region~ as input and ~bool~ resulttype.

*/

static ListExpr
SpatialTypeMapBool1( ListExpr args )
{
  ListExpr arg1;
  if ( nl->ListLength( args ) == 1 )
  {
    arg1 = nl->First( args );
    if ( TypeOfSymbol( arg1 ) == stpoint )
      return (nl->SymbolAtom( "bool" ));
    if ( TypeOfSymbol( arg1 ) == stpoints )
      return (nl->SymbolAtom( "bool" ));
    if ( TypeOfSymbol( arg1 ) == stline )
      return (nl->SymbolAtom( "bool" ));
    if ( TypeOfSymbol( arg1 ) == stregion )
      return (nl->SymbolAtom( "bool" ));
  }
  return (nl->SymbolAtom( "typeerror" ));
}

/*
9.1.3 The dummy model mapping:

*/
static Word
SpatialNoModelMapping( ArgVector arg, Supplier opTreeNode )
{
  return (SetWord( Address( 0 ) ));
}

/*
9.2 Selection function

A selection function is quite similar to a type mapping function. The only
difference is that it doesn't return a type but the index of a value
mapping function being able to deal with the respective combination of
input parameter types.

Note that a selection function does not need to check the correctness of
argument types; it has already been checked by the type mapping function that it
is applied to correct arguments.

9.2.1 Selection function ~SimpleSelect~

Is used for all non-overloaded operators.

*/
static int
SimpleSelect( ListExpr args ) 
{
  return (0);
}

/*
9.2.2 Selection function ~SpatialSelectIsEmpty~

It is used for the ~isempty~ operator

*/
static int
SpatialSelectIsEmpty( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  if ( TypeOfSymbol( arg1 ) == stpoint )
    return (0);
  if ( TypeOfSymbol( arg1 ) == stpoints )
    return (1);
  if ( TypeOfSymbol( arg1 ) == stline )
    return (2);
  if ( TypeOfSymbol( arg1 ) == stregion )
    return (3);
  return (-1); // This point should never be reached
}

/*
9.2.3 Selection function ~SpatialSelectCompare~

It is used for compare operators ($=$, $\neq$, $<$, $>$, $\geq$, $\leq$)

*/
static int
SpatialSelectCompare( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );
  if ( TypeOfSymbol( arg1 ) == stpoint && TypeOfSymbol( arg2 ) == stpoint )
    return (0);
  if ( TypeOfSymbol( arg1 ) == stpoints && TypeOfSymbol( arg2 ) == stpoints )
    return (1);
  if ( TypeOfSymbol( arg1 ) == stline && TypeOfSymbol( arg2 ) == stline )
    return (2);
  if ( TypeOfSymbol( arg1 ) == stregion && TypeOfSymbol( arg2 ) == stregion )
    return (3);
  return (-1); // This point should never be reached
}

/*
9.2.4 Selection function ~SpatialSelectSets1~

It is used for set operators (~intersects~)

*/
static int
SpatialSelectSets1( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );
  if ( TypeOfSymbol( arg1 ) == stpoints && TypeOfSymbol( arg2 ) == stpoints )
    return (0);
  if ( TypeOfSymbol( arg1 ) == stline && TypeOfSymbol( arg2 ) == stline )
    return (1);
  if ( TypeOfSymbol( arg1 ) == stregion && TypeOfSymbol( arg2 ) == stregion )
    return (2);
  return (-1); // This point should never be reached
}

/*
9.2.5 Selection function ~SpatialSelectSets2~

It is used for set operators (~inside~) that allow the first argument to be
simple.

*/
static int
SpatialSelectSets2( ListExpr args )
{
  ListExpr arg1 = nl->First( args );
  ListExpr arg2 = nl->Second( args );
  if ( TypeOfSymbol( arg1 ) == stpoints && TypeOfSymbol( arg2 ) == stpoints )
    return (0);
  if ( TypeOfSymbol( arg1 ) == stline && TypeOfSymbol( arg2 ) == stline )
    return (1);
  if ( TypeOfSymbol( arg1 ) == stregion && TypeOfSymbol( arg2 ) == stregion )
    return (2);
  if ( TypeOfSymbol( arg1 ) == stpoint && TypeOfSymbol( arg2 ) == stpoints )
    return (3);
  if ( TypeOfSymbol( arg1 ) == stpoint && TypeOfSymbol( arg2 ) == stline )
    return (4);
  if ( TypeOfSymbol( arg1 ) == stpoint && TypeOfSymbol( arg2 ) == stregion )
    return (5);
  return (-1); // This point should never be reached
}

/*
9.3 Object Traversal functions

These functions are useful if we want to traverse the objects.  There are 6 combinations, pp, pl, pr, ll, lr, rr

*/

enum object {none, first, second, both};
enum status {endnone, endfirst, endsecond, endboth};

void ppSelectFirst(Points& P1, Points& P2, object& obj, status& stat)
{
    P1.SelectFirst();
    P2.SelectFirst();
    
    Point p1, p2;
    P1.GetPt( p1 );
    P2.GetPt( p2 );
    
    if ((!(p1.IsDefined()))&&(!(p2.IsDefined()))) {obj=none; stat=endboth;}
    else if (!(p1.IsDefined())) {obj=second; stat=endfirst; }   
    else if (!(p2.IsDefined())) {obj=first; stat=endsecond; }
    else //both defined
    {
	stat=endnone;
	if (p1<p2) obj=first;
	else if (p1>p2) obj=second;
	else obj=both;
    }
}

void ppSelectNext(Points& P1, Points& P2, object& obj, status& stat)
{
    // 1. get the current elements
    Point p1, p2;
    P1.GetPt( p1 );
    P2.GetPt( p2 );
    
    //2. move the pointers
    if ((!(p1.IsDefined()))&&(!(p2.IsDefined()))) 
    {
	//do nothing
    }
    else if (!(p1.IsDefined())) 
    {	
	P2.SelectNext();
	P2.GetPt( p2 );
    }
    else if (!(p2.IsDefined())) 
    {
	P1.SelectNext();
	P1.GetPt( p1 );
    }
    else //both currently defined
    {
	if (p1< p2) //then chs1 is the last output
	{
	    P1.SelectNext();
	    P1.GetPt( p1 );
	}
	else if (p1> p2)
	{
	    P2.SelectNext();
	    P2.GetPt( p2 );
	}  
	else
	{
	    P1.SelectNext();
	    P1.GetPt( p1 );
	    P2.SelectNext();
	    P2.GetPt( p2 );
	}
    }
    
    //3. generate the outputs
    if ((!(p1.IsDefined()))&&(!(p2.IsDefined()))) {obj=none; stat=endboth;}
    else if (!(p1.IsDefined())) {obj=second; stat=endfirst; }   
    else if (!(p2.IsDefined())) {obj=first; stat=endsecond; }
    else //both defined
    {
	stat=endnone;
	if (p1<p2) obj=first;
	else if (p1>p2) obj=second;
	else obj=both;
    }
}

void plSelectFirst(Points& P, CLine& L, object& obj, status& stat)
{
    P.SelectFirst();
    L.SelectFirst();
    
    Point p1, p2;
    CHalfSegment chs;
    
    P.GetPt( p1 );
    
    L.GetHs( chs );
    p2=chs.GetDPoint();
    
    if ((!(p1.IsDefined()))&&(!(p2.IsDefined()))) {obj=none; stat=endboth;}
    else if (!(p1.IsDefined())) {obj=second; stat=endfirst; }   
    else if (!(p2.IsDefined())) {obj=first; stat=endsecond; }
    else //both defined
    {
	stat=endnone;
	if (p1<p2) obj=first;
	else if (p1>p2) obj=second;
	else obj=both;
    }
}

void plSelectNext(Points& P, CLine& L, object& obj, status& stat)
{
    // 1. get the current elements
    Point p1, p2;
    CHalfSegment chs;
    
    P.GetPt( p1 );
    L.GetHs( chs );
    p2=chs.GetDPoint();
    
    //2. move the pointers
    if ((!(p1.IsDefined()))&&(!(p2.IsDefined()))) 
    {
	//do nothing
    }
    else if (!(p1.IsDefined())) 
    {	
	L.SelectNext();
	L.GetHs( chs );
	p2=chs.GetDPoint();
    }
    else if (!(p2.IsDefined())) 
    {
	P.SelectNext();
	P.GetPt( p1 );
    }
    else //both currently defined
    {
	if (p1< p2) //then chs1 is the last output
	{
	    P.SelectNext();
	    P.GetPt( p1 );
	}
	else if (p1> p2)
	{
	    L.SelectNext();
	    L.GetHs( chs );
	    p2=chs.GetDPoint();
	}  
	else
	{
	    P.SelectNext();
	    P.GetPt( p1 );
	    L.SelectNext();
	    L.GetHs( chs );
	    p2=chs.GetDPoint();
	}
    }
    
    //3. generate the outputs
    if ((!(p1.IsDefined()))&&(!(p2.IsDefined()))) {obj=none; stat=endboth;}
    else if (!(p1.IsDefined())) {obj=second; stat=endfirst; }   
    else if (!(p2.IsDefined())) {obj=first; stat=endsecond; }
    else //both defined
    {
	stat=endnone;
	if (p1<p2) obj=first;
	else if (p1>p2) obj=second;
	else obj=both;
    }
}

void prSelectFirst(Points& P, CRegion& R, object& obj, status& stat)
{
    P.SelectFirst();
    R.SelectFirst();
    
    Point p1, p2;
    CHalfSegment chs;
    
    P.GetPt( p1 );
    
    R.GetHs( chs );
    p2=chs.GetDPoint();
    
    if ((!(p1.IsDefined()))&&(!(p2.IsDefined()))) {obj=none; stat=endboth;}
    else if (!(p1.IsDefined())) {obj=second; stat=endfirst; }   
    else if (!(p2.IsDefined())) {obj=first; stat=endsecond; }
    else //both defined
    {
	stat=endnone;
	if (p1<p2) obj=first;
	else if (p1>p2) obj=second;
	else obj=both;
    }
}

void prSelectNext(Points& P, CRegion& R, object& obj, status& stat)
{
    // 1. get the current elements
    Point p1, p2;
    CHalfSegment chs;
    
    P.GetPt( p1 );
    R.GetHs( chs );
    p2=chs.GetDPoint();
    
    //2. move the pointers
    if ((!(p1.IsDefined()))&&(!(p2.IsDefined()))) 
    {
	//do nothing
    }
    else if (!(p1.IsDefined())) 
    {	
	R.SelectNext();
	R.GetHs( chs );
	p2=chs.GetDPoint();
    }
    else if (!(p2.IsDefined())) 
    {
	P.SelectNext();
	P.GetPt( p1 );
    }
    else //both currently defined
    {
	if (p1< p2) //then chs1 is the last output
	{
	    P.SelectNext();
	    P.GetPt( p1 );
	}
	else if (p1> p2)
	{
	    R.SelectNext();
	    R.GetHs( chs );
	    p2=chs.GetDPoint();
	}  
	else
	{
	    P.SelectNext();
	    P.GetPt( p1 );
	    R.SelectNext();
	    R.GetHs( chs );
	    p2=chs.GetDPoint();
	}
    }
    
    //3. generate the outputs
    if ((!(p1.IsDefined()))&&(!(p2.IsDefined()))) {obj=none; stat=endboth;}
    else if (!(p1.IsDefined())) {obj=second; stat=endfirst; }   
    else if (!(p2.IsDefined())) {obj=first; stat=endsecond; }
    else //both defined
    {
	stat=endnone;
	if (p1<p2) obj=first;
	else if (p1>p2) obj=second;
	else obj=both;
    }
}

void llSelectFirst(CLine& L1, CLine& L2, object& obj, status& stat)
{
    L1.SelectFirst();
    L2.SelectFirst();
    
    CHalfSegment chs1, chs2;
    L1.GetHs( chs1 );
    L2.GetHs( chs2 );
    
    if ((!(chs1.IsDefined()))&&(!(chs2.IsDefined()))) {obj=none; stat=endboth;}
    else if (!(chs1.IsDefined())) {obj=second; stat=endfirst; }   
    else if (!(chs2.IsDefined())) {obj=first; stat=endsecond; }
    else //both defined
    {
	stat=endnone;
	if (chs1<chs2) obj=first;
	else if (chs1>chs2) obj=second;
	else obj=both;
    }
}

void llSelectNext(CLine& L1, CLine& L2, object& obj, status& stat)
{
    // 1. get the current elements
    CHalfSegment chs1, chs2;
    L1.GetHs( chs1 );
    L2.GetHs( chs2 );
    
    //2. move the pointers
    if ((!(chs1.IsDefined()))&&(!(chs2.IsDefined()))) 
    {
	//do nothing
    }
    else if (!(chs1.IsDefined())) 
    {	
	L2.SelectNext();
	L2.GetHs( chs2 );
    }
    else if (!(chs2.IsDefined())) 
    {
	L1.SelectNext();
	L1.GetHs( chs1 );
    }
    else //both currently defined
    {
	if (chs1< chs2) //then chs1 is the last output
	{
	    L1.SelectNext();
	    L1.GetHs( chs1 );
	}
	else if (chs1> chs2)
	{
	    L2.SelectNext();
	    L2.GetHs( chs2 );
	}  
	else
	{
	    L1.SelectNext();
	    L1.GetHs( chs1 );
	    L2.SelectNext();
	    L2.GetHs( chs2 );
	}
    }
    
    //3. generate the outputs
    if ((!(chs1.IsDefined()))&&(!(chs2.IsDefined()))) {obj=none; stat=endboth;}
    else if (!(chs1.IsDefined())) {obj=second; stat=endfirst; }   
    else if (!(chs2.IsDefined())) {obj=first; stat=endsecond; }
    else //both defined
    {
	stat=endnone;
	if (chs1<chs2) obj=first;
	else if (chs1>chs2) obj=second;
	else obj=both;
    }
}

void lrSelectFirst(CLine& L, CRegion& R, object& obj, status& stat)
{
    L.SelectFirst();
    R.SelectFirst();
    
    CHalfSegment chs1, chs2;
    L.GetHs( chs1 );
    R.GetHs( chs2 );
    
    if ((!(chs1.IsDefined()))&&(!(chs2.IsDefined()))) {obj=none; stat=endboth;}
    else if (!(chs1.IsDefined())) {obj=second; stat=endfirst; } 
    else if (!(chs2.IsDefined())) {obj=first; stat=endsecond; }
    else //both defined
    {
	stat=endnone;
	if (chs1<chs2) obj=first;
	else if (chs1>chs2) obj=second;
	else obj=both;
    }    
}

void lrSelectNext(CLine& L, CRegion& R, object& obj, status& stat)
{
    // 1. get the current elements
    CHalfSegment chs1, chs2;
    L.GetHs( chs1 );
    R.GetHs( chs2 );
    
    //2. move the pointers
    if ((!(chs1.IsDefined()))&&(!(chs2.IsDefined()))) 
    {
	//do nothing
    }
    else if (!(chs1.IsDefined())) 
    {	
	R.SelectNext();
	R.GetHs( chs2 );
    }
    else if (!(chs2.IsDefined())) 
    {
	L.SelectNext();
	L.GetHs( chs1 );
    }
    else //both currently defined
    {
	if (chs1< chs2) //then chs1 is the last output
	{
	    L.SelectNext();
	    L.GetHs( chs1 );
	}
	else if (chs1> chs2)
	{
	    R.SelectNext();
	    R.GetHs( chs2 );
	}  
	else
	{
	    L.SelectNext();
	    L.GetHs( chs1 );
	    R.SelectNext();
	    R.GetHs( chs2 );
	}
    }
    
    //3. generate the outputs
    if ((!(chs1.IsDefined()))&&(!(chs2.IsDefined()))) {obj=none; stat=endboth;}
    else if (!(chs1.IsDefined())) {obj=second; stat=endfirst; }   
    else if (!(chs2.IsDefined())) {obj=first; stat=endsecond; }
    else //both defined
    {
	stat=endnone;
	if (chs1<chs2) obj=first;
	else if (chs1>chs2) obj=second;
	else obj=both;
    }
}

void rrSelectFirst(CRegion& R1, CRegion& R2, object& obj, status& stat)
{
    R1.SelectFirst();
    R2.SelectFirst();
    
    CHalfSegment chs1, chs2;
    R1.GetHs( chs1 );
    R2.GetHs( chs2 );
    
    if ((!(chs1.IsDefined()))&&(!(chs2.IsDefined()))) {obj=none; stat=endboth;}
    else if (!(chs1.IsDefined())) {obj=second; stat=endfirst; }   
    else if (!(chs2.IsDefined())) {obj=first; stat=endsecond; }
    else //both defined
    {
	stat=endnone;
	if (chs1<chs2) obj=first;
	else if (chs1>chs2) obj=second;
	else obj=both;
    }
}

void rrSelectNext(CRegion& R1, CRegion& R2, object& obj, status& stat)
{
    // 1. get the current elements
    CHalfSegment chs1, chs2;
    R1.GetHs( chs1 );
    R2.GetHs( chs2 );
    
    //2. move the pointers
    if ((!(chs1.IsDefined()))&&(!(chs2.IsDefined()))) 
    {
	//do nothing
    }
    else if (!(chs1.IsDefined())) 
    {	
	R2.SelectNext();
	R2.GetHs( chs2 );
    }
    else if (!(chs2.IsDefined())) 
    {
	R1.SelectNext();
	R1.GetHs( chs1 );
    }
    else //both currently defined
    {
	if (chs1< chs2) //then chs1 is the last output
	{
	    R1.SelectNext();
	    R1.GetHs( chs1 );
	}
	else if (chs1> chs2)
	{
	    R2.SelectNext();
	    R2.GetHs( chs2 );
	}  
	else
	{
	    R1.SelectNext();
	    R1.GetHs( chs1 );
	    R2.SelectNext();
	    R2.GetHs( chs2 );
	}
    }
    
    //3. generate the outputs
    if ((!(chs1.IsDefined()))&&(!(chs2.IsDefined()))) {obj=none; stat=endboth;}
    else if (!(chs1.IsDefined())) {obj=second; stat=endfirst; }   
    else if (!(chs2.IsDefined())) {obj=first; stat=endsecond; }
    else //both defined
    {
	stat=endnone;
	if (chs1<chs2) obj=first;
	else if (chs1>chs2) obj=second;
	else obj=both;
    }
}


/*
9.4 Value mapping functions

A value mapping function implements an operator's main functionality: it takes
input arguments and computes the result. Each operator consists of at least
one value mapping function. In the case of overloaded operators there are 
several value mapping functions, one for each possible combination of input
parameter types. 

9.4.1 Value mapping functions of operator ~isempty~

*/
static int
IsEmpty_p( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if( ((Point*)args[0].addr)->IsDefined() )
  {
    ((CcBool*)result.addr)->Set( true, false );
  }
  else
  {
    ((CcBool *)result.addr)->Set( true, true );
  }
  return (0);
}

static int
IsEmpty_ps( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if( ((Points*)args[0].addr)->IsEmpty() )
  {
    ((CcBool*)result.addr)->Set( true, true );
  }
  else
  {
    ((CcBool *)result.addr)->Set( true, false );
  }
  return (0);
}

static int
IsEmpty_l( Word* args, Word& result, int message, Word& local, Supplier s )
{  //To Judge whether a line value is empty
    result = qp->ResultStorage( s );
  
    if( ((CLine*)args[0].addr)->IsEmpty() )
    {
	((CcBool*)result.addr)->Set( true, true );
    }
    else
    {
	((CcBool *)result.addr)->Set( true, false );
    }
    return (0);
}

static int
IsEmpty_r( Word* args, Word& result, int message, Word& local, Supplier s )
{ 
    result = qp->ResultStorage( s );
    return (0);
}

/*
9.4.2 Value mapping functions of operator ~$=$~

*/
static int
SpatialEqual_pp( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if ( ((Point*)args[0].addr)->IsDefined() &&
       ((Point*)args[1].addr)->IsDefined() )
  {
    ((CcBool *)result.addr)->
      Set( true, *((Point*)args[0].addr) == *((Point*)args[1].addr) );
  }
  else
  {
    ((CcBool *)result.addr)->Set( false, false );
  }
  return (0);
}

static int
SpatialEqual_psps( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->Set( true, *((Points*)args[0].addr) == *((Points*)args[1].addr) );
  return (0);
}

static int
SpatialEqual_ll( Word* args, Word& result, int message, Word& local, Supplier s )
{   //to judge whether two line values are equal
    result = qp->ResultStorage( s );
    ((CcBool *)result.addr)->Set( true, *((CLine*)args[0].addr) == *((CLine*)args[1].addr) );
    return (0);
    
}

static int
SpatialEqual_rr( Word* args, Word& result, int message, Word& local, Supplier s )
{ 
    result = qp->ResultStorage( s );
    ((CcBool *)result.addr)->Set( true, true) ;
    return (0);
}

/*
9.4.3 Value mapping functions of operator ~$\neq$~

*/
static int
SpatialNotEqual_pp( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if ( ((Point*)args[0].addr)->IsDefined() &&
       ((Point*)args[1].addr)->IsDefined() )
  {
    ((CcBool *)result.addr)->
      Set( true, *((Point*)args[0].addr) != *((Point*)args[1].addr) );
  }
  else
  {
    ((CcBool *)result.addr)->Set( false, false );
  }
  return (0);
}

static int
SpatialNotEqual_psps( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->Set( true, *((Points*)args[0].addr) != *((Points*)args[1].addr) );
  return (0);
}

static int
SpatialNotEqual_ll( Word* args, Word& result, int message, Word& local, Supplier s )
{  //to judge whether two line values are not equal
    result = qp->ResultStorage( s );
    ((CcBool *)result.addr)->Set( true, !(*((CLine*)args[0].addr) == *((CLine*)args[1].addr)));
    return (0);
}

static int
SpatialNotEqual_rr( Word* args, Word& result, int message, Word& local, Supplier s )
{  
    result = qp->ResultStorage( s );
    ((CcBool *)result.addr)->Set( true, true);
    return (0);
}

/*
9.4.4 Value mapping functions of operator ~$<$~

*/
static int
SpatialLess_pp( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if ( ((Point*)args[0].addr)->IsDefined() &&
       ((Point*)args[1].addr)->IsDefined() )
  {
    ((CcBool *)result.addr)->
      Set( true, *((Point*)args[0].addr) < *((Point*)args[1].addr) );
  }
  else
  {
    ((CcBool *)result.addr)->Set( false, false );
  }
  return (0);
}

/*
9.4.5 Value mapping functions of operator ~$\leq$~

*/
static int
SpatialLessEqual_pp( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if ( ((Point*)args[0].addr)->IsDefined() &&
       ((Point*)args[1].addr)->IsDefined() )
  {
    ((CcBool *)result.addr)->
      Set( true, *((Point*)args[0].addr) <= *((Point*)args[1].addr) );
  }
  else
  {
    ((CcBool *)result.addr)->Set( false, false );
  }
  return (0);
}

/*
9.4.6 Value mapping functions of operator ~$>$~

*/
static int
SpatialGreater_pp( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if ( ((Point*)args[0].addr)->IsDefined() &&
       ((Point*)args[1].addr)->IsDefined() )
  {
    ((CcBool *)result.addr)->
      Set( true, *((Point*)args[0].addr) > *((Point*)args[1].addr) );
  }
  else
  {
    ((CcBool *)result.addr)->Set( false, false );
  }
  return (0);
}

/*
9.4.7 Value mapping functions of operator ~$\geq$~

*/
static int
SpatialGreaterEqual_pp( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if ( ((Point*)args[0].addr)->IsDefined() &&
       ((Point*)args[1].addr)->IsDefined() )
  {
    ((CcBool *)result.addr)->
      Set( true, *((Point*)args[0].addr) >= *((Point*)args[1].addr) );
  }
  else
  {
    ((CcBool *)result.addr)->Set( false, false );
  }
  return (0);
}

/*
9.4.8 Value mapping functions of operator ~intersects~

*/
static int
SpatialIntersects_psps( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true, ((Points*)args[0].addr)->Intersects( *((Points*)args[1].addr) ) );
  return (0);
}

static int
SpatialIntersects_ll( Word* args, Word& result, int message, Word& local, Supplier s )
{   //to judge whether two lines intersect each other. 
    result = qp->ResultStorage( s );
    CLine *cl1, *cl2;
    CHalfSegment chs1, chs2;
  
    cl1=((CLine*)args[0].addr);
    cl2=((CLine*)args[1].addr);
  
    for (int i=0; i<cl1->Size(); i++)
    {
	cl1->Get(i, chs1);
	if (chs1.GetLDP())   //only compare left halfsegments
	{
	    for (int j=0; j<cl2->Size(); j++)
	    {    
		cl2->Get(j, chs2);
		if (chs2.GetLDP())  //only compare left halfsegments
		{
		    if (chs1.Intersects(chs2)) 
		    {
			((CcBool *)result.addr)->Set( true, true );
			return (0);
		    }
		}
	    }
	}
    }
    ((CcBool *)result.addr)->Set( true, false);
    return (0);
}

static int
SpatialIntersects_rr( Word* args, Word& result, int message, Word& local, Supplier s )
{   
    result = qp->ResultStorage( s );
    ((CcBool *)result.addr)->Set( true, false);
    return (0);
}

/*
9.4.9 Value mapping functions of operator ~inside~

*/
static int
SpatialInside_psps( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->
    Set( true, ((Points*)args[0].addr)->Inside( *((Points*)args[1].addr) ) );
  return (0);
}

static int
SpatialInside_ll( Word* args, Word& result, int message, Word& local, Supplier s )
{   //to judge whether one line value is inside another
    result = qp->ResultStorage( s );
    CLine *cl1, *cl2;
    CHalfSegment chs1, chs2;
  
    cl1=((CLine*)args[0].addr);
    cl2=((CLine*)args[1].addr);
  
    for (int i=0; i<cl1->Size(); i++)
    {
	cl1->Get(i, chs1);
	if (chs1.GetLDP())   //only compare left halfsegments
	{
	    bool found=false;//found a seg in cl2 which contain chs1
	    for (int j=0; ((j<cl2->Size()) && !found); j++)
	    {    
		cl2->Get(j, chs2);
		if (chs2.GetLDP())  //only compare left halfsegments
		{
		    if ((chs1.Inside(chs2)))
		    {
			found=true;
		    }
		}
	    }
	    if (!found) 
	    {
		((CcBool *)result.addr)->Set( true, false);
		return (0);  
	    }
	}
    }
    ((CcBool *)result.addr)->Set( true, true);
    return (0);  
}

static int
SpatialInside_rr( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->Set( false, false );
  return (0);
}

static int
SpatialInside_pps( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  if ( ((Point*)args[0].addr)->IsDefined() )
  {
    ((CcBool *)result.addr)->
      Set( true, ((Point*)args[0].addr)->Inside( *((Points*)args[1].addr) ) );
  }
  else
  {
    ((CcBool *)result.addr)->Set( false, false );
  }
  return (0);
}

static int
SpatialInside_pl( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  Point *p=((Point*)args[0].addr);
  CLine *cl=((CLine*)args[1].addr);
  CHalfSegment chs;
  
  for (int i=0; i<cl->Size(); i++)
  {
      cl->Get(i, chs);
      if (chs.Contains(*p)) 
      {
	  ((CcBool *)result.addr)->Set( true, true );
	  return (0);
      }
  }
  ((CcBool *)result.addr)->Set( true, false );
  return (0);
  
}

static int
SpatialInside_pr( Word* args, Word& result, int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  ((CcBool *)result.addr)->Set( false, false );
  return (0);
}

/*
9.5 Definition of operators

Definition of operators is done in a way similar to definition of
type constructors: an instance of class ~Operator~ is defined.

Because almost all operators are overloaded, we have first do define an array of value
mapping functions for each operator. For nonoverloaded operators there is also such and array
defined, so it easier to make them overloaded.

*/
ValueMapping spatialisemptymap[] = { IsEmpty_p, IsEmpty_ps, IsEmpty_l, IsEmpty_r };
ValueMapping spatialequalmap[] = { SpatialEqual_pp, SpatialEqual_psps, SpatialEqual_ll, SpatialEqual_rr };
ValueMapping spatialnotequalmap[] = { SpatialNotEqual_pp, SpatialNotEqual_psps, SpatialNotEqual_ll, SpatialNotEqual_rr };
ValueMapping spatiallessmap[] = { SpatialLess_pp };
ValueMapping spatiallessequalmap[] = { SpatialLessEqual_pp };
ValueMapping spatialgreatermap[] = { SpatialGreater_pp };
ValueMapping spatialgreaterequalmap[] = { SpatialGreaterEqual_pp };
ValueMapping spatialintersectsmap[] = { SpatialIntersects_psps, SpatialIntersects_ll, SpatialIntersects_rr };
ValueMapping spatialinsidemap[] = { SpatialInside_psps, SpatialInside_ll, SpatialInside_rr, SpatialInside_pps, SpatialInside_pl, SpatialInside_pr };

ModelMapping spatialnomodelmap[] = { SpatialNoModelMapping, SpatialNoModelMapping, 
                                     SpatialNoModelMapping, SpatialNoModelMapping,
                                     SpatialNoModelMapping, SpatialNoModelMapping };

const string SpatialSpecIsEmpty = "(<text> point -> bool, points -> bool, line -> bool, region -> bool</text---><text> Returns whether the value is defined or not. </text--->)";
const string SpatialSpecEqual = "(<text> (point point) -> bool, (points points) -> bool, (line line) -> bool, (region region) -> bool</text---><text> Equal. </text--->)";
const string SpatialSpecNotEqual = "(<text> (point point) -> bool</text---><text> Not equal. </text--->)";
const string SpatialSpecLess = "(<text> (point point) -> bool</text---><text> Less than. </text--->)";
const string SpatialSpecLessEqual   = "(<text> (point point) -> bool</text---><text> Equal or less than. </text--->)";
const string SpatialSpecGreater = "(<text> (point point) -> bool</text---><text> Greater than. </text--->)";
const string SpatialSpecGreaterEqual = "(<text> (point point) -> bool</text---><text> Equal or greater than. </text--->)";
const string SpatialSpecIntersects = "(<text> (points points) -> bool, (line line) -> bool, (region region) -> bool</text---><text> Intersects. </text--->)";
const string SpatialSpecInside = "(<text> (points points) -> bool, (line line) -> bool, (region region) -> bool, (point points) -> bool, (point line) -> bool, (point region) -> bool</text---><text> Inside. </text--->)";

Operator spatialisempty( "isempty", SpatialSpecIsEmpty, 4, spatialisemptymap, spatialnomodelmap, SpatialSelectIsEmpty, SpatialTypeMapBool1 );
Operator spatialequal( "=", SpatialSpecEqual, 4, spatialequalmap, spatialnomodelmap, SpatialSelectCompare, SpatialTypeMapBool );
Operator spatialnotequal( "#", SpatialSpecNotEqual, 4, spatialnotequalmap, spatialnomodelmap, SpatialSelectCompare, SpatialTypeMapBool );
Operator spatialless( "<", SpatialSpecLess, 1, spatiallessmap, spatialnomodelmap, SimpleSelect, SpatialTypeMapBool );
Operator spatiallessequal( "<=", SpatialSpecLessEqual, 1, spatiallessequalmap, spatialnomodelmap, SimpleSelect, SpatialTypeMapBool );
Operator spatialgreater( ">", SpatialSpecGreater, 1, spatialgreatermap, spatialnomodelmap, SimpleSelect, SpatialTypeMapBool );
Operator spatialgreaterequal( ">=", SpatialSpecGreaterEqual, 1, spatialgreaterequalmap, spatialnomodelmap, SimpleSelect, SpatialTypeMapBool );
Operator spatialintersects( "intersects", SpatialSpecIntersects, 3, spatialintersectsmap, spatialnomodelmap, SpatialSelectSets1, SpatialTypeMapBool );
Operator spatialinside( "inside", SpatialSpecInside, 6, spatialinsidemap, spatialnomodelmap, SpatialSelectSets2, SpatialTypeMapBool );

/*
10 Creating the Algebra

*/

class SpatialAlgebra : public Algebra
{
 public:
  SpatialAlgebra() : Algebra()
  {
    AddTypeConstructor( &point );
    AddTypeConstructor( &points );
    AddTypeConstructor( &halfsegment );
    AddTypeConstructor( &line );
    AddTypeConstructor( &region );

    point.AssociateKind("DATA");   	//this means that point and rectangle
    points.AssociateKind("DATA");   	//can be used in places where types
    line.AssociateKind("DATA");	//of kind DATA are expected, e.g. in
    region.AssociateKind("DATA");	//tuples.
    halfsegment.AssociateKind("DATA");
	
    AddOperator( &spatialisempty );
    AddOperator( &spatialequal );
    AddOperator( &spatialnotequal );
    AddOperator( &spatialless );
    AddOperator( &spatiallessequal );
    AddOperator( &spatialgreater );
    AddOperator( &spatialgreaterequal );
    AddOperator( &spatialintersects );
    AddOperator( &spatialinside );
  }
  ~SpatialAlgebra() {};
};

SpatialAlgebra spatialAlgebra; 

/*
11 Initialization

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
  return (&spatialAlgebra);
}



