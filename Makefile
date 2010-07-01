# $Id$

PROD_DIR = $(shell cd ../.. && /bin/pwd)

DEP_PRODUCTS := javaproxy kram file fs zschimmer

ifeq ($(shell uname -s -m),HP-UX ia64)
# Java schafft es nicht, libhostjava.so nachzuladen. Also binden wir es ein.
DEP_PRODUCTS += hostjava hostole
endif


objects = \
 cluster.o\
 database.o\
 directory_observer.o\
 file_logger.o\
 folder.o\
 java_subsystem.o\
 include.o\
 lock.o\
 path.o\
 schedule.o\
 scheduler_client.o\
 Scheduler_event2.o\
 scheduler_messages.o\
 scheduler_object.o\
 scheduler_script.o\
 spooler.o\
 spooler_com.o\
 spooler_common.o\
 spooler_command.o\
 spooler_communication.o\
 spooler_config.o\
 spooler_event.o\
 spooler_embedded_files.o\
 spooler_embedded_files_z.o\
 spooler_http.o\
 spooler_job.o\
 spooler_log.o\
 spooler_mail.o\
 spooler_module.o\
 spooler_module_com.o\
 spooler_module_internal.o\
 spooler_module_java.o\
 spooler_module_process.o\
 spooler_module_remote.o\
 spooler_module_remote_server.o\
 spooler_module_script.o\
 spooler_order.o\
 spooler_order_file.o\
 spooler_process.o\
 spooler_security.o\
 spooler_service.o\
 spooler_subprocess.o\
 spooler_task.o\
 spooler_thread.o\
 spooler_time.o\
 spooler_wait.o\
 spooler_web_service.o\
 spooler_xslt_stylesheet.o\
 standing_order.o\
 subsystem.o\
 supervisor.o\
 supervisor_client.o\
 version.o\
 xml_client_connection.o\
 Event_subsystem.o\
 Module_monitor_instances.o


java_classes=\
 sos/spooler/Error.class\
 sos/spooler/Idispatch.class\
 sos/spooler/Job.class\
 sos/spooler/Job_chain.class\
 sos/spooler/Job_chain_node.class\
 sos/spooler/Job_impl.class\
 sos/spooler/Lock.class\
 sos/spooler/Locks.class\
 sos/spooler/Log.class\
 sos/spooler/Mail.class\
 sos/spooler/Monitor_impl.class\
 sos/spooler/Order.class\
 sos/spooler/Order_queue.class\
 sos/spooler/Process_class.class\
 sos/spooler/Process_classes.class\
 sos/spooler/Spooler.class\
 sos/spooler/Spooler_program.class\
 sos/spooler/Subprocess.class\
 sos/spooler/Task.class\
 sos/spooler/Variable_set.class\
 sos/spooler/jobs/Web_service_forwarder.class

java_headers=$(patsubst %.class, %.h, $(java_classes) )


include $(PROD_DIR)/make/standard.makefile


all:: $(BIN_DIR)/scheduler
all:: $(BIN_DIR)/sos.spooler.jar
all:: $(BIN_DIR)/setuid
all:: documentation

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
	$(PROD_DIR)/LINKS/java/bin/jar cfm $@ ../java_manifest sos/spooler/*.class sos/spooler/jobs/*.class


$(objects): $(patsubst %, sos/spooler/%.h, Idispatch)


ifeq ($(OS),HP-UX)

ifeq "$(shell uname -m)" "ia64"
LIBS += -Wl,+b,/opt/java1.4/jre/lib/IA64N/server:/opt/java1.4/jre/lib/IA64N
LINK_FLAGS += -Wl,+e,main
LINK_FLAGS += -Wl,-B -Wl,immediate 
#LINK_FLAGS += -Wl,-B -Wl,direct 
#LINK_FLAGS += -Wl,-B -Wl,verbose 
#LINK_FLAGS += -ldld
#LINK_FLAGS += -luca
#LINK_FLAGS += -l/usr/lib/hpux32/libuca.so.1
else
# Der folgende Pfad muss bei Programmaufruf gueltig sein, also auf der Produktionsmaschine!
#LIBS += -Wl,+b,/opt/java1.4/jre/lib/PA_RISC2.0:/opt/java1.4/jre/lib/PA_RISC2.0/server
endif

#ifeq ($(NO_PERL),)
## HP-UX: Der folgende Pfad muss bei Programmaufruf gueltig sein, also auf der Produktionsmaschine!
#SOS_LIBS += $(PROD_DIR)/zschimmer/$(O_DIR)/libsosperlscript.a
#endif
endif


#$(BIN_DIR)/scheduler: spooler.o $(objects) ../kram/$(O_DIR)/soswnmai.o ../zschimmer/$(O_DIR)/perl_scripting_engine_module.o $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(p).a) $(PERL_DIR)/libperl.a
#	-$(CCPP) $(DEBUG) $(LINK_FLAGS) $^ $(LIBPATH) $(SOS_LIBS) $(LIBS) -o $@

#$(BIN_DIR)/scheduler: spooler.o $(objects) ../kram/$(O_DIR)/soswnmai.o ../zschimmer/$(O_DIR)/perl_scripting_engine_module.o $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(p).a) $(PERL_DIR)/libperl.a
#	-$(CCPP) $(DEBUG) $(LINK_FLAGS) $^ $(LIBPATH) $(SOS_LIBS) $(LIBS) -o $@
#	chmod a+rx $@

$(BIN_DIR)/scheduler: spooler.o $(objects) ../kram/$(O_DIR)/sosmain0.o ../kram/$(O_DIR)/soswnmai.o  $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(p).a)  $(PROD_DIR)/3rd_party/libxslt/libxslt/$(O_DIR)/libxslt.a  $(PROD_DIR)/3rd_party/libxml2/$(O_DIR)/libxml2.a
	-$(CCPP) $(DEBUG) $(LINK_FLAGS) $^ $(LIBPATH) $(SOS_LIBS) $(LIBS) -o $@
	chmod a+rx $@
	$(PROD_DIR)/make/separate-debug-info "$@"

$(BIN_DIR)/setuid: setuid.o 
	-$(CC) $(DEBUG) $(LINK_FLAGS) $^ $(LIBPATH)  -o $@  -s
	chmod a+rx $@
	$(PROD_DIR)/make/separate-debug-info "$@"

#$(BIN_DIR)/scheduler_client: scheduler_client.o  $(PROD_DIR)/zschimmer/$(O_DIR)/libzschimmer.a  $(PROD_DIR)/3rd_party/libxml2/$(O_DIR)/libxml2.a
#	-$(CCPP) $(DEBUG) $(LINK_FLAGS) $^ $(LIBPATH) $(LIBS) -o $@ 
#	chmod a+rx $@



documention:
	( cd ../doc  &&  perl ../scheduler_keyword_to_xml.pl *.xml xml/*.xml xml/answer/*.xml )
	ant -f ../javadoc.xml
