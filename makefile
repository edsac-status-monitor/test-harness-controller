#Directories
CDIR=src
IDIR=include
ODIR=obj
BDIR=bin

#Programs
CC=gcc
RM=rm -f
MKDIR=mkdir

#Flags
LIBS=-lm `pkg-config --libs glib-2.0` -lwiringPi `xml2-config --libs` `pkg-config --libs libedsacnetworking`
CFLAGS=`xml2-config --cflags` -I$(IDIR) `pkg-config --cflags libedsacnetworking` -Werror

#Files
DEPS = $(wildcard $(IDIR)/*.h)
_SOURCES = $(wildcard $(CDIR)/*.c)
OBJ = $(patsubst $(CDIR)/%.c,$(ODIR)/%.o,$(_SOURCES))
BINARY = $(BDIR)/monitor

#Targets
build: $(BINARY)

$(ODIR)/%.o: $(CDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OBJ): | $(ODIR)

$(ODIR):
	$(MKDIR) $(ODIR)

$(BDIR)/%: $(OBJ)
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

$(BINARY): | $(BDIR)

$(BDIR):
	$(MKDIR) $(BDIR)

.PHONY: clean

clean:
	$(RM) $(ODIR)/*
	$(RM) $(BDIR)/*
	
run: build
	$(BINARY)