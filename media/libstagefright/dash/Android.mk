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

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
        MPDParser.cpp           \
	../foundation/AString.cpp \
	main_testparser.cpp

LOCAL_MODULE:= mpdparser_test

#LOCAL_ADDITIONAL_DEPENDENCIES := lib64utils libxml2

LOCAL_C_INCLUDES:= \
	$(TOP)/frameworks/av/include/media/stagefright/foundation \
	$(TOP)/frameworks/av/include \
	$(TOP)/frameworks/av/media/libstagefright \
	$(TOP)/frameworks/av/media/libstagefright/include \
	$(TOP)/frameworks/native/include \
	$(TOP)/frameworks/native/include/media/openmax \
	$(TOP)/external/libxml2/include                \
	$(TOP)/external/icu4c/common                   \

LOCAL_STATIC_LIBRARIES := \
	libxml2 \
	libutils

LOCAL_SHARED_LIBRARIES := \
	libicuuc          \
	liblog

LOCAL_LDLIBS := -lstdc++ -ldl

include $(BUILD_HOST_EXECUTABLE)
