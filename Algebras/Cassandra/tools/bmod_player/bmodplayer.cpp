/*
----
This file is part of SECONDO.

Copyright (C) 2015,
Faculty of Mathematics and Computer Science,
Database Systems for New Applications.

SECONDO is free software; you can redistribute it and/or modify
it under the Systems of the GNU General Public License as published by
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


1 Realtime player for BerlinMod GPS Data


1.1 Includes

*/

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "workerqueue.h"

#define CMDLINE_INPUTFILE   1<<0
#define CMDLINE_DESTHOST    1<<1
#define CMDLINE_DESTPORT    1<<2

#define QUEUE_ELEMENTS 100

using namespace std;

struct Configuration {
   string inputfile;
   string desthost;
   size_t destport;
};

struct Statistics {
   size_t trips;
   size_t send;
   size_t discarded; 
};

struct InputData {
    size_t moid;
    size_t tripid;
    tm time;
    float x;
    float y;
};

bool parseCSVDate(struct tm &tm, string date) {
   if (strptime(date.c_str(), "%Y-%m-%d %H:%M:%S.", &tm)) {
      return true;
   }
   
   if (strptime(date.c_str(), "%Y-%m-%d %H:%M", &tm)) {
      return true;
   }
   
   if (strptime(date.c_str(), "%Y-%m-%d %H", &tm)) {
      return true;
   }
   
   if (strptime(date.c_str(), "%Y-%m-%d", &tm)) {
      return true;
   }
   
   return false;
}

bool handleCSVLine(WorkerQueue<InputData*> &data, 
  vector<std::string> &lineData) {
   
   // Skip CSV header
   if(lineData[0] == "Moid") {
      return true;     
   }
   
   // 2007-06-08 08:32:26.781
   struct tm tm1;
   struct tm tm2;
   
   if (! parseCSVDate(tm1, lineData[2])) {
      cerr << "Unable to parse start date: " << lineData[2] << endl;
      return false;
   }
   
   if (! parseCSVDate(tm2, lineData[3])) {
      cerr << "Unable to parse end date: " << lineData[3] << endl;
      return false;
   }
   
   InputData *inputdata1 = new InputData;
   InputData *inputdata2 = new InputData;
   
   inputdata1 -> moid = atoi(lineData[0].c_str());
   inputdata2 -> moid = atoi(lineData[0].c_str());
   
   inputdata1 -> tripid = atoi(lineData[1].c_str());
   inputdata2 -> tripid = atoi(lineData[1].c_str());
   
   cout << lineData[2] << endl;
   
   inputdata1 -> time = tm1;
   inputdata2 -> time = tm2;
   
   inputdata1 -> x = atof(lineData[4].c_str());
   inputdata1 -> y = atof(lineData[5].c_str());
   
   inputdata2 -> x = atof(lineData[6].c_str());
   inputdata2 -> y = atof(lineData[7].c_str());

   data.push(inputdata1);
   data.push(inputdata2);

   return true;
}

bool parseInputData(string &filename, WorkerQueue<InputData*> &data) {
   
   if( access( filename.c_str(), F_OK ) == -1 ) {
      cerr << "Unable to open Input file: " << filename << endl;
      return false;
   }
   
   string line;
   ifstream myfile(filename.c_str());
   
   if (! myfile.is_open()) {
      cerr << "Unable to open file: " << filename << endl;
      return false;
   }
   
   while ( getline (myfile,line) ) {
          
      vector<std::string> lineData;
      stringstream lineStream(line);
      string cell;

      while(getline(lineStream,cell,',')) {
         lineData.push_back(cell);
      }
                    
      if(lineData.size() != 8) {
         cerr << "Invalid line: " << line << " skipping" << endl;
         continue;
      }
          
      handleCSVLine(data, lineData);
   }
   
   myfile.close();
   
   // Add term token
   data.push(NULL);
   
   return true;
}

void printHelpAndExit(char *progName) {
   cerr << "Usage: " << progName << " -i <inputfile> ";
   cerr << "-h <hostname> -p <port>" << endl;
   cerr << endl;
   cerr << "For example: " << progName << " -i trips.csv";
   cerr << "-h localhost -p 10000" << endl;
   exit(-1);
}

void parseParameter(int argc, char *argv[], Configuration *configuration) {
   
   unsigned int flags = 0;
   int option = 0;
   
   while ((option = getopt(argc, argv,"i:h:p:")) != -1) {
       switch (option) {
          case 'i':
             flags |= CMDLINE_INPUTFILE;
             configuration->inputfile = string(optarg);
          break;
          
          case 'h':
             flags |= CMDLINE_DESTHOST;
             configuration->desthost = string(optarg);
          break;
          
          case 'p':
             flags |= CMDLINE_DESTPORT;
             configuration->destport = atoi(optarg);
          break;
          
          default:
            printHelpAndExit(argv[0]);
       }
   }
   
   unsigned int requiredFalgs = CMDLINE_INPUTFILE |
                                CMDLINE_DESTHOST |
                                CMDLINE_DESTPORT;
   
   if(flags != requiredFalgs) {
      printHelpAndExit(argv[0]);
   }
}

class Consumer {  
   
public:
   
   Consumer(WorkerQueue<InputData*> *myQueue) 
      : queue(myQueue), socketfd(-1) {
      
   }
   
   /*
   2.4 Open the network socket

   */
   bool openSocket(char* hostname, int port) {
  
      struct hostent *server;
      struct sockaddr_in server_addr;
   
      socketfd = socket(AF_INET, SOCK_STREAM, 0); 

      if(socketfd < 0) {
         cerr << "Error opening socket" << endl;
         return false;
      }   
   
      // Resolve hostname
      server = gethostbyname(hostname);
   
      if(server == NULL) {
         cerr << "Error resolving hostname: " << hostname << endl;
         return -1; 
      }   
   
      // Connect
      memset(&server_addr, 0, sizeof(server_addr));
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(port);
   
      server_addr.sin_addr.s_addr = 
        ((struct in_addr *)server->h_addr_list[0])->s_addr;
  
      if(connect(socketfd, (struct sockaddr*) &server_addr, 
            sizeof(struct sockaddr)) < 0) {

         cerr << "Error in connect() " << endl;
         return false;
      }   
   
      return true;
   }
   
   void closeSocket() {
      if(socketfd == -1) {
         return;
      }
      
      shutdown(socketfd, 2);
      socketfd = -1;
   }
   
   void dataConsumer() {
      InputData *element = queue->pop();
   
      while(element != NULL) {
         cout << "Consume: " << element->x << " / " 
              << element->y << endl;
         
         delete element;
         
         element = queue->pop();         
      }
      
      cout << "Consumer Done" << endl;
   }  
   
private:
   WorkerQueue<InputData*> *queue;
   int socketfd;
};

void* startConsumerThreadInternal(void *ptr) {
  Consumer* consumer = (Consumer*) ptr;
  consumer -> dataConsumer();
  
  return NULL;
}



int main(int argc, char *argv[]) {
   
   Configuration *configuration = new Configuration();
   Statistics *statistics = new Statistics(); 
   pthread_t readerThread;
   pthread_t writerThread;
   
   parseParameter(argc, argv, configuration);
   
   WorkerQueue<InputData*> inputData(QUEUE_ELEMENTS);
   
   Consumer consumer(&inputData);
   
   pthread_create(&writerThread, NULL, 
                  &startConsumerThreadInternal, &consumer);
   
   
   bool result = parseInputData(configuration->inputfile, inputData);
   
   if(! result) {
      cerr << "Unable to parse input data" << endl;
      exit(-1);
   }
   
   for(size_t i = 0; i < 100; i++) {
      cout << "\r\033[2K" << "Sec: " << i;
      cout << " \033[1m Trips:\033[0m " << statistics -> trips;
      cout << " \033[1m Send:\033[0m " << statistics -> send;
      cout << " \033[1m Discarded:\033[0m " << statistics -> discarded;
      cout.flush();
      usleep(1000 * 1000);
   }
   
   pthread_join(readerThread, NULL);
   pthread_join(writerThread, NULL);

   if(statistics != NULL) {
      delete statistics;
      statistics = NULL;
   }
   
   if(configuration != NULL) {
      delete configuration;
      configuration = NULL;
   }
   
   while(! inputData.isEmpty()) {
      InputData *entry = inputData.pop();
      
      if(entry != NULL) {
         delete entry;
         entry = NULL;
      }
   }
   
   return EXIT_SUCCESS;
}