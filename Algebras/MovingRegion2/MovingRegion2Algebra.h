/*

//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]
//[TOC] [\tableofcontents]
//[ue] [\"u]
//[ae] [\"a]
//[oe] [\"o]

[1] The ~MovingRegion2Algebra~

May 2012, initial version created by Stefanie Renner for master
thesis with Prof. Dr. G[ue]ting, Fachbereich Informatik,
Fernuniversit[ae]t Hagen.

[TOC]

1 Introduction

This file contains the class definitions of ~MSegmentData2~, 
PreciseMSegmentData,
PreciseInterval, ~URegionEmb2~, ~URegion2~ and ~MRegion2~, which
are implemented in ~MovingRegion2Algebra.cpp~.

Please see ~MovingRegion2Algebra.cpp~ for more details on the
~MovingRegion2Algebra~.

*/

#ifndef MOVINGREGION2ALGBRA_H_
#define MOVINGREGION2ALGBRA_H_
//#define MR2_DEBUG

#include <queue>
#include <stdexcept>

//includes for the Gnu Multiple Precision (GMP) library
#include <gmp.h>
#include <gmpxx.h>

#include "Algebra.h"
#include "NestedList.h"
#include "ListUtils.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include "TemporalAlgebra.h"
#include "DateTime.h"
#include "MovingRegionAlgebra.h"


//Some forward declarations...
class MRegion2;
class URegion2;
class MSegmentData2;
class PreciseMSegmentData;
class ProvisionalPoint;
class ProvisionalSegment;

/*
1 Helper classes for geometrical calculations in different precisions

*/

class BasicBBox2D {

public:

	int minX;
	int maxX;
	int minY;
	int maxY;

	BasicBBox2D(int minX, int minY, int maxX, int maxY);
	bool overlapsWith (BasicBBox2D& other);
};

class ProvisionalBBox2D {

public:

	double minX;
	double maxX;
	double minY;
	double maxY;

	ProvisionalBBox2D(double minX, double minY, double maxX, 
		double maxY);

	bool overlapsWith (BasicBBox2D& other);
	bool overlapsWith (ProvisionalBBox2D& other);
};

class PreciseBBox2D {

public:

	mpq_class minX;
	mpq_class maxX;
	mpq_class minY;
	mpq_class maxY;

	PreciseBBox2D(mpq_class minX, mpq_class minY, mpq_class maxX, 
		mpq_class maxY);

	bool overlapsWith (PreciseBBox2D& other);
};

class GridPoint {

public:

	int x;
	int y;
	bool isBasic;
	int xErr;
	int yErr;

	GridPoint(int x, int y);
	GridPoint(int x, int y, bool isBasic);

	bool operator==(const GridPoint& other) const;
	ProvisionalPoint transformToProvisional();
};

class ProvisionalPoint {

public:

	double x;
	double y;
	double xErr;
	double yErr;

	ProvisionalPoint(double x, double y, double xErr, double yErr);

	bool operator==(const ProvisionalPoint& other) const;

};

class PrecisePoint {

public:

	mpq_class x;
	mpq_class y;

	PrecisePoint(mpq_class x, mpq_class y);
	PrecisePoint(mpq_class x1, int x2, mpq_class y1, int y2);

	bool operator==(const PrecisePoint& other) const;

};

class GridPointSegment {

public:

	GridPoint point1;
	GridPoint point2;
	bool isBasic;

	GridPointSegment(GridPoint point1, GridPoint point2);
	GridPointSegment(GridPoint point1, GridPoint point2, bool isBasic);

	bool operator==(const GridPointSegment& other) const;
	BasicBBox2D getBasicBbox2D();
	ProvisionalSegment transformToProvisional();

};

class ProvisionalSegment {

public:

	ProvisionalPoint point1;
	ProvisionalPoint point2;

	ProvisionalSegment(ProvisionalPoint point1, 
			ProvisionalPoint point2);

	bool operator==(const ProvisionalSegment& other) const;

};

class PreciseSegment {

public:

	PrecisePoint point1;
	PrecisePoint point2;

	PreciseSegment(PrecisePoint point1, PrecisePoint point2);

	bool operator==(const PreciseSegment& other) const;

};

class GridPointTrapezium {

public:

	GridPointSegment segment1;
	GridPointSegment segment2;
	bool isBasic;

	GridPointTrapezium(GridPointSegment segment1, 
		GridPointSegment segment2);
	GridPointTrapezium(GridPointSegment segment1, 
		GridPointSegment segment2, bool isBasic);

	BasicBBox2D getBasicBbox2D();
	GridPointSegment getConnectingSegment1();
	GridPointSegment getConnectingSegment2();

	bool operator==(const GridPointTrapezium& other) const;

};

class ProvisionalTrapezium {

public:

	ProvisionalSegment segment1;
	ProvisionalSegment segment2;

	ProvisionalTrapezium(ProvisionalSegment segment1, 
		ProvisionalSegment segment2);

	ProvisionalBBox2D getProvisionalBbox2D();
	ProvisionalSegment getConnectingSegment1();
	ProvisionalSegment getConnectingSegment2();

	bool operator==(const ProvisionalTrapezium& other) const;

};

class PreciseTrapezium {

public:

	PreciseSegment segment1;
	PreciseSegment segment2;

	PreciseTrapezium(PreciseSegment segment1, PreciseSegment segment2);

	PreciseBBox2D getPreciseBbox2D();
	PreciseSegment getConnectingSegment1();
	PreciseSegment getConnectingSegment2();

	bool operator==(const PreciseTrapezium& other) const;

};


/*
1 Supporting classes and class template

1.1 Class ~MSegmentData2~

The class ~MSegmentData2~ describes a moving segment within a region unit.
All coordinates are described on an integer grid, they are represented by
the lower left point of the respective grid cell.
Beside start and end points of the segments for the initial and final
instant, the class contains attributes to describe the behaviour during
the initial and final instant and its orientation with the region unit.

*/

class MSegmentData2 {

/*
1.1.1 Private attributes

  * ~faceno~: The number of the region's face to which the segment belongs.

  * ~cycleno~: The number of the face's cycle, to which the segment belongs.

  * ~segmentno~: The number of the segment in its cycle.

  * ~insideAbove~: ~true~, if the region's interior is left or above of the
	segment. Please note that this is independent of the degeneration status
	of the segment.

  * ~isBasicSegment~: ~true~, if the coordinates' representation on the integer
  	grid is identical with their precise representation for all of the
  	coordinates, and thus no additional precise information for this segment
  	exist.

  * ~degeneratedInitialInitial~, ~degeneratedFinalFinal~: Specifies whether
	the segment degenerated in the initial or final instant. If so, these
	attributes indicate too whether it is still border of any region area
	even though it is degenerated.

  * ~degeneratedInitialNext~, ~degeneratedFinalNext~: If segments merge into
	a single segments due to degeneration, these attributes are the next
	merged degenerated segments in the region unit's list of segments. The
	last segment will have the value $-1$ in these attributes.

  * ~initialStartX~, ~initialStartY~, ~initialEndX~, ~initialEndY~: The
	coordinates of the segment's start and end points in the initial instant.

  * ~finalStartX~, ~finalStartY~, ~finalEndX~, ~finalEndY~: The
	coordinates of the segment's start and end points in the initial instant.

  * ~pointInitial~, ~pointFinal~: A segment might be reduced to a point in the
	initial or final instant (but not both). This is indicated by these
	values of these two attributes. Please note that reduction of a segment
	to a point and degeneration of a segment are different concepts.

The constructor assures that the segment in initial and final instant is
collinear.

*/
	unsigned int faceno;
	unsigned int cycleno;
	unsigned int segmentno;

	bool insideAbove;
	bool isBasicSegment;

	int degeneratedInitialNext;
	int degeneratedFinalNext;
	temporalalgebra::DegenMode degeneratedInitial;
	temporalalgebra::DegenMode degeneratedFinal;

	int initialStartX;
	int initialStartY;
	int initialEndX;
	int initialEndY;

	int finalStartX;
	int finalStartY;
	int finalEndX;
	int finalEndY;

	bool pointInitial;
	bool pointFinal;

 public:

/*
1.1.1 Constructors

The default constructors does nothing.

*/
	MSegmentData2() { }

/*
This constructor creates a basic segment in the integer grid. ~isBasicSegment~
is set to ~true~.

This constructor sets the ~faceno~, ~cycleno~, ~segmentno~, ~insideAbove~,
~initialStartX~, ~initialStartY~, ~initialEndX~, ~initialEndY~,
~finalStartX~, ~finalStartY~, ~finalEndX~ and ~finalEndY~ attributes
according to the parameters.

~degeneratedInitialNext~ and ~degeneratedFinalNext~ are initialised with
$-1$, ~degeneratedInitial~ and ~degeneratedFinal~ with ~DGM\_UNKNOWN~.

~pointInitial~ and ~pointFinal~ are calculated from the parameters and
an exception is thrown if segment is reduced to a point in initial and
final instant.

It is assured that the segment is collinear in initial and final instant
and an exception is thrown otherwise.

*/
	MSegmentData2(
		unsigned int fno,
		unsigned int cno,
		unsigned int sno,
		bool ia,
		int isx,
		int isy,
		int iex,
		int iey,
		int fsx,
		int fsy,
		int fex,
		int fey);

/*
This constructor creates the basic part for a non-basic segment in the
integer grid. ~isBasicSegment~ is set to ~false~.

This constructor sets the ~faceno~, ~cycleno~, ~segmentno~, ~insideAbove~,
~initialStartX~, ~initialStartY~, ~initialEndX~, ~initialEndY~,
~finalStartX~, ~finalStartY~, ~finalEndX~ and ~finalEndY~ attributes
according to the parameters.

~degeneratedInitialNext~ and ~degeneratedFinalNext~ are initialised with
$-1$, ~degeneratedInitial~ and ~degeneratedFinal~ with ~DGM\_UNKNOWN~.

~pointInitial~ and ~pointFinal~ are calculated from the parameters and
an exception is thrown if segment is reduced to a point in initial and
final instant.

It is assured that the segment is collinear in initial and final instant
and an exception is thrown otherwise.

*/
	MSegmentData2(
			unsigned int fno,
			unsigned int cno,
			unsigned int sno,
			bool ia,
			int isx,
			int isy,
			int iex,
			int iey,
			int fsx,
			int fsy,
			int fex,
			int fey,
			PreciseMSegmentData& preciseSegment,
			DbArray<int>* preciseCoordinates);

/*
This constructor creates any kind of segment (basic or non-basic) 
from a pointer to an existing segment.

*/


	MSegmentData2(MSegmentData2* segmentPointer);

/*
1.1.1 Attribute read access methods

*/
	unsigned int GetFaceNo(void) const { return faceno; }
	unsigned int GetCycleNo(void) const { return cycleno; }
	unsigned int GetSegmentNo(void) const { return segmentno; }
	int GetInitialStartX(void) const { return initialStartX; }
	int GetInitialStartY(void) const { return initialStartY; }
	int GetInitialEndX(void) const { return initialEndX; }
	int GetInitialEndY(void) const { return initialEndY; }
	int GetFinalStartX(void) const { return finalStartX; }
	int GetFinalStartY(void) const { return finalStartY; }
	int GetFinalEndX(void) const { return finalEndX; }
	int GetFinalEndY(void) const { return finalEndY; }
	bool GetInsideAbove(void) const { return insideAbove; }
	bool GetIsBasicSegment(void) const { return isBasicSegment; }
	bool GetPointInitial(void) const { return pointInitial; }
	bool GetPointFinal(void) const { return pointFinal; }
	temporalalgebra::DegenMode GetDegeneratedInitial(void) const {
		return degeneratedInitial; }
	temporalalgebra::DegenMode GetDegeneratedFinal(void) const {
		return degeneratedFinal; }
	int GetDegeneratedInitialNext(void) const {
		return degeneratedInitialNext; }
	int GetDegeneratedFinalNext(void) const {
		return degeneratedFinalNext; }
	bool IsValid() const {
		return true; }

/*
1.1.1 Attribute write access methods

*/
	void SetFinalStartX(const int v) { finalStartX = v; }
	void SetFinalStartY(const int v) { finalStartY = v; }
	void SetFinalEndX(const int v) { finalEndX = v; }
	void SetFinalEndY(const int v) { finalEndY = v; }
	void SetInitialStartX(const int v) {initialStartX = v; }
	void SetInitialStartY(const int v) {initialStartY = v; }
	void SetInitialEndX(const int v) {initialEndX = v; }
	void SetInitialEndY(const int v) {initialEndY = v; }
	void SetInsideAbove(const bool ia) { insideAbove = ia; }
	void SetIsBasicSegment(const bool ibs) { isBasicSegment = ibs; }
	void SetPointInital(const bool p) { pointInitial = p; }
	void SetPointFinal(const bool p) { pointFinal = p; }
	void SetDegeneratedInitial(const temporalalgebra::DegenMode dm) {
		degeneratedInitial = dm; }
	void SetDegeneratedFinal(const temporalalgebra::DegenMode dm) {
		degeneratedFinal = dm; }
	void SetDegeneratedInitialNext(const int dn) {
		degeneratedInitialNext = dn; }
	void SetDegeneratedFinalNext(const int dn) {
		degeneratedFinalNext = dn; }

/*
1.1.1 Other methods

Generate new ~MSegmentData2~ instant in ~rDms~ from current instant, where 
the original interval ~origIv~ has been restricted to ~restrIv~.

*/
	void restrictToInterval(
		temporalalgebra::Interval<Instant> origIv,
		temporalalgebra::Interval<Instant> restrIv,
		MSegmentData2& rDms) const;

/*
Return ~string~ representation of moving segment.

*/
	string ToString(void) const;
};

/*
1.1 Class ~PreciseMSegmentData~

The class ~PreciseMSegmentData~ describes the precise representation of a
moving segment within a region unit.
Kept are not the absolute coordinates of start and end points in initial and
final instant, but just the difference between the absolute coordinates
and their respective representation in the integer grid.
The absolute value can be derived by adding the precise coordinate value
from this class with the integer value of the respective instance of 
MSegmentData2.

*/

class PreciseMSegmentData {

/*
1.1.1 Private attributes

  * ~isxStartPos~, ~isyStartPos~, ~iexStartPos~, ~ieyStartPos~, ~fsxStartPos~, 
~fsyStartPos~,  ~fexStartPos~, ~feyStartPos~ is the index position of the first
char representing the given coordinate in the respective DbArray, with isx 
initialStartX,  isy initialStartY, iex initialEndX, iey initialEndY, fsx 
finalStartX and so on.

  * ~isxNumOfChars~, ~isyNumOfChars~, ~iexNumOfChars~, ~ieyNumOfChars~, 
~fsxNumOfChars~, ~fsyNumOfChars~, ~fexNumOfChars~, ~feyNumOfChars~ is the 
number of all chars representing this respective coordinate instance in the 
DbArray.

*/

	int isxStartPos;
	int isyStartPos;
	int iexStartPos;
	int ieyStartPos;
	int fsxStartPos;
	int fsyStartPos;
	int fexStartPos;
	int feyStartPos;
	int isxNumOfChars;
	int isyNumOfChars;
	int iexNumOfChars;
	int ieyNumOfChars;
	int fsxNumOfChars;
	int fsyNumOfChars;
	int fexNumOfChars;
	int feyNumOfChars;

public:

/*
The default constructor does nothing

*/

	PreciseMSegmentData() {}

/*
Constructor that initializes all the indices.

*/

	PreciseMSegmentData(int isxPos, int isyPos, int iexPos, int ieyPos, 
		int fsxPos, int fsyPos, int fexPos, int feyPos,
		int isxNum, int isyNum, int iexNum, int ieyNum, int fsxNum, 
		int fsyNum, int fexNum, int feyNum);

	PreciseMSegmentData(int startPos);

/*
1.1.1 Attribute read access methods

All these methods fetch the chars representing the given coordinate from the 
DbArray using the indices given in this instance's private attributes, and 
convert them to the correct instance of type mpq\_class, representing the 
value of the given coordinate.

*/

	mpq_class GetInitialStartX(const 
		DbArray<int>* preciseCoordinates) const;
	mpq_class GetInitialStartY(const 
		DbArray<int>* preciseCoordinates) const;
	mpq_class GetInitialEndX(const 
		DbArray<int>* preciseCoordinates) const;
	mpq_class GetInitialEndY(const 
		DbArray<int>* preciseCoordinates) const;
	mpq_class GetFinalStartX(const 
		DbArray<int>* preciseCoordinates) const;
	mpq_class GetFinalEndX(const 
		DbArray<int>* preciseCoordinates) const;
	mpq_class GetFinalStartY(const 
		DbArray<int>* preciseCoordinates) const;
	mpq_class GetFinalEndY(const 
		DbArray<int>* preciseCoordinates) const;

/*
1.1.1 Attribute write access methods

All these methods take the argument of type mpq\_class, convert it to an
arbitrary number of chars and stores these chars in the given DbArray. The
private attributes representing the array indices to restore the coordinates
are of course also updated.

*/

	void SetInitialStartX (mpq_class x, 
		DbArray<int>* preciseCoordinates);
	void SetInitialEndX (mpq_class x, DbArray<int>* preciseCoordinates);
	void SetInitialStartY (mpq_class x, 
		DbArray<int>* preciseCoordinates);
	void SetInitialEndY (mpq_class x, DbArray<int>* preciseCoordinates);
	void SetFinalStartX (mpq_class x, DbArray<int>* preciseCoordinates);
	void SetFinalStartY (mpq_class x, DbArray<int>* preciseCoordinates);
	void SetFinalEndX (mpq_class x, DbArray<int>* preciseCoordinates);
	void SetFinalEndY (mpq_class x, DbArray<int>* preciseCoordinates);

	void SetAll (mpq_class theValue, DbArray<int>* preciseCoordinates) {
		SetInitialStartX (theValue, preciseCoordinates);
		SetInitialStartY (theValue, preciseCoordinates);
		SetInitialEndX (theValue, preciseCoordinates);
		SetInitialEndY (theValue, preciseCoordinates);
		SetFinalStartX (theValue, preciseCoordinates);
		SetFinalStartY (theValue, preciseCoordinates);
		SetFinalEndX (theValue, preciseCoordinates);
		SetFinalEndY (theValue, preciseCoordinates);
	}
};

/*
1.1 Class ~PreciseInterval~

The class ~PreciseInterval~ represents the precise information about the 
start and end instant of a time interval. Both values are not kept by 
their absolute value, but just as the difference from the respective 
integer representation. Both must be added to get the absolute value.

*/

class PreciseInterval {

/*
1.1.1 Private attributes

  * ~startStartPos~, ~endStartPos~ is the index position of the first 
char representing the given coordinate in the respective DbArray.

  * ~startNumOfChars~, ~endNumOfChars~ is the number of all chars representing
this respective coordinate instance in the DbArray.

*/

	int startStartPos;
	int endStartPos;
	int startNumOfChars;
	int endNumOfChars;

public:

/*
The default constructor does nothing

*/

	PreciseInterval() {}

/*
This constructors initializes the private attributes.

*/

	PreciseInterval(int startPos, int endPos, int startNum, int endNum);
	PreciseInterval(int pos);

/*
1.1.1 Attribute read and write access methods.

The internal functionality of these access methods is just like of those
from class PreciseMSegmentData. Private attributes are used as indices to
the given char arrays to retrieve the objects, and when object values 
are updated, the indices are updated as well.

*/

	mpq_class GetPreciseInitialInstant(const 
		DbArray<int>* preciseInstants);
	mpq_class GetPreciseFinalInstant(const 
		DbArray<int>* preciseInstants);

	void SetPreciseInitialInstant (mpq_class initial, 
		DbArray<int>* preciseInstants);
	void SetPreciseFinalInstant (mpq_class final, 
		DbArray<int>* preciseInstants);

};



/*
1 Class ~IRegion2~

The code for this data type is fairly simple because it can mostly rely on the
instantiation of the class template ~temporalalgebra::Intime~ with class 
~Region~. Just a
number of specialized constructors and methods must be overwritten to assure
that the ~DBArray~ in ~Region~ is properly handled. ~temporalalgebra::Intime~
 itself does not care about whether its ~value~ attribute is of a class with
 ~DBArray~ attributes or not.

*/

class IRegion2 : public temporalalgebra::Intime<Region> {
public:
/*
1.1 Constructors

The default constructor does nothing.

*/
    IRegion2(void) {}

/*
Create a new ~IRegion2~ instance without setting attribute default values.
The ~value~ attribute is properly initialized with an empty region, though,
to assure that its ~DBArray~ is properly initialized.

*/
    IRegion2(const bool dummy);
/*
Create a new ~IRegion2~ instance and initialize it with the value of ~ir~.

*/
    IRegion2(const IRegion2& ir);

/*
1.1 Methods for algebra integration

Clone this ~IRegion2~ instance and return a pointer to the new instance.

*/
    IRegion2* Clone(void) const;

/*
~DBArray~ access methods. These do use the ~value~ attributes ~DBArray~.

*/
    int NumOfFLOBs(void) const;
    Flob* GetFLOB(const int i);
};

/*
1 Class ~URegionEmb2~

~URegion2~ and ~MRegion2~ use the common class ~URegionEmb2~ due to 
the following
situation:

Moving regions contain a variable number of region units, and each
region unit contains a variable number of moving segments. In theory, this
requires a two-level data structure of DbArrays, which is not
supported by SECONDO. Instead, a moving region object of class ~MRegion2~
contains several ~DBArray~ instances. The first ~DbArray~ instance contains all
region units, represented as objects of class ~URegionEmb2~. The second
and third ~DbArray~ contains the moving segments of all regions units in a 
moving
region, for the integer grid and the precise representations, respectively.
Each ~URegionEmb2~ object knows the start index and the total number
of its moving segments in these second and third ~DBArray~ instances. Methods of
~URegionEmb2~, which need to access segments, receive three ~DbArray~
instances as explicit parameters: One of element type MSegmentData2 representing
the segments on the integer grid, one of element type PreciseMSegmentData 
representing
the precise coordinates and a third one of elemente type char for the internal
representation of the values of these precise coordinates.

Since region units, which are not contained in moving regions, need to be
available as SECONDO datatype too, the class ~URegion2~ has been implemented.
It has three ~DBArrays~ to store moving segments and an instance of 
~URegionEmb2~
as attributes. This facilitates reusing the ~URegionEmb2~ functionality for
standalone region unit values without duplication of code.

*/

class URegionEmb2 {

private:

  friend class MRegion2;

/*
1.1 Private attributes

  * ~segmentsStartPos~ is the index in moving segments array, where this
instances segments are starting.

  * ~segmentsNum~ is the number of segments in the ~URegion~ instance.

  * ~bbox~ contains the bounding box of the region unit.

*/
      unsigned int segmentsStartPos;
      unsigned int segmentsNum;

      Rectangle<3> bbox;

public:
/*
1.1 Public attributes

~timeInterval~ contains the interval of the unit. While a private
attribute with proper public access methods would be more clean, we
leave this attribute public to mimic a ~Unit~'s behaviour.
In the same way, ~pInterval~ contains an instance of the class
representing a precise time interval (the derivation of the two instances
representing a time interval from the respective integer grid representations).

*/
    temporalalgebra::Interval<Instant> timeInterval;
    PreciseInterval pInterval;

/*
1.1 Constructors

The default constructor does nothing.

*/
    URegionEmb2() { };

    URegionEmb2( const bool dummy );

/*
Constructor, which creates an empty unit for the specified interval.
The ~URegionEmb2~ instance will store its segments starting at ~pos~
in the moving segments array.

*/

	URegionEmb2(
		const temporalalgebra::Interval<Instant>& tiv,
		const PreciseInterval& pTiv,
		unsigned int pos);

/*
Constructor which is called to construct a unit from an existing
temporalalgebra::MRegion unit (of previous implementation)

*/

	URegionEmb2(
                     DbArray<MSegmentData2>* segments,
			DbArray<PreciseMSegmentData>* preciseSegments,
			DbArray<int>* preciseCoordinates,
			DbArray<int>* preciseInstants,
			const temporalalgebra::Interval<Instant>& iv,
			PreciseInterval& piv,
			const temporalalgebra::URegionEmb& origUremb,
                     const DbArray<temporalalgebra::MSegmentData>* origSegments,
			unsigned int pos,
			unsigned int scaleFactor);

/*
1.1 Moving segments access methods

Get number of segments, get index of starting segment,
get the minimum bounding box,
get specified segment, write specified segment.

*/
	int GetSegmentsNum(void) const;
	const int GetStartPos() const;
	void SetSegmentsNum(int i);
	void SetStartPos(int i);

	void GetSegment(
		const DbArray<MSegmentData2>* segments,
		int pos,
		MSegmentData2& dms) const;
	void PutSegment(
		DbArray<MSegmentData2>* segments,
		int pos,
		const MSegmentData2& dms,
		const bool isNew = false);
	void GetPreciseSegment(
			const DbArray<PreciseMSegmentData>* preciseSegments,
			int pos,
			PreciseMSegmentData& pdms) const;
	void PutPreciseSegment(
			DbArray<PreciseMSegmentData>* preciseSegments,
			int pos,
			const PreciseMSegmentData& pdms);
	void PutCoordinate(
			DbArray<int>* coordinates,
			int pos,
			const int& coord);



/*
Set the value of the ~insideAbove~ attribut of segment as position ~pos~ to
the value ~insideAbove~. This is required by function ~InURegionEmb2()~.

*/
	void SetSegmentInsideAbove(
		DbArray<MSegmentData2>* segments,
		int pos,
		bool insideAbove);

/*
Adds a segment to this ~URegionEmb2~ instance.

~cr~ is a ~Region~ instance, which is used to check whether the ~URegionEmb2~
instance represents a valid region in the middle of its interval, when no
degeneration can occur. All segments added to the ~URegionEmb2~ instance are
added as non-moving segments to ~cr~ too.

~rDir~ is used to check the direction of a cylce, which is required to
finalise the ~insideAbove~ attributes of the segments.

Both ~cr~ and ~rDir~ are checked outside the methods of ~URegionEmb2~, they
are just filled by ~AddSegment()~.

~faceno~, ~cycleno~, ~segmentno~ and ~pointno~ are the obvious indices
in the ~URegionEmb2~ unit's segment.

~intervalLen~ is the length of the interval covered by this ~URegionEmb2~
instance.

~start~ and ~end~ contain the initial and final positions of the segment
in list representation, which will be added to the ~URegionEmb2~ instance

~AddSegment()~ performs numerous calculations and checks. See the method
description below for details.

*/
	bool AddSegment(
		DbArray<MSegmentData2>* segments,
		DbArray<PreciseMSegmentData>* preciseSegments,
		DbArray<int>* preciseCoordinates,
		DbArray<int>* preciseInstants,
		Region& cr,
		Region& rDir,
		unsigned int faceno,
		unsigned int cycleno,
		unsigned int segmentno,
		unsigned int partnerno,
		DateTime& intervalLen,
		ListExpr start,
		ListExpr end);




/*
1.1 Methods for database operators

*/

	void Transform(double deltaX, double deltaY,
		DbArray<MSegmentData2>* segments, 
		DbArray<PreciseMSegmentData>* preciseSegments,
		DbArray<int>* preciseCoordinates);
	void Scale(double deltaX, double deltaY,
		DbArray<MSegmentData2>* segments, 
		DbArray<PreciseMSegmentData>* preciseSegments,
		DbArray<int>* preciseCoordinates);

	bool Coarse(const DbArray<MSegmentData2>* segments,
		const DbArray<PreciseMSegmentData>* preciseSegments,
		const DbArray<int>* preciseInstants,
		const int scaleFactor,
		DbArray<temporalalgebra::MSegmentData>* coarseSegments,
		temporalalgebra::URegionEmb& coarseUremb);



/*
Returns the ~Region~ value of this ~URegionEmb2~ unit at instant ~t~
in ~result~.

*/

	void TemporalFunction(
		const DbArray<MSegmentData2>* segments,
		const DbArray<PreciseMSegmentData>* preciseSegments,
		const DbArray<int>* preciseCoordinates,
		const DbArray<int>* preciseInstants,
		const Instant& t,
		const mpq_class& pt,
		const int scaleFactor,
		Region& result,
		bool ignoreLimits = false) const;

/*
Return the bounding box of the region unit. This is an $O(1)$ operation
since the bounding box is calculated when the region unit is created.

*/
	const Rectangle<3> BoundingBox(const Geoid* geoid = 0) const;

/*
Sets the bounding box.

Use it carefully to avoid inconsistencies.

*/
	void SetBBox(Rectangle<3> box) { bbox = box; }

/*
1.1 Methods to act as unit in ~Mapping~ template.

*/
	bool IsValid(void) const;

	bool Disjoint(const URegionEmb2& ur) const;
	bool TU_Adjacent(const URegionEmb2& ur) const;
	bool operator==(const URegionEmb2& ur) const;
	bool Before(const URegionEmb2& ur) const;

	bool EqualValue( const URegionEmb2& i )
	{
	  return false;
	};

/*
This method is not implemented since it is not required by the current
version of the algebra but is required by the ~Mapping~ template.. Calls
will run into a failed assertion.

*/
	bool Compare(const URegionEmb2* ur) const;


/*
The assignment operator

*/
	URegionEmb2& operator=(const URegionEmb2&);

	ostream& Print(ostream &os) const
	{
	  os << "( URegionEmb2 NOT IMPLEMENTED YET )";
	  return os;
	};

	size_t HashValue() const;

/*
SetDefined is used within Mappings. Since Mappings may only contain defined
units and URegionEmb2 is only used within URegion2 and MRegion2 objects,
 we give a dummy implementation:

*/
	void SetDefined( const bool def ){
	  return;
	}

/*
IsDefined is used within Mappings. Since Mappings may only contain defined
units and URegionEmb2 is only used within URegion2 and MRegion2 objects, 
we give a dummy implementation:

*/
	bool IsDefined() const {
	  return true;
	}

}; //end of class URegionEmb2

/*
1 Class ~URegion2~

Instances of class ~URegion2~ represent SECONDO ~uregion~ objects.

*/

class URegion2 : public temporalalgebra::SpatialTemporalUnit<Region, 3> {
private:

  friend class MRegion2; 
  // needed due to the cruel design of this classes...

/*
1.1 Private attributes

  * ~segments~ contains the moving segments of this region unit.

  * ~uremb~ is an instance of ~URegionEmb2~, which will be used to provide most
of the functionality. Please note that the ~uremb~ is using ~segments~
through a pointer to ~segments~, which is explicitely passed to ~uremb~'s
methods.

*/

  DbArray<MSegmentData2> segments;
  DbArray<PreciseMSegmentData> preciseSegments;
  DbArray<int> preciseCoordinates;
  DbArray<int> preciseInstants;
  URegionEmb2 uremb;
  int scaleFactor;

  public:
/*
1.1 Constructors

The default constructor does nothing.

*/
  URegion2() { }

  
/*
This constructor creates an URegion2 by a given list of MSegmentData2.
The MSegmentData2 are ordered, so that matching Segments are together.

*/
  URegion2(vector<MSegmentData2> linelist, 
           const temporalalgebra::Interval<Instant> &tiv);

/*
Use the following constructor to declare temporal object variables etc.

*/

  URegion2(bool is_defined){ SetDefined(is_defined); }

/*
Constructor, which creates an region unit with ~segments~ prepared for ~n~
elements (use ~0~ for ~n~ to create an empty region unit).

*/
  URegion2(unsigned int n);

/*
Constructor, which creates an uregion2 object from an existing uregion object.

*/
  URegion2(temporalalgebra::URegion& coarseRegion, const int scaleFactor);

/*
Create a URegion2 object by copying data from a given MRegion2 object.

*/
  
  URegion2(int i, MRegion2& ur);

/*
1.1 Attribute access method

Set and get the ~uremb~ attribute. Required for function ~InURegion2()~.

*/
  void SetEmbedded(const URegionEmb2* p) {
	uremb = *p;
	uremb.pInterval = p->pInterval;
	SetDefined( true );
  }
  URegionEmb2* GetEmbedded(void) { return &uremb; }

/*
1.1 Methods for database operators

Returns the ~Region~ value of this ~URegion2~ unit at instant ~t~
in ~result~.

*/

  virtual void TemporalFunction(const Instant& t,
		Region& result,
		bool ignoreLimits = false) const;
/*

Adds the MovingSegment ~newSeg~ to the Union and cares for the BoundingBox

*/
  void AddSegment(MSegmentData2 newSeg);
  void AddPreciseSegment(PreciseMSegmentData newSeg);
  void AddCoordinate(int newCoord);

/*

This Method checks, if the Time periods of the $newRegion$ are equal 
to the URegion2,
and copies the MSegments from the new one to the old one.

*/
  void AddURegion2(URegion2 *newRegion);

/*
~At()~ and ~Passes()~ are not yet implemented. Stubs
required to make this class non-abstract.

*/

  virtual bool At(const Region& val,
				  TemporalUnit<Region>& result) const;
  virtual bool Passes(const Region& val) const;

/*
Return the internal arrays containing the moving segments for read-only access.

*/
  const DbArray<MSegmentData2>* GetMSegmentData2(){
	 return &segments;
  }

  const DbArray<PreciseMSegmentData>* GetPreciseMSegmentData(){
	  return &preciseSegments;
  }

  const DbArray<int>* GetPreciseCoordinates(){
	  return &preciseCoordinates;
  }

  const DbArray<int>* GetPreciseInstants(){
	  return &preciseInstants;
  }

  const int GetScaleFactor(void);
  void SetScaleFactor(int factor);

/*
Return the bounding box of the region unit. This is an $O(1)$ operation
since the bounding box is calculated when the region unit is created.

*/
  virtual const Rectangle<3> BoundingBox(const Geoid* geoid = 0) const;

/*
1.1 Methods for algebra integration

~DBArray~ access. These make sense on ~URegion2~ instances with own segments
storage only, and will run into failed assertions for other instances.

*/
  int NumOfFLOBs(void) const;
  Flob *GetFLOB(const int i);

/*
Returns the ~sizeof~ of a ~URegion2~ instance.

*/
  virtual size_t Sizeof() const;

  virtual void SetDefined( bool Defined )
  {
	this->del.isDefined = Defined;
	if( !Defined ){
	  segments.clean();
	}
  }

/*
Clone ~URegion2~ instance. Please note that resulting instance will have
its own moving segments storage, even if the cloned instance has not.

*/
  virtual URegion2* Clone(void) const;

/*
Copy ~URegion2~ instance. Please note that resulting instance will have
its own moving segments storage, even if the copied instance has not.

*/
  virtual void CopyFrom(const Attribute* right);

/*
Print method, primarly used for debugging purposes

*/

	virtual ostream& Print( ostream &os ) const
	{
	  if( IsDefined() )
		{
		  os << "URegion2: " << "( (";
		  os << timeInterval.start.ToString();
		  os << " ";
		  os << timeInterval.end.ToString();
		  os<<" "<<(timeInterval.lc ? "TRUE " : "FALSE ");
		  os<<" "<<(timeInterval.rc ? "TRUE) " : "FALSE) ");
		  // print specific stuff:
		  os << " SegStartPos=" << uremb.GetStartPos();
		  os << " SegNum=" << uremb.GetSegmentsNum();
		  os << " BBox=";
		  uremb.BoundingBox().Print(os);
		  os << " )" << endl;
		  return os;
		}
	  else
		return os << "URegion2: (undef)" << endl;
	}

/*
The assignment operator

*/

  URegion2& operator= ( const URegion2& U);


/*
Distance function

*/
     double Distance(const Rectangle<3>& rect, const Geoid* geoid = 0) 
		const{
       cerr << "Warning URegion2::Distance(rect) not implemented. "
            << "Using Rectangle<3>::Distance(Rectangle<3>) instead!" 
	    << endl;
       if(!IsDefined()){
          return -1;
       } else {
         return BoundingBox(geoid).Distance(rect,geoid);
       }

     }

     virtual bool IsEmpty() const{
       return !IsDefined();
     }

     static string BasicType() { return "uregion2"; }
     static const bool checkType(const ListExpr type){
        return listutils::isSymbol(type, BasicType());
     }

}; //End of class URegion2

/*
1 Class ~MRegion2~

Represents a moving region. It contains an array of ~MSegmentData2~ for the
moving segments, which is referenced by its ~URegionEmb2~ units, which in turn
do not have their own storage for moving segments.
The same is true for the precise segments of type ~PreciseMSegmentData~ and for
the precise interval information of type ~PreciseInterval~.

*/

class MRegion2 : public temporalalgebra::Mapping<URegionEmb2, Region> {
private:
/*
1.1 Private attributes

  * The arrays with the segments and the precise intervals.

*/
    DbArray<MSegmentData2> msegmentdata;
    DbArray<PreciseMSegmentData> preciseSegmentData;
    DbArray<int> preciseCoordinates;
    DbArray<int> preciseInstants;

    int scaleFactor;

public:
/*
1.1 Constructors

The default constructor does nothing.

*/
    MRegion2() { }

/*
Create ~MRegion2()~ instance, which is prepared for ~n~ units.

*/
    MRegion2(const int n);

/*
Create ~MRegion2()~ instance, determine its units by the units in ~mp~ and
set each unit to the constant value of ~r~.

*/
    MRegion2(const temporalalgebra::MPoint& mp, const Region& r);

/*
Constructs a continuously moving region from the parameters. ~dummy~ is not
used.

*/
    MRegion2(const temporalalgebra::MPoint& mp, const Region& r,
             const int dummy);

    MRegion2(const temporalalgebra::MRegion& origRegion);

/*
Constructs a MRegion2 from an existing temporalalgebra::MRegion, using 
scaleFactor to transform the double coordinates into the integer grid 
coordinates.

*/
    MRegion2(temporalalgebra::MRegion& coarseRegion, const int scaleFactor);


    inline void Clear() {
      msegmentdata.clean();
      units.clean();
      SetDefined( true );
    };


/*
1.1 Attribute access methods

Get ~URegionEmb2~ unit ~i~ from this ~MRegion2~ instance and return it in ~ur~.

*/
    void Get(const int i, URegionEmb2& ur) const;

/*
Add a idependent ~URegion2~ object to the moving region. The URegions2 moving 
segment is copied to
the DBArrays for ~msegmentdata~, ~preciseSegmentData~, ~preciseCoordinates~, 
~preciseInstants~ and ~units~.

*/

    void AddURegion2(URegion2& Ureg);

/*
Allow read-only access to DbArrays.

*/
    const DbArray<MSegmentData2>* GetMSegmentData2(void);
    const DbArray<PreciseMSegmentData>* GetPreciseMSegmentData(void);
    const DbArray<int>* GetPreciseCoordinates(void);
    const DbArray<int>* GetPreciseInstants(void);

    const int GetScaleFactor(void);
	void SetScaleFactor(int factor);

/*
1.1 Methods for database operators

*/

    void Intersection(temporalalgebra::MPoint& mp, 
                      temporalalgebra::MPoint& res);
    void Inside(const temporalalgebra::MPoint& mp, temporalalgebra::MBool& res);

    void Transform(double deltaX, double deltaY);
    void Scale(double deltaX, double deltaY);

    void Final(temporalalgebra::Intime<Region>& result);
    void Initial(temporalalgebra::Intime<Region>& result);
    void AtInstant(const Instant& t, temporalalgebra::Intime<Region>& result);

/*
1.1 Methods for algebra integration

~DBArray~ access.

*/
	int NumOfFLOBs(void) const;
	Flob *GetFLOB(const int i);

/*
Clone ~MRegion2~ instance.

*/
    MRegion2* Clone(void) const;

/*
Copy ~MRegion2~ instance.

*/
	void CopyFrom(const Attribute* right);

/*
Return the name of the Secondo type.

*/
  static string BasicType(){ return "mregion2"; }
   static const bool checkType(const ListExpr type){
	  return listutils::isSymbol(type, BasicType());
   }

   
/*
1.1 Other declarations

Friend access for ~InMRegion2()~ makes live easier.

*/
   friend Word InMRegion2(
	   const ListExpr typeInfo,
	   const ListExpr instance,
	   const int errorPos,
	   ListExpr& errorInfo,
	   bool& correct);

}; //end of class MRegion2


#endif /* MOVINGREGION2ALGBRA_H_ */
