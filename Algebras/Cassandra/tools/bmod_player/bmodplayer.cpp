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
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "../timer.h"

/*
1.2 Defines

*/
#define CMDLINE_INPUTFILE        1<<0
#define CMDLINE_STATISTICS       1<<1
#define CMDLINE_DESTHOST         1<<2
#define CMDLINE_DESTPORT         1<<3
#define CMDLINE_SIMULATION_MODE  1<<4
#define CMDLINE_BEGINTIME        1<<5

#define QUEUE_ELEMENTS 10000
#define DELIMITER ","

#define SIMULATION_MODE_ADAPTIVE 1
#define SIMULATION_MODE_FIXED    2

using namespace std;

/*
1.3 Structs

*/
struct Configuration {
   string inputfile;
   string statisticsfile;
   string desthost;
   size_t destport;
   short simulationmode;
   time_t beginoffset;
   timeval start;
};

struct Statistics {
   unsigned long read;
   unsigned long send;
   bool done;
};

struct InputData {
    size_t moid;
    size_t tripid;
    tm time_start;
    tm time_end;
    float x_start;
    float y_start;
    float x_end;
    float y_end;
};

struct Position {
   size_t moid;
   size_t tripid;
   tm time;
   float x;
   float y;
};

struct QueueSync {
   pthread_mutex_t queueMutex;
   pthread_cond_t queueCondition;
};

/*
1.4 Compare functions for structs

*/
bool comparePositionTime(const Position* left, const Position* right) { 
   Position* left1  = const_cast<Position*>(left);
   Position* right1 = const_cast<Position*>(right);
   
   time_t left_time = mktime(&left1->time);
   time_t right_time = mktime(&right1->time);
   
   if(left_time <= right_time) {
      return true;
   }
   
   return false;
}

/*
2.0 Abstract Producer class - reads berlin mod csv data 

*/
class AbstractProducer {
public:

    AbstractProducer(Configuration *myConfiguration, 
        Statistics *myStatistics, QueueSync *myQueueSync) : 
        queueSync(myQueueSync) ,
        configuration(myConfiguration), statistics(myStatistics),
        jumpToOffsetDone(false) {
   
    }
    
    virtual ~AbstractProducer() {

    }

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
   
   void jumpToOffset(ifstream &myfile) {
            
      if(jumpToOffsetDone == false && 
         configuration->beginoffset > 0) {
            
         string line;
         struct tm tm1;
                  
         do {
             vector<std::string> lineData;
            
             bool result = getline(myfile,line);
             
             if(!result) {
                continue;
             }
             
             parseLineData(lineData, line);
             memset(&tm1, 0, sizeof(struct tm));
   
             if (! parseCSVDate(tm1, lineData[2])) {
                continue;
             }
             
         } while(mktime(&tm1) < configuration->beginoffset);
         
         jumpToOffsetDone = true;
      }      
   }

   bool parseLineData(vector<std::string> &lineData, string &line) {
      stringstream lineStream(line);
      string cell;

      while(getline(lineStream,cell,',')) {
         lineData.push_back(cell);
      }
                 
      if(lineData.size() != 8) {
         cerr << "Invalid line: " << line << " skipping" << endl;
         return false;
      }
      
      return true;
   }

   bool parseInputData() {
   
      if( access( configuration -> inputfile.c_str(), F_OK ) == -1 ) {
         cerr << "Unable to open input file: " 
              << configuration -> inputfile << endl;
         return false;
      }
   
      string line;
      ifstream myfile(configuration -> inputfile.c_str());
   
      if (! myfile.is_open()) {
         cerr << "Unable to open input file: " 
              << configuration -> inputfile << endl;
         return false;
      }
   
      jumpToOffset(myfile);
   
      while ( getline (myfile,line) ) {
          
         vector<std::string> lineData;
         bool result = parseLineData(lineData, line);
         
         if(result == true) {
            handleCSVLine(lineData);
         }
      }
   
      myfile.close();
   
      handleInputEnd();
   
      return true;
   }
   
   // Abstract methods
   virtual bool handleCSVLine(vector<std::string> &lineData) = 0;
   virtual void handleInputEnd() = 0;
   
protected:
   QueueSync *queueSync;
   Configuration *configuration;
   Statistics *statistics;
   bool jumpToOffsetDone;

private:
};

/*
2.1 Fixed producer class - produced a queue with points

*/
class FixedProducer : public AbstractProducer {

public:
   
   FixedProducer(Configuration *myConfiguration, Statistics *myStatistics, 
        vector<Position*> *myData, QueueSync *myQueueSync) : 
        AbstractProducer(myConfiguration, myStatistics, myQueueSync), 
        data(myData) {
      
        prepareQueue = new vector<Position*>();
   }
   
   virtual ~FixedProducer() {
      
      if(prepareQueue != NULL) {
         delete prepareQueue;
         prepareQueue = NULL;
      }
      
      if(data != NULL) {
         while(! data -> empty()) {
            Position *entry = data->back();
            data -> pop_back();
      
            if(entry != NULL) {
               delete entry;
               entry = NULL;
            }
         }
         
         delete data;
         data = NULL;
      }
   }

   virtual bool handleCSVLine(vector<std::string> &lineData) {
   
      // Skip CSV header
      if(lineData[0] == "Moid") {
         return true;     
      }
         
      // 2007-06-08 08:32:26.781
      struct tm tm1;
      struct tm tm2;
      
      // Init structs
      memset(&tm1, 0, sizeof(struct tm));
      memset(&tm2, 0, sizeof(struct tm));
   
      if (! parseCSVDate(tm1, lineData[2])) {
         cerr << "Unable to parse start date: " << lineData[2] << endl;
         return false;
      }
   
      if (! parseCSVDate(tm2, lineData[3])) {
         cerr << "Unable to parse end date: " << lineData[3] << endl;
         return false;
      }
   
      Position *pos1 = new Position;
      Position *pos2 = new Position;
   
      pos1 -> moid = atoi(lineData[0].c_str());   
      pos2 -> moid = atoi(lineData[0].c_str());   
      
      pos1 -> tripid = atoi(lineData[1].c_str());
      pos2 -> tripid = atoi(lineData[1].c_str());
      
      pos1 -> time = tm1;
      pos2 -> time = tm2;
      
      pos1 -> x = atof(lineData[4].c_str());
      pos1 -> y = atof(lineData[5].c_str());
      
      pos2 -> x = atof(lineData[6].c_str());
      pos2 -> y = atof(lineData[7].c_str());

      putDataIntoQueue(pos1, pos2);
      
      statistics->read = statistics->read + 2;
      
      return true;
   }
   
   void insertIntoQueue(Position *pos) {
      std::vector<Position*>::iterator insertPos
          = upper_bound (prepareQueue->begin(), prepareQueue->end(), 
                         pos, comparePositionTime);
      
      prepareQueue->insert(insertPos, pos);
   }
   
   void printPositionTime(Position *position) {
      char dateBuffer[80];
      strftime(dateBuffer,80,"%d-%m-%Y %H:%M:%S",&position->time);
      cout << "Time is: " << dateBuffer << endl;
   }
   
   void syncQueues(Position *position) {
      pthread_mutex_lock(&queueSync->queueMutex);
      
      bool wasEmpty = data->empty();
      
      while(prepareQueue->size() > 0 && 
            comparePositionTime(position, prepareQueue->front()) == false) {
               
         if(data->size() >= QUEUE_ELEMENTS) {
            pthread_cond_wait(&queueSync->queueCondition, 
                              &queueSync->queueMutex);
         }
         
         data->push_back(prepareQueue->front());
         prepareQueue -> erase(prepareQueue->begin());
      }
      
      if(wasEmpty) {
         pthread_cond_broadcast(&queueSync->queueCondition);
      }
      
      pthread_mutex_unlock(&queueSync->queueMutex);
   }
   
   void putDataIntoQueue(Position *pos1, Position *pos2) {
      
      insertIntoQueue(pos1);
      insertIntoQueue(pos2);
      
      // Move data from prepare queue to real queue
      if(comparePositionTime(pos1, prepareQueue->front()) == false) {
         syncQueues(pos1);
      } 

   }
   
   virtual void handleInputEnd() {
      // Add terminal token
      data->push_back(NULL);
   }

private:
   vector<Position*> *prepareQueue;
   vector<Position*> *data;
};

/*
2.2 Adaptive producer class - produced a queue with ranges

*/
class AdapiveProducer : public AbstractProducer {

public:
   
   AdapiveProducer(Configuration *myConfiguration, Statistics *myStatistics, 
        vector<InputData*> *myData, QueueSync *myQueueSync) : 
        AbstractProducer(myConfiguration, myStatistics, myQueueSync), 
        data(myData) {
      
   }
   
   virtual ~AdapiveProducer() {
      if(data != NULL) {
         while(! data -> empty()) {
            InputData *entry = data->back();
            data -> pop_back();
      
            if(entry != NULL) {
               delete entry;
               entry = NULL;
            }
         }
         
         delete data;
         data = NULL;
      }
   }
   
   void waitForLineRead() {
      
   }

   virtual bool handleCSVLine(vector<std::string> &lineData) {
   
      // Skip CSV header
      if(lineData[0] == "Moid") {
         return true;     
      }
      
      // Wait for line read
      waitForLineRead();
      
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
   
      InputData *inputdata = new InputData;
   
      inputdata -> moid = atoi(lineData[0].c_str());   
      inputdata -> tripid = atoi(lineData[1].c_str());
      inputdata -> time_start = tm1;
      inputdata -> time_end = tm2;
      inputdata -> x_start = atof(lineData[4].c_str());
      inputdata -> y_start = atof(lineData[5].c_str());
      inputdata -> x_end = atof(lineData[6].c_str());
      inputdata -> y_end = atof(lineData[7].c_str());

      putDataIntoQueue(inputdata);
      
      statistics->read++;

      return true;
   }
   
   void putDataIntoQueue(InputData *inputdata) {
      pthread_mutex_lock(&queueSync->queueMutex);
      
      if(data->size() >= QUEUE_ELEMENTS) {
         pthread_cond_wait(&queueSync->queueCondition, 
                           &queueSync->queueMutex);
      }
      
      bool wasEmpty = data->empty();
      
      data->push_back(inputdata);
      
      if(wasEmpty) {
         pthread_cond_broadcast(&queueSync->queueCondition);
      }
      pthread_mutex_unlock(&queueSync->queueMutex);
   }
   
   
   virtual void handleInputEnd() {
      // Add terminal token
      data->push_back(NULL);
   }
   
   
private:
   vector<InputData*> *data;
};


/*
3.0 Consumer class - consumes berlin mod data and write it to a tcp socket

*/
class AbstractConsumer {  
   
public:
   
   AbstractConsumer(Configuration *myConfiguration, Statistics *myStatistics, 
           QueueSync* myQueueSync) : configuration(myConfiguration), 
           statistics(myStatistics), queueSync(myQueueSync), socketfd(-1), 
        ready(false) {
      
   }
   
   virtual ~AbstractConsumer() {
      closeSocket();
   }
   
   /*
   2.4 Open the network socket

   */
   bool openSocket() {
  
      struct hostent *server;
      struct sockaddr_in server_addr;
   
      socketfd = socket(AF_INET, SOCK_STREAM, 0); 

      if(socketfd < 0) {
         cerr << "Error opening socket" << endl;
         return false;
      }   
   
      // Resolve hostname
      server = gethostbyname(configuration->desthost.c_str());
   
      if(server == NULL) {
         cerr << "Error resolving hostname: " 
              << configuration->desthost << endl;
         return -1; 
      }   
   
      // Connect
      memset(&server_addr, 0, sizeof(server_addr));
      server_addr.sin_family = AF_INET;
      server_addr.sin_port = htons(configuration->destport);
   
      server_addr.sin_addr.s_addr = 
        ((struct in_addr *)server->h_addr_list[0])->s_addr;
  
      if(connect(socketfd, (struct sockaddr*) &server_addr, 
            sizeof(struct sockaddr)) < 0) {

         cerr << "Error in connect() " << endl;
         return false;
      }   
   
      ready = true;
      return true;
   }
   
   void closeSocket() {
      if(socketfd == -1) {
         return;
      }
      
      shutdown(socketfd, 2);
      socketfd = -1;
   }
   
   bool sendData(string &buffer) {
      int ret = 0;
      int toSend = buffer.length();
      const char* buf = buffer.c_str();

      for (int n = 0; n < toSend; ) {
          ret = write(socketfd, (char *)buf + n, toSend - n);
          if (ret < 0) {
               if (errno == EINTR || errno == EAGAIN) {
                  continue;
               }
               break;
          } else {
              n += ret;
          }
      }
      
      // All data was written successfully
      if(ret > 0) {
         return true;
      }
      
      return false;
   }
   
   virtual void dataConsumer() = 0;
   

protected:
   Configuration *configuration;
   Statistics *statistics;
   QueueSync *queueSync;
   int socketfd;
   bool ready;
   
private:
};


/*
3.1 FixedConsumer class

*/
class AdaptiveConsumer : public AbstractConsumer {

public:
   
   AdaptiveConsumer(Configuration *myConfiguration, Statistics *myStatistics, 
           vector<InputData*> *myQueue, QueueSync* myQueueSync) 
      : AbstractConsumer(myConfiguration, myStatistics, myQueueSync), 
      queue(myQueue) {
      
   }
   
   InputData* getQueueElement() {
      pthread_mutex_lock(&queueSync->queueMutex);
      
      // Queue empty
      if(queue -> size() == 0) {
         pthread_cond_wait(&queueSync->queueCondition, 
                           &queueSync->queueMutex);
      }
      
      bool wasFull = queue->size() >= QUEUE_ELEMENTS;
      
      InputData *element = queue->front();
      queue -> erase(queue->begin());

      // Queue full
      if(wasFull) {
         pthread_cond_broadcast(&queueSync->queueCondition);
      }
      
      pthread_mutex_unlock(&queueSync->queueMutex);
      
      return element;
   }
   
   virtual void dataConsumer() {
      char dateBuffer[80];
      string buffer;
      stringstream ss;
      
      InputData *element = getQueueElement();
   
      while(element != NULL) {
         if(ready) {
            ss.str("");
             
            strftime(dateBuffer,80,"%d-%m-%Y %H:%M:%S",&element->time_start);

            ss << dateBuffer << DELIMITER;            
            ss << element->moid << DELIMITER;
            ss << element->tripid << DELIMITER;
            ss << element->x_start << DELIMITER;
            ss << element->y_start << "\n";
            
            buffer.clear();
            buffer = ss.str();
            
            bool res = sendData(buffer);

            if(res == true) {
               statistics->send++;
            } else {
               cerr << "Error occurred while calling write on socket" << endl;
            }

         } else {
            cerr << "Socket not ready, ignoring line" << endl;
         }
         
         delete element;
         
         element = getQueueElement();     
      }
      
      statistics -> done = true;
      cout << "Consumer Done" << endl;
   }  
   
protected:
      vector<InputData*> *queue;
      
private:
};

/*
3.2 AdaptiveConsumer class

*/
class FixedConsumer : public AbstractConsumer {
public:
   FixedConsumer(Configuration *myConfiguration, Statistics *myStatistics, 
           vector<Position*> *myQueue, QueueSync* myQueueSync) 
      : AbstractConsumer(myConfiguration, myStatistics, myQueueSync), 
      queue(myQueue) {
      
   }
   
   Position* getQueueElement() {
      pthread_mutex_lock(&queueSync->queueMutex);
       
      // Queue empty
      if(queue -> size() == 0) {
         pthread_cond_wait(&queueSync->queueCondition, 
                           &queueSync->queueMutex);
      }
   
      bool wasFull = queue->size() >= QUEUE_ELEMENTS;
      Position *element = queue->front();
      queue -> erase(queue->begin());
      
      // Queue full
      if(wasFull) {
         pthread_cond_broadcast(&queueSync->queueCondition);
      }
      
      pthread_mutex_unlock(&queueSync->queueMutex);
      
      return element;
   }
   
   virtual void dataConsumer() {
      char dateBuffer[80];
      string buffer;
      stringstream ss;
      
      Position *element = getQueueElement();
   
      while(element != NULL) {
         if(ready) {
            ss.str("");
             
            strftime(dateBuffer,80,"%d-%m-%Y %H:%M:%S",&element->time);

            ss << dateBuffer << DELIMITER;            
            ss << element->moid << DELIMITER;
            ss << element->tripid << DELIMITER;
            ss << element->x << DELIMITER;
            ss << element->y << "\n";
            
            buffer.clear();
            buffer = ss.str();
            
            bool res = sendData(buffer);

            if(res == true) {
               statistics->send++;
            } else {
               cerr << "Error occurred while calling write on socket" << endl;
            }

         } else {
            cerr << "Socket not ready, ignoring line" << endl;
         }
         
         delete element;
         
         element = getQueueElement();     
      }
      
      statistics -> done = true;
      cout << "Consumer Done" << endl;
   }  
   
   
protected:
   vector<Position*> *queue;
   
private:

};

/*
4.0 Statistics class

*/
class StatisticsDisplay {

public:
   
   StatisticsDisplay(Configuration *myConfiguration,
                     Statistics *myStatistics, Timer *myTimer) :
                     configuration(myConfiguration), 
                     statistics(myStatistics), timer(myTimer),
                     outputfile(NULL) {
                        
      openStatistics();
   }
   
   virtual ~StatisticsDisplay() {
      closeStatistics();
   }
   
   void openStatistics() {
      if(outputfile == NULL) {
         outputfile = fopen((configuration->statisticsfile).c_str(), "w");
         
         if(outputfile == NULL) {
            cerr << "Unable to open: " << configuration->statisticsfile 
                 << " for writing, exiting" << endl;
            exit(EXIT_FAILURE);
         }
         
         fprintf(outputfile, "#Sec\tRead\tWrite\n");
      }
   }
   
   void closeStatistics() {
      if(outputfile != NULL) {
         fclose(outputfile);
         outputfile = NULL;
      }
   }
   
   size_t getElapsedSeconds() {
      return timer-> getDiff() / (1000 * 1000);
   }
   
   void printStatisticsData() {
      cout << "\r\033[2K" << "Sec: " << getElapsedSeconds();
      cout << " \033[1m Read:\033[0m " << statistics -> read;
      cout << " \033[1m Send:\033[0m " << statistics -> send;
      cout.flush();
   }
   
   void writeStatisticsData() {
      if(outputfile != NULL) {
         fprintf(outputfile, "%zu\t%lu\t%lu\n", getElapsedSeconds(),
                statistics -> read, statistics -> send);
         fflush(outputfile);
      }
   }
   
   void mainLoop() {
      
      gettimeofday(&lastrun, NULL);
      
      while(statistics->done == false) {
         printStatisticsData();
         writeStatisticsData();
         waitForNextSecond();
      }
   }
   
private:
      void waitForNextSecond() {
         struct timeval curtime;
         struct timeval result;
         
         do {
            usleep(100);
            gettimeofday(&curtime, NULL);
            timersub(&curtime, &lastrun, &result);
         } while(result.tv_sec < 1);
         
         lastrun.tv_sec++;
      }
   
      Configuration *configuration;
      Statistics *statistics;
      Timer *timer;
      FILE *outputfile;
      struct timeval lastrun;
};

void* startConsumerThreadInternal(void *ptr) {
  AbstractConsumer* consumer = (AbstractConsumer*) ptr;
  
  bool res = consumer->openSocket();
  
  if(! res) {
     cerr << "Unable to open socket!" << endl;
     exit(EXIT_FAILURE);
  }
  
  consumer -> dataConsumer();
  
  return NULL;
}

void* startProducerThreadInternal(void *ptr) {
   AbstractProducer* producer = (AbstractProducer*) ptr;
   
   bool result = producer -> parseInputData();
   
   if(! result) {
      cerr << "Unable to parse input data" << endl;
      exit(EXIT_FAILURE);
   }
   
   return NULL;
}

void* startStatisticsThreadInternal(void *ptr) {
   StatisticsDisplay* statistics = (StatisticsDisplay*) ptr;
   
   statistics -> mainLoop();
   
   return NULL;
}

/*
5.0 BerlinModPlayer main class

*/
class BModPlayer {

public:
   
   BModPlayer() {
      configuration = new Configuration();
      gettimeofday(&configuration->start, NULL);
      configuration->beginoffset = 0;
   
      statistics = new Statistics(); 
      statistics->done = false;
   
      timer = new Timer();

      pthread_mutex_init(&queueSync.queueMutex, NULL);
      pthread_cond_init(&queueSync.queueCondition, NULL);
 
   }
   
   void createWorker() {
      if(configuration->simulationmode == SIMULATION_MODE_ADAPTIVE) {
          vector<InputData*> *inputData = new vector<InputData*>();
   
          consumer = new AdaptiveConsumer(configuration, statistics, 
                                  inputData, &queueSync);
                  
          producer = new AdapiveProducer(configuration, statistics, 
                                  inputData, &queueSync);
      } else if(configuration->simulationmode == SIMULATION_MODE_FIXED) {
         vector<Position*> *inputData = new vector<Position*>();
      
         consumer = new FixedConsumer(configuration, statistics, 
                                 inputData, &queueSync);
      
         producer = new FixedProducer(configuration, statistics, 
                                 inputData, &queueSync);
      } else {
         cerr << "Unknown simulation mode" << endl;
         exit(EXIT_FAILURE);
      }
   }
   
   void run(int argc, char *argv[]) {

      parseParameter(argc, argv, configuration);

      createWorker();

      StatisticsDisplay statisticsDisplay(configuration, 
                        statistics, timer);

      // Create worker threads
      pthread_create(&readerThread, NULL, 
                     &startProducerThreadInternal, producer);

      pthread_create(&writerThread, NULL, 
                     &startConsumerThreadInternal, consumer);

      pthread_create(&statisticsThread, NULL, 
                     &startStatisticsThreadInternal, 
                     &statisticsDisplay);
                  
      timer->start();
   
      // Wait for running threads
      pthread_join(readerThread, NULL);
      pthread_join(writerThread, NULL);
   
      statistics->done = true;
      pthread_join(statisticsThread, NULL);
   
      pthread_mutex_destroy(&queueSync.queueMutex);
      pthread_cond_destroy(&queueSync.queueCondition);
      
      cleanup();
   }
   
private:
   
   void printHelpAndExit(char *progName) {
      cerr << "Usage: " << progName << " -i <inputfile> -o <statisticsfile> ";
      cerr << "-h <hostname> -p <port> -s <adaptive|fixed> -b <beginoffset>";
      cerr << endl;
      cerr << endl;
      cerr << "-i is the CVS file with the trips to simulate" << endl;
      cerr << "-o is the output file for statistics" << endl;
      cerr << "-h specifies the hostname to connect to" << endl;
      cerr << "-p specifies the port to connect to" << endl;
      cerr << "-s sets the simulation mode" << endl;
      cerr << "-b is the time offset for the input data" << endl;
      cerr << endl;
      cerr << "For example: " << progName << " -i trips.csv ";
      cerr << "-o statistics.txt -h localhost ";
      cerr << "-p 10000 -s adaptive -b '2007-05-28 06:00:14'" << endl;
      exit(-1);
   }

   void parseParameter(int argc, char *argv[], Configuration *configuration) {
   
      unsigned int flags = 0;
      int option = 0;
   
      while ((option = getopt(argc, argv,"i:o:h:p:s:b:")) != -1) {
          switch (option) {
             case 'i':
                flags |= CMDLINE_INPUTFILE;
                configuration->inputfile = string(optarg);
             break;
             
             case 'o':
                flags |= CMDLINE_STATISTICS;
                configuration->statisticsfile = string(optarg);
             break;
          
             case 'h':
                flags |= CMDLINE_DESTHOST;
                configuration->desthost = string(optarg);
             break;
          
             case 'p':
                flags |= CMDLINE_DESTPORT;
                configuration->destport = atoi(optarg);
             break;

             case 's':
                flags |= CMDLINE_SIMULATION_MODE;

                if(strcmp(optarg,"adaptive") == 0) {
                   configuration->simulationmode = SIMULATION_MODE_ADAPTIVE;
                } else if(strcmp(optarg,"fixed") == 0) {
                   configuration->simulationmode = SIMULATION_MODE_FIXED;
                } else {
                    cerr << "Unknown simulation mode: " << optarg << endl;
                    cerr << endl;
                    printHelpAndExit(argv[0]);
                }
             break;
             
             case 'b':
                 flags |= CMDLINE_BEGINTIME;
                 struct tm tm;
                 
                 if (! strptime(optarg, "%Y-%m-%d %H:%M:%S", &tm)) {
                    cerr << "Unable to parse date: " << optarg << endl << endl;
                    printHelpAndExit(argv[0]);
                 }
                 
                 configuration->beginoffset = mktime(&tm);
             break;
          
             default:
               printHelpAndExit(argv[0]);
          }
      }
   
      unsigned int requiredFalgs = CMDLINE_INPUTFILE |
                                   CMDLINE_STATISTICS |
                                   CMDLINE_DESTHOST |
                                   CMDLINE_DESTPORT |
                                   CMDLINE_SIMULATION_MODE;
   
      if((flags & requiredFalgs) != requiredFalgs) {
         printHelpAndExit(argv[0]);
      }
   }
   
   void cleanup() {
      if(consumer != NULL) {
         delete consumer;
         consumer = NULL;
      }
   
      if(producer != NULL) {
         delete producer;
         producer = NULL;
      }

      if(statistics != NULL) {
         delete statistics;
         statistics = NULL;
      }
   
      if(configuration != NULL) {
         delete configuration;
         configuration = NULL;
      }
   
      if(timer != NULL) {
         delete timer;
         timer = NULL;
      }
   }
   
   Configuration *configuration;
   Statistics *statistics;
   Timer *timer;
   AbstractConsumer *consumer;
   AbstractProducer *producer;
   
   QueueSync queueSync;
   pthread_t readerThread;
   pthread_t writerThread;
   pthread_t statisticsThread;
};

/*
6.0 Main Function

*/
int main(int argc, char *argv[]) {
   BModPlayer bModPlayer;
   bModPlayer.run(argc, argv);
   
   return EXIT_SUCCESS;
}
