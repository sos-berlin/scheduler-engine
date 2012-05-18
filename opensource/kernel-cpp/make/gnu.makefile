# gnu.mak $Id: gnu.makefile 14247 2011-05-05 04:42:13Z jz $
# Einstellungen nur für Gnu

include $(PROD_DIR)/make/unix.makefile

CCPP       = g++
CC         = gcc

CWARNINGS += -Wall
CWARNINGS += -Wno-sign-compare
CCPPWARNINGS += -Wno-deprecated
#CCPPWARNINGS += -Wno-non-virtual-dtor
HIDDEN_CCPPWARNINGS += -Wno-reorder
HIDDEN_CCPPWARNINGS += -Wno-parentheses

ifeq ($(O_DIR),Debug)
CDEBUG     = -g -D_DEBUG
else
CWARNINGS += -Wno-long-long
CWARNINGS += -Wuninitialized
endif

CFLAGS    += -g
CFLAGS    += -MD
CFLAGS    += -fPIC
CFLAGS    += $(CWARNINGS)

ifeq ($(cpuArchitecture),x64)
CCPPFLAGS += -std=c++0x
endif
CCPPFLAGS += $(CCPPWARNINGS)

ifeq ($(OS),HP-UX)
else
LINK_STATIC  = -static -static-libgcc
endif

LINK_DYNAMIC = -Xlinker -Bdynamic

LINK_MAP = -Xlinker -Map -Xlinker $(basename $@).map
LINK_MAP_CREF = $(LINK_MAP) -Xlinker -cref  

LIBS += -lgcc -lz -lm -lc

INCLUDES += -I$(PROD_DIR)/LINKS
INCLUDES += -I$(JAVA_HOME)/include 


ifeq ($(OS),HP-UX)
LINK_FLAGS +=
else
ifeq ($(OS),SunOS)
LINK_FLAGS += -lrt -lnsl -ldl
else
#LINK_FLAGS += -Xlinker -Map -Xlinker $@.map
endif
endif

ifeq ($(O_DIR),Debug)
CFLAGS += $(CDEBUG)
else
CFLAGS += -DNDEBUG $(COPTIMIZE)
endif
