/*

1.1.1 Function Implementations

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
#include "Algebras/DBService2/DebugOutput.hpp"
#include "Algebras/DBService2/TraceSettings.hpp"

// #include <loguru.hpp>

#include <iostream>
#include <sstream>

namespace DBService
{

using namespace std;

void print(string& text, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << text << endl;
        //LOG_F(INFO, "%s", text.c_str());
    }
}

void printFunction(const char* text, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << "********************************" << endl;
        out << text << endl;

        //LOG_SCOPE_FUNCTION(INFO);
        //LOG_F(INFO, "%s", text);
    }
}

void printFunction(boost::thread::id tid, const char* text, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << "********************************" << endl;
        out << "[Thread " << tid << "] " << text << endl;
        // LOG_SCOPE_FUNCTION(INFO);
        // LOG_F(INFO, "%s", text);
    }
}

void print(const string& text, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << text << endl;
    }
}

void print(const char* text, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << text << endl;
        // LOG_F(INFO, "%s", text);
    }
}

void print(ListExpr nestedList, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << "length: " << nl->ListLength(nestedList) << endl;
        out << nl->ToString(nestedList).c_str() << endl;

        // LOG_F(INFO, "%s", nl->ToString(nestedList).c_str());
    }
}

void print(int number, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << number << endl;

        // LOG_F(INFO, "%d", number);
    }
}

void print(const char* text, int number, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << text << endl;
        out << number << endl;
        // LOG_F(INFO, "%s: %d", text, number);
    }
}

void print(const char* text, ListExpr nestedList, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << text << endl;
        out << "length: " << nl->ListLength(nestedList) << endl;
        out << nl->ToString(nestedList).c_str() << endl;

        // LOG_F(INFO, "Length: %d", nl->ListLength(nestedList));
        // LOG_F(INFO, "%s", nl->ToString(nestedList).c_str());
    }
}

void print(const char* text1, string& text2, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << text1 <<  ": " << text2 << endl;
        // LOG_F(INFO, "%s: %s", text1, text2.c_str());
    }
}

void print(const char* text1, const string& text2, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << text1 <<  ": " << text2 << endl;
        // LOG_F(INFO, "%s: %s", text1, text2.c_str());
    }
}

void print(boost::thread::id tid, const char* text1, const string& text2,
           std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << "[Thread " << tid << "] " << text1 << ": " << text2 << endl;
        
        stringstream tids; // no-pun intended
        tids << tid;

        // LOG_F(INFO, "[Thread %s] %s: %s", tids.str().c_str(), text1, 
        //    text2.c_str());
    }
}

void print(const string& text1, const char* text2, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        // LOG_F(INFO, "%s\n%s", text1.c_str(), text2);        
    }
}

void print(const LocationInfo& locationInfo, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << "LocationInfo:" << endl;
        printLocationInfo(locationInfo, out);
    }
}

//JF: Deprecated //TODO Remove
void printLocationInfo(const LocationInfo& locationInfo, std::ostream& out)
{
    out << "Host:\t\t" << locationInfo.getHost() << endl;
    out << "Port:\t\t" << locationInfo.getPort() << endl;
    out << "Disk:\t\t" << locationInfo.getDisk() << endl;
    out << "CommPort:\t" << locationInfo.getCommPort() << endl;
    out << "TransferPort:\t" << locationInfo.getTransferPort() << endl;
    
}

//JF: Deprecated //TODO Remove
void print(const RelationInfo& relationInfo, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << "RelationInfo:" << endl;
        printRelationInfo(relationInfo, out);
    }
}

//JF: Deprecated //TODO Remove
void print(const DerivateInfo& derivateInfo, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
        out << "DerivateInfo:" << endl;
        printDerivateInfo(derivateInfo, out);
    }
}

//JF: Deprecated //TODO Remove
void printRelationInfo(const RelationInfo& relationInfo, std::ostream& out)
{
    out << "DatabaseName:\t" << relationInfo.getDatabaseName() << endl;
    out << "RelationName:\t" << relationInfo.getRelationName() << endl;
    out << "Host:\t\t" << relationInfo.getOriginalLocation().getHost() << endl;
    out << "Port:\t\t" << relationInfo.getOriginalLocation().getPort() << endl;
    out << "Disk:\t\t" << relationInfo.getOriginalLocation().getDisk() << endl;
    for(ReplicaLocations::const_iterator it
            = relationInfo.nodesBegin(); it != relationInfo.nodesEnd(); it++)
    {
        out << "Node:\t\t" << it->first << " (Replicated: " <<
                (it->second ? "TRUE" : "FALSE") << ")"
                << endl;
    }
}

//JF: Deprecated //TODO Remove
void printDerivateInfo(const DerivateInfo& derivateInfo, std::ostream& out)
{
    out << "ObjectName:\t" << derivateInfo.getName() << endl;
    out << "DependsOn:\t" << derivateInfo.getSource() << endl;
    out << "Fun:\t\t"     << derivateInfo.getFun() << endl;
    for(ReplicaLocations::const_iterator it
            = derivateInfo.nodesBegin(); it != derivateInfo.nodesEnd(); it++)
    {
        out << "Node:\t\t" << it->first << " (Replicated: " <<
                (it->second ? "TRUE" : "FALSE") << ")"
                << endl;
    }
}

//JF: Deprecated //TODO Remove
void print(const char* text, const vector<string>& values, std::ostream& out)
{
    if(TraceSettings::getInstance()->isDebugTraceOn())
    {
       out << text << endl << endl;
       for(auto &t : values){
          out << t << endl;
       }
       out << "-----" << endl;
    }  
}


}
