COSM = ../..
include $(COSM)/include/cosm/Makefile.cosm
CC_FLAGS = $(CCFLAGS) -I$(COSM)/include -L.
LD_FLAGS = $(LDFLAGS) -L$(COSM)/lib -L.
LD_LIBS =  $(LDLIBS) -lCosmUtil

SOURCES = pki-tool.c
HEADERS = 
OBJECTS = pki-tool.$(OBJ)

BINARIES = pki-tool.exe

all: $(BINARIES)

pki-tool.exe: pki-tool.$(OBJ)
	@echo Linking $@
	@$(LD)$@ $(LD_FLAGS) $(OBJECTS) $(LD_LIBS)
	@echo Striping $@
	@$(STRIP) $@

clean:
	@echo Cleaning...
	-@$(RM) $(BINARIES) $(OBJECTS)

pki-tool.$(OBJ): pki-tool.c
