COSM = ../..
include $(COSM)/include/cosm/Makefile.cosm
CC_FLAGS = $(CCFLAGS) -I$(COSM)/include -L.
LD_FLAGS = $(LDFLAGS) -L$(COSM)/lib -L.
LD_LIBS =  $(LDLIBS) -lCosmUtil

SOURCES = cssdk-engine.c cssdk-client.c cssdk.c
HEADERS = cssdk.h
OBJECTS = cssdk-engine.$(OBJ) cssdk-client.$(OBJ) cssdk.$(OBJ)

BINARIES = cssdk-engine.dylib cssdk-client.exe

all: $(BINARIES)

cssdk-engine.dylib: cssdk-engine.$(OBJ) cssdk.$(OBJ)
	@echo Linking $@
	@$(DYLIB)$@ $(LD_FLAGS) cssdk-engine.$(OBJ) cssdk.$(OBJ) $(LD_LIBS)

cssdk-client.exe: cssdk-client.$(OBJ)
	@echo Linking $@
	@$(LD)$@ $(LD_FLAGS) cssdk-client.$(OBJ) cssdk.$(OBJ) $(LD_LIBS)
	@echo Striping $@
	@$(STRIP) $@

clean:
	@echo Cleaning...
	-@$(RM) $(BINARIES) $(OBJECTS)

cssdk-engine.$(OBJ): cssdk-engine.c cssdk.c cssdk.h
cssdk-client.$(OBJ): cssdk-client.c cssdk.h
cssdk.$(OBJ): cssdk.c cssdk.h
