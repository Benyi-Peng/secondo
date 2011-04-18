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

# Begin DestributedAlgebra.spec


operator test alias TEST pattern _ op

operator makeDarray alias MAKEDARRAY pattern op (_, _ )

operator get alias GET pattern op ( _, _ )

operator put alias PUT pattern op ( _, _, _ )

operator sendD alias SENDD pattern op ( _, _ ,_)

operator receiveD alias RECEIVED pattern op ( _ , _ )

operator d_receive_rel alias D_RECEIVE_REL pattern op (_, _)

operator d_send_rel alias D_SEND_REL pattern op (_, _, _)

operator ddistribute alias DDISTRIBUTE pattern _ op [_, _]

operator dloop alias DLOOP pattern _ op [ fun ]
implicit parameter element type DELEMENT

operator dtie alias DTIE pattern _ op [ fun ] 
         implicit parameters first, second types DELEMENT, DELEMENT 

operator dsummarize alias DSUMMARIZE pattern _ op

# End DistributedAlgebra.spec
