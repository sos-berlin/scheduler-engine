# $Id: Makefile,v 1.3 2002/11/25 08:57:23 jz Exp $

ifndef PROD_DIR
prod_dir = ..

include $(prod_dir)/make/base.makefile

else

DEP_PRODUCTS = kram file rapid fs zschimmer

objects = \
 spooler.o\
 spooler_com.o\
 spooler_command.o\
 spooler_communication.o\
 spooler_config.o\
 spooler_history.o\
 spooler_log.o\
 spooler_mail.o\
 spooler_mail_jmail.o\
 spooler_module.o\
 spooler_module_com.o\
 spooler_module_java.o\
 spooler_order.o\
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
 sos/spooler/Task.class\
 sos/spooler/Thread.class\
 sos/spooler/Variable_set.class

java_headers=$(patsubst %.class, %.h, $(java_classes) )

all:: $(BIN_DIR)/spooler
all:: libspooler.a

clean:
	rm *.o *.d lib*.a lib*.so *.map

include $(PROD_DIR)/make/standard.makefile

%.class: %.java
	mkdir -p $(dir $@)
	javac -d . -classpath .. $<

%.h: %.class
	mkdir -p $(dir $@)
	javah -o $@ -classpath . $(subst /,.,$(patsubst %.class, %, $<))


libspooler.a: $(java_headers) $(objects)
	$(AR) $(ARFLAGS) $@ $(objects)


$(BIN_DIR)/spooler: spooler.o libspooler.a ../kram/$(O_DIR)/soswnmai.o $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/$(p)/$(O_DIR)/lib$(p).a) $(PERL_DIR)/libperl.a
	-$(CCPP) $(DEBUG) -static $(LINK_FLAGS) -Xlinker -Map -Xlinker $(BIN_DIR)/spooler.map  $^ $(VERBOSE) $(CFLAGS) $(INCLUDES) $(TEMPLATES) $(LIBPATH) $(SOS_LIBS) $(SOS_LIBS) $(ORACLE_LIBS) $(RW_LIBS) $(NET_LIBS) $(C_LIBS) $(LIBS) -o $@
	echo ^G



endif
