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

//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]
//[TOC] [\tableofcontents]

[1] Header File of the class ~PregelAlgebra~

November 2018, J. Mende


[TOC]

1 Overview

This file contains definitions of the members of class StartMessageServer

*/

#include <ListUtils.h>
#include <StandardTypes.h>
#include "StartMessageServer.h"
#include "../../MessageBroker/MessageBroker.h"
#include "../../PregelContext.h"


namespace pregel {
 ListExpr StartMessageServer::typeMapping(ListExpr args) {
  if (!nl->HasLength(args, 1)) {
   return listutils::typeError("You must provide 1 argument.");
  }
  const ListExpr portNo = nl->First(args);

  if (!CcInt::checkType(portNo)) {
   return listutils::typeError(
    "The second argument must be of type int");
  }

  return nl->SymbolAtom(CcBool::BasicType());
 }

 int StartMessageServer::valueMapping(Word *args, Word &result, int message,
                                      Word &local, Supplier s) {
  result = qp->ResultStorage(s);
  CcInt *portNo = (CcInt *) args[0].addr;

  PRECONDITION(portNo->IsDefined(), "portNo is undefined");
  int port = portNo->GetValue();
  PRECONDITION(portNo > 0, "port must be greater than 0");

  MessageBroker &broker = MessageBroker::get();

  PregelContext::get().setMessageServerPort(port);
  if (broker.serverMotherRunning()) {
   ((CcBool *) result.addr)->Set(true, false);
  } else {
   bool success = broker.startServerMother(port);
   ((CcBool *) result.addr)->Set(true, success);
  }

  return 0;
 }

 OperatorSpec StartMessageServer::operatorSpec(
  "int -> bool",
  "# (_)",
  "port number -> success",
  "query startMessageServer(9001);"
 );

 Operator StartMessageServer::startMessageServer(
  "startMessageServer",
  StartMessageServer::operatorSpec.getStr(),
  StartMessageServer::valueMapping,
  Operator::SimpleSelect,
  StartMessageServer::typeMapping
 );
}