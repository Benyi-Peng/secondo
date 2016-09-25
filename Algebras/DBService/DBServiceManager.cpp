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
#include "DBServiceManager.hpp"
#include "SecondoException.h"

namespace DBService
{

DBServiceManager::DBServiceManager()
{
    // TODO initialize vector with connection infos from config file
    std::vector<distributed2::ConnectionInfo> workers;
}

DBServiceManager* DBServiceManager::getInstance()
{
    if (!_instance)
    {
        if (!isInitialized)
        {
            throw new SecondoException("DBServiceManager not initialized");
        }
        _instance = new DBServiceManager();
    }
    return _instance;
}

void DBServiceManager::initialize(
        std::vector<distributed2::ConnectionInfo> connectionInfos)
{
    connections = connectionInfos;
    isInitialized = true;
}

DBServiceManager* DBServiceManager::_instance = NULL;
bool DBServiceManager::isInitialized = false;

} /* namespace DBService */
