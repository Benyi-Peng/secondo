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
#include "NestedList.h"
#include "Operator.h"
#include "StandardTypes.h"

namespace CRelAlgebra
{
  namespace Operators
  {
    /*
    Operator implementation of the 'count' operator.
    This operator returns the accumulated number of tuples from a 'crel' or a
    (stream of) 'tblock'.

    The returned value represents the count and is of type 'longint'.
    */
    class Count : public Operator
    {
    public:
      Count();

    private:
      static const OperatorInfo info;

      static ValueMapping valueMappings[];

      static ListExpr TypeMapping(ListExpr args);

      static int SelectValueMapping(ListExpr args);

      static int CRelValueMapping(ArgVector args, Word &result, int message,
                                  Word &local, Supplier s);

      static int BlockStreamValueMapping(ArgVector args, Word &result,
                                         int message, Word &local, Supplier s);

      static int BlockValueMapping(ArgVector args, Word &result, int message,
                                   Word &local, Supplier s);
    };
  }
}