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
#include <string>

#include "Algebras/DBService/LocationInfo.hpp"

using namespace std;

namespace DBService {

LocationInfo::LocationInfo(const string& host,
                           const string& port,
                           const string& disk,
                           const string& commPort,
                           const string& transferPort)
: host(host), port(port), disk(disk),
  commPort(commPort), transferPort(transferPort)
{}

const string& LocationInfo::getHost() const
{
    return host;
}

const string& LocationInfo::getPort() const
{
    return port;
}

const string& LocationInfo::getDisk() const
{
    return disk;
}

const string& LocationInfo::getCommPort() const
{
    return commPort;
}
const string& LocationInfo::getTransferPort() const
{
    return transferPort;
}

} /* namespace DBService */
