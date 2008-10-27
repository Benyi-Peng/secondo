/*
----
This file is part of SECONDO.

Copyright (C) 2004-2007, University in Hagen, Faculty of Mathematics and
Computer Science, Database Systems for New Applications.

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

1 Header File: Attribute

May 1998 Stefan Dieker

April 2002 Ulrich Telle Adjustments for the new Secondo version

Oct 2004 M. Spiekermann. Adding some more detailed documentation and some
thoughts about redesign and performance.

January 2006, M. Spiekermann. Some template functions which could be used as default
for some type constructor functions were moved to ConstructorTemplates.h

May 2006, M. Spiekermann. Documentation for the ~Compare~ function extended. Template
functions ~GenericCompare~ and ~GetValue~ added.

1.1 Overview

Classes implementing attribute data types have to be subtypes of class
attribute. Whatever the shape of such derived attribute classes might be,
their instances can be aggregated and made persistent via instances of class
~Tuple~, while the user is (almost) not aware of the additional management
actions arising from persistence.

1.1 Class "Attribute"[1]

The class ~Attribute~ defines several pure virtual methods which every
derived attribute class must implement.

*/
#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include "SecondoSystem.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "Counter.h"
//#include "AlgebraManager.h"
#include "FLOB.h"
#include "WinUnix.h"
#include "SecondoSMI.h"


extern const double FACTOR;
bool AlmostEqual( const double d1, const double d2 );
/*
3.5 Struct ~AttrDelete~

This structure defines the way attributes are deleted. First, ~refs~
maintain a reference counter of that attribute. Many tuples can
point to the same attribute to avoid unnecessary copying. Every
tuple that points to an attribute increases this value and every
time it tries to delete the attribute, it decreses this value.
The attribute will only be delete when ~refs~ = 0.

~isDelete~ indicates which function must be called to delete the
attribute. If it is ~false~, then the attribute was created
with ~malloc~ and must be deleted with ~free~. Otherwise, if it
is ~true~, the attribute was created with ~new~ and must
be deleted with ~delete~. The default is ~DeleteAttr~.

*/
struct AttrDelete
{
  AttrDelete():
  refs( 1 ), isDelete( true )
  // Note: variable isDefined belongs to the state of
  // Attribute objects thus the default constructor should
  // not change the value here since the default persistency
  // mechanism does simply copy byte blocks of the object's
  // current state!
  {}

  uint16_t refs;
  bool isDelete;
  bool isDefined;
};

/*
4 Class ~Attribute~

*/
// forward declaration
class Tuple;

class Attribute
{
  // allow a Tuple to manipulate the reference counter
  // directly
  friend class Tuple;
  public:

    inline Attribute()
    {}
/*
The simple constructor.

*/
    inline virtual ~Attribute()
    {}
/*
The virtual destructor.

*/
    virtual bool IsDefined() const { return del.isDefined; }
/*
Returns whether the attribute is defined.

*/
    virtual void SetDefined(bool defined) { del.isDefined = defined; }
/*
Sets the ~defined~ flag of the attribute.

*/

    virtual size_t Sizeof() const = 0;
/*
Returns the ~sizeof~ of the attribute class.

*/


    virtual int Compare( const Attribute *rhs ) const = 0;

/*
This function should define an order on the attribute values.
The implementation must also consider that values may be undefined.
Hence there are four cases of defined/undefined combinations which are
below referred as 11, 00, 01, 10.

Case 11 (both values are defined) is the ~normal~ comparison
of attribute values

----
    -1: *this < *rhs
     0: *this = *rhs
     1: *this > *rhs
----

The semantics for the other cases are defined below:

----
    01 -> -1: *this < *rhs
    00 ->  0: *this = *rhs
    10 ->  1: *this > *rhs
----

Thus the result of a comparison of attribute values is never undefined!

*/
    virtual int CompareAlmost( const Attribute *rhs ) const
    {
      static long& ctr = Counter::getRef("ATTR::CompareAlmost");
      ctr++;
      return Compare(rhs);
    }

/*
This function is for unprecise datatypes like ~real~ or ~point~, where one
needs to distinguish the ordinary ~Compare~, which will be used for sorting
and precise comparison/lookup in DBArrays, and the fuzzy version, which employs
~AlmostEqual~ instead of ~Equal~. It will be used to remove duplicates, gouping
etc.

For unprecise datatypes, you should always overwrite ~CompareAlmost~.

*/

/*
Below a generic compare function is implemented by means of templates.
In order to use this implement the functions

----
    inline operator==(const T& rhs) const
    inline operator<(const T& rhs) const
----

in your class and instantiate it inside your ~Compare~ function implementation.
For examples refer to the ~StandardAlgebra~.

*/

    template<class T>
    static inline int GenericCompare( const T* left,
                                      const T* right,
                                      const bool lDef,
                                      const bool rDef    )
    {
      static long& ctr = Counter::getRef("ATTR::GenericCompare");
      ctr++;
      if ( lDef &&  rDef) // case 11: value comparison
      {
        if (  *left == *right  )
          return 0;
        else
          return ( *right < *left ) ? 1 : -1;
      }
      // compare only the defined flags
      if( !lDef ) {
        if ( !rDef )  // case 00
          return 0;
        else          // case 01
          return -1;
      }
      return 1;       // case 10
    }

/*
In some cases it makes sense to offer more specialized comparisons since
some algorithms like sorting or duplicate removal need only $<$ or $=$.

If it helps to increase performance one could think about to implement the
virtual ~Equal~ or ~Less~ functions in the derived classes.

*/


    inline virtual bool Equal(const Attribute* rhs) const
    {
      static long& ctr = Counter::getRef("ATTR::Equal");
      ctr++;
      return Compare(rhs) == 0;
    }

    template<class T>
    static inline bool GenericEqual( const T* left,
                                     const T* right,
                                     const bool lDef,
                                     const bool rDef    )
    {
      static long& ctr = Counter::getRef("ATTR::GenericEqual");
      ctr++;
      if (  *left == *right  )
        return true;
      else
        return lDef == rDef;
    }

    inline virtual bool Less(const Attribute* rhs) const
    {
      static long& ctr = Counter::getRef("ATTR::Less");
      ctr++;
      return Compare(rhs) < 0;
    }

    inline virtual bool LessAlmost(const Attribute* rhs) const
    {
      static long& ctr = Counter::getRef("ATTR::LessAlmost");
      ctr++;
      return CompareAlmost(rhs) < 0;
    }

    template<class T>
    static inline bool GenericLess( const T* left,
                                    const T* right,
                                    const bool lDef,
                                    const bool rDef    )
    {
      static long& ctr = Counter::getRef("ATTR::GenericLess");
      ctr++;
      if (  *left < *right  )
        return true;
      else
        return (rDef && !lDef);
    }

    virtual bool Adjacent( const Attribute *attrib ) const = 0;
/*
This function checks if two attributes are adjacent. As an example,
1 and 2 are adjacent for integer attributes and "Victor" and "Victos"
are adjacent for string attributes.

*/
    inline virtual int NumOfFLOBs() const
    {
      return 0;
    }
/*
Returns the number of FLOBs the attribute contains. The default
value of this funcion is 0, which means that if an attribute
does not contain FLOBs, it is not necessary to implement this
function.

*/
    inline virtual FLOB* GetFLOB( const int i )
    {
      assert( false );
      return 0;
    }
/*
Returns a reference to a FLOB given an index ~i~. If the attribute
does not contain any FLOBs (the default), this function should not
be called.

*/
    inline virtual
    void Restrict( const vector< pair<int, int> >& interval )
    {}
/*
This function is called to restrict a current attribute to a
set of intervals. This function is used in double indexing.

*/
    inline virtual void Initialize() {}
    inline virtual void Finalize() {}
/*
These two functions are used to initialize and finalize values of
attributes. In some cases, the constructor and destructor are
not called, e.g. when deleting an attribute with delete type
~FreeAttr~. In these cases, these two functions are called
anyway. An example of their usage is in the JNI algebras, where
some Java initialization and destructions are done in these
functions.

*/
    inline virtual ostream& Print( ostream& os ) const
    {
      return os << "< No Print()-function for this datatype! >";
    }

/*
Prints the attribute. Used for debugging purposes.

*/

    inline virtual void
    Serialize(char* storage, size_t sz, size_t offset) const
    {
       memcpy( &storage[offset], (void*)this, sz );
    }

/*
Writes an attribute as byte sequence into ~storage~. The default
implementation simply writes the byte block of the current instance.

*/

    inline virtual size_t SerializedSize() const
    {
      return 0;
    }
/*
The default implementation returns 0 here. This indicates that the default
mechanism for making objects persistent is used and the data type's ~SizeOfObj~
function will be used to determine the block size to be stored on disk.

A datatype which support serialization must implement this function in order
to overwrite the default.

*/


    inline virtual void Rebuild(char* state, size_t sz)
    {
      memcpy(this, state, sz);
    }

    inline virtual void Rebuild(char* state, size_t sz, ObjectCast cast)
    {
      Rebuild(state,sz);
      cast(this);
    }


/*
Rebuild a stored object state given as byte sequence by ~state~. The default
implementation excpects the state contains the byte block which was produced by
the default ~Serialize~ function. The size of the object is  ~sz~

*/

   inline static Attribute*
   Create(Attribute* attr, char* state, size_t sz, int algId, int typeId)
   {

      // call the spezialized Rebuild function, if not implemented, the default
      // above will be called.
      attr->Rebuild(state, sz ,am->Cast(algId, typeId));

      // Remark: the returned pointer has been created using the new operator!
      return attr;
   }

/*
The function above creates a new instance of the C++-class which represents
a secondo object for the given algId and typeId. By default the stored
state of an objects is defined by its byte block maintained from the C++ runtime
environment.

Subclasses of an ~Attribute~ may overwrite the ~Serialize~ and ~Rebuild~
function in order to provide a more compact object state storage representation.

*/


    static void Save( SmiRecord& valueRecord, size_t& offset,
                      const ListExpr typeInfo, Attribute *elem );
/*
Default save function.

*/
    static Attribute *Open( SmiRecord& valueRecord,
                            size_t& offset, const ListExpr typeInfo );
/*
Default open function.

*/

    bool DeleteIfAllowed();
/*
Decreases the Attribute's reference counter ~refs~. If ~refs~ becomes 0,
it deletes the attribute.

*/
    Attribute* Copy();
/*
Returns a new reference to this attribute, if possible (~refs~ < 255),
otherwise it returns a reference to a fresh clone (having ~refs~ = 1).

*/
    virtual Attribute *Clone() const = 0;
/*
Clones this attribute. This function is implemented in the son classes.
~refs~ of the new clone should be set to 1! A good way to ensure this, is
to initialize ~refs~ with 1 within all constructors
(except from the standard constructor).

*/
    inline void SetFreeAttr()
    {
      del.isDelete = false;
    }
/*
Sets the delete type.

The template function below offers a generic interface for
Interaction with the query processor. This makes it easier to
retrieve parameter values inside an operators value mapping.
Examples of its usage can be found in the ~StandardAlgebra~.

In order to be able to instantiate this template you need to
implement a member function

---- S T::GetValue()
----

However, this makes only sense for types which have a simple internal
value like int, float, etc.

*/

    inline int NoRefs() const{
        return del.refs;
    }
/*
Returns the number of references for this attribute.

*/


    template<class S, class T>
    static T GetValue(Word w)
    {
      S* ptr = static_cast<S*>(w.addr);
      return ptr->GetValue();
    }

    string AttrDelete2string();
/*
Print the delete reference info to a string (for debugging)

*/
  static void InitCounters(bool show);
  static void SetCounterValues(bool show);


  virtual string getCsvStr() const{
    return "";
  }
/*
 returns a string representation for csv export

*/

  virtual bool hasBox() const { return false; }

  virtual void writeShape(ostream& o, uint32_t RecNo) const{
    // first, write the record header
    WinUnix::writeBigEndian(o,RecNo);
    uint32_t length = 2;
    WinUnix::writeBigEndian(o,length);
    uint32_t type = 0;
    WinUnix::writeLittleEndian(o,type);
  }

  virtual double getMinX() const{return 0;}
  virtual double getMaxX() const{return 0;}
  virtual double getMinY() const{return 0;}
  virtual double getMaxY() const{return 0;}
  virtual double getMinZ() const { return 0.0; }
  virtual double getMaxZ() const{ return 0.0; }
  virtual double getMinM() const { return 0.0; }
  virtual double getMaxM() const{ return 0.0; }
  virtual uint32_t getshpType() const{ return 0; }

  virtual bool hasDB3Representation() const {return false;}
  virtual unsigned char getDB3Type() const { return 'L'; }
  virtual unsigned char getDB3Length() const { return 1; }
  virtual unsigned char getDB3DecimalCount(){ return 0; }
  virtual string getDB3String() const { return "?"; }


   virtual void ReadFromString(string value){
       SetDefined(false);
   }

  enum StorageType { Default, Core, Extension };


  inline virtual StorageType GetStorageType() const { return Default; }

  protected:

    AttrDelete del;
/*
Stores the way this attribute is deleted.

*/

  private:
     inline void InitRefs(){
          del.refs=1;
     }
/*
Set the reference counter to 1

*/



/*
Counters for basic operations. Useful for verifying cost formulas and determining
cost factors.

*/
    static void counters(bool reset, bool show);

};

/*
~Auxilary function to compare Double values for equality~

*/
inline bool AlmostEqual( const double d1, const double d2 )
{
  double diff = fabs(d1-d2);
  return ( diff < FACTOR );
//   double i1, i2;
//   double dd1 = modf( d1, &i1 ),
//          dd2 = modf( d2, &i2 );
//   long ii1 = (long)i1,
//        ii2 = (long)i2;
//
//   if( abs(ii1 - ii2) > 1 )
//     return false;

//   int d = abs(ii1) - abs(ii2);
//   return fabs(dd1 - dd2 - d) < FACTOR;
}

/*
Generic ~Open~-function

*/

template<class T>
bool OpenAttribute(  SmiRecord& valueRecord,
                     size_t& offset,
                     const ListExpr typeInfo,
                     Word& value )
{
  T *bf = static_cast<T*>(Attribute::Open( valueRecord, offset, typeInfo ));
  value = SetWord( bf );
  return true;
}

/*
Generic ~Save~-function

*/
template<class T>
bool SaveAttribute( SmiRecord& valueRecord,
                    size_t& offset,
                    const ListExpr typeInfo,
                    Word& value )
{
  T *bf = static_cast<T*>(value.addr);
  Attribute::Save( valueRecord, offset, typeInfo, bf );
  return true;
}


/*
The generalized output operator

*/
ostream& operator<<(ostream& os, const Attribute& attr);

#endif

