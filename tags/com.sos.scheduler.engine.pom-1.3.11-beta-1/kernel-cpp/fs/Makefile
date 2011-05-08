# $Id$

PROD_DIR = $(shell cd ../.. && pwd)
DEP_PRODUCTS = kram file rapid fs


objects = fsfile.o rapid.o rapidfil.o


include $(PROD_DIR)/make/standard.makefile

CFLAGS += -fno-strict-aliasing
CPPFLAGS += -Wno-deprecated -Wno-reorder -Wno-long-long -Wno-sign-compare


all:: libfs.a
#all:: $(BIN_DIR)/sosfs

 
libfs.a: $(objects)
	$(AR) $(ARFLAGS) $@ $(objects)

$(BIN_DIR)/sosfs: cfs.o cfs.o $(PROD_DIR)/kram/$(O_DIR)/soswnmai.o fsman.o fs.o fssta.o ../kram/$(O_DIR)/sosprog.o ../kram/$(O_DIR)/soswnmai.o $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(p).a)
	-$(CC) $(DEBUG) $(LINK_FLAGS) -Xlinker -Map -Xlinker $(BIN_DIR)/sosfs.map  -o $@ $^ $(VERBOSE) $(CFLAGS) $(INCLUDES) $(TEMPLATES) $(LIBPATH) $(SOS_LIBS) $(SOS_LIBS) $(ORACLE_LIBS) $(RW_LIBS) $(NET_LIBS) $(C_LIBS) $(LIBS) $(LIBS)
	echo ^G
