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

#include <string>

#include "Algebra.h"
#include "NestedList.h"
#include "ListUtils.h"
#include "NList.h"
#include "LogMsg.h"
#include "QueryProcessor.h"
#include "ConstructorTemplates.h"
#include "StandardTypes.h"
#include "TypeMapUtils.h"
#include "Symbols.h"


#include "../MainMemory2/MPointer.h"
#include "../MainMemory2/Mem.h"
#include "../MainMemory2/MemoryObject.h"
#include "../MainMemory2/MemCatalog.h"

#include "../Relation-C++/RelationAlgebra.h"
#include "../Stream/Stream.h"
#include "StreamIterator.h"

#include "../FText/FTextAlgebra.h"

#include "Utils.h"
#include "RelationInfo.h"
#include "PGraph.h"
#include "PGraphMem.h"
#include "QueryTree.h"
#include "QueryGraph.h"
#include "PGraphQueryProcessor.h"
#include "QueryOutputFields.h"
#include "CypherLang.h"

using namespace std;


extern NestedList* nl;
extern QueryProcessor *qp;



namespace pgraph {

// HELPER FUNCTIONS 

   void insertPGraphMemoryObject(PGraph *pg, MemoryGraphObject *pgm)
   {
      string _inmemoryobj_catalogname = pg->name+"_MEMDATA";
      mm2algebra::MemCatalog *memCatalog = 
        mm2algebra::MemCatalog::getInstance();
      if (memCatalog->isObject(_inmemoryobj_catalogname))
      {
         memCatalog->deleteObject(_inmemoryobj_catalogname);
      }
      else
      {
         memCatalog->insert(_inmemoryobj_catalogname, pgm);
      }
   }

   MemoryGraphObject *getPGraphMemoryObject(PGraph *pg)
   {
      string _inmemoryobj_catalogname = pg->name+"_MEMDATA";
      mm2algebra::MemCatalog *memCatalog = 
        mm2algebra::MemCatalog::getInstance();
      if (!memCatalog->isObject(_inmemoryobj_catalogname))
      {
         LOGOP(10, "getPGraphMemoryObject: not loaded!");
         return NULL;
      }
      else
      {
         string memtypename = "";
         ListExpr te = 
           memCatalog->getMMObjectTypeExpr(_inmemoryobj_catalogname);
         if (nl->ToString(te) != "(mem (mpgraph 0))")
         {
            LOGOP(10, "getPGraphMemoryObject: invalid type!");
            return NULL;
         }
         MemoryGraphObject *pgm = (MemoryGraphObject *)
              memCatalog->getMMObject(_inmemoryobj_catalogname);

         return pgm;
      }
   }

//-----------------------------------------------------------------------------
// forwards
//-----------------------------------------------------------------------------
int match_process (PGraph *pg, Address *candstream, ListExpr *querytree, 
     ListExpr *outputlist, ListExpr *filterlist, Word* args, Word& result, 
     int message,  Word& local, Supplier s);


//-----------------------------------------------------------------------------
// OPERATOR info
//-----------------------------------------------------------------------------
// typemapping
ListExpr info_OpTm( ListExpr args )
{
   NList type(args);

   LOG(20,"info_OpTm");
   LOG(20, "args", type);

   if(!PGraph::checkType( type.first() ))
      return NList::typeError("first argument is not a pgraph object");
   
   NList res= NList(
      NList(Symbol::APPEND()),
      NList(NList(0).enclose() ),  
      NList(CcString::BasicType())
   );

   LOG(20, res);
   return res.listExpr();
}

// function
int info_OpFun (Word* args, Word& result, int message,
              Word& local, Supplier s)
{
   LOG(10,"info_OpFun");

   PGraph *pg = static_cast<PGraph*>( args[0].addr );
   MemoryGraphObject *pgm = getPGraphMemoryObject(pg);

   cout << pg->DumpInfo(pgm) << "\n";


   // prepare result
   string info="";
   info="ok";
   result = qp->ResultStorage(s);
   CcString* res = static_cast<CcString*>( result.addr );
   res->Set(true, info);

   return 0;
}

// register
struct info_OpInfo : OperatorInfo {

   info_OpInfo()
   {
      name      = "info";
      signature = PGraph::BasicType() + " -> " + CcString::BasicType();
      syntax    = "_ info ";
      meaning   = "Returns info about the property graph .";
   }
}; 

//-----------------------------------------------------------------------------
// OPERATOR addedgesrel
//-----------------------------------------------------------------------------
void splitEdgeEndExpr(string &expr, string &fromfield, string &relname,
    string &relfield)
{
    int i=expr.find("=");
    int j=expr.find(".");
    if ((i<0)||(j<0)||(j<i)) return;
    fromfield=expr.substr(0,i);
    relname=expr.substr(i+1,j-i-1);
    relfield=expr.substr(j+1);
}

// typemapping
ListExpr addedgesrel_OpTm( ListExpr args )
{
   NList type(args);

   LOGOP(10,"addedgesrel_OpTm", "ARGS",nl->ToString(args));

   // check argument count and types
   if (type.length() != 4) 
      return NList::typeError("Expecting a PGraph and a relname ad from "
           "and to expr ");

   if (!PGraph::checkType(type.first())) 
      return NList::typeError("first argument is not a pgraph object");

   // rough check only
   if(!type.second().isSymbol())
      return NList::typeError("Expecting from expression as second argument");
   if(!type.third().isSymbol())
      return NList::typeError("Expecting from expression as second argument");
   if(!type.fourth().isSymbol())
      return NList::typeError("Expecting from expression as second argument");

   // return result 
   ListExpr res = 
         nl->OneElemList(nl->SymbolAtom(CcBool::BasicType()) );
    
   LOGOP(10,"addedgesrel_OpTm", "RES", nl->ToString(res));
   return res;
}

// function
int addedgesrel_OpFun (Word* args, Word& result, int message,
              Word& local, Supplier s)
{
   LOG(10,"addedgesrel_OpFun");

   // prepare args
   PGraph *pg = static_cast<PGraph*>( args[0].addr );
   CcString *relname = static_cast<CcString*>( args[1].addr ); // from append
   CcString *fromexpr = static_cast<CcString*>( args[2].addr ); // from append
   CcString *toexpr = static_cast<CcString*>( args[3].addr ); //  from append
   LOGOP(10,"addedgesrel_OpFun","relname: ",relname->GetValue(), "from:",  
        fromexpr->GetValue()," to:",toexpr->GetValue());


   // check from side
   string expr=fromexpr->GetValue();
   string fieldfrom,relfrom,keyrelfrom;
   string fieldto,relto,keyrelto;
   splitEdgeEndExpr(expr ,fieldfrom, relfrom, keyrelfrom );
   expr=toexpr->GetValue();
   splitEdgeEndExpr(expr ,fieldto, relto, keyrelto );

   // check edge relation and foreign keys
   ListExpr reltype= SecondoSystem::GetCatalog()->GetObjectTypeExpr(
         relname->GetValue());
   if (nl->IsEmpty(reltype))
      throw SecondoException("relation not found");
   RelationSchemaInfo ri(reltype);
   if (ri.GetAttrInfo(fieldfrom)==NULL)
      throw SecondoException("from-name not found!");
   if (ri.GetAttrInfo(fieldto)==NULL)
      throw SecondoException("to-name not found!");
   
   // add edge
   pg->AddEdgeRel(relname->GetValue(), fieldfrom, relfrom, keyrelfrom, fieldto,
         relto, keyrelto);

   // force save pgraph object
   qp->SetModified(qp->GetSon(s, 0));

   // prepare result
   result = qp->ResultStorage(s);
   CcBool* res = static_cast<CcBool*>( result.addr );
   res->Set(true, true);
   return 0;
}

// register
struct addedgesrel_OpInfo : OperatorInfo {

   addedgesrel_OpInfo()
   {
      name      = "addedgesrel";
      signature = "mem("+PGraph::BasicType() +") x" + CcString::BasicType() 
      + " -> "      + CcBool::BasicType();
      syntax    = "_ addedgesrel (<relname>) ";
      meaning   = "Add a edges relation";
   }
}; 

//-----------------------------------------------------------------------------
// OPERATOR addnodesrel
//-----------------------------------------------------------------------------
// typemapping
ListExpr addnodesrel_OpTm( ListExpr args )
{
   NList type(args);

   LOGOP(10,"addnodesrel_OpTm", "ARGS",nl->ToString(args));

   // check argument count and types
   if (type.length() < 2) 
      return NList::typeError("Expecting a PGraph and a relname ");

   if (!PGraph::checkType(type.first())) 
      return NList::typeError("first argument is not a pgraph object");

   // get relation name
   if(!type.second().isSymbol())
      return NList::typeError("Expecting relation name as second argument");

   // optional id name
   string idname="Id";
   if (type.length()==3) {
      if(type.third().isSymbol())
         idname=type.third().str();
   }

   std::string relName = type.second().str();

   // return result 
   // APPEND relation name
   ListExpr res = nl->ThreeElemList(
    nl->SymbolAtom(Symbol::APPEND()),
         nl->OneElemList(
            nl->StringAtom(idname)),
         nl->OneElemList(nl->SymbolAtom(CcBool::BasicType()) )
   );
    
   LOGOP(10,"addnodesrel_OpTm", "RES", nl->ToString(res));
   return res;
}

// function
int addnodesrel_OpFun (Word* args, Word& result, int message,
              Word& local, Supplier s)
{
   LOG(10,"addnodesrel_OpFun");

   // prepare args
   PGraph *pg = static_cast<PGraph*>( args[0].addr );
   CcString *relname = static_cast<CcString*>( args[1].addr ); //  from append
   CcString *aidname = static_cast<CcString*>( args[2].addr ); //  from append
   string idname=(aidname!=NULL?aidname->GetValue():"Id");
   LOGOP(10,"addnodesrel_OpFun","relname: ",relname->GetValue(), "idname:",
        idname);

   // check if id in relation
   ListExpr reltype= SecondoSystem::GetCatalog()->GetObjectTypeExpr(
        relname->GetValue());
   if (nl->IsEmpty(reltype))
      throw SecondoException("relation not found");
   RelationSchemaInfo ri(reltype);
   if (ri.GetAttrInfo(idname)==NULL)
      throw SecondoException("id-name not found!");

   // add relation 
   pg->AddNodesRel(relname->GetValue(), idname);

   // force save pgraph object
   qp->SetModified(qp->GetSon(s, 0));

   // prepare result
   result = qp->ResultStorage(s);
   CcBool* res = static_cast<CcBool*>( result.addr );
   res->Set(true, true);
   return 0;
}

// register
struct addnodesrel_OpInfo : OperatorInfo {

   addnodesrel_OpInfo()
   {
      name      = "addnodesrel";
      signature = "mem("+PGraph::BasicType() +") x" + CcString::BasicType() + 
          " -> " + CcBool::BasicType();
      syntax    = "_ addnodesrel (<relname>) ";
      meaning   = "Add a nodes relation";
   }
}; 

//-----------------------------------------------------------------------------
// OPERATOR addnodeindex
//-----------------------------------------------------------------------------
// typemapping
ListExpr addnodeindex_OpTm( ListExpr args )
{
   NList type(args);

   LOGOP(10,"addnodeindex_OpTm", "ARGS",nl->ToString(args));

   // check argument count and types
   if (type.length() < 4) 
      return NList::typeError("Expecting a PGraph and three arguments ");


   // return result 
   // APPEND relation name
   ListExpr res = nl->OneElemList(
      nl->SymbolAtom(CcBool::BasicType()) );
    
   LOGOP(10,"addnodeindex_OpTm", "RES", nl->ToString(res));
   return res;
}

// function
int addnodeindex_OpFun (Word* args, Word& result, int message,
              Word& local, Supplier s)
{
   LOG(10,"addnodeindex_OpFun");

   // prepare args
   PGraph *pg = static_cast<PGraph*>( args[0].addr );
   CcString *nodetype = static_cast<CcString*>( args[1].addr ); // from append
   CcString *propname = static_cast<CcString*>( args[2].addr ); //  from append
   CcString *indexname = static_cast<CcString*>( args[3].addr ); // from append

   LOGOP(10,"addnodeindex_OpFun",nodetype->GetValue(),",", propname->
       GetValue(),",", indexname->GetValue() );

   // check if id in relation
   ListExpr reltype= SecondoSystem::GetCatalog()->GetObjectTypeExpr(
          nodetype->GetValue());
   if (nl->IsEmpty(reltype))
      throw SecondoException("relation not found");
   RelationSchemaInfo ri(reltype);
   if (ri.GetAttrInfo(propname->GetValue())==NULL)
      throw SecondoException("name not found!");

   // add relation 
   pg->AddNodeIndex(nodetype->GetValue(), propname->GetValue(), 
       indexname->GetValue());

   // force save pgraph object
   qp->SetModified(qp->GetSon(s, 0));

   // prepare result
   result = qp->ResultStorage(s);
   CcBool* res = static_cast<CcBool*>( result.addr );
   res->Set(true, true);
   return 0;
}

// register
struct addnodeindex_OpInfo : OperatorInfo {

   addnodeindex_OpInfo()
   {
      name      = "addnodeindex";
      signature = "mem("+PGraph::BasicType() +") x" + CcString::BasicType() +
           " x " + CcString::BasicType() +" x " + CcString::BasicType() + 
              " -> " + CcBool::BasicType();
      syntax    = "_ addnodeindex (<nodetype> <propname> <indexname>) ";
      meaning   = "Add a index for a nodes property";
   }
}; 

//-----------------------------------------------------------------------------
// OPERATOR loadgraph
//-----------------------------------------------------------------------------
// typemapping
ListExpr loadgraph_OpTm( ListExpr args )
{
   NList type(args);

   LOG(20,"info_OpTm");
   LOG(20, "args", type);

   if(!PGraph::checkType( type.first() ))
      return NList::typeError("first argument is not a pgraph object");
   
   NList res= NList(
      NList(Symbol::APPEND()),
      NList(NList(0).enclose() ),  
      NList(CcBool::BasicType())
   );

   LOG(20, res);
   return res.listExpr();

}

// function
int loadgraph_OpFun (Word* args, Word& result, int message,
              Word& local, Supplier s)
{
   LOG(10,"load_OpFun");
   try
   {
   PGraph *pg = static_cast<PGraph*>( args[0].addr );

   MemoryGraphObject *pgm = getPGraphMemoryObject(pg);
   if (pgm==NULL)
   {

      LOG(10,"recreating memory object!" );

      pgm = new MemoryGraphObject(
         "(mem (mpgraph 0))",
         getDBname());

      insertPGraphMemoryObject(pg,pgm);

   }
   
   pgm->LoadData(pg);
   pgm->DumpGraphDot("graph.dot");

   }
   catch(PGraphException e)
   {
      LOGERR("match1_OpTm", e.msg());
      throw SecondoException(e.msg());
   }


   // prepare result
   result = qp->ResultStorage(s);
   CcBool* res = static_cast<CcBool*>( result.addr );
   res->Set(true, true);
   return 0;
}

// register
struct loadgraph_OpInfo : OperatorInfo {

   loadgraph_OpInfo()
   {
      name      = "loadgraph";
      signature = ""+PGraph::BasicType() +" -> " + CcBool::BasicType();
      syntax    = "_ loadgraph ";
      meaning   = "loads in memory structures of the pgraph";
   }
}; 

//-----------------------------------------------------------------------------
// OPERATOR createmempgraph
//-----------------------------------------------------------------------------
// typemapping
ListExpr createmempgraph_OpTm( ListExpr args )
{
   LOG(10,"createmempgraph_OpTm");

  
  ListExpr resType = mm2algebra::MPointer::wrapType( 
      mm2algebra::Mem::wrapType(
          nl->TwoElemList(
            nl->SymbolAtom(MemoryGraphObject::BasicType()),
            nl->IntAtom(0)
            )));

  return resType;

}

// function
int createmempgraph_OpFun (Word* args, Word& result, int message,
              Word& local, Supplier s)
{
   LOG(10,"createmempgraph_OpFun");

   result = qp->ResultStorage(s);
   mm2algebra::MPointer* res = (mm2algebra::MPointer*) result.addr;

   // prepare args
   //CcInt *ms = static_cast<CcInt*>( args[0].addr );
   //cout << &ms;
   
   // prepare result
   ListExpr typeList = nl->Second(qp->GetType(s));
   //typeList = listutils::basicSymbol<MemoryGraphObject>();

   string sl=nl->ToString(typeList);

   LOG(10,"typelist", sl);

   string dbname = getDBname();

   MemoryGraphObject* mgraph = new MemoryGraphObject(
         nl->ToString(typeList), 
         dbname);

   cout <<"createing pgraphmem " << "\n";
   res->setPointer(mgraph);
   mgraph->deleteIfAllowed();
  
   return 0;
}

// register
struct createmempgraph_OpInfo : OperatorInfo {

   createmempgraph_OpInfo()
   {
      name      = "createmempgraph";
      signature = "int -> (mem(memgprah))";
      syntax    = "createmempgraph  ";
      meaning   = "Create the memory resident part of the graph";
   }
}; 


//----------------------------------------------------------------------------
// OPERATOR createpgraph
//----------------------------------------------------------------------------
// typemapping
ListExpr createpgraph_OpTm( ListExpr args )
{
   LOG(10,"createpgraph_OpTm");
   NList type(args);

  if ( type.length() != 1 ) {
    return NList::typeError("Expecting one argument ");
  }

  if ( type.first() != CcString::BasicType() ) {
    return NList::typeError("Expecting graph name ");
  }

  ListExpr resType=listutils::basicSymbol<PGraph>();
   
  return resType;

}

// function
int createpgraph_OpFun (Word* args, Word& result, int message,
              Word& local, Supplier s)
{
   LOG(10,"createpgraph_OpFun");

   // prepare args
   CcString *ms = static_cast<CcString*>( args[0].addr );
   

   // prepare result
   result = qp->ResultStorage(s);
   PGraph* pg = (PGraph*) result.addr;
   
   pg->name=ms->GetValue();

   LOG(10, "createing pgraph ", pg->GetMaxSize() );

   return 0;
}

// register
struct createpgraph_OpInfo : OperatorInfo {

   createpgraph_OpInfo()
   {
      name      = "createpgraph";
      signature = CcString::BasicType() + " -> " + PGraph::BasicType();
      syntax    = "createpgraph  ";
      meaning   = "Create a property graph";
   }
}; 

//-----------------------------------------------------------------------------
// OPERATOR match1
//-----------------------------------------------------------------------------
// typemapping
ListExpr match1_OpTm( ListExpr args )
{
   NList type(args);
   LOG(20,"match1_OpTm", nl->ToString(args));

  if ( type.length()<5 || type.length()>6 ) {
    return NList::typeError("Expecting two arguments ");
  }

  if ( type.first().first() != PGraph::BasicType() ) 
     return NList::typeError("Expecting pgraph as ");
  
   if(!Stream<Tuple>::checkType(nl->First(nl->Second(args)))) {
     return listutils::typeError("second arg is not a tuple stream");
   }

  if ( type.third().first() != FText::BasicType()  ) 
     return NList::typeError("Expecting querygraph as nested list "
         "serialized as text");

  if ( type.fourth().first() != FText::BasicType()  ) 
     return NList::typeError("Expecting filters as nested list serialized "
     "as text");

  if ( type.fifth().first() != FText::BasicType()  ) 
     return NList::typeError("Expecting output fields as nested list "
     "serialized as text");

  QueryOutputFields of;
  of.ReadFromList(type.fifth().second().str());

  return of.StreamTypeDefinition();
 
  
}


// function

int match1_OpFun (Word* args, Word& result, int message,
              Word& local, Supplier s)
{
   try
   {
      PGraphQueryProcessor *pgp;
      switch( message )
      {
         case OPEN: { // initialize the local storage

            // prepare args
            PGraph *pg = static_cast<PGraph*>( args[0].addr );

            ListExpr querylist = GetArg_FTEXT_AS_LIST(args[2].addr );
            ListExpr filterlist =  GetArg_FTEXT_AS_LIST(args[3].addr );
            ListExpr fieldlist =  GetArg_FTEXT_AS_LIST(args[4].addr );
            string options=GetArg_FTEXT_AS_STRING(args[5].addr); // optional
            
            // prepare query processor
            pgp = new PGraphQueryProcessor();
            pgp->ReadOptionsFromString(options);

            MemoryGraphObject *pgm = getPGraphMemoryObject(pg);
            if (pgm==NULL) throw PGraphException("graph not loaded!");
            pgp->pgraphMem=pgm;
            

            // transform list to query tree
            QueryTree *tree=new QueryTree();
            try{
               tree->ReadQueryTree(querylist);
               tree->ReadFilterList(filterlist);
               tree->ReadOutputFieldList(fieldlist);
               pgp->SetInputStream(args[1].addr);
               pgp->SetQueryTree(tree);        
               pgp->SetInputTupleType(qp->GetSupplierTypeExpr(qp->GetSon(s,1)));
               if (pgp->OptionDumpQueryTree)
                  tree->DumpTreeDot(NULL,"querytree.dot");
            }
            catch(PGraphException e)
            {
               cout << "ERROR: " << e.msg() << endl;
            }
            catch(SecondoException e)
            {
               cout << "ERROR: " << e.msg() << endl;
            }

            ListExpr resultType = GetTupleResultType( s );
            pgp->_OutputTupleType = new TupleType(nl->Second(resultType));
            qp->Open(args[0].addr);
            local.addr = pgp;
            return 0;
         }
         case REQUEST: { // return the next stream element
            pgp = (PGraphQueryProcessor*)local.addr;

            Tuple* tuple = pgp->ReadNextResultTuple();
            if (tuple!=NULL)
            {
               result.setAddr(tuple);
               return YIELD;
            }

            result.addr = 0;
            return CANCEL;

         }
         case CLOSE: { // free the local storage
            if (local.addr)
            {
            pgp = (PGraphQueryProcessor*)local.addr;
            pgp->_OutputTupleType->DeleteIfAllowed();
            delete pgp;
            local.addr = 0;
            }
            qp->Close(args[0].addr);
            return 0;
         }
         default: {
            return -1;
         }
      }
   }
   catch(PGraphException e)
   {
      LOGERR("match1_OpTm", e.msg());
      throw SecondoException(e.msg());
   }
}

// register
struct match1_OpInfo : OperatorInfo {

   match1_OpInfo()
   {
      name      = "match1";
      signature = "pgraph x stream x string x string x string";
      syntax    = "match1  ";
      meaning   = "Query Graph";
   }
}; 

/*
1 Operator match2

*/



//-----------------------------------------------------------------------------
// OPERATOR match2
//-----------------------------------------------------------------------------
// typemapping
ListExpr match2_OpTm( ListExpr args )
{
   NList type(args);
   LOGOP(10,"match2_OpTm", nl->ToString(args));

  if ( type.length()<4 || type.length()>5 ) {
    return NList::typeError("Argument Error ");
  }

  if ( type.first().first() != PGraph::BasicType() ) 
     return NList::typeError("Expecting pgraph as ");
 
  if ( type.second().first() != FText::BasicType()  ) 
     return NList::typeError("Expecting querygraph as nested list "
     "serialized as text");

  if ( type.third().first() != FText::BasicType()  ) 
     return NList::typeError("Expecting filters as nested list "
     "serialized as text");

  if ( type.fourth().first() != FText::BasicType()  ) 
     return NList::typeError("Expecting output fields as nested list "
     "serialized as text");

   // get list
   ListExpr querylist = 0;
   nl->ReadFromString( type.second().second().str(), querylist );
   QueryGraph qg(NULL);
   qg.ReadQueryGraph(querylist);
   if (!qg.IsConnectedAndCycleFree())
      return NList::typeError("query graph is not connected and cycle free");

  QueryOutputFields of;
  of.ReadFromList(type.fourth().second().str());

  return of.StreamTypeDefinition();
   
}


// function
int match2_OpFun (Word* args, Word& result, int message,
              Word& local, Supplier s)
{
   LOGOP(10,"match2_OpFun");
   try
   {
      PGraphQueryProcessor *pgp;
      switch( message )
      {
         case OPEN: { // initialize the local storage

            
            // prepare args
            PGraph *pg = static_cast<PGraph*>( args[0].addr );
            string options=GetArg_FTEXT_AS_STRING(args[4].addr); // optional
            ListExpr querylist=GetArg_FTEXT_AS_LIST(args[1].addr);
            ListExpr filterlist=GetArg_FTEXT_AS_LIST(args[2].addr);
            ListExpr fieldlist=GetArg_FTEXT_AS_LIST(args[3].addr);
           
            // prepare query processor
            pgp = new PGraphQueryProcessor();
            pgp->ReadOptionsFromString(options);

            // get memory object
            MemoryGraphObject *pgm = getPGraphMemoryObject(pg);
            if (pgm==NULL) throw PGraphException("graph not loaded!");
            pgp->pgraphMem=pgm;

            // convert list to optimal querytree 
            QueryGraph qg(pgp);
            qg.ReadQueryGraph(querylist);
            if (pgp->OptionDumpQueryGraph)
               qg.DumpGraphDot("querygraph.dot");

            // 
            QueryTree *tree=new QueryTree();
            try
            {
               tree=qg.CreateOptimalQueryTree();
               tree->ReadFilterList(filterlist);
               tree->ReadOutputFieldList(fieldlist);
               pgp->SetQueryTree(tree);  
               pgp->SetInputRelation(tree->Root->TypeName);
               if (pgp->OptionDumpQueryTree)
                  tree->DumpTreeDot(NULL,"querytree.dot");
            }
            catch(PGraphException e)
            {
               cout << "ERROR: " << e.msg() << endl;
            }
            catch(SecondoException e)
            {
               cout << "ERROR: " << e.msg() << endl;
            }

            ListExpr resultType = GetTupleResultType( s );
            pgp->_OutputTupleType = new TupleType(nl->Second(resultType));
            qp->Open(args[0].addr);
            local.addr = pgp;
            return 0;
         }
         case REQUEST: { // return the next stream element

            pgp = (PGraphQueryProcessor*)local.addr;

            Tuple* tuple = pgp->ReadNextResultTuple();
            if (tuple!=NULL)
            {
               result.setAddr(tuple);
               return YIELD;
            }

            result.addr = 0;
            return CANCEL;

         }
         case CLOSE: { // free the local storage
            if (local.addr)
            {
            pgp = (PGraphQueryProcessor*)local.addr;
            pgp->_OutputTupleType->DeleteIfAllowed();
            delete pgp;
            local.addr = 0;
            }
            qp->Close(args[0].addr);
            return 0;
         }
         default: {
            return -1;
         }
      }
   }
   catch(PGraphException e)
   {
      LOGERR("match2_OpTm", e.msg());
      throw SecondoException(e.msg());
   }
}

// register
struct match2_OpInfo : OperatorInfo {

   match2_OpInfo()
   {
      name      = "match2";
      signature = "pgraph x string x string x string";
      syntax    = "match2  ";
      meaning   = "Query Graph";
   }
}; 

//-----------------------------------------------------------------------------
// OPERATOR match3
//-----------------------------------------------------------------------------
// typemapping
ListExpr match3_OpTm( ListExpr args )
{
   NList type(args);
   LOGOP(10,"match3_OpTm", nl->ToString(args));

  if ( type.length()<2 || type.length()>3 ) {
    return NList::typeError("Argument Error ");
  }

  if ( type.first().first() != PGraph::BasicType() ) 
     return NList::typeError("Expecting pgraph as ");
 
  if ( type.second().first() != FText::BasicType()  ) 
     return NList::typeError("Expecting querygraph as nested list serialized "
     "as text");

   string options="";
   if (type.length()>2)
      options=type.third().second().str();

   LOGOP(10, "match3_OpTm",options);

   // parse cypher
   string cypherstring = type.second().second().str();
   CypherLanguage cypherexpr;

   ListExpr res=0;
   string cypheraslist;
   if (!cypherexpr.parse(cypherstring))
   {
      cypheraslist=cypherexpr.dumpAsListExpr();
      cout << cypheraslist << endl;
      nl->ReadFromString(cypheraslist, res);
   }
   else
   {
      return NList::typeError("ERROR parsing Cypher: "+cypherexpr._errormsg);
   }

   // 
   QueryOutputFields of;
   of.ReadFromList( nl->ToString( nl->Third(res)) );
   ListExpr outfields = of.StreamTypeDefinition();

   res = nl->ThreeElemList(
    nl->SymbolAtom(Symbol::APPEND()),
         nl->OneElemList(
            nl->TextAtom( cypheraslist )),
         outfields
   );
return res;

}


// function
int match3_OpFun (Word* args, Word& result, int message,
              Word& local, Supplier s)
{
   LOGOP(10,"match3_OpFun");
   try
   {
      PGraphQueryProcessor *pgp;
      switch( message )
      {
         case OPEN: { // initialize the local storage

            // prepare args
            PGraph *pg = static_cast<PGraph*>( args[0].addr );
            // already parsed in type mapping 
            //(take care of the index as the option arg is optional)
            string options="";
            string cypherstring="";
            if (args[3].addr!=0) {
                options=GetArg_FTEXT_AS_STRING(args[2].addr);
                cypherstring=GetArg_FTEXT_AS_STRING(args[3].addr);
            }
            else
                cypherstring=GetArg_FTEXT_AS_STRING(args[2].addr);

            LOGOP(20,"match3_OpFun","options: ",options);
            LOGOP(20,"match3_OpFun","cyptherstring from TM: ",cypherstring);
            ListExpr cypherlist=0;
            nl->ReadFromString(cypherstring, cypherlist);

            // prepare query processor
            pgp = new PGraphQueryProcessor();
            pgp->ReadOptionsFromString(options);

            // get memory object
            MemoryGraphObject *pgm = getPGraphMemoryObject(pg);
            if (pgm==NULL) throw PGraphException("graph not loaded!");
            pgp->pgraphMem=pgm;

            // convert list to optimal querytree 
            QueryGraph qg(pgp);
            qg.ReadQueryGraph(nl->First(cypherlist));
            if (pgp->OptionDumpQueryGraph)
               qg.DumpGraphDot("querygraph.dot");

            // 
            QueryTree *tree=new QueryTree();
            try
            {
               tree=qg.CreateOptimalQueryTree();
               tree->ReadFilterList(nl->Second(cypherlist));
               tree->ReadOutputFieldList(nl->Third(cypherlist));
               pgp->SetQueryTree(tree);  
               pgp->SetInputRelation(tree->Root->TypeName);
               if (pgp->OptionDumpQueryTree)
                  tree->DumpTreeDot(NULL,"querytree.dot");
            }
            catch(PGraphException e)
            {
               cout << "ERROR: " << e.msg() << endl;
            }
            catch(SecondoException e)
            {
               cout << "ERROR: " << e.msg() << endl;
            }

            ListExpr resultType = GetTupleResultType( s );
            pgp->_OutputTupleType = new TupleType(nl->Second(resultType));
            qp->Open(args[0].addr);
            local.addr = pgp;
            return 0;
         }
         case REQUEST: { // return the next stream element
            pgp = (PGraphQueryProcessor*)local.addr;

            Tuple* tuple = pgp->ReadNextResultTuple();
            if (tuple!=NULL)
            {
               result.setAddr(tuple);
               return YIELD;
            }

            result.addr = 0;
            return CANCEL;

         }
         case CLOSE: { // free the local storage
            if (local.addr)
            {
            pgp = (PGraphQueryProcessor*)local.addr;
            pgp->_OutputTupleType->DeleteIfAllowed();
            delete pgp;
            local.addr = 0;
            }
            qp->Close(args[0].addr);
            return 0;
         }
         default: {
            return -1;
         }
      }
   }
   catch(PGraphException e)
   {
      LOGERR("match3_OpTm", e.msg());
      throw SecondoException(e.msg());
   }
}

// register
struct match3_OpInfo : OperatorInfo {

   match3_OpInfo()
   {
      name      = "match3";
      signature = "pgraph x string";
      syntax    = "match3  ";
      meaning   = "Query Graph";
   }
}; 

//-----------------------------------------------------------------------------
// OPERATOR test1
//-----------------------------------------------------------------------------


// typemapping
ListExpr test1_OpTm( ListExpr args )
{
   NList type(args);

   LOGOP(10,"test1_OpTm", "ARGS",nl->ToString(args));

   //cout << "relname:" << relName<<"\n";

   // return result 
   ListExpr res = nl->OneElemList(nl->SymbolAtom(CcBool::BasicType()) );
    
   return res;
}

// function
int test1_OpFun (Word* args, Word& result, int message,
              Word& local, Supplier s)
{
   LOG(10,"test1_OpFun");

   
   //PGraph *pg = static_cast<PGraph*>( args[0].addr );
   ListExpr type;
   Relation* rel=  QueryRelation("L1", type);
   cout << "EXPR: "<< nl->ToString(type) <<endl;
   GenericRelationIterator *it = rel->MakeScan();
   while (Tuple *t = it->GetNextTuple())
   {
      cout << "tuple "<<endl;
      t->DeleteIfAllowed();
   }

   // prepare result
   result = qp->ResultStorage(s);
   CcBool* res = static_cast<CcBool*>( result.addr );
   res->Set(true, true);
   return 0;
}

// register
struct test1_OpInfo : OperatorInfo {

   test1_OpInfo()
   {
      name      = "test1";
      signature = "pgraph -> " + CcBool::BasicType();
      syntax    = "_ test1";
      meaning   = "test1";
   }
}; 
//------------------------------------------------------------------------------

class PGraphAlgebra : public Algebra
{
 public:
  PGraphAlgebra() : Algebra()
  {


    AddTypeConstructor( &pgraphTC );
    pgraphTC.AssociateKind( Kind::SIMPLE() );

    AddOperator( info_OpInfo(), info_OpFun, info_OpTm );
    AddOperator( test1_OpInfo(), test1_OpFun, test1_OpTm );
    AddOperator( addnodesrel_OpInfo(), addnodesrel_OpFun, addnodesrel_OpTm );
    AddOperator( addedgesrel_OpInfo(), addedgesrel_OpFun, addedgesrel_OpTm);
    AddOperator( addnodeindex_OpInfo(), addnodeindex_OpFun, addnodeindex_OpTm);
    AddOperator( createpgraph_OpInfo(), createpgraph_OpFun, createpgraph_OpTm );
    AddOperator( loadgraph_OpInfo(), loadgraph_OpFun, loadgraph_OpTm );
    AddOperator( createmempgraph_OpInfo(), createmempgraph_OpFun, 
        createmempgraph_OpTm );
    Operator*op = AddOperator( match1_OpInfo(), match1_OpFun, match1_OpTm );
    op->SetUsesArgsInTypeMapping();
    op = AddOperator( match2_OpInfo(), match2_OpFun, match2_OpTm );
    op->SetUsesArgsInTypeMapping();
    op = AddOperator( match3_OpInfo(), match3_OpFun, match3_OpTm );
    op->SetUsesArgsInTypeMapping();

  }
  ~PGraphAlgebra() {};
};



} // end of namespace 

extern "C"
Algebra*
InitializePGraphAlgebra( NestedList* nlRef,
                               QueryProcessor* qpRef )
{
  return new pgraph::PGraphAlgebra;
}

