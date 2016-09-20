
/*
----
This file is part of SECONDO.

Copyright (C) 2016,
Faculty of Mathematics and Computer Science,
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


//[<] [\ensuremath{<}]
//[>] [\ensuremath{>}]

*/


#include "NestedList.h"
#include "ListUtils.h"
#include "Stream.h"
#include "RelationAlgebra.h"
#include "Symbols.h"
#include "mmheap.h"

#include "Sort2Heaps.h"

#include <vector>

ListExpr Sort2HeapsTM(ListExpr args){

 if(!nl->HasLength(args,1)){
   return listutils::typeError("wrong number of args");
 }
 if(!Stream<Tuple>::checkType(nl->First(args))){
   return listutils::typeError("stream(tuple) expected");
 }

 ListExpr appendList1 = nl->OneElemList(nl->IntAtom(0));
 ListExpr appendList2 = nl->OneElemList(nl->BoolAtom(true));
 ListExpr last1 = appendList1;
 ListExpr last2 = appendList2;

 ListExpr attrList = nl->Second(nl->Second(nl->First(args)));
 attrList = nl->Rest(attrList); // 0 already part of append list
 int c = 1;
 while(!nl->IsEmpty(attrList)){
    last1 = nl->Append(last1,nl->IntAtom(c));
    last2 = nl->Append(last2, nl->BoolAtom(true));
    c++;
    attrList = nl->Rest(attrList);
 }

 return nl->ThreeElemList(nl->SymbolAtom(Symbols::APPEND()),
                          nl->TwoElemList(appendList1, appendList2),
                          nl->First(args));
}


ListExpr SortBy2HeapsTM(ListExpr args){

 std::string err = "stream[tuple] x list expected";
 if(!nl->HasLength(args,2)){
    return listutils::typeError("invalid number of arguments");
 }
  
 if(!Stream<Tuple>::checkType(nl->First(args))){
    return listutils::typeError("first argument has to be a stream of tuples");
 }

 ListExpr attrlist = nl->Second(nl->Second(nl->First(args)));

 if(nl->AtomType(nl->Second(args))!=NoAtom){
   return listutils::typeError("second argument is not a list");
 }
 
 if(nl->IsEmpty(nl->Second(args))){
   return listutils::typeError("missing BY list");
 }

 std::set<std::string> usednames;
 ListExpr positions;
 ListExpr directions;
 ListExpr lastpos;
 ListExpr lastdir;

 bool first  = true;

 ListExpr bylist = nl->Second(args);
 while(!nl->IsEmpty(bylist)){
   ListExpr by = nl->First(bylist);
   bylist = nl->Rest(bylist);
   if(   !nl->HasLength(by,1) && !nl->HasLength(by,2) 
      && (nl->AtomType(by)!=SymbolType)){
      return listutils::typeError("each elemnt in BY list must "
                                  "have one or two elements");
   }
   bool asc = true;
   ListExpr by1 = nl->AtomType(by)==SymbolType?by:nl->First(by);
   if(nl->AtomType(by1)!=SymbolType){
     return listutils::typeError("invalid attribute name : " 
                                  + nl->ToString(by1));
   }
   std::string attrname = nl->SymbolValue(by1);
   if(usednames.find(attrname)!=usednames.end()){
      return listutils::typeError("Attribute name " + attrname 
                                  + " used twice");
   }
   usednames.insert(attrname);
   ListExpr type;
   int index = listutils::findAttribute(attrlist,attrname, type);
   if(!index){
      return listutils::typeError("Attribute nma e" + attrname 
                                  + " not part of the tuple");
   }
   if(nl->HasLength(by,2)){
     ListExpr by2 = nl->Second(by);
     if(nl->AtomType(by2)!=SymbolType){
        return listutils::typeError("invalid sort direction, "
                                    "allowed are asc and desc");
     }
     std::string d = nl->SymbolValue(by2);
     if(d=="asc"){
        asc = true;
     } else if(d=="desc"){
        asc = false;
     } else {
        return listutils::typeError("invalid sort direction, "
                                    "allowed are asc and desc");
     }
   }
   if(first){
      positions = nl->OneElemList(nl->IntAtom(index-1));
      directions = nl->OneElemList(nl->BoolAtom(asc));
      lastpos = positions;
      lastdir = directions;
      first = false;
   } else {
      lastpos = nl->Append(lastpos, nl->IntAtom(index-1));
      lastdir = nl->Append(lastdir, nl->BoolAtom(asc)); 
   }
 }

 return nl->ThreeElemList(nl->SymbolAtom(Symbols::APPEND()),
                          nl->TwoElemList(positions, directions),
                          nl->First(args));
}


class TupleSmaller{
public:
   TupleSmaller(const std::vector<std::pair<int,bool> >& _p): positions(_p){}

   bool operator()(Tuple* t1, Tuple* t2) const{
     for(size_t i=0;i<positions.size();i++){
        int c = t1->GetAttribute(positions[i].first)->
                             Compare(t2->GetAttribute(positions[i].first));
        if(c < 0) return  positions[i].second;
        if(c > 0) return !positions[i].second;
     }
     // tuples are equal
     return false;
   }
   private:
     std::vector<std::pair<int, bool> > positions;
};

class pc{
       public:
         pc(TupleSmaller _comp):comp(_comp){}
         bool operator()(const std::pair<Tuple*,TupleFileIterator*>& a, 
                    const std::pair<Tuple*, TupleFileIterator*>& b){
           return comp(a.first,b.first);
       }
       private:
         TupleSmaller comp;
};


class OutHeap{
  public:

   OutHeap(std::vector<TupleFile*> _files, 
          mmheap::mmheap<Tuple*, TupleSmaller>* h,
          TupleSmaller _comp):
       files(_files), h2(0), heap(h), comp(_comp), lastFromFile(0), 
       lastFromHeap(0),nextRes(0){
      init();
   }

   ~OutHeap(){
       // merge heaps into a single one
       if(h2){
         while(!h2->empty()){
           std::pair<Tuple*, TupleFileIterator*>m  = *h2->min();
           h2->deleteMin();
           m.first->DeleteIfAllowed();
           delete m.second;
         }
         delete h2;
       }
       if(lastFromFile){
           lastFromFile->DeleteIfAllowed();
       }
       if(lastFromHeap){
          lastFromHeap->DeleteIfAllowed();
       }
       if(nextRes){
          nextRes->DeleteIfAllowed();
       }
   }


   inline Tuple* next(){
     Tuple* res = nextRes;
     retrieveNext();
     return res;
   }


   private:
       std::vector<TupleFile*> files;
       mmheap::mmheap< std::pair<Tuple*, TupleFileIterator*>, pc >* h2;
       mmheap::mmheap<Tuple*, TupleSmaller>* heap;
       TupleSmaller comp;
       Tuple* lastFromFile;
       Tuple* lastFromHeap;
       Tuple* nextRes;

       void init(){
          if(files.size()>0){
             h2 = new mmheap::mmheap<
                      std::pair<Tuple*,TupleFileIterator*>, pc>(comp);
             for(size_t i=0;i<files.size(); i++){
                TupleFileIterator* it = files[i]->MakeScan();
                Tuple* first = it->GetNextTuple();
                if(first){
                   h2->insert(std::make_pair(first,it));
                } else {
                  delete it;
                }
             }
          } else {
            h2 = 0;
          }
          retrieveNextFromHeap();
          if(!h2){
                lastFromFile = 0;
          } else {
            retrieveNextFromFile();
          }
          retrieveNext();
       }

       void retrieveNext() {
           if(!lastFromFile){
              nextRes = lastFromHeap;
              retrieveNextFromHeap();
           } else if(!lastFromHeap){
               nextRes = lastFromFile;
               retrieveNextFromFile();
           }  else {
              if( comp(lastFromHeap, lastFromFile)){
                nextRes = lastFromHeap;
                retrieveNextFromHeap();
              } else {
               nextRes = lastFromFile;
               retrieveNextFromFile();
              }
           }
       }

       void retrieveNextFromFile(){
          if(h2->empty()){
            lastFromFile=0;
          } else {
             std::pair<Tuple*,TupleFileIterator*> m = *h2->min();
             lastFromFile = m.first;
             h2->deleteMin();
             Tuple* n = m.second->GetNextTuple();
              if(n){
                 h2->insert(std::make_pair(n,m.second));
              } else {
                 delete m.second;
              }
          }
       }

       void retrieveNextFromHeap(){
          if(heap && !heap->empty()){
            lastFromHeap = *heap->min();
            heap->deleteMin();
          } else {
            lastFromHeap=0;
          }
       }
   

};


/*
LocalInfo

*/

class Sort2HeapsInfo{

  public:
     Sort2HeapsInfo(Word _stream, 
               std::vector< std::pair<int, bool> >& positions, 
               ListExpr _tt, size_t _maxFiles, size_t _maxMem): 
               stream(_stream), Comp(positions), maxFiles(_maxFiles), 
               maxMem(_maxMem){
        tt = new TupleType(_tt);
        sort();
     }


     ~Sort2HeapsInfo(){
         for(size_t i=0;i<files.size();i++){
            if(files[i]){
               files[i]->Close();
               delete files[i];
            }
         } 
         tt->DeleteIfAllowed();
         if(h1){
           while(!h1->empty()){
              Tuple* t = *h1->min();
              if(t){
                 t->DeleteIfAllowed();
              }
              h1->deleteMin();
           }
           delete h1;
         }
         // h2 should not exists
         if(outHeap){
            delete outHeap;
         }
       }
      
     Tuple* next(){
        return outHeap->next();
      }
     

  private:
     Stream<Tuple> stream;
     TupleSmaller Comp;
     size_t maxFiles;
     size_t maxMem;
     TupleType* tt;
     std::vector<TupleFile*> files;
     mmheap::mmheap<Tuple*, TupleSmaller>* h1;
     mmheap::mmheap<Tuple*, TupleSmaller>* h2;
     int stage;
     OutHeap*  outHeap;

     void sort(){
         partition();
         // after partition, we have a set of files and possible two
         // non_empty heaps.
         // we reduce the number of files by mergiung them, until 
         // maximum of maxFile files exist.
         //cout << "sorth created " << files.size() 
         //     << " files during partition" << endl;
         while( files.size() > maxFiles){
            // cout << files.size() ;
             mergeFiles();
             // cout << " files merged  to " << files.size() << endl;
         }
         //cout << "reduced to " << files.size() 
         //     << " files during merge" << endl;
         // merge h2 into h1 if necessary
         if(h2){
             while(!h2->empty()){
                Tuple* t = *h2->min();
                h1->insert(t);
                h2->deleteMin();
             }
            delete h2;
            h2 = 0;
         }  
         outHeap = new  OutHeap(files,h1,Comp);
     }



     void partition(){
        // distributes the incoming stream into a set of files
        h1 = new mmheap::mmheap<Tuple*, TupleSmaller>(Comp);    
        h2 = new mmheap::mmheap<Tuple*, TupleSmaller>(Comp);    
    
        size_t h1Mem = 0; // currentlic used memory in h1
        Tuple* nextTuple;
        stage = 1; // 1 means, all incoming tuples are inserted to h1
                   // stage 2 -> filter writing

        TupleFile* currentFile = 0;
        Tuple* lastWritten = 0;
        size_t count = 0;
        stream.open();
        h1->startBulkload();

        while( (nextTuple = stream.request()) ){
           count++;
           if(stage == 1){
               size_t tm = nextTuple->GetMemSize() + sizeof(void*);
               if( h1Mem + tm <= maxMem ){
                  h1->insert(nextTuple);
                  h1Mem += tm;
               } else {
                  h1->insert(nextTuple);
                  h1->endBulkload();
                  nextTuple = *(h1->min());
                  h1->deleteMin();
                  currentFile = new TupleFile(tt,0);
                  files.push_back(currentFile);
                  currentFile->Append(nextTuple);
                  if(lastWritten){
                      lastWritten->DeleteIfAllowed();
                  }
                  lastWritten = nextTuple; 
                  std::swap(h1,h2); 
                  h1->startBulkload();        
                  stage = 2;
               }
           } else {
              if(Comp(nextTuple, lastWritten)){
                // next tuple is smaller, store for next stage
                h1->insert(nextTuple);
              } else {
                // just filter trough heap
                h2->insert(nextTuple);
              }
              if(h2->empty()){
                 h1->endBulkload();
                 std::swap(h1,h2);
                 h1->startBulkload();
                 currentFile->Close();
                 currentFile = new TupleFile(tt,0);
                 files.push_back(currentFile);
              } 
              Tuple* min = *(h2->min());
              h2->deleteMin();
              currentFile->Append(min);
                  if(lastWritten){
                      lastWritten->DeleteIfAllowed();
                  }
              lastWritten = min;
           }
        }
        h1->endBulkload();
        if(lastWritten){
            lastWritten->DeleteIfAllowed();
        }
        if(currentFile){
            currentFile->Close();
        }
        stream.close();
     }

     void mergeFiles(){
       std::vector<TupleFile*> current;
       size_t reqFiles = (files.size() - maxFiles) + 1;
       size_t numFiles = std::min(maxFiles,reqFiles);
       for(size_t i=0;i<numFiles;i++){
          current.push_back(files[0]);
          files.erase(files.begin());
       }
       TupleFile* out = new TupleFile(tt,0);
       files.push_back(out);
       pc cmp(Comp); 
       OutHeap oh(current,0,Comp);
       Tuple* nt = 0;
       while( (nt = oh.next()) ){
          out->Append(nt);
          nt->DeleteIfAllowed();
       }
       for(size_t i = 0; i< current.size();i++){
          current[i]->Close();
          delete current[i];
       }
       

     }
};


int Sort2HeapsVM(Word* args, Word& result, int message,
          Word& local, Supplier s){

  Sort2HeapsInfo* li = (Sort2HeapsInfo*) local.addr;

  switch(message){
    case OPEN : {
        if(li){ delete li;};
        std::vector<std::pair<int, bool> > pos;
        Supplier sp = qp->GetSon(s,qp->GetNoSons(s)-2); // positions
        Supplier sd = qp->GetSon(s,qp->GetNoSons(s)-1); // directions
        assert( qp->GetNoSons(sp) == qp->GetNoSons(sd)); // ensured by tm
        for(int i=0; i<qp->GetNoSons(sd); i++){
           Word w;
           qp->Request(qp->GetSon(sp,i),w);
           int p = ( (CcInt*) w.addr)->GetValue();
           qp->Request(qp->GetSon(sd,i),w);
           int d = ( (CcBool*) w.addr)->GetValue();
           pos.push_back( std::make_pair(p,d));
        }
        size_t maxFiles = 200;  // maximum number of open files 
        size_t maxMem = qp->GetMemorySize(s)*1024*1024;
        // security factor
        maxMem = (size_t) (0.8*maxMem);
        if(maxMem<1024){
          maxMem = 1024;
        }
        local.addr = new Sort2HeapsInfo(args[0], pos,
                      nl->Second(GetTupleResultType(s)),
                      maxFiles, maxMem);
        return 0;
    }
    case REQUEST: result.addr = li?li->next():0;
                  return result.addr?YIELD:CANCEL;
    case CLOSE: 
         if(li){
            delete li;
            local.addr =0;
         }
         return 0;
  }
  return -1;
}

