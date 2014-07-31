/*
 * Copyright (C) 2013 The Android Open Source Project
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
#define LOG_TAG "MockDrmCryptoPlugin"
#include <utils/Log.h>


#include "drm/DrmAPI.h"
#include "MockDrmCryptoPlugin.h"
#include "media/stagefright/MediaErrors.h"

using namespace android;

// Shared library entry point
DrmFactory *createDrmFactory()
{
  ALOGV("createDrmFactory - Enter\n");
  return new MockDrmFactory();
}

// Shared library entry point
CryptoFactory *createCryptoFactory()
{
  ALOGV("createCryptoFactory - Enter\n");
  return new MockCryptoFactory();
}

const uint8_t mock_uuid[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                               0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};

namespace android {

    // MockDrmFactory
    bool MockDrmFactory::isCryptoSchemeSupported(const uint8_t uuid[16])
    {
      ALOGV("MockDrmFactory::isCryptoSchemeSupported - Enter\n");
      ALOGV("MockDrmFactory::isCryptoSchemeSupported - For UUID %02x%02x%02x%02x-%02x%02x-%02x%02x-"
            "%02x%02x-%02x%02x%02x%02x%02x%02x\n",
            uuid[0], uuid[1], uuid[2], uuid[3], 
            uuid[4], uuid[5], uuid[6], uuid[7], 
            uuid[8], uuid[9], uuid[10], uuid[11], 
            uuid[12], uuid[13], uuid[14], uuid[15]);
      return (!memcmp(uuid, mock_uuid, sizeof(uuid)));
    }

    status_t MockDrmFactory::createDrmPlugin(const uint8_t uuid[16], DrmPlugin **plugin)
    {
      ALOGV("MockDrmFactory::createDrmPlugin - Enter\n");
      *plugin = new MockDrmPlugin();
      return OK;
    }

    // MockCryptoFactory
    bool MockCryptoFactory::isCryptoSchemeSupported(const uint8_t uuid[16]) const
    {
      ALOGV("MockCryptoFactory::isCryptoSchemeSupported - Enter\n");
      ALOGV("MockCryptoFactory::isCryptoSchemeSupported - For UUID %02x%02x%02x%02x-%02x%02x-%02x%02x-"
            "%02x%02x-%02x%02x%02x%02x%02x%02x\n",
            uuid[0], uuid[1], uuid[2], uuid[3], 
            uuid[4], uuid[5], uuid[6], uuid[7], 
            uuid[8], uuid[9], uuid[10], uuid[11], 
            uuid[12], uuid[13], uuid[14], uuid[15]);
        return (!memcmp(uuid, mock_uuid, sizeof(uuid)));
    }

    status_t MockCryptoFactory::createPlugin(const uint8_t uuid[16], const void *data,
                                             size_t size, CryptoPlugin **plugin)
    {
      ALOGV("MockCryptoFactory::createPlugin - Enter\n");
      *plugin = new MockCryptoPlugin();
      return OK;
    }


    // MockDrmPlugin methods

    status_t MockDrmPlugin::openSession(Vector<uint8_t> &sessionId)
    {
      ALOGV("MockDrmPlugin::openSession - Enter\n");
      const size_t kSessionIdSize = 8;

      Mutex::Autolock lock(mLock);
      for (size_t i = 0; i < kSessionIdSize / sizeof(long); i++) {
        long r = random();
        sessionId.appendArray((uint8_t *)&r, sizeof(long));
      }
      mSessions.add(sessionId);
      
      ALOGD("MockDrmPlugin::openSession() -> %s\n", vectorToString(sessionId).string());
      return OK;
    }

    status_t MockDrmPlugin::closeSession(Vector<uint8_t> const &sessionId)
    {
      ALOGV("MockDrmPlugin::closeSession - Enter\n");
      Mutex::Autolock lock(mLock);
      ALOGD("MockDrmPlugin::closeSession(%s)\n", vectorToString(sessionId).string());
      ssize_t index = findSession(sessionId);
      if (index == kNotFound) {
        ALOGD("Invalid sessionId\n");
        return BAD_VALUE;
      }
      mSessions.removeAt(index);
      return OK;
    }


    status_t MockDrmPlugin::getKeyRequest(Vector<uint8_t> const &sessionId,
                                          Vector<uint8_t> const &initData,
                                          String8 const &mimeType, KeyType keyType,
                                          KeyedVector<String8, String8> const &optionalParameters,
                                          Vector<uint8_t> &request, String8 &defaultUrl)
    {
      Mutex::Autolock lock(mLock);
      ALOGD("MockDrmPlugin::getKeyRequest(sessionId=%s, initData=%s, mimeType=%s"
            ", keyType=%d, optionalParameters=%s))\n",
            vectorToString(sessionId).string(), vectorToString(initData).string(), mimeType.string(),
            keyType, stringMapToString(optionalParameters).string());
      
      ssize_t index = findSession(sessionId);
      if (index == kNotFound) {
        ALOGD("Invalid sessionId\n");
        return BAD_VALUE;
      }
      
      // Properties used in mock test, set by mock plugin and verifed cts test app
      //   byte[] initData           -> mock-initdata
      //   string mimeType           -> mock-mimetype
      //   string keyType            -> mock-keytype
      //   string optionalParameters -> mock-optparams formatted as {key1,value1},{key2,value2}
      
      mByteArrayProperties.add(String8("mock-initdata"), initData);
      mStringProperties.add(String8("mock-mimetype"), mimeType);
      
      String8 keyTypeStr;
      keyTypeStr.appendFormat("%d", (int)keyType);
      mStringProperties.add(String8("mock-keytype"), keyTypeStr);
      
      String8 params;
      for (size_t i = 0; i < optionalParameters.size(); i++) {
        params.appendFormat("%s{%s,%s}", i ? "," : "",
                            optionalParameters.keyAt(i).string(),
                            optionalParameters.valueAt(i).string());
      }
      mStringProperties.add(String8("mock-optparams"), params);
      
      // Properties used in mock test, set by cts test app returned from mock plugin
      //   byte[] mock-request       -> request
      //   string mock-default-url   -> defaultUrl
      
      index = mByteArrayProperties.indexOfKey(String8("mock-request"));
      if (index < 0) {
        ALOGD("Missing 'mock-request' parameter for mock\n");
        return BAD_VALUE;
      } else {
        request = mByteArrayProperties.valueAt(index);
      }
      
      index = mStringProperties.indexOfKey(String8("mock-defaultUrl"));
      if (index < 0) {
        ALOGD("Missing 'mock-defaultUrl' parameter for mock\n");
        return BAD_VALUE;
      } else {
        defaultUrl = mStringProperties.valueAt(index);
      }
      return OK;
    }

    status_t MockDrmPlugin::provideKeyResponse(Vector<uint8_t> const &sessionId,
                                               Vector<uint8_t> const &response,
                                               Vector<uint8_t> &keySetId)
    {
      Mutex::Autolock lock(mLock);
      ALOGD("MockDrmPlugin::provideKeyResponse(sessionId=%s, response=%s)\n",
            vectorToString(sessionId).string(), vectorToString(response).string());
      ssize_t index = findSession(sessionId);
      if (index == kNotFound) {
        ALOGD("Invalid sessionId\n");
        return BAD_VALUE;
      }
      if (response.size() == 0) {
        return BAD_VALUE;
      }
      
      // Properties used in mock test, set by mock plugin and verifed cts test app
      //   byte[] response            -> mock-response
      mByteArrayProperties.add(String8("mock-response"), response);
      
      const size_t kKeySetIdSize = 8;
      
      for (size_t i = 0; i < kKeySetIdSize / sizeof(long); i++) {
        long r = random();
        keySetId.appendArray((uint8_t *)&r, sizeof(long));
      }
      mKeySets.add(keySetId);
      
      return OK;
    }

    status_t MockDrmPlugin::removeKeys(Vector<uint8_t> const &keySetId)
    {
      Mutex::Autolock lock(mLock);
      ALOGD("MockDrmPlugin::removeKeys(keySetId=%s)\n",
            vectorToString(keySetId).string());

      ssize_t index = findKeySet(keySetId);
      if (index == kNotFound) {
        ALOGD("Invalid keySetId\n");
        return BAD_VALUE;
      }
      mKeySets.removeAt(index);

      return OK;
    }

    status_t MockDrmPlugin::restoreKeys(Vector<uint8_t> const &sessionId,
                                        Vector<uint8_t> const &keySetId)
    {
      Mutex::Autolock lock(mLock);
      ALOGD("MockDrmPlugin::restoreKeys(sessionId=%s, keySetId=%s)\n",
            vectorToString(sessionId).string(),
            vectorToString(keySetId).string());
      ssize_t index = findSession(sessionId);
      if (index == kNotFound) {
        ALOGD("Invalid sessionId\n");
        return BAD_VALUE;
      }

      index = findKeySet(keySetId);
      if (index == kNotFound) {
        ALOGD("Invalid keySetId\n");
        return BAD_VALUE;
      }

      return OK;
    }

    status_t MockDrmPlugin::queryKeyStatus(Vector<uint8_t> const &sessionId,
                                               KeyedVector<String8, String8> &infoMap) const
    {
      ALOGD("MockDrmPlugin::queryKeyStatus(sessionId=%s)\n",
            vectorToString(sessionId).string());

      ssize_t index = findSession(sessionId);
      if (index == kNotFound) {
        ALOGD("Invalid sessionId\n");
        return BAD_VALUE;
      }

      infoMap.add(String8("purchaseDuration"), String8("1000"));
      infoMap.add(String8("licenseDuration"), String8("100"));
      return OK;
    }

    status_t MockDrmPlugin::getProvisionRequest(Vector<uint8_t> &request,
                                                String8 &defaultUrl)
    {
      Mutex::Autolock lock(mLock);
      ALOGD("MockDrmPlugin::getProvisionRequest()\n");

      // Properties used in mock test, set by cts test app returned from mock plugin
      //   byte[] mock-request       -> request
      //   string mock-default-url   -> defaultUrl

      ssize_t index = mByteArrayProperties.indexOfKey(String8("mock-request"));
      if (index < 0) {
        ALOGD("Missing 'mock-request' parameter for mock\n");
        return BAD_VALUE;
      } else {
        request = mByteArrayProperties.valueAt(index);
      }

      index = mStringProperties.indexOfKey(String8("mock-defaultUrl"));
      if (index < 0) {
        ALOGD("Missing 'mock-defaultUrl' parameter for mock\n");
        return BAD_VALUE;
      } else {
        defaultUrl = mStringProperties.valueAt(index);
      }
      return OK;
    }

    status_t MockDrmPlugin::provideProvisionResponse(Vector<uint8_t> const &response)
    {
      Mutex::Autolock lock(mLock);
      ALOGD("MockDrmPlugin::provideProvisionResponse(%s)\n",
            vectorToString(response).string());

      // Properties used in mock test, set by mock plugin and verifed cts test app
      //   byte[] response            -> mock-response

      mByteArrayProperties.add(String8("mock-response"), response);
      return OK;
    }

    status_t MockDrmPlugin::getSecureStops(List<Vector<uint8_t> > &secureStops)
    {
      Mutex::Autolock lock(mLock);
      ALOGD("MockDrmPlugin::getSecureStops()\n");

      // Properties used in mock test, set by cts test app returned from mock plugin
      //   byte[] mock-secure-stop1  -> first secure stop in list
      //   byte[] mock-secure-stop2  -> second secure stop in list

      Vector<uint8_t> ss1, ss2;
      ssize_t index = mByteArrayProperties.indexOfKey(String8("mock-secure-stop1"));
      if (index < 0) {
        ALOGD("Missing 'mock-secure-stop1' parameter for mock\n");
        return BAD_VALUE;
      } else {
        ss1 = mByteArrayProperties.valueAt(index);
      }

      index = mByteArrayProperties.indexOfKey(String8("mock-secure-stop2"));
      if (index < 0) {
        ALOGD("Missing 'mock-secure-stop2' parameter for mock\n");
        return BAD_VALUE;
      } else {
        ss2 = mByteArrayProperties.valueAt(index);
      }

      secureStops.push_back(ss1);
      secureStops.push_back(ss2);
      return OK;
    }

  status_t MockDrmPlugin::releaseSecureStops(Vector<uint8_t> const &ssRelease)
  {
    Mutex::Autolock lock(mLock);
    ALOGD("MockDrmPlugin::releaseSecureStops(%s)\n",
          vectorToString(ssRelease).string());

    // Properties used in mock test, set by mock plugin and verifed cts test app
    //   byte[] secure-stop-release  -> mock-ssrelease
    mByteArrayProperties.add(String8("mock-ssrelease"), ssRelease);

    return OK;
  }

  status_t MockDrmPlugin::getPropertyString(String8 const &name, String8 &value) const
  {
    ALOGD("MockDrmPlugin::getPropertyString(name=%s)\n", name.string());
    ssize_t index = mStringProperties.indexOfKey(name);
    if (index < 0) {
      ALOGD("no property for '%s'\n", name.string());
      return BAD_VALUE;
    }
    value = mStringProperties.valueAt(index);
    return OK;
  }

  status_t MockDrmPlugin::getPropertyByteArray(String8 const &name,
                                               Vector<uint8_t> &value) const
  {
    ALOGD("MockDrmPlugin::getPropertyByteArray(name=%s)\n", name.string());
    ssize_t index = mByteArrayProperties.indexOfKey(name);
    if (index < 0) {
      ALOGD("no property for '%s'\n", name.string());
      return BAD_VALUE;
    }
    value = mByteArrayProperties.valueAt(index);
    return OK;
  }

  status_t MockDrmPlugin::setPropertyString(String8 const &name,
                                            String8 const &value)
  {
    Mutex::Autolock lock(mLock);
    ALOGD("MockDrmPlugin::setPropertyString(name=%s, value=%s)\n",
          name.string(), value.string());

    if (name == "mock-send-event") {
      unsigned code, extra;
      sscanf(value.string(), "%d %d", &code, &extra);
      DrmPlugin::EventType eventType = (DrmPlugin::EventType)code;

      Vector<uint8_t> const *pSessionId = NULL;
      ssize_t index = mByteArrayProperties.indexOfKey(String8("mock-event-session-id"));
      if (index >= 0) {
        pSessionId = &mByteArrayProperties[index];
      }

      Vector<uint8_t> const *pData = NULL;
      index = mByteArrayProperties.indexOfKey(String8("mock-event-data"));
      if (index >= 0) {
        pData = &mByteArrayProperties[index];
      }
      ALOGD("sending event from mock drm plugin: %d %d %s %s\n",
            (int)code, extra, pSessionId ? vectorToString(*pSessionId) : "{}",
            pData ? vectorToString(*pData) : "{}");

      sendEvent(eventType, extra, pSessionId, pData);
    } else {
      mStringProperties.add(name, value);
    }
    return OK;
  }

  status_t MockDrmPlugin::setPropertyByteArray(String8 const &name,
                                               Vector<uint8_t> const &value)
  {
    Mutex::Autolock lock(mLock);
    ALOGD("MockDrmPlugin::setPropertyByteArray(name=%s, value=%s)",
          name.string(), vectorToString(value).string());
    mByteArrayProperties.add(name, value);
    return OK;
  }

  status_t MockDrmPlugin::setCipherAlgorithm(Vector<uint8_t> const &sessionId,
                                             String8 const &algorithm)
  {
    Mutex::Autolock lock(mLock);

    ALOGD("MockDrmPlugin::setCipherAlgorithm(sessionId=%s, algorithm=%s)\n",
          vectorToString(sessionId).string(), algorithm.string());

    ssize_t index = findSession(sessionId);
    if (index == kNotFound) {
      ALOGD("Invalid sessionId\n");
      return BAD_VALUE;
    }

    if (algorithm == "AES/CBC/NoPadding") {
      return OK;
    }
    return BAD_VALUE;
  }

  status_t MockDrmPlugin::setMacAlgorithm(Vector<uint8_t> const &sessionId,
                                          String8 const &algorithm)
  {
    Mutex::Autolock lock(mLock);

    ALOGD("MockDrmPlugin::setMacAlgorithm(sessionId=%s, algorithm=%s)\n",
          vectorToString(sessionId).string(), algorithm.string());

    ssize_t index = findSession(sessionId);
    if (index == kNotFound) {
      ALOGD("Invalid sessionId");
      return BAD_VALUE;
    }

    if (algorithm == "HmacSHA256") {
      return OK;
    }
    return BAD_VALUE;
  }

  status_t MockDrmPlugin::encrypt(Vector<uint8_t> const &sessionId,
                                  Vector<uint8_t> const &keyId,
                                  Vector<uint8_t> const &input,
                                  Vector<uint8_t> const &iv,
                                  Vector<uint8_t> &output)
  {
    Mutex::Autolock lock(mLock);
    ALOGD("MockDrmPlugin::encrypt(sessionId=%s, keyId=%s, input=%s, iv=%s)\n",
          vectorToString(sessionId).string(),
          vectorToString(keyId).string(),
          vectorToString(input).string(),
          vectorToString(iv).string());

    ssize_t index = findSession(sessionId);
    if (index == kNotFound) {
      ALOGD("Invalid sessionId\n");
      return BAD_VALUE;
    }

    // Properties used in mock test, set by mock plugin and verifed cts test app
    //   byte[] keyId              -> mock-keyid
    //   byte[] input              -> mock-input
    //   byte[] iv                 -> mock-iv
    mByteArrayProperties.add(String8("mock-keyid"), keyId);
    mByteArrayProperties.add(String8("mock-input"), input);
    mByteArrayProperties.add(String8("mock-iv"), iv);

    // Properties used in mock test, set by cts test app returned from mock plugin
    //   byte[] mock-output        -> output
    index = mByteArrayProperties.indexOfKey(String8("mock-output"));
    if (index < 0) {
      ALOGD("Missing 'mock-request' parameter for mock\n");
      return BAD_VALUE;
    } else {
      output = mByteArrayProperties.valueAt(index);
    }
    return OK;
  }

  status_t MockDrmPlugin::decrypt(Vector<uint8_t> const &sessionId,
                                  Vector<uint8_t> const &keyId,
                                  Vector<uint8_t> const &input,
                                  Vector<uint8_t> const &iv,
                                  Vector<uint8_t> &output)
  {
    Mutex::Autolock lock(mLock);
    ALOGD("MockDrmPlugin::decrypt(sessionId=%s, keyId=%s, input=%s, iv=%s)\n",
          vectorToString(sessionId).string(),
          vectorToString(keyId).string(),
          vectorToString(input).string(),
          vectorToString(iv).string());

    ssize_t index = findSession(sessionId);
    if (index == kNotFound) {
      ALOGD("Invalid sessionId\n");
      return BAD_VALUE;
    }

    // Properties used in mock test, set by mock plugin and verifed cts test app
    //   byte[] keyId              -> mock-keyid
    //   byte[] input              -> mock-input
    //   byte[] iv                 -> mock-iv
    mByteArrayProperties.add(String8("mock-keyid"), keyId);
    mByteArrayProperties.add(String8("mock-input"), input);
    mByteArrayProperties.add(String8("mock-iv"), iv);

    // Properties used in mock test, set by cts test app returned from mock plugin
    //   byte[] mock-output        -> output
    index = mByteArrayProperties.indexOfKey(String8("mock-output"));
    if (index < 0) {
      ALOGD("Missing 'mock-request' parameter for mock\n");
      return BAD_VALUE;
    } else {
      output = mByteArrayProperties.valueAt(index);
    }
    return OK;
  }

  status_t MockDrmPlugin::sign(Vector<uint8_t> const &sessionId,
                               Vector<uint8_t> const &keyId,
                               Vector<uint8_t> const &message,
                               Vector<uint8_t> &signature)
  {
    Mutex::Autolock lock(mLock);
    ALOGD("MockDrmPlugin::sign(sessionId=%s, keyId=%s, message=%s)\n",
          vectorToString(sessionId).string(),
          vectorToString(keyId).string(),
          vectorToString(message).string());

    ssize_t index = findSession(sessionId);
    if (index == kNotFound) {
      ALOGD("Invalid sessionId\n");
      return BAD_VALUE;
    }

    // Properties used in mock test, set by mock plugin and verifed cts test app
    //   byte[] keyId              -> mock-keyid
    //   byte[] message            -> mock-message
    mByteArrayProperties.add(String8("mock-keyid"), keyId);
    mByteArrayProperties.add(String8("mock-message"), message);

    // Properties used in mock test, set by cts test app returned from mock plugin
    //   byte[] mock-signature        -> signature
    index = mByteArrayProperties.indexOfKey(String8("mock-signature"));
    if (index < 0) {
      ALOGD("Missing 'mock-request' parameter for mock\n");
      return BAD_VALUE;
    } else {
      signature = mByteArrayProperties.valueAt(index);
    }
    return OK;
  }

  status_t MockDrmPlugin::verify(Vector<uint8_t> const &sessionId,
                                 Vector<uint8_t> const &keyId,
                                 Vector<uint8_t> const &message,
                                 Vector<uint8_t> const &signature,
                                 bool &match)
  {
    Mutex::Autolock lock(mLock);
    ALOGD("MockDrmPlugin::verify(sessionId=%s, keyId=%s, message=%s, signature=%s)\n",
          vectorToString(sessionId).string(),
          vectorToString(keyId).string(),
          vectorToString(message).string(),
          vectorToString(signature).string());

    ssize_t index = findSession(sessionId);
    if (index == kNotFound) {
      ALOGD("Invalid sessionId\n");
      return BAD_VALUE;
    }

    // Properties used in mock test, set by mock plugin and verifed cts test app
    //   byte[] keyId              -> mock-keyid
    //   byte[] message            -> mock-message
    //   byte[] signature          -> mock-signature
    mByteArrayProperties.add(String8("mock-keyid"), keyId);
    mByteArrayProperties.add(String8("mock-message"), message);
    mByteArrayProperties.add(String8("mock-signature"), signature);

    // Properties used in mock test, set by cts test app returned from mock plugin
    //   String mock-match "1" or "0"         -> match
    index = mStringProperties.indexOfKey(String8("mock-match"));
    if (index < 0) {
      ALOGD("Missing 'mock-request' parameter for mock\n");
      return BAD_VALUE;
    } else {
      match = atol(mStringProperties.valueAt(index).string());
    }
    return OK;
  }

  ssize_t MockDrmPlugin::findSession(Vector<uint8_t> const &sessionId) const
  {
    ALOGD("findSession: nsessions=%d, size=%d\n", mSessions.size(), sessionId.size());
    for (size_t i = 0; i < mSessions.size(); ++i) {
      if (memcmp(mSessions[i].array(), sessionId.array(), sessionId.size()) == 0) {
        return i;
      }
    }
    return kNotFound;
  }

  ssize_t MockDrmPlugin::findKeySet(Vector<uint8_t> const &keySetId) const
  {
    ALOGD("findKeySet: nkeySets=%d, size=%d\n", mKeySets.size(), keySetId.size());
    for (size_t i = 0; i < mKeySets.size(); ++i) {
      if (memcmp(mKeySets[i].array(), keySetId.array(), keySetId.size()) == 0) {
        return i;
      }
    }
    return kNotFound;
  }


  // Conversion utilities
  String8 MockDrmPlugin::vectorToString(Vector<uint8_t> const &vector) const
  {
    return arrayToString(vector.array(), vector.size());
  }

  String8 MockDrmPlugin::arrayToString(uint8_t const *array, size_t len) const
  {
    String8 result("{ ");
    for (size_t i = 0; i < len; i++) {
      result.appendFormat("0x%02x ", array[i]);
    }
    result += "}";
    return result;
  }

  String8 MockDrmPlugin::stringMapToString(KeyedVector<String8, String8> map) const
  {
    String8 result("{ ");
    for (size_t i = 0; i < map.size(); i++) {
      result.appendFormat("%s{name=%s, value=%s}", i > 0 ? ", " : "",
                          map.keyAt(i).string(), map.valueAt(i).string());
    }
    return result + " }";
  }

  bool operator<(Vector<uint8_t> const &lhs, Vector<uint8_t> const &rhs) {
    return lhs.size() < rhs.size() || (memcmp(lhs.array(), rhs.array(), lhs.size()) < 0);
  }

  //
  // Crypto Plugin
  //

  bool MockCryptoPlugin::requiresSecureDecoderComponent(const char *mime) const
  {
    ALOGD("MockCryptoPlugin::requiresSecureDecoderComponent(mime=%s)\n", mime);
    return false;
  }

  ssize_t
  MockCryptoPlugin::decrypt(bool secure, const uint8_t key[16], const uint8_t iv[16],
                            Mode mode, const void *srcPtr, const SubSample *subSamples,
                            size_t numSubSamples, void *dstPtr, AString *errorDetailMsg)
  {
    ALOGD("MockCryptoPlugin::decrypt(secure=%d, key=%s, iv=%s, mode=%d, src=%p, "
          "subSamples=%s, dst=%p)\n",
          (int)secure,
          arrayToString(key, sizeof(key)).string(),
          arrayToString(iv, sizeof(iv)).string(),
          (int)mode, srcPtr,
          subSamplesToString(subSamples, numSubSamples).string(),
          dstPtr);
    return OK;
  }

  // Conversion utilities
  String8 MockCryptoPlugin::arrayToString(uint8_t const *array, size_t len) const
  {
    String8 result("{ ");
    for (size_t i = 0; i < len; i++) {
      result.appendFormat("0x%02x ", array[i]);
    }
    result += "}";
    return result;
  }

  String8 MockCryptoPlugin::subSamplesToString(SubSample const *subSamples,
                                               size_t numSubSamples) const
  {
    String8 result;
    for (size_t i = 0; i < numSubSamples; i++) {
      result.appendFormat("[%d] {clear:%d, encrypted:%d} ", i,
                          subSamples[i].mNumBytesOfClearData,
                          subSamples[i].mNumBytesOfEncryptedData);
    }
    return result;
  }

};

// Local Variables:  
// tab-width: 8
// mode: C++         
// End:              
