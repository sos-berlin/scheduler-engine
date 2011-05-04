# $Id$

PROD_DIR = $(shell cd ../../.. && pwd)

objects = rtf.o rtf_doc.o rtf_parser.o


include $(PROD_DIR)/make/standard.makefile


all: librtf.a


librtf.a: $(objects)
	$(AR) $(ARFLAGS) $@ $(objects)

