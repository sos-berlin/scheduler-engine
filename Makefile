# $Id: Makefile,v 1.31 2004/12/20 08:58:04 jz Exp $

ifndef PROD_DIR
prod_dir = ..

include $(prod_dir)/make/base.makefile

else

DEP_PRODUCTS = kram file rapid fs zschimmer libxml2

objects = \
 spooler.o\
 spooler_com.o\
 spooler_command.o\
 spooler_communication.o\
 spooler_config.o\
 spooler_dtd.o\
 spooler_history.o\
 spooler_http.o\
 spooler_job.o\
 spooler_log.o\
 spooler_mail.o\
 spooler_module.o\
 spooler_module_com.o\
 spooler_module_java.o\
 spooler_module_remote.o\
 spooler_module_remote_server.o\
 spooler_order.o\
 spooler_process.o\
 spooler_security.o\
 spooler_service.o\
 spooler_task.o\
 spooler_thread.o\
 spooler_time.o\
 spooler_wait.o

java_classes=\
 sos/spooler/Error.class\
 sos/spooler/Idispatch.class\
 sos/spooler/Job.class\
 sos/spooler/Job_chain.class\
 sos/spooler/Job_chain_node.class\
 sos/spooler/Job_impl.class\
 sos/spooler/Log.class\
 sos/spooler/Mail.class\
 sos/spooler/Order.class\
 sos/spooler/Order_queue.class\
 sos/spooler/Spooler.class\
 sos/spooler/Spooler_program.class\
 sos/spooler/Task.class\
 sos/spooler/Variable_set.class

java_headers=$(patsubst %.class, %.h, $(java_classes) )

all:: $(BIN_DIR)/scheduler
all:: $(BIN_DIR)/sos.spooler.jar
all:: DOC

clean:
	rm *.o *.d lib*.a lib*.so *.map `find -name "*.class"`

include $(PROD_DIR)/make/standard.makefile

#%.class: %.java
#	@mkdir -p $(dir $@)
#	javac -d . -classpath .. $<
#
#%.h: %.class
#	@mkdir -p $(dir $@)
#	javah -force -o $@ -classpath . $(subst /,.,$(patsubst %.class, %, $<))
#

#libspooler.a: $(java_headers) $(objects)
#	$(AR) $(ARFLAGS) $@ $(objects)

$(BIN_DIR)/sos.spooler.jar: $(java_classes)
	jar cfm $@ ../java_manifest sos/spooler/*.class


$(objects): $(patsubst %, sos/spooler/%.h, Idispatch)


ifeq ($(OS),HP-UX)
# Die folgende Pfad muss bei Programmaufruf gueltig sein, also auf der Produktionsmaschine!
LIBS += -Wl,+b,/opt/java1.4/jre/lib/PA_RISC2.0:/opt/java1.4/jre/lib/PA_RISC2.0/server
endif


#$(BIN_DIR)/scheduler: spooler.o $(objects) ../kram/$(O_DIR)/soswnmai.o ../zschimmer/$(O_DIR)/perl_scripting_engine_module.o $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(p).a) $(PERL_DIR)/libperl.a
#	-$(CCPP) $(DEBUG) $(LINK_FLAGS) $^ $(LIBPATH) $(SOS_LIBS) $(LIBS) -o $@

#$(BIN_DIR)/scheduler: spooler.o $(objects) ../kram/$(O_DIR)/soswnmai.o ../zschimmer/$(O_DIR)/perl_scripting_engine_module.o $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(p).a) $(PERL_DIR)/libperl.a
#	-$(CCPP) $(DEBUG) $(LINK_FLAGS) $^ $(LIBPATH) $(SOS_LIBS) $(LIBS) -o $@
#	chmod a+rx $@

$(BIN_DIR)/scheduler: spooler.o $(objects) ../kram/$(O_DIR)/soswnmai.o $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(p).a)
	-$(CCPP) $(DEBUG) $(LINK_FLAGS) $^ $(LIBPATH) $(SOS_LIBS) $(LIBS) -o $@
	chmod a+rx $@



DOC:
	( cd ../doc  &&  perl ../scheduler_keyword_to_xml.pl *.xml xml/*.xml xml/answer/*.xml )
	ant -f ../javadoc.xml

endif
