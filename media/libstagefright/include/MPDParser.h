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

#ifndef MPD_PARSER_H_
#define MPD_PARSER_H_

namespace android
{
  
  class NuPlayer::MPDParser : public RefBase
  {
    MPDParser(const char *baseURI, const void *data, size_t size);

    status_t initCheck() const;
    
    bool isComplete() const;

    sp<AMessage> meta();

    size_t size();
    bool itemAt(size_t index, AString *uri, sp<AMessage> *meta = NULL);

  protected:
    virtual ~MPDParser();

  private:

    typedef enum
    {
      MPD_FILE_TYPE_STATIC,
      MPD_FILE_TYPE_DYNAMIC
    } MPDFileType;

    struct Item {
      AString mURI;
      sp<AMessage> mMeta;
    };

    struct MPDNode {
      char *default_namespace;
      char *namespace_xsi;
      char *namespace_ext;
      char *schemaLocation;
      char *id;
      char *profiles;

      MPDFileType type;
      time_t availabilityStartTime;
      time_t availabilityEndTime;
      
      int64_t mediaPresentationDuration;  /* [ms] */
      int64_t minimumUpdatePeriod;        /* [ms] */
      int64_t minBufferTime;              /* [ms] */
      int64_t timeShiftBufferDepth;       /* [ms] */
      int64_t suggestedPresentationDelay; /* [ms] */
      int64_t maxSegmentDuration;         /* [ms] */
      int64_t maxSubsegmentDuration;      /* [ms] */

      /* list of BaseURL nodes */
      vector<AString> BaseURLs;

      /* list of Location nodes */
      vector<AString> Locations;

      /* List of ProgramInformation nodes */
      vector<AString> ProgramInfo;

      /* list of Periods nodes */
      vector<AString> Periods;

      /* list of Metrics nodes */
      vector<AString> Metrics;
    };

    status_t mInitCheck;

    AString mBaseURI;
    bool mIsComplete;
    bool mIsEvent;
    
    sp<AMessage> mMeta;
    Vector<Item> mItems;

    status_t parse(const void *data, size_t size);

    DISALLOW_EVIL_CONSTRUCTORS(MPDParser);
  };

}

#endif /* MPD_PARSER_H_ */
