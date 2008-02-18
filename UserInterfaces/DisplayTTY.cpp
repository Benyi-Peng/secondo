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

December 2005, Victor Almeida deleted the deprecated algebra levels (~executable~,
~descriptive~, and ~hibrid~). Only the executable level remains.

April 2006, M. Spiekermann. Display function for type text changed since its output
format was changed. Moreover, the word wrapping of text atoms was improved. 

\def\CC{C\raise.22ex\hbox{{\footnotesize +}}\raise.22ex\hbox{\footnotesize +}\xs
pace}
\centerline{\LARGE \bf  DisplayTTY}

\centerline{Friedhelm Becker , Mai1998}

\begin{center}
\footnotesize
\tableofcontents
\end{center}

1 Overview

There must be exactly one TTY display function of type DisplayFunction
for every type constructor provided by any of the algebra modules which
are loaded by *AlgebraManager2*. The first parameter is the type
expression in numeric nested list format describing the value which is
going to be displayed by the display function. The second parameter is
this value in nested list format.

1.2 Includes and defines

maxLineLength gives the maximum length of an input line (an command)
which will be read.

*/

using namespace std;

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <math.h>

#include "DisplayTTY.h"
#include "NestedList.h"
#include "NList.h"
#include "SecondoInterface.h"
#include "AlgebraTypes.h"
#include "Base64.h"
#include "CharTransform.h"

#define LINELENGTH 80
/*
1.3 Managing display functions

The map *displayFunctions* holds all existing display functions. It
is indexed by a string created from the algebraId and the typeId of
the corresponding type constructor.

*/
SecondoInterface* DisplayTTY::si = 0;
NestedList*       DisplayTTY::nl = 0;
map<string,DisplayFunction> DisplayTTY::displayFunctions;
int DisplayTTY::maxIndent = 0;

/*
The function *CallDisplayFunction* uses its first argument *idPair*
--- consisting of the two-elem-list $<$algebraId, typeId$>$ --- to
find the right display function in the array *displayFunctions*. The
arguments *typeArg* and *valueArg* are simply passed to this display
function.

*/

void
DisplayTTY::CallDisplayFunction( const ListExpr idPair,
                                 ListExpr type,
                                 ListExpr numType,
                                 ListExpr value )
{
  ostringstream osId;
  osId << "[" << nl->IntValue( nl->First( idPair ) )
       << "|" << nl->IntValue( nl->Second( idPair ) ) << "]";
  map<string,DisplayFunction>::iterator dfPos =
    displayFunctions.find( osId.str() );
  if ( dfPos != displayFunctions.end() )
  {
    (*(dfPos->second))( type, numType, value );
  }
  else
  {
    DisplayGeneric( type, numType, value );
  }
}

/*
The procedure *InsertDisplayFunction* inserts the procedure given as
second argument into the array displayFunctions at the index which is
determined by the type constructor name given as first argument.

*/

void
DisplayTTY::InsertDisplayFunction( const string& name,
                                   DisplayFunction df )
{
  int algebraId, typeId;
  si->GetTypeId( name, algebraId, typeId );
  ostringstream osId;
  osId << "[" << algebraId << "|" << typeId << "]";
  displayFunctions[osId.str()] = df;
}

/*
1.4 Display functions

Display functions of the DisplayTTY module are used to  transform a
nested list value into a pretty printed output in text format. Display
functions which are called with a value of compound type usually call
recursively the display functions of the subtypes, passing the subtype
and subvalue, respectively.

*/

void
DisplayTTY::DisplayGeneric( ListExpr type, ListExpr numType, ListExpr value )
{
  cout << "No specific display function defined.";
  cout << " Generic function used." << endl;
  cout << "Type: ";
  nl->WriteListExpr( type, cout );
  cout << endl << "Value: ";
  nl->WriteListExpr( value, cout );
}

void
DisplayTTY::DisplayRelation( ListExpr type, ListExpr numType, ListExpr value )
{
  type = nl->Second( type );
  numType = nl->Second( numType );
  CallDisplayFunction( nl->First( numType ), type, numType, value );
}

int
DisplayTTY::MaxHeaderLength( ListExpr type )
{
  int max, len;
  string s;
  max = 0;
  while (!nl->IsEmpty( type ))
  {
    s = nl->StringValue( nl->First( type ) );
    len = s.length();
    if ( len > max )
    {
      max = len;
    }
    type = nl->Rest( type );
  }
  return (max);
}


int
DisplayTTY::MaxAttributLength( ListExpr type )
{
  int max=0, len=0;
  string s="";
  while (!nl->IsEmpty( type ))
  {
    s = nl->ToString( nl->First( nl->First( type ) ) );
    len = s.length();
    if ( len > max )
    {
      max = len;
    }
    type = nl->Rest( type );
  }
  return (max);
}

void
DisplayTTY::DisplayTuple( ListExpr type, ListExpr numType,
                          ListExpr value, const int maxNameLen )
{
  while (!nl->IsEmpty( value ))
  {
    cout << endl;
    string s = nl->ToString( nl->First( nl->First( numType ) ) );
    string attr = string( maxNameLen-s.length() , ' ' ) + s + string(":");
    cout << attr;
    maxIndent = attr.length();

    if( nl->IsAtom( nl->First( nl->Second( nl->First( numType ) ) ) ) )
    {
      CallDisplayFunction( nl->Second( nl->First( numType ) ),
                           nl->Second( nl->First( type ) ),
                           nl->Second( nl->First( numType ) ),
                           nl->First( value ) );
    }
    else
    {
      CallDisplayFunction( nl->First( nl->Second( nl->First( numType ) ) ),
                           nl->Second( nl->First( type ) ),
                           nl->Second( nl->First( numType ) ),
                           nl->First( value ) );
    }
    maxIndent = 0;
    
    type    = nl->Rest( type );
    numType = nl->Rest( numType );
    value   = nl->Rest( value );
  }
}

void
DisplayTTY::DisplayTuples( ListExpr type, ListExpr numType, ListExpr value )
{
  int maxAttribNameLen = MaxAttributLength( nl->Second( numType ) );
  while (!nl->IsEmpty( value ))
  {
    DisplayTuple( nl->Second( type ), nl->Second( numType ),
                  nl->First( value ), maxAttribNameLen );
    value = nl->Rest( value );
    cout << endl;
  }
}

void
DisplayTTY::DisplayInt( ListExpr type, ListExpr numType, ListExpr value )
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else
  {
    cout << nl->IntValue( value );
  }
}

void
DisplayTTY::DisplayReal( ListExpr type, ListExpr numType, ListExpr value )
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else
  {
    cout.unsetf(ios_base::floatfield);
    double d = nl->RealValue( value );
    int p = min( static_cast<int>(ceil(log10(d))) + 10, 16 );
    cout << setprecision(p) << d;

    if ( RTFlag::isActive("TTY:Real:16digits") ) {
      cout << " (16digits = " << setprecision(16) << d << ")";
    }
  }
}

void
DisplayTTY::DisplayBoolean( ListExpr list, ListExpr numType, ListExpr value )
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else
  {
    if ( nl->BoolValue( value ) )
    {
      cout << "TRUE";
    }
    else
    {
      cout << "FALSE";
    }
  }
}

void
DisplayTTY::DisplayString( ListExpr type, ListExpr numType, ListExpr value )
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else
  {
    cout << nl->StringValue( value );
  }
}


void
DisplayTTY::DisplayText( ListExpr type, ListExpr numType, ListExpr value )
{

  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else
  {
    //TextScan txtscan = nl->CreateTextScan(nl->First(value));
    //nl->GetText(txtscan, nl->TextLength(nl->First(value)),printstr);
    //delete txtscan;
    string printstr="";
    nl->Text2String(value, printstr);
    cout << wordWrap(0, maxIndent, LINELENGTH, printstr);
    /*
    position = 0;
    lastblank = -1;
    line = "";
    for (unsigned i = 1; i <= printstr.length(); i++)
    {
      line += printstr[i-1];
      if (printstr[i-1] == ' ') lastblank = position;
      position++;
      lastline = ( i == printstr.length() );
      if ( position == LINELENGTH || lastline || (printstr[i-1] == '\n') )
      {
        if ( lastline || (printstr[i-1] == '\n') )
        {
          cout << line;
          line = "";
          lastblank = -1;
          position = 0;
        }
        else
        {
          if ( lastblank > 0 )
          {
            cout << line.substr(0, lastblank) << endl;
            restline = line.substr(lastblank+1, position);
            line = "";
            line += restline;
            lastblank = -1;
            position = line.length();
          }
          else
          {
            cout << line << endl;
            line = "";
            lastblank = -1;
            position = 0;
          }
        }
      }
    }
    */
  }
}

void
DisplayTTY::DisplayFun( ListExpr type, ListExpr numType, ListExpr value )
{
  cout << "Function type: ";
  nl->WriteListExpr( type, cout );
  cout << endl << "Function value: ";
  nl->WriteListExpr( value, cout );
  cout << endl;
}

void
DisplayTTY::DisplayDate( ListExpr type, ListExpr numType, ListExpr value)
{
  if (nl->IsAtom(value) && nl->AtomType(value)==StringType)
      cout <<nl->StringValue(value);
  else
      cout <<"Incorrect Data Format!";
}


void
DisplayTTY::DisplayBinfile( ListExpr type, ListExpr numType, ListExpr value)
{
   cout <<"binary file";
}



double DisplayTTY::getNumeric(ListExpr value, bool &err){
   if(nl->AtomType(value)==IntType){
      err=false;
      return nl->IntValue(value);
   }
   if(nl->AtomType(value)==RealType){
      err=false;
      return nl->RealValue(value);
   }
   if(nl->AtomType(value)==NoAtom){
      int len = nl->ListLength(value);
      if(len!=5 & len!=6){
        err=true;
  return 0;
      }
      ListExpr F = nl->First(value);
      if(nl->AtomType(F)!=SymbolType){
         err=true;
   return 0;
      }
      if(nl->SymbolValue(F)!="rat"){
        err=true;
        return 0;
      }
      value = nl->Rest(value);
      double sign = 1.0;
      if(nl->ListLength(value)==5){  // with sign
        ListExpr SignList = nl->First(value);
        if(nl->AtomType(SignList)!=SymbolType){
     err=true;
           return 0;
  }
        string SignString = nl->SymbolValue(SignList);
  if(SignString=="-")
     sign = -1.0;
  else if(SignString=="+")
     sign = 1.0;
  else{
    err=true;
    return 0;
  }
        value= nl->Rest(value);
      }
      if(nl->AtomType(nl->First(value))==IntType &&
         nl->AtomType(nl->Second(value))==IntType &&
         nl->AtomType(nl->Third(value))==SymbolType &&
   nl->SymbolValue(nl->Third(value))=="/" &&
   nl->AtomType(nl->Fourth(value))==IntType){
      err=false;
      double intpart = nl->IntValue(nl->First(value));
      double numDecimal = nl->IntValue(nl->Second(value));
      double denomDecimal = nl->IntValue(nl->Fourth(value));
      if(denomDecimal==0){
         err=true;
         return 0;
      }
      double res1 = intpart*denomDecimal + numDecimal/denomDecimal;
      return sign*res1;
   } else{
  err = true;
  return 0;
     }
   }
   err=true;
   return 0;
}

void
DisplayTTY::DisplayXPoint( ListExpr type, ListExpr numType, ListExpr value)
{
  if(nl->ListLength(value)!=2)
     cout << "Incorrect Data Format";
  else{
     bool err;
     double x = getNumeric(nl->First(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     double y = getNumeric(nl->Second(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     cout << "xpoint (" << x << "," << y << ")";
  }
}
/*
DisplayPoint

*/

void
DisplayTTY::DisplayPoint( ListExpr type, ListExpr numType, ListExpr value)
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else if(nl->ListLength(value)!=2)
     cout << "Incorrect Data Format";
  else{
     bool err;
     double x = getNumeric(nl->First(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     double y = getNumeric(nl->Second(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     cout << "point: (" << x << "," << y << ")";
  }
}

/*
DisplayRect

*/
void DisplayTTY::DisplayRect( ListExpr type, ListExpr numType, ListExpr value)
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else if( nl->ListLength(value) != 4 )
    cout << "Incorrect Data Format";
  else
  {
    bool realValue;
    double coordValue[4];
    unsigned i = 0;
    ListExpr restList, firstVal;

    restList = value;
    do
    {
      firstVal = nl->First(restList);
      realValue = nl->AtomType( firstVal ) == RealType;
      if (realValue)
      {
        restList = nl->Rest(restList);
        coordValue[i] = nl->RealValue(firstVal);
        i++;
      }
    } while (i < 4 && realValue);

    if (realValue)
    {
      cout << "rect: ( (";
      for( unsigned i = 0; i < 2; i++ )
      {
        cout << coordValue[2*i];
        if( i < 2 - 1 )
          cout << ",";
      }
      cout << ") - (";
      for( unsigned i = 0; i < 2; i++ )
      {
        cout << coordValue[2*i+1];
        if( i < 2 - 1 )
          cout << ",";
      }
      cout << ") )";
    }
    else
    {
      cout << "Incorrect Data Format";
      return;
    }
  }
}

/*
DisplayRect3

*/

void DisplayTTY::DisplayRect3( ListExpr type, ListExpr numType, ListExpr value)
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else if( nl->ListLength(value) != 6 )
    cout << "Incorrect Data Format";
  else
  {
    bool realValue;
    double coordValue[6];
    unsigned i = 0;
    ListExpr restList, firstVal;

    restList = value;
    do
    {
      firstVal = nl->First(restList);
      realValue = nl->AtomType( firstVal ) == RealType;
      if (realValue)
      {
        restList = nl->Rest(restList);
        coordValue[i] = nl->RealValue(firstVal);
        i++;
      }
    } while (i < 6 && realValue);

    if (realValue)
    {
      cout << "rect: ( (";
      for( unsigned i = 0; i < 3; i++ )
      {
        cout << coordValue[2*i];
        if( i < 3 - 1 )
          cout << ",";
      }
      cout << ") - (";
      for( unsigned i = 0; i < 3; i++ )
      {
        cout << coordValue[2*i+1];
        if( i < 3 - 1 )
          cout << ",";
      }
      cout << ") )";
    }
    else
    {
      cout << "Incorrect Data Format";
      return;
    }
  }
}

/*
DisplayRect4

*/

void DisplayTTY::DisplayRect4( ListExpr type, ListExpr numType, ListExpr value)
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else if( nl->ListLength(value) != 8 )
    cout << "Incorrect Data Format";
  else
  {
    bool realValue;
    double coordValue[8];
    unsigned i = 0;
    ListExpr restList, firstVal;

    restList = value;
    do
    {
      firstVal = nl->First(restList);
      realValue = nl->AtomType( firstVal ) == RealType;
      if (realValue)
      {
        restList = nl->Rest(restList);
        coordValue[i] = nl->RealValue(firstVal);
        i++;
      }
    } while (i < 8 && realValue);

    if (realValue)
    {
      cout << "rect: ( (";
      for( unsigned i = 0; i < 4; i++ )
      {
        cout << coordValue[2*i];
        if( i < 4 - 1 )
          cout << ",";
      }
      cout << ") - (";
      for( unsigned i = 0; i < 4; i++ )
      {
        cout << coordValue[2*i+1];
        if( i < 4 - 1 )
          cout << ",";
      }
      cout << ") )";
    }
    else
    {
      cout << "Incorrect Data Format";
      return;
    }
  }
}

/*
DisplayRect8

*/

void DisplayTTY::DisplayRect8( ListExpr type, ListExpr numType, ListExpr value)
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else if( nl->ListLength(value) != 16 )
    cout << "Incorrect Data Format";
  else
  {
    bool realValue;
    double coordValue[16];
    unsigned i = 0;
    ListExpr restList, firstVal;

    restList = value;
    do
    {
      firstVal = nl->First(restList);
      realValue = nl->AtomType( firstVal ) == RealType;
      if (realValue)
      {
        restList = nl->Rest(restList);
        coordValue[i] = nl->RealValue(firstVal);
        i++;
      }
    } while (i < 16 && realValue);

    if (realValue)
    {
      cout << "rect: ( (";
      for( unsigned i = 0; i < 8; i++ )
      {
        cout << coordValue[2*i];
        if( i < 8 - 1 )
          cout << ",";
      }
      cout << ") - (";
      for( unsigned i = 0; i < 8; i++ )
      {
        cout << coordValue[2*i+1];
        if( i < 8 - 1 )
          cout << ",";
      }
      cout << ") )";
    }
    else
    {
      cout << "Incorrect Data Format";
      return;
    }
  }
}

/*
DisplayMP3

*/

void
DisplayTTY::DisplayMP3( ListExpr type, ListExpr numType, ListExpr value)
{
    if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
        nl->SymbolValue( value ) == "undef" )
  cout << "UNDEFINED";
    else
  cout << "mp3 file";
}

/*
DisplayID3

*/
void
DisplayTTY::DisplayID3( ListExpr type, ListExpr numType, ListExpr value)
{
    if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
        nl->SymbolValue( value ) == "undef" )
    {
  cout << "UNDEFINED";
    }
    else
    {
  cout << "ID3-Tag"<<endl << endl;
  cout << "Title   : " << nl->StringValue (nl->First (value)) <<endl;
  cout << "Author  : " << nl->StringValue (nl->Second (value)) << endl;
  cout << "Album   : " << nl->StringValue (nl->Third (value)) << endl;
  cout << "Year    : " << nl->IntValue (nl->Fourth (value)) << endl;
  cout << "Comment : " << nl->StringValue (nl->Fifth (value)) << endl;

  if (nl->ListLength(value) == 6)
  {
      cout << "Genre   : " << nl->StringValue (nl->Sixth (value)) << endl;
  }
  else
  {
      cout << "Track   : " << nl->IntValue (nl->Sixth (value)) << endl;
      cout << "Genre   : ";
      cout << nl->StringValue (nl->Sixth (nl->Rest (value))) << endl;
  }
    }
}

/*
DisplayLyrics

*/

void
DisplayTTY::DisplayLyrics( ListExpr type, ListExpr numType, ListExpr value)
{
    if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
        nl->SymbolValue( value ) == "undef" )
    {
  cout << "UNDEFINED";
    }
    else
    {
  cout << "Lyrics"<<endl<<endl;
  int no = nl->ListLength (value) / 2;
  for (int i=1; i<=no; i++)
  {
      cout << "[" << nl->IntValue ( nl->First (value)) / 60 << ":";
      cout << nl->IntValue ( nl->First (value)) % 60 << "] ";
      cout << nl->StringValue (nl->Second (value));
      value = nl->Rest (nl->Rest (value));
  }
    }
}

/*
DisplayMidi

*/
void
DisplayTTY::DisplayMidi (ListExpr type, ListExpr numType, ListExpr value)
{
  int size = nl->IntValue(nl->Second(value));
  int noOfTracks = nl->IntValue(nl->Third(value));
  cout << "Midi: " << size << "bytes, ";
  cout << noOfTracks << " tracks";
}

/*
DisplayArray

*/
void
DisplayTTY::DisplayArray( ListExpr type, ListExpr numType, ListExpr value)
{

  if(nl->ListLength(value)==0)
     cout << "an empty array";
  else{
     ListExpr AType = nl->Second(type);
     ListExpr ANumType = nl->Second(numType);
     // find the idpair
     ListExpr idpair = ANumType;
     while(nl->AtomType(nl->First(idpair))!=IntType)
        idpair = nl->First(idpair);

     int No = 1;
     cout << "*************** BEGIN ARRAY ***************" << endl;
     while( !nl->IsEmpty(value)){
        cout << "--------------- Field No: ";
  cout << No++ << " ---------------" << endl;
        CallDisplayFunction(idpair,AType,ANumType,nl->First(value));
  cout << endl;
  value = nl->Rest(value);
     }
     cout << "***************  END ARRAY  ***************";

  }
}

/*
DisplayInstant

*/
void
DisplayTTY::DisplayInstant( ListExpr type, ListExpr numType, ListExpr value )
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else if(nl->AtomType(value) ==RealType){
    cout << nl->RealValue(value);   
  } else {
    cout << nl->StringValue( value );
  }
}

/*
DisplayDuration

*/
void
DisplayTTY::DisplayDuration( ListExpr type, ListExpr numType, ListExpr value )
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else
  { bool written = false;
    if(nl->ListLength(value)==2){
      if( (nl->AtomType(nl->First(value))==IntType) &&
          (nl->AtomType(nl->Second(value))==IntType)){
         int dv  = nl->IntValue(nl->First(value));
   int msv = nl->IntValue(nl->Second(value));
   written = true;
   if( (dv==0) && (msv==0)){
              cout << "0ms";
   }else{
     if(dv!=0){
         cout << dv << " days";
         if(msv>0)
             cout <<" + ";
     }
           if(msv > 0)
         cout << msv <<" ms";
   }

      }
    }
    if(!written){
       cout << "unknown list format for duration; using nested list output:";
       nl->WriteListExpr(value);
    }
  }
}

/*
DisplayTid

*/
void
DisplayTTY::DisplayTid( ListExpr type, ListExpr numType, ListExpr value )
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else
  {
    cout << nl->IntValue( value );
  }
}
/*
DisplayHtml

*/
void
DisplayTTY::DisplayHtml( ListExpr type, ListExpr numType, ListExpr value)
{
  if(nl->ListLength(value)!=3)
    cout << "Incorrect Data Format";
  else{
    string out;
    nl->WriteToString(out, type);
    cout << "Type: " << out << endl;

    cout << "Date of last modification: ";
    ListExpr First = nl->First(value);              //DateTime
    ListExpr Second = nl->Second(value);    //Text (FLOB)
    ListExpr Third = nl->Third(value);              //URL
    DisplayResult( nl->First(First), nl->Second(First) );
    DisplayResult( nl->First(Third), nl->Second(Third) );
    Base64 b;
    string text = nl->Text2String(Second);
    int sizeDecoded = b.sizeDecoded( text.size() );
    char *bytes = (char *)malloc( sizeDecoded );
    int result = b.decode( text, bytes );
    assert( result <= sizeDecoded );
    bytes[result] = 0;
    text = bytes;
    free( bytes );
    cout << "-------------Start of html content:" << endl;
    cout << text << endl;
    cout << "-------------End of html content:" << endl;
    }
}
/*
DisplayPage

*/
void
DisplayTTY::DisplayPage( ListExpr type, ListExpr numType, ListExpr value)
{
  if(nl->ListLength(value)<1)
     cout << "Incorrect Data Format";
  else{
     string out;
     nl->WriteToString(out, type);
     cout << "Type: " << out << endl;

     ListExpr First = nl->First(value);              //html
     nl->WriteToString(out, First);
     int nrOfEmb = nl->ListLength(value) - 1;
     DisplayResult( nl->First(First), nl->Second(First) );

     First = nl->Rest(value);
     //now lists of (url text)
     cout << "----Start of embedded objects" << endl;
     for( int ii=0; ii < nrOfEmb; ii++)
     {
        ListExpr emblist = nl->First(First);
        First = nl->Rest(First);

        if ( nl->ListLength( emblist ) == 3)
        {
            cout << endl << ii+1 << ". embedded object:" << endl;
            ListExpr embFirst = nl->First(emblist);         //url
            DisplayResult( nl->First(embFirst), nl->Second(embFirst) );
            cout << "The content is binary coded, no display here" << endl;
            string mime = nl->StringValue(nl->Third(emblist));
            cout << "Mimetype: " << mime << endl;
        }
        else
        {
            cout << "Incorrect Data Format";
            return;
        }
     }
     cout << "----End of embedded objects" << endl;
  }
}

/*
DisplayUrl

*/
void
DisplayTTY::DisplayUrl( ListExpr type, ListExpr numType, ListExpr value)
{
  if(nl->ListLength(value)!=3)
     cout << "Incorrect Data Format";
  else{
      string out;
      nl->WriteToString(out, type);
      cout << "Type: " << out << endl;

      ListExpr First = nl->First(value);              //protocol
      ListExpr Second = nl->Second(value);    //host
      ListExpr Third = nl->Third(value);              //path
      string prot = nl->StringValue(First);
      string host = nl->Text2String(Second);
      string path = nl->Text2String(Third);
      cout << "- Protocol: " << prot << endl;
      cout << "- Host: " << host << endl;
      cout << "- Path: " << path << endl;
      }
}
/*
DisplayVertex

*/
void
DisplayTTY::DisplayVertex( ListExpr type, ListExpr numType, ListExpr value)
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else if(nl->ListLength(value)!=2)
     cout << "Incorrect Data Format";
  else{
     bool err=false;
     int key = (int) getNumeric(nl->First(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     if( nl->IsAtom( nl->Second(value) )  &&
         nl->SymbolValue(  nl->Second(value) ) == "undef" )
      {
         cout << "vertex "<<key<<": no point defined";
         return;
      }
      double x = getNumeric(nl->First(nl->Second(value)),err);
      if(err){
        cout << "Incorrect Data Format";
        return;
      }
      double y = getNumeric(nl->Second(nl->Second(value)),err);
      if(err){
        cout << "Incorrect Data Format";
        return;
      }
      cout << "vertex "<<key<<": (" << x << "," << y << ")";
   }
 }
/*
DisplayEdge

*/
void
DisplayTTY::DisplayEdge( ListExpr type, ListExpr numType, ListExpr value)
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else if(nl->ListLength(value)!=3)
     cout << "Incorrect Data Format";
  else{
     bool err=false;
     int key1 = (int) getNumeric(nl->First(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }

     int key2 = (int) getNumeric(nl->Second(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     double cost = getNumeric(nl->Third(value),err);
     if(err){
       cout << "Incorrect Data Format";
       return;
     }
     cout << "edge "<<key1<<"---" << cost << "---->" << key2;
  }
}
/*
DisplayPath

*/
void
DisplayTTY::DisplayPath( ListExpr type, ListExpr numType, ListExpr value)
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else if(nl->ListLength(value)!=1&&(nl->ListLength(value)-1)%2!=0)
     cout << "Incorrect Data Format" ;
  else{
     cout <<"*******************BEGIN PATH***************************"<<endl;
     bool err=false;
     int key = (int) getNumeric(nl->First(nl->First(value)),err);
     if(err){
        cout << "Incorrect Data Format";
        return;
     }
     if( nl->IsAtom( nl->Second(nl->First(value)) )  &&
         nl->SymbolValue(  nl->Second(nl->First(value)) ) == "undef" )
     {
         cout << "vertex "<<key<<": no point defined";

     }
     else{
       double x = getNumeric(nl->First(nl->Second(nl->First(value))),err);
       if(err){
          cout << "Incorrect Data Format";
          return;
       }
       double y = getNumeric(nl->Second(nl->Second(nl->First(value))),err);
       if(err){
         cout << "Incorrect Data Format";
         return;
       }
       cout << "vertex "<<key<<": (" << x << "," << y << ")"<<endl;
       }
       value=nl->Rest(value);
       while (!nl->IsEmpty( value ))
       {

        double cost =  getNumeric(nl->First(value),err);
        if(err){
          cout << "Incorrect Data Format";
          return;
        }
        cout <<"---"<<cost<<"--->"<<endl;
 
        int key = (int) getNumeric(nl->First(nl->Second(value)),err);
        if(err){
           cout << "Incorrect Data Format";
           return;
        }
        if( nl->IsAtom( nl->Second(nl->Second(value)) )  &&
            nl->SymbolValue(  nl->Second(nl->Second(value)) ) == "undef" )
        {
            cout << "vertex "<<key<<": no point defined";
        }
        else
        {
          double x = getNumeric(nl->First(nl->Second(nl->Second(value))),err);
          if(err){
            cout << "Incorrect Data Format";
            return;
        }
        double y = getNumeric(nl->Second(nl->Second(nl->Second(value))),err);
        if(err){
           cout << "Incorrect Data Format";
           return;
        }
        cout << "vertex "<<key<<": (" << x << "," << y << ")"<<endl;
      }
      value = nl->Rest(nl->Rest( value ));
   }
   cout <<"********************END PATH****************************"<<endl;
  }
}
/*
DisplayGraph

*/
void
DisplayTTY::DisplayGraph( ListExpr type, ListExpr numType, ListExpr value)
{
  if( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
      nl->SymbolValue( value ) == "undef" )
  {
     cout << "UNDEFINED";
  }
  else if(nl->ListLength(value)!=2)
     cout << "Incorrect Data Format ";
  else{
     ListExpr vertices=nl->First(value);
     ListExpr edges=nl->Second(value);
     cout <<"*******************BEGIN GRAPH***************************"<<endl;
     bool err=false;
     cout <<"********************VERTICES*****************************"<<endl;
     while (!nl->IsEmpty( vertices ))
     {

        int key = (int) getNumeric(nl->First(nl->First(vertices)),err);
        if(err){
          cout << "Incorrect Data Format";
          return;
        }
        if( nl->IsAtom( nl->Second(nl->First(vertices)) )  &&
            nl->SymbolValue(  nl->Second(nl->First(vertices)) ) == "undef" )
        {
            cout << key<<": no point defined"<<endl;;

        }
        else
        {
         double x = getNumeric(nl->First(nl->Second(nl->First(vertices))),err);
         if(err){
            cout << "Incorrect Data Format";
            return;
         }
         double y = getNumeric(nl->Second(nl->Second(nl->First(vertices))),err);
        if(err){
           cout << "Incorrect Data Format";
           return;
        }
        cout << key<<": (" << x << "," << y << ")"<<endl;
        }
        vertices = nl->Rest(vertices);
  
     }
     cout <<"**********************EDGES******************************"<<endl;
     while (!nl->IsEmpty( edges ))
     {
        if(nl->ListLength(nl->First(edges))!=3)
            cout << "Incorrect Data Format";
         else{
            bool err=false;
            int key1 = (int)getNumeric(nl->First(nl->First(edges)),err);
            if(err){
               cout << "Incorrect Data Format";
               return;
            }
 
            int key2 = (int)getNumeric(nl->Second(nl->First(edges)),err);
            if(err){
               cout << "Incorrect Data Format";
               return;
            }
            double cost = getNumeric(nl->Third(nl->First(edges)),err);
            if(err){
              cout << "Incorrect Data Format";
              return;
            }
            cout << key1<<"---" << cost << "---->" << key2<<endl;
         }
         edges = nl->Rest(edges);

      }
     cout <<"********************END GRAPH****************************"<<endl;
  }
}



/*
DisplayHistogram2d

*/

void
DisplayTTY::DisplayHistogram2d( ListExpr type, 
							 ListExpr numType, 
							 ListExpr value )
{
  cout << setprecision(16);
  NList val = NList(value);
	bool err;
	if ( val.isAtom() && val.isSymbol() &&
			val.str() == "undef" )
	{
		cout << "UNDEFINED";
	}


	else if ( val.length() != 3 ||  
    ((val.first().length()-1) 
    	* (val.second().length()-1)!= 
    		val.third().length()))
		{
			err = true;
			cout << "Incorrect Data Format"; 
			nl->WriteListExpr(value, cout);
		}
		else
	{

		NList rangesX = val.first();
    NList rangesXCopy = val.first();
    NList rangesY = val.second();
    NList bins = val.third();
    
    // Maximum ermitteln, zur Skalierung
    NList elem, temp = NList();
    double maxRangeX = 0;
    double minRange = 1e300;
    double last = getNumeric(rangesX.first().listExpr(), err);
    double d, dist;
    temp.append(rangesX.first());
    rangesX.rest();
    while(!rangesX.isEmpty())
    {
        elem = rangesX.first();
        d = getNumeric(elem.listExpr(), err);
        dist = d - last;
        maxRangeX = maxRangeX > dist ? maxRangeX : dist;
        minRange = minRange < dist ? minRange : dist;
        last = d;
        rangesX.rest(); // erstes Element entfernen
        temp.append(elem);
    } 
    rangesX = temp;
    
    // Maximum ermitteln, zur Skalierung
    /*NList elem,*/ temp = NList();
    double maxBin = 1;
    //double d;
    while(!bins.isEmpty())
    {
        elem = bins.first();
        d = getNumeric(elem.listExpr(), err);
        maxBin = maxBin > d ? maxBin : d;
        
        bins.rest(); // erstes Element entfernen
        temp.append(elem);
    } 
    bins = temp;  
    
    cout << endl;
    cout << "HISTOGRAM2D:" << endl;
    cout<< endl;

    double rangeY = getNumeric(rangesY.first().listExpr(), err);
    cout << "************* RangeY:"<< rangeY;
    cout << " **************"<< endl;
    rangesY.rest(); //paints first rangeY
    
    int height;
    int width;

    while ( !(rangesY.isEmpty()))
    {
      double rangeX = getNumeric(rangesX.first().listExpr(), err);
      cout << "x: " << rangeX << endl;
      rangesX.rest(); //paints first rangeX

      double rangeY = getNumeric(rangesY.first().listExpr(), err);

      while ( !(rangesX.isEmpty()))
      {
        double lastRange = rangeX;
        rangeX = getNumeric(rangesX.first().listExpr(), err);
        double bin = getNumeric(bins.first().listExpr(), err);
        
        // only scale, if bin greater than 80
        if (maxBin <= 80)
        {
          height = bin+1;
        }
        else {
          height = (bin+1)/maxBin*80.0/(rangeX-lastRange)*minRange;
        }
        string space = "";
        
        for (int i = 0; i < height;++i)
        {
          cout << "_";
          space += " ";
        }
        
        cout << endl;
        
        string binStr = "";
        
        stringstream ss(stringstream::in | stringstream::out);
        ss << bin;
        ss >> binStr;
        
        int numLen = binStr.length();
        
        width = (rangeX-lastRange)*7.0/maxRangeX;
        
        for (int i = 0; i < width - 1;i++)
        {
          if (i == 0)
            if (numLen < space.length())
             cout << bin << space.substr(numLen) << "|" << endl;
            else
              cout << space << "|" << bin << endl;
          else
            cout << space << "|" << endl;
        }
        
        if (width < 2)
          if (numLen < space.length())
           cout << bin << space.substr(numLen) << "|" << endl;
          else
            cout << space << "|" << bin << endl;
        else
          cout << space << "|" << endl;
        
        for (int i = 0; i < height;++i)
        {
          cout << "_";
        }     
        cout << "|" << (bin/(rangeX-lastRange)) << endl;
        cout << "x: "<< rangeX << endl;

        rangesX.rest();
        bins.rest();
      } // while ( !(nl->IsEmpty(rangesX)) )

      rangesX = rangesXCopy;
      cout << "*********** RangeY: " << rangeY;
      cout << " ***************" << endl;
      rangesY.rest();
    } // while( !(nl->IsEmpty(rangesY)) )

	}
	cout << endl;
}


/*
DisplayHistogram1d

*/

void
DisplayTTY::DisplayHistogram1d( ListExpr type, 
							 ListExpr numType, 
							 ListExpr value )
{
  cout << setprecision(16);
  NList val = NList(value);
	bool err;
	if ( val.isAtom() && val.isSymbol() &&
			val.str() == "undef" )
	{
		cout << "UNDEFINED";
	}


	else if ( val.length() != 2 ||  
    (!(val.first().length()) 
    		== val.second().length() + 1))
		{
			err = true;
			cout << "Incorrect Data Format"; 
			nl->WriteListExpr(value, cout);
		}


		else
	{
    NList ranges = val.first();
    NList bins = val.second();
    
		// Maximum ermitteln, zur Skalierung
	  NList elem, temp = NList();
	  double maxRange = 0;
	  double minRange = 1e300;
	  double last = getNumeric(ranges.first().listExpr(), err);
	  double d, dist;
	  temp.append(ranges.first());
	  ranges.rest();
	  while(!ranges.isEmpty())
	  {
	      elem = ranges.first();
	      d = getNumeric(elem.listExpr(), err);
	      dist = d - last;
	      maxRange = maxRange > dist ? maxRange : dist;
	      minRange = minRange < dist ? minRange : dist;
	      last = d;
	      ranges.rest(); // erstes Element entfernen
	      temp.append(elem);
	  } 
	  ranges = temp;
	  
    // Maximum ermitteln, zur Skalierung
    /*NList elem,*/ temp = NList();
    double maxBin = 1;
    //double d;
    while(!bins.isEmpty())
    {
        elem = bins.first();
        d = getNumeric(elem.listExpr(), err);
        maxBin = maxBin > d ? maxBin : d;
        
        bins.rest(); // erstes Element entfernen
        temp.append(elem);
    } 
    bins = temp;	  

		cout << endl;
		cout << "HISTOGRAM:" << endl;
		cout<< endl;
		double range = getNumeric(ranges.first().listExpr(), err);
		cout << range << endl;
		ranges.rest();
		int height;
		int width;
		while ( !(ranges.isEmpty()) ){
		  double lastRange = range;
			range = getNumeric(ranges.first().listExpr(), err);
			
			double bin = getNumeric(bins.first().listExpr(), err);
			
			// only scale, if bin greater than 80
			if (maxBin <= 80)
			{
			  height = bin+1;
			}
			else {
        height = (bin+1)/maxBin*80.0/(range-lastRange)*minRange;
			}
			string space = "";
			
			for (int i = 0; i < height;++i)
			{
			  cout << "_";
			  space += " ";
			}
			
			cout << endl;
			
      string binStr = "";
      
      stringstream ss(stringstream::in | stringstream::out);
      ss << bin;
      ss >> binStr;
      
			int numLen = binStr.length();	
			
			width = (range-lastRange)*7.0/maxRange;
			
			for (int i = 0; i < width - 1;i++)
			{
        if (i == 0)
          if (numLen < space.length())
           cout << bin << space.substr(numLen) << "|" << endl;
          else
            cout << space << "|" << bin << endl;
        else
          cout << space << "|" << endl;
			}
			
      if (width < 2)
        if (numLen < space.length())
         cout << bin << space.substr(numLen) << "|" << endl;
        else
          cout << space << "|" << bin << endl;
      else
        cout << space << "|" << endl;
			
      for (int i = 0; i < height;++i)
      {
        cout << "_";
      }			
      cout << "|" << bin/(range-lastRange) << endl;
			cout << range << endl;

			ranges.rest();
			bins.rest();
		}

	}
	cout << endl;
}




/*
DisplayPosition

*/
void
DisplayTTY::DisplayPosition( ListExpr type, ListExpr numType, ListExpr value )
{
  string agents[ 64 ];

  if ( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
       nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else if ( nl->ListLength( value ) != 2 )
    cout << "Incorrect Data Format";
  else
  {
    ListExpr rowList, agent;
    ListExpr moveNoExpr = nl->First( value );
    ListExpr agentList = nl->Second( value );

    int moveNumber;
    if ( nl->IsAtom( moveNoExpr ) && nl->AtomType( moveNoExpr ) == IntType )
      moveNumber = nl->IntValue( moveNoExpr );
    else
    {
      cout << "Incorrect Data Format";
      return ;
    }

    if ( nl->ListLength( agentList ) != 8 )
      cout << "Incorrect Data Format";
    else
    {
      int row = 7;
      while ( !nl->IsEmpty( agentList ) )
      {
        int file = 0;
        rowList = nl->First( agentList );
        agentList = nl->Rest( agentList );

        if ( nl->ListLength( rowList ) != 8 )
          cout << "Incorrect Data Format";
        else
        {
          while ( !nl->IsEmpty( rowList ) )
          {
            agent = nl->First( rowList );
            rowList = nl->Rest( rowList );
            if ( nl->IsAtom( agent )
                 && nl->AtomType( agent ) == StringType )
            {
              agents[ ( row * 8 ) + file ] = nl->StringValue( agent );
            }
            else
            {
              cout << "Incorrect Data Format";
              return ;
            }
            file++;
          }
          row--;
        }
      }

      cout << "Position: (after move number " 
           << moveNumber << ")" << endl << endl;
      cout << "        a b c d e f g h" << endl;
      cout << "       ----------------" << endl;
      for ( int row = 7; row >= 0; row-- )
      {
        cout << "      " << row + 1 << "|";
        for ( int file = 0; file < 8; file++ )
        {
          cout << agents[ ( row * 8 ) + file ] << " ";
        }
        cout << endl;
      }
      cout << endl;
    }
  }
}
/*
DisplayMove

*/

void
DisplayTTY::DisplayMove( ListExpr type, ListExpr numType, ListExpr value )
{
  if ( nl->IsAtom( value ) && nl->AtomType( value ) == SymbolType &&
       nl->SymbolValue( value ) == "undef" )
  {
    cout << "UNDEFINED";
  }
  else if ( nl->ListLength( value ) != 8 )
    cout << "Incorrect Data Format";
  else
  {
    int moveNumber;
    string agent, captured, startfile, endfile;
    int startrow, endrow;
    bool check;
    ListExpr current = nl->First( value );
    ListExpr rest = nl->Rest( value );

    // read moveNumber
    if ( nl->IsAtom( current ) && nl->AtomType( current ) == IntType )
      moveNumber = nl->IntValue( current );
    else
    {
      cout << "Incorrect Data Format";
      return ;
    }

    // read agent
    current = nl->First( rest );
    rest = nl->Rest( rest );
    if ( nl->IsAtom( current ) && nl->AtomType( current ) == StringType )
      agent = nl->StringValue( current );
    else
    {
      cout << "Incorrect Data Format";
      return ;
    }

    // read captured
    current = nl->First( rest );
    rest = nl->Rest( rest );
    if ( nl->IsAtom( current ) && nl->AtomType( current ) == StringType )
      captured = nl->StringValue( current );
    else
    {
      cout << "Incorrect Data Format";
      return ;
    }

    // read startfile
    current = nl->First( rest );
    rest = nl->Rest( rest );
    if ( nl->IsAtom( current ) && nl->AtomType( current ) == StringType )
      startfile = nl->StringValue( current );
    else
    {
      cout << "Incorrect Data Format";
      return ;
    }
    // read startrow
    current = nl->First( rest );
    rest = nl->Rest( rest );
    if ( nl->IsAtom( current ) && nl->AtomType( current ) == IntType )
      startrow = nl->IntValue( current );
    else
    {
      cout << "Incorrect Data Format";
      return ;
    }

    // read endfile
    current = nl->First( rest );
    rest = nl->Rest( rest );
    if ( nl->IsAtom( current ) && nl->AtomType( current ) == StringType )
      endfile = nl->StringValue( current );
    else
    {
      cout << "Incorrect Data Format";
      return ;
    }

    // read endrow
    current = nl->First( rest );
    rest = nl->Rest( rest );
    if ( nl->IsAtom( current ) && nl->AtomType( current ) == IntType )
      endrow = nl->IntValue( current );
    else
    {
      cout << "Incorrect Data Format";
      return ;
    }

    // read check
    current = nl->First( rest );
    rest = nl->Rest( rest );
    if ( nl->IsAtom( current ) && nl->AtomType( current ) == BoolType )
      check = nl->BoolValue( current );
    else
    {
      cout << "Incorrect Data Format";
      return ;
    }

    cout << "Move number: " << moveNumber;
    if ( check )
      cout << " (Check!)" << endl;
    else
      cout << endl;
    
    cout << agent << " moves from "
    << startfile << startrow << " to " << endfile << endrow;

    if ( ( captured != "none" ) && ( captured != "None" ) )
      cout << " and captures " << captured << endl;
    else
      cout << endl;

    
    cout << endl;
  }
}

/*
This is the general TTY display function for all collection with list
type collectio(subtyp). In our case this is vector an set.
In order to keep the functionality of the subtype, the display function of
the subtype is used by calling it through the CallDisplayFunction method.

*/

void
DisplayTTY::DisplayCollection( ListExpr type, ListExpr numType, ListExpr value)
{
  if(nl->ListLength(value)==0){
    cout << "empty vector";
  }else{
    cout << nl->ToString(nl->First(type)) << " of ";
    cout << nl->ToString(nl->Second(type)) << endl;
    int no=1;
    if(nl->IsAtom(nl->Second(type))){
      while(!nl->IsEmpty(value)){
        cout << "Item No: " << no++ << " = ";
        CallDisplayFunction(nl->Second(numType),
  nl->Second(type),nl->Second(numType),nl->First(value));
        cout << endl;
        value = nl->Rest(value);
      }
    }else{
      while(!nl->IsEmpty(value)){
        cout << "Item No: " << no++ << " = ";
        CallDisplayFunction(nl->First(nl->Second(numType)),
  nl->Second(type),nl->Second(numType),nl->First(value));
        cout << endl;
        value = nl->Rest(value);
      }
    }
  }
}


/*
This is the Displayfunction for collections of list
type collection(subtype amount)
In our case this is the multiset collection.

*/
void
DisplayTTY::DisplayMultiset( ListExpr type, ListExpr numType, ListExpr value)
{
  if(nl->ListLength(value)==0){
    cout << "empty vector";
  }else{
    cout << nl->ToString(nl->First(type)) << " of ";
    cout << nl->ToString(nl->Second(type)) << endl;
    int no=1;
    if(nl->IsAtom(nl->Second(type))){
      while(!nl->IsEmpty(value)){
        cout << "Item No: " << no++ << " = ";
        CallDisplayFunction(nl->Second(numType),
  nl->Second(type),nl->Second(numType),nl->First(nl->First(value)));
        cout << " Anzahl: " << nl->IntValue(nl->Second(nl->First(value)));
  cout << endl;
        value = nl->Rest(value);
      }
    }else{
      while(!nl->IsEmpty(value)){
        cout << "Item No: " << no++ << " = ";
        CallDisplayFunction(nl->First(nl->Second(numType)),
  nl->Second(type),nl->Second(numType),nl->First(nl->First(value)));
        cout << endl;cout << "Anzahl: ";
  cout << nl->IntValue(nl->Second(nl->First(value))) << endl;
  value = nl->Rest(value);
      }
    }
  }
}



void
DisplayTTY::DisplayResult( ListExpr type, ListExpr value )
{
  int algebraId, typeId;
  string  name;
  si->LookUpTypeExpr( type, name, algebraId, typeId );
  ListExpr numType = si->NumericTypeExpr( type );
  if ( !nl->IsAtom( type ) )
  {
    CallDisplayFunction( nl->First( numType ), type, numType, value );
  }
  else
  {
    CallDisplayFunction( numType, type, numType, value );
  }
  nl->Destroy( numType );
  cout << endl;
}

void
DisplayTTY::DisplayDescriptionLines( ListExpr value, int  maxNameLen)
{
   NList list(value);
   string errMsg = string("Error: Unknown List Format. ") + 
                   "Expecting (name (labels) (entries)) but got \n" +
                   list.convertToString() + "\n";

   if ( !list.hasLength(3) ) {
     cout << errMsg << endl;
     return;
   }

   NList nameSymbol = list.first();   
   NList labels = list.second();
   NList entries  = list.third();

   if ( ! (  nameSymbol.isSymbol() 
             && labels.isList()
             && entries.isList()   ) ) 
   {
     cout << errMsg;
     return;
   }  
   
   if ( !(labels.length() == entries.length()) )
   {
     cout << "Error: Length of the label list does not "
          << "equal the length of the entries list." << endl;
     cout << errMsg << endl;
     return;
   } 
   
   cout << endl;
   string name = nameSymbol.str();
   string blanks = "";
   blanks.assign( maxNameLen-4 , ' ' );
   cout << blanks << "Name: " << name << endl;

   try {
   while ( !labels.isEmpty() )
   {
     string s = labels.first().str();
     blanks.assign( maxNameLen - s.length() , ' ' );
     //cout << blanks << s << ": ";
     string printstr = blanks + s + ": ";
     printstr += entries.first().str();
     cout << wordWrap(0, maxNameLen+2, LINELENGTH, printstr) << endl;
     labels.rest();
     entries.rest();
   }
  }
  catch (NListErr err) {
   cout << "The elements of the two sublists labels and entries"
        << " need to be text or string atoms." << endl
        << err.msg() << endl;
  } 
}


ListExpr
DisplayTTY::ConcatLists( ListExpr list1, ListExpr list2)
{
  if (nl->IsEmpty(list1))
  {
    return list2;
  }
  else
  {
    ListExpr first = nl->First(list1);
    ListExpr rest = nl->Rest(list1);

    ListExpr second =  ConcatLists(rest, list2);

    ListExpr newnode = nl->Cons(first,second);
    return newnode;
  }
}


/*
The function below is not a display function but used to display
lists starting with (inquiry ...). It is called directly by 
SecondoTTY.

*/

void
DisplayTTY::DisplayResult2( ListExpr value )
{
  ListExpr InquiryType = nl->First(value);
  string TypeName = nl->SymbolValue(InquiryType);
  ListExpr v = nl->Second(value);
  if(TypeName=="databases")
  {
    cout << endl << "--------------------" << endl;
    cout << "Database(s)" << endl;
    cout << "--------------------" << endl;
    if(nl->ListLength(v)==0)
       cout << "none" << endl;
    while(!nl->IsEmpty(v)){
      cout << "  " << nl->SymbolValue(nl->First(v)) << endl;
      v = nl->Rest(v);
    }
    return;
  } 
  else if(TypeName=="algebras")
  {
    cout << endl << "--------------------" << endl;
    cout << "Algebra(s) " << endl;
    cout << "--------------------" << endl;
    if(nl->ListLength(v)==0)
       cout << "none" << endl;
    while(!nl->IsEmpty(v))
    {
      cout << "  " << nl->SymbolValue(nl->First(v)) << endl;
      v = nl->Rest(v);
    }
    return;
  } 
  else if(TypeName=="types")
  {
      nl->WriteListExpr(v,cout);
      return;
  }
  else if(TypeName=="objects")
  {
    ListExpr tmp = nl->Rest(v); // ignore the OBJECTS
    if(! nl->IsEmpty(tmp))
    {
      cout << " short list " << endl;
      while(!nl->IsEmpty(tmp))
      {
        cout << "  * " << nl->SymbolValue(nl->Second(nl->First(tmp)));
        cout << endl;
        tmp = nl->Rest(tmp);
      }
      cout << endl << "---------------" << endl;
      cout << " complete list " << endl;
    }
    nl->WriteListExpr(v,cout);
    return;
  } 
  else if(TypeName=="constructors" || TypeName=="operators")
  {
    cout << endl << "--------------------" << endl;
    if(TypeName=="constructors")
      cout <<"Type Constructor(s)\n";
    else 
      cout << "Operator(s)" << endl;
    cout << "--------------------" << endl;
    if(nl->IsEmpty(v))
    {
     cout <<"  none " << endl;
    } 
    else 
    {
      ListExpr headerlist = v;
      int MaxLength = 0;
      int currentlength;
      while(!nl->IsEmpty(headerlist))
      {
        ListExpr tmp = (nl->Second(nl->First(headerlist)));
        while(!nl->IsEmpty(tmp))
        {
          currentlength = (nl->StringValue(nl->First(tmp))).length();
          tmp = nl->Rest(tmp);
          if(currentlength>MaxLength)
            MaxLength = currentlength;
        }
        headerlist = nl->Rest(headerlist);
      }

      while (!nl->IsEmpty( v ))
      {
        DisplayDescriptionLines( nl->First(v), MaxLength );
        v   = nl->Rest( v );
      }
    }
  }
  else if(TypeName=="algebra")
  {
    string AlgebraName = nl->SymbolValue(nl->First(v));
    cout << endl << "-----------------------------------" << endl;
    cout << "Algebra : " << AlgebraName << endl;
    cout << "-----------------------------------" << endl;
    ListExpr Cs = nl->First(nl->Second(v));
    ListExpr Ops = nl->Second(nl->Second(v));
    // determine the headerlength
    ListExpr tmp1 = Cs;
    int maxLength=0;
    int len;
    while(!nl->IsEmpty(tmp1))
    {
      ListExpr tmp2 = nl->Second(nl->First(tmp1));
      while(!nl->IsEmpty(tmp2))
      {
        len = (nl->StringValue(nl->First(tmp2))).length();
        if(len>maxLength)
          maxLength=len;
         tmp2 = nl->Rest(tmp2);
      }
      tmp1 = nl->Rest(tmp1);
    }
    tmp1 = Ops;
    while(!nl->IsEmpty(tmp1))
    {
      ListExpr tmp2 = nl->Second(nl->First(tmp1));
      while(!nl->IsEmpty(tmp2))
      {
        len = (nl->StringValue(nl->First(tmp2))).length();
        if(len>maxLength)
          maxLength=len;
        tmp2 = nl->Rest(tmp2);
      }
      tmp1 = nl->Rest(tmp1);
    } 

    cout << endl << "-------------------------" << endl;
    cout << "  "<< "Type Constructor(s)" << endl;
    cout << "-------------------------" << endl;
    if(nl->ListLength(Cs)==0)
       cout << "  none" << endl;
    while(!nl->IsEmpty(Cs))
    {
      DisplayDescriptionLines(nl->First(Cs),maxLength);
      Cs = nl->Rest(Cs);
    }

    cout << endl << "-------------------------" << endl;
    cout << "  " << "Operator(s)" << endl;
    cout << "-------------------------" << endl;
    if(nl->ListLength(Ops)==0)
      cout << "  none" << endl;
    while(!nl->IsEmpty(Ops))
    {
      DisplayDescriptionLines(nl->First(Ops),maxLength);
      Ops = nl->Rest(Ops);
    }
  }
  else
  {
    cout << "unknow inquiry type" << endl;
    nl->WriteListExpr(value,cout);
  }
}

void
DisplayTTY::Initialize( SecondoInterface* secondoInterface )
{
  si = secondoInterface;
  nl = si->GetNestedList();
  InsertDisplayFunction( "int",     &DisplayInt );
  InsertDisplayFunction( "real",    &DisplayReal );
  InsertDisplayFunction( "bool",    &DisplayBoolean );
  InsertDisplayFunction( "string",  &DisplayString );
  InsertDisplayFunction( "rel",     &DisplayRelation );
  InsertDisplayFunction( "trel",    &DisplayRelation );
  InsertDisplayFunction( "mrel",    &DisplayRelation );
  InsertDisplayFunction( "tuple",   &DisplayTuples );
  InsertDisplayFunction( "mtuple",  &DisplayTuples );
  InsertDisplayFunction( "map",     &DisplayFun );
  InsertDisplayFunction( "date",    &DisplayDate );
  InsertDisplayFunction( "text",    &DisplayText );
  InsertDisplayFunction( "xpoint",  &DisplayXPoint);
  InsertDisplayFunction( "rect",    &DisplayRect);
  InsertDisplayFunction( "rect3",   &DisplayRect3);
  InsertDisplayFunction( "rect4",   &DisplayRect4);
  InsertDisplayFunction( "rect8",   &DisplayRect8);
  InsertDisplayFunction( "array",   &DisplayArray);
  InsertDisplayFunction( "point",   &DisplayPoint);
  InsertDisplayFunction( "binfile", &DisplayBinfile);
  InsertDisplayFunction( "mp3",     &DisplayMP3);
  InsertDisplayFunction( "id3",     &DisplayID3);
  InsertDisplayFunction( "lyrics",  &DisplayLyrics);
  InsertDisplayFunction( "midi",    &DisplayMidi);
  InsertDisplayFunction( "instant", &DisplayInstant);
  InsertDisplayFunction( "duration",&DisplayDuration);
  InsertDisplayFunction( "tid",     &DisplayTid);
  InsertDisplayFunction( "html",     &DisplayHtml);
  InsertDisplayFunction( "page",     &DisplayPage);
  InsertDisplayFunction( "url",     &DisplayUrl);
  InsertDisplayFunction( "vertex",  &DisplayVertex);
  InsertDisplayFunction( "edge",  &DisplayEdge);
  InsertDisplayFunction( "path",  &DisplayPath);
  InsertDisplayFunction( "graph",  &DisplayGraph);
  
  InsertDisplayFunction( "histogram1d", &DisplayHistogram1d);
  InsertDisplayFunction( "histogram2d", &DisplayHistogram2d);

  InsertDisplayFunction( "vector", &DisplayCollection );
  InsertDisplayFunction( "set", &DisplayCollection );
  InsertDisplayFunction( "multiset", &DisplayMultiset );

  
  InsertDisplayFunction( "position", &DisplayPosition );
  InsertDisplayFunction( "move", &DisplayMove );
}

