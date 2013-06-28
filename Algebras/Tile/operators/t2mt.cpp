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

#include <cmath>
#include "t2mt.h"
#include "../Index.h"
#include "../grid/tgrid.h"
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
definition of template t2mtFunction

*/

template <typename Type, typename Properties>
int t2mtFunction(Word* pArguments,
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
    datetime::DateTime* pDuration = static_cast<datetime::DateTime*>
                                    (pArguments[1].addr);
    Instant* pInstant1 = static_cast<Instant*>(pArguments[2].addr);
    Instant* pInstant2 = static_cast<Instant*>(pArguments[3].addr);

    if(pType != 0 &&
       pDuration != 0 &&
       pInstant1 != 0 &&
       pInstant2 != 0)
    {
      rResult = qp->ResultStorage(supplier);

      if(rResult.addr != 0)
      {
        typename Properties::PropertiesType* pResult =
        static_cast<typename Properties::PropertiesType*>(rResult.addr);

        if(pResult != 0)
        {
          pResult->SetDefined(false);

          if(pType->IsDefined() &&
             pDuration->IsDefined() &&
             pInstant1->IsDefined() &&
             pInstant2->IsDefined())
          {
            int tDimensionSize = Properties::GetTDimensionSize();
            int startTime = static_cast<int>
                            (round(pInstant1->ToDouble() /
                                   pDuration->ToDouble()));
            int endTime = static_cast<int>
                          (round(pInstant2->ToDouble() /
                                 pDuration->ToDouble()));

            if(startTime >= 0 &&
               startTime < tDimensionSize &&
               endTime >= 0 &&
               endTime < tDimensionSize)
            {
              pResult->SetDefined(true);

              tgrid grid;
              pType->getgrid(grid);

              bool bOK = pResult->SetGrid(grid.GetX(),
                                          grid.GetY(),
                                          grid.GetLength(),
                                          *pDuration);

              if(bOK == true)
              {
                int xDimensionSize = Properties::GetXDimensionSize();
                int yDimensionSize = Properties::GetYDimensionSize();

                for(int time = startTime; time <= endTime; time++)
                {
                  for(int row = 0; row < yDimensionSize; row++)
                  {
                    for(int column = 0; column < xDimensionSize; column++)
                    {
                      typename Properties::TypeProperties::PropertiesType value
                      = Properties::TypeProperties::GetUndefinedValue();

                      Index<2> index2 = (int[]){column, row};
                      value = pType->GetValue(index2);

                      if(Properties::TypeProperties::IsUndefinedValue(value)
                         == false)
                      {
                        Index<3> index3 = (int[]){column, row, time};
                        bOK = pResult->SetValue(index3, value, true);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  return nRetVal;
}

/*
definition of t2mt functions

*/

ValueMapping t2mtFunctions[] =
{
  t2mtFunction<tint, mtProperties<int> >,
  t2mtFunction<treal, mtProperties<double> >,
  t2mtFunction<tbool, mtProperties<char> >,
  t2mtFunction<tstring, mtProperties<string> >,
  0
};

/*
definition of t2mt select function

*/

int t2mtSelectFunction(ListExpr arguments)
{
  int nSelection = -1;

  if(arguments != 0)
  {
    NList argumentsList(arguments);

    if(argumentsList.hasLength(4))
    {
      NList argument1 = argumentsList.first();
      NList argument2 = argumentsList.second();
      NList argument3 = argumentsList.third();
      NList argument4 = argumentsList.fourth();

      if(argument2.isSymbol(Duration::BasicType()) &&
         argument3.isSymbol(Instant::BasicType()) &&
         argument4.isSymbol(Instant::BasicType()))
      {
        const int TYPE_NAMES = 4;
        const std::string TYPE_NAMES_ARRAY[TYPE_NAMES] =
        {
          tint::BasicType(),
          treal::BasicType(),
          tbool::BasicType(),
          tstring::BasicType()
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
  }

  return nSelection;
}

/*
definition of t2mt type mapping function

*/

ListExpr t2mtTypeMappingFunction(ListExpr arguments)
{
  ListExpr type = NList::typeError("Operator t2mt expects "
                                   "a t type, a duration, "
                                   "an instant and an instant.");

  NList argumentsList(arguments);

  if(argumentsList.hasLength(4))
  {
    std::string argument1 = argumentsList.first().str();
    std::string argument2 = argumentsList.second().str();
    std::string argument3 = argumentsList.third().str();
    std::string argument4 = argumentsList.fourth().str();

    if(IstType(argument1) &&
       argument2 == Duration::BasicType() &&
       argument3 == Instant::BasicType() &&
       argument4 == Instant::BasicType())
    {
      type = NList(GetmtType(argument1)).listExpr();
    }
  }

  return type;
}

}
