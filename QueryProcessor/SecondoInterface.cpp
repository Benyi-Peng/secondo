/*

1 The Implementation-Module SecondoInterface

September 1996 Claudia Freundorfer

December 23, 1996 RHG Added error code information.

January 17, 1998 RHG Connected Secondo Parser (= command level 1 available).

This module implements the module ~SecondoInterface~ by using the
modules ~NestedList~, ~SecondoCatalog~, ~QueryProcessor~ and
~StorageManager~.

May 15, 1998 RHG Added a command ~model value-expression~ which is
analogous to ~query value-expression~ but computes the result model for
a given query rather than the result value.

November 18, 1998 Stefan User commands ``abort transaction'' and
``commit transaction'' are implemented by calling SMI\_Abort() and
SMI\_Commit(), respectively.

April 2002 Ulrich Telle Port to C++

August 2002 Ulrich Telle Set the current algebra level for SecondoSystem.

September 2002 Ulrich Telle Close database after creation.

November 7, 2002 RHG Implemented the ~let~ command.

December 2002 M. Spiekermann Changes in Secondo(...) and NumTypeExpr(...).

February 3, 2003 RHG Added a ~list counters~ command.

April 29 2003 Hoffmann Added save and restore commands for single objects.

April 29, 2003 M. Spiekermann bug fix in LookUpTypeExpr(...).

April 30 2003 Hoffmann Changes syntax for the restore objects command.

September 2003 Hoffmann Extended section List-Commands for Secondo-Commands
~list algebras~ and ~list algebra <algebra name>~.

October 2003 M. Spiekermann made the command echo (printing out the command in NL format) configurable.
This is useful for server configuration, since the output of big lists consumes more time than processing
the command.

May 2004, M. Spiekermann. Support of derived objects (for further Information see DerivedObj.h) introduced.
A new command derive similar to let can be used by the user to create objects which are derived from other 
objects via a more or less complex value expression. The information about dependencies is stored in two 
system tables (relation objects). The save database command omits to save list expressions for those objects.
After restoring all saved objects the derived objects are rebuild in the restore database command.

\tableofcontents

*/

using namespace std;

#include <iostream>
#include <fstream>

#include "SecondoInterface.h"
#include "SecondoSystem.h"
#include "SecondoCatalog.h"
#include "QueryProcessor.h"
#include "AlgebraManager.h"
#include "Profiles.h"
#include "FileSystem.h"
#include "LogMsg.h"

#include "RelationAlgebra.h"
#include "StandardTypes.h"

#include "SecondoSMI.h"
#include "SecParser.h"
#include "StopWatch.h"
#include "LogMsg.h"
#include "Counter.h"
#include "DerivedObj.h"

extern AlgebraListEntry& GetAlgebraEntry( const int j );

static SecondoSystem* ss = 0;

/**************************************************************************

1 Implementation of class SecondoInterface

*/

int
DerivedObj::ObjRecord::id = 0;

/*

1.1 Constructor

*/

SecondoInterface::SecondoInterface()
  : initialized( false ), activeTransaction( false ), nl( 0 ), server( 0 ), derivedObjPtr(0)
{
}


/*

1.2 Destructor

*/

SecondoInterface::~SecondoInterface()
{
  if ( initialized )
  {
    Terminate();
  }
}


/*

1.3 Initialize method

*/

bool
SecondoInterface::Initialize( const string& user, const string& pswd,
                              const string& host, const string& port,
                              string& parmFile, const bool multiUser )
{
  bool ok = false;
  cout << "Checking configuration ..." << endl;
  if ( parmFile.length() > 0 )
  {
    bool found = false;
    cout << "Configuration file '" << parmFile;
    found = FileSystem::FileOrFolderExists( parmFile );
    if ( found )
    {
      cout << "':" << endl;
    }
    else
    {
      cout << "' not found!" << endl;
    }
    if ( !found )
    {
      cout << "Searching environment for configuration file ..." << endl;
      char* home = getenv( "SECONDO_HOME" );
      if ( home != 0 )
      {
        parmFile = home;
        FileSystem::AppendSlash( parmFile );
        parmFile += "SecondoConfig.ini";
        cout << "Configuration file '" << parmFile;
        found = FileSystem::FileOrFolderExists( parmFile );
        if ( found )
        {
          cout << "':" << endl;
        }
        else
        {
          cout << "' not found!" << endl;
        }
      }
      else
      {
        cout << "Environment variable SECONDO_HOME not defined." << endl;
      }
      if ( !found )
      {
        cout << "Searching current directory for configuration file ..." << endl;
        string cwd = FileSystem::GetCurrentFolder();
        FileSystem::AppendSlash( cwd );
        parmFile = cwd + "SecondoConfig.ini";
        cout << "Configuration file '" << parmFile;
        found = FileSystem::FileOrFolderExists( parmFile );
        if ( found )
        {
          cout << "':" << endl;
        }
        else
        {
          cout << "' not found!" << endl;
        }
      }
    }

    // initialize runtime flags
    string logMsgList = SmiProfile::GetParameter( "Environment", "RTFlags", "", parmFile );    
    RTFlag::initByString(logMsgList);
    RTFlag::showActiveFlags(cout);

    string value, foundValue;
    if ( SmiProfile::GetParameter( "Environment", "SecondoHome", "", parmFile ) == "")
    {
      cout << "Error: Secondo home directory not specified." << endl;
    }
    else
    {
      ok = true;
    }
  }
  else
  {
    cout << "Error: No configuration file specified." << endl;
    return (false);
  }

  if ( ok )
  {
    // --- Check storage management interface
    SmiEnvironment::RunMode mode =  SmiEnvironment::SingleUser;
    if ( RTFlag::isActive("SMI:NoTransactions") ) {
       
       mode = SmiEnvironment::SingleUserSimple;
       cout << "Setting mode to SingleUserSimple" << endl;
       
    } else { // Transactions and logging are used
    
       mode = (multiUser) ? SmiEnvironment::MultiUser : SmiEnvironment::SingleUser;
    }                   
    
    cout << "Initializing storage management interface ... ";
    if ( SmiEnvironment::StartUp( mode, parmFile, cout ) )
    {
      SmiEnvironment::SetUser( user ); // TODO: Check for valid user/pswd
      cout << "completed." << endl;
      ok = true;
    }
    else
    {
      cout << "failed." << endl;
      string errMsg;
      SmiEnvironment::GetLastErrorCode( errMsg );
      cout << "Error: " << errMsg << endl;
      ok = false;
    }
  }
  if (ok)
  {
    cout << "Initializing the Secondo system ... ";
    ss = new SecondoSystem( &GetAlgebraEntry );
    nl = SecondoSystem::GetNestedList();
    al = SecondoSystem::GetAppNestedList();
    ok = SecondoSystem::StartUp();
    if ( ok )
    {
      cout << "completed." << endl;
    }
    else
    {
      cout << "failed." << endl;
    }
  }
  initialized = ok;
  return (ok);
}


/*

1.4 Terminate method

*/


void
SecondoInterface::Terminate()
{
  if ( initialized )
  {    
    if ( derivedObjPtr != 0 ) { // The destructor closes a relation object 
      delete derivedObjPtr;
      derivedObjPtr = 0;
    }    

    cout << "Terminating Secondo system ...";
    // --- Abort open transaction, if there is an open transaction
    if ( activeTransaction )
    {
      SecondoSystem::AbortTransaction();
      activeTransaction = false;
    }
    // --- Close database, if one is open
    if ( SecondoSystem::GetInstance()->IsDatabaseOpen() )
    {
      SecondoSystem::GetInstance()->CloseDatabase();
    }
    if ( SecondoSystem::ShutDown() )
    {
      cout << "completed." << endl;
    }
    else
    {
      cout << "failed." << endl;
    }
    if ( ss != 0 )
    {
      delete ss;
      ss = 0;
    }

    if ( !SmiEnvironment::ShutDown() )
    {
      string errMsg;
      SmiEnvironment::GetLastErrorCode( errMsg );
      cout << "Error: Shutdown of the storage management interface failed." << endl;
      cout << "Error: " << errMsg << endl;
    }
    initialized = false;
    activeTransaction = false;
    nl = 0;
    al = 0;
    server = 0;
  }
  else
  {
    cout << "Error: Secondo system already terminated." << endl;
  }
}


/*

1.5 The Secondo method. The Interface for applications.

*/

void
SecondoInterface::Secondo( const string& commandText,
                           const ListExpr commandLE,
                           const int commandLevel,
                           const bool commandAsText,
                           const bool resultAsText,
                           ListExpr& resultList,
                           int& errorCode,
                           int& errorPos,
                           string& errorMessage,
                           const string& resultFileName /* = "SecondoResult" */ )
{
/*

~Secondo~ reads a command and executes it; it possibly returns a result.
The command is one of a set of SECONDO commands.

Error Codes: see definition module.

If value 0 is returned, the command was executed without error.

*/

  ListExpr first,     list,       typeExpr,  resultType,
           valueList, modelList, valueExpr,
           typeExpr2, errorList,  errorInfo, functionList;
  string filename, dbName, objName, typeName;
  Word result;
  OpTree tree;
  int length;
  bool correct      = false;
  bool evaluable    = false;
  bool defined      = false;
  bool isFunction   = false;
  int message;                /* error code from called procedures */
  string listCommand;         /* buffer for command in list form */
  AlgebraLevel level;

  StopWatch cmdTime;  // measure the time used for executing the command.

  SecParser sp;
  NestedList* nl = SecondoSystem::GetNestedList();
  NestedList* al = SecondoSystem::GetAppNestedList();


  // copy command list to internal NL memory
  ListExpr commandLE2 = nl->TheEmptyList();
  if (commandLE) {
     commandLE2 = al->CopyList(commandLE, nl);
  }


  errorMessage = "";
  errorCode    = 0;
  errorPos     = 0;
  errorList    = nl->OneElemList( nl->SymbolAtom( "ERRORS" ) );
  errorInfo    = errorList;
  resultList   = nl->TheEmptyList();

  switch (commandLevel)
  {
    case 0:  // executable, list form
    {
      level = ExecutableLevel;
      if ( commandAsText )
      {
        if ( !nl->ReadFromString( commandText, list ) )
        {
          errorCode = ERR_SYNTAX_ERROR;  // syntax error in command/expression
        }
      }
      else
      {
        list = commandLE2;
      }
      break;
    }
    case 1:  // executable, text form
    {
      level = ExecutableLevel;
      if ( sp.Text2List( commandText, listCommand, errorMessage ) != 0 )
      {
        errorCode = ERR_SYNTAX_ERROR;  // syntax error in command/expression
      }
      else if ( !nl->ReadFromString( listCommand, list ) )
      {
        errorCode = ERR_SYNTAX_ERROR;  // syntax error in command/expression
      }
      break;
    }
    case 2:
    {
      level = DescriptiveLevel;
      if ( commandAsText )
      {
        if ( !nl->ReadFromString( commandText, list ) )
        {
          errorCode = ERR_SYNTAX_ERROR;  // syntax error in command/expression
        }
      }
      else
      {
        list = commandLE2;
      }
      break;
    }
    case 3:
    {
      level = DescriptiveLevel;
      if ( sp.Text2List( commandText, listCommand, errorMessage ) != 0 )
      {
        errorCode = ERR_SYNTAX_ERROR;  // syntax error in command/expression
      }
      else if ( !nl->ReadFromString( listCommand, list ) )
      {
        errorCode = ERR_SYNTAX_ERROR;  // syntax error in command/expression
      }
      break;
    }
    default:
    {
      errorCode = ERR_CMD_LEVEL_NOT_YET_IMPL;  // Command level not implemented
      return;
    }
  } // switch
  if ( errorCode != 0 )
  {
    return;
  }
  SecondoSystem::SetAlgebraLevel( level );

  // local references of important objects
  QueryProcessor& qp = *SecondoSystem::GetQueryProcessor();
  SecondoCatalog& ctlg = *SecondoSystem::GetCatalog(level);
  SecondoSystem& sys = *SecondoSystem::GetInstance();


  if (!RTFlag::isActive("SI:NoCommandEcho")) {
     nl->WriteListExpr(list, cerr);
     cerr << endl;
  }
  length = nl->ListLength( list );
  if ( length > 1 )
  {
    first = nl->First( list );

    // --- Transaction handling

    if ( nl->IsEqual( nl->Second( list ), "transaction" ) && (length == 2) )
    {
      if ( nl->IsEqual( first, "begin" ) )
      {
        if ( !activeTransaction )
        {
          if ( SecondoSystem::BeginTransaction() )
          {
            activeTransaction = true;
          }
          else
          {
            errorCode = ERR_BEGIN_TRANSACTION_FAILED;
          }
        }
        else
        {
          errorCode = ERR_TRANSACTION_ACTIVE;
        }
      }
      else if ( nl->IsEqual( first, "commit" ) )
      {
        if ( activeTransaction )
        {
          if ( !SecondoSystem::CommitTransaction() )
          {
            errorCode = ERR_COMMIT_OR_ABORT_FAILED;
          }
          activeTransaction = false;
        }
        else
        {
          errorCode = ERR_NO_TRANSACTION_ACTIVE;
        }
      }
      else if ( nl->IsEqual( first, "abort" ) )
      {
        if ( activeTransaction )
        {
          if ( !SecondoSystem::AbortTransaction() )
          {
            errorCode = ERR_COMMIT_OR_ABORT_FAILED;
          }
          activeTransaction = false;
        }
        else
        {
          errorCode = ERR_NO_TRANSACTION_ACTIVE;
        }
      }
      else
      {
        errorCode = ERR_CMD_NOT_RECOGNIZED;  // Command not recognized.
      }
    }

    // --- Database commands

    else if ( nl->IsEqual( nl->Second( list ), "database" ) )
    {
      if ( nl->IsEqual( first, "create" ) && (length == 3) &&
           nl->IsAtom( nl->Third( list ) ) &&
          (nl->AtomType( nl->Third( list ) ) == SymbolType) )
      {
        if ( sys.IsDatabaseOpen() )
        {
          errorCode = ERR_DATABASE_OPEN;  // a database is open
        }
        else
        {
          dbName = nl->SymbolValue( nl->Third( list ) );
          if ( sys.CreateDatabase( dbName ) )
          {
            sys.CloseDatabase();
          }
          else
          {
            errorCode = ERR_IDENT_USED;   // identifier already used
          }
        }
      }
      else if ( nl->IsEqual( first, "delete" ) &&
               (length == 3) && nl->IsAtom( nl->Third( list ) ) &&
               (nl->AtomType( nl->Third( list ) ) == SymbolType) )
      {
        if ( sys.IsDatabaseOpen() )
        {
          errorCode = ERR_DATABASE_OPEN;  // a database is open
        }
        else
        {
          dbName = nl->SymbolValue( nl->Third( list ) );
          if ( !sys.DestroyDatabase( dbName ) )
          {
            errorCode = ERR_IDENT_UNKNOWN_DB_NAME;  // identifier not a known database name
          }
        }
      }
      else if ( nl->IsEqual( first, "open" ) &&
               (length == 3) && nl->IsAtom( nl->Third( list ) ) &&
               (nl->AtomType( nl->Third( list ) ) == SymbolType) )
      {
        if ( sys.IsDatabaseOpen() )
        {
          errorCode = ERR_DATABASE_OPEN;  // a database is open
        }
        else
        {
          dbName = nl->SymbolValue( nl->Third( list ) );
          if ( !sys.OpenDatabase( dbName ) )
          {
            errorCode = ERR_IDENT_UNKNOWN_DB_NAME;  // identifier not a known database name
	    
          } else {
	    if ( !derivedObjPtr )
	       derivedObjPtr = new DerivedObj(); // create new instance if necessary
	  }
        }
      }
      else if ( nl->IsEqual( first, "close" ) )
      {
        if ( !sys.IsDatabaseOpen() )
        {
          errorCode = ERR_NO_DATABASE_OPEN;  // no database open
        }
        else
        {
          if ( activeTransaction )
          {
            SecondoSystem::CommitTransaction();
            activeTransaction = false;
          }
          sys.CloseDatabase();
	  if ( derivedObjPtr );
	    delete derivedObjPtr;
	  derivedObjPtr = 0;
        }
      }
      else if ( nl->IsEqual( first, "save" ) && (length == 4) &&
                nl->IsEqual( nl->Third( list ), "to" ) &&
                nl->IsAtom( nl->Fourth( list )) &&
               (nl->AtomType( nl->Fourth( list )) == SymbolType) )
      {
        if ( !sys.IsDatabaseOpen() )
        {
          errorCode = ERR_NO_DATABASE_OPEN;  // no database open
        }
        else
        {
          StartCommand();
          filename = nl->SymbolValue( nl->Fourth( list ) );
          if ( !sys.SaveDatabase( filename, *derivedObjPtr ) )
          {
            errorCode = ERR_PROBLEM_IN_WRITING_TO_FILE;  // Problem in writing to file
          }
          FinishCommand( errorCode );
        }
      }
      else if ( nl->IsEqual( first, "restore" ) &&
               (length == 5) && nl->IsAtom( nl->Third( list )) &&
               (nl->AtomType( nl->Third( list )) == SymbolType) &&
                nl->IsEqual( nl->Fourth( list ), "from" ) &&
                nl->IsAtom( nl->Fifth( list )) &&
               (nl->AtomType( nl->Fifth( list )) == SymbolType) )
      {
      
        if ( sys.IsDatabaseOpen() )
        {
          errorCode = ERR_DATABASE_OPEN;  // database open
        }
        else
        {
	  cout << endl;
          dbName = nl->SymbolValue( nl->Third( list ) );
          filename = nl->SymbolValue( nl->Fifth( list ) );
          message = sys.RestoreDatabase( dbName, filename, errorInfo );
          
	  if ( derivedObjPtr )
	     delete derivedObjPtr;           // create a new instance of DerivedObj and rebuild
	  derivedObjPtr = new DerivedObj();  // derived objects if they are stored in the system
	  derivedObjPtr->rebuildObjs();      // table SEC_DERIVED_OBJ

          switch (message)
          {
            case 0:
              break;
            case 1:
              errorCode = ERR_IDENT_UNKNOWN_DB_NAME;  // identifier not a known database name
              break;
            case 2:
              errorCode = ERR_DB_NAME_NEQ_IDENT;  // database name in file different from identifier
              break;
            case 3:
              errorCode = ERR_PROBLEM_IN_READING_FILE;  // problem in reading the file
              break;
            case 4:
              errorCode = ERR_IN_LIST_STRUCTURE_IN_FILE;  // error in list structure in file
              break;
            case 5:
              errorCode = ERR_IN_DEFINITIONS_FILE;  // error in type or object definitions in file
              resultList = errorList;
              break;
            default:
              errorCode = ERR_CMD_NOT_RECOGNIZED;   // Should never occur
              break;
          }
        }
		      
        if ( message > 0 ) {        
	 cerr << endl << "Error during restore detected. Closing database " << dbName;
	 if ( !sys.CloseDatabase() ) {
    	    cerr << " failed";
	 }
	 cerr << "!" << endl << endl;
        }

      }
      else
      {
        errorCode = ERR_CMD_NOT_RECOGNIZED;  // Command not recognized.
      }
    }

    // --- Writing command for objects

    else if ( nl->IsEqual( first, "save" ) &&
              nl->IsEqual( nl->Third( list ), "to" ) && (length == 4) &&
              nl->IsAtom( nl->Second( list )) &&
              nl->AtomType( nl->Second( list )) == SymbolType &&
	      nl->IsAtom( nl->Fourth( list )) &&
              nl->AtomType( nl->Fourth( list )) == SymbolType )
    {
      filename = nl->SymbolValue( nl->Fourth( list ) );
      string objectname = nl->SymbolValue( nl->Second( list ) );

      if ( !sys.IsDatabaseOpen() )
      {
        errorCode = ERR_NO_DATABASE_OPEN;  // no database open
      }
      else
      {
        if ( !sys.IsDatabaseObject( objectname ) )
	{
	  errorCode = 82; // object is not known in the database
	}
        else
        {
          StartCommand();
          if ( !sys.SaveObject( objectname,
	    filename ) )
          {
            errorCode = ERR_PROBLEM_IN_WRITING_TO_FILE;  // Problem in writing to file
          }
          FinishCommand( errorCode );
        }
      }
    }

    // --- Reading command for objects

    else if ( nl->IsEqual( first, "restore" ) &&
             (length == 4) && nl->IsAtom( nl->Second( list )) &&
             (nl->AtomType( nl->Second( list )) == SymbolType) &&
              nl->IsEqual( nl->Third( list ), "from" ) &&
              nl->IsAtom( nl->Fourth( list )) &&
             (nl->AtomType( nl->Fourth( list )) == SymbolType) )
    {
      if ( !sys.IsDatabaseOpen() )
      {
        errorCode = ERR_NO_DATABASE_OPEN;  // no database open
      }
      else
      {
        objName = nl->SymbolValue( nl->Second( list ) );
        filename = nl->SymbolValue( nl->Fourth( list ) );

        if ( sys.IsDatabaseObject( objName ) )
	{
	  errorCode = 84; // object is not known in the database
	}
	else
	{

          message = sys.RestoreObjectFromFile( objName, filename, errorInfo );
          switch (message)
          {
            case 0:
              break;
            case 1:
              errorCode = 83;  // object name in file different from identifier
              break;
            case 2:
              errorCode = ERR_PROBLEM_IN_READING_FILE;  // problem in reading the file
              break;
            case 3:
              errorCode = ERR_IN_LIST_STRUCTURE_IN_FILE;  // error in list structure in file
              break;
            case 4:
              errorCode = ERR_IN_DEFINITIONS_FILE;  // error in object definition in file
              resultList = errorList;
              break;
            default:
              errorCode = ERR_CMD_NOT_RECOGNIZED;   // Should never occur
              break;
          }
	}
      }
    }

    // --- List commands

    else if ( nl->IsEqual( first, "list" ) )
    {
      if ( nl->IsEqual( nl->Second( list ), "algebras" ) && (length == 2) )
      {
          resultList =
	    nl->TwoElemList(nl->SymbolAtom("inquiry"),
			    nl->TwoElemList(nl->SymbolAtom("algebras"),
			      SecondoSystem::GetAlgebraManager( )->ListAlgebras() ));
      }

      else if ( nl->IsEqual( nl->Second( list ), "algebra" ) && (length == 3) &&
                nl->IsAtom(nl->Third(list)) && nl->AtomType( nl->Second( list ) )
		== SymbolType )
      {
          if ( SecondoSystem::GetAlgebraManager( )->GetAlgebraId(
	       nl->SymbolValue( nl->Third(list) )))
	  {
	    int aid = SecondoSystem::GetAlgebraManager( )->GetAlgebraId(
	       nl->SymbolValue( nl->Third(list) ));
	       
	    ListExpr constOp =  nl->TwoElemList( ctlg.ListTypeConstructors( aid ),
	                                         ctlg.ListOperators( aid ));
	    resultList =
	    nl->TwoElemList( nl->SymbolAtom("inquiry"),
			     nl->TwoElemList( nl->SymbolAtom("algebra"),
					      nl->TwoElemList( nl->Third(list), constOp )));
	  }
	  else errorCode = 85;
      }


      else if ( nl->IsEqual( nl->Second( list ), "type" ) 
                && (length == 3) 
		&&  nl->IsEqual( nl->Third( list ), "constructors" ) )
      {
        resultList = nl->TwoElemList( nl->SymbolAtom("inquiry"),
			              nl->TwoElemList( nl->SymbolAtom("constructors"),
	                                               ctlg.ListTypeConstructors() ));
      }
      else if ( nl->IsEqual( nl->Second(list), "operators" ) )
      {
        resultList = nl->TwoElemList( nl->SymbolAtom("inquiry"),
			              nl->TwoElemList( nl->SymbolAtom("operators"),
                                                       ctlg.ListOperators() ));
      }
      else if ( nl->IsEqual( nl->Second( list ), "databases" ) )
      {
        resultList = nl->TwoElemList( nl->SymbolAtom("inquiry"),
			              nl->TwoElemList( nl->SymbolAtom("databases"),
			                               sys.ListDatabaseNames() ));
      }
      else if ( nl->IsEqual( nl->Second( list ), "types") )
      {
        if ( !sys.IsDatabaseOpen() )
        {
          errorCode = ERR_NO_DATABASE_OPEN;  // no database open
        }
        else
        {
          StartCommand();
          resultList = nl->TwoElemList( nl->SymbolAtom("inquiry"),
			                nl->TwoElemList( nl->SymbolAtom("types"),
                                                         ctlg.ListTypes() ));
          FinishCommand( errorCode );
        }
      }
      else if ( nl->IsEqual( nl->Second( list ), "objects" ) )
      {
        if ( !sys.IsDatabaseOpen() )
        {
          errorCode = ERR_NO_DATABASE_OPEN;  // no database open
        }
        else
        {
          StartCommand();
          resultList = nl->TwoElemList( nl->SymbolAtom("inquiry"),
			                nl->TwoElemList( nl->SymbolAtom("objects"),
                                                         ctlg.ListObjects() ));
          FinishCommand( errorCode );
        }
      }
      else if ( nl->IsEqual( nl->Second( list ), "counters" ) )
      {
        if ( !sys.IsDatabaseOpen() )
        {
          errorCode = ERR_NO_DATABASE_OPEN;  // no database open
        }
        else
        {
          StartCommand();
          resultList = qp.GetCounters();
          FinishCommand( errorCode );
        }
      }
      else
      {
        errorCode = ERR_CMD_NOT_RECOGNIZED;  // Command not recognized.
      }
    }

    // --- Type definition

    else if ( nl->IsEqual( first, "type" ) &&
             (length == 4) && nl->IsAtom( nl->Second( list ) ) &&
             (nl->AtomType( nl->Second( list )) == SymbolType) &&
              nl->IsEqual( nl->Third( list ), "=" ) )
    {
      if ( sys.IsDatabaseOpen() )
      {
        StartCommand();
        typeName = nl->SymbolValue( nl->Second( list ) );
        typeExpr = nl->Fourth( list );
        typeExpr2 = ctlg.ExpandedType( typeExpr );
	
        if ( ctlg.KindCorrect( typeExpr2, errorInfo ) )
        {
          if ( !ctlg.InsertType( typeName, typeExpr2 ) )
          {
            errorCode = ERR_IDENT_USED;  // identifier already used
          }
        }
        else
        {
          errorCode = ERR_NO_TYPE_DEFINED;     // Wrong type expression
          resultList = errorList;
        }
        FinishCommand( errorCode );
      }
      else
      {
        errorCode = ERR_NO_DATABASE_OPEN;       // no database open
      }
    }
    
    // delete type
    
    else if ( nl->IsEqual( first, "delete" ) )
    {
      if ( (length == 3) && nl->IsAtom( nl->Third( list ) ) &&
           (nl->AtomType( nl->Third( list )) == SymbolType) &&
            nl->IsEqual( nl->Second( list ), "type" ) )
      {
        if ( sys.IsDatabaseOpen() )
        {
          StartCommand();
          typeName = nl->SymbolValue( nl->Third( list ) );
          message = ctlg.DeleteType( typeName );
          if ( message == 1 )
          {
            errorCode = ERR_TYPE_NAME_USED_BY_OBJ;   // Type used by an object
          }
          else if ( message == 2 )
          {
            errorCode = ERR_IDENT_UNKNOWN_TYPE;   // identifier not a known type name
          }
          FinishCommand( errorCode );
        }
        else
        {
          errorCode = ERR_NO_DATABASE_OPEN;      // no database open
        }
      }
      
      // delete object
      
      else if ( (length == 2) && nl->IsAtom( nl->Second( list )) &&
                (nl->AtomType( nl->Second( list )) == SymbolType) )
      {
        objName = nl->SymbolValue( nl->Second( list ) );
	
        if ( !sys.IsDatabaseOpen() )
	   errorCode = ERR_NO_DATABASE_OPEN;  // no database open

        if ( !errorCode && ctlg.IsSystemObject(objName) )
           errorCode = ERR_IDENT_RESERVED;

        StartCommand();  
        if ( !errorCode ) {
           message = ctlg.DeleteObject( objName );
           if ( message > 0 ) {
              errorCode = ERR_IDENT_UNKNOWN_OBJ;   // identifier not a known object name
          
	   } else {
	     derivedObjPtr->deleteObj( objName ); // delete from derived objects table if necessary
           }
	}
	FinishCommand( errorCode );
	
     } else { // no correct delete syntax
     
       errorCode = ERR_CMD_NOT_RECOGNIZED;  // Command not recognized
     }
    } 

    // --- Create object command

    else if ( nl->IsEqual( first, "create" ) && (length == 4) &&
              nl->IsAtom( nl->Second( list )) &&
             (nl->AtomType( nl->Second( list ) ) == SymbolType) &&
              nl->IsEqual( nl->Third( list ), ":" ) )
    {
      if ( sys.IsDatabaseOpen() )
      {
        StartCommand();
        objName = nl->SymbolValue( nl->Second( list ) );
        typeExpr = nl->Fourth( list );
        typeExpr2 =
        ctlg.ExpandedType( typeExpr );
        typeName = "";
        if ( ctlg.KindCorrect( typeExpr2, errorInfo ) )
        {
          if ( nl->IsAtom( typeExpr ) &&
              (nl->AtomType( typeExpr ) == SymbolType) )
          {
            typeName = nl->SymbolValue( typeExpr );
            if ( !ctlg.MemberType( typeName ) )
            {
              typeName = "";
            }
          }
	  if ( ctlg.IsSystemObject(objName) ) {
	     errorCode = ERR_IDENT_RESERVED;
	     
	  } else if ( !ctlg.CreateObject( objName, typeName, typeExpr2, 0 ) )
          {
            errorCode = ERR_IDENT_USED;  // identifier already used
          }
        }
        else
        {
          errorCode = ERR_NO_OBJ_CREATED;     // Wrong type expression
          resultList = errorList;
        }
        FinishCommand( errorCode );
      }
      else
      {
        errorCode = ERR_NO_DATABASE_OPEN;       // no database open
      }
    }

    // --- Update object command

    else if ( nl->IsEqual( first, "update" ) && (length == 4) &&
              nl->IsAtom( nl->Second( list )) &&
             (nl->AtomType( nl->Second( list ) ) == SymbolType) &&
              nl->IsEqual( nl->Third( list ), ":=" ) )
    {
      if ( sys.IsDatabaseOpen() )
      {
        if ( level == DescriptiveLevel )
        {
          errorCode = ERR_CMD_NOT_IMPL_AT_THIS_LEVEL;  // Command not yet implemented at this level
        }
        else
        {
          StartCommand();
          objName = nl->SymbolValue( nl->Second( list ) );
          valueExpr = nl->Fourth( list );
          qp.Construct( level, valueExpr, correct, evaluable, defined,
                         isFunction, tree, resultType );
          if ( !defined )
          {
            errorCode = ERR_UNDEF_OBJ_VALUE;      // Undefined object value in expression
          }
          else if ( correct )
          {
	    if ( ctlg.IsSystemObject(objName) ) {
	       errorCode = ERR_IDENT_RESERVED;	     
   
            } else if ( derivedObjPtr && derivedObjPtr->isDerived(objName) ) {
	       errorCode = ERR_UPDATE_FOR_DERIVED_OBJ_UNSUPPORTED;
	        
	    } else if ( !ctlg.IsObjectName( objName ) )
            {
              errorCode = ERR_IDENT_UNKNOWN_OBJ;   // identifier not a known object name
            }
            else
            {	     	    
              typeExpr = ctlg.GetObjectTypeExpr( objName );

              if ( !nl->Equal( typeExpr, resultType ) )
              {
                errorCode = ERR_EXPR_TYPE_NEQ_OBJ_TYPE;   // types of object and expression do not agree
              }
              else if ( evaluable )
              {
                qp.Eval( tree, result, 1 );
                if ( IsRootObject( tree ) && !IsConstantObject( tree ) )
                {
                   ctlg.CloneObject( objName, result );
                   qp.Destroy( tree, true );
                }
                else
                {
                   ctlg.UpdateObject( objName, result );
                   qp.Destroy( tree, false );
                }
              }
              else if ( isFunction )   // abstraction or function object
              {
                if ( nl->IsAtom( valueExpr ) )  // function object
                {
                  functionList = ctlg.GetObjectValue( nl->SymbolValue( valueExpr ) );
                  ctlg.UpdateObject( objName, SetWord( functionList ) );
                }
                else
                {
                  ctlg.UpdateObject( objName, SetWord( valueExpr ) );
                }
              }
              else
              {
                errorCode = ERR_EXPR_NOT_EVALUABLE;   // Expression not evaluable
              }
            }
          }
          else
          {
            errorCode = ERR_IN_QUERY_EXPR;    // Error in expression
          }
          FinishCommand( errorCode );
        }
      }
      else
      {
        errorCode = ERR_NO_DATABASE_OPEN;        // no database open
      }
    }


    // --- Let command

    else if ( nl->IsEqual( first, "let" ) && (length == 4) &&
              nl->IsAtom( nl->Second( list )) &&
             (nl->AtomType( nl->Second( list ) ) == SymbolType) &&
              nl->IsEqual( nl->Third( list ), "=" ) )
    {
      if ( SecondoSystem::GetInstance()->IsDatabaseOpen() )
      {
        if ( level == DescriptiveLevel )
        {
          errorCode = ERR_CMD_NOT_IMPL_AT_THIS_LEVEL;  // Command not yet implemented at this level
        }
        else
        {
          StartCommand();
          objName = nl->SymbolValue( nl->Second( list ) );
          valueExpr = nl->Fourth( list );
	  
	  if ( ctlg.IsSystemObject(objName) ) {
	     errorCode = ERR_IDENT_RESERVED;
	     
	  } else if ( ctlg.IsObjectName(objName) )
          {
            errorCode = ERR_IDENT_USED;   // identifier is already used
          }
          else
          {
            qp.Construct( level, valueExpr, correct, evaluable, defined,
                          isFunction, tree, resultType );
            if ( !defined )
            {
              errorCode = ERR_UNDEF_OBJ_VALUE;      // Undefined object value in expression
            }
            else if ( correct )
            {
              if ( evaluable || isFunction )
              {
		  typeName = "";
 		  ctlg.CreateObject(objName, typeName, resultType, 0);
              }
              if ( evaluable )
              {
                qp.Eval( tree, result, 1 );

                if( IsRootObject( tree ) && !IsConstantObject( tree ) )
                {
                  ctlg.CloneObject( objName, result );
                  qp.Destroy( tree, true );
                }
                else
                {
                  ctlg.UpdateObject( objName, result );
                  qp.Destroy( tree, false );
                }
              }
              else if ( isFunction )   // abstraction or function object
              {
                if ( nl->IsAtom( valueExpr ) )  // function object
                {
                   functionList = ctlg.GetObjectValue( nl->SymbolValue( valueExpr ) );
                   ctlg.UpdateObject( objName, SetWord( functionList ) );
                }
                else
                {
                   ctlg.UpdateObject( objName, SetWord( valueExpr ) );
                }
              }
              else
              {
                errorCode = ERR_EXPR_NOT_EVALUABLE;   // Expression not evaluable
              }
            }
            else
            {
              errorCode = ERR_IN_QUERY_EXPR;    // Error in expression
            }
          }
          FinishCommand( errorCode );
        }
      }
      else
      {
        errorCode = ERR_NO_DATABASE_OPEN;        // no database open
      }
    }

   // --- derive command



    else if ( nl->IsEqual( first, "derive" ) && (length == 4) &&
              nl->IsAtom( nl->Second( list )) &&
             (nl->AtomType( nl->Second( list ) ) == SymbolType) &&
              nl->IsEqual( nl->Third( list ), "=" ) )
    {
      if ( !sys.IsDatabaseOpen() ) {
         errorCode = ERR_NO_DATABASE_OPEN;  // no database open
      }

      if ( !errorCode && level == DescriptiveLevel ) {
         errorCode = ERR_CMD_NOT_IMPL_AT_THIS_LEVEL;  // Command not yet implemented at this level
      }
      
      if ( !errorCode ) { // if no errors ocurred continue

         StartCommand();
         objName = nl->SymbolValue( nl->Second( list ) );
         valueExpr = nl->Fourth( list );

	 if ( ctlg.IsSystemObject(objName) ) {
	    errorCode = ERR_IDENT_RESERVED;
	     
         } else if ( ctlg.IsObjectName(objName) ) {
            errorCode = ERR_IDENT_USED;   // identifier is already used
 
         } else {

           errorCode = derivedObjPtr->createObj(objName, valueExpr);   
	   if ( !errorCode )
             derivedObjPtr->addObj( objName, valueExpr );
         }
       }

       FinishCommand( errorCode );
     }
 
    // --- Query command

    else if ( nl->IsEqual( first, "query" ) && (length == 2) )
    {
      if ( sys.IsDatabaseOpen() )
      {
        if ( level == DescriptiveLevel )
        {
          errorCode = ERR_CMD_NOT_IMPL_AT_THIS_LEVEL;  // Command not yet implemented at this level
        }
        else
        {
          Counter::getRef("SmiRecord::Write")=0;
          Counter::getRef("SmiRecord::Read")=0;
          
          StartCommand();

	  StopWatch queryTime;
	  cerr << "Analyze query ..." << endl;;

          qp.Construct( level, nl->Second( list ), correct, evaluable, defined,
                        isFunction, tree, resultType );


	  if (!RTFlag::isActive("SI:NoQueryAnalysis")) {
	    cerr << "Analyze " << queryTime.diffTimes() << endl;
	    queryTime.start();
	    //cerr << nl->ReportTableSizes() << endl;
          }

          if ( !defined )
          {
            errorCode = ERR_UNDEF_OBJ_VALUE;         // Undefined object value
          }
          else if ( correct )
          {
            if ( evaluable )
            {
              cerr << "Execute ..." << endl;

	      qp.Eval( tree, result, 1 );
              valueList = ctlg.OutObject( resultType, result );
              resultList = nl->TwoElemList( resultType, valueList );
              qp.Destroy( tree, true );

	       if (!RTFlag::isActive("SI:NoQueryAnalysis")) {
	         cerr << "Execute "<< queryTime.diffTimes() << endl;
	         cerr << nl->ReportTableSizes() << endl;
	       }
               if (RTFlag::isActive("SI:PrintCounters")) {
                 Counter::reportValues();
               }

               LOGMSG( "SI:Statistics", Tuple::ShowTupleStatistics( true, cout ); )
               LOGMSG( "SI:Statistics", ShowStandardTypesStatistics( true, cout ); )
            }
            else if ( isFunction ) // abstraction or function object
            {
              if ( nl->IsAtom( nl->Second( list ) ) )  // function object
              {
                functionList = ctlg.GetObjectValue( nl->SymbolValue( nl->Second( list ) ) );
                resultList = nl->TwoElemList( resultType, functionList );
              }
              else
              {
                resultList = nl->TwoElemList( resultType, nl->Second( list ) );
              }
            }
            else
            {
              ErrorReporter::GetErrorMessage(errorMessage);
							ErrorReporter::Reset();
              errorCode = ERR_EXPR_NOT_EVALUABLE;  // Query not evaluable
							
            }
          }
          else
          {
            ErrorReporter::GetErrorMessage(errorMessage);
						ErrorReporter::Reset();
            errorCode = ERR_IN_QUERY_EXPR;    // Error in query
          }
          FinishCommand( errorCode );
        }
      }
      else
      {
        errorCode = ERR_NO_DATABASE_OPEN;        // no database open
      }
    }

    // --- Model command

    else if ( nl->IsEqual( first, "model" ) && (length == 2) )
    {
      if ( SecondoSystem::GetInstance()->IsDatabaseOpen() )
      {
        StartCommand();
        qp.Construct( level, nl->Second( list ), correct, evaluable, defined,
                      isFunction, tree, resultType );
		      
        if ( !defined )
        {
          errorCode = ERR_UNDEF_OBJ_VALUE;         // Undefined object value
        }
        else if ( correct )
        {
          if ( evaluable )
          {
            qp.EvalModel( tree, result );
            modelList = ctlg.OutObjectModel( resultType, result );
            resultList = nl->TwoElemList( resultType, modelList );
            qp.Destroy( tree, true );
          }
          else
          {
            errorCode = ERR_EXPR_NOT_EVALUABLE;   // Query not evaluable
          }
        }
        else
        {
          errorCode = ERR_IN_QUERY_EXPR;     // Error in query
        }
        FinishCommand( errorCode );
      }
      else
      {
        errorCode = ERR_NO_DATABASE_OPEN;       // no database open
      }
    }
    else
    {
      errorCode = ERR_CMD_NOT_RECOGNIZED;         // Command not recognized
    }
  }
  if ( resultAsText )
  {
    nl->WriteToFile( resultFileName, resultList );
  }
  SecondoSystem::SetAlgebraLevel( UndefinedLevel );

  LOGMSG( "SI:ResultList",
    cerr << endl << "### Result List before copying: " << nl->ToString(resultList) << endl;
  )

  // copy result into application specific list container.
  StopWatch copyTime;
  if (resultList) {
     resultList = nl->CopyList(resultList, al);
     LOGMSG( "SI:CopyListTime",
        cerr << "CopyList " << copyTime.diffTimes() << endl;
     )
  }
  LOGMSG( "SI:ResultList",
    cerr << endl << "### Result after copying: " << al->ToString(resultList) << endl;
  )
  nl->initializeListMemory();
  
  LOGMSG( "SI:CommandTime",
    cerr << endl << "Command " << cmdTime.diffTimes() << endl;
  )
  
}

/*
1.3 Procedure ~NumericTypeExpr~

*/
ListExpr
SecondoInterface::NumericTypeExpr( const AlgebraLevel level, const ListExpr type )
{
  SecondoSystem::SetAlgebraLevel( level );
  ListExpr list = nl->TheEmptyList();
  if ( SecondoSystem::GetInstance()->IsDatabaseOpen() )
  {
    // use application specific list memory
    list = SecondoSystem::GetCatalog( level )->NumericType( al->CopyList(type,nl) );
    list = nl->CopyList(list, al);
  }
  SecondoSystem::SetAlgebraLevel( UndefinedLevel );
  return (list);
}

bool
SecondoInterface::GetTypeId( const AlgebraLevel level,
                             const string& name,
                             int& algebraId, int& typeId )
{
  SecondoSystem::SetAlgebraLevel( level );
  bool ok = SecondoSystem::GetCatalog( level )->
              GetTypeId( name, algebraId, typeId );
  SecondoSystem::SetAlgebraLevel( UndefinedLevel );
  return (ok);
}

bool
SecondoInterface::LookUpTypeExpr( const AlgebraLevel level,
                                  ListExpr type, string& name,
                                  int& algebraId, int& typeId )
{
  bool ok = false;
  SecondoSystem::SetAlgebraLevel( level );
  name = "";
  algebraId = 0;
  typeId = 0;
  //cout << al->reportTableStates() << endl;
  //cout << "typeExpr: " << 
  al->ToString(type); // without this an assertion in CTable fails ????

  if ( SecondoSystem::GetInstance()->IsDatabaseOpen() )
  {
    // use application specific list memory
    ok = SecondoSystem::GetCatalog( level )->
           LookUpTypeExpr( al->CopyList(type,nl), name, algebraId, typeId );

  //cout << al->reportTableStates() << endl;

  }
  SecondoSystem::SetAlgebraLevel( UndefinedLevel );
  return (ok);
}

void
SecondoInterface::StartCommand()
{
  if ( !activeTransaction )
  {
    SecondoSystem::BeginTransaction();
  }
}

void
SecondoInterface::FinishCommand( int& errorCode )
{
  if ( !activeTransaction )
  {
    //cout << "!activeTransaction" << endl;
    if ( errorCode == 0 )
    {
      //cout << "CommitTransaction" << endl;
      if ( !SecondoSystem::CommitTransaction() )
      {
        errorCode = 23;
      }
    }
    else
    {
      if ( !SecondoSystem::AbortTransaction() )
      {
        errorCode = 23;
      }
    }
  }
  //cout << "Command Finished" << endl;
}



void
SecondoInterface::SetDebugLevel( const int level )
{
  SecondoSystem::GetQueryProcessor()->SetDebugLevel( level );
}

