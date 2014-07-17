LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
        DashDataSource.cpp      \
        DashSession.cpp         \
        MPDParser.cpp

LOCAL_C_INCLUDES:= \
	$(TOP)/frameworks/native/include/utils \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/media/libstagefright \
	$(TOP)/frameworks/native/include/media/openmax \
	$(TOP)/external/openssl/include \
	$(TOP)/external/libxml2/include \
	$(TOP)/external/icu4c/common \
	$(TOP)/external/stlport/stlport \
	$(TOP)/bionic \
	$(TOP)/bionic/libstdc++/include

LOCAL_MODULE:= libstagefright_dash

ifeq ($(TARGET_ARCH),arm)
    LOCAL_CFLAGS += -Wno-psabi
endif

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
        MPDParser.cpp           \
        dash_test_main.cpp

LOCAL_C_INCLUDES:= \
	$(TOP)/frameworks/av/media/libstagefright \
	$(TOP)/frameworks/native/include/media/openmax

LOCAL_MODULE:= libstagefright_dash_test

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libicuuc \
	liblog \
	libcutils \
	libstlport \
	libstdc++ \
	libdl

LOCAL_STATIC_LIBRARIES := \
	libxml2

include $(BUILD_HOST_EXECUTABLE)

