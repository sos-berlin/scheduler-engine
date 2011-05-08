# $Id$

# libxml2 habe ich modifiziert. Also nicht die libxml2.so verwenden! 

PROD_DIR = $(shell cd ../../.. && pwd)


objects = \
 DOCBparser.o\
 HTMLparser.o\
 HTMLtree.o\
 SAX.o\
 SAX2.o\
 c14n.o\
 catalog.o\
 chvalid.o\
 debugXML.o\
 dict.o\
 encoding.o\
 entities.o\
 error.o\
 globals.o\
 hash.o\
 legacy.o\
 list.o\
 nanoftp.o\
 nanohttp.o\
 parser.o\
 parserInternals.o\
 pattern.o\
 relaxng.o\
 threads.o\
 tree.o\
 uri.o\
 valid.o\
 xinclude.o\
 xlink.o\
 xmlIO.o\
 xmlmemory.o\
 xmlreader.o\
 xmlregexp.o\
 xmlsave.o\
 xmlschemas.o\
 xmlschemastypes.o\
 xmlstring.o\
 xmlunicode.o\
 xmlwriter.o\
 xpath.o\
 xpointer.o


include $(PROD_DIR)/make/standard.makefile

C_INCLUDE_PATH += :../include
CFLAGS += -fno-strict-aliasing



all: libxml2.a


libxml2.a: $(objects)
	$(AR) $(ARFLAGS) $@ $(objects)
