/*
----
This file is part of SECONDO.

Copyright (C) 2021, University in Hagen, Department of Computer Science,
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

//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]

[1] Association Analysis Algebra Implementation

January 2021 - April 2021, P. Fedorow for bachelor thesis.

*/

#include "Common.h"

#include "Algebras/Collection/IntSet.h"
#include "NList.h"
#include "StandardTypes.h"

namespace AssociationAnalysis {
ListExpr frequentItemsetTupleType() {
  NList attrs =
      NList(NList(NList().symbolAtom("Itemset"),
                  NList().symbolAtom(collection::IntSet::BasicType())),
            NList(NList().symbolAtom("Support"),
                  NList().symbolAtom(CcReal::BasicType())));
  ListExpr type = NList().tupleOf(attrs).listExpr();
  return type;
}

// Type mapping for a frequent itemset mining operator.
ListExpr mineTM(ListExpr args) {
  NList type(args);

  bool relativeSupport = false;
  NList attrs;
  if (type.length() == 3) {
    if (!type.elem(1).first().checkRel(attrs)) {
      return NList::typeError(
          "Argument number 1 must be of type rel(tuple(...)).");
    }
    if (!type.elem(2).isSymbol(1)) {
      return NList::typeError("Argument number 2 must name an attribute in the "
                              "relation given as the first argument.");
    }
    if (type.elem(3).first().isSymbol(CcInt::BasicType())) {
      if (type.elem(3).second().intval() <= 0) {
        return NList::typeError("Argument number 3 must be of type int and > 0 "
                                "or of type real and in the interval (0, 1).");
      }
    } else if (type.elem(3).first().isSymbol(CcReal::BasicType())) {
      if (type.elem(3).second().realval() <= 0.0 ||
          type.elem(3).second().realval() >= 1.0) {
        return NList::typeError("Argument number 3 must be of type int and > 0 "
                                "or of type real and in the interval (0, 1).");
      } else {
        relativeSupport = true;
      }
    } else {
      return NList::typeError("Argument number 3 must be of type int and > 0 "
                              "or of type real and in the interval (0, 1).");
    }
    if (!type.elem(3).first().isSymbol(CcInt::BasicType()) ||
        type.elem(3).second().intval() <= 0) {
    }
  } else {
    return NList::typeError("3 arguments expected but " +
                            std::to_string(type.length()) + " received.");
  }

  std::string itemsetAttrName = type.elem(2).first().str();
  int itemsetAttr = -1;
  for (int i = 1; i <= (int)attrs.length(); i += 1) {
    NList attr = attrs.elem(i);
    if (attr.elem(1).isSymbol(itemsetAttrName)) {
      itemsetAttr = i;
    }
  }

  if (itemsetAttr == -1) {
    return NList::typeError("Argument number 2 must name an attribute in the "
                            "relation given as the first argument.");
  }

  NList tupleType = NList(frequentItemsetTupleType());
  return NList(Symbols::APPEND(),
               NList(NList().intAtom(itemsetAttr - 1),
                     NList().boolAtom(relativeSupport)),
               NList().streamOf(tupleType))
      .listExpr();
}
} // namespace AssociationAnalysis