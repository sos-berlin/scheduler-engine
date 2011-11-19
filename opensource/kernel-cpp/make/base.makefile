# $Id: base.makefile 11940 2006-03-05 19:13:06Z jz $

.PHONY: Debug Release

export PROD_DIR=../$(prod_dir)
#export O_DIR=Release
#export O_DIR=Debug
export O_DIR=${MAKE_DIR}

ifeq ($(O_DIR),)
#ERR = $(error Umgebungsvariable MAKE_DIR auf Debug oder Release setzen!")
#O_DIR = "Umgebungsvariable_MAKE_DIR_auf_Debug_oder_Release_setzen!"
export O_DIR=Debug
endif

ifeq ($(O_DIR),Debug)
export BIN_DIR=$(PROD_DIR)/bind
else
export BIN_DIR=$(PROD_DIR)/bin
endif


all:: $(O_DIR)


# Für Joacims zwei Rechner:
dispatch_filename=$(wildcard $(prod_dir)/dispatch)
ifeq "$(dispatch_filename)" ""
jobs=1
else
export dispatch=@../$(dispatch_filename)
jobs=2
endif


Debug:
	@rm -f $(prod_dir)/.dispatch.*.pid >/dev/null
	@mkdir -p $@
	@make -j$(jobs) -C $@ --no-print-directory -f ../Makefile

Release:
	@rm -f $(prod_dir)/.dispatch.*.pid >/dev/null
	@mkdir -p $@
	@make -j$(jobs) -C $@ --no-print-directory -f ../Makefile
