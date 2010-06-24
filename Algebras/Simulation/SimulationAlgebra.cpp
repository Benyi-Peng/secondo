/*
----
This file is part of SECONDO.

Copyright (C) 2007, University in Hagen, Department of Computer Science,
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

//paragraph [1] Title:  [{\Large \bf ]  [}]
//[->] [\ensuremath{\rightarrow}]

[1] Secondo SimulationAlgebra


April 04, 2007 C. Duentgen created the algebra

\begin{center}
\footnotesize
\tableofcontents
\end{center}


1 Overview

This algebra provides operators to create movement data for simulated vehicles.


2 Includes

We have to include some GSL headers and the GSLAlgebra.

*/

#include "Algebra.h"
#include "NestedList.h"
#include "QueryProcessor.h"
#include "StandardTypes.h"
#include "DateTime.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <math.h>
#include <unistd.h>
#include <errno.h>

#include <NList.h>

#include "RelationAlgebra.h"
#include "TemporalAlgebra.h"
#include "GSLAlgebra.h"

#ifndef SECONDO_WIN32
#define HAVE_INLINE
#endif
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>



/*
3. Static Objects and Parameter Settings

This algebra uses its own random number generator to stay independent from
other random generator states.

You can change most of the parameters at runtime using operators
~sim\_set\_rng~, ~sim\_set\_dest\_params~ and  ~sim\_set\_event\_params~.

*/

// setting up the RNG
unsigned long simRngSeed =  0; // 0 = use standard random seed
int           simRngType = 14; // index of random number generator to use
                               // is gsl_rng_mt19937
static GslRandomgen simRNG( simRngType, simRngSeed );

// parameter set for waiting at destination node
double sim_dest_param_mu = 15000; // mean for exponential [ms]
                                  //   distribution [ms]
double sim_dest_param_ss =  0.33; // probabilities for forced stops
double sim_dest_param_sm =  0.66; //   at crossings...
double sim_dest_param_sf =  1.00;
double sim_dest_param_ms =  0.33;
double sim_dest_param_mm =  0.50;
double sim_dest_param_mf =  0.66;
double sim_dest_param_fs =  0.05;
double sim_dest_param_fm =  0.33;
double sim_dest_param_ff =  0.10;

// maximum allowed velocities for different street types:
double sim_vmax_sidestreet = 30.0;
double sim_vmax_mainstreet = 50.0;
double sim_vmax_freeway    = 70.0;

// parameter set for subsegment events
double sim_event_param_subsegmentlength = 5.0; // maximum length of subsections
                                               //   in [m]
double sim_event_param_propconst        = 1.0; // constant c of proportionality
                                               //   p(event) = c/v_max
double sim_event_param_probstop         = 0.1; // prob.(event == forced stop)
                                               //   balanced by deceleration
double sim_event_param_acceleration     =12.0; // acceleration constant:
                                               //   increase step [m/s]
                                               //   within a single subsegment

// further parameters
double sim_startpoint_tolerance = 0.5; // tolerance for search of a line's
                                       // starting point. Set to negative value
                                       // for exact matching.

/*
4. General Typemapping Functions

*/

// real^N -> bool
template<int N>
    ListExpr sim_realN2bool_TM ( ListExpr args )
{
  ListExpr arg1, args2;
  std::ostringstream oss;
  oss << N;
//   cout << "sim_realN2bool_TM<" << N << "> called." << endl;
  if ( nl->ListLength( args ) == N ){
    args2 = args;
    for(int i=0; i<N; i++){
//       cout << "\targs[" << i << "] " << endl;
      arg1 = nl->First( args2 );
      args2 = nl->Rest( args );
      if ( !(nl->AtomType( arg1 ) == SymbolType) ||
             !(nl->SymbolValue( arg1 ) == "real")
         ){
        ErrorReporter::ReportError("SimulationAlgebra::sim_realN2bool_TM "
            "expects (real)^" + oss.str() + "." );
        return (nl->SymbolAtom( "typeerror" ));
      }
    }
    return (nl->SymbolAtom( "bool" ));
  }
  ErrorReporter::ReportError("SimulationAlgebra::sim_realN2bool_TM "
      "expects a list of length " + oss.str() + ".");
  return (nl->SymbolAtom( "typeerror" ));
}


/*
5. Operators

We will define the following operators:

----

 sim_set_rng:                int x int -> bool
 sim_set_dest_params:        real x real x real x real x real x real
                                  x real x real x real x real x real
                                  x real x real x real -> bool
 sim_set_event_params:       real x real x real x real -> bool
 sim_create_trip:            stream(tuple( ... (a_m line) ... (a_n real) ... ))
                                  x instant -> mpoint

----

*/

/*
5.1 Operator ~sim\_set\_rng~

Initialize the RNG with the passed type and random seed

*/

// int x int -> bool
ListExpr sim_set_rng_TM ( ListExpr args )
{
  ListExpr arg1, arg2;
  if ( nl->ListLength( args ) == 2 ){
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if ( nl->AtomType( arg1 ) == SymbolType &&
         nl->SymbolValue( arg1 ) == "int"  &&
         nl->AtomType( arg2 ) == SymbolType &&
         nl->SymbolValue( arg2 ) == "int" ) {
      return (nl->SymbolAtom( "bool" ));
    }
  }
  ErrorReporter::
      ReportError("SimulationAlgebra: sim_set_rng expected int x int");
  return (nl->SymbolAtom( "typeerror" ));
}

int sim_set_rng_VM ( Word* args, Word& result,
                        int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool *res = ((CcBool*)result.addr);
  CcInt *arg1 = ((CcInt*)args[0].addr);
  CcInt *arg2 = ((CcInt*)args[1].addr);
  if ( arg1->IsDefined() && arg2->IsDefined() ) {
    // overflow save implementation
    long  type = arg1->GetIntval();
    unsigned long seed = arg2->GetIntval();
    if( type >= 0 && type < GslRandomgen::rngType_getNoOfGenerators() ) {
      simRNG = GslRandomgen(type, seed);
      res->Set(true, true);
    } else {
      res->Set(true, false);
    }
  } else {
    res->Set(true, false);
  }
  return (0);
}

const string sim_set_rng_Spec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text>   int x int -> bool</text--->"
    "<text>sim_set_rng( Type , Seed )</text--->"
    "<text>Initialize the random number generator to be "
    "of type 'Type' and use a random seed 'Seed'.</text--->"
    "<text>query sim_set_rng( 14, 0 )</text--->"
    ") )";

Operator sim_set_rng(
    "sim_set_rng",
    sim_set_rng_Spec,
    sim_set_rng_VM,
    Operator::SimpleSelect,
    sim_set_rng_TM) ;

/*
5.2 Operator ~sim\_set\_dest\_params~

Set parameters for stops at the destination node

*/

int sim_set_dest_params_VM ( Word* args, Word& result,
                     int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool *res = ((CcBool*)result.addr);
  CcReal* cArgs[14];
  double dArgs[14];
  for(int i=0; i<14; i++) {
    cArgs[i] = (CcReal*) args[i].addr;
    if ( !cArgs[i]->IsDefined() ) {
      res->Set(true, false);
      ErrorReporter::ReportError("SimulationAlgebra::sim_set_dest_params "
          "expected defined param.");
      return 0;
    }
    dArgs[i] = cArgs[i]->GetRealval();
    if ( i>0 && (dArgs[i] < 0.0 || ( i<10 && dArgs[i] > 1.0 ) ) ) {
      res->Set(true, false);
      ErrorReporter::ReportError("SimulationAlgebra::sim_set_dest_params: "
          "Probability out of bounds");
      return 0;
    }
  }
  sim_dest_param_mu = dArgs[0]/86400000.0;
  sim_dest_param_ss = dArgs[1];
  sim_dest_param_sm = dArgs[2];
  sim_dest_param_sf = dArgs[3];
  sim_dest_param_ms = dArgs[4];
  sim_dest_param_mm = dArgs[5];
  sim_dest_param_mf = dArgs[6];
  sim_dest_param_fs = dArgs[7];
  sim_dest_param_fm = dArgs[8];
  sim_dest_param_ff = dArgs[9];
  sim_vmax_sidestreet = dArgs[10];
  sim_vmax_mainstreet = dArgs[11];
  sim_vmax_freeway    = dArgs[12];
  sim_startpoint_tolerance = dArgs[13];
  res->Set(true, true);
  return 0;
}

const string sim_set_dest_params_Spec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text> real x real x real x real x real x real x real x "
    " real x real x real x real x real x real x real -> bool</text--->"
    "<text>sim_set_dest_params( ExpMu, SS, SM, SF, MS, MM, MF, FS, FM, "
    "FF, VmaxS, VmaxM, VmaxF, SPT)"
    "</text--->"
    "<text>Set the parameters for stops at destination nodes: Set mean of "
    "exponential distribution of waiting times to 'ExpMu' (in milliseconds). "
    "Set probabilities "
    "for forced stops at transitions between street types. 'XY' means "
    "transition X -> Y, where S= small street, M= main street F= freeway. "
    "Observe 0.0 <= p <= 1.0 for all probabilities p. "
    "Set maximum allpwed velocities for sidestreets (VmaxS), mainstreets "
    "(VmaxM) and freewayf (VmaxF) [1000/h]. Set the startpoint tolerance "
    "to SPT [1]. Returns 'TRUE', iff the parameters have been set "
    "correctly.</text--->"
    "<text>query sim_set_dest_params( 1500.0, 0.33, 0.66, 1.0, 0.33, 0.5, "
    "0.66, 0.05, 0.33, 0.10, 30.0, 50.0, 70.0, 0.5 )</text--->"
    ") )";

Operator sim_set_dest_params(
    "sim_set_dest_params",
    sim_set_dest_params_Spec,
    sim_set_dest_params_VM,
    Operator::SimpleSelect,
    sim_realN2bool_TM<14>) ;

/*
5.3 Operator ~sim\_set\_event\_params~

Set parameters for enroute-event generation.

*/

int sim_set_event_params_VM ( Word* args, Word& result,
                             int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool *res = ((CcBool*)result.addr);
  CcReal* cArgs[4];
  double dArgs[4];
  for(int i=0; i<4; i++) {
    cArgs[i] = (CcReal*) args[i].addr;
    if ( !cArgs[i]->IsDefined() ) {
      res->Set(true, false);
      ErrorReporter::ReportError("SimulationAlgebra::sim_set_event_params "
          "expected defined param.");
      return 0;
    }
    dArgs[i] = cArgs[i]->GetRealval();
    if ( (i==0 || i==4) && dArgs[i] <= 0.0 ) {
      res->Set(true, false);
      ErrorReporter::ReportError("SimulationAlgebra::sim_set_event_params: "
          "1st parameter 'Length' must be positive!");
      return 0;
    }
    if ( i==1 && dArgs[i] < 0.0 ) {
      res->Set(true, false);
      ErrorReporter::ReportError("SimulationAlgebra::sim_set_event_params: "
          "2nd parameter 'Proportionality constant' must be <= 0.0!");
      return 0;
    }
    if ( i==2 && (dArgs[i] < 0.0 || dArgs[i] > 1.0 ) ) {
      res->Set(true, false);
      ErrorReporter::ReportError("SimulationAlgebra::sim_set_event_params: "
          "3rd parameter 'Probability' out of bounds!");
      return 0;
    }
  }
  sim_event_param_subsegmentlength = dArgs[0]; // maximum length of subsections
  sim_event_param_propconst        = dArgs[1]; // constant of proportionality
  sim_event_param_probstop         = dArgs[2]; // prob. for event = forced stop
  sim_event_param_acceleration     = dArgs[3]; // acceleration rate
                                    // [km/h/sim_event_param_subsegmentlength]
  res->Set(true, true);
  return 0;
}

const string sim_set_event_params_Spec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text> real x real x real x real -> bool</text--->"
    "<text>sim_set_event_params( Length, C, P, Acc)"
    "</text--->"
    "<text>Set the parameters for enroute-events: Routes will be divided into "
    "subsegments of maximum length 'Length'. The probability of an event is "
    "proprtional to C / Vmax. The probability for an event being a forced "
    "stop is given by 0.0 <= P <= 1.0 (the balance, 1-P, is meant to trigger "
    "deceleration events). Acceleration is set to 'Acc'."
    "Returns 'TRUE', iff the parameters have been set correctly.</text--->"
    "<text>sim_set_event_params( 5.0, 1.0, 0.1, 12.0 )</text--->"
    ") )";

Operator sim_set_event_params(
    "sim_set_event_params",
    sim_set_event_params_Spec,
    sim_set_event_params_VM,
    Operator::SimpleSelect,
    sim_realN2bool_TM<4>) ;


/*
5.3 Operator ~sim\_create\_trip~

Create a moving point from a stream of lines and a startung instant.

*/

ListExpr sim_create_trip_TM ( ListExpr args )
{
  int len = nl->ListLength(args);
  if(len != 5 && len != 6 ) {
    ErrorReporter::ReportError("five or six arguments expected");
    return nl->SymbolAtom("typeerror");
  }
  // extract the attribute names
  ListExpr a1list = nl->Second(args);
  ListExpr a2list = nl->Third(args);

  if(nl->AtomType(a1list)!=SymbolType) {
    ErrorReporter::ReportError("the second argument has to be a symbol");
    return nl->SymbolAtom("typeerror");
  }
  if(nl->AtomType(a2list)!=SymbolType) {
    ErrorReporter::ReportError("the third argument has to be a symbol");
    return nl->SymbolAtom("typeerror");
  }
  string a1 = nl->SymbolValue(a1list);
  string a2 = nl->SymbolValue(a2list);

  string restype="";
  int a1index = -1;
  int a2index = -1;

  ListExpr stype = nl->First(args);
  if(nl->AtomType(stype)!=NoAtom) {
    ErrorReporter::ReportError("stream(tuple(...))"
        " expected as the first argument");
    return nl->SymbolAtom("typeerror");
  }

  if((nl->ListLength(stype)!=2) ||
      (!nl->IsEqual(nl->First(stype),"stream" ))) {
    ErrorReporter::ReportError("stream(tuple(...))"
        " expected as the first argument");
    return nl->SymbolAtom("typeerror");
  }

  ListExpr ttype = nl->Second(stype);

  if((nl->ListLength(ttype)!=2) ||
      (!nl->IsEqual(nl->First(ttype),"tuple" ))) {
    ErrorReporter::ReportError("stream(tuple(...))"
        " expected as the first argument");
    return nl->SymbolAtom("typeerror");
  }

  ListExpr attributes = nl->Second(ttype);
  if(nl->AtomType(attributes)!=NoAtom) {
    ErrorReporter::ReportError("invalid tuple type");
    return nl->SymbolAtom("typeerror");
  }
  int pos = 0;
  while(!nl->IsEmpty(attributes)) {
    ListExpr attr = nl->First(attributes);
    if( (nl->AtomType(attr)!=NoAtom) ||
         (nl->ListLength(attr)!=2)) {
      ErrorReporter::ReportError("invalid tuple type");
      return nl->SymbolAtom("typeerror");
    }
    ListExpr anl = nl->First(attr);
    ListExpr atl = nl->Second(attr);
    if( (nl->AtomType(anl)!=SymbolType) ||
         (nl->AtomType(atl)!=SymbolType)) {
      ErrorReporter::ReportError("invalid tuple type");
      return nl->SymbolAtom("typeerror");
    }

    string aname = nl->SymbolValue(anl);
    if(aname==a1) {
      if(a1index>=0) {
        ErrorReporter::ReportError("attr name occurs twice");
        return nl->SymbolAtom("typeerror");
      }
      if(!nl->IsEqual(atl,"line")) {
        ErrorReporter::
            ReportError("first attr (Trajectory) has to be of type 'line'");
        return nl->SymbolAtom("typeerror");
      }
      a1index = pos;
    }

    if(aname==a2) {
      if(a2index >= 0) {
        ErrorReporter::ReportError("attr name occurs twice");
        return nl->SymbolAtom("typeerror");
      }
      string a2type = nl->SymbolValue(atl);
      if(a2type=="real") {
        restype = "mpoint";
      } else {
        ErrorReporter::
            ReportError("seocond attr (maximimum velocity) has to be of"
            " type 'real'");
        return nl->SymbolAtom("typeerror");
      }
      a2index = pos;
    }
    pos++;
    attributes = nl->Rest(attributes);
  }

  if(a1index<0) {
    ErrorReporter::ReportError("first attr name does"
        " not occur in the typle");
    return nl->SymbolAtom("typeerror");
  }

  if(a2index<0) {
    ErrorReporter::ReportError("second attr name does"
        " not occur in the typle");
    return nl->SymbolAtom("typeerror");
  }

  ListExpr arg5 = nl->Fifth(args);
  if ( !(nl->AtomType( arg5 ) == SymbolType) ||
         !(nl->SymbolValue( arg5 ) == "point")
     ) {
    ErrorReporter::ReportError("fifth argument must be of type 'point'" );
    return (nl->SymbolAtom( "typeerror" ));
  }


  ListExpr ind;

  if (len == 6 ) {// explicit starting velocity
    ListExpr arg6 = nl->Sixth(args);
    if ( !(nl->AtomType( arg6 ) == SymbolType) ||
           !(nl->SymbolValue( arg6 ) == "real")
       ) {
      ErrorReporter::
          ReportError("optional sixth argument must be of type 'real'" );
      return (nl->SymbolAtom( "typeerror" ));
    }
    ind = nl->TwoElemList(nl->IntAtom(a1index),
                          nl->IntAtom(a2index));
  } else {// all is correct, starting velocity == 0.0
    ind = nl->ThreeElemList(nl->RealAtom(0.0),
                            nl->IntAtom(a1index),
                            nl->IntAtom(a2index));
  }

  return nl->ThreeElemList(nl->SymbolAtom("APPEND"),
                           ind,
                           nl->SymbolAtom(restype));
}


struct Subsegment
{
  Point start;
  Point end;
};



int sim_create_trip_VM ( Word* args, Word& result,
                         int message, Word& local, Supplier s )
{
//   cout << "sim_create_trip_VM received message: ";
//   if (message == OPEN) cout << "OPEN" << endl;
//   else if (message == REQUEST) cout << "REQUEST" << endl;
//   else if (message == CLOSE) cout << "CLOSE" << endl;
//   else if (message == YIELD) cout << "YIELD" << endl;
//   else if (message == CANCEL) cout << "CANCEL" << endl;
//   else if (message == CARDINALITY) cout << "CARDINALITY" << endl;
//   else if (message == PROGRESS) cout << "PROGRESS" << endl;
//   else cout << "(unknown message)" << endl;

  result = qp->ResultStorage( s );
  MPoint*        res = ((MPoint*)result.addr);
  CcInt*  cLineIndex = (CcInt*) args[6].addr;
  CcInt*  cVmaxIndex = (CcInt*) args[7].addr;
  Instant* instStart = (Instant*) args[3].addr;
  Point*  pointStart = (Point*) args[4].addr;
  CcReal*    cVstart = (CcReal*) args[5].addr;
  long    tuplesReceived = 0;
  long    tuplesAccepted = 0;
  long invalidUnitsCreated = 0;

  res->Clear();
  res->SetDefined( true );
  res->StartBulkLoad();

  if( instStart->IsDefined() && pointStart->IsDefined() &&
      cVstart->IsDefined() &&
      cLineIndex->IsDefined() && cVmaxIndex->IsDefined() ){
    vector<Subsegment> subsegments(0);
    int          lineIndex = cLineIndex->GetIntval();
    int          VmaxIndex = cVmaxIndex->GetIntval();
    Instant    currentInst = *instStart;
    double     currentVmax = 0.0;
    Instant    currentTime = *instStart;
    Point  currentPosition = *pointStart;
    double    currentSpeed = cVstart->GetRealval();
    double       localVmax = 0.0;
    double    lastMaxSpeed = -1.0;
    double       lastAlpha = 0.0;
    bool     stopAfterThis = false;
    Point       startPoint(true,0,0);
    Point         endPoint(true,0,0);
    DateTime dummyDuration(0,0,durationtype);
    Line* currentLineC = 0;
    SimpleLine   currentLine(0);
    Word      wActualTuple;
    qp->Open(args[0].addr);
    qp->Request(args[0].addr, wActualTuple);
    while (qp->Received(args[0].addr)) {
      tuplesReceived++;
      // speed == 0.0 signals to wait for a random duration before the
      // movement may be continued!
      Tuple* tuple         = (Tuple*) (wActualTuple.addr);
      currentLineC         = (Line*)  (tuple->GetAttribute(lineIndex));
      currentLine.fromLine(*currentLineC);
      CcReal *CcurrentVmax = (CcReal*)(tuple->GetAttribute(VmaxIndex));

      // search for the first segment (starts with point currentPosition)
      assert( currentPosition.IsDefined() );
      if( currentLine.IsDefined()  &&
          CcurrentVmax->IsDefined() &&
          !currentLine.IsEmpty()   &&
          currentLine.SelectInitialSegment(currentPosition,
                                            sim_startpoint_tolerance) ) {
        // all args defined and current position found in currentLine
        tuplesAccepted++;
        currentVmax = CcurrentVmax->GetRealval();
        startPoint = currentPosition;
        endPoint = currentPosition;
        while( currentLine.getWaypoint(endPoint) ) { // for each line segment:
          assert( endPoint.IsDefined() );
          Subsegment s;
          double l =  startPoint.Distance(endPoint);
          double incrX = sim_event_param_subsegmentlength
                * (endPoint.GetX() - startPoint.GetX()) / l;
          double incrY = sim_event_param_subsegmentlength
                * (endPoint.GetY() - startPoint.GetY()) / l;
          if(AlmostEqual(incrX, 0.0) && AlmostEqual(incrY, 0.0)) {
            cout << "  X- and Y-Increment == 0.0! -> BREAK" << endl;
            break;
          }
          Point interimStart = startPoint;
          Point interimEnd   = startPoint;
          while( interimEnd.Distance(endPoint) >=
                 sim_event_param_subsegmentlength ) {
            // divide the remaining segment into subsegments
            interimEnd.Translate(incrX,incrY);
            s.start    = interimStart;
            s.end      = interimEnd;
            subsegments.push_back(s);
            interimStart = interimEnd;
          }
          // add the last subsegment
          s.start    = interimStart;
          s.end      = endPoint;
          assert( endPoint.IsDefined() );
          subsegments.push_back(s);
          startPoint = endPoint;
          currentLine.SelectSubsequentSegment();
        }
        // iterate vector to determine the speed at the end of each subsegment:
        for(unsigned int i=0 ; i<subsegments.size(); i++) {
          localVmax = currentVmax; // base for further calculations
          if ( lastMaxSpeed >= 0.0 ) {
            // Special Case:
            // Handle last subsegment of preceeding line:
            localVmax = lastMaxSpeed; // recall lastMaxSpeed for
                                      // reenqueued subsegment
            // check whether to stop at the crossing
            double pWait = 0.0;
            if ( AlmostEqual(lastMaxSpeed,sim_vmax_sidestreet) ) {
              if ( AlmostEqual(currentVmax,sim_vmax_sidestreet) ) {
                pWait = sim_dest_param_ss;
              }
              else if ( AlmostEqual(currentVmax,sim_vmax_mainstreet) ) {
                pWait = sim_dest_param_sm;
              }
              else if ( AlmostEqual(currentVmax,sim_vmax_freeway) ) {
                pWait = sim_dest_param_sf;
              }
              else {
                cout << " S->??? "; pWait = 0.0;
              }
            }
            else if ( AlmostEqual(lastMaxSpeed,sim_vmax_mainstreet) ) {
              if ( AlmostEqual(currentVmax,sim_vmax_sidestreet) ) {
                pWait = sim_dest_param_ms;}
                else if ( AlmostEqual(currentVmax,sim_vmax_mainstreet) ) {
                  pWait = sim_dest_param_mm;
                }
                else if ( AlmostEqual(currentVmax,sim_vmax_freeway) ) {
                  pWait = sim_dest_param_mf;
                }
                else {
                  cout << " M->? "; pWait = 0.0;
                }
            }
            else if ( AlmostEqual(lastMaxSpeed,sim_vmax_freeway) ) {
              if ( AlmostEqual(currentVmax,sim_vmax_sidestreet) ) {
                pWait = sim_dest_param_fs;
              }
              else if ( AlmostEqual(currentVmax,sim_vmax_mainstreet) ) {
                pWait = sim_dest_param_fm;
              }
              else if ( AlmostEqual(currentVmax,sim_vmax_freeway) ) {
                pWait = sim_dest_param_ff;
              }
              else {
                cout << " F->? "; pWait = 0.0;
              }
            } else {
              cout << " ?->? ";
              pWait = 0.0;
            }
            if(simRNG.NextReal() <= pWait) {
              // Force waiting at the crossing...
              stopAfterThis = true;
            }
            lastMaxSpeed = -1.0; // avoid double waits
          }
          if ( AlmostEqual(currentSpeed, 0.0) ) {
            // speed == 0.0 indicates, that we have to wait,
            // before we may continue the voyage:
            // Determine waiting duration using exponential distribution:
            double waittime =
                  gsl_ran_exponential(simRNG.GetGenerator(),
                                      sim_dest_param_mu/86400000.0);
            dummyDuration.ReadFrom(waittime);
            if( dummyDuration > DateTime(0,0,durationtype) ) {
              const Interval<Instant>
                  interval( currentInst,
                            currentInst + dummyDuration,
                            true,
                            false );
              UPoint up( interval,
                         subsegments[i].start, subsegments[i].start);
              if( up.IsValid() ) {
                res->MergeAdd(up);
                currentInst += dummyDuration;
              } else {
                invalidUnitsCreated++;
                cout << "Invalid unit up = "; up.Print(cout); cout << endl;
              }
            }
          }
          if (i<subsegments.size()-1) {
            // This is not the last subsegment for this line
            if( simRNG.NextReal() <= (sim_event_param_propconst/currentVmax) ) {
              // An event occurrs
              if( simRNG.NextReal() <= (sim_event_param_probstop) ) {
                // forced stop after this sub-segment
                stopAfterThis = true;
              } else {
                // forced deceleration. Use binomial distrib.
                // to determine amount of speed loss
                localVmax =
                    localVmax * gsl_ran_binomial(simRNG.GetGenerator(),
                    0.5,
                    20)
                    /20.0;
                localVmax = MAX(localVmax, 1.0);
              }
            } else {
              // no event: accelerate up to localVmax
              localVmax =
                  MIN(currentSpeed+sim_event_param_acceleration, currentVmax);
            }
            // calculate steepness of curves
            double alpha =
                  !AlmostEqual(subsegments[i].end, subsegments[i+1].end)
                  ? subsegments[i].end.Direction(subsegments[i+1].end)
              : lastAlpha;
            double curveMax =
                  (1.0-( fmod(fabs(lastAlpha-alpha), 180.0) )/180.0)
                  * currentVmax;
            localVmax = MIN( localVmax, curveMax );
            lastAlpha = alpha;
            currentSpeed = localVmax;
          } else if (i == subsegments.size()-1 ) {
            // This is the last subsegment. Delete all subsegments,
            // but the last one
            Subsegment lastSubSeg = subsegments[i];
            subsegments.clear();
            subsegments.push_back(lastSubSeg);
            lastMaxSpeed = currentVmax; // prepare special treatment
                                        // of this subsegment
            currentPosition = subsegments.back().end; //required to find next
                                            // subsegment within the next line
            break; // this exits the for()-loop
          }
          // Create a unit for the current subsegment
          double dist = subsegments[i].start.Distance(subsegments[i].end);
          dummyDuration.ReadFrom(dist/currentSpeed/24000L);
            // ^^^^ time used to travel the subsegment t=s/v ^^^
          const Interval<Instant>
              interval( currentInst,
                        currentInst + dummyDuration,
                        true,
                        false );
          UPoint up( interval, subsegments[i].start, subsegments[i].end);
          if( up.IsValid() ) {
            res->MergeAdd(up);
            currentInst += dummyDuration;
          } else {
            invalidUnitsCreated++;
//             cout << "Invalid unit up = "; up.Print(cout); cout << endl;
          }
          currentPosition = subsegments[i].end;
          if (stopAfterThis) {
            currentSpeed = 0.0;
            stopAfterThis = false;
          }
        } // endfor(unsigned int i=0 ; i<subsegments.size(); i++)
        assert( currentPosition.IsDefined() );
      } // endif( currentLine->SelectInitialSegment(currentPosition) )
      else { // undef/empty/complex/cyclic line or undef vmax or cuppentPos
        //            not found: do nothing
        cout << "current position (= " ;
        currentPosition.Print(cout);
        cout << " ) NOT found in currentLine" << endl;
      }
      tuple->DeleteIfAllowed();
      qp->Request(args[0].addr, wActualTuple);
    }
    // create and append the final unit
    if( subsegments.size() == 1 ) {
      // there is a subsegment left. Nothing happens here.
      currentSpeed = MAX(currentSpeed, 5.0);
      double dist = subsegments[0].start.Distance(subsegments[0].end);
      dummyDuration.ReadFrom(dist/currentSpeed/24000); // time used to travel
                                                     // the subsegment t=s/v
      const Interval<Instant>
          interval( currentInst,
                    currentInst + dummyDuration,
                    true,
                    false );
      UPoint up( interval, subsegments[0].start, subsegments[0].end);
      if( up.IsValid() ) {
        res->MergeAdd(up);
        currentInst += dummyDuration;
      } else {
        invalidUnitsCreated++;
        cout << "Invalid unit up = "; up.Print(cout); cout << endl;
      }
      currentPosition = subsegments.back().end; // set currentPosition to
    } // endwhile (qp->Received(args[0].addr))
    else {
      // ( subsegments.size() != 1 )
      cout << "sim_create_trip_VM: Something's wrong: subsegments.size() = "
           << subsegments.size() << "." << endl;
    }
    qp->Close(args[0].addr);
  }
  else
  { // some undef argument:
    cout << "sim_create_trip_VM: undefined Argument: " << endl;
    cout << "  StartInstant:  " << instStart->IsDefined() << endl;
    cout << "  StartPoint:    " << pointStart->IsDefined() << endl;
    cout << "  StartVelocity: " << cVstart->IsDefined() << endl;
    cout << "  LineIndex:     " << cLineIndex->IsDefined() << endl;
    cout << "  VelocityIndex: " << cVmaxIndex->IsDefined() << endl;
    res->SetDefined( false );
  }
  res->EndBulkLoad(false,true);
//   cout << "sim_create_trip_VM: Finished!" << endl;
//   cout << "  tuplesReceived = " << tuplesReceived << endl;
//   cout << "  tuplesAccepted = " << tuplesAccepted << endl;
//   cout << "  units created  = " << res->GetNoComponents() << endl;
//   cout << "  invalidUnitsCreated = " << invalidUnitsCreated << endl << endl;
//   cout << "sim_create_trip_VM: returned result.";
  return 0;
}

const string sim_create_trip_Spec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" ) "
    "( <text>stream(tuple(a1: t1) ... (an,tn) x ai x aj x instant x point -> "
    "mpoint,\nfor ti = line, tj = real, tk = point</text--->"
    "<text>_ sim_create_trip [LineAttr, VmaxAttr, StartInst, StartPoint ] \n"
    " _ sim_create_trip [LineAttr, VmaxAttr, StartInst, StartPoint, Vstart ]"
    "</text--->"
    "<text>Creates a mpoint value representing a simulated vehicle, "
    "starting at instant 'StartInst' and 'StartPoint' and moving along a "
    "trajectory formed by the stream of lines received from the stream "
    "argument. The stream contains tuples describing subsequent parts of the "
    "trajectory and the maximum allowed speed for that street section. The "
    "velocity of the vehicle reflects street topology and random events. "
    "The sixth argument 'Vstart' is optional and sets the vehicle's initial "
    "velocity. If omitted, Vmax is set to 0.0. </text--->"
    "<text>query _ sim_create_trip[ _ ]</text--->"
    ") )";

Operator sim_create_trip(
    "sim_create_trip",
    sim_create_trip_Spec,
    sim_create_trip_VM,
    Operator::SimpleSelect,
    sim_create_trip_TM) ;


/*
5.4 Operator ~sim\_print\_params~

Prints the parameter settings to the console.

*/

ListExpr sim_empty2bool_TM ( ListExpr args )
{
  if ( nl->ListLength( args ) == 0 ) {
    return (nl->SymbolAtom( "bool" ));
  }
  ErrorReporter::ReportError("SimulationAlgebra::sim_empty2bool_TM "
      "expects a list of length 0.");
  return (nl->SymbolAtom( "typeerror" ));
}


int sim_print_params_VM ( Word* args, Word& result,
                              int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  CcBool *res = ((CcBool*)result.addr);

  cout << endl;
  cout << "***********************************************************" <<endl;
  cout << "* SimulationAlgera   Parameter Settings *" << endl;
  cout << "***********************************************************" <<endl;
  cout << "* RANDOM NUMBER GENERATOR:" << endl;
  cout << "* simRngSeed = " << simRngSeed << endl;
  cout << "* simRngType = " << simRngType << endl;
  cout << "***********************************************************" <<endl;
  cout << "* CROSSING PARAMETERS" << endl;
  cout << "* sim_dest_param_mu = " << sim_dest_param_mu << "[ms]" << endl;
  cout << "* sim_dest_param_ss = " << sim_dest_param_ss<< "[0,1]" << endl;
  cout << "* sim_dest_param_sm = " << sim_dest_param_sm<< "[0,1]" << endl;
  cout << "* sim_dest_param_sf = " << sim_dest_param_sf<< "[0,1]" << endl;
  cout << "* sim_dest_param_ms = " << sim_dest_param_ms<< "[0,1]" << endl;
  cout << "* sim_dest_param_mm = " << sim_dest_param_mm<< "[0,1]" << endl;
  cout << "* sim_dest_param_ms = " << sim_dest_param_mf<< "[0,1]" << endl;
  cout << "* sim_dest_param_fs = " << sim_dest_param_fs<< "[0,1]" << endl;
  cout << "* sim_dest_param_fm = " << sim_dest_param_fm<< "[0,1]" << endl;
  cout << "* sim_dest_param_ff = " << sim_dest_param_ff<< "[0,1]" << endl;
  cout << "* sim_vmax_sidestreet = " << sim_vmax_sidestreet<< "[km/h]" <<endl;
  cout << "* sim_vmax_mainstreet = " << sim_vmax_mainstreet<< "[km/h]" <<endl;
  cout << "* sim_vmax_freeway    = " << sim_vmax_freeway<< "[km/h]" << endl;
  cout << "* sim_startpoint_tolerance = " << sim_vmax_freeway<< "[>0.0]"
      << endl;
  cout << "***********************************************************" <<endl;
  cout << "* SUBSEGMENT PARAMETERS" << endl;
  cout << "* sim_event_param_subsegmentlength = "
      << sim_event_param_subsegmentlength<< "[m]" << endl;
  cout << "* sim_event_param_propconst        = " << sim_event_param_propconst
      << "[>0]" << endl;
  cout << "* sim_event_param_acceleration     = "
      << sim_event_param_acceleration << "[m/s^2]" << endl;
  cout << "***********************************************************" <<endl;
  res->Set(true, true);
  return 0;
}

const string sim_print_params_Spec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text> -> bool</text--->"
    "<text>sim_print_params( )</text--->"
    "<text>Prints the paramter settings to the console. Always "
    "returns 'TRUE'</text--->"
    "<text>sim_print_params()</text--->"
    ") )";

Operator sim_print_params(
    "sim_print_params",
    sim_print_params_Spec,
    sim_print_params_VM,
    Operator::SimpleSelect,
    sim_empty2bool_TM) ;


/*
5.5 Operator ~sim\_fillup\_mpoint~

Fills the undefined intervals within an mpoint with the known positions.
immedeately before/after the ``dark periods''.

----
    sim_fillup_mpoint: (mpoint x instant x instant x bool x bool x bool) --> mpoint

----

*/

ListExpr sim_fillup_mpoint_TM ( ListExpr args )
{
  ListExpr arg1, arg2, arg3, arg4, arg5, arg6;
  if ( nl->ListLength( args ) == 6 ) {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    arg3 = nl->Third( args );
    arg4 = nl->Fourth( args );
    arg5 = nl->Fifth( args );
    arg6 = nl->Sixth( args );

    if ( nl->AtomType( arg1 ) == SymbolType &&
         nl->SymbolValue( arg1 ) == "mpoint"  &&
         nl->AtomType( arg2 ) == SymbolType &&
         nl->SymbolValue( arg2 ) == "instant" &&
         nl->AtomType( arg3 ) == SymbolType &&
         nl->SymbolValue( arg3 ) == "instant" &&
         nl->AtomType( arg4 ) == SymbolType &&
         nl->SymbolValue( arg4 ) == "bool" &&
         nl->AtomType( arg5 ) == SymbolType &&
         nl->SymbolValue( arg5 ) == "bool" &&
         nl->AtomType( arg6 ) == SymbolType &&
         nl->SymbolValue( arg6 ) == "bool" ) {
      return (nl->SymbolAtom( "mpoint" ));
    }
  }
  ErrorReporter::
      ReportError("SimulationAlgebra: sim_fillup_mpoint expected "
                  "mpoint x instant x instant x bool x bool x bool");
  return (nl->SymbolAtom( "typeerror" ));
}

int sim_fillup_mpoint_VM ( Word* args, Word& result,
                           int message, Word& local, Supplier s )
{
  result = qp->ResultStorage( s );
  MPoint *res = ((MPoint*)result.addr);
  res->Clear();

  MPoint *Input  = (MPoint*) args[0].addr;
  Instant *Start = (Instant*) args[1].addr;
  Instant *End   = (Instant*) args[2].addr;
  CcBool *LC = (CcBool*) args[3].addr;
  CcBool *RC = (CcBool*) args[4].addr;
  CcBool *CONN = (CcBool*) args[5].addr;

  if( !Input->IsDefined() ||
      !LC->IsDefined() || !RC->IsDefined() ||
      !Start->IsDefined() || !End->IsDefined() ||
      *Start > *End ||
      ( *Start == *End && (!LC->GetBoolval() || !RC->GetBoolval())) ||
      !CONN->IsDefined()
    ) {
    // undefined arguments: return undefined and empty result
    res->SetDefined(false);
    return 0;
  }
  res->SetDefined(true);
  int size = Input->GetNoComponents();
  if( size == 0 ) {
    // empty input: return defined and empty result
    res->SetDefined(true);
    return 0;
  }

  UPoint u1(true), u2(true), resunit(true);
  Interval<DateTime> gap(*Start,*Start,true,true);
  res->SetDefined(true);

  bool connect = CONN->GetBoolval();
  int pos = 0;
  //test whether to insert a prequel unit
  Input->Get(pos, u1);
//   cout << "First Unit = "; u1.Print(cout);
  if ( *Start < u1.timeInterval.start ) {
    // Start before first unit
    gap = Interval<DateTime>( *Start, u1.timeInterval.start,
                              LC->GetBoolval(), !u1.timeInterval.lc );
    resunit = UPoint(gap, u1.p0, u1.p0);
//     cout << "  Inserting prequel unit: "; resunit.Print(cout);
    res->MergeAdd( resunit );
    u2 = u1;
    pos++;
  } else if ( *Start == u1.timeInterval.start &&
            LC->GetBoolval() &&
            !u1.timeInterval.lc
          ) {
    // extend first unit by changing lc to true
    resunit = UPoint(Interval<DateTime>(u1.timeInterval.start,
                                        u1.timeInterval.end,
                                        true,
                                        u1.timeInterval.rc),
                      u1.p0, u1.p0);
    u2 = resunit;
//     cout << "  >>Extended first unit: "; resunit.Print(cout);
    pos++;
  } else {
    // just prepare insertion of first unit
    u2 = u1;
    pos++;
//     cout << "  >>Nothing to do" << endl;
  }
  while(pos < size) {
    Input->Get(pos, u1);
//     cout << "pos/size = " << pos << "/" << size << endl;
//     cout << "   u2 = "; u2.Print(cout);
//     cout << "   u1 = "; u1.Print(cout);
    if ( u2.timeInterval.end == u1.timeInterval.start ) {
      // Case 1): u2.end == u1.start
      if ( !u2.timeInterval.rc && !u1.timeInterval.lc ) {
        // 1a) minimum gap between open intervals
        // --> make u2 rightclosed and append it.
        u2.timeInterval.rc = true;
//         cout << "   Case 1a): Adding: "; u2.Print(cout);
        res->MergeAdd( u2 );
        u2 = u1;
        pos++;
      } else {
        // 1b) u2.rc != u1.lc: No gap
        // --> just add the original unit u2
//         cout << "   Case 1b): Adding: "; u2.Print(cout);
        res->MergeAdd( u2 );
        u2 = u1;
        pos++;
      }
    } else if( u2.timeInterval.end < u1.timeInterval.start ) {
      // Case 2) simple case: start and ending instant are not equal
      // (large gap of at least nearly 1 tick/ms)
      // --> append u2 and a unit for the gap
//       cout << "   Case 2) : Adding: "; u2.Print(cout);
      res->MergeAdd( u2 );
      resunit = UPoint( Interval<DateTime>( u2.timeInterval.end,
                                            u1.timeInterval.start,
                                            !u2.timeInterval.rc,
                                            !u1.timeInterval.lc),
                        (connect ? u1.p0 : u2.p1), u2.p1);
//       cout << "             Adding: "; resunit.Print(cout);
      res->MergeAdd( resunit );
      u2 = u1;
      pos++;
    } else {
      // u2-timeInterval.end > u1.timeInterval.start
      // --> Error!
      cerr << "sim_fillup_mpoint_VM: Error calculating gap unit:" << endl;
      cerr << "\tu2 = "; u2.Print(cerr);
      cerr << "\tu1 = "; u1.Print(cerr); cerr << endl;
      res->Clear();
      res->SetDefined( false );
      return 0;
    }
  }
  // test whether to insert a sequel unit
  if ( u2.timeInterval.end < *End ) {
//     cout << "Adding last unit: "; u2.Print(cout);
    res->MergeAdd( u2 );
    resunit = UPoint( Interval<DateTime>( u2.timeInterval.end,
                                          *End,
                                          !u2.timeInterval.rc,
                                          RC->GetBoolval() ),
                      u2.p1, u2.p1);
//     cout << "Adding Sequel unit: "; resunit.Print(cout); cout << endl;
    res->MergeAdd( resunit );
  } else if ( u2.timeInterval.end == *End &&
            !u2.timeInterval.rc &&
            RC->GetBoolval()
          ) {
    // extend the last unit by right-closing the timeInterval
    resunit = UPoint( Interval<DateTime>( u2.timeInterval.start,
                                          u2.timeInterval.end,
                                          u2.timeInterval.lc,
                                          !u2.timeInterval.rc),
                      u2.p0, u2.p1);
    res->MergeAdd( resunit );
//     cout << "Adding modified last unit: "; resunit.Print(cout); cout << endl;
  } else {
    // just append u2
//     cout << "Adding last unit: "; u2.Print(cout); cout << endl;
    res->MergeAdd( u2 );
  }
  return 0;
}

const string sim_fillup_mpoint_Spec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text> mpoint x instant x instant x bool x bool x bool -> mpoint"
    "</text--->"
    "<text>M sim_fillup_mpoint[ S, E, LC, RC, CONN ]"
    "</text--->"
    "<text>Fills up the definition of mpoint 'M', during the "
    "interval defined by starting instant 'S' and ending instant 'E', having "
    "closedness as specified by LC (leftclosed) and RC (rightclosed). "
    "The mpoint will not be trimmed to the interval. "
    "Gaps are filled using the last known position. For "
    "periods before M's initial instant, M's initial position is used. "
    "If M is empty, the result will be empty, too. An invalid interval "
    "specification will produce an empty/undefined result. If 'CONN' = TRUE, "
    "temporal gaps will be filled units connecting even the spatial positions."
    "</text--->"
    "<text>query trains feed extract[Trip] sim_fillup_mpoint[six30 - "
    "create_duration(-0.05), six30 - create_duration(0.5), TRUE, FALSE, FALSE]"
    "</text--->"
    ") )";

Operator sim_fillup_mpoint(
    "sim_fillup_mpoint",
    sim_fillup_mpoint_Spec,
    sim_fillup_mpoint_VM,
    Operator::SimpleSelect,
    sim_fillup_mpoint_TM) ;


/*
5.6 Operator ~sim\_trips~

Creates a stream of mpoints from a single mpoint by splitting it into single
trips. Endings of trips are identified by constant values for longer than a
given duration. Or by definition time gaps of any duration.

Undefined periods will be suppressed, but defined periods of constant value
will result in separate trips, if their duration is longer than the given
minimum pause duration.

Units are defined as stationary, if
(i) their start and end point are AlmostEqual, or
(ii) their starting and ending instant are identical, or
(iii) their velocity is below ~minVelocity~

Parameter ~minVelocity~ can be passed as an optional third argument. The unit is
m/h. It has a default value of 1.0 m/d (=0.04167 m/h = 0.6944 mm/min = 0.01157 mm/s)

----
    sim_trips: (mpoint x duration       ) --> (stream mpoint)
    sim_trips: (mpoint x duration x real) --> (stream mpoint)

----

*/

ListExpr sim_trips_TM ( ListExpr args )
{
  ListExpr arg1, arg2, arg3;
  if ( nl->ListLength( args ) == 2 ) {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    if ( nl->AtomType( arg1 ) == SymbolType &&
         nl->SymbolValue( arg1 ) == "mpoint"  &&
         nl->AtomType( arg2 ) == SymbolType &&
         nl->SymbolValue( arg2 ) == "duration") {
      return nl->TwoElemList(nl->SymbolAtom("stream"),
                             nl->SymbolAtom("mpoint"));
    }
  }

  if ( nl->ListLength( args ) == 3 ) {
    arg1 = nl->First( args );
    arg2 = nl->Second( args );
    arg3 = nl->Third( args );
    if ( nl->AtomType( arg1 ) == SymbolType &&
         nl->SymbolValue( arg1 ) == "mpoint"  &&
         nl->AtomType( arg2 ) == SymbolType &&
         nl->SymbolValue( arg2 ) == "duration" &&
         nl->AtomType( arg3 ) == SymbolType &&
         nl->SymbolValue( arg3 ) == "real") {
      return nl->TwoElemList(nl->SymbolAtom("stream"),
                             nl->SymbolAtom("mpoint"));
    }
  }

  ErrorReporter::
      ReportError("SimulationAlgebra: sim_trips expected "
      "(mpoint x duration) or (mpoint x duration x real)");
  return (nl->SymbolAtom( "typeerror" ));
}

bool IsAlmostStationaryUPoint(const UPoint u, const double minVelocity)
{
  if( !u.IsDefined() ){
    assert (false);
    return false;
  }
  if (AlmostEqual(u.p0,u.p1) || (u.timeInterval.start == u.timeInterval.end) )
    return true;
  double ds = u.p0.Distance(u.p1);
  double dt = (u.timeInterval.end - u.timeInterval.start).ToDouble();
  if (ds/dt < minVelocity)
    return true;
  return false;
}

struct SplitMpointLocalInfo
{
  SplitMpointLocalInfo():
      pos( 0 ), size( 0 ), finished( true ), u1 ( UPoint(false) )
      {}

  int  pos;       // unit counter
  int  size;      // number of units
  int trip;       // number of current trip
  bool finished;  // whether we are finished or not
  double minVelocity;
  UPoint u1;
};

int sim_trips_VM ( Word* args, Word& result,
                           int message, Word& local, Supplier s )
{
  SplitMpointLocalInfo *sli;
  MPoint               *mpoint = (MPoint*)(args[0].addr);
  DateTime             *mindur = (DateTime*)(args[1].addr);
  result = qp->ResultStorage( s );
  MPoint *res = 0;
  bool newtrip = false;

  switch (message)
  {
    case OPEN :

//       cout << "sim_trips_VM received OPEN" << endl;
      sli = new SplitMpointLocalInfo;
      sli->pos = 0;
      sli->trip = 0;
      sli->minVelocity = 1.0; // default value for minimum velocity

      if(qp->GetNoSons(s)==3)
      {
        CcReal* mv = (CcReal*)args[2].addr;
        if(!mv->IsDefined())
        { // undefined optional argument -> return empty stream
          sli->finished = true;
          sli->u1 = UPoint(false);
          return 0;
        }
        sli->minVelocity = mv->GetRealval() * 24.0;
        if(sli->minVelocity < 0.0)
        { // negative minimal velocity -> return empty stream
          sli->minVelocity = 0;
          sli->finished = true;
          sli->u1 = UPoint(false);
          return 0;
        }
      }
      else
      { // no optional argument
        sli->minVelocity = 1.0;
      }
      local.setAddr(sli);

      if ( !mpoint->IsDefined() ||
            mpoint->IsEmpty()   ||
           !mindur->IsDefined() ||
            mindur->GetType() != durationtype ||
            *mindur <= DateTime(0,0,durationtype)
         )
      { // argument undefined or invalid
        // -> return empty stream
        sli->finished = true;
        sli->u1 = UPoint(false);
        return 0;
      }
      sli->size = mpoint->GetNoComponents();
      sli->finished = false;
      return 0;

    case REQUEST :

//       cout << "sim_trips_VM received REQUEST" << endl;

      if (local.addr == 0)
      {
//         cout << "We have already finished!" << endl;
        result.setAddr(0);
        return CANCEL;
      }
      sli = (SplitMpointLocalInfo*) local.addr;
      if ( sli->finished )
      {
//         cout << "We have already finished!" << endl;
        result.setAddr(0);
        return CANCEL;
      }

      if ( sli->pos >= sli->size )
      {
        if ( sli->pos == sli->size && sli->u1.IsDefined() )
        { // special case: last unit of the mpoint is stationary and
          // was scheduled to form an own trip
          sli->trip++;
          res = new MPoint(0);
          res->Clear();
          res->StartBulkLoad();
//           cout << " Starting New Trip #" << sli->trip << ":" << endl;
//           cout << " => Finished. Final single unit trip" << endl;
//           cout << " Adding (0)"; sli->u1.Print(cout); cout << endl;
          res->MergeAdd( sli->u1 );
          res->EndBulkLoad( false, true ); // do not sort units
//           cout << " Finished Trip " << sli->trip << "." << endl;
          assert( res->IsValid() );           // XRIS: only for debugging
          result.setAddr( res );
          sli->u1.SetDefined( false );
          sli->finished = true;
          return YIELD;
        }
        else
        { // We have just finished the complete mpoint
//           cout << "We have already finished!" << endl;
          sli->finished = true;
          result.setAddr(0);
          return CANCEL;
        }
      }

      // else: regular case
      newtrip = false;
      sli->trip++;
      res = new MPoint(0);
      res->Clear();
      res->StartBulkLoad();
//       cout << " Starting Trip " << sli->trip << ":" << endl;

      while( sli->pos < sli->size && !newtrip)
      {
//         cout << "pos/size = " << sli->pos << "/" << sli->size << endl;
        UPoint u2(true);
        mpoint->Get(sli->pos++, u2);
//         cout << endl;
//         cout << " u1 = "; sli->u1.Print(cout);  cout << endl;
//         cout << " u2 = "; u2.Print(cout); cout << endl;
        assert( u2.IsDefined() );
        if ( !sli->u1.IsDefined() )
        { // sli->u1 is undefined
          // This means we just started to process mpoint
          // Do nothing, but move u2 to u1 for next iteration
          sli->u1 = u2;
        }
        else // ELSE: Inside a running trip
        if( sli->u1.timeInterval.end < u2.timeInterval.start ||
            ( (sli->u1.timeInterval.end == u2.timeInterval.start) &&
              !sli->u1.timeInterval.rc &&
              !u2.timeInterval.lc
            )
          )
        { // found GAP between sli->u1 and u2
//           cout << " => Gap between u1, u2." << endl;
//           cout << " Adding (1)"; sli->u1.Print(cout); cout << endl;
          res->MergeAdd(sli->u1);
          sli->u1 = u2;
          newtrip = true;
        }
        else // ELSE: No Gap --- consecutive units
        {
          if ( IsAlmostStationaryUPoint(sli->u1, sli->minVelocity) )
          // if ( AlmostEqual(sli->u1.p0, sli->u1.p1) )
          { // sli->u1 is constant
            // if ( AlmostEqual(u2.p0, u2.p1) )
            if ( IsAlmostStationaryUPoint(u2, sli->minVelocity) )
            { // sli->u1 is constant and u2 is constant
              if ( AlmostEqual(sli->u1.p1, u2.p0) )
              // if ( AlmostEqual(sli->u1.p0, u2.p0) )
              { // sli->u1 and u2 are constant AND equalvalue
                  // extend u1 by u2
                sli->u1.timeInterval.end = u2.timeInterval.end;
                sli->u1.timeInterval.rc  = u2.timeInterval.rc;
                sli->u1.p1 = u2.p1;
                if ( (sli->u1.timeInterval.end - sli->u1.timeInterval.start)
                      >= *mindur )
                { // concatenation sli->u1+u2 is long enough
                  if(res->GetNoComponents() == 0)
                  { // this is a new trip. We can use it for the pause.
                    // We do not need re-visit current unit u2
//                     cout << " => merged u1, u2. Immediate Pause." << endl;
//                     cout << " Adding (2)"; sli->u1.Print(cout); cout << endl;
                    res->MergeAdd( sli->u1 );
                    newtrip = true;
                  }
                  else
                  { // this trip was already used. We need to close it and
                    // create a new trip.
                    // We do not need re-visit current unit u2
//                     cout << " => merged u1, u2. Deferred Pause." << endl;
                    newtrip = true;
                  }
                }
                else
                { // extended u1 is not long enough for a pause.
                  // still do not add anything (u1 might be needed to extended
                  // further)
//                   cout << " => merged u1, u2. No pause yet." << endl;
                }
              }
              else // sli->u1 and u2 are constant BUT have different values
              {
                if ( (sli->u1.timeInterval.end - sli->u1.timeInterval.start)
                      >= *mindur )
                { // sli->u1 is long enough to form a separate pause trip;
                  // u1 needs to form a separate trip.
                  if(res->GetNoComponents() == 0)
                  { // the current trip is empty, so we can use it for u1
//                     cout << " => Immediate Pause u1." << endl;
//                     cout << " Adding (3)"; sli->u1.Print(cout); cout << endl;
                    res->MergeAdd( sli->u1 );
                    sli->u1 = u2;
                    newtrip = true;
                  }
                  else
                  { // The current trip was already used. We need to close it
                    // and start a new one for u1.
//                     cout << " => Deferred Pause u1. Revisit u2" << endl;
                    sli->pos--; // re-visit u2
                    newtrip = true;
                  }
                }
                else
                { // sli->u1 is not long enough for a pause.
//                   cout << " => No pause u1." << endl;
//                   cout << " Adding (4)"; sli->u1.Print(cout); cout << endl;
                  res->MergeAdd( sli->u1 );
                  sli->u1 = u2;
                }
              }
            }
            else // sli->u1 is constant, u2 is nonconstant
            {
              if ( (sli->u1.timeInterval.end - sli->u1.timeInterval.start )
                    >= *mindur )
              { // u1 is a pause.
                if(res->GetNoComponents() == 0)
                { // the current trip is empty, so we can use it for u1
//                   cout << " => Immediate Pause u1" << endl;
//                   cout << " Adding (5)"; sli->u1.Print(cout); cout << endl;
                  res->MergeAdd( sli->u1 );
                  sli->u1 = u2;
                  newtrip = true;
                }
                else
                { // The current trip was already used. We need to close it
                  // and start a new one for u1.
//                   cout << " => Deferred Pause u1, revisit u2" << endl;
                  sli->pos--; // re-visit current unit u2
                  newtrip = true;
                }
              }
              else
              { // u1 is not a pause
//                 cout << " => Continue u1, u2." << endl;
//                 cout << " Adding (6)"; sli->u1.Print(cout); cout << endl;
                res->MergeAdd( sli->u1 );
                sli->u1 = u2;
              }
            }
          } // end sli->u1 is constant
          else // sli->u1 is nonconstant
          {
            if ( IsAlmostStationaryUPoint(u2, sli->minVelocity) )
            // if (AlmostEqual(u2.p0, u2.p1))
            { // sli->u1 is nonconstant and u2 is constant
              // Add u1 to current trip.
//               cout << " => Continue u1." << endl;
//               cout << " Adding (7)"; sli->u1.Print(cout); cout << endl;
              res->MergeAdd( sli->u1 );
              sli->u1 = u2;
              if ( (u2.timeInterval.end - u2.timeInterval.start ) >= *mindur )
              { // start a new trip for current unit u2
//                 cout << " => Deferred Pause u2. No revisit." << endl;
                newtrip = true;
              }
              else
              {
//                 cout << " => Continue u2." << endl;
              }
            }
            else
            { // sli->u1 and u2 are nonconstant
//               cout << " => Continue u1, u2." << endl;
//               cout << " Adding (8)"; sli->u1.Print(cout); cout << endl;
              res->MergeAdd( sli->u1 );
              sli->u1 = u2;
            }
          }
        } // end No Gap --- consecutive unit
      } // end while( sli->pos < sli->size && !newtrip )
      if ( !newtrip && sli->u1.IsDefined() )
      { // insert the final unit
//          cout << " => Global final unit append." << endl;
//          cout << " Adding Final (9)"; sli->u1.Print(cout); cout << endl;
        res->MergeAdd( sli->u1 );
        sli->u1.SetDefined( false ); // invalidate unit (for additional safety)
        sli->finished = true;        // we have finished
      }
      res->EndBulkLoad( false, true ); // do not sort units
//       cout << " Finished Trip #" << sli->trip << "." << endl;
      result.setAddr( res );
//        cout << endl
//             << "===================================================" << endl;
      return YIELD;

    case CLOSE :

//       cout << "sim_trips_VM received CLOSE" << endl;
      if (local.addr != 0)
      {
        sli = (SplitMpointLocalInfo*) local.addr;
        delete sli;
        sli = 0;
        local.setAddr(0);
      }
      return 0;

  } // end switch
  return 0;   // should not be reached
}

const string sim_trips_Spec  =
    "( ( \"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
    "( <text> mpoint x duration [ x real ] -> stream(mpoint)</text--->"
    "<text>M sim_trips[ D ], "
    "M sim_trips[ D , Vmin ]</text--->"
    "<text>Splits a single mpoint 'M' into a stream of mpoints representing"
    "single 'trips'. Endings of trips are recognized by 1) stationary "
    "intervals with a duration of at least 'D', and 2) enclosed undefined "
    "intervals of any length. A unit is considered stationary, if "
    "(i)   its starting and ending positions are equal, or "
    "(ii)  its starting and ending instants are identical, or "
    "(iii) its velocity is smaller than stated by the (optional) parameter "
    "'Vmin' (given in unit [m/h]). If 'Vmin' is omitted, a standart value "
    "of 0.041667 m/h (corresponding to 1.0 m/d) is applied. "
    "Stationary intervals are interpreted as "
    "'pauses' and will be returned as separate trips. For definition gaps, "
    "no Trip will be created. The parameter 'D' must be "
    "positive ( > create_duration(0) ), 'Vmin' must be non-negative.</text--->"
    "<text>query train7 sim_trips[create_duration(0,1000)] count"
    "</text--->"
    ") )";

Operator sim_trips(
    "sim_trips",
    sim_trips_Spec,
    sim_trips_VM,
    Operator::SimpleSelect,
    sim_trips_TM) ;

/*
6 Class ~SimulationAlgebra~

The last steps in adding an algebra to the Secondo system are

  * Associating value mapping functions with their operators

  * ``Bunching'' all
type constructors and operators in one instance of class ~Algebra~.

Therefore, a new subclass ~SimulationAlgebra~ of class ~Algebra~ is declared. The only
specialization with respect to class ~Algebra~ takes place within the
constructor: all type constructors and operators are registered at the actual
algebra.

After declaring the new class, its only instance ~simulationAlgebra~ is defined.

*/


class SimulationAlgebra : public Algebra
{
 public:
  SimulationAlgebra() : Algebra()
    {
      AddOperator( &sim_set_rng );
      AddOperator( &sim_set_event_params );
      AddOperator( &sim_set_dest_params );
      AddOperator( &sim_create_trip );
      AddOperator( &sim_print_params );
      AddOperator( &sim_fillup_mpoint );
      AddOperator( &sim_trips );
    }
  ~SimulationAlgebra() {};

  private:
    bool initialized;
    const gsl_rng_type **simalg_randomgeneratorTable;
    // number of generator types available:
    int sim_randomgeneratorTableSize;
    int sim_rng_defaultIndex;
};


/*
7 Initialization

Each algebra module needs an initialization function. The algebra manager
has a reference to this function if this algebra is included in the list
of required algebras, thus forcing the linker to include this module.

The algebra manager invokes this function to get a reference to the instance
of the algebra class and to provide references to the global nested list
container (used to store constructor, type, operator and object information)
and to the query processor.

The function has a C interface to make it possible to load the algebra
dynamically at runtime.

*/

extern "C"
Algebra*
InitializeSimulationAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return (new SimulationAlgebra());
}
