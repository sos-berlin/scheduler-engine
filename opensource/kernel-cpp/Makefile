# $Id: Makefile 14253 2011-05-06 09:14:57Z jz $

ifeq ($(O_DIR),)
#O_DIR=Debug
O_DIR=Release
endif

ifeq ($(O_DIR),Release)
BIN_DIR=$(shell /bin/pwd)/bin
else
BIN_DIR=$(shell /bin/pwd)/bind
endif

CDPATH=

# Fuer Spidermonekey:
JDK=$(JAVA_HOME)
export O_DIR

WITH_JAVA=1
WITH_PERL=1
SCHEDULER_DEPENDENTS:=

ifeq ($(shell uname),HP-UX)
#SCHEDULER_DEPENDENTS:=libhostole.a libhostjava.a $(BIN_DIR)/sos.hostware.jar
endif

ifeq ($(cpuArchitecture),x64)
XML_LIB=
XSLT_LIB=
else
XML_LIB=libxml.a
XSLT_LIB=libxslt.a
endif

.PHONY: fs
.PHONY: kram
.PHONY: libxml2
.PHONY: libxslt
.PHONY: javascript
.PHONY: perlscript
.PHONY: scheduler
.PHONY: setuid
.PHONY: zschimmer

.PHONY: libxml2.a
.PHONY: libxslt.a
.PHONY: libzschimmer.a
.PHONY: libkram.a
.PHONY: libfile.a


all:: scheduler
all:: setuid

ifeq ($(cpuArchitecture),x64)
else
all:: spidermonkey
endif

all:: $(BIN_DIR)/libsosperlscript.so
	
clean:
	rm -fr */{Debug,Release} */*/{Debug,Release} */*/{Debug,Release} */*/*/{Debug,Release}
	rm -fr `find .  -name "*.class"  -o -name "*.o"  -o -name "lib*.a"  -o -name "*.d"  -print`
	-BUILD_OPT=1 JS_LIVECONNECT=1  $(MAKE)  -C js/src  -f Makefile.ref  clean
	-$(MAKE)  -C spidermonkey/Release  -f ../Makefile  clean

#common:
#	for f in .dispatch.*.pid; do [ -f $f ] && rm -v $f; done

$(BIN_DIR):
	mkdir $BIN_DIR

# Abkürzungen:

setuid:    $(BIN_DIR)/setuid

perl:      $(BIN_DIR)/libsosperlscript.so

perlscript: $(BIN_DIR)/libsosperlscript.so


spidermonkey: $(XML_LIB) $(XSLT_LIB) libzschimmer.a
	CFLAGS=-fPIC BUILD_OPT=1 JS_LIVECONNECT=1  JDK=$(JDK)  $(MAKE)  -C js/src/fdlibm  -f Makefile.ref
	CFLAGS=-fPIC BUILD_OPT=1 JS_LIVECONNECT=1  JDK=$(JDK)  $(MAKE)  -C js/src         -f Makefile.ref
	mkdir -p scripting_engine/Release
	$(MAKE)  -C scripting_engine/Release  -f ../Makefile

scheduler: javaproxy libs $(XSLT_LIB) $(SCHEDULER_DEPENDENTS)
	@make/make scheduler

$(BIN_DIR)/libjobscheduler-engine.so: javaproxy libs $(XSLT_LIB) $(SCHEDULER_DEPENDENTS)
	@make/make scheduler $@

$(BIN_DIR)/setuid:
	@make/make scheduler $(BIN_DIR)/setuid

$(BIN_DIR)/libspidermonkey.so: libzschimmer.a
	@make -C ../spidermonkey

$(BIN_DIR)/libsosperlscript.so:
	@make/make zschimmer $@


ifeq ($(cpuArchitecture),x64)
else
libs:: libxml2.a
endif
libs:: libzschimmer.a
libs:: libkram.a
libs:: libfile.a


javaproxy: libjavaproxy.a

libjavaproxy.a:
	@make/make javaproxy libjavaproxy.a

libxml2.a:
	@make/make 3rd_party/libxml2 libxml2.a

libxslt.a:
	@make/make 3rd_party/libxslt/libxslt libxslt.a

libfile.a:
	@make/make file libfile.a

libkram.a:
	@make/make kram libkram.a sosmain0.o

libzschimmer.a:
	@make/make zschimmer libzschimmer.a
