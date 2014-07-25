/*
**
** Copyright 2012, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_NDEBUG 0
#define LOG_TAG "MediaPlayerFactory"
#include <utils/Log.h>

#include <cutils/properties.h>
#include <media/IMediaPlayer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <utils/Errors.h>
#include <utils/misc.h>

#include "MediaPlayerFactory.h"

#include "MidiFile.h"
#include "TestPlayerStub.h"
#include "StagefrightPlayer.h"
#include "nuplayer/NuPlayerDriver.h"
#include <dlfcn.h>

namespace android {

  static const char * PLAYER_NAMES[7] = {
    /* 0, */ "- (none at 0)",
    /* 1, */ "- (none at 1)",
    /* 2, */ "SONIVOX_Player",
    /* 3, */ "STAGEFRIGHT_Player",
    /* 4, */ "NU_Player",
    /* 5, */ "TEST_Player",
    /* 6, */ "OMX_Player",
  };

  Mutex MediaPlayerFactory::sLock;
  MediaPlayerFactory::tFactoryMap MediaPlayerFactory::sFactoryMap;
  bool MediaPlayerFactory::sInitComplete = false;

  status_t MediaPlayerFactory::registerFactory_l(IFactory* factory,
                                                 player_type type) {
    if (NULL == factory) {
      ALOGE("Failed to register MediaPlayerFactory of type %d, factory is"
            " NULL.", type);
      return BAD_VALUE;
    }

    if (sFactoryMap.indexOfKey(type) >= 0) {
      ALOGE("Failed to register MediaPlayerFactory of type %d, type is"
            " already registered.", type);
      return ALREADY_EXISTS;
    }

    if (sFactoryMap.add(type, factory) < 0) {
      ALOGE("Failed to register MediaPlayerFactory of type %d, failed to add"
            " to map.", type);
      return UNKNOWN_ERROR;
    }

    return OK;
  }

  player_type MediaPlayerFactory::getDefaultPlayerType() {
    char value[PROPERTY_VALUE_MAX];
    ALOGV("MediaPlayerFactory::getDefaultPlayerType - Enter");
    if (property_get("media.stagefright.use-nuplayer", value, NULL)
        && (!strcmp("1", value) || !strcasecmp("true", value))) 
      {
        ALOGV("MediaPlayerFactory::getDefaultPlayerType - media.stagefright.use-nuplayer == 1 : Using NuPlayer\n");
        return NU_PLAYER;
      }
    if (value == NULL)
      {
        ALOGV("MediaPlayerFactory::getDefaultPlayerType - media.stagefright.use-nuplayer not set  : Using StagefrightPlayer\n");
      }
    else
      {
        ALOGV("MediaPlayerFactory::getDefaultPlayerType - media.stagefright.use-nuplayer == 0  : Using StagefrightPlayer\n");
      }
    return STAGEFRIGHT_PLAYER;
  }

  status_t MediaPlayerFactory::registerFactory(IFactory* factory,
                                               player_type type) {
    Mutex::Autolock lock_(&sLock);
    return registerFactory_l(factory, type);
  }

  void MediaPlayerFactory::unregisterFactory(player_type type) {
    Mutex::Autolock lock_(&sLock);
    sFactoryMap.removeItem(type);
  }

#define GET_PLAYER_TYPE_IMPL(a...)                      \
  Mutex::Autolock lock_(&sLock);			\
                                                        \
  player_type ret = STAGEFRIGHT_PLAYER;			\
  float bestScore = 0.0;				\
  int winner = -1; 				        \
                                                        \
  ALOGD("GET_PLAYER_TYPE_IMPL - %d registered players\n", sFactoryMap.size());  \
  for (size_t i = 0; i < sFactoryMap.size(); ++i) {	\
    ALOGD("GET_PLAYER_TYPE_IMPL -     try player #%d\n", i);\
                                                        \
    IFactory* v = sFactoryMap.valueAt(i);		\
    float thisScore;					\
    CHECK(v != NULL);					\
    thisScore = v->scoreFactory(a);			\
    ALOGD("GET_PLAYER_TYPE_IMPL -     player[%d]=%f\n", i, thisScore);	\
    if (thisScore > bestScore) {			\
      ret = sFactoryMap.keyAt(i);			\
      bestScore = thisScore;				\
      winner = i;	 			        \
      ALOGD("GET_PLAYER_TYPE_IMPL -     new best score for %s\n", PLAYER_NAMES[ret]);	\
    }							\
  }							\
                                                        \
  if (0.0 == bestScore)                                 \
    {				                        \
      ALOGD("GET_PLAYER_TYPE_IMPL - no best score - get default\n");	\
      ret = getDefaultPlayerType();			\
    }							\
  else                                                  \
    {							\
      ALOGD("GET_PLAYER_TYPE_IMPL - best score = %f\n", bestScore); \
      ALOGD("GET_PLAYER_TYPE_IMPL - for player[%d]=%s\n", winner, PLAYER_NAMES[ret]); \
    }							\
                                                        \
  return ret;

  player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client, const char* url) 
  {
    ALOGV("MediaPlayerFactory::getPlayerType - Enter\n");
    ALOGV("MediaPlayerFactory::getPlayerType - url = '%s'\n", url);
    GET_PLAYER_TYPE_IMPL(client, url);
  }

  player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
						int fd,
						int64_t offset,
						int64_t length) 
  {
    ALOGV("MediaPlayerFactory::getPlayerType - Enter\n");
    ALOGV("MediaPlayerFactory::getPlayerType - fd[%d] at (%lld=>%lld)\n", fd, offset, length);
    GET_PLAYER_TYPE_IMPL(client, fd, offset, length);
  }

  player_type MediaPlayerFactory::getPlayerType(const sp<IMediaPlayer>& client,
						const sp<IStreamSource> &source) 
  {
    ALOGV("MediaPlayerFactory::getPlayerType - Enter\n");
    ALOGV("MediaPlayerFactory::getPlayerType - IStreamSource\n");
    GET_PLAYER_TYPE_IMPL(client, source);
  }

#undef GET_PLAYER_TYPE_IMPL

  sp<MediaPlayerBase> MediaPlayerFactory::createPlayer(
  player_type playerType,
    void* cookie,
    notify_callback_f notifyFunc) {
  sp<MediaPlayerBase> p;
  IFactory* factory;
  status_t init_result;
  Mutex::Autolock lock_(&sLock);

  ALOGV("MediaPlayerFactory::createPlayer - Enter\n");
  ALOGD("MediaPlayerFactory::createPlayer - playerType = %d\n", playerType);
  ALOGD("MediaPlayerFactory::createPlayer - cookie = %p\n", cookie);
    
  if (sFactoryMap.indexOfKey(playerType) < 0) 
    {
      ALOGE("Failed to create player object of type %d, no registered"
	    " factory", playerType);
      return p;
    }

  factory = sFactoryMap.valueFor(playerType);
  CHECK(NULL != factory);
  p = factory->createPlayer();

  if (p == NULL) 
    {
      ALOGE("Failed to create player object of type %d, create failed",
	    playerType);
      return p;
    }

  init_result = p->initCheck();
  if (init_result == NO_ERROR) 
    {
      ALOGD("MediaPlayerFactory::createPlayer - playerType = %d init OK\n", playerType);
      p->setNotifyCallback(cookie, notifyFunc);
    } 
  else 
    {
      ALOGE("Failed to create player object of type %d, initCheck failed"
	    " (res = %d)", playerType, init_result);
      p.clear();
    }
  
  ALOGD("MediaPlayerFactory::createPlayer - playerType = %d created\n", playerType);
  return p;
  }

  /*****************************************************************************
   *                                                                           *
   *                     Built-In Factory Implementations                      *
   *                                                                           *
   *****************************************************************************/

  class StagefrightPlayerFactory :
    public MediaPlayerFactory::IFactory 
  {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
			       int fd,
			       int64_t offset,
			       int64_t length,
			       float curScore) 
    {
      char buf[20];
      lseek(fd, offset, SEEK_SET);
      read(fd, buf, sizeof(buf));
      lseek(fd, offset, SEEK_SET);
      
      long ident = *((long*)buf);

      ALOGV("StagefrightPlayerFactory::scoreFactory - Enter\n");
      
      ALOGD("StagefrightPlayerFactory:scoreFactory - fd[%d] at {%lld=>%lld}\n", fd, offset, length);
      ALOGD("StagefrightPlayerFactory:scoreFactory - cur score = %f\n", curScore);
      ALOGD("StagefrightPlayerFactory:scoreFactory - def score = 1.0\n");

      // Ogg vorbis?
      if (ident == 0x5367674f) // 'OggS'
	{
	  ALOGV("StagefrightPlayerFactory::scoreFactory - Ogg scored 1.0\n");
	  return 1.0;
	}
      
      ALOGV("StagefrightPlayerFactory::scoreFactory - not Ogg scored 0.0\n");
      return 0.0;
    }

    virtual sp<MediaPlayerBase> createPlayer() {
      ALOGV(" create StagefrightPlayer");
      return new StagefrightPlayer();
    }
  };

  class NuPlayerFactory : public MediaPlayerFactory::IFactory 
  {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
			       const char* url,
			       float curScore) 
    {
      static const float kOurScore = 0.8;
      static const float kOurStreamingBonus = 0.2;

      ALOGV("NuPlayerFactory::scoreFactory - Enter (default score 0.8)\n");
      
      ALOGD("NuPlayerFactory:scoreFactory - url '%s'\n", url);
      ALOGD("NuPlayerFactory:scoreFactory - cur score = %f\n", curScore);
      ALOGD("NuPlayerFactory:scoreFactory - def score = %f\n", kOurScore);

      if (kOurScore <= curScore)
	{
	  ALOGV("NuPlayerFactory::scoreFactory - curScore greater => scored 0.0\n");
	  return 0.0;
	}
      
      if (!strncasecmp("http://" , url, 7) ||
	  !strncasecmp("https://", url, 8) ||
	  !strncasecmp("file://" , url, 7)) 
	{
	  ALOGV("NuPlayerFactory::scoreFactory - http/https/file scheme\n");
	  size_t len = strlen(url);
	  if (len >= 5 && !strcasecmp(".m3u8", &url[len - 5])) 
	    {
	      ALOGV("NuPlayerFactory::scoreFactory - m3u8 ext playlist (HLS) => scored %f\n", kOurScore);
	      return kOurScore;
	    }
	
	  if (strstr(url,"m3u8")) 
	    {
	      ALOGV("NuPlayerFactory::scoreFactory - m3u8 playlist (HLS) => scored %f\n", kOurScore);
	      return kOurScore + kOurStreamingBonus;
	    }
	
	  if (len >= 4 && !strcasecmp(".mpd", &url[len - 4])) 
	    {
	      ALOGV("NuPlayerFactory::scoreFactory - mpd ext manifest (DASH) => scored %f\n", kOurScore + kOurStreamingBonus);
	      return kOurScore + kOurStreamingBonus;
	    }
	
	  if (strstr(url,"mpd")) 
	    {
	      ALOGV("NuPlayerFactory::scoreFactory - mpd manifest (DASH) => scored %f\n", kOurScore + kOurStreamingBonus);
	      return kOurScore;
	    }
	
	  if ((len >= 4 && !strcasecmp(".sdp", &url[len - 4])) || strstr(url, ".sdp?")) 
	    {
	      ALOGV("NuPlayerFactory::scoreFactory - sdp manifest (RTP) => scored %f\n", kOurScore);
	      return kOurScore;
	    }
	}
      
      if (!strncasecmp("rtsp://", url, 7)) 
	{
	  ALOGV("NuPlayerFactory::scoreFactory - rtsp scheme (RTP) => scored %f\n", kOurScore);
	  return kOurScore;
	}
      
      ALOGV("NuPlayerFactory::scoreFactory - NO MATCH => scored 0.0\n");
      return 0.0;
    }

    virtual float scoreFactory(const sp<IMediaPlayer>& client,
			       const sp<IStreamSource> &source,
			       float curScore) 
    {
      ALOGV("NuPlayerFactory::scoreFactory - Enter => cur score %f\n", curScore);
      ALOGV("NuPlayerFactory::scoreFactory - Enter => scoring 1.0\n");
      return 1.0;
    }

    virtual sp<MediaPlayerBase> createPlayer() 
    {
      ALOGV(" create NuPlayer");
      return new NuPlayerDriver;
    }
  };

  class SonivoxPlayerFactory : public MediaPlayerFactory::IFactory 
  {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
			       const char* url,
			       float curScore) 
    {
      ALOGV("SonivoxPlayerFactory::scoreFactory - Enter\n");
      ALOGV("SonivoxPlayerFactory::scoreFactory - url = %s\n", url);
      ALOGV("SonivoxPlayerFactory::scoreFactory - curScore = %f\n", curScore);
      ALOGV("SonivoxPlayerFactory::scoreFactory - default score 0.4\n");
      static const float kOurScore = 0.4;
      static const char* const FILE_EXTS[] = { ".mid",
					       ".midi",
					       ".smf",
					       ".xmf",
					       ".mxmf",
					       ".imy",
					       ".rtttl",
					       ".rtx",
					       ".ota" };
      if (kOurScore <= curScore)
	{
	  ALOGV("SonivoxPlayerFactory::scoreFactory - default score lower than curScore - scored 0.0\n");
	  return 0.0;
	}
      
      // use MidiFile for MIDI extensions
      int lenURL = strlen(url);
      for (int i = 0; i < NELEM(FILE_EXTS); ++i) 
	{
	  int len = strlen(FILE_EXTS[i]);
	  int start = lenURL - len;
	  if (start > 0) 
	    {
	      if (!strncasecmp(url + start, FILE_EXTS[i], len)) 
		{
		  ALOGV("SonivoxPlayerFactory::scoreFactory - found ext %s - scored %f\n", FILE_EXTS[i], kOurScore);
		  return kOurScore;
		}
	    }
	}
      
      ALOGV("SonivoxPlayerFactory::scoreFactory - NO MATCH - scored 0.0\n");
      return 0.0;
    }
    
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
			       int fd,
			       int64_t offset,
			       int64_t length,
			       float curScore) 
    {
      static const float kOurScore = 0.8;
      
      ALOGV("SonivoxPlayerFactory::scoreFactory - Enter\n");
      ALOGV("SonivoxPlayerFactory::scoreFactory - fd[%d] at (%lld=>%lld)\n", fd, offset, length);
      ALOGV("SonivoxPlayerFactory::scoreFactory - curScore = %f\n", curScore);
      ALOGV("SonivoxPlayerFactory::scoreFactory - default score 0.8\n");

      if (kOurScore <= curScore)
	{
	  ALOGV("SonivoxPlayerFactory::scoreFactory - default score lower than curScore - scored 0.0\n");
	  return 0.0;
	}
      
      // Some kind of MIDI?
      EAS_DATA_HANDLE easdata;
      if (EAS_Init(&easdata) == EAS_SUCCESS) 
	{
	  EAS_FILE locator;
	  locator.path = NULL;
	  locator.fd = fd;
	  locator.offset = offset;
	  locator.length = length;
	  EAS_HANDLE  eashandle;
	  if (EAS_OpenFile(easdata, &locator, &eashandle) == EAS_SUCCESS) {
	    ALOGV("SonivoxPlayerFactory::scoreFactory - found midi device - scored %f\n", kOurScore);
	    EAS_CloseFile(easdata, eashandle);
	    EAS_Shutdown(easdata);
	    return kOurScore;
	  }
	  EAS_Shutdown(easdata);
	}
      
      ALOGV("SonivoxPlayerFactory::scoreFactory - NO midi device - scored 0.0\n");
      return 0.0;
    }
    
    virtual sp<MediaPlayerBase> createPlayer() 
    {
      ALOGV(" create MidiFile");
      return new MidiFile();
    }
  };
  
  class TestPlayerFactory : public MediaPlayerFactory::IFactory 
  {
  public:
    virtual float scoreFactory(const sp<IMediaPlayer>& client,
                               const char* url,
                               float curScore) 
    {
      ALOGV("TestPlayerFactory::scoreFactory - Enter\n");
      ALOGV("TestPlayerFactory::scoreFactory - url = '%s'\n", url);
      ALOGV("TestPlayerFactory::scoreFactory - curScore = %f\n", curScore);
      ALOGV("TestPlayerFactory::scoreFactory - default score 1.0\n");

      if (TestPlayerStub::canBeUsed(url)) 
	{
	  ALOGV("TestPlayerFactory::scoreFactory - Test Player stub can be used => score 1.0\n");
	  return 1.0;
	}
      
      ALOGV("TestPlayerFactory::scoreFactory - Test Player stub cannot be used => score 0.0\n");
      return 0.0;
    }
    
    virtual sp<MediaPlayerBase> createPlayer() 
    {
      ALOGV("Create Test Player stub");
      return new TestPlayerStub();
    }
  };
  
  void MediaPlayerFactory::registerBuiltinFactories() 
  {
    Mutex::Autolock lock_(&sLock);
    ALOGV("MediaPlayerFactory::registerBuiltinFactories - Enter\n");
    
    if (sInitComplete)
      {
	ALOGV("MediaPlayerFactory::registerBuiltinFactories - Already initialized: Discard init\n");
	return;
      }
    
    ALOGV("MediaPlayerFactory::registerBuiltinFactories - Register Stagefright player\n");
    registerFactory_l(new StagefrightPlayerFactory(), STAGEFRIGHT_PLAYER);
    ALOGV("MediaPlayerFactory::registerBuiltinFactories - Register Nu player\n");
    registerFactory_l(new NuPlayerFactory(), NU_PLAYER);
    ALOGV("MediaPlayerFactory::registerBuiltinFactories - Register Sonivox player\n");
    registerFactory_l(new SonivoxPlayerFactory(), SONIVOX_PLAYER);
    ALOGV("MediaPlayerFactory::registerBuiltinFactories - Register Test  player\n");
    registerFactory_l(new TestPlayerFactory(), TEST_PLAYER);
    
    const char* FACTORY_LIB           = "libdashplayer.so";
    const char* FACTORY_CREATE_FN     = "CreateDASHFactory";
    
    MediaPlayerFactory::IFactory* pFactory  = NULL;
    void* pFactoryLib = NULL;
    typedef MediaPlayerFactory::IFactory* (*CreateDASHDriverFn)();
    pFactoryLib = ::dlopen(FACTORY_LIB, RTLD_LAZY);
    if (pFactoryLib != NULL) 
      {
	ALOGV("MediaPlayerFactory::registerBuiltinFactories - Dash player lib opened\n");
	CreateDASHDriverFn pCreateFnPtr;
	pCreateFnPtr = (CreateDASHDriverFn) dlsym(pFactoryLib, FACTORY_CREATE_FN);
	if (pCreateFnPtr == NULL) 
	  {
	    ALOGE("Could not locate pCreateFnPtr");
	  } 
	else 
	  {
	    ALOGV("MediaPlayerFactory::registerBuiltinFactories - Dash player Create sym loaded\n");
	    pFactory = pCreateFnPtr();
	    if(pFactory == NULL) 
	      {
		ALOGE("Failed to invoke CreateDASHDriverFn...");
	      } 
	    else 
	      {
		ALOGV("MediaPlayerFactory::registerBuiltinFactories - Dash player created... registering\n");
		registerFactory_l(pFactory,DASH_PLAYER);
	      }
	  }
      }

    ALOGV("MediaPlayerFactory::registerBuiltinFactories - init completed\n");
    sInitComplete = true;
  }
  
}  // namespace android
