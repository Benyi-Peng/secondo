/*
----
This file is part of SECONDO.

Copyright (C) 2004-2008, University in Hagen, Faculty of Mathematics and
Computer Science, Database Systems for New Applications.

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

[1] Definition of Auxiliary Classes in HadoopParallelAlgebra

April 2010 Jiamin Lu

[newpage]

1 Auxiliary Classes in HadoopParallelAlgebra

This file claims all relevant classes and methods that is
used in HadoopParallelAlgebra. Includes follow classes:

  * ~deLocalInfo~. Assists ~doubleexport~ operator.

  * ~phjLocalInfo~. Assists ~parahashjoin~ operator.

And includes one method:

  * ~renameList~. Assists ~parahashjoin~ operator

*/
#ifndef HADOOPPARALLELALGEBRA_H_
#define HADOOPPARALLELALGEBRA_H_

#define MAX_WAITING_TIME 10

#include "RTuple.h"
#include "FTextAlgebra.h"
#include "Base64.h"
#ifdef SECONDO_WIN32
#include <winsock2.h>
#endif

#include "FileSystem.h"
#include "Profiles.h"
#include "Progress.h"
#include "TemporalAlgebra.h"
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>


const int MAX_COPYTIMES = 50;
const size_t MAX_OPENFILE_NUM = 100;
const string dbLoc = "<READ DB/>";

class fList;

//Uses Function from ArrayAlgebra
namespace arrayalgebra{
  void extractIds(const ListExpr,int&,int&);
}


string tranStr(const string& s, const string& from, const string& to);
string getLocalFilePath(string path, const string fileName,
            string suffix, bool appendFileName = true);
string addFileIndex(string fileName, int index);
ListExpr AntiNumericType(ListExpr numericType);
int copyFile(string source, string dest, bool cfn = false);
Tuple* readTupleFromFile(ifstream* file, TupleType* type, int mode,
    string flobFile = "");
int getRoundRobinIndex(int row, int clusterSize);
ListExpr rmTermNL(ListExpr list, string term, int& count);
NList addIncomplete(const NList& attrList);


//Set up the remote copy command options uniformly.
const string scpCommand = "scp -q ";


/*
1.1 deLocalInfo Class

Assists ~doubleexport~ operator.
First traverse tuples from streamA to the end, then traverse streamB.

*/
class deLocalInfo
{
private:

  Word streamA;
  Word streamB;
  bool isAEnd;
  //Check out whether gets to the end of streamA.

  int attrIndexA;
  int attrIndexB;

  ListExpr tupTypeA;
  ListExpr tupTypeB;

  TupleType *resultTupleType;

  Tuple* makeTuple(Word stream, int index, int sig);

public:
  deLocalInfo(Word _streamA, Word wAttrIndexA,
              Word _streamB, Word wAttrIndexB,
              Supplier s);

  ~deLocalInfo()
  {
    //delete resultTupleType;
    if (resultTupleType != 0)
      resultTupleType->DeleteIfAllowed();
  }

  Tuple* nextResultTuple();
};

/*
1.2 renameList Method

Assists ~parahashjoin~ operator.
Append an ~appendName~ after each attribute name of ~oldTupleList~,
to avoid the situation that joined relations have a same attribute name.

*/
ListExpr renameList(ListExpr oldTupleList, string appendName);

/*
1.3 binEncode Method

Assist ~doubleexport~, ~add0Tuple~ operator.
Transform a nestedList into a transportable Base 64 binary string

*/
string binEncode(ListExpr nestList);

/*
1.4 binDecode Method

Assist ~parahashjoin~, ~parajoin~ opertor.
To decode a Base 64 binary string back to nestedList efficiently.

*/
ListExpr binDecode(string binStr);

/*
1.3 phjLocalInfo Class

Assists ~parahashjoin~ operator.
~getNewProducts~ collects tuples in each buckets,
and make products if the tuples come from different sources.
Put the product results in the TupleBuffer ~joinedTuples~.


*/
class phjLocalInfo
{
private:

  Word mixStream;
  ListExpr aTypeInfo;
  ListExpr bTypeInfo;
  TupleType *resultTupleType;
  TupleType *tupleTypeA, *tupleTypeB;

  TupleBuffer *joinedTuples;
  GenericRelationIterator *tupleIterator;

  GenericRelationIterator* getNewProducts( Supplier s);

public:
  phjLocalInfo(Word _stream, Supplier s,
               ListExpr ttA, ListExpr ttB);

  ~phjLocalInfo()
  {
    if (joinedTuples != 0)
      delete joinedTuples;
    joinedTuples = 0;

    if (resultTupleType != 0)
      resultTupleType->DeleteIfAllowed();
    resultTupleType = 0;
    if (tupleTypeA != 0)
      tupleTypeA->DeleteIfAllowed();
    tupleTypeA = 0;
    if (tupleTypeB != 0)
      tupleTypeB->DeleteIfAllowed();
    tupleTypeB = 0;

    if (tupleIterator != 0)
      delete tupleIterator;
    tupleIterator = 0;
  }

  Word nextJoinTuple( Supplier s );
};


/*
1.4 pjLocalInfo Class

Assists ~parajoin~ operator.

*/
typedef enum { tupBufferA, tupBufferB } tupleBufferType;

class pjLocalInfo
{
private:

  Word mixedStream;
  Supplier JNfun;
  ListExpr aTypeInfo;
  ListExpr bTypeInfo;
  TupleType *tupleTypeA, *tupleTypeB;

  TupleBuffer *tbA;
  GenericRelationIterator *itrA;
  TupleBuffer *tbB;
  GenericRelationIterator *itrB;
  const int maxMem;
  bool endOfStream;
  bool isBufferFilled;

  int bucketNum;
  int tupNum;


  //Get the tuples within one bucket,
  //and fill them into tupleBuffers
  void loadTuples();
public:
  pjLocalInfo(Word inputStream, Supplier fun, Supplier s,
      ListExpr ttA, ListExpr ttB,
      int mem) :
    mixedStream(inputStream),
    aTypeInfo(ttA),
    bTypeInfo(ttB),
    tbA(0),
    itrA(0),
    tbB(0),
    itrB(0),
    maxMem(mem),
    endOfStream(false),
    isBufferFilled(false)
  {
    //Indicator both arguments of the function accept stream
    JNfun = fun;
    qp->SetupStreamArg(JNfun, 1, s);
    qp->SetupStreamArg(JNfun, 2, s);
    qp->Open(JNfun);

    tupleTypeA = new TupleType(ttA);
    tupleTypeB = new TupleType(ttB);
  }

  ~pjLocalInfo()
  {
    if (tbA != 0)
      delete tbA;
    tbA = 0;
    if (tbB != 0)
      delete tbB;
    tbB = 0;

    if (tupleTypeA != 0)
      tupleTypeA->DeleteIfAllowed();
    tupleTypeA = 0;
    if (tupleTypeB != 0)
      tupleTypeB->DeleteIfAllowed();
    tupleTypeB = 0;
  }

  // Get the next tuple from tupleBuffer A or tuppleBuffer B.
  Tuple* getNextInputTuple(tupleBufferType tbt);

  // Get the next result tuple
  void* getNextTuple();
};

/*
1.5 a0tLocalInfo

Assists ~add0Tuple~ operator.

*/
struct a0tLocalInfo
{
  string key;
  TupleType *resultTupleType;
  Tuple *cachedTuple;
  bool needInsert;
  string sepTupleStr;

  inline a0tLocalInfo(ListExpr rtNL):
      key(""), cachedTuple(0),
      needInsert(false)
  {
    resultTupleType = new TupleType(nl->Second(rtNL));
  }

  inline ~a0tLocalInfo()
  {
    if (resultTupleType != 0)
      resultTupleType->DeleteIfAllowed();
    resultTupleType = 0;

    if ( cachedTuple != 0)
      cachedTuple->DeleteIfAllowed();
    cachedTuple = 0;

  }
};

/*
1.6 clusterInfo

We define two list files in all nodes involved in a parallel Secondo.
These files list all nodes' ip addresses, cell file locations,
and Secondo Monitor's access ports.

These two files are separated to master and slaves list.
The master list only have one line, and the slave list have lines of
as same as the disks of the slaves.
These two files are specified in nodes, by setting their
PARALLEL\_SECONDO\_MASTER and PARALLEL\_SECONDO\_SLAVES
environment variables.

This class is used to verify these two files.

*/
class clusterInfo
{
public:
  clusterInfo();
  clusterInfo(clusterInfo& rhg);

  ~clusterInfo(){
      if(dataServers){
         delete dataServers;
      }
   }

  string getRemotePath(size_t loc,
      bool includeMaster = true,
      bool round = false,
      bool appendTargetIP = true,
      bool appendFileName = false,
      string fileName = "",
      bool attachProducerIP = false,
      string producerIP = "");

  string getMSECPath(size_t loc,
      bool includeMaster = true, bool round = false,
      bool appendIP = true);
/*
Get the remote mini-Secondo path

*/

  string getIP(size_t loc, bool round = false);

  inline int getLocalNode(){
    if (localNode < 0)
      localNode = searchLocalNode();
    return localNode;
  }

  inline int getMasterNode(){
    return masterNode;
  }

  inline bool isLocalTheMaster(){
    return (getLocalNode() == masterNode);
  }

  string getLocalIP();

/*
~getLocalPath~ function can only return the constant value
SecondoFilePath that is defined in SECONDO\_CONFIG file.
If the file location defined in PARALLEL\_SECONDO\_SLAVES
is different from the default value,
then the ~searchLocalNode~ function cannot return a correct result.

*/
  inline string getLocalPath()
  {

    string confPath = string(getenv("SECONDO_CONFIG"));
    string result = SmiProfile::GetParameter("ParallelSecondo",
        "SecondoFilePath","", confPath);
    if (result.find_last_of("/") == result.length() - 1)
      result = result.substr(0, result.length() - 1);

    return result;
  }

  inline bool isOK(){  return available; }
/*
Number of slaves

*/
  inline size_t getSlaveSize(){
    if (dataServers){
        return (dataServers->size() - 1);
    }
    else
      return 0;
  }

/*
Number of all DSs. If the mDS is also a sDS, then it is counted repeatedly.

*/
  inline size_t getClusterSize(){
    if (dataServers){
      return dataServers->size();
    }
    else
      return 0;
  }

  void print();

  NList toNestedList();
  bool covers(NList& clusterList);
  size_t getInterIndex(size_t loc,bool includeMaster,bool round);

private:
  string ps_master;
  string ps_slaves;
  typedef pair<string, pair<string, int> > dservDesc; //data server description
  vector<dservDesc> *dataServers;
  bool available;
  int localNode;
  int masterNode;

  int searchLocalNode();
  vector<string>* getAvailableIPAddrs();
};

/*
1.4 FFeedLocalInfo Class

Assists ~ffeed~ operator.
Support progress estimation.

*/
class FFeedLocalInfo: public ProgressLocalInfo
{
public:
  FFeedLocalInfo( Supplier s, bool _nf, int _prd, string _fp)
  : tupleBlockFile(0), fileFound(false), noFlob(_nf),
    prdIndex(_prd), filePath(_fp)
  {

    if (noFlob)
    {
/*
Prepared for the ~ffeed3~ operator.
The type in data file contains no DS\_IDX, while the type for output needs DS\_IDX

*/
      string ostStr = ((FText*)qp->Request(
          qp->GetSupplierSon(s, 4)).addr)->GetValue();
      ListExpr rcdTypeList;
      nl->ReadFromString(ostStr, rcdTypeList);
      rcdTupleType = new TupleType(SecondoSystem::GetCatalog()
                      ->NumericType(nl->Second(rcdTypeList)));

      ListExpr newTypeList = qp->GetType(s);
      newTupleType = new TupleType(SecondoSystem::GetCatalog()
                      ->NumericType(nl->Second(newTypeList)));
    }
    else
    {
      ListExpr streamTypeList = qp->GetType(s);
      rcdTupleType = new TupleType(SecondoSystem::GetCatalog()
                      ->NumericType(nl->Second(streamTypeList)));
      newTupleType = rcdTupleType;
    }
  }

  ~FFeedLocalInfo() {
    if (tupleBlockFile){
      tupleBlockFile->close();
      delete tupleBlockFile;
      tupleBlockFile = 0;
    }
    if (rcdTupleType){
      rcdTupleType->DeleteIfAllowed();
    }
    if (noFlob && newTupleType){
      newTupleType->DeleteIfAllowed();
    }
  }

  bool fetchBlockFile(
      string relName, string fileSuffix, Supplier s,
      int pdi = -1, int tgi = -1, int att = -1);

  Tuple* getNextTuple();  //~ffeed~
  Tuple* getNextTuple2(); //~ffeed2~
  Tuple* getNextTuple3(); //~ffeed3~

  ifstream *tupleBlockFile;
  TupleType* rcdTupleType;  //The tuple type in the data file
  TupleType* newTupleType;  //The output tuple type

/*
In ~ffeed~, the rcdTupleType and the newTupleType are the same.
In ~ffeed2~, they are different, the newTupleType has
an additional integer attribute DS\_IDX.

*/

private:
  bool isLocalFileExist(string filePath);
  bool fileFound;
  bool noFlob;
  int  prdIndex;  //The index of the produce DS
  string filePath;
};

/*
1.5 structure ~fconsumeLocalInfo~

Assist ~fconsume~ operator.

*/
struct fconsumeLocalInfo
{
  int state;
  int current;
};

/*
1.6 hdpJoinLocalInfo class

*/

class hdpJoinLocalInfo
{
private:
  vector<pair<int, int> > resultList;
  vector<pair<int, int> >::iterator it;
  TupleType *resultTupleType;

public:
  hdpJoinLocalInfo(Supplier s){

    ListExpr resultTTList = GetTupleResultType(s);
    resultTupleType = new TupleType(nl->Second(resultTTList));
  }

  void insertPair(pair<int, int> elem){
    resultList.push_back(elem);
  }

  void setIterator(){
    it = resultList.begin();
  }

  Tuple* getTuple()
  {
    if (it == resultList.end())
      return 0;
    else {
      Tuple *t = new Tuple(resultTupleType);
      t->PutAttribute(0, new CcInt((*it).first));
      t->PutAttribute(1, new CcInt((*it).second));

      it++;
      return t;
    }
  }
};


/*
1.7 fDistributeLocalInfo class

*/
class fileInfo{
public:
  fileInfo(size_t _cs, string _fp, string _fn,
      size_t _an, int _rs, bool _nf = false, SmiRecordId _sid = -1);

  bool openFile();

  void closeFile();

  bool writeTuple(Tuple* tuple, size_t tupleIndex, TupleType* exTupleType,
       int ai, bool kai, int aj = -1, bool kaj = false);

  int writeLastDscr();

  ~fileInfo()
  {
    if (fileOpen){
      blockFile.close();
      fileOpen = false;
    }
    attrExtSize->clear();
    delete attrExtSize;
    attrSize->clear();
    delete attrSize;
  }

  inline string getFileName() {
    return blockFileName;
  }

  inline string getFilePath() {
    return blockFilePath;
  }
  inline bool isFileOpen()
  {  return fileOpen;  }
  inline size_t getLastTupleIndex()
  {  return lastTupleIndex;  }

private:

  int cnt;
  double totalExtSize;
  double totalSize;
  vector<double>* attrExtSize;
  vector<double>* attrSize;

  string blockFilePath, blockFileName;
  SmiFileId flobFileId;
  SmiRecordId sourceDS;
  string flobFilePath, flobFileName;
  ofstream blockFile, flobFile;
  SmiSize flobBlockOffset;

  size_t lastTupleIndex;
  bool fileOpen, noFlob;
};

bool static compFileInfo(fileInfo* fi1, fileInfo* fi2)
{
  return fi1->getLastTupleIndex() > fi2->getLastTupleIndex();
}
class FDistributeLocalInfo
{

private:
  size_t HashTuple(Tuple* tuple)
  {
    size_t hashValue =
        ((Attribute*)tuple->GetAttribute(attrIndex))->HashValue();
    if (nBuckets != 0)
      return hashValue % nBuckets;
    else
      return hashValue;
  }
  bool openFile(fileInfo* tgtFile);

  size_t nBuckets;
  int attrIndex;
  bool kpa;
  TupleType *resultTupleType, *exportTupleType;
  size_t tupleCounter;

  string fileBaseName;
  int rowNumSuffix;
  string filePath;
  map<size_t, fileInfo*> fileList;
  map<size_t, fileInfo*>::iterator fit;

  //data remote variables
  int firstDupTarget, dupTimes, localIndex;
  string cnIP;  //current node IP
  clusterInfo *ci;
  bool* copyList;
  bool noFlob;
  SmiRecordId sourceDS;
  bool ok;

  //~openFileList~ keeps at most MAX_FILEHANDLENUM file handles.
  vector<fileInfo*> openFileList;
  bool duplicateOneFile(fileInfo* fi);

public:
  FDistributeLocalInfo(string baseName, int rowNum,
                       string path,
                       int nBuckets, int attrIndex,
                       bool keepKeyAttribute,
                       ListExpr resultTupleType,
                       ListExpr inputTupleType,
                       int dtIndex, int dupTimes, bool noFlob);

  bool insertTuple(Word tuple);
  bool startCloseFiles();
  Tuple* closeOneFile();

  ~FDistributeLocalInfo(){
    map<size_t, fileInfo*>::iterator fit;
    for(fit = fileList.begin(); fit != fileList.end(); fit++)
    { delete (*fit).second; }

    openFileList.clear();
    fileList.clear();
    if (resultTupleType)
      resultTupleType->DeleteIfAllowed();
    if (exportTupleType)
      exportTupleType->DeleteIfAllowed();
  }

  bool isOK(){ return ok;}
};

/*
1.8 FetchFlobLocalInfo class

*/

class FlobSheet;

class FetchFlobLocalInfo : public ProgressLocalInfo
{
public:
  FetchFlobLocalInfo(const Supplier s, NList resultTypeList,
      NList raList, NList daList);

  ~FetchFlobLocalInfo(){
    if (resultType)
      resultType->DeleteIfAllowed();

    pthread_mutex_destroy(&FFLI_mutex);

    if (ci){
      delete ci;
      delete standby;
      delete prepared;
      delete []sheetCounter;
    }
  }

  Tuple* getNextTuple(const Supplier s);

  void fetching2prepared(FlobSheet* fs);
/*
When one Flob sheet is finished, copy the file back
and set it into the preparedSheets.

*/

  void clearFetchedFiles();

  inline int getLocalDS(){
    return ci->getLocalNode();
  }

  inline string getMSecPath(int dest, bool appendIP = true){
    return ci->getMSECPath(dest, true, false, appendIP);
  }

  inline string getPSFSPath(int dest, bool appendIP = true){
    return ci->getRemotePath(dest, true, false, appendIP);
  }

  inline string getIP(int dest) {
    return ci->getIP(dest);
  }

private:
  Tuple* setResultTuple(Tuple* tuple);
  Tuple* setLocalFlob(Tuple* tuple);

  Tuple* readLocalFlob(Tuple* tuple);
  int getSheetKey(const vector<int>& sdss);
  int getMaxSheetKey();

  string LFPath;         //local flob file path
  //Use NL for flob attributes to transfer the list to flobSheet object
  NList faList;
  NList daList;          //deleted attribute list
  TupleType* resultType;


  clusterInfo *ci;
  int cds;               //index of the current DS
  bool moreInput;

/*
Three sheet lists are used to indicate the their status:

  * standby: unsent sheets, collecting input tuples and their FlobOrder.
Each DS has one slot here.

  * fetching: preparing the sheet by copying the flob file within a thread.

  * prepared: the flob file is prepared. It has unlimited size.


Instead of using the fetching vector, I simply use a fetchingNum to find how many
sheets are being fetched.
During the fetching procedure, the sheet is kept inside the thread object.
Since using the vector makes the same sheet to be kept twice:
in the thread and the vector.

*/

  vector<FlobSheet*>* standby;
  int fetchingNum;
  vector<FlobSheet*>* prepared;
  int *sheetCounter;    //Count the sent sheet number for each DS
  vector<string>* fetchedFiles;

  bool sendSheet(FlobSheet* fs, int times);

  int maxSheetMem;
  //thread tokens
  static const int PipeWidth = 10;
  bool tokenPass[PipeWidth];
  pthread_t threadID[PipeWidth];
  static void* sendSheetThread(void* ptr);
  static pthread_mutex_t FFLI_mutex;

};

/*
Thread for fetching a remote file


*/
class FFLI_Thread
{
public:
  FFLI_Thread(FetchFlobLocalInfo* pt, FlobSheet* _fs, int _t, int _tk):
    ffli(pt), sheet(_fs), times(_t), token(_tk){}

  FetchFlobLocalInfo* ffli;
  FlobSheet* sheet;
  int times;
  int token;
};


/*
Flob orders prepared for one tuple,
which may contain Flobs coming from several Data Servers.

*/
class FlobSheet
{
public:

  FlobSheet(const vector<int>& _sources, const int _si,
      const NList& _faList, int maxMemory)
    : sheetIndex(_si), cachedSize(0), maxMem(maxMemory), faList(_faList){

    flobFiles = new map<int, pair<string, size_t> >();
    buffer = new TupleBuffer(maxMem);
    it = 0;
    rtCounter = 0;

    int faCounter = 0;
    NList rest = faList;
    while (!rest.isEmpty()){
      int ai = rest.first().intval();
      int source = _sources[faCounter];
      flobFiles->insert(make_pair(ai, make_pair("", 0)));
      sourceDSs.push_back(source);
      rest.rest();
      faCounter++;
    }
  }

  ~FlobSheet(){
    delete flobFiles;
    delete buffer;
  }

  bool addOrder(Tuple* tuple);
/*
Put one tuple into the sheet, if the buffer limit is not reached.

*/

  Tuple* getCachedTuple();

  const vector<int>& getSDSs() { return sourceDSs;}

  inline void makeScan(){
    it = buffer->MakeScan();
  }

  inline string getResultFilePath(int sds, int attrId){
    return flobFiles->find(attrId)->second.first;
  }

  string setSheet(int source, int dest, int times, int attrId);

  string setResultFile(int source, int dest, int times, int attrId);

  ostream& print(ostream& os) const {

    os << "Sheet index is: " << sheetIndex << endl;
    os << "source DSs: ";
    for (vector<int>::const_iterator it = sourceDSs.begin();
        it != sourceDSs.end(); it++){
      os << (*it) << " ";
    }
    os << endl;

    os << "flobFiles: ------------- " << endl;
    for (map<int, pair<string, size_t> >::iterator it = flobFiles->begin();
        it != flobFiles->end(); it++ ){
      os << "attrId(" << it->first << ") : " << it->second.first
          << " offset(" << it->second.second << ")" << endl;
    }
    os << "-----------------------------" << endl;

    os << "maxsize (" << maxMem
        << ") cachedSize (" << cachedSize << ")" << endl;
    os << "Cached tuples : " << buffer->GetNoTuples() << endl;
    os << "Read tuples : " << rtCounter << endl;
    os << endl << endl;

    return os;
  }


private:
  // Map with : (attrId (flobFilePath, flobOffset))
  //Collect one Flob for each attribute, hence use the attribute id as the key
  map<int, pair<string, size_t> >* flobFiles;

  vector<int> sourceDSs;
  int sheetIndex;
  int cachedSize;
/*
If it is larger than a certain threshold, stop adding other order,
then fetch the needed flob file.

*/
  int maxMem;

/*
newRecIds is used to record the id of all new Flobs created within this sheet.
It happens that a Flob is listed in the current sheet,
but has already been created in another sheet.

*/
  set<SmiRecordId> newRecIds;

  TupleBuffer* buffer;
  GenericRelationIterator* it;
  size_t rtCounter;

  NList faList;  //Flob Attribute list
};

ostream& operator<<(ostream& os, const FlobSheet& f);

#endif /* HADOOPPARALLELALGEBRA_H_ */
