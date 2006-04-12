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

August 2005, M. Spiekermann. Initial version.

January - March 2006, M. Spiekermann. New constructors and functions added. Documentation revised.

*/

#ifndef SECONDO_NLIST_H
#define SECONDO_NLIST_H

#include <assert.h>
#include <sstream>

#include "NestedList.h"
#include "LogMsg.h"
#include "NList.h"


/*

1 Overview
   
The class ~NList~ can be used as replacement for the syntactical ~noisy~
interface defined in ~NestedList.h~. Currently, not all methods of the ~old~ 
interface are supported, but may be easily added when needed. 

This interface simplifes the check of a list structure, e.g.  member function
~isSymbol()~ will return TRUE, if ~n~ is a symbol atom. I think this will make
type mappings better readable since most Operations are done by
~list.function()~ instead of ~nl->Function(list)~. For example retrieving the
second element of the first element can be done by

----    old: nl->Second(nl->First(list))
        new: nlist.first().second()  
----

Conversion from/to type ~ListExpr~ which is used inside SECONDO whenever nested
lists are used is done by

---- #include "NList.h"
  
     ListExpr f(ListExpr list) 
     {
       Nlist nlist(list);
       ...
       return nlist.listExpr();
     }  
----

*/

extern NestedList* nl;

/*
2 String Constants
   
   
Type ~Symbols~ defines some string constants for symbols reserved as keywords in
the query processor or often used types. This should be used to avoid writing
always redundant string constants into the code.
   
*/   

struct Symbols {

    const string rel;
    const string map;
    const string tuple;
    const string ptuple;
    const string stream;
    const string typeerror;
    const string integer;
   
    Symbols() :
      rel("rel"),
      map("map"),
      tuple("tuple"),
      ptuple("ptuple"),
      stream("stream"),
      typeerror("typeerror"),
      integer("int")
    {} 
};  


/*
3 Exception Class ~NListErr~ 

A class representing nested list exceptions.
   
*/

class NListErr : public SecondoException {

  public:
  NListErr(const string& Msg) : SecondoException(Msg) {}
  NListErr(const Cardinal n, const Cardinal len, const string& ext="") : 
    SecondoException() 
  {
    stringstream s; 
    s << "Out of range exception in nested list! "
      << "Element " << n << " requested, but "
      << "list has length " << len << "." << ext;
    msgStr = s.str();
  }
 
  const string msg() { return msgStr; }
  
};


#define CHECK(n) {if (n>0 && length()<n) { \
        throw NListErr("Element "#n" not in list!"); } }

/*
4 Class ~NList~

This is a ~wrapper~ class for values of type ~ListExpr~. 

*/
        
class NList {


 public:

/*
2.2 Construction and Destruction
   
  * ~NList()~: Empty List.
  
  * NList(const NList\& rhs): Create a new reference to ~rhs~.
  
  * ~NList(const ListExpr list)~: Initialize with ~list~.
  
  * ~NList(const NList\& a, const NList\& b)~: Create a two elem list ~(a b)~.
  
  * ~NList(const NList\& a, const NList\& b, const NList\& c)~: Three elems ~(a b
c)~.
  
  * ~NList(const string\& s, const bool isStr = false)~: Create a symbol or string atom.
  
*/  
  
  NList() : 
    l(nl->Empty()), 
    e(nl->Empty()),
    len(0) 
  {}
  NList(const NList& rhs) : 
    l(rhs.l), 
    e(rhs.e),
    len(rhs.len) 
  {}
  NList(const ListExpr list) : 
    l(list), 
    e(nl->Empty()),
    len(nl->ListLength(list)) 
  {}
  NList(const NList& a, const NList& b) : 
    l( nl->TwoElemList(a.l, b.l) ),
    e( b.l ), 
    len(2)
  {}
  NList(const NList& a, const NList& b, const NList& c) : 
    l( nl->ThreeElemList(a.l, b.l, c.l) ),
    e( c.l ), 
    len(3)
  {}
  NList(const string& s, const bool isStr = false) :
    e( nl->Empty() ),
    len(0)
  {
   
    if (isStr)
      l = nl->StringAtom(s);
    else 
      l = nl->SymbolAtom(s);
  }
 
  NList(const int n) :
    l( nl->IntAtom(n) ),
    e( nl->Empty() ),
    len(0)
  {}

  NList(const double d) :
    l( nl->RealAtom(d) ),
    e( nl->Empty() ),
    len(0)
  {}
    
  inline NList& stringAtom(const string& val) 
  {
    len = 0;
    e = nl->Empty();
    l = nl->StringAtom(val);
    return *this;
  } 
  
  inline NList& textAtom(const string& val) 
  {
    len = 0;
    e = nl->Empty();
    l = nl->TextAtom(val);
    return *this;
  } 

  
  inline NList& symbolAtom(const string& val) 
  {
    len = 0;
    e = nl->Empty();
    l = nl->SymbolAtom(val);
    return *this;
  } 
  
  inline NList& enclose() 
  {
    len = 1;
    l = nl->OneElemList(l);
    e = l;
    return *this;
  } 

  ~NList() {}

/*
2.3 Structure Tests and Value Retrieval

The following functions can be used to get information about the
list stucture and to extract subexpressions or atom values.

*/  
  
  inline bool isEmpty() const { return nl->IsEmpty(l); }

  inline bool isEqual(const string& s) const { 
    return nl->IsEqual(l, s); 
  }
  inline bool isAtom() const { return nl->IsAtom(l); }
  
  inline bool isSymbol(const Cardinal n = 0) const { 
    CHECK(n) return isNodeType(n, SymbolType); 
  }
  inline bool isSymbol(const string& s) const { 
    return isNodeType(0, SymbolType) && (s == nl->SymbolValue(l)); 
  }
  inline bool isString(const Cardinal n = 0) const { 
    CHECK(n) return isNodeType(n, StringType); 
  }
  inline bool isText(const Cardinal n = 0) const { 
    CHECK(n) return isNodeType(n, TextType); 
  }
  inline bool isNoAtom(const Cardinal n = 0) const { 
    CHECK(n) return isNodeType(n, NoAtom); 
  }
  inline bool isList(const Cardinal n = 0) const { 
    CHECK(n) return isNodeType(n, NoAtom); 
  }
  inline bool isInt(const Cardinal n  = 0) const { 
    CHECK(n) return isNodeType(n, IntType); 
  }
  inline bool isReal(const Cardinal n = 0) const { 
    CHECK(n) return isNodeType(n, RealType); 
  }
  inline bool isBool(const Cardinal n = 0) const { 
    CHECK(n) return isNodeType(n, BoolType); 
  }
  
  inline bool hasLength(const Cardinal c) const { return (length() == c);  }

  inline NList first()  const { return elem(1); }
  inline NList second() const { return elem(2); }
  inline NList third()  const { return elem(3); }
  inline NList fourth() const { return elem(4); }
  inline NList fifth()  const { return elem(5); }
  inline NList sixth()  const { return elem(6); }

  inline Cardinal length() const
  {
    return len;
  }

  inline NList elem(Cardinal n) const
  {
    if ( (length() < n)   ) {
      throw NListErr(n,len);
    } 
    
    if ( (n == 1) && isAtom() ) {
      throw NListErr("Element 1 requested but list is an Atom!");
    }

    return NList(nl->Nth(n,l)); 
  }
  
  // for usage in type mappings 
  inline static ListExpr typeError(const string& msg)
  { 
    ErrorReporter::ReportError(msg); 
    return nl->TypeError(); 
  }

  // interchange with type ~ListExpr~ 
  inline ListExpr listExpr() const { return l; }
 
/*
Functions applicable for ~symbol/string/text~ atoms.

*/
  
  inline bool hasStringValue() const 
  { 
    return (isString() || isText() || isSymbol()); 
  }

  inline string str() const // retrieve a string value 
  {
    static string result = "";

    if( !hasStringValue() )
      throw NListErr("List has no string value exception!");

    if ( isSymbol() )
      return nl->SymbolValue(l);

    if ( isString() )
      return nl->StringValue(l);
    
    if ( isText() )
      return nl->Text2String(l);

    return result;
  } 

/*
Conversion to C++ strings or a string Atom. The latter conversion can be
used to convert a symbol or text atom into a string atom.

*/  
  inline string convertToString() const { return nl->ToString(l); } 
  inline NList toStringAtom() const { return NList( str(), true); } 
  
/*
Comparison between ~NList/NList~ and ~NList/string~ instances

*/  
  inline bool operator==(const NList& rhs) const 
  { 
    return nl->Equal(l,rhs.l);
  }
  
  inline bool operator==(const string& rhs) const
  {
    if( !hasStringValue() ) 
    {
      return false;
    }
    else
    {
      return ( str() == rhs );
    }
  }
 

/*
Construction of big lists

*/
  inline void makeHead(NList head) 
  {
    l = nl->OneElemList( head.l );
    e = l;
    len = 1;
  } 
  
  inline void append(NList tail)
  {
     e = nl->Append(e, tail.l);
     len++;
  }
           
 private:
  ListExpr l; 
  ListExpr e; // points to the last element of a list
  Cardinal len;
  inline bool isNodeType(const int n, const NodeType t) const
  {
    if ( !n ) 
      return ( /*nl->IsAtom(l) &&*/ (nl->AtomType(l) == t) );

    ListExpr list = nl->Nth(n,l);
    return ( /*nl->IsAtom(list) &&*/ (nl->AtomType(list) == t) ); 
  }
  
}; 

ostream& operator<<(ostream& os, const NList& n);

#endif
