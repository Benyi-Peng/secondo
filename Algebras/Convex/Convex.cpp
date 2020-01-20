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
using namespace std; 



extern NestedList* nl;


namespace convex { 
    

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

Convex::Convex(const std::vector
  <std::tuple <double, double>>& src):Attribute(true), value(0){
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





Word Convex::In(const ListExpr typeInfo, const ListExpr le1,
                const int errorPos, ListExpr& errorInfo, bool& correct) {

   ListExpr le = le1; 
   ListExpr f, fxpoi, fypoi;
   
   std::vector<std::tuple<double, double>> tmp;
   std::vector<std::tuple<double, double>> finalvec;
   
   
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
   
   
   
   
   unsigned int  aboveleftsize;
   unsigned int  aboverightsize;
   unsigned int  belowleftsize;
   unsigned int  belowrightsize;
  
   
  
   bool okflag = true;
   bool firstperpend = false;
   bool secondperpend = false;
   bool setfperflag = false;
   bool firstmflag = false;
   
   unsigned int iter, count, sectorflag;
   string lexprstr;
   double m, b, mlast, mnew;
   
   
  
   //Word res((void*)0);
   Word res = SetWord(Address(0));
   
   
   if(listutils::isSymbolUndefined(le)){
      res.addr = new Convex(false);
      correct = true;
      return res;
   }
   
  
   
   if(nl->ListLength(le) <= 2){
     correct = false;
     ErrorReporter::ReportError(
                "You need more than 2 values to define a polygon"); 
     
     return res;
   }
   
            

   while(!nl->IsEmpty(le)){
     f = nl->First(le);
     le = nl->Rest(le);
     
     if (nl->ListLength(f) != 2) {
               
      correct = false;
      
      nl->WriteToString(lexprstr, f);
      
        
      ErrorReporter::ReportError(
                "A pair of coordinates consist only of 2 arguments: '" 
                +lexprstr+ "'");
     return res;
      
     }    
     
     
     fxpoi = nl->First(f);
     fypoi = nl->Second(f);
     
      
      
     if ( ( nl->IsAtom(fxpoi) && nl->IsAtom(fypoi))       
         == false)
     
      {
      correct = false;
     
      nl->WriteToString(lexprstr, f);  
      ErrorReporter::ReportError(
                "Coordinates must bei atoms: '" 
                +lexprstr+ "'");    
      return res;
     }    
     
    
     if ( (  (nl->IsAtom(fxpoi) && nl->IsAtom(fypoi)) &&
          (  (nl->AtomType(fxpoi) == RealType) &&
             (nl->AtomType(fypoi) == RealType) ) 
          == false))
     
      {
      correct = false;
     
      nl->WriteToString(lexprstr, f);  
      ErrorReporter::ReportError(
                "Only values of type real are accepted: '" 
                +lexprstr+ "'");
     
     
      return res;
     }    
     
          
     
     if ( (  (nl->IsAtom(fxpoi) && nl->IsAtom(fypoi)) &&
          (  (nl->AtomType(fxpoi) == RealType) && 
             (nl->AtomType(fypoi) == RealType) ) 
            == false))
     
      {
      correct = false;
      return res;
     }    
     
   //contructing the vektor of tuples       
   
   
   tmp.push_back(std::make_tuple(nl->RealValue(fxpoi), nl->RealValue(fypoi)));
   
   
       
   } //end of while
   
   
   xysortedvecasc = tmp;
   yxsortedvecasc = tmp;
         
  // sorting vecs by x resp. y coord 
         
       sort(xysortedvecasc.begin(), xysortedvecasc.end(), sortxupyup); 
       sort(yxsortedvecasc.begin(), yxsortedvecasc.end(), sortyupxdown); 
       
        //eliminate redundant points 
    count = 0;
    for (unsigned int i = 0; i < xysortedvecasc.size() - 1; i++) { 
        if ( (get<0>(xysortedvecasc[i]) == get<0>(xysortedvecasc[i+1]))&&
              (get<1>(xysortedvecasc[i]) == get<1>(xysortedvecasc[i+1]))) { 
          
           xysortedvecasc.erase(xysortedvecasc.begin()+count);
           
           }
         count++;
         }
    
       
          
       
    // get leftpoint and right point of polygon
    
    leftpoint = xysortedvecasc[0];
    rightpoint = xysortedvecasc[xysortedvecasc.size() - 1];
    downpoint = yxsortedvecasc[0];
    uppoint = yxsortedvecasc[yxsortedvecasc.size() - 1];
   
    
   
    //get points above and below the imagenary "line" 
    //from leftpoint to rightpoint
   
    
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
        
        
        } //end of for 
       
            
          
          
   //move points above and below to aboveleft, aboveright,
   //belowleft and belowright.
   // using the "line" from downpoint to uppoint
    
    
     
   //get points above and below the imagenary "line" 
   //from downpoint to uppoint
    
   //first: testing for if point are perpendicular
    
    
    
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
        
        
        } //end of for 
        
        
        
        
        
     for (unsigned int i = 0; i < below.size(); i++)  {
        
        
      if ( ((m * get<0>(below[i])) + b) >  get<1>(below[i]) ) {
            
            
        belowright.push_back (below[i]);
        
        }
                
        else {
         
         belowleft.push_back (below[i]);
            
        }
        
        
        } //end of for 
        
        
        if (m < 0) {
         
         temp1 = aboveright;
         temp2 = belowright;
         
         aboveright = aboveleft;
         belowright = belowleft;
         
         aboveleft = temp1;
         belowleft = temp2;
              
         }
        
        
       } //end of if
     
   
   
   // uppoint and downpoint are perpendicular
   
   if (get<0>(uppoint) == get<0>(downpoint) ) {
       
       for (unsigned int i = 0; i < above.size(); i++)  {
        
        
        if ( (get<0>(above[i])) >=  get<0>(uppoint)) {
            
            
        aboveright.push_back (above[i]);
        
        }
                
        else {
         
         aboveleft.push_back (above[i]);
            
        }
        
        
        } //end of for 
        
        
        
        
        
       for (unsigned int i = 0; i < below.size(); i++)  {
        
        
        if ( (get<0>(below[i])) >  get<0>(uppoint)) {
            
        belowright.push_back (below[i]);
        
        }
                
        else {
         
         belowleft.push_back (below[i]);
            
        }
        
        
        } //end of for 
       
       
   } //end of if     
     
     
    
   // sorting aboveleft, ab aboveright, belowright and belowleft...
   
   
   // aboveleft is already sorted: x up , y up 
   
   sort(aboveleft.begin(), aboveleft.end(), sortxupyup);
   
   // sorting aboveright: x up, y down
    
  sort(aboveright.begin(), aboveright.end(), sortxupydown); 
       
  // sorting belowright: x down, y down
  
  sort(belowright.begin(), belowright.end(), sortxdownydown); 
    
  //sorting belowleft: x down,  y up
  
  sort(belowleft.begin(), belowleft.end(), sortxdownyup); 
  
   
    
   //constructing the final vector
   
  
     
   aboveleftsize  = aboveleft.size();
   aboverightsize = aboveright.size();
   belowleftsize = belowleft.size();
   belowrightsize = belowright.size();
  
      
  for (unsigned int i = 0; i < aboveleftsize; i++) {
    
    finalvec.push_back(aboveleft[i]);
       
   }
  
  
  for (unsigned int i = 0; i < aboverightsize; i++) {
    
    finalvec.push_back(aboveright[i]);
       
   }
    
  
  
  
 for (unsigned int i = 0; i < belowrightsize; i++) {
    
    finalvec.push_back(belowright[i]);
       
   }
   
   
 
    
 for (int i = belowleftsize-1; i >= 0; i--) {
    
    finalvec.push_back(belowleft[i]);
       
   }
    
 
 
   
  // put left point at the end of the final vec for testing for purposes 
   
   
   finalvec.push_back(leftpoint);
   
   actpoint = leftpoint; 
   intermpoint = uppoint;
   sectorflag = 1;
   
   iter = 1;
   
   
   // testing for double vertical at the beginning 
   
   if ((get<0>(leftpoint) == get<0>(finalvec[iter])) &&
       (get<0>(leftpoint) == get<0>(finalvec[iter+1]))) {
       
   
    correct = false;
    ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
   return res;  
       
       
   }
   
   
   
   
   if ((get<0>(leftpoint) == get<0>(finalvec[iter])) &&
       (get<0>(leftpoint) != get<0>(finalvec[iter+1]))) {
       
   firstperpend = true; 
   setfperflag = true;
  
   // set mlast > mnew just as start value;
   mlast = 1;
   mnew = 0;   
   }
   
   else { // first gradient
    
   mlast = (get<1>(finalvec[iter] ) - get<1>(leftpoint)) / 
           (get<0>(finalvec[iter]) - get<0>(leftpoint));       
       
   firstmflag = true;
   
   mnew = mlast - 1; // just a dummy value
       
  }
   
   
   
   
   
 while ( (iter < finalvec.size()) && (okflag == true))  {
     
     
     
     
     // begin sector 2 case
      
         
        
    if (actpoint == uppoint)    {
        
        intermpoint = rightpoint;
        sectorflag = 2;
        firstperpend = false; 
        setfperflag = false;
         }    
        
        
        
        
   if (sectorflag == 2) {
       
      // testing for double verticals 
      if ( (iter+1 <= finalvec.size()) &&  
           (get<0>(actpoint) == get<0>(finalvec[iter])) &&
           (get<0>(actpoint) == get<0>(finalvec[iter+1]))) {
          
              
          correct = false;
          ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
          return res;  
      
      }
      
      
        //testing for single vertical
     if (get<0>(actpoint) == get<0>(finalvec[iter]) && 
         (actpoint != rightpoint) ) {
        
        correct = false;
        ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
          return res;  
               
        }
   
          
      
      
      if (get<0>(actpoint) != get<0>(finalvec[iter]) 
          && actpoint == uppoint)  {              
                 
             mlast = (get<1>(finalvec[iter] ) -  // first gradient
                     get<1>(uppoint)) / 
                     (get<0>(finalvec[iter]) - get<0>(uppoint));       
       
             
             firstmflag = true;
   
             mnew = mlast - 1; // just a dummy value
       
             }   
             
    }  // end of sectorflag == 2 case
      
      
      
      
      
   // begin sector 3 case
      
     if (actpoint == rightpoint){
        intermpoint = downpoint;
        sectorflag = 3;
        firstperpend = false; 
        setfperflag = false;
        secondperpend = false;
        firstmflag = false;
        
    }
        
        
        
    if (sectorflag == 3) { 
        
     
    
    if ( (iter+1 <= finalvec.size()) &&  //testing for double verticals
         (get<0>(actpoint) == get<0>(finalvec[iter])) &&
         (get<0>(actpoint) == get<0>(finalvec[iter+1])) &&
         (actpoint == rightpoint) ) {     
          
          correct = false;
          ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
          return res;  
      
      }
      
       
   
   if ((iter+1 <= finalvec.size()) && //testing for single vertical
          (get<0>(rightpoint) == get<0>(finalvec[iter])) &&
          (get<0>(rightpoint) != get<0>(finalvec[iter+1])) &&
          (actpoint == rightpoint) )  {
       
   firstperpend = true; 
   setfperflag = true;
  
   // set mlast > mnew just as start value;
   mlast = 1;
   mnew = 0;   
   
     }
   
   else if (actpoint == rightpoint) { // first gradient
    
   mlast = (get<1>(finalvec[iter] ) - get<1>(rightpoint)) / 
           (get<0>(finalvec[iter]) - get<0>(rightpoint));       
       
   firstmflag = true; 
   
   mnew = mlast - 1; // just a dummy value
       
  }
   
       
      
    } //  end of    sectorflag == 3 case
      
      
      
     // begin sector 4 case
        
        
        
     if ((actpoint == downpoint)  && (downpoint != leftpoint)) {
        intermpoint = leftpoint;
        sectorflag = 4;
       
     
     }
       
    
    
    if ( (sectorflag == 4) && (actpoint != leftpoint) )  { 
        
       //testing for double verticals
        if ( (iter+1 <= finalvec.size()) && 
           (get<0>(actpoint) == get<0>(finalvec[iter])) &&
           (get<0>(actpoint) == get<0>(finalvec[iter+1]))) {
                   
          correct = false;
          ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
          return res;  
      
      }
      
      
       // testing for single vertical
     if (get<0>(actpoint) == get<0>(finalvec[iter]) && 
         (actpoint != uppoint) ) {
        correct = false;
        ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
          return res;  
            
        }
   
          
      
      
      if (get<0>(actpoint) != get<0>(finalvec[iter]) 
          && actpoint == downpoint)  {
          
          //first gradient
          mlast = (get<1>(finalvec[iter] ) - get<1>(uppoint)) /    
                  (get<0>(finalvec[iter]) - get<0>(uppoint))  ;       
       
            
             firstmflag = true;
   
              mnew = mlast - 1; // just a dummy value
       
             }   //end of sector 4 case
    
    
    }
    
    
          
       
    switch (sectorflag) {
        
        case 1: {
              
               
              if  (get<0>(actpoint) == get<0>(finalvec[iter])&&
                  (setfperflag == false) )  {
                  
                  secondperpend = true;   
               
                      
                   }
         
         
              if (! ( (get<0>(actpoint) <= get<0>(finalvec[iter])) &&
                      (get<1>(actpoint) <= get<1>(finalvec[iter])) ) ||
                      (setfperflag == false && secondperpend == true) ||
                      mlast <= mnew)   {
                 
                 okflag = false;    
                 break; 
                          
                }
                 
                 
                 
                 if ((firstperpend == true) && (setfperflag == true) &&
                     (get<0>(finalvec[iter+1]) != get<0>(finalvec[iter])))   {
                     
                   
                   actpoint = finalvec[iter];
                   mnew = (get<1>(finalvec[iter+1] ) - get<1>(actpoint)) / 
                          (get<0>(finalvec[iter+1]) - get<0>(actpoint));       
                   mlast = mnew + 1;
                   setfperflag = false;   
                    
                  }
                 
                 else {
                     
                  actpoint = finalvec[iter];
                   
                  if (get<0>(finalvec[iter+1]) != get<0>(actpoint) &&
                      (firstmflag == false) ) {
                     
                     mlast = mnew; 
                     mnew = (get<1>(finalvec[iter+1] ) - get<1>(actpoint)) / 
                            (get<0>(finalvec[iter+1]) - get<0>(actpoint));      
                        
                    }
                    
                    else {
                        
                      // mlast ist the first m value  
                     mnew = (get<1>(finalvec[iter+1] ) - get<1>(actpoint)) / 
                            (get<0>(finalvec[iter+1]) - get<0>(actpoint));      
                      firstmflag = false; 
                                            
                    }
                 
                 }
                 
                 break;}
            
        
        case 2:{ 
                
           
            if (! ((get<0>(actpoint) <= get<0>(finalvec[iter])) &&
                   (get<1>(actpoint) >= get<1>(finalvec[iter])) ) ||
                     mlast <= mnew ) {
                    
                       
                   okflag = false;    
                   break; }
                
                
               
                if (get<0>(finalvec[iter+1]) != get<0>(actpoint) &&
                     (firstmflag == true) ) {
                      actpoint = finalvec[iter];
                      mnew = (get<1>(finalvec[iter+1] ) - get<1>(actpoint)) / 
                             (get<0>(finalvec[iter+1]) - get<0>(actpoint));   
                      firstmflag = false;
                        
                    }  
                
                 else {
                    actpoint = finalvec[iter];
                     mlast = mnew; 
                     mnew = (get<1>(finalvec[iter+1] ) - get<1>(actpoint)) / 
                            (get<0>(finalvec[iter+1]) - get<0>(actpoint));      
                        
                 }
                
                
                
                break;} 
        
         
        
        
        case 3:{
            
             
            
             if  (get<0>(actpoint) == get<0>(finalvec[iter])&&
                 (setfperflag == false) )  {
                   secondperpend = true;      
                
               }
         
         
              if (! ( (get<0>(actpoint) >= get<0>(finalvec[iter])) &&
                      (get<1>(actpoint) >= get<1>(finalvec[iter])) ) ||
                      (setfperflag == false && secondperpend == true) ||
                      mlast <= mnew)  {
                
                 okflag = false;    
                 break; 
                          
                }
                 
                 
                 
                 if ((firstperpend == true) && (setfperflag == true) &&
                     (get<0>(finalvec[iter+1]) != get<0>(finalvec[iter])))   {
                     
                  
                   actpoint = finalvec[iter];
                   mnew = (get<1>(finalvec[iter+1] ) - get<1>(actpoint)) / 
                          (get<0>(finalvec[iter+1]) - get<0>(actpoint));       
                   mlast = mnew + 1;
                   setfperflag = false;   
                    
                  }
                 
                 else {
                     
                   
                   actpoint = finalvec[iter];
                   
                  if (get<0>(finalvec[iter+1]) != get<0>(actpoint) &&
                      (firstmflag == false) ) {
                     
                     mlast = mnew; 
                     mnew = (get<1>(finalvec[iter+1] ) - get<1>(actpoint)) / 
                            (get<0>(finalvec[iter+1]) - get<0>(actpoint));      
                     
                    }
                    
                    else {
                        
                      // mlast ist the first m value  
                      if  (get<0>(finalvec[iter+1]) != get<0>(actpoint)) {
                          
                      mnew = (get<1>(finalvec[iter+1] ) - get<1>(actpoint)) / 
                             (get<0>(finalvec[iter+1]) - get<0>(actpoint));   
                      }
                      
                      
                      firstmflag = false; 
                                            
                    }
                 
                 } //end of else
                 
                 break;
            
                }
            
            
            
      
        
        case 4: {
            
            
          
           
            if (! ((get<0>(actpoint) >= get<0>(finalvec[iter])) &&
                   (get<1>(actpoint) <= get<1>(finalvec[iter])) ) ||
                     mlast <= mnew ) {
                    
                       
                   okflag = false;    
                   break;
                         
                    }
                
                
               
                if (get<0>(finalvec[iter+1]) != get<0>(actpoint) &&
                     (firstmflag == true) ) {
                      actpoint = finalvec[iter];
                      mnew = (get<1>(finalvec[iter+1] ) - get<1>(actpoint)) / 
                             (get<0>(finalvec[iter+1]) - get<0>(actpoint));   
                      firstmflag = false;
                        
                    }  
                
                 else {
                    actpoint = finalvec[iter];
                     mlast = mnew; 
                     mnew = (get<1>(finalvec[iter+1] ) - get<1>(actpoint)) / 
                            (get<0>(finalvec[iter+1]) - get<0>(actpoint));      
                        
                 }
                
                
                
                break;
            
        } 
        
            
            
           
       
       
   
  
    }
   if  (okflag == false) break; 
   iter++; 
   }	
	
    
    
    
  
    
       
    
    
    
    
    
    
    //checking if concave scenarios have been found     
    //give back the result
    
      
  if (okflag == false) {
       
    correct = false;
    ErrorReporter::ReportError(
                "The coordinates do not form a convex polygon");  
   return res;   
   }   
      
  else { 
   finalvec.pop_back();
   Convex* r = new Convex(finalvec);
   res.addr = r;
   correct = true; 
   return res;
   
   }
   
   
}







ListExpr Convex::Out(const ListExpr typeInfo, Word value) {
  
  Convex* is = (Convex*) value.addr;
  
  Point point = is->value[0];
 
    
  
  if(!is->IsDefined()){
     
     return listutils::getUndefined();
  }
  
  
  
  if(is->size == 0){
     
     return nl->TheEmptyList();
  }
  
  
  ListExpr res =  nl->OneElemList( nl->TwoElemList( 
                      nl->RealAtom(is->value[0].GetX()),
                      nl->RealAtom(is->value[0].GetY()) ));
  
  
  
  
  
  
  ListExpr last = res;
  for(unsigned int i=1;i<is->size;i++){
    
    last = nl->Append( last,                       
                        nl->TwoElemList ( 
                          nl->RealAtom(is->value[i].GetX()),
                          nl->RealAtom(is->value[i].GetY()) ) );
  }
  
  return res;
}




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
            
    
         
         
     // converting the right tuple vector into point sequence    
         
      
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









TypeConstructor ConvexTC(
  Convex::BasicType(),
  Convex::Property, Convex::Out, Convex::In, 0,0,
  Convex::Create, Convex::Delete,
  Convex::Open, Convex::Save, Convex::Close, Convex::Clone,
  Convex::Cast, Convex::Size, Convex::TypeCheck
);




  
//Implementation of the Algebra Class

class ConvexAlgebra : public Algebra
{
 public:
  ConvexAlgebra() : Algebra()
  

//Registration of Types

  
  
  {
    AddTypeConstructor(&ConvexTC);
    
    
    ConvexTC.AssociateKind(Kind::DATA() );   
   

// Registration of operators
 
    
 
    
  }
  
  ~ConvexAlgebra() {};
};    


    
} //end of namespace


//Initialization 




extern "C"
Algebra*
InitializeConvexAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return new convex::ConvexAlgebra;
}


