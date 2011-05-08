# $Id$

PROD_DIR = $(shell cd ../.. && /bin/pwd)

DEP_PRODUCTS  = hostole file fs kram zschimmer zschimmer/rtf 

objects=\
 hostjava.o\
 java_Factory_processor.o\
 java_Process_with_pid.o\
 java_Series_factory.o\
 java_Variable.o

java_classes=$(patsubst %, sos/hostware/%.class, Factory_processor Global File Process_with_pid Record Rerun Scripttext_flags)


include $(PROD_DIR)/make/standard.makefile


all:: $(BIN_DIR)/sos.hostware.jar
all:: libhostjava.a
all:: $(BIN_DIR)/libhostjava.so


$(BIN_DIR)/sos.hostware.jar: $(java_classes)
	$(PROD_DIR)/LINKS/java/bin/jar cf $@  sos/hostware/*.class
	chmod a+r $@


#$(objects): $(patsubst %, sos/hostware/%.h, Factory_processor File Record Rerun Scripttext_flags Variable) 

$(objects): $(patsubst %.class, %.h, $(java_classes))


hostjava_export =  JNI_OnLoad
hostjava_export += JNI_OnUnload
hostjava_export += Java_sos_hostware_Idispatch_com_1construct
hostjava_export += Java_sos_hostware_Idispatch_com_1release
hostjava_export += Java_sos_hostware_Idispatch_com_1call
hostjava_export += Java_sos_hostware_Idispatch_boolean_1com_1call
hostjava_export += Java_sos_hostware_Idispatch_string_1com_1call
hostjava_export += Java_sos_hostware_Idispatch_string_1com_1call_1string
hostjava_export += Java_sos_hostware_Factory_1processor_close_1native
hostjava_export += Java_sos_hostware_Factory_1processor_eval
hostjava_export += Java_sos_hostware_Factory_1processor_parse
hostjava_export += Java_sos_hostware_Factory_1processor_add_1parameters
hostjava_export += Java_sos_hostware_Factory_1processor_process
hostjava_export += Java_sos_hostware_Factory_1processor_script_1text
hostjava_export += Java_sos_hostware_Factory_1processor_error_1filename
hostjava_export += Java_sos_hostware_Factory_1processor_error_1document
hostjava_export += Java_sos_hostware_Factory_1processor_set_1document_1filename
hostjava_export += Java_sos_hostware_Factory_1processor_document_1filename
hostjava_export += Java_sos_hostware_Factory_1processor_set_1head_1filename
hostjava_export += Java_sos_hostware_Factory_1processor_head_1filename
hostjava_export += Java_sos_hostware_Factory_1processor_set_1language
hostjava_export += Java_sos_hostware_Factory_1processor_language
hostjava_export += Java_sos_hostware_Factory_1processor_set_1param
hostjava_export += Java_sos_hostware_Factory_1processor_set_1parameter
hostjava_export += Java_sos_hostware_Factory_1processor_set_1parameter_1bool
hostjava_export += Java_sos_hostware_Factory_1processor_set_1parameter_1int
hostjava_export += Java_sos_hostware_Factory_1processor_set_1parameter_1double
hostjava_export += Java_sos_hostware_Factory_1processor_set_1parameter_1currency
hostjava_export += Java_sos_hostware_Factory_1processor_parameter_1as_1string
hostjava_export += Java_sos_hostware_Factory_1processor_set_1template_1filename
hostjava_export += Java_sos_hostware_Factory_1processor_template_1filename
hostjava_export += Java_sos_hostware_Process_1with_1pid_construct
hostjava_export += Java_sos_hostware_Process_1with_1pid_close
hostjava_export += Java_sos_hostware_Process_1with_1pid_start
hostjava_export += Java_sos_hostware_Process_1with_1pid_start_1array
hostjava_export += Java_sos_hostware_Process_1with_1pid_pid
hostjava_export += Java_sos_hostware_Process_1with_1pid_terminated
hostjava_export += Java_sos_hostware_Process_1with_1pid_destroy
hostjava_export += Java_sos_hostware_Process_1with_1pid_waitFor
hostjava_export += Java_sos_hostware_Process_1with_1pid_exitValue
hostjava_export += Java_sos_hostware_Process_1with_1pid_kill_1pid
hostjava_export += Java_sos_hostware_Process_1with_1pid_try_1kill_1pid
hostjava_export += Java_sos_hostware_Process_1with_1pid_set_1own_1process_1group



# Fuer HP-UX: libhostjava.a wird im Scheduler eingebunden, weil dlopen() statische Variablen nicht initialisiert (gcc 3.2)
libhostjava.a: $(objects)
	$(AR) $(ARFLAGS) $@ $^


$(BIN_DIR)/libhostjava.so: $(objects) $(PROD_DIR)/hostole/$(O_DIR)/hostoled.o $(PROD_DIR)/document_processor/$(O_DIR)/factory_processor.o $(PROD_DIR)/hostole/$(O_DIR)/rerun_file.o $(PROD_DIR)/document_processor/$(O_DIR)/libdocument_processor.a $(SOS_LIBS) $(PROD_DIR)/3rd_party/libxml2/$(O_DIR)/libxml2.a  $(PROD_DIR)/3rd_party/libxslt/libxslt/$(O_DIR)/libxslt.a
	$(CCPP) -shared $(LINK_FLAGS) $^ $(LIBS) `$(PROD_DIR)/make/linker-export-symbols $(hostjava_export)`  -o $@
	@( ldd -r $@ >/dev/null ) 2>&1 | c++filt
	chmod a+rx $@
	$(PROD_DIR)/make/separate-debug-info "$@"


#$(BIN_DIR)/libhostjava.so: $(objects) $(PROD_DIR)/hostole/$(O_DIR)/hostoled.o $(PROD_DIR)/factory/processor/$(O_DIR)/factory_processor.o $(PROD_DIR)/factory/processor/$(O_DIR)/libfactory_processor.a ../zschimmer/$(O_DIR)/perl_scripting_engine_module.o $(SOS_LIBS) $(PERL_DIR)/libperl.a
#	$(CCPP) -shared $(LINK_FLAGS) $^ $(LIBS) -Xlinker -Map -Xlinker $(basename $@).map -Xlinker -cref  -o $@
#	chmod a+rx $@

test: Hostjava_test.class
	java -classpath . -Djava.library.path=../../prod/bind Hostjava_test
