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

//paragraph    [10]    title:           [{\Large \bf ] [}]
//paragraph    [21]    table1column:    [\begin{quote}\begin{tabular}{l}]     [\end{tabular}\end{quote}]
//paragraph    [22]    table2columns:   [\begin{quote}\begin{tabular}{ll}]    [\end{tabular}\end{quote}]
//paragraph    [23]    table3columns:   [\begin{quote}\begin{tabular}{lll}]   [\end{tabular}\end{quote}]
//paragraph    [24]    table4columns:   [\begin{quote}\begin{tabular}{llll}]  [\end{tabular}\end{quote}]
//[--------]    [\hline]
//characters    [1]    verbatim:   [$]    [$]
//characters    [2]    formula:    [$]    [$]
//characters    [3]    capital:    [\textsc{]    [}]
//characters    [4]    teletype:   [\texttt{]    [}]
//[ae] [\"a]
//[oe] [\"o]
//[ue] [\"u]
//[ss] [{\ss}]
//[<=] [\leq]
//[#]  [\neq]
//[tilde] [\verb|~|]
//[Contents] [\tableofcontents]

1 Header File: Progress View

*/

#ifndef PROGRESSVIEW_H
#define PROGRESSVIEW_H

#include "StopWatch.h"
#include "NList.h"
#include "Messages.h"          // msg->Send(msgList)
#include "Profiles.h"          // SmiProfile::GetParameter
#include "Progress.h"
#include "CharTransform.h"

using namespace std;

class ProgressView
{
  public:

  ProgressView()
  {
    parmFile = expandVar("$(SECONDO_CONFIG)");
    PROGRESS_NORM = SmiProfile::GetParameter("ProgressEstimation", 
      "ProgNorm", 50, parmFile);
    DETPROT = SmiProfile::GetParameter("ProgressEstimation", 
      "ProgProt", 0, parmFile);
    DETPROT += 2 * SmiProfile::GetParameter("ProgressEstimation", 
      "ProgLog", 0, parmFile);
    PROGTYPECARD = SmiProfile::GetParameter("ProgressEstimation", 
      "ProgTypeCard", 0, parmFile);
    PROGTYPEPROG = SmiProfile::GetParameter("ProgressEstimation", 
      "ProgTypeProg", 0, parmFile);
    if (PROGTYPEPROG)
    {
      initialized = false;
      fileName    = "proglogt.csv";
      if (DETPROT & 2) ofs.open(fileName.c_str(), ios::app);
    }
  }

  ~ProgressView()
  {        
    if (DETPROT & 2) { ofs << endl; ofs.close(); }
  }

  void
  InitProgressView()
  {

    if (PROGTYPEPROG)
    {
      initialized  = true;
      pointCounter = 0;
	  startClock  = clock();
      msg = MessageCenter::GetInstance();

      msgList = NList(NList("progress"), NList(NList(-1), 
        NList(PROGRESS_NORM)));
      msg->Send(msgList);

    }
  }

  void
  WriteCommandToProtocol(const string& commandText)
  {
    if (PROGTYPEPROG)
    {
      if (DETPROT & 2) {
        ofs << "'" << commandText << "'" << endl;
        ofs << endl;
      }
    }
  }

  void
  ModifyProgressView(ProgressInfo progress)
  {
    if (PROGTYPEPROG)
    {
      if (!initialized) InitProgressView();

      actMaxPoints = (int) (progress.Progress * PROGRESS_NORM);
      while (pointCounter < actMaxPoints)
      {
        pointCounter++;
        msgList = 
          NList(NList("progress"),
            NList(NList((int) ((pointCounter * 100.0) / PROGRESS_NORM)),
            NList(100)));
        msg->Send(msgList);
	  }

      if (DETPROT & 2)
      {

       uint64_t clocks = clock() - startClock;
       // convert clocks to milliseconds
       clocks = (clocks * 1000) / CLOCKS_PER_SEC;


       ofs << clocks << ";" ;

	sprintf (progstr, "%6.2f", progress.Progress * 100.0);
        progstr [3] = ',';

        ofs << (int) progress.Card << ";";
        ofs << (int) progress.Time << ";";
	ofs << progstr << ";" ;
	ofs << endl;
      }
    }
  }

  void
  FinishProgressView()
  {
    if (PROGTYPEPROG && initialized)
    {
      initialized = false;

      while (pointCounter < PROGRESS_NORM)
      {
        pointCounter++;
        msgList = 
          NList(NList("progress"),
            NList(NList((int) ((pointCounter * 100.0) / PROGRESS_NORM)),
            NList(100)));
        msg->Send(msgList);
      }
      msgList = NList(NList("progress"), NList(NList(0), NList(-1)));
      msg->Send(msgList);

      if (DETPROT & 2) 	ofs << endl;

    }
  }

  private:
    bool initialized;
    clock_t startClock;
    int pointCounter, actMaxPoints;
    char progstr [6];
    string fileName, parmFile;
    ofstream ofs;
    NList msgList;
    MessageCenter* msg;
    int PROGRESS_NORM;
    int DETPROT;
    int PROGTYPECARD;
    int PROGTYPEPROG;
};

#endif
