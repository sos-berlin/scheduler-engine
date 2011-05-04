# $Id$
# Einstellungen nur fuer HP-UX


include $(PROD_DIR)/make/gnu.makefile
#CFLAGS += -DSYSTEM_HPUX

#LIBS += -lsocket
#INCLUDES += -I$(PROD_DIR)/../unixodbc/include
#INCLUDES += -I/export/home/sos/jz/j2sdk/include
#INCLUDES += -I/export/home/sos/jz/j2sdk/include/solaris

INCLUDES += -I$(PROD_DIR)/LINKS/java/include -I$(PROD_DIR)/LINKS/java/include/hp-ux

#CFLAGS += -Wuninitialized
CFLAGS += -fno-strict-aliasing
CFLAGS += -D_REENTRANT
CFLAGS += -fPIC

CFLAGS += -O2
# Das Kommando chatr bin/scheduler gibt Informationen zum des gebundenen Programms aus

ifndef OBJDIR
OBJDIR = $(shell uname -s)$(shell uname -r)_OPT.OBJ
endif

# NO_PERL: Ohne Perl uebersetzen und binden
NO_PERL=1

#ifeq "$(NO_PERL)" ""
ifeq ($(shell uname -m),ia64)
else
# HP-UX B11.11 (PA-RISC) In .so faengt catch kein throw. Deshalb binden wir alles ein. Joacim Zschimmer 2008-04-15
LIBS += -L`perl -e 'print readlink("../../LINKS/perl");'`/CORE -lperl  $(wildcard $(PROD_DIR)/LINKS/perl/auto/DynaLoader/DynaLoader.a)
CFLAGS += -DZ_LINK_STATIC
DEP_PRODUCTS += hostjava hostole fs file kram
SOS_LIBS += $(PROD_DIR)/../prod/zschimmer/$(O_DIR)/libsosperlscript.a
SOS_LIBS += $(PROD_DIR)/../spidermonkey/scripting_engine/$(O_DIR)/libspidermonkey.a
SOS_LIBS += $(PROD_DIR)/../spidermonkey/js/src/$(OBJDIR)/libjs.a
SOS_LIBS += $(PROD_DIR)/../spidermonkey/js/src/liveconnect/$(OBJDIR)/libjsj.a
endif
#endif


# Fuer pthread.h:
CFLAGS += -D_POSIX_C_SOURCE=199506L 

# Fuer gcc 3.1 STL (Standard Template Library), gcc 3.1 scheint nicht richtig an HP-UX angepasst zu sein.
#CFLAGS += -D_GLIBCPP__PTHREADS=1
#CFLAGS += -D_PTHREADS

LIBS += -lpthread
LIBS += -liconv
LIBS += -lsec

LINK_FLAGS += -Wl,-z

# Damit die Pfade von libstdc++.sl und libgcc_s.sl nicht absolut sind und mit der Umgebungsvariablen SHLIB_PATH angegeben werden können
# (Herausgefunden von Ghassan Beydoun, 2007-01-19)
LINK_FLAGS += -Wl,+s

LINK_FLAGS += -fPIC
LINK_FLAGS += -L/usr/local/lib/hpux32

