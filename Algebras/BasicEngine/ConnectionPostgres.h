/*
----
This file is part of SECONDO.

Copyright (C) 2021,
Faculty of Mathematics and Computer Science,
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

//[$][\$]

@author
c. Behrndt

@note
Checked - 2020

@history
Version 1.0 - Created - C.Behrndt - 2020

*/
#ifndef _ConnectionPG_H_
#define _ConnectionPG_H_

#include "Attribute.h"
#include "NestedList.h"
#include "StandardTypes.h"
#include "Algebra.h"
#include "Stream.h"
#include "Algebras/Relation-C++/OperatorConsume.h"
#include "Algebras/FText/FTextAlgebra.h"
#include "WinUnix.h"

#include "ConnectionGeneric.h"
#include "ResultIteratorPostgres.h"

#include <string>
#include <boost/log/trivial.hpp>

#include <boost/algorithm/string.hpp>
#include <bits/stdc++.h>
#include <postgres.h>
#include <libpq-fe.h>
#include <catalog/pg_type.h>

namespace BasicEngine {

/*
5 Class ~ConnectionPG~

This class represents the controling from the system.

*/
class ConnectionPG : public ConnectionGeneric {

  public:

  /*
  5.1 Public Methods

  */
  ConnectionPG(const std::string &_dbUser, const std::string &_dbPass, 
       const int _dbPort, const std::string &_dbName);

  virtual ~ConnectionPG();

  std::string getDbType() {
    return DBTYPE;
  }

  bool sendCommand(const std::string &command, bool print = true);
  
  bool createConnection();

  bool checkConnection();

  bool validateQuery(const std::string &query);

  std::string getCreateTableSQL(const std::string &table);

  std::string getDropTableSQL(const std::string &table) {
    return "DROP TABLE IF EXISTS " + table + ";";
  }

  std::string getDropIndexSQL(const std::string& table, 
    const std::string &column) {
    return "DROP INDEX IF EXISTS " + table + "_idx;";
  }

  std::string getCreateGeoIndexSQL(const std::string &table, 
    const std::string &geo_col) {

    return "CREATE INDEX " + table + "_idx ON"
                " " + table + " USING GIST (" + geo_col + ");";
  }

  bool partitionRoundRobin(const std::string &table, 
    const std::string &key, const size_t anzSlots, 
    const std::string &targetTab);

  std::string getPartitionHashSQL(const std::string &table, 
    const std::string &key, const size_t anzSlots, 
    const std::string &targetTab);

  std::string getPartitionSQL(const std::string &table, 
    const std::string &keyS, const size_t anzSlots,
    const std::string &fun, const std::string &targetTab);

  std::string getPartitionGridSQL(const std::string &table,
    const std::string &key, const std::string &geo_col, 
    const size_t anzSlots, const std::string &x0, 
    const std::string &y0, const std::string &size, 
    const std::string &targetTab);

  std::string getExportDataSQL(const std::string &table, 
    const std::string &join_table, const std::string &key, 
    const std::string &nr, const std::string &path,
    size_t numberOfWorker);

  std::string getImportTableSQL(
        const std::string &table, const std::string &full_path);

  std::string getExportTableSQL(
        const std::string &table, const std::string &full_path);

  std::string getFilenameForPartition(const std::string &table, 
    const std::string &number) {

    return table + "_" + std::to_string(WinUnix::getpid())
      + "_" + number + ".bin";
  }

  std::string getCreateTabSQL(const std::string &table, 
    const std::string &query) {

    return "CREATE TABLE " + table + " AS ("+ query + ")";
  }

  std::string getCopySchemaSQL(const std::string &table) {
    return "SELECT * FROM " + table + " LIMIT 0";
  }

  std::string getRenameTableSQL(const std::string &source, 
        const std::string &destination) {
  
    return "ALTER TABLE " + source + " RENAME TO " + destination + ";";
  }

  std::vector<std::tuple<std::string, std::string>> getTypeFromSQLQuery(
        const std::string &sqlQuery);

  ResultIteratorGeneric* performSQLSelectQuery(const std::string &sqlQuery);

  // The DB Type
  inline static const std::string DBTYPE = "pgsql";

  private:

  /*
  5.2 Members

  5.2.1 ~conn~

  The connection to PostgreSQL

  */
  PGconn* conn = nullptr;

  PGresult* sendQuery(const std::string &query);

  bool createFunctionRandom(const std::string &table, 
    const std::string &key, const size_t numberOfWorker, 
    std::string &select);

  bool createFunctionDDRandom(const std::string &table, 
    const std::string &key, const std::string &numberOfWorker, 
    const std::string &select);

  void getFieldInfoFunction(const std::string &table, 
    const std::string &key, std::string &fields, 
    std::string &valueMap, std::string &select);

  std::string getjoin(const std::string &key);

  std::vector<std::tuple<std::string, std::string>> getTypeFromQuery(
      PGresult* res);
};

}; /* namespace BasicEngine */
#endif //_ConnectionPG_H_
