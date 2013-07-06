/*
This file is part of SECONDO.

Copyright (C) 2013, University in Hagen, Department of Computer Science,
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

*/

#ifndef TILEALGEBRA_MATCHGRID_H
#define TILEALGEBRA_MATCHGRID_H

#include "AlgebraTypes.h"
#include "Operator.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include "../Types.h"
#include "../grid/tgrid.h"
#include "../grid/mtgrid.h"

namespace TileAlgebra
{

/*
definition of matchgrid Operator Info structure

*/

struct matchgridInfo : OperatorInfo
{
  matchgridInfo()
  {
    name      = "matchgrid";
    syntax    = "_ matchgrid [_, _, _]";
    meaning   = "Resamples a t type object or a mt type object "
                "by applying a user function.";

    std::vector<std::string> valueWrapperTypes;
    std::vector<std::string> tTypes;
    std::vector<std::string> mtTypes;
    GetValueWrapperTypes(valueWrapperTypes);
    GettTypes(tTypes);
    GetmtTypes(mtTypes);

    if(valueWrapperTypes.size() == tTypes.size() &&
       valueWrapperTypes.size() == mtTypes.size())
    {
      for(size_t i = 0; i < tTypes.size(); i++)
      {
        for(size_t j = 0; j < valueWrapperTypes.size(); j++)
        {
          if(signature.empty())
          {
            signature = tTypes[i] + " x " + tgrid::BasicType() + " x " +
                        "(rel(tuple([Elem : T])) -> " +
                        valueWrapperTypes[j] + ") x " +
                        CcBool::BasicType() + " -> " + tTypes[j];
          }

          else
          {
            appendSignature(tTypes[i] + " x " + tgrid::BasicType() + " x " +
                            "(rel(tuple([Elem : T])) -> " +
                            valueWrapperTypes[j] + ") x " +
                            CcBool::BasicType() + " -> " + tTypes[j]);
          }
        }
      }

      for(size_t i = 0; i < mtTypes.size(); i++)
      {
        for(size_t j = 0; j < valueWrapperTypes.size(); j++)
        {
          appendSignature(mtTypes[i] + " x " + mtgrid::BasicType() + " x " +
                          "(rel(tuple([Elem : T])) -> " +
                          valueWrapperTypes[j] + ") x " +
                          CcBool::BasicType() + " -> " + mtTypes[j]);
        }
      }
    }

    else
    {
      assert(false);
    }
  }
};

/*
declaration of matchgrid functions

*/

extern ValueMapping matchgridFunctions[];

/*
declaration of matchgrid select function

*/

int matchgridSelectFunction(ListExpr arguments);

/*
declaration of matchgrid type mapping function

*/

ListExpr matchgridTypeMappingFunction(ListExpr arguments);

}

#endif // TILEALGEBRA_MATCHGRID_H
