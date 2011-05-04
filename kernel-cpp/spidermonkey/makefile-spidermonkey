# $Id$

export O_DIR=Release

JDK=$(shell pwd)/LINKS/java
#JDK_IA32=$(shell pwd)/LINKS/java/../ia32-$(shell basename `readlink "$(JDK)"`)
#JDK:=$(if $(shell test -e "$(JDK_IA32)" && echo 1),$(JDK_IA32),$(JDK))


all: bin spidermonkey scripting_engine_


clean:
	-BUILD_OPT=1 JS_LIVECONNECT=1  $(MAKE)  -C js/src  -f Makefile.ref  clean
	-$(MAKE)  -C scripting_engine/Release  -f ../Makefile  clean
	-rm -rf `find js -name "*.o"`
	-rm -rf `find js -name "lib*.a"`
	-rm -rf `find js -name "lib*.so"`
	-rm -rf `find scripting_engine -name "*.o"`
	-rm -rf `find scripting_engine -name "lib*.a"`
	-rm -rf `find scripting_engine -name "lib*.so"`


bin:
	mkdir bin


spidermonkey:
	CFLAGS=-fPIC BUILD_OPT=1 JS_LIVECONNECT=1  JDK=$(JDK)  $(MAKE)  -C js/src/fdlibm  -f Makefile.ref
	CFLAGS=-fPIC BUILD_OPT=1 JS_LIVECONNECT=1  JDK=$(JDK)  $(MAKE)  -C js/src         -f Makefile.ref 


scripting_engine_: spidermonkey
	mkdir -p scripting_engine/Release
	$(MAKE)  -C scripting_engine/Release  -f ../Makefile
