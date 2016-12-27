LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)
LOCAL_C_INCLUDES:= external/tinyalsa/include
LOCAL_SRC_FILES:= dlp_test.c
LOCAL_MODULE := dlp_i2c_ctl 
LOCAL_SHARED_LIBRARIES:= libcutils libutils 
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

