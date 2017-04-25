/*
----
This file is part of SECONDO.

Copyright (C) 2016,
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
#include <sstream>
#include <vector>

#include "SecParser.h"
#include "Algebra.h"

#include "Algebras/DBService/Replicator.hpp"
#include "Algebras/DBService/ReplicationServer.hpp"
#include "Algebras/DBService/ServerRunnable.hpp"
#include "Algebras/DBService/SecondoUtils.hpp"
#include "Algebras/DBService/DebugOutput.hpp"
#include "Algebras/DBService/CommunicationClientRunnable.hpp"

using namespace std;

namespace DBService
{

Replicator::Replicator()
{
    string fileTransferPort;
    SecondoUtils::readFromConfigFile(fileTransferPort,
            "DBService",
            "FileTransferPort",
            "");
    print(fileTransferPort);
    transferPort = atoi(fileTransferPort.c_str());
    ServerRunnable replicationServer(transferPort);
    replicationServer.run<ReplicationServer>();

    SecondoUtils::readFromConfigFile(host,
            "Environment",
            "SecondoHost",
            "");
    print(host);
}

Replicator::~Replicator()
{
    // TODO Auto-generated destructor stub
}

string Replicator::getFileName(const std::string& relationName) const
{
    return SecondoSystem::GetInstance()->GetDatabaseName()
            + "_-_" + relationName + ".bin";
}

void Replicator::replicateRelation(const string& relationName,
        const vector<LocationInfo>& locations) const
{
    createFileOnCurrentNode(relationName);
    runReplication(relationName, locations);
}

void Replicator::createFileOnCurrentNode(const string& relationName) const
{
    stringstream query;
    query << "query saveObjectToFile "
          << relationName
          << "[\""
          << getFileName(relationName)
          << "\"]";

    SecParser secondoParser;
    string queryAsNestedList;
    if (secondoParser.Text2List(query.str(), queryAsNestedList) != 0)
    {
        // TODO
    } else
    {
        Word result;
        QueryProcessor::ExecuteQuery(queryAsNestedList, result, 1024);
    }
}

void Replicator::runReplication(const string& relationName,
        const vector<LocationInfo>& locations) const
{
    for(vector<LocationInfo>::const_iterator it = locations.begin();
            it != locations.end(); it++)
    {
        CommunicationClientRunnable commClient(
                           host,
                           transferPort,
                           it->getHost(),
                           atoi(it->getCommPort().c_str()),
                           getFileName(relationName),
                           SecondoSystem::GetInstance()->GetDatabaseName(),
                           relationName);
}
}

} /* namespace DBService */
