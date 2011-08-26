# $Id: Makefile 13739 2008-11-18 08:59:23Z jz $

PROD_DIR = $(shell cd ../.. && /bin/pwd)

DEP_PRODUCTS  = document_processor  kram file fs zschimmer

objects = hostoled.o hostole.o ghostscript.o rerun_file.o variables.o  hostole_ptr.o

include $(PROD_DIR)/make/standard.makefile

CFLAGS += -fno-strict-aliasing
CPPFLAGS += -Wno-deprecated -Wno-reorder -Wno-long-long -Wno-sign-compare

MY_LIBS  = $(PROD_DIR)/3rd_party/libxml2/$(O_DIR)/libxml2.a
MY_LIBS += $(PROD_DIR)/3rd_party/libxslt/libxslt/$(O_DIR)/libxslt.a
MY_LIBS += $(PROD_DIR)/zschimmer/rtf/$(O_DIR)/librtf.a



all:: $(BIN_DIR)/libhostole.so 
all:: libhostole.a
#all:: $(BIN_DIR)/test_hostole



#libhostole_ptr.a: hostole_ptr.o
#	$(AR) $(ARFLAGS) $@ $^

libhostole.a: $(objects)
	$(AR) $(ARFLAGS) $@ $^

libhostole.so: $(BIN_DIR)/libhostole.so

$(BIN_DIR)/libhostole.so: $(objects) $(PROD_DIR)/document_processor/$(O_DIR)/factory_processor.o $(SOS_LIBS) $(MY_LIBS)
	$(CCPP) -shared $(LINK_FLAGS) $^ $(SOS_LIBS) $(LIBS)  -o $@  $(shell $(PROD_DIR)/make/linker-export-symbols hostole_DllMain hostole_DllGetClassObject)
	@( ldd -r $@ >/dev/null ) 2>&1 | c++filt
	@echo
	chmod a+rx $@
	$(PROD_DIR)/make/separate-debug-info "$@"


$(BIN_DIR)/test_hostole: test_hostole.o $(PROD_DIR)/zschimmer/$(O_DIR)/com.o $(PROD_DIR)/zschimmer/$(O_DIR)/libzschimmer.a
	$(CC) test_hostole.o $(PROD_DIR)/zschimmer/$(O_DIR)/com.o $(PROD_DIR)/zschimmer/$(O_DIR)/libzschimmer.a $(LINK_FLAGS) $(LIBS) -Xlinker -Map -Xlinker $(basename $@).map -Xlinker -cref  -o $@
	$(PROD_DIR)/make/separate-debug-info "$@"
