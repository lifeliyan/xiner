LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)
LOCAL_C_INCLUDES:= external/edu/include
LOCAL_SRC_FILES:= main.c uart.c
LOCAL_MODULE := dlp_uart_ctl
LOCAL_SHARED_LIBRARIES:= libcutils libutils libtinyalsa
LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

