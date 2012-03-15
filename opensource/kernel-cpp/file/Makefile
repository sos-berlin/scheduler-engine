PROD_DIR = $(shell cd ../.. && pwd)

DEP_PRODUCTS = file fs kram zschimmer

objects = \
 absfile.o\
 alias.o \
 anyfile.o \
 filebase.o \
 filetab.o \
 flstream.o\
 jdbc.o \
 jmail_file.o\
 odbctype.o \
 sosdb.o \
 sqlfile.o\
 stdfile.o \
 zlibfl.o \

include $(PROD_DIR)/make/standard.makefile

CFLAGS += -fno-strict-aliasing
CPPFLAGS += -Wno-deprecated -Wno-reorder -Wno-long-long -Wno-sign-compare

all:: libfile.a

libfile.a: $(objects)
	$(AR) $(ARFLAGS) $@ $(objects)
