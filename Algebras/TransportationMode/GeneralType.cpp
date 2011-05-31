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

[1] Header File of the Transportation Mode Algebra

Jan, 2011 Jianqiu xu

[TOC]

1 Overview

2 Defines and includes

*/


#include "Algebra.h"

#include "NestedList.h"

#include "QueryProcessor.h"
#include "RTreeAlgebra.h"
#include "BTreeAlgebra.h"
#include "TemporalAlgebra.h"
#include "StandardTypes.h"
#include "LogMsg.h"
#include "NList.h"
#include "RelationAlgebra.h"
#include "ListUtils.h"
#include "NetworkAlgebra.h"
#include "FTextAlgebra.h"
#include <fstream>
#include "GeneralType.h"
#include "PaveGraph.h"
#include "BusNetwork.h"
#include "Indoor.h"

///////////////////////////random number generator//////////////////////////
unsigned long GetRandom()
{
    /////// only works in linux /////////////////////////////////////
  //  struct timeval tval;
//  struct timezone tzone;
//  gettimeofday(&tval, &tzone);
//  srand48(tval.tv_sec);
// return lrand48();  
  ///////////////////////////////////////////////////////////////////
   return gsl_random.NextInt();  
}


//////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////
//////////////////// Data Type: IORef ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

ListExpr IORefProperty()
{
  return (nl->TwoElemList(
            nl->FourElemList(nl->StringAtom("Signature"),
                       nl->StringAtom("Example Type List"),
           nl->StringAtom("List Rep"),
           nl->StringAtom("Example List")),
            nl->FourElemList(nl->StringAtom("-> DATA"),
                       nl->StringAtom("ioref"),
           nl->StringAtom("(<oid, symbol>) (int string)"),
           nl->StringAtom("((1 \"mpptn\" ))"))));
}


/*
Output  (oid, symbol) 

*/

ListExpr OutIORef( ListExpr typeInfo, Word value )
{
//  cout<<"OutIORef"<<endl; 
  IORef* ref = (IORef*)(value.addr);
  if(!ref->IsDefined()){
    return nl->SymbolAtom("undef");
  }

  if( ref->IsEmpty() ){
    return nl->TheEmptyList();
  }
  return nl->TwoElemList(nl->IntAtom(ref->GetOid()), 
                         nl->StringAtom(GetSymbolStr(ref->GetLabel())));
}


/*
In function

*/
Word InIORef( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct )
{
  if(nl->IsEqual(instance,"undef")) {
      IORef* ref = new IORef();
      ref->SetDefined(false);
      correct=true;
      return SetWord(Address(ref));
  }
  
  
//  cout<<"length "<<nl->ListLength(instance)<<endl;

  if( !nl->IsAtom( instance ) ){
    IORef* ref = new IORef();
    if(nl->ListLength(instance) != 2){
      cout<<"length should be 2"<<endl; 
      correct = false;
      return SetWord(Address(0));
    }
    ListExpr first = nl->First(instance);
    if(!nl->IsAtom(first) || nl->AtomType(first) != IntType){
      cout<< "ioref(): oid must be int type"<<endl;
      correct = false;
      return SetWord(Address(0));
    }
    unsigned int oid = nl->IntValue(first);
    
    ListExpr second = nl->Second(instance);
    if(!nl->IsAtom(second) || nl->AtomType(second) != StringType){
      cout<< "ioref(): parameter2 must be string type"<<endl;
      correct = false;
      return SetWord(Address(0));
    }
    string s = nl->StringValue(second);
   ////////////////very important /////////////////////////////
    correct = true; 
  ///////////////////////////////////////////////////////////
    ref->SetValue(oid, s);
    return SetWord(ref);
  }

  correct = false;
  return SetWord(Address(0));
}



/*
Open an reference object 

*/
bool OpenIORef(SmiRecord& valueRecord, size_t& offset,
                 const ListExpr typeInfo, Word& value)
{
//  cout<<"OpenGenLoc()"<<endl; 

  IORef* ref = (IORef*)Attribute::Open(valueRecord, offset, typeInfo);
  value = SetWord(ref);
  return true; 
}

/*
Save an reference object 

*/
bool SaveIORef(SmiRecord& valueRecord, size_t& offset,
                 const ListExpr typeInfo, Word& value)
{
//  cout<<"SaveIORef"<<endl; 
  IORef* ref = (IORef*)value.addr; 
  Attribute::Save(valueRecord, offset, typeInfo, ref);
  return true; 
}

Word CreateIORef(const ListExpr typeInfo)
{
// cout<<"CreateIORef()"<<endl;
  return SetWord (new IORef(false, 0, 0));
}


void DeleteIORef(const ListExpr typeInfo, Word& w)
{
// cout<<"DeleteIORef()"<<endl;
  IORef* ref = (IORef*)w.addr;
  delete ref;
   w.addr = NULL;
}


void CloseIORef( const ListExpr typeInfo, Word& w )
{
//  cout<<"CloseIORef"<<endl; 
  ((IORef*)w.addr)->DeleteIfAllowed();
  w.addr = 0;
}

Word CloneIORef( const ListExpr typeInfo, const Word& w )
{
//  cout<<"CloneGenLoc"<<endl; 
  return SetWord( new IORef( *((IORef*)w.addr) ) );
}

int SizeOfIORef()
{
//  cout<<"SizeOfIORef"<<endl; 
  return sizeof(IORef);
}

bool CheckIORef( ListExpr type, ListExpr& errorInfo )
{
//  cout<<"CheckIORef"<<endl; 
  return (nl->IsEqual( type, "ioref" ));
}
/////////////////////////////////////////////////////////////////////////////
//////////////////////Data Type: GenLoc//////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

ListExpr GenLocProperty()
{
  return (nl->TwoElemList(
            nl->FourElemList(nl->StringAtom("Signature"),
                       nl->StringAtom("Example Type List"),
           nl->StringAtom("List Rep"),
           nl->StringAtom("Example List")),
            nl->FourElemList(nl->StringAtom("-> DATA"),
                       nl->StringAtom("genloc"),
           nl->StringAtom("(<oid,loc>) (int (real, real))"),
           nl->StringAtom("((1 (10.0 1.0 )))"))));
}

/*
Output  (oid, loc)

*/

ListExpr OutGenLoc( ListExpr typeInfo, Word value )
{
//  cout<<"OutGenLoc"<<endl; 
  GenLoc* genl = (GenLoc*)(value.addr);
  if(!genl->IsDefined()){
    return nl->SymbolAtom("undef");
  }

  if( genl->IsEmpty() ){
    return nl->TheEmptyList();
  }
  
  if(genl->GetOid() > 0){
      Loc loc = genl->GetLoc();
      if(loc.loc1 >= 0.0 && loc.loc2 >= 0.0){
          ListExpr loc_list = nl->TwoElemList(
                    nl->RealAtom(loc.loc1),
                    nl->RealAtom(loc.loc2));
      return nl->TwoElemList(nl->IntAtom(genl->GetOid()), loc_list);
      }else if(loc.loc1 >= 0.0 && loc.loc2 < 0.0){
        ListExpr loc_list = nl->OneElemList(nl->RealAtom(loc.loc1));
        return nl->TwoElemList(nl->IntAtom(genl->GetOid()), loc_list);
      }else if(loc.loc1 < 0.0 && loc.loc2 < 0.0){
        return nl->OneElemList(nl->IntAtom(genl->GetOid()));
      }else
      return nl->TheEmptyList(); 
  }else{  //free space oid = 0 
          Loc loc = genl->GetLoc();
          ListExpr loc_list = nl->TwoElemList(
                    nl->RealAtom(loc.loc1),
                    nl->RealAtom(loc.loc2));
          return nl->OneElemList(loc_list);
  }

}


/*
In function

*/
Word InGenLoc( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct )
{
  if(nl->IsEqual(instance,"undef")) {
      GenLoc* genl = new GenLoc();
      genl->SetDefined(false);
      correct=true;
      return SetWord(Address(genl));
  }
  
  
//  cout<<"length "<<nl->ListLength(instance)<<endl;

  if( !nl->IsAtom( instance ) ){
    GenLoc* genl = new GenLoc();
    if(nl->ListLength(instance) != 2){
      cout<<"length should be 2"<<endl; 
      correct = false;
      return SetWord(Address(0));
    }
    ListExpr first = nl->First(instance);
    if(!nl->IsAtom(first) || nl->AtomType(first) != IntType){
      cout<< "genloc(): oid must be int type"<<endl;
      correct = false;
      return SetWord(Address(0));
    }
    unsigned int oid = nl->IntValue(first);
    ListExpr second = nl->Second(instance);
    if(nl->ListLength(second) != 2){
      cout<<"length should be 2"<<endl; 
      correct = false;
      return SetWord(Address(0));
    }
    
    ListExpr list_l1 = nl->First(second);
    if(!nl->IsAtom(list_l1) || nl->AtomType(list_l1) != RealType){
      cout<< "genloc(): loc.loc1 must be real type"<<endl;
      correct = false;
      return SetWord(Address(0));
    }
    double loc1 = nl->RealValue(list_l1);
    
    ListExpr list_l2 = nl->Second(second);
    if(!nl->IsAtom(list_l2) || nl->AtomType(list_l2) != RealType){
      cout<< "genloc(): loc.loc2 must be real type"<<endl;
      correct = false;
      return SetWord(Address(0));
    }
    double loc2 = nl->RealValue(list_l2);
    Loc loc(loc1,loc2);
   ////////////////very important /////////////////////////////
    correct = true; 
  ///////////////////////////////////////////////////////////
    genl->SetValue(oid, loc);
    return SetWord(genl);
  }

  correct = false;
  return SetWord(Address(0));
}



/*
Open a GenLoc object 

*/
bool OpenGenLoc(SmiRecord& valueRecord, size_t& offset,
                 const ListExpr typeInfo, Word& value)
{
//  cout<<"OpenGenLoc()"<<endl; 

  GenLoc* genl = (GenLoc*)Attribute::Open(valueRecord, offset, typeInfo);
  value = SetWord(genl);
  return true; 
}

/*
Save a GenLoc object 

*/
bool SaveGenLoc(SmiRecord& valueRecord, size_t& offset,
                 const ListExpr typeInfo, Word& value)
{
//  cout<<"SaveGenRange"<<endl; 
  GenLoc* genl = (GenLoc*)value.addr; 
  Attribute::Save(valueRecord, offset, typeInfo, genl);
  return true; 
  
}

Word CreateGenLoc(const ListExpr typeInfo)
{
// cout<<"CreateLoc()"<<endl;
  return SetWord (new GenLoc(0));
}


void DeleteGenLoc(const ListExpr typeInfo, Word& w)
{
// cout<<"DeleteGenLoc()"<<endl;
  GenLoc* genl = (GenLoc*)w.addr;
  delete genl;
   w.addr = NULL;
}


void CloseGenLoc( const ListExpr typeInfo, Word& w )
{
//  cout<<"CloseGenLoc"<<endl; 
  ((GenLoc*)w.addr)->DeleteIfAllowed();
  w.addr = 0;
}

Word CloneGenLoc( const ListExpr typeInfo, const Word& w )
{
//  cout<<"CloneGenLoc"<<endl; 
  return SetWord( new GenLoc( *((GenLoc*)w.addr) ) );
}

int SizeOfGenLoc()
{
//  cout<<"SizeOfGenLoc"<<endl; 
  return sizeof(GenLoc);
}

bool CheckGenLoc( ListExpr type, ListExpr& errorInfo )
{
//  cout<<"CheckGenLoc"<<endl; 
  return (nl->IsEqual( type, "genloc" ));
}

ostream& operator<<(ostream& o, const GenLoc& gloc)
{
  if(gloc.IsDefined()){
    o<<"oid "<<gloc.GetOid()<<" ("<<gloc.GetLoc().loc1
     <<" "<<gloc.GetLoc().loc2<<" )"; 
  }else
    o<<" undef";
  return o;

}

////////////////////////////////////////////////////////////////////////////
/////////////////////////Data Type: GenRange///////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/*
Add new elements into the array (oid,l)

*/
void GenRange::Add(unsigned int id, Line* l, int s)
{
  GenRangeElem* grelem = new GenRangeElem(id, seglist.Size(), l->Size(), s);
  elemlist.Append(*grelem);
  delete grelem; 
  for(int i = 0;i < l->Size();i++){
    HalfSegment hs;
    l->Get(i, hs);
    seglist.Append(hs);
  }
}

/*
the length of a genrange object 
each halfsegment is stored twice: left dom point and right dom point 

*/
double GenRange::Length()
{
  double l = 0; 
  for(int i = 0;i < SegSize();i++){
    HalfSegment hs;
    GetSeg(i, hs); 
    l += hs.Length(); 
  }
  return l/2.0; 
}

/*
Output a list of elements (oid, l)

*/

ListExpr OutGenRange( ListExpr typeInfo, Word value )
{
//  cout<<"OutGenRange"<<endl; 
  GenRange* gen_range = (GenRange*)(value.addr);
  if(!gen_range->IsDefined()){
    return nl->SymbolAtom("undef");
  }

  if( gen_range->IsEmpty() ){
    return nl->TheEmptyList();
  }

  ListExpr result = nl->TheEmptyList();
  ListExpr last = result;
  bool first = true;
  ListExpr sb_list; 
  for( int i = 0; i < gen_range->Size(); i++ ){
    Line l(0);
    GenRangeElem grelem;
    gen_range->Get( i, grelem, l);

    sb_list = nl->ThreeElemList(nl->IntAtom(grelem.oid),
                  OutLine(nl->TheEmptyList(), SetWord(&l)),
                                nl->StringAtom(GetTMStr(grelem.tm)));

    if( first == true ){
        result = nl->OneElemList( sb_list );
        last = result;
        first = false;
    }else
      last = nl->Append( last, sb_list);
  }
  return result;
}

/*
In function

*/
Word InGenRange( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct )
{
  if(nl->IsEqual(instance,"undef")) {
      GenRange* gr = new GenRange();
      gr->SetDefined(false);
      correct=true;
      return SetWord(Address(gr));
  }
  
  GenRange* gr = new GenRange(0);
//  cout<<"length "<<nl->ListLength(instance)<<endl;

  ListExpr first;
  ListExpr rest = instance;
  if( !nl->IsAtom( instance ) ){
     while( !nl->IsEmpty( rest ) ){
        first = nl->First( rest );
        rest = nl->Rest( rest );
        assert(nl->ListLength(first) == 3);
        ListExpr oid_list = nl->First(first);
        ListExpr line_list = nl->Second(first);
        Line* l = (Line*)(InLine(typeInfo,line_list,errorPos,
                                 errorInfo,correct).addr);
        ListExpr tm_list = nl->Third(first);
        unsigned int oid = nl->IntValue(oid_list);
        string str_tm = nl->StringValue(tm_list);
        
//        assert(oid > 0 && l->Size() > 0 && GetTM(str_tm) >= 0);
        if(oid <= 0){
          cout<<"oid invalid"<<endl;
          correct = false;
          return SetWord(Address(0));
        }
        if(l->Size() == 0){
          cout<<"line empty"<<endl;
          correct = false;
          return SetWord(Address(0));
        }
        if(GetTM(str_tm) < 0 ){
          cout<<"this transportation mode does not exist"<<endl;
          correct = false; 
          return SetWord(Address(0));
        }
        gr->Add(oid, l, GetTM(str_tm));
    }
  }
  ////////////////very important /////////////////////////////
  correct = true; 
  ///////////////////////////////////////////////////////////
  return SetWord(gr);

}

ListExpr GenRangeProperty()
{
  return (nl->TwoElemList(
            nl->FourElemList(nl->StringAtom("Signature"),
                       nl->StringAtom("Example Type List"),
           nl->StringAtom("List Rep"),
           nl->StringAtom("Example List")),
            nl->FourElemList(nl->StringAtom("-> DATA"),
                       nl->StringAtom("genrange"),
           nl->StringAtom("(<subrange>*) ((int, line)(int , line))"),
           nl->StringAtom("((1 [const line value ((10 1 5 3))]))"))));
}


/*
Open a GenRange object 

*/
bool OpenGenRange(SmiRecord& valueRecord, size_t& offset,
                 const ListExpr typeInfo, Word& value)
{
//  cout<<"OpenGenRange()"<<endl; 

  GenRange* gr = (GenRange*)Attribute::Open(valueRecord, offset, typeInfo);
  value = SetWord(gr);
  return true; 
  
}

/*
Save a GenRange object 

*/
bool SaveGenRange(SmiRecord& valueRecord, size_t& offset,
                 const ListExpr typeInfo, Word& value)
{
//  cout<<"SaveGenRange"<<endl; 
  GenRange* gr = (GenRange*)value.addr; 
  Attribute::Save(valueRecord, offset, typeInfo, gr);
  return true; 
  
}

Word CreateGenRange(const ListExpr typeInfo)
{
// cout<<"CreateGenRange()"<<endl;
  return SetWord (new GenRange(0));
}


void DeleteGenRange(const ListExpr typeInfo, Word& w)
{
//  cout<<"DeleteGenRange()"<<endl;
  GenRange* gr = (GenRange*)w.addr;
  delete gr;
   w.addr = NULL;
}


void CloseGenRange( const ListExpr typeInfo, Word& w )
{
//  cout<<"CloseGenRange"<<endl; 
  ((GenRange*)w.addr)->DeleteIfAllowed();
  w.addr = 0;
}

Word CloneGenRange( const ListExpr typeInfo, const Word& w )
{
//  cout<<"CloneGenRange"<<endl; 
  return SetWord( new GenRange( *((GenRange*)w.addr) ) );
}

int SizeOfGenRange()
{
//  cout<<"SizeOfGenRange"<<endl; 
  return sizeof(GenRange);
}

bool CheckGenRange( ListExpr type, ListExpr& errorInfo )
{
//  cout<<"CheckGenRange"<<endl; 
  return (nl->IsEqual( type, "genrange" ));
}

/////////////////////////////////////////////////////////////////////////////
//////////////////////Data Type: UGenLoc//////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

ListExpr UGenLocProperty()
{
  return (nl->TwoElemList(
            nl->FourElemList(nl->StringAtom("Signature"),
                       nl->StringAtom("Example Type List"),
           nl->StringAtom("List Rep"),
           nl->StringAtom("Example List")),
            nl->FourElemList(nl->StringAtom("-> DATA"),
                       nl->StringAtom("ugenloc"),
           nl->StringAtom("(<interval,gloc,gloc,tm>)"),
      nl->StringAtom("((interval (1 (10.0 1.0))(1 (12.0 3.0)) Indoor))"))));
}


/*
Output  (interval, genloc1, genloc2, tm)

*/

ListExpr OutUGenLoc( ListExpr typeInfo, Word value )
{
//  cout<<"OutUGenLoc"<<endl; 
  UGenLoc* ugenloc = (UGenLoc*)(value.addr);
  if(ugenloc->IsDefined() == false){
    return nl->SymbolAtom("undef");
  }

  if( ugenloc->IsEmpty() ){
    return nl->TheEmptyList();
  }
    ListExpr timeintervalList = nl->FourElemList(
          OutDateTime( nl->TheEmptyList(),
          SetWord(&ugenloc->timeInterval.start) ),
          OutDateTime( nl->TheEmptyList(), 
                       SetWord(&ugenloc->timeInterval.end) ),
          nl->BoolAtom( ugenloc->timeInterval.lc ),
          nl->BoolAtom( ugenloc->timeInterval.rc)); 
    ListExpr genloc1 = OutGenLoc(nl->TheEmptyList(), &(ugenloc->gloc1));
    ListExpr genloc2 = OutGenLoc(nl->TheEmptyList(), &(ugenloc->gloc2));
    ListExpr tm = nl->StringAtom(GetTMStr(ugenloc->tm)); 


    return nl->FourElemList(timeintervalList,genloc1,genloc2,tm); 
}


/*
In function

*/
Word InUGenLoc( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct )
{
  string errmsg;
  if ( nl->ListLength( instance ) == 4 ){
    ListExpr first = nl->First( instance );

    if( nl->ListLength( first ) == 4 &&
        nl->IsAtom( nl->Third( first ) ) &&
        nl->AtomType( nl->Third( first ) ) == BoolType &&
        nl->IsAtom( nl->Fourth( first ) ) &&
        nl->AtomType( nl->Fourth( first ) ) == BoolType ){

       correct = true;
       Instant *start = (Instant *)InInstant( nl->TheEmptyList(),
       nl->First( first ),
        errorPos, errorInfo, correct ).addr;

      if( !correct || !start->IsDefined() ){
        errmsg = "InUGneLoc(): Error in first instant (Must be defined!).";
        errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
        delete start;
        return SetWord( Address(0) );
      }

      Instant *end = (Instant *)InInstant( nl->TheEmptyList(),
       nl->Second( first ),
                                           errorPos, errorInfo, correct ).addr;

      if( !correct  || !end->IsDefined() )
      {
        errmsg = "InUGneLoc(): Error in second instant (Must be defined!).";
        errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
        delete start;
        delete end;
        return SetWord( Address(0) );
      }

      Interval<Instant> tinterval( *start, *end,
                                   nl->BoolValue( nl->Third( first ) ),
                                   nl->BoolValue( nl->Fourth( first ) ) );
      delete start;
      delete end;

      correct = tinterval.IsValid();
      if (!correct)
        {
          errmsg = "InUGneLoc(): Non valid time interval.";
          errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
          return SetWord( Address(0) );
        }

      //////////////////////////////////////////////////////////////////
      ListExpr second = nl->Second( instance );
      if(nl->ListLength(second) != 2){
        errmsg = "InUGneLoc(): the length for GenLoc should be 2.";
        errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
        return SetWord( Address(0) );
      }
      
      bool gloc1_correct; 
      GenLoc* gloc1 = (GenLoc*)InGenLoc(typeInfo, second, 
                                        errorPos,errorInfo, gloc1_correct).addr;
      if(gloc1_correct == false){
          errmsg = "InUGneLoc(): Non correct first location.";
          errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
          return SetWord( Address(0) );
      }

      /////////////////////////////////////////////////////////////////////
      ListExpr third = nl->Third(instance); 
      if(nl->ListLength(third) != 2){
        errmsg = "InUGneLoc(): the length for GenLoc should be 2.";
        errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
        return SetWord( Address(0) );
      }
      bool gloc2_correct; 
      GenLoc* gloc2 = (GenLoc*)InGenLoc(typeInfo, third, 
                                        errorPos,errorInfo, gloc2_correct).addr;
      if(gloc2_correct == false){
          errmsg = "InUGneLoc(): Non correct second location.";
          errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
          return SetWord( Address(0) );
      }

      /////////////////////////////////////////////////////////////////////
      if(gloc1->GetOid() != gloc2->GetOid()){
         errmsg = "InUGneLoc(): two oid should be the same.";
         errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
         return SetWord( Address(0) );
      }

      ListExpr fourth = nl->Fourth(instance);

      if(nl->AtomType( fourth ) == StringType){
        string str_tm = nl->StringValue(fourth);
        int tm = GetTM(str_tm); 

//        cout<<tinterval<<" "<<*gloc1<<" "<<*gloc2<<" "<<str_tm<<endl; 

        UGenLoc *ugenloc = new UGenLoc( tinterval, *gloc1, *gloc2, tm);

        correct = ugenloc->IsValid();
        if( correct )
          return SetWord( ugenloc );

        errmsg = "InUGenLoc(): Error in start/end point.";
        errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
        delete ugenloc;
      }
    }
  }else if ( nl->IsAtom( instance ) && nl->AtomType( instance ) == SymbolType
            && nl->SymbolValue( instance ) == "undef" ){
      UGenLoc *ugenloc = new UGenLoc(false);
      ugenloc->timeInterval=
                Interval<DateTime>(DateTime(instanttype),
                           DateTime(instanttype),true,true);
      correct = ugenloc->timeInterval.IsValid();
      if ( correct )
        return (SetWord( ugenloc ));
  }
  errmsg = "InGenLoc(): Error in representation.";
  errorInfo = nl->Append(errorInfo, nl->StringAtom(errmsg));
  correct = false;
  return SetWord( Address(0) );
}

/*
Open a UGenLoc object 

*/
bool OpenUGenLoc(SmiRecord& valueRecord, size_t& offset,
                 const ListExpr typeInfo, Word& value)
{
//  cout<<"OpenGenLoc()"<<endl; 

  UGenLoc* genl = (UGenLoc*)Attribute::Open(valueRecord, offset, typeInfo);
  value = SetWord(genl);
  return true; 
}

/*
Save a UGenLoc object 

*/
bool SaveUGenLoc(SmiRecord& valueRecord, size_t& offset,
                 const ListExpr typeInfo, Word& value)
{
//  cout<<"SaveUGenLoc"<<endl; 
  UGenLoc* genl = (UGenLoc*)value.addr; 
  Attribute::Save(valueRecord, offset, typeInfo, genl);
  return true; 
  
}

Word CreateUGenLoc(const ListExpr typeInfo)
{
// cout<<"CreateUGenLoc()"<<endl;
  return SetWord (new UGenLoc(0));
}


void DeleteUGenLoc(const ListExpr typeInfo, Word& w)
{
// cout<<"DeleteUGenLoc()"<<endl;
  UGenLoc* ugenl = (UGenLoc*)w.addr;
  delete ugenl;
   w.addr = NULL;
}


void CloseUGenLoc( const ListExpr typeInfo, Word& w )
{
//  cout<<"CloseUGenLoc"<<endl; 
  ((UGenLoc*)w.addr)->DeleteIfAllowed();
  w.addr = 0;
}

Word CloneUGenLoc( const ListExpr typeInfo, const Word& w )
{
//  cout<<"CloneUGenLoc"<<endl; 
  return SetWord( new UGenLoc( *((UGenLoc*)w.addr) ) );
}

int SizeOfUGenLoc()
{
//  cout<<"SizeOfUGenLoc"<<endl; 
  return sizeof(UGenLoc);
}

bool CheckUGenLoc( ListExpr type, ListExpr& errorInfo )
{
//  cout<<"CheckUGenLoc"<<endl; 
  return (nl->IsEqual( type, "ugenloc" ));
}

/*
interporlation function for the location 

*/
void UGenLoc::TemporalFunction( const Instant& t, GenLoc& result,
                               bool ignoreLimits ) const
{
  if( !IsDefined() ||
      !t.IsDefined() ||
      (!timeInterval.Contains( t ) && !ignoreLimits) )
    {
      result.SetDefined(false);
    }
  else if( t == timeInterval.start )
    {
      result = gloc1;
      result.SetDefined(true);
    }
  else if( t == timeInterval.end )
    {
      result = gloc2;
      result.SetDefined(true);
    }
  else
    {
      Instant t0 = timeInterval.start;
      Instant t1 = timeInterval.end;

      Point p0(gloc1.GetLoc().loc1, gloc1.GetLoc().loc2);
      Point p1(gloc2.GetLoc().loc1, gloc2.GetLoc().loc2);

      double x = (p1.GetX() - p0.GetX()) * ((t - t0) / (t1 - t0)) + p0.GetX();
      double y = (p1.GetY() - p0.GetY()) * ((t - t0) / (t1 - t0)) + p0.GetY();

      Loc loc(x,y);
      int oid = (const_cast<GenLoc*>(&gloc1))->GetOid(); 
      result.SetValue(oid, loc);
      result.SetDefined(true);
    }
}

/*
check whether a location is visited 

*/
bool UGenLoc::Passes( const GenLoc& gloc ) const
{
/*
VTA - I could use the spatial algebra like this

----    HalfSegment hs;
        hs.Set( true, p0, p1 );
        return hs.Contains( p );
----
but the Spatial Algebra admit rounding errors (floating point operations). It
would then be very hard to return a true for this function.

*/
  assert( gloc.IsDefined() );
  assert( IsDefined() );
  

  if(gloc.GetOid() != gloc1.GetOid()) return false;
  if(gloc.GetOid() != gloc2.GetOid()) return false;

  Point p(true, gloc.GetLoc().loc1, gloc.GetLoc().loc2); 
  Point p0(true, gloc1.GetLoc().loc1, gloc1.GetLoc().loc2);
  Point p1(true, gloc2.GetLoc().loc1, gloc2.GetLoc().loc2);
  

  if( (timeInterval.lc && AlmostEqual( p, p0 )) ||
      (timeInterval.rc && AlmostEqual( p, p1 )) )
    return true;

  if( AlmostEqual( p0.GetX(), p1.GetX() ) &&
      AlmostEqual( p0.GetX(), p.GetX() ) )
    // If the segment is vertical
  {
    if( ( p0.GetY() <= p.GetY() && p1.GetY() >= p.GetY() ) ||
        ( p0.GetY() >= p.GetY() && p1.GetY() <= p.GetY() ) )
      return true;
  }
  else if( AlmostEqual( p0.GetY(), p1.GetY() ) &&
      AlmostEqual( p0.GetY(), p.GetY() ) )
    // If the segment is horizontal
  {
    if( ( p0.GetX() <= p.GetX() && p1.GetX() >= p.GetX() ) ||
        ( p0.GetX() >= p.GetX() && p1.GetX() <= p.GetX() ) )
      return true;
  }
  else
  {
    double k1 = ( p.GetX() - p0.GetX() ) / ( p.GetY() - p0.GetY() ),
           k2 = ( p1.GetX() - p0.GetX() ) / ( p1.GetY() - p0.GetY() );

    if( AlmostEqual( k1, k2 ) &&
        ( ( p0.GetX() < p.GetX() && p1.GetX() > p.GetX() ) ||
          ( p0.GetX() > p.GetX() && p1.GetX() < p.GetX() ) ) )
      return true;
  }
  return false;
}

/*
restrict the movement at a location 

*/

bool UGenLoc::At( const GenLoc& genloc, TemporalUnit<GenLoc>& res ) const 
{

  assert(genloc.IsDefined());
  assert(this->IsDefined());

  UGenLoc* result = static_cast<UGenLoc*>(&res);
  *result = *this;

  Point p0(true, gloc1.GetLoc().loc1, gloc1.GetLoc().loc2); 
  Point p1(true, gloc2.GetLoc().loc1, gloc2.GetLoc().loc2); 
  Point p(true, genloc.GetLoc().loc1, genloc.GetLoc().loc2); 
  
  // special case: static unit
  if(AlmostEqual(p0,p1)){
     if(AlmostEqual(p,p0) || AlmostEqual(p,p1)){
        return true;
     } else {
        result->SetDefined(false);
        return false;
     }
  }
  // special case p on p0
  if(AlmostEqual(p0,p)){
    if(!timeInterval.lc){
       result->SetDefined(false);
      return false;
    } else {
//       result->p1 = result->p0;
       result->gloc2 = result->gloc1; 
       result->timeInterval.rc = true;
       result->timeInterval.end = timeInterval.start;
       return true;
    }
  }
  // special case p on p1
  if(AlmostEqual(p,p1)){
    if(!timeInterval.rc){
      result->SetDefined(false);
      return false;
    } else {
//      result->p0 = result->p1;
      result->gloc1 = result->gloc2; 
      result->timeInterval.lc = true;
      result->timeInterval.start = timeInterval.end;
      return true;
    }
  }

  double d_x = p1.GetX() - p0.GetX();
  double d_y = p1.GetY() - p0.GetY();
  double delta;
  bool useX;

  if(fabs(d_x)> fabs(d_y)){
     delta = (p.GetX()-p0.GetX() ) / d_x;
     useX = true;
  } else {
     delta = (p.GetY()-p0.GetY() ) / d_y;
     useX = false;
  }

  if(AlmostEqual(delta,0)){
    delta = 0;
  }
  if(AlmostEqual(delta,1)){
    delta = 1;
  }

  if( (delta<0) || (delta>1)){
    result->SetDefined(false);
    return false;
  }

  if(useX){ // check y-value
    double y = p0.GetY() + delta*d_y;
    if(!AlmostEqual(y,p.GetY())){
       result->SetDefined(false);
       return false;
    }
  } else { // check x-value
    double x = p0.GetX() + delta*d_x;
    if(!AlmostEqual(x,p.GetX())){
       result->SetDefined(false);
       return false;
    }
  }


  Instant time = timeInterval.start +
                 (timeInterval.end-timeInterval.start)*delta;

//  result->p0 = p;
//  result->p1 = p;
  result->gloc1 = genloc;
  result->gloc2 = genloc; 
  result->timeInterval.lc = true;
  result->timeInterval.rc = true;
  result->timeInterval.start = time;
  result->timeInterval.end = time;
  return true;
  
}


UGenLoc* UGenLoc::Clone() const
{
  UGenLoc* res;
  if(this->IsDefined()){
    
    res = new UGenLoc(timeInterval, gloc1, gloc2, tm); 
    res->del.isDefined = del.isDefined;
  }else{
    res = new UGenLoc(false); 
  }
  return res; 
}

void UGenLoc::CopyFrom(const Attribute* right)
{
  const UGenLoc* ugloc = static_cast<const UGenLoc*>(right); 
  if(ugloc->del.isDefined){
    timeInterval.CopyFrom(ugloc->timeInterval);
    gloc1 = ugloc->gloc1;
    gloc2 = ugloc->gloc2;
    tm = ugloc->tm; 
  }
  del.isDefined = ugloc->del.isDefined; 
}

ostream& operator<<(ostream& o, const UGenLoc& gloc)
{
  if(gloc.IsDefined()){
    o<<gloc.timeInterval<<" "<<gloc.gloc1<<" "
     <<gloc.gloc2<<" "<<GetTMStr(gloc.tm); 
  }else
    o<<" undef";
  return o;

}

/*
it returns the 3D bounding box. at somewhere else, the program should check
the object identifier to know whether the movement is outdoor or indoor. if 
it is indoor, it has to convert 3D box to 4D box (x,y,z,t). And it also has to
calculate the absolute coordinates in space. 

*/
const Rectangle<3> UGenLoc::BoundingBox() const
{
  if(this->IsDefined()){

    return Rectangle<3>(true, 
                       MIN(gloc1.GetLoc().loc1, gloc2.GetLoc().loc1),
                       MAX(gloc1.GetLoc().loc1, gloc2.GetLoc().loc1), 
                       MIN(gloc1.GetLoc().loc2, gloc2.GetLoc().loc2),
                       MAX(gloc1.GetLoc().loc2, gloc2.GetLoc().loc2),  
                       timeInterval.start.ToDouble(),
                       timeInterval.end.ToDouble());
  }else
      return Rectangle<3>(false); 

}

//////////////////////////////////////////////////////////////////////////
//////////////////////general moving objects//////////////////////////////
//////////////////////////////////////////////////////////////////////////
ListExpr GenMOProperty()
{
  return (nl->TwoElemList(
            nl->FourElemList(nl->StringAtom("Signature"),
                       nl->StringAtom("Example Type List"),
           nl->StringAtom("List Rep"),
           nl->StringAtom("Example List")),
            nl->FourElemList(nl->StringAtom("-> MAPPING"),
                       nl->StringAtom("genmpoint"),
           nl->StringAtom("(u1,...,un)"),
      nl->StringAtom("((interval (1 (10.0 1.0))(1 (12.0 3.0)) Indoor))"))));
}

bool CheckGenMO( ListExpr type, ListExpr& errorInfo )
{
//  cout<<"CheckGenMPoint"<<endl; 
  return (nl->IsEqual( type, "genmo" ));
}


void GenMO::Clear()
{
  Mapping<UGenLoc, GenLoc>::Clear();
}


GenMO::GenMO(const GenMO& mo):Mapping<UGenLoc,GenLoc>(0)
{
    del.refs = 1;
    del.SetDelete();
    del.isDefined = mo.IsDefined();

    Clear();
    if( !this->IsDefined() ) {
      return;
    }
    StartBulkLoad();
    UGenLoc unit;
    for( int i = 0; i < mo.GetNoComponents(); i++ ){
      mo.Get( i, unit );
      Add( unit );
//      cout<<unit<<endl; 
    }
    EndBulkLoad( false );

}

/*
copy it from another data 

*/
void GenMO::CopyFrom(const Attribute* right)
{
//    cout<<"CopyFrom "<<endl; 
    const GenMO *genmo = (const GenMO*)right;
    assert( genmo->IsOrdered() );
    Clear();
    this->SetDefined(genmo->IsDefined());
    if( !this->IsDefined() ) {
      return;
    }
    StartBulkLoad();
    UGenLoc unit;
    for( int i = 0; i < genmo->GetNoComponents(); i++ ){
      genmo->Get( i, unit );
      Add( unit );
    }
    EndBulkLoad( false );
}

Attribute* GenMO::Clone() const
{
    assert( IsOrdered() );
    GenMO *result;
    if( !this->IsDefined() ){
      result = new GenMO( 0 );
    } else {
      result = new GenMO( GetNoComponents() );
      if(GetNoComponents()>0){
        result->units.resize(GetNoComponents());
      }
      result->StartBulkLoad();
      UGenLoc unit;
      for( int i = 0; i < GetNoComponents(); i++ ){
        Get( i, unit );
        result->Add( unit );
      }
      result->EndBulkLoad( false );
    }
    result->SetDefined(this->IsDefined());
    return (Attribute*) result;

}

/*
put new unit. it only inserts the new unit and does not calculate the bounding
box. 

*/
void GenMO::Add(const UGenLoc& unit)
{
  assert(unit.IsDefined());
  assert(unit.IsValid()); 
  if(!IsDefined()){
    SetDefined(false);
    return; 
  }
  assert(unit.gloc1.GetOid() == unit.gloc2.GetOid()); 
  units.Append(unit); 
}

void GenMO::EndBulkLoad(const bool sort, const bool checkvalid)
{
  Mapping<UGenLoc, GenLoc>::EndBulkLoad(sort, checkvalid); 

}


/*
low resolution for a generic moving object (oid,tm)

*/
void GenMO::LowRes(GenMO& mo)
{
    mo.Clear();

    mo.StartBulkLoad();
    UGenLoc unit;
    Loc loc(-1.0, -1.0);
    for( int i = 0; i < GetNoComponents(); i++ ){
      Get( i, unit );
      unit.gloc1.SetLoc(loc); 
      unit.gloc2.SetLoc(loc); 
      mo.Add( unit );
//      cout<<unit<<endl; 
    }
    mo.EndBulkLoad( false );
}

/*
get the trajectory of a generic moving object. it needs the space 
because loc1 loc2 have different meanings for different infrastructures 

a better method is we first open all infrastructures and store the pointers
after processing we close all of them.

if it references to the same bus trip, we load the trajectory in a bulkload way
  
*/
void GenMO::Trajectory(GenRange* genrange, Space* sp)
{
  genrange->Clear();
  
  vector<void*> infra_pointer; 
  sp->OpenInfra(infra_pointer);
  
  for(int i = 0;i < GetNoComponents();i++){
      UGenLoc unit;
      Get(i, unit);
      int oid = unit.gloc1.GetOid();
      int tm = unit.tm;

      int infra_type = sp->GetInfraType(oid);

      Line* l = new Line(0);
      l->StartBulkLoad();
      if(infra_type == IF_BUSNETWORK && i < GetNoComponents() - 1){
        int j = i + 1;
        UGenLoc unit2;
        Get(j, unit2);
        int oid2 = unit2.gloc1.GetOid();
        int infra_type2 = sp->GetInfraType(oid2);
        Interval<Instant> time_range = unit.timeInterval;
        ///////////////same bus trip/////////////////////////////
        while(infra_type2 == IF_BUSNETWORK && oid2 == oid){
            time_range.end = unit2.timeInterval.end;
            j++;
            if(j == GetNoComponents()) break;
            Get(j, unit2);
            oid2 = unit2.gloc1.GetOid();
            infra_type2 = sp->GetInfraType(oid2);
        }

        sp->GetLineInIFObject(oid, unit.gloc1, unit.gloc2, l, 
                            infra_pointer, time_range, infra_type);
        i = j - 1;

      }else
        sp->GetLineInIFObject(oid, unit.gloc1, unit.gloc2, l, 
                            infra_pointer, unit.timeInterval, infra_type);
      l->EndBulkLoad();

      genrange->Add(oid, l, tm);
      delete l;
  }

  sp->CloseInfra(infra_pointer);

}

/*
get the sub movement of a generic moving object according to a mode

*/
void GenMO::AtMode(string tm, GenMO* sub)
{
  int mode = GetTM(tm);
  if(mode < 0 || mode >= (int)(ARR_SIZE(str_tm))){
    cout<<"invalid mode "<<tm<<endl;
    return;
  }
  sub->Clear();
  sub->StartBulkLoad();
  for(int i = 0 ;i < GetNoComponents();i++){
    UGenLoc unit;
    Get( i, unit );
    if(unit.GetTM() == mode)
      sub->Add(unit);
  }

  sub->EndBulkLoad();
}

/////////////////////////////////////////////////////////////////////////
/////////////// get information from generic moving objects///////////////
/////////////////////////////////////////////////////////////////////////
string GenMObject::StreetSpeedInfo = "(rel (tuple ((id_b int) (Vmax real))))";


void GenMObject::GetMode(GenMO* mo)
{
  tm_list.clear(); 
  for(int i = 0;i < mo->GetNoComponents();i++){
    UGenLoc unit;
    mo->Get(i, unit);
    int tm = unit.GetTM(); 
    assert(tm >= 0); 
    if(tm_list.size() == 0)tm_list.push_back(tm); 
    else if(tm_list[tm_list.size() - 1] != tm)
      tm_list.push_back(tm); 
  }
}

/*
get the referenced infrastructure objects in a light way 

*/
void GenMObject::GetRef(GenMO* mo)
{

  for(int i = 0;i < mo->GetNoComponents();i++){
    UGenLoc unit;
    mo->Get(i, unit);
    int oid = unit.gloc1.GetOid();
    int tm = unit.tm;
    if(oid_list.size() == 0){
      int infra_label;
      if(oid == 0)
          infra_label = IF_FREESPACE;
      else
          infra_label = TM2InfraLabel(tm);

      oid_list.push_back(oid);
      label_list.push_back(infra_label);
    }else if(oid != oid_list[oid_list.size() - 1]){
      int infra_label;
      if(oid == 0)
          infra_label = IF_FREESPACE;
      else
          infra_label = TM2InfraLabel(tm);

      oid_list.push_back(oid);
      label_list.push_back(infra_label);
    }
  }

}

void GenMObject::GetTMStr(bool v)
{
  if(v == false)return;
  for(unsigned int i = 0;i < ARR_SIZE(genmo_tmlist);i++){
      tm_str_list.push_back(genmo_tmlist[i]);
  }

}

/*
get the reference id of generic moving objects 

*/
void GenMObject::GetIdList(GenMO* genmo)
{
    for( int i = 0; i < genmo->GetNoComponents(); i++ ){
      UGenLoc unit;
      genmo->Get( i, unit );
      assert(unit.gloc1.GetOid() == unit.gloc2.GetOid()); 
      id_list.push_back(unit.gloc1.GetOid()); 
    }
}

/*
get the reference id of genrange objects

*/

void GenMObject::GetIdList(GenRange* gr)
{
    for(int i = 0;i < gr->ElemSize();i++){
      GenRangeElem elem;
      gr->GetElem(i, elem); 
      id_list.push_back(elem.oid); 
    }
}

/*
create generic moving objects

*/
void GenMObject::GenerateGenMO(Space* sp, Periods* peri, int mo_no, 
                               int type, Relation* rel)
{
//  cout<<"GenerateGenMO()"<<endl; 
  if(mo_no < 1){
    cout<<" invalid number of moving objects "<<mo_no<<endl;
    return;
  }

  if(!(0 <= type && type < int(ARR_SIZE(genmo_tmlist)))){
    cout<<" invalid type value "<<type<<endl;
  }

  switch(type){
    case 1:
      GenerateGenMO_Walk(sp, peri, mo_no, rel);
      break;

    case 4:
      GenerateGenMO_CarTaxi(sp, peri, mo_no, rel, "Car");
      break;

    case 6:
      GenerateGenMO_CarTaxi(sp, peri, mo_no, rel, "Taxi");
      break;

    default:
      assert(false);
      break;  
  }

}

/*
create generic moving objects

*/
void GenMObject::GenerateGenMO2(Space* sp, Periods* peri, int mo_no, 
                               int type, Relation* rel1, 
                               BTree* btree, Relation* rel2)
{
//  cout<<"GenerateGenMO()"<<endl; 
  if(mo_no < 1){
    cout<<" invalid number of moving objects "<<mo_no<<endl;
    return;
  }
  if(!(0 <= type && type < int(ARR_SIZE(genmo_tmlist)))){
    cout<<" invalid type value "<<type<<endl;
  }

  switch(type){
    case 7:
      GenerateGenMO_CarTaxiWalk(sp, peri, mo_no, rel1, btree, rel2, "Car");
      break;
    case 11:
      GenerateGenMO_CarTaxiWalk(sp, peri, mo_no, rel1, btree, rel2, "Taxi");
      break;

    default:
      assert(false);
      break;  
  }

}

/*
create generic moving objects: bus, walk

*/
void GenMObject::GenerateGenMO3(Space* sp, Periods* peri, int mo_no, 
                      int type, Relation* rel1, Relation* rel2, 
                                R_Tree<2,TupleId>* rtree)
{
  if(mo_no < 1){
    cout<<" invalid number of moving objects "<<mo_no<<endl;
    return;
  }
  if(!(0 <= type && type < int(ARR_SIZE(genmo_tmlist)))){
    cout<<" invalid type value "<<type<<endl;
  }
    switch(type){
      case 8:
        GenerateGenMO_BusWalk(sp, peri, mo_no, rel1, rel2, rtree, "Bus");
        break;

      default:
        assert(false);
        break;  
  }


}
/*
generic moving objects with transportation mode walk

*/
void GenMObject::GenerateGenMO_Walk(Space* sp, Periods* peri, 
                                    int mo_no, Relation* tri_rel)
{

//  cout<<"Mode Walk "<<endl;
  const double min_len = 50.0;
  Pavement* pm = sp->LoadPavement(IF_REGION);
  vector<GenLoc> genloc_list;
  GenerateLocPave(pm, mo_no, genloc_list);///generate locations on pavements 
  int count = 1;

  DualGraph* dg = pm->GetDualGraph();
  int no_triangle = dg->GetNodeRel()->GetNoTuples();
  int mini_oid = dg->min_tri_oid_1;
  VisualGraph* vg = pm->GetVisualGraph();


  Walk_SP* wsp = new Walk_SP(dg, vg, NULL, NULL);
  wsp->rel3 = tri_rel;
  
  while(count <= mo_no){
    int index1 = GetRandom() % genloc_list.size();
    GenLoc loc1 = genloc_list[index1];
    int index2 = GetRandom() % genloc_list.size();
    GenLoc loc2 = genloc_list[index2];

    if(index1 == index2) continue;
    Point p1(true, loc1.GetLoc().loc1, loc1.GetLoc().loc2);
    Point p2(true, loc2.GetLoc().loc1, loc2.GetLoc().loc2);
    if(p1.Distance(p2) < min_len) continue;


    int oid1 = loc1.GetOid() - mini_oid;
//    cout<<"oid1 "<<oid1<<endl;
    assert(1 <= oid1 && oid1 <= no_triangle);

    int oid2 = loc2.GetOid() - mini_oid;
//    cout<<"oid2 "<<oid2<<endl;
    assert(1 <= oid2 && oid2 <= no_triangle);

    Line* path = new Line(0);

    wsp->WalkShortestPath2(oid1, oid2, p1, p2, path);
//    cout<<"path length "<<path->Length()<<endl; 
    GenerateWalk(dg, count, peri, path, p1);

    delete path;
    count++;

  }
  delete wsp;

  pm->CloseDualGraph(dg);
  pm->CloseVisualGraph(vg);

  sp->ClosePavement(pm);
}

/*
generate locations on the pavment area 
!!! note that here we put the ----absolute position---- in loc instead of the 
relative position according to the triangle !!!

*/
void GenMObject::GenerateLocPave(Pavement* pm, int mo_no, 
                                 vector<GenLoc>& genloc_list)
{
  Walk_SP* wsp = new Walk_SP(NULL, NULL, pm->GetPaveRel(), NULL);
  wsp->GenerateData1(mo_no*2);
  for(unsigned int i = 0;i < wsp->oids.size();i++){
    int oid = wsp->oids[i];
/*    double loc1 = wsp->q_loc1[i].GetX();
    double loc2 = wsp->q_loc1[i].GetY();*/

    double loc1 = wsp->q_loc2[i].GetX();
    double loc2 = wsp->q_loc2[i].GetY();

//    cout<<"oid "<<oid<<" loc1 "<<loc1<<" loc2 "<<loc2<<endl; 
    Loc loc(loc1, loc2);
    GenLoc genloc(oid, loc);
    genloc_list.push_back(genloc);
  }

  delete wsp;
}

/*
generate moving objects with mode walk and the path 

*/
void GenMObject::GenerateWalk(DualGraph* dg, int count, Periods* peri, 
                              Line* l, Point start_loc)
{

   MPoint* mo = new MPoint(0);
   GenMO* genmo = new GenMO(0);
   mo->StartBulkLoad();
   genmo->StartBulkLoad();
   //////////////////////////////////////////////////////////////
   ///////////////set start time/////////////////////////////////
   /////////////////////////////////////////////////////////////
   Interval<Instant> periods;
   peri->Get(0, periods);
   Instant start_time = periods.start;
   int time_range = 14*60;//14 hours 
   if(count % 3 == 0)
     start_time.ReadFrom(periods.start.ToDouble() + 
                        (GetRandom() % time_range)/(24.0*60.0));
   else
    start_time.ReadFrom(periods.end.ToDouble() -
                        (GetRandom() % time_range)/(24.0*60.0));

   GenerateWalkMovement(dg, l, start_loc, genmo, mo, start_time);

   mo->EndBulkLoad();
   genmo->EndBulkLoad();

   trip1_list.push_back(*genmo);
   trip2_list.push_back(*mo);
   delete mo; 
   delete genmo; 
   
}

/*
provide an interface for creating moving objects with mode walk

*/
void GenMObject::GenerateWalkMovement(DualGraph* dg, Line* l, Point start_loc,
                            GenMO* genmo, MPoint* mo, Instant& start_time)
{

   SimpleLine* path = new SimpleLine(0);
   path->fromLine(*l);

   SpacePartition* sp = new SpacePartition();
   vector<MyHalfSegment> seq_halfseg; //reorder it from start to end
   sp->ReorderLine(path, seq_halfseg);
   delete sp;
   delete path;
   Point temp_sp1 = seq_halfseg[0].from; 
   Point temp_sp2 = seq_halfseg[seq_halfseg.size() - 1].to; 
   const double delta_dist = 0.01;

  ///////////////////////////////////////////////////////////////
   ////////we reference to the big overall large polygon/////////////
   //////////////////////////////////////////////////////////////////
   int gen_mo_ref_id = dg->min_tri_oid_1 + dg->GetNodeRel()->GetNoTuples() + 1;
//   cout<<gen_mo_ref_id<<endl;
    Rectangle<2> bbox = dg->rtree_node->BoundingBox();
//    cout<<bbox<<endl;
   ///////////////////////////////////////////////////////////////////

   if(start_loc.Distance(temp_sp1) < delta_dist){
        Instant st = start_time;
        Instant et = start_time; 
        Interval<Instant> up_interval; 
        for(unsigned int i = 0;i < seq_halfseg.size();i++){
            Point from_loc = seq_halfseg[i].from;
            Point to_loc = seq_halfseg[i].to; 

            double dist = from_loc.Distance(to_loc);
            double time = dist; //assume the speed for pedestrian is 1.0m
 
            if(dist < delta_dist){//ignore such small segment 
                if((i + 1) < seq_halfseg.size()){
                seq_halfseg[i+1].from = from_loc; 
                }
                continue; 
            }
            //double 1.0 means 1 day 
            et.ReadFrom(st.ToDouble() + time*1.0/(24.0*60.0*60));
        ////////////////////create a upoint////////////////////////
            up_interval.start = st;
            up_interval.lc = true;
            up_interval.end = et;
            up_interval.rc = false; 
            UPoint* up = new UPoint(up_interval,from_loc,to_loc);
            mo->Add(*up);
            delete up; 
            //////////////////////////////////////////////////////////////
            st = et; 
            start_time = et;
            ////////////////////////////////////////////////////////////////
            Loc loc1(from_loc.GetX() - bbox.MinD(0), 
                     from_loc.GetY() - bbox.MinD(1));
            Loc loc2(to_loc.GetX() - bbox.MinD(0),
                     to_loc.GetY() - bbox.MinD(1));
            GenLoc gloc1(gen_mo_ref_id, loc1);
            GenLoc gloc2(gen_mo_ref_id, loc2);
            int tm = GetTM("Walk");
            UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
            genmo->Add(*unit); 
            delete unit; 
        }
   }else if(start_loc.Distance(temp_sp2) < delta_dist){
              Instant st = start_time;
              Instant et = start_time; 
              Interval<Instant> up_interval; 
              for(int i = seq_halfseg.size() - 1;i >= 0;i--){
                  Point from_loc = seq_halfseg[i].to;
                  Point to_loc = seq_halfseg[i].from; 
                  double dist = from_loc.Distance(to_loc);
                  double time = dist; 
                  if(dist < delta_dist){//ignore such small segment 
                        if((i + 1) < seq_halfseg.size()){
                            seq_halfseg[i+1].from = from_loc; 
                        }
                    continue;
                  }
                //double 1.0 means 1 day 
                 et.ReadFrom(st.ToDouble() + time*1.0/(24.0*60.0*60.0));

                 ////////////////////create a upoint////////////////////////
                  up_interval.start = st;
                  up_interval.lc = true;
                  up_interval.end = et;
                  up_interval.rc = false; 
                  UPoint* up = new UPoint(up_interval,from_loc,to_loc);
                  mo->Add(*up);
                  delete up; 
                  /////////////////////////////////////////////////////////
                  st = et;
                  start_time = et;
                  /////////////////////////////////////////////////////////
                  Loc loc1(from_loc.GetX() - bbox.MinD(0), 
                           from_loc.GetY() - bbox.MinD(1));
                  Loc loc2(to_loc.GetX() - bbox.MinD(0),
                           to_loc.GetY() - bbox.MinD(1));
                  GenLoc gloc1(gen_mo_ref_id, loc1);
                  GenLoc gloc2(gen_mo_ref_id, loc2);
                  int tm = GetTM("Walk");
                  UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
                  genmo->Add(*unit); 
                  delete unit; 
              }
   }else assert(false);

}

/*
free movement, transportation mode is given 

*/
void GenMObject::GenerateFreeMovement(Line* l, Point start_loc,
                            GenMO* genmo, MPoint* mo, Instant& start_time,
                                      string mode)
{

   SimpleLine* path = new SimpleLine(0);
   path->fromLine(*l);

   SpacePartition* sp = new SpacePartition();
   vector<MyHalfSegment> seq_halfseg; //reorder it from start to end
   sp->ReorderLine(path, seq_halfseg);
   delete sp;
   delete path;
   Point temp_sp1 = seq_halfseg[0].from; 
   Point temp_sp2 = seq_halfseg[seq_halfseg.size() - 1].to; 
   const double delta_dist = 0.01;
   ///////////////////////////////////////////////////////////////////

   if(start_loc.Distance(temp_sp1) < delta_dist){
        Instant st = start_time;
        Instant et = start_time; 
        Interval<Instant> up_interval; 
        for(unsigned int i = 0;i < seq_halfseg.size();i++){
            Point from_loc = seq_halfseg[i].from;
            Point to_loc = seq_halfseg[i].to; 

            double dist = from_loc.Distance(to_loc);
            double time = dist; //assume the speed for pedestrian is 1.0m
 
            if(dist < delta_dist){//ignore such small segment 
                if((i + 1) < seq_halfseg.size()){
                seq_halfseg[i+1].from = from_loc; 
                }
                continue; 
            }
            //double 1.0 means 1 day 
            et.ReadFrom(st.ToDouble() + time*1.0/(24.0*60.0*60));
        ////////////////////create a upoint////////////////////////
            up_interval.start = st;
            up_interval.lc = true;
            up_interval.end = et;
            up_interval.rc = false; 
            UPoint* up = new UPoint(up_interval,from_loc,to_loc);
            mo->Add(*up);
            delete up; 
            //////////////////////////////////////////////////////////////
            st = et; 
            start_time = et;
            ////////////////////////////////////////////////////////////////
            Loc loc1(from_loc.GetX(),from_loc.GetY());
            Loc loc2(to_loc.GetX(), to_loc.GetY());
            GenLoc gloc1(0, loc1);
            GenLoc gloc2(0, loc2);
            int tm = GetTM(mode);
            UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
            genmo->Add(*unit); 
            delete unit; 
        }
   }else if(start_loc.Distance(temp_sp2) < delta_dist){
              Instant st = start_time;
              Instant et = start_time; 
              Interval<Instant> up_interval; 
              for(int i = seq_halfseg.size() - 1;i >= 0;i--){
                  Point from_loc = seq_halfseg[i].to;
                  Point to_loc = seq_halfseg[i].from; 
                  double dist = from_loc.Distance(to_loc);
                  double time = dist; 
                  if(dist < delta_dist){//ignore such small segment 
                        if((i + 1) < seq_halfseg.size()){
                            seq_halfseg[i+1].from = from_loc; 
                        }
                    continue;
                  }
                //double 1.0 means 1 day 
                 et.ReadFrom(st.ToDouble() + time*1.0/(24.0*60.0*60.0));

                 ////////////////////create a upoint////////////////////////
                  up_interval.start = st;
                  up_interval.lc = true;
                  up_interval.end = et;
                  up_interval.rc = false; 
                  UPoint* up = new UPoint(up_interval,from_loc,to_loc);
                  mo->Add(*up);
                  delete up; 
                  /////////////////////////////////////////////////////////
                  st = et;
                  start_time = et;
                  /////////////////////////////////////////////////////////
                  Loc loc1(from_loc.GetX(), from_loc.GetY());
                  Loc loc2(to_loc.GetX(),to_loc.GetY());
                  GenLoc gloc1(0, loc1);
                  GenLoc gloc2(0, loc2);
                  int tm = GetTM(mode);
                  UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
                  genmo->Add(*unit); 
                  delete unit; 
              }
   }else assert(false);

}

/*
free movement, transportation mode is given 

*/
void GenMObject::GenerateFreeMovement2(Point start_loc, Point end_loc,
                            GenMO* genmo, MPoint* mo, Instant& start_time,
                                      string mode)
{
   const double delta_dist = 0.01;
   if(start_loc.Distance(end_loc) < delta_dist) return;
   ///////////////////////////////////////////////////////////////////
    Instant st = start_time;
    Instant et = start_time; 
    Interval<Instant> up_interval; 

    double dist = start_loc.Distance(end_loc);
    double time = dist; //assume the speed for pedestrian is 1.0m
 

    //double 1.0 means 1 day 
    et.ReadFrom(st.ToDouble() + time*1.0/(24.0*60.0*60));
    ////////////////////create a upoint////////////////////////
    up_interval.start = st;
    up_interval.lc = true;
    up_interval.end = et;
    up_interval.rc = false; 
    UPoint* up = new UPoint(up_interval,start_loc,end_loc);
    mo->Add(*up);
    delete up; 
    //////////////////////////////////////////////////////////////
    st = et; 
    start_time = et;
   ////////////////////////////////////////////////////////////////
    Loc loc1(start_loc.GetX(),start_loc.GetY());
    Loc loc2(end_loc.GetX(), end_loc.GetY());
    GenLoc gloc1(0, loc1);
    GenLoc gloc2(0, loc2);
    int tm = GetTM(mode);
    UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
    genmo->Add(*unit); 
    delete unit; 
}



/*
generic moving objects with transportation mode car or taxi

*/
void GenMObject::GenerateGenMO_CarTaxi(Space* sp, Periods* peri, int mo_no, 
                                   Relation* rel, string mode)
{
//  cout<<"Mode Car "<<endl;

  const double min_len = 500.0;
  Network* rn = sp->LoadRoadNetwork(IF_LINE);
  vector<GPoint> gp_list; 
  GenerateGPoint(rn, mo_no, gp_list);/////////get road network locations 
  int count = 1;
  while(count <= mo_no){
    int index1 = GetRandom() % gp_list.size();
    GPoint gp1 = gp_list[index1];
//    cout<<"gp1 "<<gp1<<endl;
    int index2 = GetRandom() % gp_list.size();
    GPoint gp2 = gp_list[index2];
    if(index1 == index2) continue; 

    Point* start_loc = new Point();
    gp1.ToPoint(start_loc);

//      cout<<"gp2 "<<gp2<<endl; 
    GLine* gl = new GLine(0);
    gp1.ShortestPath(&gp2, gl);////////compute the shortest path 
//    cout<<"gline len "<<gl->GetLength()<<endl; 
    if(gl->GetLength() > min_len){
        BusRoute* br = new BusRoute(rn, NULL,NULL);
        GLine* newgl = new GLine(0);
        br->ConvertGLine(gl, newgl);
//        cout<<"new gl length "<<newgl->GetLength()<<endl; 
        GenerateCarTaxi(rn, count, peri, newgl, rel, *start_loc, mode);

        delete newgl; 
        delete br;
        count++;
    }
    delete gl;
    delete start_loc; 
  }

  sp->CloseRoadNetwork(rn);
}


/*
create a moving object with mode car or taxi

*/
void GenMObject::GenerateCarTaxi(Network* rn, int i, Periods* peri, 
                                 GLine* newgl,
                                 Relation* rel, Point start_loc, string mode)
{
  Interval<Instant> periods;
  peri->Get(0, periods);
  Instant start_time = periods.start;
  int time_range = 14*60;//14 hours 
  if(i % 3 == 0)
    start_time.ReadFrom(periods.start.ToDouble() + 
                        (GetRandom() % time_range)/(24.0*60.0));
  else
    start_time.ReadFrom(periods.end.ToDouble() -
                        (GetRandom() % time_range)/(24.0*60.0));
//  cout<<"start time "<<start_time<<endl;

  MPoint* mo = new MPoint(0);
  GenMO* genmo = new GenMO(0);
  mo->StartBulkLoad();
  genmo->StartBulkLoad(); 
  const double delta_dist = 0.01;

  for(int i = 0;i < newgl->Size();i++){
    RouteInterval* ri = new RouteInterval();
    newgl->Get(i, *ri);
    int rid = ri->GetRouteId();
    Tuple* speed_tuple = rel->GetTuple(rid, false);
    assert(((CcInt*)speed_tuple->GetAttribute(SPEED_RID))->GetIntval() == rid);
    double speed = 
      ((CcReal*)speed_tuple->GetAttribute(SPEED_VAL))->GetRealval();
    speed_tuple->DeleteIfAllowed();
//    cout<<"rid "<<rid<<" speed "<<speed<<endl; 

    if(speed < 0.0)speed = 10.0;
    speed = speed * 1000.0/3600.0;// meters in second 
//    cout<<"rid "<<rid<<" new speed "<<speed<<endl;
    SimpleLine* sl = new SimpleLine(0);
    rn->GetLineValueOfRouteInterval(ri, sl);
    ////////////////////////////////////////////////////////////////////
    /////////////get the subline of this route interval////////////////
    ///////////////////////////////////////////////////////////////////
    SpacePartition* sp = new SpacePartition();
    vector<MyHalfSegment> seq_halfseg; //reorder it from start to end
    sp->ReorderLine(sl, seq_halfseg);
    delete sp;
    //////////////////////////////////////////////////////////////////
    delete sl;
    
    /////////////////////////////////////////////////////////////////////

    Interval<Instant> unit_interval;
    unit_interval.start = start_time;
    unit_interval.lc = true;

    Point temp_sp1 = seq_halfseg[0].from; 
    Point temp_sp2 = seq_halfseg[seq_halfseg.size() - 1].to; 
    if(start_loc.Distance(temp_sp1) < delta_dist){
      CreateCarTrip1(mo, seq_halfseg, start_time, speed);

      start_loc = temp_sp2;
    }else if(start_loc.Distance(temp_sp2) < delta_dist){

      CreateCarTrip2(mo, seq_halfseg, start_time, speed);
      start_loc = temp_sp1;
    }else assert(false);

    unit_interval.end = start_time;
    unit_interval.rc = false;
    //////////////////////////////////////////////////////////////
    ////////////////generic units/////////////////////////////////
    //////////////////////////////////////////////////////////////
    Loc loc1(ri->GetStartPos(), -1); 
    Loc loc2(ri->GetEndPos(), -1); 
    GenLoc gloc1(ri->GetRouteId(), loc1);
    GenLoc gloc2(ri->GetRouteId(), loc2);
    int tm = GetTM(mode); 

    //////////////////////////////////////////////////////////////////
    /////////////correct way to create UGenLoc///////////////////////
    //////////////////////////////////////////////////////////////////
    UGenLoc* unit = new UGenLoc(unit_interval, gloc1, gloc2, tm);
    genmo->Add(*unit); 
    delete unit; 
    delete ri;

  }
  mo->EndBulkLoad();
  genmo->EndBulkLoad();
  
  trip1_list.push_back(*genmo);
  trip2_list.push_back(*mo);
  delete mo; 
  delete genmo; 

}

/*
create a set of random gpoint locations 

*/
void GenMObject::GenerateGPoint(Network* rn, int mo_no, vector<GPoint>& gp_list)
{
  Relation* routes_rel = rn->GetRoutes();
  for (int i = 1; i <= mo_no * 2;i++){
     int m = GetRandom() % routes_rel->GetNoTuples() + 1;
     Tuple* road_tuple = routes_rel->GetTuple(m, false);
     int rid = ((CcInt*)road_tuple->GetAttribute(ROUTE_ID))->GetIntval();
     SimpleLine* sl = (SimpleLine*)road_tuple->GetAttribute(ROUTE_CURVE);
//     cout<<"rid "<<rid<<" len: "<<sl->Length()<<endl; 
     double len = sl->Length();
     int pos = GetRandom() % (int)len;
     GPoint gp(true, rn->GetId(), rid, pos);
     gp_list.push_back(gp);
     road_tuple->DeleteIfAllowed();
   }
}


/*
create moving bus units and add them into the trip, 
it travers from index small to big in myhalfsegment list 
use the maxspeed as car speed 

*/
void GenMObject::CreateCarTrip1(MPoint* mo, vector<MyHalfSegment> seq_halfseg,
                      Instant& start_time, double speed_val)
{
  const double dist_delta = 0.01; 
  
  Instant st = start_time;
  Instant et = start_time; 
  Interval<Instant> up_interval; 
  for(unsigned int i = 0;i < seq_halfseg.size();i++){
    Point from_loc = seq_halfseg[i].from;
    Point to_loc = seq_halfseg[i].to; 

    double dist = from_loc.Distance(to_loc);
    double time = dist/speed_val; 

 //   cout<<"dist "<<dist<<" time "<<time<<endl; 
//    printf("%.10f, %.10f",dist, time); 
    //////////////////////////////////////////////////////////////
    if(dist < dist_delta){//ignore such small segment 
        if((i + 1) < seq_halfseg.size()){
          seq_halfseg[i+1].from = from_loc; 
        }
        continue; 
    }
    /////////////////////////////////////////////////////////////////

    //double 1.0 means 1 day 
    et.ReadFrom(st.ToDouble() + time*1.0/(24.0*60.0*60));
//    cout<<st<<" "<<et<<endl;
    ////////////////////create a upoint////////////////////////
    up_interval.start = st;
    up_interval.lc = true;
    up_interval.end = et;
    up_interval.rc = false; 
    UPoint* up = new UPoint(up_interval,from_loc,to_loc);
//    cout<<*up<<endl; 
    mo->Add(*up);
    delete up; 
    //////////////////////////////////////////////////////////////
    st = et; 
  }

  start_time = et; 

}

/*
create moving carunits and add them into the trip, 
!!! it traverse from index big to small in myhalfsegment list 
use the maxspeed as car speed 

*/
void GenMObject::CreateCarTrip2(MPoint* mo, vector<MyHalfSegment> seq_halfseg,
                      Instant& start_time, double speed_val)
{
 const double dist_delta = 0.01; 
//  cout<<"trip2 max speed "<<speed_val<<" start time "<<start_time<<endl; 
  Instant st = start_time;
  Instant et = start_time; 
  Interval<Instant> up_interval; 
  for(int i = seq_halfseg.size() - 1;i >= 0;i--){
    Point from_loc = seq_halfseg[i].to;
    Point to_loc = seq_halfseg[i].from; 
    double dist = from_loc.Distance(to_loc);
    double time = dist/speed_val; 

 //   cout<<"dist "<<dist<<" time "<<time<<endl; 
    ///////////////////////////////////////////////////////////////////
    if(dist < dist_delta){//ignore such small segment 
        if((i + 1) < seq_halfseg.size()){
          seq_halfseg[i+1].from = from_loc; 
        }
        continue; 
    }

    //double 1.0 means 1 day 
    et.ReadFrom(st.ToDouble() + time*1.0/(24.0*60.0*60.0));
//    cout<<st<<" "<<et<<endl;
    ////////////////////create a upoint////////////////////////
    up_interval.start = st;
    up_interval.lc = true;
    up_interval.end = et;
    up_interval.rc = false; 
    UPoint* up = new UPoint(up_interval,from_loc,to_loc);
    mo->Add(*up);
    delete up; 
    //////////////////////////////////////////////////////////////
    st = et; 
  }

  start_time = et; 

}

/*
generate generic moving objects with mode car or taxi and walk

*/
void GenMObject::GenerateGenMO_CarTaxiWalk(Space* sp, Periods* peri, int mo_no,
                             Relation* rel, BTree* btree, 
                                       Relation* speed_rel, string mode)
{

//  cout<<"Mode Car-Walk "<<endl;
  const double min_len = 50.0;
  Pavement* pm = sp->LoadPavement(IF_REGION);
  vector<GenLoc> genloc_list;
  GenerateLocPave(pm, mo_no, genloc_list);///generate locations on pavements 
  int count = 1;
  Network* rn = sp->LoadRoadNetwork(IF_LINE);

  Interval<Instant> periods;
  peri->Get(0, periods);
    
  while(count <= mo_no){
    int index1 = GetRandom() % genloc_list.size();
    GenLoc loc1 = genloc_list[index1];
    int index2 = GetRandom() % genloc_list.size();
    GenLoc loc2 = genloc_list[index2];

    if(index1 == index2) continue;

    MPoint* mo = new MPoint(0);
    mo->StartBulkLoad();
    GenMO* genmo = new GenMO(0);
    genmo->StartBulkLoad();
  
    
    Instant start_time = periods.start;
    int time_range = 14*60;//14 hours 
  
    if(count % 3 == 0)//////////one third on sunday 
      start_time.ReadFrom(periods.start.ToDouble() + 
                        (GetRandom() % time_range)/(24.0*60.0));
    else  ////////////two third on Monday 
      start_time.ReadFrom(periods.end.ToDouble() -
                        (GetRandom() % time_range)/(24.0*60.0));
    ////////////////////////////////////////////////////////////////////////
    /////////////1 map the two positions to gpoints/////////////////////////
    ///////////////////////////////////////////////////////////////////////
    vector<GPoint> gpoint_list;
    vector<Point> p_list;
    PaveLoc2GPoint(loc1, loc2, sp, rel, btree, gpoint_list, p_list);
    GPoint gp1 = gpoint_list[0];
    GPoint gp2 = gpoint_list[1];
    Point start_loc = p_list[0];
    Point end_loc = p_list[1];
    //////////////////////////////////////////////////////////////////////
    ////////////2 connect the path to the end point//////////////////////
    //////////////////////////////////////////////////////////////////////

    ConnectStartMove(loc1, start_loc, mo, genmo, start_time, pm, mode);

    //////////////////////////////////////////////////////////////////////
    ///////////3 shortest path between two gpoints////////////////////////
    //////////////////////////////////////////////////////////////////////
    GLine* gl = new GLine(0);
    gp1.ShortestPath(&gp2, gl);////////compute the shortest path 

    if(gl->GetLength() < min_len){
      delete gl;
      continue;
    }

    BusRoute* br = new BusRoute(rn, NULL,NULL);
    GLine* newgl = new GLine(0);
    br->ConvertGLine(gl, newgl);
    ConnectGP1GP2(rn, start_loc, newgl, mo, genmo, start_time, 
                  speed_rel, mode);

    delete newgl;

    delete br;
    delete gl;

    //////////////////////////////////////////////////////////////////////
    ////////////4 connect the path to the end point//////////////////////
    //////////////////////////////////////////////////////////////////////

    ConnectEndMove(end_loc, loc2, mo, genmo, start_time, pm, mode);

    ////////////////////////////////////////////////////////////////////

    count++;

    mo->EndBulkLoad();
    genmo->EndBulkLoad();

    trip1_list.push_back(*genmo);
    trip2_list.push_back(*mo);

    delete mo;
    delete genmo; 
  }

  sp->ClosePavement(pm);
  sp->CloseRoadNetwork(rn);

}

/*
map the two points on the pavement to loations on the road network 

*/
void GenMObject::PaveLoc2GPoint(GenLoc loc1, GenLoc loc2, Space* sp, 
                                Relation* rel,
                      BTree* btree, vector<GPoint>& gpoint_list, 
                                vector<Point>& p_list)
{
    Network* rn = sp->LoadRoadNetwork(IF_LINE);

  /////////////////////////////////////////////////////////////////////
    CcInt* search_id1 = new CcInt(true, loc1.GetOid());
    BTreeIterator* btree_iter1 = btree->ExactMatch(search_id1);
    vector<int> route_id_list1;
    while(btree_iter1->Next()){
      Tuple* tuple = rel->GetTuple(btree_iter1->GetId(), false);
      int rid = ((CcInt*)tuple->GetAttribute(DualGraph::RID))->GetIntval();
      route_id_list1.push_back(rid); 
      tuple->DeleteIfAllowed();
    }
    delete btree_iter1;
    delete search_id1;

//     for(unsigned int i = 0;i < route_id_list1.size();i++)
//       cout<<route_id_list1[i]<<endl;

    Point p1(true, loc1.GetLoc().loc1, loc1.GetLoc().loc2);

    Walk_SP* wsp1 = new Walk_SP();
    wsp1->PaveLocToGPoint(&p1, rn, route_id_list1);
    GPoint gp1 = wsp1->gp_list[0];
    Point gp_loc1 = wsp1->p_list[0];
    delete wsp1;
    /////////////////////////////////////////////////////////////////////////

    CcInt* search_id2 = new CcInt(true, loc2.GetOid());
    BTreeIterator* btree_iter2 = btree->ExactMatch(search_id2);
    vector<int> route_id_list2;
    while(btree_iter2->Next()){
        Tuple* tuple = rel->GetTuple(btree_iter2->GetId(), false);
        int rid = ((CcInt*)tuple->GetAttribute(DualGraph::RID))->GetIntval();
        route_id_list2.push_back(rid); 
        tuple->DeleteIfAllowed();
    }
    delete btree_iter2;
    delete search_id2;

//     for(unsigned int i = 0;i < route_id_list2.size();i++)
//       cout<<route_id_list2[i]<<endl;

    Point p2(true, loc2.GetLoc().loc1, loc2.GetLoc().loc2);

    Walk_SP* wsp2 = new Walk_SP();
    wsp2->PaveLocToGPoint(&p2, rn, route_id_list2);
    GPoint gp2 = wsp2->gp_list[0];
    Point gp_loc2 = wsp2->p_list[0];
    delete wsp2;
    ///////////////////////////////////////////////////////////////////////
    sp->CloseRoadNetwork(rn);

     /////////////////////for debuging/////////////////////////////////
    //////////start and end locations on the pavement////////////////

//      loc_list1.push_back(p1);
//    loc_list1.push_back(p2);
//    loc_list1.push_back(gp_loc1);

    ///////////////start and end gpoint ////////////////////////////////
//    gp_list.push_back(gp1);
//    gp_list.push_back(gp2);

    ////////////////////start and end point in space/////////////////////
//    loc_list2.push_back(gp_loc1);
//    loc_list2.push_back(gp_loc2);

//    loc_list2.push_back(gp_loc2);
//    loc_list2.push_back(p2);

    /////////////////////////////////////////////////////////////////////
    gpoint_list.push_back(gp1);
    gpoint_list.push_back(gp2);
    p_list.push_back(gp_loc1);
    p_list.push_back(gp_loc2);

}

/*
build the connection from a point on the pavement to the road network point
subpath1: on the pavement
subpath2: in the free space (mode: car)
loc1: point on the pavement; end loc: point maps to a gpoint 

*/
void GenMObject::ConnectStartMove(GenLoc loc1, Point end_loc, MPoint* mo,
                        GenMO* genmo, Instant& start_time, 
                                  Pavement* pm, string mode)
{
  const double delta_dist = 0.01;
  DualGraph* dg = pm->GetDualGraph();

  Point start_loc(true, loc1.GetLoc().loc1, loc1.GetLoc().loc2);

  HalfSegment hs(true, start_loc, end_loc);
  Line* l = new Line(0);
  l->StartBulkLoad();
  hs.attr.edgeno = 0;
  *l += hs;
  hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
  *l += hs;
  l->EndBulkLoad();

//  cout<<"gp "<<end_loc<<" pave p"<<start_loc<<endl;

  vector<Line> line_list; 
  dg->LineIntersectTri(l, line_list);
  
  if(line_list.size() == 0){/////start location is on the pavement border 
      double dist = start_loc.Distance(end_loc);
      double slowspeed = 10.0*1000.0/3600.0;
      double time = dist/slowspeed;///// define as car or taxi
    //////////////////////////////////////////////////////////////
      if(dist > delta_dist){//ingore too small segment 
          Instant st = start_time;
          Instant et = start_time; 
          Interval<Instant> up_interval; 
          et.ReadFrom(st.ToDouble() + time*1.0/(24.0*60.0*60));

          ////////////////////create a upoint////////////////////////
          up_interval.start = st;
          up_interval.lc = true;
          up_interval.end = et;
          up_interval.rc = false; 
          UPoint* up = new UPoint(up_interval, start_loc, end_loc);
          mo->Add(*up);
          delete up; 
          start_time = et;
          /////////////generic unit/// tm--car or taxi//////////////////////
          Loc loc1(start_loc.GetX(), start_loc.GetY());
          Loc loc2(end_loc.GetX(), end_loc.GetY());
          GenLoc gloc1(0, loc1);
          GenLoc gloc2(0, loc2);
          int tm = GetTM(mode);
          UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
          genmo->Add(*unit); 
          delete unit; 
      }
    delete l;
    pm->CloseDualGraph(dg);
    return;
  }

  assert(line_list.size() > 0);
  /////////////////////////////////////////////////////////////////////
  ///////////////////get the point on the pavement border/////////////
  ///////////////////intersection line inside pavement////////////////
  /////////////////////////////////////////////////////////////////////
  Line* line = new Line(0);
  for(unsigned int i = 0;i < line_list.size();i++){
    Line* temp = new Line(0);
    line->Union(line_list[i], *temp);
    *line = *temp;
    delete temp;
  }  

     ///////////////////////////////////////////////////////////////
   ////////we reference to the big overall large polygon/////////////
   //////////////////////////////////////////////////////////////////
   
   int gen_mo_ref_id = dg->min_tri_oid_1 + dg->GetNodeRel()->GetNoTuples() + 1;
    Rectangle<2> bbox = dg->rtree_node->BoundingBox();


//  cout<<line->Length()<<" size "<<line->Size()<<endl; 
  ///////////////////////////////////////////////////////////////////////
  //////////// the intersection line still belongs to pavement area//////
  //////////////////////////////////////////////////////////////////////
  if(fabs(l->Length() - line->Length()) < delta_dist){///////one movement

//        cout<<"still on pavement "<<endl;

        double dist = start_loc.Distance(end_loc);
        double time = dist/1.0;///// define as walk
    //////////////////////////////////////////////////////////////
       if(dist > delta_dist){//ingore too small segment 

          Instant st = start_time;
          Instant et = start_time; 
          Interval<Instant> up_interval; 
          et.ReadFrom(st.ToDouble() + time*1.0/(24.0*60.0*60));

          ////////////////////create a upoint////////////////////////
          up_interval.start = st;
          up_interval.lc = true;
          up_interval.end = et;
          up_interval.rc = false; 
          UPoint* up = new UPoint(up_interval, start_loc, end_loc);
          mo->Add(*up);
          delete up; 
          start_time = et;
          ////////////////generic unit/// tm--walk////////////////////////
          Loc loc1(start_loc.GetX() - bbox.MinD(0), 
                   start_loc.GetY() - bbox.MinD(1));
          Loc loc2(end_loc.GetX() - bbox.MinD(0),
                   end_loc.GetY() - bbox.MinD(1));
          GenLoc gloc1(gen_mo_ref_id, loc1);
          GenLoc gloc2(gen_mo_ref_id, loc2);
          int tm = GetTM("Walk");
          UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
          genmo->Add(*unit); 
          delete unit; 
        }
  }else{//////////two movements 

  vector<MyPoint> mp_list;
  for(int i = 0;i < line->Size();i++){
    HalfSegment hs;
    line->Get(i, hs);
    if(!hs.IsLeftDomPoint())continue;
    Point lp = hs.GetLeftPoint();
    Point rp = hs.GetRightPoint();
    MyPoint mp1(lp, lp.Distance(end_loc));
    mp_list.push_back(mp1);
    MyPoint mp2(rp, rp.Distance(end_loc));
    mp_list.push_back(mp2);
  }
  sort(mp_list.begin(), mp_list.end());

//    for(unsigned int i = 0;i < mp_list.size();i++)
//      mp_list[i].Print();
//   cout<<endl;
  Point middle_loc = mp_list[0].loc;
  ////////////////////movement---1//////from pavement to border/////////////
      double dist1 = start_loc.Distance(middle_loc);
      double time1 = dist1/1.0;///// define as walk
      if(dist1 > delta_dist){//ingore too small segment 

          Instant st = start_time;
          Instant et = start_time; 
          Interval<Instant> up_interval; 
          et.ReadFrom(st.ToDouble() + time1*1.0/(24.0*60.0*60));

          ////////////////////create a upoint////////////////////////
          up_interval.start = st;
          up_interval.lc = true;
          up_interval.end = et;
          up_interval.rc = false; 
          UPoint* up = new UPoint(up_interval, start_loc, middle_loc);
          mo->Add(*up);
          delete up; 
          start_time = et;
          ////////////////generic unit////////////
          /////////////tm--walk///////////////////
          //////////////////////////////////////////
          Loc loc1(start_loc.GetX() - bbox.MinD(0), 
                   start_loc.GetY() - bbox.MinD(1));
          Loc loc2(middle_loc.GetX() - bbox.MinD(0),
                   middle_loc.GetY() - bbox.MinD(1));
          GenLoc gloc1(gen_mo_ref_id, loc1);
          GenLoc gloc2(gen_mo_ref_id, loc2);
          int tm = GetTM("Walk");
          UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
          genmo->Add(*unit); 
          delete unit; 
      }

  ////////////////movement---2/////from border to road line ///free space/////
      double dist2 = middle_loc.Distance(end_loc);
      double lowspeed = 10.0*1000/3600.0;
      double time2 = dist2/lowspeed;///// define as car (slow speed)
      if(dist2 > delta_dist){//ingore too small segment 

          Instant st = start_time;
          Instant et = start_time; 
          Interval<Instant> up_interval; 
          et.ReadFrom(st.ToDouble() + time2*1.0/(24.0*60.0*60));

          ////////////////////create a upoint////////////////////////
          up_interval.start = st;
          up_interval.lc = true;
          up_interval.end = et;
          up_interval.rc = false; 
          UPoint* up = new UPoint(up_interval, middle_loc, end_loc);
          mo->Add(*up);
          delete up; 
          start_time = et;
          ////////////////generic unit////////////
          /////////////tm--car or taxi///////////////////
          //////////////////////////////////////////
          Loc loc1(middle_loc.GetX(), middle_loc.GetY());
          Loc loc2(end_loc.GetX(), end_loc.GetY());
          GenLoc gloc1(0, loc1);///////////0 -- free space 
          GenLoc gloc2(0, loc2);///////////0 -- free space 
          int tm = GetTM(mode);
          UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
          genmo->Add(*unit);
          delete unit;

      }
  } 
  //////////////////for debuging show the line//////////////////////////////
//  cout<<line->Length()<<endl;
//  line_list1.push_back(*line);
//  line_list1.push_back(*l);

  delete l;
  delete line;
  //////////////////////////////////////////////////////////////////////

  pm->CloseDualGraph(dg);
}

/*
build the moving object on the road network

*/
void GenMObject::ConnectGP1GP2(Network* rn, Point start_loc, GLine* newgl, 
                               MPoint* mo, GenMO* genmo, Instant& start_time,
                     Relation* rel, string mode)
{

//    const double delta_dist = 0.01;

    for(int i = 0;i < newgl->Size();i++){
        RouteInterval* ri = new RouteInterval();
        newgl->Get(i, *ri);
        int rid = ri->GetRouteId();
        Tuple* speed_tuple = rel->GetTuple(rid, false);
     assert(((CcInt*)speed_tuple->GetAttribute(SPEED_RID))->GetIntval() == rid);
        double speed = 
        ((CcReal*)speed_tuple->GetAttribute(SPEED_VAL))->GetRealval();
        speed_tuple->DeleteIfAllowed();

      if(speed < 0.0)speed = 10.0;
      speed = speed * 1000.0/3600.0;// meters in second 
//    cout<<"rid "<<rid<<" new speed "<<speed<<endl;
      SimpleLine* sl = new SimpleLine(0);
      rn->GetLineValueOfRouteInterval(ri, sl);
    ////////////////////////////////////////////////////////////////////
    /////////////get the subline of this route interval////////////////
    ///////////////////////////////////////////////////////////////////
      SpacePartition* sp = new SpacePartition();
      vector<MyHalfSegment> seq_halfseg; //reorder it from start to end
      sp->ReorderLine(sl, seq_halfseg);
      delete sp;
    //////////////////////////////////////////////////////////////////
      delete sl;

    /////////////////////////////////////////////////////////////////////

      Interval<Instant> unit_interval;
      unit_interval.start = start_time;
      unit_interval.lc = true;

      Point temp_sp1 = seq_halfseg[0].from; 
      Point temp_sp2 = seq_halfseg[seq_halfseg.size() - 1].to; 
      double d1 = start_loc.Distance(temp_sp1);
      double d2 = start_loc.Distance(temp_sp2);
//      if(start_loc.Distance(temp_sp1) < delta_dist){
      if(d1 < d2 ){
        CreateCarTrip1(mo, seq_halfseg, start_time, speed);

        start_loc = temp_sp2;
//      }else if(start_loc.Distance(temp_sp2) < delta_dist){

        }else if(d1 > d2){
        CreateCarTrip2(mo, seq_halfseg, start_time, speed);
        start_loc = temp_sp1;
      }else{
        cout<<"start loc "<<start_loc
            <<" p1 "<<temp_sp1<<" p2 "<<temp_sp2<<endl;
        assert(false);
      }

      unit_interval.end = start_time;
      unit_interval.rc = false;
      
    //////////////////////////////////////////////////////////////
    ////////////////generic units/////////////////////////////////
    //////////////////////////////////////////////////////////////
      Loc loc1(ri->GetStartPos(), -1); 
      Loc loc2(ri->GetEndPos(), -1); 
      GenLoc gloc1(ri->GetRouteId(), loc1);
      GenLoc gloc2(ri->GetRouteId(), loc2);
      int tm = GetTM(mode); 

    //////////////////////////////////////////////////////////////////
    /////////////correct way to create UGenLoc///////////////////////
    //////////////////////////////////////////////////////////////////
      UGenLoc* unit = new UGenLoc(unit_interval, gloc1, gloc2, tm);
      genmo->Add(*unit); 
      delete unit; 
      delete ri;

  }
   /////////////////////////////////////////////////////////////////////
   //////////////////for debuging////////////////////////////////////
   //////////////////////////////////////////////////////////////////
   
//     Line* l = new Line(0);
//     newgl->Gline2line(l);
//     Line* res = new Line(0);
//     l->Union(line_list1[line_list1.size() - 1], *res);
//     line_list1[line_list1.size() - 1] = *res;
//     delete res;
//     delete l;

}


/*
build the connection from a point on the pavement to the road network point
subpath1: on the pavement
subpath2: in the free space (mode: car)
loc: point on the pavement; start loc: point maps to a gpoint 

*/
void GenMObject::ConnectEndMove(Point start_loc, GenLoc loc, MPoint* mo, 
                        GenMO* genmo, Instant& start_time, 
                                Pavement* pm, string mode)
{
  const double delta_dist = 0.01;
  DualGraph* dg = pm->GetDualGraph();

  Point end_loc(true, loc.GetLoc().loc1, loc.GetLoc().loc2);

  HalfSegment hs(true, start_loc, end_loc);
  Line* l = new Line(0);
  l->StartBulkLoad();
  hs.attr.edgeno = 0;
  *l += hs;
  hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
  *l += hs;
  l->EndBulkLoad();

//  cout<<"gp "<<end_loc<<" pave p"<<start_loc<<endl;

  vector<Line> line_list; 
  dg->LineIntersectTri(l, line_list);

  if(line_list.size() == 0){///////the point on the pavement is on the border

      double dist = start_loc.Distance(end_loc);
      double slowspeed = 10.0*1000.0/3600.0;
      double time = dist/slowspeed;///// define as car or taxi
    //////////////////////////////////////////////////////////////
      if(dist > delta_dist){//ingore too small segment 
          Instant st = start_time;
          Instant et = start_time; 
          Interval<Instant> up_interval; 
          et.ReadFrom(st.ToDouble() + time*1.0/(24.0*60.0*60));

          ////////////////////create a upoint////////////////////////
          up_interval.start = st;
          up_interval.lc = true;
          up_interval.end = et;
          up_interval.rc = false; 
          UPoint* up = new UPoint(up_interval, start_loc, end_loc);
          mo->Add(*up);
          delete up; 
          start_time = et;
          /////////////generic unit/// tm--car or taxi//////////////////////
          Loc loc1(start_loc.GetX(), start_loc.GetY());
          Loc loc2(end_loc.GetX(), end_loc.GetY());
          GenLoc gloc1(0, loc1);
          GenLoc gloc2(0, loc2);
          int tm = GetTM(mode);
          UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
          genmo->Add(*unit); 
          delete unit; 
      }
    delete l;
    pm->CloseDualGraph(dg);
    return;
  }

  assert(line_list.size() > 0); 

  /////////////////////////////////////////////////////////////////////
  ///////////////////get the point on the pavement border/////////////
  //////////////////intersection line inside the pavement//////////////
  /////////////////////////////////////////////////////////////////////
  Line* line = new Line(0);
  for(unsigned int i = 0;i < line_list.size();i++){
    Line* temp = new Line(0);
    line->Union(line_list[i], *temp);
    *line = *temp;
    delete temp;
  }

   ///////////////////////////////////////////////////////////////
   ////////we reference to the big overall large polygon/////////////
   //////////////////////////////////////////////////////////////////

   int gen_mo_ref_id = dg->min_tri_oid_1 + dg->GetNodeRel()->GetNoTuples() + 1;
   Rectangle<2> bbox = dg->rtree_node->BoundingBox();

  ///////////////////////////////////////////////////////////////////////
  //////////// the intersection line still belongs to pavement area//////
  //////////////////////////////////////////////////////////////////////
  if(fabs(l->Length() - line->Length()) < delta_dist){///////one movement

//        cout<<"still on pavement "<<endl;

        double dist = start_loc.Distance(end_loc);
        double time = dist/1.0;///// define as walk
    //////////////////////////////////////////////////////////////
       if(dist > delta_dist){//ingore too small segment 

          Instant st = start_time;
          Instant et = start_time; 
          Interval<Instant> up_interval; 
          et.ReadFrom(st.ToDouble() + time*1.0/(24.0*60.0*60));

          ////////////////////create a upoint////////////////////////
          up_interval.start = st;
          up_interval.lc = true;
          up_interval.end = et;
          up_interval.rc = false; 
          UPoint* up = new UPoint(up_interval, start_loc, end_loc);
          mo->Add(*up);
          delete up; 
          start_time = et;
          ////////////////generic unit/// tm--walk////////////////////////
          Loc loc1(start_loc.GetX() - bbox.MinD(0), 
                   start_loc.GetY() - bbox.MinD(1));
          Loc loc2(end_loc.GetX() - bbox.MinD(0),
                   end_loc.GetY() - bbox.MinD(1));
          GenLoc gloc1(gen_mo_ref_id, loc1);
          GenLoc gloc2(gen_mo_ref_id, loc2);
          int tm = GetTM("Walk");
          UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
          genmo->Add(*unit); 
          delete unit; 
        }
  }else{  //////////two movements 
        vector<MyPoint> mp_list;
        for(int i = 0;i < line->Size();i++){
            HalfSegment hs;
            line->Get(i, hs);
            if(!hs.IsLeftDomPoint())continue;
            Point lp = hs.GetLeftPoint();
            Point rp = hs.GetRightPoint();
            MyPoint mp1(lp, lp.Distance(start_loc));
            mp_list.push_back(mp1);
            MyPoint mp2(rp, rp.Distance(start_loc));
            mp_list.push_back(mp2);
        }
        sort(mp_list.begin(), mp_list.end());
        Point middle_loc = mp_list[0].loc;
      //////////////movement---1//////from road line to pavemet border///////
        double dist1 = start_loc.Distance(middle_loc);
        double lowspeed = 10*1000/3600.0;
        double time1 = dist1/lowspeed;///// define as car or taxi 
        if(dist1 > delta_dist){//ingore too small segment 

          Instant st = start_time;
          Instant et = start_time; 
          Interval<Instant> up_interval; 
          et.ReadFrom(st.ToDouble() + time1*1.0/(24.0*60.0*60));

          ////////////////////create a upoint////////////////////////
          up_interval.start = st;
          up_interval.lc = true;
          up_interval.end = et;
          up_interval.rc = false; 
          UPoint* up = new UPoint(up_interval, start_loc, middle_loc);
          mo->Add(*up);
          delete up; 
          start_time = et;
          ////////////////generic unit////////////
          /////////////tm--car or taxi///////////////////
          //////////////////////////////////////////
          Loc loc1(start_loc.GetX(), start_loc.GetY());
          Loc loc2(middle_loc.GetX(), middle_loc.GetY());
          GenLoc gloc1(0, loc1);///////////0 -- free space 
          GenLoc gloc2(0, loc2);///////////0 -- free space 
          int tm = GetTM(mode);
          UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
          genmo->Add(*unit); 
          delete unit; 
        }

        ///movement---2/////from pavement border to pavement loc///free space//
        double dist2 = middle_loc.Distance(end_loc);
        double time2 = dist2/1.0;///// define as walk
        if(dist2 > delta_dist){//ingore too small segment 

          Instant st = start_time;
          Instant et = start_time; 
          Interval<Instant> up_interval; 
          et.ReadFrom(st.ToDouble() + time2*1.0/(24.0*60.0*60));

          ////////////////////create a upoint////////////////////////
          up_interval.start = st;
          up_interval.lc = true;
          up_interval.end = et;
          up_interval.rc = false; 
          UPoint* up = new UPoint(up_interval, middle_loc, end_loc);
          mo->Add(*up);
          delete up; 
          start_time = et;
          ////////////////generic unit////////////
          /////////////tm--walk///////////////////
          //////////////////////////////////////////
          Loc loc1(middle_loc.GetX() - bbox.MinD(0), 
                   middle_loc.GetY() - bbox.MinD(1));
          Loc loc2(end_loc.GetX() - bbox.MinD(0), 
                   end_loc.GetY() - bbox.MinD(1));
          GenLoc gloc1(gen_mo_ref_id, loc1);///walk on the overall pavement
          GenLoc gloc2(gen_mo_ref_id, loc2);///walk on the overall pavement
          int tm = GetTM("Walk");
          UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
          genmo->Add(*unit); 
          delete unit; 
        }
  }

  //////////////////for debuging show the line//////////////////////////////
//  cout<<line->Length()<<endl;

//   Line* res = new Line(0);
//   l->Union(line_list1[line_list1.size() - 1], *res);
//   line_list1[line_list1.size() - 1] = *res;
//   delete res;


  delete l;
  delete line;
  //////////////////////////////////////////////////////////////////////
  pm->CloseDualGraph(dg);

}

/*
moving objects with mode bus and walk.
randomly select two locations on the pavement and their closest bus stops.
build the connection between them. this is not so correct for real trip 
plannings because the closest bus stops may not involve the best trip.
but it is ok to generate generic moving objects

*/
void GenMObject::GenerateGenMO_BusWalk(Space* sp, Periods* peri, int mo_no,
                             Relation* rel1, Relation* rel2, 
                             R_Tree<2,TupleId>* rtree, string mode)
{
    Pavement* pm = sp->LoadPavement(IF_REGION);
    BusNetwork* bn = sp->LoadBusNetwork(IF_BUSNETWORK);
    
    vector<GenLoc> genloc_list;
    GenerateLocPave(pm, 2*mo_no, genloc_list);///generate locations on pavements
    DualGraph* dg = pm->GetDualGraph();
    VisualGraph* vg = pm->GetVisualGraph();

    Interval<Instant> periods;
    peri->Get(0, periods);
    int count = 1;
    while(count <= mo_no){
        ////////////////////////////////////////////////////////////
        ///////////// initialization////////////////////////////////
        ////////////////////////////////////////////////////////////
        MPoint* mo = new MPoint(0);
        GenMO* genmo = new GenMO(0);
        mo->StartBulkLoad();
        genmo->StartBulkLoad();
        //////////////////////////////////////////////////////////////
        ///////////////set start time/////////////////////////////////
        /////////////////////////////////////////////////////////////
        Instant start_time = periods.start;
        int time_range = 14*60;//14 hours 
        bool time_type;
        if(count % 3 == 0) time_type = true;
        else time_type = false;

        if(time_type)
            start_time.ReadFrom(periods.start.ToDouble() + 
                        (GetRandom() % time_range)/(24.0*60.0));
        else
            start_time.ReadFrom(periods.end.ToDouble() -
                        (GetRandom() % time_range)/(24.0*60.0));

        int index1 = GetRandom() % genloc_list.size();
        GenLoc loc1 = genloc_list[index1];
        int index2 = GetRandom() % genloc_list.size();
        GenLoc loc2 = genloc_list[index2];
        ///////////////debuging//////////////////////////////
        /*
          Loc temp1(1.0, 2.0);
          Loc temp2(2.0, 3.0);
          loc1.Set(123, temp1);
          loc2.Set(232, temp2);
        */
        /////////////////////////////////////////////////////
        //////////////////////////////////////////////
//        cout<<"genloc1 "<<loc1<<" genloc2 "<<loc2<<endl; 
        ////////////////////////////////////////////////////////////////////
        //////1. find closest bus stops to the two points on the pavement///
        ///////////////////////////////////////////////////////////////////
        Bus_Stop bs1(true, 0, 0, true);
        vector<Point> ps_list1;//1 point on the pavement 2 point of bus stop
        GenLoc gloc1(0, Loc(0, 0));//bus stop on pavement by genloc 
        bool b1 = true;
        b1 = NearestBusStop(loc1,  rel2, rtree, bs1, ps_list1, gloc1, true);

        Bus_Stop bs2(true, 0, 0, true);
        vector<Point> ps_list2;//1 point on the pavement 2 point of bus stop
        GenLoc gloc2(0, Loc(0, 0));//bus stop on pavement by genloc
        bool b2 = true;
        b2 = NearestBusStop(loc2,  rel2, rtree, bs2, ps_list2, gloc2, false);

        if((b1 && b2) == false) continue;
        /////////////////////////////////////////////////////////////////
        /////////2 connect start location to start bus stop/////////////
        ////////////////////////////////////////////////////////////////
        Line* res_path = new Line(0);
        ConnectStartBusStop(dg, vg, rel1, loc1, ps_list1, gloc1.GetOid(),
                            genmo, mo, start_time, res_path);

        /////////////////////////////////////////////////////////////////
        //////3. get the path in bus network/////////////////////////////
        /////////////////////////////////////////////////////////////////
//        cout<<"time type "<<time_type<<endl; 
//        cout<<"bs1 "<<bs1<<" bs2 "<<bs2<<endl; 
        BNNav* bn_nav = new BNNav(bn);
        if(count % 2 == 0)
            bn_nav->ShortestPath_Time2(&bs1, &bs2, &start_time);
        else
            bn_nav->ShortestPath_Transfer2(&bs1, &bs2, &start_time);

        if(bn_nav->path_list.size() == 0){
//          cout<<"two unreachable bus stops"<<endl;
          mo->EndBulkLoad();
          genmo->EndBulkLoad();
          delete mo;
          delete genmo;
          delete bn_nav;
          delete res_path;
          continue;
        }


        int last_walk_id = ConnectTwoBusStops(bn_nav, ps_list1[1], ps_list2[1],
                           genmo, mo, start_time, dg, res_path);

        ///////////////////////////////////////////////////////////
        if(last_walk_id > 0){
          //////change the last bus stop/////////
          ///change  ps list2, gloc2.oid ///
          Bus_Stop cur_bs1(true, 0, 0, true);
          StringToBusStop(bn_nav->bs1_list[last_walk_id], 
                          cur_bs1);

//           Bus_Stop cur_bs2(true, 0, 0, true);
//           StringToBusStop(bn_nav->bs2_list[bn_nav->bs2_list.size() - 1], 
//                           cur_bs2);
//          cout<<"cur_bs1 "<<cur_bs1<<" cur_bs2 "<<cur_bs2<<endl;

            ChangeEndBusStop(bn, dg, cur_bs1, ps_list2, gloc2, rel2, rtree);
        }

        ////////////////////////////////////////////////////////////
        delete bn_nav;
        ///////////////////////////////////////////////////////////////////
        /////////////4 connect end location to last bus stop////////////////
        //////////////////////////////////////////////////////////////////
        ConnectEndBusStop(dg, vg, rel1, loc2, ps_list2, gloc2.GetOid(),
                          genmo, mo, start_time, res_path);
        ///////////////////////////////////////////////////////////////////////
        cout<<count<<" moving object "<<endl;
        count++;

        mo->EndBulkLoad();
        genmo->EndBulkLoad();

        line_list1.push_back(*res_path);
        trip1_list.push_back(*genmo);
        trip2_list.push_back(*mo);

        delete res_path;
        delete mo;
        delete genmo;
    }

    pm->CloseDualGraph(dg);
    pm->CloseVisualGraph(vg);
    sp->CloseBusNetwork(bn);
    sp->ClosePavement(pm);

}

/*
find the nearest bus stops to the query location with e.g., 200 meters.
this method is not for trip planning with walk and bus
in that case, it should define a distance e.g., 200 meters and find all 
possible connections.
but here, it finds the closet, nearest bus stop to the location in the pavement

*/
bool GenMObject::NearestBusStop(GenLoc loc, Relation* rel,
                      R_Tree<2,TupleId>* rtree, Bus_Stop& res, 
                      vector<Point>& ps_list, GenLoc& bs_gloc, bool start_pos)
{
  Point p(true, loc.GetLoc().loc1, loc.GetLoc().loc2);
  SmiRecordId adr = rtree->RootRecordId();

  vector<int> tid_list;
  DFTraverse1(rtree, adr, rel, p, tid_list);

//  cout<<p<<" "<<tid_list.size()<<endl;
  
  if(tid_list.size() == 0) return false;
  
  vector<MyPoint_Tid> mp_tid_list;
  for(unsigned int i = 0;i < tid_list.size();i++){
    Tuple* tuple = rel->GetTuple(tid_list[i], false);
    Point* q = (Point*)tuple->GetAttribute(BN::BN_PAVE_LOC2);
    MyPoint_Tid mp_tid(*q, q->Distance(p), tid_list[i]);
    mp_tid_list.push_back(mp_tid);
    tuple->DeleteIfAllowed();
  }
  sort(mp_tid_list.begin(), mp_tid_list.end());

//  for(unsigned int i = 0;i < mp_tid_list.size();i++)
//    mp_tid_list[i].Print();
  Tuple* bs_pave = rel->GetTuple(mp_tid_list[0].tid, false);
  Bus_Stop* bs = (Bus_Stop*)bs_pave->GetAttribute(BN::BN_BUSSTOP);
  Point* bs_loc = (Point*)bs_pave->GetAttribute(BN::BN_BUSLOC);
  GenLoc* bs_loc2 = (GenLoc*)bs_pave->GetAttribute(BN::BN_PAVE_LOC1);

  res = *bs;
  ps_list.push_back(mp_tid_list[0].loc);
  ps_list.push_back(*bs_loc);
  bs_gloc = *bs_loc2;
  bs_pave->DeleteIfAllowed();

  ///////////////////////////////////////////////////////////////////////
  //////////////for debuging, they are located on the pavement//////////
  //////////////////////////////////////////////////////////////////////
  if(start_pos){
//    loc_list1.push_back(p);
//    loc_list2.push_back(mp_tid_list[0].loc);
  }else{
//    loc_list1.push_back(mp_tid_list[0].loc);
//    loc_list2.push_back(p);
  }
  ///////////////////////////////////////////////////////////////////////

  return true;
}

/*
find all points that their distance to query loc is smaller than the 
defined max dist

*/
void GenMObject::DFTraverse1(R_Tree<2,TupleId>* rtree, SmiRecordId adr, 
                             Relation* rel,
                             Point query_loc, vector<int>& tid_list)
{
  const double max_dist = 500.0;
  R_TreeNode<2,TupleId>* node = rtree->GetMyNode(adr,false,
                  rtree->MinEntries(0), rtree->MaxEntries(0));
  for(int j = 0;j < node->EntryCount();j++){
      if(node->IsLeaf()){
              R_TreeLeafEntry<2,TupleId> e =
                 (R_TreeLeafEntry<2,TupleId>&)(*node)[j];
              Tuple* dg_tuple = rel->GetTuple(e.info, false);
              Point* loc = (Point*)dg_tuple->GetAttribute(BN::BN_PAVE_LOC2);

              if(loc->Distance(query_loc) < max_dist){
                tid_list.push_back(e.info);
              }
              dg_tuple->DeleteIfAllowed();
      }else{
            R_TreeInternalEntry<2> e =
                (R_TreeInternalEntry<2>&)(*node)[j];
            if(query_loc.Distance(e.box) < max_dist){
                DFTraverse1(rtree, e.pointer, rel, query_loc, tid_list);
            }
      }
  }
  delete node;
}

/*
connect the start location to the nearest bus stop
walk + bus (free space)

*/

void GenMObject::ConnectStartBusStop(DualGraph* dg, VisualGraph* vg,
                                     Relation* rel, GenLoc genloc1,
                                     vector<Point> ps_list1, int oid,
                                GenMO* genmo, MPoint* mo, 
                                Instant& start_time, Line* res_path)
{
  Walk_SP* wsp = new Walk_SP(dg, vg, NULL, NULL);
  wsp->rel3 = rel;

  int no_triangle = dg->GetNodeRel()->GetNoTuples();
  int mini_oid = dg->min_tri_oid_1;

  Point p1(true, genloc1.GetLoc().loc1, genloc1.GetLoc().loc2);
  Point p2 = ps_list1[0];
  int oid1 = genloc1.GetOid() - mini_oid;
  assert(1 <= oid1 && oid1 <= no_triangle);

  int oid2 = oid - mini_oid;
//    cout<<"oid2 "<<oid2<<endl;
  assert(1 <= oid2 && oid2 <= no_triangle);

  Line* path = new Line(0);

  ///////////////////walk segment////////////////////////////////////
  wsp->WalkShortestPath2(oid1, oid2, p1, p2, path);
  

  ////////////////create moving objects///////////////////////////////////
  GenerateWalkMovement(dg, path, p1, genmo, mo, start_time);
  /////////////////////////////////////////////////////////////////////////
  
  /////////////////////////////////////////////////////////////////////////
  ////connection between bus stop and its mapping point on the pavement////
  //////////////////////for debuging/////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  
  Line* path2 = new Line(0);
  path2->StartBulkLoad();
  HalfSegment hs(true, ps_list1[0], ps_list1[1]);
  hs.attr.edgeno = 0;
  *path2 += hs;
  hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
  *path2 += hs;
  path2->EndBulkLoad();

  path->Union(*path2, *res_path);

  delete path2;


  ///////////////////////////////////////////////////////////////////////
  /////////////// create moving objects ////////////////////////////////
  ///////////////////////////////////////////////////////////////////////
  ShortMovement(genmo, mo, start_time, &ps_list1[0], &ps_list1[1]);

  delete path;
  delete wsp;
}

/*
create the moving object between bus stops in bus network

*/
int GenMObject::ConnectTwoBusStops(BNNav* bn_nav, Point sp, Point ep,
                                    GenMO* genmo, MPoint* mo, 
                                    Instant& start_time, 
                                    DualGraph* dg, Line* res_path)
{
    const double delta_t = 0.01;//seconds
    const double delta_dist = 0.01;

    Line* l = new Line(0); ////////trajectory in bus network
    l->StartBulkLoad();
    int edgeno = 0;

    ////////////////////////////////////////////////////////////////////
    ///////////////////find the last walk segment///////////////////////
    ///////////////////////////////////////////////////////////////////
    int last_walk_id = -1;
    for(int i = bn_nav->path_list.size() - 1; i >= 0; i--){
        int tm = GetTM(bn_nav->tm_list[i]);
        if(tm < 0 ) continue;
        if(tm == TM_WALK){
          last_walk_id = i;
        }else
          break;
    }
    ////////////////////////////////////////////////////////////////////

    Bus_Stop last_bs(false, 0, 0, true);
    MPoint mo_bus(0);
    int mo_bus_index = -1;
    int mobus_oid = 0;

    for(unsigned int i = 0;i < bn_nav->path_list.size();i++){
      if(last_walk_id == (int)i) break;/////stop at the last walk segment 

      SimpleLine* sl = &(bn_nav->path_list[i]);
      ////////////time cost: second///////////////////
      Bus_Stop bs1(true, 0, 0, true);
      StringToBusStop(bn_nav->bs1_list[i], bs1);

      Bus_Stop bs2(true, 0, 0, true);
      StringToBusStop(bn_nav->bs2_list[i], bs2);

      double t = bn_nav->time_cost_list[i];
      int tm = GetTM(bn_nav->tm_list[i]);
      /////////////////////////////////////////////////////////////
      Point* start_loc = new Point(true, 0, 0);
      bn_nav->bn->GetBusStopGeoData(&bs1, start_loc);
      Point* end_loc = new Point(true, 0, 0);
      bn_nav->bn->GetBusStopGeoData(&bs2, end_loc);

      /////filter the first part and transfer without movement ////////
      if(sl->Size() == 0 && AlmostEqual(t, 0.0)) continue;

//       cout<<sl->Length()<<" bs1 "<<bs1<<" bs2 "<<bs2
//           <<" time "<<t<<" tm "<<bn_nav->tm_list[i]<<endl;
//       cout<<"start time 1 "<<start_time<<endl;
      ///////////////////////////////////////////////////////////

      Instant temp_start_time = start_time; 

      if(tm == TM_WALK){

        Line* l1 = new Line(0);
        ///////////////////////////////////////////////////////////////////
        /////////////filter the first and last segment//////////////////////
        /////////////connection between bus stop and its mapping point//////
        ////this part is not considered as walk segment in the overall region//
        //////////////////////////////////////////////////////////////////
//        sl->toLine(*l1);

        Point new_start_loc;
        Point new_end_loc;
        bool init_start = false;
        bool init_end = false;
        l1->StartBulkLoad();
        int l_edgeno = 0;

        for(int j = 0;j < sl->Size();j++){
          HalfSegment hs1;
          sl->Get(j, hs1);
          if(!hs1.IsLeftDomPoint())continue;
          Point lp = hs1.GetLeftPoint();
          Point rp = hs1.GetRightPoint();
          if(start_loc->Distance(lp) < delta_dist){
            new_start_loc = rp;
            init_start = true;
            continue;
          }
          if(start_loc->Distance(rp) < delta_dist){
            new_start_loc = lp;
            init_start = true;
            continue;
          }
          if(end_loc->Distance(lp) < delta_dist){
            new_end_loc = rp;
            init_end = true;
            continue;
          }
          if(end_loc->Distance(rp) < delta_dist){
            new_end_loc = lp;
            init_end = true;
            continue;
          }
          HalfSegment hs2(true, lp, rp);
          hs2.attr.edgeno = l_edgeno++;
          *l1 += hs2;
          hs2.SetLeftDomPoint(!hs2.IsLeftDomPoint());
          *l1 += hs2;
        }
        l1->EndBulkLoad();

        ///////////////////////////////////////////////////////////////
        assert(init_start && init_end); 

        ShortMovement(genmo, mo, start_time, start_loc, &new_start_loc);

        GenerateWalkMovement(dg, l1, new_start_loc, genmo, mo, start_time);
        delete l1;

        ShortMovement(genmo, mo, start_time, &new_end_loc, end_loc);
        /////////////////////////////////
        MPoint temp_mo(0);
        mo_bus = temp_mo;
        mo_bus_index = -1;
        mobus_oid = 0;

      }else{
        assert(bs1.GetId() == bs2.GetId() && bs1.GetUp() == bs2.GetUp());

        if(sl->Size() == 0){////////////no movement in space

            Instant st = start_time;
            Instant et = start_time; 
            Interval<Instant> up_interval; 
            et.ReadFrom(st.ToDouble() + t*1.0/(24.0*60.0*60.0));
            up_interval.start = st;
            up_interval.lc = true;
            up_interval.end = et;
            up_interval.rc = false; 

//            if(AlmostEqual(t, 30.0)){///bus waiting at the bus stop
            if(fabs(t - 30.0) < delta_t){///bus waiting at the bus stop
                  ////////generic moving objects/////////////////
                  ////////reference to the bus///////////////////
                  /////with bs find br, with br.uoid find mobus///
                  //////////// mode = bus////////////////////////
                  ///////////////////////////////////////////////
                int mbus_oid = 
                      bn_nav->bn->GetMOBus_Oid(&bs1, start_loc, start_time);
                Loc loc1(-1.0, -1.0);
                Loc loc2(-1.0, -1.0);
                GenLoc gloc1(mbus_oid, loc1);
                GenLoc gloc2(mbus_oid, loc2);
                int tm = GetTM("Bus");
                UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
                genmo->Add(*unit); 
                delete unit; 

            }else{/////////waiting for transfer//////////////////////
                /////////generic moving objects///////////////////
                /////////referenc to free space oid = 0, mode = bus ////
                ////////////////////////////////////////////////////
                Loc loc1(start_loc->GetX(), start_loc->GetY());
                Loc loc2(start_loc->GetX(), start_loc->GetY());
                GenLoc gloc1(0, loc1);
                GenLoc gloc2(0, loc2);
                int tm = GetTM("Bus");
                UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
                genmo->Add(*unit); 
                delete unit; 
                ////////////////////////////////////////////////
                MPoint temp_mo(0);
                mo_bus = temp_mo;
                mo_bus_index = -1;
                mobus_oid = 0;
            }

            UPoint* up = new UPoint(up_interval,*start_loc, *start_loc);
            mo->Add(*up);
            delete up;

            start_time = et;

        }else{//////////moving with the bus 
//           cout<<"moving with bus"
//               <<" mo_bus size "<<mo_bus.GetNoComponents()<<endl;

          if(mo_bus.GetNoComponents() == 0){
            ////////////////////////////////////////
            //////////find the bus//////////////////
            //////////mode = bus////////////////////
            ////////////////////////////////////////
            assert(mobus_oid == 0);
            mobus_oid = 
              bn_nav->bn->GetMOBus_MP(&bs1, start_loc, start_time, mo_bus);
//            cout<<"bus mpoint size "<<mo_bus.GetNoComponents()
//                <<" mobus oid "<<mobus_oid<<endl;
            assert(mobus_oid > 0);
            int pos1 = -1;
            int pos2 = -1;
            /////////find the range in mpoint for bs1, bs2//////////////
            FindPosInMP(&mo_bus, start_loc, end_loc, pos1, pos2, 0);
            assert(pos1 >= 0 && pos2 >= 0 && pos1 <= pos2);
//            cout<<"pos1 "<<pos1<<" pos2 "<<pos2<<endl;
            /////////////////////////////////////////////////////////////
            ///////////////set up mpoint//////////////////////////////
            ////////////////////////////////////////////////////////////
//            cout<<"initial start time "<<start_time<<endl;
            SetMO_GenMO(&mo_bus, pos1, pos2, start_time, mo, genmo, mobus_oid);
            ///////////////////////////////////////////////////////////////
            mo_bus_index = pos2; 
            mo_bus_index++;/////////omit the 30 seconds waiting movement 

          }else{
            assert(last_bs.IsDefined());
            if(bs1.GetId() == last_bs.GetId() && 
               bs1.GetUp() == last_bs.GetUp()){////reference to the same bus

            int pos1 = -1;
            int pos2 = -1;
            /////////find the range in mpoint for bs1, bs2//////////////
            FindPosInMP(&mo_bus, start_loc, end_loc, pos1, pos2, mo_bus_index);
//            cout<<"pos1 "<<pos1<<" pos2 "<<pos2<<endl; 
            assert(pos1 >= 0 && pos2 >= 0 && pos1 <= pos2);

            SetMO_GenMO(&mo_bus, pos1, pos2, start_time, mo, genmo, mobus_oid);

            ///////////////////////////////////////////////////////////////
            mo_bus_index = pos2; 
            mo_bus_index++;/////////omit the 30 seconds waiting movement 

            }else{
                //////////doing transfer without any waiting time/////////////
             cout<<"seldomly happend"<<endl;
            //////////////////////////////////////////
            /////////////generic unit/////////////////
            //////////reference to bus, mode = bus////
            //////////////////////////////////////////
              MPoint temp_mo(0);
              mo_bus = temp_mo;
              mo_bus_index = -1;
              mobus_oid = 0;

              mobus_oid = 
                bn_nav->bn->GetMOBus_MP(&bs1, start_loc, start_time, mo_bus);

              assert(mobus_oid > 0);
              int pos1 = -1;
              int pos2 = -1;
            /////////find the range in mpoint for bs1, bs2//////////////
              FindPosInMP(&mo_bus, start_loc, end_loc, pos1, pos2, 0);
              assert(pos1 >= 0 && pos2 >= 0 && pos1 <= pos2);
             SetMO_GenMO(&mo_bus, pos1, pos2, start_time, mo, genmo, mobus_oid);
             ///////////////////////////////////////////////////////////////
              mo_bus_index = pos2; 
              mo_bus_index++;/////////omit the 30 seconds waiting movement 
            }
          } 

         ///////////remove this part when the above code works correctly//////
//         start_time.ReadFrom(temp_start_time.ToDouble() + 
//                            t*1.0/(24.0*60.0*60.0));
        }

      }

//      cout<<"start time 2 "<<start_time<<endl;

      for(int j = 0;j < sl->Size();j++){
        HalfSegment hs1;
        sl->Get(j, hs1);
        if(!hs1.IsLeftDomPoint()) continue;
        HalfSegment hs2(true, hs1.GetLeftPoint(), hs1.GetRightPoint());
        hs2.attr.edgeno = edgeno++;
        *l += hs2;
        hs2.SetLeftDomPoint(!hs2.IsLeftDomPoint());
        *l += hs2;
      }

      last_bs = bs2;
      delete start_loc;
      delete end_loc; 

    }////////end of big for 

    l->EndBulkLoad();

    Line* temp_l = new Line(0);
    l->Union(*res_path, *temp_l);
    *res_path = *temp_l;
    delete temp_l;

    delete l;

    return last_walk_id;
}


/*
get the bus stop from input string with the format
br: 1 stop: 13 UP

*/
void GenMObject::StringToBusStop(string str, Bus_Stop& bs)
{
  char* cstr = new char [str.size()+1];
  strcpy (cstr, str.c_str());

  char* p=strtok (cstr," ");
  int count = 0;
  
  int br_id = 0;
  int stop_id = 0;
  bool direction;
  bool init = false;
  while (p!=NULL)
  {
//    cout << p << endl;

    if(count == 1){
      br_id = atoi(p);
    }
    if(count == 3){
      stop_id = atoi(p);
    }
    if(count == 4){
        char down[] = "DOWN";
        char up[] = "UP";
        if(strcmp(p, down) == 0){
          direction = false;
        }else if(strcmp(p, up) == 0)
          direction = true;

        init = true;
    }
    count++;
    p=strtok(NULL," ");
  }

  delete[] cstr;

  assert(br_id > 0 && stop_id > 0 && init);
  bs.Set(br_id, stop_id, direction);
//  cout<<bs<<endl;

}


/*
the movement between bus stops on the bus network and its mapping points on
the pavement 

*/
void GenMObject::ShortMovement(GenMO* genmo, MPoint* mo, Instant& start_time,
                     Point* p1, Point* p2)
{

  Instant st = start_time;
  Instant et = start_time; 
  Interval<Instant> up_interval; 
  
  double dist = p1->Distance(*p2);
  double slow_speed = 10.0*1000.0/3600.0;
  double time = dist/slow_speed;
  et.ReadFrom(st.ToDouble() + time*1.0/(24.0*60.0*60));
  ////////////////////create a upoint////////////////////////
  up_interval.start = st;
  up_interval.lc = true;
  up_interval.end = et;
  up_interval.rc = false; 
  UPoint* up = new UPoint(up_interval, *p1, *p2);
  mo->Add(*up);
  delete up; 
  /////////////////generic units////////////////////////////////
  Loc loc1(p1->GetX(), p1->GetY());
  Loc loc2(p2->GetX(), p2->GetY());
  GenLoc gloc1(0, loc1);
  GenLoc gloc2(0, loc2);
  int tm = GetTM("Bus");
  UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
  genmo->Add(*unit); 
  delete unit; 

  start_time = et;

}

/*
find the two positions in dbarray units that their spatial location equal to
the two input locations, the start entry is given by the input index

*/
void GenMObject::FindPosInMP(MPoint* mo_bus, Point* start_loc, Point* end_loc,
                   int& pos1, int& pos2, int index)
{
//  cout<<"index "<< index<< endl; 
//  cout<<*start_loc<<" "<<*end_loc<<endl; 

  const double delta_dist = 0.01;
  for(int i = index;i < mo_bus->GetNoComponents();i++){
    UPoint unit;
    mo_bus->Get(i, unit);
    Point p1 = unit.p0;
    Point p2 = unit.p1;

//    cout<<start_loc->Distance(p1)<<" "<<start_loc->Distance(p2)<<endl;
//    cout<<end_loc->Distance(p1)<<" "<<end_loc->Distance(p2)<<endl;

    if(start_loc->Distance(p1) < delta_dist && 
       start_loc->Distance(p2) > delta_dist){
      pos1 = i;
    }
    if(end_loc->Distance(p1) > delta_dist && 
      end_loc->Distance(p2) < delta_dist){
      pos2 = i;
      assert(pos1 >= 0);
      break;
    }
  }
//  cout<<"pos1 "<<pos1<<" pos2 "<<pos2<<endl; 

}

/*
set the value for mpoint and genmo 

*/
void GenMObject::SetMO_GenMO(MPoint* mo_bus, int pos1, int pos2, 
                             Instant& start_time, 
                   MPoint* mo, GenMO* genmo, int mobus_oid)
{

      Instant st_temp = start_time;
      Interval<Instant> up_interval;
      bool first_time = true;
      for(; pos1 <= pos2;pos1++){
          UPoint unit;
          mo_bus->Get(pos1, unit);
          Instant st = unit.timeInterval.start;
          Instant et = unit.timeInterval.end;
          if(first_time){////////////waiting for bus moving 
              if(st > st_temp){
                up_interval.start = st_temp;
                up_interval.lc = true;
                up_interval.end = st;
                up_interval.rc = false; 

                UPoint* temp_up = new UPoint(up_interval, unit.p0, unit.p0);
                mo->Add(*temp_up);
                delete temp_up;
              }
              first_time = false;
              start_time = st;
          }

          up_interval.start = st;
          up_interval.lc = true;
          up_interval.end = et;
          up_interval.rc = false; 

          UPoint* up = new UPoint(up_interval, unit.p0, unit.p1);
          mo->Add(*up);
          delete up;

          start_time = et;
//              cout<<"start time "<<start_time<<endl;
      }

      up_interval.start = st_temp;
      up_interval.lc = true;
      up_interval.end = start_time;
      up_interval.rc = false; 

      ////////////////////////////////////////////////////////////
      ////////////////generic moving object//////////////////////
      ////////////////////////////////////////////////////////////
      Loc loc1(-1.0, -1.0);
      Loc loc2(-1.0, -1.0);
      GenLoc gloc1(mobus_oid, loc1);
      GenLoc gloc2(mobus_oid, loc2);
      int tm = GetTM("Bus");
//            cout<<up_interval<<endl; 
      UGenLoc* unit = new UGenLoc(up_interval, gloc1, gloc2, tm);
      genmo->Add(*unit);
      delete unit;

}

/*
change the last bus stop as the last connection in bus network is walk 
segment. so it can directly walk from the second last bus stop in bus network
to destination

*/
void GenMObject::ChangeEndBusStop(BusNetwork* bn, DualGraph* dg, 
                        Bus_Stop cur_bs1, vector<Point>& ps_list2, 
                        GenLoc& gloc2, Relation* rel, R_Tree<2,TupleId>* rtree)
{

  Point* bs_loc = new Point(true, 0, 0);
  bn->GetBusStopGeoData(&cur_bs1, bs_loc);

  Point res(true, 0, 0);
  int tri_id = 0;
  NearestBusStop2(*bs_loc, rel, rtree, res, tri_id);
  assert(tri_id > 0);
  ps_list2[0] = res;
  ps_list2[1] = *bs_loc;
  Loc loc(gloc2.GetLoc().loc1, gloc2.GetLoc().loc2);
  gloc2.SetValue(tri_id, loc);
  delete bs_loc;
}

/*
get the mapping point on the pavement of the bus stop and the triangle id

*/
void GenMObject::NearestBusStop2(Point loc, Relation* rel, 
                      R_Tree<2,TupleId>* rtree, Point& res, int& oid)
{

  SmiRecordId adr = rtree->RootRecordId();

  vector<int> tid_list;
  DFTraverse1(rtree, adr, rel, loc, tid_list);

//  cout<<p<<" "<<tid_list.size()<<endl;

  assert(tid_list.size() > 0);

  vector<MyPoint_Tid> mp_tid_list;
  for(unsigned int i = 0;i < tid_list.size();i++){
    Tuple* tuple = rel->GetTuple(tid_list[i], false);
    Point* q = (Point*)tuple->GetAttribute(BN::BN_PAVE_LOC2);
    MyPoint_Tid mp_tid(q, q->Distance(loc), tid_list[i]);
    mp_tid_list.push_back(mp_tid);
    tuple->DeleteIfAllowed();
  }
  sort(mp_tid_list.begin(), mp_tid_list.end());

//  for(unsigned int i = 0;i < mp_tid_list.size();i++)
//    mp_tid_list[i].Print();
  Tuple* bs_pave = rel->GetTuple(mp_tid_list[0].tid, false);
  Point* pave_loc = (Point*)bs_pave->GetAttribute(BN::BN_PAVE_LOC2);
  GenLoc* bs_gloc = (GenLoc*)bs_pave->GetAttribute(BN::BN_PAVE_LOC1);
  
  res = *pave_loc;
  oid = bs_gloc->GetOid();
  bs_pave->DeleteIfAllowed();

}

/*
connect the end location to the nearest bus stop
 bus (free space) + walk 

*/

void GenMObject::ConnectEndBusStop(DualGraph* dg, VisualGraph* vg,
                                     Relation* rel, GenLoc genloc1,
                                     vector<Point> ps_list1, int oid,
                                 GenMO* genmo, MPoint* mo, 
                                   Instant& start_time, Line* res_path)
{
  Walk_SP* wsp = new Walk_SP(dg, vg, NULL, NULL);
  wsp->rel3 = rel;

  int no_triangle = dg->GetNodeRel()->GetNoTuples();
  int mini_oid = dg->min_tri_oid_1;

  Point p1 = ps_list1[0];
  Point p2(true, genloc1.GetLoc().loc1, genloc1.GetLoc().loc2);

  int oid1 = oid - mini_oid;
//    cout<<"oid2 "<<oid2<<endl;
  assert(1 <= oid1 && oid1 <= no_triangle);
  
  int oid2 = genloc1.GetOid() - mini_oid;
  assert(1 <= oid2 && oid2 <= no_triangle);

  ///////////////////////////////////////////////////////////////////////
  /////////////// create moving objects ////////////////////////////////
  ///////////////////////////////////////////////////////////////////////

  ShortMovement(genmo, mo, start_time, &ps_list1[1], &ps_list1[0]);
  ///////////////////////////////////////////////////////////////////////
  //////////////////walk segment for debuging///////////////////////////
  ///////////////////////////////////////////////////////////////////////

  Line* path = new Line(0);

  wsp->WalkShortestPath2(oid1, oid2, p1, p2, path);
  
  /////////////////////////for debuging//////////////////////////////
//  line_list2.push_back(*path);

  ////////////////create moving objects///////////////////////////////////
  GenerateWalkMovement(dg, path, p1, genmo, mo, start_time);
  /////////////////////////////////////////////////////////////////////////
  
    /////////////////////////////////////////////////////////////////////////
  ////connection between bus stop and its mapping point on the pavement////
  //////////////////////////////////////////////////////////////////////////
  ///////////////////for debuging/////////////////////////////
   Line* path2 = new Line(0);
   path2->StartBulkLoad();
   HalfSegment hs(true, ps_list1[1], ps_list1[0]);
   hs.attr.edgeno = 0;
   *path2 += hs;
   hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
   *path2 += hs;
   path2->EndBulkLoad();

  Line* path12 = new Line(0);
  path->Union(*path2, *path12);
  Line* temp_l = new Line(0);
  path12->Union(*res_path, *temp_l);
  *res_path = *temp_l;
  delete temp_l;
  delete path12;

  delete path2; 

  //////////////////////////////////////////////////////////////////////////
  delete path;
  delete wsp;
}

/*
this structure is used to find the shortest path between the entrances of two
buildings

*/
struct ID_Length{
  int id1;
  int id2;
  double l;
  ID_Length(){}
  ID_Length(int a, int b, double len):id1(a), id2(b), l(len){}
  ID_Length(const ID_Length& id_l):id1(id_l.id1), id2(id_l.id2),l(id_l.l){}
  ID_Length& operator=(const ID_Length& id_l)
  {
    id1 = id_l.id1;
    id2 = id_l.id2;
    l = id_l.l;
    return *this;
  }
  bool operator<(const ID_Length& id_l) const
  {
    return l < id_l.l;
  }
  void Print()
  {
    cout<<"id1 "<<id1<<" id2 "<<id2<<" len "<<l<<endl;
  }

};

/*
create generic moving objects with indoor + walk

*/

void GenMObject::GenerateGenMO4(Space* sp,
                                 Periods* peri, int mo_no, int type, 
                                 Relation* tri_rel)
{

  if(mo_no < 1){
    cout<<" invalid number of moving objects "<<mo_no<<endl;
    return;
  }
  if(!(0 <= type && type < int(ARR_SIZE(genmo_tmlist)))){
    cout<<" invalid type value "<<type<<endl;
  }

  IndoorInfra* i_infra = sp->LoadIndoorInfra(IF_GROOM);
  if(i_infra == NULL){
    cout<<"indoor infrastructure does not exist "<<endl;
    return;
  }

  ////////////////////////////////////////////////////////////////
  //////////////////////Initialization/////////////////////////////
  ////////////////////////////////////////////////////////////////
  Pavement* pm = sp->LoadPavement(IF_REGION);
  DualGraph* dg = pm->GetDualGraph();
  int no_triangle = dg->GetNodeRel()->GetNoTuples();
  int mini_oid = dg->min_tri_oid_1;
  VisualGraph* vg = pm->GetVisualGraph();
  Walk_SP* wsp = new Walk_SP(dg, vg, NULL, NULL);
  wsp->rel3 = tri_rel;
  
  //////////////////////////////////////////////////////////////////
  ////////////////load all buildings and indoor graphs////////////////////////
  MaxRect* maxrect = new MaxRect();
  maxrect->OpenBuilding();
  maxrect->OpenIndoorGraph();
  
  ////////////////////////////////////////////////////////////////
  ////////////select a pair of buildings/////////////////////////
  //////////////////////////////////////////////////////////////
  vector<RefBuild> build_id1_list;
  vector<RefBuild> build_id2_list;
  CreateBuildingPair(i_infra, build_id1_list, build_id2_list, 
                     mo_no, maxrect);
  ///////////////////////////////////////////////////////////////////
//  Relation* build_type_rel = i_infra->BuildingType_Rel();
  Relation* build_path_rel = i_infra->BuildingPath_Rel();
  //////////////////////////////////////////////////////////////////
  ///////////////start time///////////////////////////////////////
   Interval<Instant> periods;
   peri->Get(0, periods);
   Instant start_time = periods.start;
   int time_range = 12*60;//12 hours in range 
  ////////////////////////////////////////////////////////////
  
  int count = 0;
  while(count < mo_no){

   //////////////////////////////start time///////////////////////////
   if(count % 3 == 0)
     start_time.ReadFrom(periods.start.ToDouble() + 
                        (GetRandom() % time_range)/(24.0*60.0));
   else
     start_time.ReadFrom(periods.end.ToDouble() -
                        (GetRandom() % time_range)/(24.0*60.0));

    /////////////////////load all paths from this building////////////////
    vector<int> path_id_list1;
    i_infra->GetPathIDFromTypeID(build_id1_list[count].reg_id, path_id_list1);
//    cout<<"number of paths "<<path_id_list1.size()<<endl;

    vector<int> path_id_list2;
    i_infra->GetPathIDFromTypeID(build_id2_list[count].reg_id, path_id_list2);
//    cout<<"number of paths "<<path_id_list2.size()<<endl;
   ////////////////////////////////////////////////////////////////////
   //////////////if a building has several entrances///////////////////
   ///////////it selects the shortest outdoor pair///////////////////////
   ////////////////////////////////////////////////////////////////////
   if(path_id_list1.size() == 0 || path_id_list2.size() == 0) continue;
   //////////////////////////////////////////////////////////////////////
   cout<<"building 1 "<<GetBuildingStr(build_id1_list[count].type)
        <<" building 2 "<<GetBuildingStr(build_id2_list[count].type)<<endl;


   ////////////////////////////////////////////////////////////////////////
   MPoint* mo = new MPoint(0);
   GenMO* genmo = new GenMO(0);
   mo->StartBulkLoad();
   genmo->StartBulkLoad();

   /////////////////////////////////////////////////////////////////////
   //////calculate all possible outdoor paths between two buildings/////
   ////////////////////////////////////////////////////////////////////

    vector<ID_Length> id_len_list;
    for(unsigned int i = 0;i < path_id_list1.size();i++){
      int path_tid1 = path_id_list1[i];
      Tuple* path_tuple1 = build_path_rel->GetTuple(path_tid1, false);
      GenLoc* gloc1 = 
        (GenLoc*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_EP2_GLOC);
      Point* ep1_2 = (Point*)path_tuple1->GetAttribute(IndoorInfra::
                                                     INDOORIF_EP2);
      int oid1 = gloc1->GetOid() - mini_oid;
      assert(1 <= oid1 && oid1 <= no_triangle);
      for(unsigned int j = 0;j < path_id_list2.size();j++){
          int path_tid2 = path_id_list2[j];
          Tuple* path_tuple2 = build_path_rel->GetTuple(path_tid2, false);
          GenLoc* gloc2 = 
        (GenLoc*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_EP2_GLOC);
         Point* ep2_2 = (Point*)path_tuple2->GetAttribute(IndoorInfra::
                                                          INDOORIF_EP2);

         int oid2 = gloc2->GetOid() - mini_oid;

         assert(1 <= oid2 && oid2 <= no_triangle);

         Line* path = new Line(0);

         wsp->WalkShortestPath2(oid1, oid2, *ep1_2, *ep2_2, path);

         ID_Length id_len(path_tid1, path_tid2, path->Length());
         id_len_list.push_back(id_len);
         delete path;
      }
    }
    sort(id_len_list.begin(), id_len_list.end());

/*    for(unsigned int i = 0;i < id_len_list.size();i++)
        id_len_list[i].Print();*/

    //////////////////////////////////////////////////////////////////////////
     ////////////////select the shortest path between two entrances//////////
     /////////////////////////////////////////////////////////////////////////

     int path_tid1 = id_len_list[0].id1;
     int path_tid2 = id_len_list[0].id2;

     Tuple* path_tuple1 = build_path_rel->GetTuple(path_tid1, false);
     Tuple* path_tuple2 = build_path_rel->GetTuple(path_tid2, false);
    GenLoc* gloc1 = 
        (GenLoc*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_EP2_GLOC);

    Point* ep1_1 = (Point*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_EP);
    Point* ep1_2 = (Point*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_EP2);


    GenLoc* gloc2 = 
        (GenLoc*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_EP2_GLOC);
    Point* ep2_1 = (Point*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_EP);
    Point* ep2_2 = (Point*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_EP2);
    
    int oid1 = gloc1->GetOid() - mini_oid;

    assert(1 <= oid1 && oid1 <= no_triangle);

    int oid2 = gloc2->GetOid() - mini_oid;

    assert(1 <= oid2 && oid2 <= no_triangle);
    /////////////////////////////////////////////////////////////////////////
    ///////////////////movement inside the first building////////////////////
    ////////////////////////////////////////////////////////////////////////
    ////////////////////to show which entrance it is////////////////////////
    int entrance_index1 = ((CcInt*)path_tuple1->GetAttribute(IndoorInfra::
                           INDOORIF_SP_INDEX))->GetIntval();
    int reg_id1 =  ((CcInt*)path_tuple1->GetAttribute(IndoorInfra::
                                               INDOORIF_REG_ID))->GetIntval();
     Point* sp1 = (Point*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_SP);
     GenerateIndoorMovementToExit(i_infra, genmo, mo, start_time, *sp1,
                                  entrance_index1, reg_id1, maxrect, peri);


    ///////////////////////////////////////////////////////////////////////
    /////////////path1: from entranct to pavement//////////////////////////
    //////////////////////////////////////////////////////////////////////
    Line* path1 = (Line*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_PATH);
    
//    line_list1.push_back(*path1);

    GenerateFreeMovement(path1, *sp1, genmo, mo, start_time, "Walk");
    GenerateFreeMovement2(*ep1_1, *ep1_2, genmo, mo, start_time, "Walk");

    //////////////////////////////////////////////////////////////////////////
    //////////////movement in pavement area///////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    Line* path = new Line(0);

    wsp->WalkShortestPath2(oid1, oid2, *ep1_2, *ep2_2, path);


//     cout<<" length "<<path->Length()
//         <<" start_time "<<start_time<<endl;

    GenerateWalkMovement(dg, path, *ep1_2, genmo, mo, start_time);

//    path_list.push_back(*path);

    delete path; 


    //////////////////path2: from pavement to building/////////////////////////
    Line* path2 = (Line*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_PATH);
 //   line_list2.push_back(*path2);
    GenerateFreeMovement2(*ep2_2, *ep2_1, genmo, mo, start_time, "Walk");
    GenerateFreeMovement(path2, *ep2_1, genmo, mo, start_time, "Walk");

    //////////////////////////////////////////////////////////////////////
    /////////////////////////movement inside the second building//////////
    //////////////////////////////////////////////////////////////////////
    ////////////////////to show which entrance it is////////////////////
   Point* sp2 = (Point*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_SP);
   int entrance_index2 = ((CcInt*)path_tuple2->GetAttribute(IndoorInfra::
                                   INDOORIF_SP_INDEX))->GetIntval();
   int reg_id2 =  ((CcInt*)path_tuple2->GetAttribute(IndoorInfra::
                                              INDOORIF_REG_ID))->GetIntval();
    GenerateIndoorMovementFromExit(i_infra, genmo,mo,start_time, *sp2, 
                                   entrance_index2, reg_id2, maxrect, peri);

//    cout<<"reg_id1 "<<reg_id1<<" reg_id2 "<<reg_id2<<endl;

    path_tuple1->DeleteIfAllowed();
    path_tuple2->DeleteIfAllowed();
    ////////////////////////////////////////////////////////////////////////

    mo->EndBulkLoad();
    genmo->EndBulkLoad();

    trip1_list.push_back(*genmo);
    trip2_list.push_back(*mo);

    build_type_list1.push_back(build_id1_list[count].type); 
    build_type_list2.push_back(build_id2_list[count].type); 

    delete mo; 
    delete genmo; 
    ///////////////////////////////////////////////////////////////////////
    count++;
  }

  maxrect->CloseIndoorGraph();
  maxrect->CloseBuilding();
  delete maxrect;

  delete wsp;

  pm->CloseDualGraph(dg);
  pm->CloseVisualGraph(vg);
  sp->ClosePavement(pm);
  sp->CloseIndoorInfra(i_infra);

}

/*
select a pair of buildings. 
the buildings should not be too far away from each other

*/
void GenMObject::CreateBuildingPair(IndoorInfra* i_infra, 
                                    vector<RefBuild>& build_tid1_list, 
                                    vector<RefBuild>& build_tid2_list, int no,
                                    MaxRect* maxrect)
{
  const double max_dist = 500.0;//Euclidean distance
  Relation* build_type_rel = i_infra->BuildingType_Rel();
  int count = 0;
  while(count < no){
    int id1 = GetRandom() % build_type_rel->GetNoTuples() + 1; 
    int id2 = GetRandom() % build_type_rel->GetNoTuples() + 1; 
    if(id1 == id2) continue;
    
    Tuple* tuple1 = build_type_rel->GetTuple(id1, false);
    Tuple* tuple2 = build_type_rel->GetTuple(id2, false);

    Rectangle<2>* bbox1 = 
        (Rectangle<2>*)tuple1->GetAttribute(IndoorInfra::INDOORIF_GEODATA);
    Rectangle<2>* bbox2 = 
        (Rectangle<2>*)tuple2->GetAttribute(IndoorInfra::INDOORIF_GEODATA);
    if(bbox1->Distance(*bbox2) > max_dist){
      tuple1->DeleteIfAllowed();
      tuple2->DeleteIfAllowed();
      continue;
    }

    int type1 = ((CcInt*)tuple1->GetAttribute(IndoorInfra::
                INDOORIF_BUILD_TYPE))->GetIntval();

    int type2 = ((CcInt*)tuple2->GetAttribute(IndoorInfra::
                INDOORIF_BUILD_TYPE))->GetIntval();

                
//    cout<<GetBuildingStr(type1)<<" "<<GetBuildingStr(type2)<<endl;
    //////////////////check whether the building is available/////////////////
    ///////////////////there is no building and indoor graph ///////////////
    ///////////////////for personal houses/////////////////////////////////
    if(type1 > 1 && maxrect->build_pointer[type1] == NULL){
      tuple1->DeleteIfAllowed();
      tuple2->DeleteIfAllowed();
      continue;
    }
    if(type2 > 1 && maxrect->build_pointer[type2] == NULL){
      tuple1->DeleteIfAllowed();
      tuple2->DeleteIfAllowed(); 
      continue;
    }


    int reg_id1 = ((CcInt*)tuple1->GetAttribute(IndoorInfra::
                INDOORIF_REG_ID))->GetIntval();

    int reg_id2 = ((CcInt*)tuple2->GetAttribute(IndoorInfra::
                INDOORIF_REG_ID))->GetIntval();

    int build_id1 = ((CcInt*)tuple1->GetAttribute(IndoorInfra::
                INDOORIF_BUILD_ID))->GetIntval();

    int build_id2 = ((CcInt*)tuple2->GetAttribute(IndoorInfra::
                INDOORIF_BUILD_ID))->GetIntval();

    RefBuild ref_b1(true, reg_id1, build_id1, type1, *bbox1, id1 );
    RefBuild ref_b2(true, reg_id2, build_id2, type2, *bbox2, id2 );
    ////////////////////////////////////////////////////////////////////////
    build_tid1_list.push_back(ref_b1);
    build_tid2_list.push_back(ref_b2);

    count++;
//    cout<<"count "<<count<<endl;

//    rect_list1.push_back(*bbox1);
//    rect_list2.push_back(*bbox2);

    tuple1->DeleteIfAllowed();
    tuple2->DeleteIfAllowed();

  }
}

/*
generate an indoor movement from somewhere to the entrance
entrance index: which entrance it is
reg id: get the building id and type

*/
void GenMObject::GenerateIndoorMovementToExit(IndoorInfra* i_infra,
                                              GenMO* genmo, MPoint* mo, 
                                              Instant& start_time, Point loc,
                                              int entrance_index, 
                                              int reg_id, 
                                              MaxRect* maxrect, Periods* peri)
{
  ///////////////////////////////////////////////////////////////////////////
  //////////load the building: with reg id , we know the building type///////
  ///////////////////////////////////////////////////////////////////////////
  int build_type;
  int build_id;
  i_infra->GetTypeFromRegId(reg_id, build_type, build_id);
//  cout<<"to exit buiding: "<<GetBuildingStr(build_type)<<endl;
  if(build_type == BUILD_HOUSE){///not necessary to load the indoor graph
    MPoint3D* mp3d = new MPoint3D(0); 
    mp3d->StartBulkLoad();
    indoor_mo_list1.push_back(*mp3d);
    mp3d->EndBulkLoad();
    delete mp3d;
  }else{
//    cout<<"load indoor graph (to exit)"<<endl;
    assert(maxrect->igraph_pointer[build_type] != NULL);
    assert(maxrect->build_pointer[build_type] != NULL);
    Building* build = maxrect->build_pointer[build_type];
    Relation* room_rel = build->GetRoom_Rel();
    IndoorNav* indoor_nav = new IndoorNav(room_rel, NULL);
    unsigned int num_elev = indoor_nav->NumerOfElevators();

    IndoorGraph* ig = maxrect->igraph_pointer[build_type];
    BTree* btree_room = build->GetBTree();
    R_Tree<3, TupleId>* rtree_room = build->GetRTree();


//    cout<<"number of elevators "<<num_elev<<endl;

    MPoint3D* mp3d = new MPoint3D(0); 
    if(num_elev <= 1){
      Instant t1 = start_time;
//    cout<<"t1 "<<t1<<endl;
      indoor_nav->GenerateMO3_End(ig, btree_room, rtree_room, start_time,
                                build_id, entrance_index,mp3d,genmo,peri);
      Instant t2 = start_time;
//    cout<<"t2 "<<t2<<endl;
      ////////////////////////////////////////////////////////////////////
      ///////////////set up outdoor movement/////////////////////////////
      //////////////////////////////////////////////////////////////////
      Interval<Instant> up_interval; 
      up_interval.start = t1;
      up_interval.lc = true;
      up_interval.end = t2;
      up_interval.rc = false; 
      UPoint* up = new UPoint(up_interval, loc, loc);
      mo->Add(*up);
      delete up; 

    }else{
//      cout<<"several elevators "<<endl;
      Instant t1 = start_time;
//      cout<<"t1 "<<t1<<endl;
      indoor_nav->GenerateMO3_New_End(ig, btree_room, rtree_room, start_time,
                            build_id, entrance_index,mp3d,genmo,peri,num_elev);

      Instant t2 = start_time;
//      cout<<"t2 "<<t2<<endl;
      ////////////////////////////////////////////////////////////////////
      ///////////////set up outdoor movement/////////////////////////////
      //////////////////////////////////////////////////////////////////
      Interval<Instant> up_interval; 
      up_interval.start = t1;
      up_interval.lc = true;
      up_interval.end = t2;
      up_interval.rc = false; 
      UPoint* up = new UPoint(up_interval, loc, loc);
      mo->Add(*up);
      delete up; 
    }
    indoor_mo_list1.push_back(*mp3d);
    delete mp3d;

    delete indoor_nav;
  }
}

/*
create the indoor movement from a location to the building entrance 
entrance index: which entrance it is
reg id: get the building id and type

*/

void GenMObject::GenerateIndoorMovementFromExit(IndoorInfra* i_infra,
                                              GenMO* genmo, MPoint* mo, 
                                              Instant& start_time, Point loc,
                                              int entrance_index, 
                                              int reg_id,  
                                              MaxRect* maxrect, Periods* peri)
{
  ///////////////////////////////////////////////////////////////////////////
  //////////load the building: with reg id , we know the building type///////
  ///////////////////////////////////////////////////////////////////////////
  int build_type;
  int build_id;
  i_infra->GetTypeFromRegId(reg_id, build_type, build_id);
//  cout<<"from exit buiding: "<<GetBuildingStr(build_type)<<endl;
  if(build_type == BUILD_HOUSE){///not necessary to load the indoor graph
    MPoint3D* mp3d = new MPoint3D(0); 
    indoor_mo_list2.push_back(*mp3d);
    delete mp3d;
  }else{
//    cout<<"load indoor graph (from exit)"<<endl;
    assert(maxrect->igraph_pointer[build_type] != NULL);

    assert(maxrect->build_pointer[build_type] != NULL);
    Building* build = maxrect->build_pointer[build_type];
    Relation* room_rel = build->GetRoom_Rel();
    IndoorNav* indoor_nav = new IndoorNav(room_rel, NULL);
    unsigned int num_elev = indoor_nav->NumerOfElevators();

    IndoorGraph* ig = maxrect->igraph_pointer[build_type];
    BTree* btree_room = build->GetBTree();
    R_Tree<3, TupleId>* rtree_room = build->GetRTree();


//    cout<<"number of elevators "<<num_elev<<endl;

    MPoint3D* mp3d = new MPoint3D(0); 
    if(num_elev <= 1){
      Instant t1 = start_time;

      indoor_nav->GenerateMO3_Start(ig, btree_room, rtree_room, start_time,
                                build_id, entrance_index,mp3d,genmo,peri);
      Instant t2 = start_time;

      ////////////////////////////////////////////////////////////////////
      ///////////////set up outdoor movement/////////////////////////////
      //////////////////////////////////////////////////////////////////
      Interval<Instant> up_interval; 
      up_interval.start = t1;
      up_interval.lc = true;
      up_interval.end = t2;
      up_interval.rc = false; 
      UPoint* up = new UPoint(up_interval, loc, loc);
      mo->Add(*up);
      delete up; 

    }else{
//      cout<<"several elevators "<<endl;
      Instant t1 = start_time;

      indoor_nav->GenerateMO3_New_Start(ig, btree_room, rtree_room, start_time,
                          build_id, entrance_index,mp3d,genmo,peri,num_elev);

      Instant t2 = start_time;

      ////////////////////////////////////////////////////////////////////
      ///////////////set up outdoor movement/////////////////////////////
      //////////////////////////////////////////////////////////////////
      Interval<Instant> up_interval; 
      up_interval.start = t1;
      up_interval.lc = true;
      up_interval.end = t2;
      up_interval.rc = false; 
      UPoint* up = new UPoint(up_interval, loc, loc);
      mo->Add(*up);
      delete up;

    }

    indoor_mo_list2.push_back(*mp3d);
    delete mp3d;

    delete indoor_nav;
  }

}

/*
create generic moving objects with modes: walk + indoor + car (taxi)

*/
void GenMObject::GenerateGenMO5(Space* sp, Periods* peri,
                      int mo_no, int type, Relation* rel1, BTree* btree, 
                      Relation* rel2)
{
  if(mo_no < 1){
    cout<<" invalid number of moving objects "<<mo_no<<endl;
    return;
  }
  if(!(0 <= type && type < int(ARR_SIZE(genmo_tmlist)))){
    cout<<" invalid type value "<<type<<endl;
    return; 
  }

  IndoorInfra* i_infra = sp->LoadIndoorInfra(IF_GROOM);
  if(i_infra == NULL){
    cout<<"indoor infrastructure does not exist "<<endl;
    return;
  }

  switch(type){
    case 13:
      GenerateGenMO_IndoorWalkCarTaxi(sp, i_infra, peri, mo_no, 
                                      rel1, btree, rel2, "Car");
      break;
    case 16:
      GenerateGenMO_IndoorWalkCarTaxi(sp,i_infra, peri, mo_no,
                                      rel1, btree, rel2, "Taxi");
      break;

    default:
      assert(false);
      break;  
  }

  sp->CloseIndoorInfra(i_infra);

}

/*
indoor + walk + car or taxi

*/
void GenMObject::GenerateGenMO_IndoorWalkCarTaxi(Space* sp, 
                                                 IndoorInfra* i_infra,
                                       Periods* peri, int mo_no, 
                                       Relation* dg_node_rel, BTree* btree, 
                                       Relation* speed_rel, string mode)
{
  ////////////////////////////////////////////////////////////////
  //////////////////////Initialization/////////////////////////////
  ////////////////////////////////////////////////////////////////
  Pavement* pm = sp->LoadPavement(IF_REGION);
  Network* rn = sp->LoadRoadNetwork(IF_LINE);
  //////////////////////////////////////////////////////////////////
  ////////////////load all buildings and indoor graphs////////////////////////
  MaxRect* maxrect = new MaxRect();
  maxrect->OpenBuilding();
  maxrect->OpenIndoorGraph();
  
  ////////////////////////////////////////////////////////////////
  ////////////select a pair of buildings/////////////////////////
  //////////////////////////////////////////////////////////////
  vector<RefBuild> build_id1_list;
  vector<RefBuild> build_id2_list;
  CreateBuildingPair2(i_infra, build_id1_list, build_id2_list, 
                     mo_no, maxrect);

  ///////////////////////////////////////////////////////////////////
  Relation* build_path_rel = i_infra->BuildingPath_Rel();
  //////////////////////////////////////////////////////////////////
  ///////////////start time///////////////////////////////////////
   Interval<Instant> periods;
   peri->Get(0, periods);
   Instant start_time = periods.start;
   int time_range = 12*60;//12 hours in range 
  ////////////////////////////////////////////////////////////
   int count = 0;
   while(count < mo_no){

   //////////////////////////////start time///////////////////////////
   if(count % 3 == 0)
     start_time.ReadFrom(periods.start.ToDouble() + 
                        (GetRandom() % time_range)/(24.0*60.0));
   else
     start_time.ReadFrom(periods.end.ToDouble() -
                        (GetRandom() % time_range)/(24.0*60.0));

    /////////////////////load all paths from this building////////////////
    vector<int> path_id_list1;
    i_infra->GetPathIDFromTypeID(build_id1_list[count].reg_id, path_id_list1);
//    cout<<"number of paths "<<path_id_list1.size()<<endl;

    vector<int> path_id_list2;
    i_infra->GetPathIDFromTypeID(build_id2_list[count].reg_id, path_id_list2);
//    cout<<"number of paths "<<path_id_list2.size()<<endl;
   ////////////////////////////////////////////////////////////////////
   //////////////if a building has several entrances///////////////////
   ///////////it randomly selects an entrance  ///////////////////////
   ////////////the buildings are far away from each///////////////////
   //////////which entrance to go out does not influence the distance a lot///
   ////////////////////////////////////////////////////////////////////
   if(path_id_list1.size() == 0 || path_id_list2.size() == 0) continue;
   //////////////////////////////////////////////////////////////////////
   cout<<"building 1 "<<GetBuildingStr(build_id1_list[count].type)
        <<" building 2 "<<GetBuildingStr(build_id2_list[count].type)<<endl;

   int path_tid1 = path_id_list1[GetRandom() % path_id_list1.size()];
   int path_tid2 = path_id_list2[GetRandom() % path_id_list2.size()];

   Tuple* path_tuple1 = build_path_rel->GetTuple(path_tid1, false);
   Tuple* path_tuple2 = build_path_rel->GetTuple(path_tid2, false);

    /////////////////////////////////////////////////////////////////////
    //////////////////1 get buildings and two positions for network /////
    ////////////////// the end point of path in pavement/////////////////
    ////////////////////////////////////////////////////////////////////

    GenLoc* gloc1 = 
        (GenLoc*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_EP2_GLOC);
    Point* ep1_1 = (Point*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_EP);
    Point* ep1_2 = (Point*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_EP2);


    GenLoc* gloc2 = 
        (GenLoc*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_EP2_GLOC);
    Point* ep2_1 = (Point*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_EP);
    Point* ep2_2 = (Point*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_EP2);


    /////////////reset the general location//////////////////////
    Loc loc1(ep1_2->GetX(), ep1_2->GetY());
    GenLoc newgloc1(gloc1->GetOid(), loc1);
    Loc loc2(ep2_2->GetX(), ep2_2->GetY());
    GenLoc newgloc2(gloc2->GetOid(), loc2);
    
    /////////////////////////////////////////////////////////////////////
    //////////////////2 map two positions to gpoints////////////////////
    /////////////////get the start and end location of the trip/////////
    ////////////////////////////////////////////////////////////////////
    vector<GPoint> gpoint_list;
    vector<Point> p_list;
    PaveLoc2GPoint(newgloc1, newgloc2, sp, dg_node_rel,
                   btree, gpoint_list, p_list);
    GPoint gp1 = gpoint_list[0];
    GPoint gp2 = gpoint_list[1];
    Point start_loc = p_list[0];
    Point end_loc = p_list[1];

//    cout<<"ep1 "<<*ep1_2<<" ep2 "<<*ep2_2<<endl;
//    cout<<"gp1 "<<start_loc<<" gp2 "<<end_loc<<endl;
//    loc_list1.push_back(start_loc);
//    loc_list2.push_back(end_loc);

    MPoint* mo = new MPoint(0);
    GenMO* genmo = new GenMO(0);
    mo->StartBulkLoad();
    genmo->StartBulkLoad();
    /////////////////////////////////////////////////////////////////////
    ///////////////3 indoor movement1 + from entrance to pavement//////
    ////////////////////////////////////////////////////////////////////

    //////////////////////indoor movement///////////////////////////////////
    ////////////////////to show which entrance it is////////////////////////
    int entrance_index1 = ((CcInt*)path_tuple1->GetAttribute(IndoorInfra::
                           INDOORIF_SP_INDEX))->GetIntval();
    int reg_id1 =  ((CcInt*)path_tuple1->GetAttribute(IndoorInfra::
                                               INDOORIF_REG_ID))->GetIntval();
     Point* sp1 = (Point*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_SP);
     GenerateIndoorMovementToExit(i_infra, genmo, mo, start_time, *sp1,
                                  entrance_index1, reg_id1, maxrect, peri);


    ///////////////   outdoor  movement //////////////////////////////
    Line* path1 = (Line*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_PATH);
//    line_list1.push_back(*path1);
    GenerateFreeMovement(path1, *sp1, genmo, mo, start_time, "Walk");
    GenerateFreeMovement2(*ep1_1, *ep1_2, genmo, mo, start_time, "Walk");

    /////////////////////////////////////////////////////////////////////
    //////////////////4 pavement to network (start location)/////////////
    ////////////////////////////////////////////////////////////////////
    ConnectStartMove(newgloc1, start_loc, mo, genmo, start_time, pm, mode);

    /////////////////////////////////////////////////////////////////////
    //////////////////5 road network movement //////////////////////////
    ////////////////////////////////////////////////////////////////////
    GLine* gl = new GLine(0);
    gp1.ShortestPath(&gp2, gl);

    BusRoute* br = new BusRoute(rn, NULL, NULL);
    GLine* newgl = new GLine(0);
    br->ConvertGLine(gl, newgl);

//     Line* road_path = new Line(0);
//     newgl->Gline2line(road_path);
//     path_list.push_back(*road_path);
//     cout<<road_path->Length()<<endl;
//     delete road_path;

    ConnectGP1GP2(rn, start_loc, newgl, mo, genmo, start_time, speed_rel,mode);

    delete newgl; 

    delete br;
    delete gl;
    /////////////////////////////////////////////////////////////////////
    //////////////////6 end network location to pavement//////////////////
    ////////////////////////////////////////////////////////////////////
    ConnectEndMove(end_loc, newgloc2, mo, genmo, start_time, pm, mode);

    /////////////////////////////////////////////////////////////////////
    ////////7 pavement to building entrance + indoor movement2//////////
    ////////////////////////////////////////////////////////////////////

    ////////////////outdoor movement/////////////////////////////////////////
    Line* path2 = (Line*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_PATH);
//    line_list2.push_back(*path2);
    GenerateFreeMovement2(*ep2_2, *ep2_1, genmo, mo, start_time, "Walk");
    GenerateFreeMovement(path2, *ep2_1, genmo, mo, start_time, "Walk");

    ///////////////////indoor movement//////////////////////////////////////
    ////////////////////to show which entrance it is////////////////////
    Point* sp2 = (Point*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_SP);
    int entrance_index2 = ((CcInt*)path_tuple2->GetAttribute(IndoorInfra::
                                   INDOORIF_SP_INDEX))->GetIntval();
    int reg_id2 =  ((CcInt*)path_tuple2->GetAttribute(IndoorInfra::
                                              INDOORIF_REG_ID))->GetIntval();
    GenerateIndoorMovementFromExit(i_infra, genmo,mo,start_time, *sp2, 
                                   entrance_index2, reg_id2, maxrect, peri);

   /////////////////////////////////////////////////////////////////////////
   
    mo->EndBulkLoad();
    genmo->EndBulkLoad();

    trip1_list.push_back(*genmo);
    trip2_list.push_back(*mo);
    
    ///////////////////////store building type//////////////////////////////
    build_type_list1.push_back(build_id1_list[count].type); 
    build_type_list2.push_back(build_id2_list[count].type); 

    delete mo; 
    delete genmo; 


    path_tuple1->DeleteIfAllowed();
    path_tuple2->DeleteIfAllowed(); 

    count++;

  }
  maxrect->CloseIndoorGraph();
  maxrect->CloseBuilding();
  delete maxrect;


  ////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  sp->CloseRoadNetwork(rn);
  sp->ClosePavement(pm);

}

/*
select a pair of buildings. 
the buildings should be far away from each other

*/
void GenMObject::CreateBuildingPair2(IndoorInfra* i_infra, 
                                    vector<RefBuild>& build_tid1_list, 
                                    vector<RefBuild>& build_tid2_list, int no,
                                    MaxRect* maxrect)
{
  const double min_dist = 1000.0;//Euclidean distance
  Relation* build_type_rel = i_infra->BuildingType_Rel();
  int count = 0;
  while(count < no){
    int id1 = GetRandom() % build_type_rel->GetNoTuples() + 1; 
    int id2 = GetRandom() % build_type_rel->GetNoTuples() + 1; 
    if(id1 == id2) continue;
    
    Tuple* tuple1 = build_type_rel->GetTuple(id1, false);
    Tuple* tuple2 = build_type_rel->GetTuple(id2, false);

    Rectangle<2>* bbox1 = 
        (Rectangle<2>*)tuple1->GetAttribute(IndoorInfra::INDOORIF_GEODATA);
    Rectangle<2>* bbox2 = 
        (Rectangle<2>*)tuple2->GetAttribute(IndoorInfra::INDOORIF_GEODATA);
    if(bbox1->Distance(*bbox2) < min_dist){
      tuple1->DeleteIfAllowed();
      tuple2->DeleteIfAllowed();
      continue;
    }

    int type1 = ((CcInt*)tuple1->GetAttribute(IndoorInfra::
                INDOORIF_BUILD_TYPE))->GetIntval();

    int type2 = ((CcInt*)tuple2->GetAttribute(IndoorInfra::
                INDOORIF_BUILD_TYPE))->GetIntval();


    ////////////do not select two personal houses////////////
    if(type1 == 1 && type2 == 1){
      tuple1->DeleteIfAllowed();
      tuple2->DeleteIfAllowed();
      continue;
    }

//    cout<<GetBuildingStr(type1)<<" "<<GetBuildingStr(type2)<<endl;
    //////////////////check whether the building is available/////////////////
    ///////////////////there is no building and indoor graph ///////////////
    ///////////////////for personal houses/////////////////////////////////
    if(type1 > 1 && maxrect->build_pointer[type1] == NULL){
      tuple1->DeleteIfAllowed();
      tuple2->DeleteIfAllowed();
      continue;
    }
    if(type2 > 1 && maxrect->build_pointer[type2] == NULL){
      tuple1->DeleteIfAllowed();
      tuple2->DeleteIfAllowed(); 
      continue;
    }


    int reg_id1 = ((CcInt*)tuple1->GetAttribute(IndoorInfra::
                INDOORIF_REG_ID))->GetIntval();

    int reg_id2 = ((CcInt*)tuple2->GetAttribute(IndoorInfra::
                INDOORIF_REG_ID))->GetIntval();

    int build_id1 = ((CcInt*)tuple1->GetAttribute(IndoorInfra::
                INDOORIF_BUILD_ID))->GetIntval();

    int build_id2 = ((CcInt*)tuple2->GetAttribute(IndoorInfra::
                INDOORIF_BUILD_ID))->GetIntval();

    RefBuild ref_b1(true, reg_id1, build_id1, type1, *bbox1, id1 );
    RefBuild ref_b2(true, reg_id2, build_id2, type2, *bbox2, id2 );
    ////////////////////////////////////////////////////////////////////////
    build_tid1_list.push_back(ref_b1);
    build_tid2_list.push_back(ref_b2);

    count++;

//    rect_list1.push_back(*bbox1);
//    rect_list2.push_back(*bbox2);

    tuple1->DeleteIfAllowed();
    tuple2->DeleteIfAllowed();

  }
}

/*
generic moving objects with modes: indoor walk bus

*/
void GenMObject::GenerateGenMO6(Space* sp, Periods* peri,
                      int mo_no, int type, Relation* rel1, Relation* rel2,
                      R_Tree<2,TupleId>* rtree)
{

  if(mo_no < 1){
    cout<<" invalid number of moving objects "<<mo_no<<endl;
    return;
  }
  if(!(0 <= type && type < int(ARR_SIZE(genmo_tmlist)))){
    cout<<" invalid type value "<<type<<endl;
    return; 
  }
  
  IndoorInfra* i_infra = sp->LoadIndoorInfra(IF_GROOM);
  if(i_infra == NULL){
    cout<<"indoor infrastructure does not exist "<<endl;
    return;
  }


  ////////////////////////////////////////////////////////////////
  //////////////////////Initialization/////////////////////////////
  ////////////////////////////////////////////////////////////////
  Pavement* pm = sp->LoadPavement(IF_REGION);
  DualGraph* dg = pm->GetDualGraph();
  VisualGraph* vg = pm->GetVisualGraph();
  BusNetwork* bn = sp->LoadBusNetwork(IF_BUSNETWORK);
  //////////////////////////////////////////////////////////////////
  ////////////////load all buildings and indoor graphs////////////////////////
  MaxRect* maxrect = new MaxRect();
  maxrect->OpenBuilding();
  maxrect->OpenIndoorGraph();
  
  ////////////////////////////////////////////////////////////////
  ////////////select a pair of buildings/////////////////////////
  //////////////////////////////////////////////////////////////
  vector<RefBuild> build_id1_list;
  vector<RefBuild> build_id2_list;
  CreateBuildingPair2(i_infra, build_id1_list, build_id2_list, 
                     mo_no, maxrect);

  ///////////////////////////////////////////////////////////////////
  Relation* build_path_rel = i_infra->BuildingPath_Rel();
  //////////////////////////////////////////////////////////////////
  ///////////////start time///////////////////////////////////////
   Interval<Instant> periods;
   peri->Get(0, periods);
   Instant start_time = periods.start;
   int time_range = 12*60;//12 hours in range 
  ////////////////////////////////////////////////////////////
   int count = 0;
   int real_count = 1;
   while(count < mo_no){

   //////////////////////////////start time///////////////////////////
   if(count % 3 == 0)
     start_time.ReadFrom(periods.start.ToDouble() + 
                        (GetRandom() % time_range)/(24.0*60.0));
   else
     start_time.ReadFrom(periods.end.ToDouble() -
                        (GetRandom() % time_range)/(24.0*60.0));

    /////////////////////load all paths from this building////////////////
    vector<int> path_id_list1;
    i_infra->GetPathIDFromTypeID(build_id1_list[count].reg_id, path_id_list1);
//    cout<<"number of paths "<<path_id_list1.size()<<endl;

    vector<int> path_id_list2;
    i_infra->GetPathIDFromTypeID(build_id2_list[count].reg_id, path_id_list2);
//    cout<<"number of paths "<<path_id_list2.size()<<endl;
   ////////////////////////////////////////////////////////////////////
   //////////////if a building has several entrances///////////////////
   ///////////it randomly selects an entrance  ///////////////////////
   ////////////the buildings are far away from each///////////////////
   //////////which entrance to go out does not influence the distance a lot///
   ////////////////////////////////////////////////////////////////////
   if(path_id_list1.size() == 0 || path_id_list2.size() == 0) continue;
   //////////////////////////////////////////////////////////////////////
   cout<<"building 1 "<<GetBuildingStr(build_id1_list[count].type)
        <<" building 2 "<<GetBuildingStr(build_id2_list[count].type)<<endl;

    int path_tid1 = path_id_list1[GetRandom() % path_id_list1.size()];
    int path_tid2 = path_id_list2[GetRandom() % path_id_list2.size()];

    Tuple* path_tuple1 = build_path_rel->GetTuple(path_tid1, false);
    Tuple* path_tuple2 = build_path_rel->GetTuple(path_tid2, false);
    /////////////////////////////////////////////////////////////////////////
    ////////////////get start and end locaton of the path in pavement////////
    /////////////////////////////////////////////////////////////////////////

    GenLoc* build_gloc1 = 
        (GenLoc*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_EP2_GLOC);
    Point* ep1_1 = (Point*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_EP);
    Point* ep1_2 = (Point*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_EP2);


    GenLoc* build_gloc2 = 
        (GenLoc*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_EP2_GLOC);
    Point* ep2_1 = (Point*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_EP);
    Point* ep2_2 = (Point*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_EP2);

//    cout<<"ep1 "<<*ep1_1<<" ep2 "<<*ep2_2<<endl; 
    
    /////////////////////////////////////////////////////////////
    /////////////reset the general location//////////////////////
    Loc loc1(ep1_2->GetX(), ep1_2->GetY());
    GenLoc newgloc1(build_gloc1->GetOid(), loc1);
    Loc loc2(ep2_2->GetX(), ep2_2->GetY());
    GenLoc newgloc2(build_gloc2->GetOid(), loc2);

    //////////////////////////////////////////////////////////////////////////
    //////find closest bus stops to the two locations on the pavement/////////
    Bus_Stop bs1(true, 0, 0, true);
    vector<Point> ps_list1;//1 point on the pavement 2 point of bus stop
    GenLoc gloc1(0, Loc(0, 0));//bus stop on pavement by genloc 
    bool b1 = true;
    b1 = NearestBusStop(newgloc1,  rel2, rtree, bs1, ps_list1, gloc1, true);

    Bus_Stop bs2(true, 0, 0, true);
    vector<Point> ps_list2;//1. point on the pavement 2. point of bus stop
    GenLoc gloc2(0, Loc(0, 0));//bus stop on pavement by genloc
    bool b2 = true;
    b2 = NearestBusStop(newgloc2,  rel2, rtree, bs2, ps_list2, gloc2, false);

    if((b1 && b2) == false){
      path_tuple1->DeleteIfAllowed();
      path_tuple2->DeleteIfAllowed();
      count++;
      continue;
    }

//    cout<<"bs1 "<<bs1<<" bs2 "<<bs2<<endl;

    MPoint* mo = new MPoint(0);
    GenMO* genmo = new GenMO(0);
    mo->StartBulkLoad();
    genmo->StartBulkLoad();

    /////////////////////////////////////////////////////////////////////
    ///////////////1. indoor movement 1 + pavement//////////////////////
    ////////////////////////////////////////////////////////////////////

    //////////////////////indoor movement///////////////////////////////
    ////////////////////to show which entrance it is////////////////////

    int entrance_index1 = ((CcInt*)path_tuple1->GetAttribute(IndoorInfra::
                           INDOORIF_SP_INDEX))->GetIntval();
    int reg_id1 =  ((CcInt*)path_tuple1->GetAttribute(IndoorInfra::
                                               INDOORIF_REG_ID))->GetIntval();
    Point* sp1 = (Point*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_SP);
//     GenerateIndoorMovementToExit(i_infra, genmo, mo, start_time, *sp1,
//                                 entrance_index1, reg_id1, maxrect, peri);

    MPoint3D* mp3d = new MPoint3D(0);
    GenerateIndoorMovementToExit2(i_infra, genmo, mo, start_time, *sp1,
                                 entrance_index1, reg_id1, maxrect, peri, mp3d);


    Line* path1 = (Line*)path_tuple1->GetAttribute(IndoorInfra::INDOORIF_PATH);
    GenerateFreeMovement(path1, *sp1, genmo, mo, start_time, "Walk");
    GenerateFreeMovement2(*ep1_1, *ep1_2, genmo, mo, start_time, "Walk");


    ///////////////////////////////////////////////////////////////////
    ////////////////2. path (walk + bus)///////////////////////////////
    ///////////////////////////////////////////////////////////////////

    ////////2.1 connect from pavement to start bus stop////////////////////
    Line* res_path = new Line(0);
    ConnectStartBusStop(dg, vg, rel1, newgloc1, ps_list1, gloc1.GetOid(),
                            genmo, mo, start_time, res_path);

//    cout<<"path1 length "<<res_path->Length()<<endl; 
    /////////////////////////////////////////////////////////////////
    //////2.2. get the path in bus network////////////////////////////
    /////////////////////////////////////////////////////////////////
    BNNav* bn_nav = new BNNav(bn);
    if(count % 2 == 0) bn_nav->ShortestPath_Time2(&bs1, &bs2, &start_time);
    else bn_nav->ShortestPath_Transfer2(&bs1, &bs2, &start_time);

    if(bn_nav->path_list.size() == 0){
//        cout<<"two unreachable bus stops"<<endl;
        mo->EndBulkLoad();
        genmo->EndBulkLoad();

        delete mo;
        delete genmo;
        delete bn_nav;
        delete res_path;
        delete mp3d;
        path_tuple1->DeleteIfAllowed();
        path_tuple2->DeleteIfAllowed();
        count++;
        continue;
    }

    //////////put the first part of movement inside a building/////////////////
    indoor_mo_list1.push_back(*mp3d);
    delete mp3d; 


//    line_list1.push_back(*path1);

    int last_walk_id = ConnectTwoBusStops(bn_nav, ps_list1[1], ps_list2[1],
                           genmo, mo, start_time, dg, res_path);

//    cout<<"path2 length "<<res_path->Length()<<endl;
    ///////////////////////////////////////////////////////////
    if(last_walk_id > 0){
          //////change the last bus stop/////////
          ///change  ps list2, gloc2.oid ///
          Bus_Stop cur_bs1(true, 0, 0, true);
          StringToBusStop(bn_nav->bs1_list[last_walk_id], 
                          cur_bs1);
            ChangeEndBusStop(bn, dg, cur_bs1, ps_list2, gloc2, rel2, rtree);
    }

    ////////////////////////////////////////////////////////////
    delete bn_nav;
    /////////////////2.3 connect from end bus stop to pavement///////////
    ConnectEndBusStop(dg, vg, rel1, newgloc2, ps_list2, gloc2.GetOid(),
                          genmo, mo, start_time, res_path);
//    cout<<"path3 length "<<res_path->Length()<<endl;

     ////////////////////////////////////////////////////////////////////

     ////////////////////////////////////////////////////////////////////
     /////////////3. pavement + indoor movement 2////////////////////////
     ////////////////////////////////////////////////////////////////////


     ////////////////outdoor movement/////////////////////////////////////////
    Line* path2 = (Line*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_PATH);
//    line_list2.push_back(*path2);
    GenerateFreeMovement2(*ep2_2, *ep2_1, genmo, mo, start_time, "Walk");
    GenerateFreeMovement(path2, *ep2_1, genmo, mo, start_time, "Walk");

    ///////////////////indoor movement//////////////////////////////////////
    ////////////////////to show which entrance it is////////////////////
    Point* sp2 = (Point*)path_tuple2->GetAttribute(IndoorInfra::INDOORIF_SP);
    int entrance_index2 = ((CcInt*)path_tuple2->GetAttribute(IndoorInfra::
                                   INDOORIF_SP_INDEX))->GetIntval();
    int reg_id2 =  ((CcInt*)path_tuple2->GetAttribute(IndoorInfra::
                                              INDOORIF_REG_ID))->GetIntval();
    GenerateIndoorMovementFromExit(i_infra, genmo,mo,start_time, *sp2, 
                                   entrance_index2, reg_id2, maxrect, peri);

    //////////////////////////////////////////////////////////////////////
     mo->EndBulkLoad();
     genmo->EndBulkLoad();

     trip1_list.push_back(*genmo);
     trip2_list.push_back(*mo);

//     path_list.push_back(*res_path);

     delete res_path;
     delete genmo;
     delete mo;

    ///////////////////////////////////////////////////////////////////////
    path_tuple1->DeleteIfAllowed();
    path_tuple2->DeleteIfAllowed(); 

    ///////////////////////store building type//////////////////////////////
    build_type_list1.push_back(build_id1_list[count].type);
    build_type_list2.push_back(build_id2_list[count].type); 


    cout<<real_count<<" moving object "<<endl;
    real_count++;
    count++;

  }

  maxrect->CloseIndoorGraph();
  maxrect->CloseBuilding();
  delete maxrect;


  ////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////
  sp->CloseBusNetwork(bn);
  pm->CloseDualGraph(dg);
  pm->CloseVisualGraph(vg);
  sp->ClosePavement(pm);


//   cout<<trip1_list.size()<<" "<<trip2_list.size()
//       <<" "<<indoor_mo_list1.size()<<" "<<build_type_list1.size()
//       <<" "<<indoor_mo_list2.size()<<" "<<build_type_list2.size()<<endl;

  sp->CloseIndoorInfra(i_infra);

}

/*
almost the same procedure as GenerateIndoorMovementToExit but takes in the 
mpoint3d. because in producing bus route, if there is no such route, the 
indoor movement should not be put into the result

*/
void GenMObject::GenerateIndoorMovementToExit2(IndoorInfra* i_infra,
                                              GenMO* genmo, MPoint* mo, 
                                              Instant& start_time, Point loc,
                                              int entrance_index, 
                                              int reg_id, 
                                              MaxRect* maxrect, 
                                               Periods* peri, MPoint3D* mp3d)
{
  ///////////////////////////////////////////////////////////////////////////
  //////////load the building: with reg id , we know the building type///////
  ///////////////////////////////////////////////////////////////////////////
  int build_type;
  int build_id;
  i_infra->GetTypeFromRegId(reg_id, build_type, build_id);
//  cout<<"to exit buiding: "<<GetBuildingStr(build_type)<<endl;
  if(build_type == BUILD_HOUSE){///not necessary to load the indoor graph

/*    MPoint3D* mp3d = new MPoint3D(0); 
    mp3d->StartBulkLoad();
    indoor_mo_list1.push_back(*mp3d);
    mp3d->EndBulkLoad();
    delete mp3d;*/

  }else{
//    cout<<"load indoor graph (to exit)"<<endl;
    assert(maxrect->igraph_pointer[build_type] != NULL);
    assert(maxrect->build_pointer[build_type] != NULL);
    Building* build = maxrect->build_pointer[build_type];
    Relation* room_rel = build->GetRoom_Rel();
    IndoorNav* indoor_nav = new IndoorNav(room_rel, NULL);
    unsigned int num_elev = indoor_nav->NumerOfElevators();

    IndoorGraph* ig = maxrect->igraph_pointer[build_type];
    BTree* btree_room = build->GetBTree();
    R_Tree<3, TupleId>* rtree_room = build->GetRTree();


//    cout<<"number of elevators "<<num_elev<<endl;


    if(num_elev <= 1){
      Instant t1 = start_time;
//    cout<<"t1 "<<t1<<endl;
      indoor_nav->GenerateMO3_End(ig, btree_room, rtree_room, start_time,
                                build_id, entrance_index,mp3d,genmo,peri);
      Instant t2 = start_time;
//    cout<<"t2 "<<t2<<endl;
      ////////////////////////////////////////////////////////////////////
      ///////////////set up outdoor movement/////////////////////////////
      //////////////////////////////////////////////////////////////////
      Interval<Instant> up_interval; 
      up_interval.start = t1;
      up_interval.lc = true;
      up_interval.end = t2;
      up_interval.rc = false; 
      UPoint* up = new UPoint(up_interval, loc, loc);
      mo->Add(*up);
      delete up; 

    }else{
//      cout<<"several elevators "<<endl;
      Instant t1 = start_time;
//      cout<<"t1 "<<t1<<endl;
      indoor_nav->GenerateMO3_New_End(ig, btree_room, rtree_room, start_time,
                            build_id, entrance_index,mp3d,genmo,peri,num_elev);

      Instant t2 = start_time;
//      cout<<"t2 "<<t2<<endl;
      ////////////////////////////////////////////////////////////////////
      ///////////////set up outdoor movement/////////////////////////////
      //////////////////////////////////////////////////////////////////
      Interval<Instant> up_interval; 
      up_interval.start = t1;
      up_interval.lc = true;
      up_interval.end = t2;
      up_interval.rc = false; 
      UPoint* up = new UPoint(up_interval, loc, loc);
      mo->Add(*up);
      delete up; 
    }

    delete indoor_nav;
  }
}


/////////////////////////////////////////////////////////////////////////
////////////////navigation system///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/*
find a trip with minimum traveling time from one location on the pavement 
to another location on the pavement 

*/
void Navigation::Navigation1(Space* sp, Relation* rel1, Relation* rel2, 
                   Instant* query_time, Relation* rel3, Relation* rel4, 
                   R_Tree<2,TupleId>* rtree)
{
  Pavement* pm = sp->LoadPavement(IF_REGION);
  BusNetwork* bn = sp->LoadBusNetwork(IF_BUSNETWORK);
  DualGraph* dg = pm->GetDualGraph();
  VisualGraph* vg = pm->GetVisualGraph();
  ///////////////////////////////////////////////////////////////////
  ////////////check the two query locations on the pavement/////////
  //////////////////////////////////////////////////////////////////
  if(rel1->GetNoTuples() != 1 || rel2->GetNoTuples() != 1){
    cout<<"input query relation is not correct"<<endl;
    return;
  }
  Tuple* t1 = rel1->GetTuple(1, false);
  int oid1 = ((CcInt*)t1->GetAttribute(VisualGraph::QOID))->GetIntval();
  Point* p1 = (Point*)t1->GetAttribute(VisualGraph::QLOC2);
  Point loc1(*p1);
  t1->DeleteIfAllowed(); 
  
  Tuple* t2 = rel2->GetTuple(1, false);
  int oid2 = ((CcInt*)t2->GetAttribute(VisualGraph::QOID))->GetIntval();
  Point* p2 = (Point*)t2->GetAttribute(VisualGraph::QLOC2);
  Point loc2(*p2);
  t2->DeleteIfAllowed(); 

  oid1 -= dg->min_tri_oid_1;
  oid2 -= dg->min_tri_oid_1; 

  int no_node_graph = dg->No_Of_Node();
  if(oid1 < 1 || oid1 > no_node_graph){
    cout<<"loc1 does not exist"<<endl;
    return;
  }
  if(oid2 < 1 || oid2 > no_node_graph){
    cout<<"loc2 does not exist"<<endl;
    return;
  }
  if(AlmostEqual(loc1,loc2)){
    cout<<"start location equals to end location"<<endl;
    return;
  }
  Tuple* tuple1 = dg->GetNodeRel()->GetTuple(oid1, false);
  Region* reg1 = (Region*)tuple1->GetAttribute(DualGraph::PAVEMENT);
  
  CompTriangle* ct1 = new CompTriangle(reg1);
  assert(ct1->PolygonConvex()); 
  delete ct1; 
  
  if(loc1.Inside(*reg1) == false){
    tuple1->DeleteIfAllowed();
    cout<<"point1 is not inside the polygon"<<endl;
    return;
  }
  Tuple* tuple2 = dg->GetNodeRel()->GetTuple(oid2, false);
  Region* reg2 = (Region*)tuple2->GetAttribute(DualGraph::PAVEMENT);

  CompTriangle* ct2 = new CompTriangle(reg2);
  assert(ct2->PolygonConvex()); 
  delete ct2; 
  
  if(loc2.Inside(*reg2) == false){
    tuple1->DeleteIfAllowed();
    tuple2->DeleteIfAllowed();
    cout<<"point2 is not inside the polygon"<<endl;
    return;
  }
  tuple1->DeleteIfAllowed();
  tuple2->DeleteIfAllowed();
  ///////////////////////////////////////////////////////////////////////
  //////////1. find all possible bus stops to the two locations/////////
  //////////////////////////////////////////////////////////////////////
  vector<Bus_Stop> bs_list1;///bus stops
  vector<Point> ps_list1;//bus stops mapping points on the pavement 
  vector<Point> ps_list2;//bus stops 2d locations
  vector<GenLoc> gloc_list1;//bus stops mapping points by genloc 

  bool b1 = true;
  b1 = NearestBusStop1(loc1, rel4, rtree, bs_list1, 
                       ps_list1, ps_list2, gloc_list1);
//  cout<<"neighbor bus stops size1 "<<bs_list1.size()<<endl; 



  vector<Bus_Stop> bs_list2;///bus stops
  vector<Point> ps_list3;//bus stops mapping points on the pavement 
  vector<Point> ps_list4;//bus stops 2d locations
  vector<GenLoc> gloc_list2;//bus stops mapping points by genloc 

  bool b2 = true;
  b2 = NearestBusStop1(loc2, rel4, rtree, bs_list2, 
                       ps_list3, ps_list4, gloc_list2);
//  cout<<"neighbor bus stops size2 "<<bs_list2.size()<<endl; 

  if((b1 && b2) == false){
    cout<<" no valid bus stops for such two locations"<<endl;
    pm->CloseDualGraph(dg);
    pm->CloseVisualGraph(vg);
    sp->CloseBusNetwork(bn);
    sp->ClosePavement(pm);
    return;

  }

  vector<GenMO> res_list1; 
  vector<MPoint> res_list2;
  
  const double min_dist = 200.0;
  
  for(unsigned int i = 0;i < bs_list1.size();i++){

    for(unsigned int j = 0;j < bs_list2.size();j++){

    //////////////////////////////////////////////////////////////////
      GenMObject* genobj = new GenMObject();
      Instant start_time = *query_time;

     //////////////////////////////////////////////////////////////////////
     ////////////////add a trip only by walk, bus is not required//////////
     /////////////////////////////////////////////////////////////////////
     if(loc1.Distance(loc2) < min_dist){
        Instant start_time2 = *query_time;
        Walk_SP* wsp = new Walk_SP(dg, vg, NULL, NULL);
        wsp->rel3 = rel3;

        Line* path = new Line(0);
        ///////////////////walk segment////////////////////////////////////
        wsp->WalkShortestPath2(oid1, oid2, loc1, loc2, path);

        MPoint* mo2 = new MPoint(0);
        GenMO* genmo2 = new GenMO(0);
        mo2->StartBulkLoad();
        genmo2->StartBulkLoad();
        genobj->GenerateWalkMovement(dg, path, loc1, genmo2, mo2, start_time2);

        mo2->EndBulkLoad();
        genmo2->EndBulkLoad(); 


        res_list1.push_back(*genmo2);
        res_list2.push_back(*mo2);

        delete genmo2;
        delete mo2;

        delete path;
        delete wsp; 
        continue;
     }
      MPoint* mo = new MPoint(0);
      GenMO* genmo = new GenMO(0);
      mo->StartBulkLoad();
      genmo->StartBulkLoad();
    //////////////////////////////////////////////////////////////////
    /////2.connect the movement from start location to the bus stop//////
    //////////////////////////////////////////////////////////////////
    Line* res_path = new Line(0);
    vector<Point> bs_loc_list1;
    bs_loc_list1.push_back(ps_list1[i]);//bus stop mapping point on the pavement
    bs_loc_list1.push_back(ps_list2[i]);//bus stop 2d location
    Loc temp_loc1(loc1.GetX(), loc1.GetY());
    GenLoc query_loc1(oid1 + dg->min_tri_oid_1, temp_loc1); 
    genobj->ConnectStartBusStop(dg, vg, rel3, query_loc1, bs_loc_list1, 
                                gloc_list1[i].GetOid(),
                                genmo, mo, start_time, res_path);


    //////////////////////////////////////////////////////////////////
    /////3.connect the movement between two bus stops/////////////////
    //////////////////////////////////////////////////////////////////
    cout<<bs_list1[i]<<" "<<bs_list2[j]<<endl; 

    BNNav* bn_nav = new BNNav(bn);
    bn_nav->ShortestPath_Time(&bs_list1[i], &bs_list2[j], &start_time);

    if(bn_nav->path_list.size() == 0){
//          cout<<"two unreachable bus stops"<<endl;
        mo->EndBulkLoad();
        genmo->EndBulkLoad();
        delete mo;
        delete genmo;
        delete bn_nav;
        delete res_path;
        delete genobj;
        continue;
    }

    
    ///////////////////////////////////////////////////////////////////
    vector<Point> bs_loc_list2;
    bs_loc_list2.push_back(ps_list3[j]);//bus stop mapping point on the pavement
    bs_loc_list2.push_back(ps_list4[j]);//bus stop 2d location
    Loc temp_loc2(loc2.GetX(), loc2.GetY());
    GenLoc query_loc2(oid2 + dg->min_tri_oid_1, temp_loc2); 
    int end_bus_stop_tri_id = gloc_list2[j].GetOid(); 
    ///////////////////////////////////////////////////////////////////
    int last_walk_id = genobj->ConnectTwoBusStops(bn_nav, bs_loc_list1[1], 
                                                  bs_loc_list2[1],
                       genmo, mo, start_time, dg, res_path);

      ///////////////////////////////////////////////////////////
      if(last_walk_id > 0){
          //////change the last bus stop/////////
          ///change  ps list2, gloc2.oid ///
          Bus_Stop cur_bs1(true, 0, 0, true);
          genobj->StringToBusStop(bn_nav->bs1_list[last_walk_id], 
                          cur_bs1);
          GenLoc temp_gloc = gloc_list2[j];
          genobj->ChangeEndBusStop(bn, dg, cur_bs1, bs_loc_list2, 
                                 temp_gloc, rel4, rtree);
          end_bus_stop_tri_id = temp_gloc.GetOid(); 
      }
      delete bn_nav;


    //////////////////////////////////////////////////////////////////
    /////4.connect the movement from end bus stop to end location////
    //////////////////////////////////////////////////////////////////

    genobj->ConnectEndBusStop(dg, vg, rel3, query_loc2, bs_loc_list2, 
                              end_bus_stop_tri_id,
                              genmo, mo, start_time, res_path);
    ////////////////////////////////////////////////////////////////////
    //////////////store the paths//////////////////////////////////////
    ////////////////////////////////////////////////////////////////////
     mo->EndBulkLoad(); 
     genmo->EndBulkLoad();


     res_list1.push_back(*genmo);
     res_list2.push_back(*mo);

   /////////////////////////////////////////////////////////////////////
      delete res_path;
      delete genobj; 
      delete mo;
      delete genmo;
    }

  }

  //////////////////////////////////////////////////////////////////////
  pm->CloseDualGraph(dg);
  pm->CloseVisualGraph(vg);
  sp->CloseBusNetwork(bn);
  sp->ClosePavement(pm);
  
  /////////////////////////////////////////////////////////////////////
  int index = -1;
  double time_cost = numeric_limits<double>::max();
  

  for(unsigned int i = 0;i < res_list1.size();i++){
    MPoint mp = res_list2[i];
    UPoint unit1;
    mp.Get(0, unit1);
    UPoint unit2;
    mp.Get(mp.GetNoComponents() - 1, unit2);
    double t = unit2.timeInterval.end.ToDouble() - 
               unit1.timeInterval.start.ToDouble();
    if(t < time_cost){
      index = i;
      time_cost = t;
    }
  }

  if(index >= 0){
    trip_list1.push_back(res_list1[index]);
    trip_list2.push_back(res_list2[index]);
    loc_list1.push_back(loc1);
    loc_list2.push_back(loc2);
  }
}

/*
find all bus stops that their Euclidean distance to the query location is 
less than 200 meters

*/
bool Navigation::NearestBusStop1(Point loc, Relation* rel, 
                                 R_Tree<2,TupleId>* rtree, 
                       vector<Bus_Stop>& bs_list1, vector<Point>& ps_list1, 
                       vector<Point>& ps_list2, vector<GenLoc>& gloc_list1)
{

  Point p = loc;
  SmiRecordId adr = rtree->RootRecordId();

  vector<int> tid_list;
  GenMObject* genobj = new GenMObject();
  genobj->DFTraverse1(rtree, adr, rel, p, tid_list);
  delete genobj; 

//  cout<<p<<" "<<tid_list.size()<<endl;
  
  if(tid_list.size() == 0) return false;///no bus stops with Euclidean 500m
  
  vector<MyPoint_Tid> mp_tid_list;
  for(unsigned int i = 0;i < tid_list.size();i++){
    Tuple* tuple = rel->GetTuple(tid_list[i], false);
    Point* q = (Point*)tuple->GetAttribute(BN::BN_PAVE_LOC2);
    MyPoint_Tid mp_tid(*q, q->Distance(p), tid_list[i]);
    mp_tid_list.push_back(mp_tid);
    tuple->DeleteIfAllowed();
  }
  sort(mp_tid_list.begin(), mp_tid_list.end());

  const double delta_dist = 200.0;

  if(mp_tid_list[0].dist > delta_dist){//larger than delta dist, take the first
      Tuple* bs_pave = rel->GetTuple(mp_tid_list[0].tid, false);
      Bus_Stop* bs = (Bus_Stop*)bs_pave->GetAttribute(BN::BN_BUSSTOP);
      Point* bs_loc = (Point*)bs_pave->GetAttribute(BN::BN_BUSLOC);
      GenLoc* bs_loc1 = (GenLoc*)bs_pave->GetAttribute(BN::BN_PAVE_LOC1);
      Point* bs_loc2 = (Point*)bs_pave->GetAttribute(BN::BN_PAVE_LOC2);

      bs_list1.push_back(*bs);
      ps_list1.push_back(*bs_loc2);
      ps_list2.push_back(*bs_loc);
      gloc_list1.push_back(*bs_loc1);

      bs_pave->DeleteIfAllowed();
  }else{

    for(unsigned int i = 0;i < mp_tid_list.size();i++){
      if(mp_tid_list[i].dist > delta_dist) break;

      Tuple* bs_pave = rel->GetTuple(mp_tid_list[i].tid, false);
      Bus_Stop* bs = (Bus_Stop*)bs_pave->GetAttribute(BN::BN_BUSSTOP);
      Point* bs_loc = (Point*)bs_pave->GetAttribute(BN::BN_BUSLOC);
      GenLoc* bs_loc1 = (GenLoc*)bs_pave->GetAttribute(BN::BN_PAVE_LOC1);
      Point* bs_loc2 = (Point*)bs_pave->GetAttribute(BN::BN_PAVE_LOC2);

      bs_list1.push_back(*bs);
      ps_list1.push_back(*bs_loc2);
      ps_list2.push_back(*bs_loc);
      gloc_list1.push_back(*bs_loc1);

      bs_pave->DeleteIfAllowed();

      ///////filter bus stops with the same 2D location but different routes//
      ////////this can be found later by the bus graph///////////////////
      //////////transfer without moving/////////////////////////////////
      unsigned int j = i + 1;
      if( j < mp_tid_list.size()){
        double cur_dist = mp_tid_list[i].dist;
        const double delta_d = 0.01;
        while(fabs(mp_tid_list[j].dist - cur_dist) < delta_d && 
              j < mp_tid_list.size())
          j++;

        i = j - 1;
      }

    }
  }

  if(bs_list1.size() > 0) return true; 

  return false;
  
}


//////////////////////////////////////////////////////////////////////
//////////////////////Global Space////////////////////////////////////
//////////////////////////////////////////////////////////////////////
string Space::FreeSpaceTypeInfo = "(rel (tuple ((oid int))))";

ListExpr SpaceProperty()
{
  return (nl->TwoElemList(
            nl->FourElemList(nl->StringAtom("Signature"),
                       nl->StringAtom("Example Type List"),
           nl->StringAtom("List Rep"),
           nl->StringAtom("Example List")),
            nl->FourElemList(nl->StringAtom("-> MAPPING"),
                       nl->StringAtom("space"),
           nl->StringAtom("(infrastructures)"),
      nl->StringAtom("(infrastructure objects)"))));
}

/*
In function

*/
Word InSpace( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct )
{

  if(!(nl->ListLength(instance) == 1)){
     string strErrorMessage = "Space(): initial length must be 1.";
     errorInfo =
        nl->Append ( errorInfo, nl->StringAtom ( strErrorMessage ) );
      correct = false;
    return SetWord(Address(0));
  }

  ListExpr xIdList = nl->First ( instance );

  // Read Id
  if ( !nl->IsAtom ( xIdList ) ||
          nl->AtomType ( xIdList ) != IntType )
  {
    string strErrorMessage = "Space(): Id is missing.";
    errorInfo = nl->Append ( errorInfo,
                             nl->StringAtom ( strErrorMessage ) );
    correct = false;
    return SetWord(Address(0));
  }
  
    unsigned int space_id = nl->IntValue(xIdList);
    Space* sp = new Space(true, space_id);
    correct = true; 
    return SetWord(sp);

}

ListExpr OutSpace( ListExpr typeInfo, Word value )
{
//  cout<<"OutSpace"<<endl; 
  Space* sp = (Space*)(value.addr);

 if(!sp->IsDefined()){
   return nl->SymbolAtom("undef");
 }
 ListExpr space_list = nl->TwoElemList(nl->StringAtom("SpaceId:"),
                        nl->IntAtom(sp->GetSpaceId())); 

 ListExpr infra_list = nl->TheEmptyList();
 bool bFirst = true;
 ListExpr xNext = nl->TheEmptyList();
 ListExpr xLast = nl->TheEmptyList();
 for(int i = 0;i < sp->Size();i++){
  InfraRef inf_ref;
  sp->Get(i, inf_ref); 
  string str = GetSymbolStr(inf_ref.infra_type);
  ListExpr list1 = nl->OneElemList(nl->StringAtom(str));
  ListExpr list2 = nl->TwoElemList(nl->StringAtom("InfraId:"),
                        nl->IntAtom(inf_ref.infra_id));
  ListExpr list3 = nl->TwoElemList(
                           nl->TwoElemList(
                                nl->StringAtom("min ref id:"),
                                nl->IntAtom(inf_ref.ref_id_low)),
                           nl->TwoElemList(
                                nl->StringAtom("max ref id:"),
                                nl->IntAtom(inf_ref.ref_id_high)));

  xNext = nl->ThreeElemList(list1, list2, list3);
  if(bFirst){
      infra_list = nl->OneElemList(xNext);
      xLast = infra_list;
      bFirst = false;
  }else
    xLast = nl->Append(xLast,xNext);
 }
 
 return nl->TwoElemList(space_list, infra_list);
}


bool CheckSpace( ListExpr type, ListExpr& errorInfo )
{
//  cout<<"CheckSpace"<<endl;
  return (nl->IsEqual( type, "space" ));
}

int SizeOfSpace()
{
//  cout<<"SizeOfSpacc"<<endl; 
  return sizeof(Space);
}


Word CreateSpace(const ListExpr typeInfo)
{
// cout<<"CreateSpace()"<<endl;
  return SetWord (new Space(false));
}


void DeleteSpace(const ListExpr typeInfo, Word& w)
{
//  cout<<"DeleteSpace()"<<endl;
  Space* sp = (Space*)w.addr;
  delete sp;
   w.addr = NULL;
}


void CloseSpace( const ListExpr typeInfo, Word& w )
{
//  cout<<"CloseSpace"<<endl; 
  delete static_cast<Space*>(w.addr);
  w.addr = NULL;
}

Word CloneSpace( const ListExpr typeInfo, const Word& w )
{
//  cout<<"CloneSpace"<<endl; 
//  return SetWord( new Space( *((Space*)w.addr) ) );
  return SetWord(Address(0));
}


Space::Space(const Space& sp):Attribute(sp.IsDefined()), infra_list(0)
{
  if(sp.IsDefined()){
    def = sp.def;
    space_id = sp.space_id; 
    for(int i = 0;i < sp.Size();i++){
      InfraRef inf_ref; 
      sp.Get(i, inf_ref); 
      infra_list.Append(inf_ref);
    }
  }

}


Space& Space::operator=(const Space& sp)
{
  SetDefined(sp.IsDefined());
  if(def){
    space_id = sp.space_id;
    for(int i = 0;i < sp.Size();i++){
      InfraRef inf_ref; 
      sp.Get(i, inf_ref); 
      infra_list.Append(inf_ref);
    }
  }
  return *this; 
}


Space::~Space()
{

}

void Space::SetId(int id)
{
  if(id > 0){
    space_id = id;
    def = true;
  }else
    def = false; 

}


/*
get infrastructure ref representation 

*/
void Space::Get(int i, InfraRef& inf_ref) const 
{
  assert(0 <= i && i < infra_list.Size());
  infra_list.Get(i, inf_ref);
}

void Space::Add(InfraRef& inf_ref)
{
  infra_list.Append(inf_ref); 
}

/*
add network infrastructure to the space 

*/
void Space::AddRoadNetwork(Network* n)
{
  InfraRef inf_ref; 
  if(!n->IsDefined()){
    cout<<"road network is not defined"<<endl;
    return; 
  }
  inf_ref.infra_id = n->GetId();
  inf_ref.infra_type = GetSymbol("LINE"); 
  int min_id = numeric_limits<int>::max();
  int max_id = numeric_limits<int>::min();
  Relation* routes_rel = n->GetRoutes();
  for(int i = 1;i <= routes_rel->GetNoTuples();i++){
    Tuple* route_tuple = routes_rel->GetTuple(i, false);
    int rid = ((CcInt*)route_tuple->GetAttribute(ROUTE_ID))->GetIntval(); 
    if(rid < min_id) min_id = rid;
    if(rid > max_id) max_id = rid; 
    route_tuple->DeleteIfAllowed(); 
  }
  inf_ref.ref_id_low = min_id;
  inf_ref.ref_id_high = max_id; 
  if(CheckExist(inf_ref) == false){
      inf_ref.Print(); 
      Add(inf_ref); 
  }else
    cout<<"this infrastructure exists already"<<endl; 
}

/*
check whether the infrastructure has been added already 
and overlapping oids 

*/
bool Space::CheckExist(InfraRef& inf_ref)
{

  for(int i = 0;i < infra_list.Size();i++){
    InfraRef elem;
    infra_list.Get(i, elem); 
    if(elem.infra_type == inf_ref.infra_type && 
       elem.infra_id == inf_ref.infra_id) return true; 

    if(inf_ref.ref_id_low > elem.ref_id_high || 
       inf_ref.ref_id_high < elem.ref_id_low){
    }else
      return true; 
  }
  return false; 
}

/*
get the required infrastructure relation by type 

*/
Relation* Space::GetInfra(string type)
{
  Relation* result = NULL;
  int infra_type = GetSymbol(type);

  if(infra_type == IF_LINE){ //////////road network 
      Network* rn = LoadRoadNetwork(IF_LINE);
      if(rn != NULL){ 
          result = rn->GetRoutes()->Clone();
          CloseRoadNetwork(rn);
      }else{
          cout<<"road network does exist "<<endl; 
          ListExpr xTypeInfo;
          nl->ReadFromString(Network::routesTypeInfo, xTypeInfo);
          ListExpr xNumType = 
                SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
          result = new Relation(xNumType, true);
      }
   }else if(infra_type == IF_FREESPACE){//////////free space 
       ListExpr xTypeInfo;
       nl->ReadFromString(FreeSpaceTypeInfo, xTypeInfo);
       ListExpr xNumType = 
                SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
       result = new Relation(xNumType, true);
   }else if(infra_type == IF_REGION){ //////pavement 
      Pavement* pn = LoadPavement(IF_REGION);
      if(pn != NULL){ 
          result = pn->GetPaveRel()->Clone();
          ClosePavement(pn);
      }else{
          cout<<"pavement does exist "<<endl; 
          ListExpr xTypeInfo;
          nl->ReadFromString(Pavement::PaveTypeInfo, xTypeInfo);
          ListExpr xNumType = 
                SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
          result = new Relation(xNumType, true);
      }
   }else if(infra_type == IF_BUSSTOP){ ////bus stops 
        BusNetwork* bn = LoadBusNetwork(IF_BUSNETWORK);
        if(bn != NULL){ 
          result = bn->GetBS_Rel()->Clone();
          CloseBusNetwork(bn);
        }else{
          cout<<"bus network does exist "<<endl; 
          ListExpr xTypeInfo;
          nl->ReadFromString(BusNetwork::BusStopsInternalTypeInfo, xTypeInfo);
          ListExpr xNumType = 
                SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
          result = new Relation(xNumType, true);
        }
   }else if(infra_type == IF_BUSROUTE){ ///bus routes 
        BusNetwork* bn = LoadBusNetwork(IF_BUSNETWORK);
        if(bn != NULL){ 
          result = bn->GetBR_Rel()->Clone();
          CloseBusNetwork(bn);
        }else{
          cout<<"bus network does exist "<<endl; 
          ListExpr xTypeInfo;
          nl->ReadFromString(BusNetwork::BusRoutesTypeInfo, xTypeInfo);
          ListExpr xNumType = 
                SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
          result = new Relation(xNumType, true);
        }

   }else if(infra_type == IF_MPPTN){ ///bus trips 
        BusNetwork* bn = LoadBusNetwork(IF_BUSNETWORK);
        if(bn != NULL){ 
          result = bn->GetBT_Rel()->Clone();
          CloseBusNetwork(bn);
        }else{
          cout<<"bus network does exist "<<endl; 
          ListExpr xTypeInfo;
          nl->ReadFromString(BusNetwork::BusTripsTypeInfo, xTypeInfo);
          ListExpr xNumType = 
                SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
          result = new Relation(xNumType, true);
        }

   }else if(infra_type == IF_METROSTOP){ ///metro stops
        MetroNetwork* mn = LoadMetroNetwork(IF_METRONETWORK);
        if(mn != NULL){ 
          result = mn->GetMS_Rel()->Clone();
          CloseMetroNetwork(mn);
        }else{
          cout<<"metro network does exist "<<endl; 
          ListExpr xTypeInfo;
          nl->ReadFromString(MetroNetwork::MetroStopsTypeInfo, xTypeInfo);
          ListExpr xNumType = 
                SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
          result = new Relation(xNumType, true);
        }

   }else if(infra_type == IF_METROROUTE){ // metro routes 
        MetroNetwork* mn = LoadMetroNetwork(IF_METRONETWORK);
        if(mn != NULL){ 
          result = mn->GetMR_Rel()->Clone();
          CloseMetroNetwork(mn);
        }else{
          cout<<"metro network does exist "<<endl; 
          ListExpr xTypeInfo;
          nl->ReadFromString(MetroNetwork::MetroRoutesTypeInfo, xTypeInfo);
          ListExpr xNumType = 
                SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
          result = new Relation(xNumType, true);
        }

   }else if(infra_type == IF_METRO){// metro trips 
      MetroNetwork* mn = LoadMetroNetwork(IF_METRONETWORK);
      if(mn != NULL){ 
        result = mn->GetMetro_Rel()->Clone();
        CloseMetroNetwork(mn);
      }else{
        cout<<"metro network does exist "<<endl; 
        ListExpr xTypeInfo;
        nl->ReadFromString(MetroNetwork::MetroTripTypeInfo, xTypeInfo);
        ListExpr xNumType = 
                SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
        result = new Relation(xNumType, true);
      }
   }else{
        cout<<"wrong type or does not exist"<<endl;
   }
  if(result == NULL){/////////returns an empty relation 
      ListExpr xTypeInfo;
      nl->ReadFromString(FreeSpaceTypeInfo, xTypeInfo);
      ListExpr xNumType = SecondoSystem::GetCatalog()->NumericType(xTypeInfo);
      result = new Relation(xNumType, true);
  }
  return result; 
}

/*
Load the road network

*/
Network* Space:: LoadRoadNetwork(int type)
{
  InfraRef rn_ref; 
  bool found = false; 
  for(int i = 0;i < infra_list.Size();i++){
    InfraRef elem;
    infra_list.Get(i, elem); 
    if(elem.infra_type == type){
       rn_ref = elem; 
       found = true;
       break; 
    }
  }
  if(found){
      ListExpr xObjectList = SecondoSystem::GetCatalog()->ListObjects();
      xObjectList = nl->Rest(xObjectList);
      while(!nl->IsEmpty(xObjectList)){
          // Next element in list
          ListExpr xCurrent = nl->First(xObjectList);
          xObjectList = nl->Rest(xObjectList);
          // Type of object is at fourth position in list
          ListExpr xObjectType = nl->First(nl->Fourth(xCurrent));
          if(nl->IsAtom(xObjectType) &&
              nl->SymbolValue(xObjectType) == "network"){
            // Get name of the bus graph 
            ListExpr xObjectName = nl->Second(xCurrent);
            string strObjectName = nl->SymbolValue(xObjectName);

            // Load object to find out the id of the network. Normally their
            // won't be to much networks in one database giving us a good
            // chance to load only the wanted network.
            Word xValue;
            bool bDefined;
            bool bOk = SecondoSystem::GetCatalog()->GetObject(strObjectName,
                                                        xValue,
                                                        bDefined);
            if(!bDefined || !bOk){
              // Undefined
              continue;
            }
            Network* rn = (Network*)xValue.addr;
            if(rn->GetId() == rn_ref.infra_id){
              // This is the road network we have been looking for
              return rn;
            }
          }
      }
  }
    return NULL;
}

/*
close the road network 

*/
void Space::CloseRoadNetwork(Network* rn)
{
  if(rn == NULL) return; 
  Word xValue;
  xValue.addr = rn;
  SecondoSystem::GetCatalog()->CloseObject(nl->SymbolAtom( "network" ),
                                           xValue);
}


/*
add pavement infrastructure to the space 
for the pavement infrastructure, we record the maximum is the largest 
  triangle id plus 1. the largest id means the overall polygon pavement 

*/
void Space::AddPavement(Pavement* pn)
{
  InfraRef inf_ref; 
  if(!pn->IsDefined()){
    cout<<"pavement is not defined"<<endl;
    return; 
  }
  inf_ref.infra_id = pn->GetId();
  inf_ref.infra_type = GetSymbol("REGION"); 
  int min_id = numeric_limits<int>::max();
  int max_id = numeric_limits<int>::min();
  Relation* pave_rel = pn->GetPaveRel();
  for(int i = 1;i <= pave_rel->GetNoTuples();i++){
    Tuple* pave_tuple = pave_rel->GetTuple(i, false);
    int id = ((CcInt*)pave_tuple->GetAttribute(Pavement::P_OID))->GetIntval();
    if(id < min_id) min_id = id;
    if(id > max_id) max_id = id; 
    pave_tuple->DeleteIfAllowed(); 
  }
  inf_ref.ref_id_low = min_id;
  inf_ref.ref_id_high = max_id + 1;///for the overall large region 
  if(CheckExist(inf_ref) == false){
      inf_ref.Print(); 
      Add(inf_ref); 
  }else{
    cout<<"insert infrastructure wrong"<<endl; 
    cout<<"infrastructure exists already or wrong oid"<<endl; 
  }
}

/*
load pavement infrastructure 

*/
Pavement* Space::LoadPavement(int type)
{
  InfraRef rn_ref; 
  bool found = false; 
  for(int i = 0;i < infra_list.Size();i++){
    InfraRef elem;
    infra_list.Get(i, elem); 
    if(elem.infra_type == type){
       rn_ref = elem; 
       found = true;
       break; 
    }
  }
  if(found){
      ListExpr xObjectList = SecondoSystem::GetCatalog()->ListObjects();
      xObjectList = nl->Rest(xObjectList);
      while(!nl->IsEmpty(xObjectList)){
          // Next element in list
          ListExpr xCurrent = nl->First(xObjectList);
          xObjectList = nl->Rest(xObjectList);
          // Type of object is at fourth position in list
          ListExpr xObjectType = nl->First(nl->Fourth(xCurrent));
          if(nl->IsAtom(xObjectType) &&
              nl->SymbolValue(xObjectType) == "pavenetwork"){
            // Get name of the bus graph 
            ListExpr xObjectName = nl->Second(xCurrent);
            string strObjectName = nl->SymbolValue(xObjectName);

            // Load object to find out the id of the pavement
            Word xValue;
            bool bDefined;
            bool bOk = SecondoSystem::GetCatalog()->GetObject(strObjectName,
                                                        xValue,
                                                        bDefined);
            if(!bDefined || !bOk){
              // Undefined
              continue;
            }
            Pavement* pn = (Pavement*)xValue.addr;
            if((int)pn->GetId() == rn_ref.infra_id){
              // This is the pavement we have been looking for

              return pn;
            }
          }
      }
  }
  return NULL;
}


void Space::ClosePavement(Pavement* pn)
{
 if(pn == NULL) return; 
  Word xValue;
  xValue.addr = pn;
  SecondoSystem::GetCatalog()->CloseObject(nl->SymbolAtom( "pavenetwork" ),
                                           xValue);
}

/*
add bus network into the space 

*/
void Space::AddBusNetwork(BusNetwork* bn)
{
  InfraRef inf_ref; 
  if(!bn->IsDefined()){
    cout<<"bus network is not defined"<<endl;
    return; 
  }
  inf_ref.infra_id = bn->GetId();
  inf_ref.infra_type = GetSymbol("BUSNETWORK"); 
  int min_id = numeric_limits<int>::max();
  int max_id = numeric_limits<int>::min();
  Relation* bn_routes = bn->GetBR_Rel();
  for(int i = 1;i <= bn_routes->GetNoTuples();i++){
    Tuple* br_tuple = bn_routes->GetTuple(i, false);
    int br_id = 
       ((CcInt*)br_tuple->GetAttribute(BusNetwork::BN_BR_OID))->GetIntval();
    if(br_id < min_id) min_id = br_id;
    if(br_id > max_id) max_id = br_id; 
    br_tuple->DeleteIfAllowed(); 
  }

  Relation* bn_bus = bn->GetBT_Rel();
  for(int i = 1;i <= bn_bus->GetNoTuples();i++){
    Tuple* bus_tuple = bn_bus->GetTuple(i, false);
    int bus_id = 
       ((CcInt*)bus_tuple->GetAttribute(BusNetwork::BN_BUS_OID))->GetIntval();
    if(bus_id < min_id) min_id = bus_id;
    if(bus_id > max_id) max_id = bus_id; 
    bus_tuple->DeleteIfAllowed(); 
  }


  inf_ref.ref_id_low = min_id;
  inf_ref.ref_id_high = max_id; 
  if(CheckExist(inf_ref) == false){
      inf_ref.Print(); 
      Add(inf_ref); 
  }else
    cout<<"this infrastructure exists already"<<endl; 

}

/*
load the bus network 

*/
BusNetwork* Space::LoadBusNetwork(int type)
{

  InfraRef rn_ref; 
  bool found = false; 
  for(int i = 0;i < infra_list.Size();i++){
    InfraRef elem;
    infra_list.Get(i, elem); 
    if(elem.infra_type == type){
       rn_ref = elem; 
       found = true;
       break; 
    }
  }
  if(found){
      ListExpr xObjectList = SecondoSystem::GetCatalog()->ListObjects();
      xObjectList = nl->Rest(xObjectList);
      while(!nl->IsEmpty(xObjectList)){
          // Next element in list
          ListExpr xCurrent = nl->First(xObjectList);
          xObjectList = nl->Rest(xObjectList);
          // Type of object is at fourth position in list
          ListExpr xObjectType = nl->First(nl->Fourth(xCurrent));
          if(nl->IsAtom(xObjectType) &&
              nl->SymbolValue(xObjectType) == "busnetwork"){
            // Get name of the bus graph 
            ListExpr xObjectName = nl->Second(xCurrent);
            string strObjectName = nl->SymbolValue(xObjectName);

            // Load object to find out the id of the pavement
            Word xValue;
            bool bDefined;
            bool bOk = SecondoSystem::GetCatalog()->GetObject(strObjectName,
                                                        xValue,
                                                        bDefined);
            if(!bDefined || !bOk){
              // Undefined
              continue;
            }
            BusNetwork* bn = (BusNetwork*)xValue.addr;
            if((int)bn->GetId() == rn_ref.infra_id){
              // This is the busnetwork we have been looking for

              return bn;
            }
          }
      }
  }
  return NULL;

}

void Space::CloseBusNetwork(BusNetwork* bn)
{
  if(bn == NULL) return; 
  Word xValue;
  xValue.addr = bn;
  SecondoSystem::GetCatalog()->CloseObject(nl->SymbolAtom( "busnetwork" ),
                                           xValue);
}


/*
add metro network into the space 

*/
void Space::AddMetroNetwork(MetroNetwork* mn)
{
  InfraRef inf_ref;
  if(!mn->IsDefined()){
    cout<<"metro network is not defined"<<endl;
    return; 
  }
  inf_ref.infra_id = mn->GetId();
  inf_ref.infra_type = GetSymbol("METRONETWORK");
  int min_id = numeric_limits<int>::max();
  int max_id = numeric_limits<int>::min();

  Relation* mn_routes = mn->GetMR_Rel();

  for(int i = 1;i <= mn_routes->GetNoTuples();i++){
    Tuple* mr_tuple = mn_routes->GetTuple(i, false);
    int mr_id = 
       ((CcInt*)mr_tuple->GetAttribute(MetroNetwork::M_R_OID))->GetIntval();
    if(mr_id < min_id) min_id = mr_id;
    if(mr_id > max_id) max_id = mr_id;
    mr_tuple->DeleteIfAllowed(); 
  }

  Relation* mn_metro = mn->GetMetro_Rel();
  for(int i = 1;i <= mn_metro->GetNoTuples();i++){
    Tuple* metro_tuple = mn_metro->GetTuple(i, false);
    int metro_id = 
     ((CcInt*)metro_tuple->GetAttribute(MetroNetwork::M_TRIP_OID))->GetIntval();
    if(metro_id < min_id) min_id = metro_id;
    if(metro_id > max_id) max_id = metro_id; 
    metro_tuple->DeleteIfAllowed(); 
  }


  inf_ref.ref_id_low = min_id;
  inf_ref.ref_id_high = max_id; 
  if(CheckExist(inf_ref) == false){
      inf_ref.Print(); 
      Add(inf_ref); 
  }else
    cout<<"this infrastructure exists already"<<endl; 

}

/*
load the metro network 

*/
MetroNetwork* Space::LoadMetroNetwork(int type)
{

  InfraRef rn_ref; 
  bool found = false; 
  for(int i = 0;i < infra_list.Size();i++){
    InfraRef elem;
    infra_list.Get(i, elem); 
    if(elem.infra_type == type){
       rn_ref = elem; 
       found = true;
       break; 
    }
  }
  if(found){
      ListExpr xObjectList = SecondoSystem::GetCatalog()->ListObjects();
      xObjectList = nl->Rest(xObjectList);
      while(!nl->IsEmpty(xObjectList)){
          // Next element in list
          ListExpr xCurrent = nl->First(xObjectList);
          xObjectList = nl->Rest(xObjectList);
          // Type of object is at fourth position in list
          ListExpr xObjectType = nl->First(nl->Fourth(xCurrent));
          if(nl->IsAtom(xObjectType) &&
              nl->SymbolValue(xObjectType) == "metronetwork"){
            // Get name of the metro graph
            ListExpr xObjectName = nl->Second(xCurrent);
            string strObjectName = nl->SymbolValue(xObjectName);

            // Load object to find out the id of the metro network
            Word xValue;
            bool bDefined;
            bool bOk = SecondoSystem::GetCatalog()->GetObject(strObjectName,
                                                        xValue,
                                                        bDefined);
            if(!bDefined || !bOk){
              // Undefined
              continue;
            }
            MetroNetwork* mn = (MetroNetwork*)xValue.addr;
            if((int)mn->GetId() == rn_ref.infra_id){
              // This is the metronetwork we have been looking for
              return mn;
            }
          }
      }
  }
  return NULL;

}


void Space::CloseMetroNetwork(MetroNetwork* mn)
{
  if(mn == NULL) return; 
  Word xValue;
  xValue.addr = mn;
  SecondoSystem::GetCatalog()->CloseObject(nl->SymbolAtom("metronetwork"),
                                           xValue);
}


/*
add indoor infrastructure to the space 

*/
void Space::AddIndoorInfra(IndoorInfra* indoor)
{
//  cout<<"AddIndoorInfra "<<endl;

  InfraRef inf_ref; 
  if(!indoor->IsDefined()){
    cout<<"indoor infrastructure is not defined"<<endl;
    return; 
  }
  inf_ref.infra_id = indoor->GetId();
  inf_ref.infra_type = GetSymbol("GROOM"); 
  int min_id = numeric_limits<int>::max();
  int max_id = numeric_limits<int>::min();
  Relation* build_type_rel = indoor->BuildingType_Rel();
  
  for(int i = 1;i <= build_type_rel->GetNoTuples();i++){
    Tuple* build_tuple = build_type_rel->GetTuple(i, false);
    int build_id = ((CcInt*)build_tuple->GetAttribute(IndoorInfra::
                                           INDOORIF_BUILD_ID))->GetIntval();
    if(build_id < min_id) min_id = build_id;
    if(build_id > max_id) max_id = build_id; 

    build_tuple->DeleteIfAllowed(); 
  }

  inf_ref.ref_id_low = min_id;
  inf_ref.ref_id_high = max_id;

  if(CheckExist(inf_ref) == false){
      inf_ref.Print(); 
      Add(inf_ref); 
  }else{
    cout<<"insert infrastructure wrong"<<endl; 
    cout<<"infrastructure exists already or wrong oid"<<endl; 
  }

}


/*
load indoor infrastructure 

*/
IndoorInfra* Space::LoadIndoorInfra(int type)
{
//  cout<<"loadindoorinfra"<<endl; 
  
  InfraRef rn_ref; 
  bool found = false; 
  for(int i = 0;i < infra_list.Size();i++){
    InfraRef elem;
    infra_list.Get(i, elem); 
    if(elem.infra_type == type){
       rn_ref = elem; 
       found = true;
       break; 
    }
  }
  if(found){
      ListExpr xObjectList = SecondoSystem::GetCatalog()->ListObjects();
      xObjectList = nl->Rest(xObjectList);
      while(!nl->IsEmpty(xObjectList)){
          // Next element in list
          ListExpr xCurrent = nl->First(xObjectList);
          xObjectList = nl->Rest(xObjectList);
          // Type of object is at fourth position in list
          ListExpr xObjectType = nl->First(nl->Fourth(xCurrent));
          if(nl->IsAtom(xObjectType) &&
              nl->SymbolValue(xObjectType) == "indoorinfra"){
            // Get name of the bus graph 
            ListExpr xObjectName = nl->Second(xCurrent);
            string strObjectName = nl->SymbolValue(xObjectName);

            // Load object to find out the id of the pavement
            Word xValue;
            bool bDefined;
            bool bOk = SecondoSystem::GetCatalog()->GetObject(strObjectName,
                                                        xValue,
                                                        bDefined);
            if(!bDefined || !bOk){
              // Undefined
              continue;
            }
            IndoorInfra* indoor_infra = (IndoorInfra*)xValue.addr;
            if((int)indoor_infra->GetId() == rn_ref.infra_id){
            // This is the indoor infrastructure we have been looking for
              return indoor_infra;
            }
          }
      }
  }
  return NULL;

}

void Space::CloseIndoorInfra(IndoorInfra* indoor_infra)
{
//  cout<<"close indoorinfra"<<endl; 
  
  if(indoor_infra == NULL) return; 
    Word xValue;
    xValue.addr = indoor_infra;
    SecondoSystem::GetCatalog()->CloseObject(nl->SymbolAtom( "indoorinfra"),
                                           xValue);
}


/*
open all infrastructures
"BUSSTOP", "BUSROUTE", "MPPTN", "BUSNETWORK", "GROOM", 
"REGION", "LINE", "FREESPACE"

*/
void Space::OpenInfra(vector<void*>& infra_pointer)
{

  infra_pointer.push_back(NULL);//bus stop 
  infra_pointer.push_back(NULL);//bus route
  infra_pointer.push_back(NULL);//mpptn
  infra_pointer.push_back(LoadBusNetwork(IF_BUSNETWORK));//bus network
  infra_pointer.push_back(LoadIndoorInfra(IF_GROOM));//indoor 
  infra_pointer.push_back(LoadPavement(IF_REGION));// region based outdoor
  infra_pointer.push_back(LoadRoadNetwork(IF_LINE));// road netowrk
  infra_pointer.push_back(NULL);// free space
  infra_pointer.push_back(LoadMetroNetwork(IF_METRONETWORK));// metro network 


  if((BusNetwork*)infra_pointer[IF_BUSNETWORK] == NULL){
    cerr<<__FILE__<<__LINE__<<"bus network can not be opend"<<endl;
  }

  if((Network*)infra_pointer[IF_LINE] == NULL){
    cerr<<__FILE__<<__LINE__<<"road network can not be opend"<<endl;
  }


  if((Pavement*)infra_pointer[IF_REGION] == NULL){
    cerr<<__FILE__<<__LINE__<<"pavement can not be opend"<<endl;
  }

  if((IndoorInfra*)infra_pointer[IF_GROOM] == NULL){
    cerr<<__FILE__<<__LINE__<<"indoor infrastructure can not be opend"<<endl;
  }

  if((MetroNetwork*)infra_pointer[IF_METRONETWORK] == NULL){
    cerr<<__FILE__<<__LINE__<<"metro network can not be opend"<<endl;
  }

}

/*
close all infrastructures

*/
void Space::CloseInfra(vector<void*>& infra_pointer)
{
  if(infra_pointer[IF_BUSNETWORK] != NULL)
    CloseBusNetwork((BusNetwork*)infra_pointer[IF_BUSNETWORK]);

   if(infra_pointer[IF_GROOM] != NULL)
     CloseIndoorInfra((IndoorInfra*)infra_pointer[IF_GROOM]);


  if(infra_pointer[IF_REGION] != NULL)
    ClosePavement((Pavement*)infra_pointer[IF_REGION]);

  if(infra_pointer[IF_LINE] != NULL)
    CloseRoadNetwork((Network*)infra_pointer[IF_LINE]);

  ////////////metro network //////////////////////////

  if(infra_pointer[IF_METRONETWORK] != NULL)
    CloseMetroNetwork((MetroNetwork*)infra_pointer[IF_METRONETWORK]);
}


/*
from the infrastructure object oid, it gets the infrastructure type 

*/
int Space::GetInfraType(int oid)
{
  int infra_type = -1;
  if(oid == 0){ ///////////////in free space
     infra_type = IF_FREESPACE;
  }else{
    if(oid > MaxRefId()){////////////indoor movement 
      infra_type = IF_GROOM;
    }else{

      for(int i = 0;i < infra_list.Size();i++){
        InfraRef elem;
        infra_list.Get(i, elem); 

        if(elem.ref_id_low <= oid && oid <= elem.ref_id_high){
          infra_type = elem.infra_type;
          break;
        }
      }
    }
  }

  if(infra_type < 0){
    cout<<"error no such infrastructure object"<<endl;
    assert(false);

  }

  return infra_type; 
}

/*
get the maximum reference id in the current space 

*/
int Space::MaxRefId()
{
  int max_ref_id = 0;
  
  for(int i = 0;i < infra_list.Size();i++){
      InfraRef elem;
      infra_list.Get(i, elem); 
      if(elem.ref_id_high > max_ref_id){
        max_ref_id = elem.ref_id_high;
      }
  }
  return max_ref_id;
}

/*
get the movement inside an infrastructure object 

*/
void Space::GetLineInIFObject(int& oid, GenLoc gl1, GenLoc gl2, 
                              Line* l, vector<void*> infra_pointer, 
                              Interval<Instant> time_interval, 
                              int infra_type)
{
  assert(oid >= 0); 

  switch(infra_type){
    case IF_LINE:
//      cout<<"road network "<<endl;
      GetLineInRoad(oid, gl1, gl2, l, (Network*)infra_pointer[IF_LINE]);
      break;

    case IF_REGION:
//      cout<<"region based outdoor"<<endl;
      GetLineInRegion(oid, gl1, gl2, l);
      break; 

    case IF_FREESPACE:
//      cout<<"free space"<<endl;
      GetLineInFreeSpace(gl1, gl2, l);
      break;

    case IF_BUSNETWORK:
//      cout<<"bus network"<<endl;
      GetLineInBusNetwork(oid, l,
                          (BusNetwork*)infra_pointer[IF_BUSNETWORK],
                          time_interval);
      break;

    case IF_GROOM:
//      cout<<"indoor "<<endl;
      GetLineInGRoom(oid, gl1, gl2, l);
      break;

    default:
      assert(false);
      break;
  }
}

/*
get the sub movement on a road 

*/
void Space::GetLineInRoad(int oid, GenLoc gl1, GenLoc gl2, Line* l, Network* rn)
{
  
  Tuple* route_tuple = rn->GetRoute(oid);
  SimpleLine* sl = (SimpleLine*)route_tuple->GetAttribute(ROUTE_CURVE);
  bool dual = ((CcBool*)route_tuple->GetAttribute(ROUTE_DUAL))->GetBoolval();

//   cout<<"pos1 "<<gl1.GetLoc().loc1<<" "
//       <<gl2.GetLoc().loc1<<" dual "<<dual<<endl;

  double pos1 = gl1.GetLoc().loc1;
  double pos2 = gl2.GetLoc().loc1;
  if(dual){
    if(pos1 > pos2){
      double pos = pos1;
      pos1 = pos2;
      pos2 = pos;
    }
  }else{
    if(pos1 < pos2){
      double pos = pos1;
      pos1 = pos2;
      pos2 = pos;
    }
  }

  SimpleLine* sub_sl = new SimpleLine(0);
  sl->SubLine(pos1, pos2, dual, *sub_sl);
  Rectangle<2> bbox = sl->BoundingBox();

//  cout<<"length1 "<<sl->Length()<<" length2 "<<sub_sl->Length()<<endl; 

  int edgeno = 0;
  for(int i = 0;i < sub_sl->Size();i++){
    HalfSegment hs1;
    sub_sl->Get(i, hs1);
    if(!hs1.IsLeftDomPoint())continue;
    Point lp = hs1.GetLeftPoint();
    Point rp = hs1.GetRightPoint();
    Point newlp(true, lp.GetX() - bbox.MinD(0), lp.GetY() - bbox.MinD(1));
    Point newrp(true, rp.GetX() - bbox.MinD(0), rp.GetY() - bbox.MinD(1));
    HalfSegment hs2(true, newlp, newrp);
    hs2.attr.edgeno = edgeno++;
    *l += hs2;
    hs2.SetLeftDomPoint(!hs2.IsLeftDomPoint());
    *l += hs2;
  }

  delete sub_sl;
  route_tuple->DeleteIfAllowed();
}


/*
get the sub movement in a region

*/
void Space::GetLineInRegion(int oid, GenLoc gl1, GenLoc gl2, Line* l)
{
  Point p1(true, gl1.GetLoc().loc1, gl1.GetLoc().loc2);
  Point p2(true, gl2.GetLoc().loc1, gl2.GetLoc().loc2);
  HalfSegment hs(true, p1, p2);
  hs.attr.edgeno = 0;
  *l += hs;
  hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
  *l += hs;
}

/*
get the subment in free space 

*/
void Space::GetLineInFreeSpace(GenLoc gl1, GenLoc gl2, Line* l)
{
  Point p1(true, gl1.GetLoc().loc1, gl1.GetLoc().loc2);
  Point p2(true, gl2.GetLoc().loc1, gl2.GetLoc().loc2);
  
  if(AlmostEqual(p1, p2)) return; 

  HalfSegment hs(true, p1, p2);
  hs.attr.edgeno = 0;
  *l += hs;
  hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
  *l += hs;
}

/*
get the sub movement in bus network, bus routes 
can be optimized that for the same bus trip, the same bus route. it does not
have to access many times, only once is enough. oid corresponds to a bus route

*/
void Space::GetLineInBusNetwork(int& oid, Line* l, BusNetwork* bn, 
                                Interval<Instant> time_range)
{
  //////////////////////////////////////////////////////
  ///////////////change the oid to be bus route uid/////
  /////////////////////////////////////////////////////
  MPoint mp(0);
  int br_uoid = 0;
  bn->GetMOBUS(oid, mp, br_uoid);
//  cout<<mp.GetNoComponents()<<endl;
//  cout<<"br ref id "<<br_uoid<<endl; 
  oid = br_uoid;

  SimpleLine br_sl(0);
  bn->GetBusRouteGeoData(br_uoid, br_sl);
//  cout<<"bus route length "<<br_sl.Length()<<endl; 


  Periods* peri = new Periods(0);
  peri->StartBulkLoad();
  peri->MergeAdd(time_range);
  peri->EndBulkLoad();
  MPoint sub_mp(0);
  mp.AtPeriods(*peri, sub_mp);
  delete peri; 

//  cout<<sub_mp.GetNoComponents()<<endl; 

  Line l1(0);
  sub_mp.Trajectory(l1);
//  cout<<"line size "<<l1.Size()<<endl;
  if(l1.Size() > 0){
    Rectangle<2> bbox = br_sl.BoundingBox();
    int edgeno = 0;
    for(int i = 0;i < l1.Size();i++){
      HalfSegment hs1;
      l1.Get(i, hs1);
      if(!hs1.IsLeftDomPoint()) continue;
      Point lp = hs1.GetLeftPoint();
      Point rp = hs1.GetRightPoint();
      Point newlp(true, lp.GetX() - bbox.MinD(0), lp.GetY() - bbox.MinD(1));
      Point newrp(true, rp.GetX() - bbox.MinD(0), rp.GetY() - bbox.MinD(1));

      HalfSegment hs2(true, newlp, newrp);
      hs2.attr.edgeno = edgeno++;
      *l += hs2;
      hs2.SetLeftDomPoint(!hs2.IsLeftDomPoint());
      *l += hs2;
    }
  }

}

/*
get the sub movement in indoor environment
be careful with the movement inside an elevator or on a staircase 
the movement in an office room, corridor or bath room is clear 

*/
void Space::GetLineInGRoom(int oid, GenLoc gl1, GenLoc gl2, Line* l)
{
//  cout<<"indoor not implemented"<<endl;
  ////////////if the room is a staircase, then l is empty////////////////
  ////////       Loc loc_1(p1.GetZ(), -1);  //////////////////////////
  /////////////  Loc loc_2(p2.GetZ(), -1); ///////////////////////////
  ////////////////////////////////////////////////////////////////////
  if(gl1.GetLoc().loc2 < 0.0 && gl2.GetLoc().loc2 < 0.0){
//    cout<<"movement inside an elevator 2d line is empty"<<endl;

  }else{
//    cout<<"not inside an elevator "<<endl;

    Point p1(true, gl1.GetLoc().loc1, gl1.GetLoc().loc2);
    Point p2(true, gl2.GetLoc().loc1, gl2.GetLoc().loc2);

    if(AlmostEqual(p1, p2)) return; 

    HalfSegment hs(true, p1, p2);
    hs.attr.edgeno = 0;
    *l += hs;
    hs.SetLeftDomPoint(!hs.IsLeftDomPoint());
    *l += hs;

  }

}
