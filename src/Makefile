include ../include/cosm/Makefile.cosm
CC_FLAGS = -I../include $(CCFLAGS)
FC_FLAGS = -I../include $(FCFLAGS)
LD_FLAGS = -L../lib $(LDFLAGS)
LD_LIBS = -lCosm $(LDLIBS)

SRC_CPUOS = os_file.c os_io.c os_math.c os_mem.c os_net.c os_task.c
HEAD_CPUOS = ../include/cosm/cosm.h ../include/cosm/cputypes.h \
  ../include/cosm/os_file.h ../include/cosm/os_io.h \
  ../include/cosm/os_math.h ../include/cosm/os_mem.h \
  ../include/cosm/os_net.h ../include/cosm/os_task.h
OBJ_CPUOS = os_file.$(OBJ) os_io.$(OBJ) os_math.$(OBJ) os_mem.$(OBJ) \
  os_net.$(OBJ) os_task.$(OBJ)

SRC_UTIL = bignum.c buffer.c config.c cosmtest.c email.c hashtable.c http.c \
  language.c log.c security.c time.c transform.c
HEAD_UTIL = ../include/cosm/bignum.h ../include/cosm/buffer.h \
  ../include/cosm/config.h ../include/cosm/email.h \
  ../include/cosm/hashtable.h ../include/cosm/http.h \
  ../include/cosm/language.h ../include/cosm/log.h \
  ../include/cosm/security.h ../include/cosm/time.h \
  ../include/cosm/transform.h
OBJ_UTIL = bignum.$(OBJ) buffer.$(OBJ) config.$(OBJ) cosmtest.$(OBJ) \
  email.$(OBJ) hashtable.$(OBJ) http.$(OBJ) language.$(OBJ) log.$(OBJ) \
  security.$(OBJ) time.$(OBJ) transform.$(OBJ)

SOURCES = $(SRC_CPUOS) $(SRC_UTIL)
HEADERS = $(HEAD_CPUOS) $(HEAD_UTIL)
OBJECTS = $(OBJ_CPUOS) $(OBJ_UTIL) test_dl.$(OBJ)

SUBDIRS = bzip2
SUBDEPS = $(SUBDIRS:%=%/*.c) $(SUBDIRS:%=%/*.h)

LIBRARIES = ../lib/libCosm.a

BINARIES = testlib.exe test_dl.dylib

all: $(LIBRARIES) $(BINARIES)

../lib/libCosm.a: $(OBJECTS) sub-dirs
	@echo Creating library $@
	@$(RM) $@
	@$(AR)$@ $(OBJECTS)
	@for dir in $(SUBDIRS); do \
          echo "Adding $$dir/ to library"; \
	  $(AR)$@ $$dir/*.$(OBJ); \
	done
	@-$(RNLIB) $@

sub-dirs:
	@for dir in $(SUBDIRS); do \
          echo "-- Entering subdirectory $$dir"; \
	  $(MAKE) -s -C $$dir; \
	done

test_dl.dylib: test_dl.$(OBJ)
	@echo Linking $@
	@$(DYLIB)$@ $(LD_FLAGS) test_dl.$(OBJ)

testlib.exe: testlib.$(OBJ) $(LIBRARIES)
	@echo Linking $@
	@$(LD)$@ $(LD_FLAGS) testlib.$(OBJ) $(LD_LIBS)
	@echo Striping $@
	@$(STRIP) $@

install: $(LIBRARIES) install-header install-lib

install-header: ../include/cosm/Makefile.cosm
	mkdir -p $(COSM_INC)
	cp -r ../include/ $(COSM_INC)
	chmod -R 644 $(COSM_INC)/cosm

install-lib: $(LIBRARIES)
	mkdir -p $(COSM_LIB)
	cp $(LIBRARIES) $(COSM_LIB)
	chmod 644 $(COSM_LIB)/*.a

clean:
	@echo Cleaning...
	-@$(RM) $(BINARIES)
	-@$(RM) $(LIBRARIES)
	-@$(RM) $(OBJECTS) testlib.$(OBJ)
	@for dir in $(SUBDIRS); do \
          echo "-- Entering subdirectory $$dir"; \
	  $(MAKE) -C $$dir clean; \
	done

os_file.o: os_file.c ../include/cosm/os_file.h ../include/cosm/cputypes.h \
  ../include/cosm/os_task.h ../include/cosm/os_math.h \
  ../include/cosm/os_mem.h ../include/cosm/os_io.h
os_io.o: os_io.c ../include/cosm/os_mem.h ../include/cosm/cputypes.h \
  ../include/cosm/os_io.h ../include/cosm/os_math.h \
  ../include/cosm/os_file.h ../include/cosm/os_task.h
os_math.o: os_math.c ../include/cosm/os_math.h ../include/cosm/cputypes.h
os_mem.o: os_mem.c ../include/cosm/cputypes.h ../include/cosm/os_mem.h \
  ../include/cosm/cputypes.h ../include/cosm/os_math.h \
  ../include/cosm/os_io.h ../include/cosm/os_file.h \
  ../include/cosm/os_task.h
os_net.o: os_net.c ../include/cosm/os_net.h ../include/cosm/cputypes.h \
  ../include/cosm/os_task.h ../include/cosm/os_math.h \
  ../include/cosm/os_mem.h ../include/cosm/os_io.h \
  ../include/cosm/os_file.h
os_task.o: os_task.c ../include/cosm/os_task.h ../include/cosm/cputypes.h \
  ../include/cosm/os_math.h ../include/cosm/os_file.h \
  ../include/cosm/os_io.h ../include/cosm/os_mem.h \
  ../include/cosm/os_net.h

bignum.o: bignum.c ../include/cosm/bignum.h ../include/cosm/cputypes.h \
  ../include/cosm/os_mem.h ../include/cosm/os_file.h \
  ../include/cosm/os_task.h ../include/cosm/os_math.h \
  ../include/cosm/os_io.h ../include/cosm/buffer.h \
  ../include/cosm/security.h ../include/cosm/transform.h
buffer.o: buffer.c ../include/cosm/buffer.h ../include/cosm/cputypes.h \
  ../include/cosm/os_task.h ../include/cosm/os_math.h \
  ../include/cosm/os_mem.h
config.o: config.c ../include/cosm/config.h ../include/cosm/cputypes.h \
  ../include/cosm/os_task.h ../include/cosm/os_math.h \
  ../include/cosm/os_io.h ../include/cosm/os_file.h \
  ../include/cosm/os_mem.h
cosmtest.o: cosmtest.c ../include/cosm/cosm.h ../include/cosm/cputypes.h \
  ../include/cosm/os_file.h ../include/cosm/os_task.h \
  ../include/cosm/os_math.h ../include/cosm/os_io.h \
  ../include/cosm/os_mem.h ../include/cosm/os_net.h \
  ../include/cosm/bignum.h ../include/cosm/buffer.h \
  ../include/cosm/config.h ../include/cosm/email.h \
  ../include/cosm/hashtable.h ../include/cosm/http.h \
  ../include/cosm/log.h ../include/cosm/language.h \
  ../include/cosm/security.h ../include/cosm/transform.h \
  ../include/cosm/time.h
email.o: email.c ../include/cosm/email.h ../include/cosm/cputypes.h \
  ../include/cosm/os_net.h ../include/cosm/os_task.h \
  ../include/cosm/os_math.h ../include/cosm/os_io.h \
  ../include/cosm/os_file.h ../include/cosm/os_mem.h
hashtable.o: hashtable.c ../include/cosm/hashtable.h \
  ../include/cosm/cputypes.h ../include/cosm/os_mem.h
http.o: http.c ../include/cosm/http.h ../include/cosm/cputypes.h \
  ../include/cosm/os_net.h ../include/cosm/os_task.h \
  ../include/cosm/os_math.h ../include/cosm/buffer.h \
  ../include/cosm/log.h ../include/cosm/os_file.h \
  ../include/cosm/os_mem.h ../include/cosm/os_io.h \
  ../include/cosm/transform.h
language.o: language.c ../include/cosm/language.h \
  ../include/cosm/cputypes.h ../include/cosm/os_file.h \
  ../include/cosm/os_task.h ../include/cosm/os_math.h
log.o: log.c ../include/cosm/log.h ../include/cosm/cputypes.h \
  ../include/cosm/os_file.h ../include/cosm/os_task.h \
  ../include/cosm/os_math.h ../include/cosm/os_io.h \
  ../include/cosm/os_mem.h
security.o: security.c ../include/cosm/security.h \
  ../include/cosm/cputypes.h ../include/cosm/os_task.h \
  ../include/cosm/os_math.h ../include/cosm/buffer.h \
  ../include/cosm/transform.h ../include/cosm/bignum.h \
  ../include/cosm/os_mem.h ../include/cosm/os_file.h \
  ../include/cosm/time.h ../include/cosm/os_io.h
time.o: time.c ../include/cosm/os_math.h ../include/cosm/cputypes.h \
  ../include/cosm/time.h ../include/cosm/os_task.h \
  ../include/cosm/os_file.h ../include/cosm/os_net.h
transform.o: transform.c ../include/cosm/transform.h \
  ../include/cosm/cputypes.h ../include/cosm/os_mem.h \
  ../include/cosm/os_net.h ../include/cosm/os_task.h \
  ../include/cosm/os_math.h ../include/cosm/os_file.h \
  ../include/cosm/buffer.h

test_dl.o: test_dl.c ../include/cosm/cputypes.h

testlib.o: testlib.c ../include/cosm/cosm.h ../include/cosm/cputypes.h \
  ../include/cosm/os_file.h ../include/cosm/os_task.h \
  ../include/cosm/os_math.h ../include/cosm/os_io.h \
  ../include/cosm/os_mem.h ../include/cosm/os_net.h \
  ../include/cosm/bignum.h ../include/cosm/buffer.h \
  ../include/cosm/config.h ../include/cosm/email.h \
  ../include/cosm/hashtable.h ../include/cosm/http.h \
  ../include/cosm/log.h ../include/cosm/language.h \
  ../include/cosm/security.h ../include/cosm/transform.h \
  ../include/cosm/time.h
