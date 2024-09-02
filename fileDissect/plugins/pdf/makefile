CPP = g++
CPPFLAGS = -ggdb -fPIC -Wall -Wextra `wx-config --cflags`
FDPATH = ../../..
INCLUDE = -I$(FDPATH)/fileDissect -I$(FDPATH)/wxHexView -I$(FDPATH)/wxFileMap -I$(FDPATH)/wxPluginLoader
LDFLAGS = `wx-config --libs`

BINDIR = $(FDPATH)/bin/plugins


PDF = $(BINDIR)/pdf.so
BINS = $(PDF)
PDF_OBJS = \
	pdf.o \
	wxFileMap.o


all: plugins-dir $(BINS)


$(PDF): $(PDF_OBJS)
	$(CPP) $(CPPFLAGS) -fpic -shared -o $@ -Wl,-soname,PDF_SONAME $^ $(LDFLAGS)


plugins-dir:
	make -C plugins


plugins-clean:
	make -C plugins clean

clean: plugins-clean
	rm -f $(PDF_OBJS) $(BINS)
	rm -f fileDissectSel.cpp wxFileMap.cpp


.cpp.o:
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<


fileDissectSel.o: fileDissectSel.cpp
fileDissectSel.cpp:
	ln -s $(FDPATH)/fileDissect/fileDissectSel.cpp


wxHexView.o: wxHexView.cpp
wxHexView.cpp:
	ln -s $(FDPATH)/wxHexView/wxHexView.cpp

wxFileMap.o: wxFileMap.cpp
wxFileMap.cpp:
	ln -s $(FDPATH)/wxFileMap/wxFileMap.cpp
