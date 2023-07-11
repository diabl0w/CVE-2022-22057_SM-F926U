LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE            := ssl
LOCAL_SRC_FILES         := libssl.a
LOCAL_EXPORT_C_INCLUDES := include 
include $(PREBUILT_STATIC_LIBRARY)

	include $(CLEAR_VARS)
	LOCAL_MODULE := liblog
	LOCAL_SRC_FILES := liblog.so
	include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libtimeline
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := starter.cpp misc.cpp
#LOCAL_CXX :=/mnt/e/linux/ollvm/build/bin/clang++
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_STATIC_LIBRARIES := ssl
LOCAL_CFLAGS += -Wno-unused-variable -Wno-unused-parameter -fexceptions
#LOCAL_CFLAGS += -mllvm -sub -mllvm -fla -mllvm -bcf
LOCAL_LDFLAGS += -Wl,--export-dynamic
LOCAL_C_INCLUDES += frameworks/native/include system/core/include
LOCAL_C_INCLUDES += $(NDK_PROJECT_PATH)/include
include $(BUILD_EXECUTABLE)
