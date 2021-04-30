/*
----
This file is part of SECONDO.

Copyright (C) 2012, University in Hagen
Faculty of Mathematic and Computer Science,
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

#ifndef QUERFILTERFIELDS_H
#define QUERFILTERFIELDS_H

#include "NestedList.h"
#include "ListUtils.h"
#include "NList.h"
#include <list> 
#include "../Relation-C++/RelationAlgebra.h"
#include "RelationSchemaInfo.h"


namespace pgraph2 {

//-----------------------------------------------------------------------------
class QueryFilterField
{
public:
   std::string NodeAlias="";
   std::string PropertyName="";
   std::string Operator="";
   std::string FilterValue="";
};

//-----------------------------------------------------------------------------
class QueryFilterFields
{
public:
   std::list<QueryFilterField*> Fields;

   void ReadFromList(ListExpr list);
   bool Matches(std::string typname, RelationSchemaInfo *schema, Tuple *tuple);
};


} // namespace
#endif