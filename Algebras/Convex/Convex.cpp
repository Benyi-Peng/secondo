/*
----
This file is part of SECONDO.

Copyright (C) 2019, 
University in Hagen, 
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

*/


#include "Convex.h"
#include "Attribute.h"
#include <set>
#include <iostream>
#include "NestedList.h"
#include "Algebras/Spatial/SpatialAlgebra.h"
#include <tuple>
#include <vector> 
#include <algorithm> 
#include "ListUtils.h"
#include "NList.h"
#include "StandardTypes.h"
#include "Stream.h"
#include "Algebras/FText/FTextAlgebra.h"
#include <cstdio>


#include <boost/polygon/voronoi.hpp>
using boost::polygon::voronoi_builder;
using boost::polygon::voronoi_diagram;
using boost::polygon::x;
using boost::polygon::y;
using boost::polygon::low;
using boost::polygon::high;
using boost::polygon::construct_voronoi;


/*
 
  1.1 boost definitions needed for constructing the voronoi diag
  
*/
  


namespace boost {
  namespace polygon {


    struct VoronoiPoint {
      double a;
      double b;
      VoronoiPoint(double x, double y) : a(x), b(y) {}
    };   
    

    template<>
    struct geometry_concept<VoronoiPoint>{typedef point_concept type;};

    template <>
    struct point_traits<VoronoiPoint> {
      typedef double coordinate_type;

      static inline coordinate_type get(
          const VoronoiPoint& point, orientation_2d orient) {
        return (orient == HORIZONTAL) ? point.a : point.b;
      }
    };



}}

using namespace std; 


extern NestedList* nl;
using boost::polygon::VoronoiPoint;


namespace convex { 

/*
 
   2.1 constructors
  
*/
    

Convex::Convex(const Convex& src):Attribute(src),value(0),size(src.size){
  if(size>0){
     value = new Point[size];
     memcpy((char*)value, src.value,size*sizeof(Point));
  }
}



Convex::Convex(Convex&& src): Attribute(src), 
   value(src.value), size(src.size) {
   src.size = 0;
   src.value = 0;
}

Convex::Convex(const std::vector<std::tuple 
                <double, double>>& src):Attribute(true), value(0){
   setTo(src);
}



Convex& Convex::operator=(const Convex& src) {
   Attribute::operator=(src);
   if(this->size != src.size){
      delete[] value;
      value = 0;
      this->size = src.size;
      if(size > 0){
        value = new Point[size];  
      }
   }
   if(size > 0){
     memcpy((char*)value, src.value,size*sizeof(Point));
   }
   return *this;
}





Convex& Convex::operator=(Convex&& src) {
   Attribute::operator=(src);
   size = src.size;
   value = src.value;
   src.size = 0;
   src.value = 0;
   return *this;
}





void Convex::setTo(const std::vector<std::tuple <double, double>>& src){
      
      clear();
      SetDefined(true);
      if(src.size()>0){
         size = src.size();
         value = new Point[size];
         auto it = src.begin();
         size_t pos = 0;
         std::tuple<double, double> temptup;
         double tempxval;
         double tempyval;
         Point poi;
            
    
         
         
     /* converting the right tuple vector into point sequence  */   
      
         while(it!=src.end()) {
           
         temptup = *it;
         tempxval = std::get<0>(temptup);
         tempyval = std::get<1>(temptup);
         
         poi.Set(true, tempxval, tempyval);
         value[pos] = poi;          
              
           it++;
           pos++;
         }
      }
   }







/*
 
  3.1 auxillary  functions
  
*/



bool sortyupxup(const tuple<double, double>& a,  
                   const tuple<double, double>& b) 
{ 
    
    if ( (get<1>(a) < get<1>(b)) ||
          ( ( get<1>(a) == get<1>(b)) && (get<0>(a) <= get<0>(b))) ) 
          return true;
     
     return false;   
    
} 


bool sortyupxdown(const tuple<double, double>& a,  
                   const tuple<double, double>& b) 
{ 
    
    if ( (get<1>(a) < get<1>(b)) ||
          ( ( get<1>(a) == get<1>(b)) && (get<0>(a) >= get<0>(b))) ) 
          return true;
     
     return false;
    
    
    
} 




bool sortxupyup(const tuple<double, double>& a,  
                const tuple<double, double>& b) 
{ 
     if ( (get<0>(a) < get<0>(b)) ||
          ( ( get<0>(a) == get<0>(b)) && (get<1>(a) <= get<1>(b))) ) 
          return true;
     
     return false;
         
} 








bool sortxdownydown (const tuple<double, double>& a,  
                const tuple<double, double>& b) 
{ 
     if ( (get<0>(a) > get<0>(b)) ||
          ( ( get<0>(a) == get<0>(b)) && (get<1>(a) >= get<1>(b))) ) 
          return true;
     
     return false;
         
} 


bool sortxdownyup (const tuple<double, double>& a,  
                const tuple<double, double>& b) 
{ 
     if ( (get<0>(a) > get<0>(b)) ||
          ( ( get<0>(a) == get<0>(b)) && (get<1>(a) <= get<1>(b))) ) 
          return true;
     
     return false;
         
} 

bool sortxupydown (const tuple<double, double>& a,  
                const tuple<double, double>& b) 
{ 
     if ( (get<0>(a) < get<0>(b)) ||
          ( ( get<0>(a) == get<0>(b)) && (get<1>(a) >= get<1>(b))) ) 
          return true;
     
     return false;
         
} 

/*
 
  3.2 checkme function
  
*/

bool checkme(std::vector<std::tuple <double, double>>& src){
      
      
   
   std::vector<std::tuple<double, double>> finalcheckedvec;
   std::vector<std::tuple<double, double>> tmp;
   
   std::vector<std::tuple <double, double>> xysortedvecasc;
   std::vector<std::tuple <double, double>> yxsortedvecasc;
   std::vector<std::tuple <double, double>> xysortedvecdesc;
   std::vector<std::tuple <double, double>> ydownsortedvec;
   
   
   std::tuple<double, double> leftpoint;
   std::tuple<double, double> rightpoint;
   std::tuple<double, double> downpoint;
   std::tuple<double, double> uppoint;  
   std::tuple<double, double> actpoint;  
   std::tuple<double, double> intermpoint;
   
   
   
   std::vector<std::tuple <double, double>> above;
   std::vector<std::tuple <double, double>> below;
   std::vector<std::tuple <double, double>> aboveleft;
   std::vector<std::tuple <double, double>> belowleft;
   std::vector<std::tuple <double, double>> aboveright;
   std::vector<std::tuple <double, double>> belowright;
   std::vector<std::tuple <double, double>> temp1, temp2;
   
   
   
   unsigned int iter, count, sectorflag;
   unsigned int  aboveleftsize;
   unsigned int  aboverightsize;
   unsigned int  belowleftsize;
   unsigned int  belowrightsize;
   unsigned int  tmpsize;
   
   bool okflag = true;
   bool firstperpend = false;
   bool secondperpend = false;
   bool setfperflag = false;
   bool firstmflag = false;
   
   double m, b, mlast, mnew;
   
   
   
   if (src.size() <= 2) {
       return false;
       
     }
       
       
      
   tmp = src;
 
   xysortedvecasc = tmp;
   yxsortedvecasc = tmp;
   
   
  
    

/* sorting vecs by x resp. y coord */ 


sort(xysortedvecasc.begin(), xysortedvecasc.end(), sortxupyup); 
sort(yxsortedvecasc.begin(), yxsortedvecasc.end(), sortyupxdown); 
       
       
         
    
 
/* eliminate redundant points */
       
       count = 0;
       
    
    for (unsigned int i = 0; i < xysortedvecasc.size() - 1; i++)
       { if ( (get<0>(xysortedvecasc[i]) == get<0>(xysortedvecasc[i+1]))&&
              (get<1>(xysortedvecasc[i]) == get<1>(xysortedvecasc[i+1]))) { 
          
           xysortedvecasc.erase(xysortedvecasc.begin()+count);
           
         }
         count++;
         }
    
       
    if ( xysortedvecasc.size() <= 2) {
       return false;
       
     }
       
       
       
       
   /* get leftpoint and right point of polygon  */


    leftpoint = xysortedvecasc[0];
    rightpoint = xysortedvecasc[xysortedvecasc.size() - 1];
    downpoint = yxsortedvecasc[0];
    uppoint = yxsortedvecasc[yxsortedvecasc.size() - 1];
   
   
    
   
   /* get points above and below the imagenary "line" 
    from leftpoint to rightpoint

   */   
    
    m = (get<1>(rightpoint) - get<1>(leftpoint)) / 
        (get<0>(rightpoint) - get<0>(leftpoint));
    
    b =  get<1>(rightpoint) - (m * get<0>(rightpoint));
    
    
   
    
    
    for (unsigned int i = 0; i < xysortedvecasc.size(); i++)  {
        
        
        if ( ((m * get<0>(xysortedvecasc[i])) + b) > 
              get<1>(xysortedvecasc[i]) ) {
            
            
        below.push_back (xysortedvecasc[i]);
        
        }
                
        else {
         
        above.push_back (xysortedvecasc[i]);
            
        }
        
        
        } 
       
        
   
      
          
          
          
          
 /* 
move points above and below to 
aboveleft, aboveright, belowleft and belowright.
using the "line" from downpoint to uppoint
     
get points above and below the imagenary "line" from downpoint to uppoint
    
first: testing if point are perpendicular
 */    
    
    
  if (get<0>(uppoint) != get<0>(downpoint) ){   
    
    m = (get<1>(uppoint) - get<1>(downpoint)) / 
        (get<0>(uppoint) - get<0>(downpoint));
    
    b =  get<1>(uppoint) - (m * get<0>(uppoint));
    
    
    
      }
  
   if (get<0>(uppoint) != get<0>(downpoint)) {
  
  
     for (unsigned int i = 0; i < above.size(); i++)  {
        
        
        if ( ((m * get<0>(above[i])) + b) >  get<1>(above[i]) ) {
            
            
        aboveright.push_back (above[i]);
        
        }
                
        else {
         
         aboveleft.push_back (above[i]);
            
        }
        
        
        } 
        
        
        
        
     for (unsigned int i = 0; i < below.size(); i++)  {
        
        
        if ( ((m * get<0>(below[i])) + b) >  get<1>(below[i]) ) {
            
            
        belowright.push_back (below[i]);
        
        }
                
        else {
         
         belowleft.push_back (below[i]);
            
        }
        
        
        } 
        
        
     if (m < 0) {
         
         temp1 = aboveright;
         temp2 = belowright;
         
         aboveright = aboveleft;
         belowright = belowleft;
         
         aboveleft = temp1;
         belowleft = temp2;
         
         
         
     }
        
        
     } 
     
   
   
  /* uppoint and downpoint are perpendicular   */

   if (get<0>(uppoint) == get<0>(downpoint) ) {
       
       for (unsigned int i = 0; i < above.size(); i++)  {
        
        
        if ( (get<0>(above[i])) >=  get<0>(uppoint)) {
            
            
        aboveright.push_back (above[i]);
        
        }
                
        else {
         
         aboveleft.push_back (above[i]);
            
        }
        
        
        }
        
        
        
        
        
       for (unsigned int i = 0; i < below.size(); i++)  {
        
        
        if ( (get<0>(below[i])) >  get<0>(uppoint)) {
            
        belowright.push_back (below[i]);
        
        }
                
        else {
         
         belowleft.push_back (below[i]);
            
        }
        
        
        } 
       
       
   } 
     
     
    
   /* sorting aboveleft, ab aboveright, belowright and belowleft... */
   
   /* aboveleft is already sorted: x up , y up  */
   
   sort(aboveleft.begin(), aboveleft.end(), sortxupyup);
   
   /* sorting aboveright: x up, y down  */ 
  sort(aboveright.begin(), aboveright.end(), sortxupydown); 
       
  /* sorting belowright: x down, y down  */
  sort(belowright.begin(), belowright.end(), sortxdownydown); 
    
  /* sorting belowleft: x down,  y up  */
  sort(belowleft.begin(), belowleft.end(), sortxdownyup); 
  
  
       
  
    
  
    
    
    
   /* constructing the final vector */
  
     
   aboveleftsize  = aboveleft.size();
   aboverightsize = aboveright.size();
   belowleftsize = belowleft.size();
   belowrightsize = belowright.size();
   tmpsize = tmp.size();
   
      
  for (unsigned int i = 0; i < aboveleftsize; i++) {
    
    finalcheckedvec.push_back(aboveleft[i]);
       
   }
  
  
  for (unsigned int i = 0; i < aboverightsize; i++) {
    
    finalcheckedvec.push_back(aboveright[i]);
       
   }
    
  
  
  
 for (unsigned int i = 0; i < belowrightsize; i++) {
    
    finalcheckedvec.push_back(belowright[i]);
       
   }
   
   
 
    
  for (unsigned int i = 0; i < belowleftsize; i++) {
    
    finalcheckedvec.push_back(belowleft[i]);
       
   }
    
 
    
 
 
 if (tmpsize == finalcheckedvec.size()) {
  
     
     
     } 
 
 
 
  
  
 
 
    
    
   
   
  /* put left point at the end of the final vec for testing purposes */ 
   
   finalcheckedvec.push_back(leftpoint);
   
   actpoint = leftpoint; 
   intermpoint = uppoint;
   sectorflag = 1;
   
   iter = 1;
   
   
   if ((get<0>(leftpoint) == get<0>(finalcheckedvec[iter])) &&
       (get<0>(leftpoint) == get<0>(finalcheckedvec[iter+1]))) {
       
    
    
    ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
   return false;  
       
       
   }
   
   
   
   
   if ((get<0>(leftpoint) == get<0>(finalcheckedvec[iter])) &&
       (get<0>(leftpoint) != get<0>(finalcheckedvec[iter+1]))) {
       
   firstperpend = true; 
   setfperflag = true;
  
   /* set mlast > mnew as start value; */
   mlast = 1;
   mnew = 0;   
   
   
   
   }
   
   else {
    
   mlast = (get<1>(finalcheckedvec[iter] ) - get<1>(leftpoint)) /
           (get<0>(finalcheckedvec[iter]) - get<0>(leftpoint));       
       
  
   
   firstmflag = true;
   
   mnew = mlast - 1; // just a dummy value
       
  }
   
   
   
   
   
 while ( (iter < finalcheckedvec.size()) && (okflag == true))  {
     
     
     
     
     /* begin sector 2 case */
      
       
    
        
    if (actpoint == uppoint)    {
        
        intermpoint = rightpoint;
        sectorflag = 2;
        firstperpend = false; 
        setfperflag = false;
         }    
        
        
        
        
   if (sectorflag == 2) {
       
      
    
      if ( (iter+1 <  finalcheckedvec.size()) &&
           (get<0>(actpoint) == get<0>(finalcheckedvec[iter])) &&
           (get<0>(actpoint) == get<0>(finalcheckedvec[iter+1]))) {
          
          
                    
          ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
          return false;  
      
      }
      
      
       
     if (get<0>(actpoint) == get<0>(finalcheckedvec[iter]) &&
         (actpoint != rightpoint) ) {
        
        
        ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
          return false;  
      
         
      
        }
   
          
      
      
      if (get<0>(actpoint) != get<0>(finalcheckedvec[iter]) && 
         actpoint == uppoint)
          
             {
    
            
                 
                 
             mlast = (get<1>(finalcheckedvec[iter] ) - get<1>(uppoint)) /
                     (get<0>(finalcheckedvec[iter]) - get<0>(uppoint));       
       
             firstmflag = true;
   
              mnew = mlast - 1; // just a dummy value
       
             }   
    }  
    /* end of sectorflag == 2 case */
      
      
      
   /* begin sector 3 case */
      
     if (actpoint == rightpoint){
        intermpoint = downpoint;
        sectorflag = 3;
        firstperpend = false; 
        setfperflag = false;
        secondperpend = false;
        firstmflag = false;
        
    }
        
        
        
    if (sectorflag == 3) { 
        
        
   
     
    
   if ( (iter+1 <  finalcheckedvec.size()) &&
        (get<0>(actpoint) == get<0>(finalcheckedvec[iter])) &&
        (get<0>(actpoint) == get<0>(finalcheckedvec[iter+1])) &&
        (actpoint == rightpoint) ) {     
          
          
          
          ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
          return false;  
      
      }
      
        
   
   
   
   
   
  if ((iter+1 <  finalcheckedvec.size()) &&
         (get<0>(rightpoint) == get<0>(finalcheckedvec[iter])) &&
         (get<0>(rightpoint) != get<0>(finalcheckedvec[iter+1])) &&
         (actpoint == rightpoint) )  {
       
   firstperpend = true; 
   setfperflag = true;
  
   /* set mlast > mnew as start value; */
   
   mlast = 1;
   mnew = 0;   
   
   
   }
   
   else if (actpoint == rightpoint) {
    
   mlast = (get<1>(finalcheckedvec[iter] ) - get<1>(rightpoint)) /
           (get<0>(finalcheckedvec[iter]) - get<0>(rightpoint));       
       
      
   firstmflag = true;
   
   mnew = mlast - 1; 
   /* just a dummy value */
       
  }
   
       
      
    } 
    
    /*  end of    sectorflag == 3 case */
      
      
      
     /* begin sector 4 case */
        
        
        
     if ((actpoint == downpoint)  && (downpoint != leftpoint)) {
        intermpoint = leftpoint;
        sectorflag = 4;
       
     
     }
       
    
    
    if ( (sectorflag == 4) && (actpoint != leftpoint) )  {
       
      
    
      if ( (iter+1 <  finalcheckedvec.size()) &&
           (get<0>(actpoint) == get<0>(finalcheckedvec[iter])) &&
           (get<0>(actpoint) == get<0>(finalcheckedvec[iter+1]))) {
          
          
                    
          ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
          return false;  
      
      }
      
      
       
     if (get<0>(actpoint) == get<0>(finalcheckedvec[iter]) &&
         (actpoint != uppoint) ) {
        
       
        ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
          return false;  
      
         
          
        }
   
          
      
      
      if (get<0>(actpoint) != get<0>(finalcheckedvec[iter]) && 
          actpoint == downpoint)
          
             {
    
            
                 
                 
             mlast = (get<1>(finalcheckedvec[iter] ) - get<1>(downpoint)) / 
                     (get<0>(finalcheckedvec[iter]) - get<0>(downpoint));       
       
             
             firstmflag = true;
   
              mnew = mlast - 1; 
              /* just a dummy value */
       
             }   
             
             /* end of sector 4 case */
    
    }
    
    
    
   
       
       
       
    switch (sectorflag) {
        
        case 1: {
              
               
              if  (get<0>(actpoint) == get<0>(finalcheckedvec[iter])&&
                  (setfperflag == false) )
                  
               {secondperpend = true;
                
                
               
                
               }
         
         
              if (! ( (get<0>(actpoint) <= get<0>(finalcheckedvec[iter])) &&
                      (get<1>(actpoint) <= get<1>(finalcheckedvec[iter])) ) ||
                      (setfperflag == false && secondperpend == true) ||
                      mlast <= mnew)
              {
                 
                 okflag = false;    
                 break; }
                 
                 
                 
                 if ((iter+1 <  finalcheckedvec.size()) &&
                     
                     (firstperpend == true) && (setfperflag == true) &&
                     (get<0>(finalcheckedvec[iter+1]) != 
                      get<0>(finalcheckedvec[iter])))   {
                     
                  
                   actpoint = finalcheckedvec[iter];
                   mnew = (get<1>(finalcheckedvec[iter+1] ) - 
                           get<1>(actpoint)) / 
                          (get<0>(finalcheckedvec[iter+1]) - 
                           get<0>(actpoint));     
                          
                   mlast = mnew + 1;
                 
                   setfperflag = false;   
                    
                  }
                 
                 else {
                     
                  
                   actpoint = finalcheckedvec[iter];
                   
                  if ( (iter+1 <  finalcheckedvec.size() ) &&
                      
                      (get<0>(finalcheckedvec[iter+1]) != get<0>(actpoint))&&
                    
                      (firstmflag == false) ) {
                     
                     mlast = mnew; 
                  
                     mnew = (get<1>(finalcheckedvec[iter+1] ) - 
                             get<1>(actpoint)) / 
                            (get<0>(finalcheckedvec[iter+1]) - 
                             get<0>(actpoint));      
                        
                    }
                    
                    else {
                        
                      /* mlast ist the first m value */
                      
                      mnew = (get<1>(finalcheckedvec[iter+1] ) - 
                              get<1>(actpoint)) / 
                             (get<0>(finalcheckedvec[iter+1]) - 
                              get<0>(actpoint));      
                             
                      firstmflag = false; 
                                            
                    }
                 
                 }
                 
                 break;}
            
        
        case 2:{ 
                
           
            if (! ((get<0>(actpoint) <= get<0>(finalcheckedvec[iter])) &&
                   (get<1>(actpoint) >= get<1>(finalcheckedvec[iter])) ) ||
                     mlast <= mnew ) {
                    
                    
                   okflag = false;    
                   break; }
                
                
               
                if ( (iter+1 <  finalcheckedvec.size() ) &&
                    
                    (get<0>(finalcheckedvec[iter+1]) != get<0>(actpoint)) &&
                     
                                         
                      (firstmflag == true) ) {
                    
                      actpoint = finalcheckedvec[iter];
                      
                      mnew = (get<1>(finalcheckedvec[iter+1] ) - 
                              get<1>(actpoint)) /
                             (get<0>(finalcheckedvec[iter+1]) - 
                              get<0>(actpoint));   
                             
                      firstmflag = false;
                        
                    }  
                
                 else {
                    actpoint = finalcheckedvec[iter];
                    
                     mlast = mnew; 
                     
                     mnew = (get<1>(finalcheckedvec[iter+1] ) - 
                             get<1>(actpoint)) / 
                            (get<0>(finalcheckedvec[iter+1]) - 
                             get<0>(actpoint));      
                        
                 }
                
                
                
                break;} 
        
         
        
        
        case 3:{
            
            
            
             if  (get<0>(actpoint) == get<0>(finalcheckedvec[iter])&&
                  (setfperflag == false) )  {
                 
                 secondperpend = true;
                                             
                
               }
         
         
              if (! ( (get<0>(actpoint) >= get<0>(finalcheckedvec[iter])) &&
                      (get<1>(actpoint) >= get<1>(finalcheckedvec[iter])) ) ||
                      (setfperflag == false && secondperpend == true) ||
                      mlast <= mnew)
              {
                 
                 okflag = false;    
                 break; }
                 
                 
                 
                 if ((iter+1 < finalcheckedvec.size() ) &&                     
                     
                     (firstperpend == true) && 
                     (setfperflag == true) &&                    
                     
                     (get<0>(finalcheckedvec[iter+1]) != 
                      get<0>(finalcheckedvec[iter])))   {
                     
                   
                   actpoint = finalcheckedvec[iter];
                 
                   mnew = (get<1>(finalcheckedvec[iter+1] ) - 
                           get<1>(actpoint)) / 
                          (get<0>(finalcheckedvec[iter+1]) - 
                           get<0>(actpoint));       
                          
                   mlast = mnew + 1;
                 
                   setfperflag = false;   
                    
                  }
                 
                 else {
                     
                   
                   actpoint = finalcheckedvec[iter];
                   
                  if ((iter+1 < finalcheckedvec.size()) &&                      
                      
                      (get<0>(finalcheckedvec[iter+1]) !=
                       get<0>(actpoint)) &&
                       
                      (firstmflag == false) ) {
                     
                     mlast = mnew; 
                  
                     mnew = (get<1>(finalcheckedvec[iter+1] ) -
                             get<1>(actpoint)) / 
                            (get<0>(finalcheckedvec[iter+1]) - 
                             get<0>(actpoint));      
                     
                    }
                    
                    else {
                        
                      /* mlast ist the first m value */
                      
                      if  (get<0>(finalcheckedvec[iter+1]) != 
                           get<0>(actpoint)) {
                          
                      mnew = (get<1>(finalcheckedvec[iter+1] ) - 
                              get<1>(actpoint)) / 
                            (get<0>(finalcheckedvec[iter+1]) - 
                             get<0>(actpoint));   
                      }
                      
                     
                      firstmflag = false; 
                                            
                    }
                 
                 }
                 
                 break;}
            
            
            
      
        
        case 4: {
            
            
            
           
            if (! ((get<0>(actpoint) >= get<0>(finalcheckedvec[iter])) &&
                   (get<1>(actpoint) <= get<1>(finalcheckedvec[iter])) ) ||
                     mlast <= mnew ) {
                    
                  
                   okflag = false;    
                   break; }
                
                
               
                if ( (iter+1 <  finalcheckedvec.size() ) &&
                    
                    (get<0>(finalcheckedvec[iter+1]) != get<0>(actpoint)) &&
                    
                     (firstmflag == true) ) {
                    
                      actpoint = finalcheckedvec[iter];
                     
                      mnew = (get<1>(finalcheckedvec[iter+1] ) -
                              get<1>(actpoint)) / 
                             (get<0>(finalcheckedvec[iter+1]) - 
                              get<0>(actpoint));  
                             
                      
                                 
                             
                      firstmflag = false;
                        
                    }  
                
                 else if (iter+1 <  finalcheckedvec.size())
                 
                 {
                    actpoint = finalcheckedvec[iter];
                    
                     mlast = mnew; 
                     
                     mnew = (get<1>(finalcheckedvec[iter+1] ) - 
                             get<1>(actpoint)) / 
                            (get<0>(finalcheckedvec[iter+1]) -
                             get<0>(actpoint));      
                        
                 }
                
                
                
                break;} 
        
   
   
       
       
   
  
    } 
    /* end of switch */
    
    
    
     
   if  (okflag == false) break; 
   iter++; 
   
   }
   /* end of while	*/
   
   
   
    finalcheckedvec.pop_back();
   
   src = finalcheckedvec;   
   
   
	
    
    
  if (okflag == true) 
      
      return true;
      
      else 
          return false;
  
    
  
     

 } 
 
 /* the end of checkme */


/*
 
  4.1   Compare and HasValue 
  
*/
 
 

int Convex::Compare(const Convex& arg) const {
   if(!IsDefined()){
      return arg.IsDefined()?-1:0;
   }
   if(!arg.IsDefined()){
      return 1;
   }
   if(size != arg.size){
     return size < arg.size ? -1 : 1;
   }
   for(size_t i=0;i<size;i++){
     if(value[i].GetX() < arg.value[i].GetX()) return -1;
     if(value[i].GetX() > arg.value[i].GetY()) return 1;
     if(value[i].GetX() == arg.value[i].GetX()
        &&
       (value[i].GetY() < arg.value[i].GetY())) return -1;
     
     
   }
   return 0;
}




size_t Convex::HashValue() const {
  if(!IsDefined() ) return 0;
  if(value==nullptr) return 42;

  unsigned int step = size / 2;
  if(step<1) step = 1;
  size_t pos = 0;
  size_t res = 0;
  while(pos < size){
    res = res + value[pos].HashValue();
    pos += step;
  }
  return res;
}



/*
 
  5.1 In functions
  
*/

Word Convex::In(const ListExpr typeInfo, const ListExpr le1,
                const int errorPos, ListExpr& errorInfo, bool& correct) {

   ListExpr le = le1; 
   ListExpr f, fxpoi, fypoi;
   std::vector<std::tuple<double, double>> tmpo;
   
   bool checkokflag = true;
     
   string lexprstr;
  
  
  
    Word res = SetWord(Address(0));
   
   
    if(listutils::isSymbolUndefined(le)){
      
      
      Convex* co = new Convex(false);
      co->SetDefined(false);
      res.addr = co;      
      correct = true;
      return res;
   }
   
   
  
   
   if(nl->ListLength(le) <= 2){
     Convex* co = new Convex(false);
     correct = true;
       
      co->SetDefined(false);
      res.addr = co;      
     return res;
   }
   
            

   while(!nl->IsEmpty(le)){
     f = nl->First(le);
     le = nl->Rest(le);
    
     
     if(nl->ListLength(f) != 2) 
     
     {
               
      correct = true;
      
      nl->WriteToString(lexprstr, f);
      
      Convex* co = new Convex(false);
   
      co->SetDefined(false);
      res.addr = co;      
     return res;
      
     }    
     
     fxpoi = nl->First(f);
     fypoi = nl->Second(f);
     
      
      
     if ( ( nl->IsAtom(fxpoi) && nl->IsAtom(fypoi))       
         == false)
     
      {
      Convex* co = new Convex(false);
      correct = true;
        
      co->SetDefined(false);
      res.addr = co;      
      return res;
     }    
     
    
     if ( (  (nl->IsAtom(fxpoi) && nl->IsAtom(fypoi)) &&
          (  (nl->AtomType(fxpoi) == RealType) && 
             (nl->AtomType(fypoi) == RealType) ) 
        == false))
     
      {
          
      Convex* co = new Convex(false);
      correct = true;
 
      co->SetDefined(false);
      res.addr = co;      
      return res;
     }    
     
     
      
    
     
     
    
   /* 
   contructing the vektor of tuples       
   */
   
   tmpo.push_back(std::make_tuple(nl->RealValue(fxpoi), nl->RealValue(fypoi)));
   
   
       
   } 
   
   /* end of while */
   
   
   
   /* CHECKME Call  */
   
   checkokflag = checkme(tmpo);
   
  
   
   if (checkokflag == false) {
    
       
   
    ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
   
   
   correct = false; 
   return res;
    
   
    }   
      
  else { 
   
   Convex* r = new Convex(tmpo);
   res.addr = r;
   correct = true; 
   return res;
   
   }
   
   
}
   
 
/*
 
  5.1 Out function
  
*/
   
   
ListExpr Convex::Out(const ListExpr typeInfo, Word value) {
  
  Convex* is = (Convex*) value.addr;
  
 
  if(!is->IsDefined()){
     return listutils::getUndefined();
  }
  
 
    
  if(is->size == 0){
      
     return nl->TheEmptyList();
  }
  
  
  ListExpr res =  nl->OneElemList( 
                      nl->TwoElemList( nl->RealAtom(is->value[0].GetX()),
                                       nl->RealAtom(is->value[0].GetY()) ));
  
   
  
  
  ListExpr last = res;
  for(unsigned int i=1;i<is->size;i++){
    
    last = nl->Append( last,                       
                        nl->TwoElemList ( nl->RealAtom(is->value[i].GetX()),
                                          nl->RealAtom(is->value[i].GetY()) ) );
  }
  
  return res;
}



/*
 
  6.1 Open and save function
  
*/



bool Convex::Open( SmiRecord& valueRecord, size_t& offset, 
           const ListExpr typeInfo, Word& value){

  bool def;
  if(!valueRecord.Read(&def, sizeof(bool), offset)){
    return false;
  } 
  offset += sizeof(bool);
  if(!def){
     value.addr = new Convex(false);
     return true;
  }

  size_t size;  
  if(!valueRecord.Read(&size, sizeof(size_t), offset)){
    return false;
  } 

  offset+=sizeof(size_t);
  if(size==0){
    value.addr = new Convex(0,0);
    return true;
  } 
  Point* v = new Point[size];
  if(!valueRecord.Read(v,size*sizeof(Point),offset)){
    return false;
  }
  value.addr = new Convex(size,v);
  return true;
}




bool Convex::Save(SmiRecord& valueRecord, size_t& offset,
          const ListExpr typeInfo, Word& value) {

   Convex* is = (Convex*) value.addr;
   bool def = is->IsDefined();
   if(!valueRecord.Write(&def, sizeof(bool), offset)){
     return false;
   }
   offset += sizeof(bool);
   if(!def){
     return true;
   }
   size_t size = is->size;
   if(!valueRecord.Write(&size, sizeof(size_t), offset)){
     return false;
   }
   offset += sizeof(size_t);
   if(is->size>0){
      if(!valueRecord.Write(is->value, sizeof(Point) * is->size, offset)){
        return false;
      }
      offset += sizeof(int) * is->size;
   }
   return true;
}





void Convex::Rebuild(char* buffer, size_t sz) {
   if(value!=nullptr){
     delete[] value;
     value = nullptr;
   }
   size = 0;
   bool def;
   size_t offset = 0;
   memcpy(&def,buffer + offset, sizeof(bool));
   offset += sizeof(bool);
   if(!def){
      SetDefined(false);
      return;
   }       
   SetDefined(true);
   memcpy(&size, buffer+offset, sizeof(size_t));
   offset += sizeof(size_t);
   if(size > 0){
     value = new Point[size];
     memcpy(value, buffer+offset, size * sizeof(Point));
     offset += size * sizeof(Point);
   }
}



/*
 
  7.1 Auxillary print function 
  
*/


std::ostream& Convex::Print( std::ostream& os ) const {
    if(!IsDefined()){
       os << "undefined";
       return os;
    } 
    os << "{";
    for(size_t i=0;i<size;i++){
      if(i>0) os << ", ";
      os << value[i]; 
    }
    os << "}";
    return os;
}



/*

8 Operator Definitions

*/


/*

8.1 TypeMapping Definitions

*/




ListExpr createconvextypemap( ListExpr args)
{ 
  
    
  if(!nl->HasLength(args,1))
   {
    return listutils::typeError("only one  arguments expected");
   }
   
  ListExpr arg1 = nl->First(args);
  
  if(!Stream<Point>::checkType(arg1))
    
   {
    return listutils::typeError("first argument must be a stream of points");
   }
   
  
  return nl->SymbolAtom(Convex::BasicType());
      
  
}







ListExpr voronoitypemap ( ListExpr args)

{   
    ListExpr extendconv, stream, namenewattr, attrtype;
    ListExpr second, third, secondname, thirdname;
    string secondnamestr, thirdnamestr;
    int posit1, posit2;
    
   
    if(nl->ListLength(args)!=3){
    ErrorReporter::ReportError("three args expected");
    return nl->TypeError();
  }
    
    
    
     if(nl->AtomType(nl->Second(args))!=SymbolType){
      return listutils::typeError("second arg does not represent a valid "
                                  "attribute name");
    }    
    
    
     if(nl->AtomType(nl->Third(args))!=SymbolType){
      return listutils::typeError("third arg does not represent a valid "
                                  "attribute name");
    }    
    
    second = nl->Second(args); 
    third = nl->Third(args);
    
    if ((nl->ListLength(second) != -1 ) 
       || (nl->ListLength(third)  != -1) ) {
  
  
    ErrorReporter::ReportError("two attribute name arguments expected");
    return nl->TypeError();
   }

    
    
    
    if(!IsStreamDescription(nl->First(args))){
    ErrorReporter::ReportError("first argument is not a tuple stream");
    return nl->TypeError();
  }

    
    stream = nl->First(args);
    namenewattr = nl->Third(args);
    
    
     /* copy attrlist to newattrlist */
  ListExpr attrList = nl->Second(nl->Second(stream));
  ListExpr newAttrList = nl->OneElemList(nl->First(attrList));
  ListExpr lastlistn = newAttrList;
  attrList = nl->Rest(attrList);
  
  
  while (!(nl->IsEmpty(attrList)))
  {
     lastlistn = nl->Append(lastlistn,nl->First(attrList));
     attrList = nl->Rest(attrList);
  }

  /* reset attrList */
  attrList = nl->Second(nl->Second(stream));
  
  
  secondname =  second;
  secondnamestr = nl->SymbolValue(secondname);
  
  thirdname = third;
  thirdnamestr = nl->SymbolValue(thirdname);
  
  
  
  posit1 = FindAttribute(attrList,secondnamestr,attrtype);
  
  if(posit1==0){
       ErrorReporter::ReportError("Attribute "+ secondnamestr +
                                  " must be a member of the tuple");
       return nl->TypeError();
    }
  
  
  
  posit2 = FindAttribute(attrList,thirdnamestr,attrtype);
 if(posit2!=0){
       ErrorReporter::ReportError("Attribute "+ thirdnamestr +
                                  " is already a member of the tuple");
       return nl->TypeError();
    }
 
  
    
    
    
  extendconv = nl->SymbolAtom(Convex::BasicType());
  
  lastlistn = nl->Append(lastlistn, (nl->TwoElemList(namenewattr, extendconv)));
  
  
  
  return 
   
   nl->ThreeElemList(            
        nl->SymbolAtom(Symbol::APPEND()),
        nl->OneElemList(nl->IntAtom(posit1)),  
        nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                        nl->TwoElemList(nl->SymbolAtom(Tuple::BasicType()),
				        newAttrList)));
  
    
}








/*

8.2 ValueMapping Definitions

*/






int createconvexVM (Word* args, Word& result,
                   int message, Word& local, Supplier s) 


{ 
 
 qp->DeleteResultStorage(s);
 qp->ReInitResultStorage(s);
 result = qp->ResultStorage(s);
 
 
 Stream<Point> stream(args[0]);
 Convex* res = static_cast<Convex*>(result.addr);  
 Point* elem; 
 vector<Point> points;
 std::vector<std::tuple<double, double>> temp;
 bool checkgood;
 
 
 stream.open();
 
     
 
 while ( (elem = stream.request() ) ){
     
  
     
   if (!elem->IsDefined()) {
     res->SetDefined(false);
     return 0;
   }
   
  
        
   /* contructing the vektor of tuples */   
   
   
   temp.push_back(std::make_tuple(elem->GetX(), elem->GetY()));
   
  }

  
  
  
checkgood = checkme(temp);  

if (checkgood == true) {

 

 res -> setTo(temp);; 
 
 stream.close(); 
 return 0;    
 }

 
 
 
 else {
 stream.close();
 return 0;    
 }
      
}    
    

    

    
    
    
struct voronoiInfo {
 

map<std::tuple<double, double>, 
std::vector<std::tuple<double, double>> > center2vorpoi;

   
TupleBuffer *rel;

GenericRelationIterator *reliter;

int attrcount;

int pointposition;

};
   
     







    
int voronoiVM (Word* args, Word& result, int message, Word& local, Supplier s)
{
  Word tee;
  
  Tuple* tup;
  
  
      
  voronoiInfo*  localInfo = (voronoiInfo*) qp->GetLocal2(s).addr;
  
  std::vector<VoronoiPoint> voropoints; 
  
  voronoi_diagram<double> vorodiag;
   
  double xval, yval;
  
 
  
  
  
switch (message)
  
{
      
   
      
 case OPEN : {
        
           
       localInfo = new voronoiInfo;
       qp->GetLocal2(s).addr = localInfo;    
          
       ListExpr resultType = GetTupleResultType(s);
       TupleType *tupleType = new TupleType(nl->Second(resultType));
       local.addr = tupleType;
    
       ListExpr attrsave = qp->GetType(qp->GetSon(s,0));
       ListExpr attrsave2 = nl->Second(nl->Second(attrsave));
       
             
       int counter = 0; 
       int counttup = 0;
             
       localInfo->reliter = 0;
       
       int pointpos = ((CcInt*)args[3].addr)->GetIntval();       
        
       std::tuple<double, double> center;
       std::vector<std::tuple<double, double>> voropoi;
       
   
       
   /* storing pointpos in localInfo */
   
   localInfo->pointposition = pointpos;
   
       
  while (!(nl->IsEmpty(attrsave2)))
     {
     
     attrsave2 = nl->Rest(attrsave2);
     
     /* counting the attributes*/ 
     counter++;
     }
     
     /*Init localInfo */
     localInfo->attrcount = counter;     
     
           
    
     qp->Open(args[0].addr);
     qp->Request(args[0].addr, tee);
     

     
     if(qp->Received(args[0].addr))
      {
        localInfo->rel = new TupleBuffer( );
      }
      else
      {
        localInfo->rel = 0;
      }
      
      
   while(qp->Received(args[0].addr))
      { 
 
        /* storing tuples in localInfo*/
        Tuple *tup2 = (Tuple*)tee.addr;
        localInfo->rel->AppendTuple( tup2 );
        
        counttup++;
        
        /*Setting up the VoroPoint vector */
     Point* pointval = static_cast<Point*>(tup2->GetAttribute(pointpos-1));
        
     if(pointval->IsDefined()) {
         
        xval = pointval->GetX(); 
        yval = pointval->GetY();
        voropoints.push_back(VoronoiPoint(xval, yval));
        
        
       }
        
        tup2->DeleteIfAllowed();
        qp->Request(args[0].addr, tee);
      }

    
        
  
     if( localInfo->rel)
      {
        localInfo->reliter = localInfo->rel->MakeScan();
      }
      else
      {
        localInfo->reliter = 0;
      }
       
       
       
     /*constructing the voronoi diagramm using the boost library */
     construct_voronoi(voropoints.begin(), voropoints.end(), &vorodiag);
     
     
  
    unsigned int cell_index = 0;
    for (voronoi_diagram<double>::const_cell_iterator it =
         vorodiag.cells().begin();
         it != vorodiag.cells().end(); ++it) {
      if (it->contains_point()) {
        if (it->source_category() ==
            boost::polygon::SOURCE_CATEGORY_SINGLE_POINT) {
          std::size_t index = it->source_index();
          VoronoiPoint p = voropoints[index];

        
          center =  std::make_tuple(x(p), y(p));
        
                   const voronoi_diagram<double>::cell_type& cell = *it;
          const voronoi_diagram<double>::edge_type* edge = cell.incident_edge();


    //  iterate edges around Voronoi cell.


  do {
      if (true) {

      
      

      if( (edge->vertex0() != NULL) && 
          (edge->vertex1() != NULL))  {
        
       voropoi.push_back(std::make_tuple((edge->vertex0())->x(),
                                         (edge->vertex0())->y()));
       voropoi.push_back(std::make_tuple((edge->vertex1())->x(),
                                         (edge->vertex1())->y()));
 
      }


      if( (edge->vertex0() != NULL) && 
          (edge->vertex1() == NULL))  {
       
       voropoi.push_back(std::make_tuple((edge->vertex0())->x(), 
                                         (edge->vertex0())->y()));
   
       }

 
      if( (edge->vertex1() != NULL) && 
          (edge->vertex0() == NULL))  {

        voropoi.push_back(std::make_tuple((edge->vertex1())->x(), 
                                          (edge->vertex1())->y()));
               
       }


      

          }
      edge = edge->next();


    } while (edge != cell.incident_edge());



    }



      ++cell_index;
    }
    
   localInfo->center2vorpoi.insert
   (pair<std::tuple<double, double>,  std::vector<std::tuple<double, double>>>
   (center, voropoi));
 
   
     voropoi.clear();    
     
    }
  
      return 0;
      
    }
    
   
  
   
 case REQUEST: {
  
   TupleType *tupleType = (TupleType*)local.addr;
   Tuple *newTuple = new Tuple( tupleType );
   
   map<std::tuple<double, double>,  std::vector<std::tuple<double, double>>>::
   iterator iter=localInfo->center2vorpoi.begin();   
   
   int maxattrpos = 0;
   std::vector<std::tuple<double, double>> tmpo;
   std::vector<std::tuple<double, double>> dummy;
   
   bool checkokflag;
   
   double xv, yv;
   
   int pointposit = localInfo->pointposition;

   std::tuple<double, double> search;
   
    /*calculate max attribute position*/            
    maxattrpos = localInfo->attrcount - 1;
    
    Convex* conv2;
       
    
        
   
  if ((tup = localInfo->reliter->GetNextTuple()) != 0 ) {
     
   Point* pointval = static_cast<Point*>(tup->GetAttribute(pointposit-1));
        
     if(pointval->IsDefined()) {
         
        xv = pointval->GetX(); 
        yv = pointval->GetY();
     }
     
     
              
     search =  std::make_tuple(xv, yv);
     
     iter = localInfo->center2vorpoi.find(search);
         
  
   
   if (iter !=localInfo->center2vorpoi.end())   {
       
       
        
   /* setting up the new tuple*/
   
   tmpo = localInfo->center2vorpoi.at(search);   
   
   checkokflag = checkme(tmpo);
   
   if (checkokflag == true) {
    
   Convex* conv = new Convex(tmpo);
   
   conv2 = conv;
   
      
   }
   
  else { 
      
      
     Convex* conv = new Convex(false);
     conv2 = conv;
     
  }
   
   
   
   for( int i = 0; i < tup->GetNoAttributes(); i++ ) {
   
        newTuple->CopyAttribute( i, tup, i );
       }
        
    
   newTuple->PutAttribute(maxattrpos+1, conv2);        
  
   result = SetWord(newTuple);
   
  
   
   return YIELD;
   
   }
   
   else { // not possible
       
       
         
   Convex* conv2 = new Convex(false);
   
   
   for( int i = 0; i < tup->GetNoAttributes(); i++ ) {
   
        newTuple->CopyAttribute( i, tup, i );
       }
        
    
    newTuple->PutAttribute(maxattrpos+1, conv2);        
  
    result = SetWord(newTuple);       
       
   return YIELD;
   
   }
   
   
   return YIELD; //never happens
    
   }
   
   else return CANCEL; 
   
   return 0;
 }
    
    
    
 case CLOSE : {
    
    
    if(localInfo){
        if( localInfo->reliter != 0 )
          delete localInfo->reliter;
        
         if( localInfo->rel )  {
          localInfo->rel->Clear();
          delete localInfo->rel;
           }
        delete localInfo;
         qp->GetLocal2(s).addr=0;
      }
        
      qp->Close(args[0].addr);
      
      if (local.addr)
      {
        ((TupleType*)local.addr)->DeleteIfAllowed();
        local.setAddr(0);
      
      }
     
 return 0;    
 }
 
 

      
 } 
 
 return 0;
}
  
    
 
 
    
    
/*

8.3 Specifications

*/


    



const string createconvexSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>stream(points)  -> convex </text--->"
    "<text>_ createconvex </text--->"
    "<text>Creates a convex polygon in form of a point sequence "
    "starting from the left point in clockwise order."    
    "The input point sequence does not have to be ordered in that way. "
    "Returns the polygon "
    "if the input point sequence form a convex polygon. "
    "Otherwise undef is returned either if the point stream "
    "does not form a convex polygon or " 
    "any other error occurs </text--->"    
    "<text> query Kinos feed head [3] projecttransformstream[GeoData] "
    "createconvex </text--->"
    ") )";
    


    
 const string voronoiSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>((stream (tuple (..., (ak1 Tk1),...))) x ak1 x ak2 "
    "-> (stream (tuple (..., (ak1 Tk1),..., (ak2 Tk2))))) "
    "<text>__ voronoi [ list ] </text--->"
    "<text>Expands each tuple of the relation with a convex polygon "
    " representing the voronoi cell that belongs to the "
    "centerpoint specified "    
    " with point value Tk1 of the attribute ak1.  The complete voronoi "
    " diagramm is set up with "
    " all points of ak1 from all tuples, in other words with "
    "all points of the "
    " ak1 column of the relation. "
    "The value of ak2 ist the type convex forming a convex polygon "
    "representing the voronoi region "
    "that is corresponding to the point value of ak1 </text--->"    
    "<text> query testrel feed voronoi [p, conv ] </text--->" 
    ") )";
    
    
/*
 
10 Registration an initialization

*/   
    
    
/*
 
10.1 Registration of Types

*/



TypeConstructor ConvexTC(
  Convex::BasicType(),
  Convex::Property, Convex::Out, Convex::In, 0,0,
  Convex::Create, Convex::Delete,
  Convex::Open, Convex::Save, Convex::Close, Convex::Clone,
  Convex::Cast, Convex::Size, Convex::TypeCheck
);




/*
  
10.2 Operator instances

*/

Operator createconvex ( "createconvex",
                   createconvexSpec,
                   createconvexVM,
                   Operator::SimpleSelect,
                   createconvextypemap );
         


Operator voronoi  ( "voronoi",
                   voronoiSpec,
                   voronoiVM,
                   Operator::SimpleSelect,
                   voronoitypemap );
         




  
/*
  
10.3 Creating the Algebra 

*/

class ConvexAlgebra : public Algebra 
{
 public:
  ConvexAlgebra() : Algebra()
   

  /* Registration of Types */

  
  
  {
    AddTypeConstructor(&ConvexTC);
    
    
    ConvexTC.AssociateKind(Kind::DATA() );   
   

  /* Registration of operators */
 
    AddOperator( &createconvex);
    AddOperator( &voronoi);
  
 
    
  }
  
  ~ConvexAlgebra() {};
};    


    
} 
 /* end of namespace */




/*
  
10.4 Initialization

*/ 

extern "C"
Algebra*
InitializeConvexAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return new convex::ConvexAlgebra;
}

