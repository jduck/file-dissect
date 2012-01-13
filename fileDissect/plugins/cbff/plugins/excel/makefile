CPP = g++
CPPFLAGS = -ggdb -fPIC -Wall -Wextra `wx-config --cflags`
FDPATH = ../../../../..
CBFFPATH = ../..
INCLUDE = -I$(FDPATH)/fileDissect -I$(FDPATH)/wxHexView -I$(FDPATH)/wxFileMap -I$(FDPATH)/wxPluginLoader \
	-I$(CBFFPATH)
LDFLAGS = `wx-config --libs`

BINDIR = $(FDPATH)/bin/plugins/cbff


SI = $(BINDIR)/excel.so
SI_OBJS = \
	Workbook.o \
	fileDissectSel.o \
	cbffStream.o


BINS = $(SI)


all: bindir $(BINS)

bindir:
	if test \! -d $(BINDIR); then mkdir $(BINDIR); fi


$(SI): $(SI_OBJS)
	$(CPP) $(CPPFLAGS) -fpic -shared -o $@ -Wl,-soname,CBFF_SONAME $^ $(LDFLAGS)


clean:
	rm -f $(SI_OBJS) $(BINS)
	rm -f fileDissectSel.cpp cbffStream.cpp


.cpp.o:
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<


fileDissectSel.o: fileDissectSel.cpp
fileDissectSel.cpp:
	ln -s $(FDPATH)/fileDissect/fileDissectSel.cpp

cbffStream.o: cbffStream.cpp
cbffStream.cpp:
	ln -s $(CBFFPATH)/cbffStream.cpp
