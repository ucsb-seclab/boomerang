LOCAL_PATH := $(call my-dir)

VERSION = $(shell git describe --always --dirty=-dev 2>/dev/null || echo Unknown)
OPTEE_CLIENT_PATH ?= $(LOCAL_PATH)/../optee_client

include $(CLEAR_VARS)
LOCAL_MODULE := teec
LOCAL_SRC_FILES := $(OPTEE_CLIENT_PATH)/libs/$(TARGET_ARCH_ABI)/libteec.so
LOCAL_EXPORT_C_INCLUDES := $(OPTEE_CLIENT_PATH)/public
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := sha-perf
LOCAL_SRC_FILES := host/sha-perf.c
LOCAL_CFLAGS := -Os -D_POSIX_C_SOURCE=200112L -DVERSION="$(VERSION)"
LOCAL_C_INCLUDES := $(LOCAL_PATH)/host $(LOCAL_PATH)/ta
LOCAL_SHARED_LIBRARIES := teec
LOCAL_LDLIBS += -lm
include $(BUILD_EXECUTABLE)
