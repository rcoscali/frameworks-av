/*
 * Copyright (C) 2014 Nagravision
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "DashMpdSource"
#include <utils/Log.h>

#include "FragmentedMP4Parser.h"
#include "DashDataSource.h"
#include "DashSession.h"
#include "DashMpdSource.h"

#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>

namespace android
{

  NuPlayer::DashMpdSource::DashMpdSource(
	    const sp<AMessage> &notify,
	    const char *url,
	    const KeyedVector<String8, String8> *headers,
	    bool uidValid, uid_t uid)
    : Source(notify),
      mURL(url),
      mUIDValid(uidValid),
      mUID(uid),
      mFlags(0),
      mFinalResult(OK),
      mOffset(0) 
  {
    if (headers) 
      {
	mExtraHeaders = *headers;
	
	ssize_t index =
	  mExtraHeaders.indexOfKey(String8("x-hide-urls-from-log"));
	
	if (index >= 0) 
	  {
	    mFlags |= kFlagIncognito;
	    
	    mExtraHeaders.removeItemsAt(index);
	  }
      }
  };

  NuPlayer::DashMpdSource::~DashMpdSource() 
  {
    if (mDashSession != NULL) 
      {
	mDashSession->disconnect();
	mDashLooper->stop();
      }
  };

  void NuPlayer::DashMpdSource::prepareAsync()
  {
    mDashLooper = new ALooper;
    mDashLooper->setName("Dynamic Adaptive Streaming over HTTP");
    mDashLooper->start();

    sp<AMessage> notify = new AMessage(kWhatSessionNotify, id());

    mDashSession = new DashSession(notify,
				   (mFlags & kFlagIncognito) ? DashSession::kFlagIncognito : 0,
				   mUIDValid, mUID);

    mDashLooper->registerHandler(mDashSession);

    mDashSession->connect(mURL.c_str(), 
			  mExtraHeaders.isEmpty() ? NULL : &mExtraHeaders);

    mMP4Parser = new FragmentedMP4Parser();
  };

  void NuPlayer::DashMpdSource::start() 
  {
  };

  sp<MetaData> NuPlayer::DashMpdSource::getFormatMeta(bool audio) 
  {
    /*
    FragmentedMP4Parser::SourceType type =
      audio ? ATSParser::AUDIO : ATSParser::VIDEO;

    sp<AnotherPacketSource> source =
      static_cast<AnotherPacketSource *>(mTSParser->getSource(type).get());

    return (source == NULL ? NULL : source->getFormat());
    */
    return NULL;
  };

  status_t NuPlayer::DashMpdSource::dequeueAccessUnit(bool audio, sp<ABuffer> *accessUnit) 
  {
    /*
    ATSParser::SourceType type =
      audio ? ATSParser::AUDIO : ATSParser::VIDEO;
    
    sp<AnotherPacketSource> source =
      static_cast<AnotherPacketSource *>(mTSParser->getSource(type).get());
    
    if (source == NULL) 
      return -EWOULDBLOCK;

    status_t finalResult;
    if (!source->hasBufferAvailable(&finalResult))
      return finalResult == OK ? -EWOULDBLOCK : finalResult;

    return source->dequeueAccessUnit(accessUnit);
    */
    return -EWOULDBLOCK;
  };

  status_t NuPlayer::DashMpdSource::getDuration(int64_t *durationUs) 
  {
    return mDashSession->getDuration(durationUs);
  };

  status_t NuPlayer::DashMpdSource::seekTo(int64_t seekTimeUs) 
  {
    return OK;
  };

  void NuPlayer::DashMpdSource::onMessageReceived(const sp<AMessage> &msg) 
  {
    switch (msg->what()) 
      {
      case kWhatSessionNotify:
	{
	  onSessionNotify(msg);
	  break;
	}
	
      default:
	Source::onMessageReceived(msg);
	break;
      }
  };

  void NuPlayer::DashMpdSource::onSessionNotify(const sp<AMessage> &msg) 
  {
    int32_t what;
    CHECK(msg->findInt32("what", &what));
    
    switch (what) 
      {
      case DashSession::kWhatPrepared:
	{
	  notifyVideoSizeChanged(0, 0);
	  
	  uint32_t flags = FLAG_CAN_PAUSE;
	  if (mDashSession->isSeekable()) 
	    {
	      flags |= FLAG_CAN_SEEK;
	      flags |= FLAG_CAN_SEEK_BACKWARD;
	      flags |= FLAG_CAN_SEEK_FORWARD;
	    }

	  if (mDashSession->hasDynamicDuration()) 
	    {
	      flags |= FLAG_DYNAMIC_DURATION;
	    }
	  
	  notifyFlagsChanged(flags);
	  
	  notifyPrepared();
	  break;
	};

      case DashSession::kWhatPreparationFailed:
	{
	  status_t err;
	  CHECK(msg->findInt32("err", &err));
	  
	  notifyPrepared(err);
	  break;
	};

      default:
	TRESPASS();
      };
  };

}; // namespace


