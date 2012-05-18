# $Id: Makefile 15017 2011-08-24 14:01:36Z jz $

PROD_DIR = $(shell cd ../.. && /bin/pwd)

DEP_PRODUCTS := javaproxy kram file zschimmer

ifeq ($(shell uname -s -m),HP-UX ia64)
# Java schafft es nicht, libhostjava.so nachzuladen. Also binden wir es ein.
DEP_PRODUCTS += hostjava hostole
endif

include ../jni_cppproxy.makefile.include

objects = \
 $(jni_cppproxy_objects)\
 Event_subsystem.o\
 Module_monitor_instances.o\
 Order.o\
 Settings.o\
 cluster.o\
 database.o\
 directory_observer.o\
 file_logger.o\
 folder.o\
 folder_Base_file_info.o\
 folder_Dependant.o\
 folder_Dependencies.o\
 folder_File_based.o\
 folder_File_based_subsystem.o\
 folder_Folder.o\
 folder_Folder_subsystem.o\
 folder_Requisite_path.o\
 folder_Subfolder_folder.o\
 folder_Typed_folder.o\
 java_subsystem.o\
 include.o\
 lock.o\
 log_cache_Request.o\
 log_cache_Request_cache.o\
 path.o\
 schedule.o\
 scheduler_client.o\
 scheduler_messages.o\
 scheduler_object.o\
 scheduler_script.o\
 spooler.o\
 spooler_com.o\
 spooler_common.o\
 spooler_command.o\
 spooler_communication.o\
 spooler_config.o\
 spooler_dll.o\
 spooler_event.o\
 spooler_embedded_files.o\
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
 xml_client_connection.o

include $(PROD_DIR)/make/standard.makefile

all:: spooler.h.gch
all:: $(BIN_DIR)/scheduler
all:: $(BIN_DIR)/libjobscheduler-engine.so
all:: $(BIN_DIR)/setuid


ifeq ($(OS),HP-UX)
ifeq "$(shell uname -m)" "ia64"
LIBS += -Wl,+b,/opt/java1.4/jre/lib/IA64N/server:/opt/java1.4/jre/lib/IA64N
LINK_FLAGS += -Wl,+e,main
LINK_FLAGS += -Wl,-B -Wl,immediate 
endif
endif

ifeq ($(cpuArchitecture),x64)
XML_LIBS=	
else
XML_LIBS=$(PROD_DIR)/3rd_party/libxslt/libxslt/$(O_DIR)/libxslt.a  $(PROD_DIR)/3rd_party/libxml2/$(O_DIR)/libxml2.a
endif

ifeq ($(cpuArchitecture),x64)
else	
$(BIN_DIR)/scheduler: spooler.o $(objects) ../kram/$(O_DIR)/sosmain0.o ../kram/$(O_DIR)/soswnmai.o  $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(p).a) $(XML_LIBS)
	-$(CCPP) $(DEBUG) $(LINK_FLAGS) $^ $(LIBPATH) $(SOS_LIBS) $(LIBS) -o $@
	chmod a+rx $@
	$(PROD_DIR)/make/separate-debug-info "$@"
endif

$(BIN_DIR)/libjobscheduler-engine.so: spooler_dll_java.o $(objects) ../kram/$(O_DIR)/sosmain0.o ../kram/$(O_DIR)/soswnmai.o  $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(p).a) $(XML_LIBS)
	-$(CCPP) $(DEBUG) -shared $(LINK_FLAGS) $^ $(LIBPATH) $(SOS_LIBS) $(LIBS) -o $@ $(shell $(PROD_DIR)/make/linker-export-symbols spooler_program JNI_OnLoad JNI_OnUnload Java_com_sos_scheduler_engine_kernel_CppScheduler_runNative)
	chmod a+rx $@
	$(PROD_DIR)/make/separate-debug-info "$@"

ifeq ($(cpuArchitecture),x64)
$(BIN_DIR)/scheduler: spooler_start_dll.o $(BIN_DIR)/libjobscheduler-engine.so
	-$(CCPP) $(DEBUG) $(LINK_FLAGS) spooler_start_dll.o -L$(BIN_DIR) -ljobscheduler-engine $(LIBPATH) $(LIBS) -o $@
	chmod a+rx $@
	$(PROD_DIR)/make/separate-debug-info "$@"
endif

$(BIN_DIR)/setuid: setuid.o 
	-$(CC) $(DEBUG) $(LINK_FLAGS) $^ $(LIBPATH)  -o $@  -s
	chmod a+rx $@
	$(PROD_DIR)/make/separate-debug-info "$@"

documention:
	( cd ../doc  &&  perl ../scheduler_keyword_to_xml.pl *.xml xml/*.xml xml/answer/*.xml )
	ant -f ../javadoc.xml
