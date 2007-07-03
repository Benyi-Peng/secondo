/*
---- 
This file is part of SECONDO.

Copyright (C) 2004-2007, University in Hagen, Faculty of Mathematics and Computer Science, 
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

May 2007, M. Spiekermann. Initial version.

This file contains some functions which can be used for simple type mappings.

*/

#ifndef SEC_TYPEMAP_UTILS
#define SEC_TYPEMAP_UTILS


#include <string>
#include <sstream>

#include "CharTransform.h"
#include "NestedList.h"
#include "NList.h"

using namespace std;

namespace mappings {

bool
CheckSimpleMap(const string map[], size_t n, ListExpr args, ListExpr& res);

ListExpr
SimpleMap(const string map[], size_t n, ListExpr arg);

template<int n, int m>
ListExpr
SimpleMaps(const string map[n][m], ListExpr args) 
{ 
  ListExpr res;	
  bool ok = false;	
  for(size_t i = 0; i < n; i++ ) {
    ok = CheckSimpleMap(map[i], m, args, res);
    if (ok)
      return res;	    
  }
  stringstream err;
  err << "None of " << n << " alternative mappings matches!";
  // to do: more precise error messages
  return NList().typeError(err.str());
}	

template<int n, int m>
int
SimpleSelect(const string map[n][m], ListExpr args) 
{ 	
  ListExpr res; // dummy arg	
  bool ok = false;	
  for(size_t i = 0; i < n; i++ ) {
    ok = CheckSimpleMap(map[i], m, args, res);
    if (ok)
      return i;	    
  }
  string err="SimpleSelect failed!";
  // to do: more precise error messages
  NList().typeError(err);
  return -1;
}

}

#endif
