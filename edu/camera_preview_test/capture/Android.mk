#  ldswfun for csi camera test
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#LOCAL_SRC_FILES := camera_preview.c
LOCAL_SRC_FILES := captrue.c
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_MODULE := captrue

include $(BUILD_EXECUTABLE)
