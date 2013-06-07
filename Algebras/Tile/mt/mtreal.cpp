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

#include "mtreal.h"

namespace TileAlgebra
{

/*
implementation of template class mtProperties<double>

*/

int mtProperties<double>::GetDimensionSize()
{
  int dimensionSize = static_cast<unsigned int>
                      (std::pow((WinUnix::getPageSize() -
                                 sizeof(mtgrid) -
                                 2 * sizeof(double)) /
                                 sizeof(double),
                                 1.0 / 3.0)
                      );

  return dimensionSize;
}

int mtProperties<double>::GetFlobElements()
{
  int nFlobElements = static_cast<unsigned int>
                      (std::pow(GetDimensionSize(), 3));

  return nFlobElements;
}

SmiSize mtProperties<double>::GetFlobSize()
{
  SmiSize flobSize = GetFlobElements() * sizeof(double);

  return flobSize;
}

std::string mtProperties<double>::GetTypeName()
{
  return TYPE_NAME_MTREAL;
}

}
