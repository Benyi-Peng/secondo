#This file is part of SECONDO.

#Copyright (C) 2004, University in Hagen, Department of Computer Science,
#Database Systems for New Applications.

#SECONDO is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#(at your option) any later version.

#SECONDO is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with SECONDO; if not, write to the Free Software
#Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

operator distancescan alias DISTANCESCAN pattern _ _ op [_, _]
operator distancescan2 alias DISTANCESCAN2 pattern _ _ op [_, _]
operator distancescan3 alias DISTANCESCAN3 pattern _ _ op [_, _]
operator distancescan4 alias DISTANCESCAN4 pattern _ _ op [_, _, _]
operator knearest alias KNEAREST pattern _ _ op [_, _]
operator knearestvector alias KNEARESTVECTOR pattern _ _ op [_, _]
operator knearestfilter alias KNEARESTFILTER pattern _ _ op [_, _]
operator bboxes alias bboxes pattern  _ op [_]
operator coverage alias COVERAGE pattern op(_)
operator coverage2 alias COVERAGE2 pattern op(_)
operator newknearestfilter alias NEWKNEARESTFILTER pattern _ _ _ _ op [_,_]
