/*
This file is part of SECONDO.

Copyright (C) 2011, University in Hagen, Department of Computer Science,
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

*/

#ifndef RASTER2_UTIL_NONCOPYABLE_H
#define RASTER2_UTIL_NONCOPYABLE_H

namespace raster2
{
    namespace util
    {
/*
1 Class ~noncopyable~

The class ~noncopyable~ is a base class for classes whose objects should not be
copied.

It declares a copy constructor and a copy assignment operator. This prevents
the C++ compiler from implementing a copy constructor and a copy assignment
operator. The declared copy constructor and a copy assignment operator do not
have an implementation. This leads to the C++ compiler issuing an error when
someone tries to copy an object of a derived class.

*/
        class noncopyable
        {
          protected:
            noncopyable() {}
            ~noncopyable() {}
          private:
            noncopyable( const noncopyable& ); // Don't implement
            const noncopyable& operator=(const noncopyable&); // Don't implement
        };
    }
}

#endif
