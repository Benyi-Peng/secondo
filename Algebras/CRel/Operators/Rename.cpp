/*
----
This file is part of SECONDO.

Copyright (C) 2004, University in Hagen, Department of Computer Science,
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

*/

#include "Rename.h"

#include <cstddef>
#include <exception>
#include "ListUtils.h"
#include "LogMsg.h"
#include "QueryProcessor.h"
#include <stdexcept>
#include <string>
#include "StreamValueMapping.h"
#include "Symbols.h"
#include "TBlockTC.h"

using namespace CRelAlgebra::Operators;

using listutils::isValidAttributeName;
using std::exception;
using std::string;

extern NestedList *nl;
extern QueryProcessor *qp;

Rename::Rename() :
  Operator(info, StreamValueMapping<State>, TypeMapping)
{
}

const OperatorInfo Rename::info = OperatorInfo(
  "rename", "crel(c, m, (A)) -> stream(tblock(m, (A)))",
  "_ feed",
  "Produces a stream of tuple-blocks from a column-oriented relation.",
  "query cities feed cconsume");

ListExpr Rename::TypeMapping(ListExpr args)
{
  if (!nl->HasLength(args, 2))
  {
    return listutils::typeError("Expected two arguments.");
  }

  ListExpr stream = nl->First(args);

  if (!nl->HasLength(stream, 2) ||
      !nl->IsEqual(nl->First(stream), Symbols::STREAM()))
  {
    return listutils::typeError("First argument isn't a stream.");
  }

  const ListExpr tblock = nl->Second(stream);
  string typeError;

  if (!TBlockTI::Check(tblock, typeError))
  {
    return listutils::typeError("First argument isn't a stream of tblock: " +
                                typeError);
  }

  const ListExpr suffixExpr = nl->Second(args);

  if (!nl->IsNodeType(SymbolType, suffixExpr))
  {
    return listutils::typeError("Second argument isn't a symbol.");
  }

  const string suffix = nl->SymbolValue(suffixExpr);

  string error;

  TBlockTI blockInfo(tblock);

  const size_t blockAttributeCount = blockInfo.attributeInfos.size();
  for (size_t i = 0; i < blockAttributeCount; ++i)
  {
    const string name = blockInfo.attributeInfos[i].name + "_" + suffix;

    if (!isValidAttributeName(nl->SymbolAtom(name), error))
    {
      return listutils::typeError("Resulting attribute name is not valid: " +
                                  error);
    }

    blockInfo.attributeInfos[i].name = name;
  }

  return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                         blockInfo.GetTypeInfo());
}

Rename::State::State(Word* args, Supplier s) :
  m_stream(args[0])
{
  qp->DeleteResultStorage(s);

  m_stream.open();
}

Rename::State::~State()
{
  m_stream.close();
}

TBlock *Rename::State::Request()
{
  return m_stream.request();
}