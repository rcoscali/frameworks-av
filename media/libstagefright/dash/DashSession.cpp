/*
 * Copyright (C) 2014 
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
#define LOG_TAG "DashSession"
#include <utils/Log.h>

#include "include/DashSession.h"

#include "DashDataSource.h"

#include "include/MPDParser.h"
#include "include/HTTPBase.h"

#include <cutils/properties.h>
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/DataSource.h>
#include <media/stagefright/FileSource.h>
#include <media/stagefright/MediaErrors.h>

#include <ctype.h>
#include <openssl/aes.h>
#include <openssl/md5.h>

namespace android {

  DashSession::DashSession(const sp<AMessage> &notify, 
			   uint32_t flags, 
			   bool uidValid, 
			   uid_t uid)
    : mNotify(notify),
      mFlags(flags),
      mUIDValid(uidValid),
      mUID(uid),
      mInPreparationPhase(true),
      mDataSource(new DashDataSource),
      mHTTPDataSource(HTTPBase::Create((mFlags & kFlagIncognito)
				       ? HTTPBase::kFlagIncognito
				       : 0)),
      mPrevBandwidthIndex(-1),
      mLastMpdFetchTimeUs(-1),
      mSeqNumber(-1),
      mSeekTimeUs(-1),
      mNumRetries(0),
    mStartOfPlayback(true),
    mDurationUs(-1),
    mDurationFixed(false),
    mSeekDone(false),
    mDisconnectPending(false),
    mMonitorQueueGeneration(0),
    mRefreshState(INITIAL_MINIMUM_RELOAD_DELAY) {
    if (mUIDValid) {
      mHTTPDataSource->setUID(mUID);
    }
  }

  DashSession::~DashSession() {
  }

  sp<DataSource> DashSession::getDataSource() {
    return mDataSource;
  }

  void DashSession::connect(const char *url, 
			    const KeyedVector<String8, String8> *headers) 
  {
    sp<AMessage> msg = new AMessage(kWhatConnect, id());
    msg->setString("url", url);

    if (headers != NULL) 
      {
	msg->setPointer("headers",
			new KeyedVector<String8, String8>(*headers));
      }

    msg->post();
  }

  void DashSession::disconnect() {
    Mutex::Autolock autoLock(mLock);
    mDisconnectPending = true;

    mHTTPDataSource->disconnect();

    (new AMessage(kWhatDisconnect, id()))->post();
  }

  void DashSession::seekTo(int64_t timeUs) {
    Mutex::Autolock autoLock(mLock);
    mSeekDone = false;

    sp<AMessage> msg = new AMessage(kWhatSeek, id());
    msg->setInt64("timeUs", timeUs);
    msg->post();

    while (!mSeekDone) {
      mCondition.wait(mLock);
    }
  }

  void DashSession::onMessageReceived(const sp<AMessage> &msg) 
  {
    switch (msg->what()) 
      {
      case kWhatConnect:
	onConnect(msg);
	break;

      case kWhatDisconnect:
	onDisconnect();
	break;

      case kWhatMonitorQueue:
	{
	  int32_t generation;
	  CHECK(msg->findInt32("generation", &generation));

	  if (generation != mMonitorQueueGeneration) 
	    {
	      // Stale event
	      break;
	    }

	  onMonitorQueue();
	  break;
	}

      case kWhatSeek:
	onSeek(msg);
	break;
	
      default:
	TRESPASS();
	break;
      }
  }

  // static
  int DashSession::SortByBandwidth(const BandwidthItem *a, const BandwidthItem *b) 
  {
    if (a->mBandwidth < b->mBandwidth) 
      {
	return -1;
      } 
    else if (a->mBandwidth == b->mBandwidth) 
      {
	return 0;
      }

    return 1;
  }

  void DashSession::onConnect(const sp<AMessage> &msg) {
    AString url;
    CHECK(msg->findString("url", &url));

    KeyedVector<String8, String8> *headers = NULL;
    if (!msg->findPointer("headers", (void **)&headers)) {
      mExtraHeaders.clear();
    } else {
      mExtraHeaders = *headers;

      delete headers;
      headers = NULL;
    }

    //ALOGI("onConnect <URL suppressed>");
    ALOGI("onConnect %s", url.c_str());

    mMasterURL = url;

    bool dummy;
    sp<MPDParser> mpd = fetchMpd(url.c_str(), &dummy);

    if (mpd == NULL) {
      ALOGE("unable to fetch manifest '%s'.", url.c_str());

      signalEOS(ERROR_IO);
      return;
    }

    if (mpd->isDynamic()) {
      for (size_t i = 0; i < mpd->size(); ++i) {
	BandwidthItem item;

	sp<AMessage> meta;
	mpd->itemAt(i, &item.mURI, &meta);

	unsigned long bandwidth;
	CHECK(meta->findInt32("bandwidth", (int32_t *)&item.mBandwidth));

	mBandwidthItems.push(item);
      }

      CHECK_GT(mBandwidthItems.size(), 0u);

      mBandwidthItems.sort(SortByBandwidth);
    }

    postMonitorQueue();
  }

  void DashSession::onDisconnect() {
    ALOGI("onDisconnect");

    signalEOS(ERROR_END_OF_STREAM);

    Mutex::Autolock autoLock(mLock);
    mDisconnectPending = false;
  }

  status_t DashSession::fetchFile(const char *url, sp<ABuffer> *out,
				  int64_t range_offset, int64_t range_length) 
  {
    *out = NULL;

    sp<DataSource> source;

    if (!strncasecmp(url, "file://", 7)) {
      source = new FileSource(url + 7);
    } else if (strncasecmp(url, "http://", 7)
	       && strncasecmp(url, "https://", 8)) {
      return ERROR_UNSUPPORTED;
    } else {
      {
	Mutex::Autolock autoLock(mLock);

	if (mDisconnectPending) {
	  return ERROR_IO;
	}
      }

      KeyedVector<String8, String8> headers = mExtraHeaders;
      if (range_offset > 0 || range_length >= 0) {
	headers.add(String8("Range"),
                    String8(StringPrintf("bytes=%lld-%s",
					 range_offset,
					 range_length < 0 ?
					 "" : 
					 StringPrintf("%lld", 
						      range_offset + range_length - 1).c_str()).c_str()));
      }
      status_t err = mHTTPDataSource->connect(url, &headers);

      if (err != OK) {
	return err;
      }

      source = mHTTPDataSource;
    }

    off64_t size;
    status_t err = source->getSize(&size);

    if (err != OK) {
      size = 65536;
    }

    sp<ABuffer> buffer = new ABuffer(size);
    buffer->setRange(0, 0);

    for (;;) {
      size_t bufferRemaining = buffer->capacity() - buffer->size();

      if (bufferRemaining == 0) {
	bufferRemaining = 32768;

	ALOGV("increasing download buffer to %d bytes",
	      buffer->size() + bufferRemaining);

	sp<ABuffer> copy = new ABuffer(buffer->size() + bufferRemaining);
	memcpy(copy->data(), buffer->data(), buffer->size());
	copy->setRange(0, buffer->size());

	buffer = copy;
      }

      size_t maxBytesToRead = bufferRemaining;
      if (range_length >= 0) {
	int64_t bytesLeftInRange = range_length - buffer->size();
	if (bytesLeftInRange < maxBytesToRead) {
	  maxBytesToRead = bytesLeftInRange;

	  if (bytesLeftInRange == 0) {
	    break;
	  }
	}
      }

      ssize_t n = source->readAt(buffer->size(), buffer->data() + buffer->size(),
				 maxBytesToRead);

      if (n < 0) return n;
      if (n == 0) break;

      buffer->setRange(0, buffer->size() + (size_t)n);
    }

    *out = buffer;

    return OK;
  }

  sp<MPDParser> DashSession::fetchMpd(const char *url, bool *unchanged) {
    ALOGV("fetchMpd '%s'", url);

    *unchanged = false;

    sp<ABuffer> buffer;
    status_t err = fetchFile(url, &buffer);

    if (err != OK) {
      return NULL;
    }

    // MD5 functionality is not available on the simulator, treat all
    // playlists as changed.

#if defined(HAVE_ANDROID_OS)
    uint8_t hash[16];

    MD5_CTX m;
    MD5_Init(&m);
    MD5_Update(&m, buffer->data(), buffer->size());

    MD5_Final(hash, &m);

    if (mMpd != NULL && !memcmp(hash, mMpdHash, 16)) {
      // playlist unchanged

      if (mRefreshState != THIRD_UNCHANGED_RELOAD_ATTEMPT) {
	mRefreshState = (RefreshState)(mRefreshState + 1);
      }

      *unchanged = true;

      ALOGV("Mpd unchanged, refresh state is now %d",
	    (int)mRefreshState);

      return NULL;
    }

    memcpy(mMpdHash, hash, sizeof(hash));

    mRefreshState = INITIAL_MINIMUM_RELOAD_DELAY;
#endif

    sp<MPDParser> mpd =
      new MPDParser(url, buffer->data(), buffer->size());

    if (playlist->initCheck() != OK) {
      ALOGE("failed to parse media presentation descriptor");

      return NULL;
    }

    return playlist;
  }

  int64_t DashSession::getSegmentStartTimeUs(int32_t seqNumber) const {
    CHECK(mMpd != NULL);

    int32_t firstSeqNumberInMpd;
    if (mMpd->meta() == NULL || !mMpd->meta()->findInt32(
								   "media-sequence", &firstSeqNumberInMpd)) {
      firstSeqNumberInMpd = 0;
    }

    int32_t lastSeqNumberInMpd =
      firstSeqNumberInMpd + (int32_t)mMpd->size() - 1;

    CHECK_GE(seqNumber, firstSeqNumberInMpd);
    CHECK_LE(seqNumber, lastSeqNumberInMpd);

    int64_t segmentStartUs = 0ll;
    for (int32_t index = 0;
	 index < seqNumber - firstSeqNumberInMpd; ++index) {
      sp<AMessage> itemMeta;
      CHECK(mMpd->itemAt(
			      index, NULL /* uri */, &itemMeta));

      int64_t itemDurationUs;
      CHECK(itemMeta->findInt64("durationUs", &itemDurationUs));

      segmentStartUs += itemDurationUs;
    }

    return segmentStartUs;
  }

  static double uniformRand() {
    return (double)rand() / RAND_MAX;
  }

  size_t DashSession::getBandwidthIndex() {
    if (mBandwidthItems.size() == 0) {
      return 0;
    }

#if 1
    int32_t bandwidthBps;
    if (mHTTPDataSource != NULL
	&& mHTTPDataSource->estimateBandwidth(&bandwidthBps)) {
      ALOGV("bandwidth estimated at %.2f kbps", bandwidthBps / 1024.0f);
    } else {
      ALOGV("no bandwidth estimate.");
      return 0;  // Pick the lowest bandwidth stream by default.
    }

    char value[PROPERTY_VALUE_MAX];
    if (property_get("media.httplive.max-bw", value, NULL)) {
      char *end;
      long maxBw = strtoul(value, &end, 10);
      if (end > value && *end == '\0') {
	if (maxBw > 0 && bandwidthBps > maxBw) {
	  ALOGV("bandwidth capped to %ld bps", maxBw);
	  bandwidthBps = maxBw;
	}
      }
    }

    // Consider only 80% of the available bandwidth usable.
    bandwidthBps = (bandwidthBps * 8) / 10;

    // Pick the highest bandwidth stream below or equal to estimated bandwidth.

    size_t index = mBandwidthItems.size() - 1;
    while (index > 0 && mBandwidthItems.itemAt(index).mBandwidth
	   > (size_t)bandwidthBps) {
      --index;
    }
#elif 0
    // Change bandwidth at random()
    size_t index = uniformRand() * mBandwidthItems.size();
#elif 0
    // There's a 50% chance to stay on the current bandwidth and
    // a 50% chance to switch to the next higher bandwidth (wrapping around
    // to lowest)
    const size_t kMinIndex = 0;

    size_t index;
    if (mPrevBandwidthIndex < 0) {
      index = kMinIndex;
    } else if (uniformRand() < 0.5) {
      index = (size_t)mPrevBandwidthIndex;
    } else {
      index = mPrevBandwidthIndex + 1;
      if (index == mBandwidthItems.size()) {
	index = kMinIndex;
      }
    }
#elif 0
    // Pick the highest bandwidth stream below or equal to 1.2 Mbit/sec

    size_t index = mBandwidthItems.size() - 1;
    while (index > 0 && mBandwidthItems.itemAt(index).mBandwidth > 1200000) {
      --index;
    }
#else
    size_t index = mBandwidthItems.size() - 1;  // Highest bandwidth stream
#endif

    return index;
  }

  bool DashSession::timeToRefreshMpd(int64_t nowUs) const {
    if (mMpd == NULL) {
      CHECK_EQ((int)mRefreshState, (int)INITIAL_MINIMUM_RELOAD_DELAY);
      return true;
    }

    int32_t targetDurationSecs;
    CHECK(mMpd->meta()->findInt32("target-duration", &targetDurationSecs));

    int64_t targetDurationUs = targetDurationSecs * 1000000ll;

    int64_t minMpdAgeUs;

    switch (mRefreshState) {
    case INITIAL_MINIMUM_RELOAD_DELAY:
      {
	size_t n = mMpd->size();
	if (n > 0) {
	  sp<AMessage> itemMeta;
	  CHECK(mMpd->itemAt(n - 1, NULL /* uri */, &itemMeta));

	  int64_t itemDurationUs;
	  CHECK(itemMeta->findInt64("durationUs", &itemDurationUs));

	  minMpdAgeUs = itemDurationUs;
	  break;
	}

	// fall through
      }

    case FIRST_UNCHANGED_RELOAD_ATTEMPT:
      {
	minMpdAgeUs = targetDurationUs / 2;
	break;
      }

    case SECOND_UNCHANGED_RELOAD_ATTEMPT:
      {
	minMpdAgeUs = (targetDurationUs * 3) / 2;
	break;
      }

    case THIRD_UNCHANGED_RELOAD_ATTEMPT:
      {
	minMpdAgeUs = targetDurationUs * 3;
	break;
      }

    default:
      TRESPASS();
      break;
    }

    return mLastMpdFetchTimeUs + minMpdAgeUs <= nowUs;
  }

  void DashSession::onDownloadNext() {
    size_t bandwidthIndex = getBandwidthIndex();

  rinse_repeat:
    int64_t nowUs = ALooper::GetNowUs();

    if (mLastMpdFetchTimeUs < 0
	|| (ssize_t)bandwidthIndex != mPrevBandwidthIndex
	|| (!mMpd->isComplete() && timeToRefreshMpd(nowUs))) {
      AString url;
      if (mBandwidthItems.size() > 0) {
	url = mBandwidthItems.editItemAt(bandwidthIndex).mURI;
      } else {
	url = mMasterURL;
      }

      if ((ssize_t)bandwidthIndex != mPrevBandwidthIndex) {
	// If we switch bandwidths, do not pay any heed to whether
	// playlists changed since the last time...
	mMpd.clear();
      }

      bool unchanged;
      sp<MPDParser> playlist = fetchMpd(url.c_str(), &unchanged);
      if (playlist == NULL) {
	if (unchanged) {
	  // We succeeded in fetching the playlist, but it was
	  // unchanged from the last time we tried.
	} else {
	  ALOGE("failed to load playlist at url '%s'", url.c_str());
	  signalEOS(ERROR_IO);

	  return;
	}
      } else {
	mMpd = playlist;
      }

      if (!mDurationFixed) {
	Mutex::Autolock autoLock(mLock);

	if (!mMpd->isComplete() && !mMpd->isEvent()) {
	  mDurationUs = -1;
	  mDurationFixed = true;
	} else {
	  mDurationUs = 0;
	  for (size_t i = 0; i < mMpd->size(); ++i) {
	    sp<AMessage> itemMeta;
	    CHECK(mMpd->itemAt(
				    i, NULL /* uri */, &itemMeta));

	    int64_t itemDurationUs;
	    CHECK(itemMeta->findInt64("durationUs", &itemDurationUs));

	    mDurationUs += itemDurationUs;
	  }

	  mDurationFixed = mMpd->isComplete();
	}
      }

      mLastMpdFetchTimeUs = ALooper::GetNowUs();
    }

    int32_t firstSeqNumberInMpd;
    if (mMpd->meta() == NULL || !mMpd->meta()->findInt32(
								   "media-sequence", &firstSeqNumberInMpd)) {
      firstSeqNumberInMpd = 0;
    }

    bool seekDiscontinuity = false;
    bool explicitDiscontinuity = false;
    bool bandwidthChanged = false;

    if (mSeekTimeUs >= 0) {
      if (mMpd->isComplete() || mMpd->isEvent()) {
	size_t index = 0;
	int64_t segmentStartUs = 0;
	while (index < mMpd->size()) {
	  sp<AMessage> itemMeta;
	  CHECK(mMpd->itemAt(
				  index, NULL /* uri */, &itemMeta));

	  int64_t itemDurationUs;
	  CHECK(itemMeta->findInt64("durationUs", &itemDurationUs));

	  if (mSeekTimeUs < segmentStartUs + itemDurationUs) {
	    break;
	  }

	  segmentStartUs += itemDurationUs;
	  ++index;
	}

	if (index < mMpd->size()) {
	  int32_t newSeqNumber = firstSeqNumberInMpd + index;

	  ALOGI("seeking to seq no %d", newSeqNumber);

	  mSeqNumber = newSeqNumber;

	  mDataSource->reset();

	  // reseting the data source will have had the
	  // side effect of discarding any previously queued
	  // bandwidth change discontinuity.
	  // Therefore we'll need to treat these seek
	  // discontinuities as involving a bandwidth change
	  // even if they aren't directly.
	  seekDiscontinuity = true;
	  bandwidthChanged = true;
	}
      }

      mSeekTimeUs = -1;

      Mutex::Autolock autoLock(mLock);
      mSeekDone = true;
      mCondition.broadcast();
    }

    const int32_t lastSeqNumberInMpd =
      firstSeqNumberInMpd + (int32_t)mMpd->size() - 1;

    if (mSeqNumber < 0) {
      if (mMpd->isComplete()) {
	mSeqNumber = firstSeqNumberInMpd;
      } else {
	// If this is a live session, start 3 segments from the end.
	mSeqNumber = lastSeqNumberInMpd - 3;
	if (mSeqNumber < firstSeqNumberInMpd) {
	  mSeqNumber = firstSeqNumberInMpd;
	}
      }
    }

    if (mSeqNumber < firstSeqNumberInMpd
	|| mSeqNumber > lastSeqNumberInMpd) {
      if (mPrevBandwidthIndex != (ssize_t)bandwidthIndex) {
	// Go back to the previous bandwidth.

	ALOGI("new bandwidth does not have the sequence number "
	      "we're looking for, switching back to previous bandwidth");

	mLastMpdFetchTimeUs = -1;
	bandwidthIndex = mPrevBandwidthIndex;
	goto rinse_repeat;
      }

      if (!mMpd->isComplete() && mNumRetries < kMaxNumRetries) {
	++mNumRetries;

	if (mSeqNumber > lastSeqNumberInMpd) {
	  mLastMpdFetchTimeUs = -1;
	  postMonitorQueue(3000000ll);
	  return;
	}

	// we've missed the boat, let's start from the lowest sequence
	// number available and signal a discontinuity.

	ALOGI("We've missed the boat, restarting playback.");
	mSeqNumber = lastSeqNumberInMpd;
	explicitDiscontinuity = true;

	// fall through
      } else {
	ALOGE("Cannot find sequence number %d in playlist "
	      "(contains %d - %d)",
	      mSeqNumber, firstSeqNumberInMpd,
	      firstSeqNumberInMpd + mMpd->size() - 1);

	signalEOS(ERROR_END_OF_STREAM);
	return;
      }
    }

    mNumRetries = 0;

    AString uri;
    sp<AMessage> itemMeta;
    CHECK(mMpd->itemAt(
			    mSeqNumber - firstSeqNumberInMpd,
			    &uri,
			    &itemMeta));

    int32_t val;
    if (itemMeta->findInt32("discontinuity", &val) && val != 0) {
      explicitDiscontinuity = true;
    }

    int64_t range_offset, range_length;
    if (!itemMeta->findInt64("range-offset", &range_offset)
	|| !itemMeta->findInt64("range-length", &range_length)) {
      range_offset = 0;
      range_length = -1;
    }

    ALOGV("fetching segment %d from (%d .. %d)",
          mSeqNumber, firstSeqNumberInMpd, lastSeqNumberInMpd);

    sp<ABuffer> buffer;
    status_t err = fetchFile(uri.c_str(), &buffer, range_offset, range_length);
    if (err != OK) {
      ALOGE("failed to fetch .ts segment at url '%s'", uri.c_str());
      signalEOS(err);
      return;
    }

    CHECK(buffer != NULL);

    err = decryptBuffer(mSeqNumber - firstSeqNumberInMpd, buffer);

    if (err != OK) {
      ALOGE("decryptBuffer failed w/ error %d", err);

      signalEOS(err);
      return;
    }

    if (buffer->size() == 0 || buffer->data()[0] != 0x47) {
      // Not a transport stream???

      ALOGE("This doesn't look like a transport stream...");

      mBandwidthItems.removeAt(bandwidthIndex);

      if (mBandwidthItems.isEmpty()) {
	signalEOS(ERROR_UNSUPPORTED);
	return;
      }

      ALOGI("Retrying with a different bandwidth stream.");

      mLastMpdFetchTimeUs = -1;
      bandwidthIndex = getBandwidthIndex();
      mPrevBandwidthIndex = bandwidthIndex;
      mSeqNumber = -1;

      goto rinse_repeat;
    }

    if ((size_t)mPrevBandwidthIndex != bandwidthIndex) {
      bandwidthChanged = true;
    }

    if (mPrevBandwidthIndex < 0) {
      // Don't signal a bandwidth change at the very beginning of
      // playback.
      bandwidthChanged = false;
    }

    if (mStartOfPlayback) {
      seekDiscontinuity = true;
      mStartOfPlayback = false;
    }

    if (seekDiscontinuity || explicitDiscontinuity || bandwidthChanged) {
      // Signal discontinuity.

      ALOGI("queueing discontinuity (seek=%d, explicit=%d, bandwidthChanged=%d)",
	    seekDiscontinuity, explicitDiscontinuity, bandwidthChanged);

      sp<ABuffer> tmp = new ABuffer(188);
      memset(tmp->data(), 0, tmp->size());

      // signal a 'hard' discontinuity for explicit or bandwidthChanged.
      uint8_t type = (explicitDiscontinuity || bandwidthChanged) ? 1 : 0;

      if (mMpd->isComplete() || mMpd->isEvent()) {
	// If this was a live event this made no sense since
	// we don't have access to all the segment before the current
	// one.
	int64_t segmentStartTimeUs = getSegmentStartTimeUs(mSeqNumber);
	memcpy(tmp->data() + 2, &segmentStartTimeUs, sizeof(segmentStartTimeUs));

	type |= 2;
      }

      tmp->data()[1] = type;

      mDataSource->queueBuffer(tmp);
    }

    mDataSource->queueBuffer(buffer);

    mPrevBandwidthIndex = bandwidthIndex;
    ++mSeqNumber;

    postMonitorQueue();
  }

  void DashSession::signalEOS(status_t err) {
    if (mInPreparationPhase && mNotify != NULL) {
      sp<AMessage> notify = mNotify->dup();

      notify->setInt32(
		       "what",
		       err == ERROR_END_OF_STREAM
		       ? kWhatPrepared : kWhatPreparationFailed);

      if (err != ERROR_END_OF_STREAM) {
	notify->setInt32("err", err);
      }

      notify->post();

      mInPreparationPhase = false;
    }

    mDataSource->queueEOS(err);
  }

  void DashSession::onMonitorQueue() {
    if (mSeekTimeUs >= 0
	|| mDataSource->countQueuedBuffers() < kMaxNumQueuedFragments) {
      onDownloadNext();
    } else {
      if (mInPreparationPhase) {
	if (mNotify != NULL) {
	  sp<AMessage> notify = mNotify->dup();
	  notify->setInt32("what", kWhatPrepared);
	  notify->post();
	}

	mInPreparationPhase = false;
      }

      postMonitorQueue(1000000ll);
    }
  }

  /*
   *
   */
  status_t DashSession::decryptBuffer(size_t playlistIndex, const sp<ABuffer> &buffer) 
  {
    // sp<AMessage> itemMeta;
    // bool found = false;
    // AString method;

    // for (ssize_t i = playlistIndex; i >= 0; --i) {
    //   AString uri;
    //   CHECK(mMpd->itemAt(i, &uri, &itemMeta));

    //   if (itemMeta->findString("cipher-method", &method)) {
    // 	found = true;
    // 	break;
    //   }
    // }

    // if (!found) {
    //   method = "NONE";
    // }

    // if (method == "NONE") {
    //   return OK;
    // } else if (!(method == "AES-128")) {
    //   ALOGE("Unsupported cipher method '%s'", method.c_str());
    //   return ERROR_UNSUPPORTED;
    // }

    // AString keyURI;
    // if (!itemMeta->findString("cipher-uri", &keyURI)) {
    //   ALOGE("Missing key uri");
    //   return ERROR_MALFORMED;
    // }

    // ssize_t index = mAESKeyForURI.indexOfKey(keyURI);

    // sp<ABuffer> key;
    // if (index >= 0) {
    //   key = mAESKeyForURI.valueAt(index);
    // } else {
    //   key = new ABuffer(16);

    //   sp<HTTPBase> keySource =
    // 	HTTPBase::Create(
    // 			 (mFlags & kFlagIncognito)
    // 			 ? HTTPBase::kFlagIncognito
    // 			 : 0);

    //   if (mUIDValid) {
    // 	keySource->setUID(mUID);
    //   }

    //   status_t err =
    // 	keySource->connect(
    // 			   keyURI.c_str(),
    // 			   mExtraHeaders.isEmpty() ? NULL : &mExtraHeaders);

    //   if (err == OK) {
    // 	size_t offset = 0;
    // 	while (offset < 16) {
    // 	  ssize_t n = keySource->readAt(
    // 					offset, key->data() + offset, 16 - offset);
    // 	  if (n <= 0) {
    // 	    err = ERROR_IO;
    // 	    break;
    // 	  }

    // 	  offset += n;
    // 	}
    //   }

    //   if (err != OK) {
    // 	ALOGE("failed to fetch cipher key from '%s'.", keyURI.c_str());
    // 	return ERROR_IO;
    //   }

    //   mAESKeyForURI.add(keyURI, key);
    // }

    // AES_KEY aes_key;
    // if (AES_set_decrypt_key(key->data(), 128, &aes_key) != 0) {
    //   ALOGE("failed to set AES decryption key.");
    //   return UNKNOWN_ERROR;
    // }

    // unsigned char aes_ivec[16];

    // AString iv;
    // if (itemMeta->findString("cipher-iv", &iv)) {
    //   if ((!iv.startsWith("0x") && !iv.startsWith("0X"))
    // 	  || iv.size() != 16 * 2 + 2) {
    // 	ALOGE("malformed cipher IV '%s'.", iv.c_str());
    // 	return ERROR_MALFORMED;
    //   }

    //   memset(aes_ivec, 0, sizeof(aes_ivec));
    //   for (size_t i = 0; i < 16; ++i) {
    // 	char c1 = tolower(iv.c_str()[2 + 2 * i]);
    // 	char c2 = tolower(iv.c_str()[3 + 2 * i]);
    // 	if (!isxdigit(c1) || !isxdigit(c2)) {
    // 	  ALOGE("malformed cipher IV '%s'.", iv.c_str());
    // 	  return ERROR_MALFORMED;
    // 	}
    // 	uint8_t nibble1 = isdigit(c1) ? c1 - '0' : c1 - 'a' + 10;
    // 	uint8_t nibble2 = isdigit(c2) ? c2 - '0' : c2 - 'a' + 10;

    // 	aes_ivec[i] = nibble1 << 4 | nibble2;
    //   }
    // } else {
    //   memset(aes_ivec, 0, sizeof(aes_ivec));
    //   aes_ivec[15] = mSeqNumber & 0xff;
    //   aes_ivec[14] = (mSeqNumber >> 8) & 0xff;
    //   aes_ivec[13] = (mSeqNumber >> 16) & 0xff;
    //   aes_ivec[12] = (mSeqNumber >> 24) & 0xff;
    // }

    // AES_cbc_encrypt(
    // 		    buffer->data(), buffer->data(), buffer->size(),
    // 		    &aes_key, aes_ivec, AES_DECRYPT);

    // // hexdump(buffer->data(), buffer->size());

    // size_t n = buffer->size();
    // CHECK_GT(n, 0u);

    // size_t pad = buffer->data()[n - 1];

    // CHECK_GT(pad, 0u);
    // CHECK_LE(pad, 16u);
    // CHECK_GE((size_t)n, pad);
    // for (size_t i = 0; i < pad; ++i) {
    //   CHECK_EQ((unsigned)buffer->data()[n - 1 - i], pad);
    // }

    // n -= pad;

    // buffer->setRange(buffer->offset(), n);

    // return OK;
  }

  void DashSession::postMonitorQueue(int64_t delayUs) {
    sp<AMessage> msg = new AMessage(kWhatMonitorQueue, id());
    msg->setInt32("generation", ++mMonitorQueueGeneration);
    msg->post(delayUs);
  }

  void DashSession::onSeek(const sp<AMessage> &msg) {
    int64_t timeUs;
    CHECK(msg->findInt64("timeUs", &timeUs));

    mSeekTimeUs = timeUs;
    postMonitorQueue();
  }

  status_t DashSession::getDuration(int64_t *durationUs) const {
    Mutex::Autolock autoLock(mLock);
    *durationUs = mDurationUs;

    return OK;
  }

  bool DashSession::isSeekable() const {
    int64_t durationUs;
    return getDuration(&durationUs) == OK && durationUs >= 0;
  }

  bool DashSession::hasDynamicDuration() const {
    return !mDurationFixed;
  }

}  // namespace android

// Local Variables:
// mode:C++
// End:
