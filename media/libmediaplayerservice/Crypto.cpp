/*
 * Copyright (C) 2012 The Android Open Source Project
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
#define LOG_TAG "Crypto"
#include <utils/Log.h>
#include <dirent.h>
#include <dlfcn.h>

#include "Crypto.h"

#include <media/hardware/CryptoAPI.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/MediaErrors.h>

namespace android {

  KeyedVector<Vector<uint8_t>, String8> Crypto::mUUIDToLibraryPathMap;
  KeyedVector<String8, wp<SharedLibrary> > Crypto::mLibraryPathToOpenLibraryMap;
  Mutex Crypto::mMapLock;

  static bool operator<(const Vector<uint8_t> &lhs, const Vector<uint8_t> &rhs) {
    if (lhs.size() < rhs.size()) {
      return true;
    } else if (lhs.size() > rhs.size()) {
      return false;
    }

    return memcmp((void *)lhs.array(), (void *)rhs.array(), rhs.size()) < 0;
  }

  Crypto::Crypto()
    : mInitCheck(NO_INIT),
      mFactory(NULL),
      mPlugin(NULL) {
  }

  Crypto::~Crypto() {
    delete mPlugin;
    mPlugin = NULL;
    closeFactory();
  }

  void Crypto::closeFactory() {
    delete mFactory;
    mFactory = NULL;
    mLibrary.clear();
  }

  status_t Crypto::initCheck() const {
    return mInitCheck;
  }

  /*
   * Search the plugins directory for a plugin that supports the scheme
   * specified by uuid
   *
   * If found:
   *    mLibrary holds a strong pointer to the dlopen'd library
   *    mFactory is set to the library's factory method
   *    mInitCheck is set to OK
   *
   * If not found:
   *    mLibrary is cleared and mFactory are set to NULL
   *    mInitCheck is set to an error (!OK)
   */
  void Crypto::findFactoryForScheme(const uint8_t uuid[16]) {
    ALOGV("Crypto::findFactoryForScheme - Enter");
    closeFactory();

    // lock static maps
    Mutex::Autolock autoLock(mMapLock);

    ALOGV("Crypto::findFactoryForScheme - searching plugin cache...");
    ALOGV("... for UUID %02x%02x%02x%02x-%02x%02x-%02x%02x-"
	  "%02x%02x-%02x%02x%02x%02x%02x%02x",
	  uuid[0], uuid[1], uuid[2], uuid[3], 
	  uuid[4], uuid[5], uuid[6], uuid[7], 
	  uuid[8], uuid[9], uuid[10], uuid[11], 
	  uuid[12], uuid[13], uuid[14], uuid[15]);

    // first check cache
    Vector<uint8_t> uuidVector;
    uuidVector.appendArray(uuid, sizeof(uuid));
    ssize_t index = mUUIDToLibraryPathMap.indexOfKey(uuidVector);
    if (index >= 0) {
      ALOGV("Crypto::findFactoryForScheme - found a cache entry for this UUID");
      if (loadLibraryForScheme(mUUIDToLibraryPathMap[index], uuid)) {
	mInitCheck = OK;
	return;
      } else {
	ALOGE("Failed to load from cached library path!");
	mInitCheck = ERROR_UNSUPPORTED;
	return;
      }
    }

    ALOGV("Crypto::findFactoryForScheme - No cache entry found !");
    ALOGV("Crypto::findFactoryForScheme - searching a plugin to load ...");
    // no luck, have to search
    String8 dirPath("/vendor/lib/mediadrm");
    String8 pluginPath;

    ALOGV("Crypto::findFactoryForScheme - ... from '%s'", dirPath.string());

    DIR* pDir = opendir(dirPath.string());
    if (pDir) {
      struct dirent* pEntry;
      while ((pEntry = readdir(pDir))) {

	pluginPath = dirPath + "/" + pEntry->d_name;
	ALOGV("Crypto::findFactoryForScheme - Trying file at %s", pluginPath.string());
	if (pluginPath.getPathExtension() == ".so") {

	  ALOGV("Crypto::findFactoryForScheme - Try to load this library and matching UUID ...");
	  ALOGV("... %02x%02x%02x%02x-%02x%02x-%02x%02x-"
		"%02x%02x-%02x%02x%02x%02x%02x%02x",
		uuid[0], uuid[1], uuid[2], uuid[3], 
		uuid[4], uuid[5], uuid[6], uuid[7], 
		uuid[8], uuid[9], uuid[10], uuid[11], 
		uuid[12], uuid[13], uuid[14], uuid[15]);
	  if (loadLibraryForScheme(pluginPath, uuid)) {
	    ALOGV("Crypto::findFactoryForScheme - Plugin found & loaded !!");
	    mUUIDToLibraryPathMap.add(uuidVector, pluginPath);
	    mInitCheck = OK;
	    closedir(pDir);
	    return;
	  }
	}
      }

      closedir(pDir);
    }

    ALOGV("Crypto::findFactoryForScheme - No luck: trying legacy libdrmdecrypt.so");

    // try the legacy libdrmdecrypt.so
    pluginPath = "libdrmdecrypt.so";
    if (loadLibraryForScheme(pluginPath, uuid)) {
      ALOGV("Crypto::findFactoryForScheme - Yes :) ! this is your lucky day !");
      mUUIDToLibraryPathMap.add(uuidVector, pluginPath);
      mInitCheck = OK;
      return;
    }

    ALOGE("Failed to find crypto plugin");
    mInitCheck = ERROR_UNSUPPORTED;
  }

  bool Crypto::loadLibraryForScheme(const String8 &path, const uint8_t uuid[16]) {

    ALOGV("Crypto::loadLibraryForScheme - Enter");

    ALOGV("Crypto::loadLibraryForScheme - path = '%s'", path.string());
    ALOGV("Crypto::loadLibraryForScheme - UUID = '%02x%02x%02x%02x-%02x%02x-%02x%02x-"
	  "%02x%02x-%02x%02x%02x%02x%02x%02x'",
	  uuid[0], uuid[1], uuid[2], uuid[3], 
	  uuid[4], uuid[5], uuid[6], uuid[7], 
	  uuid[8], uuid[9], uuid[10], uuid[11], 
	  uuid[12], uuid[13], uuid[14], uuid[15]);
    
    // get strong pointer to open shared library
    ssize_t index = mLibraryPathToOpenLibraryMap.indexOfKey(path);
    if (index >= 0) {
      mLibrary = mLibraryPathToOpenLibraryMap[index].promote();
    } else {
      index = mLibraryPathToOpenLibraryMap.add(path, NULL);
    }

    ALOGV("Crypto::loadLibraryForScheme - Found library from cache");

    if (!mLibrary.get()) {
      mLibrary = new SharedLibrary(path);
      if (!*mLibrary) {
	ALOGE("Couldn't load library %s", path.string());
	return false;
      }

      mLibraryPathToOpenLibraryMap.replaceValueAt(index, mLibrary);
    }

    ALOGV("Crypto::loadLibraryForScheme - Library loaded");

    typedef CryptoFactory *(*CreateCryptoFactoryFunc)();

    CreateCryptoFactoryFunc createCryptoFactory =
      (CreateCryptoFactoryFunc)mLibrary->lookup("createCryptoFactory");

    if (createCryptoFactory == NULL ||
        (mFactory = createCryptoFactory()) == NULL ||
        !mFactory->isCryptoSchemeSupported(uuid)) {
      ALOGV("Crypto::loadLibraryForScheme - Factory not available");
      closeFactory();
      return false;
    }

    ALOGV("Crypto::loadLibraryForScheme - Plugin for scheme '%02x%02x%02x%02x"
	  "-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x' ready",
	  uuid[0], uuid[1], uuid[2], uuid[3], 
	  uuid[4], uuid[5], uuid[6], uuid[7], 
	  uuid[8], uuid[9], uuid[10], uuid[11], 
	  uuid[12], uuid[13], uuid[14], uuid[15]);
    return true;
  }

  bool Crypto::isCryptoSchemeSupported(const uint8_t uuid[16]) {
    Mutex::Autolock autoLock(mLock);

    if (mFactory && mFactory->isCryptoSchemeSupported(uuid)) {
      return true;
    }

    findFactoryForScheme(uuid);
    ALOGV("Crypto::loadLibraryForScheme - Scheme %s", (mInitCheck == OK ? "SUPPORTED" : "NOT Supported"));
    return (mInitCheck == OK);
  }

  status_t Crypto::createPlugin(
				const uint8_t uuid[16], const void *data, size_t size) {
    Mutex::Autolock autoLock(mLock);

    if (mPlugin != NULL) {
      return -EINVAL;
    }

    if (!mFactory || !mFactory->isCryptoSchemeSupported(uuid)) {
      findFactoryForScheme(uuid);
    }

    if (mInitCheck != OK) {
      return mInitCheck;
    }

    return mFactory->createPlugin(uuid, data, size, &mPlugin);
  }

  status_t Crypto::destroyPlugin() {
    Mutex::Autolock autoLock(mLock);

    if (mInitCheck != OK) {
      return mInitCheck;
    }

    if (mPlugin == NULL) {
      return -EINVAL;
    }

    delete mPlugin;
    mPlugin = NULL;

    return OK;
  }

  bool Crypto::requiresSecureDecoderComponent(const char *mime) const {
    Mutex::Autolock autoLock(mLock);

    if (mInitCheck != OK) {
      return mInitCheck;
    }

    if (mPlugin == NULL) {
      return -EINVAL;
    }

    return mPlugin->requiresSecureDecoderComponent(mime);
  }

  ssize_t Crypto::decrypt(bool secure,
			  const uint8_t key[16],
			  const uint8_t iv[16],
			  CryptoPlugin::Mode mode,
			  const void *srcPtr,
			  const CryptoPlugin::SubSample *subSamples, size_t numSubSamples,
			  void *dstPtr,
			  AString *errorDetailMsg) {
    Mutex::Autolock autoLock(mLock);

    if (mInitCheck != OK) {
      return mInitCheck;
    }

    if (mPlugin == NULL) {
      return -EINVAL;
    }

    return mPlugin->decrypt(
			    secure, key, iv, mode, srcPtr, subSamples, numSubSamples, dstPtr,
			    errorDetailMsg);
  }

}  // namespace android
