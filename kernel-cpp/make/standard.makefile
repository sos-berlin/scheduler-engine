# $Id: standard.makefile 13795 2009-06-23 11:13:12Z sos $
 
GNUMAKE=1
#SYSTEM_LINUX=1
 
# makefile.std $Id: standard.makefile 13795 2009-06-23 11:13:12Z sos $


#------------------------------------------------------------SYSTEM feststellen

OS=$(shell uname)
ifeq ($(OS),Linux)
SYSTEM_LINUX := 1
else
ifeq ($(OS),HP-UX)
SYSTEM_HPUX := 1
else
ifeq ($(OS),AIX)
SYSTEM_AIX := 1
else
SYSTEM_SOLARIS := 1
endif
endif
endif

ifdef SYSTEM_LINUX
SYSTEM      := linux
GCC         := 1
SYSTEM_UNIX := 1
SYSTEM_GNU  := 1
endif

ifdef SYSTEM_HPUX
SYSTEM      := hp-ux
GCC         := 1
SYSTEM_UNIX := 1
SYSTEM_GNU  := 1
endif

ifdef SYSTEM_AIX
SYSTEM      := aix
SYSTEM_UNIX := 1
SYSTEM_GNU  := 1
endif

ifdef SYSTEM_SOLARIS
SYSTEM      := solaris
SYSTEM_UNIX := 1
SYSTEM_GNU  := 1
endif

#----------------------------------------------------------Für alle Plattformen

VPATH = ..

#SOS_LIBS     := $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(notdir $p).a)


SHELL := /bin/sh

#-----------------------------------------------------------------------BIN_DIR

ifeq ($(BIN_DIR),)
ifeq ($(notdir $(shell pwd)),Release)
BIN_DIR=$(PROD_DIR)/bin
else
BIN_DIR=$(PROD_DIR)/bind
endif
x:=$(shell test -d $(BIN_DIR) || mkdir $(BIN_DIR))
endif

#------------------------------------------------------------------------------

include $(PROD_DIR)/make/$(SYSTEM).makefile

#ifeq ($(O_DIR),Release)
#CFLAGS+=$(COPTIMIZE)
#endif

# Beide Defines müssen gleich sein, das wird geprüft, damit #undef FD_SETSIZE erkannt wird, z.B. von posix_types.h
#CFLAGS += -DZ_FD_SETSIZE=100
#CFLAGS += -DFD_SETSIZE=100


#------------------------------------------------------------------------------

_all_:	all


clean:
	rm *.o *.d lib*.a lib*.so *.map `find . -name "*.class"`


#include $(objects:.o=.d)
include $(shell ls *.d)
