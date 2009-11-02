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

//paragraph [1] Title: [{\Large \bf \begin {center}] [\end {center}}]

[1] FText Algebra

March - April 2003 Lothar Sowada

The algebra ~FText~ provides the type constructor ~text~ and two operators:

(i) ~contains~, which search text or string in a text.

(ii) ~length~ which give back the length of a text.

*/

#ifndef __F_TEXT_ALGEBRA__
#define __F_TEXT_ALGEBRA__

#include <iostream>

#include "IndexableAttribute.h"
#include "../../Tools/Flob/Flob.h"

class FText: public IndexableAttribute
{
public:

  inline FText();
  inline FText( bool newDefined, const char *newText = NULL) ;
  inline FText( bool newDefined, const string& newText );
  inline FText(const FText&);
  inline ~FText();
  inline void Destroy();

  inline bool  SearchString( const char* subString );
  inline void  Set( const char *newString );
  inline void  Set( bool newDefined, const char *newString );
  inline void  Set( bool newDefined, const string& newString );
  inline int TextLength() const;
  inline const char *Get() const;
  inline const string GetValue() const;

/*************************************************************************

  The following virtual functions:
  IsDefined, SetDefined, HashValue, CopyFrom, Compare, Sizeof, Clone, Print, Adjacent
  need to be defined if we want to use ~text~ as an attribute type in tuple definitions.

*************************************************************************/

  inline size_t Sizeof() const;
  size_t HashValue() const;
  void CopyFrom(const Attribute* right);
  int Compare(const Attribute * arg) const;
  inline FText* Clone() const;
  ostream& Print(ostream &os) const;
  bool Adjacent(const Attribute * arg) const;

  inline int NumOfFLOBs() const;
  inline Flob* GetFLOB( const int );

/*************************************************************************

  For use with Btree-Indices, we need text to be in kind INDEXABLE.
  therefore, we need to inherit from IndexableAttribute

*************************************************************************/

  virtual void WriteTo( char *dest ) const;
  virtual void ReadFrom( const char *src );
  virtual SmiSize SizeOfChars() const;

  virtual bool hasDB3Representation() const {return true;}
  virtual unsigned char getDB3Type() const { return 'M'; }
  virtual unsigned char getDB3Length() const { return 0; }
  virtual unsigned char getDB3DecimalCount(){ return 0; }
  virtual string getDB3String() const { return GetValue(); }

  virtual void ReadFromString(string value){
    Set(true,value);
  }


private:
  Flob theText;
};

/*
2 Inline Functions

*/
const string typeName="text";
const bool traces=false;

inline FText::FText()
{
  LOGMSG( "FText:Trace", cout << '\n' <<"Start FText()"<<'\n'; )
  LOGMSG( "FText:Trace",  cout <<"End FText()"<<'\n'; )
}

inline FText::FText( bool newDefined, const char *newText /* =0*/ ) :
theText( 0 )
{
  LOGMSG( "FText:Trace",
           cout << '\n'
                <<"Start FText(bool newDefined, textType newText)"
                <<'\n'; )
  Set( newDefined, newText );
  LOGMSG( "FText:Trace",
           cout <<"End FText(bool newDefined, textType newText)"
                <<'\n'; )
}

inline FText::FText( bool newDefined, const string& newText ) :
    theText( 0 )
{
  LOGMSG( "FText:Trace",
          cout << '\n'
              <<"Start FText( bool newDefined, const string newText )"
              <<'\n'; )
      Set( newDefined, newText );
  LOGMSG( "FText:Trace",
          cout <<"End FText( bool newDefined, const string newText )"
              <<'\n'; )
}


inline FText::FText( const FText& f ) :
theText( 0 )
{
  LOGMSG( "FText:Trace", cout << '\n' <<"Start FText(FText& f)"<<'\n'; )
  //SPM? Assuming Flob fits into memory  
  //const char* s = new char(f.theText.getSize());
  theText.copyFrom( f.theText );
  SetDefined( f.IsDefined() );
  LOGMSG( "FText:Trace",  cout <<"End FText(FText& f)"<<'\n'; )

}

inline FText::~FText()
{
  LOGMSG( "FText:Trace",  cout << '\n' <<"Start ~FText()"<<'\n'; )
  LOGMSG( "FText:Trace",  cout <<"End ~FText()"<<'\n'; )
}

inline void FText::Destroy()
{
  theText.destroy();
  SetDefined(false);
}

inline bool FText::SearchString( const char* subString )
{
  SmiSize sz = theText.getSize();	
  char* text = new char(sz);
  theText.read(text, sz);
  return strstr( text, subString ) != NULL;
  //SPM: delete added
  delete text;
}

inline void FText::Set( const char *newString )
{
  LOGMSG( "FText:Trace",
           cout << '\n'
                << "Start Set with *newString='"<< newString << endl; )

  Set( true, newString );

  LOGMSG( "FText:Trace", cout <<"End Set"<<'\n'; )
}

inline void FText::Set( bool newDefined, const char *newString )
{
  LOGMSG( "FText:Trace",
          cout << '\n' << "Start Set with *newString='"
               << newString << endl; )

  if( newString != NULL )
  {
    theText.clean();
    //  theText.Resize( strlen( newString ) + 1 );
    theText.write( newString, strlen( newString ) + 1 );
  }
  SetDefined( newDefined );

  LOGMSG( "FText:Trace", cout <<"End Set"<<'\n'; )
}

inline void FText::Set( bool newDefined, const string& newString )
{
  LOGMSG( "FText:Trace",
          cout << '\n' << "Start Set with newString='"
              << newString << endl; )
  theText.clean();
  if(newDefined)
  {
//     theText.Resize( newString.length() + 1 );
    theText.write( newString.c_str(), newString.length() + 1 );
  }
  SetDefined( newDefined );
  LOGMSG( "FText:Trace", cout <<"End Set"<<'\n'; )
}


inline int FText::TextLength() const
{
  return theText.getSize() - 1;
}

// SPM: caller is responsible for delete
inline const char *FText::Get() const
{
  SmiSize sz = theText.getSize();	
  char* s = new char(sz);
  theText.read(s, sz);
  return s;
}

inline const string FText::GetValue() const
{
  SmiSize sz = theText.getSize();	
  char* s = new char(sz);
  theText.read(s, sz);
  string res(s);
  delete s;
  return res;
}

inline size_t FText::Sizeof() const
{
  if(traces)
    cout << '\n' << "Start Sizeof" << '\n';
  return sizeof( *this );
}

inline FText* FText::Clone() const
{
  return new FText( *this );
}

inline int FText::NumOfFLOBs() const
{
  return 1;
}

inline Flob* FText::GetFLOB( const int i )
{
  assert( i == 0 );
  return &theText;
}


namespace ftext{

Word CreateFText( const ListExpr typeInfo );

void DeleteFText( const ListExpr typeInfo, Word& w );

void CloseFText( const ListExpr typeInfo, Word& w );

Word CloneFText( const ListExpr typeInfo, const Word& w );

int SizeOfFText();

void* CastFText( void* addr );

ListExpr OutFText( ListExpr typeInfo, Word value );

Word InFText( const ListExpr typeInfo, const ListExpr instance,
       const int errorPos, ListExpr& errorInfo, bool& correct );

bool OpenFText( SmiRecord& valueRecord, size_t& offset,
                const ListExpr typeInfo, Word& value );

bool SaveFText( SmiRecord& valueRecord, size_t& offset,
                const ListExpr typeInfo, Word& value );

} // end namespace ftext
#endif

