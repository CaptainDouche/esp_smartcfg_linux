DEFS		= -D_LINUX -D__RASPI__ -D__USE_POSIX199309 -D_BSD_SOURCE -D_POSIX_C_SOURCE=199309L
CC       	= gcc
LINKER   	= $(CC) -o

MYSQL_CFLAGS = 
MYSQL_LFLAGS = 

CFLAGS   	= -O2 -x c -std=c99 -Wall $(DEFS)
LFLAGS   	= -Wall -lm -lrt $(MYSQL_LFLAGS)

# change these to set the proper directories where each files should be
SRCDIR     = .
OBJDIR     = ./bin
BINDIR     = ./bin
INSTALLDIR = /bin/
TARGET := smartcfg

GLOBCFGS := defs.h
SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

all: $(BINDIR)/$(TARGET)
	@echo "$(BINDIR)/$(TARGET): done"

install:
	mkdir -p $(INSTALLDIR)
	cp $(BINDIR)/$(TARGET) $(INSTALLDIR)
	chmod a+s $(INSTALLDIR)/$(TARGET)

clean:
	rm -f $(OBJECTS)	

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/%.h
	LT=$<
	AT=$@
	mkdir -p $(OBJDIR)	
	@echo "Compile $@ -> $< ..."
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compile $@ -> $< done."
	
$(BINDIR)/$(TARGET): $(OBJECTS)
	LT=$<
	AT=$@
	mkdir -p $(BINDIR)	
	@echo "Linking $(OBJECTS) -> $(BINDIR)/$(TARGET) ..."
	$(LINKER) $(BINDIR)/$(TARGET) $(LFLAGS) $(OBJECTS)
	@echo "Linking $(BINDIR)/$(TARGET) done."
