/*

\title{DateTime Algebra}
\author{Thomas Behr}
\maketitle
\noindent

\abstract{
The DateTime-Algebra is originally writtes as a Part of the Periodic-Algebra.
Because time is a important data type which is needed in
several algebras, the DateTime type is extracted from this
Periodic Algebra. The class declaration is located in the File
SECONDO\_HOME\_DIR/include/DateTime.h.
}

\tableofcontents

1 Includes and Definitions

*/

#ifndef __DATE_TIME_H__
#include "DateTime.h"
#endif

#include <string>
#include <iostream>
#include <sstream>
#include "NestedList.h"
#include "Algebra.h"
#include "QueryProcessor.h"
#include "SecondoSystem.h"
#include "Attribute.h"
#include "StandardAttribute.h"
#include "StandardTypes.h"
#include <math.h>
#include <time.h>
#include <sys/timeb.h>


extern NestedList* nl;
extern QueryProcessor *qp;

using namespace std;

namespace datetime{

/*
~GetValue~

The function ~GetValue~ returns the integer value of an given
character. If this character does not represent any digit, -1
is returned.

*/
static int GetValue(char c){
  switch(c){
    case '0' : return 0;
    case '1' : return 1;
    case '2' : return 2;
    case '3' : return 3;
    case '4' : return 4;
    case '5' : return 5;
    case '6' : return 6;
    case '7' : return 7;
    case '8' : return 8;
    case '9' : return 9;
    default  : return -1;
  }
}



/*
4.2 The Implementation of the Class DateTime

~The Standard Constructor~

This constructor makes nothing and should never be called.
It is used in a special way in a cast function.

*/
DateTime::DateTime(){}

/*
~Constructor with a dummy argument~

This constructor ignores the argument. The purpose of this
parameter is to make this constructor distinguishable to
the standard one, which can't have any implementation.

*/
DateTime::DateTime(int dummy){
  day=0;
  milliseconds=0;
  defined = true;
  canDelete=false;
}


/*
~A Constructor ~

The Value of MilliSeconds has to be greater than or equals to zero.

*/
DateTime::DateTime(const long Day,const long MilliSeconds){
   assert(MilliSeconds>=0);
   day = Day;
   milliseconds = MilliSeconds;
   if(milliseconds>=MILLISECONDS){
      long dif = milliseconds / MILLISECONDS;
      day += dif;
      milliseconds -= dif*MILLISECONDS;
   }
   defined = true;
   canDelete=false;
}

/*
~Destructor~

*/
DateTime::~DateTime(){}

void DateTime::Destroy(){
   canDelete=true;
}



/*
~Now~

Gets the value of this DateTime from the System.
*/
void DateTime::Now(){
  timeb tb;
  time_t now;
  int ms;

  ftime(&tb);
  now = tb.time;
  ms = tb.millitm;

  tm* time = localtime(&now);
  day = ToJulian(time->tm_year+1900,time->tm_mon+1,time->tm_mday);
  milliseconds = ((((time->tm_hour)*60)+time->tm_min)*60+time->tm_sec)*1000+ms;
}

/*
~GetDay~

This function returns the day of this Time instance as
difference to the reference time.

*/
long DateTime::GetDay(){
   return day;
}

/*
~GetAllMilliSeconds~

This function returns the milliseconds of the day
of this Time instance.

*/
long DateTime::GetAllMilliSeconds(){
   return milliseconds;
}

/*
~Gregorian Calender~

The following functions return single values of this DateTime instance.

*/

int DateTime::GetGregDay(){
    int y,m,d;
    ToGregorian(day,y,m,d);
    return d;
}

int DateTime::GetMonth(){
   int y,m,d;
   ToGregorian(day,y,m,d);
   return m;
}
int DateTime::GetYear(){
   int y,m,d;
   ToGregorian(day,y,m,d);
   return y;
}

int DateTime::GetHour(){
   return (int)(milliseconds / 3600000);
}

int DateTime::GetMinute(){
   return (int) ( (milliseconds / 60000) % 60);
}

int DateTime::GetSecond(){
  return (int) ( (milliseconds / 1000) % 60);
}

int DateTime::GetMillisecond(){
  return (int) (milliseconds % 1000);
}

/*
~ToJulian~

This function computes the Julian day number of the given
gregorian date + the reference time.
Positive year signifies A.D., negative year B.C.
Remember that the year after 1 B.C. was 1 A.D.

Julian day 0 is a Monday.

This algorithm is from Press et al., Numerical Recipes
in C, 2nd ed., Cambridge University Press 1992

*/
long DateTime::ToJulian(int year, int month, int day){
  int jy = year;
  if (year < 0)
     jy++;
  int jm = month;
  if (month > 2)
     jm++;
  else{
     jy--;
     jm += 13;
  }

  int jul = (int)(floor(365.25 * jy) + floor(30.6001*jm)
                  + day + 1720995.0);
  int IGREG = 15 + 31*(10+12*1582);
  // Gregorian Calendar adopted Oct. 15, 1582
  if (day + 31 * (month + 12 * year) >= IGREG){
     // change over to Gregorian calendar
     int ja = (int)(0.01 * jy);
     jul += 2 - ja + (int)(0.25 * ja);
  }
  return jul-NULL_DAY;
}

/*
~ToGregorian~

This function converts a Julian day to a date in the gregorian calender.

This algorithm is from Press et al., Numerical Recipes
in C, 2nd ed., Cambridge University Press 1992

*/
void DateTime::ToGregorian(long Julian, int &year, int &month, int &day){
   int j=(int)(Julian+NULL_DAY);
   int ja = j;
   int JGREG = 2299161;
   /* the Julian date of the adoption of the Gregorian
      calendar
   */
    if (j >= JGREG){
    /* cross-over to Gregorian Calendar produces this
       correction
    */
       int jalpha = (int)(((float)(j - 1867216) - 0.25)/36524.25);
       ja += 1 + jalpha - (int)(0.25 * jalpha);
    }
    int jb = ja + 1524;
    int jc = (int)(6680.0 + ((float)(jb-2439870) - 122.1)/365.25);
    int jd = (int)(365 * jc + (0.25 * jc));
    int je = (int)((jb - jd)/30.6001);
    day = jb - jd - (int)(30.6001 * je);
    month = je - 1;
    if (month > 12) month -= 12;
    year = jc - 4715;
    if (month > 2) --year;
    if (year <= 0) --year;
}

/*

~ToDouble~

This function converts this Time instance to the corresponding
double value;

*/
double DateTime::ToDouble(){
   return (double)day + (double)milliseconds/MILLISECONDS;
}

/*
~ToString~

This function returns the string representation of this
Time instance in the format:\\
if the absolute flag is set: \\
YEAR-MONTH-DAY-HOUR:MINUTE:SECOND.MILLISECONDS\\
if the absolute flag is not set:\\
day\mid{}\mid{}milliseconds

*/
string DateTime::ToString(bool absolute){
  ostringstream tmp;
  if(!defined){
    return "undefined";
  }
  if(!absolute){
    tmp << day << "||" << milliseconds;
  }else{
    int day,month,year;
    ToGregorian(this->day,year,month,day);
    tmp << year << "-" << month << "-" << day;
    long value = milliseconds;
    long ms = value % 1000;
    value = value / 1000;
    long sec = value % 60;
    value = value / 60;
    long min = value % 60;
    long hour = value / 60;

    if(value==0) // without time
       return tmp.str();

    tmp << "-" << hour << ":";
    if(min<10)
      tmp << "0";
    tmp << min;

    if((sec==0) && (ms == 0))
       return tmp.str();

    tmp << ":";
    if(sec<10)
       tmp << "0";
    tmp << sec;
    if(ms==0)
       return tmp.str();

    tmp << ".";
    if(ms <100)
       tmp << "0";
    if(ms <10)
       tmp << "0";
    tmp << ms;
  }
  return tmp.str();
}


/*
~ReadFrom~

This function reads the Time from a given string. If the string
don't represent a valid Time value, this instance remains unchanged
and false is returned. In the other case, this instance will hold the
the time represented by the string and the result is true.
The format of the string must be:\\
 \hspace{1cm}    YEAR-MONTH-DAY-HOUR:MIN[:SECOND[.MILLISECOND]] \\
Spaces are not allowed in this string. The squared brackets
indicates optional parts.

*/
bool DateTime::ReadFrom(const string Time){

   canDelete = false;
   if(Time=="undefined"){
      defined=false;
      return true;
   }
   int year = 0;
   int digit;
   int len = Time.length();
   if(len==0) return false;
   int pos = 0;
   // read the year
   int signum = 1;
   if(Time[0]=='-'){
      signum=-1;
      pos++;
   }

   if(pos==len) return false;
   while(Time[pos]!='-'){
      digit = datetime::GetValue(Time[pos]);
      if(digit<0) return false;
      pos++;
      if(pos==len) return false;
      year = year*10+digit;
   }
   year = year*signum;
   pos++; // read over  '-'
   if(pos==len) return false;
   // read the month
   int month = 0;
   while(Time[pos]!='-'){
      digit = datetime::GetValue(Time[pos]);
      if(digit<0) return false;
      pos++;
      if(pos==len) return false;
      month = month*10+digit;
   }
   pos++; // read over '-'
   if(pos==len) return false;
   // read the day
   int day = 0;
   while(Time[pos]!='-'){
      digit = datetime::GetValue(Time[pos]);
      if(digit<0) return false;
      pos++;
      day = day*10+digit;
      if(pos==len){ // we allow pure date string without any hour
         if(!IsValid(year,month,day))
	    return false;
         this->day = ToJulian(year,month,day);;
	 milliseconds=0;
	 defined=true;
	 return true;
      }
   }
   pos++; // read over '-'
   if(pos==len) return false;
   if(!IsValid(year,month,day))
      return false;
   // read the hour
   int hour = 0;
   while(Time[pos]!=':'){
      digit = datetime::GetValue(Time[pos]);
      if(digit<0) return false;
      pos++;
      if(pos==len) return false;
      hour = hour*10+digit;
   }
   pos++; // read over ':'
   if(pos==len) return false;
   // read the minute
   int minute = 0;
   bool done = false;
   bool next = false;
   while(!done && !next){
      digit = datetime::GetValue(Time[pos]);
      if(digit<0) return false;
      pos++;
      minute = minute*10+digit;
      done = pos==len;
      if(!done)
          next = Time[pos]==':';
   }
   // initialize seconds and milliseconds with zero
   long seconds = 0;
   long mseconds = 0;
   if(!done){ // we have to read seconds
     pos++; // read over the ':'
     if(pos==len) return false;
     next = false;
     while(!done && !next){
         digit = datetime::GetValue(Time[pos]);
         if(digit<0) return false;
         pos++;
         seconds = seconds*10+digit;
         done = pos==len;
         if(!done)
           next = Time[pos]=='.';
     }
     if(!done ){ // milliseconds are available
        pos++; // read over the '.'
        if(pos==len) return false;
        next = false;
        while(!done){
            digit = datetime::GetValue(Time[pos]);
            if(digit<0) return false;
            pos++;
            mseconds = mseconds*10+digit;
            done = pos==len;
        }
     }
   }
   // At this place we have all needed information to create a date
   this->day = ToJulian(year,month,day);
   milliseconds = ((hour*60+minute)*60+seconds)*1000+mseconds;
   if(milliseconds>MILLISECONDS){
      long dif = milliseconds/MILLISECONDS;
      this->day += dif;
      milliseconds -= dif*MILLISECONDS;
   }
   defined=true;
   return true;
}


/*
~ReadFrom~

This functions reads the value of this instance from the given double.

*/
bool DateTime::ReadFrom(const double Time){
   day = (long) Time;
   long dms =  (long) ((Time - (double) day)*MILLISECONDS+0.5);
   if( dms<0 ){
      day--;
      dms += MILLISECONDS;
   }
   milliseconds = dms;
   defined = true;
   return true;
}

/*
~IsValid~

This functions checks if the given arguments represent a valid gregorian
date. E.G. this function will return false id month is greater than twelve,
or the day is not include in the given month/year.

*/
bool DateTime::IsValid(int year,int month,int day){
   long jday = ToJulian(year,month,day);
   int y=0,m=0,d=0;
   ToGregorian(jday,y,m,d);
   return year==y && month==m && day==d;
}

/*
~ReadFrom~

This function reads the time value from the given nested list.
If the format is not correct, this function will return false and this
instance remains unchanged. Otherwise this instance will take the value
represented by this list and the result will be true.

*/
bool DateTime::ReadFrom(ListExpr LE,const bool typeincluded){
  canDelete=false;
  ListExpr ValueList;
  if(typeincluded){
     if(nl->ListLength(LE)!=2){
        return false;
     }
     ListExpr TypeList = nl->First(LE);
     if(!nl->IsEqual(TypeList,"datetime") &&
        !nl->IsEqual(TypeList,"instant")){ // wrong type
        return false;
     }
     ValueList = nl->Second(LE);
  }else{
     ValueList=LE;
  }

  // Special Representation in this Algebra
  if(nl->AtomType(ValueList)==SymbolType){
    if(nl->SymbolValue(ValueList)=="now"){
          Now();
	  return true;
    }
  }

  // string representation
  if(nl->AtomType(ValueList)==StringType)
     return ReadFrom(nl->StringValue(ValueList));

  // real representation
  if(nl->AtomType(ValueList)==RealType )
     return ReadFrom(nl->RealValue(ValueList));

  // (day month year [hour minute [second [millisecond]]])
  if( (nl->ListLength(ValueList)>=3) && nl->ListLength(ValueList)<=7){
     int len = nl->ListLength(ValueList);
     if(len==4) return false; // only hours is not allowed
     ListExpr tmp = ValueList;
     while(nl->IsEmpty(tmp)){
        if(nl->AtomType(nl->First(tmp))!=IntType)
	   return false;
	tmp = nl->Rest(tmp);
     }
     int d,m,y,h,min,sec,ms;

     d = nl->IntValue(nl->First(ValueList));
     m = nl->IntValue(nl->Second(ValueList));
     y = nl->IntValue(nl->Third(ValueList));
     h = len>3? nl->IntValue(nl->Fourth(ValueList)):0;
     min = len>4? nl->IntValue(nl->Fifth(ValueList)):0;
     sec = len>5? nl->IntValue(nl->Sixth(ValueList)):0;
     ms = 0;
     if(len==7){
          ValueList = nl->Rest(ValueList);
	  ms = nl->IntValue(nl->Sixth(ValueList));
     }
     // check the ranges
     if(!IsValid(y,m,d)) return false;
     if(h<0 || h>23) return false;
     if(min<0 || min > 59) return false;
     if(sec<0 || sec > 59) return false;
     if(ms<0 || ms > 999) return false;
     // set the values
     this->day = ToJulian(y,m,d);
     this->milliseconds = (((h*60)+min)*60 + sec)*1000 +ms;
     defined = true;
     return true;
  }

  // (julianday milliseconds)
  if(nl->ListLength(ValueList)!=2){
    return false;
  }
  ListExpr DayList = nl->First(ValueList);
  ListExpr MSecList = nl->Second(ValueList);
  if(nl->AtomType(DayList)!=IntType || nl->AtomType(MSecList)!=IntType){
     return false;
  }
  day = nl->IntValue(DayList);
  milliseconds = nl->IntValue(MSecList);
  if(milliseconds>MILLISECONDS){
      long dif = milliseconds/MILLISECONDS;
      day += dif;
      milliseconds -= dif*MILLISECONDS;
  }
  return  true;
}

/*
~CompareTo~

This function compare this DateTime instance with another one.
The result will be:
\begin{description}
   \item[-1] if this instance is before P2
   \item[0] if this instance is equals to P2
   \item[1] if this instance is after P2
\end{description}

*/
int DateTime::CompareTo(DateTime* P2){
   if(!defined && !P2->defined)
      return 0;
   if(!defined && P2->defined)
      return -1;
   if(defined && !P2->defined)
      return 1;
   // at this point this and P2 are defined
   if(day<P2->day) return -1;
   if(day>P2->day) return 1;
   if(milliseconds<P2->milliseconds) return -1;
   if(milliseconds>P2->milliseconds) return 1;
   return 0;
}

/*
~Clone~

This funtion returns a copy of this instance.

*/
DateTime* DateTime::Clone(){
   DateTime* res = new DateTime(day,milliseconds);
   res->defined = defined;
   return res;
}

/*
~ReadFromSmiRecord~

This function reads the value of this DateTime instance from the
givem SmiRecord beginning from the given offset.

*/
void DateTime::ReadFromSmiRecord(SmiRecord& valueRecord,int& offset){
   valueRecord.Read(&day,sizeof(long),offset);
   offset += sizeof(long);
   valueRecord.Read(&milliseconds,sizeof(long),offset);
   offset += sizeof(long);
   valueRecord.Read(&defined,sizeof(bool),offset);
   offset += sizeof(bool);
   valueRecord.Read(&canDelete,sizeof(bool),offset);
   offset += sizeof(bool);
}


/*
~Open~

The ~Open~ function reads the DateTime value from the argument
{\tt valueRecord}. Because the DateTime class don't uses FLOBs,
this is very simple.

*/
void DateTime::Open(SmiRecord& valueRecord, const ListExpr typeinfo){
   int offset = 0;
   ReadFromSmiRecord(valueRecord,offset);
}

/*
~WriteToSmiRecord~

This function write the data part of this DateTime instance to
{\tt valueRecord} beginning at the given offset. After calling
this function the offset will be behind the written data part.

*/
void DateTime::WriteToSmiRecord(SmiRecord& valueRecord,int& offset){
   valueRecord.Write(&day,sizeof(long),offset);
   offset += sizeof(long);
   valueRecord.Write(&milliseconds,sizeof(long),offset);
   offset += sizeof(long);
   valueRecord.Write(&defined,sizeof(bool),offset);
   offset += sizeof(bool);
   valueRecord.Write(&canDelete,sizeof(bool),offset);
   offset += sizeof(bool);
}

/*
~Save~

The ~Save~ functions saves the data of the DateTime value to  {\tt valueRecord}.

*/
void DateTime::Save(SmiRecord& valueRecord, const ListExpr typeinfo){
   int offset = 0;
   WriteToSmiRecord(valueRecord,offset);
}

/*
~Compare~

This function compare this DateTime with the given Attribute.

*/
int DateTime::Compare(Attribute* arg){
  return CompareTo( (DateTime*) arg);
}

/*
~Adjacent~

This function returns true if this is directly neighbooring with arg.
Because we use a fixed time resolution, we can implement this function.

*/
bool DateTime::Adjacent(Attribute* arg){
  DateTime* T2 = (DateTime*) arg;
  if(day==T2->day && abs(milliseconds-T2->milliseconds)==1)
    return true;
  if((day-1==T2->day) && (milliseconds==MILLISECONDS-1) && (T2->milliseconds==0))
     return true;
  if( (day==T2->day-1) && (milliseconds==0) && (T2->milliseconds==MILLISECONDS-1))
     return true;
  return false;
}

/*
~Sizeof~

This functions returns the size of the class DateTime;

*/
int DateTime::Sizeof(){
  return sizeof(DateTime);
}

/*
~IsDefined~

~IsDefined~ returns true if this instance contains a
defined value.

*/
bool DateTime::IsDefined() const{
   return defined;
}

/*
~SetDefined~

The function ~SetDefined~ sets the defined flasg of this time
instance to the value of the argument.

*/
void DateTime::SetDefined( bool defined ){
   this->defined = defined;
}

/*
~HashValue~

This function return the HashValue for this DateTime instance.

*/
size_t DateTime::HashValue(){
  return (size_t) (int)(day*MILLISECONDS+milliseconds);
}

/*
~CopyFrom~

This Time instance take its value from arg if this function is
called.

*/
void DateTime::CopyFrom(StandardAttribute* arg){
   Equalize(((DateTime*) arg));
}

/*
~add~

Adds P2 to this instance. P2 remains unchanged.

*/
void DateTime::Add(DateTime* P2){
   day += P2->day;
   milliseconds += P2->milliseconds;
   if(milliseconds>MILLISECONDS){
      long dif = milliseconds/MILLISECONDS;
      day += dif;
      milliseconds -= dif*MILLISECONDS;
   }
}

/*
~minus~

Subtracts P2 from this instance.

*/
void DateTime::Minus(DateTime* P2){
   day -= P2->day;
   milliseconds -= P2->milliseconds;
   if(milliseconds<0){
      day--;
      milliseconds = MILLISECONDS+milliseconds; //note ms<0
   }
}

/*
~mul~

Computes the M'th multiple of this time.

*/
void DateTime::Mul(int factor){
   day = day*factor;
   milliseconds = milliseconds*factor;
   if(milliseconds>MILLISECONDS){
      long dif = milliseconds/MILLISECONDS;
      day += dif;
      milliseconds -= dif*MILLISECONDS;
   }
}

/*
~ToListExpr~

This functions returns this time value in nested list format.
The argument controls the format of the output.
If absolute is false, the value-representation will be a
list containing two int atoms. The first integer is the
day of this time and the second one the part of this day
in milliseconds. If the parameter is true, the value will be a
string in format year-month-day-hour:minute:second.millisecond

*/
ListExpr DateTime::ToListExpr(bool absolute,bool typeincluded){
  ListExpr value;

  if(absolute)
      value = nl->StringAtom(this->ToString(true));

  else
    value = nl->TwoElemList( nl->IntAtom((int)day),
		             nl->IntAtom((int)milliseconds)
		             );
  if(typeincluded)
    return nl->TwoElemList(nl->SymbolAtom("datetime"),value);
  else
    return value;
}

/*
~Equalize~

This function changes the value of this Time instance to be equal to
P2.

*/
void DateTime::Equalize(DateTime* P2){
   day = P2->day;
   milliseconds = P2->milliseconds;
   defined = P2->defined;
}

/*
~IsZero~

~IsZero~ returns true iff this

*/
bool DateTime::IsZero(){
  return day==0 && milliseconds==0;
}

/*
~LessThanZero~

This function returns true if this instnace is before the Null-Day

*/
bool DateTime::LessThanZero(){
   return day<0;
}


/*
2 Algebra Functions

2.1 In and Out functions

*/

ListExpr OutDateTime( ListExpr typeInfo, Word value ){
   DateTime* T = (DateTime*) value.addr;
   return T->ToListExpr(false,false);
}


Word InDateTime( const ListExpr typeInfo, const ListExpr instance,
              const int errorPos, ListExpr& errorInfo, bool& correct ){

  DateTime* T = new DateTime(0);
  if(T->ReadFrom(instance,false)){
    correct=true;
    return SetWord(T);
  }
  correct = false;
  delete(T);
  return SetWord(Address(0));
}


/*
2.2 Property Function

*/
ListExpr DateTimeProperty(){
  return (nl->TwoElemList(
            nl->FiveElemList(
                nl->StringAtom("Signature"),
                nl->StringAtom("Example Type List"),
                nl->StringAtom("List Rep"),
                nl->StringAtom("Example List"),
                nl->StringAtom("Remarks")),
            nl->FiveElemList(
                nl->StringAtom("-> Data"),
                nl->StringAtom("datetime"),
                nl->StringAtom("(int int)"),
                nl->StringAtom("(25 14400)"),
                nl->StringAtom("The list can also given as date string"))
         ));
}

/*
2.3 ~Create~ function

*/
Word CreateDateTime(const ListExpr typeInfo){
  return SetWord(new DateTime(11));
}

/*
2.4 ~Delete~ function

*/
void DeleteDateTime(Word &w){
  DateTime* T = (DateTime*) w.addr;
  T->Destroy();
  delete T;
  w.addr=0;
}


/*
2.5 ~Open~ function

*/
bool OpenDateTime( SmiRecord& valueRecord,
                const ListExpr typeInfo,
                Word& value ){
  DateTime* T = new DateTime(0);
  T->Open(valueRecord,typeInfo);
  value = SetWord(T);
  return true;
}

/*
2.6 ~Save~ function

*/
bool SaveDateTime( SmiRecord& valueRecord,
                const ListExpr typeInfo,
                Word& value ){
  DateTime* T = (DateTime *)value.addr;
  T->Save( valueRecord, typeInfo );
  return true;
}

/*
2.7 ~Close~ function

*/
void CloseDateTime( Word& w ){
  delete (DateTime *)w.addr;
  w.addr = 0;
}

/*
2.8 ~Clone~ function

*/
Word CloneDateTime( const Word& w )
{
  return SetWord( ((DateTime *)w.addr)->Clone() );
}

/*
2.9 ~SizeOf~-Function

*/
int SizeOfDateTime(){
  return sizeof(DateTime);
}

/*
2.10 ~Cast~-Function

*/
void* CastDateTime( void* addr )
{
  return new (addr) DateTime;
}

/*
2.11 Kind Checking Function

This function checks whether the type constructor is applied correctly. Since
the type constructor don't have arguments, this is trivial.

*/
bool CheckDateTime( ListExpr type, ListExpr& errorInfo )
{
  return (nl->IsEqual( type, "datetime" ));
}

/*
3 Type Constructor

*/
TypeConstructor datetime(
	"datetime",		//name
	DateTimeProperty, 	        //property function describing signature
        OutDateTime, InDateTime,	//Out and In functions
        0,           	                //SaveToList and
	0,                              //           RestoreFromList functions
	CreateDateTime, DeleteDateTime,	//object creation and deletion
        OpenDateTime,    SaveDateTime, 	//object open and save
        CloseDateTime,  CloneDateTime,    	//object close and clone
	CastDateTime,				//cast function
        SizeOfDateTime, 			//sizeof function
	CheckDateTime,	                //kind checking function
	0, 					//predef. pers. function for model
        TypeConstructor::DummyInModel,
        TypeConstructor::DummyOutModel,
        TypeConstructor::DummyValueToModel,
        TypeConstructor::DummyValueListToModel );


/*
4 Operators

4.1 Type Mappings

*/
ListExpr VoidDateTime(ListExpr args){
  if(nl->IsEmpty(args))
     return nl->SymbolAtom("datetime");
  return nl->SymbolAtom("typeerror");
}

ListExpr IntBool(ListExpr args){
  if(nl->ListLength(args)!=1)
     return nl->SymbolAtom("typeerror");
  if(nl->IsEqual(nl->First(args),"int"))
     return nl->SymbolAtom("bool");
  return nl->SymbolAtom("typeerror");
}

ListExpr DateTimeInt(ListExpr args){
  if(nl->ListLength(args)==1)
     if(nl->IsEqual(nl->First(args),"datetime"))
         return nl->SymbolAtom("int");
  return nl->SymbolAtom("typeerror");
}

ListExpr DateTimeDateTimeDateTime(ListExpr args){
  if(nl->ListLength(args)!=2)
     return nl->SymbolAtom("typeerror");
  if(nl->IsEqual(nl->First(args),"datetime") &&
     nl->IsEqual(nl->Second(args),"datetime"))
     return nl->SymbolAtom("datetime");
  return nl->SymbolAtom("typeerror");
}

ListExpr DateTimeDateTimeBool(ListExpr args){
  if(nl->ListLength(args)!=2)
     return nl->SymbolAtom("typeerror");
  if(nl->IsEqual(nl->First(args),"datetime") &&
     nl->IsEqual(nl->Second(args),"datetime"))
     return nl->SymbolAtom("bool");
  return nl->SymbolAtom("typeerror");
}

ListExpr DateTimeIntDateTime(ListExpr args){
  if(nl->ListLength(args)!=2)
     return nl->SymbolAtom("typeerror");
  if(nl->IsEqual(nl->First(args),"datetime") &&
     nl->IsEqual(nl->Second(args),"int"))
     return nl->SymbolAtom("datetime");
  return nl->SymbolAtom("typeerror");
}

ListExpr DateTimeString(ListExpr args){
  if(nl->ListLength(args)!=1)
     return nl->SymbolAtom("typeerror");
  if(nl->IsEqual(nl->First(args),"datetime"))
     return nl->SymbolAtom("string");
  return nl->SymbolAtom("typeerror");
}

/*
4.2 Value Mappings

*/
int LeapYearFun_intbool(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    CcInt* Y = (CcInt*) args[0].addr;
    DateTime T;
    bool res = T.IsValid(Y->GetIntval(),2,29);
    ((CcBool*) result.addr)->Set(true,res);
    return 0;
}

int NowFun_VoidDateTime(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    ((DateTime*) result.addr)->Now();
    return 0;
}


int DayFun_DTInt(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T = (DateTime*) args[0].addr;
    ((CcInt*) result.addr)->Set(true,T->GetGregDay());
    return 0;
}

int MonthFun_DTInt(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T = (DateTime*) args[0].addr;
    ((CcInt*) result.addr)->Set(true,T->GetMonth());
    return 0;
}

int YearFun_DTInt(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T = (DateTime*) args[0].addr;
    ((CcInt*) result.addr)->Set(true,T->GetYear());
    return 0;
}

int HourFun_DTInt(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T = (DateTime*) args[0].addr;
    ((CcInt*) result.addr)->Set(true,T->GetHour());
    return 0;
}

int MinuteFun_DTInt(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T = (DateTime*) args[0].addr;
    ((CcInt*) result.addr)->Set(true,T->GetMinute());
    return 0;
}


int SecondFun_DTInt(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T = (DateTime*) args[0].addr;
    ((CcInt*) result.addr)->Set(true,T->GetSecond());
    return 0;
}

int MillisecondFun_DTInt(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T = (DateTime*) args[0].addr;
    ((CcInt*) result.addr)->Set(true,T->GetMillisecond());
    return 0;
}


int AddFun_datetimedatetime(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T1 = (DateTime*) args[0].addr;
    DateTime* T2 = (DateTime*) args[1].addr;
    DateTime* TRes = T1->Clone();
    TRes->Add(T2);
    ((DateTime*) result.addr)->Equalize(TRes);
    delete TRes;
    return 0;
}

int MinusFun_datetimedatetime(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T1 = (DateTime*) args[0].addr;
    DateTime* T2 = (DateTime*) args[1].addr;
    DateTime* TRes = T1->Clone();
    TRes->Minus(T2);
    ((DateTime*) result.addr)->Equalize(TRes);
    delete TRes;
    return 0;
}

int EqualsFun_datetimedatetime(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T1 = (DateTime*) args[0].addr;
    DateTime* T2 = (DateTime*) args[1].addr;
    bool res = T1->CompareTo(T2)==0;
    ((CcBool*) result.addr)->Set(true,res);
    return 0;
}

int BeforeFun_datetimedatetime(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T1 = (DateTime*) args[0].addr;
    DateTime* T2 = (DateTime*) args[1].addr;
    bool res = T1->CompareTo(T2)<0;
    ((CcBool*) result.addr)->Set(true,res);
    return 0;
}

int AfterFun_datetimedatetime(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T1 = (DateTime*) args[0].addr;
    DateTime* T2 = (DateTime*) args[1].addr;
    bool res = T1->CompareTo(T2)>0;
    ((CcBool*) result.addr)->Set(true,res);
    return 0;
}

int MulFun_datetimeint(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T1 = (DateTime*) args[0].addr;
    CcInt* Fact = (CcInt*) args[1].addr;
    DateTime* TRes = T1->Clone();
    TRes->Mul(Fact->GetIntval());
    ((DateTime*) result.addr)->Equalize(TRes);
    delete TRes;
    return 0;
}

int WeekdayFun(Word* args, Word& result, int message, Word& local, Supplier s){
    result = qp->ResultStorage(s);
    DateTime* T = (DateTime*) args[0].addr;
    int day = (T->GetDay() )%7;
    STRING* WD;
    switch(day){
       case 0 :  WD = (STRING*) "Monday";
                 break;
       case 1 : WD =  (STRING*) "Tuesday";
                 break;
       case 2 : WD =  (STRING*) "Wednesday";
                break;
       case 3 : WD = (STRING*) "Thursday";
                break;
       case 4 : WD = (STRING*) "Friday";
                break;
       case 5 : WD = (STRING*) "Saturday";
                break;
       case 6 :  WD = (STRING*) "Sunday";
                 break;

       default : WD = (STRING*)"Errorsday";
                 break;
    }
    ((CcString*)result.addr)->Set(true,WD);
    return 0;
}

int DateTimeToStringFun(Word* args, Word& result, int message, Word& local, Supplier s){
   result = qp->ResultStorage(s);
   DateTime* T = (DateTime*) args[0].addr;
   string TStr = T->ToString(true);
   if(TStr.length()>47){
      string TStr2 = TStr.substr(0,47);
      TStr = TStr2;
   }
   ((CcString*)result.addr)->Set(true,(STRING*)TStr.c_str());
   return 0;
}

/*
4.3 Specifications

*/
const string LeapYearSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"int -> bool\""
   " \" _ leapyear \" "
   "   \"checks whether the given int is a leap year\" "
   "   \" query 2000 leapyear\" ))";

const string NowSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \" -> datetime\""
   " \" now \" "
   "   \"creates a datetime from the current systemtime\" "
   "   \" query now()\" ))";


const string DaySpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"daytime -> int\""
   " \" _ day \" "
   "   \"return the day of this datetime\" "
   "   \" query T1 day\" ))";

const string MonthSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"daytime -> int\""
   " \" _ day \" "
   "   \"return the month of this datetime\" "
   "   \" query T1 month\" ))";

const string YearSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"daytime -> int\""
   " \" _ day \" "
   "   \"return the year of this datetime\" "
   "   \" query T1 year\" ))";


const string HourSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"daytime -> int\""
   " \" _  hour\" "
   "   \"return the hour of this datetime\" "
   "   \" query T1 hour\" ))";

const string MinuteSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"daytime -> int\""
   " \" _ minute \" "
   "   \"return the minute of this datetime\" "
   "   \" query T1 minute\" ))";

const string SecondSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"daytime -> int\""
   " \" _ second \" "
   "   \"return the second of this datetime\" "
   "   \" query T1 second\" ))";

const string MillisecondSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"daytime -> int\""
   " \" _ millisecond \" "
   "   \"return the millisecond of this datetime\" "
   "   \" query T1 millisecond\" ))";

const string AddSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"datetime x datetime -> datetime\""
   " \" _ + _ \" "
   "   \"adds the time values\" "
   "   \" query T1 + T2\" ))";

const string MinusSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"datetime x datetime -> datetime\""
   " \" _ - _ \" "
   "   \"Computes the difference\" "
   "   \" query T1 - T2\" ))";


const string EqualsSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"datetime x datetime -> bool\""
   "\" _ = _ \" "
   "   \"checks for equality\" "
   "   \" query T1 = T2\" ))";

const string LessSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"datetime x datetime -> bool\""
   " \" _ < _ \" "
   "   \"returns true if T1 is before t2\" "
   "   \" query T1 < T2\" ))";

const string GreaterSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"datetime x datetime -> bool\""
   " \" _ > _ \" "
   "   \"returns true if T1 is after T2\" "
   "   \" query T1 > T2\" ))";

const string MulSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"datetime x int -> datetime\""
   " \" _ * _ \" "
   "   \"computes the product of the parameters\" "
   "   \" query T1 * 7\" ))";

const string WeekdaySpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"datetime -> string\""
   " \" _ weekday \" "
   "   \"returns the weekday in human readable format\" "
   "   \" query T weekday\" ))";

const string toStringSpec =
   "((\"Signature\" \"Syntax\" \"Meaning\" \"Example\" )"
   " ( \"datetime -> string\""
   " \" _ toString \" "
   "   \"returns the time in format year-month-day:hour:minute:sec.millisecond\" "
   "   \" query T toString\" ))";


/*
4.4 Definition of Operators

*/
Operator dt_leapyear(
       "leapyear", // name
       LeapYearSpec, // specification
       LeapYearFun_intbool,
       Operator::DummyModel,
       Operator::SimpleSelect,
       IntBool);

Operator dt_now(
       "now", // name
       NowSpec, // specification
       NowFun_VoidDateTime,
       Operator::DummyModel,
       Operator::SimpleSelect,
       VoidDateTime);


Operator dt_day(
       "day", // name
       DaySpec, // specification
       DayFun_DTInt,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeInt);

Operator dt_month(
       "month", // name
       MonthSpec, // specification
       MonthFun_DTInt,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeInt);

Operator dt_year(
       "year", // name
       YearSpec, // specification
       YearFun_DTInt,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeInt);

Operator dt_hour(
       "hour", // name
       HourSpec, // specification
       HourFun_DTInt,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeInt);

Operator dt_minute(
       "minute", // name
       MinuteSpec, // specification
       MinuteFun_DTInt,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeInt);

Operator dt_second(
       "second", // name
       SecondSpec, // specification
       SecondFun_DTInt,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeInt);

Operator dt_millisecond(
       "millisecond", // name
       MillisecondSpec, // specification
       MillisecondFun_DTInt,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeInt);

Operator dt_add(
       "+", // name
       AddSpec, // specification
       AddFun_datetimedatetime,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeDateTimeDateTime);

Operator dt_minus(
       "-", // name
       MinusSpec, // specification
       MinusFun_datetimedatetime,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeDateTimeDateTime);


Operator dt_less(
       "<", // name
       LessSpec, // specification
       BeforeFun_datetimedatetime,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeDateTimeBool);

Operator dt_greater(
       ">", // name
       GreaterSpec, // specification
       AfterFun_datetimedatetime,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeDateTimeBool);

Operator dt_equals(
       "=", // name
       EqualsSpec, // specification
       EqualsFun_datetimedatetime,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeDateTimeBool);


Operator dt_mul(
       "*", // name
       MulSpec, // specification
       MulFun_datetimeint,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeIntDateTime);

Operator dt_weekday(
       "weekday", // name
       WeekdaySpec, // specification
       WeekdayFun,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeString);

Operator dt_toString(
       "toString", // name
       toStringSpec, // specification
       DateTimeToStringFun,
       Operator::DummyModel,
       Operator::SimpleSelect,
       DateTimeString);


/*
5 Creating the Algebra

*/
class DateTimeAlgebra : public Algebra
{
 public:
  DateTimeAlgebra() : Algebra()
  {
    // type constructors
    AddTypeConstructor( &datetime );
    datetime.AssociateKind("DATA");
    // operators
    AddOperator(&dt_add);
    AddOperator(&dt_minus);
    AddOperator(&dt_equals);
    AddOperator(&dt_less);
    AddOperator(&dt_greater);
    AddOperator(&dt_mul);
    AddOperator(&dt_weekday);
    AddOperator(&dt_toString);
    AddOperator(&dt_leapyear);
    AddOperator(&dt_year);
    AddOperator(&dt_month);
    AddOperator(&dt_day);
    AddOperator(&dt_hour);
    AddOperator(&dt_minute);
    AddOperator(&dt_second);
    AddOperator(&dt_millisecond);
    AddOperator(&dt_now);


  }
  ~DateTimeAlgebra() {};
};

DateTimeAlgebra dateTimeAlgebra;

/*
6 Initialization

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
InitializeDateTimeAlgebra( NestedList* nlRef, QueryProcessor* qpRef )
{
  nl = nlRef;
  qp = qpRef;
  return (&dateTimeAlgebra);
}

} // end of namespace

