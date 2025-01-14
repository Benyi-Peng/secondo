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

########################################################################
#
# generic makefile for an algebra module 
#
########################################################################

# inlcude global definitions
include ../../makefile.env
CURRENT_ALGEBRA := PregelAlgebra
ALGEBRA_DEPENDENCIES += Distributed2Algebra

# expand compile flags if necessary
#CCFLAGS += $(ALG_INC_DIRS) 
CCFLAGS += -D BOOST_LOG_DYN_LINK
CCFLAGS += -D GATHER_PREGEL_METRICS 

# create objects and algebra library. The values for the variables
# OBJECTS and LIBOBJ are computed automatically by makefile.env 
.PHONY: all
all: check_dependencies $(OBJECTS) $(LIBOBJ)
	$(MAKE) -C Operators
	$(MAKE) -C Operators/Messaging
	$(MAKE) -C Operators/OnMaster
	$(MAKE) -C Operators/OnWorker
	$(MAKE) -C MessageBroker
	$(MAKE) -C Helpers

check_dependencies:
	$(check-algebra-dependencies)

# include pre-computed dependencies
ifneq ($(MAKECMDGOALS),clean)
-include $(DEP_FILES)
endif

.PHONY: deps
deps: $(DEP_FILES)

# generic rule for creating a library
$(LIBOBJ): $(OBJECTS)
	$(create-lib)

# generic rule for cleaning all generated files
.PHONY: clean
clean:
	$(clean-algebra)
	$(MAKE) -C Operators clean
	$(MAKE) -C Operators/Messaging clean
	$(MAKE) -C MessageBroker clean
	$(MAKE) -C Operators/OnMaster clean
	$(MAKE) -C Operators/OnWorker clean
	$(MAKE) -C Helpers clean
