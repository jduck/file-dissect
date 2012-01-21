CPP = g++
CPPFLAGS = -ggdb -fPIC -Wall -Wextra `wx-config --cflags`
FDPATH = ../../../../..
CBFFPATH = ../..
INCLUDE = -I$(FDPATH)/fileDissect -I$(FDPATH)/libfileDissect -I$(FDPATH)/libfileDissect/wxFileMap -I$(FDPATH)/wxHexView -I$(FDPATH)/wxPluginLoader \
	-I$(CBFFPATH)
LDFLAGS = -L$(FDPATH)/bin/plugins -lcbff `wx-config --libs`

BINDIR = $(FDPATH)/bin/plugins/cbff


SI = $(BINDIR)/excel.so
SI_OBJS = \
	Workbook.o


BINS = $(SI)


all: bindir $(BINS)

bindir:
	if test \! -d $(BINDIR); then mkdir $(BINDIR); fi


$(SI): $(SI_OBJS)
	$(CPP) $(CPPFLAGS) -fpic -shared -o $@ -Wl,-soname,CBFF_SONAME $^ $(LDFLAGS)


clean:
	rm -f $(SI_OBJS) $(BINS)


.cpp.o:
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<
