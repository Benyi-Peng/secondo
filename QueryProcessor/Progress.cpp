/*
----
This file is part of SECONDO.

Copyright (C) 2004-2007, University in Hagen, Faculty of Mathematics and
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

*/

#include "Progress.h"


ProgressInfo::ProgressInfo()
{
  sizesChanged = false;
  BTime = 0.001;	//default assumes non-blocking operator,
  BProgress = 1.0;	//time value must be > 0
}


void ProgressInfo::CopySizes(ProgressInfo p)	//copy the size fields
  {
    Size = p.Size;
    SizeExt = p.SizeExt;
    noAttrs = p.noAttrs;
    attrSize = p.attrSize;
    attrSizeExt = p.attrSizeExt;
    sizesChanged = p.sizesChanged;
  }


void ProgressInfo::CopySizes(ProgressLocalInfo* pli) //copy the size fields
{
  Size = pli->Size;
  SizeExt = pli->SizeExt;
  noAttrs = pli->noAttrs;
  attrSize = pli->attrSize;
  attrSizeExt = pli->attrSizeExt;
  sizesChanged = pli->sizesChanged;
}



void ProgressInfo::CopyBlocking(ProgressInfo p)
				//copy BTime, BProgress
				//for non blocking unary op.
{
  BTime = p.BTime;
  BProgress = p.BProgress;
}

void ProgressInfo::CopyBlocking(ProgressInfo p1, ProgressInfo p2)
				//copy BTime, BProgress
				//for non-blocking binary op. (join)
{
  BTime = p1.BTime + p2.BTime;
  BProgress =
    (p1.BProgress * p1.BTime + p2.BProgress * p2.BTime) / BTime;
}


void ProgressInfo::Copy(ProgressInfo p)		//copy all fields
  {
    Card = p.Card;
    CopySizes(p);
    Time = p.Time;
    Progress = p.Progress;
    BTime = p.BTime;
    BProgress = p.BProgress;
  }




ProgressLocalInfo::ProgressLocalInfo() :
    returned(0), read(0), readFirst(0), readSecond(0),
    total(0), defaultValue(0), state(0), memoryFirst(0), memorySecond(0),
    firstLocalInfo(0), secondLocalInfo(0), sizesInitialized(false),
    sizesChanged(false), Size(0.0), SizeExt(0.0), noAttrs(0), attrSize(0),
    attrSizeExt(0)  {}

ProgressLocalInfo::~ProgressLocalInfo()
{
  if(attrSize){
    delete [] attrSize;
    attrSize = 0;
  }
  if(attrSizeExt){
    delete [] attrSizeExt;
    attrSizeExt = 0;
  }
}

void ProgressLocalInfo::SetJoinSizes( ProgressInfo& p1, ProgressInfo& p2 )
{
  sizesChanged = false;
  if(sizesInitialized && (noAttrs != p1.noAttrs + p2.noAttrs)){
     sizesInitialized = false;
  }

  if ( !sizesInitialized )
  {
    if(attrSize){
      delete [] attrSize;
      attrSize = 0;
    }
    if(attrSizeExt){
      delete [] attrSizeExt;
      attrSizeExt = 0;
    }
    noAttrs = p1.noAttrs + p2.noAttrs;
    if(attrSize){
      delete [] attrSize;
    }
    attrSize = new double[noAttrs];
    if(attrSizeExt){
       delete [] attrSizeExt;
    }
    attrSizeExt = new double[noAttrs];
  }

  if ( !sizesInitialized || p1.sizesChanged || p2.sizesChanged )
  {
    Size = p1.Size + p2.Size;
    SizeExt = p1.SizeExt + p2.SizeExt;
    noAttrs = p1.noAttrs + p2.noAttrs;

    for (int i = 0; i < p1.noAttrs; i++)
    {
      attrSize[i] = p1.attrSize[i];
      attrSizeExt[i] = p1.attrSizeExt[i];
    }
    for (int j = 0; j < p2.noAttrs; j++)
    {
       attrSize[p1.noAttrs + j] = p2.attrSize[j];
       attrSizeExt[p1.noAttrs + j] = p2.attrSizeExt[j];
    }
    sizesInitialized = true;
    sizesChanged = true;
  }
}


















