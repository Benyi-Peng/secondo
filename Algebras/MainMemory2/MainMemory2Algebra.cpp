/*

----
This file is part of SECONDO.

Copyright (C) 2016, 
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


//[_] [\_]

*/


#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "ListUtils.h"
#include "Attribute.h"
#include "AlgebraManager.h"
#include "Operator.h"
#include "StandardTypes.h"
#include "NList.h"
#include "Symbols.h"
#include "SecondoCatalog.h"
#include "SecondoSystem.h"
#include "RelationAlgebra.h"
#include "OrderedRelationAlgebra.h"
#include "GraphAlgebra.h"
#include "Stream.h"
#include "MMRTree.h"
#include "MMMTree.h"
#include "FTextAlgebra.h"
#include "MovingRegionAlgebra.h"
#include "RectangleAlgebra.h"
#include "AvlTree.h"

#include "MainMemoryExt.h"
#include "ttree.h"

#include "MPointer.h"
// #include "mapmatch.h"


#include <ctime>
#include <exception>
#include <string>
#include <map>
#include <vector>
#include <limits>
#include <algorithm>

#include <ctime>


#include "MemoryVectorObject.h"

using namespace std;

extern NestedList* nl;
extern QueryProcessor *qp;
extern SecondoSystem* instance;


ostream& operator<<(ostream& o, const mm2algebra::AttrIdPair& t) {
  o << "(";
  t.getAttr()->Print(o);
  o << ", " << t.getTid()   << ")";
  return o;
}


namespace mm2algebra {

MemCatalog* catalog =0;

#define MEMORYMTREEOBJECT "memoryMTreeObject"



bool checkUsesArgs(ListExpr args){
  ListExpr tmp = args;
  while(!nl->IsEmpty(tmp)){
     if(!nl->HasLength(nl->First(args),2)){
        return false;
     }
     tmp = nl->Rest(tmp);
  }
  return true;
}

/*

4 Auxiliary functions

*/
namespace rtreehelper{


   int getDimension(ListExpr type){
      if(listutils::isKind(type,Kind::SPATIAL2D())) return 2;
      if(listutils::isKind(type,Kind::SPATIAL3D())) return 3;
      if(listutils::isKind(type,Kind::SPATIAL4D())) return 4;
      if(listutils::isKind(type,Kind::SPATIAL8D())) return 8;
      return -1;   
   }

   string BasicType(){
     return "rtree";
   }

   bool checkType(ListExpr type){
      if(!nl->HasLength(type,2)){
        return false;
      }
      if(!listutils::isSymbol(nl->First(type),BasicType())){
         return false;
      }
      if(nl->AtomType(nl->Second(type))!=IntType){
         return false;
      }
      int dim = nl->IntValue(nl->Second(type));
      return (dim==2) || (dim==3) || (dim==4) || (dim==8);
   }

   bool checkType(ListExpr type, int dim){
     if(!checkType(type)){
       return false;
     }
     return nl->IntValue(nl->Second(type))==dim;
   }



}

namespace mtreehelper{

  double distance(const Point* p1, const Point* p2) {
     if(!p1->IsDefined() && !p2->IsDefined()){
       return 0;
     }
     if(!p1->IsDefined() || !p2->IsDefined()){
         return std::numeric_limits<double>::max();
     } 
     return p1->Distance(*p2);
  }

  double distance(const CcString* s1, const CcString* s2) {
     if(!s1->IsDefined() && !s2->IsDefined()){
        return 0;
     }
     if(!s1->IsDefined() || !s2->IsDefined()){
        return std::numeric_limits<double>::max();
     }
     return stringutils::ld(s1->GetValue() ,s2->GetValue());
  }

  double distance(const CcInt* i1, const CcInt* i2) {
     if(!i1->IsDefined() && !i2->IsDefined()){
        return 0;
     }
     if(!i1->IsDefined() || !i2->IsDefined()){
        return std::numeric_limits<double>::max();
     }
     return abs(i1->GetValue() - i2->GetValue());
  }
  
  double distance(const CcReal* r1, const CcReal* r2) {
     if(!r1->IsDefined() && !r2->IsDefined()){
        return 0;
     }
     if(!r1->IsDefined() || !r2->IsDefined()){
        return std::numeric_limits<double>::max();
     }
     return abs(r1->GetValue() - r2->GetValue());
  }

  template<unsigned int dim>
  double distance(const Rectangle<dim>* r1, 
                  const Rectangle<dim>* r2) {
     if(!r1->IsDefined() && !r2->IsDefined()){
        return 0;
     }
     if(!r1->IsDefined() || !r2->IsDefined()){
        return std::numeric_limits<double>::max();
     }
     return r1->Distance(*r2);
  }



/*
   6.4 ~getTypeNo~

   Returns a number for supported types, -1 if not supported.

*/
  typedef Point t1;
  typedef CcString t2;
  typedef CcInt t3;
  typedef CcReal t4;
  typedef Rectangle<1> t5;
  typedef Rectangle<2> t6;
  typedef Rectangle<3> t7;
  typedef Rectangle<4> t8;
  typedef Rectangle<8> t9;

  int getTypeNo(ListExpr type, int expectedNumbers){
     assert(expectedNumbers==9);
     if( t1::checkType(type)){ return 0;}
     if( t2::checkType(type)){ return 1;}
     if( t3::checkType(type) ){ return 2;}
     if( t4::checkType(type)){ return 3; }
     if( t5::checkType(type)){ return 4;}
     if( t6::checkType(type)){ return 5;}
     if( t7::checkType(type)){ return 6;}
     if( t8::checkType(type)){ return 7;}
     if( t9::checkType(type)){ return 8;}
     return -1;
  }

  string BasicType(){
    return  "mtree";
  }

  bool checkType(ListExpr type, ListExpr subtype){
    if(!nl->HasLength(type,2)){
       return  false;
    }
    if(!listutils::isSymbol(nl->First(type), BasicType())){
       return false;
    }
    if(getTypeNo(subtype, 9) < 0){
      return false;
    }
    return nl->Equal(nl->Second(type), subtype);
  }


}


template<class T>
class StdDistComp{
  public:
    double  operator()(const pair<T,TupleId>&  o1, 
                       const pair<T,TupleId>& o2){
       return mtreehelper::distance(&o1.first,&o2.first);
    }

    ostream& print( const pair<T, TupleId> & p,  ostream& o){
       o << "<"; p.first.Print(o); o << p.second << ">";
       return o;
    }
  
    void reset(){} // not sure
};


namespace ttreehelper{

  string BasicType(){
    return  "ttree";
  }
  
  bool checkType(ListExpr type){
    if(!nl->HasLength(type,2)){
      return false;
    }
    if(!listutils::isSymbol(nl->First(type),BasicType())){
      return false;
    }
    return true;
  }
} // end namespace ttreehelper


/*

Some functions for getting a certain memory object by name.
The name may be given as a CcString or as a Mem instance. 
A lot of checkes ensure correctness of operations. If one of 
the test failes, the result will be 0 , otherwise a 
pointer to the memory object.

*/

template<class T>
memAVLtree* getAVLtree(T* treeN, ListExpr subtype){
   if(!treeN->IsDefined()){
      return 0;
   }
   string treen = treeN->GetValue();
   if(!catalog->isMMObject(treen) || !catalog->isAccessible(treen)){
      return 0;
   }
   ListExpr treet = nl->Second(catalog->getMMObjectTypeExpr(treen));
   if(!MemoryAVLObject::checkType(treet)){
       return 0;
   }
   if(!nl->Equal(nl->Second(treet),subtype)){
      return 0;
   }
   MemoryAVLObject* mao = (MemoryAVLObject*)catalog->getMMObject(treen);
   return mao->getAVLtree();
}

template<class T>
memAVLtree* getAVLtree(T* treeN){
   if(!treeN->IsDefined()){
      return 0;
   }
   string treen = treeN->GetValue();
   if(!catalog->isMMObject(treen) || !catalog->isAccessible(treen)){
      return 0;
   }
   ListExpr treet = nl->Second(catalog->getMMObjectTypeExpr(treen));
   if(!MemoryAVLObject::checkType(treet)){
       return 0;
   }
   MemoryAVLObject* mao = (MemoryAVLObject*)catalog->getMMObject(treen);
   return mao->getAVLtree();
}

template<>
memAVLtree* getAVLtree(MPointer* tree, ListExpr subtype){
  MemoryObject* ptr = (*tree)();
  return ((MemoryAVLObject*)ptr)->getAVLtree();
}

template<>
memAVLtree* getAVLtree(MPointer* tree){
  MemoryObject* ptr = (*tree)();
  return ((MemoryAVLObject*)ptr)->getAVLtree();
}


template<class T>
MemoryRelObject* getMemRel(T* relN){

  if(!relN->IsDefined()) { return 0; }
  string reln = relN->GetValue();
  if(!catalog->isMMObject(reln) || !catalog->isAccessible(reln)) {
    return 0;
  }
  ListExpr relType = nl->Second(catalog->getMMObjectTypeExpr(reln));
    
  if(!Relation::checkType(relType)) { return 0; }
  
  return (MemoryRelObject*) catalog->getMMObject(reln);
}

template<>
MemoryRelObject* getMemRel( MPointer* rel){
   return (MemoryRelObject*) ((*rel)());
}



template<class T>
MemoryRelObject* getMemRel(T* relN, ListExpr tupleType, string& error) {
  if(!relN->IsDefined()) { 
     error = "Relation name not defined";
     return 0;
  }
  
  string reln = relN->GetValue();
  if(!catalog->isMMObject(reln) || !catalog->isAccessible(reln)){
    error ="relation '"+reln+"' not known or not accessible";
    return 0;
  }
  ListExpr relType = nl->Second(catalog->getMMObjectTypeExpr(reln));
    
  if(!Relation::checkType(relType)){
     error = "main memory object '"+reln+"' is not a relation";
     return 0;
  }
  if(!nl->Equal(nl->Second(relType), tupleType)){
     error = string("memory relation has an unexpected tuple type \n")
             + "expected: " + nl->ToString(tupleType) +"\n"
             + "found   : " + nl->ToString(nl->Second(relType));
     return 0;
  }
  error="";
  return (MemoryRelObject*) catalog->getMMObject(reln);
}


template<>
MemoryRelObject* getMemRel(MPointer* rel, ListExpr tupleType, string& error) {
   return (MemoryRelObject*) ((*rel)());
}



template<class T>
MemoryRelObject* getMemRel(T* relN, ListExpr tupleType) {
  string dummy;
  return getMemRel(relN, tupleType, dummy); 
}

template<>
MemoryRelObject* getMemRel(MPointer* rel, ListExpr tupleType) {
  string dummy;
  return getMemRel(rel, tupleType, dummy); 
}

template<class T>
MemoryORelObject* getMemORel(T* relN, ListExpr tupleType){
  if(!relN->IsDefined()){
     return 0;
  }
  string reln = relN->GetValue();
  if(!catalog->isMMObject(reln) || !catalog->isAccessible(reln)){
    return 0;
  }
  ListExpr relType = nl->Second(catalog->getMMObjectTypeExpr(reln));

  if(!listutils::isOrelDescription(relType)){
     return 0;
  }
  return (MemoryORelObject*) catalog->getMMObject(reln);
}

template<>
MemoryORelObject* getMemORel(MPointer* rel, ListExpr tupleType){
  return (MemoryORelObject*) ((*rel)());
}


template<class T>
MemoryGraphObject* getMemGraph(T* aN){
  if(!aN->IsDefined()){
     return 0;
  }
  string an = aN->GetValue();
  if(!catalog->isMMObject(an) || !catalog->isAccessible(an)){
    return 0;
  }
  ListExpr type = nl->Second(catalog->getMMObjectTypeExpr(an));
  if(!MemoryGraphObject::checkType(type)){
    return 0;
  }
  
  return (MemoryGraphObject*) catalog->getMMObject(an);
}

template<>
MemoryGraphObject* getMemGraph(MPointer* a){
  return (MemoryGraphObject*) ((*a)());
}

template<class T>
MemoryAttributeObject* getMemAttribute(T* aN, ListExpr _type){
  if(!aN->IsDefined()){
     return 0;
  }
  string an = aN->GetValue();
  if(!catalog->isMMObject(an) || !catalog->isAccessible(an)){
    return 0;
  }
  ListExpr type = nl->Second(catalog->getMMObjectTypeExpr(an));
  if(!Attribute::checkType(type)){
    return 0;
  }
  if(!nl->Equal(type,_type)){
    return 0;
  }
  return (MemoryAttributeObject*) catalog->getMMObject(an);
}

template<>
MemoryAttributeObject* getMemAttribute(MPointer* a, ListExpr _type){
  return (MemoryAttributeObject*) ((*a)());
}

template<class T, int dim>
MemoryRtreeObject<dim>* getRtree(T* tN){
  if(!tN->IsDefined()){
    return 0;
  }
  string tn = tN->GetValue();
  if(!catalog->isMMObject(tn)){ 
    // because we store only rectangle, we do'nt have to check
    // accessibility
    return 0;
  }
  ListExpr type = nl->Second(catalog->getMMObjectTypeExpr(tn));
  if(!rtreehelper::checkType(type,dim)){
    return 0;
  }
  return (MemoryRtreeObject<dim>*) catalog->getMMObject(tn);
}

template<int dim>
MemoryRtreeObject<dim>* getRtree(MPointer* t){
  return (MemoryRtreeObject<dim>*) ((*t)());
}


template<class T, class K>
MemoryMtreeObject<K, StdDistComp<K> >* getMtree(T* name){
  if(!name->IsDefined()){
    return 0;
  }
  string n = name->GetValue();
  if(!catalog->isMMObject(n)){
    return 0;
  } 
  ListExpr type = nl->Second(catalog->getMMObjectTypeExpr(n));
  if(!MemoryMtreeObject<K, StdDistComp<K> >::checkType(type)){
    return 0;
  }
  if(!K::checkType(nl->Second(type))){
     return 0;
  }
  return (MemoryMtreeObject<K, StdDistComp<K> >*) 
          catalog->getMMObject(n);
}


template<class K>
MemoryMtreeObject<K, StdDistComp<K> >* getMtree(MPointer* t){
   return (MemoryMtreeObject<K, StdDistComp<K> >*) ((*t)());
}

template<class T>
MemoryTTreeObject* getTtree(T* treeN){
   if(!treeN->IsDefined()){
      return 0;
   }
   string treen = treeN->GetValue();
   if(!catalog->isMMObject(treen) || !catalog->isAccessible(treen)){
     return 0;
   }
   ListExpr treet = nl->Second(catalog->getMMObjectTypeExpr(treen));
   if(!MemoryTTreeObject::checkType(treet)){
     return 0;
   }
   return (MemoryTTreeObject*)catalog->getMMObject(treen);
}

template<>
MemoryTTreeObject* getTtree(MPointer* tree){
  return (MemoryTTreeObject*) ((*tree)());
}


/*
Function returning the current database name.

*/

string getDBname() {
    SecondoSystem* sys = SecondoSystem::GetInstance();
    return sys->GetDatabaseName();
}

/*
This function extract the type information from a 
memory object. If the type is given as mem object,
just this type is set to be the result. Otherwise type
has to be a CcString having the given value.
In case of an error (missing objects or whatever), the
result will be false and some errorsmessage is set. 
In the other case, the reuslt is true, errMsg is keept unchanged
and result will have the type in the memory.

*/
bool getMemType(ListExpr type, ListExpr value, 
                ListExpr & result, string& error, 
                bool allowMPointer=false){
    if(allowMPointer){
      if(MPointer::checkType(type)){
         if(Mem::checkType(nl->Second(type))){
            result = nl->Second(type);
            return true;
         }
      }
    }
    if(Mem::checkType(type)){
        result = type;
        return true;
    }

    if(!CcString::checkType(type)){
       error = "not of type mem or string";
       return false;
    }
    if(nl->AtomType(value)!=StringType){
       error="only constant strings are supported";
       return false;
    }
    string n = nl->StringValue(value);
    if(!catalog->isMMObject(n)){
       error = n + " is not a memory object";
       return false;
    }
    if(!catalog->isAccessible(n)){
       error = n + " is not accessible";
       return false;
    }
    result = catalog->getMMObjectTypeExpr(n);
    if(!Mem::checkType(result)){
      error = "internal error: memory object " + n + " not of type mem";
      return false;
    }
    return true;
}


/*
some functions related to strings.

*/

string rtrim(string s, const string& delim = " \t\r\n")
{
  string::size_type last = s.find_last_not_of(delim.c_str());
  return last == string::npos ? "" : s.erase(last + 1);
}

string ltrim(string s, const string& delim = " \t\r\n")
{
  return s.erase(0, s.find_first_not_of(delim.c_str()));
}

string trim(string s, const string& delim = " \t\r\n")
{
  return ltrim(rtrim(s, delim), delim);
}



/*
2 Operator ~memload~

This operator transfers a DB object into the main memory.
The object can be of type rel, orel or must be in kind DATA.


The operator supports two variants. One variant loads also the 
flobs into the memory, the other variant does not touches the flobs.
The next function supports both variants.

*/
int memload(Word* args, Word& result,
            int message, Word& local, Supplier s, bool flob) {

    result  = qp->ResultStorage(s);
    CcBool* b = static_cast<CcBool*>(result.addr);

    CcString* oN = (CcString*) args[0].addr;
    if(!oN->IsDefined()){
       b->Set(true,false);
       return 0;
    }

    string objectName = oN->GetValue();
    if(catalog->isMMObject(objectName)){
       // name already exist
       b->Set(true,false);
       return 0;
    }

    SecondoCatalog* cat = SecondoSystem::GetCatalog();
    bool memloadSucceeded=false;
    Word object; //save the persistent object
    ListExpr objectTypeExpr=0; //type expression of the persistent object
    string objectTypeName=""; //used by cat->GetObjectTypeExpr
    bool defined = false;
    bool hasTypeName = false;

    memloadSucceeded = cat->GetObjectExpr(
                objectName,
                objectTypeName,
                objectTypeExpr,
                object,
                defined,
                hasTypeName);

     if(!memloadSucceeded){
       // object not found
       b->Set(true,false);
       return 0;
     }

    // object is a relation
    if(Relation::checkType(objectTypeExpr)&&defined) {
        GenericRelation* r= static_cast<Relation*>( object.addr );
        MemoryRelObject* mmRelObject = new MemoryRelObject();
        memloadSucceeded = mmRelObject->relToVector(
                                          r,
                                          objectTypeExpr,
                                          getDBname(), flob);
        if (memloadSucceeded) {
            catalog->insert(objectName,mmRelObject);
        } else {
            delete mmRelObject;
        }
        delete r;

    // object is an ordered relation
    } else if(listutils::isOrelDescription(objectTypeExpr)&&defined) {
        GenericRelation* r= static_cast<OrderedRelation*>(object.addr);
        MemoryORelObject* mmORelObject = new MemoryORelObject();
        memloadSucceeded = mmORelObject->relToTree(
                                          r,
                                          objectTypeExpr,
                                          getDBname(), flob);
        if (memloadSucceeded) {
            catalog->insert(objectName,mmORelObject);
        } else {
            delete mmORelObject;
        }
        delete r;
        
    } else if (Attribute::checkType(objectTypeExpr)&&defined){
        Attribute* attr = (Attribute*)object.addr;
        MemoryAttributeObject* mmA = new MemoryAttributeObject();
        memloadSucceeded = mmA->attrToMM(attr, objectTypeExpr,
                                getDBname(),flob);
        if (memloadSucceeded){
          catalog->insert(objectName, mmA);
        }
        else {
          delete mmA;
        }
        attr->DeleteIfAllowed();
    } else {
       // only attributes and relations are supported
       b->Set(true,false);
       return 0;
    }

    b->Set(true, memloadSucceeded);
    return 0;
}



/*

5 Creating Operators

5.1 Operator ~memload~

Load a persistent relation into main memory. If there is not enough space
it breaks up. The created ~MemoryRelObject~ or ~MemoryORelOBject~
is usable but not complete.

5.1.1 Type Mapping Functions of operator ~memload~ (string -> bool)


*/
ListExpr memloadTypeMap(ListExpr args) {

    if (nl->ListLength(args)!=1){
        return listutils::typeError("wrong number of arguments");
    }
    ListExpr arg1 = nl->First(args);
    if (!CcString::checkType(nl->First(arg1))) {
        return listutils::typeError("string expected");
    }
    //check for database object
    ListExpr str = nl->Second(arg1);
    string objectName = nl->StringValue(str);
    SecondoCatalog* cat = SecondoSystem::GetCatalog();

    if (!cat->IsObjectName(objectName)){
        return listutils::typeError("identifier is not in use");
    }

    //already main memory object?
    if (catalog->isMMObject(objectName)){
        return listutils::typeError("identifier is "
        " already used for a main memory object");
    }
    return listutils::basicSymbol<CcBool>();
}


/*
5.1.3  The Value Mapping Functions of operator ~memload~

*/
int memloadValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    return memload(args, result, message, local, s, false);

}


/*

5.1.4 Description of operator ~memload~

*/
OperatorSpec memloadSpec(
    "string -> bool",
    "memload(_)",
    "loads a persistent object to main memory (without flobs) "
    "if there is not enough space, the loaded object may be not complete "
    "but usable",
    "query memload('plz')"
);

/*

5.1.5 Instance of operator ~memload~

*/

Operator memloadOp (
    "memload",
    memloadSpec.getStr(),
    memloadValMap,
    Operator::SimpleSelect,
    memloadTypeMap
);

/*
5.2 Operator ~memloadflob~

Like ~memload~ but loads also the flob part into the main memory

*/

/*
5.2.3  The Value Mapping Functions of operator ~memloadflob~

*/


int memloadflobValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    return memload(args, result, message, local, s, true);

}


/*

5.2.4 Description of operator ~memloadflob~

*/

OperatorSpec memloadflobSpec(
    "string -> bool",
    "memloadflob(_)",
    "loads a persistent object together with the associated flobs to  "
    "main memory. If there is not enough space, the loaded object "
    "may be not complete but usable",
    "query memloadflob('Trains')"
);

/*

5.2.5 Instance of operator ~memloadflob~

*/

Operator memloadflobOp (
    "memloadflob",
    memloadflobSpec.getStr(),
    memloadflobValMap,
    Operator::SimpleSelect,
    memloadTypeMap
);

/*
5.3 Operator ~meminit~

Initialises the main memory which is used within the main memory algebra.
The default value is 256MB.
The maximum value is limited by the value set in ~SecondoConfig.ini~.
If the wanted value is smaller then the memory that is already in use,
the value will be set to the smallest possible value
without deleting any main memory objects.

*/

/*
5.3.1 Type Mapping Functions of operator ~meminit~ (int->int)

*/

ListExpr meminitTypeMap(ListExpr args)
{
  if(nl->ListLength(args)!=1){
     return listutils::typeError("wrong number of arguments");
  }
  if (!CcInt::checkType(nl->First(args))) {
  return listutils::typeError("int expected");
  }
  return listutils::basicSymbol<CcInt>();
}


/*

5.3.3  The Value Mapping Functions of operator ~meminit~

*/

int meminitValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    int maxSystemMainMemory = qp->GetMemorySize(s);
    int newMainMemorySize = ((CcInt*)args[0].addr)->GetIntval();
    int res=0;

    if (newMainMemorySize<0){
        res=catalog->getMemSizeTotal();
    }
    else if ((double)newMainMemorySize <
                    (catalog->getUsedMemSize()/1024.0/1024.0)){
            res = (catalog->getUsedMemSize()/1024.0/1024.0)+1;
            catalog->setMemSizeTotal
                        (catalog->getUsedMemSize()/1024.0/1024.0+1);
        }
    else if (newMainMemorySize>maxSystemMainMemory){
            res = maxSystemMainMemory;
            catalog->setMemSizeTotal(maxSystemMainMemory);
    }
    else {
        res = newMainMemorySize;
        catalog->setMemSizeTotal(newMainMemorySize);
    }

    result  = qp->ResultStorage(s);
    CcInt* b = static_cast<CcInt*>(result.addr);
    b->Set(true, res);
    return 0;
}

/*

5.3.4 Description of operator ~meminit~

*/

OperatorSpec meminitSpec(
    "int -> int",
    "meminit(_)",
    "initialises the main memory, the maximum size "
    " is limited by the global memory which is set in SecondoConfig.ini",
    "query meminit(256)"
);

/*

5.3.5 Instance of operator ~meminit~

*/

Operator meminitOp (
    "meminit",
    meminitSpec.getStr(),
    meminitValMap,
    Operator::SimpleSelect,
    meminitTypeMap
);


/*
5.4 Operator ~mfeed~

~mfeed~ produces a stream of tuples from a main memory relation or
ordered relation, similar to the ~feed~-operator

5.4.1 Type Mapping Functions of operator ~mfeed~ (string -> stream(Tuple))

*/

ListExpr mfeedTypeMap(ListExpr args) {

    if(nl->ListLength(args)!=1){
        return listutils::typeError("wrong number of arguments");
    }

    ListExpr arg = nl->First(args);
    
    if(!nl->HasLength(arg,2)){
        return listutils::typeError("internal error");
    }
    string errMsg;

    if(!getMemType(nl->First(arg), nl->Second(arg), arg, errMsg)){
      return listutils::typeError("string or mem(rel) expected : " + errMsg);
    }

    arg = nl->Second(arg); // remove mem
    if((!Relation::checkType(arg)) && (!listutils::isOrelDescription(arg))){
      return listutils::typeError(
        "memory object is not a relation or ordered relation");
    }
    return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                           nl->Second(arg));
}


/*

5.4.3  The Value Mapping Functions of operator ~mfeed~

*/


class mfeedInfo{
  public:
    mfeedInfo(vector<Tuple*>* _rel) : rel(_rel), orel(0), pos(0) { }
    
    mfeedInfo(ttree::TTree<TupleWrap,TupleComp>* _orel) : 
         rel(0),orel(_orel),pos(0) {
      iter = orel->begin();
    }
    
    ~mfeedInfo() {}

    Tuple* next() {
      Tuple* res;
      if(rel){
        while(pos < rel->size()){
          res = (*rel)[pos];
          pos++;
          if(res){
            res->SetTupleId(pos);
            res->IncReference();
            return res;
          }
        }  
        return 0;
      } else {
        while(!iter.end()) {
          res = (*iter).getPointer();
          iter++;
          if(res){
            res->IncReference();
            return res;
          }
        }
        return 0;
      }
   }

  private:
     vector<Tuple*>* rel;
     ttree::TTree<TupleWrap,TupleComp>* orel;
     size_t pos;
     ttree::Iterator<TupleWrap,TupleComp> iter;
};


template<class T>
int mfeedValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

  mfeedInfo* li = (mfeedInfo*) local.addr;

  switch (message) {
    
    case OPEN: {
      
      if(li) {
        delete li;
        local.addr=0;
      }
      
      T* oN = (T*) args[0].addr;
      
      ListExpr type = catalog->getMMObjectTypeExpr(oN->GetValue());
      
      // check for rel
      if(Relation::checkType(nl->Second(type))) {  
        MemoryRelObject* rel = getMemRel(oN);
        if(!rel) {
          return 0;
        }
        local.addr= new mfeedInfo(rel->getmmrel());
        return 0;
      } else if(listutils::isOrelDescription(nl->Second(type))) { 
        MemoryORelObject* orel = getMemORel(oN, nl->Second(qp->GetType(s)));
        if(!orel) {
          return 0;
        }
        local.addr= new mfeedInfo(orel->getmmorel());
        return 0;
      }
    }

    case REQUEST:
        result.addr=li?li->next():0;
        return result.addr?YIELD:CANCEL;

    case CLOSE:
        if(li) {
          delete li;
          local.addr = 0;
        }
        return 0;
  }

  return -1;
}


ValueMapping mfeedVM[] = {
  mfeedValMap<CcString>,
  mfeedValMap<Mem>
};

int mfeedSelect(ListExpr args) {
  return CcString::checkType(nl->First(args))?0:1;
}


/*

5.4.4 Description of operator ~mfeed~


*/

OperatorSpec mfeedSpec(
    "{string, mem(rel(tuple(x))}  -> stream(Tuple)",
    "_ mfeed",
    "produces a stream from a main memory (ordered) relation",
    "query 'ten' mfeed"
);

/*

5.4.5 Instance of operator ~mfeed~

*/

Operator mfeedOp (
    "mfeed",
    mfeedSpec.getStr(),
    2,
    mfeedVM,
    mfeedSelect,
    mfeedTypeMap
);


/*
5.5 Operator gettuples

This operator gets a stream of tuple ids, 
and a main memory relation and returns the
tuples from the ids out of the relation.

*/
ListExpr gettuplesTM(ListExpr args){
  string err ="stream(X) x {string, mem(rel(tuple))} expected";
  if(!nl->HasLength(args,2)){
    return listutils::typeError(err + " (wrong number of args)");
  }
  ListExpr a2 = nl->Second(args);
  string errMsg;
  if(!getMemType(nl->First(a2), nl->Second(a2), a2, errMsg)){
     return listutils::typeError(err + "\nproblem in 2nd arg: " + errMsg);
  }
  a2 = nl->Second(a2);
  if(!Relation::checkType(a2)){
    return listutils::typeError(err + " (second arg is not a "
                                "memory relation.)");
  }
  ListExpr a1 = nl->First(nl->First(args));
  if(Stream<TupleIdentifier>::checkType(a1)){
     return nl->TwoElemList( listutils::basicSymbol<Stream<Tuple> >(),
                             nl->Second(a2));
  }
  // second variant accepts a stream of tuples containing a tupleID
  if(!Stream<Tuple>::checkType(a1)){
    return listutils::typeError("1st arg is neither a stream of "
                                "tids nor a stream of tuples");
  }
  ListExpr attrList = nl->Second(nl->Second(a1));
  int index = 0;
  int tidIndex = 0;
  ListExpr iattr = nl->TheEmptyList();
  ListExpr last;
  set<string> usednames;
  bool first = true;
  

  while(!nl->IsEmpty(attrList)){
    ListExpr attr = nl->First(attrList); 
    attrList = nl->Rest(attrList);
    index++;
    if(TupleIdentifier::checkType(nl->Second(attr))){
       if(tidIndex!=0){
          return listutils::typeError("incoming tuple stream contains "
                                      "more than one tid attribute");
       }
       tidIndex = index; 
    } else {
      if(first){
         iattr = nl->OneElemList(attr);
         last = iattr;
         first = false;
      } else {
         last = nl->Append(last, iattr); 
      }
    }
  }  
  if(!tidIndex){
     return listutils::typeError("incoming tuple stream does not "
                                 "contains an attribute of type tid");
  }
  ListExpr relAttrList = nl->Second(nl->Second(a2));
  ListExpr resAttrList = listutils::concat(iattr, relAttrList);
  if(!listutils::isAttrList(resAttrList)){
     return listutils::typeError("found name conflicts");
  }

  return nl->ThreeElemList(nl->SymbolAtom(Symbols::APPEND()),
                           nl->OneElemList(nl->IntAtom(tidIndex-1)),
                           nl->TwoElemList(
                               listutils::basicSymbol<Stream<Tuple> >(),
                               nl->TwoElemList(
                                   listutils::basicSymbol<Tuple>(),
                                   resAttrList)));
}


class gettuplesInfo{
  public:
    gettuplesInfo(Word _stream, MemoryRelObject* _rel): 
        stream(_stream), rel(_rel->getmmrel()){
      stream.open();
    }

    ~gettuplesInfo(){
        stream.close();
     }

     Tuple* next(){
        TupleIdentifier* tid;
        while((tid=stream.request())){
           if(!tid->IsDefined()){
             tid->DeleteIfAllowed();
           } else {
              TupleId id = tid->GetTid();
              tid->DeleteIfAllowed();
              if(id>0 && id<=rel->size()){
                 Tuple* res = (*rel)[id-1];
                 if(res){ // ignore deleted tuples
                    res->IncReference();
                    res->SetTupleId(id);
                    return res;
                 } else {
                    cout << "ignore deleted tuple" << endl;
                 }
              } else {
                 cout << "ignore id " << id << endl;
              }
           }
        }
        return 0;
     }

  private:
     Stream<TupleIdentifier> stream;
     vector<Tuple*>* rel;
};

template<class T>
int gettuplesVMT (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    gettuplesInfo* li = (gettuplesInfo*) local.addr;
    switch(message){
         case OPEN :{
             if(li){
                delete li;
                local.addr = 0;
             }
             T* n = (T*) args[1].addr;
             MemoryRelObject* rel = getMemRel(n,nl->Second(qp->GetType(s)));
             if(!rel){
               return 0;
             }
             local.addr = new gettuplesInfo(args[0], rel);
             return 0; 

         }
         case REQUEST: 
                result.addr = li?li->next():0;
                return result.addr?YIELD:CANCEL;
         case CLOSE: {
               if(li){
                 delete li;
                 local.addr = 0;
               }
               return 0;
         }
    }    
    return -1;
}


class gettuplesInfoUnwrap{
  public:
    gettuplesInfoUnwrap(Word _stream, MemoryRelObject* _rel, int _tidpos,
                        ListExpr resTupleType): 
        stream(_stream), rel(_rel->getmmrel()), tidpos(_tidpos){
      stream.open();
      tt = new TupleType(resTupleType);
    }

    ~gettuplesInfoUnwrap(){
        stream.close();
        tt->DeleteIfAllowed();
     }

     Tuple* next(){
        Tuple* inTuple;
        while((inTuple=stream.request())){
           TupleIdentifier* tid = (TupleIdentifier*) 
                                  inTuple->GetAttribute(tidpos);
           if(!tid->IsDefined()){
              inTuple->DeleteIfAllowed();
           } else {
              TupleId id = tid->GetTid();
              if(id<1 || id >= rel->size()){
                inTuple->DeleteIfAllowed();
              } else {
                Tuple* relTuple = rel->at(id-1);
                if(!relTuple){
                   inTuple->DeleteIfAllowed();
                } else {
                  Tuple* resTuple = createResTuple(inTuple,relTuple);
                  inTuple->DeleteIfAllowed();
                  return resTuple;
                }
              }
           }
        }
        return 0;
     }

  private:
     Stream<Tuple> stream;
     vector<Tuple*>* rel;
     int tidpos;
     TupleType* tt;

     Tuple* createResTuple(Tuple* inTuple, Tuple* relTuple){
        Tuple* resTuple = new Tuple(tt);
        // copy attributes before tid from intuple to restuple
        for(int i=0; i < tidpos; i++){
           resTuple->CopyAttribute(i,inTuple,i);
        }
        // copy attributes after tidpos from intuple to restuple
        for(int i=tidpos+1;i<inTuple->GetNoAttributes(); i++){
           resTuple->CopyAttribute(i,inTuple,i-1);
        }
        // copy attributes from relTuple to resTuple
        for(int i=0;i<relTuple->GetNoAttributes(); i++){
           resTuple->CopyAttribute(i, relTuple, 
                                   i + inTuple->GetNoAttributes()-1);
        }
        return resTuple;
     }
};


template<class T>
int gettuplesUnwrapVMT (Word* args, Word& result,
                        int message, Word& local, Supplier s) {

    gettuplesInfoUnwrap* li = (gettuplesInfoUnwrap*) local.addr;
    switch(message){
         case OPEN :{
             if(li){
                delete li;
                local.addr = 0;
             }
             T* n = (T*) args[1].addr;
             MemoryRelObject* rel = getMemRel(n,nl->Second(qp->GetType(s)));
             if(!rel){
               return 0;
             }
             int tidpos = ((CcInt*)args[2].addr)->GetValue();
             ListExpr tt = nl->Second(GetTupleResultType(s));
             local.addr = new gettuplesInfoUnwrap(args[0], rel,tidpos,tt);
             return 0; 

         }
         case REQUEST: 
                result.addr = li?li->next():0;
                return result.addr?YIELD:CANCEL;
         case CLOSE: {
               if(li){
                 delete li;
                 local.addr = 0;
               }
               return 0;
         }
    }    
    return -1;
}



ValueMapping gettuplesVM[] =  {
   gettuplesVMT<CcString>,
   gettuplesVMT<Mem>,
   gettuplesUnwrapVMT<CcString>,
   gettuplesUnwrapVMT<Mem>

};

int gettuplesSelect(ListExpr args){
  int n2 = CcString::checkType(nl->Second(args))?0:1;
  int n1 = Stream<TupleIdentifier>::checkType(nl->First(args))?0:2;
  return n1+n2;
}

OperatorSpec gettuplesSpec(
  "stream({tid,tuple}) x {string, mem(rel(tuple(X)))} -> stream(tuple(X))",
  "_ _ gettuples",
  "Retrieves tuples from a main memory relation whose ids are "
  "specified in the incoming stream. If the incoming stream is a tuple stream,"
  "exactly one attribute must be of type tid. Thius attribute is used to "
  "extract the tuples from the main memory relation. The tid-attribute is "
  "removed from the incoming stream and the tuple from the relation is "
  "appended to the remaining tuples",
  "query \"strassen_Name\" mdistScan2[\"str\"] \"strassen\" gettuples consume"
);

Operator gettuplesOp(
  "gettuples",
  gettuplesSpec.getStr(),
  4,
  gettuplesVM,
  gettuplesSelect,
  gettuplesTM
);


template<bool flob>
int letmconsumeValMap(Word* args, Word& result,
                int message, Word& local, Supplier s) {

    result  = qp->ResultStorage(s);
    Mem* str = (Mem*)result.addr;

    Supplier t = qp->GetSon( s, 0 );
    ListExpr le = qp->GetType(t);
    bool succeed;
    CcString* oN = (CcString*) args[1].addr;
    if(!oN->IsDefined()){
        str->SetDefined(false);
        return 0;
    }
    string res = oN->GetValue();
    if(catalog->isMMObject(res)){
        str->SetDefined(false);
        return 0;
    };

    MemoryRelObject* mmRelObject = new MemoryRelObject();
    succeed = mmRelObject->tupleStreamToRel(args[0],
        le, getDBname(), flob);
    if (succeed) {
        catalog->insert(res,mmRelObject);
    } else {
        delete mmRelObject;
    }
    str->set(succeed,res);

   return 0;
}


/*

5.5 Operator ~letmconsume~

~letmconsume~ produces a main memory relation from a stream(tuples),
similar to the ~consume~-operator. The name of the main memory relation 
is given by the second parameter.

*/

/*

5.5.1 Type Mapping Functions of operator ~letmconsume~
        (stream(Tuple) x string -> mem(rel(tuple(X))))

*/
ListExpr letmconsumeTypeMap(ListExpr args)
{
    if(nl->ListLength(args)!=2){
        return listutils::typeError("(wrong number of arguments)");
    }

    if (!Stream<Tuple>::checkType(nl->First(args))
        || !CcString::checkType(nl->Second(args)) ) {
        return listutils::typeError ("stream(Tuple) x string expected!");
    }
    return nl->TwoElemList(
                listutils::basicSymbol<Mem>(),
                nl->TwoElemList(
                    listutils::basicSymbol<Relation>(),
                    nl->Second(nl->First(args))));
}


/*

5.5.4 Description of operator ~letmconsume~

*/

OperatorSpec letmconsumeSpec(
    "stream(Tuple) x string -> mem(rel(tuple(X)))",
    "_ letmconsume [_]",
    "produces a main memory relation from a stream(Tuple)",
    "query ten feed letmconsume ['zehn']"
);



/*

5.5.5 Instance of operator ~letmconsume~

*/

Operator letmconsumeOp (
    "letmconsume",
    letmconsumeSpec.getStr(),
    letmconsumeValMap<false>,
    Operator::SimpleSelect,
    letmconsumeTypeMap
);


/*

5.5.4 Description of operator ~letmconsume~

*/

OperatorSpec letmconsumeflobSpec(
    "stream(Tuple) x string -> mem(relType(tuple(X)))",
    "_ letmconsumeflob [_]",
    "produces a main memory relation from a stream(Tuple)"
    "and load the associated flobs",
    "query trains feed letmconsumeflob ['trains1']"
);



/*

5.5.5 Instance of operator ~letmconsumeflob~

*/

Operator letmconsumeflobOp (
    "letmconsumeflob",
    letmconsumeflobSpec.getStr(),
    letmconsumeValMap<true>,
    Operator::SimpleSelect,
    letmconsumeTypeMap
);

/*
5.6 Operator ~memdelete~

~memdelete~ deletes an object from main memory

5.6.1 Type Mapping Functions of operator ~memdelete~ (string -> bool)

*/
ListExpr memdeleteTypeMap(ListExpr args)
{
 if(nl->ListLength(args)!=1){
        return listutils::typeError("wrong number of arguments");
    }

    ListExpr arg = nl->First(args);

    if(!nl->HasLength(arg,2)){
        return listutils::typeError("internal error");
    }

    if (!CcString::checkType(nl->First(arg))) {
        return listutils::typeError("string expected");
    };

    ListExpr fn = nl->Second(arg);

    if(nl->AtomType(fn)!=StringType){
        return listutils::typeError("error");
    }

    string oN = nl->StringValue(fn);

    if(!catalog->isMMObject(oN))
    {
      return listutils::typeError("not a MainMemory member");
    }
     return listutils::basicSymbol<CcBool>();
}


/*

5.6.3  The Value Mapping Functions of operator ~memdelete~

*/


int memdeleteValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {
    result  = qp->ResultStorage(s);
    CcBool* b = static_cast<CcBool*>(result.addr);
    bool deletesucceed = false;
    CcString* oN = (CcString*) args[0].addr;
    if(!oN->IsDefined()){
            b->Set(true, deletesucceed);
            return 0;
        }
    string objectName = oN->GetValue();
    deletesucceed = catalog->deleteObject(objectName);

    b->Set(true, deletesucceed);
    return 0;

}


/*

5.6.4 Description of operator ~memdelete~

Similar to the ~property~ function of a type constructor, an operator needs to
be described, e.g. for the ~list operators~ command.  This is now done by
creating a subclass of class ~OperatorInfo~.

*/

OperatorSpec memdeleteSpec(
    "string -> bool",
    "memdelete (_)",
    "deletes a main memory object",
    "query memdelete ('ten')"
);



/*

5.6.5 Instance of operator ~memdelete~

*/

Operator memdeleteOp (
    "memdelete",
    memdeleteSpec.getStr(),
    memdeleteValMap,
    Operator::SimpleSelect,
    memdeleteTypeMap
);


/*
5.7 Operator ~memobject~

~memobject~ gets a name of a main memory object and return a persistent version

5.7.1 Type Mapping Functions of operator ~memobject~ (string -> m:MEMLOADABLE)

*/
ListExpr memobjectTypeMap(ListExpr args) {
    if(nl->ListLength(args)!=1){
        return listutils::typeError("wrong number of arguments");
    }
    ListExpr arg1 = nl->First(args);

    if(!nl->HasLength(arg1,2)){
        return listutils::typeError("internal error");
    }

    string err = "string or mem(X) expected";
     
    string errMsg;
    if(!getMemType(nl->First(arg1), nl->Second(arg1), arg1, errMsg)){
      return listutils::typeError(err + "(" + errMsg +")");
    }
    ListExpr subtype = nl->Second(arg1);
    if(   !Attribute::checkType(subtype)
       && !Relation::checkType(subtype)){
         return listutils::typeError("only rel and DATA supported");
    }
    return subtype;
}


/*

5.7.3  The Value Mapping Functions of operator ~memobject~

*/
template<class T>
int memobjectValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {
        result = qp->ResultStorage(s);
        ListExpr resType = qp->GetType(s);
        T* oN = (T*) args[0].addr;
        if(Attribute::checkType(resType)){  // Attribute
           MemoryAttributeObject* mao = getMemAttribute(oN, resType);
           if(!mao){
              ((Attribute*) result.addr)->SetDefined(false);
              return 0;
           }
           ((Attribute*) result.addr)->CopyFrom(mao->getAttributeObject());
           return 0;
        } else if(Relation::checkType(resType)){ // relation
           GenericRelation* r = (GenericRelation*) result.addr;
            if(r->GetNoTuples() > 0) {
                r->Clear();
            }

           MemoryRelObject* mro = getMemRel(oN, nl->Second(resType));
           if(!mro){
              return 0;
           }
           vector<Tuple*>* relation = mro->getmmrel();
           vector<Tuple*>::iterator it;
           it=relation->begin();

           while( it!=relation->end()){
                Tuple* tup = *it;
                r->AppendTuple(tup);
                it++;
            }
            return 0;
        }
        assert(false); // unsupported type
        return 0;
}

/*

5.7.4 Description of operator ~memobject~

*/
OperatorSpec memobjectSpec(
    "string | mem(X)  -> m:MEMLOADABLE , X in {DATA, rel}",
    "memobject (_)",
    "returns a persistent object created from a main memory object",
    "query memobject (\"Trains100\")"
);

/*
5.7.5 Value Mapping Array and Selection

*/
ValueMapping memobjectVM[] = {
   memobjectValMap<CcString>,
   memobjectValMap<Mem>
};

int memobjectSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}

/*

5.7.5 Instance of operator ~memobject~

*/

Operator memobjectOp (
    "memobject",
    memobjectSpec.getStr(),
    2,
    memobjectVM,
    memobjectSelect,
    memobjectTypeMap
);

/*

5.8 Operator ~memgetcatalog~

Returns a ~stream(Tuple)~.
Each tuple describes one element of the main memory catalog.


5.8.1 Type Mapping Functions of operator ~memgetcatalog~ (  -> stream(Tuple) )

*/


ListExpr memgetcatalogTypeMap(ListExpr args)
{

     if(nl->ListLength(args)!=0){
        return listutils::typeError("no argument expected");
    }

    string stringlist = "(stream(tuple((TotalMB int)"
        "(UsedMB real)(Name string)"
        "(ObjectType text)(ObjSizeInB string)(ObjSizeInMB real)"
            "(Database string)(Accessible bool)(FlobsLoaded bool))))";

    ListExpr res =0;
    if(nl->ReadFromString(stringlist, res)){};
    return res;

}


/*

5.8.3  The Value Mapping Functions of operator ~memgetcatalog~

*/

class memgetcatalogInfo{
    public:

        memgetcatalogInfo(ListExpr _resultType){
        resultType = _resultType;
        memContents = catalog->getMemContent();
        it = memContents->begin();
        };
        ~memgetcatalogInfo(){}

        string long_to_string(unsigned long value) {
            std::ostringstream stream;
            stream << value;
            return stream.str();
        }

        Tuple* next(){
            if(it==memContents->end()) {
                return 0;
            }
        string name = it->first;
        MemoryObject* memobj = it->second;
        string objTyp ="nn";

        ListExpr objectType = catalog->getMMObjectTypeExpr(name);
        objTyp = nl->ToString(objectType);

        TupleType* tt = new TupleType(nl->Second(resultType));
        Tuple *tup = new Tuple( tt );
        tt->DeleteIfAllowed();

        CcInt* totalMB = new CcInt (true, catalog->getMemSizeTotal());
        CcReal* usedMB =
            new CcReal (true, (double)catalog->getUsedMemSize()/1024.0/1024.0);
        CcString* objectName = new CcString(true,name);
        FText* oT = new FText(true,objTyp);
        CcString* memSizeB = new CcString
                            (true, long_to_string(memobj->getMemSize()));
        CcReal* memSizeMB =
            new CcReal(true, (double)memobj->getMemSize()/1024.0/1024.0);
        CcString* database = new CcString(true,(string)memobj->getDatabase());
        CcBool* accessible =
                new CcBool(true, (bool)(memobj->getDatabase()==getDBname()
                    || memobj->hasflob()));
        CcBool* flobs = new CcBool(true, (bool)memobj->hasflob());

        tup->PutAttribute(0,totalMB);
        tup->PutAttribute(1,usedMB);
        tup->PutAttribute(2,objectName);
        tup->PutAttribute(3,oT);
        tup->PutAttribute(4,memSizeB);
        tup->PutAttribute(5,memSizeMB);
        tup->PutAttribute(6,database);
        tup->PutAttribute(7,accessible);
        tup->PutAttribute(8,flobs);

        it++;
        return tup;
    }
 private:

       map<string, MemoryObject*>* memContents;
       map<string, MemoryObject*>::iterator it;
       ListExpr resultType;

};


int memgetcatalogValMap (Word* args, Word& result,
            int message, Word& local, Supplier s) {


   memgetcatalogInfo* li = (memgetcatalogInfo*) local.addr;

   switch (message)
   {
        case OPEN: {
             if(li){
             delete li;
             local.addr=0;

          }
        ListExpr resultType;
        resultType = GetTupleResultType( s );
        local.addr= new memgetcatalogInfo(resultType);
        return 0;
        }

        case REQUEST:
            result.addr=(li?li->next():0);
            return result.addr?YIELD:CANCEL;


        case CLOSE:
            if(li)
            {
            delete li;
            local.addr = 0;
            }
            return 0;
   }

    return 0;

}



/*

5.8.4 Description of operator ~memgetcatalog~

*/

OperatorSpec memgetcatalogSpec(
    " -> stream(Tuple)",
    "memgetcatalog()",
    "returns a stream(Tuple) with information about  main memory objects",
    "query memgetcatalog()"
);

/*

5.8.5 Instance of operator ~memgetcatalog~

*/

Operator memgetcatalogOp (
    "memgetcatalog",
    memgetcatalogSpec.getStr(),
    memgetcatalogValMap,
    Operator::SimpleSelect,
    memgetcatalogTypeMap
);



int memlet(Word* args, Word& result,
           int message, Word& local, Supplier s, bool flob) {

    result  = qp->ResultStorage(s);
    CcBool* b = static_cast<CcBool*>(result.addr);
    bool memletsucceed = false;
    bool correct = true;

    CcString* oN = (CcString*) args[0].addr;
    if(!oN->IsDefined()){
        return 0;
    }
    string objectName = oN->GetValue();
    Supplier t = qp->GetSon( s, 1 );
    ListExpr le = qp->GetType(t);

    // relation
    if(Relation::checkType(le)) {
        GenericRelation* rel = static_cast<Relation*>(args[1].addr);
        MemoryRelObject* mmRelObject = new MemoryRelObject();
        memletsucceed = mmRelObject->relToVector(rel,le, 
                                                 getDBname(),flob);
        if(memletsucceed) 
          catalog->insert(objectName,mmRelObject);
        else 
          delete mmRelObject;
        
    }
    // main memory relation
    else if(Relation::checkType(nl->Second(le))) {
        CcString* oN = static_cast<CcString*>(args[1].addr);
        MemoryRelObject* rel = getMemRel(oN);
        if(!rel){
          cerr << "no relation with by that name in main memory catalog" 
               << endl;
          rel = new MemoryRelObject(); 
        }
        MemoryRelObject* mmRelObject = new MemoryRelObject();
        memletsucceed = mmRelObject->mmrelToVector(rel->getmmrel(),le, 
                                                   getDBname(),flob);
        if(memletsucceed) 
          catalog->insert(objectName,mmRelObject);
        else 
          delete mmRelObject;
        delete rel;
    } 
    // ordered relation
    else if(listutils::isOrelDescription(le)) {
        GenericRelation* orel= static_cast<OrderedRelation*>(args[1].addr);
        MemoryORelObject* mmORelObject = new MemoryORelObject();
        memletsucceed = mmORelObject->relToTree(orel,le,
                                                getDBname(),flob);
        if(memletsucceed) 
            catalog->insert(objectName,mmORelObject);
        else 
          delete mmORelObject;
        
    }
    // attribute
    else if(Attribute::checkType(le)) {
      Attribute* attr = (Attribute*)args[1].addr;
      MemoryAttributeObject* mmA = new MemoryAttributeObject();
      memletsucceed = mmA->attrToMM(attr, le,
                            getDBname(),flob);
      if(memletsucceed) {
        catalog->insert(objectName, mmA);
      }
      else {
        delete mmA;
      }
    } 
    // stream
    else if(Stream<Tuple>::checkType(le)){
        MemoryRelObject* mmRelObject = new MemoryRelObject();
        memletsucceed = mmRelObject->tupleStreamToRel(args[1],
                                                      le, 
                                                      getDBname(), 
                                                      flob);
        if (memletsucceed) {
            catalog->insert(objectName,mmRelObject);
        } else {
            delete mmRelObject;
        }
    } else {
      correct = false;
    }

    b->Set(correct, memletsucceed);

    return 0;

}


/*
5.9 Operator ~memlet~
creates a new main memory object. The first parameter is the
name of the new main memory object, the second is the query/the
MEMLOADABLE object from which the mm-object will be created.

5.9.1 Type Mapping Functions of operator ~memlet~

        (string X m:MEMLOADABLE -> bool)

*/
ListExpr memletTypeMap(ListExpr args)
{
    if(nl->ListLength(args)!=2){
        return listutils::typeError("wrong number of arguments");
    }
    ListExpr arg1 = nl->First(args);
    if (!CcString::checkType(nl->First(arg1))) {
        return listutils::typeError("string expected");
    }
    ListExpr str = nl->Second(arg1);
    string objectName = nl->StringValue(str);
    if (catalog->isMMObject(objectName)){
        return listutils::typeError("identifier already in use");
    }
    ListExpr arg2 = nl->Second(args);
    if(listutils::isRelDescription(nl->First(arg2)) ||
       listutils::isOrelDescription(nl->First(arg2)) ||
       listutils::isDATA(nl->First(arg2)) ||
       listutils::isTupleStream(nl->First(arg2))) {
      return listutils::basicSymbol<CcBool>();
    }
    return listutils::typeError(
             "the second argument has to "
             "be of kind DATA or a relation or an ordered relation "
             "or a tuplestream");
}


/*

5.9.3  The Value Mapping Functions of operator ~memlet~

*/

int memletValMap(Word* args, Word& result,
                 int message, Word& local, Supplier s) {

   return memlet(args, result, message, local,s, false);
}


/*

5.9.4 Description of operator ~memlet~

*/



OperatorSpec memletSpec(
    "string x m:MEMLOADABLE -> bool",
    "memlet (_,_)",
    "creates a main memory object from a given MEMLOADABLE",
    "query memlet ('Trains100', Trains feed head[100])"
);



/*

5.9.5 Instance of operator ~memlet~

*/

Operator memletOp (
    "memlet",
    memletSpec.getStr(),
    memletValMap,
    Operator::SimpleSelect,
    memletTypeMap
);

/*
5.9 Operator ~memletflob~

creates a new main memory object. The first parameter is the
name of the new main memory object, the second is the query/the
MEMLOADABLE object from which the mm-object will be created.
The associated flobs will be loaded to main memory too.

*/


/*

5.9.3  The Value Mapping Functions of operator ~memletflob~

*/

int memletflobValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {

   return memlet(args, result, message, local, s,true);
}


/*

5.9.4 Description of operator ~memletflob~

*/

OperatorSpec memletflobSpec(
    "string x MEMLOADABLE -> bool",
    "memletflob (_,_)",
    "creates a main memory object from a given MEMLOADABLE."
    "the associated flobs will be loaded",
    "query memletflob ('Trains100', Trains feed head[100])"
);



/*

5.9.5 Instance of operator ~memletflob~

*/

Operator memletflobOp (
    "memletflob",
    memletflobSpec.getStr(),
    memletflobValMap,
    Operator::SimpleSelect,
    memletTypeMap
);


/*
5.10 Operator ~memupdate~
updates a main memory object. The tuple description for a stream or a relation
must be the same as the one of the main memory object.


5.10.1 Type Mapping Functions of operator ~memupdate~
        (string x m:MEMLOADABLE -> bool)

*/
ListExpr memupdateTypeMap(ListExpr args)
{

    if(nl->ListLength(args)!=2){
        return listutils::typeError("wrong number of arguments");
    }

    ListExpr arg2 = nl->First(nl->Second(args));
    // replace <stream> by <rel> if necessary. this enables unique handling
    if(Stream<Tuple>::checkType(arg2)){
       arg2 = nl->TwoElemList( listutils::basicSymbol<Relation>(),
                               nl->Second(arg2));
    }
    string errMsg;
    ListExpr arg1 = nl->First(args);
    if(!getMemType(nl->First(arg1), nl->Second(arg1), arg1, errMsg)){
      return listutils::typeError("error in 1st arg: " + errMsg);
    }

    ListExpr subtype = nl->Second(arg1);
    if(!nl->Equal(subtype,arg2)){
       return listutils::typeError("Update type and memory type differ");
    }
    if(   !Attribute::checkType(subtype)
       && !Relation::checkType(subtype)){
       return listutils::typeError("unsupported subtype");
    }
    return  listutils::basicSymbol<CcBool>();

}


/*

5.10.3  The Value Mapping Functions of operator ~memupdate~

*/
template<class T>
int memupdateValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {

    result  = qp->ResultStorage(s);
    CcBool* b = static_cast<CcBool*>(result.addr);
    bool memupdatesucceed = false;
    bool correct = true;
    Supplier t = qp->GetSon( s, 1 );
    ListExpr le = qp->GetType(t);

    bool isDATA = Attribute::checkType(le);

    T* oN = (T*) args[0].addr;
    if(!oN->IsDefined()){
        cerr << "memupdate: undefined object name" << endl;
        if(isDATA){
           ((Attribute*) result.addr)->SetDefined(false);
        }
        return 0;
    }
    string objectName = oN->GetValue();

    // check whether object is present
    if(!catalog->isMMObject(objectName)){
        cerr << "memupdate: object not found" << endl;
        if(isDATA){
           ((Attribute*) result.addr)->SetDefined(false);
        }
        return 0;
    }

    // check whether memory object has expected type
    ListExpr expType = Stream<Tuple>::checkType(le) 
               ? nl->TwoElemList( listutils::basicSymbol<Relation>(),
                                  nl->Second(le))
               : le;

    
     ListExpr memType = catalog->getMMObjectTypeExpr(objectName);
     if(!Mem::checkType(memType)){
       cerr << "internal error: memory object not of type mem" << endl;
       return 0;
     }
     memType = nl->Second(memType);
     if(!nl->Equal(memType,expType)){
        cerr << "memupdate: object has not the expected type" << endl;
        if(isDATA){
           ((Attribute*) result.addr)->SetDefined(false);
        }
        return 0;
     }

     bool flob = catalog->getMMObject(objectName)->hasflob();
     if(Relation::checkType(le)) {
        catalog->deleteObject(objectName);
        GenericRelation* rel= static_cast<Relation*>( args[1].addr );
        MemoryRelObject* mmRelObject = new MemoryRelObject();
        memupdatesucceed = mmRelObject->relToVector(rel, le, 
                                         getDBname(), flob);
        if (memupdatesucceed) {
            catalog->insert(objectName,mmRelObject);
        } else {
            delete mmRelObject;
        }
     } else if(isDATA){
        catalog->deleteObject(objectName);
        Attribute* attr = (Attribute*)args[1].addr;
        MemoryAttributeObject* mmA = new MemoryAttributeObject();
        memupdatesucceed = mmA->attrToMM(attr, le,
                                getDBname(),flob);
        if (memupdatesucceed){
           catalog->insert(objectName, mmA);
        } else {
            delete mmA;
        }
    } else if (listutils::isTupleStream(le)) {
        catalog->deleteObject(objectName);
        MemoryRelObject* mmRelObject = new MemoryRelObject();
            memupdatesucceed = mmRelObject->tupleStreamToRel(args[1],
        le, getDBname(), flob);
        if (memupdatesucceed) {
            catalog->insert(objectName,mmRelObject);
        } else {
            delete mmRelObject;
        }
    } else {
       cerr << "memupdate: unsupported type" << endl;
       correct = false;
    }

    b->Set(correct, memupdatesucceed);

    return 0;
}

ValueMapping memupdateVM[] = {
   memupdateValMap<CcString>,
   memupdateValMap<Mem>
};


int memupdateSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}



/*

5.10.4 Description of operator ~memupdate~

*/

OperatorSpec memupdateSpec(
    "{string, mem} x m:MEMLOADABLE -> bool",
    "memupdate (_,_)",
    "updates a main memory object with a given MEMLOADABLE",
    "query memupdate ('fuenf', ten feed head[7])"
);

/*

5.10.5 Instance of operator ~memupdate~

*/

Operator memupdateOp (
    "memupdate",
    memupdateSpec.getStr(),
    2,
    memupdateVM,
    memupdateSelect,
    memupdateTypeMap
);


/*
5.10 Operator ~mcreateRtree~
creates a an mmRTree over a given main memory relation

5.10.1 Type Mapping Functions of operator ~mcreateRtree~
        (string x Ident -> string ||
         mem(rel(tuple)) x Ident -> mem(rtree dim))
        the first parameter identifies the main memory relation, the
        second parameter identifies the attribute

*/

ListExpr mcreateRtreeTypeMap(ListExpr args){

    if(nl->ListLength(args)!=2){
        return listutils::typeError("wrong number of arguments");
    }

    // Split argument in two parts
    ListExpr arg1 = nl->First(args);
    ListExpr oTE_Rel;
    string errMsg;
    if(!getMemType(nl->First(arg1), nl->Second(arg1), oTE_Rel, errMsg)){
      return listutils::typeError("problem in 1st arg:" + errMsg);
    }
    oTE_Rel = nl->Second(oTE_Rel); // remove leading mem
    if(!Relation::checkType(oTE_Rel)){
       return listutils::typeError("memory object is not a relation");
    }

    ListExpr a = nl->First(nl->Second(args));
    if(nl->AtomType(a)!=SymbolType){
       return listutils::typeError("second argument must be an identifier");
    }
    string attrName = nl->SymbolValue(a);

    ListExpr attrType = 0;
    int attrPos = 0;
    ListExpr attrList = nl->Second(nl->Second(oTE_Rel));

    attrPos = listutils::findAttribute(attrList, attrName, attrType);

    if (attrPos == 0){
        return listutils::typeError("attribute  " + attrName
           + " not present in tuple");
    }

    int dim = rtreehelper::getDimension(attrType);

    if(dim<0){
      return listutils::typeError("referenced attribute not "
                                  "in kind SPATIAL<X>D");
    }


    ListExpr resType = nl->TwoElemList(
                         listutils::basicSymbol<Mem>(),
                         nl->TwoElemList(
                             nl->SymbolAtom(rtreehelper::BasicType()),
                             nl->IntAtom(dim)));

   return nl->ThreeElemList(
               nl->SymbolAtom(Symbol::APPEND()),
               nl->ThreeElemList( nl->IntAtom(attrPos-1), 
                                nl->IntAtom(dim),
                                nl->StringAtom(attrName)),
               resType);

}

/*

5.10.3  The Value Mapping Functions of operator ~mcreateRtree~

*/

template<int dim>
bool mcreateRtree(MemoryRelObject* mmrel, 
                  int attrPos, 
                  const string& rtreeName,
                  ListExpr typeList){


    bool flob = mmrel->hasflob();
    string database = mmrel->getDatabase();
    vector<Tuple*>* relVec = mmrel->getmmrel();
    vector<Tuple*>::iterator it;
    it=relVec->begin();
    unsigned int i=1;
    mmrtree::RtreeT<dim, size_t>* rtree =
                    new mmrtree::RtreeT<dim, size_t>(4,8);

    StandardSpatialAttribute<dim>* attr=0;
    size_t usedMainMemory=0;
    unsigned long availableMemSize = catalog->getAvailableMemSize();
    while( it!=relVec->end()){
        Tuple* tup = *it;
        if(tup){
          attr=(StandardSpatialAttribute<dim>*)tup->GetAttribute(attrPos);
          if(attr->IsDefined()){
             Rectangle<dim> box = attr->BoundingBox();
             if(box.IsDefined()){
               rtree->insert(box, i);
             }
          }
          it++;
         }
         i++;
    } // end while

    usedMainMemory = rtree->usedMem();
    MemoryRtreeObject<dim>* mmRtreeObject =
        new MemoryRtreeObject<dim>(rtree, usedMainMemory,
                        nl->ToString(typeList), flob, database);

    if (usedMainMemory>availableMemSize){
        cout<<"there is not enough memory left to create the rtree";
    }
    if(    usedMainMemory<=availableMemSize
        && catalog->insert(rtreeName,mmRtreeObject)){
        return true;
    }else {
       delete mmRtreeObject;
       return false;
    }
}

template<class T>
int mcreateRtreeValMapT (Word* args, Word& result,
                int message, Word& local, Supplier s) {

    result  = qp->ResultStorage(s);
    ListExpr resType = qp->GetType(s);
    Mem* str = static_cast<Mem*>(result.addr);
    bool succeed = false;

    // the main memory relation
    T* roN = (T*) args[0].addr;
    if(!roN->IsDefined()){
        str->SetDefined(false);
        return 0;
    }
    string relObjectName = roN->GetValue();
    if(!catalog->isMMObject(relObjectName)){
       cerr << "memory object " << relObjectName << " not found" << endl;
       str->SetDefined(false);
       return 0;
    }
    ListExpr memObjectType = catalog->getMMObjectTypeExpr(relObjectName);
    if(!Mem::checkType(memObjectType)){
       cerr << "internal error: memory object not of type mem" << endl;
       str->SetDefined(false);
       return 0;
    }
    memObjectType = nl->Second(memObjectType);
    if(!Relation::checkType(memObjectType)){
       cerr << "memory object is not a relation" << endl;
       str->SetDefined(false);
       return 0;
    }

    ListExpr attrList = nl->Second(nl->Second(memObjectType));
    int attrPos = ((CcInt*) args[2].addr)->GetValue();
    int dim = ((CcInt*) args[3].addr)->GetValue();
    while(!nl->IsEmpty(attrList) && attrPos > 0){
       attrList = nl->Rest(attrList);
       attrPos--;
    }
    if(nl->IsEmpty(attrList)){
       cerr << "Attribute not present" << endl;
       str->SetDefined(false);
       return 0;
    }
    attrPos = ((CcInt*) args[2].addr)->GetValue();
    ListExpr attr = nl->Second(nl->First(attrList));
    int dim2 = rtreehelper::getDimension(attr);
    if(dim!=dim2){
      cerr << "dimension change between type mapping and value mapping" 
           << endl;
      str->SetDefined(false);
      return 0;
    }
     
    MemoryRelObject* mmrel =
        (MemoryRelObject*)catalog->getMMObject(relObjectName);

    if(!mmrel){
       cerr << "internal error, rel is not present" << endl;
       str->SetDefined(false);
       return 0;
    }
    
    string attrName = ((CcString*)args[4].addr)->GetValue();
    string res = relObjectName + "_" + attrName;

    switch (dim){
       case 2: 
               succeed = mcreateRtree<2>(mmrel, attrPos, res, resType);
               break;
       case 3: succeed = mcreateRtree<3>(mmrel, attrPos, res, resType);
               break;
       case 4: succeed = mcreateRtree<4>(mmrel, attrPos, res, resType);
               break;
       case 8: succeed = mcreateRtree<8>(mmrel, attrPos, res, resType);
               break;
    }
    str->set(succeed, res);
    return 0;
 } //end mcreateRtreeValMap


ValueMapping mcreateRtreeValMap[] =
{
    mcreateRtreeValMapT<CcString>,
    mcreateRtreeValMapT<Mem>,
};

int mcreateRtreeSelect(ListExpr args){
    // string case at index 0
    if ( CcString::checkType(nl->First(args)) ){
       return 0;
    }
    // Mem(rel(tuple)) case at index 1
    if ( Mem::checkType(nl->First(args)) ){
       return 1;
    }
    // should never be reached
    return -1;
  }

/*

5.10.4 Description of operator ~mcreateRtree~

*/

OperatorSpec mcreateRtreeSpec(
    "string x string -> string || mem(rel(tuple)) x string -> mem(rtree)",
    "_ mcreateRtree [_]",
    "creates an main memory r-tree over a main memory relation given by the "
    "first string || mem(rel(tuple)) and an attribute "
    "given by the second argument.",
    "query 'WFlaechen' mcreateRtree [GeoData]"
);



/*

5.10.5 Instance of operator ~mcreateRtree~

*/

Operator mcreateRtreeOp (
    "mcreateRtree",             //operator's name
    mcreateRtreeSpec.getStr(),  //specification
    2,
    mcreateRtreeValMap,         // value mapping array
    mcreateRtreeSelect,    //selection function
    mcreateRtreeTypeMap         //type mapping
);

/*

5.10 Operator ~mcreateRtree2~
creates a an mmRTree the keytype must be of Kind SPATIAL2D,
SPATIAL3D, SPATIAL4D, SPATIAL8D, or of type rect

5.10.1 Type Mapping Functions of operator ~mcreateRtree2~
        (stream(Tuple) x T x string-> string) mit T of KIND SPATIAL2D,
        SPATIAL3D, SPATIAL4D, SPATIAL8D, or of type rect

*/

ListExpr mcreateRtree2TypeMap(ListExpr args){
    string err = "stream(Tuple) x attrName x string expected";
    if(!nl->HasLength(args,3)){
        return listutils::typeError("wrong number of arguments");
    }
    // first arg stream(Tuple)?
    if(!Stream<Tuple>::checkType(nl->First(args))){
        return listutils::typeError("first argument must be a stream(Tuple)");
    }
    // second Arg is AttrName
    if(nl->AtomType(nl->Second(args))!=SymbolType){
        return listutils::typeError("second argument must be an attribute");
    }
    // third arg string (name of the rtree)
    if(!CcString::checkType(nl->Third(args))){
        return listutils::typeError("third argument must be a string");
    }
    string name = nl->SymbolValue(nl->Second(args));
    // get attributelist
    ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
    ListExpr type;
    int j = listutils::findAttribute(attrList,name,type);
    if(j==0){
        return listutils::typeError("Attr " + name +" not found");
    }
    int dim = rtreehelper::getDimension(type);
    if(dim < 0){
       return listutils::typeError("type " + nl->ToString(type) 
                                   + " not supported");
    }


    ListExpr tid = listutils::basicSymbol<TupleIdentifier>();
    // find a tid in the attribute list
    string tidn;
    int k = listutils::findType(attrList, tid, tidn);
    if(k==0){
        return listutils::typeError("no tid in tuple");
    }

    ListExpr resType = nl->TwoElemList(
                          listutils::basicSymbol<Mem>(),
                          nl->TwoElemList(
                             nl->SymbolAtom(rtreehelper::BasicType()),
                             nl->IntAtom(dim)));
   

    return nl->ThreeElemList(
                   nl->SymbolAtom(Symbols::APPEND()),
                   nl->TwoElemList( nl->IntAtom(j-1),
                                    nl->IntAtom(k-1)),
                   resType);
}


/*

5.10.3  The Value Mapping Functions of operator ~mcreateRtree2~

*/
template <int dim>
int mcreateRtree2ValMapT (Word* args, Word& result,
                int message, Word& local, Supplier s) {

    result  = qp->ResultStorage(s);
    Mem* str = static_cast<Mem*>(result.addr);
    bool succeed = false;
    size_t usedMainMemory=0;
    unsigned long availableMemSize = catalog->getAvailableMemSize();

    //get r-tree name
    string name = ((CcString*)args[2].addr)->GetValue();

    // create mainmemory rtrees
    mmrtree::RtreeT<dim, size_t>* rtree =
                    new mmrtree::RtreeT<dim, size_t>(4,8);

    // get attribute-index
    int MBRIndex = ((CcInt*) args[3].addr)->GetValue();
    int TIDIndex = ((CcInt*) args[4].addr)->GetValue();

    Stream<Tuple> stream(args[0]);
    stream.open();
    Tuple* t;

    while( (t=stream.request())!=0){
        TupleIdentifier* tid = (TupleIdentifier*) t->GetAttribute(TIDIndex);
        if(tid->IsDefined()){
            StandardSpatialAttribute<dim>* attr
               =(StandardSpatialAttribute<dim>*) t->GetAttribute(MBRIndex);
            if(attr->IsDefined()){
              Rectangle<dim> rect = attr->BoundingBox();
              if(rect.IsDefined()){ 
                rtree->insert(rect, tid->GetTid());
              }
            }
        }
        t->DeleteIfAllowed();
    }

    stream.close();
    usedMainMemory = rtree->usedMem();

    ListExpr le = nl->TwoElemList(
                      listutils::basicSymbol<Mem>(),
                      nl->TwoElemList(
                        nl->SymbolAtom(rtreehelper::BasicType()),
                        nl->IntAtom(dim)
                      ));

    MemoryRtreeObject<dim>* mmRtreeObject =
        new MemoryRtreeObject<dim>(rtree, usedMainMemory,
                        nl->ToString(le), false, "");

    if (usedMainMemory>availableMemSize){
        cout<<"there is not enough memory left to create the rtree";
    }
    if (usedMainMemory<=availableMemSize &&
                catalog->insert(name,mmRtreeObject)){
        succeed = true;
    }else {
       delete mmRtreeObject;
       succeed = false;
    }
    str->set(succeed, name);
    return 0;

}

ValueMapping mcreateRtree2ValMap[] =
{
    mcreateRtree2ValMapT<2>,
    mcreateRtree2ValMapT<3>,
    mcreateRtree2ValMapT<4>,
    mcreateRtree2ValMapT<8>,
};

/*
1.3 Selection method for value mapping array ~mcreateRtree2~

*/
 int mcreateRtree2Select(ListExpr args)
 {
    ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
    ListExpr type;
    string name = nl->SymbolValue(nl->Second(args));
    listutils::findAttribute(attrList,name,type);

    int dim = rtreehelper::getDimension(type);
    switch(dim){
         case 2: return 0;
         case 3: return 1;
         case 4: return 2;
         case 8: return 3;
    }
    return -1;
}

/*

5.10.4 Description of operator ~mcreateRtree2~

*/

OperatorSpec mcreateRtree2Spec(
    "stream(Tuple) x Ident  x string -> mem(rtree)",
    "_ mcreateRtree2 [_,_]",
    "creates a main memory r-tree over a tuple stream (1st argument)."
    "The second argument specifies the attribute over that the index "
    "is created. The type of this attribute must be in kind SPATIALxD. "
    "The third argument specifies the name of the main memory object.",
    "query strassen feed head[5] mcreateRtree2 [GeoData, 'strassen_GeoData']"
);



/*

5.10.5 Instance of operator ~mcreateRtree2~

*/

Operator mcreateRtree2Op (
    "mcreateRtree2",             //operator's name
    mcreateRtree2Spec.getStr(),  //specification
    4,
    mcreateRtree2ValMap,         // value mapping array
    mcreateRtree2Select,    //selection function
    mcreateRtree2TypeMap         //type mapping
);


/*
5.11 Operator ~memsize~

returns the currently set main memory size

5.11.1 Type Mapping Functions of operator ~memsize~ (->int)

*/

ListExpr memsizeTypeMap(ListExpr args)
{

  if(nl->ListLength(args)!=0){
     return listutils::typeError("no argument expected");
  }

  return listutils::basicSymbol<CcInt>();
}


/*

5.11.3  The Value Mapping Functions of operator ~memsize~

*/

int memsizeValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    int res = catalog->getMemSizeTotal();

    result  = qp->ResultStorage(s);
    CcInt* b = static_cast<CcInt*>(result.addr);
    b->Set(true, res);


    return 0;
}

/*

5.11.4 Description of operator ~memsize~

*/

OperatorSpec memsizeSpec(
    "-> int",
    "memsize()",
    "returns the currently set main memory size ",
    "query memsize()"
);

/*

5.11.5 Instance of operator ~memsize~

*/

Operator memsizeOp (
    "memsize",
    memsizeSpec.getStr(),
    memsizeValMap,
    Operator::SimpleSelect,
    memsizeTypeMap
);



/*
5.12 Operator ~memclear~

deletes all main memory objects

5.12.1 Type Mapping Functions of operator ~memclear~ (-> bool)

*/

ListExpr memclearTypeMap(ListExpr args)
{

  if(nl->ListLength(args)!=0){
     return listutils::typeError("no argument expected");
  }

  return listutils::basicSymbol<CcBool>();
}


/*

5.12.3  The Value Mapping Functions of operator ~memclear~

*/

int memclearValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    bool res = false;
    catalog->clear();
    res = true;
    result  = qp->ResultStorage(s);
    CcBool* b = static_cast<CcBool*>(result.addr);
    b->Set(true, res);
    return 0;
}

/*

5.12.4 Description of operator ~memclear~

*/

OperatorSpec memclearSpec(
    "-> bool",
    "memclear()",
    "deletes all main memory objects",
    "query memclear()"
);

/*

5.12.5 Instance of operator ~memclear~

*/

Operator memclearOp (
    "memclear",
    memclearSpec.getStr(),
    memclearValMap,
    Operator::SimpleSelect,
    memclearTypeMap
);


/*
5.13 Operator ~meminsert~

inserts the tuple of a stream into a existing main memory relation

5.13.1 Type Mapping Functions of operator ~meminsert~
    (stream(tuple(x)) x string -> stream(tuple(x))
    the second argument identifies the main memory relation

*/

ListExpr meminsertTypeMap(ListExpr args)
{
    if(nl->ListLength(args)!=2){
        return listutils::typeError("two arguments expected");
    }

    if(!checkUsesArgs(args)){
      return listutils::typeError("internal error");
    } 

    ListExpr stream = nl->First(nl->First(args));

    if (!Stream<Tuple>::checkType(stream)) {
        return listutils::typeError
            ("stream(Tuple) as first argument expected");
    }
    
    ListExpr argSec = nl->Second(args); //string + query

    string errMsg;
    ListExpr a2t;
    if(!getMemType(nl->First(argSec), nl->Second(argSec), a2t, errMsg)){
      return listutils::typeError("problem in 2nd arg: " + errMsg);
    }

    ListExpr subtype = nl->Second(a2t);
    if(!Relation::checkType(subtype)){
       return listutils::typeError("mem relation expected ");
    }
    if(!nl->Equal(nl->Second(stream), nl->Second(subtype))){
       return listutils::typeError("stream type and mem relation type differ");
    }
    return stream;

}


class meminsertInfo{
  public:
     meminsertInfo( Word w, vector<Tuple*>* _relation, bool _flob):
          stream(w),relation(_relation), flob(_flob){
        stream.open();
     }

    ~meminsertInfo(){
       stream.close();
     }

     Tuple* next(){
       Tuple* res = stream.request();
       if(!res){
         return 0;
       }
       if(flob){
         res->bringToMemory();
       }
       res->IncReference();
       relation->push_back(res);
       res->SetTupleId(relation->size());
       return res;
     }

  private:
     Stream<Tuple> stream;
     vector<Tuple*>* relation;
     bool flob;

};


/*

5.13.3  The Value Mapping Functions of operator ~meminsert~

*/
template<class T>
int meminsertValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    meminsertInfo* li = (meminsertInfo*) local.addr;

    switch (message)
    {
        case OPEN: {
            if(li){
               delete li;
               local.addr=0;
            }
            T* oN = (T*) args[1].addr;
            ListExpr tt = nl->Second(qp->GetType(qp->GetSon(s,0)));
            MemoryRelObject* mro = getMemRel(oN,tt);
            if(!mro){
              return 0;
            }
            local.addr = new meminsertInfo(args[0], mro->getmmrel(), 
                                         mro->hasflob()); 
            return 0;
        }

        case REQUEST:
            result.addr=(li?li->next():0);
            return result.addr?YIELD:CANCEL;

        case CLOSE:
            if(li) {
              delete li;
              local.addr = 0;
            }
            return 0;
   }

   return -1;
}

ValueMapping meminsertVM[] = {
   meminsertValMap<CcString>,
   meminsertValMap<Mem>
};

int meminsertSelect(ListExpr args){
   return CcString::checkType(nl->Second(args))?0:1;
}



/*

5.13.4 Description of operator ~meminsert~

*/

OperatorSpec meminsertSpec(
    "stream(Tuple) x {string, mem(rel)}  -> stream(Tuple)",
    "meminsert(_,_)",
    "inserts the tuple of a stream into an "
    "existing main memory relation",
    "query meminsert (ten feed head[5],'ten') count"
);

/*

5.13.5 Instance of operator ~meminsert~

*/

Operator meminsertOp (
    "meminsert",
    meminsertSpec.getStr(),
    2,
    meminsertVM,
    meminsertSelect,
    meminsertTypeMap
);

/*
5.14 Operator ~mwindowintersects~
        Uses the given MemoryRtreeObject (as first argument)to find all tuples
        in the given MemoryRelObject (as second argument)
        with intersects the third argument value's bounding box

5.14.1 Type Mapping Functions of operator ~mwindowintersects~
    string x string x T -> stream(Tuple)
    where T in {rect<d>} U SPATIAL2D U SPATIAL3D U SPATIAL4D U SPATIAL8D

*/

ListExpr mwindowintersectsTypeMap(ListExpr args)
{
  if(nl->ListLength(args)!=3){
    return listutils::typeError("three arguments expected");
  }

  if(!checkUsesArgs(args)){
     return listutils::typeError("internal error");
  }

  /* Split argument in three parts */
  ListExpr a1 = nl->First(args);
  ListExpr a2 = nl->Second(args);
  ListExpr a3 = nl->Third(args);

  string err=" {string, mem(rtree)} x {string, mem(rel)} x SPATIALXD expected";

  ListExpr a1t;
  string errMsg;

  if(!getMemType(nl->First(a1), nl->Second(a1), a1t, errMsg)){
    return listutils::typeError(err + "\n problem in arg 1: " + errMsg);
  } 
  
  ListExpr rtreetype = nl->Second(a1t); // remove leading mem
  
  if(!rtreehelper::checkType(rtreetype)){
    return listutils::typeError(err + " (first arg is not a mem rtree)");
  }

  int rtreedim = nl->IntValue(nl->Second(rtreetype));

  ListExpr a2t;
  if(!getMemType(nl->First(a2), nl->Second(a2), a2t, errMsg)){
    return listutils::typeError(err + "\n problem in arg 2: " + errMsg);
  }
  ListExpr relType = nl->Second(a2t);
  if(!Relation::checkType(relType)){
    return listutils::typeError(err + " (second argument is not "
                                "a memory relation)");
  }
 
  int dim2 = rtreehelper::getDimension(nl->First(a3));

  if(dim2 < 0){
    return listutils::typeError("third argument not in kind SPATIALxD");
  }

  if(rtreedim != dim2){
     return listutils::typeError("dimensions of rtree and "
                                 "search object differ");
  }
  
  ListExpr res = nl->TwoElemList( 
               listutils::basicSymbol<Stream<Tuple> >(),
               nl->Second(relType)); 
  return res;
}



template<int dim>
class mwiInfo{
  public:
     mwiInfo( MemoryRtreeObject<dim>* _tree,
              MemoryRelObject* _rel,
              Rectangle<dim>& _box) {
        mmrtree::RtreeT<dim, size_t>* t = _tree->getrtree();
        rel = _rel->getmmrel();
        it = t->find(_box);
     }

    ~mwiInfo(){
        delete it;
    }

     Tuple* next(){
        Tuple* res = 0;
        while(!res){
          const size_t* indexp = it->next();
          if(!indexp){
             return 0;
          }
          if(*indexp < rel->size()){ // ignores index outside vector
             res = (*rel)[*indexp];
             if(res){ // may be deleted
                res->IncReference();
             }
          }
        }
        return res;
     }

  private:
     vector<Tuple*>* rel;
     typename mmrtree::RtreeT<dim, size_t>::iterator* it;
};

/*

5.14.3  The Value Mapping Functions of operator ~mwindowintersects~

*/
template <int dim, class T, class R>
int mwindowintersectsValMapT (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

   mwiInfo<dim>* li = (mwiInfo<dim>*) local.addr;

   switch (message)
   {
        case OPEN: {
             if(li){
               delete li;
               local.addr=0;
             }
             T* treeN = (T*) args[0].addr;
             R* relN = (R*) args[1].addr;
             typedef StandardSpatialAttribute<dim> W;
             typedef  MemoryRtreeObject<dim> treetype;
             W* window = (W*) args[2].addr;
             ListExpr tupleT = nl->Second(qp->GetType(s));
             MemoryRelObject* mro = getMemRel(relN,tupleT);
             if(!mro){
               return 0;
             }
             treetype* tree = getRtree<T,dim>(treeN);
             if(!tree){
               return 0;
             }
             if(!window->IsDefined()){
                 return 0;
             }
             Rectangle<dim> box = window->BoundingBox();
             if(!box.IsDefined()){ // empty spatial object
                return 0;
             }
             local.addr = new mwiInfo<dim>(tree, mro, box);
             return 0;
        }

        case REQUEST:
            result.addr=(li?li->next():0);
            return result.addr?YIELD:CANCEL;


        case CLOSE:
            if(li) {
              delete li;
              local.addr = 0;
            }
            return 0;
   }

    return -1;
}

ValueMapping mwindowintersectsValMap[] =
{
    mwindowintersectsValMapT<2,CcString, CcString>,
    mwindowintersectsValMapT<3,CcString, CcString>,
    mwindowintersectsValMapT<4,CcString, CcString>,
    mwindowintersectsValMapT<8,CcString, CcString>,

    mwindowintersectsValMapT<2,CcString, Mem>,
    mwindowintersectsValMapT<3,CcString, Mem>,
    mwindowintersectsValMapT<4,CcString, Mem>,
    mwindowintersectsValMapT<8,CcString, Mem>,
    
    mwindowintersectsValMapT<2,Mem, CcString>,
    mwindowintersectsValMapT<3,Mem, CcString>,
    mwindowintersectsValMapT<4,Mem, CcString>,
    mwindowintersectsValMapT<8,Mem, CcString>,

    mwindowintersectsValMapT<2,Mem, Mem>,
    mwindowintersectsValMapT<3,Mem, Mem>,
    mwindowintersectsValMapT<4,Mem, Mem>,
    mwindowintersectsValMapT<8,Mem, Mem>,
};

/*
1.3 Selection method for value mapping array ~mcreateRtree~

*/
 int mwindowintersectsSelect(ListExpr args)
 {
   int n1 = CcString::checkType(nl->First(args))?0:1;
   int n2 = CcString::checkType(nl->Second(args))?0:1;
   int dim = rtreehelper::getDimension(nl->Third(args));
   int n3 = -1;
   switch (dim){
     case 2 : n3 = 0;break;
     case 3 : n3 = 1;break;
     case 4 : n3 = 2;break;
     case 8 : n3 = 3;break;
   }
   return 8*n1 + 4*n2 + n3;

  return -1;
 }


/*

5.14.4 Description of operator ~mwindowintersects~

*/

OperatorSpec mwindowintersectsSpec(
   "{string, mem(rtree <dim>) x {string, mem(rel(X)) x T -> stream(tuple(X)) "
   "where T in SPATIAL<dim>D",
   "_ _ mwindowintersects[_]",
   "Uses the given rtree to find all tuples"
   " in the given relation which intersects the "
   " argument value's bounding box.",
   "query \"strassen_GeoData\" \"strassen\" mwindowintersects[thecenter] count"
);

/*

5.14.5 Instance of operator ~mwindowintersects~

*/

Operator mwindowintersectsOp (
    "mwindowintersects",
    mwindowintersectsSpec.getStr(),
    16,
    mwindowintersectsValMap,
    mwindowintersectsSelect,
    mwindowintersectsTypeMap
);


/*
5.16 Operator mwindowintersectsS 

*/
template<bool wrap>
ListExpr mwindowintersectsSTM(ListExpr args){

  string err = " {string, memory(rtree <dim> )} x SPATIAL<dim>D expected";
  if(!nl->HasLength(args,2)){
     return listutils::typeError(err + " (wrong number of args)");
  }
  ListExpr a2t = nl->First(nl->Second(args));
  int dim = rtreehelper::getDimension(a2t);
  if(dim < 0){
     return listutils::typeError(err + " (second arg is not in "
                                 "kind SPATIALxD)");
  }
  ListExpr a1 = nl->First(args);
  ListExpr a1t;
  string errMsg;
  if(!getMemType(nl->First(a1), nl->Second(a1), a1t, errMsg)){
     return listutils::typeError(err +"\n error in 1st arg: " + errMsg);
  }
  ListExpr tree = nl->Second(a1t);
  if(!rtreehelper::checkType(tree)){
      return listutils::typeError(err + " (first arg is not an r-tree");
  }
  int tdim = nl->IntValue(nl->Second(tree));
  if(dim!=tdim){
     return listutils::typeError(err + "tree and query object have "
                                       "different dimensions");
  }
  if(!wrap){
     return nl->TwoElemList(
                    listutils::basicSymbol<Stream<TupleIdentifier> >(),
                    listutils::basicSymbol<TupleIdentifier> ());
  } else {
    ListExpr attrList = nl->OneElemList(
                            nl->TwoElemList( 
                              nl->SymbolAtom("TID"),
                              listutils::basicSymbol<TupleIdentifier>()));

    return nl->TwoElemList( listutils::basicSymbol<Stream<Tuple> >(),
                            nl->TwoElemList( listutils::basicSymbol<Tuple>(),
                            attrList));
  }
}

template<int dim>
struct mwindowintersectsSInfo{
    typename mmrtree::RtreeT<dim,size_t>::iterator* it;
    TupleType* tt;
};


template <int dim, class T, bool wrap>
int mwindowintersectsSVMT (Word* args, Word& result,
                    int message, Word& local, Supplier s) {


   mwindowintersectsSInfo<dim>* li = (mwindowintersectsSInfo<dim>*)local.addr;

   switch(message){
      case OPEN:{
             if(li){
               delete li->it;
               if(li->tt){
                  li->tt->DeleteIfAllowed();
               }
               delete li;
               local.addr = 0;
             }
             T* name = (T*) args[0].addr;
             MemoryRtreeObject<dim>* tree = getRtree<T,dim>(name);
             if(!tree){
                return 0;
             }
             typedef StandardSpatialAttribute<dim> boxtype;
             boxtype* o = (boxtype*) args[1].addr;
             if(!o->IsDefined()){
               return 0;
             }
             Rectangle<dim> box = o->BoundingBox();
             if(!box.IsDefined()){ // empty spatial object
               return 0;
             }
             li = new mwindowintersectsSInfo<dim>();
             li->it = tree->getrtree()->find(box);
             if(wrap){
               li->tt = new TupleType( nl->Second(GetTupleResultType(s)));
             } else {
               li->tt = 0;
             }
             local.addr = li;
             return 0;
      }
      case REQUEST : {
               if(!li){
                  return CANCEL;
               }
               const size_t* index = li->it->next();
               if(!index){
                 result.addr=0;
               } else { // iterator exhausted
                 TupleIdentifier* tid = new TupleIdentifier(true,*index);
                 if(!wrap){
                    result.addr = tid;
                 } else {
                   Tuple* tuple = new Tuple(li->tt);
                   tuple->PutAttribute(0,tid);
                   result.addr = tuple;
                 }
               }
               return result.addr?YIELD:CANCEL;
        }
       case CLOSE:
                 if(li){
                    delete li->it;
                    if(li->tt){
                       li->tt->DeleteIfAllowed();
                    }
                    delete li;
                    local.addr = 0;
                 }
                 return 0;
   }
   return -1;
}

ValueMapping mwindowintersectsSVM[] = {
   mwindowintersectsSVMT<2, CcString, true>,
   mwindowintersectsSVMT<3, CcString, true>,
   mwindowintersectsSVMT<4, CcString, true>,
   mwindowintersectsSVMT<8, CcString, true>,
   mwindowintersectsSVMT<2, Mem, true>,
   mwindowintersectsSVMT<3, Mem, true>,
   mwindowintersectsSVMT<4, Mem, true>,
   mwindowintersectsSVMT<8, Mem, true>
};

int mwindowintersectsSSelect(ListExpr args){
   int n1 = CcString::checkType(nl->First(args))?0:1;
   int dim = rtreehelper::getDimension(nl->Second(args));
   int n2;
   switch(dim){
      case 2 : n2 = 0; break;
      case 3 : n2 = 1; break;
      case 4 : n2 = 2; break;
      case 8 : n2 = 3; break;
      default : assert(false);
   }
   return 4*n1 + n2;
}


OperatorSpec mwindowintersectsSSpec(
  "{string, memory(rtree <dim>)} x SPATIAL<dim>D -> stream(tuple((TID tid)))",
  " _ mwindowintersectsS[_]",
  "Returns the tuple ids belonging to rectangles intersecting the "
  "bounding box of the second argument wraped in a tuple. ",
  "query \"strassen_GeoData\" mwindowintersectsS[ thecenter ] count" 
);

Operator mwindowintersectsSOp(
   "mwindowintersectsS",
   mwindowintersectsSSpec.getStr(),
   8,
   mwindowintersectsSVM,
   mwindowintersectsSSelect,
   mwindowintersectsSTM<true>
);



/*

5.15 Operator ~mconsume~

~mconsume~ Collects objects from a stream in a ~MemoryRelObject~

5.4.1 Type Mapping Functions of operator ~mconsume~
        (stream(Tuple) -> memoryRelObject)

*/
ListExpr mconsumeTypeMap(ListExpr args)
{
    if(nl->ListLength(args)!=1){
        return listutils::typeError("(wrong number of arguments)");
    }
    if (!Stream<Tuple>::checkType(nl->First(args))) {
        return listutils::typeError ("stream(tuple) expected!");
    }
    ListExpr l1 = nl->Second(nl->First(args));
    ListExpr l2 = nl->SymbolAtom(MemoryRelObject::BasicType());
    return nl->TwoElemList (l2,l1);;
}


/*

5.15.3  The Value Mapping Functions of operator ~mconsume~

*/

int mconsumeValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {
    Supplier t = qp->GetSon( s, 0 );
    ListExpr le = qp->GetType(t);
    result  = qp->ResultStorage(s);
    MemoryRelObject* mrel = (MemoryRelObject*)result.addr;
    Stream<Tuple> stream(args[0]);
    stream.open();
    Tuple* tup = 0;
    while( (tup = stream.request()) != 0){
        mrel->addTuple(tup);
    }
    mrel->setObjectTypeExpr(nl->ToString(nl->Second(le)));
    stream.close();
    return 0;
}


/*

5.15.4 Description of operator ~mconsume~

*/

OperatorSpec mconsumeSpec(
    "stream(Tuple) -> memoryrelobject",
    "_ mconsume",
    "collects the objects from a stream(tuple) into a memory relation",
    "query 'ten' mfeed mconsume"
);



/*

5.15.5 Instance of operator ~mconsume~

*/

Operator mconsumeOp (
    "mconsume",
    mconsumeSpec.getStr(),
    mconsumeValMap,
    Operator::SimpleSelect,
    mconsumeTypeMap
);




/*
5.16 Operator ~mcreateAVLtree~
creates a an AVLTree over a given main memory relation

5.16.1 Type Mapping Functions of operator ~mcreateAVLtree~
        (string x string -> string  ||
        mem(rel(tuple)) x string -> string)

        the first parameter identifies the main memory relation, the
        second parameter identifies the attribute

*/

ListExpr mcreateAVLtreeTypeMap(ListExpr args){

    if(nl->ListLength(args)!=2){
     return listutils::typeError("two arguments expected");
    }

    // Split argument in two parts
    ListExpr a1 = nl->First(args);
    ListExpr a2 = nl->Second(args);
    ListExpr oTE_Rel;

    string errMsg;
    if(!getMemType(nl->First(a1), nl->Second(a1), oTE_Rel, errMsg)){
      return listutils::typeError("problem in 1st arg: " + errMsg);
    }
    oTE_Rel = nl->Second(oTE_Rel);

    if(!Relation::checkType(oTE_Rel)){
       return listutils::typeError("memory object is not a relation");
    }

    if(nl->AtomType(nl->First(a2))!=SymbolType){
       return listutils::typeError("second argument is not a valid "
                                   "attribute name");
    }

    string attrName = nl->SymbolValue(nl->First(a2));
    ListExpr attrType = 0;
    int attrPos = 0;
    ListExpr attrList = nl->Second(nl->Second(oTE_Rel));
    attrPos = listutils::findAttribute(attrList, attrName, attrType);

    if (attrPos == 0){
        return listutils::typeError
        ("there is no attribute having  name " + attrName);
    }

    ListExpr resType = nl->TwoElemList(
                          listutils::basicSymbol<Mem>(),
                          nl->TwoElemList(
                             listutils::basicSymbol<MemoryAVLObject>(),
                             attrType
                       ));


    return nl->ThreeElemList(
                nl->SymbolAtom(Symbols::APPEND()),
                nl->TwoElemList(nl->IntAtom(attrPos - 1),
                                nl->StringAtom(attrName)),
                resType);
}

/*

5.16.3  The Value Mapping Functions of operator ~mcreateAVLtree~

*/
template<class T>
int mcreateAVLtreeValMapT (Word* args, Word& result,
                int message, Word& local, Supplier s) {

   result  = qp->ResultStorage(s);
   Mem* str = static_cast<Mem*>(result.addr);

   // the main memory relation
   T* roN = (T*) args[0].addr;
   if(!roN->IsDefined()){
        str->SetDefined(false);
        return 0;
   }
   string relObjectName = roN->GetValue();

   int attrPos = ((CcInt*) args[2].addr)->GetValue();

   ListExpr memObjectType = catalog->getMMObjectTypeExpr(relObjectName);
   memObjectType = nl->Second(memObjectType);

   if(!Relation::checkType(memObjectType)){
     cerr << "object " << relObjectName  << "is not a relation" << endl;
     str->SetDefined(false);
     return false;
   } 

   // extract attribute 
   ListExpr attrList = nl->Second(nl->Second(memObjectType));
   int ap = attrPos;
   while(!nl->IsEmpty(attrList) && ap>0){
      attrList = nl->Rest(attrList);
      ap--;
   }
   if(nl->IsEmpty(attrList)){
     cerr << "not enough attributes in tuple";
     str->SetDefined(false);
     return 0;
   }
   ListExpr relattrType = nl->Second(nl->First(attrList));
   ListExpr avlattrType = nl->Second(nl->Second(qp->GetType(s)));

   if(!nl->Equal(relattrType, avlattrType)){
     cerr << "expected type and type in relation differ" << endl;
     str->SetDefined(false);
     return 0;
   }

   string attrName = ((CcString*)args[3].addr)->GetValue();

   string resName = relObjectName + "_" + attrName;
   if(catalog->isMMObject(resName)){
      cerr << "object " << resName << "already exists" << endl;
      str->SetDefined(false);
      return 0;
   }

   MemoryRelObject* mmrel =
        (MemoryRelObject*)catalog->getMMObject(relObjectName);
   bool flob = mmrel->hasflob();
   vector<Tuple*>* relVec = mmrel->getmmrel();
   vector<Tuple*>::iterator it;
   it=relVec->begin();
   unsigned int i=1; // start tuple counting with 1

   memAVLtree* tree = new memAVLtree();
   Attribute* attr;
   AttrIdPair aPair;

   size_t usedMainMemory = 0;
   unsigned long availableMemSize = catalog->getAvailableMemSize();

   while ( it!=relVec->end()){
       Tuple* tup = *it;
       if(tup){
         attr=tup->GetAttribute(attrPos);
         aPair = AttrIdPair(attr,i);
         // size for a pair is 16 bytes, plus an additional pointer 8 bytes
         size_t entrySize = 24;
         if (entrySize<availableMemSize){
             tree->insert(aPair);
             usedMainMemory += (entrySize);
             availableMemSize -= (entrySize);
             i++;
         } else {
            cout<<"there is not enough main memory available"
            " to create an AVLTree"<<endl;
            delete tree;
            str->SetDefined(false);
            return 0;
         }
       }
       it++;
    }
    MemoryAVLObject* avlObject =
        new MemoryAVLObject(tree, usedMainMemory,
            nl->ToString(qp->GetType(s)),flob, getDBname());
    catalog->insert(resName,avlObject);

    str->set(true, resName);
    return 0;
} //end mcreateAVLtreeValMap


ValueMapping mcreateAVLtreeValMap[] =
{
    mcreateAVLtreeValMapT<CcString>,
    mcreateAVLtreeValMapT<Mem>,
};

int mcreateAVLtreeSelect(ListExpr args){
    // string case at index 0
    if ( CcString::checkType(nl->First(args)) ){
       return 0;
    }
    // Mem(rel(tuple))case at index 1
    if ( Mem::checkType(nl->First(args)) ){
       return 1;
    }
    // should never be reached
    return -1;
  }

/*

5.16.4 Description of operator ~mcreateAVLtree~

*/

OperatorSpec mcreateAVLtreeSpec(
    "string x string -> string || mem(rel(tuple(X))) x string -> string",
    "_ mcreateAVLtree [_]",
    "creates an AVLtree over a main memory relation given by the"
    "first string || mem(rel(tuple))  and an attribute "
    "given by the second iargument",
    "query \"Staedte\" mcreateAVLtree [SName]"
);

/*

5.16.5 Instance of operator ~mcreateAVLtree~

*/

Operator mcreateAVLtreeOp (
    "mcreateAVLtree",
    mcreateAVLtreeSpec.getStr(),
    2,
    mcreateAVLtreeValMap,
    mcreateAVLtreeSelect,
    mcreateAVLtreeTypeMap
);


/*
5.17 Operator createAVLtree2

This operator creates an AVL tree from a tuple stream.

*/
ListExpr mcreateAVLtree2TM(ListExpr args){
  string err = "stream(tuple) x Ident x Indent x string expected";
  if(!nl->HasLength(args,4)){
    return listutils::typeError(err + " (wrong number of args)");
  }

  if(!Stream<Tuple>::checkType(nl->First(args))){
    return listutils::typeError(err + " ( first arg is not a tuple stream)");
  }
  if(nl->AtomType(nl->Second(args)) != SymbolType){
    return listutils::typeError(err + " (second arg is not a "
                                "valid identifier)");
  }
  if(nl->AtomType(nl->Third(args)) != SymbolType){
    return listutils::typeError(err + " (third arg is not a "
                                "valid identifier)");
  }
  if(!CcString::checkType(nl->Fourth(args))){
    return listutils::typeError(err+ " (fourth arg is not a string)");
  }

  ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
  string keyName = nl->SymbolValue(nl->Second(args));
  string tidName = nl->SymbolValue(nl->Third(args));
  ListExpr keyType;
  int keyIndex = listutils::findAttribute(attrList, keyName, keyType);
  if(!keyIndex){
     return listutils::typeError("attribute " + keyName 
                                 + " not found in tuple");
  }
  ListExpr tidType;
  int tidIndex = listutils::findAttribute(attrList, tidName, tidType);
  if(!tidIndex){
     return listutils::typeError("attribute " + tidName 
                                 + " not found in tuple");
  }
  if(!TupleIdentifier::checkType(tidType)){
     return listutils::typeError("attribute " + tidName + " is not a tid");
  }
  ListExpr resType = nl->TwoElemList(
                          listutils::basicSymbol<Mem>(),
                          nl->TwoElemList(
                               listutils::basicSymbol<MemoryAVLObject>(),
                               keyType));

  return nl->ThreeElemList(
                  nl->SymbolAtom(Symbols::APPEND()),
                  nl->TwoElemList( nl->IntAtom(keyIndex-1),
                                   nl->IntAtom(tidIndex-1)),
                  resType);
}


int mcreateAVLtree2VM (Word* args, Word& result,
                int message, Word& local, Supplier s) {

    result = qp->ResultStorage(s);
    Mem* res = (Mem*) result.addr;

    CcString* memName = (CcString*) args[3].addr;
    if(!memName->IsDefined()){
      res->SetDefined(false);
      return 0;
    }
    string name = memName->GetValue();
    if(catalog->isMMObject(name)){
      res->SetDefined(false);
      return 0;
    }

    int keyIndex = ((CcInt*) args[4].addr)->GetValue();
    int tidIndex = ((CcInt*) args[5].addr)->GetValue();

    Stream<Tuple> stream(args[0]);
    Tuple* tuple;
    memAVLtree* tree = new memAVLtree();

    stream.open();
    size_t availableMem = catalog->getAvailableMemSize();
    size_t usedMem = 0;

    while((tuple=stream.request()) && (usedMem < availableMem)){
      TupleIdentifier* tid = (TupleIdentifier*) tuple->GetAttribute(tidIndex);
      if(tid->IsDefined()){
          Attribute* key = tuple->GetAttribute(keyIndex);
          key->bringToMemory();
          AttrIdPair p(key, tid->GetTid());
          usedMem += key->GetMemSize() + sizeof(size_t);
          tree->insert(p); 
      }
      tuple->DeleteIfAllowed();
    }
    stream.close();
    if(usedMem > availableMem){
       delete tree;         
       res->SetDefined(false);
       return 0;
    }
    MemoryAVLObject* mo = new MemoryAVLObject(tree, usedMem, 
                       nl->ToString(qp->GetType(s)), true, getDBname());

    bool success = catalog->insert(name, mo);
    if(!success){
       delete mo;
    }
    res->set(success,name);
    return 0;
}


OperatorSpec mcreateAVLtree2Spec(
  "stream(tuple) x Ident x Ident x string -> mem(avltree ...)",
  "_ mcreateAVLtree2[_,_,_] ",
  "creates an avl tree over an tuple stream. The second argument "
  "specifies the attribute over that the index is build, the "
  "third argument specifies a TID attribute within the tuple. "
  "The last argument specifies the name in the memory representation.",
  "query strassen feed addid createAVLtree2[Name, TID, \"strassen_Name\"]"
);


Operator mcreateAVLtree2Op (
    "mcreateAVLtree2",
    mcreateAVLtree2Spec.getStr(),
    mcreateAVLtree2VM,
    Operator::SimpleSelect,
    mcreateAVLtree2TM
);


/*
5.17 Operator ~mexactmatch~
        Uses the given MemoryAVLObject or MemoryTTreeObject (as first argument)
        to find all tuples in the given MemoryRelObject (as second argument)
        with the same key value


5.17.1 Type Mapping Functions of operator ~mexactmatch~
    string x string x key -> stream(Tuple)


*/
ListExpr mexactmatchTM(ListExpr args) {
    string err ="{string, mem(avltree(T)), mem(rel(...)) } "
                "x {string, mem(avltree)} x T expected";
    if(nl->ListLength(args)!=3){
        return listutils::typeError("three arguments expected");
    }

    // process first argument
    ListExpr a1t = nl->First(args);
    string errMsg;

    if(!getMemType(nl->First(a1t), nl->Second(a1t), a1t, errMsg,true)){
       return listutils::typeError(  err + "(problem in first arg : " 
                                   + errMsg+")");
    }
   // process second argument
    ListExpr a2t = nl->Second(args);
    if(!getMemType(nl->First(a2t), nl->Second(a2t), a2t, errMsg,true)){
       return listutils::typeError(  err + "(problem in first arg : " 
                                   + errMsg+")");
    }

    
    a1t = nl->Second(a1t); // remove mem
    a2t = nl->Second(a2t);

    bool avl = MemoryAVLObject::checkType(a1t);
    bool ttree = MemoryTTreeObject::checkType(a1t);
    
    if(!avl && !ttree){
      return listutils::typeError("first arg is not an avl tree or a t tree");
    }


    if(!Relation::checkType(a2t)){
      return listutils::typeError("second arg is not a memory relation");
    }

    ListExpr a3t = nl->First(nl->Third(args));

    if(!nl->Equal(a3t, nl->Second(a1t))){
      return listutils::typeError("type managed by tree and key type differ");
    }

    ListExpr res = nl->TwoElemList( listutils::basicSymbol<Stream<Tuple> >(),
                            nl->Second(a2t));

    return nl->ThreeElemList(nl->SymbolAtom(Symbols::APPEND()),
                             nl->OneElemList(
                                nl->BoolAtom(avl)),
                             res);

}

class avlOperLI{
    public:
        avlOperLI(
           memAVLtree* _tree,
           vector<Tuple*>* _relation, 
           Attribute* _attr1,
           Attribute* _attr2, 
           bool _below)
           :relation(_relation), avltree(_tree), attr1(_attr1), attr2(_attr2),
           below(_below){
           isAvl = true;
           if(!below){
              avlit = avltree->tail(AttrIdPair(attr1,0));    
           }
           res = true; 
        }
        
        avlOperLI(
          memttree* _tree,
          vector<Tuple*>* _relation, 
          Attribute* _attr1,
          Attribute* _attr2, 
          bool _below)
          :relation(_relation), ttree(_tree), attr1(_attr1), attr2(_attr2),
          below(_below){
          isAvl = false;
          if(!_below){
            tit = ttree->tail(AttrIdPair(attr1,0)); 
          }
          res = true; 
        }


        ~avlOperLI(){}


        Tuple* next(){
          assert(!below);
          // T-Tree
          if(!isAvl) {
            while(!tit.end()) {
              thit = *tit; 
              if ((thit.getAttr())->Compare(attr2) > 0){ // end reached
                  return 0;
              }
              Tuple* result = relation->at(thit.getTid()-1);
              if(result){
                 result->IncReference();
                 tit++;
                 return result;
              } else {
                 tit++;
              }
            } // end of iterator reached
             return 0;
          }  else { // avl tree
            while(!avlit.onEnd()){
              avlhit = avlit.Get();
              if ((avlhit->getAttr())->Compare(attr2) > 0){ // end reached
                 return 0;
               }

               Tuple* result = relation->at(avlhit->getTid()-1);
               if(result){ // tuple exist
                 result->IncReference();
                 avlit.Next();
                 return result;
               } else { // tuple deleted
                 avlit.Next();
               }
            }
            return 0;     // end of iterator reached
          }
          return 0; // should never be reached
        }


        Tuple* matchbelow(){
          assert(below);
          if (res) {
            // AVL-Tree
            if(isAvl) {
              size_t i = relation->size();
              avlhit = avltree->GetNearestSmallerOrEqual
                          (AttrIdPair(attr1,i));
              if (avlhit==0) {
                  return 0;
              }
              Tuple* result = relation->at(avlhit->getTid()-1);
              if(result){ // present in index, but deleted in rel
                result->IncReference();
              }
              res = false;
              return result;
            } else { // ttree
              size_t i = relation->size();
              AttrIdPair s(attr1,i);
              const AttrIdPair* t = ttree->GetNearestSmallerOrEqual(s, 0);
              if(!t){
                return 0;
              }
              Tuple* result = relation->at(t->getTid()-1);
              if(result){
                 result->IncReference();
              }
              res = false;
              return result;
            }
          }
          return 0;
        }

    private:
        vector<Tuple*>* relation;
        memAVLtree* avltree;
        memttree* ttree;
        Attribute* attr1;
        Attribute* attr2;
        string keyType;
        bool below;
        avlIterator avlit;
        ttree::Iterator<AttrIdPair,AttrComp> tit;
        const AttrIdPair* avlhit;
        AttrIdPair thit;
        bool res;
        bool isAvl;
};


/*

5.17.3  The Value Mapping Functions of operator ~mexactmatch~

*/

template<class T, class R, bool below>
int mexactmatchVMT (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    avlOperLI* li = (avlOperLI*) local.addr;

    switch (message)
    {
        case OPEN: {
            if(li){
                delete li;
                local.addr=0;
            }
            
            R* relN = (R*) args[1].addr;
            MemoryRelObject* mro = getMemRel(relN, nl->Second(qp->GetType(s)));
            if(!mro){
              return 0;
            }
            Attribute* key = (Attribute*) args[2].addr;

            bool avl = ((CcBool*) args[3].addr)->GetValue();
            
            T* treeN = (T*) args[0].addr;
            ListExpr subtype = qp->GetType(qp->GetSon(s,2));


            if(avl) {
              memAVLtree* avltree = getAVLtree(treeN, subtype);
              local.addr= new avlOperLI(avltree,
                                      mro->getmmrel(),
                                      key,key,
                                      below);
            } else {
              MemoryTTreeObject* ttree = getTtree(treeN);
              if(ttree) {
                local.addr= new avlOperLI(ttree->gettree(),
                                      mro->getmmrel(),
                                      key,key,
                                      below);
              } else {
                return 0;
              }
            }
            return 0;
        }

        case REQUEST:
            if(below){
               result.addr=li?li->matchbelow():0;
            } else {
               result.addr=li?li->next():0;
            }
            return result.addr?YIELD:CANCEL;

        case CLOSE:
            if(li) {
              delete li;
              local.addr = 0;
            }
            return 0;
   }

    return 0;
}


ValueMapping mexactmatchVM[] = {
   mexactmatchVMT<CcString, CcString, false>,
   mexactmatchVMT<CcString, Mem, false>,
   mexactmatchVMT<CcString, MPointer, false>,
   mexactmatchVMT<Mem, CcString, false>,
   mexactmatchVMT<Mem, Mem, false>,
   mexactmatchVMT<Mem, MPointer, false>,
   mexactmatchVMT<MPointer, CcString, false>,
   mexactmatchVMT<MPointer, Mem, false>,
   mexactmatchVMT<MPointer, MPointer, false>,
};

ValueMapping matchbelowVM[] = {
   mexactmatchVMT<CcString, CcString, true>,
   mexactmatchVMT<CcString, Mem, true>,
   mexactmatchVMT<CcString, MPointer, true>,
   mexactmatchVMT<Mem, CcString, true>,
   mexactmatchVMT<Mem, Mem, true>,
   mexactmatchVMT<Mem, MPointer, true>,
   mexactmatchVMT<MPointer, CcString, true>,
   mexactmatchVMT<MPointer, Mem, true>,
   mexactmatchVMT<MPointer, MPointer, true>,
};

int mexactmatchSelect(ListExpr args){
  int n1 = CcString::checkType(nl->First(args))
           ?0
           :Mem::checkType(nl->First(args))
           ?3
           :6;
  
  int n2 = CcString::checkType(nl->Second(args))
           ?0
           :Mem::checkType(nl->Second(args))
           ?1
           :2;
           

  return n1 + n2;
}

/*

5.17.4 Description of operator ~mexactmatch~

*/

OperatorSpec mexactmatchSpec(
    "{string, mem(avltree T) x {string, mem(rel(tuple(X)))} "
    " x T -> stream(Tuple(X))",
    "_ _ mexactmatch[_]",
    "Uses the given MemoryAVLObject (as first argument) to find all tuples "
    "in the given MemoryRelObject (as second argument) "
    "that have the same attribute value",
    "query \"Staedte_SName\", \"Staedte\" mexactmatch [\"Dortmund\"] count"
);

/*

5.17.5 Instance of operator ~mexactmatch~

*/
Operator mexactmatchOp (
    "mexactmatch",
    mexactmatchSpec.getStr(),
    9,
    mexactmatchVM,
    mexactmatchSelect,
    mexactmatchTM
);


/*

5.19.4 Description of operator ~matchbelow~

*/

OperatorSpec matchbelowSpec(
    "{string, mem(avltree T)}  x {string, mem(rel(tuple(X)))}  "
    "x T -> stream(tuple(X)) ",
    "_ _ matchbelow[_]",
    "returns for a key  (third argument) the tuple which "
    " contains the biggest attribute value in the AVLtree (first argument) "
    " which is smaller than key  or equal to key",
    "query \"Staedte_SName\" \"Staedte\" matchbelow [\"Dortmund\"] count"
);

Operator matchbelowOp (
    "matchbelow",
    matchbelowSpec.getStr(),
    9,
    matchbelowVM,
    mexactmatchSelect,
    mexactmatchTM
);


/*
5.18 Operator ~mrange~
        Uses the given MemoryAVLObject or MemoryTTreeObject (as first argument)
        to find all tuples in the given MemoryRelObject (as second argument)
        which are between the first and the second attribute value
        (as third and fourth argument)

5.18.1 Type Mapping Functions of operator ~mrange~
    string x string x key x key -> stream(Tuple)


*/
ListExpr mrangeTypeMap(ListExpr args)
{
    if(nl->ListLength(args)!=4){
        return listutils::typeError("four arguments expected");
    }
    ListExpr a1 = nl->First(args);
    ListExpr a2 = nl->Second(args);
    ListExpr a3 = nl->Third(args);
    ListExpr a4 = nl->Fourth(args);

    string err = "{string, mem(avltree)}  x "
                 "{string, mem(rel)} x T x T expected";

    string errMsg;
    if(!getMemType(nl->First(a1), nl->Second(a1), a1, errMsg)){
      return listutils::typeError(err + "\n problem in first arg:" + errMsg);
    }

    if(!getMemType(nl->First(a2), nl->Second(a2), a2, errMsg)){
      return listutils::typeError(err + "\n problem in first arg:" + errMsg);
    }

    a1 = nl->Second(a1); // remove mem
    a2 = nl->Second(a2); 
    a3 = nl->First(a3);  // extract type
    a4 = nl->First(a4);

    if(!MemoryAVLObject::checkType(a1) && !MemoryTTreeObject::checkType(a1)){
      return listutils::typeError(err 
                    + " (first arg is not an avl tree or a ttree)");
    }
    if(!Relation::checkType(a2)){
      return listutils::typeError(err + " (second arg is not a relation)");
    }
    ListExpr avlType = nl->Second(a1);
    if(!nl->Equal(avlType, a3)){
      return listutils::typeError("avltype and type of arg 3 differ");
    }
    if(!nl->Equal(avlType, a4)){
      return listutils::typeError("avltype and type of arg 4 differ");
    }
    return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                           nl->Second(a2));
}

/*

5.18.3  The Value Mapping Functions of operator ~mrange~

*/

template<class T, class R>
int mrangeVMT (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

    avlOperLI* li = (avlOperLI*) local.addr;

    switch (message)
    {
        case OPEN: {
            if(li){
                delete li;
                local.addr=0;
            }
            
            R* relN = (R*) args[1].addr;
            MemoryRelObject* mro = getMemRel(relN, nl->Second(qp->GetType(s)));
            if(!mro){
              return 0;
            }
            Attribute* key1 = (Attribute*) args[2].addr;
            Attribute* key2 = (Attribute*) args[3].addr;
            T* treeN = (T*) args[0].addr;

            ListExpr subtype = qp->GetType(qp->GetSon(s,2));
            memAVLtree* avltree = getAVLtree(treeN, subtype);
            if(avltree) {
              local.addr= new avlOperLI(avltree,
                                      mro->getmmrel(),
                                      key1,key2,
                                      false);
            } else {
              MemoryTTreeObject* ttree = getTtree(treeN);
              if(ttree) {
                local.addr= new avlOperLI(ttree->gettree(),
                                      mro->getmmrel(),
                                      key1,key2,
                                      false);
              }
              else {
                return 0;
              }
            }
            return 0;
        }

        case REQUEST:
            result.addr=li?li->next():0;
            return result.addr?YIELD:CANCEL;

        case CLOSE:
            if(li) {
              delete li;
              local.addr = 0;
            }
            return 0;
   }

   return -1;
}

ValueMapping mrangeVM[] = {
    mrangeVMT<CcString, CcString>,
    mrangeVMT<CcString, Mem>,
    mrangeVMT<Mem, CcString>,
    mrangeVMT<Mem, Mem>
};

int mrangeSelect(ListExpr args){
   int n1 = CcString::checkType(nl->First(args))?0:2;
   int n2 = CcString::checkType(nl->Second(args))?0:1;
   return n1+n2;
}

/*

5.18.4 Description of operator ~mrange~

*/

OperatorSpec mrangeSpec(
    "{string,mem(avltree T) } x {string, mem(rel(tuple(X)))} "
    "x T x T -> stream(Tuple(X)) ",
    "_ _ mrange[_,_]",
    "Uses the given avl-tree to find all tuples"
    " in the given relation which are between "
     "the first and the second attribute value.",
    "query 'Staedte_SName' 'Staedte' mrange ['Aachen','Dortmund'] count"
);

/*

5.18.5 Instance of operator ~mrange~

*/
Operator mrangeOp (
    "mrange",
    mrangeSpec.getStr(),
    4,
    mrangeVM,
    mrangeSelect,
    mrangeTypeMap
);



/*
Operators providing a tid-stream

mexactzmatchS
mrangeS
matchbelowS

*/
template<bool wrap>
ListExpr mexactmatchSTM(ListExpr args){
  string err =" (mem (avl T)) x T  expected";
  if(!nl->HasLength(args,2)){
     return listutils::typeError(err + " (wrong number of args)");
  }
  string errMsg;
  ListExpr a1 = nl->First(args);
  if(!getMemType(nl->First(a1), nl->Second(a1), a1, errMsg)){
    return listutils::typeError(err + "\n problem in 1st arg: " + errMsg);
  }
  a1 = nl->Second(a1);
  if(!MemoryAVLObject::checkType(a1)){
    return listutils::typeError(err + " (1st arg i not an avl tree)");
  }
  if(!nl->Equal(nl->Second(a1), nl->First(nl->Second(args)))){
     return listutils::typeError("avl type and search type differ");
  }
  if(!wrap){
      return nl->TwoElemList( 
                   listutils::basicSymbol<Stream<TupleIdentifier> >(),
                   listutils::basicSymbol<TupleIdentifier>());
  } else {
      ListExpr attrList = nl->OneElemList(
                            nl->TwoElemList(
                                nl->SymbolAtom("TID"),
                                listutils::basicSymbol<TupleIdentifier>()));
      return nl->TwoElemList(
                     listutils::basicSymbol<Stream<Tuple> >(),
                     nl->TwoElemList(
                         listutils::basicSymbol<Tuple>(),
                         attrList));
  }
}


template<bool wrap>
ListExpr mrangeSTM(ListExpr args){
  string err =" (mem (avl T)) x T x T  expected";
  if(!nl->HasLength(args,3)){
     return listutils::typeError(err + " (wrong number of args)");
  }
  string errMsg;
  ListExpr a1 = nl->First(args);
  if(!getMemType(nl->First(a1), nl->Second(a1), a1, errMsg)){
    return listutils::typeError(err + "\n problem in 1st arg: " + errMsg);
  }
  a1 = nl->Second(a1);
  if(!MemoryAVLObject::checkType(a1)){
    return listutils::typeError(err + " (1st arg i not an avl tree)");
  }
  if(!nl->Equal(nl->Second(a1), nl->First(nl->Second(args)))){
     return listutils::typeError("avl type and search type  1 differ");
  }
  if(!nl->Equal(nl->Second(a1), nl->First(nl->Third(args)))){
     return listutils::typeError("avl type and search type 2 differ");
  }
  if(!wrap){
      return nl->TwoElemList( 
                   listutils::basicSymbol<Stream<TupleIdentifier> >(),
                   listutils::basicSymbol<TupleIdentifier>());
  } else {
      ListExpr attrList = nl->OneElemList(
                            nl->TwoElemList(
                                nl->SymbolAtom("TID"),
                                listutils::basicSymbol<TupleIdentifier>()));
      return nl->TwoElemList(
                     listutils::basicSymbol<Stream<Tuple> >(),
                     nl->TwoElemList(
                         listutils::basicSymbol<Tuple>(),
                         attrList));
  }
}

class AVLOpS{
  public:
     AVLOpS(memAVLtree* _tree, 
            Attribute* _beg, Attribute* _end): 
        tree(_tree), end(_end){
        it = tree->tail(AttrIdPair(_beg,0));
        tt = 0;
     }

     AVLOpS(memAVLtree* _tree, 
            Attribute* _beg, 
            Attribute* _end,
            ListExpr tupleType): 
        tree(_tree), end(_end){
        it = tree->tail(AttrIdPair(_beg,0));
        tt = new TupleType(tupleType);
     }
  
     ~AVLOpS(){
       if(tt){
         tt->DeleteIfAllowed();
       }
     }
     

     TupleIdentifier* nextTID(){
         if(it.onEnd()){
           return 0;
         } 
         const AttrIdPair* p = *it;
         it++;
         int cmp = p->getAttr()->Compare(end);
         if(cmp>0){
           return 0;
         }
         return new TupleIdentifier(true,p->getTid());
     }

     Tuple* nextTuple(){
        assert(tt);
        TupleIdentifier* tid = nextTID();
        if(!tid){
          return 0;
        }
        Tuple* res = new Tuple(tt);
        res->PutAttribute(0,tid);
        return res; 
     }



   private:
       memAVLtree* tree;
       Attribute* end;
       avlIterator it;
       TupleType* tt;
};


template<class T, bool wrap>
int mrangeSVMT (Word* args, Word& result,
                    int message, Word& local, Supplier s) {


  AVLOpS* li = (AVLOpS*) local.addr;
  switch(message){
     case OPEN:{
        if(li){
          delete li;
          local.addr = 0;
        }
        T* treeN = (T*) args[0].addr;
        memAVLtree* tree = getAVLtree(treeN, qp->GetType(qp->GetSon(s,1)));
        if(!tree){
           return 0;
        }
        Attribute* beg = (Attribute*) args[1].addr;
        Attribute* end = (Attribute*) args[qp->GetNoSons(s)-1].addr;
        if(!wrap){
           local.addr = new AVLOpS(tree,beg,end);
        } else {
           local.addr = new AVLOpS(tree,beg,end,
                                   nl->Second(GetTupleResultType(s)));
        }
        return 0;
     }
     case REQUEST: {
        if(!li){
           result.addr = 0;
        } else {
           if(wrap){
              result.addr = li->nextTuple();
           } else {
              result.addr = li->nextTID();
           }
        }
        return result.addr?YIELD:CANCEL;
     }
          
     case CLOSE:{
         if(li){
            delete li;
            local.addr = 0;
         }
         return 0;
     }
  }
  return -1;
}


ValueMapping mrangeSVM[] = {
   mrangeSVMT<CcString,true>,
   mrangeSVMT<Mem, true>
};

int mrangeSSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}


OperatorSpec mexactmatchSSpec(
  "{string, mem(avltree T) x T -> stream(tuple((TID tid))) ",
  "_ memexactmatchS[_]",
  "Retrieves the tuple ids from an avl-tree whose "
  "keys have the value given by the second arg.",
  "query \"strassen_Name\" mexactmatch[\"Hirzerweg\"] count"
);

OperatorSpec mrangeSSpec(
  "{string, mem(avltree T) x T x T -> stream(tuple((TID tid)))",
  "_ mrangeS[_,_] ",
  "Retrieves the tuple ids from an avl-tree whose key "
  "is within the range defined by the last two arguments.",
  "query \"strassen_Name\" mrangeS[\"A\",\"B\"] count"
);



Operator mexactmatchSOp(
  "mexactmatchS",
  mexactmatchSSpec.getStr(),
  2,
  mrangeSVM,
  mrangeSSelect,
  mexactmatchSTM<true>
);

Operator mrangeSOp(
  "mrangeS",
  mrangeSpec.getStr(),
  2,
  mrangeSVM,
  mrangeSSelect,
  mrangeSTM<true>
);

template<bool wrap>
ListExpr matchbelowSTM(ListExpr args){
  string err =" (mem (avl T)) x T  expected";
  if(!nl->HasLength(args,2)){
     return listutils::typeError(err + " (wrong number of args)");
  }
  string errMsg;
  ListExpr a1 = nl->First(args);
  if(!getMemType(nl->First(a1), nl->Second(a1), a1, errMsg)){
    return listutils::typeError(err + "\n problem in 1st arg: " + errMsg);
  }
  a1 = nl->Second(a1);
  if(!MemoryAVLObject::checkType(a1)){
    return listutils::typeError(err + " (1st arg i not an avl tree)");
  }
  if(!nl->Equal(nl->Second(a1), nl->First(nl->Second(args)))){
     return listutils::typeError("avl type and search type differ");
  }
  if(!wrap){
      return nl->TwoElemList( 
                   listutils::basicSymbol<Stream<TupleIdentifier> >(),
                   listutils::basicSymbol<TupleIdentifier>());
  } else {
      ListExpr attrList = nl->OneElemList(
                            nl->TwoElemList(
                                nl->SymbolAtom("TID"),
                                listutils::basicSymbol<TupleIdentifier>()));
      return nl->TwoElemList(
                     listutils::basicSymbol<Stream<Tuple> >(),
                     nl->TwoElemList(
                         listutils::basicSymbol<Tuple>(),
                         attrList));
  }
}


template<class T, bool wrap>
int matchbelowSVMT (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

   switch (message){
      case OPEN: {
           if(local.addr){
             if(wrap){
                ((Tuple*)local.addr)->DeleteIfAllowed();
             } else {
                ((TupleIdentifier*)local.addr)->DeleteIfAllowed();
             }
             local.addr=0;
           }
           T* treeN = (T*) args[0].addr;
           memAVLtree* tree = getAVLtree(treeN, qp->GetType(qp->GetSon(s,1)));
           if(!tree){
             return 0;
           }
           Attribute* attr = (Attribute*) args[1].addr;
           AttrIdPair searchP(attr, numeric_limits<size_t>::max()); 
           const AttrIdPair* p = tree->GetNearestSmallerOrEqual(searchP);
           if(p){
              TupleIdentifier* tid =  new TupleIdentifier(true,p->getTid());
              if(wrap){
                 TupleType* tt = new TupleType(nl->Second(
                                               GetTupleResultType(s)));
                 Tuple* res = new Tuple(tt);
                 res->PutAttribute(0,tid);
                 local.addr = res; 
                 tt->DeleteIfAllowed();
              } else {
                 local.addr = tid;
              }
           } 
           return 0;
      }
      case REQUEST : {
             result.addr = local.addr;
             local.addr = 0;
             return result.addr?YIELD:CANCEL;
      }
      case CLOSE :{
           if(local.addr){
             if(wrap){
                ((Tuple*)local.addr)->DeleteIfAllowed();
             } else {
                ((TupleIdentifier*)local.addr)->DeleteIfAllowed();
             }
             local.addr=0;
           }
             return 0;
      }

   }
   return -1;
}

ValueMapping matchbelowSVM[] = {
   matchbelowSVMT<CcString, true>,
   matchbelowSVMT<Mem, true>
};

int matchbelowSSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}

OperatorSpec matchbelowSSpec(
  "{string, mem(avltree T) x T -> stream(tuple((TID tid))) ",
  "_ matchbelowS[_]",
  "Returns the tid of the entry in the avl tree whose key "
  "is smaller or equal to the key attribute. If the given "
  "key is smaller than all stored values, the resulting stream "
  "will be empty.",
  "query \"strassen_Name\" matchbelowS[\"B\"] count"
);

Operator matchbelowSOp(
  "matchbelowS",
  matchbelowSSpec.getStr(),
  2,
  matchbelowSVM,
  matchbelowSSelect,
  matchbelowSTM<true>
);


/*
6 M-tree support


6.1 mcreateMtree2: Creation of an M-tree for a persistent relation

6.1.1 Type Mapping

*/


ListExpr mcreateMtree2TM(ListExpr args){
  string err="expected: stream(tuple) x attrname x attrname x memory name ";
  if(!nl->HasLength(args,4)){
    return listutils::typeError(err+" (wrong number of args)");
  }
  if(!Stream<Tuple>::checkType(nl->First(args))){
    return listutils::typeError(err + " (first arg is not a tuple stream)");
  }
  if(nl->AtomType(nl->Second(args)) != SymbolType){
    return listutils::typeError(err + " (second argument is not a valid "
                                "attribute name)");
  }
  if(nl->AtomType(nl->Third(args)) != SymbolType){
    return listutils::typeError(err + " (third argument is not a valid "
                                "attribute name)");
  }
  if(!CcString::checkType(nl->Fourth(args))){
    return listutils::typeError(err + " (third argument is not a string");
  }

  ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
  string name = nl->SymbolValue(nl->Second(args));
  ListExpr type;
  int index1 = listutils::findAttribute(attrList, name, type);
  if(!index1){
    return listutils::typeError("attribute " + name 
                                + " not part of the tuple");
  }
  string tidname = nl->SymbolValue(nl->Third(args));
  ListExpr tidtype;
  int index2 = listutils::findAttribute(attrList, tidname, tidtype);
  if(!index2){
     return listutils::typeError("attribute " + tidname 
                                 + "not known in tuple");
  }
  if(!TupleIdentifier::checkType(tidtype)){
     return listutils::typeError("attribute " + tidname 
                                 + " not of type tid");
  }

  // check for supported type, extend if required
  ListExpr resType = nl->TwoElemList(
                        listutils::basicSymbol<Mem>(),
                        nl->TwoElemList(
                           listutils::basicSymbol<
                             MemoryMtreeObject<Point, StdDistComp<Point> > >(),
                           type
                        ));


  ListExpr result = nl->ThreeElemList(
                     nl->SymbolAtom(Symbols::APPEND()),
                     nl->ThreeElemList( 
                         nl->IntAtom(index1-1),
                         nl->IntAtom(index2-1),
                         nl->StringAtom(nl->ToString(type))),
                     resType);

  int no = mtreehelper::getTypeNo(type,9);
  if(no <0){
     return listutils::typeError("there is no known distance fuction for type "
                               + nl->ToString(type));
  }
  return result;
}





/*
6.2 Value Mapping template

*/
template <class T>
int mcreateMtree2VMT (Word* args, Word& result, int message,
                    Word& local, Supplier s) {

   result = qp->ResultStorage(s);
   Mem* res = (Mem*) result.addr;

   CcString* name = (CcString*) args[3].addr;
   if(!name->IsDefined()){
      // invalid name
      res->SetDefined(false);
      return 0;
   }
   string n = name->GetValue();
   if(catalog->isMMObject(n)){
      // name already used
      res->SetDefined(false);
      return 0;
   }

   int index1 = ((CcInt*) args[4].addr)->GetValue(); 
   int index2 = ((CcInt*) args[5].addr)->GetValue(); 
   string tn = ((CcString*) args[6].addr)->GetValue();

   StdDistComp<T> dc;
   MMMTree<pair<T,TupleId>,StdDistComp<T> >* tree = 
           new MMMTree<pair<T,TupleId>,StdDistComp<T> >(4,8,dc);

   Stream<Tuple> stream(args[0]);
   stream.open();
   Tuple* tuple;

   bool flobused = false;
   while( (tuple = stream.request())){
      T* attr = (T*) tuple->GetAttribute(index1);
      TupleIdentifier* tid = (TupleIdentifier*) tuple->GetAttribute(index2);
      if(tid->IsDefined()){
        T copy = *attr;
        flobused = flobused || (copy.NumOfFLOBs()>0);
        pair<T,TupleId> p(copy, tid->GetTid());
        tree->insert(p);
      }
      tuple->DeleteIfAllowed();
   }
   stream.close();
   size_t usedMem = tree->memSize();
   ListExpr typeList = nl->TwoElemList( 
                            listutils::basicSymbol<Mem>(),
                            nl->TwoElemList(
                                 nl->SymbolAtom(mtreehelper::BasicType()),
                                 nl->SymbolAtom(tn)));

   MemoryMtreeObject<T,StdDistComp<T> >* mtree = 
          new MemoryMtreeObject<T, StdDistComp<T> > (tree,  
                             usedMem, 
                             nl->ToString(typeList), 
                             !flobused, getDBname());
   bool success = catalog->insert(n, mtree);
   res->set(success, n);
   return 0;
}

/*
6.3 Selection and  Value Mapping Array

*/
int mcreateMtree2Select(ListExpr args){
   ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
   string attrName = nl->SymbolValue(nl->Second(args));
   ListExpr type;
   listutils::findAttribute(attrList, attrName, type);
   return mtreehelper::getTypeNo(type,9);
   assert(false);
   return -1; // invalid
}

 // note: if adding attributes with flobs, the value mapping must be changed

ValueMapping mcreateMtree2VM[] = {
  mcreateMtree2VMT<mtreehelper::t1>,
  mcreateMtree2VMT<mtreehelper::t2>,
  mcreateMtree2VMT<mtreehelper::t3>,
  mcreateMtree2VMT<mtreehelper::t4>,
  mcreateMtree2VMT<mtreehelper::t5>,
  mcreateMtree2VMT<mtreehelper::t6>,
  mcreateMtree2VMT<mtreehelper::t7>,
  mcreateMtree2VMT<mtreehelper::t8>,
  mcreateMtree2VMT<mtreehelper::t9>
};

OperatorSpec mcreateMtree2Spec(
  "stream(tuple) x attrname x attrname x string -> mem(mtree X) ",
  "elements mcreateMtreeSpec[ indexAttr, TID_attr, mem_name]",
  "creates an main memory m tree from a tuple stream",
  "query kinos feed addid mcreateMtree2[GeoData, TID, \"kinos_mtree\"]"
);

Operator mcreateMtree2Op(
   "mcreateMtree2",
   mcreateMtree2Spec.getStr(),
   4,
   mcreateMtree2VM,
   mcreateMtree2Select,
   mcreateMtree2TM
);


/*
Operator ~mdistRange2~

This operator creates a stream of TupleIDs that are inside a 
given distance to a reference object. The used index is a 
main memory based mtree.

*/
template<bool wrap>
ListExpr mdistRange2TM(ListExpr args){

  string err = "{string, mem(mtree T)}  x T x real expected";
  if(!nl->HasLength(args,3)){
     return listutils::typeError(err + " ( wrong number of args)");
  }

  string errMsg;
  ListExpr a1 = nl->First(args);
  if(!getMemType(nl->First(a1), nl->Second(a1), a1, errMsg)){
    return listutils::typeError(err + "\n error in first arg: " + errMsg);
  }
  a1 = nl->Second(a1);
  ListExpr mt = nl->TwoElemList(
                          nl->SymbolAtom(mtreehelper::BasicType()),
                          nl->First(nl->Second(args)));

  if(!nl->Equal(a1, mt)){
    return listutils::typeError("arg 1 is not an mtree over arg 2 (" 
                                + nl->ToString(nl->First(nl->Second(args)))
                                + ")");
  }
  if(!CcReal::checkType(nl->First(nl->Third(args)))){
     return listutils::typeError(err + " (third arg is not a real)");
  }
  if(!wrap){
      return nl->TwoElemList( 
                   listutils::basicSymbol<Stream<TupleIdentifier> >(),
                   listutils::basicSymbol<TupleIdentifier>());
  } else {
      ListExpr attrList = nl->OneElemList(
                            nl->TwoElemList(
                                nl->SymbolAtom("TID"),
                                listutils::basicSymbol<TupleIdentifier>()));
      return nl->TwoElemList(
                     listutils::basicSymbol<Stream<Tuple> >(),
                     nl->TwoElemList(
                         listutils::basicSymbol<Tuple>(),
                         attrList));
  }
}

template<class T>
struct mdistrange2Info{
   RangeIterator<pair<T,TupleId>, StdDistComp<T>  >* it;
   TupleType* tt;
};


template <class T, class N, bool wrap>
int mdistRange2VMT (Word* args, Word& result, int message,
                    Word& local, Supplier s) {

  mdistrange2Info<T>* li = (mdistrange2Info<T>*) local.addr;
  switch(message){
    case OPEN: {
            if(li) {
              delete li->it;
              if(li->tt){
                li->tt->DeleteIfAllowed();
              }
              local.addr = 0;
            }

            T* attr = (T*) args[1].addr;
            CcReal* dist = (CcReal*) args[2].addr;
            if(!dist->IsDefined()){
               return 0;
            }
            double d = dist->GetValue();
            if(d < 0){
               return 0;
            }
            N* Name = (N*) args[0].addr;
            typedef  MemoryMtreeObject<T,StdDistComp<T> > mtot;
            mtot* mtreeo = getMtree<N,T>(Name);
            if(!mtreeo){
              return 0;
            }            
            typedef MMMTree<pair<T,TupleId>,StdDistComp<T> > mtt;
            mtt* mtree = mtreeo->getmtree();
            if(mtree){
                T a = *attr;
                mdistrange2Info<T>* info = new mdistrange2Info<T>();
                info->it = mtree->rangeSearch(pair<T,TupleId>(a,0), d);
                if(!wrap){
                   info->tt = 0;
                } else {
                   info->tt = new TupleType(nl->Second(GetTupleResultType(s)));
                }
                local.addr = info;
            }
            return 0;
          }
     case REQUEST: {
               if(!li){
                 result.addr=0;
                 return CANCEL;
               }
               const pair<T,TupleId>* n = li->it->next();
               TupleIdentifier* res = n? new TupleIdentifier(true,n->second):0;
               if(!res){
                 result.addr=0;
                 return CANCEL;
               }
               if(!wrap){
                 result.addr = res;
               } else {
                  Tuple* tuple = new Tuple(li->tt);
                  tuple->PutAttribute(0,res);
                  result.addr = tuple;
               }
               return result.addr?YIELD:CANCEL;
            }
     case CLOSE:
               if(li){
                   delete li->it;
                   if(li->tt){
                      li->tt->DeleteIfAllowed();
                   }
                   delete li;
                   local.addr = 0;
               }
               return 0;
     }
     return -1;            
}

/*
6.3 Selection and  Value Mapping Array

*/
int mdistRange2Select(ListExpr args){
   ListExpr type = nl->Second(args);
   int m = 9;
   int n;
   if(CcString::checkType(nl->First(args))){
      n = 0;
   } else {
      n = m;
   }

   int res =  mtreehelper::getTypeNo(type,m) + n;

   return res;

}

 // note: if adding attributes with flobs, the value mapping must be changed

ValueMapping mdistRange2VM[] = {
  mdistRange2VMT<mtreehelper::t1, CcString, true>,
  mdistRange2VMT<mtreehelper::t2, CcString, true>,
  mdistRange2VMT<mtreehelper::t3, CcString, true>,
  mdistRange2VMT<mtreehelper::t4, CcString, true>,
  mdistRange2VMT<mtreehelper::t5, CcString, true>,
  mdistRange2VMT<mtreehelper::t6, CcString, true>,
  mdistRange2VMT<mtreehelper::t7, CcString, true>,
  mdistRange2VMT<mtreehelper::t8, CcString, true>,
  mdistRange2VMT<mtreehelper::t9, CcString, true>,

  mdistRange2VMT<mtreehelper::t1, Mem, true>,
  mdistRange2VMT<mtreehelper::t2, Mem, true>,
  mdistRange2VMT<mtreehelper::t3, Mem, true>,
  mdistRange2VMT<mtreehelper::t4, Mem, true>,
  mdistRange2VMT<mtreehelper::t5, Mem, true>,
  mdistRange2VMT<mtreehelper::t6, Mem, true>,
  mdistRange2VMT<mtreehelper::t7, Mem, true>,
  mdistRange2VMT<mtreehelper::t8, Mem, true>,
  mdistRange2VMT<mtreehelper::t9, Mem, true>
};

OperatorSpec mdistRange2Spec(
  "string x DATA x real -> stream(tuple((TID tid))) ",
  "mem_mtree mdistRange2[keyAttr, maxDist] ",
  "Retrieves those tuple ids from an mtree those key value has "
  "a maximum distaance of the given dist",
  "query \"kinos_mtree\" mdistRange2[ alexanderplatz , 2000.0] count"
);

Operator mdistRange2Op(
   "mdistRange2",
   mdistRange2Spec.getStr(),
   18,
   mdistRange2VM,
   mdistRange2Select,
   mdistRange2TM<true>
);


/*
Operator ~mdistScan2~

This operator creates a stream of TupleIDs 
whose associated objects are in increasing order
to the reference object.

*/
template<bool wrap>
ListExpr mdistScan2TM(ListExpr args){
  string err = "{string, mem(mtree (T))}  x T  expected";
  if(!nl->HasLength(args,2)){
     return listutils::typeError(err + " ( wrong number of args)");
  }
  ListExpr a1 = nl->First(args);
  ListExpr a2 = nl->Second(args);
  string errMsg;
  if(!getMemType(nl->First(a1), nl->Second(a1), a1,  errMsg)){
    return listutils::typeError(err + "\n problem in 1st arg: " + errMsg);
  }
  a1 = nl->Second(a1); // remove mem
  a2 = nl->First(a2);  // extract type
  if(!mtreehelper::checkType(a1,a2)){
     return listutils::typeError(err+ "( first arg is not an "
                                      "mtree over key type)");
  }
  if(!wrap){
      return nl->TwoElemList( 
                   listutils::basicSymbol<Stream<TupleIdentifier> >(),
                   listutils::basicSymbol<TupleIdentifier>());
  } else {
      ListExpr attrList = nl->OneElemList(
                            nl->TwoElemList(
                                nl->SymbolAtom("TID"),
                                listutils::basicSymbol<TupleIdentifier>()));
      return nl->TwoElemList(
                     listutils::basicSymbol<Stream<Tuple> >(),
                     nl->TwoElemList(
                         listutils::basicSymbol<Tuple>(),
                         attrList));
  }
}

template<class T>
struct mdistScan2Info{
   NNIterator<pair<T,TupleId>, StdDistComp<T>  >* it;
   TupleType* tt;
};


template <class T, class N, bool wrap>
int mdistScan2VMT (Word* args, Word& result, int message,
                    Word& local, Supplier s) {
  mdistScan2Info<T>* li = (mdistScan2Info<T>*) local.addr;  
  switch(message){
    case OPEN: {
            if(li) {
              delete li->it;
              if(li->tt){
                li->tt->DeleteIfAllowed();
              }
              delete li;
              local.addr = 0;
            }
            T* attr = (T*) args[1].addr;
            N* Name = (N*) args[0].addr;
            typedef MemoryMtreeObject<T,StdDistComp<T> > mtot;
            mtot* tree = getMtree<N,T>(Name);
            if(!tree){
              return 0;
            }
            MMMTree<pair<T, TupleId>,StdDistComp<T> >* mtree = 
                    tree->getmtree();
            if(mtree){
              pair<T,TupleId> p(*attr,0);
              li = new mdistScan2Info<T>();
              li->it =  mtree->nnSearch(p);
              if(!wrap){
                li->tt=0;
              } else {
                li->tt = new TupleType(nl->Second(GetTupleResultType(s)));
              }
              local.addr = li;
            }
            return 0;
          }
     case REQUEST: {
               if(!li){
                 result.addr=0;
                 return CANCEL;
               }
               const pair<T,TupleId>* n = li->it->next();
               TupleIdentifier* tid = n? new TupleIdentifier(true,n->second):0;
               if(!tid){
                 result.addr=0;
                 return CANCEL;
               } 
               if(!wrap){
                  result.addr = tid;
               } else {
                  Tuple* tuple = new Tuple(li->tt);
                  tuple->PutAttribute(0,tid);
                  result.addr = tuple;
               }
                
               return result.addr?YIELD:CANCEL;
            }
     case CLOSE:
               if(li){
                 delete li->it;
                 if(li->tt){
                    li->tt->DeleteIfAllowed();
                 }
                 delete li;
                 local.addr = 0;
               }
               return 0;
     }
     return -1;            
}

/*
6.3 Selection and  Value Mapping Array

*/
int mdistScan2Select(ListExpr args){
   ListExpr type = nl->Second(args);
   int n;
   int m = 9;
   if(CcString::checkType(nl->First(args))){
     n = 0;
   } else {
     n = m;
   }

   return mtreehelper::getTypeNo(type,m) + n;
}

 // note: if adding attributes with flobs, the value mapping must be changed

ValueMapping mdistScan2VM[] = {
  mdistScan2VMT<mtreehelper::t1, CcString, true>,
  mdistScan2VMT<mtreehelper::t2, CcString, true>,
  mdistScan2VMT<mtreehelper::t3, CcString, true>,
  mdistScan2VMT<mtreehelper::t4, CcString, true>,
  mdistScan2VMT<mtreehelper::t5, CcString, true>,
  mdistScan2VMT<mtreehelper::t6, CcString, true>,
  mdistScan2VMT<mtreehelper::t7, CcString, true>,
  mdistScan2VMT<mtreehelper::t8, CcString, true>,
  mdistScan2VMT<mtreehelper::t9, CcString, true>,
  
  mdistScan2VMT<mtreehelper::t1, Mem, true>,
  mdistScan2VMT<mtreehelper::t2, Mem, true>,
  mdistScan2VMT<mtreehelper::t3, Mem, true>,
  mdistScan2VMT<mtreehelper::t4, Mem, true>,
  mdistScan2VMT<mtreehelper::t5, Mem, true>,
  mdistScan2VMT<mtreehelper::t6, Mem, true>,
  mdistScan2VMT<mtreehelper::t7, Mem, true>,
  mdistScan2VMT<mtreehelper::t8, Mem, true>,
  mdistScan2VMT<mtreehelper::t9, Mem, true>
};

OperatorSpec mdistScan2Spec(
  "{string, (mem (mtree DATA))}  x DATA -> stream(tuple((TID tid))) ",
  "mem_mtree mdistScan2[keyAttr] ",
  "Scans the tuple ids within an m-tree in increasing "
  "distance of the reference object to the associated "
  "objects.",
  "query \"kinos_mtree\" mdistScan2[ alexanderplatz] count"
);

Operator mdistScan2Op(
   "mdistScan2",
   mdistScan2Spec.getStr(),
   18,
   mdistScan2VM,
   mdistScan2Select,
   mdistScan2TM<true>
);



/*
Operator ~mcreateMTree~

This operator creates an m-tree over a main memory relation.

*/
ListExpr mcreateMtreeTM(ListExpr args){

  string err = "string x Ident  x string expected";
  if(!nl->HasLength(args,3)){
    return listutils::typeError(err + " (wrong number of args)");
  }
  ListExpr a1 = nl->First(args);
  ListExpr a2 = nl->Second(args);
  ListExpr a3 = nl->Third(args);
  string errMsg;
  if(!getMemType(nl->First(a1), nl->Second(a1), a1, errMsg)){
    return listutils::typeError("problem in 1st arg : " + errMsg);
  }

  a1 = nl->Second(a1);
  a2 = nl->First(a2);
  a3 = nl->First(a3);

  if(!Relation::checkType(a1)){
    return listutils::typeError(err + " (first arg is not a mem rel");
  }

  if(nl->AtomType(a2)!=SymbolType){
    return listutils::typeError(err + " (second arg is not a valid Id");
  }
  if(!CcString::checkType(a3)){
    return listutils::typeError(err + " (third arg is not a string)");
  }

  // ListExpr resNameV = nl->Second(nl->Third(args));
  // if(nl->AtomType(resNameV)!=StringType){
  //  return listutils::typeError("third arg must be a constant string");
  // }
  // if(catalog->isMMObject(nl->StringValue(resNameV))){
  //  return listutils::typeError("memory object already there.");
  // }

  ListExpr attrList = nl->Second(nl->Second(a1));
  ListExpr at;
  string attrName = nl->SymbolValue(a2);
  int index = listutils::findAttribute(attrList, attrName, at);
  if(!index){
     return listutils::typeError( attrName+ " is not known in tuple");
  }
  int typeNo = mtreehelper::getTypeNo(at,9);
  if(typeNo < 0){
    return listutils::typeError("Type " + nl->ToString(at) + " not supported");
  }

  ListExpr resType = nl->TwoElemList(
                        nl->SymbolAtom(Mem::BasicType()),
                        nl->TwoElemList(
                             nl->SymbolAtom(mtreehelper::BasicType()),
                             at));

  ListExpr result = nl->ThreeElemList(
                          nl->SymbolAtom(Symbols::APPEND()),
                          nl->TwoElemList(nl->IntAtom(index-1),
                                          nl->IntAtom(typeNo)),
                          resType);
  
  return result;
}



template <class T, class R>
int mcreateMtreeVMT (Word* args, Word& result, int message,
                    Word& local, Supplier s) {

   result = qp->ResultStorage(s);
   Mem* res = (Mem*) result.addr;
   R* RelName = (R*) args[0].addr;
   CcString* Name = (CcString*) args[2].addr; 
   if(!RelName->IsDefined() || !Name->IsDefined()){
      res->SetDefined(false);
      return 0;
   }
   string relName = RelName->GetValue();
   string name = Name->GetValue();
   MemoryObject* mmobj = catalog->getMMObject(relName);
   if(!mmobj){
      res->SetDefined(false);
      return 0;
   }
   ListExpr mmType = nl->Second(catalog->getMMObjectTypeExpr(relName));
   if(!Relation::checkType(mmType)){
      res->SetDefined(false);
      return 0;
   }
   if(catalog->isMMObject(name)){
      // name already used
      res->SetDefined(false);
      return 0;
   }

   int index = ((CcInt*) args[3].addr)->GetValue();
   // extract attribute type
   ListExpr attrList = nl->Second(nl->Second(mmType));
   int i2 = index;
   while(!nl->IsEmpty(attrList) && i2>0){
     attrList = nl->Rest(attrList);
     i2--;
   }
   if(nl->IsEmpty(attrList)){
      res->SetDefined(false);
      return 0;
   }
   ListExpr attrType = nl->Second(nl->First(attrList));
   if(!T::checkType(attrType)){
      res->SetDefined(false);
      return 0;
   }
 

   StdDistComp<T> dc;
   MMMTree<pair<T,TupleId>,StdDistComp<T> >* tree = 
           new MMMTree<pair<T,TupleId>,StdDistComp<T> >(4,8,dc);

   MemoryRelObject* mrel = (MemoryRelObject*) mmobj;

   vector<Tuple*>* rel = mrel->getmmrel();

   // insert attributes

   for(size_t i=0;i<rel->size();i++){
       Tuple* t = (*rel)[i];
       if(t){ // tuple not deleted
         T* attr = (T*) t->GetAttribute(index);
         pair<T,TupleId> p(*attr,i+1);
         tree->insert(p); 
       }
   }
   size_t usedMem = tree->memSize();
   MemoryMtreeObject<T,StdDistComp<T> >* mtree = 
          new MemoryMtreeObject<T, StdDistComp<T> > (tree,  
                             usedMem, 
                             nl->ToString(qp->GetType(s)),
                             mrel->hasflob(), getDBname());
   bool success = catalog->insert(name, mtree);
   res->set(success, name);
   
   return 0;
}

ValueMapping mcreatetreeVMA[] = {
  mcreateMtreeVMT<mtreehelper::t1,CcString>,
  mcreateMtreeVMT<mtreehelper::t2,CcString>,
  mcreateMtreeVMT<mtreehelper::t3,CcString>,
  mcreateMtreeVMT<mtreehelper::t4,CcString>,
  mcreateMtreeVMT<mtreehelper::t5,CcString>,
  mcreateMtreeVMT<mtreehelper::t6,CcString>,
  mcreateMtreeVMT<mtreehelper::t7,CcString>,
  mcreateMtreeVMT<mtreehelper::t8,CcString>,
  mcreateMtreeVMT<mtreehelper::t9,CcString>,
  mcreateMtreeVMT<mtreehelper::t1,Mem>,
  mcreateMtreeVMT<mtreehelper::t2,Mem>,
  mcreateMtreeVMT<mtreehelper::t3,Mem>,
  mcreateMtreeVMT<mtreehelper::t4,Mem>,
  mcreateMtreeVMT<mtreehelper::t5,Mem>,
  mcreateMtreeVMT<mtreehelper::t6,Mem>,
  mcreateMtreeVMT<mtreehelper::t7,Mem>,
  mcreateMtreeVMT<mtreehelper::t8,Mem>,
  mcreateMtreeVMT<mtreehelper::t9,Mem>
};


int mcreateMtreeVM (Word* args, Word& result, int message,
                    Word& local, Supplier s) {

  int typeNo = ((CcInt*) args[4].addr)->GetValue();
  int offset = CcString::checkType(qp->GetType(qp->GetSon(s,0)))?0:9;

  return mcreatetreeVMA[typeNo+ offset](args,result,message,local,s);
}

OperatorSpec mcreateMtreeSpec(
  "(string, mem(rel))  x attrname x string -> string ",
  "memrel  mcreateMtree[ indexAttr, mem_name]",
  "creates an main memory m tree from a main memory relation",
  "query \"mkkinos\"  mcreateMtree[GeoData, \"kinos_mtree\"]"
);

Operator mcreateMtreeOp(
   "mcreateMtree",
   mcreateMtreeSpec.getStr(),
   mcreateMtreeVM,
   Operator::SimpleSelect,
   mcreateMtreeTM
);

/*
Operator mdistRange

*/
ListExpr mdistRangeTM(ListExpr args){
   string err="{string, (mem mtree)} x {string x DATA x real expected";
   if(!nl->HasLength(args,4)){
      return listutils::typeError(err + " (wrong number of args)");
   }

   ListExpr a1 = nl->First(args);
   ListExpr a2 = nl->Second(args);
   string errMsg;
   if(!getMemType(nl->First(a1), nl->Second(a1), a1, errMsg)){
     return listutils::typeError(err + "\n problen in 1st arg: " + errMsg);
   }
   if(!getMemType(nl->First(a2), nl->Second(a2), a2, errMsg)){
     return listutils::typeError(err + "\n problen in 2nd arg: " + errMsg);
   }

   a1 = nl->Second(a1);
   a2 = nl->Second(a2);
   ListExpr a3 = nl->First(nl->Third(args));
   ListExpr a4 = nl->First(nl->Fourth(args));

   if(!mtreehelper::checkType(a1,a3)){
     return listutils::typeError("first arg is not a mtree over " 
                                 + nl->ToString(a1));
   }
   if(!Relation::checkType(a2)){
     return listutils::typeError("second arg is not a relation");
   }

   if(!CcReal::checkType(a4)){
     return listutils::typeError(err + "(4th arg is not a real)");
   }

   return nl->TwoElemList(
                 listutils::basicSymbol<Stream<Tuple> >(),
                 nl->Second(a2)); 
}

template<class T>
class distRangeInfo{
  public:

     distRangeInfo( MemoryMtreeObject<T, StdDistComp<T> >* mtree, 
                    MemoryRelObject* mrel, 
                    T* ref, 
                    double dist){
                 
                rel = mrel->getmmrel();
                pair<T,TupleId> p(*ref,0);
                it = mtree->getmtree()->rangeSearch(p,dist);
              }

     ~distRangeInfo(){
       delete it;
     }

     Tuple* next(){
        while(true){
            const pair<T,TupleId>* p = it->next();
            if(!p){ return 0;}
            if(p->second < rel->size()){
               Tuple* res = (*rel)[p->second-1];
               if(res){ // ignore deleted tuples
                 res->IncReference();
                 return res;
               }
            }
        }
        return 0;
     }


  private:
     vector<Tuple*>* rel;
     RangeIterator<pair<T,TupleId> , StdDistComp<T> >* it;

};

template<class K, class T, class R>
int mdistRangeVMT (Word* args, Word& result, int message,
                    Word& local, Supplier s) {
   distRangeInfo<K>* li = (distRangeInfo<K>*) local.addr;
   switch(message){
     case OPEN : {
               if(li){
                 delete li;
                 local.addr = 0;
               }
               R* relN = (R*) args[1].addr;
               MemoryRelObject* rel = getMemRel(relN, 
                                         nl->Second(qp->GetType(s)));
               if(!rel){
                 return 0;
               }
               CcReal* dist = (CcReal*) args[3].addr;
               if(!dist->IsDefined()){
                 return 0;
               }
               double d = dist->GetValue();
               if(d<0){
                  return 0;
               }
               
               T* treeN = (T*) args[0].addr;
               MemoryMtreeObject<K, StdDistComp<K> >* m = getMtree<T,K>(treeN);
               if(!m){
                 return 0;
               }
               K* key = (K*) args[2].addr;
               local.addr = new distRangeInfo<K>(m,rel,key,d);
               return 0;
             }
      case REQUEST:
               result.addr = li?li->next():0;
               return result.addr?YIELD:CANCEL;
      case CLOSE :
               if(li){
                  delete li;
                  local.addr = 0;
               }
               return 0;
   }
   return -1;
}

int mdistRangeSelect(ListExpr args){
   ListExpr typeL = nl->Third(args);
   int type =  mtreehelper::getTypeNo(typeL,9);
   int o1 = CcString::checkType(nl->First(args))?0:18;
   int o2 = CcString::checkType(nl->Second(args))?0:9;
   return type + o1 + o2;
}

 // note: if adding attributes with flobs, the value mapping must be changed

ValueMapping mdistRangeVM[] = {
  mdistRangeVMT<mtreehelper::t1,CcString,CcString>,
  mdistRangeVMT<mtreehelper::t2,CcString,CcString>,
  mdistRangeVMT<mtreehelper::t3,CcString,CcString>,
  mdistRangeVMT<mtreehelper::t4,CcString,CcString>,
  mdistRangeVMT<mtreehelper::t5,CcString,CcString>,
  mdistRangeVMT<mtreehelper::t6,CcString,CcString>,
  mdistRangeVMT<mtreehelper::t7,CcString,CcString>,
  mdistRangeVMT<mtreehelper::t8,CcString,CcString>,
  mdistRangeVMT<mtreehelper::t9,CcString,CcString>,

  mdistRangeVMT<mtreehelper::t1,CcString,Mem>,
  mdistRangeVMT<mtreehelper::t2,CcString,Mem>,
  mdistRangeVMT<mtreehelper::t3,CcString,Mem>,
  mdistRangeVMT<mtreehelper::t4,CcString,Mem>,
  mdistRangeVMT<mtreehelper::t5,CcString,Mem>,
  mdistRangeVMT<mtreehelper::t6,CcString,Mem>,
  mdistRangeVMT<mtreehelper::t7,CcString,Mem>,
  mdistRangeVMT<mtreehelper::t8,CcString,Mem>,
  mdistRangeVMT<mtreehelper::t9,CcString,Mem>,
  
  mdistRangeVMT<mtreehelper::t1,Mem,CcString>,
  mdistRangeVMT<mtreehelper::t2,Mem,CcString>,
  mdistRangeVMT<mtreehelper::t3,Mem,CcString>,
  mdistRangeVMT<mtreehelper::t4,Mem,CcString>,
  mdistRangeVMT<mtreehelper::t5,Mem,CcString>,
  mdistRangeVMT<mtreehelper::t6,Mem,CcString>,
  mdistRangeVMT<mtreehelper::t7,Mem,CcString>,
  mdistRangeVMT<mtreehelper::t8,Mem,CcString>,
  mdistRangeVMT<mtreehelper::t9,Mem,CcString>,

  mdistRangeVMT<mtreehelper::t1,Mem,Mem>,
  mdistRangeVMT<mtreehelper::t2,Mem,Mem>,
  mdistRangeVMT<mtreehelper::t3,Mem,Mem>,
  mdistRangeVMT<mtreehelper::t4,Mem,Mem>,
  mdistRangeVMT<mtreehelper::t5,Mem,Mem>,
  mdistRangeVMT<mtreehelper::t6,Mem,Mem>,
  mdistRangeVMT<mtreehelper::t7,Mem,Mem>,
  mdistRangeVMT<mtreehelper::t8,Mem,Mem>,
  mdistRangeVMT<mtreehelper::t9,Mem,Mem>,


};

OperatorSpec mdistRangeSpec(
  "{string, mem(mtree T)} x {string, mem(rel(tuple))}  x T  "
  "x real -> stream(tuple) ",
  "mem_mtree mem_rel mdistRange[keyAttr, maxDist] ",
  "Retrieves those tuples from a memory relation "
  "having a distance smaller or equals to a given dist "
  "to a key value. This operation is aided by a memory "
  "based m-tree.",
  "query \"kinos_mtree\" \"Kinos\" mdistRange[ alexanderplatz , 2000.0] count"
);

Operator mdistRangeOp(
   "mdistRange",
   mdistRangeSpec.getStr(),
   36,
   mdistRangeVM,
   mdistRangeSelect,
   mdistRangeTM
);


/*
Operator mdistScan

*/
ListExpr mdistScanTM(ListExpr args){
   string err="{string, mem} x {string,mem} x DATA expected";
   if(!nl->HasLength(args,3)){
      return listutils::typeError(err + " (wrong number of args)");
   }
   ListExpr a1 = nl->First(args);
   string errMsg;
   if(!getMemType(nl->First(a1), nl->Second(a1), a1, errMsg)){
     return listutils::typeError(err + "\n problem in first arg: " + errMsg);
   }
   ListExpr a2 = nl->Second(args);
   if(!getMemType(nl->First(a2), nl->Second(a2), a2, errMsg)){
     return listutils::typeError(err + "\n problem in second arg: " + errMsg);
   }
   a1 = nl->Second(a1);
   a2 = nl->Second(a2);
   ListExpr a3 = nl->First(nl->Third(args));

   if(!mtreehelper::checkType(a1,a3)){
     return listutils::typeError(err + "(first arg is not a mtree over " 
                                 + nl->ToString(a3) + ")");
   }
   if(!Relation::checkType(a2)){
     return listutils::typeError(err + "( second arg is not a mem relation)");
   }
   
   return nl->TwoElemList(
                 listutils::basicSymbol<Stream<Tuple> >(),
                 nl->Second(a2)); 
}

template<class T>
class distScanInfo{
  public:

     distScanInfo( MemoryMtreeObject<T, StdDistComp<T> >* mtree, 
                    MemoryRelObject* mrel, 
                    T* ref){
                 
                rel = mrel->getmmrel();
                pair<T,TupleId> p(*ref,0);
                it = mtree->getmtree()->nnSearch(p);
              }

     ~distScanInfo(){
       delete it;
     }

     Tuple* next(){
        while(true){
            const pair<T,TupleId>* p = it->next();
            if(!p){ return 0;}
            if(p->second < rel->size()){
               Tuple* res = (*rel)[p->second-1];
               if(res){ // ignore deleted tuples
                 res->IncReference();
                 return res;
               }
            }
        }
        return 0;
     }


  private:
     vector<Tuple*>* rel;
     NNIterator<pair<T,TupleId> , StdDistComp<T> >* it;

};

template<class K, class T, class R>
int mdistScanVMT (Word* args, Word& result, int message,
                    Word& local, Supplier s) {
   distScanInfo<K>* li = (distScanInfo<K>*) local.addr;
   switch(message){
     case OPEN : {
        if(li){
          delete li;
          local.addr = 0;
        }
        R* relN = (R*) args[1].addr;
        MemoryRelObject* mro=getMemRel(relN, nl->Second(qp->GetType(s)));
        if(!mro){
          return 0;
        }

        K* key = (K*) args[2].addr;

        T* tree = (T*) args[0].addr;
        MemoryMtreeObject<K, StdDistComp<K> >*m = getMtree<T,K>(tree);
        if(m){
          local.addr = new distScanInfo<K>(m,mro,key);
        }
        return 0;
      }
      case REQUEST:
               result.addr = li?li->next():0;
               return result.addr?YIELD:CANCEL;
      case CLOSE :
               if(li){
                  delete li;
                  local.addr = 0;
               }
               return 0;
   }
   return -1;
}

int mdistScanSelect(ListExpr args){
   ListExpr type = nl->Third(args);
   int m = 9;
   int keyTypeNo = mtreehelper::getTypeNo(type,m);
   int n1 = CcString::checkType(nl->First(args))?0:2*m;
   int n2 = CcString::checkType(nl->Second(args))?0:m;

   return keyTypeNo + n1 + n2;

}

 // note: if adding attributes with flobs, the value mapping must be changed

ValueMapping mdistScanVM[] = {
  mdistScanVMT<mtreehelper::t1, CcString, CcString>,
  mdistScanVMT<mtreehelper::t2, CcString, CcString>,
  mdistScanVMT<mtreehelper::t3, CcString, CcString>,
  mdistScanVMT<mtreehelper::t4, CcString, CcString>,
  mdistScanVMT<mtreehelper::t5, CcString, CcString>,
  mdistScanVMT<mtreehelper::t6, CcString, CcString>,
  mdistScanVMT<mtreehelper::t7, CcString, CcString>,
  mdistScanVMT<mtreehelper::t8, CcString, CcString>,
  mdistScanVMT<mtreehelper::t9, CcString, CcString>,

  mdistScanVMT<mtreehelper::t1, CcString, Mem>,
  mdistScanVMT<mtreehelper::t2, CcString, Mem>,
  mdistScanVMT<mtreehelper::t3, CcString, Mem>,
  mdistScanVMT<mtreehelper::t4, CcString, Mem>,
  mdistScanVMT<mtreehelper::t5, CcString, Mem>,
  mdistScanVMT<mtreehelper::t6, CcString, Mem>,
  mdistScanVMT<mtreehelper::t7, CcString, Mem>,
  mdistScanVMT<mtreehelper::t8, CcString, Mem>,
  mdistScanVMT<mtreehelper::t9, CcString, Mem>,

  mdistScanVMT<mtreehelper::t1, Mem, CcString>,
  mdistScanVMT<mtreehelper::t2, Mem, CcString>,
  mdistScanVMT<mtreehelper::t3, Mem, CcString>,
  mdistScanVMT<mtreehelper::t4, Mem, CcString>,
  mdistScanVMT<mtreehelper::t5, Mem, CcString>,
  mdistScanVMT<mtreehelper::t6, Mem, CcString>,
  mdistScanVMT<mtreehelper::t7, Mem, CcString>,
  mdistScanVMT<mtreehelper::t8, Mem, CcString>,
  mdistScanVMT<mtreehelper::t9, Mem, CcString>,

  mdistScanVMT<mtreehelper::t1, Mem, Mem>,
  mdistScanVMT<mtreehelper::t2, Mem, Mem>,
  mdistScanVMT<mtreehelper::t3, Mem, Mem>,
  mdistScanVMT<mtreehelper::t4, Mem, Mem>,
  mdistScanVMT<mtreehelper::t5, Mem, Mem>,
  mdistScanVMT<mtreehelper::t6, Mem, Mem>,
  mdistScanVMT<mtreehelper::t7, Mem, Mem>,
  mdistScanVMT<mtreehelper::t8, Mem, Mem>,
  mdistScanVMT<mtreehelper::t9, Mem, Mem>

};

OperatorSpec mdistScanSpec(
  "{string, mem(mtree T)} x {string, mem(rel(tuple))}  x T -> stream(tuple) ",
  "mem_mtree mem_rel mdistScan[keyAttr] ",
  "Retrieves tuples from an memory relation in increasing "
  "distance to a reference object aided by a memory based "
  "m-tree.",
  "query \"kinos_mtree\" \"Kinos\" mdistScan[ alexanderplatz] consume"
);

Operator mdistScanOp(
   "mdistScan",
   mdistScanSpec.getStr(),
   36,
   mdistScanVM,
   mdistScanSelect,
   mdistScanTM
);




////////////////////////////////////////////////////////////////////////
// NEW
///////////////////////////////////////////////////////////////////////




/*

Operators of MainMemory2Algebra

7.1 Operator ~mwrap~

Converts a string into an mem(X) according to the
stored type in the memory catalog

7.1.1 Type Mapping Functions of operator ~mwrap~
    string -> mem(X)

*/
ListExpr mwrapTM(ListExpr args){

   if(!nl->HasLength(args,1)){
      return listutils::typeError("one argument expected");
   }
   ListExpr first = nl->First(args);
   if(!nl->HasLength(first,2)) {
      return listutils::typeError("internal error");
   }
   ListExpr type = nl->First(first);
   ListExpr value = nl->Second(first);
   if(!CcString::checkType(type)){
      return listutils::typeError("string expected");
   }
   ListExpr res;
   string error;
   if(!getMemType(type, value,res, error)){
     return listutils::typeError(error);
   }
   return res;
}

/*

7.1.2 Value Mapping Function of operator ~mwrap~

*/
int mwrapVM(Word* args, Word& result,
             int message, Word& local, Supplier s) {
   CcString* arg = (CcString*) args[0].addr;
   result = qp->ResultStorage(s);
   Mem* res = (Mem*) result.addr;
   bool def = arg->IsDefined();
   string v = def?arg->GetValue():"";
   res->set(def,v);
   return 0;
}

/*

7.1.3 Description of operator ~mwrap~

*/
OperatorSpec mwrapSpec(
    "string -> mem(X)",
    "mwrap(_)",
    "Converts a string into a mem object"
    " according to the type from the memory catalog",
    "query mwrap(\"ten\")"
);

/*

7.1.4 Instance of operator ~mwrap~

*/
Operator mwrapOp(
   "mwrap",
   mwrapSpec.getStr(),
   mwrapVM,
   Operator::SimpleSelect,
   mwrapTM
);



template<int pos>
ListExpr MTUPLETM(ListExpr args){

   if(!nl->HasMinLength(args,pos)){
      return listutils::typeError("too less arguments, expected "
               + stringutils::int2str(pos) + " but got " 
               + stringutils::int2str(nl->ListLength(args)));
   }
   // remove elements before pos
   for(int i=1; i<pos;i++){
     args = nl->Rest(args);
   }
   ListExpr mem = nl->First(args);
   if(!nl->HasLength(mem,2)){
     return  listutils::typeError("internal error");
   }
   string errMsg;
   if(!getMemType(nl->First(mem), nl->Second(mem), mem, errMsg)){
    return listutils::typeError("problem in arg: " + errMsg);
   }
   if(!Mem::checkType(mem)){
     return listutils::typeError("not a mem object");
   }
   ListExpr subtype = nl->Second(mem);
   ListExpr res;
   if(nl->HasMinLength(subtype,2)){
     res = nl->Second(subtype); // enclosed tuple: stream, rel, orel ...
   } else {
     return listutils::typeError("unsupported subtype");
   }
   if(!Tuple::checkType(res)){
     return listutils::typeError("no tuple found");
   }
   return res;
}

OperatorSpec MTUPLESpec(
   "... mem(rel(X))  x ... -> X or string -> tuple",
   "MTUPLE<X>(_)",
   "Retrieves the tuple type of a memory relation at position "
   "<X> in the argument list",
   "query \"ten\" mupdatebyid[[const tid value 5]; No: .No + 10000] count"
);

Operator MTUPLEOp(
    "MTUPLE",
    MTUPLESpec.getStr(),
    0,
    Operator::SimpleSelect,
    MTUPLETM<1>
);

Operator MTUPLE2Op(
    "MTUPLE2",
    MTUPLESpec.getStr(),
    0,
    Operator::SimpleSelect,
    MTUPLETM<2>
);




/*
7.2 Operator ~mcreatettree~

The operator creates a TTree over a given main memory relation.

7.2.1 Type Mapping Functions of operator ~mcreatettree~
        {string, mem(rel(...))} x ident -> mem(ttree string)

        the first parameter identifies the main memory relation, 
        the second parameter identifies the attribute

*/
ListExpr mcreatettreeTypeMap(ListExpr args){
  
    if(nl->ListLength(args)!=2){
     return listutils::typeError("two arguments expected");
    }

    ListExpr first = nl->First(args);
    ListExpr second = nl->Second(args);
    ListExpr rel;

    string errMsg;
    if(!getMemType(nl->First(first), nl->Second(first), rel, errMsg)){
      return listutils::typeError("problem in 1st arg: " + errMsg);
    }
    rel = nl->Second(rel);

    if(!Relation::checkType(rel)){
       return listutils::typeError("memory object is not a relation");
    }
    
    
    if(nl->AtomType(nl->First(second))!=SymbolType){
       return listutils::typeError("second argument is not a valid "
                                   "attribute name");
    }

    string attrName = nl->SymbolValue(nl->First(second));
    ListExpr attrType = 0;
    int attrPos = 0;
    ListExpr attrList = nl->Second(nl->Second(rel));
    attrPos = listutils::findAttribute(attrList, attrName, attrType);

    if (attrPos == 0){
        return listutils::typeError
        ("there is no attribute having  name " + attrName);
    }

    ListExpr resType = nl->TwoElemList(
                          listutils::basicSymbol<Mem>(),
                          nl->TwoElemList(
                             nl->SymbolAtom(ttreehelper::BasicType()),
                             attrType
                       ));
    
    return nl->ThreeElemList(
                nl->SymbolAtom(Symbols::APPEND()),
                nl->TwoElemList(nl->IntAtom(attrPos - 1),
                                nl->StringAtom(attrName)),
                resType);
}

/*

7.2.3  Value Mapping Functions of operator ~mcreatettree~

*/
template<class T>
int mcreatettreeValueMap(Word* args, Word& result,
                int message, Word& local, Supplier s) {
  
  result  = qp->ResultStorage(s);
  Mem* str = static_cast<Mem*>(result.addr);

  // the main memory relation
  T* rel = (T*) args[0].addr;
  if(!rel->IsDefined()){
      str->SetDefined(false);
      return 0;
  }
  string name = rel->GetValue();

  int attrPos = ((CcInt*) args[2].addr)->GetValue();

  ListExpr type = catalog->getMMObjectTypeExpr(name);
  type = nl->Second(type);

  if(!Relation::checkType(type)){
    cerr << "object " << name  << "is not a relation" << endl;
    str->SetDefined(false);
    return false;
  } 

  // extract attribute 
  ListExpr attrList = nl->Second(nl->Second(type));
  int pos = attrPos;
  while(!nl->IsEmpty(attrList) && pos>0){
    attrList = nl->Rest(attrList);
    pos--;
  }
  if(nl->IsEmpty(attrList)){
    cerr << "not enough attributes in tuple";
    str->SetDefined(false);
    return 0;
  }
  ListExpr relattrType = nl->Second(nl->First(attrList));
  ListExpr ttreeattrType = nl->Second(nl->Second(qp->GetType(s)));

  if(!nl->Equal(relattrType, ttreeattrType)){
    cerr << "expected type and type in relation differ" << endl;
    str->SetDefined(false);
    return 0;
  }
  // extract name of relation
  string attrName = ((CcString*)args[3].addr)->GetValue();
  // create name for ttree object
  string resName = name + "_" + attrName;
  if(catalog->isMMObject(resName)){
    cerr << "object " << resName << "already exists" << endl;
    str->SetDefined(false);
    return 0;
  }
  // get main memory relation
  MemoryRelObject* mmrel =
      (MemoryRelObject*)catalog->getMMObject(name);
  
  vector<Tuple*>* relation = mmrel->getmmrel();
  vector<Tuple*>::iterator it = relation->begin();
  unsigned int i = 1;

  memttree* tree = new memttree(8,10);
  size_t usedMainMemory = 0;
  unsigned long availableMemSize = catalog->getAvailableMemSize();

  // insert (Attribute, TupleId)-pairs into ttree

  cout << "insert elements from the relation" << endl;

  while(it != relation->end()) {
      Tuple* tup = *it;
      if(tup){
         Attribute* attr = tup->GetAttribute(attrPos);
         AttrIdPair tPair(attr, i);

         // size for a pair is 16 bytes, plus an additional pointer 8 bytes
         size_t entrySize = 24;
         if (entrySize<availableMemSize){
            tree->insert(tPair);
            usedMainMemory += (entrySize);
            availableMemSize -= (entrySize);
         } else {
           cout << "there is not enough main memory available"
                   " to create an TTree" << endl;
           delete tree;
           str->SetDefined(false);
           return 0;
         }
      }
      i++;
      it++;
  }

  cout << "insertion done" << endl;  


  MemoryTTreeObject* ttreeObject = 
        new MemoryTTreeObject(tree, 
                              usedMainMemory,
                              nl->ToString(qp->GetType(s)),
                              mmrel->hasflob(), 
                              getDBname());
  catalog->insert(resName,ttreeObject);

  str->set(true, resName);
  return 0;
}

/*

7.2.4 Value Mapping Array and Selection

*/
ValueMapping mcreatettreeVM[] =
{
    mcreatettreeValueMap<CcString>,
    mcreatettreeValueMap<Mem>,
};

int mcreatettreeSelect(ListExpr args){
    // string case at index 0
    if ( CcString::checkType(nl->First(args)) ){
       return 0;
    }
    // Mem(rel(tuple))case at index 1
    if (Mem::checkType(nl->First(args)) ){
       return 1;
    }
    // should never be reached
    return -1;
  }

/*

7.2.5 Description of operator ~mcreatettree~

*/
OperatorSpec mcreatettreeSpec(
    "string x string -> string || mem(rel(tuple(X))) x string -> string",
    "_ mcreatettree [_]",
    "creates an T-Tree over a main memory relation given by the"
    "first argument string || mem(rel(tuple)) and an attribute "
    "given by the second argument",
    "query \"Staedte\" mcreatettree [SName]"
);

/*

7.1.6 Instance of operator ~mcreatettree~

*/

Operator mcreatettreeOp (
    "mcreatettree",
    mcreatettreeSpec.getStr(),
    2,
    mcreatettreeVM,
    mcreatettreeSelect,
    mcreatettreeTypeMap
);


/*

7.3 Operator ~minsertttree~, ~mdeletettree~

These operators insert, delete or update objects in a main memory ttree index.
They receive a stream of tuples (including an attribute tid), whose attributes,
determined by the third argument, will be either inserted, deleted or updated
in the tree.

7.3.1 Type Mapping Functions of operator ~minsertttree~,
      ~mdeletettree~.

*/
enum ChangeTypeTree {
  MInsertTree,
  MDeleteTree
};

template<class TreeType>
ListExpr minsertdeletetreeTypeMap(ListExpr args){
  
  if(nl->ListLength(args)!=3) {
    return listutils::typeError("three arguments expected");
  }
  ListExpr tmp = args;
  while(!nl->IsEmpty(tmp)){
    if(!nl->HasLength(nl->First(tmp),2)){
      return listutils::typeError("internal error");
    }
    tmp = nl->Rest(tmp);
  }
  
  // process stream
  ListExpr first = nl->First(args); //stream + query
  ListExpr stream = nl->First(first);

  if(!listutils::isTupleStream(stream)){
    return listutils::typeError("first argument must be a tuple stream");
  }

  // check if last attribute is of type 'tid'
  ListExpr rest = nl->Second(nl->Second(stream));
  ListExpr next;
  while (!(nl->IsEmpty(rest))) {
    next = nl->First(rest);
    rest = nl->Rest(rest);
  }
  if(!TupleIdentifier::checkType(nl->Second(next))) {
    return listutils::typeError("last attribute in the tuple must be a tid");
  }
  
  // allow ttree or avl ttree as second argument
  ListExpr second = nl->Second(args);
  string errMsg;

  if(!getMemType(nl->First(second), nl->Second(second), second, errMsg)){
    return listutils::typeError("\n problem in second arg: " + errMsg);
  }
  
  ListExpr tree = nl->Second(second); // remove leading mem
  if(!TreeType::checkType(tree)) {
    return listutils::typeError("second arg is not a mem ttree");
  }

  // check attribute
  ListExpr third = nl->Third(args);
  if(nl->AtomType(nl->First(third))!=SymbolType) {
      return listutils::typeError("third argument is not a valid "
                                  "attribute name");
  }

  string attrName = nl->SymbolValue(nl->First(third));
  ListExpr attrType = 0;
  int attrPos = 0;
  ListExpr attrList = nl->Second(nl->Second(stream));
  attrPos = listutils::findAttribute(attrList, attrName, attrType);

  if (attrPos == 0){
      return listutils::typeError
      ("there is no attribute having name " + attrName);
  }

  
  ListExpr append = nl->OneElemList(nl->IntAtom(attrPos));
  
  return nl->ThreeElemList(
            nl->SymbolAtom(Symbol::APPEND()),
            nl->OneElemList(append),
            stream);
}


template<ChangeTypeTree ct, class TreeType, class TreeElem>
class minsertdeleteInfo {
  public:

    typedef TreeType treetype;

    minsertdeleteInfo(Word w, TreeType* _tree, int _attrPos)
      : stream(w), tree(_tree), attrPos(_attrPos) {
      stream.open();
    }

    ~minsertdeleteInfo(){
      stream.close();
    }

    
    Tuple* next(){
      Tuple* res = stream.request();
      if(!res) {
        return 0; 
      }
      
      Attribute* tidAttr = res->GetAttribute(res->GetNoAttributes() - 1);
      TupleId oldTid = ((TupleIdentifier*)tidAttr)->GetTid();
      Attribute* attr = res->GetAttribute(attrPos-1);
      TreeElem  elem = TreeElem(attr,oldTid);
      switch(ct){
        case MInsertTree : tree->insert(elem); break;
        case MDeleteTree : tree->remove(elem); break;
        default: assert(false);
      }
      return res;
    }
  private:
     Stream<Tuple> stream;
     TreeType* tree;
     int attrPos;
};


/*

7.3.2  The Value Mapping Functions of operator ~minsertttree~
       and ~mdeletettree~

*/
template<class T, class LocalInfo, class MemTree>
int minserttreeValueMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {
  
  LocalInfo* li = (LocalInfo*) local.addr;
  
  switch(message) {
    
    case OPEN : {
      if(li) {
        delete li;
        local.addr = 0;
      }

      T* tree = (T*) args[1].addr;
      if(!tree->IsDefined()){
          cout << "undefined" << endl;
          return 0;
      }
      string name = tree->GetValue();
      // cut blank from the front
      name = name.substr(1, name.size()-1);
      
      MemTree* mmtree =(MemTree*) catalog->getMMObject(name);
      if(!mmtree) {
        return 0;
      }
      
      ListExpr attrlist = nl->Second(nl->Second(qp->GetType(s)));  // stream
      ListExpr attrToSort = qp->GetType(qp->GetSon(s,2));
      ListExpr attrType = 0;
      int attrPos = 0;
      
      attrPos = listutils::findAttribute(attrlist, 
                                         nl->ToString(attrToSort), 
                                         attrType);
      if(attrPos == 0) {
        return listutils::typeError
          ("there is no attribute having name " + nl->ToString(attrToSort));
      }
        
      local.addr = new LocalInfo(args[0],
                                 mmtree->gettree(),
                                 attrPos);
      return 0;
    }

    case REQUEST : {
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;
    }
    
    case CLOSE :
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
      
  }    
  return 0;

}


/*
7.3.3 Value Mapping Array and Selection

*/
ValueMapping minsertttreeVM[] = {
    minserttreeValueMap<CcString,minsertdeleteInfo<MInsertTree, 
                                 memttree, AttrIdPair>,MemoryTTreeObject >,
    minserttreeValueMap<Mem,minsertdeleteInfo<MInsertTree, 
                                 memttree, AttrIdPair>, MemoryTTreeObject >,
    minserttreeValueMap<CcString,minsertdeleteInfo<MDeleteTree, 
                                 memttree, AttrIdPair>,MemoryTTreeObject >,
    minserttreeValueMap<Mem,minsertdeleteInfo<MDeleteTree, 
                                 memttree, AttrIdPair>, MemoryTTreeObject >
};

template<ChangeTypeTree ct>
int minsertttreeSelect(ListExpr args){
  if(ct==MInsertTree) 
    return CcString::checkType(nl->First(args))?0:1;
  else if(ct==MDeleteTree) 
    return CcString::checkType(nl->First(args))?2:3;
}

/*
7.3.4 Description of operator ~minsertttree~

*/

OperatorSpec minsertttreeSpec(
    "stream(tuple(x@[TID:tid])) x {string, mem(ttree)} x ident "
    "-> stream(tuple(x@[TID:tid]))",
    "_ op [_,_]",
    "inserts an object into a main memory ttree",
    "query ten feed head[5] minsert[\"ten\"] "
    "minsertttree[\"ten_No\", No] count"
);

/*
7.3.5 Instance of operator ~minsertttree~

*/
Operator minsertttreeOp (
    "minsertttree",
    minsertttreeSpec.getStr(),
    6,
    minsertttreeVM,
    minsertttreeSelect<MInsertTree>,
    minsertdeletetreeTypeMap<MemoryTTreeObject>
);

/*
7.4.4 Description of operator ~mdeletettree~

*/

OperatorSpec mdeletettreeSpec(
    "stream(tuple(x@[TID:tid])) x {string, mem(ttree)} x ident "
    "-> stream(tuple(x@[TID:tid]))",
    "_ op [_,_]",
    "deletes an object identified by tupleid from a main memory ttree",
    "query ten feed head[5] mdelete[\"ten\"] " 
    "mdeletettree[\"ten_No\", No] count"
);

/*
7.4.5 Instance of operator ~mdeletettree~

*/
Operator mdeletettreeOp (
    "mdeletettree",
    mdeletettreeSpec.getStr(),
    6,
    minsertttreeVM,
    minsertttreeSelect<MDeleteTree>,
    minsertdeletetreeTypeMap<MemoryTTreeObject>
);

/*
7.5 ~insert~ and ~delete~ for AVL trees

*/

OperatorSpec minsertavltreeSpec(
    "stream(tuple(x@[TID:tid])) x {string, mem(avltree)} x ident "
    "-> stream(tuple(x@[TID:tid]))",
    "_ op [_,_]",
    "inserts an object into a main memory avltree",
    "query ten feed head[5] minsert[\"ten\"] "
    "minsertavltree[\"ten_No\", No] count"
);


ValueMapping minsertAVLtreeVM[] = {
    minserttreeValueMap<CcString,minsertdeleteInfo<MInsertTree, 
                                 memAVLtree, AttrIdPair>,MemoryAVLObject >,
    minserttreeValueMap<Mem,minsertdeleteInfo<MInsertTree, 
                                 memAVLtree, AttrIdPair>, MemoryAVLObject >,
};

int minsertavltreeSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}


Operator minsertavltreeOp (
    "minsertavltree",
    minsertavltreeSpec.getStr(),
    2,
    minsertAVLtreeVM,
    minsertavltreeSelect,
    minsertdeletetreeTypeMap<MemoryAVLObject>
);


OperatorSpec mdeleteavltreeSpec(
    "stream(tuple(x@[TID:tid])) x {string, mem(avltree)} x ident "
    "-> stream(tuple(x@[TID:tid]))",
    "_ op [_,_]",
    "Removes ojjects from  a main memory avltree",
    "query ten feed head[5] modelete[\"ten\"] "
    "mdeleteavltree[\"ten_No\", No] count"
);


ValueMapping mdeleteAVLtreeVM[] = {
    minserttreeValueMap<CcString,minsertdeleteInfo<MDeleteTree, 
                                    memAVLtree, AttrIdPair>,MemoryAVLObject >,
    minserttreeValueMap<Mem,minsertdeleteInfo<MDeleteTree, 
                                    memAVLtree, AttrIdPair>, MemoryAVLObject >
};

int mdeleteavltreeSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}


Operator mdeleteavltreeOp (
    "mdeleteavltree",
    mdeleteavltreeSpec.getStr(),
    2,
    mdeleteAVLtreeVM,
    mdeleteavltreeSelect,
    minsertdeletetreeTypeMap<MemoryAVLObject>
);


/*
7.6 Operators ~mcreateinsertrel~, ~mcreatedeleterel~ and
~mcreateupdaterel~

These operators create an auxiliary relation with the same tuple schema
as the given relation including an attribute tid. In case of 
~mcreateupdaterel~, additionally all attributes with an appended sting 
'old' are added to the schema.

7.6.1 General Type mapping function of operators ~mcreateinsertrel~,
~mcreatedeleterel~ and ~mcreateupdaterel~

*/
ListExpr mcreateAuxiliaryRelTypeMap(const ListExpr& args,
                                    const string opName) {

  if(nl->ListLength(args) != 1) {
    return listutils::typeError("one argument expected");
  }
  
  ListExpr first = nl->First(args);
  if(!nl->HasLength(first,2))
    return listutils::typeError("internal error");
  
  string errMsg;

  if(!getMemType(nl->First(first), nl->Second(first), first, errMsg)){
    return listutils::typeError(
      "string or mem(rel) or mem(orel) expected : " + errMsg);
  }

  // check for rel
  bool isOrel;
  first = nl->Second(first); // remove mem
  if(Relation::checkType(first)) {
    isOrel = false;
  }
  else if(listutils::isOrelDescription(first)) {
    isOrel = true;
  }
  else {
    return listutils::typeError(
    "first arg is not a memory relation or ordered relation");
  }

  // build first part of the result-tupletype
  ListExpr rest = nl->Second(nl->Second(first));
  ListExpr listn = nl->OneElemList(nl->First(rest));
  ListExpr lastlistn = listn;
  rest = nl->Rest(rest);
  while(!(nl->IsEmpty(rest))) {
     lastlistn = nl->Append(lastlistn,nl->First(rest));
     rest = nl->Rest(rest);
  }
  
  if(opName == "mcreateupdaterel") {
    // Append once again all attributes from the argument-relation
    // to the result-tupletype but the names of the attributes
    // extendend by 'old'
  
    rest = nl->Second(nl->Second(first));
    string oldName;
    ListExpr oldAttribute;
    while (!(nl->IsEmpty(rest))) {
      nl->WriteToString(oldName, nl->First(nl->First(rest)));
      oldName += "_old";
      oldAttribute =
        nl->TwoElemList(
          nl->SymbolAtom(oldName),
          nl->Second(nl->First(rest)));
      lastlistn = nl->Append(lastlistn,oldAttribute);
      rest = nl->Rest(rest);
    }
  }

  //Append last attribute for the tupleidentifier
  lastlistn = nl->Append(lastlistn,
                         nl->TwoElemList(
                           nl->SymbolAtom("TID"),
                           nl->SymbolAtom(TupleIdentifier::BasicType())));
  
  ListExpr outlist;
  
  if(!isOrel) {
    outlist = nl->TwoElemList(
                listutils::basicSymbol<Mem>(),
                nl->TwoElemList(
                  listutils::basicSymbol<Relation>(),
                  nl->TwoElemList(
                    nl->SymbolAtom(Tuple::BasicType()),
                    listn)));
  }
  else {
    outlist = nl->TwoElemList(
                listutils::basicSymbol<Mem>(),
                nl->ThreeElemList(
                  nl->SymbolAtom(OREL),
                  nl->TwoElemList(  
                    nl->SymbolAtom(Tuple::BasicType()),
                    listn),
                  nl->Third(first)));
  }
  return outlist;
}

ListExpr mcreateinsertrelTypeMap(ListExpr args) {
  return mcreateAuxiliaryRelTypeMap(args, "mcreateinsertrel");
}

ListExpr mcreatedeleterelTypeMap(ListExpr args) {
  return mcreateAuxiliaryRelTypeMap(args, "mcreatedeleterel");
}

ListExpr mcreateupdaterelTypeMap(ListExpr args) {
  return mcreateAuxiliaryRelTypeMap(args, "mcreateupdaterel");
}

/*
7.6.2 General Value mapping function of operators ~mcreateinsertrel~, 
       ~mcreatedeleterel~ and ~mcreateupdaterel~

*/
int mcreateinsertrelValueMap(Word* args, Word& result, int message,
                             Word& local, Supplier s) {
  result = qp->ResultStorage(s);
  return 0;
}


/*
7.6.4 Specification of operator ~mcreateinsertrel~

*/
OperatorSpec mcreateinsertrelSpec(
    "{string, mem(rel(...))} -> mem(rel(tuple(x@[TID:tid]))) ",
    "mcreateinsertrel(_)",
    "creates an auxiliary relation",
    "query memlet (\"fuenf\", mcreateinsertrel(\"ten\"))"
);


/*
7.6.5 Definition of operator ~mcreateinsertrel~

*/
Operator mcreateinsertrelOp (
  "mcreateinsertrel",             // name
  mcreateinsertrelSpec.getStr(),  // specification
  mcreateinsertrelValueMap,       // value mapping
  Operator::SimpleSelect,         // trivial selection function
  mcreateinsertrelTypeMap         // type mapping
);


/*
7.7.4  Specification of operator ~mcreatedeleterel~

*/
OperatorSpec mcreatedeleterelSpec(
    "{string, mem(rel(...))} -> mem(rel(tuple(x@[TID:tid]))) ",
    "mcreatedeleterel(_)",
    "creates an auxiliary relation",
    "query memlet (\"fuenf\", mcreatedeleterel(\"ten\"))"
);

/*
7.7.5  Definition of operator ~mcreatedeleterel~

*/
Operator mcreatedeleterelOp(
  "mcreatedeleterel",             // name
  mcreatedeleterelSpec.getStr(),  // specification
  mcreateinsertrelValueMap,             // value mapping
  Operator::SimpleSelect,         // trivial selection function
  mcreatedeleterelTypeMap         // type mapping
);


/*
7.8.4 Specification of operator ~mcreateupdaterel~

*/

OperatorSpec mcreateupdaterelSpec(
    "{string, mem(rel(...))} -> "
    "mem(rel(tuple(x@[(a1_old x1)...(an_old xn)(TID:tid)]))) ",
    "mcreateupdaterel(_)",
    "creates an auxiliary relation",
    "query memlet (\"fuenf\", mcreateupdaterel(\"ten\"))"
);

    
/*
7.8.5 Definition of operator ~mcreateupdaterel~

*/
Operator mcreateupdaterelOp (
  "mcreateupdaterel",             // name
  mcreateupdaterelSpec.getStr(),  // specification
  mcreateinsertrelValueMap,             // value mapping
  Operator::SimpleSelect,         // selection function
  mcreateupdaterelTypeMap         // type mapping
);




/*
7.9 Operator ~minsert~

Inserts each tuple of the inputstream into the memory relation. Returns 
a stream of tuples which is  basically the same as the inputstream 
but each tuple extended by an attribute of type 'tid' which is the 
tupleidentifier of the inserted tuple in the extended relation.

7.9.1 General type mapping function of operators ~minsert~, ~minsertsave~,
      ~mdelete~ and ~mdeletesave~
      
   stream(tuple(x)) x {string, mem(rel(...))} -> stream(tuple(x@[TID:tid])) or
   
   stream(tuple(x)) x {string, mem(rel(...))} x {string, mem(rel(...))} 
     -> stream(tuple(x@[TID:tid])) 
     
   
 
*/
template<int noargs>
ListExpr minsertTypeMap(ListExpr args) {
  
  if(!nl->HasLength(args,noargs)) {
    return listutils::typeError("wrong number of arguments");
  }

  if(!checkUsesArgs(args)){
    return listutils::typeError("internal error");
  }
  
  ListExpr first = nl->First(args); //stream + query
  ListExpr stream = nl->First(first);

  // process stream
  if(!Stream<Tuple>::checkType(stream)){
    return listutils::typeError("first argument must be a tuple stream");
  }
  
  string errMsg;
  
  // process second argument (mem(rel))
  ListExpr second  = nl->Second(args);
  if(!getMemType(nl->First(second), nl->Second(second), second, errMsg)) {
      return listutils::typeError("(problem in second arg: " + errMsg + ")");
  }
  second = nl->Second(second);
  if((!Relation::checkType(second)) && 
     (!listutils::isOrelDescription(second))){
    return listutils::typeError(
      "second arg is not a memory relation");
  }
  if(!nl->Equal(nl->Second(stream), nl->Second(second))){
      return listutils::typeError("stream type and mem relation type differ\n"
                               "stream: " + nl->ToString(stream) + "\n"
                               + "rel   : " + nl->ToString(second));
  }
  
  
  // append tupleidentifier
  ListExpr rest = nl->Second(nl->Second(second));
  ListExpr at;
  if(listutils::findAttribute(rest,"TID",at)>0){
    return listutils::typeError("There is already an TID attribute");
  }
  ListExpr listn = nl->OneElemList(nl->First(rest));
  ListExpr lastlistn = listn;
  rest = nl->Rest(rest);
  while (!(nl->IsEmpty(rest))) {
    lastlistn = nl->Append(lastlistn,nl->First(rest));
    rest = nl->Rest(rest);
  }
  

  lastlistn = nl->Append(lastlistn,
                         nl->TwoElemList(
                           nl->SymbolAtom("TID"),
                           nl->SymbolAtom(TupleIdentifier::BasicType())));
  
  // process third argument (minsertsave only)
  if(nl->ListLength(args)==3) {
    ListExpr third  = nl->Third(args);
    if(!getMemType(nl->First(third), nl->Second(third), third, errMsg)) {
      return listutils::typeError("(problem in third arg: " + errMsg + ")");
    }
    third = nl->Second(third);
    if(!Relation::checkType(third)){
      return listutils::typeError(
       "third arg is not a memory relation");
    }
    // the scheme of this relation must be equal to the output tuple scheme
    if(!nl->Equal(listn, nl->Second(nl->Second(third)))){
       return listutils::typeError("Scheme of the auxiliary relation is not "
                  "equal to the main relation + TID");
    }
  }
  return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                         nl->TwoElemList(
                           nl->SymbolAtom(Tuple::BasicType()),
                           listn));
}


class minsertInfo {
  public:
     minsertInfo(Word w, vector<Tuple*>* _relation, 
                 bool _flob, ListExpr _type)
        : stream(w),relation(_relation), flob(_flob) {
        type = new TupleType(_type);
        stream.open();
     }

    ~minsertInfo(){
       stream.close();
       type->DeleteIfAllowed();     
     }

     Tuple* next(){
       Tuple* res = stream.request();
       if(!res) { return 0; }
       if(flob) {
         res->bringToMemory();
       }
       
       Tuple* newtup = new Tuple(type); 
       for(int i = 0; i < res->GetNoAttributes(); i++) {
         newtup->CopyAttribute(i,res,i);
       }
      
       
       // add the main memory tuple id to the output tuple
       Attribute* tidAttr = new TupleIdentifier(true,
                                       relation->size()+1);
       newtup->PutAttribute(res->GetNoAttributes(), tidAttr);
       relation->push_back(res);
    
       return newtup;
     }

  private:
     Stream<Tuple> stream;
     vector<Tuple*>* relation;
     bool flob;
     TupleType* type;
};


/*

7.9.2  Value Mapping Function of operator ~minsert~

*/
template<class T>
int minsertValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {
  
  minsertInfo* li = (minsertInfo*) local.addr;
  
  switch (message) {
    
    case OPEN : {
      
      if(li) {
        delete li;
        local.addr=0;
      }
      
      qp->Open(args[0].addr);
      
      T* oN = (T*) args[1].addr;   
      MemoryRelObject* rel = getMemRel(oN);
      if(!rel) return 0;
      
      
      local.addr = new minsertInfo(args[0],rel->getmmrel(),rel->hasflob(),
                                     nl->Second(GetTupleResultType(s)));
      return 0;
    }
    
    case REQUEST : {
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;
    }
    
    case CLOSE :
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
      
  }
  return 0;

}

ValueMapping minsertVM[] = {
   minsertValMap<CcString>,
   minsertValMap<Mem>
};

int minsertSelect(ListExpr args){
   return CcString::checkType(nl->Second(args))?0:1;
}

/*
7.9.4 Description of operator ~minsert~

*/

OperatorSpec minsertSpec(
    "stream(tuple(x)) x {string, mem(rel(...))} -> stream(tuple(x@[TID:tid]))",
    "_ minsert [_]",
    "inserts the tuples of a stream into an "
    "existing main memory relation. All tuples get an additional "
    "attribute of type 'tid'",
    "query minsert (ten feed head[5],\"ten\") count"
);

/*
7.9.5 Instance of operator ~minsert~

*/
Operator minsertOp (
    "minsert",
    minsertSpec.getStr(),
    2,
    minsertVM,
    minsertSelect,
    minsertTypeMap<2>
);


/*
7.10 Operator ~minsertsave~

Inserts each tuple of the inputstream into the memory relation. Returns 
a stream of tuples which is basically the same as the inputstream 
but each tuple extended by an attribute of type 'tid' which is the 
tupleidentifier of the inserted tuple in the extended relation.
Additionally the tuple of the input stream are saved in an auxiliary 
main memory realtion.

*/
class minsertsaveInfo {
  public:
     minsertsaveInfo(Word w, vector<Tuple*>* _relation, 
         vector<Tuple*>* _auxrel, 
                 bool _flob, ListExpr _type)
        : stream(w),relation(_relation), auxrel(_auxrel), 
          flob(_flob) {
        stream.open();
        type = new TupleType(_type);
     }
     
    ~minsertsaveInfo(){
       stream.close();
       type->DeleteIfAllowed();
     }

    Tuple* next(){
      Tuple* res = stream.request();
      if(!res){
         return 0;
      }
      
      if(flob){ 
        res->bringToMemory();
      }
      
      //get tuple id and append it to tuple
      Tuple* newtup = new Tuple(type); 
      for(int i = 0; i < res->GetNoAttributes(); i++){
        newtup->CopyAttribute(i,res,i);
      }
      Attribute* tidAttr = new TupleIdentifier(true,relation->size()+1);
      newtup->PutAttribute(res->GetNoAttributes(), tidAttr);
      
      // insert tuple in memory relation 
      relation->push_back(res);
      // insert tuple in auxrel
      auxrel->push_back(newtup);
      newtup->IncReference();
       
      return newtup;
    }

  private:
     Stream<Tuple> stream;
     vector<Tuple*>* relation;
     vector<Tuple*>* auxrel;
     bool flob;
     TupleType* type;
};


/*
7.10.2  Value Mapping Functions of operator ~minsertsave~

*/
template<class T>
int minsertsaveValueMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {
  
  minsertsaveInfo* li = (minsertsaveInfo*) local.addr;
  
  switch (message) {
    
    case OPEN : {
      
      if(li) {
        delete li;
        local.addr=0;
      }
      
      qp->Open(args[0].addr);
      
      T* oN = (T*) args[1].addr;
      T* aux = (T*) args[2].addr;
      
      MemoryRelObject* rel = getMemRel(oN);;
      if(!rel) { return 0; }
      MemoryRelObject* auxrel = getMemRel(aux);;
      if(!auxrel) { return 0; }
      
      ListExpr tt = nl->Second(GetTupleResultType(s));
      
      local.addr = new minsertsaveInfo(args[0],rel->getmmrel(),
                                       auxrel->getmmrel(),
                                       rel->hasflob(),tt);
      return 0;
    }
    
    case REQUEST : {
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;
    }
    
    case CLOSE :
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;     
  }
  return 0;

}

ValueMapping minsertsaveVM[] = {
   minsertsaveValueMap<CcString>,
   minsertsaveValueMap<Mem>
};

int minsertsaveSelect(ListExpr args){
   return CcString::checkType(nl->Second(args))?0:1;
}

/*
7.10.4 Description of operator ~minsertsave~

*/
OperatorSpec minsertsaveSpec(
    "stream(tuple(x)) x {string, mem(rel(...))} x {string, mem(rel(...))} "
    "-> stream(tuple(x@[TID:tid]))",
    "_ minsertsave [_,_]",
    "insert all tuple of an input stream into two main memory relations",
    "query ten feed head[5] minsertsave['ten','fuenf'] count"
);

/*
7.10.5 Instance of operator ~minsertsave~

*/
Operator minsertsaveOp (
    "minsertsave",
    minsertsaveSpec.getStr(),
    2,
    minsertsaveVM,
    minsertsaveSelect,
    minsertTypeMap<3>
);


/*
7.11 Operator ~minserttuple~

This Operator inserts the given list of attributes as a new tuple into 
a main memory relation.

7.11.1 General type mapping function of operators ~minserttuple~
      and ~minserttuplesave~.
      
*/
template<int noArgs>
ListExpr minserttupleTypeMap(ListExpr args) {

  
  if((nl->ListLength(args)!=noArgs)) {
    return listutils::typeError(stringutils::int2str(noArgs) 
                                + " arguments expected");
  }



  // process first argument (mem(rel))
  ListExpr first = nl->First(args);
  if(!nl->HasLength(first,2)){
    return listutils::typeError("internal error");
  }

  string errMsg;
  if(!getMemType(nl->First(first), nl->Second(first), first, errMsg)){
    return listutils::typeError("string or mem(rel) expected : " + errMsg);
  }
  first = nl->Second(first); // remove mem
  if(!Relation::checkType(first)){
    return listutils::typeError("first arg is not a memory relation");
  }
    
  // check tuple
  ListExpr second = nl->Second(args);
  if(nl->AtomType(second)!=NoAtom){
    return listutils::typeError("list as second arg expected");
  }
  if(nl->ListLength(nl->Second(first))!=nl->ListLength(second)){
    return listutils::typeError("different lengths in tuple and update");
  }
  
  // check whether types of relation matches types of attributes  
  ListExpr restrel = nl->Second(nl->Second(first));
  ListExpr resttuple = nl->First(second);

  while(!(nl->IsEmpty(restrel))) {
    if(!nl->Equal(nl->Second(nl->First(restrel)),nl->First(resttuple))){
      return listutils::typeError("type mismatch in attribute "
                                  "list and update list");
    }
    restrel = nl->Rest(restrel);
    resttuple = nl->Rest(resttuple);
  }
  
  // append tupleid
  restrel = nl->Second(nl->Second(first));

  ListExpr at;
  if(listutils::findAttribute(restrel,"TID",at)>0){
    return listutils::typeError("TID attribute already present in "
                                "argument relation.");
  }

  ListExpr listn = nl->OneElemList(nl->First(restrel));
  ListExpr lastlistn = listn;
  restrel = nl->Rest(restrel);
  while (!(nl->IsEmpty(restrel))) {
    lastlistn = nl->Append(lastlistn,nl->First(restrel));
    restrel = nl->Rest(restrel);
  }
  
  lastlistn = nl->Append(lastlistn,
                          nl->TwoElemList(
                            nl->SymbolAtom("TID"),
                            nl->SymbolAtom(TupleIdentifier::BasicType())));

  // process third argument (minserttuplesave only)
  if(noArgs==3) {
    ListExpr third  = nl->Third(args);
    if(!nl->HasLength(third,2)){
      return listutils::typeError("internal error");
    }
    if(!getMemType(nl->First(third), nl->Second(third), third, errMsg)) {
      return listutils::typeError("(problem in third arg: " + errMsg + ")");
    }
    third = nl->Second(third); // remove mem
    if(!Relation::checkType(third)){
      return listutils::typeError(
        "third arg is not a memory relation");
    }
    // check whether this relations corresponds to the output tuples
    ListExpr attrList = nl->Second(nl->Second(third));
    if(!nl->Equal(attrList, listn)){
      return listutils::typeError("auxiliary relation type differs to " 
                                  "main relationtype + TID");
    }

  }

 
  return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                          nl->TwoElemList(
                            nl->SymbolAtom(Tuple::BasicType()),
                            listn));
}


class minserttupleInfo{
  public:
    minserttupleInfo(vector<Tuple*>* _relation, bool _flob,
                     ListExpr _listType, TupleType* _type, Word _tupleList)
      : relation(_relation), flob(_flob), 
        listType(_listType) ,type(_type), tupleList(_tupleList) {
      
      firstcall = true;
    }
    
    ~minserttupleInfo() {
      type->DeleteIfAllowed();
    }
    
    Tuple* next() {
      
      if(!firstcall) {
         return 0;
      } else {
         firstcall = false;
      }
      
      Tuple* res = new Tuple(type);
      
      ListExpr rest = nl->Second(nl->Second(listType));
      ListExpr insertType = nl->OneElemList(nl->First(rest));
      ListExpr last = insertType;

      rest = nl->Rest(rest);
      while(nl->ListLength(rest)>1) {
        last = nl->Append(last, nl->First(rest));
        rest = nl->Rest(rest);
      }
      insertType = nl->TwoElemList(nl->SymbolAtom(Tuple::BasicType()),
                                   insertType);

      TupleType* insertTupleType = new TupleType(insertType);  
      Tuple* tup = new Tuple(insertTupleType);
      insertTupleType->DeleteIfAllowed();
      Supplier supplier = tupleList.addr;
      
      Word attrValue;
      Supplier s;
      for(int i=0; i<res->GetNoAttributes()-1; i++) {
        s = qp->GetSupplier(supplier, i);
        qp->Request(s,attrValue);
        Attribute* attr = (Attribute*) attrValue.addr;
        res->PutAttribute(i,attr->Clone());
        tup->CopyAttribute(i,res,i);
      }
      const TupleId& tid = relation->size()+1;
      tup->SetTupleId(tid);
      // add TID
      Attribute* tidAttr = new TupleIdentifier(true,tid);
      res->PutAttribute(res->GetNoAttributes()-1, tidAttr);
      
      if(flob){
        tup->bringToMemory();
      }
      // add Tuple to memory relation
      relation->push_back(tup);
      return res;
    }

private:
    vector<Tuple*>* relation;
    bool flob;
    ListExpr listType;
    TupleType* type;
    Word tupleList;
    bool firstcall;
};


/*

7.11.2  Value Mapping Function of operator ~minserttuple~

*/
template<class T>
int minserttupleValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

  minserttupleInfo* li = (minserttupleInfo*) local.addr;

  switch (message) {
    case OPEN: {
      
      if(li) {
        delete li;
        local.addr=0;
      }
      
      T* oN = (T*) args[0].addr;
      
      MemoryRelObject* rel = getMemRel(oN);
      if(!rel) { 
        return 0; }
      
      ListExpr tupleType = GetTupleResultType(s);
      TupleType* tt = new TupleType(nl->Second(tupleType));
      
      local.addr = new minserttupleInfo(rel->getmmrel(),rel->hasflob(),
                                        tupleType,tt,args[1]);
      qp->SetModified(qp->GetSon(s, 0));
      return 0;
    }

    case REQUEST:
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;

    case CLOSE:
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
  }
  return 0;
}


ValueMapping minserttupleVM[] = {
  minserttupleValMap<CcString>,
  minserttupleValMap<Mem>
};

int minserttupleSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}


/*
7.11.4 Description of operator ~minserttuple~

*/
OperatorSpec minserttupleSpec(
    "{string, mem(rel(...))} x [t1 ... tn] -> stream(tuple(x@[TID:tid]))",
    "_ minserttuple [list]",
    "inserts a tuple into a main memory relation",
    "query 'Staedte' minserttuple['AA',34,5666,'899','ZZ'] count"
);

/*
7.11.5 Instance of operator ~minserttuple~

*/
Operator minserttupleOp (
    "minserttuple",
    minserttupleSpec.getStr(),
    2,
    minserttupleVM,
    minserttupleSelect,
    minserttupleTypeMap<2>
);




/*
7.12 Operator ~minserttuplesave~

The Operator ~minserttuplesave~ adds a tuple to a given main memory relation
(argument 1) and saves it additionally in an auxiliary main memory 
relation (argument 3). 
   
*/
class minserttuplesaveInfo{
  public:
    minserttuplesaveInfo(vector<Tuple*>* _relation, vector<Tuple*>* _auxrel, 
                         bool _flob, ListExpr _listType, TupleType* _type, 
                         Word _tupleList)
      : relation(_relation), auxrel(_auxrel), flob(_flob), 
        listType(_listType) ,type(_type), tupleList(_tupleList) {
      
      firstcall = true;
    }
    
    ~minserttuplesaveInfo() {
      type->DeleteIfAllowed();
    }
    
    Tuple* next() {
      
      if(!firstcall){
          return 0;
      } else {
          firstcall = false;
      }
      
      Tuple* res = new Tuple(type);
      
      ListExpr rest = nl->Second(nl->Second(listType));
      ListExpr insertType = nl->OneElemList(nl->First(rest));
      ListExpr last = insertType;
      
      rest = nl->Rest(rest);
      while(nl->ListLength(rest)>1) {
        last = nl->Append(last, nl->First(rest));
        rest = nl->Rest(rest);
      }
      insertType = nl->TwoElemList(nl->SymbolAtom(Tuple::BasicType()),
                                   insertType);


      TupleType* insertTupleType = new TupleType(insertType);
      Tuple* tup = new Tuple(insertTupleType);
      insertTupleType->DeleteIfAllowed();
      Supplier supplier = tupleList.addr;
      
      Word attrValue;
      Supplier s;
      for(int i=0; i<res->GetNoAttributes()-1; i++) {
        s = qp->GetSupplier(supplier, i);
        qp->Request(s,attrValue);
        Attribute* attr = (Attribute*) attrValue.addr;
        res->PutAttribute(i,attr->Clone());
        tup->CopyAttribute(i,res,i);
      }
      const TupleId tid = relation->size()+1;
      tup->SetTupleId(tid);
      
      // add TID
      Attribute* tidAttr = new TupleIdentifier(true,tid);
      res->PutAttribute(res->GetNoAttributes()-1, tidAttr);
      
      // flob
      if(flob) {
        tup->bringToMemory();
      }
      
      // add Tuple to memory relation
      relation->push_back(tup);

      // add tuple to auxrel
      res->IncReference();
      auxrel->push_back(res);
      return res;
    }

  private:
    vector<Tuple*>* relation;
    vector<Tuple*>* auxrel;
    bool flob;
    ListExpr listType;
    TupleType* type;
    Word tupleList;
    bool firstcall;
};

/*
7.12.2  Value Mapping Function of operator ~minserttuplesave~

*/
template<class T>
int minserttuplesaveValueMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

  minserttuplesaveInfo* li = (minserttuplesaveInfo*) local.addr;

  switch (message) {
    case OPEN: {
      
      if(li) {
        delete li;
        local.addr=0;
      }
      
      T* oN = (T*) args[0].addr;

      ListExpr tupleType = GetTupleResultType(s);
      TupleType* tt = new TupleType(nl->Second(tupleType));
      
      T* aux = (T*) args[2].addr;
      
      MemoryRelObject* rel = getMemRel(oN);
      if(!rel) { return 0; }
      MemoryRelObject* auxrel = getMemRel(aux);
      if(!auxrel) { return 0; }
      local.addr = new minserttuplesaveInfo(rel->getmmrel(),
                                            auxrel->getmmrel(),
                                            rel->hasflob(),
                                            tupleType,tt,args[1]);
      qp->SetModified(qp->GetSon(s, 0));  // correct ?
      return 0;
    }

    case REQUEST:
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;

    case CLOSE:
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
  }
  return 0;
}


ValueMapping minserttuplesaveVM[] = {
  minserttuplesaveValueMap<CcString>,
  minserttuplesaveValueMap<Mem>
};

int minserttuplesaveSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}


/*
7.12.4 Description of operator ~minserttuplesave~

*/
OperatorSpec minserttuplesaveSpec(
    "{string, mem(rel(tuple(x)))} x [t1 ... tn] x "
    "{string, mem(rel(tuple(x@[TID:tid])))} -> stream(tuple(x@[TID:tid]))",
    "_ minserttuplesave [list; _]",
    "inserts a tuple into a main memory relation and an auxiliary main "
    "memory relation",
    "query 'Staedte' "
    "minserttuplesave['AusgedachtDorf',34,5666,899,'ZZ'; 'Stadt'] count"
);

/*
7.12.5 Instance of operator ~minserttuplesave~

*/
Operator minserttuplesaveOp (
    "minserttuplesave",
    minserttupleSpec.getStr(),
    2,
    minserttuplesaveVM,
    minserttuplesaveSelect,
    minserttupleTypeMap<3>
);



/*
7.13 Operator ~mdelete~


7.13.1 Auxiliary function ~remove~

This function removes the first tuple in a relation having the
same attribute values as the given tuple. The tuple id of
res is set to the former tuple id of the removed tuple.
If the tuple is not found, its tuple id is set to null.

*/
void remove(vector<Tuple*>* relation, Tuple* res) {
    
  for (size_t i = 0; i < relation->size(); i++) {
    Tuple* tup = relation->at(i);
    if(tup){
      bool equal = true;
      for(int j=0; j<tup->GetNoAttributes() && equal; j++) {
        int cmp = ((Attribute*)tup->GetAttribute(j))->Compare(
                  ((Attribute*)res->GetAttribute(j)));
        if(cmp!=0){
          equal = false;
        }
      }
      if(equal){ // hit
        tup->DeleteIfAllowed();  
        (*relation)[i] = 0;
        res->SetTupleId(i+1);
        return;
      }
    }
  }
  res->SetTupleId(0);
}

class mdeleteInfo {
public:
mdeleteInfo(Word w, 
            vector<Tuple*>* _mainRelation, 
            vector<Tuple*>* _auxRelation,
            ListExpr _type)
  : stream(w),
    mainRelation(_mainRelation),
    auxRelation(_auxRelation) {
    type = new TupleType(_type);
    stream.open();
  }

  ~mdeleteInfo(){
     stream.close();
     type->DeleteIfAllowed();
   }
     
  Tuple* next() {
     TupleIdentifier* tid;
     while((tid=stream.request())){
       tid->Print(cout) << endl; 
       if(!tid->IsDefined()){
         tid->DeleteIfAllowed();
       } else {
         TupleId id = tid->GetTid();
         if(id<1 || id >= mainRelation->size()){
           tid->DeleteIfAllowed();
         } else {
           Tuple* tuple = mainRelation->at(id-1);
           if(!tuple){
             tid->DeleteIfAllowed();
           } else {
             (*mainRelation)[id-1]=0;
             return createResultTuple(tuple,tid);
           }
         }
       }

     }
     return 0;
  }

  private:
     Stream<TupleIdentifier> stream;
     vector<Tuple*>* mainRelation;
     vector<Tuple*>* auxRelation;
     TupleType* type;


     Tuple* createResultTuple(Tuple* orig, TupleIdentifier* tid){
       Tuple* res = new Tuple(type);
       assert(res->GetNoAttributes()==orig->GetNoAttributes()+1);


       for(int i=0;i<orig->GetNoAttributes(); i++){
          res->CopyAttribute(i,orig,i);
       }
       res->PutAttribute(orig->GetNoAttributes(), tid);
       orig->DeleteIfAllowed();
       res->SetTupleId(tid->GetTid());
       // save tuple if required
       if(auxRelation){
         res->IncReference();
         auxRelation->push_back(res);
       }
       return res;
     }
};

/*

7.13 Operator ~mdelete~

This Operator deletes all tuples of an input stream from a main memory 
relation. All tuple of the input stream are returned in an output 
stream with an additional Attribut containing the tupleid.

7.13.2 Value Mapping Function of operator ~mdelete~

*/
template<bool save>
ListExpr mdeleteTM(ListExpr args){
  int noargs =  save?3:2;
  if(!nl->HasLength(args,noargs)){
    return listutils::typeError("wrong number of arguments");
  }
  if(!checkUsesArgs(args)){
     return listutils::typeError("internal error");
  }
  if(!Stream<TupleIdentifier>::checkType(nl->First(nl->First(args)))){
    return listutils::typeError("first argument mut be a stream of tids");
  }
  string errMsg;
  ListExpr mainRel = nl->Second(args);
  if(!getMemType(nl->First(mainRel), nl->Second(mainRel), mainRel, errMsg)){
    return listutils::typeError("second arg is not an mrel" + errMsg);
  }
  mainRel = nl->Second(mainRel); // remove mem
  if(!Relation::checkType(mainRel)){
    return listutils::typeError("second arg is not a memory relation");
  }
  ListExpr mat = nl->Second(nl->Second(mainRel));
  // append TID to mtt
  ListExpr at = listutils::concat(mat, nl->OneElemList(
                               nl->TwoElemList(nl->SymbolAtom("TID"),
                                  listutils::basicSymbol<TupleIdentifier>())));
  ListExpr tt = nl->TwoElemList( listutils::basicSymbol<Tuple>(), at);
  ListExpr res = nl->TwoElemList(listutils::basicSymbol<Stream<Tuple> >(),
                           tt);
  if(!save){
     return res;
  }
  // check the aux relation
  ListExpr auxRel = nl->Third(args);
  if(!getMemType(nl->First(auxRel), nl->Second(auxRel), auxRel, errMsg)){
    return listutils::typeError("third arg is not an mrel" + errMsg);
  }
  auxRel = nl->Second(auxRel); // remove mem
  if(!Relation::checkType(auxRel)){
    return listutils::typeError("third arg is not a memory relation");
  }
  if(!nl->Equal(nl->Second(auxRel),tt)){
    return listutils::typeError("auxiliary relation is not main "
                                "relation + tid");
  } 
  return res;
}


template<class T, class S, bool save>
int mdeleteValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {
  
  mdeleteInfo* li = (mdeleteInfo*) local.addr;
  
  switch (message) {
    case OPEN : {
      
      if(li) {
        delete li;
        local.addr=0;
      }
      
      T* oN = (T*) args[1].addr;
      MemoryRelObject* rel = getMemRel(oN);
      if(!rel) { 
        return 0;
      }
      
      ListExpr  tt = nl->Second(GetTupleResultType(s));

      vector<Tuple*>* aux = 0;
      if(save){
         S* an = (S*) args[2].addr;
         MemoryRelObject* arel = getMemRel(an);
         if(!arel){
           return 0;
         }
         aux = arel->getmmrel();
      }

      local.addr = new mdeleteInfo(args[0],
                                   rel->getmmrel(),
                                   aux, 
                                   tt);
      return 0;
    }
    
    case REQUEST : {
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;
    }
    
    case CLOSE :
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;  
  }
  return 0;
}

ValueMapping mdeleteVM[] = {
   mdeleteValMap<CcString, CcString, false>,
   mdeleteValMap<Mem, CcString, false>
};

int mdeleteSelect(ListExpr args){
   return CcString::checkType(nl->Second(args))?0:1;
}

/*
7.13.4 Description of operator ~mdelete~

*/
OperatorSpec mdeleteSpec(
    "stream(tid) x {string, mem(rel(...))} -> stream(tuple(x@[TID:tid]))",
    "_ mdelete [_]",
    "deletes the tuple of a stream from an "
    "existing main memory relation",
    "query ten feed filter [.No = 2] tids mdelete['ten'] count"
);

/*
7.13.5 Instance of operator ~mdelete~

*/
Operator mdeleteOp (
    "mdelete",
    mdeleteSpec.getStr(),
    2,
    mdeleteVM,
    mdeleteSelect,
    mdeleteTM<false>
);


/*
7.14 Operator ~mdeletesave~

The operator ~mdeletesave~ deletes all tuples of an input stream from
a main memory relation. It returns the stream, all tuples now containing an
additional attribut of type 'tid' and adds the tuples to an auxiliary 
main memory relation.

*/

/*
7.14.2 Value Mapping of operator ~mdeletesave~

*/
ValueMapping mdeletesaveVM[] = {
  mdeleteValMap<CcString, CcString, true>,
  mdeleteValMap<CcString, Mem, true>,
  mdeleteValMap<Mem, CcString, true>,
  mdeleteValMap<Mem, Mem, true>,
  
};

int mdeletesaveSelect(ListExpr args){
  int n1 = CcString::checkType(nl->Second(args))?0:2;
  int n2 = CcString::checkType(nl->Third(args))?0:1;
  return n1+n2;
}



/*
7.14.4 Description of operator ~mdeletesave~

*/

OperatorSpec mdeletesaveSpec(
    "stream(tid) x {string, mem(rel)} x {string, mem(rel)} -> stream(Tuple)",
    "_ mdeletesave[_,_]",
    "Deletes tuples with given ids from a relation and stores "
    "deleted tuple into a auxiliary relation. ",
    "query \"ten\" mfeed filter[.No > 6] tids "
    "mdeletesave[\"ten\", \"tenDel\"]  count"
);

/*
7.1.5 Instance of operator ~mdeletesave~

*/
Operator mdeletesaveOp (
    "mdeletesave",
    mdeletesaveSpec.getStr(),
    4,
    mdeletesaveVM,
    mdeletesaveSelect,
    mdeleteTM<true>
);


  
/*
7.2 Operator ~mdeletebyid~

The Operator ~mdeletebyid~ deletes all tuples possessing a given 
TupleIdentifier from a main memory relation. It returns the deleted
Tuple including a new attribute of type `tid` as a stream.

7.15.1 Type Mapping Funktion for operator ~mdeletebyid~

{string, mem(rel(tuple(x)))} x (tid) -> stream(tuple(x@[TID:tid]))

*/
ListExpr mdeletebyidTypeMap(ListExpr args) {

  if(nl->ListLength(args) != 2) {
    return listutils::typeError("wrong number of arguments");
  }

  // process first arg
  if(!checkUsesArgs(args)){
    return listutils::typeError("internal error");
  }
  ListExpr first = nl->First(args);
  string errMsg;
  if(!getMemType(nl->First(first), nl->Second(first), first, errMsg)){
    return listutils::typeError("string or mem(rel) expected : " + errMsg);
  }
  first = nl->Second(first); // remove mem
  if(!Relation::checkType(first)){
    return listutils::typeError("first arg is not a memory relation");
  }

  // process second arg (tid)
  ListExpr second = nl->First(nl->Second(args));
  if(!TupleIdentifier::checkType(second)){
    return listutils::typeError("second argument must be a tid");
  }
  
  ListExpr rest = nl->Second(nl->Second(first));
  ListExpr at;
  if(listutils::findAttribute(rest,"TID",at)){
    return listutils::typeError("Relation has a TID attribute");
  }
  ListExpr attrlist = nl->OneElemList(nl->First(rest));
  ListExpr currentattr = attrlist;
  rest = nl->Rest(rest);
  while (!(nl->IsEmpty(rest))) {
    currentattr = nl->Append(currentattr,nl->First(rest));
    rest = nl->Rest(rest);
  }
  
  currentattr = nl->Append(currentattr,
                           nl->TwoElemList(
                             nl->SymbolAtom("TID"),
                             nl->SymbolAtom(TupleIdentifier::BasicType())));
  
  return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                         nl->TwoElemList(
                           nl->SymbolAtom(Tuple::BasicType()),
                           attrlist));
}


class mdeletebyidInfo{
  public:
    mdeletebyidInfo(vector<Tuple*>* _relation, 
                    TupleIdentifier* _tid, 
                    ListExpr _type)
      : relation(_relation), tid(_tid) {
        type = new TupleType(_type);
        firstcall = true;
      }
      
    ~mdeletebyidInfo() {
         type->DeleteIfAllowed();
     }
     
    Tuple* next() {
      if(firstcall) {
        firstcall = false;
        if(!tid->IsDefined()){
          return 0;
        }
        size_t index = tid->GetTid();
        if(index==0 || index>relation->size()){
          return 0;
        }
        Tuple* res = relation->at(index-1);
        if(!res ) {
          return 0;
        }
        (*relation)[index-1] = 0;
        // overtake the attributes from the removed tuple
        Tuple* newtup = new Tuple(type); 
        for(int i=0; i<res->GetNoAttributes(); i++) {
           newtup->CopyAttribute(i,res,i);
        }
        Attribute* tidAttr = new TupleIdentifier(true,index);
        newtup->PutAttribute(res->GetNoAttributes(), tidAttr);
        res->DeleteIfAllowed();
        return newtup;
      }
      return 0;
    }

  private:
    vector<Tuple*>* relation;
    TupleIdentifier* tid;
    TupleType* type;
    bool firstcall;
};

/*

7.15.2  Value Mapping Function of operator ~mdeletebyid~

*/
template<class T>
int mdeletebyidValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

  mdeletebyidInfo* li = (mdeletebyidInfo*) local.addr;

    switch (message) {
      case OPEN: {
          
        if(li){
          delete li;
          local.addr=0;
        }
        
        T* oN = (T*) args[0].addr;
      
        MemoryRelObject* rel = getMemRel(oN);
        if(!rel) { return 0; }
        
        TupleIdentifier* tid = (TupleIdentifier*)(args[1].addr);
        
        local.addr = new mdeletebyidInfo(rel->getmmrel(),
                                         tid,
                                         nl->Second(GetTupleResultType(s)));
        return 0;
      }

      case REQUEST:
        result.addr=li?li->next():0;
        return result.addr?YIELD:CANCEL;

      case CLOSE:
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
   }

   return 0;
}


ValueMapping mdeletebyidVM[] = {
  mdeletebyidValMap<CcString>,
  mdeletebyidValMap<Mem>
};

int mdeleteelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}


/*
7.15.4 Description of operator ~mdeletebyid~

*/
OperatorSpec mdeletepec(
    "{string, mem(rel(tuple(x)))} x (tid) -> stream(tuple(x@[TID:tid]))] ",
    "_ mdeletebyid [_]",
    "deletes the tuples possessing a given tupleid from the main memory "
    "relation",
    "query 'Staedte' mdeletebyid[[const tid value 5]] count"
);

/*
7.15.5 Instance of operator ~mdeletebyid~

*/
Operator mdeletebyidOp (
    "mdeletebyid",
    mdeletepec.getStr(),
    2,
    mdeletebyidVM,
    mdeleteelect,
    mdeletebyidTypeMap
);


/*
7.16 Operator ~mdeletedirect~

*/
ListExpr mdeletedirectTM(ListExpr args){
   string err = "stream(tuple(X)) x (mem(rel(tuple(X)))) expected";
   if(!nl->HasLength(args,2)){
      return listutils::typeError("two arguments expected");
   }
   if(!checkUsesArgs(args)){
     return listutils::typeError("internal error");
   }
   ListExpr stream = nl->First(nl->First(args));
   if(!Stream<Tuple>::checkType(stream)){
     return listutils::typeError("first arg is not a tuple stream");
   }
   ListExpr mrel = nl->Second(args);
   string error;
   if(!getMemType(nl->First(mrel), nl->Second(mrel), mrel, error, true)){
      return listutils::typeError(error);
   } 
   mrel = nl->Second(mrel);
   ListExpr streamAttrList = nl->Second(nl->Second(stream));
   ListExpr relAttrList = nl->Second(nl->Second(mrel));
   if(!nl->Equal(streamAttrList, relAttrList)){
     return listutils::typeError("attributes of stream and relation differ");
   }  
   ListExpr append = nl->OneElemList(nl->TwoElemList(
                            nl->SymbolAtom("TID"),
                            listutils::basicSymbol<TupleIdentifier>()));

   ListExpr resAttrList = listutils::concat( streamAttrList, append);
   if(!listutils::isAttrList(resAttrList)){
     return listutils::typeError("attribute TID already present. ");
   }
   return nl->TwoElemList(listutils::basicSymbol<Stream<Tuple> >(),
                          nl->TwoElemList(
                              listutils::basicSymbol<Tuple>(),
                              resAttrList));

}


class mdeletedirectInfo{

  public:
    mdeletedirectInfo(Word _stream, vector<Tuple*>* _rel,
                      vector<Tuple*>* _saverel,
                      ListExpr resultTupleType):
     stream(_stream), rel(_rel), saverel(_saverel) {
       tt = new TupleType(resultTupleType);
       stream.open();
   }
  
   ~mdeletedirectInfo(){
       stream.close();
       tt->DeleteIfAllowed();
    }

    Tuple* next(){
       Tuple* inTuple = stream.request();
       if(!inTuple){
          return 0;
       }
       TupleId tid = inTuple->GetTupleId();
       if(tid>0 && tid <= rel->size()){
          Tuple* victim = rel->at(tid-1);
          if(victim){
             rel->at(tid-1) = 0;
             victim->DeleteIfAllowed();
          }
       }
       Tuple* resTuple = createResTuple(inTuple);
       inTuple->DeleteIfAllowed();
       if(saverel){
         resTuple->IncReference();
         saverel->push_back(resTuple);
       }
       return resTuple;
    }
  
  private:
     Stream<Tuple> stream;
     vector<Tuple*>* rel;
     vector<Tuple*>* saverel;
     TupleType* tt;

     Tuple* createResTuple(Tuple* inTuple){
        Tuple* res = new Tuple(tt);
        for(int i=0;i<inTuple->GetNoAttributes(); i++){
           res->CopyAttribute(i,inTuple,i);
        }
        res->PutAttribute(inTuple->GetNoAttributes(),
                          new TupleIdentifier(true,inTuple->GetTupleId()));
        return res;
     }

};


template<class T>
int mdeletedirectVMT(Word* args, Word& result,
                    int message, Word& local, Supplier s) {

  mdeletedirectInfo* li = (mdeletedirectInfo*) local.addr;

    switch (message) {
      case OPEN: {
        if(li){
          delete li;
          local.addr=0;
        }
        
        T* oN = (T*) args[1].addr;
      
        MemoryRelObject* rel = getMemRel(oN);
        if(!rel) { return 0; }
       
        local.addr = new  mdeletedirectInfo(
                            args[0],
                            rel->getmmrel(),
                            0,
                            nl->Second(GetTupleResultType(s)));
        return 0;
      }

     case REQUEST:
            result.addr = li?li->next():0;
            return result.addr?YIELD:CANCEL;

     case CLOSE :
             if(li){
                delete li;
                local.addr = 0;
             }
             return 0;
   }
   return -1;
}

ValueMapping mdeletedirectVM[] = {
  mdeletedirectVMT<CcString>,
  mdeletedirectVMT<Mem>,
  mdeletedirectVMT<MPointer>
};

int mdeletedirectSelect(ListExpr args){
  ListExpr s = nl->Second(args);
  if(CcString::checkType(s)) return 0;
  if(Mem::checkType(s)) return 1;
  if(MPointer::checkType(s)) return 2;
  return -1;
}

OperatorSpec mdeletedirectSpec(
  "stream(tuple(X)) x mrel(tuple(X)) -> stream(tuple(X@(TID tid)))",
  "_ _ mdeletedirect",
  "This function extracts the tuple ids from the incoming stream "
  "and removes the tuples having this id from the main memory relation "
  "which can be given as a string, a mem(rel) or an mpointer(mem(rel)). "
  "The output are the incoming tuples extended by this id. Note that "
  "the values of the result tuples have nothing to do with the values "
  "stored in the relation. ",
  "query ten feed \"ten\" mdeletedirect consume"
);

Operator mdeletedirectOp(
  "mdeletedirect",
  mdeletedirectSpec.getStr(),
  3,
  mdeletedirectVM,
  mdeletedirectSelect,
  mdeletedirectTM
);

/*
7.17 Operator ~mdeletedirectsave~

*/
ListExpr mdeletedirectsaveTM(ListExpr args){
   string err = "stream(tuple(X)) x (mem(rel(tuple(X)))) x "
                "mem(rel(tuple(X@TID))) expected";
   if(!nl->HasLength(args,3)){
      return listutils::typeError("three arguments expected");
   }
   if(!checkUsesArgs(args)){
     return listutils::typeError("internal error");
   }
   ListExpr stream = nl->First(nl->First(args));
   if(!Stream<Tuple>::checkType(stream)){
     return listutils::typeError("first arg is not a tuple stream");
   }
   ListExpr mrel = nl->Second(args);
   string error;
   if(!getMemType(nl->First(mrel), nl->Second(mrel), mrel, error, true)){
      return listutils::typeError(error);
   } 
   mrel = nl->Second(mrel);
   ListExpr streamAttrList = nl->Second(nl->Second(stream));
   ListExpr relAttrList = nl->Second(nl->Second(mrel));
   if(!nl->Equal(streamAttrList, relAttrList)){
     return listutils::typeError("attributes of stream and relation differ");
   }  
   ListExpr append = nl->OneElemList(nl->TwoElemList(
                            nl->SymbolAtom("TID"),
                            listutils::basicSymbol<TupleIdentifier>()));

   ListExpr resAttrList = listutils::concat( streamAttrList, append);
   if(!listutils::isAttrList(resAttrList)){
     return listutils::typeError("attribute TID already present. ");
   }
   // check the saverel
   ListExpr saverel = nl->Third(args);
   if(!getMemType(nl->First(saverel), nl->Second(saverel), saverel, 
                  error, true)){
      return listutils::typeError(error);
   } 
   saverel = nl->Second(saverel);
   ListExpr saveAttrList = nl->Second(nl->Second(saverel));
   if(!nl->Equal(saveAttrList, resAttrList)){
      return listutils::typeError("save relation scheme does not fit "
                                  "the result scheme");
   } 

   return nl->TwoElemList(listutils::basicSymbol<Stream<Tuple> >(),
                          nl->TwoElemList(
                              listutils::basicSymbol<Tuple>(),
                              resAttrList));

}

template<class T, class S>
int mdeletedirectsaveVMT(Word* args, Word& result,
                    int message, Word& local, Supplier s) {

  mdeletedirectInfo* li = (mdeletedirectInfo*) local.addr;

    switch (message) {
      case OPEN: {
        if(li){
          delete li;
          local.addr=0;
        }
        
        T* oN = (T*) args[1].addr;
      
        MemoryRelObject* rel = getMemRel(oN);
        if(!rel) { return 0; }
        
        S* sN = (S*) args[2].addr;
      
        MemoryRelObject* srel = getMemRel(sN);
        if(!srel) { return 0; }
       
        local.addr = new  mdeletedirectInfo(
                            args[0],
                            rel->getmmrel(),
                            srel->getmmrel(),
                            nl->Second(GetTupleResultType(s)));
        return 0;
      }

     case REQUEST:
            result.addr = li?li->next():0;
            return result.addr?YIELD:CANCEL;

     case CLOSE :
             if(li){
                delete li;
                local.addr = 0;
             }
             return 0;
   }
   return -1;
}

ValueMapping mdeletedirectsaveVM[] = {
  mdeletedirectsaveVMT<CcString, CcString>,
  mdeletedirectsaveVMT<CcString, Mem>,
  mdeletedirectsaveVMT<CcString, MPointer>,
  mdeletedirectsaveVMT<Mem, CcString>,
  mdeletedirectsaveVMT<Mem, Mem>,
  mdeletedirectsaveVMT<Mem, MPointer>,
  mdeletedirectsaveVMT<MPointer, CcString>,
  mdeletedirectsaveVMT<MPointer, Mem>,
  mdeletedirectsaveVMT<MPointer, MPointer>
};

int mdeletedirectsaveSelect(ListExpr args){
  ListExpr s = nl->Second(args);
  int n1 = 0;
  if(CcString::checkType(s)) n1 = 0;
  else if(Mem::checkType(s)) n1 = 3;
  else if(MPointer::checkType(s)) n1 = 6;

  s = nl->Third(args);
  int n2 = 0;
  if(CcString::checkType(s)) n2 = 0;
  else if(Mem::checkType(s)) n2 = 1;
  else if(MPointer::checkType(s)) n2 = 2;

  return n1+n2;
}

OperatorSpec mdeletedirectsaveSpec(
  "stream(tuple(X)) x mrel(tuple(X)) x mem(rel(tuple(X@TID))"
  " -> stream(tuple(X@(TID tid)))",
  "_ _ _ mdeletedirectsave",
  "This function extracts the tuple ids from the incoming stream "
  "and removes the tuples having this id from the main memory relation "
  "which can be given as a string, a mem(rel) or an mpointer(mem(rel)). "
  "The output tuples are the incoming tuples extended by this id. "
  "Beside giving the result tuples into the output stream, these "
  "tuples are saved into an auxiliary relation given as the third argument. "
  "Note that "
  "the values of the result tuples have nothing to do with the values "
  "stored in the relation. ",
  "query ten feed \"ten\" \"ten_aux\" mdeletedirectsave consume"
);

Operator mdeletedirectsaveOp(
  "mdeletedirectsave",
  mdeletedirectsaveSpec.getStr(),
  3,
  mdeletedirectsaveVM,
  mdeletedirectsaveSelect,
  mdeletedirectsaveTM
);


/*
7.19 Operator ~mupdate~

*/


ListExpr mupdateTM(ListExpr args){
  string err = "stream(tid) x {string, mem(rel) } x funlist expected";
  if(!nl->HasLength(args,3)){
    return listutils::typeError(err + " (wrong number of args)");
  }
  if(!checkUsesArgs(args)){
    return listutils::typeError("internal error");
  }
  ListExpr stream = nl->First(nl->First(args));
  if(!Stream<TupleIdentifier>::checkType(stream)){
    return listutils::typeError(err+ " (first arg is not a tid stream)");
  }
  ListExpr mem = nl->Second(args);
  if(!getMemType(nl->First(mem), nl->Second(mem), mem, err)) {
      return listutils::typeError("(problem in second arg : " 
                                  + err + ")");
  }
  if(!Relation::checkType(nl->Second(mem))){
     return listutils::typeError("Second arg is not a memory relation");
  }
  ListExpr funList  = nl->First(nl->Third(args));
  ListExpr tupleList = nl->Second(nl->Second(mem));
  ListExpr attrList = nl->Second(tupleList);

  ListExpr at;
  if(listutils::findAttribute(attrList,"TID", at)){
      return listutils::typeError("attribute TID already present");
  }


  // each element of funlist must be a list consisting
  // of an attribute name in attrList and a function mapping
  // tupleList into the attribute's type
  set<string> used; // used attribute names
  ListExpr appendList = nl->TheEmptyList();
  ListExpr last;
  bool first = true;

  while(!nl->IsEmpty(funList)){
    ListExpr fun = nl->First(funList);
    funList = nl->Rest(funList);
    if(!nl->HasLength(fun,2)){ //(name map)
       return listutils::typeError("found element in typelist having "
                                   "invalid length");
    }
    ListExpr name = nl->First(fun);
    if(nl->AtomType(name)!=SymbolType){
       return listutils::typeError("invalid attribute name");
    } 
    string attr = nl->SymbolValue(nl->First(fun));
    if(used.find(attr)!=used.end()){
      return listutils::typeError("used attribute name " + attr + " twice");
    }
    used.insert(attr);
    ListExpr map = nl->Second(fun);
    ListExpr attrType;
    int attrIndex = listutils::findAttribute(attrList, attr, attrType);
    if(!attrIndex){
      return listutils::typeError("Attribute " + attr 
                                  + " not part of the relation");
    }
    if(first){
      appendList = nl->OneElemList( nl->IntAtom(attrIndex-1));     
      last = appendList;
    } else {
      last = nl->Append(last, nl->IntAtom(attrIndex-1));
    }
    if(!listutils::isMap<1>(map)){
      return listutils::typeError("invalid map definition");
    }
    if(!nl->Equal(nl->Second(map), tupleList)){
      return listutils::typeError("function argument for " + attr +
                              + " does not corresponds to the relation type");
    }
    if(!nl->Equal(nl->Third(map), attrType)){
      return listutils::typeError("function result for " + attr
                             + " does not corresponds to the attr type");
    }
  }
  if(nl->IsEmpty(appendList)){
    return listutils::typeError("function list is empty");     
  }
  
  // create result tuple list
  // step 1: overtake attributes
  ListExpr tmp = attrList;
  ListExpr resAttrList = nl->OneElemList(nl->First(tmp));
  last = resAttrList;
  tmp = nl->Rest(tmp); 
  while(!nl->IsEmpty(tmp)){
     last = nl->Append(last, nl->First(tmp));
     tmp = nl->Rest(tmp);
  }
  // append _old attributes
  tmp = attrList;
  while(!nl->IsEmpty(tmp)){
    ListExpr attr = nl->First(tmp);
    tmp = nl->Rest(tmp);
    string newName = nl->SymbolValue(nl->First(attr))+"_old";
    if(listutils::findAttribute(attrList,newName,at)){
      return listutils::typeError("Attribute " + newName 
                                  + "already known in tuple");
    }
    ListExpr na = nl->TwoElemList(nl->SymbolAtom(newName),
                                  nl->Second(attr));
    last = nl->Append(last, na);
  }
  // append TID
  last = nl->Append(last, nl->TwoElemList( nl->SymbolAtom("TID"),
                                 listutils::basicSymbol<TupleIdentifier>()));

  tupleList = nl->TwoElemList(listutils::basicSymbol<Tuple>(),
                              resAttrList);
  return nl->ThreeElemList(
                 nl->SymbolAtom(Symbols::APPEND()),
                 appendList,
                 nl->TwoElemList( listutils::basicSymbol<Stream<Tuple> >(),
                                  tupleList));
}


class mupdateInfoN{

 public:
   mupdateInfoN(Word _stream, vector<Tuple*>* _rel,
               vector<int> _attrs,Supplier _funs,
               ListExpr tt ) :stream(_stream),
                   rel(_rel), attrs(_attrs){
       stream.open();
       assert(qp->GetNoSons(_funs) == (int) attrs.size());
       for(int i=0;i<qp->GetNoSons(_funs); i++){
         Supplier s = qp->GetSupplier(qp->GetSupplier(_funs,i),1);
         funs.push_back(s);
         funargs.push_back(qp->Argument(s));
       }
       resType = new TupleType(tt);
   }
   ~mupdateInfoN(){
     stream.close();
     resType->DeleteIfAllowed();
   }

   Tuple* next(){
    TupleIdentifier* tid;
     while( (tid=stream.request()) ){
       if(tid->IsDefined()){
         TupleId id = tid->GetTid();
         if(id>0 && id <= rel->size()){
            Tuple* res = rel->at(id-1);
            if(res){
               res->SetTupleId(id);
               tid->DeleteIfAllowed();
               return updateTuple(res);
            }
         }
       }
       tid->DeleteIfAllowed();
     } // stream exhausted
     return 0;
   }



 private:
    Stream<TupleIdentifier> stream;
    vector<Tuple*>* rel;
    vector<int> attrs;
    vector<Supplier> funs;
    vector<ArgVectorPointer> funargs;
    TupleType* resType;

    Tuple* updateTuple(Tuple* orig){
       Tuple* res = new Tuple(resType);
       // copy all attributes twice
       for(int i=0;i<orig->GetNoAttributes();i++){
         res->CopyAttribute(i,orig,i);
         res->CopyAttribute(i,orig, i + orig->GetNoAttributes());
       }
       res->PutAttribute(res->GetNoAttributes()-1, 
                         new TupleIdentifier(true, orig->GetTupleId()));

       // update according to the functions
       for(size_t i=0;i<funs.size(); i++){
          Supplier s = funs[i];
          ArgVectorPointer a = funargs[i];
          (*a)[0].setAddr(orig);
          Word funres;
          qp->Request(s,funres);
          Attribute* resAttr = ((Attribute*)funres.addr)->Clone();
          orig->PutAttribute(attrs[i], resAttr);
          res->PutAttribute(attrs[i], resAttr->Copy());
       }
       return res;

    }
};



template<class T>
int mupdateVMT(Word* args, Word& result,
               int message, Word& local, Supplier s) {

   mupdateInfoN* li = (mupdateInfoN*) local.addr;
   switch(message){
     case OPEN : {
                   if(li){
                     delete li;
                     local.addr = 0;
                   }
                   T* mrel = (T*) args[1].addr;
                   MemoryRelObject* rel = getMemRel(mrel);
                   if(!rel) { 
                      return 0;
                   }
                   vector<int> attrs;
                   for(int i=3; i<qp->GetNoSons(s); i++){
                     attrs.push_back(((CcInt*)args[i].addr)->GetValue());
                   }
                   ListExpr tt = nl->Second(GetTupleResultType(s));
                   local.addr = new mupdateInfoN(args[0],
                                                rel->getmmrel(),
                                                attrs,
                                                qp->GetSon(s,2), tt);
                   return 0;
                 }
      case REQUEST: result.addr = li?li->next():0;
                    return result.addr?YIELD:CANCEL;
      case CLOSE : if(li){
                      delete li;
                      local.addr=0;
                   }
                   return 0; 
   }
   return -1;
}

ValueMapping mupdateVMN[] = {
   mupdateVMT<CcString>,
   mupdateVMT<Mem>
};

int mupdateSelectN(ListExpr args){
  return CcString::checkType(nl->Second(args))?0:1;
}

OperatorSpec mupdateSpecN(
  "stream(tid) x {string, mem(rel)} x funlist -> stream(tuple)",
  "<stream> mupdate[ <memrel> ; funs]",
  "Applies functions to the tuples in <memrel> specified by "
  "their id's in the incomming stream and updates the tuples "
  "in the relation. ",
  "query \"ten\" mfeed  filter[.No = 6] mupdate[\"ten\", No : .No + 23] count"
);


Operator mupdateNOp(
  "mupdate",
  mupdateSpecN.getStr(),
  2,
  mupdateVMN,
  mupdateSelectN,
  mupdateTM
);









enum UpdateOp {
  MUpdate,
  MUpdateSave,
  MUpdateByID
};

void update(vector<Tuple*>* relation, Tuple* res, 
            int changedIndex, Attribute* attr) {
      
  vector<int> attrPos;
  
  attrPos.push_back(changedIndex+1);
  
  vector<Tuple*>::iterator it = relation->begin();
  
  while(it != relation->end()) {
    Tuple* tup = *it;
    if(tup){
      if(TupleComp::equal(res,tup,&attrPos)) {
        tup->PutAttribute(changedIndex, attr->Clone());
        return;
      }
    }
    it++;
  }
}

Tuple* copyAttributes(Tuple* res, TupleType* type) {
  Tuple* tup = new Tuple(type);
  assert(tup->GetNoAttributes() == 2 * res->GetNoAttributes() + 1);
  for (int i=0; i<res->GetNoAttributes(); i++)
    tup->PutAttribute(res->GetNoAttributes()+i,
                      res->GetAttribute(i)->Clone());
  return tup;
}
    
/*
7.2 Operator ~mupdate~

The Operator ~mupdate~ updates all attributes in the tuples of a main 
memory relation. The tuple to update are given by an input stream, the new
values of the attributes a calculated with a set of given functions.
The updated tuple, including a new attribute of type `tid`, 
are appended to an output stream.

7.16.1 General Type Mapping Funktions for operator ~mupdate~ and
       ~mupdatesave~.

  stream(tid) x mrel x funlist

  stream(tid) x mrel x mrel x funlist 

*/
template<bool save>    
ListExpr mupdateTypeMap(ListExpr args) {

  int noargs = save?4:3;
  if(!checkUsesArgs(args)){
    return listutils::typeError("internal error");
  }
  if(!nl->HasLength(args,noargs)){
    return listutils::typeError("wrong number of arguments");
  }  

  // process stream
  ListExpr stream = nl->First(nl->First(args));
  if(!Stream<TupleIdentifier>::checkType(stream)){
    return listutils::typeError("first argument must be a tuple id's");
  }
  
  // process second argument (mem(rel))
  string errMsg;
  ListExpr second = nl->Second(args);
  if(!getMemType(nl->First(second), nl->Second(second), second, errMsg)) {
      return listutils::typeError("(problem in second arg : " 
                                  + errMsg + ")");
  }
  second = nl->Second(second);
  if(!Relation::checkType(second)) {
    return listutils::typeError("second arg is not a memory relation");
  }
  ListExpr at;
  if(listutils::findAttribute(nl->Second(nl->Second(second)),"TID",at)){
    return listutils::typeError("Relation already contains a TID attribute");
  }

  ListExpr maps = save?nl->Fourth(args):nl->Third(args);
  maps = nl->First(maps);

  if(   (nl->AtomType(maps)!=NoAtom)
     || nl->IsEmpty(maps)){
    return listutils::typeError("last arg must be a non-empty list of maps");
  } 

  // check maps
  ListExpr tuple = nl->Second(second);
  ListExpr attrList = nl->Second(tuple);

  ListExpr indexList = nl->TheEmptyList();
  ListExpr lastIndex = indexList;

  // create the result list as a copy from attrlist
  ListExpr newAttrList = nl->OneElemList(nl->First(attrList));
  ListExpr last = newAttrList;
  ListExpr tmp = nl->Rest(attrList);
  while(!nl->IsEmpty(tmp)){
     last = nl->Append(last,nl->First(tmp));
     tmp = nl->Rest(tmp);
  }
  // append the same attributes with additional _old
  tmp = attrList;
  while(!nl->IsEmpty(tmp)){
     ListExpr a = nl->First(tmp);
     tmp = nl->Rest(tmp);
     last = nl->Append(last, nl->TwoElemList( 
                                nl->SymbolAtom(
                                   nl->SymbolValue(nl->First(a))+"_old"),
                                nl->Second(a)));
  } 
  last = nl->Append(last,nl->TwoElemList(
                             nl->SymbolAtom("TID"),
                             listutils::basicSymbol<TupleIdentifier>())); 
  if(!listutils::isAttrList(newAttrList)){
    return listutils::typeError("Found name conflict in result type");
  }

  set<string> used;
  while(!nl->IsEmpty(maps)){
    ListExpr map = nl->First(maps);
    maps = nl->Rest(maps);
    if(!nl->HasLength(map,2)){  // (name map)
       return listutils::typeError("funlist contains not a named function");
    }
    if(  !listutils::isSymbol(nl->First(map))
       ||!listutils::isMap<1>(nl->Second(map))){
       return listutils::typeError("funlist contains not a named function");
    }
    string attrName = nl->SymbolValue(nl->First(map));
    if(used.find(attrName)!=used.end()){
      return listutils::typeError("used " + attrName + " twice");
    }
    used.insert(attrName);
    if(listutils::findAttribute(attrList, attrName+"_old", at)){
      return listutils::typeError(attrName
                   + "_old already part of the relation");
    }
    int index = listutils::findAttribute(attrList, attrName, at);
    if(index==0){
      return listutils::typeError(  "Attribute " + attrName 
                                  + " not part of the relation");
    }
    ListExpr fun = nl->Second(map);
    if(!nl->Equal(nl->Second(fun), tuple)){
      return listutils::typeError("function argument for " + attrName 
                    + " differs from the tuple type");
    } 
    if(!nl->Equal(nl->Third(fun), at)){
      return listutils::typeError("function result for " + attrName 
                    + " differs from the attribute type");
    } 
    // checks successful, extend lists
    if(nl->IsEmpty(indexList)){
       indexList = nl->OneElemList(nl->IntAtom(index-1));
       lastIndex = indexList;
    } else {
       lastIndex = nl->Append(lastIndex,nl->IntAtom(index-1));
    }
  }
  ListExpr newTuple = nl->TwoElemList(
                         listutils::basicSymbol<Tuple>(),
                         newAttrList);


  if(save){ // check third argument
     ListExpr auxRel = nl->Third(args);
     if(!getMemType(nl->First(auxRel), nl->Second(auxRel), auxRel, errMsg)) {
        return listutils::typeError("(problem in third arg : " 
                                  + errMsg + ")");
     }
     auxRel = nl->Second(auxRel);
     if(!Relation::checkType(auxRel)){
        return listutils::typeError("third arg is not a memory relation");
     }
     if(!nl->Equal(nl->Second(auxRel), newTuple)){
        return listutils::typeError("auxiliarary relation has another type "
                         "than the result");
     }
  }

  return nl->ThreeElemList(
                nl->SymbolAtom(Symbols::APPEND()),
                indexList,
                nl->TwoElemList(
                      listutils::basicSymbol<Stream<Tuple> >(),
                      newTuple));
}

class mupdateInfo {
  public:
    mupdateInfo(Word _stream, 
                vector<Tuple*>* _mainRelation, 
                bool _flob, 
                Supplier _funs, 
                vector<Tuple*>* _auxRelation,
                vector<int>& _indexes, 
                ListExpr _type) :
       stream(_stream), mainRelation(_mainRelation),
       flob(_flob),  auxRelation(_auxRelation),
       indexes(_indexes) {
         type = new TupleType(_type);
         stream.open();
         for(int i=0;i<qp->GetNoSons(_funs); i++){
            Supplier s = qp->GetSupplier(qp->GetSupplier(_funs,i),1);
            ArgVectorPointer p = qp->Argument(s);
            funs.push_back(s);
            funargs.push_back(p); 
         } 
         assert(indexes.size()==funs.size());

    }


    ~mupdateInfo(){
      stream.close();
      type->DeleteIfAllowed();
    }
    
    Tuple* next() {
       TupleIdentifier* t;
       while( (t=stream.request())){
          if(!t->IsDefined()){
            t->DeleteIfAllowed();
          } else {
            TupleId id = t->GetTid();
            t->DeleteIfAllowed();
            if(id>0 && id <=mainRelation->size()){
               Tuple* tuple = mainRelation->at(id-1);
               if(tuple){
                  return createResultTuple(id, tuple);
               }
            }
          }
       }
       return 0;
    }

  private:
     Stream<TupleIdentifier> stream;
     vector<Tuple*>* mainRelation; 
     bool flob;
     vector<Tuple*>* auxRelation;
     vector<int> indexes;
     vector<Supplier> funs;
     vector<ArgVectorPointer> funargs;
     TupleType* type;

     Tuple* createResultTuple(TupleId id, Tuple* tuple){
         Tuple* res = new Tuple(type);
         // copy all attributes twice into the new tuple
         for(int i=0;i<tuple->GetNoAttributes(); i++){
           res->CopyAttribute(i,tuple,i);
           res->CopyAttribute(i,tuple, i + tuple->GetNoAttributes());
         }
         // put tid into tuple
         res->PutAttribute(res->GetNoAttributes()-1, 
                           new TupleIdentifier(true,id));
         res->SetTupleId(id);
         // update attributes
         for(size_t i=0;i<funs.size();i++){
            (*(funargs[i]))[0] = tuple;
            Word v;
            qp->Request(funs[i],v);
            Attribute* newAttr = ((Attribute*)v.addr)->Clone();
            if(flob){
             newAttr->bringToMemory();
            }
            res->PutAttribute(indexes[i], newAttr); // put to res
            tuple->PutAttribute(indexes[i], newAttr->Copy()); // update relation
         }
         // save if necessary
         if(auxRelation){
           res->IncReference();
           auxRelation->push_back(res); 
         }
         return res;
     }
};


/*

7.16.3  Value Mapping Function of operator ~mupdate~

*/
template<class T, class S, bool save>
int mupdateValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {
  
  mupdateInfo* li = (mupdateInfo*) local.addr;

  switch (message) {
    
    case OPEN : {
      
      if(li) {
        delete li;
        local.addr=0;
      }
      T* oN = (T*) args[1].addr; 
      MemoryRelObject* rel = getMemRel(oN);
      ListExpr tt = nl->Second(GetTupleResultType(s));
      int funindex = 2;
      vector<Tuple*>* auxRel = 0;
      if(save){
         funindex = 3;
         S* aRel = (S*) args[2].addr;
         MemoryRelObject* arel = getMemRel(aRel);
         auxRel = arel->getmmrel();
      }      

      vector<int> indexes;
      for(int i=funindex+1;i<qp->GetNoSons(s);i++){
        indexes.push_back(((CcInt*)args[i].addr)->GetValue());
      }
      local.addr = new mupdateInfo(args[0], // stream
                                   rel->getmmrel(), // main relation
                                   rel->hasflob(), // flob?
                                   qp->GetSon(s,funindex), // funlist
                                   auxRel, // auxiliary relation
                                   indexes,  // attribute indexes
                                   tt); // result tuple type
      return 0;
    }
    
    case REQUEST : {
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;
    }
    
    case CLOSE :
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
  }
  return 0;
}

ValueMapping mupdateVM[] = {
   mupdateValMap<CcString, CcString, false>,
   mupdateValMap<Mem, CcString, false>
};

int mupdateSelect(ListExpr args){
   return CcString::checkType(nl->Second(args))?0:1;
}



/*
7.16.4 Description of operator ~mupdate~

*/
OperatorSpec mupdateSpec(
    "stream(tuple(x)) x string x [(a1, (tuple(x) -> d1)) "
    "... (an, (tuple(x) -> dn))] -> "
    "stream(tuple(x @ [x1_old t1] @...[xn_old tn] @ [TID tid])))",
    "_ mupdate[_; funlist] implicit parameter tuple type TUPLE",
    "updates the tuple of a stream in an "
    "existing main memory relation",
    "query Staedte feed filter[.SName = 'Hannover'] "
    "mupdate['Staedte';Bev: .Bev + 1000] count"
);

/*
7.16.5 Instance of operator ~mupdate~

*/
Operator mupdateOp (
    "mupdate",
    mupdateSpec.getStr(),
    2,
    mupdateVM,
    mupdateSelect,
    mupdateTypeMap<false>
);


ValueMapping mupdatesaveVM[] = {
   mupdateValMap<CcString, CcString, true>,
   mupdateValMap<CcString, Mem, true>,
   mupdateValMap<Mem, CcString, true>,
   mupdateValMap<Mem, Mem, true>
};

int mupdatesaveSelect(ListExpr args){
   int n1 = CcString::checkType(nl->Second(args))?0:2;
   int n2 = CcString::checkType(nl->Third(args))?0:1;
   return n1+n2;
}



/*
7.17.4 Description of operator ~mupdatesave~

*/
OperatorSpec mupdatesaveSpec(
    "stream(tuple(x)) x {string, mem(rel(tuple(x)))} "
    "x [(a1, (tuple(x) -> d1)) ... (an,(tuple(x) -> dn))] "
    "x {string, mem(rel((tuple(x@[TID:tid]))))} -> "
    "stream(tuple(x @ [x1_old t1] @...[xn_old tn] @ [TID tid])))",
    "_ mupdatesave[_,_; funlist] implicit parameter tuple type TUPLE",
    "updates the tuple of a stream in an "
    "existing main memory relation and saves the tuples of the output "
    "stream in an additional main memory relation",
    "query Staedte feed filter[.SName = 'Hannover'] "
    "mupdatesave['Staedte','Stadt';Bev: .Bev + 1000] count"
);

/*
7.17.5 Instance of operator ~mupdatesave~

*/
Operator mupdatesaveOp (
    "mupdatesave",
    mupdatesaveSpec.getStr(),
    4,
    mupdatesaveVM,
    mupdatesaveSelect,
    mupdateTypeMap<true>
);


/*
7.18 Operator ~mupdatebyid~

The Operator ~mupdatebyid~ updates the tuple with the given tupleidentifier
in a main memory relation, using functions to calculate the new attribute 
values. The updated tuple is returned in an output stream with all attributes
copied and extended by the suffix 'old'. Also an attribute of type 'tid' is
added.

7.18.1 Type Mapping Function of operator


*/
ListExpr mupdatebyidTypeMap(ListExpr args) {
  
  if(nl->ListLength(args) != 3) {
    return listutils::typeError("wrong number of arguments");
  }

  // process first arg
  ListExpr first = nl->First(args);
  if(!nl->HasLength(first,2)){
      return listutils::typeError("internal error");
  }
  string errMsg;
  if(!getMemType(nl->First(first), nl->Second(first), first, errMsg)){
    return listutils::typeError("string or mem(rel) expected : " + errMsg);
  }
  first = nl->Second(first); // remove mem
  if(!Relation::checkType(first)){
    return listutils::typeError("first arg is not a memory relation");
  }

  // process second arg (tid)
  ListExpr second = nl->First(nl->Second(args));
  if(!listutils::isSymbol(second,TupleIdentifier::BasicType())){
    return listutils::typeError("second argument must be a tid");
  }
    
  // process update function
  ListExpr map = nl->Third(args);
  
  // argument is not a map
  if(nl->ListLength(map<1)) {
    return listutils::typeError("arg must be a list of maps");
  }
  ListExpr maprest = nl->First(map);
  int noAttrs = nl->ListLength(map);
  
  // Go through all functions
  ListExpr mapfirst, mapsecond;
  ListExpr attrType;
  ListExpr indices, indicescurrent;
  
  while (!(nl->IsEmpty(maprest))) {
    map = nl->First(maprest);
    maprest = nl->Rest(maprest);
    mapfirst = nl->First(map);
    mapsecond = nl->Second(map);
    
    // check if argument map is a function
    if(!listutils::isMap<1>(mapsecond)){
      return listutils::typeError("not a map found");
    }
    if(!nl->Equal(nl->Second(first),nl->Second(mapsecond))) {
      return listutils::typeError("argument of map is wrong");
    }

    // check for valid attribute name in function
    if(!listutils::isSymbol(mapfirst)){
      return listutils::typeError("not a valid attribute name:" +
                                  nl->ToString(mapfirst));
    }
    string argstr;
    int attrIndex;
    nl->WriteToString(argstr, mapfirst);
    attrIndex = listutils::findAttribute(nl->Second(nl->Second(first)),
                              argstr, attrType);
    if(attrIndex==0){
      return listutils::typeError("attribute " + argstr + " not known");
    }
    if(!nl->Equal(attrType, nl->Third(mapsecond))){
      return listutils::typeError("result of the map and attribute"
                                  " type differ");
    }
    
    // construct list with all indices of the changed attributes
    // in the inputstream to be appended to the resultstream
    bool firstcall = true;
    if(firstcall) {
      indices = nl->OneElemList(nl->IntAtom(attrIndex));
      indicescurrent = indices;
      firstcall = false;
    }
    else 
      indicescurrent = nl->Append(indicescurrent,
                                  nl->IntAtom(attrIndex));
  }
  
  maprest = nl->Second(nl->Second(first));
  
  ListExpr attrlist = nl->OneElemList(nl->First(maprest));
  ListExpr currentattr = attrlist;
  maprest = nl->Rest(maprest);
  
  while (!(nl->IsEmpty(maprest))) {
    currentattr = nl->Append(currentattr,nl->First(maprest));
    maprest = nl->Rest(maprest);
  }
  
  // build second part of the resultstream
  maprest = nl->Second(nl->Second(first));
  string oldName;
  ListExpr oldattr;
  while (!(nl->IsEmpty(maprest))) {
    nl->WriteToString(oldName, nl->First(nl->First(maprest)));
    oldName += "_old";
    oldattr = nl->TwoElemList(nl->SymbolAtom(oldName),
                              nl->Second(nl->First(maprest)));
    currentattr = nl->Append(currentattr,oldattr);
    maprest = nl->Rest(maprest);
  }
  currentattr = nl->Append(currentattr,
                           nl->TwoElemList(
                             nl->SymbolAtom("TID"),
                             nl->SymbolAtom(TupleIdentifier::BasicType())));
  
  ListExpr resType = nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                                     nl->TwoElemList(
                                       nl->SymbolAtom(Tuple::BasicType()),
                                       attrlist));
  
  return nl->ThreeElemList(nl->SymbolAtom(Symbols::APPEND()),
                           nl->TwoElemList(nl->IntAtom(noAttrs),
                                           indices),
                           resType);
}

class mupdatebyidInfo {
  public:
    mupdatebyidInfo(vector<Tuple*>* _relation,
                    TupleIdentifier* _tid, Word _arg,
                    Word _arg2, TupleType* _type)
      : relation(_relation), tid(_tid), arg(_arg), 
        arg2(_arg2), type(_type) {
        firstcall = true;
      }
    
    ~mupdatebyidInfo(){
      type->DeleteIfAllowed();
    }
    
    Tuple* next() {
      if(!firstcall) return 0;
      firstcall = false;
      if(!tid->IsDefined()){
          return 0;
      } 
      TupleId id = tid->GetTid();
      if(id<1 || id>relation->size()){
          return 0;
      } 
      Tuple* res = relation->at(id-1);
      if(!res) return 0;
      
      Tuple* newtup = copyAttributes(res,type);
        
      // Supplier for the functions
      Supplier supplier = arg.addr;

      Word elem, value;  
      Supplier son = qp->GetSupplier(arg2.addr, 0);
      qp->Request(son, elem);
      int changedIndex = ((CcInt*)elem.addr)->GetIntval()-1;

      // Get next appended index
      Supplier s1 = qp->GetSupplier(supplier, 0);
      // Get the function
      Supplier s2 = qp->GetSupplier(s1, 1);
      ArgVectorPointer funargs = qp->Argument(s2);
      ((*funargs)[0]).setAddr(res);
      qp->Request(s2,value);
      Attribute* newAttr = ((Attribute*)value.addr)->Clone();
      
      update(relation,res,changedIndex,newAttr); 
      res->SetTupleId(tid->GetTid());
      for(int i = 0; i < res->GetNoAttributes(); i++) {
        if(i!=changedIndex)
          newtup->CopyAttribute(i,res,i);
        else
          newtup->PutAttribute(i,newAttr);
      }
      //get tuple id and append it to tuple
      Attribute* tidAttr = new TupleIdentifier(true,tid->GetTid());
      newtup->PutAttribute(newtup->GetNoAttributes()-1,tidAttr);
        
      return newtup;
    }

  private:
     vector<Tuple*>* relation;
     TupleIdentifier* tid;
     Word arg;
     Word arg2;
     TupleType* type;
     bool firstcall;
};


/*

7.18.3  Value Mapping Function of operator ~mupdatebyid~

*/
template<class T>
int mupdatebyidValueMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {
  
  mupdatebyidInfo* li = (mupdatebyidInfo*) local.addr;
  
  switch (message) {
    
    case OPEN : {
      
      if(li){
        delete li;
        local.addr=0;
      }
      
      T* oN = (T*) args[0].addr;
      MemoryRelObject* rel = getMemRel(oN);
      if(!rel) {return 0;}
      
      TupleIdentifier* tid = (TupleIdentifier*)(args[1].addr);
      TupleType* tt = new TupleType(nl->Second(GetTupleResultType(s)));
      
      local.addr = new mupdatebyidInfo(rel->getmmrel(),tid,args[2],args[4],tt);
      return 0;
    }
    
    case REQUEST : {
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;
    }
    
    case CLOSE :
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
      
  }
  return 0;

}

ValueMapping mupdatebyidVM[] = {
   mupdatebyidValueMap<CcString>,
   mupdatebyidValueMap<Mem>
};

int mupdatebyidSelect(ListExpr args){
   return CcString::checkType(nl->Second(args))?0:1;
}

/*
7.18.4 Description of operator ~mupdatebyid~

*/
OperatorSpec mupdatebyidSpec(
    "{string, mem(rel(tuple))} x (tid) x "
    "[(a1, (tuple(x) -> d1)) ... (an,(tuple(x) -> dn))] "
    "-> stream(tuple(x @ [x1_old t1] @...[xn_old tn] @ [TID tid])))",
    "_ mupdatebyid[_; funlist] implicit parameter tuple type MTUPLE",
    "updates the tuple with the given tupleidentifier from "
    "a main memory relation",
    "query Staedte feed filter[.SName = 'Hannover'] "
    "mupdate['Staedte'; Bev: .Bev + 1000] count"
);

/*

7.18.5 Instance of operator ~mupdatebyid~

*/
Operator mupdatebyidOp (
    "mupdatebyid",
    mupdatebyidSpec.getStr(),
    2,
    mupdatebyidVM,
    mupdatebyidSelect,
    mupdatebyidTypeMap
);


/////////////////////////////////////// ORDERED RELATION ////
enum ChangeType{
  MOInsert,
  MODelete,
  MOUpdate
};



/*
7.19 Operator ~moinsert~

The Operator ~moinsert~ insert all tuples of an input stream into an 
ordered main memory relation. The tuples of the input stream are given 
an additional attribute of type 'tid' and added to the output stream.

*/
template<ChangeType ct>
class moinsertInfo {
  public:
         
    moinsertInfo(Word w, ttree::TTree<TupleWrap,TupleComp>* _orel, 
                 bool _flob, TupleType* _type,vector<int>* _attrPos) 
      : stream(w),orel(_orel),flob(_flob),type(_type),attrPos(_attrPos) {
      stream.open();
    }

  ~moinsertInfo() {
      stream.close();
      type->DeleteIfAllowed();
    }

    Tuple* next() {
      Tuple* res = stream.request();
      if(!res) { return 0; }
      
      if((ct==MOInsert) && flob) {
        res->bringToMemory();
      }
      
      // copy attributes of tuple from input stream
      Tuple* newtup = new Tuple(type); 
      for(int i = 0; i < res->GetNoAttributes(); i++){
        newtup->CopyAttribute(i,res,i);
      }
      
      TupleId tid = res->GetTupleId();
      switch(ct){
        case MOInsert : {
                          TupleWrap tw(res);
                          orel->insert(tw,attrPos); 
                        }
                        break;
        case MODelete : {
                          TupleWrap tw(res);
                          orel->remove(tw, attrPos); 
                        }
                        break;
        default : assert(false);
      }
      res->DeleteIfAllowed(); // remove old tuple
      
      //get tuple id and append it to the stored tuple
      Attribute* tidAttr = new TupleIdentifier(true,tid);
      newtup->PutAttribute(newtup->GetNoAttributes()-1, tidAttr);
     
       
      return newtup;
    }

  private:
     Stream<Tuple> stream;
     ttree::TTree<TupleWrap,TupleComp>* orel;
     bool flob;
     TupleType* type;
     vector<int>* attrPos;
};


/*

7.19.2  Value Mapping Functions of operator ~moinsert~

*/
template<class T, ChangeType ct>
int moinsertValueMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {
  
  moinsertInfo<ct>* li = (moinsertInfo<ct>*) local.addr;

  switch (message) {
    
    case OPEN : {
      
      if(li) {
        delete li;
        local.addr=0;
      }
      
      qp->Open(args[0].addr);
      
      T* oN = (T*) args[1].addr;
      ListExpr ttype = nl->Second(qp->GetType(qp->GetSon(s,0)));
      MemoryORelObject* orel = getMemORel(oN,ttype);
      if(!orel) { return 0; }
      
      TupleType* tt = new TupleType(nl->Second(GetTupleResultType(s)));
      
      local.addr = new moinsertInfo<ct>(args[0],orel->getmmorel(),
                                        orel->hasflob(),tt,
                                        orel->getAttrPos());
      
      return 0;
    }
    
    case REQUEST : {
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;
    }
    
    case CLOSE :
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
      
  }
  return 0;

}

ValueMapping moinsertVM[] = {
   moinsertValueMap<CcString,MOInsert>,
   moinsertValueMap<Mem,MOInsert>,
   moinsertValueMap<CcString,MODelete>,
   moinsertValueMap<Mem,MODelete>
};


template<ChangeType ct>
int moinsertSelect(ListExpr args){
  if(ct == MOInsert) 
    return CcString::checkType(nl->Second(args))?0:1;
  if(ct == MODelete)
    return CcString::checkType(nl->Second(args))?2:3;
}



/*
7.19.4 Description of operator ~moinsert~

*/
OperatorSpec moinsertSpec(
    "stream(tuple(x)) x {string, mem(orel(tuple(x)))} -> "
    "stream(tuple(x@[TID:tid]))",
    "_ moinsert [_]",
    "inserts the tuple of a stream into an "
    "existing main memory ordered relation",
    "query moinsert (ten feed head[5],\"oten\") count"
);

/*
7.19.5 Instance of operator ~moinsert~

*/
Operator moinsertOp (
    "moinsert",
    moinsertSpec.getStr(),
    4,
    moinsertVM,
    moinsertSelect<MOInsert>,
    minsertTypeMap<2>
);


/*
7.20 Operator ~modelete~

The operator ~modelete~ deletes all tuple of the input stream from the given
main memory ordered relation. The tuples of the input stream including an 
attribute of type 'tid' are appended to the output stream.

7.20.4 Description of operator ~modelete~

*/
OperatorSpec modeleteSpec(
    "stream(tuple(x)) x {string, mem(orel(tuple(x)))} "
    "-> stream(tuple(x@[TID:tid]))",
    "_ modelete [_]",
    "all tuples of an input stream will be deleted from an "
    "main memory ordered relation",
    "query ten feed head[5] modelete['oten'] count"
);

/*
7.20.5 Instance of operator ~modelete~

*/
Operator modeleteOp (
    "modelete",
    modeleteSpec.getStr(),
    4,
    moinsertVM,
    moinsertSelect<MODelete>,
    minsertTypeMap<2>
);


/*
7.21 Operator ~moconsume~

  The Operator ~moconsume~ creates an MemoryORelOBject for output on
  an user interface, i.e. the console from
  a input stream and the attribute identifiers over which the main
  memory ordered relation will be sorted.

7.21.1 Type Mapping Functions of operator ~moconsume~

stream(tuple(x)) x (ident1 ... identn) -> mem(orel(tuple(tuple(x))))

*/
ListExpr moconsumeTypeMap(ListExpr args) {
  
    if(nl->ListLength(args)!=2){
        return listutils::typeError("(wrong number of arguments)");
    }
    if(!Stream<Tuple>::checkType(nl->First(args))) {
        return listutils::typeError ("stream(tuple) expected!");
    }
    if(!listutils::isKeyDescription(nl->Second(nl->First(args)),
                                    nl->Second(args))) {
      return listutils::typeError("all identifiers of second argument must "
                                  "appear in the first argument");
    }
    ListExpr l1 = nl->Second(nl->First(args));
    ListExpr l2 = nl->SymbolAtom(MemoryORelObject::BasicType());
    return nl->TwoElemList (l2,l1);
}


/*
7.21.2  Value Mapping Function of operator ~moconsume~

*/
int moconsumeValueMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {
    
    Supplier t = qp->GetSon(s,0);
    ListExpr rel = qp->GetType(t);

    t = qp->GetSon(s,1);
    ListExpr attrToSort = qp->GetType(t);
    ListExpr type = attrToSort;
    
    result = qp->ResultStorage(s); 
    MemoryORelObject* morel = (MemoryORelObject*)result.addr;
    
    ListExpr attr;
    while (!(nl->IsEmpty(attrToSort))) {
      attr = nl->First(attrToSort);
      attrToSort = nl->Rest(attrToSort);
      
      ListExpr attrType = 0;
      int attrPos = 0;
      ListExpr attrList = nl->Second(nl->Second(rel));
      
      attrPos = listutils::findAttribute(attrList, 
                                         nl->ToString(attr), 
                                         attrType);
      if(attrPos == 0) {
        return listutils::typeError
          ("there is no attribute having name " + nl->ToString(attr));
      }
      morel->setAttrPos(attrPos,true);
    }
    
    Stream<Tuple> stream(args[0]);
    stream.open();
    Tuple* tup = 0;
    while((tup = stream.request()) != 0) {
      morel->addTuple(tup);
      tup->DeleteIfAllowed();
    }
    
    morel->setObjectTypeExpr(nl->ToString(nl->TwoElemList(nl->Second(rel),
                                                          type)));
    stream.close();
    return 0;
}

/*
7.21.4 Description of operator ~moconsume~

*/
OperatorSpec moconsumeSpec(
    "stream(Tuple) x (ident1 ... identn) "
    "-> (mem(orel (tuple(x)) (ident1 ... identn)))",
    "_ moconsume[list]",
    "collects the objects from a stream(tuple) into a memory ordered relation",
    "query 'ten' mfeed head[2] moconsume[No]"
);


/*
7.21.5 Instance of operator ~moconsume~

*/
Operator moconsumeOp (
    "moconsume",
    moconsumeSpec.getStr(),
    moconsumeValueMap,
    Operator::SimpleSelect,
    moconsumeTypeMap
);


/*
7.22 Operator ~letmoconsume~ and ~letmoconsumeflob~

  The Operator ~letmoconsume~ creates a new MemoryORelOBject from an 
  input stream. It's name is given in the second argument. The third
  argument provides a list of attribute identifiers over which the relation
  will be sorted. The Operator ~letmoconsumeflob~ additionally loads the 
  flobs of all tuple of the stream into main memory.
  
7.21.1 General Type Mapping Functions of operator ~letmoconsume~
       and ~letmoconsumeflob~

  stream(tuple(x)) x string x (ident1 ... identn) -> 
  mem(orel(tuple(x)) (ident1 ... identn)) 

*/
ListExpr letmoconsumeTypeMap(ListExpr args) {
    if(nl->ListLength(args)!=3){
      return listutils::typeError("(wrong number of arguments)");
    }
    if (!Stream<Tuple>::checkType(nl->First(args)) || 
        !CcString::checkType(nl->Second(args)) ) {
      return listutils::typeError ("stream(Tuple) x string expected!");
    }
    
    ListExpr attrlist = nl->Second(nl->Second(nl->First(args)));
    if(!listutils::checkAttrListForNamingConventions(attrlist)){
      return listutils::typeError("Some of the attributes do "
                   "not fit into Secondo's naming conventions");
    }

    if(!listutils::isKeyDescription(nl->Second(nl->First(args)),
                                    nl->Third(args))) {
      return listutils::typeError("all identifiers of third argument must "
                                  "appear in the first argument");
    }
    
    return nl->TwoElemList(
                  listutils::basicSymbol<Mem>(),
                  nl->ThreeElemList(
                      nl->SymbolAtom(OREL),
                      nl->Second(nl->First(args)),
                      nl->Third(args)));
}

/*
7.22.2 Value Mapping Functions of operator ~letmoconsume~
       and ~letmoconsumeflob~

*/
template<bool flob>
int letmoconsumeValMap(Word* args, Word& result,
                int message, Word& local, Supplier s) {
  
  result  = qp->ResultStorage(s);
  Mem* str = (Mem*)result.addr;

  CcString* oN = (CcString*) args[1].addr;
  if(!oN->IsDefined()){
      str->SetDefined(false);
      return 0;
  }
  
  string res = oN->GetValue();
  if(catalog->isMMObject(res)) {
      str->SetDefined(false);
      return 0;
  }
  
  ListExpr rel = qp->GetType(qp->GetSon(s,0));
  ListExpr attrToSort = qp->GetType(qp->GetSon(s,2));
  MemoryORelObject* mmorel = new MemoryORelObject();
  
  ListExpr attrs = attrToSort;
  ListExpr attr;
 
  while (!(nl->IsEmpty(attrToSort))) {
    attr = nl->First(attrToSort);
    attrToSort = nl->Rest(attrToSort);
    
    ListExpr attrType = 0;
    int attrPos = 0;
    ListExpr attrList = nl->Second(nl->Second(rel));
    
    attrPos = listutils::findAttribute(attrList, nl->ToString(attr), attrType);
    if(attrPos == 0) {
      return listutils::typeError
        ("there is no attribute having name " + nl->ToString(attr));
    }
    mmorel->setAttrPos(attrPos,true);
  }
  
  bool succeed = mmorel->tupleStreamToORel(args[0],
                                           rel, attrs,
                                           getDBname(), flob);
  if(succeed) 
    catalog->insert(res,mmorel);
  else 
    delete mmorel;
  
  str->set(succeed,res);
  return 0;
}


/*
7.22.4 Description of operator ~letmoconsume~

*/
OperatorSpec letmoconsumeSpec(
    "stream(tuple(x)) x string x (ident1 ... identn) -> "
    "mem(orel(tuple(x)) (ident1 ... identn)) ",
    "_ letmoconsume [_;list]",
    "produces a main memory ordered relation from a stream(tuple)",
    "query ten feed head[5] minsert['ten'] letmoconsume['fuenf'; No]"
);

/*
7.22.5 Instance of operator ~letmoconsume~

*/
Operator letmoconsumeOp (
    "letmoconsume",
    letmoconsumeSpec.getStr(),
    letmoconsumeValMap<false>,
    Operator::SimpleSelect,
    letmoconsumeTypeMap
);


/*
7.23.4 Description of operator ~letmoconsumeflob~

*/
OperatorSpec letmoconsumeflobSpec(
    "stream(tuple(x)) x string x (ident1 ... identn) "
    "-> mem(orel(tuple(x)) (ident1 ... identn))",
    "_ letmoconsumeflob [_; list]",
    "produces a main memory ordered relation from a stream(tuple)"
    "and loads the associated flobs",
    "query 'oTrains' mfeed letmoconsumeflob ['moTrains'; Id]"
);


/*
7.23.5 Instance of operator ~letmoconsumeflob~

*/
Operator letmoconsumeflobOp (
    "letmoconsumeflob",
    letmoconsumeflobSpec.getStr(),
    letmoconsumeValMap<true>,
    Operator::SimpleSelect,
    letmoconsumeTypeMap
);



/*
7.24 Operators ~morange~, ~moleftrange~, ~morightrange~
        In case of ~morange~ finds all tuples in the given MemoryORelObject 
        (as first argument) which are between the first and the second 
        attribute value (as second and third argument).
        In case of ~moleftrange~, ~morightrange~ finds all tuples in 
        the given MemoryORelObject (as first argument) which are left or 
        right of the given value (as second argument).

7.24.1 General Type Mapping Functions of operators ~morange~, ~moleftrange~ 
       and ~morightrange~

{string, mem(orel(tuple(x)))}  x T x T -> stream(tuple(x)) or

{string, mem(orel(tuple(x)))}  x T -> stream(tuple(x))

*/
enum RangeKind {
  LeftRange,
  RightRange,
  Range
};

template<RangeKind rk>
ListExpr morangeTypeMap(ListExpr args) {
    // if morange
    if(rk==Range && nl->ListLength(args)!=3){
        return listutils::typeError("three arguments expected");
    }
    // if moleftrange or morightrange
    if((rk==LeftRange || rk==RightRange) && nl->ListLength(args)!=2){
        return listutils::typeError("two arguments expected");
    }

    ListExpr a1 = nl->First(args);      //mem(orel)
    ListExpr a2 = nl->Second(args);
    // if morange
    ListExpr a3 = nl->TheEmptyList();        
    if(rk==Range) {
      a3 = nl->Third(args);
    }

    string err;
    if(rk==Range) {
      err = "{string, mem(orel)} x T x T expected";
    } else {
      err = "{string, mem(orel)} x T expected"; 
    }
  
    string errMsg;
    if(!getMemType(nl->First(a1), nl->Second(a1), a1, errMsg)){
      return listutils::typeError(err + "\n problem in first arg:" + errMsg);
    }
    // remove mem from mem(orel) 
    a1 = nl->Second(a1); 
   
    // process MemoryORelObject
    if(!listutils::isOrelDescription(a1)) {
      return listutils::typeError(err + " (first arg is not an orel)");
    }
    // extract type of key1
    a2 = nl->First(a2);  
    // extract type of key2 if morange
    if(rk==Range) {
      a3 = nl->First(a3);  
    } else {
      a3 = a2;
    }
  
    ListExpr attrList = nl->Second(nl->Second(a1));
    ListExpr attrNames = nl->Third(a1);
    ListExpr typeList;
    ListExpr lastType;
    ListExpr indexList;
    ListExpr lastIndex;
    bool first = true;

    // extract the types from the key attributes and collect their positions
    while(!nl->IsEmpty(attrNames)){
       ListExpr attrName = nl->First(attrNames);
       attrNames = nl->Rest(attrNames);
       if(nl->AtomType(attrName)!=SymbolType){
         return listutils::typeError("error in orel description");
       }
       string name = nl->SymbolValue(attrName);
       ListExpr type;
       int index = listutils::findAttribute(attrList, name, type);
       if(!index){
          return listutils::typeError("invalid orel description");
       }
       if(first){
         typeList = nl->OneElemList(type);
         lastType = typeList;
         indexList = nl->OneElemList( nl->IntAtom(index-1));
         lastIndex = indexList;
         first = false;
       } else {
         lastType = nl->Append(lastType, type);
         lastIndex = nl->Append(lastIndex, nl->IntAtom(index-1));
       }
    }

    if(!nl->Equal(typeList, a2)){
       return listutils::typeError("key attribute type of orel does not fit "
                                   "the boundary attributes");
    }
    if(!nl->Equal(typeList, a2)){
       return listutils::typeError("key attribute type of orel does not fit "
                                   "the max boundary attributes (1)");
    }

    ListExpr resType =  nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                                        nl->Second(a1)); 
    return nl->ThreeElemList(nl->SymbolAtom(Symbols::APPEND()),
                             indexList,
                             resType);
}

class morangeInfo{
    public:
      
      morangeInfo(ttree::TTree<TupleWrap,TupleComp>* _tree, 
                  TupleWrap _min,
                  TupleWrap _max, 
                  vector<int> _attrPos)
          : tree(_tree),max(_max),attrPos(_attrPos) {
        
        if(_min()==0){ // no left boundary, start from beginning
           iter = tree->begin();
        } else {
           iter = tree->tail(_min, &attrPos);
        }
      }
      
      ~morangeInfo(){}


      Tuple* next() {
         if(iter.end()){ // scan finished
            return 0;
         }
         Tuple* result = (*iter).getPointer();
         if(!result){
           return 0;             
         }
         if(max()==0){ // no right boundary 
            result->IncReference();
            iter++;
            return result;
         }
         // check right boundary
         if(TupleComp::smaller(max(), result, &attrPos)){
           return 0;
         } 
         result->IncReference();
         iter++;
         return result;
      }
    
     private:
        ttree::TTree<TupleWrap,TupleComp>* tree;
        TupleWrap max;
        vector<int> attrPos;
        ttree::Iterator<TupleWrap,TupleComp> iter;
};

/*
7.24.2 The Value Mapping Function of operator ~mrange~, ~moleftrange~ and 
       ~morightrange~

*/
template<class T, RangeKind rk>
int morangeVMT (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

   
    morangeInfo* li = (morangeInfo*) local.addr;
      
    switch (message) {
       
      case OPEN: {
        
            if(li){
                delete li;
                local.addr=0;
            }

            T* orelN = (T*) args[0].addr;
            MemoryORelObject* orel = 
                getMemORel(orelN, nl->Second(qp->GetType(s)));
            if(!orel){
              return 0; 
            }
            Tuple* minTuple = 0;
            Tuple* maxTuple = 0;           
            vector<int> positions; 
            // collect minTuple, maxTuple and 
            // positions            
            TupleType* tt = new TupleType(nl->Second(GetTupleResultType(s)));
            int keyPos = rk==Range?3:2;
            for(int i=keyPos;i<qp->GetNoSons(s); i++){
               positions.push_back(((CcInt*)args[i].addr)->GetValue());
            }
            // collect minTuple
            int maxpos = 1;
            if(rk!=LeftRange){ // minValue is required for rightrange and range
               maxpos = 2;
               minTuple = new Tuple(tt);
               Word aw;
               for(size_t i=0;i<positions.size();i++){ 
                  Supplier son = qp->GetSupplier(args[1].addr,i);
                  qp->Request(son,aw);
                  Attribute* attr = (Attribute*) aw.addr;
                  minTuple->PutAttribute(positions[i], attr->Copy());
               }
            }

            // collect maxTuple
            if(rk!=RightRange){ // maxValue is required for leftrange and range
               maxTuple = new Tuple(tt);
               Word aw;
               for(size_t i=0;i<positions.size();i++){ 
                  Supplier son = qp->GetSupplier(args[maxpos].addr,i);
                  qp->Request(son,aw);
                  Attribute* attr = (Attribute*) aw.addr;
                  maxTuple->PutAttribute(positions[i], attr->Copy());
               }
            }
            // the tuple comparator vector starts counting with 1
            for(size_t i=0;i<positions.size();i++){ 
               positions[i]++;
            }  

            local.addr = new morangeInfo(orel->getmmorel(),
                                         minTuple,maxTuple, 
                                         positions); 
            if(minTuple){
               minTuple->DeleteIfAllowed();
            }
            if(maxTuple){
               maxTuple->DeleteIfAllowed();
            }
            tt->DeleteIfAllowed();
            return 0;
        }

        case REQUEST:
            result.addr=li?li->next():0;
            return result.addr?YIELD:CANCEL;

        case CLOSE:
            if(li) {
              delete li;
              local.addr = 0;
            }
            return 0;
   }
   return -1;
}


ValueMapping morangeVM[] = {
    morangeVMT<CcString,Range>,
    morangeVMT<Mem,Range>,
    morangeVMT<CcString,LeftRange>,
    morangeVMT<Mem,LeftRange>,
    morangeVMT<CcString,RightRange>,
    morangeVMT<Mem,RightRange>
};

template<RangeKind rk>
int morangeSelect(ListExpr args){
  if(rk==Range) {
    return CcString::checkType(nl->First(args))?0:1;
  }
  else if(rk==LeftRange) {
    return CcString::checkType(nl->First(args))?2:3;
  }
  else if(rk==RightRange)
    return CcString::checkType(nl->First(args))?4:5;
}

/*
7.24.4 Description of operator ~morange~

*/
OperatorSpec morangeSpec(
    "{string, mem(orel(tuple(x)))}  x T x T -> stream(tuple(x))",
    "_ morange [_,_]",
    "returns all tuples in a main memory ordered relation whose attributes "
    "are lying between the two given values",
    "query 'oten' morange[2,2] count"
);


/*
7.25.4 Description of operator ~moleftrange~

*/
OperatorSpec moleftrangeSpec(
    "{string, mem(orel(tuple(x)))}  x T -> stream(tuple(x))",
    "_ moleftrange [_]",
    "returns all tuples in a main memory ordered relation whose attributes "
    "are smaller than the given value",
    "query 'oten' moleftrange[2] count"
);

/*
7.26.4 Description of operator ~morightrange~

*/
OperatorSpec morightrangeSpec(
    "{string, mem(orel(tuple(x)))}  x T -> stream(tuple(x))",
    "_ morightrange [_]",
    "returns all tuples in a main memory ordered relation whose attributes "
    "are larger than the given value",
    "query 'oten' morightrange[8] count"
);



/*
7.24.5 Instance of operator ~morange~

*/
Operator morangeOp (
    "morange",
    morangeSpec.getStr(),
    6,
    morangeVM,
    morangeSelect<Range>,
    morangeTypeMap<Range>
);

/*
7.25.5 Instance of operator ~moleftrange~

*/
Operator moleftrangeOp (
    "moleftrange",
    moleftrangeSpec.getStr(),
    6,
    morangeVM,
    morangeSelect<LeftRange>,
    morangeTypeMap<LeftRange>
);

/*
7.26.5 Instance of operator ~morightrange~

*/
Operator morightrangeOp (
    "morightrange",
    morightrangeSpec.getStr(),
    6,
    morangeVM,
    morangeSelect<RightRange>,
    morangeTypeMap<RightRange>
);


enum SPType{
  DIJKSTRA,
  ASTAR
};

Tuple* findTuple(ttree::TTree<TupleWrap,TupleComp>* mmorel, 
                 int nr, int prev) {
    
  CcInt* startNodeInt = new CcInt(true,prev);
  CcInt* endNodeInt = new CcInt(true,nr);

  ttree::Iterator<TupleWrap,TupleComp> it = mmorel->begin();
  
  Tuple* tup;
  while(!it.end()) {
    tup = (*it).getPointer();
    if(!tup)
      return 0;
    // find all tuples with startnode as first argument
    if(tup->GetAttribute(0)->Compare(startNodeInt) < 0) 
      it++;
    // find EndNode in second argument
    else if(tup->GetAttribute(1)->Compare(endNodeInt) < 0)
      it++;
    else break;
  }

  startNodeInt->DeleteIfAllowed();
  endNodeInt->DeleteIfAllowed();
  return tup;
}


bool appendTuple(Tuple* tup,TupleType* tt, 
                 int seqNo,
                 ttree::TTree<TupleWrap,TupleComp>* result) {
  Tuple* newTuple = new Tuple(tt);
  int i = 0;
  for(; i<tup->GetNoAttributes(); i++) {
    newTuple->CopyAttribute(i,tup,i);
  }
  int s = seqNo;
  CcInt* noOfNodes = new CcInt(true, s);
  newTuple->PutAttribute(i,noOfNodes);
  TupleWrap tw(newTuple);
  result->insert(tw,i+1);   // sort by SeqNo
  newTuple->DeleteIfAllowed();
  return true;
}

QueueEntryWrap findNextNode(
           ttree::TTree<QueueEntryWrap,EntryComp>* visitedNodes,
           QueueEntryWrap& current) {
    
  ttree::Iterator<QueueEntryWrap,EntryComp> iter = visitedNodes->begin();
  while(!iter.end()) {
    QueueEntryWrap entry = *iter;

    if(entry.getPointer()->nodeNumber == current()->prev) {
      return entry;
    }
    iter++;
  }
  return 0;
}


/*

7.27 Operator ~moshortestpathd~ and ~moshortestpatha~

The Operator ~moshortestpathd~ and ~moshortestpatha~ calculate the shortest
path between two given nodes of a graph. The graph is in this case represented
by a main memory ordered relation. The tuples of such a relation need to
have two integer values as their first two attributes identifying the start and
end node of an edge and at least one double value containing the cost
the edge.
The main memory relation must be ordered first by the start nodes of the tuples
then by the end nodes.
The operator expects an main memory ordered relation, two integers describing
the start and goal node of the path, an integer choosing the resulting output
of the shortest path calculation, as well as an cost function, which maps
the values of the tuples to a double value. The Operator ~moshortestpatha~
expects an additional function calculating the distance to the goal node.
The following results can be selected by integer: 0 shortest path,
1 remaining priority queue at end of computation, 2 visited sections of 
shortest path search, 3 shortest path tree.
For all cases an additional attribute 'seqNo' of type integer will be appended
to the tuples of the output stream.
The Operator ~moshortestpathd~ uses Dijkstras Algorithm and the Operator
~moshortestpatha~ uses the AStar-Algorithm.

7.27.1 General Type Mapping for Operators ~moshortestpathd~ and
       ~moshortestpatha~

{string, mem(orel(tuple(X)))} x int x int x int x (tuple->real)
-> stream(tuple(a1:t1,...an+1:tn+1)) and

{string, mem(orel(tuple(X)))} x int x int x int x (tuple->real) 
x (tuple->real) -> 
stream(tuple(a1:t1,...an+1:tn+1))

*/
template<SPType sptype>
ListExpr moshortestpathTypeMap(ListExpr args) {
  
  if(sptype == DIJKSTRA && nl->ListLength(args) != 5) {
    return listutils::typeError("moshortestpath expects 5 arguments");
  }
  
  if(sptype == ASTAR && nl->ListLength(args) != 6) {
    return listutils::typeError("moshortestpath expects 6 arguments");
  }
 
  ListExpr tmp = args;
  while(!nl->IsEmpty(tmp)){
    if(!nl->HasLength(nl->First(tmp),2)){
      return listutils::typeError("internal error");
    }
    tmp = nl->Rest(tmp);
  }

  // process first arg
  ListExpr a1 = nl->First(args);
  string errMsg;
  if(!getMemType(nl->First(a1), nl->Second(a1), a1, errMsg)){
    return listutils::typeError("string or mem(orel) expected : " + errMsg);
  }
  
  a1 = nl->Second(a1); // remove mem from mem(orel) 
  //MemoryORelObject
  if(!listutils::isOrelDescription(a1)) {
    return listutils::typeError("first arg is not an orel");
  }
  
  ListExpr startNodeList = nl->First(nl->Second(args));
  ListExpr endNodeList = nl->First(nl->Third(args));
  ListExpr resultSelect = nl->First(nl->Fourth(args));
  ListExpr functionWeightMap = nl->First(nl->Fifth(args));
  ListExpr functionDistMap;
  if(sptype == ASTAR){
    functionDistMap = nl->First(nl->Sixth(args));
  }

  ListExpr orelTuple = nl->Second(a1);    

  ListExpr orelAttrList(nl->Second(orelTuple));

  if(!nl->HasMinLength(orelAttrList, 3)){
    return listutils::typeError("Ordered relation must have at "
                                "least 3 attributes");
  }


  ListExpr firstAttr = nl->First(orelAttrList);
  if(!CcInt::checkType(nl->Second(firstAttr))){
    return listutils::typeError("first attribute must be of type int");
  }
  ListExpr secondAttr = nl->Second(orelAttrList);
  if(!CcInt::checkType(nl->Second(secondAttr))){
    return listutils::typeError("second attribute must be of type int");
  }

  //Check of second argument
  if (!CcInt::checkType(startNodeList)) {
    return listutils::typeError("Second argument must be int");
  }

  //Check of third argument
  if (!CcInt::checkType(endNodeList)) {
    return listutils::typeError("Third argument should be int");
  }

  //Check of fourth argument
  if(!CcInt::checkType(resultSelect) ) {
    return listutils::typeError("Fourth argument should be int");
  }

  //Check of fifth argument
  if(!listutils::isMap<1>(functionWeightMap)) {
    return listutils::typeError("Fifth argument should be a map");
  }

  ListExpr mapTuple = nl->Second(functionWeightMap);

  if(!nl->Equal(orelTuple,mapTuple)) {
    return listutils::typeError("Tuple of map function must match orel tuple");
  }

  ListExpr mapres = nl->Third(functionWeightMap);

  if(!CcReal::checkType(mapres)) {
    return listutils::typeError(
                "Wrong mapping result type for oshortestpathd");
  }
  
  // check of sixth argument if ASTAR
  if(sptype == ASTAR) {
    if (!listutils::isMap<1>(functionDistMap)) {
      return listutils::typeError(
                      "Sixth argument must be a map");
    }
    ListExpr mapTuple2 = nl->Second(functionDistMap);

    if (!nl->Equal(orelTuple,mapTuple2)) {
      return listutils::typeError(
                       "Tuple of map function must match orel tuple");
    }
    ListExpr mapres2 = nl->Third(functionDistMap);

    if(!CcReal::checkType(mapres2)) {
      return listutils::typeError(
                            "Wrong mapping result type for oshortestpath");
    }
  }

  // appends Attribute SeqNo to Attributes in orel
  NList extendAttrList(nl->TwoElemList(nl->SymbolAtom("SeqNo"),
                                       nl->SymbolAtom(CcInt::BasicType())));
  NList extOrelAttrList(nl->TheEmptyList());


  for(int i = 0; i < nl->ListLength(orelAttrList); i++) {
    NList attr(nl->Nth(i+1,orelAttrList));
    extOrelAttrList.append(attr);
  }

  extOrelAttrList.append(extendAttrList);

  ListExpr outlist = nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                                     nl->TwoElemList(
                                       nl->SymbolAtom(Tuple::BasicType()),
                                       extOrelAttrList.listExpr()));
  return outlist;
}


class moshortestpathdInfo {
public:
  
  moshortestpathdInfo(ttree::TTree<TupleWrap,TupleComp>* _tree,  
                     int _startNode, int _endNode, int _resultSelect,
                     Word _arg, TupleType* _tt) 
      : mmorel(_tree),startNode(_startNode),
        endNode(_endNode),resultSelect(_resultSelect),
        arg(_arg),tt(_tt),seqNo(1) {
          
    queue = new Queue();
    visitedNodes = new ttree::TTree<QueueEntryWrap,EntryComp>(16,18);
    sptree = new ttree::TTree<TupleWrap,TupleComp>(16,18);
    result = new ttree::TTree<TupleWrap,TupleComp>(16,18);
    
    bool found = shortestPath();
    if(resultSelect<0 || resultSelect>3){ // bring to allowed values
       resultSelect = 0;
    }
    if(found || resultSelect == 3) {
      resultSelection();
    } else  {
      cerr << "no path found" << endl;
    }
    iterator = result->begin();
  }

  ~moshortestpathdInfo() {
    while(!queue->empty())
      queue->pop();
    delete queue;
    
    tt->DeleteIfAllowed();
    tt = 0;
    
    delete visitedNodes;
    delete result;
    delete sptree;
  };
  
  Tuple* next() {
    if(iterator.end()) 
      return 0;
    Tuple* res = (*iterator).getPointer();
    if(res==NULL) {
      return 0;
    }
    iterator++;
    res->IncReference();
    return res;
  }
  
  
/*
~shortestpath~
 
*/
  bool shortestPath() {
  
  // INIT
    int toNode = startNode;
    
    QueueEntry* startEntry = new QueueEntry(startNode,-1,0.0,0.0);
    QueueEntryWrap qw(startEntry);
    startEntry->DeleteIfAllowed();
    queue->push(qw);
    visitedNodes->insert(qw);
    
    double dist = 0.0;
    
  // SEARCH SHORTESTPATH
    while(!queue->empty()) {
      current = queue->top();
      current()->visited = true;
      queue->pop();
      
      // goal node found
      if (resultSelect<3 && current()->nodeNumber == endNode) {
        return true;
      }
      // process next node
      else {
        CcInt* currentNodeNumber = new CcInt(true,current()->nodeNumber);
        // get tuple with currentNodeNumber as startnode
        ttree::Iterator<TupleWrap,TupleComp> it = mmorel->begin();
        while(!it.end()) {
          Tuple* tup = (*it).getPointer();
          if(tup->GetAttribute(0)->Compare(currentNodeNumber) < 0) {
            it++;
          }
          // tuple found
          else break;
        }
        // process edges
        while(!it.end()) {
          
          Tuple* currentTuple = (*it).getPointer();
          // all edges processed
          if(currentTuple->GetAttribute(0)->Compare(currentNodeNumber) > 0) {
            break;
          }
          if(resultSelect!=3) {
            TupleWrap tw(currentTuple);
            sptree->insert(tw);   
          }
          toNode = ((CcInt*)currentTuple->GetAttribute(1))->GetIntval();
          
          if(current()->nodeNumber != toNode) {
            ArgVectorPointer funArgs = qp->Argument(arg.addr);
            Word funResult;
            ((*funArgs)[0]).setAddr(currentTuple);
            qp->Request(arg.addr,funResult);
            double edgeCost = ((CcReal*)funResult.addr)->GetRealval();
            if (edgeCost < 0.0) {
              cerr << "Found negativ edge cost computation aborted." << endl;
              return 0;
            }
            dist = current()->dist + edgeCost;
            bool contained = false;
            ttree::Iterator<QueueEntryWrap,EntryComp> it = 
                                            visitedNodes->begin();
            
            // check if shortening of path possible
            while(!it.end()) {
              QueueEntryWrap entry = *it;
              if(entry.getPointer()->nodeNumber == toNode) {
                if(entry.getPointer()->dist > dist) {
                  QueueEntry* prevEntry = new QueueEntry(entry()->nodeNumber,
                                                          entry()->prev,
                                                          entry()->dist,
                                                          entry()->priority);
                  prevEntry->visited = entry()->visited;
                  entry()->dist = dist;
                  entry()->prev = current()->nodeNumber;
                  entry()->priority = dist;
                  if(entry()->visited){
                    QueueEntryWrap  qw(prevEntry);
                    visitedNodes->update(entry,qw);
                  }
                  prevEntry->DeleteIfAllowed();
                }
                contained = true;
                break;
              }
              it++;
            }

            if(!contained) {
              QueueEntry* to = 
                  new QueueEntry(toNode,current()->nodeNumber,dist,dist);
              QueueEntryWrap qw(to);
              to->DeleteIfAllowed();
              visitedNodes->insert(qw,true);
              queue->push(qw);
              if(resultSelect==3){
                TupleWrap tw(currentTuple);
                sptree->insert(tw);
              }
              currentTuple->IncReference();
            }
          }
          it++;
        } 
        currentNodeNumber->DeleteIfAllowed();
      }
    } // end while search shortestpath
    return false;
  }
  
  
  void resultSelection() {
    switch(resultSelect) {
      case 0: {  // shortest path  
      
        Tuple* currentTuple = 
                   findTuple(mmorel,current()->nodeNumber,current()->prev);
        bool found = false;
    
        while (!found && currentTuple != 0) {

          appendTuple(currentTuple,tt,seqNo,result);
          seqNo++;
          
          if(current()->prev != startNode) {
            current = findNextNode(visitedNodes,current);

            currentTuple = 
                    findTuple(mmorel,current()->nodeNumber,current()->prev);
          }
          else {
            found = true;
            if(currentTuple != 0) {
              currentTuple->DeleteIfAllowed();
              currentTuple = 0;
            }
          }
        }
        break;
      }
      case 1: { //Remaining elements in priority queue
        while(queue->size() > 0) {
          current = queue->top();
          queue->pop();
          Tuple* currentTuple = findTuple(mmorel,
                                          current()->nodeNumber,
                                          current()->prev);
          appendTuple(currentTuple,tt,seqNo,result);
          seqNo++;
        }
        break;
      }
      case 2: { //visited sections
        ttree::Iterator<TupleWrap,TupleComp> iter = sptree->begin();
         while(!iter.end()) {
           Tuple* currentTuple = (*iter).getPointer();
           appendTuple(currentTuple,tt,seqNo,result);
           iter++;
           seqNo++;
         }
         break;
      }
      case 3: { //shortest path tree     
        ttree::Iterator<TupleWrap,TupleComp> iter = sptree->begin();
        while(!iter.end()) {
          Tuple* currentTuple = (*iter).getPointer();
          appendTuple(currentTuple,tt,seqNo,result);
          iter++;
          seqNo++;
        }
        break;
      }

      default: { //should have never been reached
        break;
      }
    }
      
  }


protected:

  ttree::TTree<TupleWrap,TupleComp>* mmorel;     
  ttree::Iterator<TupleWrap,TupleComp> iterator;
  int startNode;
  int endNode;
  int resultSelect;
  Word arg; 
  TupleType* tt;
  int seqNo;
  Queue* queue;
  
  ttree::TTree<QueueEntryWrap,EntryComp>* visitedNodes;
  ttree::TTree<TupleWrap,TupleComp>* sptree;
  ttree::TTree<TupleWrap,TupleComp>* result;
  
  
  QueueEntryWrap current;
  int seqNoAttrIndex;
  
};



/*

7.27.2 Value Mapping Function of operator ~moshortestpathd~

*/
template<class T>
int moshortestpathdValueMap(Word* args, Word& result, int message,
                          Word& local, Supplier s) {

  moshortestpathdInfo* li = (moshortestpathdInfo*) local.addr;

  switch(message) {
    
    case OPEN: {
      
      if(li) {
        delete li;
        local.addr=0;
      }

      int resultSelect = ((CcInt*)args[3].addr)->GetIntval();
      if(resultSelect<0 || resultSelect>3) {
        cerr << "Selected result value does not exist. Enter 0 for shortest "
        << "path, 1 for remaining priority queue elements, 2 for visited "
        << "edges, 3 for shortest path tree." << endl;
        return 0;
      }

      // Check for simplest Case
      int startNode = ((CcInt*)args[1].addr)->GetIntval();
      int endNode = ((CcInt*)args[2].addr)->GetIntval();
      if(resultSelect<3) {
        if(startNode == endNode) {
          cerr << "source and target node are equal, no path";
          return 0;
        }
      }

      ListExpr tupleType = GetTupleResultType(s);
      TupleType* tt = new TupleType(nl->Second(tupleType));

      T* orelN = (T*) args[0].addr;
      MemoryORelObject* orel = getMemORel(orelN, nl->Second(qp->GetType(s)));
      if(!orel){
        return 0; 
      }

      local.addr = new moshortestpathdInfo(orel->getmmorel(),
                                          startNode, 
                                          endNode, 
                                          resultSelect, 
                                          args[4], tt);
      return 0;
    }

    case REQUEST: 
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;
    
    case CLOSE: 
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
  }
  return 0;
}


ValueMapping moshortestpathdVM[] = {
    moshortestpathdValueMap<CcString>,
    moshortestpathdValueMap<Mem>
};


int moshortestpathSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}


/*
7.27.4 Specification of operator ~moshortestpathd~

*/
OperatorSpec moshortestpathdSpec(
    "{string, mem(orel(tuple(x)))} x int x int x int x "
    "(tuple->real) -> stream(tuple(a1:t1,...an+1:tn+1))",
    "morel moshortestpathd [startNode,endNode,resultSelect; fun] ",
    "Caculates the shortest path from startNode to endNode in a graph "
    "representent in a ordered memory relation using dijkstras algorithm."
    "The ordered relation must have SourceNodes as the first attribute and "
    "target nodes as second attribute. " 
    "Using the argument resultselect, the output of this operator is "
    "controlled. 0 : shortest path, 1 : remaining elements in queue, "
    " 2 : visited sections, 3 : shortest path tree ",
    "query mwrap('otestrel') moshortestpathd"
    "[1,3,0; distance(.GeoData_s1,.GeoData_s2)] count"
);

/*
7.27.4 Instance of operator ~moshortestpathd~

*/
Operator moshortestpathdOp (
    "moshortestpathd",
    moshortestpathdSpec.getStr(),
    2,
    moshortestpathdVM,
    moshortestpathSelect,
    moshortestpathTypeMap<DIJKSTRA>
);


class moshortestpathaInfo {
public:
  
  moshortestpathaInfo(ttree::TTree<TupleWrap,TupleComp>* _tree,  
                     int _startNode, int _endNode, int _resultSelect,
                     Word _arg, Word _arg2, TupleType* _tt) 
      : mmorel(_tree),startNode(_startNode),
        endNode(_endNode),resultSelect(_resultSelect),
        arg(_arg),arg2(_arg2),tt(_tt),seqNo(1) {
          
          queue = new Queue();
          visitedNodes = new ttree::TTree<QueueEntryWrap,EntryComp>(16,18);
          sptree = new ttree::TTree<TupleWrap,TupleComp>(16,18);
          result = new ttree::TTree<TupleWrap,TupleComp>(16,18);
          
          bool found = shortestPath();
          if(found || resultSelect == 3) {
            resultSelection();
          }
          else  
            cerr << "no path found" << endl;
          
          iterator = result->begin();
        }

  ~moshortestpathaInfo() {
    while(!queue->empty())
      queue->pop();
    delete queue;
    
    tt->DeleteIfAllowed();
    tt = 0;
    
    delete visitedNodes;
    delete result;
    delete sptree;
  };
  
  Tuple* next() {
    if(iterator.end()) 
      return 0;
    Tuple* res = (*iterator).getPointer();
    if(res==NULL) {
      return 0;
    }
    iterator++;
    res->IncReference();
    return res;
  }
  
  
/*
~shortestpath~
 
*/
  bool shortestPath() {
  
  // INIT
    int toNode = startNode;
    
    QueueEntry* startEntry = new QueueEntry(startNode,-1,0.0,0.0);
    QueueEntryWrap qw(startEntry);
    queue->push(qw);
    visitedNodes->insert(qw);
    startEntry->DeleteIfAllowed();
    
    double dist = 0.0;
    
  // SEARCH SHORTESTPATH
    while(!queue->empty()) {
      current = queue->top();
      current()->visited = true;
      queue->pop();
      
      if (resultSelect<3 && current()->nodeNumber == endNode) {
        return true;
      }
      else {
        CcInt* currentNodeNumber = new CcInt(true,current()->nodeNumber);
        
        ttree::Iterator<TupleWrap,TupleComp> it = mmorel->begin();
        while(!it.end()) {
          Tuple* tup = (*it).getPointer();
          // get tuple with currentNodeNumber as startnode
          if(tup->GetAttribute(0)->Compare(currentNodeNumber) < 0) {
            it++;
          }
          else break;
        }
        
        while(!it.end()) {
          Tuple* currentTuple = (*it).getPointer();            
          if(currentTuple->GetAttribute(0)->Compare(currentNodeNumber) > 0) {
            break;
          }
          toNode = ((CcInt*)currentTuple->GetAttribute(1))->GetIntval();
          if(resultSelect!=3) {
            TupleWrap tw(currentTuple); 
            sptree->insert(tw);      
          }
          if(current()->nodeNumber != toNode) {
            ArgVectorPointer funArgs = qp->Argument(arg.addr);
            Word funResult;
            ((*funArgs)[0]).setAddr(currentTuple);
            qp->Request(arg.addr,funResult);
            double edgeCost = ((CcReal*)funResult.addr)->GetRealval();
            if (edgeCost < 0.0) {
              cerr << "Found negativ edge cost computation aborted." << endl;
              return 0;
            }
            dist = current()->dist + edgeCost;
            
            ArgVectorPointer funArgs2 = qp->Argument(arg2.addr);
            Word funResult2;
            ((*funArgs2)[0]).setAddr(currentTuple);
            qp->Request(arg2.addr,funResult2);
            double restCost = ((CcReal*)funResult2.addr)->GetRealval();
            if (restCost < 0) restCost = 0;
            double prio = dist + restCost;
            
            bool contained = false;
            ttree::Iterator<QueueEntryWrap,EntryComp> it = 
                                                      visitedNodes->begin();
            while(!it.end()) {
              QueueEntryWrap entry = *it;
              // found node before
              if(entry()->nodeNumber == toNode) {
                if(entry()->priority > prio) {
                  QueueEntry* prevEntry = new QueueEntry(entry()->nodeNumber,
                                                          entry()->prev,
                                                          entry()->dist,
                                                          entry()->priority);
                  prevEntry->visited = entry()->visited;
                  entry()->prev = current()->nodeNumber;
                  entry()->priority = prio;
                  entry()->dist = dist;
                  if(entry()->visited){
                    QueueEntryWrap qw(prevEntry);
                    visitedNodes->update(entry,qw);
                  }
                  prevEntry->DeleteIfAllowed();
                }
                contained = true;
                break;
              }
              it++;
            }

            if(!contained) {
              QueueEntry* to = 
                  new QueueEntry(toNode,current()->nodeNumber,dist,prio);
              QueueEntryWrap qw(to);
              visitedNodes->insert(qw,true);
              to->DeleteIfAllowed();
              queue->push(qw);
              if(resultSelect==3) {
                TupleWrap tw(currentTuple);
                sptree->insert(tw);
              }
              currentTuple->IncReference();
            }
          }
          it++;
        } 
        currentNodeNumber->DeleteIfAllowed();
      }
    } // end while search shortestpath
    return false;
  }
  
  
  void resultSelection() {
    switch(resultSelect) {
      case 0: {  // shortest path  
        Tuple* currentTuple = findTuple(mmorel,
                                        current()->nodeNumber,
                                        current()->prev);
        bool found = false;
    
        int i=0;
        while (!found && currentTuple != 0) {
          
          i++;
          appendTuple(currentTuple,tt,seqNo,result);
          seqNo++;
          
          if(current()->prev != startNode) {
            current = findNextNode(visitedNodes,current);
            currentTuple = findTuple(mmorel,
                                     current()->nodeNumber,
                                     current()->prev);
          }
          else {
            found = true;
            if(currentTuple != 0) {
              currentTuple->DeleteIfAllowed();
              currentTuple = 0;
            }
          }
        }
        break;
      }
      case 1: { //Remaining elements in priority queue
        while(queue->size() > 0) {
          current = queue->top();
          queue->pop();
          Tuple* currentTuple = findTuple(mmorel,
                                          current()->nodeNumber,
                                          current()->prev);
          appendTuple(currentTuple,tt,seqNo,result);
          seqNo++;
        }
        break;
      }
      case 2: { //visited sections
        ttree::Iterator<TupleWrap,TupleComp> it = sptree->begin();
        while(!it.end()) {
          Tuple* currentTuple = (*it).getPointer();;
          appendTuple(currentTuple,tt,seqNo,result);
          seqNo++;
          it++;
        }
        break;
      }
      case 3: { //shortest path tree     
        ttree::Iterator<TupleWrap,TupleComp> iter = sptree->begin();
        while(!iter.end()) {
          Tuple* currentTuple = (*iter).getPointer();
          appendTuple(currentTuple,tt,seqNo,result);
          iter++;
          seqNo++;
        }
        break;
      }

      default: { //should have never been reached
        break;
      }
    }
      
  }


protected:

  ttree::TTree<TupleWrap,TupleComp>* mmorel;     
  ttree::Iterator<TupleWrap,TupleComp> iterator;
  int startNode;
  int endNode;
  int resultSelect;
  Word arg; 
  Word arg2;
  TupleType* tt;
  int seqNo;
  Queue* queue;
  
  ttree::TTree<QueueEntryWrap,EntryComp>* visitedNodes;
  ttree::TTree<TupleWrap,TupleComp>* sptree;
  ttree::TTree<TupleWrap,TupleComp>* result;


  QueueEntryWrap current;
  int seqNoAttrIndex;
  
};



/*

7.28.2  Value Mapping Functions of operator ~moshortestpatha~

*/
template<class T>
int moshortestpathaValueMap(Word* args, Word& result, int message,
                          Word& local, Supplier s) {

  moshortestpathaInfo* li = (moshortestpathaInfo*) local.addr;

  switch(message) {

    case OPEN: {

      if(li) {
        delete li;
        local.addr=0;
      }

      int resultSelect = ((CcInt*) args[3].addr)->GetIntval();
      if(resultSelect<0 || resultSelect>3) {
       cerr << "Selected result value does not exist. Enter 0 for shortest "
       << "path, 1 for remaining priority queue elements, 2 for visited "
       << "edges, 3 for shortest path tree." << endl;
        return 0;
      }

      // Check for simplest Case
      int startNode = ((CcInt*)args[1].addr)->GetIntval();
      int endNode = ((CcInt*)args[2].addr)->GetIntval();
      if(resultSelect<3) {
        if(startNode == endNode) {
          cerr << "source and target node are equal, no path";
          return 0;
        }
      }

      ListExpr tupleType = GetTupleResultType(s);
      TupleType* tt = new TupleType(nl->Second(tupleType));

      T* orelN = (T*) args[0].addr;
      MemoryORelObject* orel = getMemORel(orelN, nl->Second(qp->GetType(s)));
      if(!orel){
        return 0;
      }

      local.addr = new moshortestpathaInfo(orel->getmmorel(),
                                          startNode,
                                          endNode,
                                          resultSelect,
                                          args[4], args[5], tt);
      return 0;
    }

    case REQUEST:
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;

    case CLOSE:
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
  }
  return 0;
}


ValueMapping moshortestpathaVM[] = {
    moshortestpathaValueMap<CcString>,
    moshortestpathaValueMap<Mem>
};

/*

7.28.4 Description of operator ~moshortestpatha~

*/
OperatorSpec moshortestpathaSpec(
    "{string, mem(orel(tuple(x)))} x int x int x int x "
    "(tuple->real) x (tuple->real)-> stream(tuple(a1:t1,...an+1:tn+1))",
    "_ moshortestpatha [_,_,_; fun,fun] implicit parameter tuple type MTUPLE",
    "calculates the shortest path for a given start and goal node in a main "
    "memory ordered relation using the astar algorithm",
    "query mwrap('otestrel') moshortestpatha [1,3,0; "
    "distance(.GeoData_s1,.GeoData_s2), "
    "distance(.GeoData_s1,.GeoData_s2) * 0.0] count"
);

/*

7.28.5 Instance of operator ~moshortestpatha~

*/
Operator moshortestpathaOp (
    "moshortestpatha",
    moshortestpathaSpec.getStr(),
    2,
    moshortestpathaVM,
    moshortestpathSelect,
    moshortestpathTypeMap<ASTAR>
);




/*
7.30 Operator ~moconnectedcomponents~

The operator ~moconnectedcomponents~ calculates the strongly connected
components in a graph given here as a main memory ordered relation.
The operator expects a main memory ordered relation with its tuples
containing at least two integers. They have to be ordered first by the start
node of each edge and secondly bey the end node.

The output is a tuple stream containing all tuples of the main memory
ordered relation with an additional attribute 'compNo' of type integer.


7.30.1 Type Mapping Funktion for operator ~moconnectedcomponents~

{string, mem(orel(tuple(x)))} -> stream(tuple(x@[compNo:int]))

*/
ListExpr moconnectedcomponentsTypeMap(ListExpr args) {

    if(nl->ListLength(args) != 1) {
        return listutils::typeError("one arguments expected");
    }

    ListExpr arg = nl->First(args);

    if(!nl->HasLength(arg,2)){
        return listutils::typeError("internal error");
    }

    string errMsg;

    if(!getMemType(nl->First(arg), nl->Second(arg), arg, errMsg)){
      return listutils::typeError("string or mem(rel) expected : " + errMsg);
    }

    arg = nl->Second(arg); // remove mem
    if(!listutils::isOrelDescription(arg)){
      return listutils::typeError("memory object is not a relation");
    }


    ListExpr rest = nl->Second(nl->Second(arg));
    ListExpr listn = nl->OneElemList(nl->First(rest));
    ListExpr lastlistn = listn;
    rest = nl->Rest(rest);
    while (!(nl->IsEmpty(rest))) {
      lastlistn = nl->Append(lastlistn,nl->First(rest));
      rest = nl->Rest(rest);
    }

    lastlistn = nl->Append(lastlistn,
                          nl->TwoElemList(
                            nl->SymbolAtom("CompNo"),
                            listutils::basicSymbol<CcInt>()));

    ListExpr outList = nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                                       nl->TwoElemList(
                                         nl->SymbolAtom(Tuple::BasicType()),
                                         listn));
    return outList;
}

class Vertex {
public:
  Vertex(int _nr)
    : nr(_nr), inStack(false) {}
  
  size_t operator()(const Vertex &v) const {
    return v.nr;
  }
  
  void setIndex(int _index) {
    index = _index; 
  }
  
  void setLowlink(int _lowlink) {
    lowlink = _lowlink; 
  }
 
  int nr;
  int index;
  int lowlink;
  int compNo;
  bool inStack;
};

class VertexComp {
public:
  bool operator()(const Vertex& v1, const Vertex& v2) const {
    return v1.nr < v2.nr;
  }
};

// geaendert 9.9.2016
class moconnectedComponentsInfo{
  public:
    moconnectedComponentsInfo(ttree::TTree<TupleWrap,TupleComp>* _tree,
                              ListExpr _tt)
        : tree(_tree) {
      tt = new TupleType(_tt);

      scctree = new ttree::TTree<TupleWrap,TupleComp>(16,18);
      nodes = new set<Vertex,VertexComp>();
      scc(tree);
      appendTuples();
      iter = scctree->begin();
    }


    ~moconnectedComponentsInfo(){
      delete scctree;
      //set<Vertex,VertexComp>::iterator it = nodes->begin();
      delete nodes;
      tt->DeleteIfAllowed();
    }


    Tuple* next() {
      if(iter.end()) 
        return 0;
      Tuple* res = (*iter).getPointer();
      if(!res) {
        return 0;
      }
      iter++;
      res->IncReference();
      return res;
    }

    // appends all tuples including their component number to scctree
    bool appendTuples() {
      set<Vertex,VertexComp>::iterator iter;
      ttree::Iterator<TupleWrap,TupleComp> it = tree->begin();
      while(!it.end()) {
        Tuple* t = (*it).getPointer();
        Vertex v(((CcInt*)t->GetAttribute(1))->GetIntval());
        iter = nodes->find(v);
        if(iter != nodes->end()) {
          v = *iter;
          Tuple* newTuple = new Tuple(tt);
          for(int i=0; i<t->GetNoAttributes(); i++) {
            newTuple->CopyAttribute(i,t,i);
          }
          CcInt* noOfNodes = new CcInt(true, v.compNo);
          newTuple->PutAttribute(t->GetNoAttributes(),noOfNodes);
          TupleWrap tw(newTuple);
          scctree->insert(tw,t->GetNoAttributes());   // sort by CompNo
          newTuple->DeleteIfAllowed();
        }
        else return false; 
        it++;
      }
      return true;
    }


    void scc(ttree::TTree<TupleWrap,TupleComp>* tree) {
      int compNo = 1;
      int index = 0;
      ttree::Iterator<TupleWrap,TupleComp> it = tree->begin();
      std::stack<Vertex>* stack = new std::stack<Vertex>(); 
      // process first node
      Tuple* tuple = (*it).getPointer();
      scc(tuple,index,stack,compNo);  

      // process nodes
      while(!it.end()) {
        // find next node
        Tuple* t = (*it).getPointer();
        while(t->GetAttribute(0)->Compare(tuple->GetAttribute(0)) == 0) {
          it++;
          if(!it.end()) {
            tuple = (*it).getPointer();
          }
          // all nodes processed
          else {
            delete stack;
            return;
          }
        }
        // find scc for node
        scc(tuple,index,stack,compNo);
      }

    }

    // caculates the scc for each node
    void scc(Tuple* tuple, int& index, 
             std::stack<Vertex>* stack, int& compNo) {

      set<Vertex,VertexComp>::iterator iter;
      Vertex v(((CcInt*)tuple->GetAttribute(0))->GetIntval());
      iter = nodes->find(v);
      
      // v already seen
      if(iter != nodes->end()) {
        return;
      }

      // v not yet seen
      v.index = index;
      v.lowlink = index;
      index++;
      stack->push(v);
      v.inStack = true;
      nodes->insert(v);
      ttree::Iterator<TupleWrap,TupleComp> it = tree->begin();
      Tuple* t = (*it).getPointer();
      
      // find node in orel
      while(t->GetAttribute(0)->Compare(tuple->GetAttribute(0)) < 0) {
        it++;
        if(!it.end())
          t = (*it).getPointer(); 
        else return;
      }

      // while node has adjacent nodes
      while(((CcInt*)t->GetAttribute(0))->GetIntval() == v.nr) {
        Vertex w(((CcInt*)t->GetAttribute(1))->GetIntval());
        iter = nodes->find(w);
        // w not seen yet
        if(iter == nodes->end()) { 
          it = tree->begin();
          t = (*it).getPointer();
          while(((CcInt*)t->GetAttribute(0))->GetIntval() != w.nr) {
            it++;
            if(!it.end())
              t = (*it).getPointer();     
            // no adjacent nodes for this node
            else {
              t = tuple;
              w.index = index;
              w.lowlink = index;
              w.compNo = compNo;
              compNo++;
              index++;
              nodes->insert(w);
              break;
            }
          }
          // recursive call
          scc(t,index,stack,compNo);   
          iter = nodes->find(w);
          w = *iter;
          v.setLowlink(min(v.lowlink,w.lowlink));
          // find next adjacent edge for v
          it = tree->begin();
          t = (*it).getPointer();
          while(t->GetAttribute(0)->Compare(tuple->GetAttribute(0)) < 0) {
            it++;
            if(!it.end())
              t = (*it).getPointer(); 
            else return;
          }
        }
        // w already seen
        else {
          w = *iter;
          if(w.inStack) {
            v.setLowlink(min(v.lowlink,w.index));  
          }
        }
        it++;
        if(!it.end())
          t = (*it).getPointer();
        else break;
      }
      
      // root of scc found
      if (v.lowlink == v.index) { 
        v.compNo = compNo;
        while(true) {
          Vertex w = stack->top();
          stack->pop();
          w.inStack = false;
          if(v.nr == w.nr)
            break;
          w.compNo = compNo;
        }
        compNo++;
      }
    }

  private:
     ttree::TTree<TupleWrap,TupleComp>* tree;
     ttree::TTree<TupleWrap,TupleComp>* scctree;
     set<Vertex,VertexComp>* nodes;
     ttree::Iterator<TupleWrap,TupleComp> iter;
     TupleType* tt;
};


/*

7.30.2 Value Mapping Functions of operator ~moconnectedcomponents~

*/
template<class T>
int moconnectedcomponentsValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

  moconnectedComponentsInfo* li = (moconnectedComponentsInfo*) local.addr;

  switch (message) {
    case OPEN: {
      
      if(li){
        delete li;
        local.addr=0;
      }
      T* oN = (T*) args[0].addr;

      ListExpr ttype = nl->Second(nl->Second(qp->GetType(s)));

      MemoryORelObject* orel = getMemORel(oN,ttype);
      if(!orel){
        return listutils::typeError
          ("could not find memory orel object");
      }

      ListExpr tupleType = GetTupleResultType(s);
      ListExpr tt = nl->Second(tupleType);

      local.addr= new moconnectedComponentsInfo(orel->getmmorel(),tt);
      return 0;
    }

    case REQUEST:
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;

    case CLOSE:
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
  }

  return -1;
}


ValueMapping moconnectedcomponentsVM[] = {
  moconnectedcomponentsValMap<CcString>,
  moconnectedcomponentsValMap<Mem>
};

int moconnectedcomponentsSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}


/*

7.30.4 Description of operator ~moconnectedcomponents~

*/
OperatorSpec moconnectedcomponentsSpec(
    "{string, mem(orel(tuple(x))}  -> stream(Tuple)",
    "_ moconnectedcomponents",
    "",
    "query \"otestrel\" moconnectedcomponents"
);

/*

7.30.5 Instance of operator ~moconnectedcomponents~

*/
Operator moconnectedcomponentsOp (
    "moconnectedcomponents",
    moconnectedcomponentsSpec.getStr(),
    2,
    moconnectedcomponentsVM,
    moconnectedcomponentsSelect,
    moconnectedcomponentsTypeMap
);


/*
7.31 Operators ~mquicksort~ and ~mquicksortby~

The Operator ~mquicksort~ sorts all tuples of a main memory relation
over all of their attributes, as long as they are atomic. It returns 
the tuples with an additional attribut of type 'tid' in an output stream.

The Operator ~mquicksortby~ sorts all the elements in a main memory relation
by one or more given attributes and returns a stream of the sorted tuples 
and their original tupleid's.

7.31.1 Type Mapping Functions of operator ~mquicksort~ and ~mquicksortby~

{string, mem(rel(tuple(x)))} -> stream(tuple(x)) or 
{string, mem(rel(tuple(x)))} x (ident1 ... identn) -> stream(tuple(x))

*/
enum SORTTYPE{
  Sort,
  SortBy
};

template<SORTTYPE st>
ListExpr mquicksortTypeMap(ListExpr args) {

  if(st == Sort) {
    if(nl->ListLength(args) != 1) 
        return listutils::typeError("one argument expected");
  }
  if(st == SortBy) {
    if(nl->ListLength(args) != 2) 
        return listutils::typeError("two arguments expected");
  }

  // check relation
  ListExpr first = nl->First(args);    
  if(!nl->HasLength(first,2)){
      return listutils::typeError("internal error");
  }
  string errMsg;
  if(!getMemType(nl->First(first), nl->Second(first), first, errMsg)){
    return listutils::typeError("string or mem(rel) expected : " + errMsg);
  }
  first = nl->Second(first); // remove mem
  if(!Relation::checkType(first)){
    return listutils::typeError("memory object is not a relation");
  }

    // check if second argument attribute of relation
  if(st == SortBy) {
    int numberOfSortAttrs = nl->ListLength(nl->Second(args));
    if(numberOfSortAttrs < 0){
      return listutils::typeError("Attribute list may not be empty!");
    }

    if(!listutils::isKeyDescription(nl->Second(first),
                                    nl->First(nl->Second(args)))) {
      return listutils::typeError("all identifiers of second argument must "
                                  "appear in the first argument");
    }
  }

  return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                         nl->Second(first));
}


/*

7.31.2  The Value Mapping Functions of operators ~mquicksort~ and 
        ~mquicksortby~
        
*/
class mquicksortInfo{
  public:
    mquicksortInfo(vector<Tuple*>* _relation, vector<int> _pos) 
    : relation(_relation), pos(_pos) {
        
      quicksort(0,relation->size()-1,pos);
      it = relation->begin();
    }
    
    ~mquicksortInfo(){}
    
    void quicksort(int left, int right, vector<int> pos) {
      
      int i = left, j = right;
      Tuple* tmp;
      Tuple* pivot = relation->at(((left + right) / 2)); 
            
      /* partition */
      while (i <= j) {
        
        while(TupleComp::smaller(relation->at(i),pivot,&pos))
          i++;
        while (TupleComp::greater(relation->at(j),pivot,&pos))
          j--;
        
        if (i <= j) {
          tmp = relation->at(i);
          relation->at(i) = relation->at(j);
          relation->at(j) = tmp;
          
          i++;
          j--;
        }
      }      
      /* recursion */
      if (left < j)
        quicksort(left, j, pos);
      if (i < right)
        quicksort(i, right, pos);
    }

    
    Tuple* next() {
      while(it != relation->end()){ 
         Tuple* res = *it;
         it++;
         if(res) {
           res->IncReference();
           return res;
         }
      }
      return 0;
    }

  private:
     vector<Tuple*>* relation;
     vector<int> pos;
     vector<Tuple*>::iterator it;
};

/*

7.31.2 Value Mapping Function of operators ~mquicksort~ and ~mquicksortby~

*/
template<class T, SORTTYPE st>
int mquicksortValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

  mquicksortInfo* li = (mquicksortInfo*) local.addr;

  switch (message) {
    case OPEN: {
      
      if(li){
        delete li;
        local.addr=0;
      }
      vector<int> pos; 
      T* oN = (T*) args[0].addr;
      
      MemoryRelObject* rel = getMemRel(oN, nl->Second(qp->GetType(s)));
      if(!rel){
        return listutils::typeError
          ("could not find memory rel object");
      }
      
      if(st == SortBy) {
        Supplier t = qp->GetSon(s,0);
        ListExpr listrel = nl->Second(qp->GetType(s));
        t = qp->GetSon(s,1);
        ListExpr attrToSort = qp->GetType(t);
        ListExpr attr;
        while (!(nl->IsEmpty(attrToSort))) {
          attr = nl->First(attrToSort);
          attrToSort = nl->Rest(attrToSort);
          
          ListExpr attrType = 0;
          int attrPos = 0;
          ListExpr attrList = nl->Second(listrel);
          
          attrPos = listutils::findAttribute(attrList,
                                             nl->ToString(attr), 
                                             attrType);
          if(attrPos == 0) {
            return listutils::typeError
              ("there is no attribute having name " + nl->ToString(attr));
          }
          pos.push_back(attrPos);
        }
      }
      else if(st == Sort) {
        ListExpr listrel = nl->Second(nl->Second(qp->GetType(s)));
        for(int i=0; i<nl->ListLength(listrel); i++)
          pos.push_back(i+1);
      }
      
      local.addr= new mquicksortInfo(rel->getmmrel(),pos); 
      return 0;
    }

    case REQUEST:
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;

    case CLOSE:
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
  }

  return -1;
}


ValueMapping mquicksortVM[] = {
  mquicksortValMap<CcString,Sort>,
  mquicksortValMap<Mem,Sort>,
  mquicksortValMap<CcString,SortBy>,
  mquicksortValMap<Mem,SortBy>,
};

template<SORTTYPE st>
int mquicksortSelect(ListExpr args){
  if(st == Sort)
    return CcString::checkType(nl->First(args))?0:1;
  else 
    return CcString::checkType(nl->First(args))?2:3;
}


/*
7.31.4 Description of operator ~mquicksort~

*/
OperatorSpec mquicksortSpec(
    "{string, mem(rel(tuple(x))} -> stream(Tuple)",
    "_ mquicksort",
    "sorts all elements of a main memory relation over all attributes",
    "query 'Staedte' mquicksort count"
);

/*
7.31.5 Instance of operator ~mquicksort~

*/
Operator mquicksortOp (
    "mquicksort",
    mquicksortSpec.getStr(),
    4,
    mquicksortVM,
    mquicksortSelect<Sort>,
    mquicksortTypeMap<Sort>
);

/*

7.32.4 Description of operator ~mquicksortby~

*/
OperatorSpec mquicksortbySpec(
    "{string, mem(rel(tuple(x))} x ID -> stream(Tuple)",
    "_ mquicksortby[]",
    "sorts the main memory over the given attributes",
    "query 'Staedte' mquicksortby[Bev,SName] count"
);

/*

7.32.5 Instance of operator ~mquicksortby~

*/
Operator mquicksortbyOp (
    "mquicksortby",
    mquicksortbySpec.getStr(),
    4,
    mquicksortVM,
    mquicksortSelect<SortBy>,
    mquicksortTypeMap<SortBy>
);

/////////////////////////////////////// GRAPH ////


/*

~dijkstra~

Calculates the shortest path between the vertices __start__ and
__dest__ in the __graph__ using dijkstra's algorithm.

*/
bool dijkstra(graph::Graph* graph, Word arg, 
              graph::Vertex* start, graph::Vertex* dest) {
  
  graph::Queue queue;

  set<graph::Vertex*,graph::Vertex::EqualVertex>::iterator it = 
                                                graph->getGraph()->begin();
  // init all nodes
  while(it != graph->getGraph()->end()) {
    graph::Vertex* v = *it;
    v->setCost(numeric_limits<double>::max());
    v->isSeen(false);
    v->setDist(0);
    v->setPrev(0);
    it++;
  }

  // cost to start node
  start->setCost(0);
  queue.push(start);
  
  // as long as queue has entries
  while(!queue.empty()) {
      graph::Vertex* v = queue.top();
      queue.pop();
      if(v->wasSeen()) continue;
      v->isSeen(true);

      // process edges
      for(size_t i=0; i<v->getEdges()->size(); i++) {
        
        vector<graph::EdgeWrap>* edges = v->getEdges();
        
        graph::Vertex* w = graph->getVertex(edges
                                      ->at(i).getPointer()->getDest());
        
        ArgVectorPointer funArgs = qp->Argument(arg.addr);
        Word funResult;
        ((*funArgs)[0]).setAddr(v->getEdges()->at(i).getPointer()->getTuple());
        qp->Request(arg.addr,funResult);
        double cost = ((CcReal*)funResult.addr)->GetRealval(); 

        if(cost<0) {
            std::cout << "Error: cost is negative" << std::endl;
            return false;
        }

        // shortening of path possible
        if(w->getCost() > v->getCost()+cost) {
            w->setCost(v->getCost()+cost);
            w->setDist(w->getCost());
            w->setPrev(v);
            queue.push(w);
        }
          
      }
      if(v->getNr() == dest->getNr()) {
        return true;
      }
  }
  return false;
}

/*

~astar~

Calculates the shortest path between the vertices __start__ and
__dest__ in the __graph__ using the AStar-algorithm.

*/
bool astar(graph::Graph* graph, Word arg, Word arg2,
           graph::Vertex* start, graph::Vertex* dest) {
  
  graph::Queue openlist;

  set<graph::Vertex*,graph::Vertex::EqualVertex>::iterator it = 
                                                graph->getGraph()->begin();
  while(it != graph->getGraph()->end()) {
    graph::Vertex* v = *it;
    v->setCost(numeric_limits<double>::max());
    v->isSeen(false);
    v->setDist(numeric_limits<double>::max());
    v->setPrev(0);
    it++;
  }

  // cost to start node
  start->setCost(0.0);
  openlist.push(start);


  // as long as queue has entries
  while(!openlist.empty()) {
    graph::Vertex* v = openlist.top();
    openlist.pop();
    if(v->wasSeen()) continue;
    v->isSeen(true);
    
    if(v->getNr() == dest->getNr()) {
      return true;
    }
    
    // for every adjacent edge of v
    for(size_t i=0; i<v->getEdges()->size(); i++) {
      graph::Vertex* w = graph->getVertex(v->getEdges()
                                 ->at(i).getPointer()->getDest());
      ArgVectorPointer funArgs = qp->Argument(arg.addr);
      Word funResult;
      ((*funArgs)[0]).setAddr(v->getEdges()->at(i).getPointer()->getTuple());
      qp->Request(arg.addr,funResult);
      double edgecost = ((CcReal*)funResult.addr)->GetRealval();      
      
      if(edgecost<0.0) {
        return false;
      }
      
      ArgVectorPointer funArgs2 = qp->Argument(arg2.addr);
      Word funResult2;
      ((*funArgs2)[0]).setAddr(v->getEdges()->at(i).getPointer()->getTuple());
      qp->Request(arg2.addr,funResult2);
      double prio = ((CcReal*)funResult2.addr)->GetRealval();    

      if(prio<0.0) {
        prio = 0.0;
      }

      // shortening of path possible
      if(w->getDist() > v->getCost()+edgecost+prio) {
          w->setCost(v->getCost()+edgecost);
          w->setDist(prio+w->getCost());
          
          if(!w->wasSeen())
            w->setPrev(v);
          openlist.push(w);
      }
     }
      
  }
  return false;
}

bool shortestPath(graph::Graph* graph, int startNode, 
                  int endNode, Word arg, Word arg2, 
                  bool isAstar) {
  bool found = false;
  if(!isAstar) {
    found = dijkstra(graph, arg,
                     graph->getVertex(startNode), 
                     graph->getVertex(endNode));
  }
  if(isAstar) {
    found = astar(graph,arg,arg2,
                  graph->getVertex(startNode), 
                  graph->getVertex(endNode));
  }
  if(found) {
    graph->getPath(graph->getVertex(endNode));
  }
  return found;        
}

/*
7.33 Operator ~mgshortestdpathd~

The Operator ~mgshortestpathd~ and ~mgshortestpatha~ calculate the shortest
path between two given nodes of a graph. The graph is in this case represented
by a main memory graph. The tuples of such a graph need to 
have two integer values as their first two attributes identifying the start and
end node of an edge and at least one double value containing the cost
the edge.

The operator expects an main memory graph, two integers describing 
the start and goal node of the path, an integer choosing the resulting output
of the shortest path calculation, as well as an cost function, which maps 
the values of the tuples to a double value. The Operator ~moshortestpatha~ 
expects an additional function calculating the distance to the goal node.

The following results can be selected by integer:
0 shortest path
1 remaining priority queue at end of computation
2 visited sections of shortest path search
3 shortest path tree

For all cases an additional attribute 'seqNo' of type integer will be appended
to the tuples of the output stream.

The Operator ~mgshortestpathd~ uses Dijkstras Algorithm and the Operator 
~mgshortestpatha~ uses the AStar-Algorithm.


7.33.1 General Type Mapping for Operators ~mgshortestpathd~ and 
       ~mgshortestpatha~
   
{string, mem(rel(tuple(X)))} x int x int x int -> 
stream(tuple(a1:t1,...an+1:tn+1))  and

{string, mem(graph(tuple(X)))} x int x int x int x (tuple->real) 
x (tuple->real) -> 
stream(tuple(a1:t1,...an+1:tn+1))
   
*/
template<bool isAstar>
ListExpr mgshortestpathdTypeMap(ListExpr args) {

  if(!isAstar && nl->ListLength(args) != 5) {
      return listutils::typeError("four arguments expected");
  }
  
  else if(isAstar && nl->ListLength(args) != 6) {
      return listutils::typeError("five arguments expected");
  }

  ListExpr first = nl->First(args);
    
  if(!nl->HasLength(first,2)){
      return listutils::typeError("internal error");
  }
  
  string errMsg;

  if(!getMemType(nl->First(first), nl->Second(first), first, errMsg))
    return listutils::typeError("\n problem in first arg: " + errMsg);
  
  ListExpr graph = nl->Second(first); // remove leading mem
  if(!MemoryGraphObject::checkType(graph))
    return listutils::typeError("first arg is not a mem graph");
  

  ListExpr startNodeList = nl->First(nl->Second(args));
  ListExpr endNodeList = nl->First(nl->Third(args));
  ListExpr resultSelect = nl->First(nl->Fourth(args));
  ListExpr map = nl->First(nl->Fifth(args));
  ListExpr distMap;
  if(isAstar)
    distMap = nl->First(nl->Sixth(args));

  graph = nl->Second(graph);

  if(!listutils::isTupleDescription(graph)) {
    return listutils::typeError(
                       "second value of graph is not of type tuple");
  }
  
  ListExpr relAttrList(nl->Second(graph));

  if(!listutils::isAttrList(relAttrList)) {
    return listutils::typeError("Error in rel attrlist.");
  }

  if(!(nl->ListLength(relAttrList) >= 3)) {
    return listutils::typeError("rel has less than 3 attributes.");
  }

  //Check of second argument
  if (!listutils::isSymbol(startNodeList,CcInt::BasicType())) {
    return listutils::typeError("Second argument should be int");
  }

  //Check of third argument
  if (!listutils::isSymbol(endNodeList,CcInt::BasicType())) {
    return listutils::typeError("Third argument should be int");
  }

  //Check of fourth argument
  if(!listutils::isSymbol(resultSelect,CcInt::BasicType())) {
    return listutils::typeError("Fourth argument should be int");
  }
  
  //Check of fifth argument
  if(!listutils::isMap<1>(map)) {
    return listutils::typeError("Fifth argument should be a map");
  }

  ListExpr mapTuple = nl->Second(map);

  if(!nl->Equal(graph,mapTuple)) {
    return listutils::typeError(
                       "Tuple of map function must match graph tuple");
  }

  ListExpr mapres = nl->Third(map);

  if(!listutils::isSymbol(mapres,CcReal::BasicType())) {
    return listutils::typeError(
                 "Wrong mapping result type for mgshortestpatha");
  }
  
  //Check of sixth argument if AStar
  if(isAstar) {
    if(!listutils::isMap<1>(distMap)) {
      return listutils::typeError(
                      "Sixth argument should be a map");
    }

    ListExpr distmapTuple = nl->Second(distMap);

    if(!nl->Equal(graph,distmapTuple)) {
      return listutils::typeError(
             "Tuple of map function must match graph tuple");
    }

    ListExpr distmapres = nl->Third(distMap);

    if(!listutils::isSymbol(distmapres,CcReal::BasicType())) {
      return listutils::typeError(
             "Wrong mapping result type for mgshortestpatha");
    }
  }

  // appends Attribute SeqNo to Attributes in orel
  ListExpr rest = nl->Second(graph);
  ListExpr listn = nl->OneElemList(nl->First(rest));
  ListExpr lastlistn = listn;
  rest = nl->Rest(rest);
  while (!(nl->IsEmpty(rest))) {
    lastlistn = nl->Append(lastlistn,nl->First(rest));
    rest = nl->Rest(rest);
  }
  lastlistn = nl->Append(lastlistn,
                        nl->TwoElemList(
                          nl->SymbolAtom("SeqNo"),
                          nl->SymbolAtom(TupleIdentifier::BasicType())));
  
  return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                                      nl->TwoElemList(
                                        nl->SymbolAtom(Tuple::BasicType()),
                                        listn));
}


class mgshortestpathdInfo {
public:
  
  mgshortestpathdInfo(graph::Graph* _graph, 
                      int _startNode, int _endNode, 
                      int _resultSelect, Word _arg,
                      TupleType* _tt) 
      : graph(_graph),startNode(_startNode),
        endNode(_endNode), resultSelect(_resultSelect),
        arg(_arg), tt(_tt),seqNo(1) {
          
          result = new ttree::TTree<TupleWrap,TupleComp>(16,18); 
          bool found = shortestPath(graph,startNode,endNode,arg,arg,false);
          
          if(found || resultSelect == 3) {
            resultSelection();
          }
          else { 
            cerr << "no path found" << endl;
          }
          iterator = result->begin();

        }

  ~mgshortestpathdInfo() {
    graph->reset();
    delete result;
    tt->DeleteIfAllowed();
  }
  
  Tuple* next() {
    if(iterator.end()) 
      return 0;
    Tuple* res = (*iterator).getPointer();
    if(res==NULL) {
      return 0;
    }
    iterator++;
    res->IncReference();
    return res;
  }
  
 
  
/*
~resultSelection~
 
*/  
  void resultSelection() { 
  
    switch(resultSelect) {
      case 0: {  // shortest path  
      
        seqNo = 1;
        int source = startNode;
        int dest = 0;
        
        for(size_t i=1; i<graph->getResult()->size(); i++) {
          dest = graph->getResult()->at(i);
          Tuple* tup = graph->getEdge(source,dest);          
          if(!tup)
            break;

          appendTuple(tup,tt,seqNo,result);
          seqNo++;
          source = dest;
        }
        graph->reset();
        break;
      }
      case 1: { //Remaining elements in priority queue
        
        break;
      }
      
      case 2: { //visited sections
      
        break;
      }
  
      case 3: { //shortest path tree     
      
        break;
      }

      default: { //should have never been reached
        break;
      }
    }
      
  }


protected:

  graph::Graph* graph;
  int startNode;
  int endNode;
  int resultSelect;
  Word arg;
  TupleType* tt;  
  int seqNoAttrIndex;
  int seqNo;
  ttree::TTree<TupleWrap,TupleComp>* result;
  ttree::Iterator<TupleWrap,TupleComp> iterator;  
};


/*

7.33.2 Value Mapping Function of operator ~mgshortestpathd~ 

*/
template<class T>
int mgshortestpathdValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

  mgshortestpathdInfo* li = (mgshortestpathdInfo*) local.addr;

  switch (message) {
    case OPEN: {
      
      if(li){
        delete li;
        local.addr=0;
      }
      
      int resultSelect = ((CcInt*)args[3].addr)->GetIntval();
      if(resultSelect<0 || resultSelect>3) {
        cerr << "Selected result value does not exist. Enter 0 for shortest "
        << "path, 1 for remaining priority queue elements, 2 for visited "
        << "edges, 3 for shortest path tree." << endl;
        return 0;
      }

      // Check for simplest Case
      int startNode = ((CcInt*)args[1].addr)->GetIntval();
      int endNode = ((CcInt*)args[2].addr)->GetIntval();
      if(resultSelect<3) {
        if(startNode == endNode) {
          cerr << "source and target node are equal, no path";
          return 0;
        }
      }
      
      T* oN = (T*) args[0].addr;
      
      MemoryGraphObject* memgraph = getMemGraph(oN);
      if(!memgraph) {
        return 0;
      }

      ListExpr tupleType = GetTupleResultType(s);
      TupleType* tt = new TupleType(nl->Second(tupleType));

      local.addr = new mgshortestpathdInfo(memgraph->getgraph(),
                                           startNode, 
                                           endNode, 
                                           resultSelect,
                                           args[4],tt);
      return 0;
    }

    case REQUEST:
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;

    case CLOSE:
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
  }

  return -1;
}


ValueMapping mgshortestpathdVM[] = {
  mgshortestpathdValMap<CcString>,
  mgshortestpathdValMap<Mem>
};

int mgshortestpathdSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}


/*

7.33.4 Description of operator ~mgshortestpathd~

*/
OperatorSpec mgshortestpathdSpec(
    "{string, mem(graph(tuple(x)))} x int x int x int -> "
    "stream(tuple(a1:t1,...an+1:tn+1))",
    "_ mgshortestpathd [_,_,_; fun] implicit parameter tuple type MTUPLE",
    "finds the shortest path between to given nodes in a main memory "
    "graph using dijkstras algorithm",
    "query mwrap('graph') mgshortestpathd "
    "[1,3,0; distance(.GeoData_s1,.GeoData_s2)] count"
);


/*

7.33.5 Instance of operator ~mgshortestpathd~

*/
Operator mgshortestpathdOp (
    "mgshortestpathd",
    mgshortestpathdSpec.getStr(),
    2,
    mgshortestpathdVM,
    mgshortestpathdSelect,
    mgshortestpathdTypeMap<false>
);



class mgshortestpathaInfo {
public:
  
  mgshortestpathaInfo(graph::Graph* _graph, 
                     int _startNode, int _endNode, int _resultSelect,
                     Word _arg, Word _arg2, TupleType* _tt) 
      : graph(_graph),startNode(_startNode),
        endNode(_endNode),resultSelect(_resultSelect),
        arg(_arg),arg2(_arg2), tt(_tt),seqNo(1) {

          result = new ttree::TTree<TupleWrap,TupleComp>(16,18); 
        
          bool found = shortestPath(graph,startNode,endNode,arg,arg2,true);
          if(found || resultSelect == 3) {
            resultSelection();
          }
          else 
            cerr << "no path found" << endl;
          
          iterator = result->begin();

        }
        
  ~mgshortestpathaInfo() {
    graph->reset();
    delete result;
    result = 0;
    tt->DeleteIfAllowed();
  }
  
  Tuple* next() {
    if(iterator.end()) 
      return 0;
    Tuple* res = (*iterator).getPointer();
    if(res==NULL) {
      return 0;
    }
    iterator++;
    res->IncReference();
    return res;
  }
  


/*
~resultSelection~
 
*/  
  void resultSelection() { 
  
    switch(resultSelect) {
      case 0: {  // shortest path  
      
        seqNo = 1;
        int source = startNode;
        int dest = 0;
        
        for(size_t i=1; i<graph->getResult()->size(); i++) {
          dest = graph->getResult()->at(i);
          Tuple* tup = graph->getEdge(source,dest);          
          if(!tup)
            break;
          appendTuple(tup,tt,seqNo,result);
          seqNo++;
          source = dest;
        }
        graph->reset();
        break;
      }
      case 1: { //Remaining elements in priority queue
        
        break;
      }
      
      case 2: { //visited sections
      
        break;
      }
  
      case 3: { //shortest path tree     
      
        break;
      }

      default: { //should have never been reached
        break;
      }
    }
      
  }


protected:

  graph::Graph* graph;
  int startNode;
  int endNode;
  int resultSelect;
  Word arg;
  Word arg2;
  TupleType* tt;  
  int seqNoAttrIndex;
  int seqNo;
  ttree::TTree<TupleWrap,TupleComp>* result;
  ttree::Iterator<TupleWrap,TupleComp> iterator;
};


/*

7.34.2 Value Mapping Function of operator ~mgshortestpatha~

*/
template<class T>
int mgshortestpathaValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

  mgshortestpathaInfo* li = (mgshortestpathaInfo*) local.addr;

  switch (message) {
    case OPEN: {
      
      if(li){
        delete li;
        local.addr=0;
      }
      
      int resultSelect = ((CcInt*)args[3].addr)->GetIntval();
      if(resultSelect<0 || resultSelect>3) {
        cerr << "Selected result value does not exist. Enter 0 for shortest "
        << "path, 1 for remaining priority queue elements, 2 for visited "
        << "edges, 3 for shortest path tree." << endl;
        return 0;
      }

      // Check for simplest Case
      int startNode = ((CcInt*)args[1].addr)->GetIntval();
      int endNode = ((CcInt*)args[2].addr)->GetIntval();
      if(resultSelect<3) {
        if(startNode == endNode) {
          cerr << "source and target node are equal, no path";
          return 0;
        }
      }
      
      T* oN = (T*) args[0].addr;
      
      MemoryGraphObject* memgraph = getMemGraph(oN);
      if(!memgraph) {
        return 0;
      }
      
      ListExpr tupleType = GetTupleResultType(s);
      TupleType* tt = new TupleType(nl->Second(tupleType));

      local.addr = new mgshortestpathaInfo(memgraph->getgraph(),
                                           startNode, 
                                           endNode, 
                                           resultSelect,
                                           args[4],args[5],tt);
      return 0;
    }

    case REQUEST:
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;

    case CLOSE:
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
  }

  return -1;
}


ValueMapping mgshortestpathaVM[] = {
  mgshortestpathaValMap<CcString>,
  mgshortestpathaValMap<Mem>
};

int mgshortestpathaSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}


/*

7.34.4 Description of operator ~mgshortestpatha~

*/
OperatorSpec mgshortestpathaSpec(
    "{string, mem(graph(tuple(x)))} x int x int x int x (tuple->real) x "
    "(tuple->real) -> stream(tuple(a1:t1,...an+1:tn+1))",
    "_ mgshortestpatha [_,_,_; fun, fun] implicit parameter tuple type MTUPLE",
    "finds the shortest path between to given nodes in a main memory "
    "graph using the a*-algorithm",
    "query mwrap('graph') mgshortestpatha "
    "[1,40,0; distance(.SourcePos,.TargetPos), "
    "distance(.SourcePos,.TargetPos) * 2.0] count"
);


/*

7.34.5 Instance of operator ~mgshortestpatha~

*/
Operator mgshortestpathaOp (
    "mgshortestpatha",
    mgshortestpathaSpec.getStr(),
    2,
    mgshortestpathaVM,
    mgshortestpathaSelect,
    mgshortestpathdTypeMap<true>
);




int memglet(Word* args, Word& result,
            int message, Word& local, Supplier s, bool flob) {

    result  = qp->ResultStorage(s);
    CcBool* b = static_cast<CcBool*>(result.addr);
    bool memletsucceed = false;
    bool correct = true;

    CcString* oN = (CcString*) args[0].addr;
    if(!oN->IsDefined()){
        return 0;
    }
    string objectName = oN->GetValue();
    Supplier t = qp->GetSon( s, 1 );
    ListExpr le = qp->GetType(t);
    
    if(listutils::isOrelDescription(le)) {
        GenericRelation* rel = static_cast<Relation*>(args[1].addr);
        MemoryGraphObject* memgraph = new MemoryGraphObject();
        memletsucceed = memgraph->relToGraph(rel,le,getDBname(),flob);
        if(memletsucceed) {
          catalog->insert(objectName,memgraph);
//           memgraph->getgraph()->print(cout);
        }
        else 
          delete memgraph;
    }    
    else {
      correct = false;
    }

    b->Set(correct, memletsucceed);

    return 0;

}

/*
7.35 Operator ~memglet~ and ~memgletflob~

The operator ~memglet~ creates a main memory graph using adjacency list
from an ordered relation which represents a graph as well. As an additional
argument the name of the graph is required. If the creation
of the main memory graph is successful the operator returns true.
In addition the operator ~memgletflob~ loads the flobs of the attributes into
the main memory.

7.35.1 Type Mapping Functions of operator ~memlet~ and ~memgletflob~

string x orel(tuple(x)) -> bool

*/
ListExpr memgletTypeMap(ListExpr args) {
  
 if(nl->ListLength(args)!=2){
    return listutils::typeError("wrong number of arguments");
 }
 if(!checkUsesArgs(args)){
   return listutils::typeError("internal error");
 }


 ListExpr arg1 = nl->First(args);
 if (!CcString::checkType(nl->First(arg1))) {
    return listutils::typeError("string expected");
 }
 ListExpr str = nl->Second(arg1);
 if(nl->AtomType(str) != StringType){
   return listutils::typeError("The first arg must be a constant string");
 }
 string objectName = nl->StringValue(str);
 if (catalog->isMMObject(objectName)){
    return listutils::typeError("identifier already in use");
 }
 ListExpr arg2 = nl->Second(args);
    
 if(!listutils::isOrelDescription(nl->First(arg2))) {
   return listutils::typeError("the second argument has to be "
                               "an ordered relation");
 }
    
 // check if tuples contain at least to integer values
 ListExpr orelTuple = nl->Second(nl->First(arg2));    
 ListExpr attrlist(nl->Second(orelTuple));

    
 ListExpr rest = attrlist;
 bool foundSource = false;
 bool foundTarget = false;

 if(!nl->HasMinLength(attrlist,2)){
   return listutils::typeError("orel has less than "
                               "two integer attributes.");

 }

    
 while(!nl->IsEmpty(rest)) {
    ListExpr listn = nl->OneElemList(nl->Second(nl->First(rest)));  
    if(listutils::isSymbol(nl->First(listn),CcInt::BasicType())) {    
      if(!foundSource) 
         foundSource = true;
      else {
         foundTarget = true;
         break;
      }
    }
    rest = nl->Rest(rest);
  }

  if(!foundSource || !foundTarget) {
      return listutils::typeError("orel has less than 2 int attributes.");
  }
    
  return listutils::basicSymbol<CcBool>();
}


/*

7.35.2  The Value Mapping Functions of operator ~memglet~

*/
int memgletValMap(Word* args, Word& result,
                 int message, Word& local, Supplier s) {

   return memglet(args, result, message, local,s, false);
}


/*

7.35.4 Description of operator ~memglet~

*/
OperatorSpec memgletSpec(
    "string x orel(tuple(x)) -> bool",
    "memglet (_,_)",
    "creates a main memory graph object from a given ordered relation",
    "query memglet ('graph', otestrel)"
);


/*

7.35.5 Instance of operator ~memglet~

*/
Operator memgletOp (
    "memglet",
    memgletSpec.getStr(),
    memgletValMap,
    Operator::SimpleSelect,
    memgletTypeMap
);

/*

7.36.2  The Value Mapping Functions of operator ~memgletflob~

*/
int memgletflobValMap (Word* args, Word& result,
                int message, Word& local, Supplier s) {

   return memglet(args, result, message, local, s,true);
}

/*

7.36.4 Description of operator ~memgletflob~

*/
OperatorSpec memgletflobSpec(
    "string x orel(tuple(x)) -> bool",
    "memgletflob (_,_)",
    "creates a main memory graph object from a given ordered relation "
    "and loads the flobs into the main memory",
    "query memgletflob ('graph', otestrel)"
);


/*

7.36.5 Instance of operator ~memgletflob~

*/
Operator memgletflobOp (
    "memgletflob",
    memgletflobSpec.getStr(),
    memgletflobValMap,
    Operator::SimpleSelect,
    memgletTypeMap
);


/*

7.37 Operator ~mgconnectedcomponents~

The operator ~mgconnectedcomponents~ computes the strongly connected 
components in a given main meory graph. the tuples of the graph
with the appended component number are returned in an output stream.

7.37.1 Type Mapping Functions of operator ~mgconnectedcomponents~

{string, mem(graph(tuple(x)))} -> stream(tuple(a1:t1,...an+1:tn+1))
    
*/
ListExpr mgconnectedcomponentsTypeMap(ListExpr args) {
  
    if(nl->ListLength(args) != 1) {
        return listutils::typeError("one argument expected");
    }

    ListExpr first = nl->First(args);
    
    if(!nl->HasLength(first,2)){
        return listutils::typeError("internal error");
    }
    
    string errMsg;

    if(!getMemType(nl->First(first), nl->Second(first), first, errMsg))
      return listutils::typeError("\n problem in first arg: " + errMsg);
    
    ListExpr graph = nl->Second(first); // remove leading mem
    if(!MemoryGraphObject::checkType(graph))
      return listutils::typeError("first arg is not a mem graph");
    
    // check if second argument attribute of relation
    graph = nl->Second(graph);
    ListExpr relAttrList(nl->Second(graph));

    if(!listutils::isAttrList(relAttrList)) {
      return listutils::typeError("Error in rel attrlist.");
    }
    
    // appends Attribute CompNo to Attributes in orel
    ListExpr rest = nl->Second(graph);
    ListExpr listn = nl->OneElemList(nl->First(rest));
    ListExpr lastlistn = listn;
    rest = nl->Rest(rest);
    while (!(nl->IsEmpty(rest))) {
      lastlistn = nl->Append(lastlistn,nl->First(rest));
      rest = nl->Rest(rest);
    }
    lastlistn = nl->Append(lastlistn,
                          nl->TwoElemList(
                            nl->SymbolAtom("CompNo"),
                            listutils::basicSymbol<CcInt>()));
    
    return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                                        nl->TwoElemList(
                                          nl->SymbolAtom(Tuple::BasicType()),
                                          listn));
}


class mgconnectedComponentsInfo{
  public:
    mgconnectedComponentsInfo(graph::Graph* _graph,
                              TupleType* _tt)
        : graph(_graph),tt(_tt) {
      
      // compute the strongly connected components
      graph->tarjan();
      it = graph->getGraph()->begin();
      j = 0;
    }
    
    ~mgconnectedComponentsInfo(){
      tt->DeleteIfAllowed();
      graph->clearScc();
    }
    
    
    Tuple* next() {
      if(it == graph->getGraph()->end()) {
        graph->print(cout);
        return 0;
      }      
      // get next edge from graph
      graph::Vertex* v = *it;
      if(j>=v->getEdges()->size()) {
        it++;
        j=0;
        if(it != graph->getGraph()->end()) {
          v = *it;
        }
        else return 0;
      }
      while(v->getEdges()->size() == 0) {
        it++;
        if(it != graph->getGraph()->end()) {
          v = *it;
        }
        else return 0;
      }

      // get the tuple
      graph::Edge* e = v->getEdges()->at(j).getPointer();
      Tuple* tup = e->getTuple();
      graph::Vertex* u = graph->getVertex(e->getDest());
      int compNo = u->getCompNo();
      j++;

      // add component number to output tuple
      CcInt* comp = new CcInt(true,compNo);
      Tuple* res = new Tuple(tt);
      int i = 0;
      for(; i<tup->GetNoAttributes(); i++) {
        res->CopyAttribute(i,tup,i);
      }     
      res->PutAttribute(i,comp);
      return res;  
    }
    
  private:
    graph::Graph* graph;
    TupleType* tt;
    size_t j;
    set<graph::Vertex*,graph::Vertex::EqualVertex>::iterator it;
};

/*

7.37.2 Value Mapping Function of operator ~mgconnectedcomponents~

*/
template<class T>
int mgconnectedcomponentsValMap (Word* args, Word& result,
                    int message, Word& local, Supplier s) {

  mgconnectedComponentsInfo* li = (mgconnectedComponentsInfo*) local.addr;

  switch (message) {
    case OPEN: {
      
      if(li){
        delete li;
        local.addr=0;
      }
      T* oN = (T*) args[0].addr;
      

      MemoryGraphObject* memgraph = getMemGraph(oN);
      if(!memgraph) {
        return 0;
      }
      
      ListExpr tupleType = GetTupleResultType(s);
      TupleType* tt = new TupleType(nl->Second(tupleType));
      
      local.addr= new mgconnectedComponentsInfo(memgraph->getgraph(),tt); 
      return 0;
    }

    case REQUEST:
      result.addr=li?li->next():0;
      return result.addr?YIELD:CANCEL;

    case CLOSE:
      if(li) {
        delete li;
        local.addr = 0;
      }
      return 0;
  }

  return -1;
}


ValueMapping mgconnectedcomponentsVM[] = {
  mgconnectedcomponentsValMap<CcString>,
  mgconnectedcomponentsValMap<Mem>
};

int mgconnectedcomponentsSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}


/*

7.37.4 Description of operator ~mgconnectedcomponents~

*/
OperatorSpec mgconnectedcomponentsSpec(
    "(mem(graph)) -> stream(Tuple)",
    "_ mgconnectedcomponents",
    "computes the scc's for a given main memory graph",
    "query 'graph' mgconnectedcomponents"
);

/*

7.37.5 Instance of operator ~mgconnectedcomponents~

*/
Operator mgconnectedcomponentsOp (
    "mgconnectedcomponents",
    mgconnectedcomponentsSpec.getStr(),
    2,
    mgconnectedcomponentsVM,
    mgconnectedcomponentsSelect,
    mgconnectedcomponentsTypeMap
);


/*
7.38 ~momapmatchmht~

7.38.1 Operator-Info
    map matching with a main memory ordered relation
    Result: Tuples of matched edges with timestamps

*/
// struct MOMapMatchMHTInfo : OperatorInfo {
// MOMapMatchMHTInfo() {
//   name      = "momapmatchmht";
//   signature = MemoryORelObject::BasicType() + " x " +
//               rtreehelper::BasicType() + " x " +
//               MemoryRelObject::BasicType() + " x " +
//               temporalalgebra::MPoint::BasicType() + " -> " +
//               "stream(tuple([<attributes of tuple of edge>,"
//                             "StartTime:DateTime, EndTime:DateTime]))";
// 
//   appendSignature(MemoryORelObject::BasicType() + " x " +
//                   rtreehelper::BasicType() + " x " +
//                   MemoryRelObject::BasicType() + " x " +
//                   FText::BasicType()  + " -> " +
//                   "stream(tuple([<attributes of tuple of edge>,"
//                                 "StartTime:DateTime, EndTime:DateTime]))");
// 
//   appendSignature(MemoryORelObject::BasicType() + " x " +
//                   rtreehelper::BasicType() + " x " +
//                   MemoryRelObject::BasicType() + " x " +
//                   "(stream (tuple([Lat:real, Lon:real, Time:DateTime "
//                                   "[,Fix:int] [,Sat:int] [,Hdop : real]"
//                                   "[,Vdop:real] [,Pdop:real] "
//                                   "[,Course:real] [,Speed(m/s):real]])))"
//                   + " -> " +
//                   "stream(tuple([<attributes of tuple of edge>,"
//                                 "StartTime:DateTime, EndTime:DateTime]))");
// 
// 
//   syntax    = "momapmatchmht ( _ , _ , _ , _ )";
//   meaning   = "The operation maps the MPoint or "
//               "the data of a gpx-file or "
//               "the data of a tuple stream "
//               "to the given network, which is based on a "
//               "main memory ordered relation."
//               "Result is a stream of tuples with the matched edges "
//               "of the network with timestamps.";
//   example   = "momapmatchmht('Edges', 'EdgeIndex_Box_rtree', "
//                              "'EdgeIndex', 'Trk_Dortmund.gpx')";
// }
// };


// static ListExpr GetORelNetworkAttrIndexes(ListExpr ORelAttrList);


// static ListExpr GetMMDataIndexesOfTupleStream(ListExpr TupleStream);


// enum MOMapMatchingMHTResultType {
//     OMM_RESULT_STREAM,
//     OMM_RESULT_GPX,
//     OMM_RESULT_MPOINT
// };


// static ListExpr AppendLists(ListExpr List1, ListExpr List2) {
//     
//   ListExpr ResultList = nl->TheEmptyList();
//   ListExpr last = nl->TheEmptyList();
// 
//   ListExpr Lauf = List1;
// 
//   while (!nl->IsEmpty(Lauf)) {
//     ListExpr attr = nl->First(Lauf);
// 
//     if (nl->IsEmpty(ResultList)) {
//       ResultList = nl->OneElemList(attr);
//       last = ResultList;
//     }
//     else
//       last = nl->Append(last, attr);
//     
//     Lauf = nl->Rest(Lauf);
//   }
// 
//   Lauf = List2;
// 
//   while (!nl->IsEmpty(Lauf)) {
//     ListExpr attr = nl->First(Lauf);
// 
//     if (nl->IsEmpty(ResultList)) {
//         ResultList = nl->OneElemList(attr);
//         last = ResultList;
//     }
//     else 
//       last = nl->Append(last, attr);
// 
//     Lauf = nl->Rest(Lauf);
//   }
// 
//   return ResultList;
// }


// template<class T>
// static typename mapmatching::MmORelNetwork<T>::OEdgeAttrIndexes 
//                         GetOEdgeAttrIndexes(Word* args, int nOffset) {
//     // s. GetORelNetworkAttrIndexes
// 
//     typename mapmatching::MmORelNetwork<T>::OEdgeAttrIndexes Indexes;
// 
//    const CcInt* pIdxSource    = static_cast<CcInt*>(args[nOffset + 0].addr);
//    const CcInt* pIdxTarget    = static_cast<CcInt*>(args[nOffset + 1].addr);
//    const CcInt* pIdxSourcePos = static_cast<CcInt*>(args[nOffset + 2].addr);
//    const CcInt* pIdxTargetPos = static_cast<CcInt*>(args[nOffset + 3].addr);
//    const CcInt* pIdxCurve     = static_cast<CcInt*>(args[nOffset + 4].addr);
//    const CcInt* pIdxRoadName  = static_cast<CcInt*>(args[nOffset + 5].addr);
//     const CcInt* pIdxRoadType  = static_cast<CcInt*>(args[nOffset + 6].addr);
//    const CcInt* pIdxMaxSpeed  = static_cast<CcInt*>(args[nOffset + 7].addr);
//    const CcInt* pIdxWayId     = static_cast<CcInt*>(args[nOffset + 8].addr);
// 
// 
//     Indexes.m_IdxSource    = pIdxSource->GetValue();
//     Indexes.m_IdxTarget    = pIdxTarget->GetValue();
//     Indexes.m_IdxSourcePos = pIdxSourcePos->GetValue();
//     Indexes.m_IdxTargetPos = pIdxTargetPos->GetValue();
//     Indexes.m_IdxCurve     = pIdxCurve->GetValue();
//     Indexes.m_IdxRoadName  = pIdxRoadName->GetValue();
//     Indexes.m_IdxRoadType  = pIdxRoadType->GetValue();
//     Indexes.m_IdxMaxSpeed  = pIdxMaxSpeed->GetValue();
//     Indexes.m_IdxWayId     = pIdxWayId->GetValue();
// 
//     if (Indexes.m_IdxSource < 0 || Indexes.m_IdxTarget < 0 ||
//         Indexes.m_IdxSourcePos < 0 || Indexes.m_IdxTargetPos < 0 ||
//         Indexes.m_IdxCurve < 0 || Indexes.m_IdxWayId < 0)
//     {
//         assert(false);
//     }
// 
//     return Indexes;
// }


// static ListExpr GetORelNetworkAttrIndexes(ListExpr ORelAttrList) {
//     ListExpr attrType;
//     int nAttrSourceIndex = 
//                 listutils::findAttribute(ORelAttrList, "Source", attrType);
//     if (nAttrSourceIndex == 0) 
//         return listutils::typeError("'Source' not found in attr list");
//     else {
//         if (!CcInt::checkType(attrType) &&
//             !LongInt::checkType(attrType)) {
//             return listutils::typeError(
//                                "'Source' must be " + CcInt::BasicType() +
//                                " or " + LongInt::BasicType());
//         }
//     }
// 
//     int nAttrTargetIndex = 
//                 listutils::findAttribute(ORelAttrList, "Target", attrType);
//     if (nAttrTargetIndex == 0) {
//       return listutils::typeError("'Target' not found in attr list");
//     }
//     else {
//         if (!CcInt::checkType(attrType) &&
//             !LongInt::checkType(attrType)) {
//             return listutils::typeError(
//                                    "'Target' must be " + CcInt::BasicType()
//                                    + " or " + LongInt::BasicType()) ;
//         }
//     }
// 
//     int nAttrSourcePosIndex = listutils::findAttribute(
//                                       ORelAttrList, "SourcePos", attrType);
//     if (nAttrSourcePosIndex == 0) {
//         return listutils::typeError("'SourcePos' not found in attr list");
//     }
//     else {
//         if (!listutils::isSymbol(attrType, Point::BasicType())) {
//             return listutils::typeError(
//                               "'SourcePos' must be " + Point::BasicType());
//         }
//     }
// 
//     int nAttrTargetPosIndex = listutils::findAttribute(
//                                       ORelAttrList, "TargetPos", attrType);
//     if (nAttrTargetPosIndex == 0) {
//         return listutils::typeError("'TargetPos' not found in attr list");
//     }
//     else {
//         if (!listutils::isSymbol(attrType, Point::BasicType())) {
//             return listutils::typeError(
//                                "'TargetPos' must be " + Point::BasicType());
//         }
//     }
// 
//     int nAttrCurveIndex = listutils::findAttribute(
//                                           ORelAttrList, "Curve", attrType);
//     if (nAttrCurveIndex == 0) {
//         return listutils::typeError("'Curve' not found in attr list");
//     }
//     else {
//         if (!listutils::isSymbol(attrType, SimpleLine::BasicType())) {
//             return listutils::typeError(
//                              "'Curve' must be " + SimpleLine::BasicType());
//         }
//     }
// 
//     int nAttrRoadNameIndex = listutils::findAttribute(
//                                         ORelAttrList, "RoadName", attrType);
//     if (nAttrRoadNameIndex != 0)
//     {
//         if (!listutils::isSymbol(attrType, FText::BasicType()))
//         {
//             return listutils::typeError(
//                                 "'RoadName' must be " + FText::BasicType());
//         }
//     }
// 
//     int nAttrRoadTypeIndex = listutils::findAttribute(
//                                        ORelAttrList, "RoadType", attrType);
//     if (nAttrRoadTypeIndex != 0)
//     {
//         if (!listutils::isSymbol(attrType, FText::BasicType()))
//         {
//             return listutils::typeError(
//                                "'RoadType' must be " + FText::BasicType());
//         }
//     }
// 
//     int nAttrMaxSpeedTypeIndex = listutils::findAttribute(
//                                         ORelAttrList, "MaxSpeed", attrType);
//     if (nAttrMaxSpeedTypeIndex != 0)
//     {
//         if (!listutils::isSymbol(attrType, FText::BasicType()))
//         {
//             return listutils::typeError(
//                                 "'MaxSpeed' must be " + FText::BasicType());
//         }
//     }
// 
//     int nAttrWayIdTypeIndex = listutils::findAttribute(
//                                           ORelAttrList, "WayId", attrType);
//     if (nAttrWayIdTypeIndex == 0)
//     {
//         return listutils::typeError("'WayId' not found in attr list");
//     }
//     else
//     {
//         if (!CcInt::checkType(attrType) &&
//             !LongInt::checkType(attrType))
//         {
//             return listutils::typeError( 
//                                     "'WayId' must be " + CcInt::BasicType()
//                                     + " or " + LongInt::BasicType());
//         }
//     }
// 
//     --nAttrSourceIndex;
//     --nAttrTargetIndex;
//     --nAttrSourcePosIndex;
//     --nAttrTargetPosIndex;
//     --nAttrCurveIndex;
//     --nAttrRoadNameIndex;
//     --nAttrRoadTypeIndex;
//     --nAttrMaxSpeedTypeIndex;
//     --nAttrWayIdTypeIndex;
// 
//     ListExpr Ind = nl->OneElemList(nl->IntAtom(nAttrSourceIndex));
//     ListExpr Last = Ind;
//     Last = nl->Append(Last, nl->IntAtom(nAttrTargetIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrSourcePosIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrTargetPosIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrCurveIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrRoadNameIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrRoadTypeIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrMaxSpeedTypeIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrWayIdTypeIndex));
// 
//     return Ind;
// }


// static shared_ptr<mapmatch::MapMatchDataContainer> 
//                     GetMMDataFromTupleStream(Supplier Stream,
//                                              Word* args,
//                                              int nOffset) {
//     
//     shared_ptr<mapmatch::MapMatchDataContainer> 
//                             pContData(new mapmatch::MapMatchDataContainer);
// 
//     const CcInt* pIdxLat    = static_cast<CcInt*>(args[nOffset + 0].addr);
//     const CcInt* pIdxLon    = static_cast<CcInt*>(args[nOffset + 1].addr);
//     const CcInt* pIdxTime   = static_cast<CcInt*>(args[nOffset + 2].addr);
//     const CcInt* pIdxFix    = static_cast<CcInt*>(args[nOffset + 3].addr);
//     const CcInt* pIdxSat    = static_cast<CcInt*>(args[nOffset + 4].addr);
//     const CcInt* pIdxHdop   = static_cast<CcInt*>(args[nOffset + 5].addr);
//     const CcInt* pIdxVdop   = static_cast<CcInt*>(args[nOffset + 6].addr);
//     const CcInt* pIdxPdop   = static_cast<CcInt*>(args[nOffset + 7].addr);
//     const CcInt* pIdxCourse = static_cast<CcInt*>(args[nOffset + 8].addr);
//     const CcInt* pIdxSpeed  = static_cast<CcInt*>(args[nOffset + 9].addr);
//   //const CcInt* pIdxEle    = static_cast<CcInt*>(args[nOffset + 10].addr);
// 
//     const int nIdxLat    = pIdxLat->GetValue();
//     const int nIdxLon    = pIdxLon->GetValue();
//     const int nIdxTime   = pIdxTime->GetValue();
//     const int nIdxFix    = pIdxFix->GetValue();
//     const int nIdxSat    = pIdxSat->GetValue();
//     const int nIdxHdop   = pIdxHdop->GetValue();
//     const int nIdxVdop   = pIdxVdop->GetValue();
//     const int nIdxPdop   = pIdxPdop->GetValue();
//     const int nIdxCourse = pIdxCourse->GetValue();
//     const int nIdxSpeed  = pIdxSpeed->GetValue();
//     //const int nIdxEle    = pIdxEle->GetValue();
// 
//     if (nIdxLat < 0 || nIdxLon < 0 || nIdxTime < 0) {
//         assert(false);
//         return pContData;
//     }
// 
//     Word wTuple;
//     qp->Open(Stream);
//     qp->Request(Stream, wTuple);
//     while (qp->Received(Stream)) {
//       Tuple* pTpl = (Tuple*)wTuple.addr;
// 
//       CcReal* pLat    = static_cast<CcReal*>(pTpl->GetAttribute(nIdxLat));
//       CcReal* pLon    = static_cast<CcReal*>(pTpl->GetAttribute(nIdxLon));
//       DateTime* pTime = 
//                      static_cast<DateTime*>(pTpl->GetAttribute(nIdxTime));
// 
//       CcInt* pFix     = NULL;
//       if (nIdxFix >= 0)
//           pFix = static_cast<CcInt*>(pTpl->GetAttribute(nIdxFix));
// 
//       CcInt* pSat     = NULL;
//       if (nIdxSat >= 0)
//           pSat = static_cast<CcInt*>(pTpl->GetAttribute(nIdxSat));
// 
//       CcReal* pHdop   = NULL;
//       if (nIdxHdop >= 0)
//           pHdop = static_cast<CcReal*>(pTpl->GetAttribute(nIdxHdop));
// 
//       CcReal* pVdop   = NULL;
//       if (nIdxVdop >= 0)
//           pVdop = static_cast<CcReal*>(pTpl->GetAttribute(nIdxVdop));
// 
//       CcReal* pPdop   = NULL;
//       if (nIdxPdop >= 0)
//           pPdop = static_cast<CcReal*>(pTpl->GetAttribute(nIdxPdop));
// 
//       CcReal* pCourse = NULL;
//       if (nIdxCourse >= 0)
//           pCourse = static_cast<CcReal*>(pTpl->GetAttribute(nIdxCourse));
// 
//       CcReal* pSpeed  = NULL;
//       if (nIdxSpeed >= 0)
//           pSpeed = static_cast<CcReal*>(pTpl->GetAttribute(nIdxSpeed));
// 
//       if (pLat != NULL && pLon != NULL && pTime != NULL &&
//           pLat->IsDefined() && pLon->IsDefined() && pTime->IsDefined()) {
//         //cout << "found valid data point" << endl;
//           mapmatch::MapMatchData Data(pLat->GetValue(),
//                             pLon->GetValue(),
//                             pTime->millisecondsToNull());
// 
//           if (pFix != NULL && pFix->IsDefined())
//               Data.m_nFix = pFix->GetValue();
// 
//           if (pSat != NULL && pSat->IsDefined())
//               Data.m_nSat = pSat->GetValue();
// 
//           if (pHdop != NULL && pHdop->IsDefined())
//               Data.m_dHdop = pHdop->GetValue();
// 
//           if (pVdop != NULL && pVdop->IsDefined())
//               Data.m_dVdop = pVdop->GetValue();
// 
//           if (pPdop != NULL && pPdop->IsDefined())
//               Data.m_dPdop = pPdop->GetValue();
// 
//           if (pCourse != NULL && pCourse->IsDefined())
//               Data.m_dCourse = pCourse->GetValue();
// 
//           if (pSpeed != NULL && pSpeed->IsDefined())
//               Data.m_dSpeed = pSpeed->GetValue();
//           
//           pContData->Append(Data);
//       }
// 
//       pTpl->DeleteIfAllowed();
//       pTpl = NULL;
// 
//       qp->Request(Stream, wTuple);
//   }
//   qp->Close(Stream);
// 
//   return pContData;
// }


// static ListExpr GetMMDataIndexesOfTupleStream(ListExpr TupleStream) {
//     ListExpr attrType;
//     int nAttrLatIndex = listutils::findAttribute(
//                     nl->Second(nl->Second(TupleStream)), "Lat", attrType);
//     if(nAttrLatIndex==0)
//     {
//         return listutils::typeError("'Lat' not found in attr list");
//     }
//     else
//     {
//         if (!listutils::isSymbol(attrType, CcReal::BasicType()))
//         {
//             return listutils::typeError("'Lat' must be " +
//                                         CcReal::BasicType());
//         }
//     }
// 
//     int nAttrLonIndex = listutils::findAttribute(
//                      nl->Second(nl->Second(TupleStream)), "Lon", attrType);
//     if (nAttrLonIndex == 0)
//     {
//         return listutils::typeError("'Lon' not found in attr list");
//     }
//     else
//     {
//         if (!listutils::isSymbol(attrType, CcReal::BasicType()))
//         {
//             return listutils::typeError("'Lon' must be " +
//                                         CcReal::BasicType());
//         }
//     }
// 
//     int nAttrTimeIndex = listutils::findAttribute(
//                     nl->Second(nl->Second(TupleStream)), "Time", attrType);
//     if (nAttrTimeIndex == 0)
//     {
//         return listutils::typeError("'Time' not found in attr list");
//     }
//     else
//     {
//         if (!listutils::isSymbol(attrType, DateTime::BasicType()))
//         {
//             return listutils::typeError("'Time' must be " +
//                                         DateTime::BasicType());
//         }
//     }
// 
//     int nAttrFixIndex = listutils::findAttribute(
//                       nl->Second(nl->Second(TupleStream)), "Fix", attrType);
//     if (nAttrFixIndex != 0)
//     {
//         if (!listutils::isSymbol(attrType, CcInt::BasicType()))
//         {
//             return listutils::typeError("'Fix' must be " +
//                                         CcInt::BasicType());
//         }
//     }
// 
//     int nAttrSatIndex = listutils::findAttribute(
//                      nl->Second(nl->Second(TupleStream)), "Sat", attrType);
//     if (nAttrSatIndex != 0)
//     {
//         if (!listutils::isSymbol(attrType, CcInt::BasicType()))
//         {
//             return listutils::typeError("'Sat' must be " +
//                                         CcInt::BasicType());
//         }
//     }
// 
//     int nAttrHdopIndex = listutils::findAttribute(
//                     nl->Second(nl->Second(TupleStream)), "Hdop", attrType);
//     if (nAttrHdopIndex != 0)
//     {
//         if (!listutils::isSymbol(attrType, CcReal::BasicType()))
//         {
//             return listutils::typeError("'Hdop' must be " +
//                                         CcReal::BasicType());
//         }
//     }
// 
//     int nAttrVdopIndex = listutils::findAttribute(
//                     nl->Second(nl->Second(TupleStream)), "Vdop", attrType);
//     if (nAttrVdopIndex != 0)
//     {
//         if (!listutils::isSymbol(attrType, CcReal::BasicType()))
//         {
//             return listutils::typeError("'Vdop' must be " +
//                                         CcReal::BasicType());
//         }
//     }
// 
//     int nAttrPdopIndex = listutils::findAttribute(
//                     nl->Second(nl->Second(TupleStream)), "Pdop", attrType);
//     if (nAttrPdopIndex != 0)
//     {
//         if (!listutils::isSymbol(attrType, CcReal::BasicType()))
//         {
//             return listutils::typeError("'Pdop' must be " +
//                                         CcReal::BasicType());
//         }
//     }
// 
//     int nAttrCourseIndex = listutils::findAttribute(
//                   nl->Second(nl->Second(TupleStream)), "Course", attrType);
//     if (nAttrCourseIndex != 0)
//     {
//         if (!listutils::isSymbol(attrType, CcReal::BasicType()))
//         {
//             return listutils::typeError(
//                     "'Course' must be " + CcReal::BasicType());
//         }
//     }
// 
//     int nAttrSpeedIndex = listutils::findAttribute(
//                    nl->Second(nl->Second(TupleStream)), "Speed", attrType);
//     if (nAttrSpeedIndex != 0)
//     {
//         if (!listutils::isSymbol(attrType, CcReal::BasicType()))
//         {
//             return listutils::typeError(
//                     "'Speed' must be " + CcReal::BasicType());
//         }
//     }
// 
//     int nAttrEleIndex = listutils::findAttribute(
//                       nl->Second(nl->Second(TupleStream)), "Ele", attrType);
//     if (nAttrEleIndex != 0)
//     {
//         if (!listutils::isSymbol(attrType, CcReal::BasicType()))
//         {
//             return listutils::typeError(
//                     "'Ele' must be " + CcReal::BasicType());
//         }
//     }
// 
//     --nAttrLatIndex;
//     --nAttrLonIndex;
//     --nAttrTimeIndex;
//     --nAttrFixIndex;
//     --nAttrSatIndex;
//     --nAttrHdopIndex;
//     --nAttrVdopIndex;
//     --nAttrPdopIndex;
//     --nAttrCourseIndex;
//     --nAttrSpeedIndex;
//     --nAttrEleIndex;
// 
//     ListExpr Ind = nl->OneElemList(nl->IntAtom(nAttrLatIndex));
//     ListExpr Last = Ind;
//     Last = nl->Append(Last, nl->IntAtom(nAttrLonIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrTimeIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrFixIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrSatIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrHdopIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrVdopIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrPdopIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrCourseIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrSpeedIndex));
//     Last = nl->Append(Last, nl->IntAtom(nAttrEleIndex));
// 
//     return Ind;
// }


/*

7.38.2 General Type Mapping for Operators ~momapmatchmht~

*/
// ListExpr momapmatchmhtTypeMap_Common(ListExpr args,
//                                 MOMapMatchingMHTResultType eResultType) {
//     
//     if(nl->ListLength(args) != 4) {
//       return listutils::typeError("four arguments expected");
//     }
// 
//     // Check Network - OrderedRelation, RTree, Relation
// 
//     // mem(orel)
//     ListExpr arg1 = nl->First(args);
//     
//     if(!nl->HasLength(arg1,2)){
//         return listutils::typeError("internal error");
//     }
//     
//     string errMsg;
// 
//     if(!getMemType(nl->First(arg1), nl->Second(arg1), arg1, errMsg)){
//              return listutils::typeError("string or mem(orel) expected : " 
//                                           + errMsg);
//     }
// 
//     arg1 = nl->Second(arg1); // remove mem
//     if(!listutils::isOrelDescription(arg1)){
//     return listutils::typeError("memory object is not an ordered relation");
//     }
//     
//     ListExpr orelTuple = nl->Second(arg1);
//     if (!listutils::isTupleDescription(orelTuple))
//     {
//         return listutils::typeError
//                     ("2nd value of mem orel is not of type tuple");
//     }
// 
//     ListExpr orelAttrList = nl->Second(orelTuple);
//     if (!listutils::isAttrList(orelAttrList)) {
//         return listutils::typeError("Error in orel attrlist");
//     }
// 
//     // Check attributes of orel
//     ListExpr IndNetwAttr = GetORelNetworkAttrIndexes(orelAttrList);
// 
//     if(nl->Equal(IndNetwAttr, nl->TypeError()))
//       return IndNetwAttr;
// 
//     // mem(rtree 2)
//     ListExpr arg2 = nl->Second(args);
//     if(!getMemType(nl->First(arg2), nl->Second(arg2), arg2, errMsg)){
//       return listutils::typeError("\n problem in second arg: " + errMsg);
//     } 
//     
//     ListExpr rtreetype = nl->Second(arg2); // remove leading mem
//     if(!rtreehelper::checkType(rtreetype)){
//       return listutils::typeError("second arg is not a mem rtree");
//     }
//    
//     int rtreedim = nl->IntValue(nl->Second(rtreetype));
//     
//     if (rtreedim != 2) {
//         return listutils::typeError("rtree with dim==2 expected");
//     }
// 
//     // mem(rel)
//     ListExpr arg3 = nl->Third(args);
//     
//     if(!nl->HasLength(arg3,2)){
//         return listutils::typeError("internal error");
//     }
//     
//     if(!getMemType(nl->First(arg3), nl->Second(arg3), arg3, errMsg)){
//     return listutils::typeError("string or mem(rel) expected : " + errMsg);
//     }
// 
//     arg3 = nl->Second(arg3); // remove mem
//     if(!Relation::checkType(arg3)){
//       return listutils::typeError("memory object is not a relation");
//     }
// 
//     // GPS-Data (MPoint, FileName, TupleStream)
//     ListExpr arg4 = nl->First(nl->Fourth(args));
//     if (!listutils::isSymbol(arg4,temporalalgebra::MPoint::BasicType()) &&
//         !listutils::isSymbol(arg4,FText::BasicType()) &&
//         !listutils::isTupleStream(arg4)) {
//       
//         return listutils::typeError("4th argument must be " +
//                                     temporalalgebra::MPoint::BasicType() 
//                                     + " or " +
//                                     FText::BasicType() + " or " +
//                                     "tuple stream");
//     }
//    
//     // Result
//     ListExpr ResultType = nl->TheEmptyList();
// 
//     switch (eResultType) {
//       case OMM_RESULT_STREAM: {
//         ListExpr addAttrs = nl->TwoElemList(
//                     nl->TwoElemList(nl->SymbolAtom("StartTime"),
//                                     nl->SymbolAtom(DateTime::BasicType())),
//                     nl->TwoElemList(nl->SymbolAtom("EndTime"),
//                                     nl->SymbolAtom(DateTime::BasicType())));
// 
//         ListExpr ResAttr = AppendLists(orelAttrList, addAttrs);
// 
//         ResultType = nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
//                                       nl->TwoElemList(
//                                         nl->SymbolAtom(Tuple::BasicType()),
//                                         ResAttr));
//       }
//       break;
// 
//       case OMM_RESULT_GPX: {
//         ListExpr addAttrs = nl->SixElemList(
//                     nl->TwoElemList(nl->SymbolAtom("StartTime"),
//                                     nl->SymbolAtom(DateTime::BasicType())),
//                     nl->TwoElemList(nl->SymbolAtom("EndTime"),
//                                     nl->SymbolAtom(DateTime::BasicType())),
//                     nl->TwoElemList(nl->SymbolAtom("StartPos"),
//                                     nl->SymbolAtom(Point::BasicType())),
//                     nl->TwoElemList(nl->SymbolAtom("EndPos"),
//                                     nl->SymbolAtom(Point::BasicType())),
//                     nl->TwoElemList(nl->SymbolAtom("StartLength"),
//                                     nl->SymbolAtom(CcReal::BasicType())),
//                     nl->TwoElemList(nl->SymbolAtom("EndLength"),
//                                     nl->SymbolAtom(CcReal::BasicType())));
// 
//         ListExpr ResAttr = AppendLists(orelAttrList, addAttrs);
// 
//         ResultType = nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
//                                      nl->TwoElemList(
//                                         nl->SymbolAtom(Tuple::BasicType()),
//                                         ResAttr));
//       }
//       break;
// 
//       case OMM_RESULT_MPOINT: {
//           ResultType = nl->SymbolAtom(temporalalgebra::MPoint::BasicType());
//       }
//       break;
//     }
// 
// 
//     if (listutils::isTupleStream(arg4)) {
//       ListExpr IndMMData = GetMMDataIndexesOfTupleStream(arg4);
// 
//       if (nl->Equal(IndMMData, nl->TypeError()))
//           return IndMMData;
//       else {
//           return nl->ThreeElemList(nl->SymbolAtom(Symbol::APPEND()),
//                                     AppendLists(IndNetwAttr, IndMMData),
//                                     ResultType);
//       }
//     }
//     else {
//       return nl->ThreeElemList(nl->SymbolAtom(Symbol::APPEND()),
//                                 IndNetwAttr,
//                                 ResultType);
//     }
// }


// ListExpr momapmatchmhtTypeMap(ListExpr in_xArgs) {
//     return momapmatchmhtTypeMap_Common(in_xArgs, OMM_RESULT_STREAM);
// }

/*

7.38.3 Value Mapping Function of operator ~momapmatchmht~

*/
// template<class T, class R, class Q>
// int momapmatchmhtMPointValueMap(Word* args,
//                                 Word& result,
//                                 int message,
//                                 Word& local,
//                                 Supplier s,
//           typename mapmatching::MmORelEdgeStreamCreator<T>::EMode eMode) {
//   
// //     cout << "momapmatchmhtMPointValueMap called" << endl;
// 
//     mapmatching::MmORelEdgeStreamCreator<T>* pCreator =
//           static_cast<mapmatching::MmORelEdgeStreamCreator<T>*>(local.addr);
//     switch (message) {
//       
//       case OPEN: {
//         if(pCreator) {
//             delete pCreator;
//             pCreator = NULL;
//         }
// 
//         // mem(orel)
//         R* orelN = (R*) args[0].addr;
//         MemoryORelObject* orel = 
//                  getMemORel(orelN,nl->Second(qp->GetType(s)));
//         if(!orel) 
//           return 0;
//         
//         // mem(rtree)
//         Q* treeN = (Q*) args[1].addr;
//         string name = treeN->GetValue();
//         // cut blank from the front of the string
//         name = name.substr(1, name.size()-1);
//         MemoryRtreeObject<2>* tree =
//               (MemoryRtreeObject<2>*)catalog->getMMObject(name);
//         if(!tree) 
//           return 0;
//            
//         // mem(rel)
//         R* relN = (R*) args[2].addr;
//         MemoryRelObject* rel = getMemRel(relN);
//         if(!rel)
//           return 0;
//        
//         typename mapmatching::MmORelNetwork<T>::OEdgeAttrIndexes Indexes = 
//                                            GetOEdgeAttrIndexes<T>(args, 4);
// 
//         mapmatching::MmORelNetwork<T> Network(orel, tree, rel, Indexes);
// 
//         temporalalgebra::MPoint* pMPoint = 
//                     static_cast<temporalalgebra::MPoint*>(args[3].addr);
// 
//         // Do Map Matching
//         mapmatching::MmORelNetworkAdapter<T> NetworkAdapter(&Network);
//         mapmatch::MapMatchingMHT MapMatching(&NetworkAdapter, pMPoint);
//                     
//         mapmatching::MmORelEdgeStreamCreator<T>* pCreator =
//         new mapmatching::MmORelEdgeStreamCreator<T>(s,NetworkAdapter,eMode);
//         if (!MapMatching.DoMatch(pCreator)) {
//             // Error
//             delete pCreator;
//             pCreator = NULL;
//         }
// 
//         local.setAddr(pCreator);
//           return 0;
//       }
//       case REQUEST: {
//         if (pCreator == NULL) {
//           return CANCEL;
//         }
//         else {
//           result.addr = pCreator->GetNextTuple();
//           return result.addr ? YIELD : CANCEL;
//         }
//       }
//       case CLOSE: {
//         if (pCreator) {
//           delete pCreator;
//           local.addr = NULL;
//         }
//           return 0;
//       }
//       default: {
//         return 0;
//       }
//     }
// }


// template<class T, class R, class Q>
// int momapmatchmhtTextValueMap(Word* args,
//                               Word& result,
//                               int message,
//                               Word& local,
//                               Supplier s,
//          typename mapmatching::MmORelEdgeStreamCreator<T>::EMode eMode) {
// 
// //     cout << "momapmatchmhtTextValueMap called" << endl;
// 
//     mapmatching::MmORelEdgeStreamCreator<T>* pCreator =
//          static_cast<mapmatching::MmORelEdgeStreamCreator<T>*>(local.addr);
// 
//     switch (message) {
//       case OPEN: {
// 
//         if(pCreator) {
//             delete pCreator;
//             pCreator = NULL;
//         }
// 
//         // mem(orel)
//         R* orelN = (R*) args[0].addr;
//         MemoryORelObject* orel = 
//                              getMemORel(orelN,nl->Second(qp->GetType(s)));
//         if(!orel) 
//           return 0;
// 
//         //mem(rtree)
//         Q* treeN = (Q*) args[1].addr;
//         string name = treeN->GetValue();
//         // cut blank from the front of the string
//         name = name.substr(1, name.size()-1);
//         MemoryRtreeObject<2>* tree =
//               (MemoryRtreeObject<2>*)catalog->getMMObject(name);
//         if(!tree) 
//           return 0;
// 
//         // mem(rel)
//         R* relN = (R*) args[2].addr;
//         MemoryRelObject* rel = getMemRel(relN);
//         if(!rel)
//           return 0;;
//          
//         typename mapmatching::MmORelNetwork<T>::OEdgeAttrIndexes Indexes = 
//                                           GetOEdgeAttrIndexes<T>(args, 4);
// 
//         mapmatching::MmORelNetwork<T> Network(orel, tree, rel, Indexes);
//         FText* pFileName = static_cast<FText*>(args[3].addr);
//         std::string strFileName = pFileName->GetValue();
// 
//         // Do Map Matching
//         mapmatching::MmORelNetworkAdapter<T> NetworkAdapter(&Network);
//         mapmatch::MapMatchingMHT MapMatching(&NetworkAdapter, strFileName);
//         mapmatching::MmORelEdgeStreamCreator<T>* pCreator =
//         new mapmatching::MmORelEdgeStreamCreator<T>(s,NetworkAdapter,eMode);
//         
//         if (!MapMatching.DoMatch(pCreator)) {
//             //Error
//             delete pCreator;
//             pCreator = NULL;
//         }
// 
//         local.setAddr(pCreator);
//         return 0;
//       }
//       case REQUEST: {
//         if(pCreator == NULL) {
//           return CANCEL;
//         }
//         else {
//           result.addr = pCreator->GetNextTuple();
//           return result.addr ? YIELD : CANCEL;
//         }
//       }
//       case CLOSE: {
//         if (pCreator) {
//           delete pCreator;
//           local.addr = NULL;
//         }
//         return 0;
//       }
//       default: {
//         return 0;
//       }
//     }
// }


// template<class T, class R, class Q>
// int momapmatchmhtStreamValueMap(Word* args,
//                                 Word& result,
//                                 int message,
//                                 Word& local,
//                                 Supplier s,
//             typename mapmatching::MmORelEdgeStreamCreator<T>::EMode eMode) {
//   
// //     cout << "momapmatchmhtStreamValueMap called" << endl;
// 
//     mapmatching::MmORelEdgeStreamCreator<T>* pCreator =
//           static_cast<mapmatching::MmORelEdgeStreamCreator<T>*>(local.addr);
//     
//     switch (message) {
//     case OPEN: {
//       
//       if (pCreator != NULL) {
//           delete pCreator;
//           pCreator = NULL;
//       }
// 
//       // mem(orel)
//       R* orelN = (R*) args[0].addr;
//       MemoryORelObject* orel = getMemORel(orelN,nl->Second(qp->GetType(s)));
//       if(!orel) 
//         return 0;
//       
//       // mem(rtree)
//       Q* treeN = (Q*) args[1].addr;
//       string name = treeN->GetValue();
//       // cut blank from the front of the string
//       name = name.substr(1, name.size()-1);
//       MemoryRtreeObject<2>* tree =
//             (MemoryRtreeObject<2>*)catalog->getMMObject(name);
//       if(!tree) 
//         return 0;
//           
//       // mem(rel)
//       R* relN = (R*) args[2].addr;
//       MemoryRelObject* rel = getMemRel(relN);
//       if(!rel)
//         return 0;
// 
//       typename mapmatching::MmORelNetwork<T>::OEdgeAttrIndexes Indexes = 
//                                            GetOEdgeAttrIndexes<T>(args, 4);
// 
//       mapmatching::MmORelNetwork<T> Network(orel, tree, rel, Indexes);
// 
//       shared_ptr<mapmatch::MapMatchDataContainer> pContData = 
//                   GetMMDataFromTupleStream(args[3].addr, args, 4 + 9);
//                   // 9 OEdge-Attr-Indexes
//       // Do Map Matching
//       mapmatching::MmORelNetworkAdapter<T> NetworkAdapter(&Network);
//       mapmatch::MapMatchingMHT MapMatching(&NetworkAdapter, pContData); 
//       mapmatching::MmORelEdgeStreamCreator<T>* pCreator =
//        new mapmatching::MmORelEdgeStreamCreator<T>(s,NetworkAdapter,eMode);
// 
//       if (!MapMatching.DoMatch(pCreator)) {
//           //Error
//           delete pCreator;
//           pCreator = NULL;
//       }
// 
//       local.setAddr(pCreator);
//       return 0;
//     }
//     case REQUEST: {
//       if (pCreator == NULL) {
//         return CANCEL;
//       }
//       else {
//         result.addr = pCreator->GetNextTuple();
//         return result.addr ? YIELD : CANCEL;
//       }
//     }
//     case CLOSE: {
//       if (pCreator) {
//         delete pCreator;
//         local.addr = NULL;
//       }
//       return 0;
//     }
//     default: {
//       return 0;
//     }
//   }
// }
 

// template<class T, class R, class Q>
// int momapmatchmhtMPointVMT(Word* args,
//                            Word& result,
//                            int message,
//                            Word& local,
//                            Supplier in_xSupplier) {
//   
//     return momapmatchmhtMPointValueMap<T,R,Q>(
//                   args,
//                   result,
//                   message,
//                   local,
//                   in_xSupplier,
//                   mapmatching::MmORelEdgeStreamCreator<T>::MODE_EDGES);
// }
// 
// 
// template<class T, class R, class Q>
// int momapmatchmhtTextVMT(Word* args,
//                         Word& result,
//                         int message,
//                         Word& local,
//                         Supplier in_xSupplier) {
//   
//     return momapmatchmhtTextValueMap<T,R,Q>(
//                     args,
//                     result,
//                     message,
//                     local,
//                     in_xSupplier,
//                     mapmatching::MmORelEdgeStreamCreator<T>::MODE_EDGES);
// }


// template<class T, class R, class Q>
// int momapmatchmhtStreamVMT(Word* args,
//                            Word& result,
//                            int message,
//                            Word& local,
//                            Supplier in_xSupplier) {
//   
//     return momapmatchmhtStreamValueMap<T,R,Q>(
//                       args,
//                       result,
//                       message,
//                       local,
//                       in_xSupplier,
//                       mapmatching::MmORelEdgeStreamCreator<T>::MODE_EDGES);
// }


/*

7.38.4 Selection Function ~momapmatchmht~

*/
// int momapmatchmhtSelect(ListExpr args) {
//   
//     NList type(args);
//     if (type.length() == 4) {
//       
//         if (type.fourth().isSymbol(temporalalgebra::MPoint::BasicType())) 
//             return 0/* + offset*/;
//         
//         else if (type.fourth().isSymbol(FText::BasicType())) 
//             return 1 /*+ offset*/;
//         
//         else if (listutils::isTupleStream(type.fourth().listExpr())) 
//             return 2/* + offset*/;
//         
//         else 
//             return -1;        
//     }
//     else 
//       return -1;
// }


// ValueMapping momapmatchmhtVM[] = {
//   momapmatchmhtMPointVMT<CcInt,CcString,Mem>,
//   momapmatchmhtTextVMT<CcInt,CcString,Mem>,
//   momapmatchmhtStreamVMT<CcInt,CcString,Mem>
// };

 
/*

7.38.5 Description of operator ~momapmatchmht~

*/
// OperatorSpec momapmatchmhtSpec(
//     "mem(orel(tuple(x))) x mem(rtree) x mem(rel(tuple(x)) x var "
//     "-> stream(tuple(x))",
//     "momapmatchmht (_,_,_,_)",
//     "map matching with a main memory ordered relation "
//     "Result: Tuples of matched edges with timestamps",
//     "query momapmatchmht('Edges','EdgeIndex_Box_rtree', "
//     "'EdgeIndex','Trk_MapMatchTest.gpx') count"
// );


/*
7.38.5 Instance of operator ~momapmatchmht~

*/
// Operator momapmatchmhtOp (
//     "momapmatchmht",
//     momapmatchmhtSpec.getStr(),
//     3,
//     momapmatchmhtVM,
//     momapmatchmhtSelect,
//     momapmatchmhtTypeMap
// );




TypeConstructor MemoryRelObjectTC(
    MemoryRelObject::BasicType(),     // name of the type in SECONDO
    MemoryRelObject::Property,        // property function describing signature
    MemoryRelObject::Out, MemoryRelObject::In,          // out und in functions
    0, 0,                             // SaveToList, RestoreFromList functions
    // object creation and deletion create und delete
    MemoryRelObject::create,MemoryRelObject::Delete,
    MemoryRelObject::Open, MemoryRelObject::Save,        // object open, save
    MemoryRelObject::Close, MemoryRelObject::Clone,      // close and clone
    MemoryRelObject::Cast,                                // cast function
    MemoryRelObject::SizeOfObj,      // sizeof function
    MemoryRelObject::KindCheck);      // kind checking

GenTC<Mem> MemTC;


TypeConstructor MemoryORelObjectTC(
    MemoryORelObject::BasicType(),    // name of the type in SECONDO
    MemoryORelObject::Property,       // property function describing signature
    MemoryORelObject::Out, MemoryORelObject::In,        // out und in functions
    0, 0,                             // SaveToList, RestoreFromList functions
    // object creation and deletion create und delete
    MemoryORelObject::create,MemoryORelObject::Delete,
    MemoryORelObject::Open, MemoryORelObject::Save,        // object open, save
    MemoryORelObject::Close, MemoryORelObject::Clone,      // close and clone
    MemoryORelObject::Cast,                                // cast function
    MemoryORelObject::SizeOfObj,      // sizeof function
    MemoryORelObject::KindCheck);     // kind checking




TypeConstructor MPointerTC(
   MPointer::BasicType(),
   MPointer::Property,
   MPointer::Out, MPointer::In,
   0,0,
   MPointer::Create, MPointer::Delete,
   MPointer::Open, MPointer::Save,
   MPointer::Close, MPointer::Clone,
   MPointer::Cast,
   MPointer::SizeOf,
   MPointer::TypeCheck
);


/*

9 Operations on Attribute vectors

9.1 collect[_]mvector

stream(DATA) x string x bool -> bool;

*/
ListExpr collect_mvectorTM(ListExpr args){
  if(!nl->HasLength(args,3)){
    return listutils::typeError("3 arguments expected");
  }
  string err = "stream(DATA) x string x bool expected";
  if(  !Stream<Attribute>::checkType(nl->First(args))
     ||!CcString::checkType(nl->Second(args))
     ||!CcBool::checkType(nl->Third(args))){
    return listutils::typeError(err);
  }
  return listutils::basicSymbol<CcBool>();
}

int collect_mvectorVM(Word* args, Word& result, int message,
                Word& local, Supplier s){
    CcString* name = (CcString*) args[1].addr;
    CcBool* flob = (CcBool*) args[2].addr;
    result = qp->ResultStorage(s);
    CcBool* res = (CcBool*) result.addr;
    if(!name->IsDefined() || !flob->IsDefined()){
      res->SetDefined(false);
      return 0;
    }
    string n = name->GetValue();
    stringutils::trim(n);
    if(n.empty() || catalog->isMMObject(n)){
      res->Set(true,false);
      return 0;
    }
    string db =  SecondoSystem::GetInstance()->GetDatabaseName();
    ListExpr st = qp->GetType(qp->GetSon(s,0));
    st = nl->TwoElemList(listutils::basicSymbol<Mem>(),
            nl->TwoElemList(listutils::basicSymbol<MemoryVectorObject>(),
                            nl->Second(st)));
    string type = nl->ToString(nl->Second(st));
    MemoryVectorObject* v = new MemoryVectorObject(flob->GetValue(),db,type);
    Stream<Attribute> stream(args[0]);
    stream.open();
    Attribute* a;
    while((a = stream.request())){
        v->add(a);
        a->DeleteIfAllowed();
    }
    stream.close();
    catalog->insert(n,v);
    res->Set(true,true);
    return 0;                            
}

OperatorSpec collect_mvectorSpec(
  "stream(DATA) x string x bool -> bool",
  " _ collect_mvector[_,_]",
  "Collects a stream of attributes into a main memory"
  " vector. The second argument specifies the name for "
  " the main memory object. The third attribute specifies wether "
  " flobs should be loaded into memory too.",
  " query plz feed projecttransformstream[PLZ] collect_mvector[\"P\",TRUE] "
);

Operator collect_mvectorOp(
  "collect_mvector",
  collect_mvectorSpec.getStr(),
  collect_mvectorVM,
  Operator::SimpleSelect,
  collect_mvectorTM
);




/*
10 MPointer Creation

*/
ListExpr pwrapTM(ListExpr args){
  if(!nl->HasLength(args,1)){
    return listutils::typeError("pwrap needs one argument");
  }
  ListExpr arg = nl->First(args);
  if(!nl->HasLength(arg,2)){
    return listutils::typeError("internal error");
  }
  ListExpr t = nl->First(arg);
  if(!CcString::checkType(t)
     &&!Mem::checkType(t)){
    return listutils::typeError("string or mem(...) expected");
  }  
  ListExpr v = nl->Second(arg);
  if(nl->AtomType(v)!=StringType){
    return listutils::typeError("only constant objects allowed");
  }
  string n = nl->StringValue(v);
  MemoryObject* mo = catalog->getMMObject(n);
  if(mo==0){
    return listutils::typeError("No mm object with name " + n + " found");
  }
  ListExpr res = nl->TwoElemList(listutils::basicSymbol<MPointer>(),
                                 mo->getType());

  return res; 
}


template<class T>
int pwrapVMT(Word* args, Word& result, int message,
                Word& local, Supplier s){


  result = qp->ResultStorage(s);
  MPointer* mp = (MPointer*) result.addr;

  if((*mp)()){ // avaluate only once
    return 0;
  }
   
  T* t = (T*) args[0].addr;
  if(!t->IsDefined()){ // keep 0 pointer
    return 0;
  }
  MemoryObject* ptr = catalog->getMMObject(t->GetValue());
  mp->setPointer(ptr);
  return 0;
}

int pwrapSelect(ListExpr args){
  return CcString::checkType(nl->First(args))?0:1;
}

ValueMapping pwrapVM[] = {
  pwrapVMT<CcString>,
  pwrapVMT<Mem>
};

OperatorSpec pwrapSpec(
  "{string, mem} -> mpointer",
  "pwrap(_)",
  "converts a name into a memory pointer",
  "query pwrap(\"mten\")"
);

Operator pwrapOp(
  "pwrap",
  pwrapSpec.getStr(),
  2,
  pwrapVM,
  pwrapSelect,
  pwrapTM
);

/*
11 Variant of matchbelow

*/

ListExpr matchbelow2TM(ListExpr args){

  if(!nl->HasLength(args,5)){
    return listutils::typeError("wrong number of arguments, expected 5");
  }
  if(!checkUsesArgs(args)){
    return listutils::typeError("internal error");
  }
  ListExpr tree = nl->First(args); 
  string err;
  if(!getMemType(nl->First(tree), nl->Second(tree), tree, err, true)){
    return listutils::typeError(err);
  }
  // remove mem from tree description
  tree = nl->Second(tree);
  if(   !MemoryAVLObject::checkType(tree)
     && !MemoryTTreeObject::checkType(tree)){
     return listutils::typeError("first arg must be a main memory "
                                 "avl-tree or t-tree.");
  }
  bool isAVL = MemoryAVLObject::checkType(tree);
  ListExpr treeType = nl->Second(tree);

  ListExpr rel = nl->Second(args);
  if(!getMemType(nl->First(rel), nl->Second(rel), rel,err, true)){
     return listutils::typeError(err);
  }
  rel = nl->Second(rel);
  if(MemoryRelObject::checkType(rel)){
     return listutils::typeError("Second arg is not a memory relation");
  }

  ListExpr searchType = nl->First(nl->Third(args));

  if(!nl->Equal(searchType,treeType)){
     return listutils::typeError("search key type differs from tree type");
  }  

  ListExpr attrName = nl->First(nl->Fourth(args));

  if(nl->AtomType(attrName)!=SymbolType){
    return listutils::typeError("fourth arg must be an attribute name");
  }
  string aname = nl->SymbolValue(attrName);
  ListExpr attrType;
  ListExpr attrList = nl->Second(nl->Second(rel));
  int index = listutils::findAttribute(attrList,aname,attrType);
  if(!index){
    return listutils::typeError("attribute " + aname 
                                + " not found in relation");
  }
  ListExpr defaultType = nl->First(nl->Fifth(args));
  if(!nl->Equal(attrType,defaultType)){
    return listutils::typeError("default value type differs "
                                "from attribute type");
  }
  return nl->ThreeElemList(
               nl->SymbolAtom(Symbols::APPEND()),
               nl->TwoElemList(nl->BoolAtom(isAVL),
                               nl->IntAtom(index-1)),
               defaultType
             );
}


template<class T, class R>
int matchbelow2VMT(Word* args, Word& result, int message,
                Word& local, Supplier s){

   result = qp->ResultStorage(s);
   Attribute* res = (Attribute*) result.addr;

   R* relN = (R*) args[1].addr;
   MemoryRelObject* mro = getMemRel(relN);
   if(!mro){
     res->SetDefined(false);
     return 0;      
   }

   vector<Tuple*>* rel = mro->getmmrel();
   size_t size = rel->size();
   T* treeN = (T*) args[0].addr;
   bool isAVL = ((CcBool*) args[5].addr)->GetValue();
   
   Attribute* searchValue = (Attribute*) args[2].addr;
   Attribute* defaultValue = (Attribute*) args[4].addr;
   int attrIndex = ((CcInt*)args[6].addr)->GetValue();
   AttrIdPair const* hit=0; 

   AttrIdPair p(searchValue,size);
   if(isAVL){
      memAVLtree* tree = getAVLtree(treeN);    
      if(!tree){
        res->SetDefined(0);
        return 0;
      }
      hit = tree->GetNearestSmallerOrEqual(p);
   } else {
      MemoryTTreeObject*  tree = getTtree(treeN);
      if(!tree){
        res->SetDefined(0);
        return 0;
      } 
      hit = tree->gettree()->GetNearestSmallerOrEqual(p,0);
   }
   if(!hit){ // tree empty or searchvalue smaller than all entries
      res->CopyFrom(defaultValue);
      return 0;
   }
   TupleId tid = hit->getTid();
   if(tid>size || tid<1){
     res->SetDefined(false);
     return 0;
   } 
   Tuple* rt = rel->at(tid-1);
   if(!rt){ // deleted tuple
     res->SetDefined(false);
     return 0;
   }
   res->CopyFrom(rt->GetAttribute(attrIndex));

   return 0;
}

ValueMapping matchbelow2VM[] = {
  matchbelow2VMT<CcString,CcString>,
  matchbelow2VMT<CcString,Mem>,
  matchbelow2VMT<CcString,MPointer>,
  matchbelow2VMT<Mem,CcString>,
  matchbelow2VMT<Mem,Mem>,
  matchbelow2VMT<Mem,MPointer>,
  matchbelow2VMT<MPointer,CcString>,
  matchbelow2VMT<MPointer,Mem>,
  matchbelow2VMT<MPointer,MPointer>
};

int matchbelow2Select(ListExpr args){
   ListExpr tree = nl->First(args);
   int n1 =   CcString::checkType(tree)
            ? 0
            : Mem::checkType(tree)
              ?3
              :6;
   ListExpr rel = nl->Second(args);
   int n2 =   CcString::checkType(rel)
            ? 0
            : Mem::checkType(rel)
              ?1
              :2;
   return n1+n2;
}

OperatorSpec matchbelow2Spec(
   "{avltree, ttree} x mrel x T x Ident x V -> V ",
   "tree rel matchbelow2[searchV, attrName, defaultV]",
   "Retrieves from an avl tree the entry which is "
   "less or equal to searchV. From the returned tuple id, "
   "the tuple in the relation is retrieved and the "
   "attribute with given name is extracted. " 
   "If the avl tree does not returns a hit, the defaultV is "
   "used as the result. If the tuple id is not present in the "
   "relation, the result is undefined. ",
   "query avl rel matchbelow2[ 23, Name, \"Unknown\"] "
);


Operator matchbelow2Op(
  "matchbelow2",
  matchbelow2Spec.getStr(),
  9,
  matchbelow2VM,
  matchbelow2Select,
  matchbelow2TM
);



class MainMemory2Algebra : public Algebra {

    public:
        MainMemory2Algebra() : Algebra() {
          if(!catalog){
             catalog = new MemCatalog();
          }

/*

8.2 Registration of Types


*/
          AddTypeConstructor (&MPointerTC);
          MPointerTC.AssociateKind( Kind::SIMPLE() );


          AddTypeConstructor (&MemoryRelObjectTC);
          MemoryRelObjectTC.AssociateKind( Kind::SIMPLE() );

          AddTypeConstructor (&MemTC);
          MemTC.AssociateKind( Kind::DATA() );
          
          AddTypeConstructor (&MemoryORelObjectTC);
          MemoryORelObjectTC.AssociateKind( Kind::SIMPLE() );
/*
8.3 Registration of Operators

*/
          AddOperator (&memloadOp);
          memloadOp.SetUsesArgsInTypeMapping();
          AddOperator (&memloadflobOp);
          memloadflobOp.SetUsesArgsInTypeMapping();
          AddOperator (&meminitOp);
          meminitOp.SetUsesMemory();
          AddOperator (&mfeedOp);
          mfeedOp.SetUsesArgsInTypeMapping();
          AddOperator (&letmconsumeOp);
          AddOperator (&letmconsumeflobOp);
          AddOperator (&memdeleteOp);
          memdeleteOp.SetUsesArgsInTypeMapping();
          AddOperator (&memobjectOp);
          memobjectOp.SetUsesArgsInTypeMapping();
          AddOperator (&memgetcatalogOp);
          AddOperator (&memletOp);
          memletOp.SetUsesArgsInTypeMapping();
          AddOperator (&memletflobOp);
          memletflobOp.SetUsesArgsInTypeMapping();
          AddOperator (&memupdateOp);
          memupdateOp.SetUsesArgsInTypeMapping();
          AddOperator (&mcreateRtreeOp);
          mcreateRtreeOp.SetUsesArgsInTypeMapping();
          AddOperator (&mcreateRtree2Op);
          AddOperator (&memsizeOp);
          AddOperator (&memclearOp);
          AddOperator (&meminsertOp);
          meminsertOp.SetUsesArgsInTypeMapping();
          AddOperator (&mwindowintersectsOp);
          mwindowintersectsOp.SetUsesArgsInTypeMapping();
          AddOperator (&mwindowintersectsSOp);
          mwindowintersectsSOp.SetUsesArgsInTypeMapping();
          AddOperator (&mconsumeOp);
          AddOperator (&mcreateAVLtreeOp);
          mcreateAVLtreeOp.SetUsesArgsInTypeMapping();

          AddOperator (&mcreateAVLtree2Op);
          AddOperator (&mexactmatchOp);
          mexactmatchOp.SetUsesArgsInTypeMapping();

          AddOperator (&mrangeOp);
          mrangeOp.SetUsesArgsInTypeMapping();
          AddOperator (&matchbelowOp);
          matchbelowOp.SetUsesArgsInTypeMapping();

          AddOperator (&matchbelow2Op);
          matchbelow2Op.SetUsesArgsInTypeMapping();



          AddOperator(&mcreateMtree2Op);
          AddOperator(&mdistRange2Op);
          mdistRange2Op.SetUsesArgsInTypeMapping();
          AddOperator(&mdistScan2Op);
          mdistScan2Op.SetUsesArgsInTypeMapping();

          AddOperator(&mcreateMtreeOp);
          mcreateMtreeOp.SetUsesArgsInTypeMapping();
          AddOperator(&mdistRangeOp);
          mdistRangeOp.SetUsesArgsInTypeMapping();
          AddOperator(&mdistScanOp);
          mdistScanOp.SetUsesArgsInTypeMapping();

          AddOperator(&mexactmatchSOp);
          mexactmatchSOp.SetUsesArgsInTypeMapping();
          AddOperator(&mrangeSOp);
          mrangeSOp.SetUsesArgsInTypeMapping();
          AddOperator(&matchbelowSOp);
          matchbelowSOp.SetUsesArgsInTypeMapping();

          AddOperator(&gettuplesOp);
          gettuplesOp.SetUsesArgsInTypeMapping();
          
  ////////////////////// MainMemory2Algebra////////////////////////////
          
          AddOperator(&mwrapOp);
          mwrapOp.SetUsesArgsInTypeMapping();
          AddOperator(&MTUPLEOp);
          MTUPLEOp.SetUsesArgsInTypeMapping();
          AddOperator(&MTUPLE2Op);
          MTUPLE2Op.SetUsesArgsInTypeMapping(); 
          
          AddOperator(&mcreatettreeOp);
          mcreatettreeOp.SetUsesArgsInTypeMapping();
          AddOperator(&minsertttreeOp);
          minsertttreeOp.SetUsesArgsInTypeMapping();
          AddOperator(&mdeletettreeOp);
          mdeletettreeOp.SetUsesArgsInTypeMapping();
          AddOperator(&minsertavltreeOp);
          minsertavltreeOp.SetUsesArgsInTypeMapping();
          AddOperator(&mdeleteavltreeOp);
          mdeleteavltreeOp.SetUsesArgsInTypeMapping();
          
          AddOperator(&mcreateinsertrelOp);
          mcreateinsertrelOp.SetUsesArgsInTypeMapping();
          AddOperator(&minsertOp);
          minsertOp.SetUsesArgsInTypeMapping();
          AddOperator(&minsertsaveOp);
          minsertsaveOp.SetUsesArgsInTypeMapping();
          AddOperator(&minserttupleOp);
          minserttupleOp.SetUsesArgsInTypeMapping();
          AddOperator(&minserttuplesaveOp);
          minserttuplesaveOp.SetUsesArgsInTypeMapping();
          
          AddOperator(&mcreatedeleterelOp);
          mcreatedeleterelOp.SetUsesArgsInTypeMapping();
          AddOperator(&mdeleteOp);
          mdeleteOp.SetUsesArgsInTypeMapping();
          AddOperator(&mdeletesaveOp);
          mdeletesaveOp.SetUsesArgsInTypeMapping();
          AddOperator(&mdeletebyidOp);
          mdeletebyidOp.SetUsesArgsInTypeMapping();

          AddOperator(&mdeletedirectOp);
          mdeletedirectOp.SetUsesArgsInTypeMapping();

          AddOperator(&mdeletedirectsaveOp);
          mdeletedirectsaveOp.SetUsesArgsInTypeMapping();

          AddOperator(&mcreateupdaterelOp);
          mcreateupdaterelOp.SetUsesArgsInTypeMapping();
          AddOperator(&mupdateNOp);
          mupdateNOp.SetUsesArgsInTypeMapping();
          AddOperator(&mupdatesaveOp);
          mupdatesaveOp.SetUsesArgsInTypeMapping();
          AddOperator(&mupdatebyidOp);
          mupdatebyidOp.SetUsesArgsInTypeMapping();
          
          AddOperator(&moinsertOp);
          moinsertOp.SetUsesArgsInTypeMapping();
          AddOperator(&modeleteOp);
          modeleteOp.SetUsesArgsInTypeMapping();
          AddOperator(&moconsumeOp);
          AddOperator (&letmoconsumeOp);
          AddOperator (&letmoconsumeflobOp);
          AddOperator(&morangeOp);
          morangeOp.SetUsesArgsInTypeMapping();
          AddOperator(&moleftrangeOp);
          moleftrangeOp.SetUsesArgsInTypeMapping();
          AddOperator(&morightrangeOp);
          morightrangeOp.SetUsesArgsInTypeMapping();
          
          AddOperator(&moshortestpathdOp);
          moshortestpathdOp.SetUsesArgsInTypeMapping();
          AddOperator(&moshortestpathaOp);
          moshortestpathaOp.SetUsesArgsInTypeMapping();
          
          AddOperator(&moconnectedcomponentsOp);
          moconnectedcomponentsOp.SetUsesArgsInTypeMapping();
          
          AddOperator(&mquicksortOp);
          mquicksortOp.SetUsesArgsInTypeMapping();
          AddOperator(&mquicksortbyOp);
          mquicksortbyOp.SetUsesArgsInTypeMapping();
          
          AddOperator (&memgletOp);
          memgletOp.SetUsesArgsInTypeMapping();
          AddOperator (&memgletflobOp);
          memgletflobOp.SetUsesArgsInTypeMapping();
          
          AddOperator(&mgshortestpathdOp);
          mgshortestpathdOp.SetUsesArgsInTypeMapping();       
          AddOperator(&mgshortestpathaOp);
          mgshortestpathaOp.SetUsesArgsInTypeMapping();
          AddOperator(&mgconnectedcomponentsOp);
          mgconnectedcomponentsOp.SetUsesArgsInTypeMapping();
        
//           AddOperator(&momapmatchmhtOp);
//           momapmatchmhtOp.SetUsesArgsInTypeMapping(); 


          AddOperator(&collect_mvectorOp);

          AddOperator(&pwrapOp);
          pwrapOp.SetUsesArgsInTypeMapping();

        }
        
        ~MainMemory2Algebra() {
            if(catalog){
              delete catalog; 
              catalog = 0;
            }
        };
};

} // end of namespace mmalgebra

/*
9 Initialization

Each algebra module needs an initialization function. The algebra manager
has a reference to this function if this algebra is included in the list
of required algebras, thus forcing the linker to include this module.

The algebra manager invokes this function to get a reference to the instance
of the algebra class and to provide references to the global nested list
container (used to store constructor, type, operator and object information)
and to the query processor.

The function has a C interface to make it possible to load the algebra
dynamically at runtime (if it is built as a dynamic link library). The name
of the initialization function defines the name of the algebra module. By
convention it must start with "Initialize<AlgebraName>".

To link the algebra together with the system you must create an
entry in the file "makefile.algebra" and to define an algebra ID in the
file "Algebras/Management/AlgebraList.i.cfg".

*/

extern "C"
Algebra*
InitializeMainMemory2Algebra(NestedList* nlRef, QueryProcessor* qpRef)
{
  return (new mm2algebra::MainMemory2Algebra);
}

