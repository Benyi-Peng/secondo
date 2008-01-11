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

\vspace{1cm}
\centerline{\LARGE \bf  SecondoPL}

\begin{center}
\footnotesize
\tableofcontents
\end{center}

1 Overview

This is the source code of a PROLOG shell which has
support for calling Secondo from PROLOG.

2 Includes and defines

*/

#include <string.h>
#include <iostream>
#include <list>

#include "stdlib.h"

#include "SWI-Prolog.h"
#include "SecondoPL.h"

using namespace std;

#include "NestedList.h"
#include "NList.h"
#include "SecondoInterface.h"
#include "Profiles.h"
#include "LogMsg.h"
#include "License.h"
#include "TTYParameter.h"


#ifdef SECONDO_USE_ENTROPY
#include "../Optimizer/Entropy/entropy.h"
#endif

SecondoInterface* si = 0;
NestedList* plnl = 0;

int lastErrorCode = 0;
string lastErrorMessage = "";

/*

3 Function handle\_exit   

This function is registerd as exit handler in the main function.

*/
void handle_exit(void) {
  
  /* PROLOG interpreter has terminated, shutdown Secondo */
  if(si != 0)
  {
    si->Terminate();
    delete si;
  };

}


/*

3 Function ListExprToTerm

Converts a Secondo list expression to a PROLOG term.

*/
term_t
ListExprToTerm(ListExpr expr, NestedList* nl)
{
  ListExpr current;
  int length;
  int i;
  std::list<ListExpr> listElements;
  std::list<ListExpr>::iterator iter;

  long intValue;
  double realValue;
  bool boolValue;
  string stringValue;
  string stringRepr;
  TextScan scan;

  term_t elem;
  term_t result = PL_new_term_ref();

  if(nl->IsAtom(expr))
  {
    switch(nl->AtomType(expr))
    {
      case IntType:
        intValue = nl->IntValue(expr);
        PL_put_integer(result, intValue);
        break;

      case RealType:
        realValue = nl->RealValue(expr);
        PL_put_float(result, realValue);
        break;

      case BoolType:
        boolValue = nl->BoolValue(expr);
        PL_put_atom_chars(result, boolValue ? "true" : "false");
        break;

      case StringType:
        stringValue = nl->StringValue(expr);
        stringRepr = string("\"") + stringValue + '\"';
        PL_put_atom_chars(result, stringRepr.c_str());
        break;

      case SymbolType:
        stringValue = nl->SymbolValue(expr);
        PL_put_atom_chars(result, stringValue.c_str());
        break;

      case TextType:
        scan = nl->CreateTextScan(expr);
        nl->GetText(scan, nl->TextLength(expr) + 2, stringValue);
        nl->DestroyTextScan(scan);
        PL_put_atom_chars(result, stringValue.c_str());
        break;

      default:
        /* this should not happen */
        assert(false);
    }
  }
  else
  {
    PL_put_nil(result);
    current = expr;
    length = nl->ListLength(current);

    for(i = 0; i < length; i++)
    {
      listElements.push_front(nl->First(current));
      current = nl->Rest(current);
    }

    for(iter = listElements.begin(); iter != listElements.end(); iter++)
    {
      elem = ListExprToTerm(*iter, nl);
      PL_cons_list(result, elem, result);
    }
  }

  return result;
}

/*

4 Function AtomToListExpr

Converts a PROLOG atom (represented as a string) to
a ListExpr.

*/
ListExpr
AtomToListExpr(NestedList* nl, char* str, bool& error)
{
  ListExpr result;
  string atomStr;

  error = false;

  if(strcmp(str, "true") == 0)
  {
    result = nl->BoolAtom(true);
  }
  else if(strcmp(str, "false") == 0)
  {
    result = nl->BoolAtom(false);
  }
  else if(str[0] == '\"')
  {
    str++;
    atomStr = str;

    if(atomStr.size() > 0 && atomStr[atomStr.size() - 1] == '\"')
    {
      atomStr.erase(atomStr.size() - 1);
      result = nl->StringAtom(atomStr);
    }
    else
    {
      error = true;
      result = nl->TheEmptyList();
    }
  }
  else
  {
    result = nl->SymbolAtom(string(str));
  }

  return result;
}

/*

4 Function TermToListExpr

Converts a PROLOG term to a ListExpr.

*/
ListExpr
TermToListExpr(term_t t, NestedList* nl, bool& error)
{
  long intValue;
  double realValue;
  char* charValue;
  char* strValue;
  size_t len;

  term_t head;
  term_t list;

  std::list<ListExpr> elementList;
  std::list<ListExpr>::iterator iter;

  ListExpr result;

  error = false;

  switch(PL_term_type(t))
  {
    case PL_VARIABLE:
      error = true;
      result = nl->TheEmptyList();
      break;

    case PL_INTEGER:
      PL_get_long(t, &intValue);
      result = nl->IntAtom(intValue);
      break;

    case PL_FLOAT:
      PL_get_float(t, &realValue);
      result = nl->RealAtom(realValue);
      break;

    case PL_ATOM:
      PL_get_atom_chars(t, &charValue);
      result = AtomToListExpr(nl, charValue, error);
      break;

    case PL_STRING:
      PL_get_string_chars(t, &charValue, &len);
      strValue = new char[len + 1];
      strValue[len] = 0;
      memcpy(strValue, charValue, len);
      result = AtomToListExpr(nl, strValue, error);
      delete[] strValue;
      break;

    case PL_TERM:
      if(PL_is_list(t))
      {
        result = nl->TheEmptyList();
        head = PL_new_term_ref();
        list = PL_copy_term_ref(t);

        while(PL_get_list(list, head, list))
        {
          elementList.push_front(TermToListExpr(head, nl, error));
          if(error)
            break;
        }

        if(!error)
        {
          for(iter = elementList.begin(); iter != elementList.end(); iter++)
          {
            result = nl->Cons(*iter, result);
          }
        }
        else
        {
          result = nl->TheEmptyList();
        }
      }
      else
      {
        error = true;
        result = nl->TheEmptyList();
      }
      break;

    default:
      assert(false); /* should not happen */
  }

  return result;
}

// some code below can only be translated
// when the Optimizaton-Library (OPT++) is
// present
#ifdef SECONDO_USE_ENTROPY

/*

4 Function FloatListToVector

Converts a PROLOG list of float numbers to a Vector of float numbers (double).

*/
void
FloatListToVector(term_t t, std::vector<double>& v, bool& error)
{
  error = 1;
  if( PL_is_list(t) )
  {
    term_t head = PL_new_term_ref();
    term_t list = PL_copy_term_ref(t);

    v.clear();
    while( PL_get_list(list, head, list) )
    {
      double d;

      if ( PL_get_float(head, &d) )
        v.push_back(d);
      else
        return;
    }

    error = 0;
  }
}

/*

4 Function FloatListPairToVectorPair

Converts a PROLOG list of (integer,float) pair to a vector of (int,float) pair.

*/
void
FloatListPairToVectorPair( term_t t, 
                           ProbabilityPairVec& v, 
                           bool& error                        )
{
  error = 1;
  if( PL_is_list(t) )
  {
    // Outer list
    term_t o_head = PL_new_term_ref();
    term_t o_list = PL_copy_term_ref(t);

    v.clear();
    while( PL_get_list(o_list, o_head, o_list) )
    {
      ProbabilityPair p;
      term_t i_head = PL_new_term_ref();
      term_t i_list = PL_copy_term_ref(o_head);

      PL_get_list(i_list, i_head, i_list);
      if ( PL_get_integer(i_head, &p.first) )
        if (    PL_get_list(i_list, i_head, i_list) 
             && PL_get_float(i_head, &p.second)     )
          v.push_back(p);
        else
          return;
      else
        return;
    }

    error = 0;
  }
}

/*
4 Function FloatVectorToList

Converts a Vector of float numbers to a PROLOG list of float numbers (double).

*/
void
FloatVectorToList(ProbabilityVec& v,term_t& t, bool& error)
{
  term_t list = PL_copy_term_ref(t);
  term_t head = PL_new_term_ref();
  ProbabilityVec::iterator iter;

  error = 1;
  for( iter = v.begin(); iter != v.end(); iter++ )
  {
    if ( !PL_unify_list(list, head, list) || !PL_unify_float(head, *iter) )
      return;
  }

  error = !PL_unify_nil(list);
}

/*
4 Function FloatVectorPairToListPair

Converts a vector of (int,float) numbers to a PROLOG list of (integer,float).

*/
void
FloatVectorPairToListPair( ProbabilityPairVec& v,
                           term_t& t, bool& error             )
{
  // Outer list
  term_t o_list = PL_copy_term_ref(t);
  term_t o_head = PL_new_term_ref();
  ProbabilityPairVec::iterator iter;

  error = 1;
  for( iter = v.begin(); iter != v.end(); iter++ )
  {
    if ( !PL_unify_list(o_list, o_head, o_list) ) return;

    term_t i_list = PL_copy_term_ref(o_head);
    term_t i_head = PL_new_term_ref();

    if ( !PL_unify_list(i_list, i_head, i_list) ) return;
    if ( !PL_unify_integer(i_head, iter->first) ) return;
    if ( !PL_unify_list(i_list, i_head, i_list) ) return;
    if ( !PL_unify_float(i_head, iter->second) ) return;
    if ( !PL_unify_nil(i_list) ) return;
  }

  error = !PL_unify_nil(o_list);
}

#endif

/*

4 Function pl\_print\_term\_le

Converts a PROLOG term to a ListExpr and then prints
that ListExpr using the routines from NestedList.

*/
static foreign_t
pl_print_term_le(term_t term)
{
  ListExpr listLE;
  bool error;

  listLE = TermToListExpr(term, plnl, error);
  if(error)
  {
    PL_fail;
  }
  else
  {
    // for debugging
    // plnl->WriteListExpr(listLE);
    PL_succeed;
  }
}

/*

4 Function pl\_get\_error\_info

Get error code (an integer) and error message (a string)
of the last issued Secondo command.

*/
static foreign_t
pl_get_error_info(term_t errorCode, term_t errorMessage)
{
  int unify1 = PL_unify_integer(errorCode, lastErrorCode);
 
  string msg = SecondoInterface::GetErrorMessage(lastErrorCode) + "\n" 
               + lastErrorMessage;
  
  if( (unify1 != 0) && PL_unify_atom_chars( errorMessage, msg.c_str()) != 0)
  {
    PL_succeed;
  }
  else
  {
    PL_fail;
  }
}

/*

4 Function pl\_call\_secondo

Call Secondo. The first argument must either be an atom
representing a query in text format or it must be
a PROLOG list. The result is a PROLOG nested list
which is unified with the second argument. The command level is
set to executable. If something goes wrong, the predicate fails and
error information can be obtained via predicate secondo\_error\_info.

*/
static foreign_t
pl_call_secondo(term_t command, term_t result)
{
  bool error = false;
  char* commandCStr;
  string commandStr;
  int commandLevel;

  ListExpr commandLE = plnl->TheEmptyList();
  ListExpr resultList = plnl->TheEmptyList();
  int errorPos;

  if(PL_get_atom_chars(command, &commandCStr))
  {
    error = false;
    commandStr = commandCStr;
    /* executable command in text syntax */
    commandLevel = 1;
  }
  else
  {
    commandLE = TermToListExpr(command, plnl, error);
    /* executable command in nested list syntax */
    commandLevel = 0;
    if (error)
    {
      cerr << "SecondoPL: TermToListExpr() failed." << endl; 
      PL_fail;
    }       
  }

  lastErrorCode = 0;
  si->Secondo(commandStr,
              commandLE,
              commandLevel,
              false,
              false,
              resultList,
              lastErrorCode,
              errorPos,
              lastErrorMessage);

  // reset NestedList pointer for the NList interface
  NList::setNLRef(plnl);

  if(lastErrorCode != 0)
  {
      PL_fail;
  }
  else
  {
    if(PL_unify(result, ListExprToTerm(resultList, plnl)) != 0)
    {
      plnl->initializeListMemory();
      PL_succeed;
    }
    else
    {
      cerr << "SecondoPL: Predicate secondo/2 failed, but error code was 0."
           << endl; 
      plnl->WriteListExpr(resultList);
    }
  }

  PL_fail;
}

/*

4 Function pl\_maximize\_entropy

Function to compute the remaining conditional probabilities. 
Syntax:

----
    maximize_entropy( [[1, p1] [2, p2] ... ], [[3, cp1], [5, cp2] ...], R )
----  

pi, cpi are floating point values.

*/
#ifdef SECONDO_USE_ENTROPY



static foreign_t
pl_maximize_entropy(term_t predicates, term_t probabilities, term_t result)
{
  // Type checking.
  if( !PL_is_list(predicates) || !PL_is_list(probabilities) )
    PL_fail;
  else
  {
    //ProbabilityVec vectorPredicates;
    ProbabilityPairVec vectorPredicates, vectorProbabilities, vectorResult;
    bool error1=false, error2=false;

    //FloatListToVector(predicates, vectorPredicates, error1);
    FloatListPairToVectorPair(predicates, vectorPredicates, error1);
    FloatListPairToVectorPair(probabilities, vectorProbabilities, error2);
    if( error1 || error2 )
      PL_fail;
    else
    {
      bool error=false;
      try
      {
        maximize_entropy( vectorPredicates, 
                          vectorProbabilities, 
                          vectorResult         );

        FloatVectorPairToListPair(vectorResult, result, error);
      }
      catch(...)
      {
        PL_fail;
      }

      if( error )
        PL_fail;
    }

    PL_succeed;
  }
}

#endif

PL_extension predicates[] =
{
  { "secondo", 2, (void*)pl_call_secondo, 0 },
  { "secondo_error_info", 2, (void*)pl_get_error_info, 0 },
  { "secondo_print_le", 1, (void*)pl_print_term_le, 0 },
#ifdef SECONDO_USE_ENTROPY
  { "maximize_entropy", 3, (void*)pl_maximize_entropy, 0 },
#endif
  { 0, 0, 0, 0 } /* terminating line */
};

/*

8 Function StartSecondoC

Starts Secondo. Assumes that the argument is the name of the
configuration file. Return true iff successful.

*/
bool
StartSecondoC(TTYParameter& tp)
{
  if ( !tp.CheckConfiguration() ) {
    return false;
  }  
  //tp.Print(cout);  

  si = new SecondoInterface();

  if(si->Initialize(tp.user, tp.pswd, tp.host, tp.port, tp.parmFile))
  {
    plnl = si->GetNestedList();
    NList::setNLRef(plnl);
    return true;
  }
  else
  {
    delete si;
    si = 0;
    cout
      << "Error while starting Secondo with config file "
      << tp.parmFile << "." << endl;
    return false;
  }
}


/* 

9. registerSecondo

This function registers the secondo predicate at the prolog engine.

*/

int registerSecondo(const char* user, const char* passwd){
  
  cout << "register secondo" << endl;
  
  TTYParameter tp(0,0);
  if ( string(user) != "" ) { 
    tp.user = user;
  }	    
  if ( string(passwd) != "" ) { 
    tp.pswd = passwd;
  }	    

  atexit(handle_exit);

  if( !StartSecondoC(tp) )
  {
    return -1;
  }

  /* Start PROLOG interpreter with our extensions. */
  PL_register_extensions(predicates);
  return 0;
}



