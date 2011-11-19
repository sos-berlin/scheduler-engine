# $Id: Makefile 14067 2010-10-07 07:44:57Z rb $

PROD_DIR = $(shell cd ../.. && pwd)

objects = \
 a2a.o\
 com_simple_standards.o\
 com_server.o\
 ebc2iso.o\
 iso2ebc.o\
 area.o\
 charset.o\
 chkarray.o\
 cobfield.o\
 ddeobj.o\
 decimal.o\
 delphi.o\
 dynobj.o\
 e2a.o\
 ebcdic.o\
 ebcdifld.o\
 env.o\
 exp10.o\
 frmfield.o\
 itoa.o\
 licence.o\
 licenceg.o\
 log.o\
 logwin.o\
 msec.o\
 nullasgn.o\
 oleserv.o\
 pointer.o\
 set.o\
 sleep.o\
 sos_mail.o\
 sos_mail_java.o\
 sosalloc.o\
 sosarray.o\
 sosbeep.o\
 sosblock.o\
 sosclien.o\
 sosctype.o\
 sosdate.o\
 sosdll.o\
 sosdde.o\
 sosdumcl.o\
 soserror.o\
 sosfact.o\
 sosfiltr.o\
 sosfield.o\
 sosfld2.o\
 sosfunc.o\
 soslimt2.o\
 soslimtx.o\
 soslist.o\
 sosmsg.o\
 sosobj.o\
 sosobjba.o\
 sosopt.o\
 sosprof.o\
 sosprog.o\
 sosscrpt.o\
 sosset.o\
 sossock.o\
 sossql1.o\
 sossql2.o\
 sossql2a.o\
 sossql3.o\
 sossql4.o\
 sossqlfl.o\
 sossqlfn.o\
 sossqlty.o\
 sossql_group_by.o\
 sosstat.o\
 sosstrea.o\
 sosstrg0.o\
 sosstrng.o\
 sosmswin.o\
 sossv.o\
 sossv2.o\
 sostimer.o\
 sosuser.o\
 soswin.o\
 sosxml.o\
 sqlutil.o\
 sqlfield.o\
 stdfield.o\
 strlwr.o\
 strupr.o\
 subrange.o\
 svstring.o\
 sysdep.o\
 sysxcept.o\
 tabucase.o\
 thread_semaphore.o\
 truncsp.o\
 type.o\
 typereg.o\
 xception.o\
 xlat.o\
 xlatgerm.o\


#### java_classes=\
#### sos/mail/Message.class


include $(PROD_DIR)/make/standard.makefile


CFLAGS += -fno-strict-aliasing
CPPFLAGS += -Wno-deprecated -Wno-reorder -Wno-long-long -Wno-sign-compare 



all:: libkram.a
all:: soswnmai.o
all:: sosmain0.o
### all:: $(BIN_DIR)/sos.mail.jar




# Message.java erzeugt mehrere .class-Dateien, für jede verschachtelte Klasse eine eigene
#### $(BIN_DIR)/sos.mail.jar: $(java_classes)
#### $(PROD_DIR)/LINKS/java/bin/jar cf $@  sos/mail/*.class

libkram.a: $(objects) soswnmai.o
	$(AR) $(ARFLAGS) $@ $(objects)


soswnmai.o: soswnmai.cxx

sosmain0.o: sosmain0.cxx
