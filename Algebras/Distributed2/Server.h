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
#ifndef ALGEBRAS_DISTRIBUTED2_SERVER_H_
#define ALGEBRAS_DISTRIBUTED2_SERVER_H_

class Socket;

namespace distributed2 {

class Server {
public:
    Server(int _port);
    virtual ~Server();
    virtual int start() = 0;

protected:
    int port;
    Socket* listener;
    Socket* server;
};

} /* namespace distributed2 */

#endif /* ALGEBRAS_DISTRIBUTED2_SERVER_H_ */
