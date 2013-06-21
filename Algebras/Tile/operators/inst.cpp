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

#include "inst.h"
#include "../Types.h"
#include "../it/itint.h"
#include "../it/itreal.h"
#include "../it/itbool.h"
#include "../it/itstring.h"

namespace TileAlgebra
{

/*
definition of inst functions

*/

ValueMapping instFunctions[] =
{
  instFunction<itint>,
  instFunction<itreal>,
  instFunction<itbool>,
  instFunction<itstring>,
  0
};

/*
definition of inst select function

*/

int instSelectFunction(ListExpr arguments)
{
  int nSelection = -1;

  if(arguments != 0)
  {
    NList argumentsList(arguments);

    if(argumentsList.hasLength(1))
    {
      NList argument1 = argumentsList.first();
      const int TYPE_NAMES = 4;
      const std::string TYPE_NAMES_ARRAY[TYPE_NAMES] =
      {
        itint::BasicType(),
        itreal::BasicType(),
        itbool::BasicType(),
        itstring::BasicType(),
      };

      for(int i = 0; i < TYPE_NAMES; i++)
      {
        if(argument1.isSymbol(TYPE_NAMES_ARRAY[i]))
        {
          nSelection = i;
          break;
        }
      }
    }
  }

  return nSelection;
}

/*
definition of inst type mapping function

*/

ListExpr instTypeMappingFunction(ListExpr arguments)
{
  ListExpr type = NList::typeError("Expecting an it type.");

  NList argumentsList(arguments);

  if(argumentsList.hasLength(1))
  {
    std::string argument1 = argumentsList.first().str();

    if(IsitType(argument1))
    {
      type = NList(Instant::BasicType()).listExpr();
    }
  }

  return type;
}

}
