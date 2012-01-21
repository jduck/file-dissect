CPP = g++
CPPFLAGS = -ggdb -fPIC -Wall -Wextra `wx-config --cflags`
FDPATH = ..
INCLUDE = -I$(FDPATH)/fileDissect -IwxFileMap
LDFLAGS = `wx-config --libs`

BINDIR = $(FDPATH)/bin


LIBFD_SONAME=libfileDissect.so
LIBFD = $(BINDIR)/libfileDissect.so
BINS = $(LIBFD)
LIBFD_OBJS = \
	fileDissectItemData.o \
	fileDissectSel.o \
	wxFileMap/wxFileMap.o


all: $(BINS)


$(LIBFD): $(LIBFD_OBJS)
	$(CPP) $(CPPFLAGS) -fpic -shared -o $@ -Wl,-soname,$(LIBFD_SONAME) $^ $(LDFLAGS)


clean:
	rm -f $(LIBFD_OBJS) $(BINS)


.cpp.o:
	$(CPP) $(CPPFLAGS) $(INCLUDE) -o $@ -c $<
