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

*/
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/make_shared.hpp>

#include "Algebra.h"
#include "NestedList.h"
#include "Operator.h"
#include "SecondoException.h"
#include "SecParser.h"
#include "StringUtils.h"

#include "Algebras/Distributed2/ConnectionInfo.h"

#include "Algebras/DBService/CommunicationServer.hpp"
#include "Algebras/DBService/DBServiceManager.hpp"
#include "Algebras/DBService/DBServicePersistenceAccessor.hpp"
#include "Algebras/DBService/DebugOutput.hpp"
#include "Algebras/DBService/RelationInfo.hpp"
#include "Algebras/DBService/SecondoUtilsLocal.hpp"
#include "Algebras/DBService/SecondoUtilsRemote.hpp"
#include "Algebras/DBService/ServerRunnable.hpp"

using namespace std;
using namespace distributed2;

namespace DBService
{

DBServiceManager::DBServiceManager()
{
    printFunction("DBServiceManager::DBServiceManager");
    restoreConfiguration();
    restoreReplicaInformation();
}

void DBServiceManager::restoreConfiguration()
{
    printFunction("DBServiceManager::restoreConfiguration");
    string port;
    SecondoUtilsLocal::readFromConfigFile(
            port, "DBService","DBServicePort", "9989");
    ServerRunnable commServer(atoi(port.c_str()));
    commServer.run<CommunicationServer>();

    string replicaNumber;
    SecondoUtilsLocal::readFromConfigFile(
            replicaNumber, "DBService","ReplicaNumber", "1");
    replicaCount = atoi(replicaNumber.c_str());

    string faultToleranceMode;
    SecondoUtilsLocal::readFromConfigFile(
            faultToleranceMode, "DBService","FaultToleranceLevel", "2");
    mode = static_cast<FaultToleranceMode>(atoi(faultToleranceMode.c_str()));
}

void DBServiceManager::restoreReplicaInformation()
{
    printFunction("DBServiceManager::restoreReplicaInformation");
    map<ConnectionID, LocationInfo> locations;
    DBServicePersistenceAccessor::restoreLocationInfo(locations);
    for(map<ConnectionID, LocationInfo>::const_iterator it = locations.begin();
            it != locations.end(); it++)
    {
        ConnectionInfo* connectionInfo =
                ConnectionInfo::createConnection(
                            it->second.getHost(),
                            atoi(it->second.getPort().c_str()),
                            *(const_cast<string*>(&(it->second.getConfig()))));
        pair<LocationInfo, ConnectionInfo*> workerConnDetails(it->second,
                                                              connectionInfo);
        connections.insert(
                pair<size_t, pair<LocationInfo, ConnectionInfo*> >(
                        it->first, workerConnDetails));

        SecondoUtilsRemote::openDatabase(connectionInfo, "dbservice");
        if(!startServersOnWorker(connectionInfo))
        {
            // TODO more descriptive error message (host, port, etc)
            throw new SecondoException("could not start file transfer server");
        }
    }

    DBServicePersistenceAccessor::restoreRelationInfo(relations);

    queue<pair<std::string, ConnectionID> > mapping;
    DBServicePersistenceAccessor::restoreLocationMapping(mapping);

    while(!mapping.empty())
    {
        relations.at(mapping.front().first).addNode(mapping.front().second);
        mapping.pop();
    }

    for(map<string, RelationInfo>::const_iterator it = relations.begin();
            it != relations.end(); it++)
    {
        print("RelationID: ", it->first);
        print("Number of Replicas: ",
                const_cast<RelationInfo*>(&(it->second))->getNodeCount());
    }
}

DBServiceManager* DBServiceManager::getInstance()
{
    printFunction("DBServiceManager::getInstance");
    if (!_instance)
    {
        _instance = new DBServiceManager();
    }
    return _instance;
}

ConnectionID DBServiceManager::getNextFreeConnectionID()
{
    printFunction("DBServiceManager::getNextFreeConnectionID");
    return connections.size() + 1;
}

bool DBServiceManager::addNode(const string host,
        const int port,
        string config)
{
    printFunction("DBServiceManager::addNode");

    for(const auto& connection : connections)
    {
        if(connection.second.first.isSameWorker(
                host, stringutils::int2str(port)))
        {
            return false;
        }
    }

    ConnectionInfo* connectionInfo =
            ConnectionInfo::createConnection(host, port, config);

    SecondoUtilsRemote::createDatabase(connectionInfo, "dbservice");
    SecondoUtilsRemote::openDatabase(connectionInfo, "dbservice");

    // retrieve location parameters from worker
    string dir;
    getConfigParamFromWorker(dir, connectionInfo,
            "Environment", "SecondoHome");
    string commPort;
    getConfigParamFromWorker(commPort, connectionInfo,
            "DBService", "CommunicationPort");
    string transferPort;
    getConfigParamFromWorker(transferPort, connectionInfo,
            "DBService", "FileTransferPort");

    if(!startServersOnWorker(connectionInfo))
    {
        return false;
    }

    LocationInfo location(host, stringutils::int2str(port), config, dir,
            commPort, transferPort);

    pair<LocationInfo, ConnectionInfo*> workerConnDetails(location,
                                                          connectionInfo);
    ConnectionID connID = getNextFreeConnectionID();
    connections.insert(
            pair<size_t, pair<LocationInfo, ConnectionInfo*> >(
                    connID, workerConnDetails));

    DBServicePersistenceAccessor::persistLocationInfo(connID, location);

    for(auto& replicaLocation : possibleReplicaLocations)
    {
        addToPossibleReplicaLocations(
                connID,
                location,
                replicaLocation.second,
                host,
                dir);
    }
    return true;
}

bool DBServiceManager::startServersOnWorker(
        distributed2::ConnectionInfo* connectionInfo)
{
    printFunction("DBServiceManager::startServersOnWorker");
    string queryInit("query initdbserviceworker()");
    print("queryInit", queryInit);

    return SecondoUtilsRemote::executeQuery(connectionInfo, queryInit);
}

bool DBServiceManager::getConfigParamFromWorker(string& result,
        distributed2::ConnectionInfo* connectionInfo, const char* section,
        const char* key)
{
    printFunction("DBServiceManager::getConfigParamFromWorker");
    string resultAsString;
    stringstream query;
    query << "query getconfigparam(\""
          << section
          << "\", \""
          << key
          << "\")";
    bool resultOk = SecondoUtilsRemote::executeQuery(
            connectionInfo,
            query.str(),
            resultAsString);
    print("resultAsString", resultAsString);

    ListExpr resultAsNestedList;
    nl->ReadFromString(resultAsString, resultAsNestedList);
    result.assign(nl->StringValue(nl->Second(resultAsNestedList)));
    print("result", result);

    return resultOk && result.size() != 0;
}

void DBServiceManager::determineReplicaLocations(const string& databaseName,
                                         const string& relationName,
                                         const string& host,
                                         const string& port,
                                         const string& disk)
{
    printFunction("DBServiceManager::storeRelationInfo");
    RelationInfo relationInfo(databaseName,
                              relationName,
                              host,
                              port,
                              disk);

    vector<ConnectionID> locations;
    getWorkerNodesForReplication(locations,
                                 host,
                                 disk);
    relationInfo.addNodes(locations);
    relations.insert(pair<string, RelationInfo>(relationInfo.toString(),
            relationInfo));
}

void DBServiceManager::deleteReplicaLocations(const string& databaseName,
                                              const string& relationName)
{
    relations.erase(RelationInfo::getIdentifier(databaseName, relationName));
}

void DBServiceManager::persistReplicaLocations(const string& databaseName,
                                               const string& relationName)
{
    RelationInfo relationInfo =
            relations.at(
                    RelationInfo::getIdentifier(databaseName, relationName));
    DBServicePersistenceAccessor::persistRelationInfo(relationInfo);
}

void DBServiceManager::getReplicaLocations(
        const string& relationAsString,
        vector<pair<ConnectionID, bool> >& ids)
{
    printFunction("DBServiceManager::getReplicaLocations");
    RelationInfo& relInfo = getRelationInfo(relationAsString);
    ids.insert(ids.begin(), relInfo.nodesBegin(), relInfo.nodesEnd());
}

void DBServiceManager::addToPossibleReplicaLocations(
        const ConnectionID connectionID,
        const LocationInfo& location,
        vector<ConnectionID>& potentialReplicaLocations,
        const string& host,
        const string& disk)
{
    switch(mode)
    {
    case FaultToleranceMode::DISK:
        if(!location.isSameDisk(host, disk))
        {
            potentialReplicaLocations.push_back(connectionID);
        }
        break;
    case FaultToleranceMode::NODE:
        if(!location.isSameHost(host))
        {
            potentialReplicaLocations.push_back(connectionID);
        }
        break;
    case FaultToleranceMode::NONE:
    default:
        potentialReplicaLocations.push_back(connectionID);
        break;
    }
}

void DBServiceManager::getWorkerNodesForReplication(
        vector<ConnectionID>& nodes,
        const string& host,
        const string& disk)
{
    printFunction("DBServiceManager::getWorkerNodesForReplication");
    string locationID = LocationInfo::getIdentifier(host, disk);
    AlternativeLocations::const_iterator it =
            possibleReplicaLocations.find(locationID);
    if(it == possibleReplicaLocations.end())
    {
        vector<ConnectionID> potentialReplicaLocations;
        for(const auto& connection : connections)
        {
            const LocationInfo& location = connection.second.first;
            addToPossibleReplicaLocations(
                    connection.first,
                    location,
                    potentialReplicaLocations,
                    host,
                    disk);
        }
        possibleReplicaLocations.insert(
                pair<std::string, vector<ConnectionID> >(
                        locationID, potentialReplicaLocations));
        it = possibleReplicaLocations.find(locationID);
    }
    vector<ConnectionID>& connectionsForLocation =
            const_cast<vector<ConnectionID>& >(it->second);
    random_shuffle(
            connectionsForLocation.begin(),
            connectionsForLocation.end());

    nodes.insert(nodes.begin(),
            connectionsForLocation.begin(),
            connectionsForLocation.end());

    if(connectionsForLocation.size() > replicaCount)
    {
        nodes.resize(replicaCount);
    }
}

ConnectionInfo* DBServiceManager::getConnection(ConnectionID id)
{
    printFunction("DBServiceManager::getConnection");
    return connections.at(id).second;
}

LocationInfo& DBServiceManager::getLocation(ConnectionID id)
{
    printFunction("DBServiceManager::getLocation");
    return connections.at(id).first;
}

RelationInfo& DBServiceManager::getRelationInfo(const string& relationAsString)
{
    printFunction("DBServiceManager::getRelationInfo");
    return relations.at(relationAsString);
}

void DBServiceManager::maintainSuccessfulReplication(
        const string& relID,
        const string& replicaLocationHost,
        const string& replicaLocationPort)
{
    RelationInfo& relInfo = getRelationInfo(relID);
    // TODO catch out_of_range exception

    for(map<ConnectionID, bool>::const_iterator it
            = relInfo.nodesBegin(); it != relInfo.nodesEnd(); it++)
    {
        LocationInfo& location = getLocation(it->first);
        if(location.isSameWorker(replicaLocationHost, replicaLocationPort))
        {
            relInfo.updateReplicationStatus(it->first, true);
            DBServicePersistenceAccessor::updateLocationMapping(
                    relID,
                    it->first,
                    true);
            // TODO check return values
            break;
        }
    }
}

void DBServiceManager::deleteReplicaMetadata(const string& relID)
{
    DBServicePersistenceAccessor::deleteRelationInfo(getRelationInfo(relID));
    relations.erase(relID);
}

DBServiceManager* DBServiceManager::_instance = nullptr;

} /* namespace DBService */
