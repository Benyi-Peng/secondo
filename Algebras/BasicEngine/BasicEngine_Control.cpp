/*
----
This file is part of SECONDO.

Copyright (C) 2018,
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
#include "BasicEngine_Control.h"
#include "FileSystem.h"

using namespace distributed2;
using namespace std;

namespace BasicEngine {

/*
3 Class ~BasicEngine\_Control~

Implementation.

3.1 ~createConnection~

Creating a specified and saves it in the vec\_ci. Additionally add an entry
to the importer vector.

*/
bool BasicEngine_Control::createConnection(long unsigned int* index) {

  NestedList* mynl = new NestedList("temp_nested_list");
  const int defaultTimeout = 0;

  bool val = false;
  string host;
  string config;
  string port;
  string dbName;
  string dbPort;
  string errMsg;
  int err = 0;
  double rt;
  CommandLog CommandLog;
  string res;
  SecondoInterfaceCS* si ;
  ConnectionInfo* ci;
  Tuple* tuple;
  GenericRelationIterator* it = worker->MakeScan();

  tuple = it->GetNthTuple(*index+1,false);
  si = new SecondoInterfaceCS(true,mynl, true);
  host = tuple->GetAttribute(0)->toText();
  port = tuple->GetAttribute(1)->toText();
  config = tuple->GetAttribute(2)->toText();
  dbPort = tuple->GetAttribute(3)->toText();
  dbName = tuple->GetAttribute(4)->toText();

  if (si->Initialize("", "", host, port, config,"", errMsg, true)) {
    ci = new ConnectionInfo(host,stoi(port),config,si,mynl,0,defaultTimeout);
    if(ci){
        vec_ci.push_back(ci);
        importer.push_back(new BasicEngine_Thread(vec_ci[*index]));

        val=ci->switchDatabase(dbName, true, false, true, defaultTimeout);

        ci->simpleCommand(dbs_conn->get_init(&dbName,&dbPort),err,res,false
            ,rt,false,CommandLog,true,defaultTimeout);
        if(err != 0) {
          cout << std::string("ErrCode:" + err) << endl;
        } else {
          val = (res == "(bool TRUE)") && val;
        }
    }
  } else {
        cout << std::string("Couldn't connect to secondo-Worker on host"
        "" + host + " with port " + port + "!\n") << endl;
  }

  if(it != NULL) {
    delete it;
    it = NULL;
  }

  if(tuple != NULL) {
    tuple->DeleteIfAllowed();
  }

  return val;
}

/*
3.2 Destructor

*/
BasicEngine_Control::~BasicEngine_Control() {
    if(dbs_conn != NULL) {
      delete dbs_conn;
      dbs_conn = NULL;
    }

    // Delete importer
    for(const BasicEngine_Thread* basic_engine_thread: importer) {
      delete basic_engine_thread;
    }
    importer.clear();

    // Delete connections
    for(const distributed2::ConnectionInfo* connection: vec_ci) {
      delete connection;
    }
    vec_ci.clear();

    // Delete cloned worker relation
    if(worker != NULL) {
      worker -> Delete();
      worker = NULL;
    }
  }

/*
3.2 ~createAllConnection~

Creating all connection from the worker relation.

*/
bool BasicEngine_Control::createAllConnection(){
  bool val = true;
  long unsigned int i = 0;

  while (i < anzWorker and val){
    val = createConnection(&i);
    i++;
  }

  return val;
}

/*
3.5 ~getparttabname~

Returns a name of a table with the keys included.

*/
string BasicEngine_Control::getparttabname(string* tab, string* key){

  string usedKey(*key);

  boost::replace_all(usedKey, ",", "_");
  string res = *tab + "_" + usedKey;
  boost::replace_all(res, " ", "");

  return res;
}

/*
3.7 ~createTabFile~

Creates a table create statement from the input tab
and store the statement in a file.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::createTabFile(string tab) {

  ofstream write;
  string statement;
  bool val = false;

  statement = dbs_conn->createTabFile(&tab);

  if (statement.length() > 0){
    write.open(getFilePath() + createTabFileName(&tab));
    if (write.is_open()){
      write << statement;
      write.close();
      val = write.good();
    }else{ cout << "Couldn't write file into " + getFilePath() + ""
      ". Please check the folder and permissions." << endl;}
  } else { 
     cout << "Table " + tab + " not found." << endl;
  }

   return val;
}

/*
3.8 ~partRoundRobin~

The data were partitions in the database by round robin.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::partRoundRobin(string* tab,
                    string* key, int* slotnum) {

  bool val = false;
  string query_exec = "";
  string partTabName;
  string anzSlots = to_string(*slotnum);

  partTabName = getparttabname(tab,key);
  drop_table(partTabName);

  query_exec = dbs_conn->get_partRoundRobin(tab, key,
      &anzSlots, &partTabName);
  
  if (query_exec != "") {
    val = dbs_conn->sendCommand(&query_exec);
  }

  return val;
}

/*
3.9 ~exportToWorker~

The data of a partitions table were sanded to the worker
and after that imported by the worker.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::exportToWorker(string *tab){
bool val = true;
string query_exec;
long unsigned int index = 0;
string remoteName;
string localName;
string importPath;
string strindex;
SecondoInterfaceCS* si ;

string remoteCreateName = createTabFileName(tab);
string localCreateName = getFilePath() + remoteCreateName;

  while(index < anzWorker and val){
    if (vec_ci[index]){
      si = vec_ci[index]->getInterface();

      //sending data
      strindex = to_string(index+1);
      remoteName = get_partFileName(tab,&strindex);
      localName = getFilePath() + remoteName;
      val = (si->sendFile(localName, remoteName, true) == 0);
      val = (remove(localName.c_str()) == 0) && val;
      if (!val){
        cout << "\n Couldn't send the data to the worker." << endl;
        return val;}

      //sending create Table
      val = (si->sendFile(localCreateName, remoteCreateName, true) == 0);
      if (!val){
        cout << "\n Couldn't send the structure-file "
            "to the worker." << endl;
        return val;}

      index++;
      } else{
        createConnection(&index);
        if(!vec_ci[index]) val = false; ;
      }
  };

  if(val){
    val = (remove(localCreateName.c_str()) == 0);
    //doing the import with one thread for each worker
    for(size_t i=0;i<importer.size();i++){
      strindex = to_string(i+1);
      remoteName =get_partFileName(tab,&strindex);
      importer[i]->startImport(*tab,remoteCreateName,remoteName);
    }

    //waiting for finishing the threads
    for(size_t i=0;i<importer.size();i++){
      val = importer[i]->getResult() && val;
    }
  }else cout << "\n Something goes wrong with the"
  " export or the transfer." << endl;

  if(!val) cout << "\n Something goes wrong with the "
  "import at the worker." << endl;
return val;
}

/*
3.10 ~partHash~

The data were partitions in the database by an hash value.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::partHash(string* tab
                    , string* key, int* slotnum){
bool val = false;
string query_exec = "";
string partTabName;
string anzSlots = to_string(*slotnum);

  partTabName = getparttabname(tab,key);
  drop_table(partTabName);

  query_exec = dbs_conn->get_partHash(tab,key
    ,&anzSlots,&partTabName);
  if (query_exec != "")  val = dbs_conn->sendCommand(&query_exec);

return val;
}

/*
3.11 ~partFun~

The data were partitions in the database by an defined function.
This function have to be defined before using it.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::partFun(string* tab
                    , string* key,string* fun, int* slotnum){
bool val = false;
string query_exec = "";
string partTabName = getparttabname(tab,key);
string anzSlots;;

  drop_table(partTabName);

  if (boost::iequals(*fun, "share")){
    anzSlots = to_string(anzWorker);
  }else{
    anzSlots = to_string(*slotnum);
  }

  query_exec = dbs_conn->get_partFun(tab,key
      ,&anzSlots,fun,&partTabName);

  if (query_exec != "") val = dbs_conn->sendCommand(&query_exec);

return val;
}

/*
3.12 ~exportData~

Exporting the data from the DBMS to a local file.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::exportData(string* tab, string* key,
   long unsigned int* slotnum){
bool val = true;
string path = getFilePath();
string parttabname = getparttabname(tab,key);
string strindex;
long unsigned int i;

  for(i=1;i<=anzWorker;i++){
    strindex = to_string(i);
    val = sendCommand(dbs_conn->get_exportData(tab
          ,&parttabname, key,&strindex,&path,slotnum)) && val;
  }
return val;
}

/*
3.13 ~importData~

Importing data from a local file into the dbms. At first the
table will be created and after that starts the import from a file.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::importData(string *tab) {

  bool val = true;
  string full_path;
  string cmd;
  string strindex;
  long unsigned int i;

  //create Table
  full_path = getFilePath() + createTabFileName(tab);

  // Read data into memory
  ifstream inFile;
  stringstream strStream;
  inFile.open(full_path);
  strStream << inFile.rdbuf();
  cmd = strStream.str();

  val = dbs_conn->sendCommand(&cmd) && val;
  if(!val) return val;
  FileSystem::DeleteFileOrFolder(full_path);

  //import data (local files from worker)
  for(i=1;i<=anzWorker;i++){
    strindex = to_string(i);
    full_path = getFilePath() + get_partFileName(tab, &strindex);
    val = copy(full_path,*tab,true) && val;
    FileSystem::DeleteFileOrFolder(full_path);
  }
return val;
}

/*
3.14 ~partTable~

Partitions the data, export the data and
import them into the worker.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::partTable(string tab, string key, string art
      , int slotnum, string geo_col, float x0, float y0, float slotsize){

  bool val = true;

  if (boost::iequals(art, "RR")){val = partRoundRobin(&tab, &key, &slotnum);}
  else if (boost::iequals(art, "Hash")){val = partHash(&tab, &key, &slotnum);}
  else if (boost::iequals(art, "Grid")){val = partGrid(&tab, &key, &geo_col
                                               ,&slotnum,&x0, &y0, &slotsize);}
  else {val = partFun(&tab, &key, &art, &slotnum);};
  if(!val){
    cout << "\n Couldn't partition the table." << endl;
    return val;
  }

  val = exportData(&tab, &key, &anzWorker);
  if(!val){
    cout << "\n Couldn't export the data from the table." << endl;
    return val;
  }

  val = createTabFile(tab);
  if(!val){
    cout << "\n Couldn't create the structure-file" << endl;
    return val;
  }

  val = exportToWorker(&tab);
  if(!val){cout << "\n Couldn't transfer the data to the worker." << endl;}

  return val;
}

/*
3.15 ~munion~

Exports the data from the worker and sending them to the master.
The master imports them into the local db.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::munion(string tab) {

  bool val = true;
  string path;
  string strindex;

  //doing the export with one thread for each worker
  for(size_t i=0;i<importer.size();i++){
    strindex = to_string(i+1);
    path = getFilePath() + get_partFileName(&tab,&strindex);
    importer[i]->startExport(tab,path,strindex
             ,createTabFileName(&tab),get_partFileName(&tab,&strindex));
  }

  //waiting for finishing the threads
  for(size_t i=0; i<importer.size(); i++){
    val = importer[i]->getResult() && val;
  }

  //import in local PG-Master
  if(val) val = importData(&tab);

  return val;
}

/*
3.16 ~mquery~

The multi query sends and execute a query to all worker
and stores the result in a table.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::mquery(string query,
                    string tab) {
                
  bool val = true;

  //doing the query with one thread for each worker
  for(size_t i=0;i<importer.size();i++){
    importer[i]->startQuery(tab,query);
  }

  //waiting for finishing the threads
  for(size_t i=0;i<importer.size();i++){
    val = importer[i]->getResult() && val;
  }

  return val;
}

/*
3.17 ~mcommand~

The multi command sends and execute a query to all worker.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::mcommand(string query) {

 bool val = true;

 //doing the command with one thread for each worker
 for(size_t i=0;i<importer.size();i++){
   importer[i]->startCommand(query);
 }

 //waiting for finishing the threads
 for(size_t i=0;i<importer.size();i++){
   val = importer[i]->getResult() && val;
 }

 return val;
}

/*
3.17 ~shutdownWorker~

Shutdown the remote worker

*/
bool BasicEngine_Control::shutdownWorker() {

   bool result = true;
   string shutdownCommand("query be_shutdown()");

   for(BasicEngine_Thread* remote : importer) {
     result = result && remote->simpleCommand(&shutdownCommand);
   }

   return result;
}


/*
3.18 ~checkConn~

Checking the Connection to the secondary Master System and to the Worker.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::checkConn() {
bool val = true;
long unsigned int i = 0;
const int defaultTimeout = 0;
CommandLog CommandLog;

  //checking connection to the worker
  if (vec_ci.size() == anzWorker) {
    while (i < anzWorker and val and anzWorker > 0) {
      val = vec_ci[i]->check(false,CommandLog,defaultTimeout);
      i++;
    }
     //checking the connection to the secondary dbms system
     val = dbs_conn->checkConn() && val;
  } else {
    val = false;
  }

return val;
}

/*
3.19 ~runsql~

Runs a query from a file.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::runsql(string filepath) {

  if (access(filepath.c_str(), 0) == 0) {

    // Read file into memory
    ifstream inFile;
    stringstream strStream;

    inFile.open(filepath);
    strStream << inFile.rdbuf();
    string query = strStream.str();

    //execute the sql-Statement
    if (query != "") {
      bool result = dbs_conn->sendCommand(&query);
      return result;
    }

  } else {
    cout << "Couldn't find the file at path:" + filepath << endl;
    return false;
  }

  return false;
}

/*
3.20 ~partGrid~

The data were partitions in the database by a grid.
Returns true if everything is OK and there are no failure.

*/
bool BasicEngine_Control::partGrid(std::string* tab, std::string* key
    ,std::string* geo_col,int* slotnum,float* x0,float* y0,float* slotsize){
bool val = false;
string query_exec = "";
string partTabName;
string anzSlots = to_string(*slotnum);
string x_start = to_string(*x0);
string y_start = to_string(*y0);
string sizSlots = to_string(*slotsize);

  //Dropping parttable
  partTabName = getparttabname(tab,key);
  drop_table(partTabName);

  //creating Index on table
  query_exec =  dbs_conn->get_drop_index(tab) + " "
            "" + dbs_conn->create_geo_index(tab, geo_col);
  val = dbs_conn->sendCommand(&query_exec);

  //
  query_exec = dbs_conn->get_partGrid(tab,key,geo_col,&anzSlots, &x_start
                            , &y_start, &sizSlots, &partTabName);
  if (query_exec != "" && val) val = dbs_conn->sendCommand(&query_exec);

return val;
}

} /* namespace BasicEngine */
