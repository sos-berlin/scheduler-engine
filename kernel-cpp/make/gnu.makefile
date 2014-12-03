# gnu.mak $Id: gnu.makefile 14247 2011-05-05 04:42:13Z jz $
# Einstellungen nur für Gnu

include $(PROD_DIR)/make/unix.makefile

CCPP       = g++
CC         = gcc

GCC_VERSION_MAJOR := $(shell gcc -dumpversion | cut -f1 -d.)
GCC_VERSION_MINOR := $(shell gcc -dumpversion | cut -f2 -d.)
GCC_VERSION_PATCH := $(shell gcc -dumpversion | cut -f3 -d.)
GCC_HAS_PRECOMPILED_HEADERS := $(shell expr $(GCC_VERSION_MAJOR) '>=' 4 '&' $(GCC_VERSION_MINOR) '>=' 4)

HIDDEN_CWARNINGS += -Wall
CWARNINGS += -Wno-sign-compare
CCPPWARNINGS += -Wno-deprecated
HIDDEN_CCPPWARNINGS += -Wno-reorder
HIDDEN_CCPPWARNINGS += -Wno-parentheses

ifeq ($(O_DIR),Debug)
CDEBUG = -D_DEBUG
else
HIDDEN_CWARNINGS += -Wno-long-long
HIDDEN_CWARNINGS += -Wuninitialized
endif

HIDDEN_CFLAGS += -g
HIDDEN_CFLAGS += -MD
HIDDEN_CFLAGS += -fPIC
CFLAGS    += $(CWARNINGS)

ifeq ($(shell expr $(GCC_VERSION_MAJOR) '>=' 4 '&' $(GCC_VERSION_MINOR) '>=' 6),1)
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
HIDDEN_CFLAGS += -DNDEBUG
CFLAGS += $(COPTIMIZE)
endif
