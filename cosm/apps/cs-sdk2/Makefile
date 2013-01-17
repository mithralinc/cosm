COSM = ../..
include $(COSM)/include/cosm/Makefile.cosm
CC_FLAGS = $(CCFLAGS) -I$(COSM)/include -I.
LD_FLAGS = $(LDFLAGS) -L$(COSM)/lib -L.
LD_LIBS =  $(LDLIBS) -lCosm

SOURCES = cssdk-engine.c cssdk-client.c cssdk-server.c cssdk.c PostgreSQL.c
HEADERS = cssdk.h PostgreSQL.h
OBJECTS = cssdk-engine.$(OBJ) cssdk-client.$(OBJ) cssdk-server.$(OBJ) \
          cssdk.$(OBJ) PostgreSQL.$(OBJ)

BINARIES = cssdk-engine.dylib cssdk-client.exe cssdk-server.exe

all: $(BINARIES)

cssdk-engine.dylib: cssdk-engine.$(OBJ) cssdk.$(OBJ)
	@echo Linking $@
	@$(DYLIB)$@ $(LD_FLAGS) cssdk-engine.$(OBJ) cssdk.$(OBJ) $(LD_LIBS)

cssdk-client.exe: cssdk-client.$(OBJ)
	@echo Linking $@
	@$(LD)$@ $(LD_FLAGS) cssdk-client.$(OBJ) cssdk.$(OBJ) $(LD_LIBS)
	@echo Striping $@
	@$(STRIP) $@

cssdk-server.exe: cssdk-server.$(OBJ) PostgreSQL.$(OBJ)
	@echo Linking $@
	@$(LD)$@ $(LD_FLAGS) -lpq cssdk-server.$(OBJ) \
	  PostgreSQL.$(OBJ) cssdk.$(OBJ) $(LD_LIBS)
	@echo Striping $@
	@$(STRIP) $@
clean:
	@echo Cleaning...
	-@$(RM) $(BINARIES) $(OBJECTS)

cssdk-engine.$(OBJ): cssdk-engine.c cssdk.c cssdk.h
cssdk-client.$(OBJ): cssdk-client.c cssdk.h
PostgreSQL.$(OBJ): PostgreSQL.c PostgreSQL.h
cssdk-server.$(OBJ): cssdk-server.c cssdk.h
cssdk.$(OBJ): cssdk.c cssdk.h
