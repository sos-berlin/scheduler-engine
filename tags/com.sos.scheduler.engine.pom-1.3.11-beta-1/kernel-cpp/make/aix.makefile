# $Id: solaris.makefile 4788 2007-01-24 10:06:59Z jz $
# Einstellungen nur f\374r Linux


include $(PROD_DIR)/make/gnu.makefile

CFLAGS += -O2
CFLAGS += -fPIC
CFLAGS += -D_THREAD_SAFE


AR        = /usr/bin/ar


#LIBS += -lsocket
LIBS += -lpthread
LIBS += -liconv

# Damit offene Referenzen nicht bemängelt werden:
#LINK_DYNAMIC = -Xlinker -b

LINK_FLAGS += -Wl,-bnoipath


#CCPP_LIB_DIR = $(dir $(shell $(CCPP) -print-file-name=libgcc.a))
#LINKER = ld -L$(CCPP_LIB_DIR) -L/usr/local/lib -lstdc++ -lgcc_s -ldl -lnsl -lrt $(CCPP_LIB_DIR)/crt1.o $(CCPP_LIB_DIR)/crti.o $(CCPP_LIB_DIR)/crtbegin.o
#LINKER = ld -L$(dir $(shell $(CCPP) -print-file-name=libgcc.a)) -L/usr/local/lib -lstdc++ -lgcc_s -ldl -lnsl -lrt
