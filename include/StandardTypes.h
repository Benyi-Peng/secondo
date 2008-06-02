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

//[ae] [\"a]
//[ue] [\"u]
//[oe] [\"o]

1 Header File: Standard Data Types

December 1998 Friedhelm Becker

2002-2003 U. Telle. Diploma thesis "reimplementation of SECONDO"

Nov. 2004. M. Spiekermann. Modifications in ~CcInt~. Using inline directives
and avoiding to dereference pointers in the ~Compare~ method improves performance.

April 2006. M. Spiekermann. A new struct StdTypes was added. It offers static
methods for retrieving integer or string arguments from a given Word value.
Moreover, counters for calls of ~Compare~ and ~HashValue~ are implemented for types
~CcInt~ and ~CcString~. 

May 2006. M. Spiekermann. The implementation of ~Compare~ for ~CcInt~ has been
changed.  Now first the case that both values are defined is handled and
equality is tested before unequality. This makes the number of integer
comparisons for recognizing $A > B$ or $A < B$ symmetric and in the average the
same since we need 1 for validating $A = B$ and 2 for $A {<,>} B$. Before it was
1 for $A > B$ and 2 for $A {<,=} B$.


1.1 Overview

This file defines four classes which represent the data types provided
by the ~StandardAlgebra~: 

---- 
     C++       |   SECONDO
     ======================
     CcInt     |   int
     CcReal    |   real
     CcBool    |   bool
     CcString  |   string
----

*/

#ifndef STANDARDTYPES_H
#define STANDARDTYPES_H

#include <string>
#include <sstream>
#include "StandardAttribute.h"
#include "NestedList.h"
#include "Counter.h"
#include "Symbols.h"

/*
2.1 CcInt

*/



class CcInt : public StandardAttribute
{
 public:
  
  inline CcInt()
  {
    intsCreated++; 
  }
 
  inline CcInt( bool d, int v = 0 )
  { 
    defined = d; intval = v;  
    intsCreated++; 
  }
 
  inline ~CcInt()
  {
    intsDeleted++;
  }

  inline void Initialize() 
  {
  }

  inline void Finalize() 
  {
    intsDeleted++;
  }

   
  inline bool IsDefined() const 
  { 
    return (defined); 
  }
  
  inline void SetDefined(bool defined) 
  { 
    this->defined = defined;
  }

  inline size_t Sizeof() const
  {
    return sizeof( *this );
  }
    
  inline int GetIntval() const
  { 
    return (intval); 
  }
  
  inline int GetValue() const
  { 
    return (intval); 
  }

  inline void Set( int v )
  { 
    defined = true, intval = v; 
  }
  
  inline void Set( bool d, int v )
  { 
    defined = d, intval = v; 
  }
  
  inline size_t HashValue() const
  { 
    static long& ctr = Counter::getRef(symbols::CTR_INT_HASH);
    ctr++;
    return (defined ? intval : 0); 
  }
  
  inline void CopyFrom(const StandardAttribute* right)
  {
    const CcInt* r = (const CcInt*)right;
    defined = r->defined;
    intval = r->intval;
  }
  
  inline int Compare(const Attribute* arg) const
  {
    const CcInt* rhs = dynamic_cast<const CcInt*>( arg );
    static long& ctr = Counter::getRef(symbols::CTR_INT_COMPARE);
    ctr++;

    return Attribute::GenericCompare<CcInt>(this, rhs, defined, rhs->defined);
  }

  inline virtual bool Equal(const CcInt* rhs) const
  {
    static long& ctr = Counter::getRef(symbols::CTR_INT_EQUAL);
    ctr++;

    return Attribute::GenericEqual<CcInt>(this, rhs, defined, rhs->defined);
  }

  inline virtual bool Less(const CcInt* rhs) const
  {
    static long& ctr = Counter::getRef(symbols::CTR_INT_LESS);
    ctr++;

    return Attribute::GenericLess<CcInt>(this, rhs, defined, rhs->defined);
  }

  
  inline bool Adjacent(const Attribute* arg) const
  {
    static long& ctr = Counter::getRef(symbols::CTR_INT_ADJACENT);
    ctr++;

    int a = GetIntval(),
        b = dynamic_cast<const CcInt*>(arg)->GetIntval();

    return( a == b || a == b + 1 || b == a + 1 );
  }
  
  inline CcInt* Clone() const
  { 
    return (new CcInt( this->defined, this->intval )); 
  }
  
  inline ostream& Print( ostream &os ) const { return (os << intval); }

  ListExpr CopyToList( ListExpr typeInfo )
  {
      cout << "CcInt CopyToList" << endl;
      NestedList *nl = SecondoSystem::GetNestedList();
      AlgebraManager* algMgr = SecondoSystem::GetAlgebraManager();
      int algId = nl->IntValue( nl->First( nl->First( typeInfo ) ) ),
          typeId = nl->IntValue( nl->Second( nl->First( typeInfo ) ) );

      return (algMgr->OutObj(algId, typeId))( typeInfo, SetWord(this) );
  }

  Word CreateFromList( const ListExpr typeInfo, const ListExpr instance,
                       const int errorPos, ListExpr& errorInfo, bool& correct )
  {
      cout << "CcInt CreateFromList" << endl;
      NestedList *nl = SecondoSystem::GetNestedList();
      AlgebraManager* algMgr = SecondoSystem::GetAlgebraManager();
      int algId = nl->IntValue( nl->First( nl->First( typeInfo ) ) ),
          typeId = nl->IntValue( nl->Second( nl->First( typeInfo ) ) );

      Word result = (algMgr->InObj(algId, typeId))( typeInfo, 
                         instance, errorPos, errorInfo, correct );
      if( correct )
        return result;
      return SetWord( Address(0) );
  }

  inline bool operator==(const CcInt& rhs) const
  {
    return intval == rhs.intval;
  }

  inline bool operator<(const CcInt& rhs) const
  {
    return intval < rhs.intval;
  } 

  inline void operator=(const CcInt& rhs)
  {
    intval = rhs.intval;
    defined = rhs.defined;
  }

  virtual string getCsvStr() const{
    if(!defined){
       return "-";
    } else {
       stringstream o;
       o << intval;
       return o.str();
    }
  }
 

  virtual bool hasDB3Representation() const {return true;}
  virtual unsigned char getDB3Type() const { return 'N'; }
  virtual unsigned char getDB3Length() const { return 15; }
  virtual unsigned char getDB3DecimalCount(){ return 0; }
  virtual string getDB3String() const {
      if(!defined){
        return "";
      } 
      stringstream s;
      s << intval;
      return s.str();
  } 
  


 
  static long intsCreated;
  static long intsDeleted;

 private:
  bool defined;
  int  intval;
};

/*
3.1 CcReal

*/


#define SEC_STD_REAL double
class CcReal : public StandardAttribute
{
 public:
  inline CcReal()
  { 
     realsCreated++; 
  }

  inline CcReal( bool d, SEC_STD_REAL v = 0.0 ) 
  { 
    defined = d; 
    realval = v; 
    realsCreated++; 
  }

  inline ~CcReal() 
  { 
    realsDeleted++; 
  }

  inline void Initialize() 
  {}

  inline void Finalize() 
  {
    realsDeleted++; 
  }

  inline bool IsDefined() const 
  { 
    return defined; 
  }

  inline void SetDefined(bool defined) 
  { 
    this->defined = defined; 
  }

  inline size_t Sizeof() const
  {
    return sizeof( *this );
  }

  inline SEC_STD_REAL GetRealval() const
  { 
    return realval;
  }
  
  inline SEC_STD_REAL GetValue() const
  { 
    return realval;
  }

  
  inline void Set( SEC_STD_REAL v ) 
  { 
    defined = true, 
    realval = v; 
  }

  inline void Set( bool d, SEC_STD_REAL v ) 
  { 
    defined = d;
    realval = v; 
  }

  inline CcReal* Clone() const
  { 
    return (new CcReal(this->defined, this->realval)); 
  }

  inline size_t HashValue() const
  {
    if(!defined)
      return 0;

    unsigned long h = 0;
    char* s = (char*)&realval;
    for(unsigned int i = 1; i <= sizeof(SEC_STD_REAL) / sizeof(char); i++)
    {
      h = 5 * h + *s;
      s++;
    }
    return size_t(h);
  }

  inline void CopyFrom(const StandardAttribute* right)
  {
    const CcReal* r = (const CcReal*)right;
    defined = r->defined;
    realval = r->realval;
  }

  inline int Compare( const Attribute* arg ) const
  {
    const CcReal* rhs = static_cast<const CcReal*>( arg );
    static long& ctr = Counter::getRef("CcReal::Compare");
    ctr++;
    return Attribute::GenericCompare<CcReal>(this, rhs, defined, rhs->defined); 
  }

  inline int CompareAlmost( const Attribute* arg ) const
  {
    const CcReal* rhs = static_cast<const CcReal*>( arg );
    static long& ctr = Counter::getRef("CcReal::Compare");
    ctr++;

    double diff = fabs( GetRealval() - rhs->GetRealval() );
    if (diff < FACTOR )
      return 0;
    else
      return 
        Attribute::GenericCompare<CcReal>(this, rhs, defined, rhs->defined);
  }

  inline bool Adjacent( const Attribute *arg ) const
  {
    // original implementation:
    //    return( realval == ((const CcReal *)arg)->realval );
    // '==' changed to 'CompareAlmost() == 0' to avoid
    // problems with 64bit environments:
    return( CompareAlmost(arg) == 0 );
  }

  inline ostream& Print( ostream &os ) const { return (os << realval); }

  static long realsCreated;
  static long realsDeleted;

  inline bool operator==(const CcReal& rhs) const
  {
    return realval == rhs.realval;
  }   
  inline bool operator<(const CcReal& rhs) const
  {
    return realval < rhs.realval;
  } 

  virtual string getCsvStr() const{
    if(!defined){
       return "-";
    } else {
       stringstream o;
       o << realval;
       return o.str();
    }
  }
  

  virtual bool hasDB3Representation() const {return true;}
  virtual unsigned char getDB3Type() const { return 'N'; }
  virtual unsigned char getDB3Length() const { return 15; }
  virtual unsigned char getDB3DecimalCount(){ return 6; }
  virtual string getDB3String() const {
      if(!defined){
        return "";
      } 
      ostringstream s;
      s.setf(ios::fixed);
      s.setf(ios::showpoint);
      s.setf(ios::left);
      s.width(15);
      s.precision(6);
      s << realval;
      s.flush();
      return s.str();
  } 

 private:
  bool  defined;
  SEC_STD_REAL  realval;
};



/*
4.1 CcBool

*/

class CcBool : public StandardAttribute
{
 public:
  inline CcBool()
  { 
    boolsCreated++; 
  }

  inline CcBool( bool d, int v = false )
  { 
    defined  = d; 
    boolval = v; 
    boolsCreated++; 
  }

  inline ~CcBool() 
  { 
    boolsDeleted++; 
  }
 
  inline void Initialize() 
  {}

  inline void Finalize() 
  { 
    boolsDeleted++; 
  }

  inline void Set( bool d, bool v )
  { 
    defined = d;
    boolval = v; 
  }

  inline bool IsDefined() const 
  { 
    return defined; 
  }

  inline void SetDefined(bool defined) 
  { 
    this->defined = defined; 
  }

  inline size_t Sizeof() const
  {
    return sizeof( *this );
  }

  inline bool GetBoolval() const
  { 
    return boolval; 
  }
  
  inline bool GetValue() const
  { 
    return boolval; 
  }
  
  inline CcBool* Clone() const
  { 
    return new CcBool(this->defined, this->boolval); 
  }

  inline size_t HashValue() const
  { 
    return (defined ? boolval : false); 
  }

  inline void CopyFrom(const StandardAttribute* right)
  {
    const CcBool* r = (const CcBool*)right;
    defined = r->defined;
    boolval = r->boolval;
  }

  inline int Compare( const Attribute* arg ) const
  {
    const CcBool* rhs = static_cast<const CcBool*>( arg );
    return Attribute::GenericCompare<CcBool>(this, rhs, defined, rhs->defined); 
  }

  inline bool Adjacent( const Attribute* arg ) const
  {
    return 1;
  }

  inline ostream& Print( ostream &os ) const
  {
    if (boolval == true) return (os << "TRUE");
    else return (os << "FALSE");
  }

  inline bool operator==(const CcBool& rhs) const
  {
    return boolval == rhs.boolval;
  }   
  
  inline bool operator<(const CcBool& rhs) const
  {
    return boolval < rhs.boolval;
  } 

  
  static long boolsCreated;
  static long boolsDeleted;

  virtual string getCsvStr() const{
    if(!defined){
       return "-";
    } else {
      return boolval?"true":"false";
    }
  }

  virtual bool hasDB3Representation() const {return true;}
  virtual unsigned char getDB3Type() const { return 'L'; }
  virtual unsigned char getDB3Length() const { return 1; }
  virtual unsigned char getDB3DecimalCount(){ return 0; }
  virtual string getDB3String() const {
      if(!defined){
        return "?";
      } 
      return boolval?"T":"F";
  } 



 private:
  bool defined;
  bool boolval;
};

/*
5.1 CcString

*/

typedef char STRING_T[MAX_STRINGSIZE+1];

class CcString : public StandardAttribute
{
 public:
  inline CcString() 
  { 
    stringsCreated++; 
  }

  inline CcString( bool d, const STRING_T* v ) 
  { 
    defined = d; 
    strcpy( stringval, *v); 
    stringsCreated++; 
  }

  inline CcString( const bool d, const string v )
  {
    defined = d;
    memset ( stringval,'\0',      MAX_STRINGSIZE+1);
    strncpy( stringval, v.data(), MAX_STRINGSIZE  );
    stringsCreated++;
  }

  inline ~CcString() 
  { 
    stringsDeleted++; 
  }

  inline void Initialize() 
  {} 

  inline void Finalize() 
  { 
    stringsDeleted++; 
  }

  inline bool IsDefined() const 
  { 
    return defined; 
  }

  inline void SetDefined(bool defined) 
  { 
    this->defined = defined; 
  }

  inline size_t Sizeof() const
  {
    return sizeof( *this );
  }

  inline const STRING_T* GetStringval() const
  { 
    return &stringval; 
  }

  inline const string GetValue() const
  { 
    return stringval; 
  }

  inline CcString* Clone() const
  { 
    return (new CcString( this->defined, &this->stringval )); 
  }

  inline void Set( bool d, const STRING_T* v ) 
  { 
    defined = d;
    strcpy( stringval, *v);
  }

  inline void Set( const bool d, const string v ) 
  { 
    defined = d;
    memset ( stringval, '\0',     MAX_STRINGSIZE+1);
    strncpy( stringval, v.data(), MAX_STRINGSIZE  );
  }

  inline size_t HashValue() const
  {
    static long& ctr = Counter::getRef("CcString::HashValue");
    ctr++;
    if(!defined)
      return 0;

    unsigned long h = 0;
    const char* s = stringval;
    while(*s != 0)
    {
      h = 5 * h + *s;
      s++;
    }
    return size_t(h);
  }

  inline void CopyFrom(const StandardAttribute* right)
  {
    const CcString* r = (const CcString*)right;
    defined = r->defined;
    strcpy(stringval, r->stringval);
  }

  inline int Compare( const Attribute* arg ) const
  {
    const CcString* rhs = static_cast<const CcString*>( arg );
    static long& ctr = Counter::getRef("CcString::Compare");
    ctr++;

    if (defined && rhs->defined)
    { 
      const int cmp = strcmp(stringval, rhs->stringval);
      if (cmp == 0)
         return 0;
      else
         return (cmp < 0) ? -1 : 1;
    }
    else
    {     
      // compare only the defined flags
      if( !defined ) {
        if ( !rhs->defined )  // case 00
          return 0;         
        else          // case 01
          return -1;
      }
      return 1;       // case 10  
    }  
  }

  bool Adjacent( const Attribute* arg ) const;

  inline ostream& Print( ostream &os ) const { 
    return (os << "\"" << stringval << "\""); 
  }

  static long stringsCreated;
  static long stringsDeleted;

  virtual string getCsvStr() const{
    if(!defined){
       return "-";
    } else {
      return stringval;
    }
  }

  virtual bool hasDB3Representation() const {return true;}
  virtual unsigned char getDB3Type() const { return 'C'; }
  virtual unsigned char getDB3Length() const { return MAX_STRINGSIZE; }
  virtual unsigned char getDB3DecimalCount(){ return 0; }
  virtual string getDB3String() const {
      if(!defined){
        return "";
      } 
      return string(stringval);
  }
 
 private:
  bool   defined;
  STRING_T stringval;
};

void ShowStandardTypesStatistics( const bool reset );

/*
6 Some Functions Prototypes

*/
Word InCcBool( ListExpr typeInfo, ListExpr value, 
               int errorPos, ListExpr& errorInfo, bool& correct );
ListExpr OutCcBool( ListExpr typeinfo, Word value );
Word InCcInt( ListExpr typeInfo, ListExpr value, 
              int errorPos, ListExpr& errorInfo, bool& correct );
ListExpr OutCcInt( ListExpr typeinfo, Word value );
Word InCcReal( ListExpr typeInfo, ListExpr value, 
               int errorPos, ListExpr& errorInfo, bool& correct );
ListExpr OutCcReal( ListExpr typeinfo, Word value );
Word InCcString( ListExpr typeInfo, ListExpr value,
                 int errorPos, ListExpr& errorInfo, bool& correct );
ListExpr OutCcString( ListExpr typeinfo, Word value );

/*
Functions which convert a ~Word~ value (argument of an operator)
into the corresponding C++ type
   
*/

struct StdTypes
{
  static int GetInt(const Word& w); 
  static SEC_STD_REAL GetReal(const Word& w); 
  static bool GetBool(const Word& w); 
  static string GetString(const Word& w); 
  
  static void InitCounters(bool show);
  static void SetCounterValues(bool show);
  static long UpdateBasicOps(bool reset=false);
  static void ResetBasicOps();
};

#endif

