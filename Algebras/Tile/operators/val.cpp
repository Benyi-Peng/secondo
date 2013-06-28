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

#include "val.h"
#include "../it/itint.h"
#include "../it/itreal.h"
#include "../it/itbool.h"
#include "../it/itstring.h"

namespace TileAlgebra
{

/*
definition of template valFunction

*/

template <typename Type, typename Properties>
int valFunction(Word* pArguments,
                Word& rResult,
                int message,
                Word& rLocal,
                Supplier supplier)
{
  int nRetVal = 0;

  if(qp != 0 &&
     pArguments != 0)
  {
    Type* pType = static_cast<Type*>(pArguments[0].addr);

    if(pType != 0)
    {
      rResult = qp->ResultStorage(supplier);

      if(rResult.addr != 0)
      {
        typename Properties::tType* pResult = static_cast
                                              <typename Properties::tType*>
                                              (rResult.addr);

        if(pResult != 0)
        {
          pType->val(*pResult);
        }
      }
    }
  }

  return nRetVal;
}

/*
definition of val functions

*/

ValueMapping valFunctions[] =
{
  valFunction<itint, itProperties<int> >,
  valFunction<itreal, itProperties<double> >,
  valFunction<itbool, itProperties<char> >,
  valFunction<itstring, itProperties<std::string> >,
  0
};

/*
definition of val select function

*/

int valSelectFunction(ListExpr arguments)
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
definition of val type mapping function

*/

ListExpr valTypeMappingFunction(ListExpr arguments)
{
  ListExpr type = NList::typeError("Operator val expects an it type.");

  NList argumentsList(arguments);

  if(argumentsList.hasLength(1))
  {
    std::string argument1 = argumentsList.first().str();

    if(IsitType(argument1))
    {
      type = NList(GettType(argument1)).listExpr();
    }
  }

  return type;
}

}
