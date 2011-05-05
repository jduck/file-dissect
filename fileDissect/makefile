CPP = g++
CPPFLAGS = -ggdb -Wall -Wextra `wx-config --cflags`

PATHREL = ..
INCLUDE = -I$(PATHREL)/fileDissect -I$(PATHREL)/wxHexView -I$(PATHREL)/wxFileMap -I$(PATHREL)/wxPluginLoader
LDFLAGS = `wx-config --libs`
BINDIR = $(PATHREL)/bin

FD = $(BINDIR)/fd
BINS = $(FD)
FD_OBJS = \
	fileDissectApp.o \
	fileDissectFrame.o \
	fileDissectDnD.o \
	fileDissectSel.o \
	fileDissectFmts.o \
	fileDissectTree.o \
	wxHexView.o \
	wxFileMap.o
	

all: bindir $(BINS) plugins-dir

bindir:
	if test \! -d $(BINDIR); then mkdir $(BINDIR); fi


$(FD): $(FD_OBJS)
	$(CPP) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)


plugins-dir:
	make -C plugins


plugins-clean:
	make -C plugins clean

clean: plugins-clean
	rm -f $(BINS) $(FD_OBJS)
	rm -f wxFileMap.cpp wxHexView.cpp
	rm -f $(BINDIR)/.gdb_history
	if test -d $(BINDIR); then rmdir $(BINDIR); fi


.cpp.o:
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<


wxHexView.o: wxHexView.cpp
wxHexView.cpp:
	ln -s $(PATHREL)/wxHexView/wxHexView.cpp

wxFileMap.o: wxFileMap.cpp
wxFileMap.cpp:
	ln -s $(PATHREL)/wxFileMap/wxFileMap.cpp
