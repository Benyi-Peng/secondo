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

operator createugrid alias CREATEUGRID pattern op ( _ )
operator insertunit alias INSERTUNIT pattern _ infixop  _ 
operator insertsingle alias INSERTSINGLE pattern op (_, _ , _, _ )
operator insertstream alias INSERTSTREAM pattern op ( _ ) 
operator windowintersectug alias WINDOWINTERSECTUG pattern op (_ , _ , _)
operator windowintersectsug alias WINDOWINTERSECTSUG pattern op ( _ , _ )
 
operator intersects alias INTERSECTS pattern _ infixop _
operator inside alias INSIDE pattern _ infixop _
operator verint alias VERINT pattern _ infixop _
#operator inserthu alias INSERTHU pattern op ( _ )

