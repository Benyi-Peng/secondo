using namespace std;

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Application.h"
//#include "Processes.h"
#include "Profiles.h"
#include "FileSystem.h"
#include "SecondoSystem.h"
#include "SecondoInterface.h"
#include "SecondoSMI.h"
#include "NestedList.h"
#include "DisplayTTY.h"


// for testing the Tuple Manager
#include "StandardTypes.h"
#include "Tuple.h"


static const bool needIdent = false;

class SecondoTestFrame : public Application
{
 public:
  SecondoTestFrame( const int argc, const char** argv );
  virtual ~SecondoTestFrame() {};
  int  Execute();
  bool CheckConfiguration();
  
 private:
  string            parmFile;
  string            user;
  string            pswd;
  string            host;
  string            port;
  string            iFileName;
  string            oFileName;
  string            cmd;
//  bool              isStdInput;
  bool              quit;
//  NestedList*       nl;
//  AlgebraLevel      currentLevel;
//  bool              isQuery;
  SecondoInterface* si;
};

SecondoTestFrame::SecondoTestFrame( const int argc, const char** argv )
  : Application( argc, argv )
{
  parmFile      = "";
  user          = "";
  pswd          = "";
  host          = "";
  port          = "";
  iFileName     = "";
  oFileName     = "";
  string cmd    = "";
//  isStdInput    = true;
  quit          = false;
//  nl            = 0;
//  currentLevel  = DescriptiveLevel;
  si            = 0;
}


/*
13 SecondoMain

This is the function where everything is done. If one wants to use the
sotrage manager provided by SHORE, one should struture its program
like this. Since using SHORE makes necessary to be on top of a thread, 
we cannot just initiate the storage manager and then keep in the same
function. Using SHORE we must use this workaround. Call the initiation 
method from the main function, and then we know that the functions
SecondoMain will be "called back"

*/

/*
1 CheckConfiguration

This function checks the Secondo configuration. First it looks for the name
of the configuration file on the command line. If no file name was given on
the command line or a file with the given name does not exist, the environment
variable SECONDO\_HOME is checked. If this variable is defined it should point
to a directory where the configuration file can be found. If the configuration
file is not found there, the current directory will be checked. If no configuration
file can be found the program terminates.

If a valid configuration file was found initialization continues.

*/

bool
SecondoTestFrame::CheckConfiguration()
{
  bool ok = true;
  int i = 1;
  string argSwitch, argValue;
  bool argOk;
  while (i < GetArgCount())
  {
    argSwitch = GetArgValues()[i];
    if ( i < GetArgCount()-1)
    {
      argValue  = GetArgValues()[i+1];
      argOk = (argValue[0] != '-');
    }
    else
    {
      argValue = "";
      argOk = false;
    }
    if ( argSwitch == "-?" || argSwitch == "--help" )  // Help
    {
      cout << "Usage: SecondoTTY{BDB|ORA|CS} [options]" << endl << endl
           << "Options:                                             (Environment)" << endl
           << "  -c config  : Secondo configuration file            (SECONDO_CONFIG)" << endl
           << "  -i input   : Name of input file  (default: stdin)" << endl
           << "  -o output  : Name of output file (default: stdout)" << endl
           << "  -u user    : User id                               (SECONDO_USER)" << endl
           << "  -s pswd    : Password                              (SECONDO_PSWD)" << endl
           << "  -h host    : Host address of Secondo server        (SECONDO_HOST)" << endl
           << "  -p port    : Port of Secondo server                (SECONDO_PORT)" << endl << endl
           << "Command line options overrule environment variables." << endl;
      ok = false;
      break;
    }
    else if ( argOk && argSwitch == "-c" )  // Configuration file
    {
      parmFile = argValue;
    }
    else if ( argOk && argSwitch == "-i" )  // Input file
    {
      iFileName = argValue;
    }
    else if ( argOk && argSwitch == "-o" )  // Output file
    {
      oFileName = argValue;
    }
    else if ( argOk && argSwitch == "-u" )  // User id
    {
      user = argValue;
    }
    else if ( argOk && argSwitch == "-s" )  // Password
    {
      pswd = argValue;
    }
    else if ( argOk && argSwitch == "-h" )  // Host
    {
      host = argValue;
    }
    else if ( argOk && argSwitch == "-p" )  // Port
    {
      port = argValue;
    }
    else
    {
      cout << "Error: Invalid option: '" << argSwitch << "'." << endl;
      if ( argOk )
      {
        cout << "  having option value: '" << argValue << "'." << endl;
      }
      cout << "Use option -? or --help to get information about available options." << endl;
      ok = false;
    }
    i++;
    if ( argOk )
    {
      i++;
    }
  }
  char* envValue;
  if ( parmFile.length() == 0 )
  {
    envValue = getenv( "SECONDO_CONFIG" );
    if ( envValue != 0 )
    {
      parmFile = envValue;
    }
  }
  if ( user.length() == 0 )
  {
    envValue = getenv( "SECONDO_USER" );
    if ( envValue != 0 )
    {
      user = envValue;
    }
  }
  if ( pswd.length() == 0 )
  {
    envValue = getenv( "SECONDO_PSWD" );
    if ( envValue != 0 )
    {
      pswd = envValue;
    }
  }
  if ( host.length() == 0 )
  {
    envValue = getenv( "SECONDO_HOST" );
    if ( envValue != 0 )
    {
      host = envValue;
    }
  }
  if ( port.length() == 0 )
  {
    envValue = getenv( "SECONDO_PORT" );
    if ( envValue != 0 )
    {
      port = envValue;
    }
  }
  if ( needIdent ) // Is user identification needed?
  {
    int count = 0;
    while (count <= 3 && user.length() == 0)
    {
      count++;
      cout << "Enter user id: ";
      getline( cin, user );
    }
    ok = user.length() > 0;
    if ( !ok )
    {
      cout << "Error: No user id specified." << endl;
    }
    if ( ok && pswd.length() == 0 )
    {
      count = 0;
      while (count <= 3 && user.length() == 0)
      {
        count++;
        cout << "Enter password: ";
        getline( cin, pswd );
      }
      if ( pswd.length() == 0 )
      {
        cout << "Error: No password specified." << endl;
        ok = false;
      }
    }
  }
  else
  {
    user = "SECONDO";
    pswd = "SECONDO";
  }
  if ( ok )
  {
    // config file or (host and port) must be specified
    ok = parmFile.length() > 0 || (host.length() > 0 && port.length() > 0);
    if ( !ok )
    {
      cout << "Error: Neither config file nor host and port of Secondo server specified." << endl;
    }
  }
  return (ok);
}

/*
1 Execute

This function checks the configuration of the Secondo system. If the configuration
seems to be ok the system is intialized. If the initialization succeeds the user
commands are processed. If the initialization fails or the user finishes work
the system is terminated.

*/

int SecondoTestFrame::Execute() {
	int rc = 0;
	cout << endl << "*** Secondo Testframe ***" << endl << endl;
	
	if (CheckConfiguration()) {
		si = new SecondoInterface();
		if (si->Initialize(user, pswd, host, port, parmFile)) {
			/* start of test code. */
            SecondoCatalog* exCatalog = 0;
	    	int algId = 0;
	    	int CcRealId = 0, CcIntId =0, CcStringId = 0, CcBoolId =0;
	    
	    	cout << "* Try to get catalog of exec. level. " << endl;
	    	exCatalog = SecondoSystem::GetCatalog(ExecutableLevel);

	    	cout << "* Try to get algebra and type ids. " << endl;
	    	if ( (exCatalog->GetTypeId("int", algId, CcIntId) == true) ) {
	      		cout << "* int --> algId: " << algId << ", typeId: " << CcIntId << endl;
            } 
	    	else {
	      		cout << "* failed" << endl;
	    	}
	    	if ((exCatalog->GetTypeId("string", algId, CcStringId) == true)) {
	      		cout << "* string --> algId: " << algId << ", typeId: " << CcStringId << endl;
            }
	    	else {
	      		cout << "* failed" << endl;
	    	}
	   		if ( (exCatalog->GetTypeId("bool", algId, CcBoolId) == true) ) {
	      		cout << "* bool --> algId: " << algId << ", typeId: " << CcBoolId << endl;
            } 
	    	else {
	      		cout << "* failed" << endl;
	    	}
	    	if ((exCatalog->GetTypeId("real", algId, CcRealId) == true)) {
	      		cout << "* real --> algId: " << algId << ", typeId: " << CcRealId << endl;
            } 
	    	else {
	      		cout << "* failed" << endl;
	    	}
	    
	    	cout << "* Get a reference of the Algebra Manager" << endl;
	    	AlgebraManager* algM = 0;
	    	algM = SecondoSystem::GetAlgebraManager();
	    
	    	cout << "* Get cast function for type real" << endl;
	    	ObjectCast oca;
	    	oca = algM->Cast(algId,CcRealId);
	    
	    	cout << "* assemble attribute types" << endl;
	    
	    	AttributeType realAttr = {algId, CcRealId, sizeof(CcReal)};
	      	AttributeType intAttr = {algId, CcIntId, sizeof(CcInt)};
	      	AttributeType boolAttr = {algId, CcBoolId, sizeof(CcBool)};

         	AttributeType attrTypes[] = { intAttr, boolAttr, realAttr };

	    	cout << "* create tuple attribute description" << endl;
	    
        	TupleAttributes tupleType1(3, attrTypes);
	   
	    	cout << "* create tuple" << endl;
	    	SmiRecordFile *recFile = new SmiRecordFile(false);
			SmiRecordFile *lobFile = new SmiRecordFile(false);

			bool cdb = SmiEnvironment::OpenDatabase("LOBDB");
			if (cdb == false) {
				cout << "* Database opened: NO!!" << endl;
				cdb = SmiEnvironment::CreateDatabase("LOBDB");
				cout << "* Database created:";
				if (cdb == true) {
					cout << "yes" << endl;
				}
				else {
					cout << "NO DATABASE CREATED." << endl;
				}
			}
			else {
				 cout << "* Database opened: yes" << endl;
			}
			
			bool recFileOpen = recFile->Open("RECFILE");
			bool lobFileOpen = lobFile->Open("LOBFILE");

			cout << "* Database opened/created: " << (cdb ? "yes" : 		"NO!!!!!!") << endl;
			cout << "* File for record opened:  " << ((recFileOpen == true) ? "yes" : "NO!!!!!!") << endl;
			cout << "* File for lobs opened:    " << ((lobFileOpen == true) ? "yes" : "NO!!!!!!") << endl;
					
			bool trans = SmiEnvironment::BeginTransaction();
			cout << "* begin of transactions: ";
			cout << ((trans == true) ? " OK" : " failed.") << endl;
			
			Tuple* myTuple;
			CcReal *real1;
			CcInt *int1;
			CcBool *bool1;
			SmiRecordId recId;
			int choice;
			int i;
			 		
			do {
				cout << "* Test menu: " << endl << endl;
				cout << "1) create new tuple and save to file" << endl;
				cout << "2) read tuple from file" << endl;
				cout << "3) read tuple. change attributes and resave to file." << endl;
				cout << "4) create lots of tuples and save to file" << endl;
				cout << "5) show all tuples in the file. " << endl;
				cout << "6) create fresh tuple, destroy it and '(re-)SaveTo' it" << endl;
				cout << endl;
				cout << "9) end." << endl;
			
				cout << "Your choice: ";
				cin >> choice;
				
				float realv;
				int intv;
				char boolv;
				bool bboolv;
				int numberOfTuples;						
				//SmiRecordFileIterator *it;
				//bool more;


				switch (choice) {
					case 1:
						myTuple = new Tuple(&tupleType1);
						cout << "\ta float value, please: "; cin >> realv;
						cout << "\tan int value, please: "; cin >> intv;
						cout << "\tt = true, f = false" << endl;
						cout << "\ta boolean value, please: "; cin >> boolv;
						bboolv = ((boolv == 't') ? true : false);
						real1 = new CcReal(true, realv);
            			int1 = new CcInt(true, intv);
	    				bool1 = new CcBool(true, bboolv);
	    
	    				myTuple->Put(0, int1);
	    				myTuple->Put(1, bool1);
	    				myTuple->Put(2, real1);
					
						cout << "\ttest tuple values" << endl;
	    				cout << "\t" << *myTuple << endl;
						cout << "\tSize: " << myTuple->GetSize() << endl;
						cout << "\tAttributes: " << myTuple->GetAttrNum() << endl;
						for (i = 0; i < myTuple->GetAttrNum(); i++) {
							cout << "\t\t" << *(myTuple->Get(i)) << endl;
						}
						cout << "\tSave tuple into recFile. Persistent id = ";
						myTuple->SaveTo(recFile, lobFile);
						recId = myTuple->GetPersistentId();
						cout << recId << endl;
						
						delete(myTuple);
						
						break;
						
					case 2:
						cout << "\tID:";
						cin >> recId;
						cout << "\trecId = " << recId << endl;
						
						cout << "\treading tuple." << endl;
						myTuple = new Tuple(recFile, recId, &tupleType1, SmiFile::ReadOnly);

						cout << "\ttest tuple values" << endl;
	    				cout << "\t" << *myTuple << endl;
						cout << "\tSize: " << myTuple->GetSize() << endl;
						cout << "\tAttributes: " << myTuple->GetAttrNum() << endl;

					
						delete(myTuple);

						break;
						
					case 3:
						cout << "\tID:";
						cin >> recId;
						cout << "\trecId = " << recId << endl;
						
						cout << "\treading tuple." << endl;
						myTuple = new Tuple(recFile, recId, &tupleType1, SmiFile::ReadOnly);

						cout << "\ttest tuple values" << endl;
	    				cout << "\t" << *myTuple << endl;
						cout << "\tSize: " << myTuple->GetSize() << endl;
						cout << "\tAttributes: " << myTuple->GetAttrNum() << endl;

					
						cout << "\ta new float value, please: "; cin >> realv;
						cout << "\ta new int value, please: "; cin >> intv;
						cout << "\tt = true, f = false" << endl;
						cout << "\ta new boolean value, please: "; cin >> boolv;
						bboolv = ((boolv == 't') ? true : false);
						real1 = new CcReal(true, realv);
            			int1 = new CcInt(true, intv);
	    				bool1 = new CcBool(true, bboolv);
	    
	    				myTuple->Put(0, int1);
	    				myTuple->Put(1, bool1);
	    				myTuple->Put(2, real1);
					
						cout << "\ttest tuple values" << endl;
	    				cout << "\t" << *myTuple << endl;
						cout << "\tSize: " << myTuple->GetSize() << endl;
						cout << "\tAttributes: " << myTuple->GetAttrNum() << endl;

					
						myTuple->Save();
						
						delete(myTuple);
						
						break;
						
					case 4:
						cout << "\tnumber of tuples, please: "; cin >> numberOfTuples;
						
						cout << "\ta start float value, please: "; cin >> realv;
						cout << "\tan start int value, please: "; cin >> intv;
						cout << "\tt = true, f = false" << endl;
						cout << "\ta boolean value, please: "; cin >> boolv;
						bboolv = ((boolv == 't') ? true : false);
						cout << "\tcreating " << numberOfTuples << " tuples. Please be patient." << endl;
						for (int i = 0; i < numberOfTuples; i++) {
							myTuple = new Tuple(&tupleType1);

							real1 = new CcReal(true, realv + i);
            				int1 = new CcInt(true, intv + i);
	    					bool1 = new CcBool(true, bboolv);
	    
	    					myTuple->Put(0, int1);
	    					myTuple->Put(1, bool1);
	    					myTuple->Put(2, real1);
							myTuple->SaveTo(recFile, lobFile);
							recId = myTuple->GetPersistentId();
							
							if ((i > 0) && (i % 10000 == 0)) {
								trans = SmiEnvironment::CommitTransaction();
								cout << "* end (commit) of transactions: ";
								cout << ((trans == true) ? " OK" : " failed.") << endl;
								trans = SmiEnvironment::BeginTransaction();
								cout << "* begin of transactions: ";
								cout << ((trans == true) ? " OK" : " failed.") << endl;
							}
							
							delete(myTuple);
						}
						break;
						
					case 5:
						cout << "\tnot yet implemented." << endl;
						/*
						it = new SmiRecordFileIterator();
						myTuple = new Tuple(&tupleType1);
						do {
							more = it->Next(*myTuple);
							cout << "\t" << *myTuple << endl;
						} while (more); */
						break;
						
					case 6:
						myTuple = new Tuple(&tupleType1);
						cout << "\ta float value, please: "; cin >> realv;
						cout << "\tan int value, please: "; cin >> intv;
						cout << "\tt = true, f = false" << endl;
						cout << "\ta boolean value, please: "; cin >> boolv;
						bboolv = ((boolv == 't') ? true : false);
						real1 = new CcReal(true, realv);
            			int1 = new CcInt(true, intv);
	    				bool1 = new CcBool(true, bboolv);
	    
	    				myTuple->Put(0, int1);
	    				myTuple->Put(1, bool1);
	    				myTuple->Put(2, real1);
					
						cout << "\ttest tuple values" << endl;
	    				cout << "\t" << *myTuple << endl;
						cout << "\tSize: " << myTuple->GetSize() << endl;
						cout << "\tAttributes: " << myTuple->GetAttrNum() << endl;
						
						cout << "\tSave tuple into recFile. Persistent id = ";
						myTuple->SaveTo(recFile, lobFile);
						recId = myTuple->GetPersistentId();
						cout << recId << endl;
						
						cout << "\tDestroying the tuple..." << endl;
						myTuple->Destroy();
						cout << "\tNow this tuple is a fresh one." << endl;
						cout << "\tResave this tuple..." << endl;
						myTuple->SaveTo(recFile, lobFile);
						recId = myTuple->GetPersistentId();
						cout << "\t" << recId << endl;

						break;

					case 9:
						break;
				}
			} while (choice != 9);
					
			trans = SmiEnvironment::CommitTransaction();
			cout << "* end (commit) of transactions: ";
			cout << ((trans == true) ? " OK" : " failed.") << endl;
		
			recFile->Close();
			lobFile->Close();
			
			delete recFile;
			delete lobFile;
    	}
    	si->Terminate();
		
    	delete si;
    	cout << "*** SecondoTestFrame terminated. ***" << endl;
  	}
  	else {
    	rc = 1;
	}
  return rc;
}

/*
14 main

The main function creates the Secondo TTY application and starts its execution.

*/
int main( const int argc, const char* argv[] ) {
  SecondoTestFrame* appPointer = new SecondoTestFrame( argc, argv );
  int rc = appPointer->Execute();
  delete appPointer;
  return (rc);
}



