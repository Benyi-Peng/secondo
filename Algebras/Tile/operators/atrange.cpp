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

#include "atrange.h"
#include "../Types.h"
#include "../t/tint.h"
#include "../t/treal.h"
#include "../t/tbool.h"
#include "../t/tstring.h"
#include "../mt/mtint.h"
#include "../mt/mtreal.h"
#include "../mt/mtbool.h"
#include "../mt/mtstring.h"

namespace TileAlgebra
{

/*
definition of atrange functions

*/

ValueMapping atrangeFunctions[] =
{
  atrangeFunction<tint>,
  atrangeFunction<treal>,
  atrangeFunction<tbool>,
  atrangeFunction<tstring>,
  atrangeFunction<mtint>,
  atrangeFunction<mtreal>,
  atrangeFunction<mtbool>,
  atrangeFunction<mtstring>,
  0
};

/*
definition of atrange select function

*/

int atrangeSelectFunction(ListExpr arguments)
{
  int nSelection = -1;

  if(arguments != 0)
  {
    NList argumentsList(arguments);
    NList argument1 = argumentsList.first();
    NList argument2 = argumentsList.second();

    if(argument2.isSymbol(Rectangle<2>::BasicType()))
    {
      const int TYPE_NAMES = 8;
      const std::string TYPE_NAMES_ARRAY[TYPE_NAMES] =
      {
        tint::BasicType(),
        treal::BasicType(),
        tbool::BasicType(),
        tstring::BasicType(),
        mtint::BasicType(),
        mtreal::BasicType(),
        mtbool::BasicType(),
        mtstring::BasicType()
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
definition of atrange type mapping function

*/

ListExpr atrangeTypeMappingFunction(ListExpr arguments)
{
  ListExpr type = NList::typeError("Expecting a t type or a mt type "
                                   "and a rectangle or "
                                   "a rectangle, an instant and an instant.");

  NList argumentsList(arguments);

  if(argumentsList.hasLength(2))
  {
    std::string argument1 = argumentsList.first().str();
    std::string argument2 = argumentsList.second().str();

    if((IstType(argument1) ||
        IsmtType(argument1)) &&
        argument2 == Rectangle<2>::BasicType())
    {
      type = NList(argument1).listExpr();
    }
  }

  else if(argumentsList.hasLength(4))
  {
    std::string argument1 = argumentsList.first().str();
    std::string argument2 = argumentsList.second().str();
    std::string argument3 = argumentsList.third().str();
    std::string argument4 = argumentsList.fourth().str();

    if(IsmtType(argument1) &&
       argument2 == Rectangle<2>::BasicType() &&
       argument3 == Instant::BasicType() &&
       argument4 == Instant::BasicType())
    {
      type = NList(argument1).listExpr();
    }
  }

  return type;
}

}
