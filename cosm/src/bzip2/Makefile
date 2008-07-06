include ../../Makefile.cosm
CC_FLAGS = -I../../include $(CCFLAGS)
LD_FLAGS = -L.. $(LDFLAGS)

SOURCES = blocksort.c bzlib.c crctable.c decompress.c huffman.c \
  randtable.c bzip2.c
HEADERS = bzip2.h
OBJECTS = blocksort.$(OBJ) bzlib.$(OBJ) compress.$(OBJ) crctable.$(OBJ) \
  decompress.$(OBJ) huffman.$(OBJ) randtable.$(OBJ) bzip2.$(OBJ)

NAME = bzip2

all: $(OBJECTS)

clean:
	@echo Cleaning...
	-@$(RM) $(OBJECTS) $(CLEAN_SUFFIX)

blocksort.$(OBJ): blocksort.c bzlib_private.h bzlib.h
bzlib.$(OBJ): bzlib.c bzlib_private.h bzlib.h
compress.$(OBJ): compress.c bzlib_private.h bzlib.h
crctable.$(OBJ): crctable.c bzlib_private.h bzlib.h
decompress.$(OBJ): decompress.c bzlib_private.h bzlib.h
huffman.$(OBJ): huffman.c bzlib_private.h bzlib.h
randtable.$(OBJ): randtable.c bzlib_private.h bzlib.h

bzip2.$(OBJ): bzip2.c ../../include/cosm/bzip2/bzip2.h \
  ../../include/cosm/transform.h ../../include/cosm/cputypes.h bzlib.h \
  ../../include/cosm/os_mem.h

