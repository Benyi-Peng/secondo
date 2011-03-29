/*
----
This file is part of SECONDO.

Copyright (C) 2008, University in Hagen,
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

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//paragraph [10] Footnote: [{\footnote{] [}}]
//[ue] [\"u]
//[ae] [\"a]
//[_] [\_]
//[TOC] [\tableofcontents]

[1] ImExAlgebra

This algebra provides import export functions for different
data formats.


*/

/*

[TOC]

1 Overview

This file contains the implementation import / export operators.

2 Defines, includes, and constants

*/


#include <cmath>
#include <stack>
#include <limits>
#include <sstream>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include <stdio.h>

#include "NestedList.h"
#include "QueryProcessor.h"
#include "AlgebraManager.h"
#include "Algebra.h"
#include "StandardTypes.h"
#include "RelationAlgebra.h"
#include "SecondoSystem.h"
#include "FTextAlgebra.h"
#include "SpatialAlgebra.h"
#include "DateTime.h"
#include "TopOpsAlgebra.h"
#include "../../Tools/Flob/DbArray.h"
#include "Symbols.h"
#include "FileSystem.h"
#include "ListUtils.h"

#include "version.h"
#include "DbVersion.h"
#include "RegionTools.h"
#include "NMEAImporter.h"


extern NestedList* nl;
extern QueryProcessor* qp;
extern AlgebraManager* am;


/*
1 Operator ~csvexport~

1.1 Type Mapping

   stream(CSVEXPORTABLE) x text x bool -> stream(CSVEXPORTABLE)
   stream ( tuple ( (a1 t1) ... (an tn))) x string x bool x bool -> stream (tuple(...))
   stream ( tuple ( (a1 t1) ... (an tn))) x string x bool x bool x string -> stream (tuple(...))

*/

ListExpr csvexportTM(ListExpr args){
  int len = nl->ListLength(args);
  if(len != 3 && len != 4 && len!=5){
    ErrorReporter::ReportError("wrong number of arguments");
    return nl->TypeError();
  }
  ListExpr errorInfo = nl->OneElemList(nl->SymbolAtom("ERROR"));
  if(len == 3){ // stream(CSV) x string x bool
    if(!nl->IsEqual(nl->Second(args),symbols::TEXT) ||
       !nl->IsEqual(nl->Third(args),symbols::BOOL)){
       ErrorReporter::ReportError("stream x text x bool expected");
       return nl->TypeError();
    }
    ListExpr stream = nl->First(args);
    if(nl->ListLength(stream)!=2){
       ErrorReporter::ReportError("stream x text x bool expected");
       return nl->TypeError();
    }
    if(!nl->IsEqual(nl->First(stream),"stream")){
       ErrorReporter::ReportError("stream x text x bool expected");
       return nl->TypeError();
    }
    if(!am->CheckKind("CSVEXPORTABLE",nl->Second(stream),errorInfo)){
       if(listutils::isTupleStream(stream)){
          return listutils::typeError("for processing a tuple stream one  "
                                     "additional parameter is required"
                                     " at least");

       } else {
         return listutils::typeError("stream element not in "
                                     "kind csvexportable");
       }
    }
    return stream;
  } else { // stream(tuple(...) )
    if( !nl->IsEqual(nl->Second(args),symbols::TEXT) ||
        !nl->IsEqual(nl->Third(args),symbols::BOOL) ||
        !nl->IsEqual(nl->Fourth(args),symbols::BOOL)  ){
       ErrorReporter::ReportError("stream x text x bool x"
                                  " bool [x string] expected");
       return nl->TypeError();
    }
    if(len==5 && !nl->IsEqual(nl->Fifth(args),symbols::STRING)){
       ErrorReporter::ReportError("stream x text x bool x "
                                  "bool [x string] expected");
       return nl->TypeError();
    }
    ListExpr stream = nl->First(args);
    if(nl->ListLength(stream)!=2 ||
       !nl->IsEqual(nl->First(stream),symbols::STREAM)){
       ErrorReporter::ReportError("stream x text x bool [ x bool] expected");
       return nl->TypeError();
    }
    ListExpr tuple = nl->Second(stream);
    if(nl->ListLength(tuple)!=2 ||
       !nl->IsEqual(nl->First(tuple),"tuple")){
       ErrorReporter::ReportError("stream x text x bool [ x bool] expected");
       return nl->TypeError();
    }
    ListExpr attrList = nl->Second(tuple);
    if(nl->ListLength(attrList) < 1 ){
       ErrorReporter::ReportError("stream x text x bool [ x bool] expected");
       return nl->TypeError();
    }
    // check attrList
    while(!nl->IsEmpty(attrList)){
      ListExpr attr = nl->First(attrList);
      attrList = nl->Rest(attrList);
      if(nl->ListLength(attr)!=2){
         ErrorReporter::ReportError("invalid tuple stream");
         return nl->TypeError();
      }
      ListExpr aName = nl->First(attr);
      ListExpr atype = nl->Second(attr);
      if(nl->AtomType(aName)!=SymbolType){
         ErrorReporter::ReportError("invalid tuple stream");
         return nl->TypeError();
      }
      if(!am->CheckKind("CSVEXPORTABLE", atype,errorInfo)){
         ErrorReporter::ReportError("invalid kind in tuple");
         return nl->TypeError();
      }
    }
    return stream;
  }
}

/*
1.2 Value Mappings for csvexport

*/

class CsvExportLocalInfo {

public:

   CsvExportLocalInfo(string fname, bool append){
     if(append){
       f.open(fname.c_str(), ios::out | ios::app);
     } else {
       f.open(fname.c_str(),ios::out | ios::trunc);
     }
   }

   CsvExportLocalInfo(string fname, bool append,bool names,
                      const string& sep,
                      ListExpr type){
     this->sep = sep;
     if(append){
       f.open(fname.c_str(), ios::out | ios::app);
     } else {
       f.open(fname.c_str(),ios::out | ios::trunc);
     }
     if(names){
        ListExpr rest = nl->Second(nl->Second(type));
        ListExpr first = nl->First(rest);
        rest = nl->Rest(rest);
        f << nl->SymbolValue(nl->First(first));
        while(!nl->IsEmpty(rest)){
           f << ",";
           first = nl->First(rest);
           rest = nl->Rest(rest);
           f << nl->SymbolValue(nl->First(first));
        }
        f << endl;
     }
   }

   ~CsvExportLocalInfo(){
     f.close();
   }

   bool isOk(){
      return f.good();
   }

   void write(Attribute* attr){
     f << attr->getCsvStr() << endl;
   }

   void write(Tuple* tuple){
     int s = tuple->GetNoAttributes();
     for(int i=0;i<s;i++){
        if(i>0){
          f << sep;
        }
        f << tuple->GetAttribute(i)->getCsvStr();
     }
     f << endl;
   }

private:
  fstream f;
  string sep;
};

int CsvExportVM(Word* args, Word& result,
                           int message, Word& local, Supplier s)
{
  switch( message )
  {
    case OPEN:{
      qp->Open(args[0].addr);
      FText* fname = static_cast<FText*>(args[1].addr);
      CcBool* append = static_cast<CcBool*>(args[2].addr);
      if(!fname->IsDefined() || !append->IsDefined()){
         local.setAddr(0);
      } else {
         CsvExportLocalInfo* linfo;
         linfo = (new CsvExportLocalInfo(fname->GetValue(),
                                         append->GetBoolval()));
         if(!linfo->isOk()){
            delete linfo;
            local.setAddr(0);
         } else {
             local.setAddr(linfo);
         }
      }
      return 0;
    }

    case REQUEST:{
       if(local.addr==0){
         return CANCEL;
       } else {
         Word elem;
         qp->Request(args[0].addr,elem);
         if(qp->Received(args[0].addr)){
           CsvExportLocalInfo* linfo;
           linfo  = static_cast<CsvExportLocalInfo*>(local.addr);
           linfo->write( static_cast<Attribute*>( elem.addr));
           result = elem;
           return YIELD;
         } else {
           return CANCEL;
         }
       }
    }

    case CLOSE:{
      qp->Close(args[0].addr);
      if(local.addr){
        CsvExportLocalInfo* linfo;
        linfo  = static_cast<CsvExportLocalInfo*>(local.addr);
        delete linfo;
        local.addr = 0;
      }
      return 0;
    }
  }
  return 0;
}


int CsvExportVM2(Word* args, Word& result,
                           int message, Word& local, Supplier s)
{
  switch( message )
  {
    case OPEN:{
      qp->Open(args[0].addr);
      FText* fname = static_cast<FText*>(args[1].addr);
      CcBool* append  = static_cast<CcBool*>(args[2].addr);
      CcBool* names   = static_cast<CcBool*>(args[3].addr);
      string sep = ",";
      if(qp->GetNoSons(s)==5){
        CcString* ccSep = static_cast<CcString*>(args[4].addr);
        sep =  ccSep->GetValue();
        if(sep.length()==0){
           sep =",";
        }
      }
      if(!fname->IsDefined() || !append->IsDefined() || !names->IsDefined()){
         local.setAddr(0);
      } else {
         CsvExportLocalInfo* linfo;
         linfo = (new CsvExportLocalInfo(fname->GetValue(),
                                         append->GetBoolval(),
                                         names->GetBoolval(),
                                         sep,
                                         qp->GetType(s)));
         if(!linfo->isOk()){
            delete linfo;
            local.setAddr(0);
         } else {
             local.setAddr(linfo);
         }
      }
      return 0;
    }

    case REQUEST:{
       if(local.addr==0){
         return CANCEL;
       } else {
         Word elem;
         qp->Request(args[0].addr,elem);
         if(qp->Received(args[0].addr)){
           CsvExportLocalInfo* linfo;
           linfo  = static_cast<CsvExportLocalInfo*>(local.addr);
           linfo->write( static_cast<Tuple*>( elem.addr));
           result = elem;
           return YIELD;
         } else {
           return CANCEL;
         }
       }
    }

    case CLOSE:{
      qp->Close(args[0].addr);
      if(local.addr){
        CsvExportLocalInfo* linfo;
        linfo  = static_cast<CsvExportLocalInfo*>(local.addr);
        delete linfo;
        local.addr = 0;
      }
      return 0;
    }
  }
  return 0;
}

ValueMapping csvexportmap[] =
{  CsvExportVM, CsvExportVM2 };

int csvExportSelect( ListExpr args )
{
  if(nl->ListLength(args)==3){
   return 0;
  } else {
   return 1;
  }
}

/*
1.3 Specification

*/


const string CsvExportSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>stream(X) x  text x bool -> stream(X)\n"
    "stream(tuple(...))) x text x bool x bool "
    "[x string]-> stream(tuple...)</text--->"
    "<text> stream csvexport [ file , append, "
    "[writenames, [separator]]]</text--->"
    "<text> Exports stream content to a csv file </text--->"
    "<text>query ten feed exportcsv['ten.csv', "
    "FALSE,TRUE,\";\"] count</text--->"
    ") )";

/*
1.4 Operator instance

*/

Operator csvexport( "csvexport",
                     CsvExportSpec,
                     2,
                     csvexportmap,
                     csvExportSelect,
                     csvexportTM);



/*
1 CSV Import

1.1 Type Mapping

  rel(tuple(a[_]1 : t[_]1)...(a[_]n : t[_]n)) x text x int x string -> stream(tuple(...))

  where t[_]i is CSVIMPORTABLE.

  text   :  indicates the filename containing the csv data
  int    :  number of lines to ignore (header)
  string :  ignore Lines starting with this string (comment)
            use an empty string for disable this feature

*/

ListExpr csvimportTM(ListExpr args){
  string err = " rel(tuple(a_1 : t_1)...(a_n : t_n)) x "
               "text x int x string [x string] expected";
  int len = nl->ListLength(args);
  if((len!=4) && (len!=5)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  if(!IsRelDescription(nl->First(args)) ||
     !nl->IsEqual(nl->Second(args),symbols::TEXT) ||
     !nl->IsEqual(nl->Third(args),symbols::INT) ||
     !nl->IsEqual(nl->Fourth(args),symbols::STRING)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  if( (len==5) && !nl->IsEqual(nl->Fifth(args),symbols::STRING)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  // check attribute types for csv importable
  ListExpr errorInfo = nl->OneElemList(nl->SymbolAtom("ERROR"));

  ListExpr attrlist = nl->Second(nl->Second(nl->First(args)));
  if(nl->IsEmpty(attrlist)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  while(!nl->IsEmpty(attrlist)){
     ListExpr type = nl->Second(nl->First(attrlist));
     if(!am->CheckKind("CSVIMPORTABLE",type,errorInfo)){
        ErrorReporter::ReportError("attribute type not in kind CSVEXPORTABLE");
        return nl->TypeError();
     }
     attrlist = nl->Rest(attrlist);
  }
  return nl->TwoElemList( nl->SymbolAtom(symbols::STREAM),
                          nl->Second(nl->First(args)));
}


/*
1.2 Specification


*/


const string csvimportSpec  =
   "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
   "( <text> rel(tuple(...) x text x int x string [x string] "
   "-> stream (tuple(...))</text--->"
   "<text> rel csvimport[filename, headersize, comment, separator] </text--->"
   "<text> Returns the content of the CSV (Comma Separated Value) 'filename' "
   "as a stream of tuples. The types are defined by the first argument, 'rel', "
   "whose tuple type acts as a template for the result tuple type. 'headersize'"
   " specifies an amount of lines to ignore at the head of the imported "
   "textfile. 'comment' defines a character that marks comment lines within "
   "the textfile. 'separator' defines the character that is expected to "
   "separate the fields within each line of the textfile (default is \",\"."
   "</text--->"
   "<text> not tested !!!</text--->"
   ") )";
/*
1.3 Value Mapping for csvimport

*/
class CsvImportInfo{
public:
  CsvImportInfo(ListExpr type, FText* filename,
                CcInt* hSize , CcString* comment,
                const string& separator){

      this->separator = separator;
      BasicTuple = 0;
      tupleType = 0;
      defined = filename->IsDefined() && hSize->IsDefined() &&
                comment->IsDefined();
      if(!defined){
          return;
      }
      string name = filename->GetValue();
      file.open(name.c_str());
      if(!file.good()){
         defined = false;
         return;
      }
      // skip header
      int skip = hSize->GetIntval();
      string buffer;
      for(int i=0;i<skip && file.good(); i++){
         getline(file,buffer);
      }
      if(!file.good()){
        defined=false;
        return;
      }
      this->comment = comment->GetValue();
      useComment = this->comment.size()>0;
      ListExpr numType = nl->Second(
                         SecondoSystem::GetCatalog()->NumericType((type)));
      tupleType = new TupleType(numType);
      BasicTuple = new Tuple(tupleType);
      // build instances for each type
      ListExpr attrList = nl->Second(nl->Second(type));
      while(!nl->IsEmpty(attrList)){
         ListExpr attrType = nl->Second(nl->First(attrList));
         attrList = nl->Rest(attrList);
         int algId;
         int typeId;
         string tname;
         if(! ((SecondoSystem::GetCatalog())->LookUpTypeExpr(attrType,
                                               tname, algId, typeId))){
           defined = false;
           return;
         }
         Word w = am->CreateObj(algId,typeId)(attrType);
         instances.push_back(static_cast<Attribute*>(w.addr));
      }
  }



  ~CsvImportInfo(){
     if(BasicTuple){
       delete BasicTuple;
       BasicTuple = 0;
     }
     if(tupleType){
        tupleType->DeleteIfAllowed();
        tupleType = 0;
     }
     for(unsigned int i=0; i<instances.size();i++){
        delete instances[i];
     }
     instances.clear();
     file.close();
   }


   bool isComment(string line){
      if(!useComment){
        return false;
      } else {
        return line.find(comment)==0;
      }
   }

   Tuple* getNext(){
     if(!defined){
        return 0;
     }
     if(!file.good() || file.eof()){
        return 0;
     }
     string buf;
     do{
       getline(file,buf);
     } while(file.good() && !file.eof() && isComment(buf));
     if(file.good()){
        return createTuple(buf);
     }
     return 0;
   }


private:
  bool defined;
  ifstream file;
  bool useComment;
  string comment;
  TupleType* tupleType;
  Tuple* BasicTuple;
  vector<Attribute*> instances;
  string separator;
  Tuple* createTuple(string line){
      Tuple* result = BasicTuple->Clone();
      string::size_type lastPos = 0;
      bool end = false;
      for(unsigned int i=0;i<instances.size();i++){
         Attribute* attr = instances[i]->Clone();
         string::size_type pos  = line.find_first_of(separator, lastPos);
         if( (pos!=string::npos)  || ( (lastPos != string::npos) && !end)){
            if(pos==string::npos){
               pos = line.size();
               end=true;
            }
            string attrstr = line.substr(lastPos,pos -lastPos);
            attr->ReadFromString(attrstr);
            result->PutAttribute(i,attr);
            lastPos = pos+separator.length();
            pos = line.find_first_of(separator, lastPos);
         } else {
            attr->SetDefined(false);
            result->PutAttribute(i,attr);
         }
      }
      return result;
  }
};


int csvimportVM(Word* args, Word& result,
               int message, Word& local, Supplier s){

  switch(message){
    case OPEN: {
      ListExpr type = qp->GetType(qp->GetSon(s,0));
      int sons = qp->GetNoSons(s);
      FText* fname = static_cast<FText*>(args[1].addr);
      CcInt* skip = static_cast<CcInt*>(args[2].addr);
      CcString* comment = static_cast<CcString*>(args[3].addr);
      string separator = ",";
      if(sons==5){
        CcString* cCSep = static_cast<CcString*>(args[4].addr);
        separator = cCSep->GetValue();
        if(separator.length()==0){
          separator=",";
        }
      }
      local.setAddr(new CsvImportInfo(type,fname,skip,comment,separator));
      return 0;
    }
    case REQUEST: {
      CsvImportInfo* info = static_cast<CsvImportInfo*>(local.addr);
      if(!info){
        return CANCEL;
      } else {
        Tuple* tuple = info->getNext();
        result.addr = tuple;
        return tuple==0?CANCEL:YIELD;
      }
    }
    case CLOSE: {
      CsvImportInfo* info = static_cast<CsvImportInfo*>(local.addr);
      if(info){
        delete info;
        local.addr=0;
      }

    }
    default: {
     return 0;
    }
  }
}


/*
1.4 Operator Instance

*/
Operator csvimport( "csvimport",
                    csvimportSpec,
                    csvimportVM,
                    Operator::SimpleSelect,
                    csvimportTM);




/*
2 Operator nmeaImport


2.1 Type Mapping for nmeaimport


The signature is text x string -> stream(tuple(...))

The tuple type depends on the value of the second argument.
Because this operator evaulates the second argument in type mapping,
the types are extended to be pairs of (type, list), where list
is the representation of the value within the query.



*/
ListExpr nmeaimportTM(ListExpr args){
    string err = "text x string expected";
   if(!nl->HasLength(args,2)){
      return listutils::typeError(err);
   }
   ListExpr first = nl->First(args);
   ListExpr second = nl->Second(args);

   if(!nl->HasLength(first,2) || !nl->HasLength(second,2)){
     return listutils::typeError("invalid types for operator"
                                 " evaluating arguments in type mapping"); 
   }
   ListExpr first1 = nl->First(first);
   if(!listutils::isSymbol(first1,FText::BasicType())){
      return listutils::typeError(err);
   }
   ListExpr second1 = nl->First(second);
   if(!listutils::isSymbol(second1,CcString::BasicType())){
      return listutils::typeError(err);
   }
   // Evaulate the String value
   ListExpr val = nl->Second(second);

   Word res;
   bool success = QueryProcessor::ExecuteQuery(nl->ToString(val),res);
   if(!success){
     return listutils::typeError("could not evaluate the value of string");
   }

   CcString* value = static_cast<CcString*>(res.addr);

   if(!value->IsDefined()) {
      value->DeleteIfAllowed();
      return listutils::typeError("string argument evaluated to be undefined"); 
   }
   string v = value->GetValue();
   value->DeleteIfAllowed();

   NMEAImporter imp;
   if(!imp.setType(v)){
     return listutils::typeError(v + " is not a known nmea type id, known are "
                                 + imp.getKnownTypes());
   } 
   return nl->TwoElemList(nl->SymbolAtom("stream"), imp.getTupleType());

 
}




/*
2.2 Value Mapping for nmeaimport

*/
int nmeaimportVM(Word* args, Word& result,
               int message, Word& local, Supplier s){

  switch(message){
    case OPEN:{
      if(local.addr){
        delete (NMEAImporter*)local.addr;
        local.addr=0;
      }
      FText* fn = static_cast<FText*>(args[0].addr);
      CcString* t = static_cast<CcString*>(args[1].addr);
      if(!fn->IsDefined() || !t->IsDefined()){
         return 0;
      } 
      NMEAImporter* imp = new NMEAImporter();
      if(!imp->setType(t->GetValue())){
        delete imp;
        return 0;
      }
      string err;
      if(!imp->scanFile(fn->GetValue(),err)){
        delete imp;
        return 0;
      }
      local.addr = imp; 
      return 0;
    }

   case REQUEST:{
      if(!local.addr){
         return CANCEL;
       }
       NMEAImporter* imp = static_cast<NMEAImporter*>(local.addr);
       Tuple* t = imp->nextTuple();
       result.addr=t;
       return t?YIELD:CANCEL;
    }

    case CLOSE:{
      if(local.addr){
          NMEAImporter* imp = static_cast<NMEAImporter*>(local.addr);
          delete imp;
          local.addr=0;
      }
      return 0;
    }

    default: assert(false);

  }

  return -1;
}



/*
2.3 Specification for nmeaimport

*/

const string nmeaimportSpec  =
   "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
   "( <text> text x string -> rel(tuple(...)) </text--->"
   "<text> nmeaimport(filename, type) </text--->"
   "<text> Returns the content of all lines matching type "
   "as a stream of typles."
   "The result type depenend of the value of typeid"
   "</text--->"
   "<text> nmeaimport('trip.nmea',\"GGA\")</text--->"
   ") )";

/*
2.4 Operator instance for nmeaimport

*/

Operator nmeaimport( "nmeaimport",
                    nmeaimportSpec,
                    nmeaimportVM,
                    Operator::SimpleSelect,
                    nmeaimportTM);






/*
2 Operator shpExport

2.1 Type Mapping

*/

ListExpr shpexportTM(ListExpr args){
  int len = nl->ListLength(args);
  if(len!=2 && len != 3){
    ErrorReporter::ReportError("wrong number of arguments");
    return nl->TypeError();
  }
  string err = "   stream(SHPEXPORTABLE) x text \n "
               " or stream(tuple(...)) x text x attrname expected";
  if(!nl->IsEqual(nl->Second(args),"text")){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  if(len==2){
    ListExpr stream = nl->First(args);
    if(nl->ListLength(stream) != 2 ||
      !nl->IsEqual(nl->First(stream),"stream")){
      ErrorReporter::ReportError(err);
      return nl->TypeError();
    }
    ListExpr errorInfo = nl->OneElemList(nl->SymbolAtom("ERROR"));
    if(!am->CheckKind("SHPEXPORTABLE",nl->Second(stream), errorInfo)){
      ErrorReporter::ReportError(err);
      return nl->TypeError();
    }
    return stream;
  } else { // len = 3
    ListExpr stream = nl->First(args);
    if(! IsStreamDescription(stream)){
      ErrorReporter::ReportError(err);
      return nl->TypeError();
    }
    ListExpr attrName = nl->Third(args);
    if(nl->AtomType(attrName)!=SymbolType){
      ErrorReporter::ReportError("Invalid value for an attribute name");
      return nl->TypeError();
    }
    string an = nl->SymbolValue(attrName);
    ListExpr attrList = nl->Second(nl->Second(stream));
    int index = -1;
    int pos = 0;
    ListExpr attr;
    while(!nl->IsEmpty(attrList) && index < 0 ){
       attr = nl->First(attrList);
       attrList = nl->Rest(attrList);
       if(nl->IsEqual(nl->First(attr),an)){
         index = pos;
       } else {
         pos ++;
       }
    }
    if(index < 0){
       ErrorReporter::ReportError("Attribute name " + an +
                                 " not present in tuple");
       return nl->TypeError();
    }
    // check whether the type is in kind
    ListExpr errorInfo = nl->OneElemList(nl->SymbolAtom("ERROR"));
    if(!am->CheckKind("SHPEXPORTABLE",nl->Second(attr), errorInfo)){
      ErrorReporter::ReportError(err);
      return nl->TypeError();
    }
    // all ok, append the index to the result
    return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
                             nl->OneElemList(nl->IntAtom(index)),
                             stream);
  }
}

/*
2.2 Value Mapping

*/

class shpLInfo{
 public:
    shpLInfo(FText* name){
      if(!name->IsDefined()){
         defined = false;
      } else {
         defined = true;
         first = true;
         baseName = (name->GetValue());
         recno = 1;
         boxdef = false;
         xMin = 0;
         xMax = 0;
         yMin = 0;
         yMax = 0;
         zMin = 0;
         zMax = 0;
         mMin = 0;
         mMax = 0;
      }
    }

    bool writeHeader(uint32_t type){
       file.open((baseName ).c_str(),
                  ios::out | ios::trunc | ios_base::binary);
       if(!file.good()){
         return false;
       }
       uint32_t code = 9994;
       WinUnix::writeBigEndian(file,code);
       uint32_t unused = 0;
       for(int i=0;i<5;i++){
         WinUnix::writeBigEndian(file,unused);
       }
       // dummy for file length
       uint32_t length = 100;
       WinUnix::writeBigEndian(file,length);
       uint32_t version = 1000;
       WinUnix::writeLittleEndian(file,version);
       WinUnix::writeLittleEndian(file,type);
       double boxpos = 0.0;
       for(int i=0;i<8;i++){
         WinUnix::writeLittle64(file,boxpos);
       }
       return file.good();
    }

    bool write(Attribute* value) {
       if(!file.good() || ! defined ){
         return false;
       }
       if(first){
          if(!writeHeader(value->getshpType())){
             return false;
          }
          first = false;
       }
       value->writeShape(file,recno++);
       if(value->hasBox()){
          if(!boxdef){
             xMin = value->getMinX();
             xMax = value->getMaxX();
             yMin = value->getMinY();
             yMax = value->getMaxY();
             zMin = value->getMinZ();
             zMax = value->getMaxZ();
             mMin = value->getMinM();
             mMax = value->getMaxM();
             boxdef=true;
          } else {
             xMin = min(xMin,value->getMinX());
             xMax = max(xMax,value->getMaxX());
             yMin = min(yMin,value->getMinY());
             yMax = max(yMax,value->getMaxY());
             zMin = min(zMin,value->getMinZ());
             zMax = max(zMax,value->getMaxZ());
             mMin = min(mMin,value->getMinM());
             mMax = max(mMax,value->getMaxM());
          }
       }
       return file.good();
    }



    void close(){
       // write correct file-length into file
       // write correct bounding box into file
       uint32_t len = file.tellp();
       file.seekp(24,ios_base::beg);
       WinUnix::writeBigEndian(file,len);
       file.seekp(36,ios_base::beg);
       WinUnix::writeLittle64(file,xMin);
       WinUnix::writeLittle64(file,yMin);
       WinUnix::writeLittle64(file,xMax);
       WinUnix::writeLittle64(file,yMax);
       WinUnix::writeLittle64(file,zMin);
       WinUnix::writeLittle64(file,zMax);
       WinUnix::writeLittle64(file,mMin);
       WinUnix::writeLittle64(file,mMax);

       file.close();

    }

    int getIndex(){ return index; }
    void setIndex(int index) {this->index = index;}

 private:
    string baseName;
    bool first;
    bool defined;
    ofstream file;
    int recno;
    double xMin;
    double xMax;
    double yMin;
    double yMax;
    double zMin;
    double zMax;
    double mMin;
    double mMax;
    bool boxdef;
    int index;

};

int shpexportVM1(Word* args, Word& result,
                           int message, Word& local, Supplier s)
{
  switch( message )
  {
    case OPEN:{
       qp->Open(args[0].addr);
       FText* fname = static_cast<FText*>(args[1].addr);
       local.setAddr(new shpLInfo(fname));
       return 0;
    }
    case REQUEST: {
      shpLInfo* linfo= static_cast<shpLInfo*>(local.addr);
      if(!linfo){
         return CANCEL;
      }
      Word elem;
      qp->Request(args[0].addr, elem);
      if(qp->Received(args[0].addr)){
        bool ok;
        ok = linfo->write( static_cast<Attribute*>(elem.addr));
        ok = true;
        if(ok){
          result = elem;
          return YIELD;
        } else {
          Attribute* del = static_cast<Attribute*>(elem.addr);
          del->DeleteIfAllowed();
          result.setAddr(0);
          return CANCEL;
        }
      } else {
         result.setAddr(0);
         return CANCEL;
      }
    }
    case CLOSE:{
      qp->Close(args[0].addr);
      if(local.addr){
        shpLInfo* linfo = static_cast<shpLInfo*>(local.addr);
        linfo->close();
        delete linfo;
        local.addr=0;
      }
      return 0;
    }
    default: return 0;
  }
}

int shpexportVM2(Word* args, Word& result,
                           int message, Word& local, Supplier s)
{
  switch( message )
  {
    case OPEN:{
       qp->Open(args[0].addr);
       FText* fname = static_cast<FText*>(args[1].addr);
       shpLInfo* linfo  = new shpLInfo(fname);
       int attrPos = (static_cast<CcInt*>(args[3].addr))->GetIntval();
       linfo->setIndex( attrPos);
       local.setAddr(linfo);
       return 0;
    }
    case REQUEST: {
      shpLInfo* linfo= static_cast<shpLInfo*>(local.addr);
      if(!linfo){
         return CANCEL;
      }
      Word elem;
      qp->Request(args[0].addr, elem);
      if(qp->Received(args[0].addr)){
        Tuple* tuple = static_cast<Tuple*>(elem.addr);
        Attribute* attr;
        int attrPos = linfo->getIndex();
        attr  = tuple->GetAttribute(attrPos);
        linfo->write(attr);
        result = elem;
        return YIELD;
      } else {
         result.setAddr(0);
         return CANCEL;
      }
    }
    case CLOSE:{
      qp->Close(args[0].addr);
      if(local.addr){
        shpLInfo* linfo = static_cast<shpLInfo*>(local.addr);
        linfo->close();
        delete linfo;
        local.addr=0;
      }
      return 0;
    }
    default: return 0;
  }
}

/*
2.3 Value Mapping Array

*/

ValueMapping shpexportmap[] =
{  shpexportVM1, shpexportVM2};

/*
2.4 Selection Function

*/

int shpexportSelect( ListExpr args )
{ if(nl->ListLength(args)==2){
    return 0;
  } else {
    return 1;
  }
}

/*
2.5 Specification

*/
const string shpexportSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>stream(X) x  string -> stream(X)\n"
    "stream(tuple(...))) x string x bool x bool -> stream(tuple...)</text--->"
    "<text> stream shpexport [ file ]</text--->"
    "<text> Exports stream content to a shp file </text--->"
    "<text> not tested !!!</text--->"
    ") )";

/*
2.6 Operator Instance

*/

Operator shpexport( "shpexport",
                     shpexportSpec,
                     2,
                     shpexportmap,
                     shpexportSelect,
                     shpexportTM);


/*
3 DB3-export

3.1 Type Mapping

  stream(tuple ( ... )) x string -> stream(tuple(...))

*/



ListExpr db3exportTM(ListExpr args){
  string err =   "stream(tuple ( ... )) x text  expected";
  if(nl->ListLength(args)!=2){
     ErrorReporter::ReportError(err);
     return nl->TypeError();
  }
  if(!IsStreamDescription(nl->First(args)) ||
     !nl->IsEqual(nl->Second(args),"text")){
     ErrorReporter::ReportError(err);
     return nl->TypeError();
  }
  return nl->First(args);
}

/*
3.2 Value Mapping

*/

class Db3LInfo{
public:
   Db3LInfo(FText* fname, ListExpr type){
     if(!fname->IsDefined()){ // no correct filename
        defined=false;
     }  else {
        if(!f.good()){
          defined = false;
        } else {
          this->fname = (fname->GetValue());
          first = true;
          firstMemo=true;
          defined = true;
          memNumber = 0;
          recNumber = 0;
          ListExpr rest = nl->Second(nl->Second(type));
          while(!nl->IsEmpty(rest)){
            string  name = nl->SymbolValue(nl->First(nl->First(rest)));
            rest = nl->Rest(rest);
            name = name.substr(0,10);// allow exactly 11 chars
            names.push_back(name);
          }
        }
     }
   }

   void writeHeader(Tuple* tuple){
      bool hasMemo = false;
      uint16_t recSize = 1; // deleted marker
      int exportable = 0;
      for(int i=0;i<tuple->GetNoAttributes();i++){
        Attribute* attr = tuple->GetAttribute(i);
        if(attr->hasDB3Representation()){
          exportable++;
          bool ismemo = false;
          if(attr->getDB3Type() == 'M'){
            hasMemo = true;
            ismemo = true;
          }
          unsigned char len = attr->getDB3Length();
          if(ismemo){
            len = 10;
          }
          recSize += len;
          exps.push_back(true);
          lengths.push_back(len);
          dbtypes.push_back(attr->getDB3Type());
          decCnts.push_back(attr->getDB3DecimalCount());
          isMemo.push_back(ismemo);
        } else {
          exps.push_back(false);
          lengths.push_back(0);
          dbtypes.push_back('L');
          decCnts.push_back(0);
          isMemo.push_back(false);
        }
      }
      if(exportable == 0){ // no exportable attributes found
          defined = false;
          return;
      }
      // open the file
      f.open((this->fname).c_str(),
              ios::out | ios::trunc | ios::binary);

      if(!f.good()){
          defined = false;
          return;
      }

      unsigned char code = hasMemo?0x83:0x03;
      WinUnix::writeLittleEndian(f,code);
      // wrint an dummy date
      unsigned char yy = 99;
      unsigned char mm = 01;
      unsigned char dd = 01;
      f << yy << mm << dd;
      uint32_t num = 1;
      WinUnix::writeLittleEndian(f,num); // dummy number of records
      // header size

      uint16_t headersize = 33 + exportable*32;
      WinUnix::writeLittleEndian(f,headersize);

      //  record size
      WinUnix::writeLittleEndian(f,recSize);
      unsigned char reserved=0;
      for(int i=0;i<20;i++){
          WinUnix::writeLittleEndian(f,reserved);
      }
      unsigned char zero = 0;
      for(unsigned int i=0; i< names.size(); i++){
         if(exps[i]){
           // write Field descriptor
           string name = names[i];
           int len = name.length();
           f << name;
           // fill with zeros
           for(int j=len;j<11;j++){
             f << zero;
           }
           // type
           WinUnix::writeLittleEndian(f,dbtypes[i]);
           // address in memory
           uint32_t adr = 0;
           WinUnix::writeLittleEndian(f,adr);
           // len
           WinUnix::writeLittleEndian(f,lengths[i]);
           // decimal count
           WinUnix::writeLittleEndian(f,decCnts[i]);
           for(int j=0;j< 14; j++){
              f << reserved;
           }
         }

      }
      unsigned char term = 0x0D;
      WinUnix::writeLittleEndian(f,term);
      if(!f.good()){
         f.close();
         defined = false;
      }
   }


   void writeMemo(string memo){
     string no ="          "; // 10 spaces
     if(firstMemo){
        fmemo.open((this->fname+".dbt").c_str(),
                   ios::out | ios::trunc | ios::binary);
        memNumber = 1;
        firstMemo = false;
        // write the header
        unsigned char zero = 0;
        unsigned char version = 3;
        for(int  i=0;i<512;i++){
           if(i==16){
              fmemo << version;
           } else {
              fmemo << zero;
           }
        }
     }
     // empty memo or in
     if(!fmemo.good() || memo.length()==0){
       f << no;
       return;
     }
     // write block number to f
     stringstream ss;
     ss << memNumber;
     no = extendString(ss.str(),10,0);
     f << no;
     // write data to fmemo
     fmemo << memo;
     unsigned char term = 0x1A;
     fmemo << term << term;
     unsigned int len = memo.length() + 2 ;
     unsigned int blocks = len / 512;
     unsigned int os = len % 512;
     if(os!=0){
        blocks++;
     }
     unsigned char zero = 0;
     // fill the block with zero
     while(os!=0){
       fmemo << zero;
       os = (os + 1 ) % 512;
     }
     memNumber += blocks;

   }

   void write(Tuple* tuple){
      if(!defined){ // invalid filename input
        return;
      }
      if(first){
         writeHeader(tuple);
         first = false;
      }
      if(!defined){ // no exportable attributes
         return;
      }
      recNumber++;

      f << ' ';  // mark record as non-deleted
      for(unsigned int i=0;i<names.size();i++){
        if(exps[i]){
          unsigned char len = lengths[i];
          string s = (tuple->GetAttribute(i))->getDB3String();
          bool ismemo = isMemo[i];
          if(!ismemo){
            char e = dbtypes[i]=='C'?0:' ';
            s = extendString(s,len,e);
            f << s;
          } else {
            writeMemo(s);
          }
        }
      }
   }

   string extendString(string s, int len,const char c){
      s = s.substr(0,len); // shrink to maximum length
      int e = len-s.length();
      stringstream ss;
      ss << s;
      for(int i=0;i<e; i++){
       ss << c;
      }
      return ss.str();
   }

   void close() {
     if(defined){
       unsigned char eof = 0x1A;
       f << eof;
       f.seekp(4,ios::beg);
       WinUnix::writeLittleEndian(f,recNumber);
       f.seekp(0,ios::end);
       f.close();

       // close memo if used
       if(!firstMemo){
         fmemo.seekp(0,ios::beg);
         WinUnix::writeLittleEndian(fmemo,memNumber);
         fmemo.seekp(0,ios::end);
         fmemo.close();
       }
     }
   }

private:
  fstream f;
  fstream fmemo;
  bool defined;
  bool first;
  bool firstMemo;
  uint32_t memNumber;
  vector<string> names; // names for the attributes
  vector<unsigned char> lengths;  // lengths for the attributes
  vector<bool> exps;     // flags whether the attribute is exportable
  vector<unsigned char> dbtypes;
  vector<unsigned char> decCnts;
  vector<bool> isMemo;
  string fname;

  uint32_t recNumber;        // number of contained records
};


int db3exportVM(Word* args, Word& result,
                int message, Word& local, Supplier s){

  switch( message )
  {
    case OPEN:{
       qp->Open(args[0].addr);
       FText* fname = static_cast<FText*>(args[1].addr);
       Db3LInfo* linfo  = new Db3LInfo(fname,qp->GetType(s));
       local.setAddr(linfo);
       return 0;
    }
    case REQUEST: {
      Db3LInfo* linfo= static_cast<Db3LInfo*>(local.addr);
      if(!linfo){
         return CANCEL;
      }
      Word elem;
      qp->Request(args[0].addr, elem);
      if(qp->Received(args[0].addr)){
        Tuple* tuple = static_cast<Tuple*>(elem.addr);
        linfo->write(tuple);
        result = elem;
        return YIELD;
      } else {
         result.setAddr(0);
         return CANCEL;
      }
    }
    case CLOSE:{
      qp->Close(args[0].addr);
      if(local.addr){
        Db3LInfo* linfo = static_cast<Db3LInfo*>(local.addr);
        linfo->close();
        delete linfo;
        local.addr=0;
      }
      return 0;
    }
    default: return 0;
  }
}

/*
3.3 Specification

*/

const string db3exportSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> stream(tuple(...))) x text -> stream(tuple...)</text--->"
    "<text> stream db3export [ file ]</text--->"
    "<text> Exports stream content to a db3 file </text--->"
    "<text> not tested !!!</text--->"
    ") )";

/*
3.4 Operator Instance

*/
Operator db3export( "db3export",
                     db3exportSpec,
                     db3exportVM,
                     Operator::SimpleSelect,
                     db3exportTM);



/*
4 Operator shptype


4.1 Type Mapping

text -> text

*/

ListExpr shptypeTM(ListExpr args){
  if(nl->ListLength(args)==1 && nl->IsEqual(nl->First(args),"text")){
     return nl->SymbolAtom("text");
  }
  ErrorReporter::ReportError("text expected");
  return nl->TypeError();
}

/*
4.2 Value mapping

*/

/*
This function retrieves the type of a

*/
string getShpType(const string fname, bool& correct, string& errorMessage){
  correct = false;
  if(fname.length()==0){
     errorMessage = "invalid filename";
     return "";
  }

  ifstream f(fname.c_str(),std::ios::binary);
  if(!f.good()){
     errorMessage = "problem in reading file";
     return "";
  }

  f.seekg(0,ios::end);
  streampos flen = f.tellg();
  if(flen < 100){
     errorMessage =  "not a valid shape file";
     f.close();
     return "";
  }


  f.seekg(0,ios::beg);
  uint32_t code = 0;
  f.read(reinterpret_cast<char*>(&code),4);
  if(WinUnix::isLittleEndian()){
     code = WinUnix::convertEndian(code);
  }
  if(code!=9994){
     errorMessage = "invalid file code  detected";
     f.close();
     return "";
  }

  uint32_t version;
  f.seekg(28,ios::beg);
  f.read(reinterpret_cast<char*>(&version),4);
  if(!WinUnix::isLittleEndian()){
      version = WinUnix::convertEndian(version);
  }
  if(version != 1000){
    errorMessage = "invalid version detected";
    f.close();
    return "";
  }
  uint32_t type;
  f.read(reinterpret_cast<char*>(&type),4);
  if(!WinUnix::isLittleEndian()){
    type = WinUnix::convertEndian(type);
  }
  f.close();

  switch(type){
    case 0 : { errorMessage = "null shape, no corresponding secondo type";
               return "";
             }
    case 1 : { correct = true;
               return "point";
             }
    case 3 : { correct = true;
               return "line";
             }
    case 5 : { correct = true;
               return "region";
             }
    case 8 : { correct = true;
               return "points";
             }
    case 11 : { errorMessage = "PointZ, no corresponding secondo type";
               return "";
             }
    case 13 : { errorMessage = ("PolyLineZ, no corresponding secondo type");
               return "";
             }
    case 15 : { errorMessage = ("PolygonZ, no corresponding secondo type");
               return "";
             }
    case 18 : { errorMessage=("MultiPointZ, no corresponding secondo type");
               return "";
             }
    case 21 : { errorMessage=("PointM, no corresponding secondo type");
               return "";
             }
    case 23 : { errorMessage =("PolyLineM, no corresponding secondo type");
               return "";
             }
    case 25 : {errorMessage=("PolygonM, no corresponding secondo type");
               return "";
             }
    case 28 : { errorMessage=("MultiPointM, no corresponding secondo type");
               return "";
             }
    case 31 : { errorMessage = ("MultiPatch, no corresponding secondo type");
               return "";
             }
    default : errorMessage = " not a valid shape type";
              return "";
  }

}




int shptypeVM(Word* args, Word& result,
            int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  FText* res = static_cast<FText*>(result.addr);
  FText* FN = static_cast<FText*>(args[0].addr);
  if(!FN->IsDefined()){
     res->Set(false,"");
     return 0;
  }
  bool correct;
  string shpType;
  string errMsg;
  shpType = getShpType(FN->GetValue(), correct, errMsg);
  if(!correct){
    res->Set(true, errMsg);
    return 0;
  }
  string value = "()";
  if(shpType=="point"){
    value = "(0 0)";
  }
  res->Set(true, "[const "+shpType+" value "+value+"]");
  return 0;
}

/*
4.3 Specification

*/

const string shptypeSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> text -> text</text--->"
    "<text> shptype(filename) </text--->"
    "<text> returns an object description of the secondo object"
    " stored within the shape file specified by the name</text--->"
    "<text> not tested !!!</text--->"
    ") )";



/*
4.4 Operator instance

*/

Operator shptype( "shptype",
                   shptypeSpec,
                   shptypeVM,
                   Operator::SimpleSelect,
                   shptypeTM);



/*
5 Operator shpimport

5.1 Type Mapping

s x text -> stream(s), s in {point, points, line, region}

*/

ListExpr shpimportTM(ListExpr args){
  string err =
  " s x text -> stream(s), s in {point, points, line, region} expected";
  if(nl->ListLength(args)!=2){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  if(!nl->IsEqual(nl->Second(args),"text")){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  ListExpr arg1 = nl->First(args);
  if(nl->AtomType(arg1)!=SymbolType){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  string t1 = nl->SymbolValue(arg1);

  if(t1!="point" && t1!="points" && t1!="line" && t1!="region"){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  return nl->TwoElemList(nl->SymbolAtom("stream"), arg1);
}



/*
5.2 Value mapping

*/

class shpimportInfo{

 public:

   shpimportInfo( const ListExpr allowedType1, const FText* fname){
      int allowedType = -1;
      if(listutils::isSymbol(allowedType1,"point")){
           allowedType=1;
       } else  if(listutils::isSymbol(allowedType1,"line")){
           allowedType=3;
       } else  if(listutils::isSymbol(allowedType1,"region")){
           allowedType=5;
       } else  if(listutils::isSymbol(allowedType1,"points")){
           allowedType=8;
       }


     if(!fname->IsDefined()){
       defined = false;
     } else {
       defined = true;
       string name = fname->GetValue();
       file.open(name.c_str(),ios::binary);
       if(!file.good()){
         defined = false;
         file.close();
       } else {
         defined = readHeader(allowedType);
         if(!defined){
           file.close();
         } else {
           file.seekg(100,ios::beg); // to the first data set
         }
       }
     }
   }

   Attribute* getNext(){
      if(!defined){
        return 0;
      }
      switch(type){
        case 1: return getNextPoint();
        case 3: return getNextLine();
        case 5: return getNextPolygon();
        case 8: return getNextMultiPoint();
        default: return 0;
      }
   }
   void close(){
     if(defined){
        file.close();
        defined = false;
     }
   }

 private:

   bool defined;
   ifstream file;
   uint32_t type;
   streampos fileend;

   bool readHeader(unsigned int allowedType){
      file.seekg(0,ios::end);
      streampos p = file.tellg();
      fileend = p;
      if(p< 100){ // minimum size not reached
         return false;
      }
      file.seekg(0,ios::beg);
      uint32_t code;
      uint32_t version;
      file.read(reinterpret_cast<char*>(&code),4);
      file.seekg(28,ios::beg);
      file.read(reinterpret_cast<char*>(&version),4);
      file.read(reinterpret_cast<char*>(&type),4);
      if(WinUnix::isLittleEndian()){
        code = WinUnix::convertEndian(code);
      } else {
         version = WinUnix::convertEndian(version);
         type = WinUnix::convertEndian(type);
      }
      if(code!=9994){
        return false;
      }
      if(version != 1000){
        return false;
      }
      return type==allowedType;
   }

   uint32_t readBigInt32(){
      uint32_t res;
      file.read(reinterpret_cast<char*>(&res),4);
      if(WinUnix::isLittleEndian()){
         res = WinUnix::convertEndian(res);
      }
      return res;
   }

   uint32_t readLittleInt32(){
      uint32_t res;
      file.read(reinterpret_cast<char*>(&res),4);
      if(!WinUnix::isLittleEndian()){
         res = WinUnix::convertEndian(res);
      }
      return res;
   }

   double readLittleDouble(){
      uint64_t tmp;
      file.read(reinterpret_cast<char*>(&tmp),8);
      if(!WinUnix::isLittleEndian()){
         tmp = WinUnix::convertEndian(tmp);
      }
      double res = * (reinterpret_cast<double*>(&tmp));
      return res;
   }

   Attribute* getNextPoint(){
      if(file.tellg() == fileend){
        return 0;
      }
     // uint32_t recNo =
      readBigInt32();
      uint32_t recLen = readBigInt32();
      uint32_t type = readLittleInt32();
      if(type == 0){ // null shape
        if(recLen!=2 || !file.good()){
          cerr << "Error in shape file detected " << __LINE__ << endl;
          defined = false;
          file.close();
          return 0;
        } else {
          return new Point(false,0,0);
        }
      }
      if(type != 1 || recLen != 10){
          cerr << "Error in shape file detected " << __LINE__ << endl;
          defined = false;
          file.close();
          return 0;
      }
      double x = readLittleDouble();
      double y = readLittleDouble();
      if(!file.good()){
          cerr << "Error in shape file detected " << __LINE__ << endl;
          defined = false;
          file.close();
          return 0;
      }
      return new Point(true,x,y);
   }

   Attribute* getNextMultiPoint(){
      if(file.tellg()==fileend){
         return 0;
      }
      uint32_t recNo = 0;
      recNo =  readBigInt32();
      uint32_t len = 0;
      len = readBigInt32();
      uint32_t type = 0;
      type = readLittleInt32();

      if(!file.good()){
         cerr << " problem in reading file " << __LINE__ << endl;
         cerr << "recNo = " << recNo << endl;
         cerr << "len = " << len << endl;
         cerr << "type = " << type << endl;
         defined = false;
         return 0;
      }

      if(type==0){
        if(len!=2){
           cerr << "Error in shape file detected " << __LINE__ << endl;
           defined = false;
           file.close();
           return 0;
        } else {
           return new Points(1);
        }
      }
      if(type!=8){
         cerr << "Error in shape file detected " << __LINE__ << endl;
         cout << "type = " << type << endl;
         cout << "file.good = " << file.good() << endl;
         defined = false;
         file.close();
         return 0;
      }
      // ignore Bounding box
      readLittleDouble();
      readLittleDouble();
      readLittleDouble();
      readLittleDouble();
      uint32_t numPoints = readLittleInt32();

      uint32_t expectedLen = (40 + numPoints*16) / 2;
      if(len != (expectedLen)){
         cerr << "Error in file " << __LINE__ << endl;
         cerr << "len = " << len << endl;
         cerr << "numPoints " << numPoints << endl;
         cerr << " expected" << expectedLen << endl;
         file.close();
         defined = false;
         return 0;
      }
      Points* ps = new Points(numPoints);
      Point p;
      ps->StartBulkLoad();
      for(unsigned int i=0;i<numPoints && file.good();i++){
         double x = readLittleDouble();
         double y = readLittleDouble();
         p.Set(x,y);
         (*ps) += p;
      }
      ps->EndBulkLoad();
      if(!file.good()){
        cerr << "Error in file " << __LINE__ << endl;
        delete ps;
        return 0;
      } else {
         return ps;
      }
   }

   Attribute* getNextLine(){
     if(file.tellg()==fileend){
       return 0;
     }
     //uint32_t recNo =
     readBigInt32();
     uint32_t len = readBigInt32();
     uint32_t type = readLittleInt32();
     if(type==0){
       if(len!=2){
          cerr << "Error in file detected" << endl;
          file.close();
          defined = false;
          return 0;
       } else {
         return new Line(1);
       }
     }
     // a non-null line
     if(type!=3){
       cerr << "Error in file detected" << endl;
       file.close();
       defined = false;
       return 0;
     }
     // ignore box
     readLittleDouble();
     readLittleDouble();
     readLittleDouble();
     readLittleDouble();

     uint32_t numParts  = readLittleInt32();
     uint32_t numPoints = readLittleInt32();
     vector<int> parts;
     for(unsigned int i=0;i<numParts && file.good() ; i++){
        uint32_t part = readLittleInt32();
        parts.push_back(part);
     }
     if(!file.good()){
       cerr << "error in reading file" << endl;
       file.close();
       defined = false;
       return 0;
     }
     Point p1;
     Point p2;
     Line* line = new Line(numPoints);
     int rpoints = 0;
     int lastPoint = 0;
     line->StartBulkLoad();
     int edgeno = -1;
     HalfSegment hs;
     for(unsigned int i=0;i<parts.size() && file.good(); i++){
       lastPoint = i==parts.size()-1?numPoints:parts[i+1];
       int start = rpoints;
       for(int j=start; j<lastPoint && file.good() ;j++){
          double x = readLittleDouble();
          double y = readLittleDouble();
          p2.Set(x,y);
          if(j>start){
            if(!AlmostEqual(p1,p2)){
                hs.Set( true, p1, p2 );
                hs.attr.edgeno = ++edgeno;
                (*line) += hs;
                hs.SetLeftDomPoint( !hs.IsLeftDomPoint() );
                (*line) += hs;
            }
          }
          p1 = p2;
          rpoints++;
       }
     }
     line->EndBulkLoad();
     if(!file.good()){
       cerr << "Error in reading file" << endl;
       delete line;
       file.close();
       defined = false;
       return 0;
     }
     return line;
   }


/*
~getNextPolygon~

This function read the next region value from the file and returns it.
In case of an error or if no more regions are available, the return value
will be 0.

*/


   Attribute* getNextPolygon(){
     if(file.tellg()==fileend){ // end of file reached
       return 0;
     }

     readBigInt32(); // ignore record number
     uint32_t len = readBigInt32();
     uint32_t type = readLittleInt32();
     if(type==0){
       if(len!=2){
          cerr << "Error in file detected" << __LINE__  <<  endl;
          file.close();
          defined = false;
          return 0;
       } else { // NULL shape, return an empty region
         return new Region(0);
       }
     }
     if(type!=5){ // different shapes are not allowed
       cerr << "Error in file detected" << __LINE__ << endl;
       cerr << "Expected Type = 5, but got type: " << type << endl;
       file.close();
       defined = false;
       return 0;
     }
     // ignore box
     readLittleDouble();
     readLittleDouble();
     readLittleDouble();
     readLittleDouble();
     uint32_t numParts  = readLittleInt32();
     uint32_t numPoints = readLittleInt32();
     // for debugging the file
     uint32_t clen = (44 + 4*numParts + 16*numPoints)/2;
     if(clen!=len){
        cerr << "File invalid: length given in header seems to be wrong"
             << endl;
        file.close();
        defined = false;
        return 0;
     }
     // read the starts of the cycles
     vector<uint32_t> parts;
     for(unsigned int i=0;i<numParts;i++){
       uint32_t p = readLittleInt32();
       parts.push_back(p);
     }

     // read the cycles
     vector<vector <Point> > cycles;
     uint32_t pos = 0;
     for(unsigned int p=0;p<parts.size(); p++){
        vector<Point> cycle;
        uint32_t start = pos;
        uint32_t end = p< (parts.size()-1) ? parts[p+1]:numPoints;
        Point lastPoint(true,0.0,0.0);
        // read a single cycle
        for(unsigned int c=start; c< end; c++){
            double x = readLittleDouble();
            double y = readLittleDouble();
            Point point(true,x,y);
            if(c==start){ // the first point
               cycle.push_back(point);
               lastPoint = point;
            } else if(!AlmostEqual(lastPoint,point)){
               cycle.push_back(point);
               lastPoint = point;
            }
            pos++;
        }
        cycles.push_back(cycle);
     }
     return buildRegion(cycles);
   }

public:



/*
~numOfNeighbours~

Returns the number of halfsegments having the same dominating point
like the halfsegment at positition pos (exclusive that segment).
The DbArray has to be sorted.

*/
int numOfNeighbours1(const DbArray<HalfSegment>& segs,const int pos){
   HalfSegment hs1;
   HalfSegment hs2;
   segs.Get(pos,&hs1);
   Point dp(hs1.GetDomPoint());
   int num = 0;
   bool done= false;
   int pos2 = pos-1;
   while(pos2>0 && !done){
     segs.Get(pos2,hs2);
     if(AlmostEqual(dp,hs2.GetDomPoint())){
       num++;
       pos2--;
     }else {
       done = true;
     }
   }
   done = false;
   pos2 = pos+1;
   while(!done && pos2<segs.Size()){
     segs.Get(pos2,hs2);
     if(AlmostEqual(dp,hs2.GetDomPoint())){
       num++;
       pos2++;
     }else {
       done = true;
     }
   }
   return num;
}
// slower implementation
int numOfNeighbours(const DbArray<HalfSegment>& segs,const int pos){
   HalfSegment hs1;
   HalfSegment hs2;
   segs.Get(pos,hs1);
   Point dp = hs1.GetDomPoint();
   double dpx = dp.GetX();
   int num = 0;
   bool done= false;
   int pos2 = pos-1;
   while(pos2>=0 && !done){
     segs.Get(pos2,hs2);
     if(AlmostEqual(dp,hs2.GetDomPoint())){
       num++;
       pos2--;
     }else {
       double dpx2 = hs2.GetDomPoint().GetX();
       if(AlmostEqual(dpx,dpx2)){
         pos2--;
       }else{
         done = true;
       }
     }
   }
   done = false;
   pos2 = pos+1;
   while(!done && pos2<segs.Size()){
     segs.Get(pos2,hs2);
     if(AlmostEqual(dp,hs2.GetDomPoint())){
       num++;
       pos2++;
     }else {
       double dpx2 = hs2.GetDomPoint().GetX();
       if(AlmostEqual(dpx,dpx2)){
         pos2++;
       } else {
         done = true;
       }
     }
   }
   return num;
}

/*
~numOfUnusedNeighbours~

Returns the number of halfsegments having the same dominating point
like the halfsegment at positition pos (exclusive that segment).
The DBArray has to be sorted.

*/
int numOfUnusedNeighbours(const DbArray<HalfSegment>& segs,
                          const int pos,
                          const bool* used){
   HalfSegment hs1;
   HalfSegment hs2;
   segs.Get(pos,hs1);
   Point dp = hs1.GetDomPoint();
   double dpx = dp.GetX();
   int num = 0;
   bool done= false;
   int pos2 = pos-1;
   while(pos2>0 && !done){
     segs.Get(pos2,hs2);
     if(AlmostEqual(dp,hs2.GetDomPoint())){
       if(!used[pos2]){
         num++;
       }
       pos2--;
     }else {
       double dpx2 = hs2.GetDomPoint().GetX();
       if(AlmostEqual(dpx,dpx2)){
         pos2--;
       } else {
         done = true;
       }
     }
   }
   done = false;
   pos2 = pos+1;
   while(!done && pos2<segs.Size()){
     segs.Get(pos2,hs2);
     if(AlmostEqual(dp,hs2.GetDomPoint())){
       if(!used[pos2]){
         num++;
       }
       pos2++;
     }else {
       double dpx2 = hs2.GetDomPoint().GetX();
       if(AlmostEqual(dpx,dpx2)){
          pos2++;
       } else {
         done = true;
       }
     }
   }
   return num;
}


void removeDeadEnd(DbArray<HalfSegment>* segments,
                   vector<Point>& path, vector<int>& positions,
                   const bool* used){

   int pos = positions.size()-1;
   while((pos>=0) &&
         (numOfUnusedNeighbours(*segments,positions[pos],used) < 1)){
       pos--;
   }
   if(pos<=0){ // no extension found
      positions.clear();
      path.clear();
   } else {
     cout << "Remove path from " << pos << " until its end" << endl;
     positions.erase(positions.begin()+pos, positions.end());
     path.erase(path.begin()+pos,path.end());
   }

}





};




template<int filePos>
int shpimportVM(Word* args, Word& result,
            int message, Word& local, Supplier s){

   switch(message){
     case OPEN: {
       if(local.addr){
          delete (shpimportInfo*)local.addr;
       }
       FText* fname = static_cast<FText*>(args[filePos].addr);
       ListExpr type = nl->Second(qp->GetType(s));
       local.setAddr(new shpimportInfo(type,fname));
       return 0;
     }
     case REQUEST: {
       if(!local.addr){
          return CANCEL;
       }
       shpimportInfo* info = static_cast<shpimportInfo*>(local.addr);
       if(!info){
         return CANCEL;
       }
       Attribute* next = info->getNext();
       result.addr = next;
       return next?YIELD:CANCEL;
     }
     case CLOSE: {
       shpimportInfo* info = static_cast<shpimportInfo*>(local.addr);
       if(info){
         info->close();
         delete info;
         local.addr = 0;
       }
       return 0;
     }
     default: {
       return 0;
     }
   }
}

/*
5.3 Specification

*/
const string shpimportSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> s x text -> stream(s), s in {point, points, line, region}"
    "</text--->"
    "<text> _ shpimport [ _ ] </text--->"
    "<text> produces a stream of spatial objects from a shapefile</text--->"
    "<text> not tested !!!</text--->"
    ") )";

/*
5.6 Operator instance

*/
Operator shpimport( "shpimport",
                    shpimportSpec,
                    shpimportVM<1>,
                    Operator::SimpleSelect,
                    shpimportTM);


/*
6 Operator shpimport2

imports a shape file without given the type directly.

*/
ListExpr shpimport2TM(ListExpr args){
   if(nl->ListLength(args)!=1){
      return listutils::typeError("one argument expected");
   }
   ListExpr arg = nl->First(args);
   if(nl->ListLength(arg) !=2){
      return listutils::typeError("Error, argument has to consists of 2 parts");
   }
   ListExpr type = nl->First(arg);
   ListExpr value = nl->Second(arg);

   if(!listutils::isSymbol(type,"text")){
       return listutils::typeError("text expected");
   }

   // get the value if possible

   Word res;
   bool success = QueryProcessor::ExecuteQuery(nl->ToString(value),res);
   if(!success){
     return listutils::typeError("could not evaluate the value of  " + 
                                  nl->ToString(value) );
   }

   FText* resText = static_cast<FText*>(res.addr);

   if(!resText->IsDefined()){
      resText->DeleteIfAllowed();
       return listutils::typeError("filename evaluated to be undefined");
   }

   string name = resText->GetValue();
   resText->DeleteIfAllowed();

   string shpType;
   bool correct;
   string errmsg;

   shpType = getShpType(name, correct, errmsg);
   if(!correct){
      return listutils::typeError(errmsg);
   }
   return nl->TwoElemList(nl->SymbolAtom("stream"), nl->SymbolAtom(shpType));
}



const string shpimport2Spec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> text -> stream(s), s in {point, points, line, region}</text--->"
    "<text> shpimport2(_) </text--->"
    "<text> produces a stream of spatial objects from a shapefile</text--->"
    "<text> query shpimport2('kinos.shp') count</text--->"
    ") )";


/*
5.6 Operator instance

*/
Operator shpimport2( "shpimport2",
                    shpimport2Spec,
                    shpimportVM<0>,
                    Operator::SimpleSelect,
                    shpimport2TM);



/*
6 Operator dbtype

6.1 Type Mapping

text -> text

*/

ListExpr dbtypeTM(ListExpr args){
  if(nl->ListLength(args)==1 && nl->IsEqual(nl->First(args),"text")){
     return nl->SymbolAtom("text");
  }
  ErrorReporter::ReportError("text expected");
  return nl->TypeError();
}

/*
6.2 Value Mapping

*/

ListExpr getDBAttrList(string name, bool& correct, string& errorMessage){

  ifstream f;
  correct = true;
  f.open(name.c_str(),ios::binary);
  if(!f.good()){
     errorMessage="could not open file";
     correct = false;
     return listutils::typeError();
  }

  unsigned char code= 0;
  f.read(reinterpret_cast<char*>(&code),1);
  if(code!=0x03 && code!=0x83){
     f.close();
     errorMessage = "Not a DBase III file";
     correct = false;
     return listutils::typeError();
  }
  if(!f.good()){
    errorMessage = "problem in reading file";
    correct = false;
    return listutils::typeError();
  }
  f.seekg(8,ios::beg);

  if(!f.good()){
    errorMessage = "problem in reading file";
    correct = false;
    return listutils::typeError();
  }

  uint16_t headerlength;
  f.read(reinterpret_cast<char*>(&headerlength),2);

  if(!WinUnix::isLittleEndian()){
     headerlength = WinUnix::convertEndian(headerlength);
  }
  int check = (headerlength-1) % 32;
  if(check!=0){
    errorMessage = "wrong header length";
    correct = false;
     f.close();
    return listutils::typeError();
  }
  if(!f.good()){
    errorMessage = "problem in reading file";
    f.close();
    correct = false;
    return listutils::typeError();
  }
  int noRecords = (headerlength-32) / 32;

  

  f.seekg(0,ios::end);
  if(f.tellg() < headerlength){
    errorMessage = "invalid filesize";
    correct = false;
    f.close();
    return listutils::typeError();
  }

  f.seekg(32,ios::beg);
  char buffer[32];

  ListExpr attrList;
  ListExpr last;
  for(int i=0;i<noRecords;i++){
     f.read(buffer,32);
     stringstream ns;
     for(int j=0;j<11;j++){
        if(buffer[j]){
          ns << buffer[j];
        }
     }
     string name = ns.str();
     unsigned char typeCode = buffer[11];
     unsigned char dc = buffer[17];
     unsigned char len = buffer[16];
     string type = "unknown";
     switch(typeCode){
      case 'C' : {
         type = len <= MAX_STRINGSIZE?"string":"text";
         break;
      }
      case 'D' : {
         type = "instant";
         break;
      }
      case 'L' : {
         type = "bool";
         break;
      }
      case 'M' : {
         type = "text";
         break;
      }
      case 'N' : {
         type = dc==0?"int":"real";
         break;
      }
      default : {
         errorMessage = "unknown type ";
         f.close();
         correct = false;
         return listutils::typeError();
      }
     }
     ListExpr attr = nl->TwoElemList(nl->SymbolAtom(name), 
                                     nl->SymbolAtom(type));
     if(i==0){ // first elem
       attrList = nl->OneElemList(attr);
       last = attrList;
     } else {
        last = nl->Append(last,attr);
    }
  }
  f.close();
  return attrList;
}



int dbtypeVM(Word* args, Word& result,
            int message, Word& local, Supplier s){

  FText* arg = static_cast<FText*>(args[0].addr);
  result = qp->ResultStorage(s);
  FText* res = static_cast<FText*>(result.addr);
  if(!arg->IsDefined()){
     res->SetDefined(false);
     return 0;
  }


  string name = arg->GetValue();

  bool correct;
  string errorMessage = "";
  ListExpr attrList = getDBAttrList(name, correct, errorMessage);

  

  if(!correct){
     res->SetDefined(false);
     cout << errorMessage;
     return 0;
  }

  stringstream resstr;
  resstr << "[const rel(tuple([";
  bool first = true;
  while(!nl->IsEmpty(attrList)){
     ListExpr a = nl->First(attrList);
     if(!first){
        resstr << ", ";
     }
     resstr << nl->SymbolValue(nl->First(a));
     resstr << " : ";
     resstr << nl->SymbolValue(nl->Second(a));
     attrList = nl->Rest(attrList);
     first = false;
  }
  resstr << "])) value ()]";
 
  resstr.flush();
  res->Set(true, resstr.str());
  return 0;
}

/*
6.3 Specification

*/

const string dbtypeSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> text -> text</text--->"
    "<text> dbtype(filename) </text--->"
    "<text> returns an object description of the secondo object"
    " stored within the dbase III file specified by the name</text--->"
    "<text> not tested !!!</text--->"
    ") )";

/*
6.4 Operator instance

*/

Operator dbtype( "dbtype",
                   dbtypeSpec,
                   dbtypeVM,
                   Operator::SimpleSelect,
                   dbtypeTM);


/*
7 Operator dbimport

7.1 Type Mapping

(rel(tuple(...))) x text -> stream(tuple(...)

*/
ListExpr dbimportTM(ListExpr args){
  if(nl->ListLength(args)!=2){
    ErrorReporter::ReportError("rel x text expected");
    return nl->TypeError();
  }
  if(!IsRelDescription(nl->First(args)) ||
     !nl->IsEqual(nl->Second(args),"text")){
    ErrorReporter::ReportError("rel x text expected");
    return nl->TypeError();
  }
  ListExpr res =  nl->TwoElemList(nl->SymbolAtom("stream"),
                         nl->Second(nl->First(args)));
  return res;
}


/*
7.2 Specification

*/

const string dbimportSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> rel(tuple(...) x text -> stream (tuple(...))</text--->"
    "<text> rel dbimport[filename] </text--->"
    "<text> returns the content of the file as a stream </text--->"
    "<text> not tested !!!</text--->"
    ") )";


/*
7.3 Value Mapping

*/
class DbimportInfo{

public:

/*
~Constructor~

*/
   DbimportInfo(ListExpr type, FText* fname){
     BasicTuple = 0;
     tupleType = 0;
     ListExpr attrList = nl->Second(nl->Second(type));
     while(!nl->IsEmpty(attrList)){
        ListExpr type = nl->Second(nl->First(attrList));
        if(nl->AtomType(type)!=SymbolType){
           cerr << " composite type not allowed " << endl;
           defined = false;
           return;
        }
        types.push_back(nl->SymbolValue(type));
        attrList = nl->Rest(attrList);
     }

     if( !fname->IsDefined()){
        cerr << " undefined filename" << endl;
        defined = false;
        return;
     }
     name = fname->GetValue();
     file.open((name).c_str(),ios::binary);
     if(!file.good()) {
        cerr << "error in reading file" << endl;
        defined = false;
        return;
     }
     if(!checkFile()){
       cerr << "CheckFile failed" << endl;
       defined = false;
       return;
     }
     defined = true;
     ListExpr numType = nl->Second(
                       SecondoSystem::GetCatalog()->NumericType((type)));
     tupleType = new TupleType(numType);
     BasicTuple = new Tuple(numType);
     current = 0;
     file.seekg(headerLength,ios::beg);
     bool found = false;
     for(unsigned int i=0;i<isMemo.size() && !found;i++){
        found = isMemo[i];
     }
     if(found){
        memofile.open((name+".dbt").c_str(),ios::binary);
        if(!memofile.good()){
           cerr << "cannot open dbt file " + name + ".dbt" << endl;
        }
     }
   }

   Tuple* getNext(){
      if(!defined || current==noRecords  || file.eof() || !file.good()){
         return 0;
      }
      unsigned char buffer[recordSize];
      // read buffer
      file.read(reinterpret_cast<char*>(buffer),recordSize);

      if(!file.good()){ // error in reading file
        defined = false;
        return 0;
      }
      Tuple* ResultTuple = BasicTuple->Clone();

      unsigned int offset = 1; // ignore a deleted flag
      for(int i=0;i<noAttributes;i++){
        if(!store(types[i], ResultTuple, i, buffer,offset, fieldLength[i])){
            delete ResultTuple;
            defined = false;
            return 0;
        }
        int fl = fieldLength[i];
        offset += fl;
      }
      return ResultTuple;
   }

  ~DbimportInfo(){
     if(BasicTuple){
        delete BasicTuple;
        BasicTuple = 0;
     }
     if(tupleType){
       tupleType->DeleteIfAllowed();
       tupleType=0;
     }
   }

   void close(){
      file.close();
      memofile.close();
   }

private:
  bool defined;
  string name;
  ifstream file;
  ifstream memofile;
  uint32_t noRecords;
  uint16_t headerLength;
  uint16_t recordSize;
  uint16_t noAttributes;
  vector<string> types;
  vector<bool> isMemo;
  vector<unsigned char> fieldLength;
  Tuple* BasicTuple;
  TupleType* tupleType;
  unsigned int current;

  bool checkFile(){
    file.seekg(0,ios::beg);
    unsigned char code;
    file.read(reinterpret_cast<char*>(&code),1);
    file.seekg(4,ios::beg);
    if( !file.good() || ( (code!=3) && (code!=0x83) ) ){
       cerr << "invalid code" << endl;
       file.close();
       return false;
    }
    file.read(reinterpret_cast<char*>(&noRecords),4);
    file.read(reinterpret_cast<char*>(&headerLength),2);
    file.read(reinterpret_cast<char*>(&recordSize),2);
    if(!WinUnix::isLittleEndian()){
       noRecords = WinUnix::convertEndian(noRecords);
       headerLength = WinUnix::convertEndian(headerLength);
       recordSize = WinUnix::convertEndian(recordSize);
    }
    file.seekg(32,ios::beg);
    if(!file.good()){
       cerr << "Error in reading file" << endl;
       file.close();
       return false;
    }
    if( (headerLength-1) % 32  != 0){
      cerr << " invalid length for the header" << headerLength << endl;
      file.close();
      return false;
    }
    noAttributes = (headerLength - 32) / 32;
    if(noAttributes < 1){
      cerr << "numer of attributes invalid" << noAttributes << endl;
      file.close();
      return false;
    }
    if(noAttributes != types.size()){
      cerr << "numbers of types not match "
           << types.size() << " <-> " << noAttributes << endl;
      file.close();
      return false;
    }
    unsigned char buffer[32];
    for(int i=0;  i < noAttributes; i++){
      file.read(reinterpret_cast<char*>(buffer),32);
      unsigned char t = buffer[11];
      unsigned char len = buffer[16];
      unsigned char dc = buffer[17];
      string type;
      switch(t){
        case 'C' : type = len <= MAX_STRINGSIZE?"string":"text";
                   isMemo.push_back(false);
                   break;
        case 'D' : type = "instant";
                   isMemo.push_back(false);
                   break;
        case 'L' : type = "bool";
                   isMemo.push_back(false);
                   break;
        case 'M' : type = "text";
                   isMemo.push_back(true);
                   if(len!=10){
                      cerr << "Invalid field length for memo detected ,"
                           << " correct to 10" << endl;
                      len = 10;
                   }
                   break;
        case 'N' : type = dc==0?"int":"real";
                   isMemo.push_back(false);
                   break;
        default : file.close();
                  return false;
      }
      if(type!=types[i]){
        cerr << "non-matching types " << type << " <-> " << types[i] << endl;
        file.close();
        return false;
      } else {
         fieldLength.push_back(len);
      }
    }
    return true;
  }

  void trim(string& str) {
    string::size_type pos = str.find_last_not_of(' ');
    if(pos != string::npos) {
      str.erase(pos + 1);
      pos = str.find_first_not_of(' ');
      if(pos != string::npos){
         str.erase(0, pos);
      }
    } else {
     str.erase(str.begin(), str.end());
    }
}

  bool store(string type, Tuple* tuple, int index,
             unsigned char* buf, int offset, int length){
     stringstream ss;
     for(int i=offset; i<offset+length;i++){
       if(buf[i]!=0){
          ss << buf[i];
       }
     }
     string s = ss.str();
     if(type=="int"){
        if(s.size()==0){
          tuple->PutAttribute(index, new CcInt(false,0));
        } else {
            trim(s);
            istringstream buffer(s);
            int res_int;
            buffer >> res_int;
            tuple->PutAttribute(index, new CcInt(true,res_int));
        }
     } else if(type=="real"){
        if(s.size()==0){
          tuple->PutAttribute(index, new CcReal(false,0));
        } else {
          trim(s);
          istringstream buffer(s);
          double res_double;
          buffer >> res_double;
          tuple->PutAttribute(index, new CcReal(true,res_double));
        }
     } else if(type=="string"){
        if(s.size()==0){
          tuple->PutAttribute(index, new CcString(false,""));
        } else {
          tuple->PutAttribute(index, new CcString(true,s));
        }
     } else if(type=="bool"){
        trim(s);
        if(s.size()==0 || s=="?"){
           tuple->PutAttribute(index, new CcBool(false,false));
        }
        bool res_bool = s=="y" || s=="Y" || s=="t" || s=="T";
        tuple->PutAttribute(index,new CcBool(true,res_bool));

     } else if(type=="text"){
        bool ismemo = isMemo[index];
        if(ismemo){
           if(s.size()==0){
              tuple->PutAttribute(index, new FText(false,""));
           } else {
              trim(s);
              if(s.size() ==0){
                tuple->PutAttribute(index,new FText(true,""));
              } else { // need access to dbt file
                // compute block number
                istringstream buffer(s);
                int bn;
                buffer >> bn;
                memofile.seekg(512*bn,ios::beg);
                if(memofile.good()){
                   char c;
                   stringstream text;
                   memofile.read(&c,1);
                   while(memofile.good() && c!=0x1A){
                      text << c;
                      memofile.read(&c,1);
                   }
                   if(!memofile.good()){
                     cerr << "Error in reading memo file";
                   }
                   tuple->PutAttribute(index,new FText(true,text.str()));
                } else {
                   tuple->PutAttribute(index,new FText(false,""));
                }
              }
           }
        } else {
           if(s.size()==0){
             tuple->PutAttribute(index, new FText(false,""));
           } else {
             tuple->PutAttribute(index, new FText(true,s));
           }
        }
     } else if(type=="instant"){
        datetime::DateTime* res =
            new datetime::DateTime(datetime::instanttype);
        if(s.size()==0){
           res->SetDefined(false);
        } else {
          istringstream buffer(s);
          int resInt;
          buffer >> resInt;
          int year = resInt/10000 + 2000;
          int month = (resInt/100)%100;
          int day = resInt%100;
          res->Set(year,month,day);
        }
        tuple->PutAttribute(index,res);
     } else {
          assert(false);
     }
     return true;
  }


};


int dbimportVM(Word* args, Word& result,
               int message, Word& local, Supplier s){

  switch(message){
    case OPEN: {
      ListExpr type = qp->GetType(s);
      FText* fname = static_cast<FText*>(args[1].addr);
      local.setAddr(new DbimportInfo(type,fname));
      return 0;
    }
    case REQUEST: {
      DbimportInfo* info = static_cast<DbimportInfo*>(local.addr);
      if(!info){
        return CANCEL;
      } else {
        Tuple* tuple = info->getNext();
        result.addr = tuple;
        return tuple==0?CANCEL:YIELD;
      }
    }
    case CLOSE: {
      DbimportInfo* info = static_cast<DbimportInfo*>(local.addr);
      if(info){
        info->close();
        delete info;
        local.addr=0;
      }

    }
    default: {
     return 0;
    }
  }
}

/*
7.4 Operator Instance

*/
Operator dbimport( "dbimport",
                   dbimportSpec,
                   dbimportVM,
                   Operator::SimpleSelect,
                   dbimportTM);




/*
8 Operator db3import2

This variant of the db3import operator combines the dbtype and the 
dbimport operator to a single operator. It exploits a new feature of
secondo, namely accessing values of constant expressions within type mappings
not available during the implementation of db3type and db3import.

*/

/*
8.1 Value Mapping


The value mapping of this function is text -> stream(tuple(....)) 

The tuple type depends on the content of the file specified by the text.
If the file does not reprensent a valid db3 file, the result will be 
a typeerror.

*/
ListExpr dbimport2TM(ListExpr args){
   if(nl->ListLength(args)!=1){
      return listutils::typeError("one argument expected");
   }

   ListExpr arg = nl->First(args);
   // the format must be (text value)
   // where values represents the value of the text as 
   // query expression
   if(nl->ListLength(arg)!=2){
      return listutils::typeError("internal error");
   }

   if(!listutils::isSymbol(nl->First(arg), FText::BasicType())){
       return listutils::typeError("the argument must be of type text");
   }

   ListExpr value = nl->Second(arg);

   Word res;
   bool success = QueryProcessor::ExecuteQuery(nl->ToString(value),res);
   if(!success){
     return listutils::typeError("could not evaluate the value of text");
   }

   FText* resText = static_cast<FText*>(res.addr);

   if(!resText->IsDefined()){
      resText->DeleteIfAllowed();
      return listutils::typeError("filename evaluated to be undefined");
   }


   string name = resText->GetValue();
   resText->DeleteIfAllowed();
   resText = 0;
   bool correct;
   string errMsg="";
   ListExpr attrList = getDBAttrList(name,correct, errMsg);
   if(!correct){
       return listutils::typeError(errMsg);
   }   
   return nl->TwoElemList(nl->SymbolAtom("stream"),
                   nl->TwoElemList(nl->SymbolAtom("tuple"), attrList));


}


const string dbimport2Spec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> text -> stream (tuple(...))</text--->"
    "<text>  dbimport2(filename) </text--->"
    "<text> returns the content of the file as a stream </text--->"
    "<text> </text--->"
    ") )";



int dbimport2VM(Word* args, Word& result,
               int message, Word& local, Supplier s){

  switch(message){
    case OPEN: {
      ListExpr type = qp->GetType(s);
      FText* fname = static_cast<FText*>(args[0].addr);
      local.setAddr(new DbimportInfo(type,fname));
      return 0;
    }
    case REQUEST: {
      DbimportInfo* info = static_cast<DbimportInfo*>(local.addr);
      if(!info){
        return CANCEL;
      } else {
        Tuple* tuple = info->getNext();
        result.addr = tuple;
        return tuple==0?CANCEL:YIELD;
      }
    }
    case CLOSE: {
      DbimportInfo* info = static_cast<DbimportInfo*>(local.addr);
      if(info){
        info->close();
        delete info;
        local.addr=0;
      }

    }
    default: {
     return 0;
    }
  }
}



Operator dbimport2( "dbimport2",
                   dbimport2Spec,
                   dbimport2VM,
                   Operator::SimpleSelect,
                   dbimport2TM);


/*
8 Operator saveObject

8.1 TypeMapping

The Type mapping is:

---- T x string x text -> bool
----


*/

ListExpr saveObjectTM(ListExpr args){
  string err = "T x string x text  expected";
  if(nl->ListLength(args)!=3){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  if(!nl->IsEqual(nl->Second(args),"string") |
     !nl->IsEqual(nl->Third(args),"text")){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  ListExpr obj = nl->First(args);

  if(!nl->IsAtom(obj) &&
     !nl->IsEmpty(obj) &&
     nl->IsEqual(nl->First(obj),"stream")){
    ErrorReporter::ReportError("stream not allowes as first argument");
    return nl->TypeError();
  }
  return nl->SymbolAtom("bool");
}

/*
8.2 Specification

*/
const string saveObjectSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> T x string x text -> bool </text--->"
    "<text> _  saveObject[objName, fileName] </text--->"
    "<text> saves an object to a file in nested list format</text--->"
    "<text> query (3 + 4) saveObject[\"seven\",'seven.obj']  </text--->"
    ") )";

/*
8.3 Value Mapping

*/

int saveObjectVM(Word* args, Word& result,
               int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcBool* res = static_cast<CcBool*>(result.addr);
  CcString* objName = static_cast<CcString*>(args[1].addr);
  FText* fileName = static_cast<FText*>(args[2].addr);
  if(!objName->IsDefined() || !fileName->IsDefined()){
    res->Set(true,false);
    return 0;
  }
  ListExpr type = qp->GetType(qp->GetSon(s,0));
  int algId;
  int typeId;
  string tname;
  if(! SecondoSystem::GetCatalog()->LookUpTypeExpr(type,tname,algId,typeId)){
    res->Set(true,false);
    return 0;
  }

  ListExpr value = (am->OutObj(algId,typeId))(type,args[0]);

  string oname = objName->GetValue();

  ListExpr objList = nl->FiveElemList(
                        nl->SymbolAtom("OBJECT"),
                        nl->SymbolAtom(oname),
                        nl->TheEmptyList(),
                        type,
                        value);
  bool success = nl->WriteToFile(fileName->GetValue(),objList);
  res->Set(true,success);
  return 0;
}

/*
8.4 Operator Instance

*/
Operator saveObject( "saveObject",
                   saveObjectSpec,
                   saveObjectVM,
                   Operator::SimpleSelect,
                   saveObjectTM);


/*
9 Operator ~isFile~

This operator checks, whether a given file exists in the filesystem.
If so, it returns ~TRUE~, otherwise ~FALSE~

9.1 Type Mapping Function ~stringORtext2boolTM~

Used for operators ~isFile~, ~removeFile~, ~createDirectory~, ~isDirectory~

---- {text|string} -> bool
----

*/
ListExpr stringORtext2boolTM(ListExpr args){
  string err = "text or string expected";
  if(nl->ListLength(args)!=1){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  if(    !nl->IsEqual(nl->First(args),"string")
      && !nl->IsEqual(nl->First(args),"text")){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  return nl->SymbolAtom("bool");
}

/*
9.2 Value Mapping Function for ~checkfile~

We implement this as a template function. The template class parameter ~T~
may be either CcString or FText.

*/

template<class T>
int isFileVM(Word* args, Word& result,
                int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcBool* res = static_cast<CcBool*>(result.addr);
  T* objName = static_cast<T*>(args[0].addr);
  if(!objName->IsDefined()){
    res->Set(false,false);
  } else {
    string fileNameS = objName->GetValue();
    res->Set(true,FileSystem::FileOrFolderExists(fileNameS));
  }
  return 0;
}

ValueMapping isFilevaluemap[] = {isFileVM<CcString>, isFileVM<FText>};


/*
9.3 Specification for ~isFile~

*/
const string isFileSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {text|string} -> bool </text--->"
    "<text> isFile( Name ) </text--->"
    "<text> Checks, whether the file or directory Name exists.\n"
    "If Name is UNDEFINED, nothing is done and the result is "
    "UNDEFINED.</text--->"
    "<text> query isFile('data.csv')  </text--->"
    ") )";

/*
9.4 Selection Function for ~isFile~

*/

int stringORtextSelect( ListExpr args )
{
  if(nl->IsEqual(nl->First(args),"string")) {
    return 0;
  } else if(nl->IsEqual(nl->First(args),"text")){
    return 1;
  }
  assert( false );
  return -1;
}

/*
9.5 Operator Instance for operator ~isFile~

*/
Operator isFile ( "isFile",
                   isFileSpec,
                   2,
                   isFilevaluemap,
                   stringORtextSelect,
                   stringORtext2boolTM);


/*
10 Operator ~removeFile~

This operator removes/deletes a given file from the file system

10.1 Type Mapping for ~removeFile~

Uses ~stringORtext2boolTM~.

10.2 Value Mapping for ~removeFile~

*/

template<class T>
int removeFileVM(Word* args, Word& result,
                 int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcBool* res = static_cast<CcBool*>(result.addr);
  T* objName = static_cast<T*>(args[0].addr);
  if(!objName->IsDefined()){
    res->Set(false,false);
  } else {
    string fileNameS = objName->GetValue();
    res->Set(true,FileSystem::DeleteFileOrFolder(fileNameS));
  }
  return 0;
}



ValueMapping removeFilevaluemap[] = {removeFileVM<CcString>,
                                     removeFileVM<FText>};

/*
10.4 Selection Function for ~removeFile~

Uses ~stringORtextSelect~.

*/

/*
10.4 Specification  for ~removeFile~

*/
const string removeFileSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {text|string} -> bool </text--->"
    "<text> removeFile( Name ) </text--->"
    "<text> Deletes the file or empty folder with the given Name from"
    " the files system. Returns TRUE, if this succeeds, and FALSE if the "
    "file could not be deleted or any error occurs. If the Name is\n"
    "UNDEFINED, nothing is done and the result is UNDEFINED.\n"
    "WARNING: Be extremely careful about removing files!</text--->"
    "<text> query removeFile('data.csv')  </text--->"
    ") )";


/*
10.5 Operator Instance for operator ~removeFile~

*/
Operator removeFile ( "removeFile",
                   removeFileSpec,
                   2,
                   removeFilevaluemap,
                   stringORtextSelect,
                   stringORtext2boolTM);


/*
11 Operator ~createDirectory~

The operator creates the passed directory within the file system.

11.1 Type Mapping for ~createDirectory~

Uses ~stringORtext2boolTM~.

11.2 Value Mapping for ~createDirectory~

*/

template<class T>
int createDirectoryVM(Word* args, Word& result,
                 int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcBool* res = static_cast<CcBool*>(result.addr);
  T* objName = static_cast<T*>(args[0].addr);
  if(!objName->IsDefined()){
    res->Set(false,false);
  } else {
    string fileNameS = objName->GetValue();
    res->Set(true,FileSystem::CreateFolder(fileNameS));
  }
  return 0;
}

ValueMapping createDirectoryvaluemap[] = {createDirectoryVM<CcString>,
                                          createDirectoryVM<FText>};

/*
11.3 Specification  for ~createDirectory~

*/
const string createDirectorySpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {text|string} -> bool </text--->"
    "<text> createDirectory( Name ) </text--->"
    "<text> Creates a directory with the given Name on"
    " the files system. Returns TRUE, if this succeeds, and FALSE if the "
    "directory could not be created or any error occurs. If the Name is\n"
    "UNDEFINED, nothing is done and the result is UNDEFINED.</text--->"
    "<text> query createDirectory('my_csv_directory')  </text--->"
    ") )";

/*
11.4 Selection Function for ~createDirectory~

Uses ~stringORtextSelect~.

11.5
Operator Instance for operator ~createDirectory~

*/
Operator createDirectory ( "createDirectory",
                   createDirectorySpec,
                   2,
                   createDirectoryvaluemap,
                   stringORtextSelect,
                   stringORtext2boolTM);


/*
12 Operator ~fileSize~

Returns the size of a file or directory on the file system

12.1 Type Mapping for ~fileSize~

---- {text|string} [ x bool ] -> int
----

*/
ListExpr stringORtextOPTIONbool2intTM(ListExpr args){
  string err = "{text|string} [ x bool ] expected";
  int listLength = nl->ListLength(args);
  if(listLength!=1 && listLength !=2){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  if(    !nl->IsEqual(nl->First(args),symbols::STRING)
      && !nl->IsEqual(nl->First(args),symbols::TEXT)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  if (listLength == 2 && !nl->IsEqual(nl->Second(args),symbols::BOOL)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  return nl->SymbolAtom(symbols::INT);
}

/*
12.2 Value Mapping for ~fileSize~

*/

template<class T>
int fileSizeVM(Word* args, Word& result,
                 int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcInt* res = static_cast<CcInt*>(result.addr);
  T* objName = static_cast<T*>(args[0].addr);
  if(!objName->IsDefined()){
    res->Set(false,0);
    return 0;
  }
  bool recursive = false;
  if(qp->GetNoSons(s)==2){
     CcBool* rec = static_cast<CcBool*>(args[1].addr);
    if(!rec->IsDefined()){
      res->Set(false,0);
    } else {
      recursive = rec->GetBoolval();
    }
  }
  if(recursive){
    cerr<<"Recursive file size calculation still not implemented!\n"<<endl;
  }
  string fileNameS = objName->GetValue();
  int32_t size = FileSystem::GetFileSize(fileNameS);
  if(size<0){
    res->Set(false,0);
    return 0;
  }
  res->Set(true,size);
  return 0;
}

ValueMapping fileSizevaluemap[] = {fileSizeVM<CcString>,
                                   fileSizeVM<FText>};

/*
12.3 Selection Function for ~fileSize~

Uses ~stringORtextSelect~.

12.3 Specification for ~fileSize~

*/
const string fileSizeSpec =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {text|string} [ x bool ] -> bool </text--->"
    "<text> fileSize( Name, Recurse ) </text--->"
    "<text> Returns the file's or directory's size in Bytes.\n"
    "If Name specifies a directory and the optional parameter Recurse is TRUE, "
    "the operator recurses into subdirectories aggregating the total size.\n"
    "If Name or Recurse is UNDEFINED, nothing is done and the result is "
    "UNDEFINED.</text--->"
    "<text> query fileSize('data.csv')  </text--->"
    ") )";

/*
12.5 Operator Instance for operator ~fileSize~

*/
Operator fileSize ( "fileSize",
                   fileSizeSpec,
                   2,
                   fileSizevaluemap,
                   stringORtextSelect,
                   stringORtextOPTIONbool2intTM);

/*
13 Operator ~isDirectory~

This operator checks, whether a given file is a directory in the filesystem.
If so, it returns ~TRUE~, otherwise ~FALSE~

13.1 Type Mapping Function for ~isDirectory~

Uses ~stringORtext2boolTM~.

13.2 Value Mapping Function for ~isDirectory~

We implement this as a template function. The template class parameter ~T~
may be either CcString or FText.

*/

template<class T>
int isDirectoryVM(Word* args, Word& result,
                int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcBool* res = static_cast<CcBool*>(result.addr);
  T* objName = static_cast<T*>(args[0].addr);
  if(!objName->IsDefined()){
    res->Set(false,false);
  } else {
    string fileNameS = objName->GetValue();
    res->Set(true,FileSystem::IsDirectory(fileNameS));
  }
  return 0;
}

ValueMapping isDirectoryvaluemap[] = {isDirectoryVM<CcString>,
                                      isDirectoryVM<FText>};


/*
13.3 Specification for ~isDirectory~

*/
const string isDirectorySpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {text|string} -> bool </text--->"
    "<text> isDirectory( Name ) </text--->"
    "<text> Checks, whether Name is a directory.\n"
    "If Name is UNDEFINED, nothing is done and the result is "
    "UNDEFINED.</text--->"
    "<text> query isDirectory('data.csv')  </text--->"
    ") )";

/*
13.4 Selection Function for ~isDirectory~

Uses ~stringORtextSelect~.

13.5 Operator Instance for operator ~isDirectory~

*/
Operator isDirectory ( "isDirectory",
                   isDirectorySpec,
                   2,
                   isDirectoryvaluemap,
                   stringORtextSelect,
                   stringORtext2boolTM);


/*
14 Operator ~writeFile~

The operator writes a ~text~ to a (text-) file on the file system.

14.1 Type Mapping for ~writeFile~

---- text x {text|string} [ x bool ] -> bool
----

*/
ListExpr stringORtext_stringORtext_OPTIONALbool2boolTM(ListExpr args){
  string err = "{text|string} x {text|string} [ x bool ] expected";
  int listLength = nl->ListLength(args);
  if(listLength!=2 && listLength !=3){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  if(    !nl->IsEqual(nl->First(args),symbols::STRING)
      && !nl->IsEqual(nl->First(args),symbols::TEXT)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  if(    !nl->IsEqual(nl->Second(args),symbols::STRING)
      && !nl->IsEqual(nl->Second(args),symbols::TEXT)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }


  if (listLength == 3 && !nl->IsEqual(nl->Third(args),symbols::BOOL)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  return nl->SymbolAtom(symbols::BOOL);
}


/*
14.2 Value Mapping for ~writeFile~

*/
template<class T, class S>
int writeFileVM(Word* args, Word& result,
                int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcBool* res = static_cast<CcBool*>(result.addr);
  T* content = static_cast<T*>(args[0].addr);
  S* fileName = static_cast<S*>(args[1].addr);

  if(!content->IsDefined() || !fileName->IsDefined()){
    res->Set(false,false);
    return 0;
  }
  string fileNameS = fileName->GetValue();
  if(fileNameS == ""){
    res->Set(true,false);
    return 0;
  }
  bool append = false;
  if(qp->GetNoSons(s)==3){
    CcBool* app = static_cast<CcBool*>(args[2].addr);
    if(!app->IsDefined()){
      res->Set(false,false);
      return 0;
    } else {
      append = app->GetBoolval();
    }
  }
  bool fileExists = FileSystem::FileOrFolderExists(fileNameS);
  bool fileIsDirectory = FileSystem::IsDirectory(fileNameS);
  if(fileExists && fileIsDirectory){
    cerr << "writeFile: Cannot write to existing directory!\n"<<endl;
    res->Set(false,false);
    return 0;
  }
  string contentS = content->GetValue();
  ofstream file;
  if(append){
    file.open(fileNameS.c_str(), ios::out | ios::app | ios_base::binary);
  } else {
    file.open(fileNameS.c_str(), ios::out | ios::trunc | ios_base::binary);
  }
  if(!file.good()){
    cerr << "writeFile: Cannot open file '"<< fileName << "'!\n"<<endl;
    res->Set(false,false);
    return 0;
  }
  file << contentS;
  if(file.good()){
    res->Set(true,true);
  } else {
    res->Set(true,false);
  }
  file.close();
  return 0;
}

ValueMapping writeFilevaluemap[] = {writeFileVM<CcString, CcString>,
                                    writeFileVM<CcString, FText   >,
                                    writeFileVM<FText,    CcString>,
                                    writeFileVM<FText,    FText   >};



/*
14.3 Specification for ~writeFile~

*/
const string writeFileSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {text|string} x {text|string} [ x bool ] -> bool </text--->"
    "<text> writeFile( Content, Name, Append ) </text--->"
    "<text> Writes text Content to file Name. If the file does not exist, it\n"
    "is created. If optional argument Append = TRUE, Content will be appended\n"
    "to the file. Otherwise the file gets possibly overwritten.\n"
    "If any argument is UNDEFINED, nothing is done and the result is "
    "UNDEFINED. If writing succeeds, result is TRUE.</text--->"
    "<text> query writeFile('This is content', 'data.csv')  </text--->"
    ") )";

/*
14.4 Selection Function for ~writeFile~

*/
int stringORtext_stringORtext_Select( ListExpr args )
{
  int index = 0;
  // first arg
  if(nl->IsEqual(nl->First(args),"string")) {
    index += 0;
  } else if(nl->IsEqual(nl->First(args),"text")){
    index +=  2;
  } else {
    assert(false);
    return -1;
  }
  // second arg
  if(nl->IsEqual(nl->Second(args),"string")) {
    index += 0;
  } else if(nl->IsEqual(nl->Second(args),"text")){
    index += 1;
  } else {
    assert(false);
    return -1;
  }
  return index;
}


/*
14.5 Operator Instance for operator ~isDirectory~

*/
Operator writeFile ( "writeFile",
                   writeFileSpec,
                   4,
                   writeFilevaluemap,
                   stringORtext_stringORtext_Select,
                   stringORtext_stringORtext_OPTIONALbool2boolTM);


/*
15 Operator ~readFile~

Reads a file into a text object. If the file contains a NULL, the result is
UNDEFINED.

15.1 TypeMapping for ~readFile~

---- {text|string} -> text
----

*/

ListExpr stringORtext2textTM(ListExpr args){
  string err = "{text|string} expected";
  int listLength = nl->ListLength(args);
  if(listLength!=1){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  if(    !nl->IsEqual(nl->First(args),symbols::STRING)
      && !nl->IsEqual(nl->First(args),symbols::TEXT)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  return nl->SymbolAtom(symbols::TEXT);
}

/*
15.2 Value Mapping for ~readFile~

*/

template<class T>
int readFileVM(Word* args, Word& result,
               int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  FText* res = static_cast<FText*>(result.addr);
  T* fileName = static_cast<T*>(args[0].addr);
  if(!fileName->IsDefined()){
    res->Set(false,"");
    return 0;
  }
  string fileNameS = fileName->GetValue();
  if(   fileNameS == ""
     || !FileSystem::FileOrFolderExists(fileNameS)
     || FileSystem::IsDirectory(fileNameS)
    ){
    cerr << "readFile: Cannot open file '" << fileNameS << "'!" << endl;
    res->Set(false,"");
    return 0;
  }
  string Content = "";
  ifstream file;
  file.open(fileNameS.c_str(),ios::binary);
  if(!file.good()){
    cerr << "readFile: Error opening file '" << fileNameS << "'!" << endl;
    res->Set(false,"");
    return 0;
  }
  // read file character by character and check for NULL
  char      currChar;
  bool      ok = true;
  file.seekg(0,ios::end);
  streampos fileend = file.tellg();
  file.seekg(0,ios::beg);
  while( ok && (file.tellg() != fileend)){
    file.read(reinterpret_cast<char*>(&currChar),1);
    ok = (currChar != '\0');
    if(ok){ // append char to string
      Content.append(1,currChar);
    } else{
      cerr << "readFile: Encountered NULL at position " << file.tellg()
           << "." << endl;
    }
  }
  file.close();
  if(ok){
    res->Set(true,Content);
  } else {
    res->Set(false,"");
  }
  return 0;
}

ValueMapping readFilevaluemap[] = {readFileVM<CcString>,
                                   readFileVM<FText>};

/*
15.3 Specification for ~readFile~

*/
const string readFileSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {text|string} -> text </text--->"
    "<text> readFile( Name ) </text--->"
    "<text> Reads text from file Name. If the file does not exist, \n"
    "contains a NULL character or some error occurs, the result is "
    "UNDEFINED.</text--->"
    "<text> query readFile('data.csv')  </text--->"
    ") )";

/*
15.4 Selection Function for ~writeFile~

Uses ~stringORtextSelect~.

15.5 Operator Instance for operator ~readFile~

*/
Operator readFile ( "readFile",
                   readFileSpec,
                   2,
                   readFilevaluemap,
                   stringORtextSelect,
                   stringORtext2textTM);


/*
16 Operator ~moveFile~

Move a file to another location within the file system. Returns TRUE,
iff this succeeds.

16.1 TypeMapping for ~moveFile~

---- {text|string} x {text|string} -> bool
----

*/

ListExpr stringORtext_stringORtext2boolTM(ListExpr args){
  string err = "{text|string} expected";
  int listLength = nl->ListLength(args);
  if(listLength!=2){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  if(    !nl->IsEqual(nl->First(args),symbols::STRING)
      && !nl->IsEqual(nl->First(args),symbols::TEXT)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  if(    !nl->IsEqual(nl->Second(args),symbols::STRING)
      && !nl->IsEqual(nl->Second(args),symbols::TEXT)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }

  return nl->SymbolAtom(symbols::BOOL);
}

/*
16.2 Value Mapping for ~moveFile~

*/

template<class T, class S>
int moveFileVM(Word* args, Word& result,
               int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcBool* res = static_cast<CcBool*>(result.addr);
  T* fileNameOld = static_cast<T*>(args[0].addr);
  S* fileNameNew = static_cast<S*>(args[1].addr);
  if(!fileNameOld->IsDefined() || !fileNameNew->IsDefined()){
    res->Set(false,false);
    return 0;
  }
  string fileNameOldS = fileNameOld->GetValue();
  string fileNameNewS = fileNameNew->GetValue();
  if(   fileNameOldS == "" || fileNameNewS == ""
     || !FileSystem::FileOrFolderExists(fileNameOldS)
    ){
    cerr << "moveFile: Cannot open file '" << fileNameOldS << "'!" << endl;
    res->Set(true,false);
    return 0;
  }
  bool boolresult = FileSystem::RenameFileOrFolder(fileNameOldS,fileNameNewS);
  res->Set(true,boolresult);
  return 0;
}

ValueMapping moveFilevaluemap[] = {moveFileVM<CcString, CcString>,
                                   moveFileVM<CcString, FText>,
                                   moveFileVM<FText,    CcString>,
                                   moveFileVM<FText,    FText>};

/*
16.3 Specification for ~moveFile~

*/
const string moveFileSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {text|string} -> text </text--->"
    "<text> moveFile( OldName, NewName ) </text--->"
    "<text> Move file OldName to file NewName. Can also be used to rename a "
    "file.\nReturns TRUE, iff move-command succeeds.\n"
    "WARNING: Be extremely carefully about moving files!\n"
    "         The operator also overwrites existing files!</text--->"
    "<text> query moveFile('data.csv', 'data.csv.old')  </text--->"
    ") )";

/*
16.4 Selection Function for ~moveFile~

Uses ~stringORtext\_stringORtextSelect~.


16.5 Operator Instance for operator ~moveFile~

*/
Operator moveFile ( "moveFile",
                   moveFileSpec,
                   4,
                   moveFilevaluemap,
                   stringORtext_stringORtext_Select,
                   stringORtext_stringORtext2boolTM);

/*
17 Operator ~getDirectory~

Get a stream of text with the listing of the given directory.

17.1 TypeMapping for ~getDirectory~

---- {text|string} -> stream(text)
----

*/

ListExpr stringORtext_OPTIONALint2textstreamTM(ListExpr args){
  string err = "{text|string} [x int] expected";
  int listLength = nl->ListLength(args);
  if(listLength!=1 && listLength!=2){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  if(    !nl->IsEqual(nl->First(args),symbols::STRING)
      && !nl->IsEqual(nl->First(args),symbols::TEXT)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  if(    listLength == 2
      && !nl->IsEqual(nl->Second(args),symbols::INT)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  return nl->TwoElemList( nl->SymbolAtom(symbols::STREAM),
                          nl->SymbolAtom(symbols::TEXT));
}

/*
17.2 Value Mapping for ~getDirectory~

*/

class GetDirectoryLocalInfo
{ public:
    GetDirectoryLocalInfo(){
      elements = FilenameList(0);
      iter = elements.begin();
    }

    ~GetDirectoryLocalInfo(){}

    void SetElements(FilenameList L){
      elements = L;
      iter = elements.begin();
    }

    FilenameList elements;
    FilenameList::iterator iter;
};

template<class T>
int getDirectoryVM(Word* args, Word& result,
                   int message, Word& local, Supplier s){
  GetDirectoryLocalInfo* li;
  result = qp->ResultStorage(s);
  T* dirName = static_cast<T*>(args[0].addr);
  FText* resText;
  int levels = 1;

  switch(message){
    case OPEN:{
      li = new GetDirectoryLocalInfo();
      local.setAddr(li);
      if(!dirName->IsDefined()){
        cerr << "getDirectory: Directory parameter undefined!"
             << endl;
        return 0;
      }
      string dirNameS = dirName->GetValue();
      if(   dirNameS == ""
         || !FileSystem::FileOrFolderExists(dirNameS)
         || !FileSystem::IsDirectory(dirNameS)
        ){
        cerr << "getDirectory: Cannot open directory '" << dirNameS << "'!"
             << endl;
        return 0;
      }
      if(qp->GetNoSons(s)==2){ // get optional 2nd argument
        CcInt* levelsCc = static_cast<CcInt*>(args[1].addr);
        if(!levelsCc->IsDefined()){
          cerr << "getDirectory: Optional recursion parameter undefined!"
               << endl;
          return 0;
        } else {
          levels = levelsCc->GetIntval();
          if(levels<1){
            levels = 1;
          }
        }
      }
      FilenameList L(0);
      if(FileSystem::FileSearch(dirNameS, L, 0, levels, true, true, 0)){
        li->SetElements(L);
      }
      return 0;
    }

    case REQUEST:{
      if(local.addr){
        li = static_cast<GetDirectoryLocalInfo*>(local.addr);
      }else{
        result.setAddr(0);
        return CANCEL;
      }
      if(li->iter == li->elements.end()){
        result.setAddr(0);
        return CANCEL;
      }
      resText = new FText(true, *(li->iter));
      result.setAddr(resText);
      ++(li->iter);
      return YIELD;
    }

    case CLOSE:{
      result.setAddr(0);
      if(local.addr){
        li = static_cast<GetDirectoryLocalInfo*>(local.addr);
        delete li;
        local.setAddr(0);
      }
      return 0;
    }
  }
  // this line should never be reached!
  return -1;
}

ValueMapping getDirectoryvaluemap[] = {getDirectoryVM<CcString>,
                                       getDirectoryVM<FText>};

/*
17.3 Specification for ~getDirectory~

*/
const string getDirectorySpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> {text|string} [ x int] -> stream(text) </text--->"
    "<text> getDirectory( DirectoryName [, N]) </text--->"
    "<text> Create a stream of text containing all file names from directory\n"
    "DirectoryName and its subfolders. The function recurses down to the Nth\n"
    "level. If N is not specified, or N is set to 1 or smaller, meaning that\n"
    "only the direct contents of DirectoryName are listed.</text--->"
    "<text> query getDirectory('tmp', 2)</text--->"
    ") )";

/*
17.4 Selection Function for ~getDirectory~

Uses ~stringORtextSelect~.


17.5 Operator Instance for operator ~getDirectory~

*/
Operator getDirectory( "getDirectory",
                   getDirectorySpec,
                   2,
                   getDirectoryvaluemap,
                   stringORtextSelect,
                   stringORtext_OPTIONALint2textstreamTM);

/*
18 Operator ~toCSVtext~

Converts any object of a Type from CSVEXPORTABLE to a text object with the
corresponding CSV text representation.

18.1 TypeMapping for ~toCSVtext~

---- CSVEXPORTABLE -> text
----

*/

ListExpr CSVEXPORTABLEtoTextTM(ListExpr args){
  string err = "CSVEXPORTABLE expected";
  ListExpr errorInfo = nl->OneElemList(nl->SymbolAtom("ERROR"));
  int listLength = nl->ListLength(args);
  if(listLength!=1){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  if(!am->CheckKind("CSVEXPORTABLE",nl->First(args),errorInfo)){
    ErrorReporter::ReportError("stream element not in kind csvexportable");
    return nl->TypeError();
  }
  return nl->SymbolAtom(symbols::TEXT);
}



/*
18.2 Value Mapping for ~toCSVtext~

*/

int toCSVtextVM(Word* args, Word& result,
               int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  FText* res = static_cast<FText*>(result.addr);
  Attribute* object = static_cast<Attribute*>(args[0].addr);
  if(!object->IsDefined()){
    res->Set(false,"");
    return 0;
  }
  string objStr = object->getCsvStr();
  res->Set(true,objStr);
  return 0;
}

/*
18.3 Specification for ~toCSVtext~

*/
const string toCSVtextSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> X -> text, for X in CSVEXPORTABLE </text--->"
    "<text> toCSVtext( Obj ) </text--->"
    "<text>Converts any object of a CSVEXPORTABLE type to its CSV text\n"
    "representation.</text--->"
    "<text> query toCSVtext(3.14137) </text--->"
    ") )";

/*
18.4 Selection Function for ~toCSVtext~

Uses ~simpleselect~.


18.5 Operator Instance for operator ~toCSVtext~

*/
Operator toCSVtext( "toCSVtext",
                    toCSVtextSpec,
                    toCSVtextVM,
                    Operator::SimpleSelect,
                    CSVEXPORTABLEtoTextTM);


/*
19 Operator ~fromCSVtext~

Converts any object of a Type from CSVEXPORTABLE to a text object with the
corresponding CSV text representation.

19.1 TypeMapping for ~fromCSVtext~

---- CSVEXPORTABLE x text -> CSVEXPORTABLE
----

*/

ListExpr CSVIMPORTABLE_textORstring2CSVIMPORATABLETM(ListExpr args){
  string err = "CSVIMPORTABLE x {text|string} expected";
  ListExpr errorInfo = nl->OneElemList(nl->SymbolAtom("ERROR"));
  int listLength = nl->ListLength(args);
  if(listLength!=2){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  if(!am->CheckKind("CSVIMPORTABLE",nl->First(args),errorInfo)){
    ErrorReporter::ReportError("stream element not in kind csvimportable");
    return nl->TypeError();
  }
  if(    !nl->IsEqual(nl->Second(args),symbols::STRING)
      && !nl->IsEqual(nl->Second(args),symbols::TEXT)){
    ErrorReporter::ReportError(err);
    return nl->TypeError();
  }
  return nl->First(args);
}



/*
19.2 Value Mapping for ~fromCSVtext~

*/

template<class T>
int fromCSVtextVM(Word* args, Word& result,
               int message, Word& local, Supplier s){

  T* InText= static_cast<T*>(args[1].addr);
  result = qp->ResultStorage( s );
  Attribute* Res = static_cast<Attribute*>(result.addr);

  if(!InText->IsDefined()){
    Res->SetDefined(false);
    return 0;
  }
  string myText = InText->GetValue();
  Res->ReadFromString(myText);
  return 0;
}

ValueMapping fromCSVtextvaluemap[] = {fromCSVtextVM<FText>,
                                      fromCSVtextVM<CcString>};

/*
19.3 Specification for ~fromCSVtext~

*/
const string fromCSVtextSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> X x {text|string} -> X, for X in CSVIMPORTABLE </text--->"
    "<text> fromCSVtext( Pattern, Text ) </text--->"
    "<text>Converts the textual CSV representation of the CSVIMPORTABLE type\n"
    "specified by the type of Pattern to a secondo object.</text--->"
    "<text> query fromCSVtext(0.0, '3.141') </text--->"
    ") )";

/*
19.4 Selection Function for ~fromCSVtext~

*/

int fromCSVtextSelect(ListExpr args){
  if(nl->IsEqual(nl->Second(args),symbols::TEXT)){
    return 0;
  } else if(nl->IsEqual(nl->Second(args),symbols::STRING)){
    return 1;
  }
  return -1;
}

/*
19.4 Operator instance

*/

Operator fromCSVtext( "fromCSVtext",
                   fromCSVtextSpec,
                   2,
                   fromCSVtextvaluemap,
                   fromCSVtextSelect,
                   CSVIMPORTABLE_textORstring2CSVIMPORATABLETM);

/*
20 Operator ~getPID~

20.1 Type Mapping for ~getPID~

---- getPID:    --> int
----

*/
ListExpr getPIDTM(ListExpr args){
  if( nl->ListLength(args)==0 ){
     return nl->SymbolAtom("int");
  }
  ErrorReporter::ReportError("No argument expected");
  return nl->TypeError();
}

/*
20.2 Value Mapping

*/

int getPIDVM(Word* args, Word& result,
            int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcInt* res = static_cast<CcInt*>(result.addr);
  res->Set(true, WinUnix::getpid());
  return 0;
}

/*
20.3 Specification

*/

const string getPIDSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> -> int</text--->"
    "<text>getPID( ) </text--->"
    "<text>Returns the process identifier of the running Secondo kernel "
    "process. The result is always defined.</text--->"
    "<text>query getPID()</text--->"
    ") )";

/*
20.4 Operator instance

*/

Operator getPID( "getPID",
                   getPIDSpec,
                   getPIDVM,
                   Operator::SimpleSelect,
                   getPIDTM);

/*
21 Operator ~getSecondoVersion~

21.1 Type Mapping for ~getSecondoVersion~

---- getSecondoVersion:    --> string
----

*/
ListExpr getSecondoVersionTM(ListExpr args){
  if( nl->ListLength(args)==0 ){
     return nl->SymbolAtom("string");
  }
  ErrorReporter::ReportError("No argument expected");
  return nl->TypeError();
}

/*
21.2 Value Mapping

*/

int getSecondoVersionVM(Word* args, Word& result,
            int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcString* res = static_cast<CcString*>(result.addr);
  ostringstream os;
  os.precision(47);
  os << SECONDO_VERSION_MAJOR << "."
     << SECONDO_VERSION_MINOR << "."
     << SECONDO_VERSION_REVISION;
  res->Set( true, os.str() );
  return 0;
}

/*
21.3 Specification

*/

const string getSecondoVersionSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> -> int</text--->"
    "<text>getSecondoVersion( ) </text--->"
    "<text>Returns the version string (version.subversion.revision) for the "
    "running Secondo kernel process. The result is always defined.</text--->"
    "<text>query getSecondoVersion()</text--->"
    ") )";

/*
21.4 Operator instance

*/

Operator getSecondoVersion( "getSecondoVersion",
                   getSecondoVersionSpec,
                   getSecondoVersionVM,
                   Operator::SimpleSelect,
                   getSecondoVersionTM);


/*
22 Operator ~getBDBVersion~

21.1 Type Mapping for ~getBDBVersion~

---- getBDBVersion:    --> string
----

*/
ListExpr getBDBVersionTM(ListExpr args){
  if( nl->ListLength(args)==0 ){
     return nl->SymbolAtom("string");
  }
  ErrorReporter::ReportError("No argument expected");
  return nl->TypeError();
}

/*
22.2 Value Mapping

*/

int getBDBVersionVM(Word* args, Word& result,
            int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcString* res = static_cast<CcString*>(result.addr);
  ostringstream os;
  os.precision(47);
  os << DB_VERSION_MAJOR << "."
     << DB_VERSION_MINOR;
  res->Set( true, os.str() );
  return 0;
}

/*
22.3 Specification

*/

const string getBDBVersionSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> -> int</text--->"
    "<text>getBDBVersion( ) </text--->"
    "<text>Returns the version string (version.subversion) for the "
    "running Secondo kernel process' BerkeleyDB version. "
    "The result is always defined.</text--->"
    "<text>query getBDBVersion()</text--->"
    ") )";

/*
22.4 Operator instance

*/

Operator getBDBVersion( "getBDBVersion",
                   getBDBVersionSpec,
                   getBDBVersionVM,
                   Operator::SimpleSelect,
                   getBDBVersionTM);


/*
23 Operator ~getSecondoPlatform~

23.1 Type Mapping for ~getSecondoPlatform~

---- getSecondoPlatform:    --> string
----

*/
ListExpr getSecondoPlatformTM(ListExpr args){
  if( nl->ListLength(args)==0 ){
     return nl->SymbolAtom("string");
  }
  ErrorReporter::ReportError("No argument expected");
  return nl->TypeError();
}

/*
23.2 Value Mapping

*/

int getSecondoPlatformVM(Word* args, Word& result,
            int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcString* res = static_cast<CcString*>(result.addr);
  res->Set(true, WinUnix::getPlatformStr());
  return 0;
}

/*
23.3 Specification

*/

const string getSecondoPlatformSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> -> string</text--->"
    "<text>getSecondoPlatform( ) </text--->"
    "<text>Returns the platform, this Secondo kernel is running on. "
    "The result is always defined.</text--->"
    "<text>query getSecondoPlatform()</text--->"
    ") )";

/*
23.4 Operator instance

*/

Operator getSecondoPlatform( "getSecondoPlatform",
                   getSecondoPlatformSpec,
                   getSecondoPlatformVM,
                   Operator::SimpleSelect,
                   getSecondoPlatformTM);

/*
24 Operator ~getPageSize~

24.1 Type Mapping for ~getPageSize~

---- getPageSize:    --> int
----

*/
ListExpr getPageSizeTM(ListExpr args){
  if( nl->ListLength(args)==0 ){
     return nl->SymbolAtom("int");
  }
  ErrorReporter::ReportError("No argument expected");
  return nl->TypeError();
}

/*
24.2 Value Mapping

*/

int getPageSizeVM(Word* args, Word& result,
            int message, Word& local, Supplier s){

  result = qp->ResultStorage(s);
  CcInt* res = static_cast<CcInt*>(result.addr);
  res->Set(true, WinUnix::getPageSize());
  return 0;
}

/*
24.3 Specification

*/

const string getPageSizeSpec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text> -> int</text--->"
    "<text>getPageSize( ) </text--->"
    "<text>Returns the file-system's page size. "
    "The result is always defined.</text--->"
    "<text>query getPageSize()</text--->"
    ") )";

/*
24.4 Operator instance

*/

Operator getPageSize( "getPageSize",
                   getPageSizeSpec,
                   getPageSizeVM,
                   Operator::SimpleSelect,
                   getPageSizeTM);

/*
25 Creating the Algebra

*/

class ImExAlgebra : public Algebra
{
public:
  ImExAlgebra() : Algebra()
  {
    AddOperator( &csvexport );
    AddOperator( &shpexport );
    AddOperator( &db3export );
    AddOperator( &shptype );
    AddOperator( &shpimport );
    AddOperator( &shpimport2 );
    shpimport2.SetUsesArgsInTypeMapping();
    AddOperator( &dbtype );
    AddOperator( &dbimport);
    AddOperator( &dbimport2);
    dbimport2.SetUsesArgsInTypeMapping();
    AddOperator( &saveObject);
    AddOperator( &csvimport);
    AddOperator( &isFile);
    AddOperator( &removeFile);
    AddOperator( &createDirectory);
    AddOperator( &fileSize);
    AddOperator( &isDirectory);
    AddOperator( &writeFile);
    AddOperator( &readFile);
    AddOperator( &moveFile);
    AddOperator( &getDirectory);
    AddOperator( &toCSVtext);
    AddOperator( &fromCSVtext);
    AddOperator( &getPID);
    AddOperator( &getSecondoVersion);
    AddOperator( &getBDBVersion);
    AddOperator( &getSecondoPlatform);
    AddOperator( &getPageSize);
    AddOperator( &nmeaimport);
    nmeaimport.SetUsesArgsInTypeMapping(); 
  }
  ~ImExAlgebra() {};
};

/*
9 Initialization

*/

extern "C"
Algebra*
InitializeImExAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return (new ImExAlgebra());
}


