COSM = ../..
include $(COSM)/include/cosm/Makefile.cosm
CC_FLAGS = $(CCFLAGS) -I$(COSM)/include -L.
LD_FLAGS = $(LDFLAGS) -L$(COSM)/lib -L.
LD_LIBS =  $(LDLIBS) -lCosmUtil

SOURCES = rsa-tool.c
HEADERS = 
OBJECTS = rsa-tool.$(OBJ)

BINARIES = rsa-tool.exe

all: $(BINARIES)

rsa-tool.exe: rsa-tool.$(OBJ)
	@echo Linking $@
	@$(LD)$@ $(LD_FLAGS) $(OBJECTS) $(LD_LIBS)
	@echo Striping $@
	@$(STRIP) $@

clean:
	@echo Cleaning...
	-@$(RM) $(BINARIES) $(OBJECTS)

rsa-tool.$(OBJ): rsa-tool.c
