# $Id: Makefile 3482 2005-03-24 10:14:54Z jz $

PROD_DIR = $(shell cd ../../../.. && pwd)


objects = \
 attributes.o\
 attrvt.o\
 documents.o\
 extensions.o\
 extra.o\
 functions.o\
 imports.o\
 keys.o\
 namespaces.o\
 numbers.o\
 pattern.o\
 preproc.o\
 security.o\
 templates.o\
 transform.o\
 variables.o\
 xslt.o\
 xsltutils.o 


C_INCLUDE_PATH += :..

include $(PROD_DIR)/make/standard.makefile


all: libxslt.a


libxslt.a: $(objects)
	$(AR) $(ARFLAGS) $@ $(objects)
