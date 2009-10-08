/*
---- 
This file is part of SECONDO.

Copyright (C) 2004-2009, University in Hagen, Faculty of Mathematics and Computer Science, 
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

//[&] [\&]

*/

#include "ListUtils.h"
#include "NestedList.h"
#include "SecondoSystem.h"

#include <set>
#include <string>

extern NestedList* nl;

namespace listutils{

/*
 Returns a list containing a symbol "ERROR";

*/

  ListExpr emptyErrorInfo(){
    return nl->OneElemList(nl->SymbolAtom("ERROR"));
  }

/*
Checks for a Spatial type.

*/

  bool isSpatialType(ListExpr arg){
    AlgebraManager *algMgr = SecondoSystem::GetAlgebraManager();
    ListExpr errorInfo = emptyErrorInfo();
    return algMgr->CheckKind("SPATIAL2D", arg, errorInfo) ||
            algMgr->CheckKind("SPATIAL3D", arg, errorInfo) ||
            algMgr->CheckKind("SPATIAL4D", arg, errorInfo) ||
            algMgr->CheckKind("SPATIAL8D", arg, errorInfo);      
  }

/*
Checks for a rectangle type 

*/

  bool isRectangle(ListExpr arg){
    return nl->IsEqual(arg, "rect") ||
           nl->IsEqual(arg, "rect3") ||
           nl->IsEqual(arg, "rect4") ||
           nl->IsEqual(arg, "rect8");
 
  }



/*
Creates a list "typeerror".

*/
  ListExpr typeError(){
     return nl->TypeError();
  } 

/* 
Writes a message to the errorreporter and returns
"typeerror".

*/
  ListExpr typeError(string message){
    ErrorReporter::ReportError(message);
    return nl->TypeError();
  }


/*
Checks for a valid description of an rtree.


*/  

  bool isRTreeDescription(ListExpr rtree){
   // (rtree tupledescription type bool)
   if(nl->ListLength(rtree)!=4){
     return false;
   }
   ListExpr rtreeSymbol = nl->First(rtree);
   if(nl->AtomType(rtreeSymbol)!=SymbolType){
     return false;
   }   
   string rtreestr = nl->SymbolValue(rtreeSymbol);  

   if( (rtreestr != "rtree") &&
       (rtreestr != "rtree3") &&
       (rtreestr != "rtree4") &&
       (rtreestr != "rtree8") ){
      return false;
   }
 
   if(!isTupleDescription(nl->Second(rtree))){
     return false;
   }
   if(nl->AtomType(nl->Fourth(rtree))!= BoolType){
     return false;
   }
   // check for valid type as third element omitted
   return true;
  }


/*
Checks for a BTreeDescription

*/
bool isBTreeDescription(ListExpr btree){
  if(nl->ListLength(btree)!=3){
    return false;
  }
  return nl->IsEqual(nl->First(btree),"btree") &&
         isTupleDescription(nl->Second(btree)) &&
         isDATA(nl->Third(btree));
}

  bool isHashDescription(ListExpr hash){
    if(nl->ListLength(hash)!=3){
      return false;
    }
    return isSymbol(nl->First(hash),"hash") &&
         isTupleDescription(nl->Second(hash)) &&
         isDATA(nl->Third(hash));
  }


/*
Checks for valid description of a tuple.

*/ 

  bool isTupleDescription(ListExpr tuple, const bool ismtuple /*=false*/){
    if(nl->ListLength(tuple)!=2){
       return false;
    }
    string tuplesym = ismtuple?"mtuple":"tuple";
    if(!isSymbol(nl->First(tuple),tuplesym)){
       return false;
    }
    return isAttrList(nl->Second(tuple));
  }

/*
Checks for a valid atribute list 

*/
  bool isAttrList(ListExpr attrList){
    ListExpr rest = attrList;
    ListExpr current;
    ListExpr errorInfo = emptyErrorInfo();

    if(nl->AtomType(attrList)!=NoAtom){
       return  false;
    }
    if(nl->IsEmpty(attrList)){
       return false;
    }
    set<string> attrnames;
    while(!nl->IsEmpty(rest)) {
      current = nl->First(rest);
      rest = nl->Rest(rest);
      if(nl->ListLength(current)!=2){
         return false;
      }
      ListExpr attrName = nl->First(current);
      ListExpr attrType = nl->Second(current);
      if(nl->AtomType(attrName)!=SymbolType){
        return false;
      }
      string name = nl->SymbolValue(attrName);
      if(attrnames.find(name)!=attrnames.end()){
         return false;
      }
      attrnames.insert(name);
      if(!am->CheckKind("DATA", attrType, errorInfo)){
         return false;
      }
    }
    return true;
  }

/*
Checks for disjoint attribute lists.

Precondition isAttrList(l1) [&]  isAttrList(l2)

*/
  bool disjointAttrNames(ListExpr l1, ListExpr l2){
    assert(isAttrList(l1));
    assert(isAttrList(l2));
    set<string> names;
    ListExpr rest = l1;
    while(!nl->IsEmpty(rest)){
      names.insert(nl->SymbolValue(nl->First(nl->First(rest))));
      rest = nl->Rest(rest);
    }
    rest = l2;
    while(!nl->IsEmpty(rest)){
      string name = nl->SymbolValue(nl->First(nl->First(rest)));
      if(names.find(name)!=names.end()){
        return false;
      }
      rest = nl->Rest(rest);
    }
    return true;
  }

/*
Checks whether the list corresponds to a given symbol.

*/  
 bool isSymbol(const ListExpr list, const string& v){
   if(nl->AtomType(list)!=SymbolType){
     return false;
   }
   return nl->SymbolValue(list) == v;
 }

 bool isSymbol(const ListExpr list){
   return nl->IsAtom(list) && (nl->AtomType(list)==SymbolType);
 }

 bool isASymbolIn(const ListExpr list, const set<string>& s){
   if(!isSymbol(list)){
     return false;
   }
   string v = nl->SymbolValue(list);
   return s.find(v)!=s.end();
 }

/*
Concatenates l1 and l2.

*/
 ListExpr concat(ListExpr l1, ListExpr l2){
   assert(nl->AtomType(l1) == NoAtom);
   assert(nl->AtomType(l2) == NoAtom);
   if(nl->IsEmpty(l1)){
     return l2;
   }
   ListExpr res = nl->OneElemList(nl->First(l1));
   l1 = nl->Rest(l1);
   ListExpr last = res;
   while(!nl->IsEmpty(l1)){
     last = nl->Append(last, nl->First(l1));
     l1 = nl->Rest(l1);
   }
   while(!nl->IsEmpty(l2)){
     last = nl->Append(last, nl->First(l2));
     l2 = nl->Rest(l2);
   }
   return res;
 }

 



/*
Returns the keytype fo an rtree description.

*/
  ListExpr getRTreeType(ListExpr rtree){
     assert(isRTreeDescription(rtree));
     return nl->Third(rtree);
  }

/*
Returns the dimension of an rtree.

*/
  int getRTreeDim(ListExpr rtree){
     assert(isRTreeDescription(rtree));
     string t = nl->SymbolValue(nl->First(rtree));
     if(t=="rtree") return 2;
     if(t=="rtree3") return 3;
     if(t=="rtree4") return 4;
     if(t=="rtree8") return 8;
     assert(false);
     return -1; 
  }

/*
Checks for a valid relation description.

*/
  bool isRelDescription(ListExpr rel, const bool trel /*=false*/){
    string relsymb = trel?"trel":"rel";
    return isRelDescription2(rel, relsymb);
  }


  bool isRelDescription2(ListExpr rel, const string& relsymb){
    if(nl->ListLength(rel)!=2){
       return false;
    }
    if(!isSymbol(nl->First(rel),relsymb)){
       return false;
    }
    bool mtuple = relsymb=="mrel";
    return isTupleDescription(nl->Second(rel),mtuple);   
  }


/*
Checks for a tuple stream

*/
  bool isTupleStream(ListExpr s){
    if(nl->ListLength(s)!=2){
       return false;
    }
    if(!nl->IsEqual(nl->First(s),"stream")){
       return false;
    }
    return isTupleDescription(nl->Second(s));
  }

/*
Checks for Kind DATA

*/
bool isDATA(ListExpr type){
 AlgebraManager *algMgr = SecondoSystem::GetAlgebraManager();
 ListExpr errorInfo = emptyErrorInfo();
 return algMgr->CheckKind("DATA", type, errorInfo);
}


/*
CHecks whether this list corresponds to a type in given kind

*/
  bool isKind(ListExpr type, const string& kind){
    AlgebraManager *algMgr = SecondoSystem::GetAlgebraManager();
    ListExpr errorInfo = emptyErrorInfo();
    return algMgr->CheckKind(kind, type, errorInfo);
  }


/*
 Checks for a numeric type

*/
bool isNumeric(ListExpr num){
   return nl->AtomType(num) == IntType ||
          nl->AtomType(num) == RealType;
}

double getNumValue(ListExpr n){
  if(nl->AtomType(n)==IntType){
    return nl->IntValue(n);
  } if(nl->AtomType(n)==RealType){
    return nl->RealValue(n);
  } else {
    assert(false);
  }
}

bool isNumericType(ListExpr n){
  if(nl->AtomType(n)!=SymbolType){
     return false;
  } 
  string v = nl->SymbolValue(n);
  return v=="int" || v=="real";
}



/*
Checks for a stream of kind DATA

*/
  bool isDATAStream(ListExpr s){
    if(nl->ListLength(s)!=2){
       return false;
    }
    if(!nl->IsEqual(nl->First(s),"stream")){
       return false;
    }
    AlgebraManager *algMgr = SecondoSystem::GetAlgebraManager();
    ListExpr errorInfo = emptyErrorInfo();
    return algMgr->CheckKind("DATA", nl->Second(s), errorInfo);
  }

  int findAttribute(ListExpr attrList, const string& name, ListExpr& type){
     assert(isAttrList(attrList));
     int j = 0;
     ListExpr rest = attrList;
     while(!nl->IsEmpty(rest)){
       ListExpr current = nl->First(rest);
       j++;
       if(nl->IsEqual(nl->First(current),name)){
          type = nl->Second(current);
          return j; 
       }  
       rest = nl->Rest(rest);
     }
     return 0; 
  }

  int findType(ListExpr attrList, const ListExpr type, 
                string& name, const int start/*=1*/){
    assert(isAttrList(attrList));
    ListExpr rest = attrList;
    int j = 0;
    while(!nl->IsEmpty(rest)){
       ListExpr current = nl->First(rest);
       rest = nl->Rest(rest);
       j++;
       if(j>=start){
          if(nl->Equal(nl->Second(current),type)){
              name = nl->SymbolValue(nl->First(current));
              return j;
          }
       } 
    }
    return 0;

  }

  int removeAttributes(ListExpr list, const set<string>& names,
                       ListExpr& head, ListExpr& last){
     assert(isAttrList(list));
     bool firstCall = true;
     int count = 0;
     while(!nl->IsEmpty(list)){
       ListExpr pair = nl->First(list);
       list = nl->Rest(list);
       string name = nl->SymbolValue(nl->First(pair));
       if(names.find(name)==names.end()){
         if(firstCall){
           firstCall=false;
           head = nl->OneElemList(pair);
           last = head;
         } else {
           last = nl->Append(last, pair);
         } 
       } else {
         count++;
       }

     }
     if(firstCall){
      head = nl->TheEmptyList();
      last = head;
     }
     return count;
  }


} // end of namespace listutils
