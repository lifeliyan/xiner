LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)
LOCAL_C_INCLUDES:= external/tinyalsa/include
LOCAL_SRC_FILES:= camsys_ctl.c
LOCAL_MODULE := camsysctl 
LOCAL_SHARED_LIBRARIES:= libcutils libutils 
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

