#include <stdio.h>
#include <stdint.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define LOG_NDEBUG 0
#define LOG_TAG "MPDParser"
#include <utils/Log.h>

#include "utils/RefBase.h"
#include "utils/Errors.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaErrors.h>

#include "MPDParser.h"

using namespace android;

int32_t android_atomic_inc(volatile int32_t* addr) {return 0;};
int32_t android_atomic_dec(volatile int32_t* addr) {return 0;};
int32_t android_atomic_add(int32_t value, volatile int32_t* addr) {return 0;};
int32_t android_atomic_and(int32_t value, volatile int32_t* addr) {return 0;};
int32_t android_atomic_or(int32_t value, volatile int32_t* addr) {return 0;};
int android_atomic_acquire_cas(int32_t oldvalue, int32_t newvalue, volatile int32_t* addr) {return 0;};
int android_atomic_release_cas(int32_t oldvalue, int32_t newvalue, volatile int32_t* addr) {return 0;};


int
main(int argc, char *argv[])
{
  AString furi("file://");
  char buffer[20480];
  FILE *fp = fopen(argv[1], "r");
  size_t bytes = fread(buffer, 20480, 1, fp);
  furi.append(argv[1]);
  MPDParser mpdParser(furi.c_str(), buffer, bytes);
}
