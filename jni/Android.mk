LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

include $(CORE_DIR)/libretro/Makefile.common

include $(CLEAR_VARS)
LOCAL_MODULE     := retro
LOCAL_SRC_FILES  := $(SOURCES_C)
LOCAL_CFLAGS     := -O2 -DNDEBUG $(INCFLAGS) $(CORE_DEFINES) -std=gnu99 -Wno-unused-result
LOCAL_LDFLAGS    := -Wl,-version-script=$(CORE_DIR)/link.T -Wl,-Bsymbolic
LOCAL_LDLIBS     := -lm -llog
include $(BUILD_SHARED_LIBRARY)
