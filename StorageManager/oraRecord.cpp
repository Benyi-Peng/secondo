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

1 Implementation of SmiRecord using Oracle

April 2002 Ulrich Telle

November 30, 2002 RHG Added function ~GetKey~.

*/

#include <string>
#include <algorithm>
#include <cctype>
using namespace std;

#include "SecondoSMI.h"
#include "SmiORA.h"
#include "SmiCodes.h"
using namespace OCICPP;

/* --- Implementation of class SmiRecord --- */

SmiRecord::Implementation::Implementation()
  : oraLob(), oraCursor(), closeCursor( false )
{
}

SmiRecord::Implementation::~Implementation()
{
}

SmiRecord::SmiRecord()
  : recordKey(), recordSize( 0 ), fixedSize( false ),
    initialized( false ), writable( false )
{
  impl = new Implementation();
}
  
SmiRecord::~SmiRecord()
{
  if ( initialized )
  {
    Finish();
  }
  delete impl;
}

SmiSize
SmiRecord::Read( void* buffer, 
                 const SmiSize numberOfBytes,
                 const SmiSize offset /* = 0 */ )
{
  SmiSize actRead = 0;

  if ( initialized )
  {
    try
    {
      if ( offset <= impl->oraLob.getLen() )
      {
        impl->oraLob.seek( offset, LOB_SET );
        actRead = impl->oraLob.read( buffer, numberOfBytes );
        SmiEnvironment::SetError( E_SMI_OK );
      }
      else
      {
        SmiEnvironment::SetError( E_SMI_RECORD_READ );
      }
    }
    catch ( OraError &err )
    {
      SmiEnvironment::SetError( E_SMI_RECORD_READ, err.message );
    }
  }
  else
  {
    SmiEnvironment::SetError( E_SMI_RECORD_NOTINIT );
  }

  return (actRead);
}

SmiSize
SmiRecord::Write( const void*   buffer, 
                  const SmiSize numberOfBytes, 
                  const SmiSize offset /* = 0 */ )
{
  SmiSize actWritten = 0;

  if ( initialized && writable )
  {
    try
    {
      if ( offset <= impl->oraLob.getLen() )
      {
        impl->oraLob.seek( offset, LOB_SET );
        actWritten = impl->oraLob.write( buffer, numberOfBytes );
        if ( offset + numberOfBytes > recordSize )
        {
          recordSize = offset + numberOfBytes;
        }
        SmiEnvironment::SetError( E_SMI_OK );
      }
      else
      {
        SmiEnvironment::SetError( E_SMI_RECORD_WRITE );
      }
    }
    catch ( OraError &err )
    {
      SmiEnvironment::SetError( E_SMI_RECORD_WRITE, err.message );
    }
  }
  else
  {
    if ( !initialized )
    {
      SmiEnvironment::SetError( E_SMI_RECORD_NOTINIT );
    }
    else
    {
      SmiEnvironment::SetError( E_SMI_RECORD_READONLY );
    }
  }

  return (actWritten);
}

SmiSize
SmiRecord::Size()
{
  return (recordSize);
}

SmiKey
SmiRecord::GetKey()
{
  return recordKey;
}

bool
SmiRecord::Truncate( const SmiSize newSize )
{
  bool ok = false;

  if ( initialized && !fixedSize )
  {
    if ( newSize < recordSize )
    {
      try
      {
        impl->oraLob.trunc( newSize );
        recordSize = newSize;
        SmiEnvironment::SetError( E_SMI_OK );
        ok = true;
      }
      catch ( OraError &err )
      {
        SmiEnvironment::SetError( E_SMI_RECORD_TRUNCATE, err.message );
      }
    }
    else
    {
      SmiEnvironment::SetError( E_SMI_RECORD_TRUNCATE );
    }
  }
  else
  {
    SmiEnvironment::SetError( E_SMI_RECORD_NOTINIT );
  }

  return (ok);
}

void
SmiRecord::Finish()
{
  if ( initialized )
  {
    try
    {
      impl->oraLob.drop();
      if ( impl->closeCursor )
      {
        impl->oraCursor.drop();
        impl->closeCursor = false;
      }
    }
    catch ( OraError &err )
    {
      SmiEnvironment::SetError( E_SMI_RECORD_FINISH, err.message );
    }
  }
  initialized = false;
  recordSize  = 0;
}

/* --- oraRecord.cpp --- */

