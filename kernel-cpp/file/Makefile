# $Id$

PROD_DIR = $(shell cd ../.. && pwd)

DEP_PRODUCTS  = file fs kram zschimmer

objects = \
 alias.o \
 anyfile.o \
 absfile.o \
 ddefile.o \
 deletefl.o \
 dosdir.o \
 cache_file.o \
 cachefl.o \
 cachsqfl.o \
 colsfile.o \
 comfile.o \
 concatfl.o \
 convfile.o \
 create_table_file.o \
 dir.o \
 ebcascfl.o \
 errfile.o \
 filebase.o \
 filekey.o \
 filetab.o \
 filter.o \
 flstream.o \
 frmpage.o \
 framedm.o \
 head_file.o \
 inlinefl.o \
 insertfl.o \
 jdbc.o \
 jmail_file.o \
 keyseqfl.o \
 lockwait.o \
 logfile.o \
 mailfl.o \
 nl.o \
 nlfile.o \
 nullfile.o \
 odbc.o \
 odbctype.o \
 p_file.o \
 progfile.o \
 rectab.o \
 sam3file.o \
 selall.o \
 sosdb.o \
 sqlfile.o \
 stdfile.o \
 storefl.o \
 tabrec.o \
 tail_file.o \
 tb.o \
 teefl.o \
 typefile.o \
 updatefl.o \
 vflfile.o \
 objfile.o \
 protfile.o \
 xml_file.o \
 zlibfl.o \

#ebofile.o \


include $(PROD_DIR)/make/standard.makefile


CFLAGS += -fno-strict-aliasing
CPPFLAGS += -Wno-deprecated -Wno-reorder -Wno-long-long -Wno-sign-compare

all:: libfile.a
all:: $(BIN_DIR)/hostcopy


libfile.a: $(objects)
	$(AR) $(ARFLAGS) $@ $(objects)


ifeq ($(OS),HP-UX)
LIBS += $(PERL_DIR)/libperl.a
endif

#$(BIN_DIR)/hostcopy: soscp.o ../kram/$(O_DIR)/soswnmai.o libfile.a $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(p).a)

hostcopy $(BIN_DIR)/hostcopy: soscp.o ../kram/$(O_DIR)/sosmain0.o ../kram/$(O_DIR)/soswnmai.o libfile.a $(SOS_LIBS) ../3rd_party/libxslt/libxslt/$(O_DIR)/libxslt.a ../3rd_party/libxml2/$(O_DIR)/libxml2.a
	-$(LINKER) $(DEBUG) $(LINK_FLAGS) $(LIBPATH) $^ $(LIBS) -o $(BIN_DIR)/hostcopy
	chmod a+rx $(BIN_DIR)/hostcopy
	$(PROD_DIR)/make/separate-debug-info "$(BIN_DIR)/hostcopy"
