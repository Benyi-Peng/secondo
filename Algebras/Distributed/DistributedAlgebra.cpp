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
*/

/*
[1] DistributedAlgebra

November 2010 Tobias Timmerscheidt

This algebra implements a distributed array. This type of array
keeps its element on remote servers, called worker. Upon creation
of the array all elements are transfered to the respective workers.
The list of workers must be specified in terms of a relation in any
operator that gives back a darray.
Operations on the darray-elements are carried out on the remote machines.



1. Preliminaries

1.1 Includes

*/

#include "Algebra.h"
#include "NestedList.h"
#include "NList.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include "SecondoCatalog.h"
#include "RelationAlgebra.h"
#include "TypeMapUtils.h"
#include "Remote.h"
#include "zthread/ThreadedExecutor.h"
#include "../FText/FTextAlgebra.h"
#include "../Array/ArrayAlgebra.h"
#include "DistributedAlgebra.h"
#include "StringUtils.h"
#include "Symbols.h"


using namespace std;
using namespace mappings;

extern NestedList* nl;
extern QueryProcessor *qp;

/*
2.0 Class LogOutput

2.1 Implementation

*/

LogOutput*
LogOutput::getInstance()
{
  static LogOutput myLogOutput;
  return &myLogOutput;
}


LogOutput::LogOutput()
{
  m_out = new std::ofstream("/home/achmann/secondo/bin/log.txt",
                      std::ios::out | std::ios::trunc);
}

LogOutput::~LogOutput()
{
  m_out -> close();
  delete m_out;
}

void
LogOutput::print(const std::string &msg)
{
  *m_out << msg << endl;
}

void
LogOutput::print(int val)
{
  *m_out << val << endl;
}
/*

3.0 Auxiliary Functions

*/

//Uses Function from ArrayAlgebra
void extractIds(const ListExpr,int&,int&);



//Converts int to string
string toString_d(int i)
{
   std::string s;
   std::stringstream out;
   out << i;
   s = out.str();
   return s;
}


//Creates an unique identifier for a new distributed array
string getArrayName(int number)
{
   string t = toString_d(time(0)) + toString_d(number);
   return t;
}

//Converts a pair (algID typID) to the corresponding
//type name (used by converType)
ListExpr convertSingleType( ListExpr type)
{
   if(nl->ListLength(type) != 2) return nl->SymbolAtom("ERROR");
      if(!nl->IsAtom(nl->First(type)) || !nl->IsAtom(nl->Second(type)))
         return nl->SymbolAtom("ERROR");

   int algID, typID;

   extractIds(type,algID,typID);

   SecondoCatalog* sc = SecondoSystem::GetCatalog();
   if(algID < 0 || typID < 0) return nl->SymbolAtom("ERROR");

   return nl->SymbolAtom(sc->GetTypeName(algID,typID));
}


//Converts a numerical type to its text representation
ListExpr convertType( ListExpr type )
{
   ListExpr result,result2;

     //Is it not a type expression but an attribute name?
   if(nl->ListLength(type) < 2)
   {
      if(nl->IsAtom(type) || nl->IsEmpty(type)) return type;
      //Only one element that is not atomic
      else return convertType(nl->First(type));
   }

     //Single type expression
   if(nl->ListLength(type) == 2 &&
         nl->IsAtom(nl->First(type)) &&
         nl->IsAtom(nl->Second(type)))

         return convertSingleType(type);

   //It's a list with more than three elements, proceed recursively
   result = convertType(nl->First(type));
   result2 = convertType(nl->Rest(type));

   if(nl->ListLength(type) == 2)
   return nl->TwoElemList(result,result2);
   else
      return nl->Cons(result,result2);
}


/*


4. Type Constructor ~DArray~

4.1 Data Structure - Class ~DArray~

*/

// moved to file DistributedAlgebra.h

/*

4.2 Implementation of basic functions

*/

//Definition of static variable
int DArray::no = 0;

//Creates an undefined DArray
DArray::DArray()
{
   defined = false;
   isRelation = false;
   size=0;
   alg_id=0;
   typ_id=0;
   name="";
   no++;
}

//Creates a defined DArray
//A DArray is defined by its name, type, size and serverlist
//It can be defined while all its elements are undefined
DArray::DArray(ListExpr n_type, string n, int s, ListExpr n_serverlist)
{
   defined = true;

   type = n_type;



   extractIds( type, alg_id, typ_id);

   SecondoCatalog* sc = SecondoSystem::GetCatalog();
   if(sc->GetTypeName(alg_id,typ_id) == Relation::BasicType())
      isRelation = true;
   else
      isRelation = false;


   name = n;

   size = s;

   m_elements = vector<Word>(size);
   m_present = vector<int>(size, 0);

   serverlist = n_serverlist;
   manager = new DServerManager(serverlist, name,type,size);

   no++;

}



DArray::~DArray()
{
   no--;
   if(defined)
   {

      for(int i=0;i<size;i++)
      {
         //Elements that are present on the master are deleted
         //note that this deletes NOT the Secondo-objects on the workers
         //they need to be deleted seperately which can be done by
         //the remove-function
         if(m_present[i])
               (am->DeleteObj(alg_id,typ_id))(type,m_elements[i]);
      }
      delete manager;
      m_present.clear();
      m_elements.clear();
   }
}

void DArray::remove()
{

   if(defined)
   {
      ZThread::ThreadedExecutor exec;
      for(int i = 0;i<manager->getNoOfServers();i++)
      {
         DServer* server = manager->getServerbyID(i);
         server->setCmd(DServer::DS_CMD_DELETE,
                        &(manager->getIndexList(i)),
                        &m_elements);
         DServerExecutor* server_ex = new DServerExecutor(server);
         exec.execute(server_ex);
      }

      exec.wait();

   }

}

void DArray::refresh(int i)
{
   DServer* server = manager->getServerByIndex(i);
   list<int> l;
   l.push_front(i);

   if(isRelation)
   {

      if(m_present[i])
         (am->DeleteObj(alg_id,typ_id))(type,m_elements[i]);

      m_elements[i].addr = (am->CreateObj(alg_id,typ_id))(type).addr;
      server->setCmd(DServer::DS_CMD_READ_REL,&l,&m_elements);
      server->run();
   }

   else
   {
     server->setCmd(DServer::DS_CMD_READ,&l,&m_elements);
      server->run();
   }

   m_present[i] = 1; // true
}


void DArray::refresh()
{
   ZThread::ThreadedExecutor exec;
   DServerExecutor* server_ex;

   //Elements are deleted if they were present
   //If the darray has a relation-type new relations must be created
   for(int i=0;i<size;i++)
   {
     if(m_present[i])
       {
         (am->DeleteObj(alg_id,typ_id))(type,m_elements[i]);
       }

      if(isRelation)
        {
          m_elements[i].addr = (am->CreateObj(alg_id,typ_id))(type).addr;
        }
   }

   //elements are read, the DServer run as threads
   for(int i = 0; i < manager->getNoOfServers(); i++)
   {
      DServer* server = manager->getServerbyID(i);
      if(isRelation)
      {
        server->setCmd(DServer::DS_CMD_READ_REL,
                       &(manager->getIndexList(i)),
                       &m_elements);
      }
      else
      {
        server->setCmd(DServer::DS_CMD_READ,
                       &(manager->getIndexList(i)),
                       &m_elements);
      }

      server_ex = new DServerExecutor(server);
      exec.execute(server_ex);
   }

    exec.wait();

     //All elements are present now
    for(int i=0;i<size;i++) m_present[i] = 1;
}


bool DArray::initialize(ListExpr n_type,
                        string n, int s,
                  ListExpr n_serverlist,
                        const vector<Word> &n_elem)
{
   //initializes an undefined array, all elements are present on the master
   defined = true;
   type = n_type;

   extractIds( type , alg_id, typ_id);

   SecondoCatalog* sc = SecondoSystem::GetCatalog();

   //check if type is relation-type
   if(sc->GetTypeName(alg_id,typ_id) == Relation::BasicType())
      isRelation = true;
   else
      isRelation = false;

   name = n;
   size = s;
   serverlist = n_serverlist;

   //create DServerManager, and
   //thereby also the DServer (worker connections)
   manager = new DServerManager(serverlist, name,type,size);

   //Set the elements-array
   m_elements = n_elem;
   m_present = vector<int>(size, 1); // all true

   //sends the elements to the respective workers
   ZThread::ThreadedExecutor exec;

   if(!isRelation)
     {

       for(int i = 0; i<manager->getNoOfServers();i++)
         {
           DServer* server = manager->getServerbyID(i);
           server->setCmd(DServer::DS_CMD_WRITE,
                          &(manager->getIndexList(i)),
                          &m_elements);
           DServerExecutor* server_exec = new DServerExecutor(server);
           exec.execute(server_exec);
         }
     }
   else
     {
       for(int i= 0;i<manager->getNoOfServers();i++)
         {
           RelationWriter* write =
             new RelationWriter(manager->getServerbyID(i),
                                &m_elements,
                                &(manager->getIndexList(i)));
           exec.execute(write);
         }
     }
   exec.wait();

   for(int i= 0;i<manager->getNoOfServers();i++)
     {
       if (manager->getServerbyID(i) -> getErrorText() != "OK")
         {
           manager -> setErrorText(string("Error Server: " +
                                          toString_d( i ) + ": " +
                                          manager->getServerbyID(i)
                                          -> getErrorText()));
           return false;
         }
     }
   return true;
}

bool DArray::initialize(ListExpr n_type, string n,
                  int s,
                        ListExpr n_serverlist)
{
   //initializes an undefined, no elements are given
   //all elements must already exist on the workers
   defined = true;
   type = n_type;

   extractIds( type , alg_id, typ_id);

   SecondoCatalog* sc = SecondoSystem::GetCatalog();

   //ceck whether array of relation?
   if(sc->GetTypeName(alg_id,typ_id) == Relation::BasicType())
      isRelation = true;
   else
      isRelation = false;

   name = n;
   size = s;

   //elements-array is empty, no elements are present on the master
   m_elements = vector<Word>(size);
   m_present = vector<int>(size, 0); // all false!;

   //creates DServerManager, which creates the
   //DServer-objects for all workers
   serverlist = n_serverlist;
   manager = new DServerManager(serverlist, name,type,size);
   return true;
}


const Word& DArray::get(int i)
{
   //returns an element of the elements-array
   if(defined && m_present[i])
   {
      return m_elements[i];
   }
   else
   {
     cout << "Error: Array not ";
     if (!defined)
       cout << "defined!!";
     else
       cout << "present!!";

     cout << " (Index: " << i << ")" << endl;

     return DServer::ms_emptyWord;
   }
}

void DArray::set(Word n_elem, int i)
{
   //sets an element of the array
   //the element is subsequently written to the corresponding worker
   list<int> l;
   l.push_front(i);

   if(defined)
   {
      m_elements[i].addr = n_elem.addr;

      if(!isRelation)
      {
         DServer* server = manager->getServerByIndex(i);
         server->setCmd(DServer::DS_CMD_WRITE,
                        &l,&m_elements);
         server->run();
      }
      else
         WriteRelation(i);

      //This element is now present on the master
      m_present[i] = 1; //true
   }
}

void DArray::WriteRelation(int index)
{
   //writes the relation in elements[index] to the corresponding worker

   //create relation iterator
   GenericRelation* rel = (Relation*)m_elements[index].addr;
   GenericRelationIterator* iter = rel->MakeScan();

   DServer* worker = manager->getServerByIndex(index);

   Tuple* t;

   list<int> l;
   l.push_front(index);


   vector<Word> word(1);

   //open tuple stream to worker
   t = iter->GetNextTuple();
   word[0].addr = t;
   worker->setCmd(DServer::DS_CMD_OPEN_WRITE_REL,
                  &l,&word);
   worker->run();

   //send each tuple
   while(t != 0)
   {

      word[0].addr = t;
      t->IncReference();
      worker->setCmd(DServer::DS_CMD_WRITE_REL,
                     0,&word);
      worker->run();
      t->DeleteIfAllowed();
      t = iter->GetNextTuple();
   }

   //close tuple stream
   worker->setCmd(DServer::DS_CMD_CLOSE_WRITE_REL,0);
   worker->run();


   delete iter;


}



/*

4.3 In and Out functions

*/

Word DArray::In( const ListExpr typeInfo, const ListExpr instance,
                        const int errorPos, ListExpr& errorInfo,
                        bool& correct )
{

   Word e;
   int algID, typID;

   extractIds(nl->Second(typeInfo),algID,typID);
   DArray* a = new DArray(nl->Second(typeInfo),
                                          getArrayName(DArray::no),
                                          nl->ListLength(instance)-1,
                                          nl->First(instance));

   ListExpr listOfElements = nl->Rest(instance);
   ListExpr element;
   int i = 0;

   do
   {
      element = nl->First(listOfElements);
      listOfElements = nl->Rest(listOfElements);
      e = ((am->InObj(algID,typID))
         (nl->Second(typeInfo),element,errorPos,errorInfo,correct));

      a->set(e,i);
      i++;
   }
   while(!nl->IsEmpty(listOfElements) && correct);

   if(correct)
   {
      return SetWord(a);
   }

   correct=false;
   return SetWord(Address(0));


}


ListExpr DArray::Out( ListExpr typeInfo, Word value )
{
   DArray* a = (DArray*)value.addr;

   ListExpr list;
   ListExpr last;
   ListExpr element;
   list = nl->OneElemList(a->getServerList());
   last=list;

   a->refresh();

   if(a->isDefined())
     {
       for(int i = 0; i<a->getSize();i++)
       {
         element = ((am->OutObj(a->getAlgID(),a->getTypID()))
                  (nl->Second(typeInfo),
                   a->get(i)));

         last=nl->Append(last,element);
       }
     }

   else
   {
      ListExpr err = nl->StringAtom("Error: DArray is undefined!");
      return err;
   }

   return list;
}


/*

4.4 Persistent Storage Functions

*/


Word DArray::Create( const ListExpr typeInfo )
{
   return SetWord(new DArray());
}


//Note: The Delete-function deletes the entire DArray, therefore the remote
//obejcts are delete and thereafter the local data structure. The Close-
//function only removes the local data structure

void DArray::Delete( const ListExpr typeInfo, Word& w )
{
   ((DArray*)w.addr)->remove();
    delete (DArray*)w.addr;
   w.addr = 0;
}

void DArray::Close( const ListExpr typeInfo, Word& w )
{
   delete (DArray*)w.addr;
   w.addr = 0;
}

//A new DArray is created locally with the same type, size and serverlist
//It is given a new name and all the elements of the old array are copied
//on the workers
Word DArray::Clone( const ListExpr typeInfo, const Word& w )
{
   DArray* alt = (DArray*)w.addr;
   DArray* neu;

   neu = new DArray(nl->Second(typeInfo),
                getArrayName(DArray::no),
                alt->getSize(),
                alt->getServerList());

   for(int i =0;i<alt->getSize();i++)
   {

      list<int> l;
      l.push_front(i);

      string to = neu->getName();
      vector<Word> w(1);
      w[0].addr = &to;

      alt->getServerManager()->getServerByIndex(i)
        ->setCmd(DServer::DS_CMD_COPY,&l,&w);

      alt->getServerManager()->getServerByIndex(i)->run();
   }


   return SetWord(neu);
}


bool DArray::Open( SmiRecord& valueRecord ,
               size_t& offset ,
               const ListExpr typeInfo ,
               Word& value )
{
   char* buffer;
   string name, type, server;
   int length;
   int size;

   //Size of the array is read
   valueRecord.Read(&size,sizeof(int),offset);
   offset+=sizeof(int);

   //type-expression (string) is read
   valueRecord.Read(&length, sizeof(length), offset);
   offset += sizeof(length);
   buffer = new char[length];
   valueRecord.Read(buffer, length, offset);
   offset += length;
   type.assign(buffer, length);
   delete buffer;

   //Workerlist is read
   valueRecord.Read(&length, sizeof(length), offset);
   offset += sizeof(length);
   buffer = new char[length];
   valueRecord.Read(buffer, length, offset);
   offset += length;
   server.assign(buffer, length);
   delete buffer;

   //name is read
   valueRecord.Read(&length, sizeof(length), offset);
   offset += sizeof(length);
   buffer = new char[length];
   valueRecord.Read(buffer, length, offset);
   offset += length;
   name.assign(buffer, length);
   delete buffer;

   ListExpr typeList;
   nl->ReadFromString( type, typeList);

   ListExpr serverlist;
   nl->ReadFromString(server,serverlist);


   value.addr = ((Word)new DArray(typeList,name,size,serverlist)).addr;
   return true;
}

bool DArray::Save( SmiRecord& valueRecord ,
               size_t& offset ,
               const ListExpr typeInfo ,
               Word& value )
{
   int length;
   int size = ((DArray*)value.addr)->getSize();

   //Size of the array is saved
   valueRecord.Write(&size, sizeof(int),offset);
   offset+=sizeof(int);

   //element-type of the array is saved
   string type;
   nl->WriteToString( type, nl->Second(typeInfo) );
   length = type.length();
   valueRecord.Write( &length, sizeof(length), offset);
   offset += sizeof(length);
   valueRecord.Write ( type.data(), length, offset);
   offset += length;

   //Workerlist of the array is saved
   string server;
   nl->WriteToString( server, ((DArray*)value.addr)->getServerList());
   length = server.length();
   valueRecord.Write( &length, sizeof(length), offset);
   offset += sizeof(length);
   valueRecord.Write ( server.data(), length, offset);
   offset += length;

   //Name of the array is saved
   string name = ((DArray*)value.addr)->getName();
   length = name.length();
   valueRecord.Write( &length, sizeof(length), offset);
   offset += sizeof(length);
   valueRecord.Write ( name.data(), length, offset);
   offset += length;



   return true;
}

/*

4.5 Kind Checking

*/

bool DArray::KindCheck( ListExpr type, ListExpr& errorInfo )
{
   if(nl->ListLength(type) == 2)
   {
      if(nl->IsEqual(nl->First(type),DArray::BasicType()))
      {

         SecondoCatalog* sc = SecondoSystem::GetCatalog();

         if(sc->KindCorrect(nl->Second(type),errorInfo))
         {
            return true;
         }
      }
   }

   return false;

}



int DArray::SizeOfObj()
{
   return sizeof(DArray);
}


/*

4.6 Type Constructor

*/

struct darrayInfo : ConstructorInfo
{

   darrayInfo()
   {

      name         = DArray::BasicType();
      signature    = "typeconstructor -> ARRAY" ;
      typeExample  = "darray int";
      listRep      =  "(a1 a2 a3)";
      valueExample = "(4 12 2 8)";
      remarks      = "A darray keeps all its element on remote systems";
   }
};


struct darrayFunctions : ConstructorFunctions<DArray>
{

   darrayFunctions()
  {

      create = DArray::Create;
      in = DArray::In;
      out = DArray::Out;
      close = DArray::Close;
      deletion = DArray::Delete;
      clone = DArray::Clone;
      kindCheck = DArray::KindCheck;
      open = DArray::Open;
      save = DArray::Save;

  }
};

darrayInfo dai;
darrayFunctions daf;
TypeConstructor darrayTC( dai, daf );











/*

5.1 Operator makeDarray

Typemap (rel() t t t...) -> darray t
Creates a new DArray. The first parameter must be a workerlist of
format rel(tuple([Server: string, Port: int]))
The other elements in the list form the elements of the array

*/

static ListExpr makeDarrayTypeMap( ListExpr args )
{
   //test whether workerlist-format is correct
  if (nl -> IsEmpty(args))
    return NList::typeError( "Empty input! Expecting \
       ((rel(tuple([Server: string, Port: int]))) t t ... t)" );

   ListExpr slist = nl->First(args);
   if(nl->ToString(slist) != "(rel (tuple ((Server string) (Port int))))")
      return nl->SymbolAtom(Symbol::TYPEERROR());

   //all other types must be the same
   args = nl->Rest(args);
   if (nl -> IsEmpty(args))
     return NList::typeError( "Only workers given! Expecting \
       ((rel(tuple([Server: string, Port: int]))) t t ... t)" );

   ListExpr first = nl->First(args);
   ListExpr rest = nl->Rest(args);

   while(!(nl->IsEmpty(rest)))
   {
      if(!nl->Equal(nl->First(rest),first))
         return nl->SymbolAtom(Symbol::TYPEERROR());

      rest = nl->Rest(rest);
   }

  //return nl->TwoElemList(nl->SymbolAtom(DArray::BasicType()),nl->First(args));
   return nl->TwoElemList(nl->SymbolAtom(DArray::BasicType()),nl->First(args));

}

static int
makeDarrayfun( Word* args, Word& result,
             int message, Word& local, Supplier s )
{
   //determine element type
   SecondoCatalog* sc = SecondoSystem::GetCatalog();

   ListExpr type = qp->GetType(s);
   ListExpr typeOfElement = sc->NumericType(nl->Second(type));


   int algID, typID;
   extractIds( typeOfElement, algID, typID);

   //determine size of the array
   int size = qp->GetNoSons(s)-1;

   vector<Word> cloned(size);

   //Objects need to be cloned to be persistent after the query ends
   for(int i = 0;i<size;i++)
   {
      cloned[i] = (am->CloneObj(algID,typID))(typeOfElement,args[i+1]);
   }

   //Generate serverlist as ListExpr from Relation
   GenericRelation* r = (GenericRelation*)args[0].addr;
   GenericRelationIterator* rit = r->MakeScan();
   ListExpr reltype;
   nl->ReadFromString("(rel (tuple ((Server string) (Port int))))",reltype);
   ListExpr serverlist = Relation::Out(reltype,rit);

   result = qp->ResultStorage(s);

   bool rc = ((DArray*)result.addr)->initialize(typeOfElement,
                                                getArrayName(DArray::no),
                                                size,serverlist,
                                                cloned);
   if (!rc)
     {
       return 1;
     }

   return 0;
}

const string makeDarraySpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
     "( <text>(rel(tuple([Server: string, Port: int])) t t ...)"
       " -> darray t</text---><text>makeDarray ( _, _ )</text--->"
       "<text>Returns a distributed Array containing x element</text--->"
       "<text>query makeDarray(server_rel,1,2,3)</text---> ))";

Operator makeDarray(
         "makeDarray",
         makeDarraySpec,
         makeDarrayfun,
         Operator::SimpleSelect,
         makeDarrayTypeMap);

/*

5.2 Operator get

((darray t) int) -> t

Returns an element of a DArray.

*/

static ListExpr getTypeMap( ListExpr args )
{
   //There must be 2 parameters
   if(nl->ListLength(args) == 2)
   {
      ListExpr arg1 = nl->First(args);
      ListExpr arg2 = nl->Second(args);

      //The first one needs to a DArray and the second int
      if(!nl->IsAtom(arg1) &&
         nl->IsEqual(nl->First(arg1),DArray::BasicType()) &&
         nl->IsEqual(arg2,CcInt::BasicType()))
      {
         //The return-type of get is the element-type of the array
         ListExpr resulttype = nl->Second(arg1);
         return resulttype;
      }
   }

   return nl->SymbolAtom(Symbol::TYPEERROR());
}

static int getFun( Word* args,
                              Word& result,
                              int message,
                              Word& local,
                              Supplier s)
{
   DArray* array = ((DArray*)args[0].addr);
   CcInt* index = ((CcInt*)args[1].addr);

   int i = index->GetIntval();

   //Determine type
   ListExpr resultType = array->getType();

   int algID,typID;
   extractIds(resultType,algID,typID);

   //retrieve element from worker
   array->refresh(i);
   //copy the element
   Word cloned = (am->CloneObj(algID,typID))(resultType,(Word)array->get(i));

  //result = qp->ResultStorage(s);
   result.addr = cloned.addr;

   return 0;
}

const string getSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
     "( <text>((darray t) int) -> t</text--->"
       "<text>get ( _, _ )</text--->"
       "<text>Returns an element from a distributed Array</text--->"
       "<text>query get(makeDarray(server_rel,1,2,3),1)</text---> ))";

Operator getA(
         "get",
         getSpec,
         getFun,
         Operator::SimpleSelect,
         getTypeMap);


/*

5.3 Operator put

((darray t)<, t, int) -> (darray t)

*/

static ListExpr putTypeMap( ListExpr args )
{
   if(nl->ListLength(args) == 3)
   {
      ListExpr arg1 = nl->First(args);
      ListExpr arg2 = nl->Second(args);
      ListExpr arg3 = nl->Third(args);

      //The first argument needs to be a darray, the second one of the
      //same type as the array-elements and the third one int
      if(!nl->IsAtom(arg1) && nl->IsEqual(nl->First(arg1),DArray::BasicType())
         && nl->Equal(nl->Second(arg1),arg2)
         && nl->IsEqual(arg3,CcInt::BasicType()))
      {
        return arg1;
      }
   }

   return nl->SymbolAtom(Symbol::TYPEERROR());
}


static int putFun( Word* args,
               Word& result,
               int message,
               Word& local,
               Supplier s)
{
  DArray* array_alt = ((DArray*)args[0].addr);
  Word element = args[1];
  int i = ((CcInt*)args[2].addr)->GetIntval();

  //new elements needs to be copied
  Word elem_n = ((am->CloneObj(array_alt->getAlgID(),
                         array_alt->getTypID()))
             (array_alt->getType(),element));

  //new array is initialized
  result = qp->ResultStorage(s);
  ((DArray*)result.addr)->initialize(array_alt->getType(),
                                     getArrayName
                                     (DArray::no),
                                     array_alt->getSize(),
                                     array_alt->getServerList());

  //all elements, except the one to be substituted, are copied on the workers
  for(int j = 0; j<array_alt->getSize();j++)
    {
     vector<Word> w (1);
      string to = ((DArray*)result.addr)->getName();
      w[0].addr = &to;

      if(j!=i)
      {

         list<int> l;
         l.push_front(j);


         array_alt->
         getServerManager() ->
         getServerByIndex(j) ->
           setCmd(DServer::DS_CMD_COPY,&l,&w);

         array_alt->getServerManager()->getServerByIndex(j)->run();
      }
      else
      {
         //The substituted element is set
         ((DArray*)result.addr)->set(elem_n,j);
      }

   }

   return 0;

}

const string putSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
     "( <text>((darray t) t int) -> darray t</text--->"
       "<text>put ( _, _, _ )</text--->"
"<text>Returns a distributed array where one element is altered</text--->"
       "<text>query put(makeDarray(server_rel,1,2,3),2,2)</text---> ))";

Operator putA(
         "put",
         putSpec,
         putFun,
         Operator::SimpleSelect,
         putTypeMap);

/*

5.4 Operator send

Internal Usage for Data Transfer between Master and Worker

*/

static ListExpr sendTypeMap( ListExpr args )
{
   //Always return int
   //Hostname and Port are appended for the value-mapping
   return
     nl->ThreeElemList(
      nl->SymbolAtom(Symbol::APPEND()),
      nl->TwoElemList(nl->StringAtom(nl->ToString(nl->First(args))),
                  nl->StringAtom(nl->ToString(nl->Second(args)))),
      nl->SymbolAtom(CcInt::BasicType()));

}

static int sendFun( Word* args,
                Word& result,
                int message,
                Word& local,
                Supplier s)
{

   //retrieve Hostname and Port
   string host = (string)(char*)((CcString*)args[3].addr)->GetStringval();
   string port = (string)(char*)((CcString*)args[4].addr)->GetStringval();
   string line;

   host = stringutils::replaceAll(host,"_",".");
   host = stringutils::replaceAll(host,"h","");
   port = stringutils::replaceAll(port,"p","");

   //Connect to master
   Socket* master = Socket::Connect(host,port,Socket::SockGlobalDomain);



   if(master!=0 && master->IsOk())
   {

      iostream& iosock = master->GetSocketStream();
      getline(iosock,line);

      //receive type of the element to be send
      if(line=="<TYPE>")
      {
         getline(iosock,line);
         ListExpr type;
         nl->ReadFromString(line,type);

         getline(iosock,line);
         if(line=="</TYPE>")
         {
            int algID,typID;
            extractIds(type,algID,typID);

            SmiRecordFile recF(false,0);
            SmiRecord rec;
            SmiRecordId recID;

            recF.Open("sendop");
            recF.AppendRecord(recID,rec);
            size_t size = 0;
            am->SaveObj(algID,typID,rec,size,type,args[2]);
            char* buffer = new char[size];

            rec.Read(buffer,size,0);

            rec.Truncate(3);
            recF.DeleteRecord(recID);
            recF.Close();

            iosock << "<SIZE>" << endl << size << endl
                     << "</SIZE>" << endl;

            master->Write(buffer,size);

            delete buffer;

            TypeConstructor* t = am->GetTC(algID,typID);
            Attribute* a;
            if(t->NumOfFLOBs() > 0 )
               a = static_cast<Attribute*>
                     ((am->Cast(algID,typID))(args[2].addr));

            Flob::clearCaches();
            for(int i = 0; i < t->NumOfFLOBs(); i++)
            {
               Flob* f = a->GetFLOB(i);

               SmiSize si = f->getSize();
               int n_blocks = si / 1024 + 1;
               char* buf = new char[n_blocks*1024];
               memset(buf,0,1024*n_blocks);

               f->read(buf,si,0);

               iosock << "<FLOB>" << endl
                        << "<SIZE>" << endl << si << endl
                        << "</SIZE>" << endl;

               for(int j = 0; j<n_blocks;j++)
                  master->Write(buf+j*1024,1024);

               iosock << "</FLOB>" << endl;
               delete buf;
            }

            iosock << "<CLOSE>" << endl;


            getline(iosock,line);
            if(line!="<FINISH>")
            {
            cout << "Error: Missing 'Finish' tag from Worker!" << endl;
            cout << " Got:'" << line << "'" << endl;
            }

            result = qp->ResultStorage(s);

            ((CcInt*)result.addr)->Set(0);


         }

         master->Close();delete master;master=0;

         return 0;
      }

      result = qp->ResultStorage(s);
      ((CcInt*)result.addr)->Set(1);
   }

   return 0;


}

const string sendSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
     "( <text>-</text--->"
       "<text>-</text--->"
       "<text>Internal Usage by DistributedAlgebra</text--->"
       "<text>-</text---> ))";

Operator sendA(
         "sendD",
         sendSpec,
         sendFun,
         Operator::SimpleSelect,
         sendTypeMap);


/*
5.5 Operator receive

Internal Usage for Data Transfer between Master and Worker

*/

static ListExpr receiveTypeMap( ListExpr args )
{
   string host = nl->ToString(nl->First(args));
   string port = nl->ToString(nl->Second(args));
   string line;

   host = stringutils::replaceAll(host,"_",".");
   host = stringutils::replaceAll(host,"h","");
   port = stringutils::replaceAll(port,"p","");

   Socket* master = Socket::Connect(host,port,Socket::SockGlobalDomain);

   if(master==0 || !master->IsOk())
      return nl->SymbolAtom(Symbol::TYPEERROR());

   iostream& iosock = master->GetSocketStream();

   getline(iosock,line);
   if(line!= "<TYPE>")
      return nl->SymbolAtom(Symbol::TYPEERROR());

   getline(iosock,line);
   ListExpr type;
   nl->ReadFromString(line,type);

   getline(iosock,line);
   if(line!= "</TYPE>")
      return nl->SymbolAtom(Symbol::TYPEERROR());

   iosock << "<CLOSE>" << endl;

   master->Close(); delete master; master=0;

   int algID, typID;
   extractIds(type,algID,typID);
   return
     nl->ThreeElemList(
          nl->SymbolAtom(Symbol::APPEND()),
          nl->TwoElemList(nl->StringAtom(nl->ToString(nl->First(args))),
                      nl->StringAtom(nl->ToString(nl->Second(args)))),
          convertType(type));

}

static int receiveFun( Word* args,
                   Word& result,
                   int message,
                   Word& local,
                   Supplier s)
{
   string host = (string)(char*)((CcString*)args[2].addr)->GetStringval();
   string port = (string)(char*)((CcString*)args[3].addr)->GetStringval();
   string line;

   host = stringutils::replaceAll(host,"_",".");
   host = stringutils::replaceAll(host,"h","");
   port = stringutils::replaceAll(port,"p","");

   Socket* master = Socket::Connect(host,port,Socket::SockGlobalDomain);

   if(master!=0 && master->IsOk())
   {

      iostream& iosock = master->GetSocketStream();
      iosock << "LOS" << endl;

      getline(iosock,line);

      if(line=="<TYPE>")
      {
         getline(iosock,line);
         ListExpr type;
         nl->ReadFromString(line,type);

         getline(iosock,line);
         if(line=="</TYPE>")
         {
            int algID,typID;
            extractIds(type,algID,typID);
            size_t size =0;

            getline(iosock,line);

            if(line=="<SIZE>")
            {
               getline(iosock,line);

               size = atoi(line.data());

               getline(iosock,line);
               if(line=="</SIZE>")
               {
                  char* buffer = new char[size];
                  iosock.read(buffer,size);

                  SmiRecordFile recF(false,0);
                  SmiRecord rec;
                  SmiRecordId recID;

                  recF.Open("receiveop");
                  recF.AppendRecord(recID,rec);
                  size_t s0 = 0;

                  rec.Write(buffer,size,0);
                  Word w;
                  result = qp->ResultStorage(s);
                  am->OpenObj(algID,typID,rec,s0,type,w);

                  result.addr = w.addr;
                  rec.Truncate(3);
                  recF.DeleteRecord(recID);
                  recF.Close();
                  getline(iosock,line);

                  delete buffer;

                  int flobs = 0;
                  while(line=="<FLOB>")
                  {
                     getline(iosock,line);
                     if(line!="<SIZE>")
                        cout << "Error: Unexpected Response from Worker!";

                     getline(iosock,line);
                     SmiSize si = atoi(line.data());

                     getline(iosock,line);
                     if(line!="</SIZE>")
                        cout << "Error: Unexpected Response from Worker!";

                     int n_blocks = si / 1024 + 1;
                     char* buf = new char[n_blocks*1024];
                     memset(buf,0,1024*n_blocks);

                     for(int i = 0; i< n_blocks; i++)
                        iosock.read(buf+1024*i,1024);


                     Attribute* a = static_cast<Attribute*>
                        ((am->Cast(algID,typID))(result.addr));


                     Flob*  f = a->GetFLOB(flobs);
                     f->write(buf,si,0);

                     delete buf;

                     getline(iosock,line);
                     if(line!="</FLOB>")
                        cout << "Error: Unexpected Response from Worker!";

                     getline(iosock,line);
                     flobs++;
                  }
                  Flob::clearCaches();

                  if(line!="<CLOSE>")
                     cout << "Error: Unexpected Response from Worker!";

                  iosock << "<FINISH>" << endl;


               }
            }

         }

         master->Close();delete master;master=0;
         return 0;
      }

   }

   return 0;

}

const string receiveSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
     "( <text>-</text--->"
       "<text>-</text--->"
       "<text>Internal Usage by DistributedAlgebra</text--->"
       "<text>-</text---> ))";

Operator receiveA(
         "receiveD",
         receiveSpec,
         receiveFun,
         Operator::SimpleSelect,
         receiveTypeMap);


/*

5.6 Operator d\_receive\_rel

*/

static ListExpr receiverelTypeMap( ListExpr args )
{
   string host = nl->ToString(nl->First(args));
   string port = nl->ToString(nl->Second(args));
   string line;

   host = stringutils::replaceAll(host,"_",".");
   host = stringutils::replaceAll(host,"h","");
   port = stringutils::replaceAll(port,"p","");

   Socket* master = Socket::Connect(host,port,Socket::SockGlobalDomain);

   if(master==0 || !master->IsOk())
      return nl->SymbolAtom(Symbol::TYPEERROR());

   iostream& iosock = master->GetSocketStream();

   getline(iosock,line);
   if(line!= "<TYPE>")
      return nl->SymbolAtom(Symbol::TYPEERROR());

   getline(iosock,line);
   ListExpr type;
   nl->ReadFromString(line,type);

   getline(iosock,line);
   if(line!= "</TYPE>")
      return nl->SymbolAtom(Symbol::TYPEERROR());

   iosock << "<CLOSE>" << endl;

   master->Close(); delete master; master=0;

   int algID, typID;
   extractIds(type,algID,typID);


   return nl->ThreeElemList(
          nl->SymbolAtom(Symbol::APPEND()),
          nl->TwoElemList(nl->StringAtom(nl->ToString(nl->First(args))),
                    nl->StringAtom(nl->ToString(nl->Second(args)))),
        convertType(type));

}

static int receiverelFun( Word* args,
                                    Word& result,
                                    int message,
                                    Word& local,
                                    Supplier s)
{
   string host = (string)(char*)((CcString*)args[2].addr)->GetStringval();
   string port = (string)(char*)((CcString*)args[3].addr)->GetStringval();

   ListExpr resultType;

   string line;

   host = stringutils::replaceAll(host,"_",".");
   host = stringutils::replaceAll(host,"h","");
   port = stringutils::replaceAll(port,"p","");

   Socket* master = Socket::Connect(host,port,Socket::SockGlobalDomain);

   result = qp->ResultStorage(s);

   GenericRelation* rel = (Relation*)result.addr;

   if(master!=0 && master->IsOk())
   {

      iostream& iosock = master->GetSocketStream();

      string line;
      getline(iosock, line);

      if(line == "<TYPE>")
      {
         getline(iosock,line);
         nl->ReadFromString(line,resultType);
         resultType = nl->Second(resultType);

         TupleType* tupleType = new TupleType(resultType);
         getline(iosock,line);
         getline(iosock,line);

         while(line == "<TUPLE>")
         {
            getline(iosock,line);
            size_t size = atoi(line.data());

            int num_blocks = (size / 1024) + 1;
            getline(iosock,line);

            iosock << "<OK>" << endl << toString_d(num_blocks)
                     << endl << "</OK>" << endl;

            char* buffer = new char[1024*num_blocks];
            memset(buffer,0,1024*num_blocks);

            for(int i = 0; i<num_blocks; i++)
               master->Read(buffer+i*1024,1024);

            Tuple* t = new Tuple(tupleType);

            t->ReadFromBin(buffer+sizeof(int),size);
            rel->AppendTuple(t);

            t->DeleteIfAllowed();

            delete buffer;
            getline(iosock,line);
         }

         delete tupleType;

         if(line=="<CLOSE>")
         {
               iosock << "<FINISH>" << endl;
               master->Close();
             delete master;
             master=0;
               return 0;
         }
         else
         {
           cout << "Error: receiverelFun did not finish correctly!" << endl;
           cout << "Line='" << line << "'" << endl;
           return 1;
         }

      }

   }

   return 1;

}

const string receiverelSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
     "( <text>-</text--->"
       "<text>-</text--->"
       "<text>Internal Usage by DistributedAlgebra</text--->"
       "<text>-</text---> ))";

Operator receiverelA(
         "d_receive_rel",
         receiverelSpec,
         receiverelFun,
         Operator::SimpleSelect,
         receiverelTypeMap);


/*


5.7 Operator d\_send\_rel

Internal Usage for Data Transfer between Master and Worker

*/


static ListExpr sendrelTypeMap( ListExpr args )
{
   return nl->ThreeElemList(
                  nl->SymbolAtom(Symbol::APPEND()),
                  nl->TwoElemList(
                  nl->StringAtom(nl->ToString(nl->First(args))),
                  nl->StringAtom(nl->ToString(nl->Second(args)))),
                  nl->SymbolAtom(CcInt::BasicType()));

}

static int sendrelFun( Word* args,
                   Word& result,
                   int message,
                   Word& local,
                   Supplier s)
{

   string host = (string)(char*)((CcString*)args[3].addr)->GetStringval();
   string port = (string)(char*)((CcString*)args[4].addr)->GetStringval();
   string line;

   host = stringutils::replaceAll(host,"_",".");
   host = stringutils::replaceAll(host,"h","");
   port = stringutils::replaceAll(port,"p","");

   Socket* master = Socket::Connect(host,port,Socket::SockGlobalDomain);

   if(master!=0 && master->IsOk())
   {

      iostream& iosock = master->GetSocketStream();
      GenericRelation* rel = (Relation*)args[2].addr;
      GenericRelationIterator* iter = rel->MakeScan();
      Tuple* t;

      while((t=iter->GetNextTuple()) != 0)
      {
         size_t cS,eS,fS;
         size_t size = t->GetBlockSize(cS,eS,fS);

         iosock << "<TUPLE>" << endl << toString_d(size)
                  << endl << "</TUPLE>" << endl;

         int num_blocks = (size / 1024) + 1;

         char* buffer = new char[num_blocks*1024];
         memset(buffer,0,num_blocks*1024);

         t->WriteToBin(buffer,cS,eS,fS);

         for(int i =0; i< num_blocks; i++)
            iosock.write(buffer+i*1024,1024);

         t->DeleteIfAllowed();
         delete buffer;
      }

      iosock << "<CLOSE>" << endl;

      master->Close(); delete master; master=0;

      delete iter;

      result = qp->ResultStorage(s);
      ((CcInt*)result.addr)->Set(0);

      return 0;
   }

   result = qp->ResultStorage(s);
   ((CcInt*)result.addr)->Set(1);

   return 0;

}

const string sendrelSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
     "( <text>-</text--->"
       "<text>-</text--->"
       "<text>Internal Usage by DistributedAlgebra</text--->"
       "<text>-</text---> ))";

Operator sendrelA(
         "d_send_rel",
         sendrelSpec,
         sendrelFun,
         Operator::SimpleSelect,
         sendrelTypeMap);



/*


5.8 Operator distribute

*/


static ListExpr distributeTypeMap( ListExpr inargs )
{
   NList args(inargs);
   if( args.length() == 4)
   {
      NList stream_desc = args.first();
      ListExpr attr_desc = args.second().listExpr();
      if( stream_desc.isList() && stream_desc.first().isSymbol(Symbol::STREAM())
            && (stream_desc.length() == 2)
            && (nl->AtomType(attr_desc) == SymbolType))
      {
         ListExpr tuple_desc = stream_desc.second().listExpr();
         string attr_name = nl->SymbolValue(attr_desc);

         if(nl->IsEqual(nl->First(tuple_desc),Tuple::BasicType()) &&
            nl->ListLength(tuple_desc) == 2)
         {
            ListExpr attrL = nl->Second(tuple_desc);

            if(IsTupleDescription(attrL))
            {
               int attrIndex;
               ListExpr attrType;

               attrIndex = FindAttribute(attrL,attr_name,attrType);

               if(nl->ListLength(attrL > 1) && attrIndex > 0
                  && nl->IsEqual(attrType,CcInt::BasicType()))
               {
                  ListExpr attrL2 = nl->TheEmptyList();
                  ListExpr last;

                  while(!nl->IsEmpty(attrL))
                  {
                     ListExpr attr = nl->First(attrL);

                     if(nl->SymbolValue(nl->First(attr)) != attr_name)
                     {
                        if(nl->IsEmpty(attrL2))
                        {
                           attrL2 = nl->OneElemList(attr);
                           last = attrL2;
                        }
                        else
                           last = nl->Append(last,attr);
                     }

                     attrL = nl->Rest(attrL);
                  }
                  return nl->ThreeElemList(
                                 nl->SymbolAtom(Symbol::APPEND()),
                                 nl->OneElemList(
                                    nl->IntAtom(attrIndex)),
                                 nl->TwoElemList(
                                    nl->SymbolAtom(DArray::BasicType()),
                                    nl->TwoElemList(
                                       nl->SymbolAtom(Relation::BasicType()),
                                       nl->TwoElemList(
                                          nl->SymbolAtom(Tuple::BasicType()),
                                          attrL2))));
               }
            }
         }
      }
   }

   return args.typeError("input is not (stream(tuple(y))) x ...");
}

static int
distributeFun (Word* args, Word& result, int message, Word& local, Supplier s)
{
   int size = ((CcInt*)(args[2].addr))->GetIntval();

   GenericRelation* r = (GenericRelation*)args[3].addr;
   GenericRelationIterator* rit = r->MakeScan();
   ListExpr reltype;
   nl->ReadFromString("(rel (tuple ((Server string) (Port int))))",
                      reltype);
   ListExpr serverlist = Relation::Out(reltype,rit);

   int attrIndex = ((CcInt*)(args[4].addr))->GetIntval() - 1;

   SecondoCatalog* sc = SecondoSystem::GetCatalog();
   ListExpr restype = nl->Second(qp->GetType(s));
   restype = sc->NumericType(restype);


   DArray* array = (DArray*)(qp->ResultStorage(s)).addr;
   array->initialize(restype,getArrayName(DArray::no),
                     size, serverlist);

   DServerManager* man = array->getServerManager();
   DServer* server = 0;

   int server_no = man->getNoOfServers();
   int rel_server = (size / server_no);


   ZThread::ThreadedExecutor ex;
   try
     {
       cout << "Multiplying worker connections... (Servers:"
            << server_no << "/" << rel_server << ")" << endl;
       for(int i = 0; i<server_no;i++)
         {
           server = man->getServerbyID(i);
           DServerMultiplyer* mult = new DServerMultiplyer(server,
                                                           rel_server);
           ex.execute(mult);
         }
       ex.wait();
     }
   catch(ZThread::Synchronization_Exception& e)
     {
       cerr << "Could not multiply  DServers!" << endl;
       cerr << e.what() << endl;
       return 1;
     }

   try
     {
       for(int i = 0; i < size; i++)
         {
           server = man->getServerByIndex(i);
           int child = man->getMultipleServerIndex(i);

           if(child > -1)
             server = (server->getChilds())[child];

           list<int> l;
           l.push_front(i);

           server->setCmd(DServer::DS_CMD_OPEN_WRITE_REL, &l);
           DServerExecutor* run = new DServerExecutor(server);
           ex.execute(run);
           //server->run();
         }
       ex.wait();

     }
   catch(ZThread::Synchronization_Exception& e)
     {
       cerr << "Could not initiate ddistibute command!" << endl;
       cerr << e.what() << endl;
       return 1;
     }

   int number = 0;

   ListExpr tupleType = nl->Second(restype);
   Word current = SetWord( Address (0) );

   qp->Open(args[0].addr);
   qp->Request(args[0].addr,current);


   try
     {

       while(qp->Received(args[0].addr))
         {
           Tuple* tuple1 = (Tuple*)current.addr;
           Tuple* tuple2 = new Tuple(tupleType);


           int j = 0;
           for(int i = 0; i < tuple1->GetNoAttributes(); i++)
             {
               if(i != attrIndex)
                 tuple2->CopyAttribute(i,tuple1,j++);
             }

           int index = ((CcInt*)(tuple1->GetAttribute(attrIndex)))->GetIntval();
           tuple1->DeleteIfAllowed();

           index = index % size;
           int child = man->getMultipleServerIndex(index);
           server = man->getServerByIndex(index);

           if(child > -1)
             server = (server->getChilds())[child];

           vector<Word> *w = new vector<Word> (1);
           (*w)[0] = SetWord(tuple2);tuple2->IncReference();

           while(server->status != 0)
             ZThread::Thread::yield();

           server->status = 1;
           server->setCmd(DServer::DS_CMD_WRITE_REL,0,w);
           DServerExecutor* exec = new DServerExecutor(server);

           ex.execute(exec);

           tuple2->DeleteIfAllowed();

           qp->Request(args[0].addr,current);

           number++;
           if ( number % 1000 == 0 )
             {
               cout << toString_d(number) << " tuples sent!" << endl;
             }
         } // while (...)

       ex.wait();

     }
   catch(ZThread::Synchronization_Exception& e)
     {
       cerr << "Could not distibute data!" << endl;
       cerr << e.what() << endl;
       return 1;
     }

   try
     {
       for(int i = 0; i < size; i++)
         {
           server = man->getServerByIndex(i);
           int child = man->getMultipleServerIndex(i);
           if(child > -1)
             server = (server->getChilds())[child];

           server->setCmd(DServer::DS_CMD_CLOSE_WRITE_REL,0);
           server->run();
         }

     }
   catch(ZThread::Synchronization_Exception& e)
     {
       cerr << "Could not finalize ddistribute!" << endl;
       cerr << e.what() << endl;
       return 1;
     }

   for(int i = 0; i<server_no;i++)
   {
      server = man->getServerbyID(i);
      server->DestroyChilds();
   }


   result.addr = array;
   return 0;

}





const string distributeSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text>((stream (tuple ((x1 t1) ... (xn tn)))) xi int (rel(tuple("
      "[Server:string, Port: int]))) ) -> "
     "(darray (rel (tuple ((x1 t1) ... (xi-1 ti-1)"
     "(xi+1 ti+1) ... (xn tn)))))</text--->"
      "<text>_ ddistribute [ _ , _ , _]</text--->"
      "<text>Distributes a stream of tuples"
     "into a darray of relations.</text--->"
      "<text>plz feed ddistribute [pkg,3,server_rel]</text--->))";

Operator distributeA (
      "ddistribute",
      distributeSpec,
      distributeFun,
      Operator::SimpleSelect,
      distributeTypeMap );

/*

5.9 Operator loop

*/
template< int dim>
static ListExpr loopTypeMap(ListExpr args)
{
  ListExpr errRes = nl->SymbolAtom(Symbol::TYPEERROR());

  NList m_args(args);
  NList params;

   if(m_args.length() == dim + 1)
    {
      NList mapdesc = m_args.elem(dim + 1);

      if (mapdesc.first().length() != dim + 2 )
        {
          return  errRes;
        }

      if (mapdesc.first().first() != NList(Symbol::MAP()))
        {
          return  errRes;
        }
      if (mapdesc.first().elem(dim+2) == NList(Symbol::TYPEERROR()))
        {
          return  errRes;
        }

      params = NList(dim).enclose();

      params.append(NList(NList(NList(mapdesc.
                                      second().
                                      elem(dim + 2).
                                      convertToString(),
                                      true, true))));

      for (unsigned int i = 0; i < dim; ++i)
        {
          NList darraydesc = m_args.elem(i + 1);

          // check for correct types
          if (darraydesc.length() != 2)
            {
              return  errRes;
            }
          if(darraydesc.first() == (DArray::BasicType()))
            {
              return  errRes;
            }

          //check, if darray and map are equal
          if (darraydesc.first().second() != mapdesc.first().elem(i + 2))
            {
              return  errRes;
            }
          params.append(NList
                        (NList
                         (mapdesc.second().
                          elem(i + 2).first().convertToString(), true)));
        }

      NList res =
        NList(NList(Symbols::APPEND()),
              params,

              NList(NList(DArray::BasicType()),
                    mapdesc.first().elem(dim + 2)));

      return res.listExpr();
    }

   return errRes;
}

template< int dim>
static int loopValueMap
(Word* args, Word& result, int message, Word& local, Supplier s)
{
   result = qp->ResultStorage(s);

   SecondoCatalog* sc = SecondoSystem::GetCatalog();
   ListExpr type = sc->NumericType(nl->Second((qp->GetType(s))));
   string command = ((FText*)args[dim + 2].addr)->GetValue();

   ZThread::ThreadedExecutor exec;DServer* server;
   DServerExecutor* ex;

   int size = 0;
   ListExpr serverList;
   vector<string> from;

   string rpl = "!";
   for (int i = 0; i < dim; i ++)
     {
       DArray* alt = (DArray*)args[i].addr;
       if (size == 0)
         {
           size = alt -> getSize();
         }

       size = min(size, alt -> getSize());
       serverList = alt->getServerList();

       from.push_back(alt->getName());
       // TODO: need to compare server lists!

       string elementname = ((CcString*)args[i+ dim + 3].addr) -> GetValue();
       command = stringutils::replaceAll(command, elementname, rpl);
       rpl += "!";
     }

   string name = getArrayName(DArray::no);
   vector<Word> w (2);
   w[0].addr = &command;
   w[1].addr = &name;
   
   ((DArray*)(result.addr))->initialize(type,
                                        name,
                                        size,
                                        serverList);

   //DServerManager* man = alt->getServerManager();
   DServerManager* man = ((DArray*)(result.addr)) -> getServerManager();

   //Multiply worker connections
   int server_no = man->getNoOfServers();
   int rel_server = (size / server_no);

   cout << "Multiplying worker connections...(Servers:"
        << server_no << "/" << rel_server  << ")" << endl;
   try
     {
       for(int i = 0; i<server_no;i++)
         {
           server = man->getServerbyID(i);
           DServerMultiplyer* mult = new DServerMultiplyer(server,rel_server);
           exec.execute(mult);
         }
       exec.wait();
     }
   catch(ZThread::Synchronization_Exception& e)
     {
       cerr << "Could not multiply  DServers!" << endl;
       cerr << e.what() << endl;
       return 1;
     }

   if (!man -> checkServers())
     {
       cerr << "Check workers failed!" << endl;
       cerr << man -> getErrorText() << endl;
       return 1;
     }

   try
     {
       for(int i=0; i < size; i++)
         {
           server = man->getServerByIndex(i);
           int child = man->getMultipleServerIndex(i);
           if(child > -1)
             server = (server->getChilds())[child];

           list<int> l;
           l.push_front(i);
           server->setCmd(DServer::DS_CMD_EXEC, &l, &w, &from);
           ex = new DServerExecutor(server);
           exec.execute(ex);
         }
       exec.wait();
     }

   catch(ZThread::Synchronization_Exception& e)
     {
       cerr << "Could not execute command on workers!" << endl;
       cerr << e.what() << endl;
       return 1;
     }

   //Close additional connections

   for(int i = 0; i<server_no;i++)
   {
      server = man->getServerbyID(i);
      server->DestroyChilds();
   }

   return 0;

}

const string loopSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text>((darray t) (map t u)) -> (darray u)</text--->"
      "<text>_ dloop [ fun ]</text--->"
      "<text>Evaluates each element with a function, that needs to be given"
      "as paremeter function </text--->"
      "<text>query plz_a20 dloop[. count]</text--->))";
struct loopaSpec : OperatorInfo {
  loopaSpec() : OperatorInfo() {
    name = "dloopa";
    signature = "((darray t) (darray u) (map t u r)) -> (darray r)";
    syntax = "_ _ dloopa [ fun ]";
    meaning =
      "Evaluates each element of each darray with a function, "
      "that needs to be given as paremeter function";
  }
};


Operator loopA (
      "dloop",
      loopSpec,
      loopValueMap<1>,
      Operator::SimpleSelect,
      loopTypeMap <1>);

Operator dloopA (loopaSpec(),
                 loopValueMap<2>,
                 loopTypeMap <2>);

/*

5.10.1 Type Operator DELEMENT
(derived from Operator ELEMENT of the ArrayAlgebra)

*/

ListExpr delementTypeMap( ListExpr args )
{
   if(nl->ListLength(args) >= 1)
   {
      ListExpr first = nl->First(args);
      if (nl->ListLength(first) == 2)
      {
         if (nl->IsEqual(nl->First(first), DArray::BasicType()))
         {
            return nl->Second(first);
         }
      }
   }
   return nl->SymbolAtom(Symbol::TYPEERROR());
}

const string DELEMENTSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Remarks\" )"
    "( <text>((array t) ... ) -> t</text--->"
      "<text>type operator</text--->"
      "<text>Extracts the type of the elements from a darray type given "
      "as the first argument.</text--->"
      "<text>not for use with sos-syntax</text---> ))";

Operator dElementA (
      "DELEMENT",
      DELEMENTSpec,
      0,
      Operator::SimpleSelect,
      delementTypeMap );

/*

5.10.2 Type Operator DELEMENT2

*/

ListExpr delement2TypeMap( ListExpr args )
{
   if(nl->ListLength(args) >= 2)
   {
      ListExpr second = nl->Second(args);
      if (nl->ListLength(second) == 2)
      {
         if (nl->IsEqual(nl->First(second), DArray::BasicType()))
         {
            return nl->Second(second);
         }
      }
   }
   return nl->SymbolAtom(Symbol::TYPEERROR());
}

const string DELEMENT2Spec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Remarks\" )"
    "( <text>((array t) ... ) -> t</text--->"
      "<text>type operator</text--->"
      "<text>Extracts the type of the elements from a darray type given "
      "as the second argument.</text--->"
      "<text>not for use with sos-syntax</text---> ))";

Operator dElementA2 (
      "DELEMENT2",
      DELEMENT2Spec,
      0,
      Operator::SimpleSelect,
      delement2TypeMap );

/*
5.11 Operator ~dtie~

The operator calculates a single "value" of an
darray by evaluating the elements of an darray
with a given function from left to right, e.g.

dtie ( (a1, a2, ... , an), + ) = a1 + a2 + ... + an

The formal specification of type mapping is:

---- ((darray t) (map t t t)) -> t
----

*/
static ListExpr
dtieTypeMap( ListExpr args )
{
  if (nl->ListLength(args) == 2)
  {
    ListExpr arrayDesc = nl->First(args);
    ListExpr mapDesc = nl->Second(args);

    if ((nl->ListLength(arrayDesc) == 2)
        && (nl->ListLength(mapDesc) == 4))
    {
      if (nl->IsEqual(nl->First(arrayDesc), DArray::BasicType())
          && nl->IsEqual(nl->First(mapDesc), Symbol::MAP()))
      {
        ListExpr elementDesc = nl->Second(arrayDesc);

        if (nl->Equal(elementDesc, nl->Second(mapDesc))
            && nl->Equal(elementDesc, nl->Third(mapDesc))
            && nl->Equal(elementDesc, nl->Fourth(mapDesc)))
        {
          return elementDesc;
        }
      }
    }
  }

  return nl->SymbolAtom(Symbol::TYPEERROR());
}

static int
dtieFun( Word* args, Word& result, int message, Word& local, Supplier s )
{
  SecondoCatalog* sc = SecondoSystem::GetCatalog();
  DArray* array = ((DArray*)args[0].addr);

  ArgVectorPointer funargs = qp->Argument(args[1].addr);
  Word funresult;

  ListExpr typeOfElement = sc->NumericType(qp->GetType(s));

  int algebraId;
  int typeId;
  extractIds(typeOfElement, algebraId, typeId);
  int n = array->getSize();

   array->refresh();

 //copy the element
  Word partResult =
    (am->CloneObj(algebraId, typeId))(typeOfElement,(Word)array->get(0));

  for (int i=1; i<n; i++) {

    //copy the element
    Word ielem =
      (am->CloneObj(algebraId, typeId))(typeOfElement,(Word)array->get(i));

    (*funargs)[0] = partResult;
    (*funargs)[1] = ielem;

    qp->Request(args[1].addr, funresult);

    if (funresult.addr != partResult.addr) {
      if (i>1)
      {
        (am->DeleteObj(algebraId, typeId))(typeOfElement, partResult);
      }

      partResult =
      Array::genericClone(algebraId, typeId, typeOfElement, funresult);
    }
    (am->DeleteObj(algebraId, typeId))(typeOfElement,ielem);
  }
  // In the next statement the (by the Query Processor) provided place for
  // the result is not used in order to be flexible with regard to the
  // result type.

  result.addr = partResult.addr;
  return 0;
}

const string dtieSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text>((array t) (map t t t)) -> t</text--->"
      "<text>_ tie [ fun ]</text--->"
      "<text>Calculates the \"value\" of an darray evaluating the elements of "
      "the darray with a given function from left to right.</text--->"
      "<text>query ai tie[fun(i:int,l:int)(i+l)]</text---> ))";

Operator dtie(
      "dtie",
      dtieSpec,
      dtieFun,
      Operator::SimpleSelect,
      dtieTypeMap );


/*
5.12 Operator ~dsummarize~

The operator ~dsummarize~ provides a stream of tuples from a darray of
relations. For this purpose, the operator scans all relations beginning with
the first relation of the array.

The formal specification of type mapping is:

---- ((darray (rel t))) -> (stream t)

     at which t is of the type tuple
----

Note that the operator ~dsummarize~ is not exactly inverse to the operator
~ddistribute~ because the index of the relation is not appended to the
attributes of the outgoing tuples. If the darray has been constructed by the
operator ~ddistribute~ the order of the resulting stream in most cases does not
correspond to the order of the input stream of the operator ~ddistribute~.

*/
static ListExpr
dsummarizeTypeMap( ListExpr args )
{
  if (nl->ListLength(args) == 1)
  {
    ListExpr arrayDesc = nl->First(args);

    if (nl->ListLength(arrayDesc) == 2
        && nl->IsEqual(nl->First(arrayDesc), DArray::BasicType()))
    {
      ListExpr relDesc = nl->Second(arrayDesc);

      if (nl->ListLength(relDesc) == 2
          && nl->IsEqual(nl->First(relDesc), Relation::BasicType()))
      {
        ListExpr tupleDesc = nl->Second(relDesc);
        if (nl->IsEqual(nl->First(tupleDesc), Tuple::BasicType()))
        {
          return nl->TwoElemList(nl->SymbolAtom(Symbol::STREAM()),
                                 nl->Second(relDesc));
        }
      }
    }
  }

  ErrorReporter::ReportError(
     "dsummarize: Input type darray( rel( tuple(...))) expected!");
  return nl->SymbolAtom(Symbol::TYPEERROR());
}

static int
dsummarizeFun( Word* args, Word& result, int message, Word& local, Supplier s )
{
  struct DArrayIterator
  {
    private:
    int current;
    DArray* darray;
    GenericRelationIterator* rit;

    // create an Tuple iterater for the next array element
    bool makeNextRelIter()
    {
      if (rit)
        delete rit;

      while (current < darray->getSize())
      {
    Relation* r=static_cast<Relation*>(darray->get(current).addr);
    if (r->GetNoTuples() > 0)
    {
      current++;
      rit = r->MakeScan();
      return true;
    }
    else
    {
      current++;
      continue;
    }
      }

      rit=0;
      return false;
    }

    public:
    DArrayIterator(DArray* d, const int pos=0) : current(pos), darray(d), rit(0)
    {
      darray->refresh();
      makeNextRelIter();
    }

    ~DArrayIterator()
    {
      if (rit)
        delete rit;
    }

    Tuple* getNextTuple() // try to get next tuple
    {
      if (!rit)
        return 0;

      Tuple* t = rit->GetNextTuple();
      if ( !t )
      {
         if (!makeNextRelIter())
           return 0;
         else
           return rit->GetNextTuple();
      }
      return t;
    }
  };

  DArrayIterator* dait = 0;
  dait = (DArrayIterator*)local.addr;

  switch (message) {
    case OPEN : {
      dait = new DArrayIterator( (DArray*)args[0].addr );
      local.addr = dait;
      return 0;
    }
    case REQUEST : {

      Tuple* t = dait->getNextTuple();
      if (t != 0) {
        result = SetWord(t);
        return YIELD;
      }
      return CANCEL;
    }
    case CLOSE : {
      if(local.addr)
      {
        dait = (DArrayIterator*)local.addr;
        delete dait;
        local = SetWord(Address(0));
      }
      return 0;
    }
    default : {
      return 0;
    }
  }
  return 0;
}

const string dsummarizeSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text>((darray (rel t))) -> (stream t)</text--->"
      "<text>_ dsummarize</text--->"
      "<text>Produces a stream of the tuples from all relations in the "
      "darray.</text--->"
      "<text>query prel dsummarize consume</text---> ))";

Operator dsummarize (
      "dsummarize",
      dsummarizeSpec,
      dsummarizeFun,
      Operator::SimpleSelect,
      dsummarizeTypeMap );

/*
5.14 Operator ~checkworkers~

The operator ~ceckworkers~ takes a relation of type host:string, port:int
and checks, if there exists a running instance of SecondoMonitor at
the specified host, listening at the specified port
It also checks, if the database distributed is available.

*/
static bool
checkWorkerRunning(const string &host, int port,  
                   const string &cmd, string &msg)
{
  // check worker running
  string line;
  Socket* server = Socket::Connect( host, toString_d(port), 
                                    Socket::SockGlobalDomain,
                                    5,
                                    1);
  
      
  if(server == 0 || !server->IsOk())
    {      
      msg = "Cannot connect to worker";
      if (server != 0)
        {
          msg += ":" + server -> GetErrorText(); 
          server->Close();
          delete server;
        }

      return false;
    }

  iostream& iosock = server->GetSocketStream();
      
  if (!server -> IsOk())
    {
      msg = "Cannot access worker socket"; 
      
      server->Close();
      delete server;
      return false;
    }
  do
    {
      if (!iosock.good())
        {
          msg = "Communication is blocked! Restart Worker";
          return false;
        }
      getline( iosock, line );
      
    } while (line.empty());
      
  bool startupOK = true;

  if(line=="<SecondoOk/>")
    {
      if (!iosock.good())
        {
          msg = "Communication is blocked! Restart Worker";
          return false;
        }
      iosock << "<Connect>" << endl << endl 
             << endl << "</Connect>" << endl;
      if (!iosock.good())
        {
          msg = "Communication is blocked! Restart Worker";
          return false;
        }
      getline( iosock, line );
          
      if( line == "<SecondoIntro>")
        {
          do
            {
              if (!iosock.good())
                {
                  msg = "Communication is blocked! Restart Worker";
                  return false;
                }
              getline( iosock, line);
              
            }  while(line != "</SecondoIntro>");
            
              
        }
      else 
        startupOK = false;
          
    }
  else 
    startupOK = false;
   
  if (!startupOK)
    {
      msg = "Unexpected response from worker";
      server->Close();
      delete server;
      return false;
    }

  if (!(server -> IsOk()))
    { 
      msg = "Cannot Connect to Worker";
      server->Close();
      delete server;
      return false;
    } // if (!(server -> IsOk()))


  // check db distributed available
  if (!iosock.good())
    {
      msg = "Communication is blocked! Restart Worker";
      return false;
    }

  iosock << "<Secondo>" << endl << "1" << endl 
         << "open database distributed" << endl 
         << "</Secondo>" << endl;

   if (!iosock.good())
    {
      msg = "Communication is blocked! Restart Worker";
      return false;
    }

  getline( iosock, line );

  if(line=="<SecondoResponse>")
    {
      do
        {
          if (!iosock.good())
            {
              msg = "Communication is blocked! Restart Worker";
              return false;
            }
          getline( iosock, line );
   
          if(line.find("ERROR") != string::npos)
            {
              msg = 
                "No database \"distributed\"";
              server->Close();
              delete server;
              return false;
            }
                        
        }
      while(line.find("</SecondoResponse>") == string::npos);
    }
  else 
    msg = "Unexpected response from worker";
        
  // check if db distributed is unique
  if (!iosock.good())
    {
      msg = "Communication is blocked! Restart Worker";
      return false;
    }
  iosock << "<Secondo>" << endl << "1" << endl 
         << cmd << endl 
         << "</Secondo>" << endl;
   
  if (!iosock.good())
    {
      msg = "Communication is blocked! Restart Worker";
      return false;
    }

  getline( iosock, line );

  if(line=="<SecondoResponse>")
    {
      do
        {
          if (!iosock.good())
            {
              msg = "Communication is blocked! Restart Worker";
              return false;
            }
          getline( iosock, line );
   
          if(line.find("ERROR") != string::npos)
            {
              msg = 
                "Database \"distributed\" in use";
              server->Close();
              delete server;
              return false;
            }
                        
        }
      while(line.find("</SecondoResponse>") == string::npos);
    }
  else 
    msg = "Unexpected response from worker";

  if (!iosock.good())
    {
      msg = "Communication is blocked! Restart Worker";
      return false;
    }
  iosock << "<Disconnect/>" << endl;
  server->Close();
  
  delete server;
  server=0;

  return true;
}

static ListExpr
checkWorkersTypeMap( ListExpr args )
{
  NList myargs = NList(args).first();

  if (myargs.hasLength(2) && myargs.first().isSymbol(sym.STREAM()))
    {
      NList tupTypeL( myargs.second().second());

      if (tupTypeL.length() == 2)
        {
          //The return-type of checkWorkers is 
          // a stream of workers each appended w/ a status msg;
          NList app (NList("Status"), NList(CcString::BasicType())); 

          NList tuple = NList(tupTypeL.first(), tupTypeL.second(), app);

          NList result = NList().tupleStreamOf(tuple);
          return result.listExpr(); 
        }
      return NList::typeError("Tuple stream has too many/few attributes");
    }
  
  return 
    NList::typeError("Expecting a stream of tuples(Server:string, port:int)");
}

struct CwLocalInfo
{
  TupleType *resTupleType;
  vector<pair<string, int> > hostport;
  string cmd;

  CwLocalInfo(ListExpr inTT){ resTupleType = new TupleType(inTT); }
  ~CwLocalInfo() { resTupleType -> DeleteIfAllowed(); }
};

static int
checkWorkersFun (Word* args, Word& result, int message, Word& local, Supplier s)
{
  CwLocalInfo *localInfo;
  Tuple *curTuple;
  Tuple *resTuple;
  Word cTupleWord;

  switch (message)
    {
    case OPEN:
      {
        qp->Open(args[0].addr);
      
        ListExpr resultTupleNL = GetTupleResultType(s);

        localInfo = new CwLocalInfo(nl->Second(resultTupleNL));
      
        localInfo -> cmd = "let test1 = \"test\"";

        local.setAddr(localInfo);
        return 0;
      }
  
    case REQUEST:
      {
        if (local.addr == 0)
          return CANCEL;

        localInfo = (CwLocalInfo*)local.addr;

    
        qp->Request(args[0].addr, cTupleWord);
        if (qp->Received(args[0].addr))
          {
            curTuple = (Tuple*)cTupleWord.addr;
            string host =
              ((CcString*)(curTuple->GetAttribute(0)))->GetValue();
            int port =
              ((CcInt*)(curTuple->GetAttribute(1)))->GetValue();

            curTuple -> DeleteIfAllowed();

            string msg = "OK"; 
            
            bool retVal = checkWorkerRunning(host, port, 
                                             localInfo -> cmd, msg);
            if (!retVal)
              {
                msg = "ERROR: " + msg + "!";
              }
     
            resTuple = new Tuple(localInfo->resTupleType);
            resTuple->PutAttribute(0, new CcString(true, host));
            resTuple->PutAttribute(1, new CcInt(true, port));
            resTuple->PutAttribute(2, new CcString(true, msg));
        
            localInfo -> hostport.push_back(make_pair<string, int>(host, port));
        
            result.setAddr(resTuple);
            return YIELD;
          }
        else
          return CANCEL;
      }
    case CLOSE:
      {
        if (local.addr == 0)
          return CANCEL;

        localInfo = (CwLocalInfo*)local.addr;
        localInfo -> cmd = "delete test1";
    
        while( !localInfo -> hostport.empty() ) 
          {
            string msg = "OK";
            checkWorkerRunning(localInfo -> hostport.back().first, 
                               localInfo -> hostport.back().second, 
                               localInfo -> cmd, msg);
            
            localInfo -> hostport.pop_back();
          }
        
        delete localInfo;
        local.setAddr(0);
        qp->Close(args[0].addr);
        return 0;
      }
    }
 
  return 0;
}

const string checkWorkersSpec =
   "(( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "(<text>((stream (tuple ((Server string) (Port int))))) ->  "
    "((stream (tuple ((Server string) (Port int) (Status string)))))</text--->"
      "<text>_ check_workers</text--->"
      "<text>checks workers, if running and database 'distributed'"
      "exists</text--->"
      "<text>query workers check_workers</text---> ))";

Operator checkWorkers (
      "check_workers",
      checkWorkersSpec,
      checkWorkersFun,
      Operator::SimpleSelect,
      checkWorkersTypeMap );

/*

6 Creating the Algebra

*/



class DistributedAlgebra : public Algebra
{
   public:
      DistributedAlgebra() : Algebra()
      {
         AddTypeConstructor( &darrayTC );
         darrayTC.AssociateKind(Kind::ARRAY());
         AddOperator( &makeDarray );
         AddOperator( &getA );
         AddOperator( &putA );
         AddOperator( &sendA );
         AddOperator( &receiveA);
         AddOperator( &receiverelA);
         AddOperator( &sendrelA);
         AddOperator( &distributeA);
         AddOperator( &loopA); loopA.SetUsesArgsInTypeMapping();
         AddOperator( &dloopA); dloopA.SetUsesArgsInTypeMapping();
         AddOperator( &dElementA);
         AddOperator( &dElementA2);
         AddOperator( &dtie);
         AddOperator( &dsummarize );
         AddOperator( &checkWorkers );
      }
      ~DistributedAlgebra() {}
};



/*

7 Initialization

*/

extern "C"
Algebra*
InitializeDistributedAlgebra(NestedList *nlRef, QueryProcessor *qpRef)
{
  nl = nlRef;
  qp = qpRef;
  return new DistributedAlgebra();
}


