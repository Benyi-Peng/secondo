/*
//paragraph	[23]	table3columns:	[\begin{quote}\begin{tabular}{lll}]	[\end{tabular}\end{quote}]
//[--------]	[\hline]
//characters	[1]	verbatim:	[\verb|]	[|]
//[ae] [\"a]
//[oe] [\"o]
//[ue] [\"u]
//[ss] [{\ss}]
//[<=] [$\leq$]
//[#]  [\neq]
//[tilde] [\verb|~|]

1 Header File: PagedArray

August 2003 M. Spiekermann 

1.1 Overview

This module offers a generic persistent array implemented as a template class
on top of the SecondoSMI interface. Many slots of the array are stored inside
a fixed sized Berkeley-DB record which is a multiple of the operating systems
pagesize. All records IDs which contain array slots are hold in a vector in
main memory. The currently used record is bufferd in a memory array reducing
SMI calls. This ~cache~ or in other words the frame-buffer of the page
oriented memory organisation of the array is variable and can be defined by
setting the constant "MAX_PAGE_FRAMES"[1].

Restrictions: Currently this tool can not be used for saving and
reconstructing large arrays, but it is useful to use it temporary instead of
main memory. Therefore some code snipets are commented out and have to be
revised for usage as persistent array. 

Note: Since the record-size is an attribute of the record-file, this size is
defined at construction time of the file and hence a parameter for the
constructor of this class.  The maximium record-size is limited by the
operating systems page size

1.2 Interface methods

This module offers the following public methods:

[23]	Creation/Removal 	& Access   	& Inquiries	\\ 	
	[--------]
	PagedArray        	& Get 		& Size		\\  	
	[tilde]PagedArray	& Put		& Id		\\
	MarkDelete		&		& 		\\


1.3 Class ~PagedArray~

An instance of the class is a handle to a persistent array of fixed size with
elements of type ~T~.

*/

#ifndef PAGED_ARRAY_H
#define PAGED_ARRAY_H

#include <iostream> 
#include <iomanip>
#include <sstream> 
#include <fstream> 
#include <cassert>
#include <vector>
#include <map>
#include <typeinfo>

#include "SecondoSMI.h"
//#include "NestedList.h"

typedef unsigned long Cardinal;

class RecordBuffer
{

public:
  RecordBuffer(SmiRecordFile* bdbFile, int bufSize, int maxBuffers=4, bool traceOn=false) : 
    BUF_SIZE(bufSize),
    MAX_BUFFERS(maxBuffers),
    filePtr(bdbFile),
    maxPageNr(0),
    BufInfo(MAX_BUFFERS),
    bufferReplacements(0),
    trace(traceOn)
  {
    if (trace)
    cout << "BufSize: " << BUF_SIZE << endl;

    for (int i=0; i < MAX_BUFFERS; i++) { // initialize the buffer

       BufInfo[i].bufPtr = (void*) new char[BUF_SIZE];
       bufferMap[i] = i;
       recidVec.push_back( RecordInfo(0,i) );
       maxPageNr++;

       if (trace)
       cout << "buf: " << i << ": Adr " << (void*) BufInfo[i].bufPtr << endl;
    }
    
  }


  ~RecordBuffer() {

    for (int i=0; i < MAX_BUFFERS; i++) {
       delete [] (char*) BufInfo[i].bufPtr;
    }
  
  }

  void* GetBufPtr(const Cardinal& pageNr, bool &pageChange) {
   
     if (trace)
     cout << "pageNr: " << pageNr << endl; 
     
     int bufNr = -1;
     // check if page is currently inside the buffer
     if ( (pageNr <= maxPageNr) && (recidVec[pageNr].index >= 0) ) {
 
       bufNr = recidVec[pageNr].index;

     } else { // not in buffer
     
       // select buffer number to replace
       bufNr = RoundRobin();
       Replace(bufNr, pageNr);
       pageChange=true;

     }
     assert( bufNr >= 0 && bufNr < MAX_BUFFERS );
     return (void*) BufInfo[bufNr].bufPtr;
   }


private:

  int BUF_SIZE;
  int MAX_BUFFERS;

  SmiRecord record;
  SmiRecordId recid;
  SmiRecordFile *filePtr;

  struct RecordInfo {
  
    SmiRecordId id;
    int index;
    RecordInfo(SmiRecordId ID, int INDEX) : id(ID), index(INDEX) {}

  };
  vector<RecordInfo> recidVec;
  Cardinal maxPageNr;

  struct BufInfoRec {

    int pageNr;    
    SmiRecordId recId;  
    bool recExists;  
    void *bufPtr;   
    BufInfoRec() : pageNr(0), recId(0), recExists(false), bufPtr(0) {}  

  };
  vector<BufInfoRec> BufInfo;

  map<Cardinal, int> bufferMap;
  int bufferReplacements;

  bool trace;

  void Replace(const int bufNr, const Cardinal pageNr) { 

    assert( (pageNr <= (maxPageNr+1)) && (pageNr >= 0) );
    assert( (bufNr >= 0) && (bufNr < MAX_BUFFERS) );

    void* bufPtr = BufInfo[bufNr].bufPtr;
    // write bufNr to disk
    SmiRecord record;
    SmiRecordId recId; 
    if ( !BufInfo[bufNr].recExists ) { // a new record is needed

      record.Finish(); 
      assert( filePtr->AppendRecord( recId, record ) );

    } else {
      recId = BufInfo[bufNr].recId;
    }
    assert( filePtr->SelectRecord( recId, record, SmiFile::Update ) );

    record.Write( bufPtr, BUF_SIZE, 0);
    RecordInfo& recInfo = recidVec[ BufInfo[bufNr].pageNr ];
    recInfo.index = -1; // record is on disk
    recInfo.id = recId;
    
    // Read record into memory
    if ( pageNr > maxPageNr ) { // a new entry in the page table is needed 

      BufInfo[bufNr].recExists = false;
      recidVec.push_back( RecordInfo(0, bufNr) );
      maxPageNr++;

    } else { // entry is present in page table

      record.Finish();
      assert( filePtr->SelectRecord( recidVec[pageNr].id, record, SmiFile::Update ) );
      record.Read( bufPtr, BUF_SIZE, 0);
      BufInfo[bufNr].recExists = true;
      recidVec[pageNr].index = bufNr;
    }

    if (trace) {
      cout << "MaxPageNr: " << maxPageNr << ", pageNr: " << pageNr << ", index: " << bufNr << endl; 
    }
 
    bufferReplacements++;
  }

  int RoundRobin() {

    static int nextBuf=0;
    if ( nextBuf == MAX_BUFFERS-1 ) {
       nextBuf=0;
    } else {
       nextBuf++;
    }
    return nextBuf;
  }

};


extern unsigned int FileCtr;    // quick and dirty, sorry!

template<class T>
class PagedArray
{
 public:

  PagedArray( SmiRecordFile *parrays, bool logon=false, const int buffers=4 );

/*
Creates a new ~SmiRecord~ on the ~SmiRecordFile~ for this
persistent array. One can define an initial size of the persistent
array with the argument ~initsize~. 

*/
  
  //PagedArray( SmiRecordFile *parrays, const SmiRecordId id, 
  //            const bool update = true );

/*
Opens the ~SmiRecordFile~ and the ~SmiRecord~ for the persistent array. The
boolean value ~update~ indicates if open mode: ~true~ for update and ~false~
for read-only.

Note: Currently not implemented.

*/

  ~PagedArray();

/*
Destroys the handle. If the array is marked for deletion, then it also
destroys the persistent array.

*/

  void MarkDelete();

/*
Marks the persistent array for deletion. It will be permanently deleted on the
destruction of the object.

*Precondition:* The array must be opened in update mode.

*/

  void Put(Cardinal const index, T& elem);

/*
Copies element ~elem~ into the persistent array at index ~index~.

*Precondition:* 0 [<=] ~index~ [<=] ~size~ - 1. The array must be opened in
update mode.

*/

  void Get(Cardinal const index, T& elem);

/*
Returns the element ~index~ of the array.

*Precondition:* 0 [<=] ~index~ [<=] ~size~ - 1. 

*/

  Cardinal Size();

/*
Returns the size of this array.


1.3.1 Performance Analysis

*/

 unsigned long PageChanges() {  
     unsigned long swap = log.pageChangeCounter; 
     log.pageChangeCounter=0;    
     return swap; 
 }

  unsigned long SlotAccess() {  
     unsigned long swap = log.slotAccessCounter; 
     log.slotAccessCounter=0;    
     return swap; 
 }

/*
These two functions return useful characteristics of the ~cache~.  The first
represents the number of reading and writing a record on disk and the second
reflects the total number of the called ~Get~ operations.
   
*/ 
  
 private:

  // the next method loads another record if neccessary 
  // and calculates a slot number inside a record
  inline void GetSlot( Cardinal const index, int &slot );

  bool writeable;
  bool canDelete;
  Cardinal size;


  //Cardinal maxPageNo;
  
  struct PageRecordInfo { // define some important sizes derived from the record size
    
    int size;      // size of the record
    int slotSize;  // size of the objects stored in the array
    int slots;     // number of slots per record

    PageRecordInfo( int recSize ) : size(recSize), slotSize( sizeof(T) ) 
    {
      slots =  size / slotSize;
      assert ( (size > slotSize) && (slots > 0) );
    }

  };
  PageRecordInfo pageRecord;
  
  RecordBuffer recordBuf; // Record buffer and pointer to elements
  T* bufPtr;

/*
 The structure below groups all information used for
 creating trace files.
 
*/   
  
  struct  LogInfo {

    bool switchedOn;
    ofstream *filePtr;
    stringstream fileName;
    unsigned long pageChangeCounter; 
    unsigned long slotAccessCounter;

  };
  LogInfo log;

};


/*
2 Implementation of PagedArray

August 2003 M. Spiekermann 

2.1 Overview

This module offers a generic persistent array implemented as template class on
top of the SecondoSMI interface.

*/


template<class T>
PagedArray<T>::PagedArray( SmiRecordFile *parrays, bool logOn /*=false*/, const int buffers /*=4*/ ) :
writeable( true ),
canDelete( false ),
size( 0 ),
pageRecord( parrays->GetRecordLength() ),
recordBuf( parrays, pageRecord.size, buffers, true ),
bufPtr(0)
{
  log.switchedOn = logOn;
  log.pageChangeCounter = 0;
  log.slotAccessCounter = 0;


  size = buffers * pageRecord.slots;

  if ( log.switchedOn ) {

    FileCtr++;
    log.fileName << "PagedArray_" << typeid(T).name() << "_" << FileCtr << "_" << pageRecord.slots << ".log" << ends; 
    log.filePtr = new ofstream( log.fileName.str().c_str() );  
    (*log.filePtr) << "# slotsize:   " << pageRecord.slotSize << endl
                   << "# pagesize:   " << pageRecord.size << endl
                   << "#             " << endl
                   << "# index | changes in page buffer | page/index on page" << endl << endl;
  }

}


template<class T>
PagedArray<T>::~PagedArray()
{

  if ( log.switchedOn ) { // Write global Ctrs for page changes
    (*log.filePtr) << endl 
                   << "# Total page changes: " << log.pageChangeCounter << endl
                   << "# Total slot accesses: " << log.slotAccessCounter << endl;
    delete log.filePtr;
  }
}

/*
The function below calculates the page number which holds a specific array
index and determines if this page is still in the memory buffer or if one
of the pages in memory has to be substituted by a page on disk.

*/

template< class T>
void PagedArray<T>::GetSlot(Cardinal const index, int &slot )
{
  static Cardinal pageNo = 0;
  bool pageChange = false;
 

  // enlarge the array if necessary 
  if (index >= size) {
     size = size + pageRecord.slots;
  } 

  // calculate page number
  pageNo = index / pageRecord.slots;
  // cast the buffer pointer 
  bufPtr = (T*) recordBuf.GetBufPtr(pageNo, pageChange); 

  slot = index - (pageNo * pageRecord.slots);
  assert ( slot >= 0 && slot < pageRecord.slots);
  
  if ( log.switchedOn ) {
     log.slotAccessCounter++;
     if ( pageChange ) {
        *(log.filePtr) << " | " << pageNo << "/" << slot << endl; 
     }
  }

}



template<class T>
void PagedArray<T>::Put(Cardinal const index, T& elem)
{
  static int slot = 0;

  assert ( writeable );
  assert ( index <= size );

  GetSlot(index, slot);
  bufPtr[slot] = elem;
  if ( log.switchedOn ) {
    *(log.filePtr) << "w(" << index << "," << &(bufPtr[slot]) << ")" << endl; 
    string typeName(typeid(elem).name());
    if ( index == 1 && typeName.find("Node") ) {
      *(log.filePtr) << "v(1,nodetype:";
      for (unsigned int i=0; i<sizeof(T); i++ ) {
         char s = *(((char*) &bufPtr[slot]) + i);
         *(log.filePtr) << (255 & (unsigned int) s) << " ";
      }
      *(log.filePtr) << ")" << endl;
    }
  }
}


template<class T>
void PagedArray<T>::Get(Cardinal const index, T& elem)
{
  static int slot = 0;
  
  assert ( 0 <= index && index < size );
  
  GetSlot(index, slot); 
  // reinitialize type T at a given address
  elem = *( new(&(bufPtr[slot])) T );
  if ( log.switchedOn ) {
    
    *(log.filePtr) << "r(" << index << "," << &(bufPtr[slot]) << ")" << endl; 
    string typeName(typeid(elem).name());
    if ( index == 1 && typeName.find("Node") ) {
      *(log.filePtr) << "v(1,nodetype:";
      for (unsigned int i=0; i<sizeof(T); i++ ) {
         char s = *(((char*) &bufPtr[slot]) + i);
         *(log.filePtr) << (255 & (unsigned int) s) << " ";
      }
      *(log.filePtr) << ")" << endl;
    }
  }
}

template<class T>
void PagedArray<T>::MarkDelete() 
{
  assert( writeable );
  canDelete = true;
}

#endif
