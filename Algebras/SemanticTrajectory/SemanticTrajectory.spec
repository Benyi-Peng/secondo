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

operator similarity alias SIMILARITY pattern op (_,_,_,_)
operator makesemtraj alias MAKESEMTRAJ pattern _ op [_, _, _]
operator makeuniquelistwords alias MAKEUNIQUELISTWORDS pattern _ op [_]
operator stbox alias STBOX pattern op (_)
operator makesummaries alias MAKESUMMARIES pattern _ op [_,_,_,_,_]
operator extractkeywords alias extractkeywords pattern _ op
operator batches alias BATCHES pattern _ op[_,_,_]
