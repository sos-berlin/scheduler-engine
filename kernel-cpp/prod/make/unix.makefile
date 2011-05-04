# $Id$
# Einstellungen nur für Unix

CP        = cp

CC        = $(CC_BINDIR)/CC
CFLAGS    =

AR        = ar
ARFLAGS   = -crs
OBJSUFFIX = o

LINKER    = $(CCPP)

INCLUDES  += $(SOS_INCLUDES)
#INCLUDES  += $(SV_INCLUDES)
#INCLUDES  += $(ORACLE_INCLUDES)
#INCLUDES  += -I$(PROD_DIR)/LINKS/java/include
#INCLUDES  += -I$(PROD_DIR)/LINKS/java/include/linux
INCLUDES  += $(foreach p,$(wildcard $(PROD_DIR)/LINKS/include.*),-I$p)

LIBPATH   = $(SOS_LIBPATH) $(SV_LIBPATH) $(INGRES_LIBPATH) $(ORACLE_LIBPATH)

SOS_LIBS     = $(foreach p,$(DEP_PRODUCTS),$(PROD_DIR)/../prod/$(p)/$(O_DIR)/lib$(notdir $p).a)
EBO_LIBS     = -L$(PROD_DIR)/misc/lib/ebo -lctleasy -lsvb -lkatfunc -lctree -ldberror -ldbclnt -ldbsock -lleatools /home/joacim/kunden/sos/e/prod/misc/lib/ebo/lipcsock.a -ltools -lconfig -lfileio -lcproc -lgterm
LIBS         = $(C_LIBS)

#PERL_DIR = /usr/local/lib/perl5/5.6.1/i686-linux/CORE
PERL_DIR = $(PROD_DIR)/LINKS/perl/CORE

#-----------------------------------------------------------------------Regeln

# Ist durch $(CC) -MD überflüssig: (oder?)
%.d: %.cxx
ifdef SYSTEM_GNU
#	@echo $@
	-C_INCLUDE_PATH="$(C_INCLUDE_PATH)" $(CPP) -M $(CFLAGS) $(INCLUDES) $< >$@
#   -: Fehler werden für Linux ignoriert
else
#	@echo $@
	$(SHELL) -ec 'C_INCLUDE_PATH='$(C_INCLUDE_PATH)' $(CPP) -M $(CPPFLAGS) $(INCLUDES) $< | sed '\''s/$*.o/& $@/g'\'' >$@'
endif

#%.d: %.c
#ifdef SYSTEM_GNU
#	-C_INCLUDE_PATH="$(C_INCLUDE_PATH)"  $(CPP) -M $(CPPFLAGS) $(INCLUDES) $< >$@
#else
#	$(SHELL) -ec '$(CPP) -M $(CPPFLAGS) $(INCLUDES) $< | sed '\''s/$*.o/& $@/g'\'' >$@'
#endif

%.o: %.c
	@echo $(dispatch) $(CC) ... -c $(CFLAGS) $<
	@C_INCLUDE_PATH="$(C_INCLUDE_PATH)" $(dispatch) $(CC) -c $(HIDDEN_CFLAGS) $(CFLAGS) $(INCLUDES) -o $@  $<

%.o: %.cxx
	@echo $(dispatch) $(CCPP) ... -c $(CFLAGS) $(CCPPFLAGS) $<
	@C_INCLUDE_PATH="$(C_INCLUDE_PATH)" $(dispatch) $(CCPP) -c $(HIDDEN_CFLAGS) $(CFLAGS) $(CCPPFLAGS) $(INCLUDES) -o $@  $<

%.a: %.o
	$(dispatch) $(AR) $(ARFLAGS) $@ $^

%.class: %.java
	$(dispatch) $(PROD_DIR)/LINKS/java/bin/javac -d . -sourcepath .. -classpath $(PROD_DIR)/LINKS/mail.jar:$(PROD_DIR)/LINKS/activation.jar:$(PROD_DIR)/LINKS/xercesImpl.jar $<

%.h: %.class
	$(PROD_DIR)/LINKS/java/bin/javah -o $@ -classpath . $(subst /,.,$(subst ../$(O_DIR)/,,$(basename $<)))
