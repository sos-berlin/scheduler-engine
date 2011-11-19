# $Id: Makefile 11347 2005-03-24 10:14:54Z jz $

PROD_DIR = $(shell cd ../../.. && pwd)

objects = rtf.o rtf_doc.o rtf_parser.o


include $(PROD_DIR)/make/standard.makefile


all: librtf.a


librtf.a: $(objects)
	$(AR) $(ARFLAGS) $@ $(objects)

