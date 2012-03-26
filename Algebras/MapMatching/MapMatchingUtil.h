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
//[TOC] [\tableofcontents]
//[_] [\_]

[1] Header File of the MapMatching Algebra

January-April, 2012. Matthias Roth

[TOC]

1 Overview

This header file contains utilities for map matching

2 Defines and includes

*/
#ifndef __MAP_MATCHING_UTILITY_H__
#define __MAP_MATCHING_UTILITY_H__

#include <stdio.h>
#include <vector>

class Region;
class SimpleLine;
class GLine;
class HalfSegment;
class Point;
class Geoid;
class Network;

namespace mapmatch {

class DirectedNetworkSection;

/*
3 ~MMUtil~

*/
class MMUtil
{
public:
    static bool Intersects(const Region& rRegion, const SimpleLine& rSLine);

    static double CalcOrthogonalProjection(const HalfSegment& rHalfSegment,
                                           const Point& rPt,
                                           /*OUT*/ Point& rPtRes,
                                           const double dScale);

    static Point CalcOrthogonalProjection(const SimpleLine& rLine,
                                          const Point& rPt,
                                          /*OUT*/ double& rdDistanceRes,
                                          /*IN*/  const double dScale);

    static double CalcProjection(const HalfSegment& rHalfSegment,
                                 const Point& rPt,
                                 /*OUT*/ Point& rPtRes,
                                 /*OUT*/ bool& bIsOrthogonal,
                                 /*IN*/  const double dScale);

    static Point CalcProjection(const SimpleLine& rLine,
                                const Point& rPt,
                                /*OUT*/ double& rdDistanceRes,
                                /*OUT*/ bool& bIsOrthogonal,
                                /*IN*/  const double dScale,
                                /*OUT*/ HalfSegment* pResHS = NULL);

    static double CalcDistance(const Point& rPt1,
                               const Point& rPt2,
                               const double dScale);

    static double CalcDistance(const std::vector<const Point*>& rvecPoints,
                               const double dScale);

    static double CalcLengthCurve(const GLine* pCurve,
                                  const Network* pNetwork,
                                  const double dScale);

    static double CalcLengthCurve(const SimpleLine* pCurve,
                                  const double dScale);

    static double CalcHeading(const Point& rPt1,
                              const Point& rPt2,
                              bool bAtPt2 = false,
                              double dScale = 1.0);

    static double CalcHeading(const DirectedNetworkSection& rSection,
                              const HalfSegment& rHS,
                              double dScale = 1.0);

    static bool GetPosOnSimpleLine( const SimpleLine& rLine,
                                    const Point& p,
                                    bool startsSmaller,
                                    double tolerance,
                                    double& result);

    static Point CalcDestinationPoint(const Point& rPoint,
                                      double dBearing,
                                      double dDistanceKM);
};


/*
4 ~AttributePtr~
  Helper class for managing Attribute pointers

*/
template<class Type>
class AttributePtr
{
public:
    typedef AttributePtr<Type> _Myt;

    AttributePtr(Type* pA, bool bIncReference = false)
    :m_pA(pA)
    {
        if (bIncReference && pA != NULL)
            m_pA = dynamic_cast<Type*>(pA->Copy());
    }

    AttributePtr(const _Myt& rAttributePtr)
    :m_pA(NULL)
    {
        if (rAttributePtr.m_pA != NULL)
            m_pA = dynamic_cast<Type*>(rAttributePtr.m_pA->Copy());
    }

    ~AttributePtr()
    {
        if (m_pA != NULL)
        {
            m_pA->DeleteIfAllowed();
            m_pA = NULL;
        }
    }

    const AttributePtr& operator=(const _Myt& rAttributePtr)
    {
        if (&rAttributePtr != this)
        {
            if (m_pA != NULL)
            {
                m_pA->DeleteIfAllowed();
                m_pA = NULL;
            }

            m_pA = rAttributePtr.m_pA;
            if (m_pA != NULL)
                m_pA = dynamic_cast<Type*>(m_pA->Copy());
        }
        return *this;
    }

    void reset(Type* pA)
    {
        if (pA != m_pA)
        {
            if (m_pA != NULL)
            {
                m_pA->DeleteIfAllowed();
                m_pA = NULL;
            }

            if (pA != NULL)
                m_pA = dynamic_cast<Type*>(pA->Copy());
        }
    }

    Type& operator*() const
    {
        return *m_pA;
    }

    Type* operator->() const
    {
        return m_pA;
    }

    operator bool() const
    {
        return (m_pA != NULL);
    }

    Type* get(void) const
    {
        return m_pA;
    }

private:
    Type* m_pA;
};


/*
4 ~DbArrayPtr~
  Helper class for managing DbArrayPtr pointers (ref count)
  calls DbArray::Destroy() when ref count == 0

*/

template <typename DbaType>
class DbArrayPtr
{
public:
    typedef DbArrayPtr<DbaType> _Myt;

    DbArrayPtr()
    :ptr_(NULL), ref_count_(NULL)
    {
    }

    DbArrayPtr(DbaType* p)
    : ptr_(p), ref_count_(p ? new int(0) : NULL)
    {
        inc_ref();
    }

    DbArrayPtr(const _Myt& rhs)
    :ptr_(rhs.ptr_), ref_count_(rhs.ref_count_)
    {
        inc_ref();
    }

    ~DbArrayPtr()
    {
        if(ref_count_ && 0 == dec_ref())
        {
            ptr_->Destroy();
            delete ptr_; ptr_ = NULL;
            delete ref_count_; ref_count_ = NULL;
        }
    }

    DbaType* get() { return ptr_; }
    const DbaType* get() const { return ptr_; }

    void swap(_Myt& rhs) // throw()
    {
      std::swap(ptr_, rhs.ptr_);
      std::swap(ref_count_, rhs.ref_count_);
    }

    DbArrayPtr& operator=(const _Myt& rhs)
    {
        _Myt tmp(rhs);
        this->swap(tmp);
        return *this;
    }

    DbaType& operator*() const
    {
        return *ptr_;
    }

    DbaType* operator->() const
    {
        return ptr_;
    }

    operator bool() const
    {
        return (ptr_ != NULL);
    }

private:
    void inc_ref()
    {
       if(ref_count_)
       {
           ++(*ref_count_);
       }
    }

    int  dec_ref()
    {
       return --(*ref_count_);
    }

    DbaType* ptr_;
    int * ref_count_;
};



} // end of namespace mapmatch

#endif /* __MAP_MATCHING_UTILITY_H__ */


