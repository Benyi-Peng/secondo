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

#include "Csv.h"

#include "Common.h"

#include "Algebras/FText/FTextAlgebra.h"
#include "StandardTypes.h"

#include <fstream>
#include <iostream>

namespace AssociationAnalysis {
// Loads transactions from a csv.
csvLoadTransactionsLI::csvLoadTransactionsLI(std::string path) {
  std::fstream file;
  file.open(path, std::ios::in);
  if (file.is_open()) {
    std::string line;
    int id = 0;
    while (std::getline(file, line)) {
      std::istringstream lineStream(line);
      Transaction transaction;
      std::string item;
      while (std::getline(lineStream, item, ' ')) {
        try {
          transaction.itemset.insert(std::stoi(item));
        } catch (const std::exception &e) {
          continue;
        }
      }
      if (!transaction.itemset.empty()) {
        transaction.id = id;
        id += 1;
        this->transactions.push_back(transaction);
      }
    }
  }

  this->it = this->transactions.cbegin();

  // Setup resulting tuple type.
  this->tupleType = new TupleType(
      SecondoSystem::GetCatalog()->NumericType(transactionsTupleType()));
}

// Returns the next transactions as a tuple.
Tuple *csvLoadTransactionsLI::getNext() {
  if (this->it != this->transactions.cend()) {
    auto &transaction = *this->it;
    auto tuple = new Tuple(this->tupleType);
    tuple->PutAttribute(0, new CcInt(transaction.id));
    tuple->PutAttribute(1, new collection::IntSet(transaction.itemset));
    this->it++;
    return tuple;
  } else {
    return nullptr;
  }
}

// Type mapping for the csvLoadTransactions operator.
ListExpr csvLoadTransactionsTM(ListExpr args) {
  NList type(args);

  if (type.length() == 1) {
    const NList &arg = type.elem(1);
    if (!arg.first().isSymbol(FText::BasicType())) {
      return NList::typeError("The path argument must be of type text.");
    }
  } else {
    return NList::typeError("1 argument expected but " +
                            std::to_string(type.length()) + " received.");
  }

  NList tupleType = NList(transactionsTupleType());
  return NList().streamOf(tupleType).listExpr();
}

// Value mapping for the csvLoadTransactions operator.
int csvLoadTransactionsVM(Word *args, Word &result, int message, Word &local,
                          Supplier s) {
  auto *li = (csvLoadTransactionsLI *)local.addr;
  switch (message) {
  case OPEN: {
    delete li;
    std::string path = ((FText *)args[0].addr)->GetValue();
    local.addr = new csvLoadTransactionsLI(path);
    return 0;
  }
  case REQUEST:
    result.addr = li ? li->getNext() : nullptr;
    return result.addr ? YIELD : CANCEL;
  case CLOSE:
    delete li;
    local.addr = nullptr;
    return 0;
  default:
    return 0;
  }
}

ListExpr csvSaveTransactionsTM(ListExpr args) {
  NList type(args);

  NList attrs;
  if (type.length() == 3) {
    if (!type.elem(1).first().checkStreamTuple(attrs)) {
      return NList::typeError(
          "Argument number 1 must be of type stream(tuple(...)).");
    }
    if (!type.elem(3).first().isSymbol(FText::BasicType())) {
      return NList::typeError("Argument number 3 must be of type text.");
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
                            "tuple stream given as the first argument.");
  }

  return NList(Symbols::APPEND(), NList().intAtom(itemsetAttr - 1).enclose(),
               NList().symbolAtom(CcBool::BasicType()))
      .listExpr();
}

int csvSaveTransactionsVM(Word *args, Word &result, int message, Word &local,
                          Supplier s) {
  result = qp->ResultStorage(s);
  auto success = (CcBool *)result.addr;
  auto transactions = new Stream<Tuple>(args[0]);
  std::string path = ((FText *)args[2].addr)->GetValue();
  int itemsetAttr = ((CcInt *)args[3].addr)->GetIntval();
  (void)success;
  (void)transactions;
  (void)path;
  (void)itemsetAttr;

  std::fstream file;
  file.open(path, std::ios::out);
  if (file.is_open()) {
    transactions->open();
    Tuple *t;
    while ((t = transactions->request())) {
      auto itemset = (collection::IntSet *)t->GetAttribute(itemsetAttr);
      for (std::size_t i = 0; i < itemset->getSize(); i += 1) {
        file << itemset->get(i);
        if (i + 1 != itemset->getSize()) {
          file << ' ';
        }
      }
      file << '\n';
    }
    transactions->close();
    success->Set(true, true);
  } else {
    success->Set(true, false);
  }
  return 0;
}
} // namespace AssociationAnalysis
