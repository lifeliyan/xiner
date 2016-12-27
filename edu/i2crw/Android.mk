LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)
LOCAL_SRC_FILES:= i2crw.c
LOCAL_MODULE := i2crw 
LOCAL_SHARED_LIBRARIES:= libcutils libutils 
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

