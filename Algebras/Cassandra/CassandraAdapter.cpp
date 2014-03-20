/*
 ----
 This file is part of SECONDO.

 Copyright (C) 2014, University in Hagen, Faculty of Mathematics and
 Computer Science, Database Systems for New Applications.

 SECONDO is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by
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

 Jan Kristof Nidzwetzki

 //paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
 //paragraph [10] Footnote: [{\footnote{] [}}]
 //[TOC] [\tableofcontents]

 [TOC]

 0 Overview

 1 Includes and defines

*/

#include <string.h>
#include <iostream>

#include <cassert>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include <cql/cql.hpp>
#include <cql/cql_error.hpp>
#include <cql/cql_event.hpp>
#include <cql/cql_connection.hpp>
#include <cql/cql_session.hpp>
#include <cql/cql_cluster.hpp>
#include <cql/cql_builder.hpp>
#include <cql/cql_execute.hpp>
#include <cql/cql_result.hpp>
#include <cql/cql_set.hpp>

#include "CqlSingleNodeLoadbalancing.h"
#include "CassandraAdapter.h"

using namespace std;

//namespace to avoid name conflicts
namespace cassandra {

// Init static variables
const string CassandraAdapter::METADATA_TUPLETYPE = "_TUPLETYPE";

void CassandraAdapter::connect() {
    try {

        // Connect to cassandra cluster
        builder -> add_contact_point(
            boost::asio::ip::address::from_string(contactpoint)
        );

        // Set our single node policy
        builder -> with_load_balancing_policy( 
                boost::shared_ptr< cql::cql_load_balancing_policy_t >( 
                     new SingleNodeLoadBlancing(contactpoint)));
        
        cluster = builder -> build();
        session = cluster -> connect();
        
        // Switch keyspace
        session -> set_keyspace(keyspace);
       
        cout << "Cassandra: Connection successful established" << endl;
        cout << "You are connected to host " << contactpoint
                 << " keyspace " << keyspace << endl;
       
    } catch(std::exception& e) {
        cerr << "Got exception while connection to cassandra: "
             << e.what() << endl;

        disconnect();
    }
}

bool CassandraAdapter::isConnected() {
      if(session) {
        return true;
      } else {
        return false;
      }
}

void CassandraAdapter::writeDataToCassandra(string key, string value,
        string relation, string consistenceLevel, bool sync) {

    // Convert consistence level
    cql::cql_consistency_enum consitence 
       = CassandraHelper::convertConsistencyStringToEnum(consistenceLevel);
       
    // Write Data and wait for result
    if(sync) {
      executeCQLSync(
          getInsertCQL(key, value, relation),
          consitence
      );
      
    } else {
      executeCQLASync(
          getInsertCQL(key, value, relation),
          consitence
      );
    }
}

void CassandraAdapter::writeDataToCassandraPrepared(string key, string value,
        string relation, string consistenceLevel, bool sync) {

    // Statement unknown? => Prepare
    if(insertCQLid.empty()) {
       prepareCQLInsert(relation, consistenceLevel);
       
       if(insertCQLid.empty()) {
         cout << "Unable to prepare CQL Insert statement" << endl;
         return;
       }
    }
    
    try {
    // Use prepared query for execution
    boost::shared_ptr<cql::cql_execute_t> boundCQLInsert(
            new cql::cql_execute_t(
              insertCQLid,         
              CassandraHelper::convertConsistencyStringToEnum(consistenceLevel)
    ));

    // Bound prameter
    boundCQLInsert -> push_back(key);
    boundCQLInsert -> push_back(value);
    
    // Build future and execute
    boost::shared_future<cql::cql_future_result_t> future 
       = session->execute(boundCQLInsert);
    
    // Execution sync or async?
    if(sync) {
       executeCQLFutureSync(future);
    } else {
      pendingFutures.push_back(future);
      removeFinishedFutures();
    }
    
    } catch(std::exception& e) {
        cerr << "Got exception executing perpared cql query: " 
             << e.what() << endl;
    }
}

bool CassandraAdapter::prepareCQLInsert(string relation, 
                                        string consistenceLevel) {
     try {
        string cqlQuery = getInsertCQL("?", "?", relation);
        cout << "Preparing insert query: "  << cqlQuery << endl;
        
        boost::shared_ptr<cql::cql_query_t> unboundInsert(
            new cql::cql_query_t(cqlQuery, 
            CassandraHelper::convertConsistencyStringToEnum
                   (consistenceLevel))
        );
            
        // Prepare CQL
        boost::shared_future<cql::cql_future_result_t> future 
            = session->prepare(unboundInsert);
            
        // Wait for result
        future.wait();
        
        if(future.get().error.is_err()) {
                cerr << "Unable to prepare Insert CQL " << endl;
                cerr << "Error is " << future.get().error.message << endl;
        } else {
            insertCQLid = future.get().result->query_id();
            return true;
        }
    } catch(std::exception& e) {
        cerr << "Got exception while preparing cql query: " 
             << e.what() << endl;
    }
    
    return false;
}

string CassandraAdapter::getInsertCQL(string key, string value, 
                                      string relation) {
    stringstream ss;
    ss << "INSERT INTO ";
    ss << relation;
    ss << " (key, value)";
    ss << " VALUES (";
    
    string quote = "'";
    
    // Prepared statemnt? No quoting! 
    if(key.compare("?") == 0 && value.compare("?") == 0) {
      quote = "";
    }
    
    ss << quote << key << quote << ", ";
    ss << quote << value << quote << ");";
    
    return ss.str();
}

CassandraResult* CassandraAdapter::getAllTables(string keyspace) {

    stringstream ss;
    ss << "SELECT columnfamily_name FROM ";
    ss << "system.schema_columnfamilies WHERE keyspace_name='";
    ss << keyspace;
    ss << "';";
    
  return readDataFromCassandra(ss.str(), cql::CQL_CONSISTENCY_ONE);
}


bool CassandraAdapter::getTupleTypeFromTable(string relation, string &result) {
    stringstream ss;
    ss << "SELECT value FROM ";
    ss << relation;
    ss << " WHERE key = '";
    ss << CassandraAdapter::METADATA_TUPLETYPE;
    ss << "';";
    string query = ss.str();
    cout << "Query: " << query << endl;
    
    // Execute query
    CassandraResult* cassandraResult = 
       readDataFromCassandra(query, cql::CQL_CONSISTENCY_ONE);
    
    if(cassandraResult == NULL) {
      return false;
    }
    
    if(! cassandraResult->hasNext() ) {
      return false;
    }
    
    cassandraResult -> getStringValue(result, 0);
    
    return true;
}


CassandraResult* CassandraAdapter::readTable(string relation,
        string consistenceLevel) {

    stringstream ss;
    ss << "SELECT key, value from ";
    ss << relation;
    ss << ";";
    string query = ss.str();
    
    return readDataFromCassandra(query, 
            CassandraHelper::convertConsistencyStringToEnum(consistenceLevel));
}

CassandraResult* CassandraAdapter::readDataFromCassandra(string cql, 
         cql::cql_consistency_enum consistenceLevel) {
      
    if(! isConnected() ) {
        cerr << "Cassandra session not ready" << endl;
        return NULL;
    }

    try {
        boost::shared_ptr<cql::cql_query_t> cqlStatement(
            new cql::cql_query_t(cql,
            consistenceLevel
        ));

        boost::shared_future<cql::cql_future_result_t> future
            = session->query(cqlStatement);

        // Wait for result
        future.wait();

        if(future.get().error.is_err()) {
            cerr << "Unable to execute " << cqlStatement << endl;
            cerr << "Error is " << future.get().error.message << endl;
        } else {

            cql::cql_result_t& result = *(future.get().result);

            return new CassandraResult(result);
        }
    } catch(std::exception& e) {
        cerr << "Got exception while reading data: " << e.what() << endl;
    }

    return NULL;
}


bool CassandraAdapter::createTable(string tablename, string tupleType) {
    stringstream ss;
    ss << "CREATE TABLE IF NOT EXISTS ";
    ss << tablename;
    ss << " ( key text PRIMARY KEY, ";
    ss << " value text );";

    bool resultCreate = executeCQLSync(ss.str(), cql::CQL_CONSISTENCY_ALL);
    
    // Wait propagation of the table
    sleep(1);
    
    // Write tupletype
    if(resultCreate) {
      bool resultInsert = executeCQLSync(
            getInsertCQL(CassandraAdapter::METADATA_TUPLETYPE, 
                         tupleType, tablename),
            cql::CQL_CONSISTENCY_ALL
      );
      
      if(resultInsert) {
        // New table is created and the 
        // tuple type is stored successfully
        return true; 
      }
    }
    
    return false;
}

bool CassandraAdapter::dropTable(string tablename) {
    stringstream ss;
    ss << "DROP TABLE IF EXISTS ";
    ss << tablename;
    ss << ";";

    return executeCQLSync(ss.str(), cql::CQL_CONSISTENCY_ALL);
}

void CassandraAdapter::disconnect() {
    if(isConnected()) {

        if(! pendingFutures.empty()) {
          cout << "Wait for pending futures: " << flush;
          
          for(vector < boost::shared_future<cql::cql_future_result_t> >
             ::iterator iter = pendingFutures.begin(); 
             iter != pendingFutures.end(); ++iter) {
            
             boost::shared_future<cql::cql_future_result_t> future = *iter;
          
             if(! future.is_ready() ) {
                future.wait();
                cout << "." << flush;
             }
          }
          
          cout << endl;
          
          removeFinishedFutures();
          
          cout << "All futures finished" << endl;
        }
      
        cout << "Disconnecting from cassandra" << endl;

        // Close session and cluster
        session -> close();
        cluster -> shutdown();

        // Reset pointer
        session.reset();
        cluster.reset();
        builder.reset();
    }
}

bool CassandraAdapter::executeCQLSync
(string cql, cql::cql_consistency_enum consistency) {
      try {

        if(! isConnected() ) {
            cerr << "Cassandra session not ready" << endl;
            return false;
        }

        boost::shared_future<cql::cql_future_result_t> future
          = executeCQL(cql, consistency);
          
        return executeCQLFutureSync(future);

      } catch(std::exception& e) {
        cerr << "Got exception while executing cql: " << e.what() << endl;
      }

    return false;
}

bool CassandraAdapter::executeCQLASync
(string cql, cql::cql_consistency_enum consistency) {
      
      try {

        if(! isConnected() ) {
            cerr << "Cassandra session not ready" << endl;
            return false;
        }

        boost::shared_future<cql::cql_future_result_t> future
          = executeCQL(cql, consistency);
          
        pendingFutures.push_back(future);
        
        removeFinishedFutures();
        
        return true;
      
      } catch(std::exception& e) {
        cerr << "Got exception while executing cql: " << e.what() << endl;
      }

    return false;
}

void CassandraAdapter::removeFinishedFutures(bool force) {
  
    // The cleanup is not needed everytime
    if(pendingFutures.size() % 100 != 0 && force == false) {
      return;
    }
  
    // Are some futures finished?
    for(vector < boost::shared_future<cql::cql_future_result_t> >
      ::iterator iter = pendingFutures.begin(); 
      iter != pendingFutures.end(); 
      ) {
      
      boost::shared_future<cql::cql_future_result_t> future = *iter;
      
      // Remove finished futures
      if(future.is_ready()) {
        
        if(future.get().error.is_err()) {
          cerr << "Got error while executing future: " 
                << future.get().error.message << endl;
        }
        
        iter = pendingFutures.erase(iter);
      } else {
        ++iter;
      }
    }
}

bool CassandraAdapter::executeCQLFutureSync
(boost::shared_future<cql::cql_future_result_t> cqlFuture) {

    try {

        if(! isConnected() ) {
            cerr << "Cassandra session not ready" << endl;
            return false;
        }

        // Wait for execution
        cqlFuture.wait();

        if(cqlFuture.get().error.is_err()) {
            cerr << "Error is " << cqlFuture.get().error.message << endl;
            return false;
        }

        return true;

    } catch(std::exception& e) {
        cerr << "Got exception while executing cql future: " 
             << e.what() << endl;
    }

    return false;
}

boost::shared_future<cql::cql_future_result_t>
CassandraAdapter::executeCQL
(string cql, cql::cql_consistency_enum consistency) {


    boost::shared_ptr<cql::cql_query_t> cqlStatement(
        new cql::cql_query_t(cql, consistency));

    boost::shared_future<cql::cql_future_result_t> future
    = session->query(cqlStatement);

    return future;
}

bool CassandraAdapter::getTokensFromQuery
    (string query, vector <long> &result) {    
      
    try {
       boost::shared_future< cql::cql_future_result_t > future = 
          executeCQL(query, cql::CQL_CONSISTENCY_ONE);
  
       future.wait();
    
       if(future.get().error.is_err()) {
         cerr << "Unable to fetch local token list" << endl;
         return false;
       }
    
       cql::cql_result_t& cqlResult = *(future.get().result);
    
       while(cqlResult.next()) {
         cql::cql_set_t* setResult = NULL;
         cqlResult.get_set("tokens", &setResult);
      
         if(setResult!=NULL) {
             for (size_t i = 0; i < setResult->size(); ++i) {
                string token;
                setResult->get_string(i, token);
               result.push_back(atol(token.c_str()));
             }
    
             delete setResult;
         }
       }
     } catch(std::exception& e) {
        cerr << "Got exception while executing cql: " << e.what() << endl;
        return false;
     }
   
     return true;
}

bool CassandraAdapter::getLocalTokens(vector <long> &result) {
  return getTokensFromQuery("SELECT tokens FROM system.local", result);
}


bool CassandraAdapter::getPeerTokens(vector <long> &result) {
  return getTokensFromQuery("SELECT tokens FROM system.peers", result);
}

}
