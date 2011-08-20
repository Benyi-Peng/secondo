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

[1] Header File of the RestrictionData

June-November, 2011. Thomas Uchdorf

[TOC]

1 Overview

This header file essentially contains the definition of the class
~RestrictionData~.

2 Defines and includes

*/
#ifndef __RESTRICTION_DATA_H__
#define __RESTRICTION_DATA_H__

// --- Including header-files
//...

class RestrictionData {

public:

   // --- Constructors
   // Constructor
   RestrictionData ();
   // Destructor
   ~RestrictionData ();

   // --- Methods
   void setFrom (const int & from);

   void setTo (const int & to);

   void setType (const int & type);

   const int & getFrom () const;

   const int & getTo () const;

   const int & getType () const;

protected:

   // --- Members
   int m_from;

   int m_to;

   int m_type;

};

#endif /*__RESTRICTION_DATA_H__ */
