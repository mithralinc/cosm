COSM = ../..
include $(COSM)/include/cosm/Makefile.cosm
CC_FLAGS = $(CCFLAGS) -I$(COSM)/include -L.
LD_FLAGS = $(LDFLAGS) -L$(COSM)/lib -L.
LD_LIBS =  $(LDLIBS) -lCosmUtil

SOURCES = client.c server.c
HEADERS = client.h server.h common.h
OBJECTS = client.$(OBJ) server.$(OBJ)

BINARIES = client.exe server.exe

all: $(BINARIES)

client.exe: client.$(OBJ)
	@echo Linking $@
	@$(LD)$@ $(LD_FLAGS) client.$(OBJ) $(LD_LIBS)
	@echo Striping $@
	@$(STRIP) $@

server.exe: server.$(OBJ)
	@echo Linking $@
	@$(LD)$@ $(LD_FLAGS) server.$(OBJ) $(LD_LIBS)
	@echo Striping $@
	@$(STRIP) $@

clean:
	-$(RM) $(BINARIES) $(OBJECTS) *.log *.cfg

client.$(OBJ): client.c client.h common.h
server.$(OBJ): server.c server.h common.h
