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

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[TOC] [\tableofcontents]

[1] Header file  of the Relational Algebra

March 2003 Victor Almeida created the new Relational Algebra 
organization

Oct 2004 M. Spiekermann changed some frequently called ~small~ 
functions into inline functions implemented in the header file. This
reduced code redundance since the same code written in 
RelationMainMemory and RelationPersistent can be kept together here 
and may improve performance when the code is compiled with 
optimization flags.  

June-July 2004 M. Spiekermann. Changes in class ~Tuple~ and 
~TupleBuffer~. Storing the attribute array as member variable in 
class Tuple reduces processing overhead. Moreover the array is 
needed for both implementations "persistent" and "memory" hence it 
should not be maintained in class ~privateTuple~. The TupleBuffer 
was extended. The TupleBuffer constructor was extended by a new 
boolean parameter which indicates whether the tuples in it are 
stored as "free" or "non-free" tuples.

January 2006 Victor Almeida replaced the ~free~ tuples concept to
reference counters. There are reference counters on tuples and also
on attributes. Some assertions were removed, since the code is
stable.

May 2006 M. Spiekermann. Changes in ~TupleCompareBy~. The number of compare 
operations was antisymmetric, e.g. recognizing that two attributes fulfill
$A > B$ needed one integer comparison more than $A < B$. Now first $A = B$ is 
tested and determining one of the remaining cases needs only one extra 
comparison. Hence in the average we will use less ${=,<}$ operations than 
before.

April 2007, T. Behr, M. Spiekermann. Removal of the main memory implementation
of this algebmain memory implementation
of this algebra module.

September 2007, M. Spiekermann. Dependencies to algebra OldRelationAlgebra removed.

[TOC]


1 Overview

The Relational Algebra basically implements two type constructors, 
namely ~tuple~ and ~rel~. The type system of the Relational Algebra 
can be seen below.

\begin{displaymath}
\begin{array}{lll}
& 
\to \textrm{DATA} & 
{\underline{\smash{\mathit{int}}}}, 
{\underline{\smash{\mathit{real}}}},
{\underline{\smash{\mathit{bool}}}}, 
{\underline{\smash{\mathit{string}}}} 
\\
({\underline{\smash{\mathit{ident}}}} \times \textrm{DATA})^{+} & 
\to \textrm{TUPLE} &
{\underline{\smash{\mathit{tuple}}}} 
\\
\textrm{TUPLE} & 
\to \textrm{REL} & 
{\underline{\smash{\mathit{rel}}}}
\end{array}
\end{displaymath}

The DATA kind should be incremented with more complex data types 
such as, for example, ${\underline{\smash{\mathit{point}}}}$, 
${\underline{\smash{\mathit{points}}}}$, 
${\underline{\smash{\mathit{line}}}}$, 
and ${\underline{\smash{\mathit{region}}}}$ of the Spatial Algebra,
meaning that these data types can be inserted into relations.

As an example, a relation ~cities~ could be described as

\begin{displaymath}
{\underline{\smash{\mathit{rel}}}}
  ({\underline{\smash{\mathit{tuple}}}}
    (<
      (\textrm{name}, {\underline{\smash{\mathit{string}}}}),
      (\textrm{country}, {\underline{\smash{\mathit{string}}}}),
      (\textrm{pop}, {\underline{\smash{\mathit{int}}}}),
      (\textrm{pos}, {\underline{\smash{\mathit{point}}}})
    >)
  )
\end{displaymath}

This file will contain an interface of the memory representation 
structures (~classes~) for these two type constructors, namely 
~Tuple~ and ~Relation~, and some additional ones that are needed
for the Relational Algebra, such as ~TupleId~, ~RelationIterator~, 
~TupleType~, ~Attribute~ (which is defined inside the file 
Attribute.h), ~AttributeType~, and ~RelationDescriptor~.


                Figure 1: Relational Algebra implementation 
                architecture. [RelationAlgebraArchitecture.eps]

2 Defines, includes, and constants

*/
#ifndef _RELATION_ALGEBRA_H_
#define _RELATION_ALGEBRA_H_

#include <iostream>
#include <map>

#include "Algebra.h"
#include "StandardAttribute.h"
#include "NestedList.h"
#include "Counter.h"

#define MAX_NUM_OF_ATTR 10 


#undef TTRACE
#define TTRACE(ptr, msg) { \
	    cerr << (void*)this \
	         << " " << __FUNCTION__ << ": " \
	         << msg << endl; }

#undef DEBUG
#define DEBUG(ptr, msg) if (debug) TTRACE(ptr, msg)

extern AlgebraManager* am;

/*
3 Type Constructor ~tuple~

3.1 ~TupleId~

This class implements the unique identification for tuples inside a 
relation.

*/
typedef long TupleId;

/*
3.2 Class ~Attribute~

This abstract class ~Attribute~ is inside the file Attribute.h and 
contains a set of functions necessary to the management of 
attributes. All type constructors of the kind DATA must be a 
sub-class of ~Attribute~.

3.3 Struct ~AttributeType~

This ~AttributeType~ struct implements the type of each attribute 
inside a tuple. To identify a data type in the Secondo system the 
~algebraId~ and the ~typeId~ are necessary. The size of the 
attribute is also necessary to previously know how much space will 
be necessary to store an instance of such attribute's data type.

*/
struct AttributeType
{
  AttributeType()
    {}
/*
This constructor should not be used.

*/
  AttributeType( int algId, int typeId, int size ):
    algId( algId ),
    typeId( typeId ),
    size( size )
    {}
/*
The constructor.

*/
  int algId;
/*
The data type's algebra ~id~ of the attribute.

*/
  int typeId;
/*
The data type's ~id~ of the attribute.

*/
  int size;
/*
Size of attribute instance in bytes.

*/
};

/*
3.4 Class ~TupleType~

A ~TupleType~ is a collection (an array) of all attribute types 
(~AttributeType~) of the tuple. This structure contains the metadata of a tuple attributes.

*/
class TupleType
{
  public:
    TupleType( const ListExpr typeInfo );
/*
The first constructor. Creates a tuple type from a ~typeInfo~ list 
expression. It sets all member variables, including the total size.

*/

    ~TupleType()
    {
      delete []attrTypeArray;
    }
/*
The destructor.

*/
    inline bool DeleteIfAllowed() 
    {
      assert( refs > 0 );
      refs--;
      if( refs == 0 ){
        delete this;
        return true;
      } else {
         return false;
      }
    }
/*
Deletes the tuple type if allowed, i.e. there are no more 
references to it.

*/
    inline void IncReference()
    { 
      refs++;
    }
/*
Increment the reference count of this tuple type.

*/
    inline int GetNoAttributes() const 
    { 
      return noAttributes; 
    }
/*
Returns the number of attributes of the tuple type.

*/
    inline int GetTotalSize() const 
    {
      return totalSize; 
    }
/*
Returns the total size of the tuple.

*/
    inline const AttributeType& GetAttributeType( int index ) const
    {
      return attrTypeArray[index];
    }
/*
Returns the attribute type at ~index~ position.

*/
    inline void PutAttributeType( int index, 
                                  const AttributeType& attrType )
    {
      attrTypeArray[index] = attrType;
    }
/*
Puts the attribute type ~attrType~ in the position ~index~.

*/
  private:

    int noAttributes;
/*
Number of attributes.

*/
   AttributeType* attrTypeArray;
/*
Array of attribute type descriptions.

*/
    int totalSize;
/*
Sum of all attribute sizes.

*/
    int refs;
/*
A reference counter.

*/
};

/*
4.2 Struct ~RelationDescriptor~

This struct contains necessary information for opening a relation.

*/
struct RelationDescriptor
{
  inline
  RelationDescriptor( TupleType* tupleType ):
    tupleType( tupleType ),
    noTuples( 0 ),
    totalExtSize( 0.0 ),
    totalSize( 0.0 ),
    attrExtSize( tupleType->GetNoAttributes() ),
    attrSize( tupleType->GetNoAttributes() ),
    tupleFileId( 0 ),
    lobFileId( 0 )
    {
      tupleType->IncReference();
      for( int i = 0; i < tupleType->GetNoAttributes(); i++ )
      {
        attrExtSize[i] = 0.0;
        attrSize[i] = 0.0;
      }
    }

  inline
  RelationDescriptor( const ListExpr typeInfo ):
    tupleType( new TupleType( nl->Second( typeInfo ) ) ),
    noTuples( 0 ),
    totalExtSize( 0.0 ),
    totalSize( 0.0 ),
    attrExtSize( 0 ),
    attrSize( 0 ),
    tupleFileId( 0 ),
    lobFileId( 0 )
    {
      attrExtSize.resize( tupleType->GetNoAttributes() );
      attrSize.resize( tupleType->GetNoAttributes() );
      for( int i = 0; i < tupleType->GetNoAttributes(); i++ )
      {
        attrExtSize[i] = 0.0;
        attrSize[i] = 0.0;
      }
    }
/*
The simple constructors.

*/
  inline
  RelationDescriptor( TupleType *tupleType,
                      int noTuples,
                      double totalExtSize, double totalSize,
                      const vector<double>& attrExtSize,
                      const vector<double>& attrSize,
                      const SmiFileId tId, const SmiFileId lId ):
    tupleType( tupleType ),
    noTuples( noTuples ),
    totalExtSize( totalExtSize ),
    totalSize( totalSize ),
    attrExtSize( attrExtSize ),
    attrSize( attrSize ),
    tupleFileId( tId ),
    lobFileId( lId )
    {
      tupleType->IncReference();
    }

  inline
  RelationDescriptor( const ListExpr typeInfo,
                      int noTuples,
                      double totalExtSize, double totalSize,
                      const vector<double>& attrExtSize,
                      const vector<double>& attrSize,
                      const SmiFileId tId, const SmiFileId lId ):
    tupleType( new TupleType( nl->Second( typeInfo ) ) ),
    noTuples( noTuples ),
    totalExtSize( totalExtSize ),
    totalSize( totalSize ),
    attrExtSize( attrExtSize ),
    attrSize( attrSize ),
    tupleFileId( tId ),
    lobFileId( lId )
    {
    }

/*
The first constructor.

*/
  inline
  RelationDescriptor( const RelationDescriptor& desc ):
    tupleType( desc.tupleType ),
    noTuples( desc.noTuples ),
    totalExtSize( desc.totalExtSize ),
    totalSize( desc.totalSize ),
    attrExtSize( desc.attrExtSize ),
    attrSize( desc.attrSize ),
    tupleFileId( desc.tupleFileId ),
    lobFileId( desc.lobFileId )
    {
      tupleType->IncReference();
    }
/*
The copy constructor.

*/
  inline
  ~RelationDescriptor()
  {
    tupleType->DeleteIfAllowed();
  }
/*
The destructor.

*/
  inline RelationDescriptor& operator=( const RelationDescriptor& d )
  {
    tupleType->DeleteIfAllowed();
    tupleType = d.tupleType;
    tupleType->IncReference();
    noTuples = d.noTuples;
    totalExtSize = d.totalExtSize;
    totalSize = d.totalSize;
    tupleFileId = d.tupleFileId;
    lobFileId = d.lobFileId;
    attrExtSize = d.attrExtSize;
    attrSize = d.attrSize;

    return *this;
  }
/*
Redefinition of the assignement operator.

*/

  TupleType *tupleType;
/*
Stores the tuple type of every tuple in the relation.

*/
  int noTuples;
/*
The quantity of tuples inside the relation.

*/
  double totalExtSize;
/*
The total size occupied by the tuples in the relation taking
into account the small FLOBs, i.e. the extension part of
the tuples.

*/
  double totalSize;
/*
The total size occupied by the tuples in the relation taking
into account all parts of the tuples, including the FLOBs.

*/
  vector<double> attrExtSize;
/*
The total size occupied by the attributes in the relation
taking into account the small FLOBs, i.e. the extension part
of the tuples.

*/
  vector<double> attrSize;
/*
The total size occupied by the attributes in the relation
taking into account all parts of the tuples, including the
FLOBs.

*/
  SmiFileId tupleFileId;
/*
The tuple's file identification.

*/
  SmiFileId lobFileId;
/*
The LOB's file identification.

*/
};

/*
4.2 Class ~RelationDescriptorCompare~

*/
class RelationDescriptorCompare
{
  public:
    inline bool operator()( const RelationDescriptor& d1, 
                            const RelationDescriptor d2 )
    {
      if( d1.tupleFileId < d2.tupleFileId )
        return true;
      else if( d1.tupleFileId == d2.tupleFileId &&
               d1.lobFileId == d2.lobFileId )
        return true;
      else
        return false;
    }
};

/*
4.1 Struct ~PrivateRelation~

This struct contains the private attributes of the class ~Relation~.

*/
struct PrivateRelation
{
  PrivateRelation( const ListExpr typeInfo, bool isTemp ):
    relDesc( new TupleType( nl->Second( typeInfo ) ) ),
    tupleFile( false, 0, isTemp ),
    isTemp( isTemp )
    {
      if( !tupleFile.Create() )
      {
        string error;
        SmiEnvironment::GetLastErrorCode( error );
        cout << error << endl;
        assert( false );
      }
      relDesc.tupleFileId = tupleFile.GetFileId();
    }
/*
The first constructor. Creates an empty relation from a ~typeInfo~.

*/
  PrivateRelation( TupleType *tupleType, bool isTemp ):
    relDesc( tupleType ),
    tupleFile( false, 0, isTemp ),
    isTemp( isTemp )
    {
      tupleType->IncReference();
      if( !tupleFile.Create() )
      {
        string error;
        SmiEnvironment::GetLastErrorCode( error );
        cout << error << endl;
        assert( false );
      }
      relDesc.tupleFileId = tupleFile.GetFileId();
    }
/*
The second constructor. Creates an empty relation from a ~tupleType~.

*/
  PrivateRelation( const RelationDescriptor& relDesc, 
                   bool isTemp ):
    relDesc( relDesc ),
    tupleFile( false, 0, isTemp ),
    isTemp( isTemp )
    {
      if( !tupleFile.Open( relDesc.tupleFileId ) )
      {
        string error;
        SmiEnvironment::GetLastErrorCode( error );
        cout << error << endl;
        assert( false );
      }
      this->relDesc.tupleFileId = tupleFile.GetFileId();
    }
/*
The third constructor. Opens a previously created relation.

*/
  ~PrivateRelation()
  {
    tupleFile.Close();
  }
/*
The destuctor.

*/
  RelationDescriptor relDesc;
/*
Stores the descriptor of the relation.

*/
  SmiRecordFile tupleFile;
/*
Stores a handle to the tuple file.

*/
  bool isTemp;
/*
A flag telling whether the relation is temporary.

*/
};


/*
3.5 Class ~Tuple~

This class implements the representation of the type 
constructor ~tuple~.

*/

/*
Declaration of the struct ~PrivateTuple~. This struct contains the
private attributes of the class ~Tuple~.

*/

class Tuple
{
  public:

    inline Tuple( TupleType* tupleType ) :
    tupleType( tupleType )
    {
      Init( tupleType->GetNoAttributes());
      DEBUG(this, "Constructor Tuple(TupleType *tupleType) called.")
    }

/*
The constructor. It contructs a tuple with the metadata passed in 
the ~tupleType~ as argument.

*/
    inline Tuple( const ListExpr typeInfo ) :
    tupleType( new TupleType( typeInfo ) )
    {
      Init( tupleType->GetNoAttributes());
      DEBUG(this, "Constructor Tuple(const ListExpr typeInfo) called.")
    }
/*
A similar constructor as the above, but taking a list 
expression ~typeInfo~ as argument.

*/
  ~Tuple();
/*
The destructor.

*/
    static Tuple *In( const ListExpr typeInfo, ListExpr value, 
                      int errorPos, ListExpr& errorInfo, 
                      bool& correct );
/*
Creates a tuple from the ~typeInfo~ and ~value~ information.
Corresponds to the ~In~-function of type constructor ~tuple~.

*/
    static Tuple *RestoreFromList( const ListExpr typeInfo, ListExpr value,
                                   int errorPos, ListExpr& errorInfo,
                                   bool& correct );
/*
Acts as the ~In~ function, but uses internal representation for 
the objects.

*/
    ListExpr Out( ListExpr typeInfo );
/*
Writes a tuple into a ~ListExpr~ format.
Corresponds to the ~Out~-function of type constructor ~tuple~.

*/
    ListExpr SaveToList( ListExpr typeInfo );
/*
Acts as the ~Out~ function, but uses internal representation for 
the objects.

*/
    static void InitCounters(bool visible);
    static void SetCounterValues();
/*
Initialize tuple counters and assign values.

*/
    const TupleId& GetTupleId() const;
/*
Returns the unique ~id~ of the tuple.

*/
    void SetTupleId( const TupleId& tupleId );
/*
Sets the tuple unique ~id~ of the tuple. This function is necessary 
because at the construction time, the tuple does not know its ~id~.

*/
    inline Attribute* GetAttribute( int index ) const 
    {
      return attributes[index];
    }
/*
Returns the attribute at position ~index~ inside the tuple.

*/
    void PutAttribute( int index, Attribute* attr );
/*
Puts an attribute in the position ~index~ inside the tuple.

*/
    void UpdateAttributes( const vector<int>& changedIndices,
                           const vector<Attribute*>& newAttrs,
                           double& extSize, double& size,
                           vector<double>& attrExtSize,
                           vector<double>& attrSize );
/*
Puts the attributes from ~newAttrs~ at the corresponding position 
from ~changedIndices~ into the tuple. Destroys the physical 
representations of the old attributes and saves the new tuple to 
disk. The implementation of this function is found in the
Update Relation Algebra.

*/
    inline int GetRootSize() const
    {
      return tupleType->GetTotalSize();
    }
/*
Returns the size of the tuple's root part.

*/
    inline int GetRootSize( int i ) const
    {
      return tupleType->GetAttributeType(i).size;
    }
/*
Returns the size of the attribute's root part.

*/
    inline int GetExtSize() const
    {
      if ( !recomputeExtSize ) 
        return tupleExtSize;

      tupleExtSize = 0;
      for( int i = 0; i < noAttributes; i++)
      {
        tupleExtSize += GetExtSize(i);
      }
      recomputeExtSize = false;
      return tupleExtSize; 
    }
/*
Returns the size of the tuple taking into account the extension
part, i.e. the small FLOBs.

*/
    inline int GetExtSize( int i ) const
    {
      int attrExtSize = GetRootSize( i );
      for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++)
      {
        FLOB *tmpFLOB = attributes[i]->GetFLOB(j);
        if( !tmpFLOB->IsLob() )
          attrExtSize += tmpFLOB->Size();
      }
      return attrExtSize;
    }
/*
Returns the size of attribute i including its extension part.

*/
    inline int GetSize() const
    {
      if ( !recomputeSize ) 
        return tupleSize;

      tupleSize = 0;
      for( int i = 0; i < noAttributes; i++)
      {
	tupleSize += GetSize(i);      
      }
      recomputeSize = false;
      return tupleSize;
    }
/*
Returns the total size of the tuple taking into account the 
the FLOBs.

*/
    inline int GetSize( int i ) const
    {
      int tupleSize = GetRootSize(i);
      for( int j = 0; j < attributes[i]->NumOfFLOBs(); j++)
        tupleSize += attributes[i]->GetFLOB(j)->Size();
      return tupleSize;
    }
/*
Returns the total size of an attribute of the tuple taking
into account the FLOBs.

*/
    inline size_t HashValue(int i)
    {
      return static_cast<StandardAttribute*>( GetAttribute(i) )->HashValue();
    }

/*
Returns the hash value for attribute number i.

*/



    inline int GetNoAttributes() const 
    {
      return noAttributes;
    }
/*
Returns the number of attributes of the tuple.

*/
    inline TupleType* GetTupleType() const 
    {
      return tupleType;
    }
/*
Returns the tuple type.

*/
    Tuple *Clone() const;
/*
Create a new tuple which is a clone of this tuple.

*/
    inline bool DeleteIfAllowed() 
    {
      if( refs == 0 ){
        delete this;
        return true;
      } else{
        return false;
      }
    }
/*
Deletes the tuple if it is allowed, i.e., there are no references
(attribute ~refs~) to it anymore.

*/
    inline void IncReference()
    {
      refs++;
    }
/*
Increses the reference count of this tuple.

*/
    inline void DecReference()
    {
      if (refs > 0) 
        refs--;
    }
/*
Decreses the reference count of this tuple.

*/
    int GetNumOfRefs() const
    { 
      return refs; 
    }
/*
Returns the number of references

*/

/*
~CopyAttribute~

This function is used to copy attributes from tuples to tuples 
without cloning attributes.

*/
  inline void CopyAttribute( int sourceIndex, 
                             const Tuple *source, 
                             int destIndex )
  {
    if( attributes[destIndex] != 0 ){ 
      // remove reference from an old attribute
      (attributes[destIndex]->DeleteIfAllowed()); 
    }
    attributes[destIndex] = source->attributes[sourceIndex]->Copy();
  }
  
/*
Saves a tuple into ~tuplefile~ and ~lobfile~. Returns the 
sizes of the tuple saved.

*/

  void Save( SmiRecordFile *tuplefile, SmiFileId& lobFileId,
             double& extSize, double& size,
             vector<double>& attrExtSize, vector<double>& attrSize,
             bool ignorePersistentLOBs=false );

/*
Saves a tuple with updated attributes and reuses the old 
record. 

*/
  void UpdateSave( const vector<int>& changedIndices,
                   double& extSize, double& size,
                   vector<double>& attrExtSize,
                   vector<double>& attrSize );

/*
Opens a tuple from ~tuplefile~(~rid~) and ~lobfile~.

*/
  bool Open( SmiRecordFile *tuplefile, SmiFileId lobfileId,
             SmiRecordId rid );

/*
Opens a tuple from ~tuplefile~ and ~lobfile~ reading the 
current record of ~iter~.

*/
  bool Open( SmiRecordFile *tuplefile, SmiFileId lobfileId,
             PrefetchingIterator *iter );

/*
Opens a tuple from ~tuplefile~ and ~lobfile~ reading from 
~record~.

*/
  bool Open( SmiRecordFile *tuplefile, SmiFileId lobfileId,
             SmiRecord *record );                

  // debug flag  
  static bool debug;
    
  private:

    static long tuplesCreated;
    static long tuplesDeleted;
    static long tuplesInMemory;
    static long maximumTuples;
/*
Some statistics about tuples.

*/
    inline void InitAttrArray()
    {
      for( int i = 0; i < noAttributes; i++ )
        attributes[i] = 0;
    }
/*
Initializes the attributes array with zeros.

*/
    inline void Init( int NoAttr)
    {
      tupleType->IncReference();
      noAttributes = NoAttr;

      refs = 0;
      tupleId = 0;
      tupleSize = 0;
      tupleFile = 0;
      tupleExtSize = 0;
      lobFileId =  0;

      recomputeExtSize = true;
      recomputeSize = true;

      if ( noAttributes > MAX_NUM_OF_ATTR ) 
        attributes = new Attribute*[noAttributes];
      else 
        attributes = defAttributes;

      InitAttrArray();

      tuplesCreated++;
      tuplesInMemory++;
      if( tuplesInMemory > maximumTuples ) 
        maximumTuples = tuplesInMemory;
    }
/*
Initializes a tuple.

*/

    int refs;
/*
The reference count of this tuple. There can exist several tuple
pointers pointing to the same tuple and sometimes we want to prevent 
deleting tuples. As an example, in some operators buffers that
store tuples in memory are used. Until the memory is kept in the
buffer it cannot be deleted by other pointers. For this case, 
before appending a tuple in the buffer, the reference count of the
tuple is increased. To delete tuples we always call the function
~DeleteIfAllowed~, which first checks if ~refs~ = 0 to really 
delete the tuple. This number is initialized with 0 so that normally
the tuple will be deleted.

*/

    mutable bool recomputeExtSize;
    mutable bool recomputeSize;
/*
These two flags together with the next two attributes are used
in order to avoid re-computing the extension and the total sizes of
tuples. The first time these sizes are computed they are stored.

*/

    mutable int tupleExtSize;
/*
Stores the size of the tuples taking into account the extension
part of the tuple, i.e. the small FLOBs.

*/
    mutable int tupleSize;
/*
Stores the total size of the tuples taking into account the 
FLOBs.

*/
    int noAttributes;
/*
Store the number of attributes of the tuple.

*/

    Attribute** attributes;
    Attribute* defAttributes[MAX_NUM_OF_ATTR];
/*
The attribute array. If it contains less than ~MAX\_NUM\_OF\_ATTR~
entries, the it is statically constructed and ~attributes~ point
to ~defAttributes~, otherwise it is dinamically constructed.

*/
    //mutable PrivateTuple *privateTuple;
/*
The private implementation dependent attributes of the class 
~Tuple~.

*/



/*
The unique identification of the tuple inside a relation.

*/
  SmiRecordId tupleId;

/*
Stores the tuple type.

*/
  mutable TupleType *tupleType;

/*
Reference to an ~SmiRecordFile~ which contains LOBs.

*/
  SmiFileId lobFileId;
/*
Reference to an ~SmiRecordFile~ which contains the tuple.

*/
  SmiRecordFile* tupleFile;

/*
The members below are useful for debugging:

Profiling turned out that a separate member storing the number
of attributes makes sense since it reduces calls for TupleType::NoAttributes

*/
  int NumOfAttr;
  
/*
Debugging stuff

*/  
#ifdef MALLOC_CHECK_  
  void free (void* ptr) { cerr << "freeing ptr " << ptr << endl; ::free(ptr); }
#endif

};

ostream& operator<<(ostream &os, Attribute &attrib);
ostream& operator<<( ostream& o, const Tuple& t );
/*
The print function for tuples. Used for debugging purposes

*/

/*
3.7 Class ~LexicographicalTupleCompare~

This is a class used in the sort algorithm that specifies the 
lexicographical comparison function between two tuples.

*/

// use this one for sorting
class LexicographicalTupleCompare
{
  public:
    inline bool operator()( const Tuple* aConst, 
                            const Tuple* bConst ) const
    {
      Tuple* a = (Tuple*)aConst;
      Tuple* b = (Tuple*)bConst;

      for( int i = 0; i < a->GetNoAttributes(); i++ )
      {
        int cmp = 
          ((Attribute*)a->GetAttribute(i))->Compare(
            ((Attribute*)b->GetAttribute(i)));
        if( cmp < 0)
          return true;
        if( cmp > 0)
          return false;
      }
      return false;
    }
};

class LexicographicalTupleCompare_Reverse : public LexicographicalTupleCompare
{
  public:
    inline bool operator()( const Tuple* a, 
                            const Tuple* b ) const
    {
      return !LexicographicalTupleCompare::operator()(a, b);
    }
};



// use this one to remove duplicates
class LexicographicalTupleCompareAlmost
{
  public:
    inline bool operator()( const Tuple* aConst, 
                            const Tuple* bConst ) const
    {
      Tuple* a = (Tuple*)aConst;
      Tuple* b = (Tuple*)bConst;

      for( int i = 0; i < a->GetNoAttributes(); i++ )
      {
        int cmp = 
          ((Attribute*)a->GetAttribute(i))->CompareAlmost(
            ((Attribute*)b->GetAttribute(i)));
        if( cmp < 0)
          return true;
        if( cmp > 0)
          return false;
      }
      return false;
    }
};
/*
3.8 Class ~TupleCompareBy~

This is a class used in the sort algorithm that specifies the 
comparison function between two tuples using a set of attributes 
specified in ~SortOrderSpecification~, which is a vector of pairs 
containing the index of the attribute and a boolean flag telling 
whether the ordering is ascendant or not (descendant).

*/

// use this one to sort
typedef vector< pair<int, bool> > SortOrderSpecification;

class TupleCompareBy
{
  public:
    TupleCompareBy( const SortOrderSpecification &spec ):
      spec( spec ),
      len( spec.size() )
      {}

    inline bool operator()( const Tuple* a, 
                            const Tuple* b ) const
    {
      if (len > 1) 
      {

        SortOrderSpecification::const_iterator iter = spec.begin();
        while( iter != spec.end() )
        {
          const int pos = iter->first-1;
          const Attribute* aAttr = (const Attribute*) a->GetAttribute(pos);
          const Attribute* bAttr = (const Attribute*) b->GetAttribute(pos);
          const int cmpValue = aAttr->Compare( bAttr );

          if( cmpValue !=  0 ) 
          {
            // aAttr < bAttr ?
            return (cmpValue < 0) ? iter->second : !(iter->second);
          }
          // the current attribute is equal
          iter++;
        }
        // all attributes are equal  
        return false;
      }
      else
      {
        const int pos = spec[0].first-1;
        const Attribute* aAttr = (const Attribute*) a->GetAttribute(pos);
        const Attribute* bAttr = (const Attribute*) b->GetAttribute(pos);
        return aAttr->Less(bAttr) ? spec[0].second : !spec[0].second;
      } 
  }

  private:
    SortOrderSpecification spec;
    const size_t len;
};


class TupleCompareBy_Reverse : public TupleCompareBy
{
  public:
    TupleCompareBy_Reverse( const SortOrderSpecification& spec ) 
    : TupleCompareBy(spec)
    {}	

    inline bool operator()( const Tuple* a, 
                            const Tuple* b ) const
    {
      return !TupleCompareBy::operator()(a, b);
    }
};



// use this one to remove duplicates
class TupleCompareByAlmost
{
  public:
    TupleCompareByAlmost( const SortOrderSpecification &spec ):
      spec( spec ),
      len( spec.size() )
      {}

    inline bool operator()( const Tuple* a, 
                            const Tuple* b ) const
    {
      if (len > 1) 
      {

        SortOrderSpecification::const_iterator iter = spec.begin();
        while( iter != spec.end() )
        {
          const int pos = iter->first-1;
          const Attribute* aAttr = (const Attribute*) a->GetAttribute(pos);
          const Attribute* bAttr = (const Attribute*) b->GetAttribute(pos);
          const int cmpValue = aAttr->CompareAlmost( bAttr );

          if( cmpValue !=  0 ) 
          {
            // aAttr < bAttr ?
            return (cmpValue < 0) ? iter->second : !(iter->second);
          }
          // the current attribute is equal
          iter++;
        }
        // all attributes are equal  
        return false;
      }
      else
      {
        const int pos = spec[0].first-1;
        const Attribute* aAttr = (const Attribute*) a->GetAttribute(pos);
        const Attribute* bAttr = (const Attribute*) b->GetAttribute(pos);
        return aAttr->Less(bAttr) ? spec[0].second : !spec[0].second;
      } 
  }

  private:
    SortOrderSpecification spec;
    const size_t len;
};

/*
3.10 Class ~GenericRelationIterator~

This abstract class forces its two son classes ~RelationIterator~
and ~TupleBufferIterator~ to implement some functions so that
both classes can be used in a generic way with polymorphism.

*/
class GenericRelationIterator
{
  public:
    virtual ~GenericRelationIterator() {};
/*
The virtual destructor.

*/
    virtual Tuple *GetNextTuple() = 0;
/*
The function to retrieve the next tuple.

*/
    virtual TupleId GetTupleId() const = 0;
/*
The function to retrieve the current tuple ~id~.

*/

};

/*
3.9 Class ~GenericRelation~

This abstract class forces its two son classes ~Relation~ and
~TupleBuffer~ to implement some functions so that both classes 
can be used in a generic way with polymorphism.

*/
class GenericRelation
{
  public:
    virtual ~GenericRelation() {};
/*
The virtual destructor.

*/
    virtual int GetNoTuples() const = 0;
/*
The function to return the number of tuples.

*/
    virtual double GetTotalRootSize() const = 0;
/*
The function to return the total size of the relation
in bytes, taking into account only the root part of the 
tuples.

*/
    virtual double GetTotalRootSize(int i) const = 0;
/*
The function to return the total size of and attribute
of the relation in bytes, taking into account only the
root part of the tuples.

*/
    virtual double GetTotalExtSize() const = 0;
/*
The function to return the total size of the relation
in bytes, taking into account the root part of the 
tuples and the extension part, i.e. the small FLOBs. 

*/
    virtual double GetTotalExtSize( int i ) const = 0;
/*
The function to return the total size of an attribute
of the relation in bytes, taking into account the root
part of the tuples and the extension part, i.e. the
small FLOBs.

*/
    virtual double GetTotalSize() const = 0;
/*
The function to return the total size of the relation
in bytes.

*/
    virtual double GetTotalSize( int i ) const = 0;
/*
The function to return the total size of an attribute
of the relation in bytes, taking into account the root
part of the tuples and the extension part, i.e. the
small FLOBs.

*/
    virtual void Clear() = 0;
/*
The function that clears the set.

*/
    virtual void AppendTuple( Tuple *t ) = 0;
/*
The function to append a tuple into the set.

*/
    virtual Tuple *GetTuple( const TupleId& id ) const = 0;
/*
The function that retrieves a tuple given its ~id~.

*/
    virtual 
    Tuple *GetTuple( const TupleId& id, 
                     const int attrIndex,
                     const vector< pair<int, int> >& intervals ) 
      const = 0;
/*
The function is similar to the last one, but instead of only
retrieving the tuple, it restricts the first FLOB (or DBArray)
of the attribute indexed by ~attrIndex~ to the positions given 
by ~intervals~. This function is used in Double Indexing
(operator ~gettuplesdbl~).

*/
    virtual GenericRelationIterator *MakeScan() const = 0;
/*
The function to initialize a scan returning the iterator.

*/
};

/*
3.10 Class ~TupleBufferIterator~

This class is an iterator for the ~TupleBuffer~ class.

*/
class TupleBuffer;
/*
Forward declaration of the ~TupleBuffer~ class.

*/

class TupleBufferIterator : public GenericRelationIterator
{
  public:
    TupleBufferIterator( const TupleBuffer& buffer );
/*
The constructor.

*/
    ~TupleBufferIterator();
/*
The destructor.

*/
    Tuple *GetNextTuple();
/*
Returns the next tuple of the buffer. Returns 0 if the end of the 
buffer is reached.

*/
    TupleId GetTupleId() const;
/*
Returns the tuple identification of the current tuple.

*/

  private:

  long& readData_Bytes;
  long& readData_Pages;

  const TupleBuffer& tupleBuffer;
/*
A pointer to the tuple buffer.

*/
  size_t currentTuple;
/*
The current tuple if it is in memory.

*/
  GenericRelationIterator *diskIterator;
/*
The iterator if it is not in memory.

*/


};

/*
3.9 Class ~TupleBuffer~

This class is a collection of tuples in memory, if they fit. 
Otherwise it acts like a relation. The size of memory used is
passed in the constructor.


*/
class Relation;

/*
Forward declaration of class ~Relation~.

*/

class TupleBuffer : public GenericRelation
{
  public:
    TupleBuffer( size_t maxMemorySize = 16 * 1024 * 1024 );
/*
The constructor. Creates an empty tuple buffer.

*/
    ~TupleBuffer();
/*
The destructor. Deletes (if allowed) all tuples.

*/

    virtual Tuple* GetTuple( const TupleId& tupleId ) const;

    virtual Tuple *GetTuple( const TupleId& id,
                     const int attrIndex,
                     const vector< pair<int, int> >& intervals ) const;

    virtual void   Clear();
    virtual int    GetNoTuples() const;
    virtual double GetTotalRootSize() const;
    virtual double GetTotalRootSize(int i) const;
    virtual double GetTotalExtSize() const;
    virtual double GetTotalExtSize( int i ) const;
    virtual double GetTotalSize() const;
    virtual double GetTotalSize( int i ) const;
    virtual void   AppendTuple( Tuple *t );

    virtual GenericRelationIterator *MakeScan() const;

/*
inherited ~virtual~ functions. 

Note: The function ~AppendTuple~ does not copy the physical representation of
FLOBS which are in state "InDiskLarge", since TupleBuffers should only be used
for intermediate result sets needed in operator implementations.

*/

    Tuple* GetTupleAtPos( const size_t pos ) const;
    bool SetTupleAtPos( const size_t pos, Tuple* t);
/*
Retrieves or assigns a tuple to the memory buffer. If the
position is out of range or the buffer is not in state inMemory, 
a null pointer or false will be returned.

For a valid position the stored tuple pointer will be returned or assigned.
Positions start at zero.

*/

    inline bool IsEmpty() const;
/*
Checks if the tuple buffer is empty or not.

*/
    size_t FreeBytes() const;
/*
Returns the number of Free Bytes

*/     

    inline bool InMemory() const { return inMemory; }
/*
Checks if the tuple buffer is empty or not.

*/
    friend class TupleBufferIterator;

  private:

  typedef vector<Tuple*> TupleVec;

  const size_t MAX_MEMORY_SIZE;
/*
The maximum size of the memory in bytes. 32 MBytes being used.

*/
  TupleVec memoryBuffer;
/*
The memory buffer which is a ~vector~ from STL.

*/
  Relation* diskBuffer;
/*
The buffer stored on disk.

*/
  bool inMemory;
/*
A flag that tells if the buffer fit in memory or not.

*/
  const bool traceFlag;
/*
Switch trace messages on or off

*/  
  double totalExtSize;
/*
The total size occupied by the tuples in the buffer,
taking into account the small FLOBs.

*/
  double totalSize;
/*
The total size occupied by the tuples in the buffer,
taking into account the FLOBs.

*/
  void clearAll();

  void updateDataStatistics();

};

/*
The class ~RandomTBuf~ provides a special kind of tuple buffer
which draws a random subset of N tuples for all tuples which
are given to it. 

Only the selected tuples are stored there, rejected or released
tuples must be handled elsewhere

*/


class RandomTBuf
{
  public:
  RandomTBuf( size_t subsetSize = 500 );
  ~RandomTBuf() {};

  Tuple* ReplacedByRandom(Tuple* in, size_t& idx, bool& replaced);

  typedef vector<Tuple*> Storage;
  typedef Storage::iterator iterator;

  iterator begin() { return memBuf.begin(); }
  iterator end()   { return memBuf.end(); }

  void copy2TupleBuf(TupleBuffer& tb);

  private:
    size_t subsetSize;
    size_t streamPos;
    int run;
    bool trace;
    Storage memBuf;
}; 

/*
4 Type constructor ~rel~

4.2 Class ~RelationIterator~

This class is used for scanning (iterating through) relations.

*/


class RelationIterator : public GenericRelationIterator
{
  public:
    RelationIterator( const Relation& relation );
/*
The constructor. Creates a ~RelationIterator~ for a given ~relation~
and positions the cursor in the first tuple, if exists.

*/
    ~RelationIterator();
/*
The destructor.

*/
    Tuple *GetNextTuple();
/*
Retrieves the tuple in the current position of the iterator and moves
the cursor forward to the next tuple. Returns 0 if the cursor is in 
the end of a relation.

*/
    TupleId GetTupleId() const;
/*
Returns the tuple identifier of the current tuple.

*/
    bool EndOfScan();
/*
Tells whether the cursor is in the end of a relation.

*/
  protected:
  PrefetchingIterator *iterator;
/*
The iterator.

*/
  const Relation& relation;
/*
A reference to the relation.

*/
  bool endOfScan;
/*
Stores the state of the iterator.

*/
  TupleId currentTupleId;
/*
Stores the identification of the current tuple.

*/
};


class RandomRelationIterator : protected RelationIterator
{
  public:
    RandomRelationIterator( const Relation& relation );
/*
The constructor. Creates a ~RandomRelationIterator~ for a given ~relation~
and positions the cursor in the first tuple, if exists.

*/
    ~RandomRelationIterator();
/*
The destructor.

*/
    Tuple *GetNextTuple(int step=1);
/*
Retrieves the tuple in the current position of the iterator and moves
the cursor forward to the next tuple. Returns 0 if the cursor is in 
the end of a relation.

*/
};

/*
4.1 Class ~Relation~

This class implements the memory representation of the type 
constructor ~rel~.

*/

struct RelationDescriptor;
class RelationDescriptorCompare;
/*
Forward declaration of the struct ~RelationDescriptor~ and the 
comparison class ~RelationDescriptorCompare~. These classes will 
contain the necessary information for opening a relation.

*/

struct PrivateRelation;
/*
Forward declaration of the struct ~PrivateRelation~. This struct 
will contain the private attributes of the class ~Relation~.

*/
class Relation : public GenericRelation
{
  public:
    Relation( const ListExpr typeInfo, bool isTemp = false );
/*
The first constructor. It creates an empty relation from a 
~typeInfo~. The flag ~isTemp~ can be used to create temporary 
relations.

*/
    Relation( TupleType *tupleType, bool isTemp = false );
/*
The second constructor. It creates an empty relation from a 
~tupleType~. The flag ~isTemp~ can be used to create temporary 
relations.

*/
    Relation( const RelationDescriptor& relDesc, 
              bool isTemp = false );
/*
The third constructor. It opens a previously created relation. 
The flag ~isTemporary~ can be used to open temporary created 
relations.

*/
    ~Relation();
/*
The destructor. Deletes the memory part of an relation object.

*/

    static Relation *GetRelation( const RelationDescriptor& d );
/*

Given a relation descriptor, finds if there is an opened relation with that
descriptor and retrieves its memory representation pointer.This function is
used to avoid opening several times the same relation. A table indexed by
descriptors containing the relations is used for this purpose. 

*/
    static GenericRelation *In( ListExpr typeInfo, ListExpr value, 
                         int errorPos, ListExpr& errorInfo, 
                         bool& correct, bool tupleBuf = false );
/*
Creates a relation from the ~typeInfo~ and ~value~ information.
Corresponds to the ~In~-function of type constructor ~rel~.

*/
    static Relation *RestoreFromList( ListExpr typeInfo, 
                                      ListExpr value, int errorPos, 
                                      ListExpr& errorInfo, 
                                      bool& correct );
/*
Acts like the ~In~ function, but uses internal representation for 
the objects. Corresponds to the ~RestoreFromList~-function of type 
constructor ~rel~.

*/
    static ListExpr Out( ListExpr typeInfo, GenericRelationIterator* rit );
/*
Writes a relation into a ~ListExpr~ format.
Corresponds to the ~Out~-function of type constructor ~rel~.

*/
    ListExpr SaveToList( ListExpr typeInfo );
/*
Acts like the ~Out~ function, but uses internal representation for 
the objects. Corresponds to the ~SaveToList~-function of type 
constructor ~rel~.

*/
    static Relation *Open( SmiRecord& valueRecord, size_t& offset, 
                           const ListExpr typeInfo );
/*
Opens a relation.
Corresponds to the ~Open~-function of type constructor ~rel~.

*/
    bool Save( SmiRecord& valueRecord, size_t& offset, 
               const ListExpr typeInfo );
/*
Saves a relation.
Corresponds to the ~Save~-function of type constructor ~rel~.

*/
    void Close();
/*
Closes a relation.
Corresponds to the ~Close~-function of type constructor ~rel~.

*/
    void Delete();
/*
Deletes a relation.
Corresponds to the ~Delete~-function of type constructor ~rel~.

*/
    void DeleteAndTruncate();
/*
Deletes a relation and truncates the corresponding record files.
This releases used disk memory.

*/
    Relation *Clone();
/*
Clones a relation.
Corresponds to the ~Clone~-function of type constructor ~rel~.

*/
    bool DeleteTuple( Tuple *tuple );
/*
Deletes the tuple from the relation. Returns false if the tuple 
could not be deleted. The implementation of this function belongs
to the Update Relational Algebra.

*/
    void UpdateTuple( Tuple *tuple, 
                      const vector<int>& changedIndices, 
                      const vector<Attribute *>& newAttrs );
/*
Updates the tuple by putting the new attributes at the positions 
given by ~changedIndices~ and adjusts the physical representation.
The implementation of this function belongs to the Update 
Relational Algebra.

*/
    TupleType *GetTupleType() const;
/*
Returns the tuple type of the tuples of the relation.

*/

    virtual Tuple* GetTuple( const TupleId& tupleId ) const;

    virtual Tuple *GetTuple( const TupleId& id,
                     const int attrIndex,
                     const vector< pair<int, int> >& intervals ) const;

    virtual void   Clear();
    virtual int    GetNoTuples() const;
    virtual double GetTotalRootSize() const;
    virtual double GetTotalRootSize(int i) const;
    virtual double GetTotalExtSize() const;
    virtual double GetTotalExtSize( int i ) const;
    virtual double GetTotalSize() const;
    virtual double GetTotalSize( int i ) const;
    virtual void   AppendTuple( Tuple *tuple );

    virtual GenericRelationIterator *MakeScan() const;

/*
Inherited ~virtual~ functions

*/

    void AppendTupleNoLOBs( Tuple *tuple );

/*
A special variant of ~AppendTuple~ which does not copy the LOBs.

*/    

    RandomRelationIterator *MakeRandomScan() const;
/*
Returns a ~RandomRelationIterator~ which returns tuples in random order.
Currently this is not implemented, but first of all it this iterator skips
some tuples in the scan, hence it can iterate faster. It is used by the
sample operator.

*/
    inline PrivateRelation *GetPrivateRelation()
    { 
      return privateRelation; 
    }
/*
Function to give outside access to the private part of the 
relation class.

*/

    friend class RelationIterator;
    friend class RandomRelationIterator;
    friend struct PrivateRelationIterator;

  private:

    void ErasePointer(); 
/*
Removes the current Relation from the table of opened relations.

*/    

    PrivateRelation *privateRelation;
/*
The private attributes of the class ~Relation~.

*/
    static map<RelationDescriptor, Relation*, 
               RelationDescriptorCompare> pointerTable;
/*
A table containing all opened relations indexed by relation 
descriptors.

*/
};

/*
4 Auxiliary functions' interface

4.1 Function ~TypeOfRelAlgSymbol~

Transforms a list expression ~symbol~ into one of the values of
type ~RelationType~. ~Symbol~ is allowed to be any list. If it is 
not one of these symbols, then the value ~error~ is returned.

*/
enum RelationType { rel, trel, tuple, stream, ccmap, ccbool, error };
RelationType TypeOfRelAlgSymbol (ListExpr symbol);

/*
3.2 Function ~FindAttribute~

Here ~list~ should be a list of pairs of the form 
(~name~,~datatype~). The function ~FindAttribute~ determines 
whether ~attrname~ occurs as one of the names in this list. If so, 
the index in the list (counting from 1) is returned and the 
corresponding datatype is returned in ~attrtype~. Otherwise 0 is 
returned. Used in operator ~attr~, for example.

*/
int FindAttribute( ListExpr list, 
                   string attrname, 
                   ListExpr& attrtype);

/*
3.3 Function ~ConcatLists~

Concatenates two lists.

*/
ListExpr ConcatLists( ListExpr list1, ListExpr list2);

/*
3.5 Function ~AttributesAreDisjoint~

Checks wether two ListExpressions are of the form
((a1 t1) ... (ai ti)) and ((b1 d1) ... (bj dj))
and wether the ai and the bi are disjoint.

*/
bool AttributesAreDisjoint(ListExpr a, ListExpr b);

/*
3.6 Function ~Concat~

Copies the attribute values of two tuples ~r~ and ~s~ into 
tuple ~t~.

*/
void Concat (Tuple *r, Tuple *s, Tuple *t);

/*
3.7 Function ~CompareNames~

*/
bool CompareNames(ListExpr list);

/*

5.6 Function ~IsTupleDescription~

Checks whether a ~ListExpr~ is of the form
((a1 t1) ... (ai ti)).

*/
bool IsTupleDescription( ListExpr tupleDesc );

/*

5.6 Function ~IsRelDescription~

Checks whether a ~ListExpr~ is of the form
( {rel|trel} (tuple ((a1 t1) ... (ai ti)))).

*/
bool IsRelDescription( ListExpr relDesc, bool trel = false );

/*

5.6 Function ~IsStreamDescription~

Checks whether a ~ListExpr~ is of the form
(stream (tuple ((a1 t1) ... (ai ti)))).

*/
bool IsStreamDescription( ListExpr relDesc );

/*
5.7 Function ~GetTupleResultType~

This function returns the tuple result type as a list expression
given the Supplier ~s~.

*/
ListExpr GetTupleResultType( Supplier s );

/*
5.8 Function ~CompareSchemas~

This function takes two relations types and compare their schemas.
It returns true if they are equal, and false otherwise.

*/
bool CompareSchemas( ListExpr r1, ListExpr r2 );

#endif // _RELATION_ALGEBRA_H_
