/*
//paragraph	[10]	title:		[{\Large \bf ] [}]
//paragraph	[21]	table1column:	[\begin{quote}\begin{tabular}{l}]	[\end{tabular}\end{quote}]
//paragraph	[22]	table2columns:	[\begin{quote}\begin{tabular}{ll}]	[\end{tabular}\end{quote}]
//paragraph	[23]	table3columns:	[\begin{quote}\begin{tabular}{lll}]	[\end{tabular}\end{quote}]
//paragraph	[24]	table4columns:	[\begin{quote}\begin{tabular}{llll}]	[\end{tabular}\end{quote}]
//[--------]	[\hline]
//characters	[1]	verbatim:	[$]	[$]
//characters	[2]	formula:	[$]	[$]
//characters	[3]	capital:	[\textsc{]	[}]
//[ae] [\"a]
//[oe] [\"o]
//[ue] [\"u]
//[ss] [{\ss}]
//[<=] [\leq]
//[#]  [\neq]
//[tilde] [\verb|~|]

1 Header File: Storage Management Interface (Oracle DB)

April 2002 Ulrich Telle

1.1 Overview

The *Storage Management Interface* provides all types and classes needed
for dealing with persistent data objects in "Secondo"[3]. The interface
itself is completely independent of the implementation. To hide the
implementation from the user of the interface, the interface objects use
implementation objects for representing the implementation specific parts.

The Oracle implementation of the interface uses several concepts to
keep track of the "Secondo"[3] databases and their ~SmiFiles~.
Named ~SmiFiles~ within each "Secondo"[3] database are managed in a simple
file catalog. 

1.1.1  "Secondo"[3] databases

All "Secondo"[3] databases belong to one Oracle user, the "Secondo"[3]
manager. The connect information for this user is kept in the "Secondo"[3]
configuration file. Since the password is stored therein in plain text
the configuration file should be protected appropriately. The names of
the Oracle database tables storing ~SmiFiles~ always have the name of
the "Secondo"[3] database name as a prefix. For each "Secondo"[3] database
two Oracle objects hold the information about the ~SmiFile~ objects:

  *  *SEQUENCES* -- is an Oracle sequence providing unique identifiers
for ~SmiFile~ objects within a Secondo database.

  *  *TABLES* -- is an Oracle table used as a ~file catalog~ keeping track
of all *named* ~SmiFile~ objects. The unique identifier is used as the
primary key for the entries. The name of a ~SmiFile~ object combined with
the context name is used as a secondary unique key for the entries.

1.1.2 File catalog

Each ~SmiFile~ object - whether named or anonymous - gets a unique
identifier when created. Since in a transactional environment objects
get visible to other users only when the enclosing transaction completes
successfully, the information about named ~SmiFile~ objects is queued
for processing after completion of the transaction. Requests for
deleting ~SmiFile~ objects are queued in a similar way.

File catalog updates and file deletions take place at the time a
transaction completes. The actions taken depend on whether the transaction
is committed or aborted. In case of a commit catalog updates and file
deletions are performed as requested by the user, in case of an abort
only those files which where created during the transaction are deleted,
not catalog update is performed.

1.3 Implementation methods

The class ~SmiEnvironment::Implementation~ provides the following methods:

[23]    Database handling & Catalog handling  & File handling     \\
        [--------]
        LookUpDatabase    & LookUpCatalog     & ConstructTableName \\
        DeleteDatabase    & InsertIntoCatalog & ConstructSeqName   \\
                          & DeleteFromCatalog & ConstructIndexName \\
                          & UpdateCatalog     & GetFileId  \\
                          &                   & EraseFiles       \\

The class ~SmiFile::Implementation~ provides the following methods:

[21]    Key handling    \\
        [--------]
        BindKeyToCursor \\
        GetSeqId        \\

All other implementation classes provide only data members.

1.4 Imports, Constants, Types

*/

#ifndef SMI_ORA_H
#define SMI_ORA_H

#include <errno.h>
#include <queue>
#include <map>
#include <vector>

#include <ocicpp.h>

struct SmiCatalogEntry
{
  SmiFileId fileId;
  string    fileName;
  bool      isKeyed;
  bool      isFixed;
};
/*
defines the structure of the entries in the file catalog.
The identifier ~fileId~, the name ~fileName~ and the type is stored for each
named ~SmiFile~.

*/

struct SmiDropFilesEntry
{
  SmiFileId fileId;
  bool      dropOnCommit;   
};
/*
defines the structure of the elements in the queue for file drop requests.
Drop requests are fulfilled on successful completion of a transaction if
the flag ~dropOnCommit~ is set or on abortion of a transaction if this
flag is *not* set. In all other cases an entry is ignored.

*/

struct SmiCatalogFilesEntry
{
  SmiCatalogEntry entry;
  bool            updateOnCommit;
};
/*
defines the structure of the elements in the map for file catalog requests.
Catalog requests are fulfilled on successful completion of a transaction if
the flag ~updateOnCommit~ is set or on abortion of a transaction if this
flag is *not* set. In all other cases an entry is ignored.

*/

/**************************************************************************
1.3 Class "SmiEnvironment::Implementation"[1]

This class handles all implementation specific aspects of the storage environment
hiding the implementation from the user of the ~SmiEnvironment~ class.

*/

class SmiEnvironment::Implementation
{
 public:
  static SmiFileId GetFileId();
/*
returns a unique file identifier.

*/
  static bool LookUpCatalog( const string& fileName,
                             SmiCatalogEntry& entry );
/*
looks up a file named ~fileName~ in the file catalog. If the file was found, the
function returns ~true~ and the catalog entry ~entry~ contains information about
the file like the file identifier.

*/
  static bool LookUpCatalog( const SmiFileId fileId,
                             SmiCatalogEntry& entry );
/*
looks up a file identified by ~fileId~ in the file catalog. If the file was found,
the function returns ~true~ and the catalog entry ~entry~ contains information about
the file like the file name.

*/
  static bool InsertIntoCatalog( const SmiCatalogEntry& entry );
/*
inserts the catalog entry ~entry~ into the file catalog.

*/
  static bool DeleteFromCatalog( const string& fileName );
/*
deletes the catalog entry ~entry~ from the file catalog.

*/
  static bool UpdateCatalog( bool onCommit );
/*
updates the file catalog on completion of a transaction by inserting or deleting
entries collected during the transaction. The flag ~onCommit~ tells the function
whether the transaction is committed (~true~) or aborted (~false~).

*/
  static bool EraseFiles( bool onCommit );
/*
erases all files on completion of a transaction for which drop requests were
collected during the transaction. The flag ~onCommit~ tells the function
whether the transaction is committed (~true~) or aborted (~false~).

*/
  static string ConstructTableName( SmiFileId fileId );
  static string ConstructSeqName( SmiFileId fileId );
  static string ConstructIndexName( SmiFileId fileId );
/*
construct a valid table, sequence or index name using the file identifier ~fileId~.

*/
  static bool LookUpDatabase( const string& dbname );
/*
looks up the Secondo database ~dbname~ in the database catalog.
The function returns ~true~ if a database with the given name exists.

*/
  static bool DeleteDatabase( const string& dbname );
/*
deletes the name ~dbname~ of an existing Secondo database from the database
catalog. The function returns ~true~ if the deletion was successful.

*/
 protected:
  Implementation();
  ~Implementation();
 private:
  OCICPP::Connection usrConnection;
/*
is the connection for user commands. All data manipulation commands are
executed using this connection.

*/
  OCICPP::Connection sysConnection;
/*
is the connection for system commands. All data definition commands are
executed using this connection since Oracle issues implicitly a ~commit
transaction~ immediately before and after data definition commands.

*/
  ostream* msgStream;
/*
is the output stream for error messages.

*/
  bool     txnStarted;
/*
User transaction started

*/
  bool           listStarted;
  OCICPP::Cursor listCursor;
/*
are needed to support listing the names of all existing "Secondo"[3] databases.

*/

  queue<SmiDropFilesEntry>         oraFilesToDrop;
  map<string,SmiCatalogFilesEntry> oraFilesToCatalog;
  
  friend class SmiEnvironment;
  friend class SmiFile;
  friend class SmiFileIterator;
  friend class SmiRecordFile;
  friend class SmiKeyedFile;
  friend class SmiRecord;
};

/**************************************************************************
1.3 Class "SmiFile::Implementation"[1]

This class handles all implementation specific aspects of an SmiFile
hiding the implementation from the user of the ~SmiFile~ class.

*/

class SmiFile::Implementation
{
 public:
 protected:
  Implementation();
  ~Implementation();
  static bool BindKeyToCursor(
                const SmiKey::KeyDataType keyType,
                const void* keyAddr,
                const int keyLength,
                const string& bindname,
                OCICPP::Cursor& csr );
/*
binds the key values according to the key datatype to placeholders
in an SQL command for a given database cursor.

*/

  int GetSeqId( OCICPP::Connection& con );
/*
returns a unique record sequence number for ~SmiFiles~ with duplicate keys.
Since the current version of the OCI C++ library does not support the
~RETURNING~ clause, explicit sequence numbers are the only way to identify
newly inserted records with duplicate keys. Otherwise it would be possible
to return the internal row identification of Oracle for this purpose.

*/
 private:
  string oraTableName;  // complete table name
  string oraSeqName;    // complete sequence name
  string oraIndexName;  // complete index name

  friend class SmiFile;
  friend class SmiFileIterator;
  friend class SmiRecordFile;
  friend class SmiKeyedFile;
};

/**************************************************************************
1.3 Class "SmiFileIterator::Implementation"[1]

This class handles all implementation specific aspects of an SmiFileIterator
hiding the implementation from the user of the ~SmiFileIterator~ class.

*/

class SmiFileIterator::Implementation
{
 public:
 protected:
  Implementation();
  ~Implementation();
 private:
  OCICPP::Cursor oraCursor;  // Oracle DB cursor

  friend class SmiFile;
  friend class SmiFileIterator;
  friend class SmiRecordFile;
  friend class SmiRecordFileIterator;
  friend class SmiKeyedFile;
  friend class SmiKeyedFileIterator;
};

/**************************************************************************
1.3 Class "SmiRecord::Implementation"[1]

This class handles all implementation specific aspects of an SmiRecord
hiding the implementation from the user of the ~SmiRecord~ class.

*/

class SmiRecord::Implementation
{
 public:
 protected:
  Implementation();
  ~Implementation();
 private:
  OCICPP::Lob     oraLob;      // Oracle Large Object
  OCICPP::Cursor  oraCursor;   // Oracle cursor
  bool            closeCursor; // Request to close the cursor
                               //   in the destructor

  friend class SmiFile;
  friend class SmiFileIterator;
  friend class SmiRecordFile;
  friend class SmiKeyedFile;
  friend class SmiRecord;
};

#endif

