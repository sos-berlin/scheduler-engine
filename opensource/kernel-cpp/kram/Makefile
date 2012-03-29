# $Id: Makefile 14067 2010-10-07 07:44:57Z rb $

PROD_DIR = $(shell cd ../.. && pwd)

objects = \
 area.o\
 com_simple_standards.o\
 com_server.o\
 decimal.o\
 dynobj.o\
 ebc2iso.o\
 env.o\
 licence.o\
 licenceg.o\
 log.o\
 msec.o\
 sleep.o\
 sos_mail.o\
 sos_mail_java.o\
 sosalloc.o\
 sosarray.o\
 sosclien.o\
 sosctype.o\
 sosdate.o\
 sosdll.o\
 sosdumcl.o\
 sosfact.o\
 sosfiltr.o\
 soserror.o\
 sosfield.o\
 sosfld2.o\
 sosfunc.o\
 soslimt2.o\
 soslimtx.o\
 sosmsg.o\
 sosobj.o\
 sosobjba.o\
 sosopt.o\
 sosprof.o\
 sosscrpt.o\
 sosset.o\
 sossock.o\
 sossql1.o\
 sossql2.o\
 sossql2a.o\
 sossql3.o\
 sossql4.o\
 sossql_group_by.o\
 sossqlfl.o\
 sossqlfn.o\
 sossqlty.o\
 sosstat.o\
 sosstrea.o\
 sosstrg0.o\
 sosstrng.o\
 sossv2.o\
 sostimer.o\
 sosuser.o\
 sqlfield.o\
 stdfield.o\
 strupr.o\
 sysdep.o\
 sysxcept.o\
 truncsp.o\
 xception.o\

include $(PROD_DIR)/make/standard.makefile

CFLAGS += -fno-strict-aliasing
CPPFLAGS += -Wno-deprecated -Wno-reorder -Wno-long-long -Wno-sign-compare 

all:: libkram.a
all:: soswnmai.o
all:: sosmain0.o

libkram.a: $(objects) soswnmai.o
	$(AR) $(ARFLAGS) $@ $(objects)

soswnmai.o: soswnmai.cxx

sosmain0.o: sosmain0.cxx
