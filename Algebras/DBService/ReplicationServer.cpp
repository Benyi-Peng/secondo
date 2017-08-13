/*

1.1.1 Class Implementation

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
#include <chrono>
#include <iostream>

#include "FileSystem.h"
#include "SocketIO.h"

#include "Algebras/Distributed2/FileTransferKeywords.h"

#include "Algebras/DBService/CommunicationProtocol.hpp"
#include "Algebras/DBService/CommunicationUtils.hpp"
#include "Algebras/DBService/ReplicationServer.hpp"
#include "Algebras/DBService/ReplicationUtils.hpp"
#include "Algebras/DBService/SecondoUtilsLocal.hpp"

using namespace std;

namespace DBService {

ReplicationServer::ReplicationServer(int port) :
 MultiClientServer(port), FileTransferServer(port)
{
    string context("ReplicationServer");
    traceWriter= unique_ptr<TraceWriter>
    (new TraceWriter(context, port));
    traceWriter->writeFunction("ReplicationServer::ReplicationServer");
    traceWriter->write("port", port);
}

ReplicationServer::~ReplicationServer()
{
    traceWriter->writeFunction("ReplicationServer::~ReplicationServer");
}

int ReplicationServer::start()
{
    traceWriter->writeFunction("ReplicationServer::start");
    return MultiClientServer::start();
}

int ReplicationServer::communicate(iostream& io)
{
    const boost::thread::id tid = boost::this_thread::get_id();
    traceWriter->writeFunction(tid, "ReplicationServer::communicate");
    try
    {
        CommunicationUtils::sendLine(io,
                CommunicationProtocol::ReplicationServer());

        if(!CommunicationUtils::receivedExpectedLine(io,
                CommunicationProtocol::ReplicationClient()))
        {
            traceWriter->write(tid, "not connected to ReplicationClient");
            return 1;
        }

        queue<string> receiveBuffer;
        CommunicationUtils::receiveLines(io, 2, receiveBuffer);
        string purpose = receiveBuffer.front();
        receiveBuffer.pop();
        string fileName = receiveBuffer.front();
        receiveBuffer.pop();

        if(!FileSystem::FileOrFolderExists(fileName))
        {
            traceWriter->write(tid, "file not found, notifying client");
            CommunicationUtils::sendLine(io,
                    distributed2::FileTransferKeywords::FileNotFound());
        }
        if(purpose == CommunicationProtocol::SendReplicaForStorage())
        {
            sendFileToClient(io, true, tid);
        }else if(purpose == CommunicationProtocol::SendReplicaForUsage())
        {
            CommunicationUtils::sendLine(io,
                    CommunicationProtocol::FunctionRequest());
            string function;
            CommunicationUtils::receiveLine(io, function);
            if(function == CommunicationProtocol::None())
            {
                sendFileToClient(io, true, tid);
            }else
            {
                // TODO
                // execute query with function on relation in file
                // create new file with new name
                // notify client about new name so that it can request file
                // send to client
                // delete
            }
        }else
        {
            traceWriter->write(tid, "unexpected purpose: ", purpose);
            return 1;
        }
    } catch (...)
    {
        traceWriter->write(tid, "ReplicationServer: communication error");
        return 2;
    }
    return 0;
}

void ReplicationServer::sendFileToClient(
        iostream& io,
        bool fileCreated,
        const boost::thread::id tid)
{
    traceWriter->writeFunction(tid, "ReplicationServer::sendFileToClient");
    // expected by receiveFile function of FileTransferClient,
    // but not sent by sendFile function of FileTransferServer
    CommunicationUtils::sendLine(io,
            distributed2::FileTransferKeywords::FileTransferServer());

    queue<string> expectedLines;
    expectedLines.push(
            distributed2::FileTransferKeywords::FileTransferClient());
    expectedLines.push(distributed2::FileTransferKeywords::SendFile());

    if(!CommunicationUtils::receivedExpectedLines(io, expectedLines))
    {
        traceWriter->write(tid,
                "communication error while initiating file transfer");
    }

    traceWriter->write(tid, "file created, sending file");

    std::chrono::steady_clock::time_point begin =
            std::chrono::steady_clock::now();
    int rc = sendFile(io);
    std::chrono::steady_clock::time_point end =
            std::chrono::steady_clock::now();
    if(rc != 0)
    {
        traceWriter->write(tid, "send failed");
    }else
    {
        traceWriter->write(tid, "file sent");
        traceWriter->write("duration of send [microseconds]",
                std::chrono::duration_cast
                <std::chrono::microseconds>(end - begin).count());
    }
}

} /* namespace DBService */
