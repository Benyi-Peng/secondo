/*

*/
#ifndef SECONDO_TES_MESSAGESERVER_H
#define SECONDO_TES_MESSAGESERVER_H

#include <string>
#include <functional>
#include <SocketIO.h>
#include <memory>
#include <queue>
#include <boost/thread.hpp>
#include "Message.h"
//#include "QueueSupplier.h"
//#include "Inbox.h"

#include "../typedefs.h"

namespace distributed3 {
  class MessageServer {

  public:
  explicit MessageServer(std::shared_ptr<Socket> socket );
  ~MessageServer();
  void run();
  void interrupt();

  bool hasTuple(const int eid, const int slot); 
  Tuple* getTuple(const int eid, const int slot);
  void handleFinishedMessage(const int eid);
  void initializeInboxes(const int eid, const int numberOfSlots);
 
  private:
  boost::thread* thread = nullptr;
  std::shared_ptr<Socket> socket;

  void processMessage();
  void handleDataMessage(Message::Header& header);
  
  void pushTuple(const int eid, const int slot, char* tuple);
  std::map<int, std::map<int, boost::mutex>> inboxmutex;
  std::map<int, std::map<int, std::queue<char*>* > > inboxes; 
  
  
  };
}

#endif //SECONDO_MESSAGESERVER_H
