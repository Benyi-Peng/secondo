/*
----
This file is part of SECONDO.

Copyright (C) 2004-2009, University in Hagen, Faculty of Mathematics
and Computer Science, Database Systems for New Applications.

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

#pragma once

#include "AlgebraTypes.h"
#include "CRel.h"
#include <cstddef>
#include "NestedList.h"
#include "Operator.h"

namespace CRelAlgebra
{
  namespace Operators
  {
    /*
    Operator implementation of the 'feed' operator.
    This operator returns a stream of tuple-block's from a column-oriented
    relation.

    The operator expects one parameter.

    The parameter represents the relation and must be of type 'crel'.

    The returned value is a stream of 'tblock'.
    */
    class Feed : public Operator
    {
    public:
      Feed();

    private:
      //This class holds the operator's state between contiguous calls to
      //ValueMapping
      class State
      {
      public:
        State(ArgVector args, Supplier s);

        TBlock *Request();

      private:
        CRel &m_relation;

        size_t m_blockIndex;
      };

      static const OperatorInfo info;

      static ListExpr TypeMapping(ListExpr args);
    };
  }
}