/*
----
This file is part of SECONDO.

Copyright (C) 2014,
Faculty of Mathematics and Computer Science,
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

This file provides auxiliary function dealing with precise halfsegments.

*/


#ifndef HSTOOLS_H
#define HSTOOLS_H

#include <vector>
#include "PreciseHalfSegment.h"
#include "AvlTree.h"

namespace hstools{



enum SETOP {UNION, INTERSECTION, DIFFERENCE};


/*
~isSorted~

Checks whether ~v~ is sorted according to the halfsegment order.

*/
  bool isSorted(const std::vector<MPrecHalfSegment>& v);

/*
~sort~

Sorts ~v~ using the halfsegment order.

*/
  void sort(std::vector<MPrecHalfSegment>& v);


/*
~isLogicalSorted~

Checks whether ~v~ is sorted using the logical order (faceno, cycleno, edgeno) 

*/
   bool isLogicalSorted(const std::vector<MPrecHalfSegment>& v);


/*
~sortLogical~

Sorts ~v~ according to the logical order of halfsegments 
(faceno, cycleno, edgeno)

*/

  void sortLogical(std::vector<MPrecHalfSegment>& v);


/*
~setCoverage~

Sets the coverage number of a set of halfsegments. This number is used 
in the accerelated computation of the point in region algorithm. The argument 
v must be sorted according to the halfsegment order.

*/
   void setCoverage(std::vector<MPrecHalfSegment>& v);


/*
~setPartnerNumbers~

This function sets the partner number of the contained halfsegments. 
Partners must have the same edge number. Non-partners must have different
edge numbers. The edges must be numbered from 0 to n-1 where n is the 
number of contained segments ( v.size()/2).

*/

   bool setPartnerNumbers(std::vector<MPrecHalfSegment>& v);


/*
~checkRealm~

This function checks whether the segments contained in v are realminized. 
This means, two different segments in ~v~ have no common point except end 
points. The halfsegments in ~v~ have to be sorted in halfsegment order.

*/
   bool checkRealm(const std::vector<MPrecHalfSegment>& v);


/*
~realminize~

The function computes a realminized version of ~v~ and stores it in ~res~.
~v~ has to be sorted according to the halfsegment order.

*/

   void realminize(const std::vector<MPrecHalfSegment>& v, 
                   std::vector<MPrecHalfSegment>& res);





/*
~checkCycles~

This funtion checks whether ~v~ consists of cycles only, i.e. whether each 
dominating point is reached by an even number of halfsegments. ~v~ has to 
be sorted according to the halfsegment order.

*/
   bool checkCycles(const std::vector<MPrecHalfSegment>& v);


/*
~removeConnections~

This function will copy all halfsegments of ~v~ belonging to cycles into ~res~.
v has to be sorted in halfsegment order.

*/
   void removeConnections(const std::vector<MPrecHalfSegment>& v, 
                          std::vector<MPrecHalfSegment>& res);


/*
~setInsideAbove~

This function will compute and set the insideAbove flag for each 
halfsegment in ~v~. 
~v~ has to be sorted in halfsegment order.

*/

  void setInsideAbove(std::vector<MPrecHalfSegment>& v);


/*
~computeCycles~

This function computes the faceno, cycleno, edgeno for each halfsegment in ~v~.
The halfsegments in v has to be sorted in halfsegment order , has to be 
realminized, and has to build only cycles.

*/

  void computeCycles(std::vector<MPrecHalfSegment>& v);


/*
~Operator << ~

*/

std::ostream& operator<<(std::ostream& o, 
                          const std::vector<MPrecHalfSegment>& v);


/*
~checkRealm~

This function checks whether the segments contained in v are realminized. 
This means, two different segments in ~v~ have no common point except 
end points. The halfsegments in ~v~ have to be sorted in halfsegment order.

*/
   template<class HS> 
   class YComparator{
     public:
        static bool smaller(const HS& hs1, 
                            const HS& hs2){
           return compare(hs1,hs2)<0;
        }
        static bool equal(const HS& hs1, 
                          const HS& hs2){
           return compare(hs1,hs2)==0;
        }

        /** the less operator **/
        bool operator()(const HS& hs1, 
                        const HS& hs2){
           return compare(hs1,hs2)<0;
        }
     
      private: 
        static int compare(const HS& hs1, 
                           const HS& hs2){

            if(hs1.getMinY() > hs2.getMaxY()){
                return 1;
            }
            if(hs2.getMinY() > hs1.getMaxY()){
               return -1;
            }

            MPrecCoordinate x0(0);

            if(hs1.isVertical()){
               x0 = hs1.getLeftPoint().getX();
            } else if(hs2.isVertical()){
               x0 = hs2.getLeftPoint().getX();
            } else {
               x0 = std::max(hs1.getLeftPoint().getX(), 
                             hs2.getLeftPoint().getX());
            }

            MPrecCoordinate y1 = hs1.getY(x0);
            MPrecCoordinate y2 = hs2.getY(x0);
            int cmp = y1.compare(y2);
            if(cmp!=0){
               return cmp;
            }
            if(hs1.isVertical()){
               return hs2.isVertical()?0:1;
            }
            if(hs2.isVertical()){
              return -1;
            }
            x0 = std::min(hs1.getRightPoint().getX(), 
                          hs2.getRightPoint().getX());
            y1 = hs1.getY(x0);
            y2 = hs2.getY(x0);             
            return y1.compare(y2);
 
            //return hs1.compareSlope(hs2);
        }
   };

/*
~setOP~

This function computes the set operation specified by the last argument

*/
//  template<class S1, class S2>



  template< class S1, class S2>
  class EventStructure{
    public: 
     EventStructure(const S1* _v1,
                    const S2* _v2) : 
                          v1(_v1), v2(_v2), pos1(0), pos2(0),
                          pq1(), pq2(),pqb() {

         memset(present,0, 5*sizeof(bool)); // nothing load before
     }

     ~EventStructure(){
      }
     
     int  next(MPrecHalfSegment& result) {

         // get top values for all partially structures
         for(int i=0;i<5;i++){
           if(!present[i]){
              switch(i){
                 case 0: if(pos1<v1->size()){
                            topElements[0] = (*v1)[pos1];
                            present[0] = true;
                          } break;
                 case 1: if(pos2<v2->size()){
                            topElements[1] = (*v2)[pos1];
                            present[1] = true;
                         } break;
                 case 2: if(!pq1.empty()){
                            topElements[2] = pq1.top();
                            present[2] = true;
                         } break;
                 case 3: if(!pq2.empty()){
                            topElements[3] = pq2.top();
                            present[3] = true;
                         } break;
                 case 4: if(!pqb.empty()){
                            topElements[4] = pqb.top();
                            present[4] = true;
                         } break;
              }
           }
         }
         // search the minimum
         int index = -1;  // indicates that no minimum is present

         for(int i=0;i<5;i++){
            if(present[i]){
               if(index<0){
                   index = i;
               } else  if(cmp(topElements[i],topElements[index]) < 0 ){
                   index = i;
               }
            }
         }
 
         switch(index){
           case -1 : return 0; // no more halfsegments
           case  0 : result.set(FIRST, topElements[0]); 
                     pos1++; 
                     present[0] = false; 
                     return 1;
           case  1 : result.set(SECOND, topElements[1]); 
                     pos2++;
                     present[1] = false; 
                     return 2;
           case  2 : result.set(FIRST, topElements[2]);
                     pq1.pop(); 
                     present[2] = false;
                     return 1;
           case  3 : result.set(SECOND, topElements[3]);
                     pq2.pop(); 
                     present[3] = false;
                     return 2;
           case  4 : result.set(BOTH, topElements[4]);
                     pqb.pop(); 
                     present[4] = false;
                     return 3;
         }; 
         return -1;
     }


     void push(const MPrecHalfSegment& evt){
         switch(evt.getOwner()){
           case FIRST  : pq1.push(evt); break;
           case SECOND : pq2.push(evt); break;
           case BOTH   : pqb.push(evt); break;
           default : assert(false);
         }
     }

     size_t size() const{
       return (v1->size()-pos1) + (v2->size()-pos2) + 
               pq1.size() + pq2.size() + pqb.size();
     }


    private:
       const S1* v1;
       const S2* v2;
       size_t pos1;
       size_t pos2;
       std::priority_queue<MPrecHalfSegment, 
                      std::vector<MPrecHalfSegment>, 
                      IsGreater<MPrecHalfSegment, HalfSegmentComparator> > pq1;
       std::priority_queue<MPrecHalfSegment, 
                      std::vector<MPrecHalfSegment>, 
                      IsGreater<MPrecHalfSegment, HalfSegmentComparator> > pq2;
       std::priority_queue<MPrecHalfSegment, 
                      std::vector<MPrecHalfSegment>, 
                      IsGreater<MPrecHalfSegment, HalfSegmentComparator> > pqb;
       HalfSegmentComparator cmp;

       MPrecHalfSegment topElements[5];
       bool present[5];

  };


  template<>
  class EventStructure<std::vector<MPrecHalfSegment>, 
                       std::vector<MPrecHalfSegment> >{
    public: 
     EventStructure(const std::vector<MPrecHalfSegment>* _v1,
                    const std::vector<MPrecHalfSegment>* _v2) : 
                          v1(_v1), v2(_v2), pos1(0), pos2(0),
                          pq1(), pq2(),pqb() {
        tmpCand = new MPrecHalfSegment[5];
     }

     ~EventStructure(){
         delete[] tmpCand;
      }
     
     int  next(MPrecHalfSegment& result) {
         const MPrecHalfSegment* candidates[5];
         candidates[0] = pos1<v1->size()?&((*v1)[pos1]):0;
         candidates[1] = pos2<v2->size()?&((*v2)[pos2]):0;
         candidates[2] = pq1.empty()?0:&pq1.top();
         candidates[3] = pq2.empty()?0:&pq2.top();
         candidates[4] = pqb.empty()?0:&pqb.top();
         int index = -1;
         for(int i=0;i<5;i++){
            if(candidates[i]){
               if(index<0){
                   index = i;
               } else  if(cmp(*candidates[i],*candidates[index]) < 0 ){
                   index = i;
               }
            }
         }
 
         switch(index){
           case -1 : return 0; // no more halfsegments
           case  0 : result.set(FIRST, *(candidates[0]));pos1++; return 1;
           case  1 : result.set(SECOND, *(candidates[1]));pos2++; return 2;
           case  2 : result.set(FIRST, *candidates[2]);pq1.pop(); return 1;
           case  3 : result.set(SECOND, *candidates[3]);pq2.pop(); return 2;
           case  4 : result.set(BOTH, *candidates[4]);pqb.pop(); return 3;
         }; 
         return -1;
     }


     void push(const MPrecHalfSegment& evt){
         switch(evt.getOwner()){
           case FIRST  : pq1.push(evt); break;
           case SECOND : pq2.push(evt); break;
           case BOTH   : pqb.push(evt); break;
           default : assert(false);
         }
     }

     size_t size() const{
       return (v1->size()-pos1) + (v2->size()-pos2) + 
               pq1.size() + pq2.size() + pqb.size();
     }


    private:
       const std::vector<MPrecHalfSegment>* v1;
       const std::vector<MPrecHalfSegment>* v2;
       size_t pos1;
       size_t pos2;
       std::priority_queue<MPrecHalfSegment, 
                      std::vector<MPrecHalfSegment>, 
                      IsGreater<MPrecHalfSegment, HalfSegmentComparator> > pq1;
       std::priority_queue<MPrecHalfSegment, 
                      std::vector<MPrecHalfSegment>, 
                      IsGreater<MPrecHalfSegment, HalfSegmentComparator> > pq2;
       std::priority_queue<MPrecHalfSegment, 
                      std::vector<MPrecHalfSegment>, 
                      IsGreater<MPrecHalfSegment, HalfSegmentComparator> > pqb;
       HalfSegmentComparator cmp;
       MPrecHalfSegment* tmpCand;

  };

void makeRealm(const MPrecHalfSegment& hs1, const MPrecHalfSegment& hs2, 
              std::vector<MPrecHalfSegment>& res); 


void setOP(const std::vector<MPrecHalfSegment>& v1,
             const std::vector<MPrecHalfSegment>& v2,
             std::vector<MPrecHalfSegment>& res,
             SETOP op);




} // end of namespace hstools

#endif


