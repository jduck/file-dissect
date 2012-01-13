#
# fd plugins dir makefile for GCC/G++
#

DIRS = cbff pdf
FDPATH = ../..
BINDIR = $(FDPATH)/bin/plugins

all: plugins-dir dirs

plugins-dir:
	if test \! -d $(BINDIR); then mkdir $(BINDIR); fi


dirs: $(DIRS)
	for ii in $(DIRS); do \
		make -C $$ii; \
	done

clean: clean-dirs clean-plugins-dir

clean-dirs:
	for ii in $(DIRS); do \
		make -C $$ii clean; \
	done

clean-plugins-dir:
	if test -d $(BINDIR); then rmdir $(BINDIR); fi
