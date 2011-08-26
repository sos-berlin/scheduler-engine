# $Id: Makefile 11347 2005-03-24 10:14:54Z jz $

ifndef PROD_DIR
prod_dir = ..
include $(prod_dir)/make/base.makefile
else

DEP_PRODUCTS  = file kram fs rapid

objects =\
 odbcbase.o\
 odbccat.o\
 odbccols.o\
 odbcconn.o\
 odbcenv.o\
 odbcspec.o\
 odbcstat.o\
 odbcstmt.o


all: $(BIN_DIR)/libhostodbc.so 


clean:
	rm $(BIN_DIR)/libhostodbc.so $(BIN_DIR)/libhostodbc.map $(O_DIR)/*

include $(PROD_DIR)/make/standard.makefile
 
$(BIN_DIR)/libhostodbc.so: $(objects) $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(p).a)
	-$(CC) $(DEBUG) -shared -mno-shared-libs -Xlinker -Map -Xlinker $(BIN_DIR)/libhostodbc.map  -o $@ $^ $(VERBOSE) $(CFLAGS) $(INCLUDES) $(TEMPLATES) $(LIBPATH) $(SOS_LIBS) $(SOS_LIBS) $(ORACLE_LIBS) $(RW_LIBS) $(NET_LIBS) $(C_LIBS) $(LIBS)
	echo 

endif
