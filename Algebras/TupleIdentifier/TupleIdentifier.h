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

//paragraph [1] title: [{\Large \bf ]  [}]


[1] TupleIdentifier Algebra

March 2005 Matthias Zielke

The only purpose of this algebra is to provide a typeconstructor 'tid' so that the tupleidentifiers
of tuples from relations can be stored as attributvalues in different tuples. This feature is needed
for the implementation of operators to update relations.

1 Preliminaries

1.1 Includes

*/

#ifndef TUPLEIDENTIFIER_H
#define TUPLEIDENTIFIER_H

using namespace std;

#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include <string>

extern NestedList* nl;
extern QueryProcessor *qp;

typedef long TupleId;

/*
2 Type Constructor ~tid~

2.1 Data Structure - Class ~TupleIdentifier~

*/
class TupleIdentifier: public Attribute
{
 public:
  inline TupleIdentifier() {};
/*
This constructor should not be used.

*/
  TupleIdentifier( bool DEFINED, TupleId TID = 0 );
  TupleIdentifier(const TupleIdentifier& source);
  ~TupleIdentifier();
  TupleId      GetTid() const;
  void     SetTid( const TupleId tid);
  TupleIdentifier*   Clone() const;
  ostream& Print( ostream& os ) const;
  inline bool IsDefined() const
  {
    return (defined);
  }

  inline void Set(const bool DEFINED, const TupleId ID)
  {
    this->defined = DEFINED;
    this->tid = static_cast<long>(ID);
  }

  inline void SetDefined(bool DEFINED)
  {
    this->defined = DEFINED;
  }

  inline size_t Sizeof() const
  {
    return sizeof( *this );
  }

  inline size_t HashValue() const
  {
    return (defined ? tid : 0);
  }

  void CopyFrom(const Attribute* right);

  inline int Compare(const Attribute *arg) const
  {
    const TupleIdentifier* tupleI = (const TupleIdentifier*)(arg);
    bool argDefined = tupleI->IsDefined();
    if(!defined && !argDefined)
      return 0;
    if(!defined)
      return -1;
    if(!argDefined)
      return 1;
    if ( tid < tupleI->GetTid() )
      return (-1);
    if ( tid > tupleI->GetTid())
      return (1);
    return (0);
  }

  bool Adjacent(const Attribute *arg) const;

 private:
  long tid;
  bool defined;
};


#endif
