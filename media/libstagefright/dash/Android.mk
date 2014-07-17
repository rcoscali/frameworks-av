LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
        DashDataSource.cpp      \
        DashSession.cpp         \
        MPDParser.cpp           \

LOCAL_C_INCLUDES:= \
	$(TOP)/frameworks/av/media/libstagefright      \
	$(TOP)/frameworks/native/include/media/openmax \
	$(TOP)/external/libxml2/include                \
	$(TOP)/external/icu4c/common                   \
	$(TOP)/external/openssl/include                \
	$(TOP)/external/stlport/stlport                \
	$(TOP)/bionic                                  \
	$(TOP)/bionic/libstdc++/include

LOCAL_MODULE:= libstagefright_dash

ifeq ($(TARGET_ARCH),arm)
    LOCAL_CFLAGS += -Wno-psabi
endif

include $(BUILD_STATIC_LIBRARY)
