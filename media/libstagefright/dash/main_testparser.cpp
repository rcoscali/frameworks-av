#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define LOG_NDEBUG 0
#define LOG_TAG "MPDParserTest"
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
  struct stat stbuf;
  
  ALOGV("MPDParser Tests main - Enter\n");
  AString furi("file://");  
  furi.append(argv[1]);

  ALOGV("MPD File = %s\n", furi.c_str());

  char *buffer = (char *)NULL;

  if (stat(argv[1], &stbuf) == -1)
    {
      ALOGE("Could not stat file %s\n", argv[1]);
      exit(-1);
    }
  
  ALOGV("File size = %ld\n", stbuf.st_size);
  buffer = (char *)malloc(stbuf.st_size+1);

  FILE *fp = fopen(argv[1], "r");
  if (fp == (FILE *)NULL)
    {
      ALOGE("Cannot open MPD file %s\n", furi.c_str());
      exit (-1);
    }
  size_t bytes = read(fileno(fp), buffer, stbuf.st_size);
  if (bytes <= 0)
    {
      ALOGE("Could not read from MPD file (%d bytes returned)\n", bytes);
      exit(-1);
    }

  ALOGV("Got %u bytes. Parsing ...", bytes);
  MPDParser mpdParser(furi.c_str(), buffer, bytes);
  ALOGV("Parsed!\n");
}
