# Expects to be included from the jni/Android.mk file.
# Defines COSM_C_INCLUDES based on the current path and ABI.

include $(CLEAR_VARS)
COSM_C_INCLUDES := $(LOCAL_PATH)/cosm/include
LOCAL_MODULE := Cosm
LOCAL_C_INCLUDES := $(LOCAL_PATH)/cosm/include
LOCAL_SRC_FILES := cosm/src/bignum.c \
  cosm/src/buffer.c \
  cosm/src/config.c \
  cosm/src/cosmtest.c \
  cosm/src/email.c \
  cosm/src/hashtable.c\
  cosm/src/http.c \
  cosm/src/language.c \
  cosm/src/log.c \
  cosm/src/os_file.c \
  cosm/src/os_io.c \
  cosm/src/os_math.c \
  cosm/src/os_mem.c \
  cosm/src/os_net.c \
  cosm/src/os_task.c \
  cosm/src/security.c\
  cosm/src/time.c \
  cosm/src/transform.c \
  cosm/src/bzip2/blocksort.c \
  cosm/src/bzip2/bzip2.c \
  cosm/src/bzip2/bzlib.c \
  cosm/src/bzip2/compress.c \
  cosm/src/bzip2/crctable.c \
  cosm/src/bzip2/decompress.c \
  cosm/src/bzip2/huffman.c \
  cosm/src/bzip2/randtable.c
include $(BUILD_STATIC_LIBRARY)
