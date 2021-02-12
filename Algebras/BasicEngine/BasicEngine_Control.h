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
#ifndef _BasicEngine_Control_H_
#define _BasicEngine_Control_H_

#include "Algebras/Relation-C++/RelationAlgebra.h"
#include "BasicEngine_Thread.h"
#include "ConnectionGeneric.h"
#include "ResultIteratorGeneric.h"
#include "StandardTypes.h"

namespace BasicEngine {

/*
2 Class ~BasicEngine\_Control~

This class represents the controling from the system.

*/
class BasicEngine_Control {

public:

/*
2.1 Public Methods

*/
  BasicEngine_Control(ConnectionGeneric* _dbms_connection) 
    : dbms_connection(_dbms_connection), worker(NULL), numberOfWorker(0) {
    
  }

  BasicEngine_Control(ConnectionGeneric* _dbms_connection, Relation* _worker) 
    : dbms_connection(_dbms_connection) {

    worker = _worker->Clone();
    numberOfWorker = worker->GetNoTuples();
    createAllConnections();
  }

  virtual ~BasicEngine_Control();

  ConnectionGeneric* getDBMSConnection() {
    return dbms_connection;
  }

  bool checkAllConnections();

  bool partTable(std::string tab, std::string key, std::string art,
     size_t slotnum, std::string geo_col = "", float x0 = 0, float y0 = 0,
     float slotsize = 0);

  bool drop_table(std::string tab) {
    return sendCommand(dbms_connection->get_drop_table(&tab), false);
  }

  bool createTabFile(std::string tab);

  bool copy(std::string tab, std::string full_path, bool direct) {
    return sendCommand(dbms_connection->get_copy(&tab, &full_path, &direct));
  }

  bool createTab(std::string tab, std::string query) {
     return sendCommand(dbms_connection->getCreateTabSQL(&tab, &query));
  }

  bool munion(std::string tab);

  bool mquery(std::string query, std::string tab);

  bool mcommand(std::string query);

  bool runsql(std::string filepath);

  bool shutdownWorker();

  bool sendCommand(std::string query, bool print=true) {
    return dbms_connection->sendCommand(&query, print);
  }

  bool getTypeFromSQLQuery(std::string sqlQuery, ListExpr &resultList);

  ResultIteratorGeneric* performSQLQuery(std::string sqlQuery);

  bool shareWorkerRelation(Relation* relation);

  bool isMaster() {
    return master;
  }

  void setMaster(bool masterState) {
    master = masterState;
  }

private:

/*
2.2 Members

2.2.1 ~dbs\_conn~

In this template variable were stores the connection,
to a secondary dbms (for example postgresql)

*/
ConnectionGeneric* dbms_connection;

/*
2.2.2 ~worker~

The worker is a relation with all informations about the
worker connection like port, connection-file, ip

*/
Relation* worker;

/*
2.2.3 ~connections~

In this vector all connection to the worker are stored.

*/
std::vector<distributed2::ConnectionInfo*> connections;

/*
2.2.4 ~importer~

In this vector all informations for starting the thread
are stored.

*/
std::vector<BasicEngine_Thread*> importer;

/*
2.2.4 ~numberOfWorker~

The numberOfWorker counts the number of worker.

*/
size_t numberOfWorker;

/*
2.2.5 master is a variable which shows, if this system is a master (true)
or a worker(false).

*/
bool master = false;

/*
2.3 Private Methods

*/
  bool createAllConnections();

  bool createConnection(std::string host, std::string port, 
    std::string config, std::string dbPort, std::string dbName);

  bool partRoundRobin(std::string* tab, std::string* key, size_t slotnum);

  bool partHash(std::string* tab, std::string* key, size_t slotnum);

  bool partFun(std::string* tab, std::string* key,
         std::string* fun, size_t slotnum);

  bool partGrid(std::string* tab, std::string* key, std::string* geo_col,
         size_t slotnum, float* x0, float* y0, float* slotsize);

  bool exportData(std::string* tab, std::string* key,
         size_t slotnum);

  bool importData(std::string* tab);

  bool exportToWorker(std::string* tab);

  std::string createTabFileName(std::string* tab) {
    return "create" + *tab + ".sql";
  }

  std::string get_partFileName(std::string* tab, std::string* nr) {
    return dbms_connection->get_partFileName(tab,nr);
  }

  std::string getFilePath() {
    return std::string("/home/") + getenv("USER") + "/filetransfer/";
  }

  std::string getparttabname(std::string* tab, std::string* key);

};
};  /* namespace BasicEngine */

#endif //_BasicEngine_Control_H_
