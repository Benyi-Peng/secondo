/*
---- 
This file is part of SECONDO.

Copyright (C) 2004, University in Hagen, Department of Computer Science, 
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
    
Implementation of the Secondo Server Module

2002 U. Telle. 
 
2003-2004 M. Spiekermann. Minor modifications for messages,
binary encoded list transfer and error handling of socket streams.

December 2005, Victor Almeida deleted the deprecated algebra levels
(~executable~, ~descriptive~, and ~hybrid~). Only the executable
level remains.

February 2006. M. Spiekermann. Changes in the client server protocol. Now
the ~restore~ and ~save~ object/database commands work correctly. Moreover the
code was simplified and restructured. As a consequence protocol related procedures 
are implemented in class ~CSProtocol~ and can be used by inside the 
~TTYCS~ and the ~Server~ code. 



*/

#include <cstdlib>
#include <string>
#include <algorithm>
#include <map>
#include <climits>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

#include "Application.h"
#include "SocketIO.h"
#include "Messenger.h"
#include "AlgebraTypes.h"
#include "SecondoSystem.h"
#include "SecondoCatalog.h"
#include "SecondoInterface.h"
#include "FileSystem.h"
#include "Profiles.h"
#include "LogMsg.h"
#include "StopWatch.h"

#include "CSProtocol.h"
#include "NList.h"



class SecondoServer;
typedef void (SecondoServer::*ExecCommand)();

class SecondoServer : public Application
{
 public:
  SecondoServer( const int argc, const char** argv ) : 
     Application( argc, argv ) 
  {};
  virtual ~SecondoServer() {};
  int Execute();
  void CallSecondo();
  void CallNumericType();
  void CallGetTypeId();
  void CallLookUpType();
  void CallDbSave();
  void CallObjectSave();
  void CallDbRestore();
  void CallObjectRestore();
  void Connect();
  void Disconnect();
  void WriteResponse( const int errorCode, const int errorPos,
                      const string& errorMessage, ListExpr resultList );
  bool ReceiveFile( const string& clientFileName,
                    const string& serverFileName );
  
 private:

  void CallRestore(const string& tag, bool database=false);
  void CallSave(const string& tag, bool database=false);
  string CreateTmpName(const string& prefix);
  
  Socket*           client;
  SecondoInterface* si;
  NestedList*       nl;
  string            parmFile;
  bool              quit;
  string            registrar;
  string            user;
  string            pswd;

  CSProtocol* csp;
};


void
SecondoServer::WriteResponse( const int errorCode, const int errorPos,
                              const string& errorMessage, ListExpr resultList )
{
  ListExpr msg = nl->TextAtom();
  nl->AppendText( msg, errorMessage );
  
  ListExpr list = nl->FourElemList(
                    nl->IntAtom( errorCode ),
                    nl->IntAtom( errorPos ),
                    msg,
                    resultList );

  iostream& iosock = client->GetSocketStream();
 
  csp->IgnoreMsg(true); 
  iosock << "<SecondoResponse>" << endl;
  csp::sendList(iosock, NList(list));  
  iosock << "</SecondoResponse>" << endl;
  
}

void
SecondoServer::CallSecondo()
{
  string line="", cmdText="", cmdEnd="";
  iostream& iosock = client->GetSocketStream();
  int type=0;
  iosock >> type;
  csp->skipRestOfLine();
  csp->IgnoreMsg(false);
  bool ready=false;
  do
  {
    getline( iosock, line );
    ready = (line == "</Secondo>");
    if ( !ready )
    {
      cmdText += line + "\n";
    }
  }
  while (!ready && !iosock.fail());
  ListExpr commandLE = nl->TheEmptyList();
  ListExpr resultList = nl->TheEmptyList();
  int errorCode=0, errorPos=0;
  string errorMessage="";
  si->Secondo( cmdText, commandLE, type, true, false, 
               resultList, errorCode, errorPos, errorMessage );
  NList::setNLRef(nl);
  WriteResponse( errorCode, errorPos, errorMessage, resultList );
}

void
SecondoServer::CallNumericType()
{
  string typeStr, cmdEnd;
  iostream& iosock = client->GetSocketStream();
  iosock.clear();
  getline( iosock, typeStr );
  getline( iosock, cmdEnd );
  if ( cmdEnd == "</NumericType>" )
  {
    ListExpr typeList = 0;
    nl->ReadFromString( "("+typeStr+")", typeList );
    ListExpr list = si->NumericTypeExpr( nl->First(typeList) );
    nl->WriteToString( typeStr, list );
    iosock << "<NumericTypeResponse>" << endl
           << typeStr << endl
           << "</NumericTypeResponse>" << endl;
    nl->Destroy(list);
    nl->Destroy(typeList);
  }
  else
  {
    iosock << "<SecondoError>" << endl
           << "SECONDO-0080 Protocol error: </NUMERICTYPE> expected." << endl
           << "</SecondoError>" << endl;
  }
}

void
SecondoServer::CallGetTypeId()
{
  string name, cmdEnd;
  int algebraId, typeId;
  iostream& iosock = client->GetSocketStream();
  iosock.clear();
  iosock >> name;
  csp->skipRestOfLine();
  getline( iosock, cmdEnd );
  if ( cmdEnd == "</GetTypeId>" )
  {
    bool ok = si->GetTypeId( name, algebraId, typeId );
    if ( ok )
    {
      iosock << "<GetTypeIdResponse>" << endl
             << algebraId << " " << typeId << endl
             << "</GetTypeIdResponse>" << endl;
    }
    else
    {
      iosock << "<GetTypeIdResponse>" << endl
             << 0 << " " << 0 << endl
             << "</GetTypeIdResponse>" << endl;
    }
  }
  else
  {
    iosock << "<SecondoError>" << endl
           << "SECONDO-0080 Protocol error: </GetTypeId> expected." << endl
           << "</SecondoError>" << endl;
  }
}

void
SecondoServer::CallLookUpType()
{
  string name, typeStr, cmdEnd;
  int algebraId, typeId;
  iostream& iosock = client->GetSocketStream();
  getline( iosock, typeStr );
  getline( iosock, cmdEnd );
  if ( cmdEnd == "</LookUpType>" )
  {
    ListExpr typeList;
    nl->ReadFromString( "(" + typeStr + ")", typeList );
    si->LookUpTypeExpr( nl->First(typeList), name, algebraId, typeId );

    iosock << "<LookUpTypeResponse>" << endl
           << "((" << name << ") " << algebraId << " " << typeId << ")" << endl
           << "</LookUpTypeResponse>" << endl;
    nl->Destroy(typeList);
  }
  else
  {
    iosock << "<SecondoError>" << endl
           << "SECONDO-0080 Protocol error: </LookUpType> expected." << endl
           << "</SecondoError>" << endl;
  }
}



string
SecondoServer::CreateTmpName(const string& prefix)
{
  ostringstream os;
  os << prefix << GetOwnProcessId();
  return os.str();
}       


void
SecondoServer::CallSave(const string& tag, bool database /*=false*/)
{

  //cout << "Begin CallSave()" << endl;
 
  // parameters for Secondo interface call
  SI_Error errorCode=ERR_NO_ERROR;
  int errorPos=0;
  string errorMessage="";
  ListExpr commandLE = nl->TheEmptyList();
  ListExpr resultList = nl->TheEmptyList();

  string cmdText="";
  string serverFileName = CreateTmpName(tag);
  // Read client message and construct Secondo command
  if (!database)
  {
    // read in object or database name
    iostream& iosock = client->GetSocketStream();
    string name = "";
    iosock >> name;
    csp->skipRestOfLine();

    if(!csp->nextLine("</"+tag+">", errorMessage))
      errorCode = ERR_IN_SECONDO_PROTOCOL;
      
    cmdText= "(save " + name + " to " + serverFileName + ")";
  }
  else
  {
    cmdText = "(save database to " + serverFileName + ")";
  }  
  
  // create file on server 
  if (errorCode == ERR_NO_ERROR)
  {
   
    si->Secondo( cmdText, commandLE, 0, true, false, 
                 resultList, errorCode, errorPos, errorMessage );
    NList::setNLRef(nl); 

  } 

  // If successful create list from file
  if (errorCode == ERR_NO_ERROR)
  {
    nl->ReadFromFile( serverFileName, resultList );
  } 
   
  WriteResponse( errorCode, errorPos, errorMessage, resultList );
  //FileSystem::DeleteFileOrFolder( serverFileName );

  //cout << "End CallSave()" << endl;
}


void
SecondoServer::CallDbSave()
{
  CallSave("DbSave", true);
}


void
SecondoServer::CallObjectSave()
{
  CallSave("ObjectSave");
}


void
SecondoServer::CallRestore(const string& tag, bool database/*=false*/)
{
  // parameters for Secondo interface call
  SI_Error errorCode=0;
  int errorPos=0;
  string errorMessage="";
  ListExpr commandLE = nl->TheEmptyList();
  ListExpr resultList = nl->TheEmptyList();
  bool readLastLine=false;

  
  // read in object or database name
  iostream& iosock = client->GetSocketStream();
  string name = "";
  iosock >> name;
  csp->skipRestOfLine();
  
  //cout << "Begin CallRestore()" << endl;
  
  string serverFileName= CreateTmpName(tag);
  if ( csp->ReceiveFile( serverFileName ) )
  {
    string cmdText="";

    // construct Secondo command
    if (database)
    {
      cmdText = "(restore database " + name +
                " from " + serverFileName + ")";
            
    }
    else
    {       
      cmdText= "(restore " + name +
               " from " + serverFileName + ")";
    } 
    
    si->Secondo( cmdText, commandLE, 0, true, false, 
                 resultList, errorCode, errorPos, errorMessage );
    NList::setNLRef(nl);
    
    readLastLine=true;
  }
  else
  {
    
    errorCode = ERR_IN_SECONDO_PROTOCOL;
    errorMessage = "Protocol-Error: File not received correctly."; 
    resultList = nl->TheEmptyList();
    readLastLine=false;
  }
  
  if ( readLastLine && !csp->nextLine("</"+tag+">", errorMessage) )
  {
    errorCode = ERR_IN_SECONDO_PROTOCOL;
  }
  WriteResponse( errorCode, errorPos, errorMessage, resultList );
  //FileSystem::DeleteFileOrFolder( serverFileName );

  //cout << "End CallRestore()" << endl;
}


void
SecondoServer::CallObjectRestore()
{
  CallRestore("ObjectRestore");    
}       

void
SecondoServer::CallDbRestore()
{
  CallRestore("DbRestore",true);        
}       


void
SecondoServer::Connect()
{
  iostream& iosock = client->GetSocketStream();
  string line;
  getline( iosock, user );
  getline( iosock, pswd );
  cout << "user = " << user << endl;
  cout << "passwd = " << pswd << endl;
  getline( iosock, line ); //eat up </USER> ?
  
}

void
SecondoServer::Disconnect()
{
  quit = true;
}

int
SecondoServer::Execute()
{
  int rc = 0;
  parmFile = (GetArgCount() > 1) ? GetArgValues()[1] : "SecondoConfig.ini";
  registrar = SmiProfile::GetParameter( "Environment", "RegistrarName", 
                                        "SECONDO_REGISTRAR", parmFile );
  si = new SecondoInterface(true);
  cout << "Initialize the secondo interface " << endl;

  map<string,ExecCommand> commandTable;
  map<string,ExecCommand>::iterator cmdPos;
  commandTable["<Secondo>"]     = &SecondoServer::CallSecondo;
  commandTable["<NumericType>"] = &SecondoServer::CallNumericType;
  commandTable["<GetTypeId>"]   = &SecondoServer::CallGetTypeId;
  commandTable["<LookUpType>"]  = &SecondoServer::CallLookUpType;
  commandTable["<DbSave/>"]      = &SecondoServer::CallDbSave;
  commandTable["<ObjectSave>"]  = &SecondoServer::CallObjectSave;
  commandTable["<ObjectRestore>"]   = &SecondoServer::CallObjectRestore;
  commandTable["<DbRestore>"]   = &SecondoServer::CallDbRestore;
  commandTable["<Connect>"]     = &SecondoServer::Connect;
  commandTable["<Disconnect/>"] = &SecondoServer::Disconnect;

  string logMsgList = SmiProfile::GetParameter( "Environment", 
                                                "RTFlags", "", parmFile );
  RTFlag::initByString(logMsgList);


  client = GetSocket();
  if(!client){
    rc = -2;
  } else {
    iostream& iosock = client->GetSocketStream();
    csp = new CSProtocol(nl, iosock, true);
    //si->SetProtocolPtr(csp);
    
    ios_base::iostate s = iosock.exceptions();
    iosock.exceptions(ios_base::failbit|ios_base::badbit|ios_base::eofbit);
    iosock << "<SecondoOk/>" << endl;
    quit = false;
    // first connect to get the user and password information
    try {
        string cmd;
        getline( iosock, cmd );
        cmdPos = commandTable.find( cmd );
        if ( cmdPos != commandTable.end() )
        {
           (*this.*(cmdPos->second))();
        }
        else
        {
          iosock << "<SecondoError>" << endl
                 << "SECONDO-0080 Protocol-Error: Start tag \"" 
                 << cmd << "\" unknown!" << endl
                 << "</SecondoError>" << endl;
        }
        if ( Application::Instance()->ShouldAbort() )
        {
          iosock << "<SecondoError>" << endl
                 << "SECONDO-9999 Server going down. Disconnecting." << endl
                 << "</SecondoError>" << endl;
          quit = true;
        }
    
      } catch (ios_base::failure) {
        cerr << endl 
             << "I/O error on socket stream object!" 
             << endl;
        if ( !client->IsOk() ) {
           cerr << "Socket Error: " << client->GetErrorText() << endl;  
         }
       quit = true; 
      }

    cout << "Try to initialize the secondo system " << endl; 
    if ( si->Initialize( user, pswd, "", "", parmFile, true ) )
    {
       cout << "initialization successful" << endl;
       iosock << "<SecondoIntro>" << endl
              << "You are connected with a Secondo server." << endl
              << "</SecondoIntro>" << endl;
       //Messenger messenger( registrar );
       //string answer;
       //ostringstream os;
       //os << "LOGIN " << user << " " << GetOwnProcessId();
       //messenger.Send( os.str(), answer );
       SmiEnvironment::SetUser( user );

      // initialization successfull, send ok and wait for further requests



      nl = si->GetNestedList();
      NList::setNLRef(nl);


      do {
      try {
        string cmd;
        getline( iosock, cmd );
        cmdPos = commandTable.find( cmd );
        if ( cmdPos != commandTable.end() )
        {
           (*this.*(cmdPos->second))();
        }
        else
        {
          iosock << "<SecondoError>" << endl
                 << "SECONDO-0080 Protocol-Error: Start tag \"" 
                 << cmd << "\" unknown!" << endl
                 << "</SecondoError>" << endl;
        }
        if ( Application::Instance()->ShouldAbort() )
        {
          iosock << "<SecondoError>" << endl
                 << "SECONDO-9999 Server going down. Disconnecting." << endl
                 << "</SecondoError>" << endl;
          quit = true;
        }
    
      } catch (ios_base::failure) {
        cerr << endl 
             << "I/O error on socket stream object!" 
             << endl;
        if ( !client->IsOk() ) {
           cerr << "Socket Error: " << client->GetErrorText() << endl;  
         }
       quit = true; 
      }

      } while (!iosock.fail() && !quit);
      
      iosock.exceptions(s);

      client->Close();
      delete client;
      //os << "LOGOUT " << user << " " << GetOwnProcessId();
      //messenger.Send( os.str(), answer );
    } else {
       iosock << "<SecondoError>" << endl
              << "Initialization failed (username, password correct?)" << endl
              << "</SecondoError>" << endl;
      rc = -1;
    }
  }

  si->Terminate();
  delete si;
  return (rc);
}

int SecondoServerMode( const int argc, const char* argv[] )
{
  SecondoServer* appPointer = new SecondoServer( argc, argv );
  int rc = appPointer->Execute();
  delete appPointer;
  return (rc);
}

