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

operator be_init alias BE_INIT pattern op (_,_,_,_)
operator be_init_worker alias BE_INIT_WORKER pattern op (_,_,_,_)

operator be_shutdown alias BE_SHUTDOWN pattern op ()
operator be_shutdown_worker alias BE_SHUTDOWN_WORKER pattern op ()

operator be_partRR alias BE_PARTRR pattern op (_,_,_)
operator be_partHash alias BE_PARTHASH pattern op (_,_,_)
operator be_partFun alias BE_PARTFUN pattern op (_,_,_)
operator be_query alias BE_QUERY pattern op (_,_)
operator be_command alias BE_COMMAND pattern op (_)
operator be_collect alias BE_COLLECT pattern op (_)
operator be_copy alias BE_COPY pattern op (_,_)
operator be_mquery alias BE_MQUERY pattern op (_,_)
operator be_mcommand alias BE_MCOMMAND pattern op (_)
operator be_union alias BE_UNION pattern op (_)
operator be_struct alias BE_STRUCT pattern op (_)
operator be_runsql alias BE_RUNSQL pattern op (_)
operator be_partGrid alias BE_PARTGRID pattern op (_,_,_,_,_,_,_)
