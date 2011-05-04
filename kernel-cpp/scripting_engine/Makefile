# $Id$


DEPTH=../../js/src
BUILD_OPT=1
include $(DEPTH)/config.mk
PROD_DIR = $(shell cd ../.. && pwd)


objects=\
 spidermonkey.o\
 spidermonkey_module.o


include $(PROD_DIR)/make/standard.makefile


all:: $(BIN_DIR)/libspidermonkey.so
all:: $(BIN_DIR)/javascript


INCLUDES += -I$(PROD_DIR)/../prod
INCLUDES += -I$(PROD_DIR)/js/src/$(JS_SUBDIR)
INCLUDES += -I$(PROD_DIR)/js/src/$(OBJDIR)

MY_LIBS += $(PROD_DIR)/../prod/zschimmer/Release/libzschimmer.a
MY_LIBS += $(PROD_DIR)/js/src/$(OBJDIR)/libjs.a
MY_LIBS += $(PROD_DIR)/js/src/liveconnect/$(OBJDIR)/libjsj.a
MY_LIBS += $(PROD_DIR)/../prod/3rd_party/libxslt/libxslt/$(O_DIR)/libxslt.a
MY_LIBS += $(PROD_DIR)/../prod/3rd_party/libxml2/$(O_DIR)/libxml2.a


libspidermonkey.a: $(objects)
	$(AR) $(ARFLAGS) $@ $^


$(BIN_DIR)/libspidermonkey.so: spidermonkey_module.o libspidermonkey.a $(MY_LIBS)
	$(CCPP) -shared $(LINK_FLAGS) $^ $(LIBS) -o $@  $(shell $(PROD_DIR)/make/linker-export-symbols DllMain DllGetClassObject)
	@#$(CCPP) $^ -shared $(LINK_FLAGS) $(LINKFLAGS) $(LIBS) -o $@  $(shell $(PROD_DIR)/make/linker-export-symbols DllMain DllGetClassObject)
	@#$(CCPP) $^ -shared $(LINK_FLAGS) -lcrypt $(LINKFLAGS) $(LIBS) -o $@  $(shell $(PROD_DIR)/make/linker-export-symbols DllMain DllGetClassObject)
	@( ldd -r $@ >/dev/null ) 2>&1 | c++filt
	@echo
	chmod a+rx $@
	$(PROD_DIR)/make/separate-debug-info "$@"


$(BIN_DIR)/javascript: main.o $(MY_LIBS)
	$(CCPP) $(DEBUG) $(LINK_FLAGS) $(LIBPATH) $^ $(SOS_LIBS) $(SOS_LIBS) $(LIBS) $(MY_LIBS) -o $@  $(shell $(PROD_DIR)/make/linker-export-symbols main)
	@#$(CCPP) $^ $(LINK_FLAGS) $(LINKFLAGS) $(LIBS) -o $@  $(shell $(PROD_DIR)/make/linker-export-symbols main)
	$(PROD_DIR)/make/separate-debug-info "$@"
