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

[1] Implementation of the MRegionOpsAlgebra

April - November 2008, M. H[oe]ger for bachelor thesis.

[TOC]

1 Introduction

2 Defines and Includes

*/

#include "SetOps.h"

using namespace std;

namespace temporalalgebra {
  namespace mregionops3 {
    
/*
3 Enumeration 

4 Class RationalPoint3DExt

*/      
    RationalPoint3DExt::RationalPoint3DExt():RationalPoint3D(){
    }// Konstruktor
    
    RationalPoint3DExt::RationalPoint3DExt(const mpq_class& a, 
                                           const mpq_class& b, 
                                           const mpq_class& c,
                                           SourceFlag _sourceFlag):
        RationalPoint3D(a,b,c),sourceFlag(_sourceFlag){
    }// Konstruktor
    
    void RationalPoint3DExt::setSourceFlag(SourceFlag flag){
      this->sourceFlag = flag;
    }// setSourceFlag

    SourceFlag RationalPoint3DExt::getSourceFlag()const{
      return this->sourceFlag;
    }// getSourceFlag
    
    bool RationalPoint3DExt::operator < (const RationalPoint3DExt& point)const{
      if (NumericUtil::lower(this->x,  point.x))return true;
      if (NumericUtil::greater(this->x,point.x))return false;
      if (NumericUtil::lower(this->y,  point.y))return true;
      if (NumericUtil::greater(this->y,point.y))return false;
      if (NumericUtil::lower(this->z,  point.z))return true;
      if (NumericUtil::greater(this->z,point.z))return false;
      return this->sourceFlag < point.sourceFlag;
    }// Operator <
    
    std::ostream& operator <<(std::ostream& os, 
                              const RationalPoint3DExt& point){
      os << "RationalPoint3DExt (" << point.x.get_d();
      os << ", " << point.y.get_d();
      os << ", " << point.z.get_d() <<", ";
      if(point.sourceFlag == UNIT_A) os << "UNIT_A)";
      else os << "UNIT_B)";
      return os; 
    }// operator <<
/*
5 Class RationalPoint3DExtSet

*/     
    RationalPoint3DExtSet::RationalPoint3DExtSet(){
    }// Konstruktor

    void RationalPoint3DExtSet::insert(const RationalPoint3DExt& point){
      this->points.insert(point);
    }// insert

    size_t RationalPoint3DExtSet::size() const{
      return this->points.size();
    }// size

    bool RationalPoint3DExtSet::getIntersectionSegment(
        RationalSegment3D& result)const{
      if (this->points.size() != 4) return false;
      set<RationalPoint3DExt>::iterator it = this->points.begin();
      RationalPoint3DExt point1 = *it;
      it++;
      RationalPoint3DExt point2 = *it;
      if (point1.getSourceFlag() == point2.getSourceFlag()) return false;
      it++;
      RationalPoint3DExt point3 = *it;
      if (point2 == point3) {
        // The length of the intersection segment is zero.
        return false;
      }// if
      result = RationalSegment3D(point2, point3);
      return true;
    }// getIntersectionSegment
    
    std::ostream& operator <<(std::ostream& os, 
                              const RationalPoint3DExtSet& points){
      return points.print(os,"");
    }// operator <<

    std::ostream& RationalPoint3DExtSet::print(std::ostream& os, 
                                               std::string prefix)const{
      set<RationalPoint3DExt>::const_iterator iter;
      os << prefix << "RationalPoint3DExtSet(" << endl;
      for(iter = points.begin(); iter != points.end(); ++iter){
        os << prefix << "  " << *iter << endl;
      }// for
      os << prefix << ")" << endl;
      return os;
    }// print
/*
6 Class RationalPlane3D

*/      
    RationalPlane3D::RationalPlane3D():normalVector(),pointOnPlane(){  
    }// Konstruktor
   
    RationalPlane3D::RationalPlane3D(const RationalPlane3D& plane){
      set(plane);
    }// konstruktor
   
    RationalPlane3D::RationalPlane3D(const PFace& pf){
      RationalPoint3D leftStart = pf.getLeftStart().getR();
      RationalPoint3D leftEnd   = pf.getLeftEnd().getR();
      RationalPoint3D rightStart= pf.getRightStart().getR();
      RationalPoint3D rightEnd  = pf.getRightEnd().getR();
      this->pointOnPlane = leftStart;
      // We compute the normalvector
      if (leftStart != rightStart) {
        // Cross product of vector ab and ac
        this->normalVector = (rightStart - leftStart) ^ (leftEnd - leftStart);
        // check point d on plane
        if(!NumericUtil::nearlyEqual(distance2ToPlane(rightEnd),0.0)){
           NUM_FAIL("Not all points from the pface are located on plane.");
        }// if
      }// if
      else { // A == B
        // Cross product of vector dc and db:
        this->normalVector = (leftEnd - rightEnd) ^ (rightStart - rightEnd);
      }// else
      this->normalVector.normalize();
      // The vector w is either the normalized cross product 
      // of the normal vector and the t-unit-vector, or it's opposite.
      // This depends on the kind of set-operation, we want to perform.
      //
      // wVector = Vector3D(GetNormalVector() ^ Vector3D(0.0, 0.0, -1.0);
      // wVector.Normalize();
      // This can be simplified to:
      wVector = RationalVector3D( - normalVector.getY(), 
                                    normalVector.getX(),
                                    0.0);      
      wVector.normalize();      
    }// Konstruktor
      
    void RationalPlane3D::set(const RationalPlane3D& plane){
      this->normalVector = plane.normalVector;
      this->pointOnPlane = plane.pointOnPlane; 
    }// set
      
    void RationalPlane3D::set(const RationalVector3D& normalVector,
                              const RationalPoint3D& pointOnPlane){
      this->normalVector = normalVector;
      this->pointOnPlane = pointOnPlane; 
    }// set
   
    RationalPoint3D RationalPlane3D::getPointOnPlane() const{
      return this->pointOnPlane;  
    }// getPointOnPlane
        
    RationalVector3D RationalPlane3D::getNormalVector() const{
      return this->normalVector;
    }// getNormalVector
        
    mpq_class RationalPlane3D::distance2ToPlane(const RationalPoint3D& point)
        const{
      mpq_class n =  - (this->normalVector * (point - this->pointOnPlane));
      mpq_class d = this->normalVector * this->normalVector;
      if(d == 0) NUM_FAIL("Normalvector is zerro.");
      mpq_class b = n / d;
      RationalPoint3D result = point + b * this->normalVector;
      return result.distance2(point);
    }// distance2ToPlane
      
    bool RationalPlane3D::isParallelTo(const RationalPlane3D& plane) const{
      RationalVector3D cross = this->normalVector ^ plane.normalVector;
      return NumericUtil::nearlyEqual(cross.length2(),0.0);
    }// isParallelTo
    
    bool RationalPlane3D::isCoplanarTo(const RationalPlane3D& plane) const{
      return NumericUtil::nearlyEqual(distance2ToPlane(plane.pointOnPlane),
                                      0.0);
    }// isCoplanarTo
          
    bool RationalPlane3D::intersection(const Segment3D segment, 
                                       RationalPoint3D& result)const{
      RationalPoint3D head = segment.getHead();
      RationalPoint3D tail = segment.getTail();
      // We compute the intersection point of the plane 
      // - defined by the PFace - and the segment.
      RationalVector3D u = head - tail;
      RationalVector3D w = tail - this->pointOnPlane;
      mpq_class d = this->normalVector * u;
      mpq_class n = - this->normalVector * w;
      // Segment is parallel to plane ?
      if (NumericUtil::nearlyEqual(d, 0.0))  return false;
      mpq_class s = n / d;
      // No intersection point, if s < -eps or s > 1 + eps.
      if (NumericUtil::lower(s, 0.0) || NumericUtil::greater(s, 1.0)) 
        return false;
      // Compute segment intersection point
      result = tail + s * u;
      return true;                                
    }// intersection
                                    
    RationalPlane3D& RationalPlane3D::operator =(const RationalPlane3D& plane){
      set(plane);
      return *this;
    }// Operator =      
      
    std::ostream& operator <<(std::ostream& os, 
                              const RationalPlane3D& plane){
      os << "RationalPlane3D ( "<< plane.normalVector << ", ";
      os << plane.pointOnPlane <<")";
      return os;   
    }// operator << 
    
    void RationalPlane3D::intersection(const PFace& other,
         SourceFlag sourceFlag, RationalPoint3DExtSet& intPointSet)const{
      Point3D leftStart  = other.getLeftStart();
      Point3D leftEnd    = other.getLeftEnd();
      Point3D rightStart = other.getRightStart();
      Point3D rightEnd   = other.getRightEnd();
      RationalPoint3DExt intPoint;
      vector<Segment3D> edgesPFace;
      // We store all edges of this PFace as 3DSegments in the vector 
      // edgesPFace.
      edgesPFace.push_back(Segment3D(leftStart, leftEnd));
      edgesPFace.push_back(Segment3D(rightStart, rightEnd));
      if (leftStart != rightStart){
        edgesPFace.push_back(Segment3D(leftStart,rightStart));
      }// if
      if (leftEnd != rightEnd){
        edgesPFace.push_back(Segment3D(leftEnd,rightEnd));
      }// if
      // Intersect the plane - defined by the other PFace - 
      // with all edges of this PFace:
      RationalPoint3DExt temp;
      for(size_t i = 0, j = 0 ; j < 2 && i < edgesPFace.size();i++){        
        if(intersection(edgesPFace[i], intPoint)){          
          intPoint.setSourceFlag(sourceFlag);
          intPointSet.insert(intPoint); 
          if(j == 0){ 
            temp = intPoint;
            j++;
          }// if
          else if (!(temp == intPoint)){
            j++;           
          }// else if                    
        }// if
      }// for      
    }// intersection
    
    bool RationalPlane3D::isLeftAreaInner(const RationalSegment3D segment,
                                          const RationalPlane3D other)const{
      RationalVector3D segmentVector(segment.getHead() - segment.getTail());
      segmentVector.normalize();
      RationalVector3D vector = this->normalVector ^ segmentVector;
      if((vector * other.normalVector) > 0) return false;
      return true;
    }// isLeftAreaInner
                       
    Point2D RationalPlane3D::transform(const RationalPoint3D& point) const{
      // check point d on plane
      if(!NumericUtil::nearlyEqual(distance2ToPlane(point),0.0)){
           NUM_FAIL("Point isn,t located on plane.");
      }// if
      mpq_class w = point.getX() * wVector.getX() +
                    point.getY() * wVector.getY();
      return Point2D(w.get_d(),point.getZ().get_d());  
    }// transform
      
    Segment2D RationalPlane3D::transform(
        const RationalSegment3D& segment) const{  
      return Segment2D(transform(segment.getTail()),
                       transform(segment.getHead())); 
    }// transform
/*
7 Class IntersectionPoint

*/   
    IntersectionPoint::IntersectionPoint():x(0),y(0),z(0),w(0){
    }// Konstruktor
   
    IntersectionPoint::IntersectionPoint(const IntersectionPoint& point){
      set(point);
    }// Konstruktor
    
    IntersectionPoint::IntersectionPoint(const Point3D& point3D, 
                                         const Point2D& point2D){
      if(!(point3D.getZ() == point2D.getY())){
        NUM_FAIL("Point3D and Point2D don't discribe the same.");
      }// if
      this->x = point3D.getX();
      this->y = point3D.getY();
      this->z = point3D.getZ();
      this->w = point2D.getX();
    }// Konstruktor
    
    IntersectionPoint::IntersectionPoint(double x, double y, double z, 
                                         double w){
      this->x = x;
      this->y = y;
      this->z = z;
      this->w = w; 
    }// Konstruktor
    
    void IntersectionPoint::set(const IntersectionPoint& point){
      this->x = point.x;
      this->y = point.y;
      this->z = point.z;
      this->w = point.w;      
    }// set
    Point3D IntersectionPoint::getPoint3D() const{
      return Point3D(x,y,z);
    }// getPoint3D
      
    Point2D IntersectionPoint::getPoint2D() const{
      return Point2D(w,z);
    }// getPoint2D
      
    double IntersectionPoint::getX()const{
      return x;
    }// getX
      
    double IntersectionPoint::getY()const{
      return y;
    }// getY
      
    double IntersectionPoint::getZ()const{
      return z;
    }// getZ
      
    double IntersectionPoint::getW()const{
      return w;        
    }// getW
      
    double IntersectionPoint::getT()const{
      return z;    
    }// getT
    
    Rectangle<3> IntersectionPoint::getBoundingBox()const{
      double array[3] = {x,y,z};
      return Rectangle<3>(true,array,array);
    }// getBoundingBox 
          
    std::ostream& operator <<(std::ostream& os, 
                              const IntersectionPoint& point){
      os << "IntersectionPoint (" << point.x << ", " << point.y << ", ";
      os << point.z <<", " <<point.w <<")";
      return os; 
    }// OPerator <<
    
    IntersectionPoint& IntersectionPoint::operator =(
        const IntersectionPoint& point){
      set(point);
      return *this;
    }// Operator =
    
    bool IntersectionPoint::operator ==(const IntersectionPoint& point) const{
      return NumericUtil::nearlyEqual(this->x, point.x) && 
             NumericUtil::nearlyEqual(this->y, point.y) && 
             NumericUtil::nearlyEqual(this->z, point.z) &&
             NumericUtil::nearlyEqual(this->w, point.w);
    }// Operator ==
/*
8 Class IntersectionSegment

*/  
    IntersectionSegment::IntersectionSegment(){
      this->predicate = UNDEFINED;   
    }// Konstruktor
    
    IntersectionSegment::IntersectionSegment(
        const IntersectionSegment& segment){
      set(segment);    
    }// konstruktor    
    
    IntersectionSegment::IntersectionSegment(const IntersectionPoint& tail,
                                             const IntersectionPoint& head,
                                             const Predicate predicate){
      set(tail,head,predicate);
    }// konstruktor
    
    IntersectionSegment::IntersectionSegment(const Segment3D& segment3D, 
                                             const Segment2D& segment2D,
                                             const Predicate predicate){
      IntersectionPoint tail(segment3D.getTail(),segment2D.getTail());
      IntersectionPoint head(segment3D.getHead(),segment2D.getHead());
      set(tail,head,predicate);  
    }// Konstruktor
      
    void IntersectionSegment::set(const IntersectionPoint& tail,
                                  const IntersectionPoint& head,
                                  Predicate predicate){
      if(tail.getT()<= head.getT()){
        this->tail = tail;
        this->head = head;
        this->predicate = predicate; 
      }// if
      else {
        this->tail = head;
        this->head = tail;
        if(predicate == LEFT_IS_INNER) predicate = RIGHT_IS_INNER;
        else if (predicate == RIGHT_IS_INNER) predicate = LEFT_IS_INNER;
        this->predicate = predicate;
      }// else
    }// set
    
    void IntersectionSegment::set(const IntersectionSegment& segment){
      this->tail = segment.tail;
      this->head = segment.head;
      this->predicate = segment.predicate; 
    }// set
      
    Segment3D IntersectionSegment::getSegment3D()const{
      return Segment3D(tail.getPoint3D(),head.getPoint3D());
    }// getSegment3D
    
    Segment2D IntersectionSegment::getSegment2D()const{
      return Segment2D(tail.getPoint2D(),head.getPoint2D());
    }// getSegment2D
    
    IntersectionPoint IntersectionSegment::getTail() const{
      return tail;
    }// getTail
    
    IntersectionPoint IntersectionSegment::getHead() const{
      return head;   
    }// getHead
    
    Predicate IntersectionSegment::getPredicate()const{
      return predicate;
    }// getSegment3D
    
    
    bool IntersectionSegment::isOrthogonalToTAxis()const{
      return NumericUtil::nearlyEqual(tail.getT(),head.getT());
    }// isOrthogonalToTAxis
    
    bool IntersectionSegment::isOutOfRange(double t)const{
      if(NumericUtil::lower(this->head.getT(),t)) return true;
      return NumericUtil::nearlyEqual(this->head.getT(),t);
    }// isOutOfRange
    
    bool IntersectionSegment::isLeftOf(const IntersectionSegment& intSeg)const{
      double tail1T = this->getTail().getT();
      double head2T = intSeg.getHead().getT();
      double tail2T = intSeg.getTail().getT();
      // Precondition: 
      // this->getTail().getT() is inside the interval 
      // [intSeg.getTail().getT(), intSeg.getHead().getT()]
      // and this and intSeg don't intersect in their interior.
      if(NumericUtil::lower(tail1T,tail2T) ||
         NumericUtil::greater(tail1T,head2T)){
        NUM_FAIL ("t must between the t value form tail und haed.");
      }// if
      Segment2D segment2D1 =  this->getSegment2D();
      Segment2D segment2D2 = intSeg.getSegment2D();
      double sideOfStart = segment2D2.whichSide(segment2D1.getTail());   
      if(sideOfStart >   NumericUtil::eps) return true;
      if(sideOfStart < - NumericUtil::eps) return false;
      double sideOfEnd = segment2D2.whichSide(segment2D1.getHead());
      return sideOfEnd > NumericUtil::eps;
    }// bool
    
    Point3D IntersectionSegment::evaluate(double t) const {
      double headT = this->getHead().getT();
      double tailT = this->getTail().getT();
      Point3D head3D = head.getPoint3D();
      Point3D tail3D = tail.getPoint3D();
      // Precondition:
      // t is between t on tail and haed
      if(!(NumericUtil::between(tailT, t, headT))){
        NUM_FAIL ("t must between the t value form tail und haed.");
      }// if
      if(t == headT) return head3D;
      if(t == tailT) return tail3D;
      // Point3D pointInPlane(0.0, 0.0, t);
      // Vector3D normalVectorOfPlane(0.0, 0.0, 1.0);
      // Vector3D u = this->getHead().getPoint3D() - 
      //              this->getTail().getPoint3D();
      // Vector3D w = this->getTail().getPoint3D() - pointInPlane;
      // double d = normalVectorOfPlane * u;
      // double n = -normalVectorOfPlane * w;
      //
      // This can be simplified to:
      RationalVector3D u = head3D.getR() - tail3D.getR();
      mpq_class d =  mpq_class(headT) - tailT;
      mpq_class n =  mpq_class(t) - tailT;
      // this segment must not be parallel to plane
      if(NumericUtil::nearlyEqual(d, 0.0)){
        NUM_FAIL ("Intersection segment must not be parallel to plane."); 
      }// if  
      mpq_class s = n/d;
      if(!(NumericUtil::between(0.0, s, 1.0))){
        NUM_FAIL ("No point on segment found.");
      }// if
      // compute segment intersection point
      return (tail3D.getR() + s * u).getD();  
    }// evaluate    
    
    std::ostream& operator <<(
        std::ostream& os, const IntersectionSegment& segment){
      os << "IntersectionSegment (" << segment.tail << ", " << segment.head;
      os << ", " << toString(segment.getPredicate());
      os << ")";
      return os;  
    }// Operator <<
    
    IntersectionSegment& IntersectionSegment::operator =(
        const IntersectionSegment& segment){
       set(segment); 
       return *this;
    }// OPerator =
    
    bool IntersectionSegment::operator ==(
        const IntersectionSegment& segment) const{
      return this->head == segment.head && 
             this->tail == segment.tail &&
             this->predicate == segment.predicate;
    }// Operator ==       
/*
9 Struct IntSegCompare

*/
    bool IntSegCompare::operator()(const IntersectionSegment* const& segment1,
                                   const IntersectionSegment* const& segment2)
        const{
      IntersectionPoint tail1 = segment1->getTail();
      IntersectionPoint tail2 = segment2->getTail();
      IntersectionPoint head1 = segment1->getHead();
      IntersectionPoint head2 = segment2->getHead();
      // We sort by (t_start, w_start, IsLeft())
      // Precondition:  tail1.getT() < haed1.getT() &&
      //                tail2.getT() < head2.getT() 
      if (NumericUtil::lower(  tail1.getT(), tail2.getT())) return true;
      if (NumericUtil::greater(tail1.getT(), tail2.getT())) return false;
      // tail1.getT() == tail2.getT()
      if (NumericUtil::lower(  tail1.getW(), tail2.getW())) return true;
      if (NumericUtil::greater(tail1.getW(), tail2.getW())) return false;
      // tail1.getW() == tail2.GetW()  
      if (head1.getPoint2D() == head2.getPoint2D()) return true;    
      return segment2->getSegment2D().isLeft(head1.getPoint2D());
    }// IntSegCompare   
/*
10 Class PlaneSweepAccess

*/       
    void PlaneSweepAccess::first(double t1, double t2, 
                                 ContainerPoint3D& points,
                                 ContainerSegment& segments){
      NUM_FAIL ("method must override.");
    }// first

    void PlaneSweepAccess::next(double t1, double t2, 
                                ContainerPoint3D& points,
                                ContainerSegment& segments){
      NUM_FAIL ("method must override.");
    }// next
    
    PlaneSweepAccess::~PlaneSweepAccess(){
    }// Destruktor
/*
11 class IntSegContainer

*/   
    IntSegContainer::IntSegContainer(){
    }// Konstruktor
    
    IntSegContainer::IntSegContainer(const IntSegContainer& container){
      set(container);
    }// Konstruktor
    
    IntSegContainer::~IntSegContainer(){  
      std::set<IntersectionSegment*>::iterator iter;
      for (iter = intSegs.begin(); iter != intSegs.end(); ++iter){
        delete *iter;
      }// for  
    }// Destruktor
    
    void IntSegContainer::set(const IntSegContainer& container){
      std::set<IntersectionSegment*>::iterator iter;
      for (iter = container.intSegs.begin(); 
           iter != container.intSegs.end(); ++iter){
        this->intSegs.insert(new IntersectionSegment(**iter));
      }// for
    }// set
 
    void IntSegContainer::addIntSeg(const IntersectionSegment& seg){
      intSegs.insert(new IntersectionSegment(seg));
    }// addIntSeg
    
    size_t IntSegContainer::size()const{
      return intSegs.size();
    }// size
  
    std::ostream& operator <<(std::ostream& os, 
                              const IntSegContainer& container){
      return container.print(os,"");
    }// operator 
    
    std::ostream& IntSegContainer::print(std::ostream& os, 
                                         std::string prefix)const{
      os << prefix << "IntSegContainer (";
      if (intSegs.empty()) os << "is empty)" << endl;
      else {        
        std::set<IntersectionSegment*>::iterator iter;
        for (iter  = intSegs.begin(); 
             iter != intSegs.end(); ++iter) {
          os << endl << prefix + "  " << *(*iter);
        }// for
        os << endl << prefix << ")" <<endl;
      }// else
      return os;
    }// print
    
    bool IntSegContainer::operator ==(const IntSegContainer& container)const{
      if(this->intSegs.size() != container.intSegs.size()) return false;
      std::set<IntersectionSegment*>::iterator iter1, iter2;
      for(iter1  = this->intSegs.begin(), iter2 =  container.intSegs.begin();
          iter1 != this->intSegs.end() && iter2 != container.intSegs.end();
          ++iter1, ++iter2){
        if(!(*(*iter1) == *(*iter2))) return false;
      }// for
      return true;
    }// Operator ==  
    
    IntSegContainer& IntSegContainer::operator =(
        const IntSegContainer& container){
      set(container);
      return *this;
    }// Operator = 
    
    bool IntSegContainer::hasMoreSegsToInsert(double t)const{
      if(intSegIter == intSegs.end()) return false;
      IntersectionPoint tail((*intSegIter)->getTail());
      return NumericUtil::nearlyEqual(tail.getT(),t);
    }// hasMoreSegsToInsert
    
    void IntSegContainer::first(double t1, double t2, ContainerPoint3D& points,
                                ContainerSegment&  segments){ 
      intSegIter = intSegs.begin();
      next(t1,t2,points,segments);      
    }// first
    
    void IntSegContainer::next(double t1, double t2, ContainerPoint3D& points, 
                               ContainerSegment&  segments){
      list<IntersectionSegment*>::iterator activeIter;
      activeIter = active.begin();
      while (activeIter != active.end()){
        while (activeIter != active.end() && 
              (*activeIter)->isOutOfRange(t1)){
          activeIter = active.erase(activeIter);
        }// while
        if (activeIter == active.end()) break; 
        if (hasMoreSegsToInsert(t1)) {
          IntersectionSegment* newSeg = *intSegIter;
          if (newSeg->isLeftOf(**activeIter)) {
            activeIter = active.insert(activeIter, newSeg);
            intSegIter++;
          }// if 
        }// if 
        activeIter++;
      }// while 
      // Add the tail, if there is one:
      while (hasMoreSegsToInsert(t1)) {
        IntersectionSegment* newSeg = *intSegIter;
        activeIter = active.insert(activeIter, newSeg);
        intSegIter++;
        activeIter = active.end();
      }// while
      // create result segments   
      for(activeIter = active.begin();
          activeIter != active.end();
          activeIter++){       
        Point3D tail, head;
        IntersectionSegment* segment = *activeIter;
        if(segment->isOrthogonalToTAxis()){
          tail = segment->getTail().getPoint3D();
          head = segment->getHead().getPoint3D(); 
        }// if
        else {
          tail = segment->evaluate(t1);
          head = segment->evaluate(t2);
        }// else
        size_t p1     = points.add(tail);
        size_t p2     = points.add(head);
        Predicate predicate = segment->getPredicate();
        segments.add(Segment(p1,p2,predicate));   
      }// for   
    }// next     
/*
12 struct DoubleCompare

*/      
    bool DoubleCompare::operator()(const double& d1, const double& d2) const{ 
        return NumericUtil::lower(d1, d2);
    }// Operator
/*
13 class GlobalTimeValues

*/      
    GlobalTimeValues::GlobalTimeValues(){
    }

    void GlobalTimeValues::addTimeValue(double t){
        time.insert(t);
    }// addTimeValue
    
    size_t GlobalTimeValues::size()const{
      return time.size();
    }// size
         
    std::ostream& operator <<(std::ostream& os, 
                              const GlobalTimeValues& timeValues){
      std::set<double, DoubleCompare>::const_iterator iter;
      os << "GlobalTimeValues (";
      for (iter = timeValues.time.begin(); 
           iter != timeValues.time.end(); iter++){
        if (iter != timeValues.time.begin()) os << ", " ;      
        os << *iter;
      }// for
      os <<")" << endl;
      return os;
    }// Operator <<
    
    bool GlobalTimeValues::operator ==(const GlobalTimeValues& other)const{
      if(this->time.size() != other.time.size()) return false;
      std::set<double, DoubleCompare>::iterator iter1,iter2;
      for (iter1  = this->time.begin(),iter2  = other.time.begin(); 
           iter1 != this->time.end(); iter1++,iter2++){
        if(!(NumericUtil::nearlyEqual(*iter1,*iter2))) return false;
      }// for
      return true;
    }// Operator ==
    
     bool GlobalTimeValues::first(double& t){
       // es gibt keine zwei Zeitwerte
       if(time.size() < 2) return false;
       timeIter = time.begin();
       t = t2 = t1 = *timeIter;
       return true;
     }// first
      
     bool GlobalTimeValues::next(double& t){
       if((++timeIter) != time.end()){
         // Set the new initial timelevel:
         t1 = t2;
         // Set the final timelevel:
         t = t2 = *timeIter;
         return true;
       }// if
       return false;
     }// next  
/*
14 class MSegment 
 
*/
    MSegment::MSegment(){  
      set(Segment2D(Point2D(0,0),
                    Point2D(1,0)),
          Segment2D(Point2D(0,0),
                    Point2D(1,0)));
    }// Konstruktor
    
    MSegment::MSegment(const MSegment& msegment){
      set(msegment);      
    }// Konstruktor

    MSegment::MSegment(const Segment3D& left, const Segment3D& right){
      Point3D initialStart = left.getTail();
      Point3D initialEnd   = right.getTail();
      Point3D finalStart   = left.getHead();
      Point3D finalEnd     = right.getHead();
      set(Segment2D(Point2D(initialStart.getX(),initialStart.getY()),
                    Point2D(initialEnd.getX(),initialEnd.getY())),
          Segment2D(Point2D(finalStart.getX(),finalStart.getY()),
                    Point2D(finalEnd.getX(),finalEnd.getY())));
    }// Konstruktor
    
    MSegment::MSegment(const Segment2D& initialSegment, 
                       const Segment2D& finalSegment){
      set(initialSegment, finalSegment);
    }// Konstruktor
    
    MSegment::MSegment(const Segment2D& initialSegment, 
                       const Segment2D& finalSegment,
                       int faceno,
                       int cycleno,
                       int edgeno,
                       bool leftDomPoint,
                       bool insideAbove){
      set(initialSegment,finalSegment);
      medianHS.attr.faceno  = faceno;
      medianHS.attr.cycleno = cycleno;
      medianHS.attr.edgeno  = edgeno;
      medianHS.SetLeftDomPoint(leftDomPoint);
      insideAbove = medianHS.attr.insideAbove = insideAbove;
    }// Konstruktor
                       

    void MSegment::set(const Segment2D& initialSegment, 
                       const Segment2D& finalSegment){
      this->initialSegment= initialSegment;
      this->finalSegment  = finalSegment;
      double medianStartX = (initialSegment.getTail().getX() + 
                             finalSegment.getTail().getX())/2;
      double medianStartY = (initialSegment.getTail().getY() + 
                             finalSegment.getTail().getY())/2;
      double medianEndX   = (initialSegment.getHead().getX() + 
                             finalSegment.getHead().getX())/2;
      double medianEndY   = (initialSegment.getHead().getY() + 
                             finalSegment.getHead().getY())/2;
      Point medianStart(true,medianStartX, medianStartY);
      Point medianEnd  (true,medianEndX,medianEndY);
      medianHS = HalfSegment(true, medianStart, medianEnd);
      insideAbove = medianHS.attr.insideAbove = medianStart > medianEnd;
      medianHS.attr.faceno     = -1;
      medianHS.attr.cycleno    = -1;
      medianHS.attr.edgeno     = -1;
      medianHS.attr.coverageno = -1;
      medianHS.attr.partnerno  = -1;  
    }// set
        
    void MSegment::set(const MSegment& mSegment){
      this->initialSegment = mSegment.initialSegment;
      this->finalSegment = mSegment.finalSegment;
      this->medianHS = mSegment.medianHS;
    }// set
    
//     void MSegment::set(const Segment2D& initialSegment, 
//                        const Segment2D& finalSegment,
//                        int faceno,
//                        int cycleno,
//                        int edgeno,
//                        bool leftDomPoint,
//                        bool insideAbove){
//       set(initialSegment,finalSegment);
//       medianHS.attr.faceno  = faceno;
//       medianHS.attr.cycleno = cycleno;
//       medianHS.attr.edgeno  = edgeno;
//       medianHS.SetLeftDomPoint(leftDomPoint);
//       insideAbove = medianHS.attr.insideAbove = insideAbove;  
//     }// set
    
    int MSegment::getFaceNo() const{
      return medianHS.attr.faceno;
    }// getFaceNo
    
    int MSegment::getCycleNo() const{
      return medianHS.attr.cycleno;
    }// getCycleNo
    
    int MSegment::getSegmentNo() const{
      return medianHS.attr.edgeno;
    }// getSegmentNo
    
    int MSegment::getInsideAbove() const{
      return insideAbove;
    }// getInsideAbove
            
    HalfSegment MSegment::getMedianHS() const{
      return medianHS;
    }// getMedianHS
        
    Segment2D MSegment::getInitial() const{
      return initialSegment; 
    }// getInitial
    
    Segment2D MSegment::getFinal() const{
      return finalSegment; 
    }// getInitial
      
    bool MSegment::isLeftDomPoint() const{
      return medianHS.IsLeftDomPoint();
    }// isLeftDomPoint
      
    void MSegment::setSegmentNo(int sn){
      medianHS.attr.edgeno = sn;
    }// setSegmentNo
    
    void MSegment::setLeftDomPoint(bool ldp){
       medianHS.SetLeftDomPoint(ldp);
    }// setLeftDomPoint
    
    bool MSegment::lessByMedianHS(const MSegment& other) const {
      return this->medianHS < other.medianHS;
    }// lessByMedianHS
        
    bool MSegment::logicLess(const MSegment& other) const {
      if (isLeftDomPoint() != other.isLeftDomPoint())
        return isLeftDomPoint() > other.isLeftDomPoint();
      return this->medianHS.LogicCompare(other.medianHS) == -1;
    }// logicLess       
    
    void MSegment::copyIndicesFrom(const HalfSegment* hs) {
      medianHS.attr.faceno  = hs->attr.faceno;
      medianHS.attr.cycleno = hs->attr.cycleno;
      medianHS.attr.edgeno  = hs->attr.edgeno;
    }// copyIndicesFrom
    
    std::ostream& MSegment::print(std::ostream& os, std::string prefix)const{
      os << prefix << "MSegment (" << endl;
      os << prefix << "  Initial:=" << this->initialSegment << endl;
      os << prefix << "  Final:=" <<  this->finalSegment << endl;
      os << prefix << "  MedianHS:=" << this-> medianHS << endl;
      os << prefix <<")" << endl;
      return os;
    }// print

    std::ostream& operator <<(std::ostream& os, const MSegment& mSegment){
      mSegment.print(os,"");
      return os;
    }// Operator <<
   
    bool MSegment::operator ==(const MSegment& mSegment)const{
      if((this->initialSegment == mSegment.initialSegment) &&
         (this->finalSegment == mSegment.finalSegment) &&
         (this->medianHS == mSegment.medianHS)) return true;
      return false;
    }// Operator ==
   
    MSegment& MSegment::operator =(const MSegment& mSegment){
      set(mSegment);
      return *this;
    }// Operator =
/*
14 class ResultUnit 
 
*/
    ResultUnit::ResultUnit():index(0),startTime(0),endTime(1){  
    }// Konstruktor
    
    ResultUnit::ResultUnit(double startTime, double endTime):index(0){
      this->startTime = startTime;
      this->endTime   = endTime;
    }// Konstruktor
    
    ResultUnit::ResultUnit(const ResultUnit& other){
      set(other);
    }// Konstruktor
    
    void ResultUnit::set(const ResultUnit& other){
      this->index     = other.index;
      this->startTime = other.startTime;
      this->endTime   = other.endTime;
      for(size_t i = 0; i < other.mSegments.size(); i++){
        this->mSegments.push_back(other.mSegments[i]);
      }// for
    }// set
    
    size_t ResultUnit::size(){
      return mSegments.size();
    }// size(
    
    void ResultUnit::addMSegment(MSegment& mSegment, bool completely ){
      if(completely){
        mSegments.push_back(mSegment);
      }// if
      else {
        mSegment.setSegmentNo(this->index++);
        mSegment.setLeftDomPoint(true);
        mSegments.push_back(mSegment);
        mSegment.setLeftDomPoint(false);
        mSegments.push_back(mSegment);
      }// else
    }// set
    
    Interval<Instant> ResultUnit::getTimeInterval() const {
      Instant start(datetime::instanttype);
      Instant end(datetime::instanttype);
      start.ReadFrom(this->startTime);
      end.ReadFrom(this->endTime);
      return Interval<Instant>(start, end, true, false);
    }// getTimeInterval
 
    std::ostream& ResultUnit::print(std::ostream& os, std::string prefix)const{
      os << prefix << "ResultUnit (";
      if (mSegments.size() == 0) os << "is empty)" << endl;
      else {
        os << endl << prefix << "  Startime:=" << this->startTime;
        os << ", EndTime:=" << this->endTime << endl;
        os << endl << prefix << "  MSegments (" << endl;
        for(size_t i = 0; i < mSegments.size(); i++){
          os << prefix << "    Index:=" << i << endl; 
          this->mSegments[i].print(os,"            ");
        }// for
        os << prefix << "  )"<<endl;
        os << prefix << ")"<<endl;
      }// else
      return os;
    }// print
    
    std::ostream& operator <<(std::ostream& os, const ResultUnit& unit){
      unit.print(os,"");
      return os;
    }// Operator <<
   
    bool ResultUnit::operator ==(const ResultUnit& other)const{
      
      if(this->mSegments.size() != other.mSegments.size()) return false;  
      for(size_t i = 0; i < this->mSegments.size(); i++){
        if(!(this->mSegments[i] == other.mSegments[i])) return false;
      }// for
      return true;
    }// Operator ==
   
    ResultUnit& ResultUnit::operator =(const ResultUnit& unit){
      set(unit);
      return *this;
    }// Operator =
    
    bool ResultUnit::less(const MSegment& ms1, const MSegment& ms2) {
      return ms1.lessByMedianHS(ms2);
    }// less

    bool ResultUnit::logicLess(const MSegment& ms1, const MSegment& ms2) {
      return ms1.logicLess(ms2);
    }// logicLess
    
    void ResultUnit::finalize(){
      if(mSegments.size() == 0) return;
      // First, we sort the mSegments of this unit by their 
      // median-halfsegments. Comparison between halfsegments
      // is done by the < operator, implemented in the SpatialAlgebra.
      sort(mSegments.begin(), mSegments.end(), ResultUnit::less);
      // Second, we construct a region from all median-halfsegments
      // of each msegment of this unit:
      Region region(mSegments.size());
      region.StartBulkLoad();
      for (size_t i = 0; i < mSegments.size(); i++) {
        //cout << mSegments[i].getMedianHS() << endl;
        region.Put(i, mSegments[i].getMedianHS());
      }// for
      // Note: Sorting is already done.
      region.EndBulkLoad(false, true, true, true);
      // Third, we retrive the faceNo, cycleNo and edgeNo of
      // each halfsegment from the region, computed in the 
      // Region::EndBulkLoad procedure:
      for (unsigned int i = 0; i < mSegments.size(); i++) {
        HalfSegment halfSegment;
        region.Get(i, halfSegment);
        mSegments[i].copyIndicesFrom(&halfSegment);
      }// for
      // Sort mSegments by faceno, cycleno and segmentno:
      sort(mSegments.begin(), mSegments.end(), ResultUnit::logicLess);
      //this->Print();
      // Erase the second half of mSegments, 
      // which contains all MSegments with right dominating point:
      mSegments.erase(mSegments.begin() + mSegments.size() / 2, 
                      mSegments.end());
    }// finilize
    
    URegionEmb* ResultUnit::convertToURegionEmb(
        DbArray<MSegmentData>* segments)const {
      size_t segmentsStartPos = segments->Size();
      URegionEmb* uregion     = new URegionEmb(this->getTimeInterval(), 
                                               segmentsStartPos);
      
      double minX = DBL_MAX; // MAX_DOUBLE;
      double maxX = DBL_MIN; // MIN_DOUBLE;
      double minY = DBL_MAX; // MAX_DOUBLE;
      double maxY = DBL_MIN; // MIN_DOUBLE;
      
      for(unsigned int i = 0; i < mSegments.size(); i++) {            
        const MSegment* mSeg = &mSegments[i];
         
        MSegmentData msd(mSeg->getFaceNo(), 
                         mSeg->getCycleNo(), 
                         mSeg->getSegmentNo(), 
                         mSeg->getInsideAbove(), 

                         mSeg->getInitial().getTail().getX(),         
                         mSeg->getInitial().getTail().getY(), 
                         mSeg->getInitial().getHead().getX(), 
                         mSeg->getInitial().getHead().getY(), 
                         
                         mSeg->getFinal().getTail().getX(), 
                         mSeg->getFinal().getTail().getY(), 
                         mSeg->getFinal().getHead().getX(), 
                         mSeg->getFinal().getHead().getY());   
         
         msd.SetDegeneratedInitial(DGM_NONE);
         msd.SetDegeneratedFinal(DGM_NONE);            
        
         uregion->PutSegment(segments, i, msd, true);                     
         // Update the min/max values:
         minX = min(minX, msd.GetInitialStartX());
         minX = min(minX, msd.GetInitialEndX());
         minX = min(minX, msd.GetFinalStartX());
         minX = min(minX, msd.GetFinalEndX());

         minY = min(minY, msd.GetInitialStartY());
         minY = min(minY, msd.GetInitialEndY());
         minY = min(minY, msd.GetFinalStartY());
         minY = min(minY, msd.GetFinalEndY());

         maxX = max(maxX, msd.GetInitialStartX());
         maxX = max(maxX, msd.GetInitialEndX());
         maxX = max(maxX, msd.GetFinalStartX());
         maxX = max(maxX, msd.GetFinalEndX());

         maxY = max(maxY, msd.GetInitialStartY());
         maxY = max(maxY, msd.GetInitialEndY());
         maxY = max(maxY, msd.GetFinalStartY());
         maxY = max(maxY, msd.GetFinalEndY());
      }// for
      // Set the bbox:        
      double min[3] = { minX, minY, this->getTimeInterval().start.ToDouble() };
      double max[3] = { maxX, maxY, this->getTimeInterval().end.ToDouble() };
      uregion->SetBBox(Rectangle<3>(true, min, max));
      return uregion;
    }// convertToURegionEmb
/*
14 class ResultPfaceFactory

*/  
    ResultPfaceFactory::ResultPfaceFactory(){
    }// Konstruktor
     
    ResultPfaceFactory::ResultPfaceFactory(const ResultPfaceFactory& other){
      set(other);
    }// Konstruktor
 
    ResultPfaceFactory::ResultPfaceFactory(size_t size){
      inittialize(size);
    }// Konstruktor    
     
    ResultPfaceFactory::ResultPfaceFactory(
        ContainerPoint3D& points,
        GlobalTimeValues &timeValues,
        PlaneSweepAccess &access ){
      inittialize(timeValues.size());
      double t1,t2;
      if (timeValues.first(t1) && timeValues.next(t2)){ 
        size_t n = this->segments.size();
        access.first(t1, t2, points, this->segments);
        size_t k;
        for(k = 0; k < timeValues.size(); k++){    
          for(size_t i = n; i < segments.size(); i++){
            Point3D head, tail;
            Segment segment = segments.get(i);
            head = points.get(segment.getHead());
            tail = points.get(segment.getTail());
            if(head.getZ() == tail.getZ()){
              orthogonalEdges[k].push_back(i);
            }// if
            else {
              if(k != timeValues.size()-1){
                nonOrthogonalEdges[k].push_back(i); 
              }// if
              else {
                 NUM_FAIL("Only ortogonal segments are allowed.");
              }// else
            }// else
          }// for
          t1 = t2;
          if(k != timeValues.size()-1) timeValues.next(t2);
          n = this->segments.size();          
          access.next(t1, t2, points, segments);
        }// for
      }// if
    }// Konstruktor

    void ResultPfaceFactory::inittialize(size_t size){
      this->nonOrthogonalEdges = vector<list<size_t>>(size-1,list<size_t>());
      this->orthogonalEdges    = vector<list<size_t>>(size,list<size_t>());
      this->touchsOnLeftBorder = vector<size_t>(size,0);
    }// inittialize
     
    void ResultPfaceFactory::set(vector<list<size_t>>& edges1,
                                 const vector<list<size_t>>& edges2){
      edges1 = vector<list<size_t>>(edges2.size(),list<size_t>());
      list<size_t>::const_iterator edgeIter;
      for(size_t i = 0; i < edges2.size(); i++ ){
        for(edgeIter  = edges2[i].begin();
            edgeIter != edges2[i].end();
            edgeIter++) {
          edges1[i].push_back(*edgeIter);
        }// for
      }//for
    }// set
     
    void ResultPfaceFactory::set(const ResultPfaceFactory& other){
      this->segments = other.segments;
      set(this->nonOrthogonalEdges, other.nonOrthogonalEdges);
      set(this->orthogonalEdges, other.orthogonalEdges);
      this->touchsOnLeftBorder = vector<size_t>(
        other.touchsOnLeftBorder.size(),0);
      for(size_t i = 0; i < other.touchsOnLeftBorder.size(); i++){
        this->touchsOnLeftBorder[i] = other.touchsOnLeftBorder[i];
      }// for
    }// set
    
    ResultPfaceFactory& ResultPfaceFactory::operator =(
      const ResultPfaceFactory& other){
      set(other);
      return *this;
    }// Operator =
      
    void ResultPfaceFactory::addNonOrthogonalEdges(size_t slide, 
                                                   const Segment& segment){
      if(slide < nonOrthogonalEdges.size()){
        size_t index = segments.add(segment);
        nonOrthogonalEdges[slide].push_back(index);
      }// if
      else NUM_FAIL("Index for slide is out of range.");
    }// addNonOrthogonalEdges
    
    void ResultPfaceFactory::addOrthogonalEdges(size_t slide, 
                                                const Segment& segment){
      if(slide < orthogonalEdges.size()){
        size_t index = segments.add(segment);
        orthogonalEdges[slide].push_back(index);
      }// if
      else NUM_FAIL("Index for slide is out of range.");
    }// addOrthogonalEdge
     
    void ResultPfaceFactory::setTouchsOnLeftBorder(size_t slide, size_t value){
      if(slide < touchsOnLeftBorder.size()) touchsOnLeftBorder[slide] = value;
      else NUM_FAIL("Index for slide is out of range.");
    }// setTouch
    
    bool ResultPfaceFactory::iSEqual(const vector<list<size_t>>& edges1,
                                     const vector<list<size_t>>& edges2)const{
      if(edges1.size() != edges2.size()) return false;
      list<size_t>::const_iterator edge1Iter,edge2Iter;
      for(size_t i = 0; i < edges1.size(); i++){
        if(edges1[i].size() != edges2[i].size()) return false;
        for(edge1Iter  = edges1[i].begin(), 
            edge2Iter  = edges2[i].begin();
            edge1Iter != edges1[i].end();
            edge1Iter++,edge2Iter++){
          if(!(*edge1Iter == *edge2Iter)) return false;
        }// for
      }// for  
      return true;
    }// compare

    bool ResultPfaceFactory::operator ==(
        const ResultPfaceFactory& other)const{
      // cout << "Check segments" << endl;    
      if(!(this->segments == other.segments)) return false;     
      // cout << "Check orthogonal edges" << endl;
      if(!(iSEqual(this->orthogonalEdges,other.orthogonalEdges))) 
        return false;
      // cout << "Check non orthogonal edges" << endl;
      if(!(iSEqual(this->nonOrthogonalEdges,other.nonOrthogonalEdges)))
        return false;
      // cout << "Check touchs on left border" << endl;
      vector<size_t>::const_iterator iter1,iter2;
      if(this->touchsOnLeftBorder.size() != other.touchsOnLeftBorder.size())
        return false;
      for(iter1  = this->touchsOnLeftBorder.begin(), 
          iter2 = other.touchsOnLeftBorder.begin();
          iter1 != this->touchsOnLeftBorder.end(); iter1++, iter2 ++){
         if(*iter1 != *iter2) return false;
      }// for
      return true;
    }// Operator ==
     
    void ResultPfaceFactory::evaluate(){
      list<size_t>::iterator first;
      list<size_t>::reverse_iterator last; 
      for(size_t i = 0; i < nonOrthogonalEdges.size(); i++){
        evaluate(i);
      }// for
      for(size_t i = 1; i < nonOrthogonalEdges.size(); i++){
        if(nonOrthogonalEdges[i-1].size() < 2) break;
        first = nonOrthogonalEdges[i-1].begin();
         Predicate predicate = segments.get(*first).getPredicate();
         setSlidePredicates(predicate, i, touchsOnLeftBorder[i]);
       }// for  
       for(int i = nonOrthogonalEdges.size()-2; i >= 0; i--){
         if(nonOrthogonalEdges[i+1].size() < 2) break;
         first = nonOrthogonalEdges[i+1].begin();
         Predicate predicate = segments.get(*first).getPredicate();
         setSlidePredicates(predicate, i, touchsOnLeftBorder[i+1]);
       }// for  
    }// evaluate
    
    void ResultPfaceFactory::getBorderPredicates(Predicate& left, 
                                                 Predicate& right)const{
      size_t first,last;
      Predicate predicate;
      left  = UNDEFINED;
      right = UNDEFINED;
      if(segments.size() != 0){
        for(size_t i = 0; i < nonOrthogonalEdges.size(); i++){
          first = *nonOrthogonalEdges[i].begin();
          predicate = createPredicate(segments.get(first).getPredicate(),LEFT);
          if( left == UNDEFINED) left = predicate;
          else if( left != INTERSECT && left != predicate )left = INTERSECT;  
          last = *nonOrthogonalEdges[i].rbegin();
          predicate = createPredicate(segments.get(last).getPredicate(),RIGHT);
          if( right == UNDEFINED) right = predicate;
          else if( right != INTERSECT && right != predicate )right = INTERSECT;
        }// for
      }// if
    }// getBorderPredicates
       
    void ResultPfaceFactory::setSlidePredicates(Predicate predicate,
                                                size_t slide, size_t touch){
       list<size_t>::iterator first, iter;
       if(nonOrthogonalEdges[slide].size() < 2) return;
       first = nonOrthogonalEdges[slide].begin();
       if( predicate != UNDEFINED &&
           segments.get(*first).getPredicate() == UNDEFINED){
         predicate =createPredicate(predicate, LEFT);
         if (touch%2 == 1){
           if (predicate == OUTSIDE) predicate = INSIDE;
           else predicate = OUTSIDE;
         }// if
         for(iter = nonOrthogonalEdges[slide].begin(); 
             iter != nonOrthogonalEdges[slide].end(); iter++){
           if (segments.get(*iter).getPredicate() == UNDEFINED){
             setPredicate(*iter,predicate);
           }// if
           else {
             NUM_FAIL("Only segments with predicate UNDEFINED allowed.");
          }// else
        }// for
      }// if
    }// setSlidePredicates
       
    void ResultPfaceFactory::evaluate(size_t i ){      
      list<size_t>::iterator first, left, right, orthogonal;
      Segment firstSegment, leftSegment, rightSegment, orthogonalSegment;
      
      if(nonOrthogonalEdges[i].size()< 2) return;
      first = nonOrthogonalEdges[i].begin();
      right = left = first;  
      firstSegment = segments.get(*first);
      for(right++; right !=  nonOrthogonalEdges[i].end(); 
          right++){
        Predicate predicate = UNDEFINED;
        leftSegment  = segments.get(*left);
        rightSegment = segments.get(*right);
        checkPredicate(leftSegment,LEFT,predicate); 
        checkPredicate(rightSegment,RIGHT,predicate);   
        if(leftSegment.getPredicate() != UNDEFINED){
          if(firstSegment.getTail() == leftSegment.getTail()){
            touchsOnLeftBorder[i]++;
          }// if
          if(firstSegment.getHead() == leftSegment.getHead()){
            touchsOnLeftBorder[i+1]++;
          }// if
        }// if
        if(predicate == UNDEFINED){
          // check bottom 
          for(orthogonal  = orthogonalEdges[i].begin(); 
              orthogonal != orthogonalEdges[i].end(); 
              orthogonal ++){  
            orthogonalSegment = segments.get(*orthogonal);
            if(leftSegment.getTail() == orthogonalSegment.getTail() && 
               rightSegment.getTail()== orthogonalSegment.getHead()){
              // cout << "Bottom" <<endl;
              checkPredicate(orthogonalSegment,RIGHT,predicate); 
              break;
            }// if
          }// for
          // check top
          for(orthogonal  = orthogonalEdges[i+1].begin(); 
              orthogonal != orthogonalEdges[i+1].end(); 
              orthogonal ++){
            orthogonalSegment = segments.get(*orthogonal);
            if(leftSegment.getHead()  == orthogonalSegment.getTail() && 
               rightSegment.getHead() == orthogonalSegment.getHead()){
              // cout << "Top" <<endl;
              checkPredicate(orthogonalSegment,LEFT,predicate);
              break;
            }// if
          }// for
        }// if
        setPredicate(*left, predicate);
        setPredicate(*right, predicate);
        left = right; 
      }// for
      for(orthogonal  = orthogonalEdges[i].begin(); 
          orthogonal != orthogonalEdges[i].end(); 
          orthogonal ++){
         orthogonalSegment = segments.get(*orthogonal);
        if(firstSegment.getTail() == orthogonalSegment.getTail()){
          touchsOnLeftBorder[i]++;
        }// if
        if(firstSegment.getHead() == orthogonalSegment.getTail()){
          touchsOnLeftBorder[i+1]++;
        }// if  
        if(firstSegment.getTail() == orthogonalSegment.getHead()){
          touchsOnLeftBorder[i]++;
        }// if
        if(firstSegment.getHead() == orthogonalSegment.getHead()){
          touchsOnLeftBorder[i+1]++;          
        }// if
      }// for  
      list<size_t>::reverse_iterator last, riter;
      riter = last = nonOrthogonalEdges[i].rbegin();
      rightSegment = segments.get(*last); 
      if(firstSegment.getPredicate() == UNDEFINED && 
        rightSegment.getPredicate()  != UNDEFINED){
        for (riter++; riter != nonOrthogonalEdges[i].rend(); riter++){
          rightSegment= segments.get(*last);
          leftSegment = segments.get(*riter);
          if(leftSegment.getPredicate() == UNDEFINED){
            Predicate predicate =
              createPredicate(rightSegment.getPredicate(), LEFT);
            if(predicate != UNDEFINED){
              setPredicate(*riter, predicate);
            }// if
            else {
              NUM_FAIL("Predicate UNDEFINED ist not allowed.");
            }// esle
          }// if
          last++;
        }// for
      }// if
    }// evaluate   
     
    void ResultPfaceFactory::setPredicate(size_t index, 
                                          Predicate& predicate){
       if(segments.get(index).getPredicate() == UNDEFINED) 
          segments.set(index,predicate);
    }// setPredicate
     
    Predicate ResultPfaceFactory::createPredicate(const Predicate source,
                                                  const Border border)const{
      Predicate predicate;
      switch(source){        
        case LEFT_IS_INNER:  if(border == LEFT) predicate = OUTSIDE;
                             else predicate = INSIDE;
                             break;    
        case RIGHT_IS_INNER: if(border == LEFT) predicate = INSIDE;
                             else predicate = OUTSIDE;
                             break;     
        default:             predicate = source;         
      }// switch
      return predicate;   
    }// createPredicate
         
    void ResultPfaceFactory::checkPredicate(const Segment& segment,
                                            const Border border, 
                                             Predicate& result)const{
      Predicate predicate = createPredicate(segment.getPredicate(), border);
      if(predicate == INSIDE || predicate == OUTSIDE){
        if(result == UNDEFINED) result = predicate;
        else if (!(result == predicate)) {
          NUM_FAIL("Different Predicates on edges.");
        }// else if 
      }// if
    }// checkPredicate
         
    void ResultPfaceFactory::print(std::ostream& os, 
                                   std::string prefix,
                                   vector<list<size_t>> edges)const{
      list<size_t>::const_iterator iter;
      for (size_t i = 0; i < edges.size();i++){
        os << prefix << "     Index:=" << i << " (";
        for (iter = edges[i].begin(); iter != edges[i].end();){
          os << *iter; 
          iter++;
          if(iter != edges[i].end()) os << ", ";           
        }// for  
        os << ")" << endl;
      }// for
      os << prefix << "  )" << endl;
    }// print  

    std::ostream& ResultPfaceFactory::print(std::ostream& os, 
                                             std::string prefix)const{
      os << prefix << "ResultPFaceFactory(";
      if (segments.size() == 0) os << "is empty)" << endl;
      else {
        os << endl << prefix;
        segments.print(os, prefix+"  ");
        os << prefix << "  Non orthogonal edge for pfaces(" << endl;
        print(os, prefix, nonOrthogonalEdges);
        os << prefix << "  Orthogonal edge for pfaces(" << endl;
        print(os, prefix, orthogonalEdges);
        vector<size_t>::const_iterator iter;       
        os << prefix << "  Touch on left border(" << endl;
        for(size_t i = 0; i < touchsOnLeftBorder.size(); i++){
          os << prefix << "    Index:="<< i << " (";
          os << touchsOnLeftBorder[i] << ")" << endl;
        }// for
        os << prefix << "  )" << endl;
        os << prefix << ")" << endl;
      }// else
       return os;
    }// operator <<
     
    std::ostream& operator <<(std::ostream& os, 
                              const ResultPfaceFactory& factory){   
      return factory.print(os,"");
    }// operator 
    
    void ResultPfaceFactory::getResultUnit(size_t slide, Predicate predicate,
                                           bool reverse, 
                                           const ContainerPoint3D& points, 
                                           ResultUnit& unit){  
      list<size_t>::const_iterator left, right;
      if(slide < nonOrthogonalEdges.size()){
        if(nonOrthogonalEdges[slide].size() > 1){
          left = right = nonOrthogonalEdges[slide].begin();
          for(right++; right != nonOrthogonalEdges[slide].end(); right++){
            Segment leftSegment  = segments.get(*left);
            Segment rightSegment = segments.get(*right);
            Predicate leftPredicate  = 
              createPredicate(leftSegment.getPredicate(),LEFT);
            Predicate rightPredicate = 
              createPredicate(rightSegment.getPredicate(),RIGHT);
            if(leftPredicate == rightPredicate){
              if(leftPredicate == predicate){
                if(reverse){
                  Segment temp = leftSegment;
                  leftSegment  = rightSegment;
                  rightSegment = temp;
                }// if
                Point3D leftTail  = points.get(leftSegment.getTail());
                Point3D leftHead  = points.get(leftSegment.getHead());
                Point3D rightTail = points.get(rightSegment.getTail());
                Point3D rightHead = points.get(rightSegment.getHead());
                MSegment segment(Segment3D(leftTail,leftHead), 
                                 Segment3D(rightTail,rightHead));
                unit.addMSegment(segment,false);
              }// if
            }// if
            else {
              NUM_FAIL ("Predicate on left and right border are different.");
            }// else  
            left = right;
          }// for
        }//if
      }// if
    }// getResultPFace  
/*
15 Class PFace

*/  
    PFace::PFace(size_t left, size_t right, const ContainerPoint3D& points, 
            const ContainerSegment& segments){
      Segment borderLeft  = segments.get(left);
      Segment borderRight = segments.get(right);
      this->leftStart = points.get(borderLeft.getTail());
      this->leftEnd   = points.get(borderLeft.getHead());
      this->rightStart= points.get(borderRight.getTail());
      this->rightEnd  = points.get(borderRight.getHead());
      this->left      = left;
      this->right     = right;
      this->state     = UNKNOWN;
      boundingRect = getBoundingRec(leftStart);
      boundingRect.Extend(getBoundingRec(leftEnd));
      boundingRect.Extend(getBoundingRec(rightStart));
      boundingRect.Extend(getBoundingRec(rightEnd));      
    }// Konstruktor 
    
    PFace::PFace(const PFace& pf){
      set(pf);
    }// Konstruktor
    
    void PFace::set(const PFace& pf){
      this->leftStart = pf.leftStart;
      this->leftEnd   = pf.leftEnd;
      this->rightStart= pf.rightStart;
      this->rightEnd  = pf.rightEnd;
      this->left      = pf.left;
      this->right     = pf.right;      
      this->state       = pf.state;
      this->boundingRect= pf.boundingRect;
      this->intSegs     = pf.intSegs;     
    }// Konstruktor
    
    void PFace::setState(State state){
      this->state = state;
    }// setState
      
    Point3D PFace::getLeftStart() const{
      return this->leftStart;
    }// getA
    
    Point3D PFace::getLeftEnd() const{
      return this->leftEnd;
    }// getB
    
    Point3D PFace::getRightStart() const{
      return this->rightStart;
    }// getC
    
    Point3D PFace::getRightEnd() const{
      return this->rightEnd;
    }// getD    
    
    State PFace::getState() const{
      return state;
    }// getState
    
    bool PFace::existsIntSegs()const{
      return (this->intSegs.size() != 0);
    }// hasIntseg
    
    string PFace::toString(State state){
      switch(state){
        case UNKNOWN:      return "UNKNOWN";
        case RELEVANT:     return "RELEVANT";
        case CRITICAL:     return "CRITICAL";
        case NOT_RELEVANT: return "NOT_RELEVANT";
        default: return "";
      }// switch
    }// toString
    
    Rectangle<2> PFace::getBoundingRec(const Point3D& point)const{
      double array[2] = {point.getX(),point.getY()};
      return Rectangle<2>(true,array,array);
    }// getBoundingBox 
    
    Rectangle<2> PFace::getBoundingRec()const{
      return boundingRect;
    }// getBoundingBox 
    
    std::ostream& operator <<(std::ostream& os, const PFace& pf){
      return pf.print(os,"");
    }// operator 
    
    std::ostream& PFace::print(std::ostream& os, std::string prefix)const{
      os << "PFace ( " << endl; 
      os << prefix << "  left border:=" << left;
      os << ", right border:=" << right << " ," << endl;
      os << prefix << "  state:=" << PFace::toString(state) << " ," << endl;
      os << prefix << "  left start point:=" << leftStart;
      os << " , left end point:="<< leftEnd << " ," << endl;
      os << prefix << "  right start point:=" << rightStart;
      os << " , right end point:=" << rightEnd << " ," << endl;;
      this->intSegs.print(os,"  "+prefix);
      this->factory.print(os,"  "+prefix);
      os << prefix <<")" << endl;
      return os;
    }// print

    PFace& PFace::operator =(
        const PFace& pf){
      set(pf);
      return *this;
    }// Operator =
    
    bool PFace::operator ==(const PFace& pf)const{
      if ((this->leftStart == pf.leftStart) &&
          (this->leftEnd == pf.leftEnd) &&
          (this->rightStart == pf.rightStart) &&
          (this->rightEnd == pf.rightEnd) &&
          (this->state == pf.state) &&
          (this->boundingRect == pf.boundingRect)&&
          (this->intSegs == pf.intSegs)) return true;
      return false;
    }// Operator == 
    
    void PFace::addIntSeg(const IntersectionSegment& seg){
      this->intSegs.addIntSeg(seg);
    }// addIntSeg
    
    void PFace::addIntSeg(const RationalPlane3D &planeSelf,
                          const RationalPlane3D &planeOther,
                          const RationalSegment3D &intSeg,
                          GlobalTimeValues &timeValues){
      Predicate result = LEFT_IS_INNER;
      if(!planeSelf.isLeftAreaInner(intSeg,planeOther)){
        result = RIGHT_IS_INNER;
      } // if
      Segment2D segment = planeSelf.transform(intSeg);
      if(this->state != CRITICAL)this->state = RELEVANT;     
      IntersectionSegment iSeg(intSeg,segment,result);
      timeValues.addTimeValue(iSeg.getTail().getT());
      timeValues.addTimeValue(iSeg.getHead().getT());
      addIntSeg(iSeg);
    }// addIntSeg
    
    IntersectionSegment PFace::createBorder( 
        const RationalPlane3D &planeSelf, Border border, Predicate predicate){
      Segment3D segment3D;
      Segment2D segment2D;
      if(border == LEFT){
        segment3D = Segment3D(this->leftStart,this->leftEnd);
        segment2D = planeSelf.transform(segment3D);
      }// if
      else {
        segment3D = Segment3D(this->rightStart,this->rightEnd);
        segment2D = planeSelf.transform(segment3D);
      }// else
      return IntersectionSegment(segment3D,segment2D,predicate);
    }// createBorder 
    
    void PFace::addBorder(const RationalPlane3D &plane,
                          GlobalTimeValues &timeValues, 
                          Predicate predicate){
      IntersectionSegment iSeg = createBorder(plane,LEFT,predicate);
      timeValues.addTimeValue(iSeg.getTail().getT());
      timeValues.addTimeValue(iSeg.getHead().getT());
      addIntSeg(iSeg); 
      iSeg = createBorder(plane,RIGHT,predicate);
      timeValues.addTimeValue(iSeg.getTail().getT());
      timeValues.addTimeValue(iSeg.getHead().getT());
      addIntSeg(iSeg); 
    }// addBorder   
    
    // for pFace with intersection
    void PFace::addBorder(GlobalTimeValues &timeValues){
      RationalPlane3D plane(*this);
      addBorder(plane,timeValues,UNDEFINED);
    }// addBorder 
    
    // for pFace without intersection
    void PFace::addBorder(GlobalTimeValues &timeValues, 
                          const ContainerSegment& segments, 
                          Predicate predicate){
      Predicate leftPredicate  = segments.get(left).getPredicate();
      Predicate rightPredicate = segments.get(right).getPredicate();            
      if((state == UNKNOWN) && (leftPredicate == rightPredicate) && 
        ((leftPredicate == INSIDE) || (leftPredicate == OUTSIDE))){
        if(leftPredicate == predicate){
          state = RELEVANT;
          RationalPlane3D plane(*this);
          addBorder(plane,timeValues,predicate);
        }// if
        else {
          state = NOT_RELEVANT;
        }// if
      }// if
    }// addBorder 
 
    bool PFace::intersection(PFace& other,GlobalTimeValues &timeValues){
      Rectangle<2> bRec = boundingRect;
      // Boundingbox etwas vergrößern
      bRec.Extend(NumericUtil::eps2);
      // check bounding rectangles
      if(!(this->boundingRect.Intersects(other.boundingRect))){
        return false; 
      }// if
      // create planes
      RationalPlane3D planeSelf(*this);
      RationalPlane3D planeOther(other);
      // check planes
      if (planeSelf.isParallelTo(planeOther)) {
        if(planeSelf.isCoplanarTo(planeOther)) {
          this->state = CRITICAL;
          other.state = CRITICAL;           
        }// if 
        else {
        }// else
        return false;
      }// if
      RationalPoint3DExtSet intPointSet;
      planeSelf.intersection(other, UNIT_A, intPointSet);
      // We need exactly two intersection points.
      if (intPointSet.size() != 2) return false; 
      planeOther.intersection(*this, UNIT_B, intPointSet);  
      // There is no intersection
      RationalSegment3D intSeg;
      if(!intPointSet.getIntersectionSegment(intSeg)) return false;  
      IntersectionSegment iSeg;
      // create and save result segments
      addIntSeg(planeSelf,planeOther,intSeg,timeValues);
      other.addIntSeg(planeOther,planeSelf,intSeg,timeValues); 
      return true;    
    }// intersection   
    
    void PFace::first(double t1, double t2, ContainerPoint3D& points,
                                            ContainerSegment& segments){ 
       intSegs.first(t1, t2, points, segments);
    }// first
      
    void PFace::next(double t1, double t2, ContainerPoint3D& points, 
                                           ContainerSegment& segments){ 
      intSegs.next(t1, t2, points, segments); 
    }// next
    
    bool PFace::finalize(ContainerPoint3D& points, ContainerSegment& segments, 
                         GlobalTimeValues& timeValues){
      Predicate leftPredicate, rightPredicate;
      bool result;
      if(this->state == RELEVANT || this->state == CRITICAL){
        this->factory = ResultPfaceFactory(points, timeValues, *this);
        this->factory.evaluate();
        this->factory.getBorderPredicates(leftPredicate,rightPredicate);
        segments.set(this->left,leftPredicate);
        segments.set(this->right,rightPredicate); 
        if(leftPredicate == UNDEFINED && rightPredicate == UNDEFINED){
          result = false;
        }// if
        else if(leftPredicate != UNDEFINED && rightPredicate != UNDEFINED){
          result =true;
        }// else if
        else NUM_FAIL ("Only one edge with predicate 'UNDEFINED' exists.");
      }// if
      else {
        leftPredicate  = segments.get(this->left).getPredicate();
        rightPredicate = segments.get(this->right).getPredicate();
        if(leftPredicate == INSIDE || leftPredicate == OUTSIDE){
          if(rightPredicate == UNDEFINED) {
            segments.set(this->right,leftPredicate);
          }// if
          result = true;
        }// if
        if(rightPredicate == INSIDE || rightPredicate == OUTSIDE){
          if(leftPredicate == UNDEFINED) {
            segments.set(this->left,rightPredicate);
          }// if
          result = true;
        }// if
        result = false;
      }// else
      return result;
    }// finalize
    
    void PFace::getResultUnit(size_t slide, Predicate predicate,
                               bool reverse, 
                               const ContainerPoint3D& points, 
                               ResultUnit& unit){
      this->factory.getResultUnit(slide,predicate,reverse,points,unit);
    }// getResultPFace
    /*
16 class SourceUnit

*/          
    SourceUnit::SourceUnit():pFaceTree(4,8){      
    }// Konstruktor
    
    SourceUnit::SourceUnit(const SourceUnit& other):pFaceTree(4,8){
      set(other);
    }// Konstruktor
    
    SourceUnit::~SourceUnit(){
      vector<PFace*>::iterator iter;
      for (iter = pFaces.begin(); iter != pFaces.end(); iter++) {           
        delete *iter;
      }// for
    }// Destruktor
    
    void SourceUnit::set(const SourceUnit& other){
      this->segments = other.segments;
      for(size_t i = 0; i < other.pFaces.size(); i++){
        PFace* pFace = new PFace(PFace(*other.pFaces[i]));
        Rectangle<2> boundigRec = pFace->getBoundingRec();
        size_t index = this->pFaces.size();
        this->pFaces.push_back(pFace);
        this->pFaceTree.insert(boundigRec,index);
      }// for
    }// set     

    void SourceUnit::addPFace(const Segment& leftSegment, 
                              const Segment& rightSegment, 
                              const ContainerPoint3D& points){    
      size_t left  = this->segments.add(leftSegment);
      size_t right = this->segments.add(rightSegment);
      PFace* pFace = new PFace(left,right,points,this->segments);
      Rectangle<2> boundigRec = pFace->getBoundingRec();
      size_t index = pFaces.size();
      pFaces.push_back(pFace);
      pFaceTree.insert(boundigRec,index);                                
    }// addPFace
        
    bool SourceUnit::isEmpty()const{
      return pFaces.size()==0;
    }// is Empty
    
    bool SourceUnit::intersect(const SourceUnit& other)const{
      Rectangle<2> bRecA = this->pFaceTree.getBBox();
      Rectangle<2> bRecB = other.pFaceTree.getBBox();
      return bRecA.Intersects(bRecB);
    }// intersect
        
    void SourceUnit::addToResult(std::vector<ResultUnit>& result)const{
      if(pFaces.size()!=0){
        PFace pFace = *pFaces[0];
        if(result.size() == 0){
          double t1 = pFace.getLeftStart().getZ();
          double t2 = pFace.getLeftEnd().getZ();        
          result.push_back(ResultUnit(t1,t2));
        }// if
        for(size_t i = 0; i < pFaces.size(); i ++){
          pFace = *pFaces[i];
          Point3D leftStart = pFace.getLeftStart();
          Point3D leftEnd   = pFace.getLeftEnd();
          Point3D rightStart= pFace.getRightStart();
          Point3D rightEnd  = pFace.getRightEnd();
          MSegment msegment(Segment3D(leftStart,leftEnd), 
                            Segment3D(rightStart, rightEnd));
          result[0].addMSegment(msegment,false);
        }// for
      }// if
    }// addToResult
    
    bool SourceUnit::intersection(SourceUnit& other, 
                                  GlobalTimeValues& timeValues){
      bool result =false;
      for(size_t i = 0; i < this->pFaces.size(); i++){
        PFace* pFaceA = this->pFaces[i];
        Rectangle<2> bRec = (*pFaceA).getBoundingRec();
        // Boundingbox etwas vergrößern
        bRec.Extend(NumericUtil::eps2);
        // Iterator über die gefundenen Dreiecke erstellen
        std::unique_ptr<mmrtree::RtreeT<2, size_t>::iterator> 
          it(other.pFaceTree.find(bRec)); 
        size_t const* j;  
        while((j = it->next()) != 0) {
          PFace* pFaceB = other.pFaces[*j];
          pFaceA->intersection(*pFaceB,timeValues);
        }// while
        if(pFaceA->existsIntSegs()){
          pFaceA->addBorder(timeValues);
          this->itersectedPFace.push_back(i);
        }// if
      }//for 
      for (size_t j = 0; j < other.pFaces.size(); j++) {
        PFace* pFaceB =other.pFaces[j];
        if(pFaceB->existsIntSegs()){
          pFaceB->addBorder(timeValues);
          other.itersectedPFace.push_back(j);
        }// if
      }// for
      return result;
    }// intersection
    
    bool SourceUnit::finalize(ContainerPoint3D& points, 
                              GlobalTimeValues& timeValues, 
                              Predicate predicate){
      vector<bool> ok = vector<bool>(pFaces.size(),false);
      // zuerst alle PFaces mit Schnitte
      size_t j =0;
      bool finalize;
      do{        
        finalize = true;
        for(size_t i = 0; i < this->itersectedPFace.size(); i++){
          size_t index = itersectedPFace[i];
          if(!ok[index]){
            bool result = this->pFaces[index]->finalize(
              points, this->segments,timeValues);
            if(result != true) finalize = false;
            else ok[index] = result;
          }// if
        }// for 
        j++;
        if(j >2) return false;
      } while (!finalize);
      // jetzt alle anderen PFaces
      j = 0;
      do{
        finalize = true;
        for(size_t i = 0; i < this->pFaces.size(); i++){
          if(!ok[i]){
            this->pFaces[i]->addBorder(timeValues,segments,predicate);
            bool result = this->pFaces[i]->finalize(
              points,this->segments,timeValues);
            if(result != true) finalize = false;
            else ok[i] = result;
          }// if
        }// for  
        j++;
        if(j >2) return false;
      } while (!finalize);
      return true;
    }// finalize
    
    void SourceUnit::getResultUnit(size_t slide, Predicate predicate,
                              bool reverse, 
                              const ContainerPoint3D& points, 
                              ResultUnit& unit){
       for(size_t i = 0; i < this->pFaces.size(); i++){
         this->pFaces[i]->getResultUnit(slide,predicate,reverse,points,unit);
       }// for
    }// getResultPFace

/*
    bool SourceUnit::intersection(SourceUnit& other, GlobalTimeValues& timeValues){
      bool result =false;
      vector<PFace*>::iterator iter;
      for (iter = this->pFaces.begin(); iter != this->pFaces.end(); iter++) {
        PFace* pFaceA = *iter;
        Rectangle<2> bRec = (*pFaceA).getBoundingRec();
        // Boundingbox etwas vergrößern
        bRec.Extend(NumericUtil::eps2);
        RationalPlane3D planeSelf(*pFaceA);
        // Iterator über die gefundenen Dreiecke erstellen
        std::unique_ptr<mmrtree::RtreeT<2, size_t>::iterator> 
          it(other.pFaceTree.find(bRec));      
        size_t const *bRecIndex;
        while((bRecIndex = it->next()) != 0) {
          PFace* pFaceB = other.pFaces[*bRecIndex];
          RationalPlane3D planeOther(*pFaceB);
          // check planes
          if (planeSelf.isParallelTo(planeOther)) {
            if(planeSelf.isCoplanarTo(planeOther)) {
              pFaceA->setState(CRITICAL);
              pFaceB->setState(CRITICAL);           
            }// if 
            break;
          }// if
          RationalPoint3DExtSet intPointSet;
          planeSelf.intersection(*pFaceB, PFACE_A, intPointSet);
          // We need exactly two intersection points.
          if (intPointSet.size() != 2) break; 
          planeOther.intersection(*pFaceA, PFACE_B, intPointSet);  
          // There is no intersection
          RationalSegment3D intSeg;
          if(!intPointSet.getIntersectionSegment(intSeg)) break;  
          IntersectionSegment iSeg;
          // create and save result segments  
          pFaceA->addIntSeg(planeSelf,planeOther,intSeg,timeValues);
          pFaceB->addIntSeg(planeOther,planeSelf,intSeg,timeValues);      
          result = true;
        }// while
        if(pFaceA->existsIntSegs()){
          pFaceA->addBorder(planeSelf, timeValues, UNDEFINED);
        }// if
      }// for
      for (iter = other.pFaces.begin(); iter != other.pFaces.end(); iter++) {
        PFace* pFaceB = *iter;
        if(pFaceB->existsIntSegs()){
          RationalPlane3D planeOther(*pFaceB);
          pFaceB->addBorder(planeOther, timeValues,UNDEFINED);
        }// if
      }// for
      return result;
    }// intersection
*/    

    std::ostream& SourceUnit::print(std::ostream& os, std::string prefix)const{
      os << prefix << "SourceUnit (";
      if (segments.size() == 0) os << "is empty)" << endl;
      else {
        os << endl;
        this->segments.print(os,prefix+"  ");
        os << prefix << "  PFaces (" << endl;
        for(size_t i = 0; i < pFaces.size(); i++){
          os << prefix << "    Index:=" << i << ", " ; 
          this->pFaces[i]->print(os,prefix+"    ");
        }// for
        os << prefix << "  )"<<endl;
        os << prefix << ")"<<endl;
      }// else
      return os;
    }// print
    
    std::ostream& operator <<(std::ostream& os, const SourceUnit& unit){
      unit.print(os,"");
      return os;
    }// Operator <<
    
    bool SourceUnit::operator ==(const SourceUnit& unit)const{
      if(!(this->segments == unit.segments)) return false;
      if(this->pFaces.size() != unit.pFaces.size()) return false;
      for(size_t i = 0; i < this->pFaces.size(); i++){
        if(!(*(this->pFaces[i]) == *(unit.pFaces[i]))) return false;
      }// for
      return true;
    }// Operator ==
    
    SourceUnit& SourceUnit::operator =(const SourceUnit& unit){
      set(unit);
      return *this;
    }// Operator =
    
    SourceUnitPair::SourceUnitPair(){
    }
      
    void SourceUnitPair::addPFace(SourceFlag flag, Segment3D& leftSegment, 
                                  Segment3D& rightSegment){
      Point3D leftStart = leftSegment.getTail();
      Point3D leftEnd   = leftSegment.getHead();
      Point3D rightStart= rightSegment.getTail();
      Point3D rightEnd  = rightSegment.getHead();
      size_t iLeftStart = points.add(leftStart);
      size_t iLeftEnd   = points.add(leftEnd);
      size_t iRightStart= points.add(rightStart); 
      size_t iRightEnd  = points.add(rightEnd);
      Segment left(iLeftStart,iLeftEnd, UNDEFINED);
      Segment right(iRightStart, iRightEnd, UNDEFINED);
      
      if(flag == UNIT_A){
        unitA.addPFace(left,right,points); 
      }// if
      else {
        unitB.addPFace(left,right,points);
      }// else     
    }// addPFace
    
    std::ostream& SourceUnitPair::print(std::ostream& os, 
                                        std::string prefix)const{
      os << prefix << "SourceUnitPair (" << endl;
      os << prefix << "  Points:=" << endl;
      this->points.print(os,"    ");
      os << prefix << "  )" << endl;
      os << prefix << "  TimeValues:=" << this->timeValues << endl;
      os << prefix << "  UnitA:= "<< endl;
      this->unitA.print(os,"    ");
      os << prefix << "  )" << endl;
      os << prefix << "  UnitB:= "<< endl;
      this->unitB.print(os,"    ");
      os << prefix << "  )" << endl;
      if(result.size() == 0) os << prefix << "  No result exist" << endl;
      else {
        os << prefix << "  Result:=" << endl;
        for(size_t i = 0; i < result.size(); i++){
          result[i].print(os, prefix+"    ");
        }// for
      }// if
      os << prefix << ")" << endl;    
      return os;      
    }// print
    
    std::ostream& operator <<(std::ostream& os, 
                              const SourceUnitPair& unitPair){
      unitPair.print(os,"");
      return os;
    }// Operator <<
    
    bool SourceUnitPair::operate(SetOp setOp){  
      if(unitA.isEmpty()|| unitB.isEmpty() ||
        (!unitA.intersect(unitB))){
        if(setOp == INTERSECTION) {
          // Result is empty: nothing to do.
          return false;
        }// if
        if(setOp == MINUS){
          if(unitA.isEmpty()){
            // Result is empty: nothing to do.
            return false;
          }// if
          // unitB is empty
          else{
            unitA.addToResult(result);
            result[0].finalize();
            return false;
          }// if
        }// if
        // setOp == UNION
        unitA.addToResult(result);
        unitB.addToResult(result);
        result[0].finalize();
        return false;
      }// if
      // Intersection
      unitA.intersection(unitB, timeValues);
      // Finalize
      bool inverseB = false;
      Predicate predicateA = OUTSIDE;
      Predicate predicateB = OUTSIDE;
      if(setOp == MINUS){
        inverseB   = true;
        predicateB = INSIDE;
      }// if
      else if(setOp == INTERSECTION){
        predicateA = INSIDE;
        predicateB = INSIDE;
      }// else if      
      unitA.finalize(points, timeValues,predicateA);      
      unitB.finalize(points, timeValues,predicateB);
      // get result Units          
      result = vector<ResultUnit>(timeValues.size()-1, ResultUnit());
      double t1,t2;
      size_t i = 0;
      timeValues.first(t1);
      while(timeValues.next(t2)){
        result[i] = ResultUnit(t1,t2);
        unitA.getResultUnit(i, predicateA, false, points, result[i]);
        unitB.getResultUnit(i, predicateB, inverseB, points, result[i]);
        result[i].finalize();
        t1 = t2;
        i++;
      }// while           
      return false;      
    }// operate
    
    void SourceUnitPair::createResultMRegion(MRegion* resMRegion){
      DbArray<MSegmentData>* array = 
        (DbArray<MSegmentData>*)resMRegion->GetFLOB(1);
      for(size_t i = 0; i < result.size(); i++){        
        if (result[i].size()!=0) {
          URegionEmb* ure = result[i].convertToURegionEmb(array);
          resMRegion->Add(*ure);
          delete ure;
        }// if
      }// for
    }// createResultMRegion
    
    size_t SourceUnitPair::countResultUnits()const{
      return result.size();
    }// countResultUnits
     
    ResultUnit SourceUnitPair::getResultUnit(size_t slide)const{
      return result[slide];
    }// getResultUnit 
    
    void SetOperator::operate(SetOp setOp){
      // Beide MRegionen müssen definiert sein
      if (!mRegionA->IsDefined() || !mRegionB->IsDefined()) {
        mRegionResult->SetDefined(false);
        return;
      }// if
      // Compute the RefinementPartition of the two MRegions
      RefinementPartition< MRegion, MRegion, URegionEmb, URegionEmb> 
        rp(*mRegionA, *mRegionB);
      //cout << "RefinementPartition with " << rp.Size() << " units created.";
      // MRegion des Ergebnisses löschen
      mRegionResult->Clear();
      // Speicherbereich der MSegmentData löschen
      ((DbArray<MSegmentData>*)mRegionResult->GetFLOB(1))->clean();
      // Füllvorgang beginnt
      mRegionResult->StartBulkLoad();
      // For each interval of the refinement partition
      for (unsigned int i = 0; i < rp.Size(); i++) {
        Interval<Instant> interval;
        int aPos, bPos;
        bool aIsEmpty, bIsEmpty;
        SourceUnitPair unitPair;
        // liefere den entsprechenden Eintrag
        rp.Get(i, interval, aPos, bPos);        
        // Bestimmen ob eine der beiden zu erzuegenden Units leer ist  
        aIsEmpty = (aPos == -1);
        bIsEmpty = (bPos == -1);
        // Vereinfachungen suchen     
        if (!aIsEmpty) {
          createSourceUnit(interval, mRegionA, UNIT_A, unitPair);
       }// if
        if (!bIsEmpty) {
          createSourceUnit(interval, mRegionB, UNIT_B, unitPair);
        }// if
        unitPair.operate(setOp);
        // cout << unitPair;        
        unitPair.createResultMRegion(mRegionResult);        
      }// for
      mRegionResult->EndBulkLoad(false);
    }// operate  
    
    void SetOperator::createSourceUnit(const Interval<Instant>& interval, 
                          MRegion* mregion,
                          SourceFlag sourceFlag, 
                          SourceUnitPair& unitPair){
        MRegion* temp;
        URegionEmb unitRestrict;
        URegionEmb* resultUnit;
        const DbArray<MSegmentData>* array;     

        Periods intervalAsPeriod(1);
        intervalAsPeriod.Add(interval);        
        temp = new MRegion(1);
        temp->AtPeriods(&intervalAsPeriod, mregion);
        temp->Get(0, unitRestrict);        
        array = temp->GetMSegmentData();      
        resultUnit = new URegionEmb(unitRestrict.timeInterval,
                                    unitRestrict.GetStartPos());
        resultUnit->SetSegmentsNum(unitRestrict.GetSegmentsNum());
        resultUnit->SetBBox(unitRestrict.BoundingBox());
        MSegmentData segment;
        for(int i = 0; i < resultUnit->GetSegmentsNum();i++){
          resultUnit->GetSegment(array, i, segment);
          addPFace(segment,interval,sourceFlag,unitPair);
        }// for  
        delete temp;
        delete resultUnit;        
    }// CreateSourceUnit
      
    void SetOperator::addPFace(const MSegmentData& mSeg, 
                               const Interval<Instant>& interval,
                               SourceFlag sourceFlag, 
                               SourceUnitPair& unitPair){
      Point2D start;
      Point2D end;
      Point3D leftStart;
      Point3D leftEnd;
      Point3D rightStart;
      Point3D rightEnd;       
      double startTime = interval.start.ToDouble();
      double endTime   = interval.end.ToDouble();
      // Fallen die Initialpunkte zusammen
      if (!mSeg.GetPointInitial()) {
        // Start und Endpunkt am initialen Sgement bestimmen
        start = Point2D(mSeg.GetInitialStartX(), mSeg.GetInitialStartY());
        end = Point2D(mSeg.GetInitialEndX(), mSeg.GetInitialEndY());        
      }// if
      else {
        // Start und Endpunkt an dem finalen Segment bestimmen
        start = Point2D(mSeg.GetFinalStartX(), mSeg.GetFinalStartY());
        end = Point2D(mSeg.GetFinalEndX(), mSeg.GetFinalEndY());
      }// else
      // Startpunkt A festlegen
      if ((start < end) == mSeg.GetInsideAbove()){                     
        leftStart  = Point3D(mSeg.GetInitialStartX(), mSeg.GetInitialStartY(),
                             startTime);
        rightStart = Point3D(mSeg.GetInitialEndX(), mSeg.GetInitialEndY(),
                             startTime);
        leftEnd    = Point3D(mSeg.GetFinalStartX(), mSeg.GetFinalStartY(),
                             endTime);
        rightEnd   = Point3D(mSeg.GetFinalEndX(), mSeg.GetFinalEndY(),
                             endTime);
      }// if
      else {
        leftStart  = Point3D(mSeg.GetInitialEndX(), mSeg.GetInitialEndY(),
                             startTime);
        rightStart = Point3D(mSeg.GetInitialStartX(), mSeg.GetInitialStartY(),
                             startTime);
        leftEnd    = Point3D(mSeg.GetFinalEndX(), mSeg.GetFinalEndY(),
                             endTime);
        rightEnd   = Point3D(mSeg.GetFinalStartX(), mSeg.GetFinalStartY(),
                             endTime);        
      }// else 
      Segment3D left(leftStart, leftEnd);
      Segment3D right(rightStart, rightEnd);
      unitPair.addPFace(sourceFlag, left, right);
    }// Create PFace

  } // end of namespace mregionops3
} // end of namespace temporalalgebra