# $Id$

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
WITH_PHP=1
WITH_PERL=1
SCHEDULER_DEPENDENTS:=


ifeq ($(shell uname),Linux)
endif

ifeq ($(shell uname),SunOS)
WITH_PHP=0
endif

ifeq ($(shell uname),HP-UX)
WITH_PHP=0
SCHEDULER_DEPENDENTS:=libhostole.a libhostjava.a $(BIN_DIR)/sos.hostware.jar
endif


.PHONY: document_factory
.PHONY: document_processor
.PHONY: fs
.PHONY: kram
.PHONY: libxml2
.PHONY: libxslt
.PHONY: hostole
.PHONY: hostcopy
.PHONY: hostjava
.PHONY: hostodbc
.PHONY: hostole
.PHONY: hostphp
.PHONY: javascript
.PHONY: rapid
.PHONY: perlscript
.PHONY: scheduler
#.PHONY: scheduler_client
.PHONY: setuid
.PHONY: zschimmer
.PHONY: zschimmer/rtf

.PHONY: libxml2.a
.PHONY: libxslt.a
.PHONY: libzschimmer.a
.PHONY: librtf.a
.PHONY: libdocument_processor.a
.PHONY: libkram.a
.PHONY: libfile.a
.PHONY: libfs.a



all:: scheduler setuid
all:: hostole
all:: hostcopy
all:: hostjava

ifeq ($(WITH_PHP),1)
all:: hostphp 
endif

all:: $(BIN_DIR)/libsosperlscript.so
all:: spidermonkey
all:: scripting_engine_
	

clean:
	rm -fr */{Debug,Release} */*/{Debug,Release} */*/{Debug,Release} */*/*/{Debug,Release}
	rm -fr `find .  -name "*.class"  -o -name "*.o"  -o -name "lib*.a"  -o -name "*.d"  -print`
	-BUILD_OPT=1 JS_LIVECONNECT=1  $(MAKE)  -C js/src  -f Makefile.ref  clean
	-$(MAKE)  -C spidermonkey/Release  -f ../Makefile  clean

common:
	for f in .dispatch.*.pid; do [ -f $f ] && rm -v $f; done

$(BIN_DIR):
	mkdir $BIN_DIR


# Abkürzungen:

hostole:   $(BIN_DIR)/libhostole.so

scheduler: $(BIN_DIR)/scheduler $(BIN_DIR)/libscheduler.so

#scheduler_client: $(BIN_DIR)/scheduler_client

sos.spooler.jar: $(BIN_DIR)/sos.spooler.jar

setuid:    $(BIN_DIR)/setuid

hostcopy:  $(BIN_DIR)/hostcopy

hostphp:   $(BIN_DIR)/libhostphp-4.2.1.so

hostjava:  $(BIN_DIR)/libhostjava.so $(BIN_DIR)/sos.hostware.jar

perl:      $(BIN_DIR)/libsosperlscript.so

#javascript: $(BIN_DIR)/libspidermonkey.so $(BIN_DIR)/javascript

perlscript: $(BIN_DIR)/libsosperlscript.so

spidermonkey: libxml2.a libxslt.a libzschimmer.a
	CFLAGS=-fPIC BUILD_OPT=1 JS_LIVECONNECT=1  JDK=$(JDK)  $(MAKE)  -C js/src/fdlibm  -f Makefile.ref
	CFLAGS=-fPIC BUILD_OPT=1 JS_LIVECONNECT=1  JDK=$(JDK)  $(MAKE)  -C js/src         -f Makefile.ref
	mkdir -p scripting_engine/Release
	$(MAKE)  -C scripting_engine/Release  -f ../Makefile



$(BIN_DIR)/scheduler: javaproxy hostware libxslt.a $(SCHEDULER_DEPENDENTS)
	@make/make scheduler $@

$(BIN_DIR)/libscheduler.so: javaproxy hostware libxslt.a $(SCHEDULER_DEPENDENTS)
	@make/make scheduler $@

#$(BIN_DIR)/scheduler_client: libzschimmer.a
	@make/make scheduler $@

$(BIN_DIR)/setuid:
	@make/make scheduler $(BIN_DIR)/setuid

$(BIN_DIR)/libspidermonkey.so: libzschimmer.a
	@make -C ../spidermonkey

$(BIN_DIR)/hostcopy: hostware
	@make/make file hostcopy

$(BIN_DIR)/libhostjava.so: libhostole.a hostware libdocument_processor.a $(BIN_DIR)/sos.hostware.jar
	@make/make hostjava $@

$(BIN_DIR)/libhostphp-4.2.1.so: hostware
	@make/make hostphp $@

$(BIN_DIR)/libhostole.so: hostware libdocument_processor.a
	@make/make hostole $@

$(BIN_DIR)/libsosperlscript.so:
	@make/make zschimmer $@

### $(BIN_DIR)/sos.mail.jar:
###	@make/make kram $(BIN_DIR)/sos.mail.jar

$(BIN_DIR)/sos.hostware.jar:
	@make/make hostjava $(BIN_DIR)/sos.hostware.jar

$(BIN_DIR)/sos.spooler.jar:
	@make/make scheduler $(BIN_DIR)/sos.spooler.jar


hostware:: libxml2.a
hostware:: libzschimmer.a
hostware:: librtf.a
hostware:: libkram.a
hostware:: libfile.a
hostware:: libfs.a
### hostware:: $(BIN_DIR)/sos.mail.jar


javaproxy: libjavaproxy.a
libjavaproxy.a:
	@make/make javaproxy libjavaproxy.a

libxml2.a:
	@make/make 3rd_party/libxml2 libxml2.a

libxslt.a:
	@make/make 3rd_party/libxslt/libxslt libxslt.a

libdocument_processor.a:
	@make/make document_processor libdocument_processor.a

libfile.a:
	@make/make file libfile.a

libfs.a:
	@make/make fs libfs.a

libkram.a:
	@make/make kram libkram.a sosmain0.o

libhostole.a:
	@make/make hostole libhostole.a

libhostjava.a:
	@make/make hostjava libhostjava.a

librtf.a:
	@make/make zschimmer/rtf librtf.a

libzschimmer.a:
	@make/make zschimmer libzschimmer.a
