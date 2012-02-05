CPP = g++
CPPFLAGS = -ggdb -fPIC -Wall -Wextra `wx-config --cflags`
FDPATH = ../../..
INCLUDE = -I$(FDPATH)/fileDissect -I$(FDPATH)/libfileDissect -I$(FDPATH)/libfileDissect/wxFileMap -I$(FDPATH)/wxHexView -I$(FDPATH)/wxPluginLoader
LDFLAGS = `wx-config --libs` -ldl

BINDIR = $(FDPATH)/bin/plugins


CBFF = $(BINDIR)/cbff.so
BINS = $(CBFF) $(BINDIR)/libcbff.so
CBFF_OBJS = \
	cbff.o \
	cbffStream.o \
	cbffStreamPlugins.o


all: $(BINS) plugins-dir


$(CBFF): $(CBFF_OBJS)
	$(CPP) $(CPPFLAGS) -fpic -shared -o $@ -Wl,-soname,CBFF_SONAME $^ $(LDFLAGS)

$(BINDIR)/libcbff.so:
	cd $(BINDIR) && ln -s cbff.so libcbff.so

plugins-dir:
	make -C plugins


plugins-clean:
	make -C plugins clean

clean: plugins-clean
	rm -f $(CBFF_OBJS) $(BINS)


.cpp.o:
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<
