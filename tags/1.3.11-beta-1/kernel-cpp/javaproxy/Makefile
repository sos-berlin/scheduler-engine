# $Id: Makefile 6015 2009-06-23 11:13:12Z sos $

PROD_DIR = $(shell cd ../.. && pwd)
#INCLUDES += -I$(PROD_DIR)/LINKS

include ../javaproxy.makefile.include

objects = $(javaproxy_objects)
 
include $(PROD_DIR)/make/standard.makefile


all:: libjavaproxy.a 

libjavaproxy.a: $(objects)
	$(AR) $(ARFLAGS) $@ $^
