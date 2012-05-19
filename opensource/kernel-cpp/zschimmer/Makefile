# $Id: Makefile 14067 2010-10-07 07:44:57Z rb $


PROD_DIR = $(shell cd ../.. && pwd)
INCLUDES += -I$(PROD_DIR)/LINKS

base_objects = \
 Memory_allocator.o\
 base.o\
 check_compiler.o\
 loaded_module.o\
 message.o\
 log.o\
 log_categories.o\
 olechar.o\
 reference.o\
 string_stream.o\
 z_io.o\
 z_posix.o\
 zschimmer.o\

java_objects = \
 Has_proxy.o\
 Proxy_class.o\
 java.o\
 java2.o\
 java_com.o\
 java_jvm.o\
 java_odl.o

thread_objects = \
 mutex.o\
 threads.o\
 threads_base.o\
 z_posix_mutex.o\
 z_posix_thread.o\

regex_objects = \
 regex_class.o
 
remote_objects = \
 com_remote.o\
 com_remote_messages.o

xml_objects = \
 xml.o\
 xml_end_finder.o\
 xml_java.o\
 xml_libxml2.o\
 xslt_libxslt.o\
 xslt_java.o

com_server_objects = \
 com.o\
 com_base.o\
 com_server.o\
 z_com.o\
 z_com_server.o
 
libsosperlscript_objects = \
 perl.o\
 perl_com.o\
 perl_scripting_engine.o\
 perl_scripting_engine_module.o\
 scripting_engine.o \
 file.o \
 z_posix_directory.o \
 $(base_objects) \
 $(com_server_objects) \
 $(java_objects) \
 $(regex_objects) \
 $(thread_objects)

libzschimmer_objects = \
 $(base_objects) \
 $(com_server_objects) \
 $(java_objects) \
 $(regex_objects) \
 $(remote_objects) \
 $(thread_objects) \
 $(xml_objects) \
 base64.o\
 argv.o\
 async.o\
 async_event.o\
 async_io.o\
 async_socket.o\
 charset.o\
 charset_messages.o\
 directory_lister.o\
 embedded_files.o\
 file.o\
 file_path.o\
 md5.o\
 pipe_collector.o\
 system_command.o\
 scripting_engine.o\
 string_list.o\
 string_stream.o\
 z_gzip.o\
 z_mail.o\
 z_md5.o\
 z_posix_directory.o\
 z_posix_event.o\
 z_posix_process.o\
 z_posix_system_information.o\
 z_process.o\
 z_signals.o\
 z_sockets.o\
 z_sql.o



include $(PROD_DIR)/make/standard.makefile


ifeq "$(shell uname -m)" "ia64"
else
PERL_LIBS += -lcrypt
endif


ifeq ($(GCC_HAS_PRECOMPILED_HEADERS),1)
all:: zschimmer.h.gch
endif
all:: libzschimmer.a 


libzschimmer.a: $(libzschimmer_objects)
	$(AR) $(ARFLAGS) $@ $^

libsosperlscript.a: $(libsosperlscript_objects)
	$(AR) $(ARFLAGS) $@ $^

show-libsosperlscript-files:
	@echo $(shell cd .. && ls $(libsosperlscript_objects:.o=.{h,cxx} 2>/dev/null ))

$(BIN_DIR)/libsosperlscript.so: perl_scripting_engine_module.o libsosperlscript.a
	$(CCPP) -shared $(LINK_FLAGS) $^ -L`perl -e 'print readlink("../../LINKS/perl");'`/CORE -lperl $(PERL_LIBS) $(LINK_FLAGS) $(wildcard $(PROD_DIR)/LINKS/perl/auto/DynaLoader/DynaLoader.a) $(LIBS) -o $@  $(shell $(PROD_DIR)/make/linker-export-symbols DllMain DllGetClassObject)
	@( ldd -r $@ >/dev/null ) 2>&1 | c++filt
	@echo
	chmod a+rx $@
	$(PROD_DIR)/make/separate-debug-info "$@"



testperl.o: testperl.cxx

testperl: testperl.o libzschimmer.a $(PERL_DIR)/libperl.a
	-$(CCPP) $(DEBUG) -lcrypt $(LINK_FLAGS) -Xlinker -Map -Xlinker testperl.map  $^ $(VERBOSE) $(CFLAGS) $(INCLUDES) $(TEMPLATES) $(LIBPATH) $(C_LIBS)  $(LIBS) -o $@

test.o: test.cxx

test: test.o libzschimmer.a $(PERL_DIR)/libperl.a
	-$(CCPP) $(DEBUG) -lcrypt $(LINK_FLAGS) -Xlinker -Map -Xlinker testperl.map  $^ $(VERBOSE) $(CFLAGS) $(INCLUDES) $(TEMPLATES) $(LIBPATH) $(C_LIBS) $(LIBS) -o $@
