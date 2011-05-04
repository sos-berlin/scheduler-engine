products = kram e370 file fs rapid sosingre #foxmail winedit sab 

.PHONY: $(products)

ifeq ($(HOSTTYPE),i586)
    SYSTEM_LINUX = 1
    SYSTEM       = linux
 else
    SYSTEM_SOLARIS = 1
    SYSTEM         = solaris
endif

export

all: $(products)


$(products):
	@echo \*\*\*\*\* $@
	( cd sys/$(SYSTEM)/lib; ../../../sosmake $@ )

#@$(MAKE) --no-print-directory -C sys/$(SYSTEM)/lib -f ../../../makefile.std.gnumake
