CPP = g++
CPPFLAGS = -ggdb -Wall -Wextra `wx-config --cflags`

PATHREL = ..
INCLUDE = -I$(PATHREL)/fileDissect -I$(PATHREL)/libfileDissect -I$(PATHREL)/libfileDissect/wxFileMap -I$(PATHREL)/wxHexView -I$(PATHREL)/wxPluginLoader
BINDIR = $(PATHREL)/bin
LDFLAGS = -L$(BINDIR) -lfileDissect `wx-config --libs` -ldl

FD = $(BINDIR)/fd
BINS = $(FD)
FD_OBJS = \
	fileDissectApp.o \
	fileDissectFrame.o \
	fileDissectDnD.o \
	fileDissectFmts.o \
	fileDissectTree.o \
	$(PATHREL)/wxHexView/wxHexView.o

all: bindir libfileDissect $(BINS) plugins-dir

bindir:
	if test \! -d $(BINDIR); then mkdir $(BINDIR); fi


$(FD): $(FD_OBJS)
	$(CPP) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)


libfileDissect:
	make -C $(PATHREL)/libfileDissect

libfileDissect-clean:
	make -C $(PATHREL)/libfileDissect clean


plugins-dir:
	make -C plugins


plugins-clean:
	make -C plugins clean

clean: libfileDissect-clean plugins-clean
	rm -f $(BINS) $(FD_OBJS)
	rm -f $(BINDIR)/.gdb_history
	if test -d $(BINDIR); then rmdir $(BINDIR); fi


.cpp.o:
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<

