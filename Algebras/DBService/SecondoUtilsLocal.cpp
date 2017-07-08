/*
----
This file is part of SECONDO.

Copyright (C) 2017,
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
//[_][\_]

*/
#include <algorithm>
#include <sstream>

#include <boost/thread/mutex.hpp>

#include "CharTransform.h"
#include "NestedList.h"
#include "Profiles.h"
#include "SecParser.h"

#include "Algebras/DBService/DebugOutput.hpp"
#include "Algebras/DBService/SecondoUtilsLocal.hpp"


using namespace std;
using namespace distributed2;

namespace DBService {

void SecondoUtilsLocal::readFromConfigFile(string& resultValue,
        const char* section,
        const char* key,
        const char* defaultValue)
{
    printFunction("SecondoUtilsLocal::readFromConfigFile");
    string secondoConfig = expandVar("$(SECONDO_CONFIG)");
    resultValue = SmiProfile::GetParameter(section,
            key, defaultValue, secondoConfig);
}

bool SecondoUtilsLocal::executeQuery(const string& queryAsString)
{
    Word queryResult;
    return executeQuery(queryAsString, queryResult);
}

bool SecondoUtilsLocal::prepareQueryForProcessing(
            const string& queryAsString,
            string& queryAsPreparedNestedListString)
{
    SecParser secondoParser;
    string queryAsNestedListString;
    if (secondoParser.Text2List(queryAsString, queryAsNestedListString) != 0)
    {
        print("could not parse query");
        return false;
    }
    print("query converted to nested list string");
    print("queryAsNestedListString", queryAsNestedListString);
    queryAsNestedListString.erase(0, strlen("(query "));
    print("queryAsNestedListString", queryAsNestedListString);
    queryAsPreparedNestedListString =
            queryAsNestedListString.substr(0, queryAsNestedListString.size()-2);
    print("queryAsPreparedNestedListString", queryAsNestedListString);
    return true;

}

bool SecondoUtilsLocal::executeQuery2(const string& queryAsString)
{
    Word queryResult;
    string queryAsPreparedNestedListString;
    if(!prepareQueryForProcessing(
            queryAsString,
            queryAsPreparedNestedListString))
    {
        return false;
    }
    return QueryProcessor::ExecuteQuery(
            queryAsPreparedNestedListString,
            queryResult,
            DEFAULT_GLOBAL_MEMORY,
            3);
}

bool SecondoUtilsLocal::executeQuery(const string& queryAsString,
                                     Word& queryResult)
{
    printFunction("SecondoUtilsLocal::executeQuery");
    boost::lock_guard<boost::mutex> lock(utilsMutex);
    print("queryAsString", queryAsString);
    SecParser secondoParser;
    string queryAsNestedListString;
    if (secondoParser.Text2List(queryAsString, queryAsNestedListString) != 0) {
        print("could not parse query");
        return false;
    }
    print("query converted to nested list string");
//    ListExpr queryAsNestedList;
//    if (!nl->ReadFromString(queryAsNestedListString, queryAsNestedList)) {
//        print("could not convert string to list");
//        return false;
//    }

    ListExpr queryAsNestedList;
    NestedList* nli = SecondoSystem::GetNestedList();
    if(!nli->ReadFromString(queryAsNestedListString,
            queryAsNestedList))
    {
        print("could not read nested list from string");
        return false;
    }

    print("nested list string converted to nested list");

    print("queryAsNestedListString", queryAsNestedListString);

    bool success = true;
    bool correct, evaluable, defined, isFunction = false;
    string typeString(""), errorString("");
    try
    {
        success = QueryProcessor::ExecuteQuery(
                queryAsNestedList,
                queryResult,
                typeString,
                errorString,
                correct,
                evaluable,
                defined,
                isFunction
                /*availableMemory*/);
    } catch(...)
    {
        print("Caught exception during query execution");
        success = false;
        print("errorString", errorString);
    }
    print("success", success);
    print("correct", correct);
    print("evaluable", evaluable);
    print("defined", defined);
    print("isFunction", isFunction);
    return success;
}

bool SecondoUtilsLocal::adjustDatabase(const string& databaseName)
{
    printFunction("SecondoUtilsLocal::adjustDatabase");
    const string currentDB = SecondoSystem::GetInstance()->GetDatabaseName();
    print("current database name", currentDB);
    print("requested database name", databaseName);

    string databaseNameUppered(databaseName);

    transform(
            databaseNameUppered.begin(),
            databaseNameUppered.end(),
            databaseNameUppered.begin(),
            ::toupper);

    if(currentDB
            != databaseNameUppered)
    {
        boost::lock_guard<boost::mutex> lock(utilsMutex);
        print("need to adjust database");
        string queryClose("close database");
        SecondoUtilsLocal::executeQuery(queryClose);

        stringstream queryCreate;
        queryCreate << "create database "
                    << databaseName;
        SecondoUtilsLocal::executeQuery(queryCreate.str());

        stringstream queryOpen;
        queryOpen << "open database "
                    << databaseName;
        SecondoUtilsLocal::executeQuery(queryOpen.str());
        return true;
    }
    return false;
}

bool
SecondoUtilsLocal::createRelation(const string& queryAsString,
        string& errorMessage)
{
    printFunction("SecondoUtilsLocal::createRelation");
    bool correct = false;
    bool evaluable = false;
    bool defined = false;
    bool isFunction = false;

    boost::lock_guard<boost::mutex> lock(utilsMutex);
    NestedList* nli = SecondoSystem::GetNestedList();
    QueryProcessor* queryProcessor = new QueryProcessor( nli,
            SecondoSystem::GetAlgebraManager(),
            DEFAULT_GLOBAL_MEMORY);
    queryProcessor->SetDebugLevel(3);
    SecondoCatalog* catalog = SecondoSystem::GetCatalog();

    // TODO check for existing catalog object

    Word result = SetWord(Address(0));
    OpTree tree = 0;

    ListExpr resultType = nl->TheEmptyList();

    print("queryAsString", queryAsString);
    SecParser secondoParser;
    string queryAsNestedListString;
    if (secondoParser.Text2List(queryAsString, queryAsNestedListString) != 0) {
        print("could not parse query");
        return false;
    }
    print("query converted to nested list string");
    ListExpr queryAsNestedList;
    if (!nl->ReadFromString(queryAsNestedListString, queryAsNestedList)) {
        print("could not convert string to list");
    }
    print("nested list string converted to nested list");

    //TODO check open database?
    //TODO transaction?
    // SecondoSystem::BeginTransaction();
    string objectName = nl->SymbolValue(nl->Second(queryAsNestedList));
    ListExpr valueExpr = nl->Fourth(queryAsNestedList);

    try {
        queryProcessor->Construct(valueExpr, correct, evaluable, defined,
                isFunction, tree, resultType);

        if (evaluable) {
            string typeName = "";
            catalog->CreateObject(objectName, typeName, resultType, 0);
            queryProcessor->EvalS(tree, result, 1);
            catalog->UpdateObject(objectName, result);
            queryProcessor->Destroy(tree, false);
        } else {
            return false;
        }
    } catch (...) {

        print("caught error");
        queryProcessor->Destroy(tree, true);
    }

    // TODO
    //SecondoSystem::CommitTransaction(true);
    cout << "correct: " << correct << endl;
    cout << "evaluable: " << evaluable << endl;
    cout << "defined: " << defined << endl;
    cout << "isFunction: " << isFunction << endl;
    return true;
}

bool SecondoUtilsLocal::excuteQueryCommand(const string& queryAsString)
{
    printFunction("SecondoUtilsLocal::excuteQueryCommand (1 arg)");
    ListExpr resultList;
    string errorMessage;
    bool resultOk = SecondoUtilsLocal::excuteQueryCommand(
                                                    queryAsString,
                                                    resultList,
                                                    errorMessage);
    if(resultOk)
    {
        print("query executed successfully");
        print("resultList", resultList);
    }else
    {
        print("failed to execute query");
        print("errorMessage", errorMessage);
    }
    return resultOk;
}

bool SecondoUtilsLocal::excuteQueryCommand(const string& queryAsString,
        ListExpr& resultList, string& errorMessage) {
    printFunction("SecondoUtilsLocal::excuteQueryCommand (2 args)");
    bool correct = false;
    bool evaluable = false;
    bool defined = false;
    bool isFunction = false;

    boost::lock_guard<boost::mutex> lock(utilsMutex);
    NestedList* nli = SecondoSystem::GetNestedList();
    QueryProcessor* queryProcessor = new QueryProcessor( nli,
            SecondoSystem::GetAlgebraManager(),
            DEFAULT_GLOBAL_MEMORY);
    queryProcessor->SetDebugLevel(3);
    SecondoCatalog* catalog = SecondoSystem::GetCatalog();

    Word result = SetWord(Address(0));
    OpTree tree = 0;

    ListExpr resultType = nl->TheEmptyList();

    print("queryAsString", queryAsString);
    SecParser secondoParser;
    string queryAsNestedListString;
    if (secondoParser.Text2List(queryAsString, queryAsNestedListString) != 0) {
        // TODO
        print("could not parse query");
        return false;
    }
    print("query converted to nested list string");
    print("queryAsNestedListString", queryAsNestedListString);

    ListExpr queryAsNestedList;
    if (!nl->ReadFromString(queryAsNestedListString, queryAsNestedList)) {
        print("could not convert string to list");
    }
    print("nested list string converted to nested list");
    print("queryAsNestedList", queryAsNestedList);

    // database open?

    // transaction?

    try {
        print("queryProcessor->Construct");
        queryProcessor->Construct(nl->Second(queryAsNestedList), correct,
                evaluable, defined, isFunction, tree, resultType);
        print("trying queryProcessor->Construct done");
        cout << "correct: " << correct << endl;
        cout << "evaluable: " << evaluable << endl;
        cout << "defined: " << defined << endl;
        cout << "isFunction: " << isFunction << endl;

        if (evaluable) {
            queryProcessor->EvalS(tree, result, 1);
            print("queryProcessor->EvalP done");

            ListExpr valueList = catalog->OutObject(resultType, result);
            print("valueList done");

            resultList = nl->TwoElemList(resultType, valueList);
            print("resultList done");

            queryProcessor->Destroy(tree, true);
            print("queryProcessor->Destroy done");
        }

    } catch (...) {
        print("caught error");
        queryProcessor->Destroy(tree, true);
        return false;
    }
    return true;
}

bool SecondoUtilsLocal::lookupDBServiceLocation(
            string& host,
            string& commPort)
{
    printFunction("DBServiceConnector::lookupDBServiceLocation");
    SecondoUtilsLocal::readFromConfigFile(host,
                                           "DBService",
                                           "DBServiceHost",
                                           "");
    if(host.length() == 0)
    {
        print("could not find DBServiceHost in config file");
        return false;
    }

    SecondoUtilsLocal::readFromConfigFile(commPort,
                                       "DBService",
                                       "DBServicePort",
                                       "");
    if(commPort.length() == 0)
    {
        print("could not find DBServicePort in config file");
        return false;
    }
    return true;
}

boost::mutex SecondoUtilsLocal::utilsMutex;

} /* namespace DBService */
