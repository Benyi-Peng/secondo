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

1 Header File: Storage Management Interface (Berkeley DB)

January 2002 Ulrich Telle

1.1 Overview

The *Storage Management Interface* provides all types and classes needed
for dealing with persistent data objects in "Secondo"[3]. The interface
itself is completely independent of the implementation. To hide the
implementation from the user of the interface, the interface objects use
implementation objects for representing the implementation specific parts.

The Berkeley DB implementation of the interface uses several concepts to
keep track of the "Secondo"[3] databases and their ~SmiFiles~.

Since each Berkeley DB environment needs several control processes (deadlock
detection, logging, recovery) the decision was taken to use only one Berkeley
DB environment for managing an arbitrary number of "Secondo"[3] databases.

Named ~SmiFiles~ within each "Secondo"[3] database are managed in a simple
file catalog. 

1.1.1  "Secondo"[3] databases

In the root data directory resides a Berkeley DB file named ~databases~
which contains entries for each "Secondo"[3] database. All files of a
"Secondo"[3] database are stored in a subdirectory of the Berkeley DB
data directory. For each "Secondo"[3] database three Berkeley DB files
hold the information about the ~SmiFile~ objects:

  *  *sequences* -- provides unique identifiers for ~SmiFile~ objects.

  *  *filecatalog* -- keeps track of all *named* ~SmiFile~ objects.
The unique identifier is used as the primary key for the entries.

  *  *fileindex* -- represents a secondary index to the file catalog.
The name of a ~SmiFile~ object combined with the context name is used
as the primary key for the entries.

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

1.1.3 Berkeley DB handles

Each ~SmiFile~ object has an associated Berkeley DB handle. Unfortunately
the Berkeley DB requires that handles are kept open until after the
enclosing transaction completes. Since an ~SmiFile~ object may go out of
scope before, the destructor of the object must not close the handle. To
solve the problem the storage management environment provides a container
for Berkeley DB handles. The constructor of an ~SmiFile~ object allocates
a handle by means of the environment methods ~AllocateDbHandle~ and
~GetDbHandle~, the destructor returns the handle by means of the environment
method ~FreeDbHandle~. After completion of the enclosing transaction the
environment method ~CloseDbHandles~ closes and deallocates all Berkeley DB
handles no longer in use.

1.3 Implementation methods

The class ~SmiEnvironment::Implementation~ provides the following methods:

[23]    Database handling & Catalog handling  & File handling     \\
        [--------]
        LookUpDatabase    & LookUpCatalog     & ConstructFileName \\
        InsertDatabase    & InsertIntoCatalog & GetFileId         \\
        DeleteDatabase    & DeleteFromCatalog & EraseFiles        \\
                          & UpdateCatalog     & AllocateDbHandle  \\
                          &                   & GetDbHandle       \\
                          &                   & FreeDbHandle      \\
                          &                   & CloseDbHandles    \\

All other implementation classes provide only data members.

1.4 Imports, Constants, Types

*/

#ifndef SMI_BDB_H
#define SMI_BDB_H

#include <errno.h>
#include <queue>
#include <map>
#include <vector>

#include <db_cxx.h>

const u_int32_t CACHE_SIZE_STD = 128;
/*
Default cache size is 128 kB

*/
const u_int32_t CACHE_SIZE_MAX = 1024*1024;
/*
Maximum cache size is   1 GB

*/
/*
These are constants which define the default and maximum cache
size for the Berkeley DB environment.

*/

typedef size_t DbHandleIndex;
/*
is the type definition for indices of the Berkeley DB handle array.

*/

const DbHandleIndex DEFAULT_DBHANDLE_ALLOCATION_COUNT = 10;
/*
Space for Berkeley DB handles is reserved in chunks of ~DEFAULT\_\-DBHANDLE\_\-ALLO\-CATION\_\-COUNT~
elements to avoid frequent memory reallocation.

*/

const db_recno_t SMI_SEQUENCE_FILEID = 1;
/*
identifies the record number of the ~FileId~ sequence.

*/

struct SmiDbHandleEntry
{
  Db*           handle;
  bool          inUse;
  DbHandleIndex nextFree;
};
/*
defines the structure of the elements of the Berkeley DB handle array.
Handles which are not ~inUse~ anymore are closed and freed after the completion
of a transaction. The free element is put on a free list for later reuse.

*/

struct SmiCatalogEntry
{
  SmiFileId fileId;
  char      fileName[2*SMI_MAX_NAMELEN+2];
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
  static DbHandleIndex AllocateDbHandle();
/*
allocates a new Berkeley DB handle and returns the index within the handle array.

*/
  static Db*  GetDbHandle( DbHandleIndex idx );
/*
returns the Berkeley DB handle at the position ~idx~ of the handle array.

*/
  static void FreeDbHandle( DbHandleIndex idx );
/*
marks the Berkeley DB handle at position ~idx~ as *not in use*.

*/
  static void CloseDbHandles();
/*
closes all handles in the handle array which are not in use anymore.

*/
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
  static bool InsertIntoCatalog( const SmiCatalogEntry& entry,
                                 DbTxn* tid );
/*
inserts the catalog entry ~entry~ into the file catalog.

*/
  static bool DeleteFromCatalog( const string& fileName,
                                 DbTxn* tid );
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
  static string ConstructFileName( SmiFileId fileId );
/*
constructs a valid file name using the file identifier ~fileId~.

*/
  static bool LookUpDatabase( const string& dbname );
/*
looks up the Secondo database ~dbname~ in the database catalog.
The function returns ~true~ if a database with the given name exists.

*/
  static bool InsertDatabase( const string& dbname );
/*
inserts the name ~dbname~ of a new Secondo database into the database catalog.
The function returns ~true~ if the insert was successful.

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
  string  bdbHome;         // Home directory
  u_int32_t minutes;       // Time between checkpoints 
  DbEnv*  bdbEnv;          // Berkeley DB environment handle
  bool    envClosed;       // Flag if environment is closed
  DbTxn*  usrTxn;          // User transaction handle
  bool    txnStarted;      // User transaction started
  Db*     bdbDatabases;    // Database Catalog handle
  Db*     bdbSeq;          // Sequence handle
  Db*     bdbCatalog;      // Database File Catalog handle
  Db*     bdbCatalogIndex; // Database Catalog Index handle

  bool    listStarted;
  Dbc*    listCursor;
/*
are needed to support listing the names of all existing "Secondo"[3] databases.

*/

  queue<SmiDropFilesEntry>         bdbFilesToDrop;
  map<string,SmiCatalogFilesEntry> bdbFilesToCatalog;
  vector<SmiDbHandleEntry>         dbHandles;
  DbHandleIndex                    firstFreeDbHandle;
  
  friend class SmiEnvironment;
  friend class SmiFile;
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
  private:
    DbHandleIndex bdbHandle; // Index in handle array
    Db*           bdbFile;   // Berkeley DB handle
    bool          isSystemCatalogFile;
/*
flags an ~SmiFile~ as a system catalog file. This distinction is needed,
since transactional read operations on system catalog files could lead 
easily to deadlock situations by the way the transaction and locking
mechanism of the Berkeley DB works. Therefore read operation on system
catalog files should not be protected by transactions.

*/
  friend class SmiFile;
  friend class SmiFileIterator;
  friend class SmiRecordFile;
  friend class SmiKeyedFile;
  friend class SmiRecord;
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
  Dbc* bdbCursor;  // Berkeley DB cursor

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
  Db*  bdbFile;      // Berkeley DB handle
  Dbc* bdbCursor;    // Berkeley DB cursor
  bool useCursor;    // Flag use cursor in access methods
  bool closeCursor;  // Flag close cursor in destructor

  friend class SmiFile;
  friend class SmiFileIterator;
  friend class SmiRecordFile;
  friend class SmiKeyedFile;
  friend class SmiRecord;
};

#endif

