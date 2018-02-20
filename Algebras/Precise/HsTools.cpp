
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


#include "HsTools.h"
#include "AvlTree.h"
#include "PreciseHalfSegment.h"

#include <algorithm>

using namespace std;


namespace hstools{


/*
~isSorted~

Checks whether ~v~ is sorted according to the halfsegment order.

*/
  bool isSorted(const vector<MPrecHalfSegment>& v){
     if(v.size()<2){
       return true;
     }
     HalfSegmentComparator cmp;
     for(size_t i=0;i<v.size()-1;i++){
       if( cmp(v[i],v[i+1]) >0){
          return false;
       } 
     }
     return true;
  }

/*
~sort~

Sorts ~v~ using the halfsegment order.

*/
  void sort(vector<MPrecHalfSegment>& v){
     IsSmaller<MPrecHalfSegment, HalfSegmentComparator> is;
     ::sort(v.begin(),v.end(),is);
  }


/*
~isLogicalSorted~

Checks whether ~v~ is sorted using the logical order (faceno, cycleno, edgeno) 

*/
   bool isLogicalSorted(const vector<MPrecHalfSegment>& v){
     if(v.size()<2){
       return true;
     }
     LogicalHalfSegmentComparator cmp;
     for(size_t i=0;i<v.size()-1;i++){
       if( cmp(v[i],v[i+1]) >0){
          return false;
       } 
     }
     return true;
   }


/*
~sortLogical~

Sorts ~v~ according to the logical order of halfsegments 
(faceno, cycleno, edgeno) 

*/

  void sortLogical(vector<MPrecHalfSegment>& v){
     IsSmaller<MPrecHalfSegment, LogicalHalfSegmentComparator> is;
     ::sort(v.begin(),v.end(),is);
  }


/*
~setCoverage~

Sets the coverage number of a set of halfsegments. This number is used 
in the accerelated computation of the point in region algorithm. The 
argument v must be sorted according to the halfsegment order.

*/
   void setCoverage(vector<MPrecHalfSegment>& v){
      int currCoverageNo = 0;
      for( size_t i = 0; i < v.size(); i++ ) {
        if( v[i].isLeftDomPoint() ){
          currCoverageNo++;
        } else {
          currCoverageNo--;
        }
        v[i].attributes.coverageno = currCoverageNo;
      }
   }


/*
~setPartnerNumbers~

This function sets the partner number of the contained halfsegments. 
Partners must have the same edge number. Non-partners must have different
edge numbers. The edges must be numbered from 0 to n-1 where n is the 
number of contained segments ( v.size()/2).

*/

   bool setPartnerNumbers(vector<MPrecHalfSegment>& v){
     size_t s = v.size();
     if((s & 1u) > 0u){ // odd number of halfsegments
       return false;
     }
     int s2 = s/2;
     int* tmp = new int[s/2];

     for(size_t i=0;i<s;i++){
        if(v[i].isLeftDomPoint()){
           int en = v[i].attributes.edgeno;
           if(en>=s2){
             return false;
           }
           tmp[en] = i;
        } else {
           int en = v[i].attributes.edgeno;
           if(en>=s2){
             return false;
           }
           int pn = tmp[en];
           v[i].attributes.partnerno = pn;
           v[en].attributes.partnerno = i;
        }
     }
     delete[] tmp;  
     return true;
   }




   bool checkRealm(const vector<MPrecHalfSegment>& v){
      avltree::AVLTree<MPrecHalfSegment, YComparator<MPrecHalfSegment> > sss;
      MPrecHalfSegment * left;
      MPrecHalfSegment * right;
      for(unsigned int i=0;i<v.size();i++){
         const MPrecHalfSegment current = v[i];
         if(current.isLeftDomPoint()){
            if(!sss.insertN(current,left,right)){
               return false;
            }
            if((left!=0) && !left->checkRealm(current)){
               return false;
            }
            if(right && !right->checkRealm(current)){
               return false;
            }
         }  else { // right endpoint
            if(!sss.removeN(current,left,right)){
                return false;
            } 
            if(left && right && !left->checkRealm(*right)){
                return false;
            }
         }
      }
      return true;
   }


ostream& print(const vector<MPrecHalfSegment>& v, ostream& o){

  for(size_t i=0;i<v.size();i++){
    o << v[i] << endl;
  }
  return o;

}

/*
Creates a realminized version of thwo halfsegments. 
For computing the coverage number of a common part, it's assumed, 
that the coverage number of the first segment (hs1) is already set
correctly

*/

void makeRealm(const MPrecHalfSegment& hs1, const MPrecHalfSegment& hs2, 
              vector<MPrecHalfSegment>& res) {

   HalfSegmentComparator cmp;    
   res.clear();
   // compute coverage for common part
   int coverage = hs1.attributes.coverageno;
   if(hs2.attributes.insideAbove){
      coverage++;
   } else {
      coverage--;
   }

   if(hs1.checkRealm(hs2)){ // hs1 and hs2 are already realminized
      if(cmp(hs1,hs2)<0){
          res.push_back(hs1);
          res.push_back(hs2); 
          return;
      } else {
          res.push_back(hs2); 
          res.push_back(hs1);
          return;
      }
   }

   if(hs1==hs2){ // exact equality
      res.push_back(hs1);
      res[0].setOwner(BOTH);
      res[0].attributes.coverageno = coverage;
      return;
   }

   if(hs1.compareSlope(hs2)!=0){ // crossing or touching halfsegments
      if(hs1.crosses(hs2)){ // crossing halfsegments
         MPrecPoint* cp = hs1.crossPoint(hs2);
         res.push_back(MPrecHalfSegment (hs1.getLeftPoint(),*cp,
                                         true,hs1.attributes, hs1.getOwner()));
         res[res.size()-1].attributes.coverageno = hs1.attributes.coverageno;
         res.push_back(MPrecHalfSegment (hs2.getLeftPoint(),*cp,
                                         true,hs2.attributes, hs2.getOwner()));
         res[res.size()-1].attributes.coverageno = hs2.attributes.coverageno;
         res.push_back(MPrecHalfSegment (*cp,hs1.getRightPoint(),
                                         true,hs1.attributes, hs1.getOwner()));
         res[res.size()-1].attributes.coverageno = hs1.attributes.coverageno;
         res.push_back(MPrecHalfSegment (*cp,hs2.getRightPoint(),
                                         true,hs2.attributes, hs2.getOwner()));
         res[res.size()-1].attributes.coverageno = hs2.attributes.coverageno;
         delete cp; 
      } else { // touching halfsegments
         if(hs1.innerContains(hs2.getLeftPoint())){
            res.push_back(MPrecHalfSegment(hs1.getLeftPoint(), 
                                       hs2.getLeftPoint(), 
                                       true, hs1.attributes, hs1.getOwner()));
            res[res.size()-1].attributes.coverageno = hs1.attributes.coverageno;
            res.push_back(MPrecHalfSegment(hs2.getLeftPoint(), 
                                       hs1.getRightPoint(), 
                                       true, hs1.attributes, hs2.getOwner()));
            res[res.size()-1].attributes.coverageno = hs1.attributes.coverageno;
            res.push_back(hs2); 
         } else if(hs1.innerContains(hs2.getRightPoint())){
            res.push_back(MPrecHalfSegment(hs1.getLeftPoint(), 
                                       hs2.getRightPoint(), 
                                       true, hs1.attributes, hs1.getOwner()));
            res[res.size()-1].attributes.coverageno = hs1.attributes.coverageno;
            res.push_back(MPrecHalfSegment(hs2.getRightPoint(), 
                                       hs1.getRightPoint(), 
                                       true, hs1.attributes, hs1.getOwner()));
            res[res.size()-1].attributes.coverageno = hs1.attributes.coverageno;
            res.push_back(hs2); 
         } else if(hs2.innerContains(hs1.getLeftPoint())){
            res.push_back(MPrecHalfSegment(hs2.getLeftPoint(), 
                                       hs1.getLeftPoint(), 
                                       true, hs2.attributes, hs2.getOwner()));
            res[res.size()-1].attributes.coverageno = hs2.attributes.coverageno;
            res.push_back(MPrecHalfSegment(hs1.getLeftPoint(), 
                                       hs2.getRightPoint(), 
                                       true, hs2.attributes, hs2.getOwner()));
            res[res.size()-1].attributes.coverageno = hs2.attributes.coverageno;
            res.push_back(hs1);
         } else if(hs2.innerContains(hs1.getRightPoint())){
            res.push_back(MPrecHalfSegment(hs2.getLeftPoint(), 
                                       hs1.getRightPoint(),
                                       true, hs2.attributes, hs2.getOwner()));
            res[res.size()-1].attributes.coverageno = hs2.attributes.coverageno;
            res.push_back(MPrecHalfSegment(hs1.getRightPoint(), 
                                       hs2.getRightPoint(),
                                       true, hs2.attributes, hs2.getOwner()));
            res[res.size()-1].attributes.coverageno = hs2.attributes.coverageno;
            res.push_back(hs1);
         } else {
             throw 1;  // some error in computation or some forgotten case
         }
      }
   } else { // overlapping halfsegments
      // coverage number for common part

      int lcmp = hs1.getLeftPoint().compareTo(hs2.getLeftPoint());
      int rcmp = hs1.getRightPoint().compareTo(hs2.getRightPoint());
      // first part, single piece of hs1 or hs2
      MPrecPoint firstCommonPoint(0,0);
      if(lcmp<0){ // first part belongs to hs1
         res.push_back(MPrecHalfSegment(
                       hs1.getLeftPoint(), hs2.getLeftPoint(),
                       true, hs1.attributes, hs1.getOwner()));
         res[res.size()-1].attributes.coverageno = hs1.attributes.coverageno;
          firstCommonPoint = hs2.getLeftPoint();
      } else if(lcmp>0) { // first part belongs to hs2
         res.push_back(MPrecHalfSegment(
                       hs2.getLeftPoint(), hs1.getLeftPoint(),
                       true, hs2.attributes, hs2.getOwner()));
          res[res.size()-1].attributes.coverageno = hs2.attributes.coverageno;
          firstCommonPoint = hs1.getLeftPoint();
      } else { // lcmp = 0 : both segments starts at the same point
          firstCommonPoint = hs1.getLeftPoint();
      }

      // the common part and the last part
      if(rcmp==0){ // segments ends at the same point
         res.push_back(MPrecHalfSegment(
                       firstCommonPoint, hs1.getRightPoint(),
                       true, hs1.attributes, BOTH));
         res[res.size()-1].attributes.coverageno = coverage;
         res[res.size()-1].attributes.insideAbove = hs1.attributes.insideAbove;
      } else if(rcmp < 0){
          // last part belongs to hs2
         res.push_back(MPrecHalfSegment(
                       firstCommonPoint, hs1.getRightPoint(),
                       true, hs1.attributes, BOTH));
         res[res.size()-1].attributes.coverageno = coverage;
         res[res.size()-1].attributes.insideAbove = hs1.attributes.insideAbove;
         res.push_back(MPrecHalfSegment(
                       hs1.getRightPoint(), hs2.getRightPoint(),
                       true, hs2.attributes,hs2.getOwner()));
          res[res.size()-1].attributes.coverageno = hs2.attributes.coverageno;
      } else { // rcmp > 0
          // last part belongs to hs1
         res.push_back(MPrecHalfSegment(
                       firstCommonPoint, hs2.getRightPoint(),
                       true, hs2.attributes, BOTH));
         res[res.size()-1].attributes.coverageno = coverage;
         res[res.size()-1].attributes.insideAbove = hs1.attributes.insideAbove;
         res.push_back(MPrecHalfSegment(
                       hs2.getRightPoint(), hs1.getRightPoint(),
                       true, hs1.attributes,hs1.getOwner()));
         res[res.size()-1].attributes.coverageno = hs1.attributes.coverageno;
      }
   } 
   // sort
   IsSmaller<MPrecHalfSegment, HalfSegmentComparator> is;
   sort(res.begin(),res.end(),is);
}

   

/*
~realminize~

The function computes a realminized version of ~v~ and stores it in ~res~.
~v~ has to be sorted according to the halfsegment order.

*/

   void realminize(const vector<MPrecHalfSegment>& v, 
                   vector<MPrecHalfSegment>& res){


     avltree::AVLTree<MPrecHalfSegment, YComparator<MPrecHalfSegment> > sss;

     priority_queue<MPrecHalfSegment, 
                     vector<MPrecHalfSegment>, 
                     IsGreater<MPrecHalfSegment, HalfSegmentComparator> > es;

      MPrecHalfSegment * left;
      MPrecHalfSegment * right;
      res.clear();

      HalfSegmentComparator cmp;

      vector<MPrecHalfSegment> realmRes;

      bool done = v.size()==0;
      size_t pos = 0;
      AttrType dummy(1);
      MPrecHalfSegment current(MPrecPoint(0,0),MPrecPoint(1,1), true, dummy);

      int edgeno = 0;

      MPrecCoordinate currentX(0);
      MPrecCoordinate lastX(0);
      bool first = true;

      while(!done){
       if(pos<v.size()){ 
           if(es.empty()){ // no elements in es
              current = v[pos];
              pos++;
           } else { // v and es have elements
              if(cmp(v[pos],es.top())<0){ // use original vector
                 current = v[pos];
                 pos++;
              } else {
                 current = es.top();
                 es.pop();
              }
           }
       } else { // original vector exhausted
           current = es.top();
           es.pop();
       }

       if(first){
         lastX = current.getDomPoint().getX();
         first = false; 
       } else {
         currentX = current.getDomPoint().getX();
         assert(lastX<=currentX);
         lastX = currentX;
       }

        if(current.isLeftDomPoint()){
           MPrecHalfSegment* stored = sss.getMember(current);
           if(stored!=0){ // overlapping segment found
               makeRealm(current,*stored,realmRes);
               sss.remove(*stored);
               sss.remove(current);
               sss.insert(realmRes[0]); // shorten 
               realmRes[0].setLDP(false);
               es.push(realmRes[0]);
               for(size_t i=1;i<realmRes.size();i++){
                  es.push(realmRes[i]);
                  realmRes[i].setLDP(false);
                  es.push(realmRes[i]);
               }
           } else {
              sss.insertN(current,left,right);
              if(left && !left->checkRealm(current)){
                 makeRealm(current,*left,realmRes);
                 sss.remove(*left);
                 sss.remove(current);
                 sss.insert(realmRes[0]);
                 sss.insert(realmRes[1]);
                 realmRes[0].setLDP(false);
                 realmRes[1].setLDP(false);
                 es.push(realmRes[0]);
                 es.push(realmRes[1]);
                 for(size_t i=2;i<realmRes.size();i++){
                   es.push(realmRes[i]);
                   realmRes[i].setLDP(false);
                   es.push(realmRes[i]);
                 } 
              } 
              if( right && !right->checkRealm(current)){
                 makeRealm(current,*right,realmRes);
                 assert(sss.remove(*right));
                 assert(sss.remove(current));

                 sss.insert(realmRes[0]);
                 sss.insert(realmRes[1]);
                 realmRes[0].setLDP(false);
                 realmRes[1].setLDP(false);
                 es.push(realmRes[0]);
                 es.push(realmRes[1]);
                 for(size_t i=2;i<realmRes.size();i++){
                   es.push(realmRes[i]);
                   realmRes[i].setLDP(false);
                   es.push(realmRes[i]);
                 } 
              }
           }
        } else {
           const MPrecHalfSegment* stored = sss.getMember(current);
           if(stored!=0){
               // cout << "stored" << endl;
               if(current.sameGeometry(*stored)){
                  // cout << "same geom" << endl;
                  sss.removeN(current,left,right);
                  // cout << "removeN finished" << endl;
                  current.attributes.edgeno = edgeno;
                  edgeno++;
                  current.setLDP(true);
                  MPrecHalfSegment copy(current);
                  res.push_back(copy);
                  MPrecHalfSegment copy2(copy);
                  copy2.setLDP(false);
                  res.push_back(copy2);
                  // cout << "check neighbors" << endl;
                  if(left && right && !left->checkRealm(*right)){
                     // cout << "realm neighbors" << endl;
                     makeRealm(*left, *right, realmRes);
                     sss.remove(*left);
                     sss.remove(*right);
                     sss.insert(realmRes[0]);
                     sss.insert(realmRes[1]);
                     realmRes[0].setLDP(false);
                     realmRes[1].setLDP(false);
                     es.push(realmRes[0]);
                     es.push(realmRes[1]);
                     for(size_t i=2;i<realmRes.size();i++){
                        es.push(realmRes[i]);
                        realmRes[i].setLDP(false);
                        es.push(realmRes[i]);
                     } 
                  } else {
                   //  cout << "No realm of neighbors required" << endl;
                  }
              } else {
                 // cout << "Other geom" << endl;
               }
           } else { // stored == 0
              //cerr << "size of avl = " << sss.Size() << endl;
              //cerr << "content " << sss << endl;
           }
        }        
        done = (pos==v.size() && es.empty());
      }
      // sort result vector

      sort(res);
   }



/*
~setOP~

This function computes the set operation specified by the last argument

*/
  void setOP(const vector<MPrecHalfSegment>& v1,
             const vector<MPrecHalfSegment>& v2,
             vector<MPrecHalfSegment>& res,
             SETOP op) {

     avltree::AVLTree<MPrecHalfSegment, YComparator<MPrecHalfSegment> > sss;
     EventStructure<vector<MPrecHalfSegment>, 
                    vector<MPrecHalfSegment> >  es(&v1,&v2);
     AttrType dummy;
     MPrecHalfSegment current (MPrecPoint(0,0), MPrecPoint(1,1), true, dummy);
     int source;
     MPrecHalfSegment * left;
     MPrecHalfSegment * right;
     res.clear();
     vector<MPrecHalfSegment> realmRes;
     int edgeno = 0;

     while((source = es.next(current))){
        if(current.isLeftDomPoint()) {
          MPrecHalfSegment* stored = sss.getMember(current);  
          if(stored != 0){ // found overlapping part 
             assert(stored->getOwner()!=BOTH);
             assert(stored->getOwner()!=current.getOwner());
             if(current.getOwner()==FIRST){
                 makeRealm(current,*stored,realmRes);
             } else {
                 makeRealm(*stored, current, realmRes);
             }
             sss.remove(*stored);
             sss.remove(current);
             sss.insert(realmRes[0]); // shorten 
             realmRes[0].setLDP(false);
             es.push(realmRes[0]);
             for(size_t i=1;i<realmRes.size();i++){
                es.push(realmRes[i]);
                realmRes[i].setLDP(false);
                es.push(realmRes[i]);
             }
          }  else { // new Element
              sss.insertN(current,left,right);
              if(left && !left->checkRealm(current)){
                 makeRealm(current,*left,realmRes);
                 sss.remove(*left);
                 sss.remove(current);
                 sss.insert(realmRes[0]);
                 sss.insert(realmRes[1]);
                 realmRes[0].setLDP(false);
                 realmRes[1].setLDP(false);
                 es.push(realmRes[0]);
                 es.push(realmRes[1]);
                 for(size_t i=2;i<realmRes.size();i++){
                   es.push(realmRes[i]);
                   realmRes[i].setLDP(false);
                   es.push(realmRes[i]);
                 } 
              } 
              if( right && !right->checkRealm(current)){
                 makeRealm(current,*right,realmRes);
                 assert(sss.remove(*right));
                 assert(sss.remove(current));
                 sss.insert(realmRes[0]);
                 sss.insert(realmRes[1]);
                 realmRes[0].setLDP(false);
                 realmRes[1].setLDP(false);
                 es.push(realmRes[0]);
                 es.push(realmRes[1]);
                 for(size_t i=2;i<realmRes.size();i++){
                   es.push(realmRes[i]);
                   realmRes[i].setLDP(false);
                   es.push(realmRes[i]);
                 } 
              }
           }
        } else {  
           const MPrecHalfSegment* stored = sss.getMember(current);
           if(stored!=0){
               if(current.sameGeometry(*stored)){
                  MPrecHalfSegment copy(*stored);
                  MPrecHalfSegment copy2(copy);
                  copy2.setLDP(false);

                  sss.removeN(current,left,right);
                  current.attributes.edgeno = edgeno;
                  edgeno++;
                  current.setLDP(true);
                  switch(op){
                      case UNION : res.push_back(copy);
                              res.push_back(copy2);
                              break;
                      case INTERSECTION: if(copy.getOwner()==BOTH){
                                      res.push_back(copy);
                                      res.push_back(copy2);
                                    }
                                    break;
                      case DIFFERENCE : if(copy.getOwner()==FIRST){
                                      res.push_back(copy);
                                      res.push_back(copy2);
                                   }
                  }
                  if(left && right && !left->checkRealm(*right)){
                     makeRealm(*left, *right, realmRes);
                     sss.remove(*left);
                     sss.remove(*right);
                     sss.insert(realmRes[0]);
                     sss.insert(realmRes[1]);
                     realmRes[0].setLDP(false);
                     realmRes[1].setLDP(false);
                     es.push(realmRes[0]);
                     es.push(realmRes[1]);
                     for(size_t i=2;i<realmRes.size();i++){
                        es.push(realmRes[i]);
                        realmRes[i].setLDP(false);
                        es.push(realmRes[i]);
                     } 
                  }
               } else {
               }
           } else { // stored == 0
              //cerr << "size of avl = " << sss.Size() << endl;
              //cerr << "content " << sss << endl;
           }
        }
     }
  }




/*
~checkCycles~

This funtion checks whether ~v~ consists of cycles only, i.e. 
whether each dominating point is reached by an even number of 
halfsegments. ~v~ has to be sorted according to the halfsegment
order.

*/
   bool checkCycles(const vector<MPrecHalfSegment>& v){
      if(v.empty()) {
         return true;
      }
      size_t num = 0;
      MPrecPoint lastDom(0,0);
      for(size_t pos =0;pos<v.size();pos++){
         MPrecPoint cur = v[pos].getDomPoint();
         if(pos==0u || cur != lastDom){
             if( (num & 1) != 0){
                return false;
             }
             lastDom = cur;
             num = 1;
         } else {
             num++;
         }
      }
      return (num & 1u)==0u;
   }


/*
~removeConnections~

This function will copy all halfsegments of ~v~ belonging to cycles into ~res~.
v has to be sorted in halfsegment order.

*/
   void removeConnections(const vector<MPrecHalfSegment>& v, 
                          vector<MPrecHalfSegment>& res);


/*
~setInsideAbove~

This function will compute and set the insideAbove flag for each 
halfsegment in ~v~. 
~v~ has to be sorted in halfsegment order.

*/

  void setInsideAbove(vector<MPrecHalfSegment>& v);


/*
~computeCycles~

This function computes the faceno, cycleno, edgeno for each halfsegment in ~v~.
The halfsegments in v has to be sorted in halfsegment order , has to be 
realminized, and has to build only cycles.

*/

  void computeCycles(vector<MPrecHalfSegment>& v);



  ostream& operator<<(ostream& o, const vector<MPrecHalfSegment>& v) {

    for(size_t i=0;i<v.size();i++){
      o << i << ":\t" << v[i] << endl;
    }
    return o;

  }


} // end of namespace hstools



