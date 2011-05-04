# gnu.mak $Id$
# Einstellungen nur für Gnu

include $(PROD_DIR)/make/unix.makefile

CCPP       = g++
CC         = gcc


CWARNINGS += -Wall
CWARNINGS += -Wno-sign-compare
#CWARNINGS += -fmessage-length=0

CCPPWARNINGS += -Wno-deprecated
CCPPWARNINGS += -Wno-reorder
CCPPWARNINGS += -Wno-non-virtual-dtor

ifeq ($(O_DIR),Debug)
CDEBUG     = -g -D_DEBUG
else
CWARNINGS += -Wno-long-long
CWARNINGS += -Wuninitialized
endif

CFLAGS    += -g
CFLAGS    += -MD
#CFLAGS    += -fpermissive
CFLAGS    += $(CWARNINGS)

CCPPFLAGS += $(CCPPWARNINGS)

# Gleiche Template-Generationen zusammenfassen:
#CFLAGS    += -frepo

#Bis gcc 3.1: LINK_STATIC  = -static -Xlinker -Bstatic -nodefaultlibs `$(CCPP) -print-file-name=libstdc++.a` `$(CCPP) -print-file-name=libgcc_eh.a`
#LINK_STATIC  = -Xlinker -Bstatic -static-libgcc

ifeq ($(OS),HP-UX)
else
LINK_STATIC  = -static -static-libgcc
endif

#LINK_STATIC  = -static-libgcc
LINK_DYNAMIC = -Xlinker -Bdynamic

LINK_MAP = -Xlinker -Map -Xlinker $(basename $@).map
LINK_MAP_CREF = $(LINK_MAP) -Xlinker -cref  

#23.7.01 LINK_FLAGS = $(LINK_STATIC)
#23.7.01 Mit -Bstatic läuft Dateityp odbc nicht: libodbc.a kann den Treiber nicht laden, weil dlopen.c nicht initialisiert ist (_pagesize=0)

# Nur bestimmte Module dynamisch einbinden, den Rest statisch:
#LINK_FLAGS = $(LINK_DYNAMIC) -ldl -lm -lc $(LINK_STATIC)
#LIBS += -nodefaultlibs `g++ -print-file-name=libstdc++.a` -lm -lgcc -lc -ldl
#LIBS += -nodefaultlibs `$(CCPP) -print-file-name=libstdc++.a` `$(CCPP) -print-file-name=libgcc_eh.a` -lm -lgcc -lc -ldl -lcrypt
LIBS += -lgcc -lz -lm -lc
#LIBS += -lgcc -lcrypt -lz -lm -lc

#LINK_FLAGS += $(LINK_STATIC)

INCLUDES += -I$(PROD_DIR)/LINKS
INCLUDES += -I$(PROD_DIR)/LINKS/java/include 


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
#LINK_FLAGS += -s
#LINK_FLAGS += $(LINK_STATIC)
#LINK_FLAGS += -static-libgcc
endif
