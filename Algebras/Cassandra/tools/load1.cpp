/*
----
This file is part of SECONDO.

Copyright (C) 2014, University in Hagen,
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

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[ue] [\"u]
//[ae] [\"a]
//[_] [\_]
//[TOC] [\tableofcontents]

[1] Load1

*/

#include <iostream>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


using namespace std;

int main(int argc, char* argv[]) {
   
   int socketfd;
   int port;
   char buffer[255];
   
   struct hostent *server;
   struct sockaddr_in server_addr;

   if(argc != 3) {
      cerr << "Usage: " << argv[0] << " <hostname> <port>" << endl;
      return -1;
   }
 
   // Prepare buffer
   memset(buffer, '0', sizeof(buffer));
   strcpy(buffer,"Data;\"Data:Data\"\n");
   
   // Create socket
   port = atoi(argv[2]);
   socketfd = socket(AF_INET, SOCK_STREAM, 0);
   
   if(socketfd < 0) {
      cerr << "Error opening socket" << endl;
      return -1;
   }
   
   server = gethostbyname(argv[1]);
   
   if(server == NULL) {
      cerr << "Error resolving hostname: " << argv[1] << endl;
      return -1;
   }
   
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(port);
   
   server_addr.sin_addr.s_addr = 
     ((struct in_addr *)server->h_addr_list[0])->s_addr;

   cout << server_addr.sin_addr.s_addr << " / " 
        << server -> h_length << endl;
   
   if(connect(socketfd, (struct sockaddr*) &server_addr, 
         sizeof(struct sockaddr)) < 0) {

      cerr << "Error in connect() " << endl;
      return -1;
   }
 
   // Write data to server
   for(int i = 0; i < 10; i++) {
      write(socketfd, buffer, strlen(buffer));
      usleep(1000000);
   }
   
   // Send EOT (End of Transmission)
   write(socketfd, "\004", sizeof(char));
   
   shutdown(socketfd, 2);

   return EXIT_SUCCESS;
}
