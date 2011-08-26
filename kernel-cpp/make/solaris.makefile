# $Id: solaris.makefile 13795 2009-06-23 11:13:12Z sos $
# Einstellungen nur f\374r Linux


include $(PROD_DIR)/make/gnu.makefile

CFLAGS += -O2
#CFLAGS += -O0
CFLAGS += -fPIC

LIBS += -lsocket
LIBS += -lthread
LIBS += -liconv

# Damit offene Referenzen nicht bemängelt werden:
#LINK_DYNAMIC = -Xlinker -b


#CCPP_LIB_DIR = $(dir $(shell $(CCPP) -print-file-name=libgcc.a))
#LINKER = ld -L$(CCPP_LIB_DIR) -L/usr/local/lib -lstdc++ -lgcc_s -ldl -lnsl -lrt $(CCPP_LIB_DIR)/crt1.o $(CCPP_LIB_DIR)/crti.o $(CCPP_LIB_DIR)/crtbegin.o
#LINKER = ld -L$(dir $(shell $(CCPP) -print-file-name=libgcc.a)) -L/usr/local/lib -lstdc++ -lgcc_s -ldl -lnsl -lrt
INCLUDES += -I$(PROD_DIR)/LINKS/java/include/solaris
