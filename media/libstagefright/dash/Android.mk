LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#        DashDataSource.cpp      \
        DashSession.cpp         \


LOCAL_SRC_FILES:=               \
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
	../foundation/AString.cpp \
        main_testparser.cpp

LOCAL_C_INCLUDES:= \
	$(TOP)/frameworks/av/include/media/stagefright/foundation \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/media/libstagefright \
	$(TOP)/frameworks/av/media/libstagefright/include \
	$(TOP)/frameworks/native/include \
	$(TOP)/frameworks/native/include/media/openmax \
	$(TOP)/external/libxml2/include                \
	$(TOP)/external/icu4c/common

LOCAL_MODULE:= libstagefright_dash_test

LOCAL_SHARED_LIBRARIES := \
	libicuuc \
	liblog

LOCAL_STATIC_LIBRARIES := \
	libutils

LOCAL_CFLAGS := -O0 -g
LOCAL_LDFLAGS := -O0 -g
LOCAL_LDLIBS := -L/usr/lib/i386-linux-gnu -lxml2 /lib/i386-linux-gnu/liblzma.so.5.0.0 -lz -lstdc++ -ldl


include $(BUILD_HOST_EXECUTABLE)
