/*
----
This file is part of SECONDO.

Copyright (C) 2004-2009, University in Hagen, Faculty of Mathematics
and Computer Science, Database Systems for New Applications.

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

#pragma once

#include "SimpleAttrArray.h"
#include "TemporalAlgebra.h"

namespace ColumnMovingAlgebra
{
  class IIntEntry
  {
  public:
    typedef temporalalgebra::IInt AttributeType;

    static const bool isPrecise = true;

    IIntEntry() = default;
    IIntEntry(bool defined, int64_t time, int value);
    IIntEntry(const temporalalgebra::IInt &value);

    bool IsDefined() const;
    int64_t GetTime() const;
    int GetValue() const;

    int Compare(const IIntEntry &value) const;
    int Compare(const temporalalgebra::IInt &value) const;
    bool Equals(const IIntEntry &value) const;
    bool Equals(const temporalalgebra::IInt &value) const;

    size_t GetHash() const;

    temporalalgebra::IInt *GetAttribute(bool clone = true) const;

  private:
    
    bool m_Defined, m_ValueDefined;
    int64_t m_Time;
    int m_Value;
  };

  typedef CRelAlgebra::SimpleFSAttrArray<IIntEntry> IInts;
  typedef CRelAlgebra::SimpleFSAttrArrayIterator<IIntEntry> IIntsIterator;


  inline IIntEntry::IIntEntry(bool defined, int64_t time, int value) :
    m_Defined(defined),
    m_ValueDefined(true),
    m_Time(time),
    m_Value(value)
  {
  }

  inline IIntEntry::IIntEntry(const temporalalgebra::IInt &value) :
    m_Defined(value.IsDefined()),
    m_ValueDefined(value.value.IsDefined()),
    m_Time(value.instant.millisecondsToNull()),
    m_Value(value.value.GetIntval())
  {
  }

  inline bool IIntEntry::IsDefined() const
  {
    return m_Defined;
  }

  inline int64_t IIntEntry::GetTime() const {
    return m_Time;
  }

  inline int IIntEntry::GetValue() const {
    return m_Value;
  }

  inline int IIntEntry::Compare(const IIntEntry &value) const
  {
    if (!m_Defined)
      return !value.m_Defined ? 0 : -1;
    else if (!value.m_Defined)
      return 1;

    int64_t tDiff = m_Time - value.m_Time;
    if (tDiff != 0)
      return tDiff < 0 ? -1 : 1;

    if (!m_ValueDefined)
      return !value.m_ValueDefined ? 0 : -1;
    else if (!value.m_ValueDefined)
      return 1;

    int iDiff = m_Value - value.m_Value;
    if (iDiff != 0)
      return iDiff < 0 ? -1 : 1;

    return 0;
  }

  inline int IIntEntry::Compare(const temporalalgebra::IInt &value) const
  {
    IIntEntry b(value);
    return Compare(b);
  }

  inline bool IIntEntry::Equals(const IIntEntry &value) const
  {
    return Compare(value) == 0;
  }

  inline bool IIntEntry::Equals(const temporalalgebra::IInt &value) const
  {
    return Compare(value) == 0;
  }

  inline size_t IIntEntry::GetHash() const
  {
    if (!m_Defined)
      return 0;

    return static_cast<size_t>(m_Value) ^ static_cast<size_t>(m_Time);
  }

  inline temporalalgebra::IInt *IIntEntry::GetAttribute(bool clone) const
  {
    if (!m_Defined)
      return new temporalalgebra::IInt(false);
      
    if (!m_ValueDefined) {
      CcInt r(2);
      r.SetDefined(false);
      return new temporalalgebra::IInt(Instant(m_Time), r);
    }
        
    return new temporalalgebra::IInt(Instant(m_Time), CcInt(m_Value));
  }

}
