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
#include <ctime>
#include <sstream>

#include "Algebras/DBService2/DebugOutput.hpp"
#include "Algebras/DBService2/SecondoUtilsLocal.hpp"
#include "Algebras/DBService2/TraceSettings.hpp"
#include "Algebras/DBService2/TraceWriter.hpp"

#include <loguru.hpp>

using namespace std;

extern boost::recursive_mutex nlparsemtx;

namespace DBService {

TraceWriter::TraceWriter(string& context, int port, ostream& _out)
{
    std::time_t currentTime = std::time(0);
    stringstream fileName;
    out = &_out;

    string host;
    SecondoUtilsLocal::readFromConfigFile(
            host,
            "Environment",
            "SecondoHost",
            "");

    fileName << context << "_"
             << host << "_"
             << port << "_"
             << currentTime << ".trc";

    traceFile= unique_ptr<ofstream>
    (new ofstream(fileName.str().c_str(),std::ios::binary));
}

TraceWriter::~TraceWriter()
{
    if(*traceFile)
    {
        traceFile->close();
    }
}

void TraceWriter::write(const string& text)
{
    print(text, *out);
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        *traceFile << text << endl;
    }
}

void TraceWriter::write(const char* text)
{
    print(text, *out);
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        *traceFile << text << endl;
    }
}

void TraceWriter::write(const boost::thread::id tid, const char* text)
{
    print(text, *out);
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        *traceFile << "[Thread " << tid << "] " << text << endl;
    }
}

void TraceWriter::write(const size_t text)
{
    print(text, *out);
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        *traceFile << text << endl;
    }
}

void TraceWriter::write(const LocationInfo& location)
{
    print(location, *out);
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        *traceFile << location.getHost() << endl;
        *traceFile << location.getPort() << endl;
        *traceFile << location.getDisk() << endl;
        *traceFile << location.getCommPort() << endl;
        *traceFile << location.getTransferPort() << endl;
    }
}

void TraceWriter::write(const RelationInfo& relationInfo)
{
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        *traceFile << "RelationInfo:" << endl;
        *traceFile << relationInfo.getDatabaseName() << endl;
        *traceFile << relationInfo.getRelationName() << endl;
        *traceFile << relationInfo.getOriginalLocation().getHost() << endl;
        *traceFile << relationInfo.getOriginalLocation().getPort() << endl;
        *traceFile << relationInfo.getOriginalLocation().getDisk() << endl;
        for(ReplicaLocations::const_iterator it
                = relationInfo.nodesBegin();
                it != relationInfo.nodesEnd(); it++)
        {
            *traceFile << "Node:\t\t" << it->first << " (Replicated: " <<
                    (it->second ? "TRUE" : "FALSE") << ")"
                    << endl;
        }
    }
}

void TraceWriter::write(const char* description, const string& text)
{
    print(description, text, *out);
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        *traceFile << description << ": " << text << endl;
    }
}

void TraceWriter::write(
        const boost::thread::id tid,
        const char* description,
        const string& text)
{
    print(tid, description, text, *out);
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        *traceFile << "[Thread " << tid << "] "
                << description << ": " << text << endl;
    }
}

void TraceWriter::write(const char* description, int number)
{
    print(description, number, *out);
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        *traceFile << description << ": " << number << endl;
    }
}

void TraceWriter::writeFunction(const char* text)
{
    printFunction(text, *out);
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        *traceFile << "********************************" << endl;
        *traceFile << text << endl;
    }
}

void TraceWriter::writeFunction(const boost::thread::id tid, const char* text)
{
    printFunction(tid, text, *out);
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        boost::lock_guard<boost::mutex> lock(traceWriterMutex);
        *traceFile << "********************************" << endl;
        *traceFile << "[Thread " << tid << "] " << text << endl;
    }
}

void TraceWriter::write(
        const boost::thread::id tid,
        const char* text,
        ListExpr nestedList)
{
    print(text, nestedList, *out);
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        LOG_F(INFO, "%s", "Acquiring lock for nlparsemtx...");
        boost::lock_guard<boost::recursive_mutex> guard(nlparsemtx);
        LOG_F(INFO, "%s", "Successfully acquired lock for nlparsemtx...");
        *traceFile << "[Thread " << tid << "] " << endl;
        *traceFile << text << endl;
        *traceFile << "length: " << nl->ListLength(nestedList) << endl;
        *traceFile << nl->ToString(nestedList) << endl;
    }
}


void TraceWriter::write(
        const boost::thread::id tid,
        const char* text,
        const bool value)
{
    std::string v = value?"true":"false";
    print(tid,text, v,*out);
    if(TraceSettings::getInstance()->isFileTraceOn())
    {
        *traceFile << "[Thread " << tid << "] " << endl;
        *traceFile << text << endl;
        *traceFile << v << endl;
    }
}

} /* namespace DBService */
