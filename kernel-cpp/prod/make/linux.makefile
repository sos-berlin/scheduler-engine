# linux.mak $Id$
# Einstellungen nur für Linux

include $(PROD_DIR)/make/gnu.makefile


#CFLAGS += -march=pentium
#CFLAGS += -mtune=pentium4
#CFLAGS += -mcpu=pentium4
# -mcpu=pentium4 optimiert f|r Pentium 4, Code lLuft aber auch auf anderen Prozessoren (im Gegensatz zu -march=pentium4).


COPTIMIZE += -O2
#COPTIMIZE += -Wuninitialized

CFLAGS += -m32
#CFLAGS += -fPIC

# MAKE_UBUNTU setzen
CFLAGS += $(if $(shell test -e /usr/include/ansidecl.h && echo 1),,-DMAKE_UBUNTU)


LINK_FLAGS += -m32


INCLUDES += -I$(PROD_DIR)/LINKS/java/include/linux

# LINKS/include kann auf Suse 8.0 /usr/include verweisen, damit beim Ablauf nicht die neueste glibc verlangt wird.
INCLUDES += $(if $(shell test -e $(PROD_DIR)/LINKS/include && echo 1),-I$(PROD_DIR)/LINKS/include -I$(PROD_DIR)/LINKS/include/linux,)


# libstdc++ statisch einbinden (geht leider nicht auf Solaris)
#LIBS += -nodefaultlibs `g++ -print-file-name=libstdc++.a` `g++ -print-file-name=libgcc_eh.a` -lm -lgcc -lc -ldl -lcrypt

LIBS += -lpthread
LIBS += -ldl
