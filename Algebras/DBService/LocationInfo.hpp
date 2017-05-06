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
#ifndef ALGEBRAS_DBSERVICE_LOCATIONINFO_HPP_
#define ALGEBRAS_DBSERVICE_LOCATIONINFO_HPP_

#include <string>

namespace DBService {

class LocationInfo {
public:
    LocationInfo(const std::string& host,
                 const std::string& port,
                 const std::string& config,
                 const std::string& disk,
                 const std::string& commPort,
                 const std::string& transferPort);
    const std::string& getHost() const;
    const std::string& getPort() const;
    const std::string& getConfig() const;
    const std::string& getDisk() const;
    const std::string& getCommPort() const;
    const std::string& getTransferPort() const;
private:
    std::string host;
    std::string port;
    std::string config;
    std::string disk;
    std::string commPort;
    std::string transferPort;
};

} /* namespace DBService */

#endif /* ALGEBRAS_DBSERVICE_LOCATIONINFO_HPP_ */
