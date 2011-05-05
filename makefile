#
# top level makefile for GCC/G++
#

CPP = g++

DIRS = fileDissect

all: dirs

dirs: $(DIRS)
	for ii in $(DIRS); do \
		make -C $$ii; \
	done

clean: clean-dirs

clean-dirs:
	for ii in $(DIRS); do \
		make -C $$ii clean; \
	done
