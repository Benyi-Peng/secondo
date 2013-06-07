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

#ifndef TILEALGEBRA_TBOOL_H
#define TILEALGEBRA_TBOOL_H

#include "t.h"
#include "tProperties.h"
#include "../Properties/Propertiesbool.h"

namespace TileAlgebra
{

/*
typedef of tbool type

*/

typedef t<char> tbool;

/*
declaration of template class tProperties<char>

*/

template <>
class tProperties<char>
{
  public:

  typedef Properties<char> TypeProperties;
  typedef tgrid gridType;
  typedef tbool tType;
  static int GetDimensionSize();
  static int GetFlobElements();
  static SmiSize GetFlobSize();
  static std::string GetTypeName();
};

}

#endif // TILEALGEBRA_TBOOL_H
