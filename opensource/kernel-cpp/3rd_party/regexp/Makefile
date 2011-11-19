# $Id: Makefile 11347 2005-03-24 10:14:54Z jz $

ifndef PROD_DIR
prod_dir = ../..
include $(prod_dir)/make/base.makefile
else

objects = regcomp.o regerror.o regexec.o regfree.o split.o


all::
#all: libregexp.a

clean:
	rm *.o *.d lib*.a lib*.so *.map

include $(PROD_DIR)/make/standard.makefile


libregexp.a: $(objects)
	$(AR) $(ARFLAGS) $@ $(objects)

endif
