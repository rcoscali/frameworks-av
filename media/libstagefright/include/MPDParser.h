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

#include <vector>

using namespace std;

namespace android
{
  
  class MPDParser : public RefBase
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
    /*
     * Stream type
     */
    enum {
      kStreamVideo,           /* video stream (the main one) */
      kStreamAudio,           /* audio stream (optional) */
      kStreamApplication      /* application stream (optional): for timed text/subtitles */
    };

    /*
     * MPD Type
     */
    enum {
      kMpdTypeUninitialized,  /* No MPD@type */
      kMpdTypeStatic,	      /* MPD@type == static */
      kMpdTypeDynamic         /* MPD@type == dynamic */
    };

    /*
     * SAP types
     */
    enum {
      kSapType_0 = 0,
      kSapType_1,
      kSapType_2,
      kSapType_3,
      kSapType_4,
      kSapType_5,
      kSapType_6
    };

    /*
     * Specific ClockTime values
     */
    enum {
      kClockTimeNone = -1
    };

    /*
     * MPDBaseUrl
     */
    class MPDBaseUrl : public RefBase {

    public:
      MPDBaseUrl()
	: mBaseUrl(new AString("")),
	  mServiceLocation(new AString("")),
	  mByteRange(new AString("")) 
      {};

      MPDBaseUrl(const char *baseUrl)
	: mBaseUrl(baseUrl),
	  mServiceLocation(new AString("")),
	  mByteRange(new AString("")) 
      {};

    protected:
      virtual ~MPDBaseUrl() {
	delete mBaseUrl; mBaseUrl = (AString *)NULL;
	delete mServiceLocation; mServiceLocation = (AString *)NULL;
	delete mByteRange; mByteRange = (AString *)NULL;
      };

    private:
      AString *mBaseUrl;
      AString *mServiceLocation;
      AString *mByteRange;
      
      DISALLOW_EVIL_CONSTRUCTORS(MPDBaseUrl);
    };

    /*
     * MPDRange
     */
    class MPDRange : public RefBase {

    public:
      MPDRange()
	: mFirstBytePos((uint64_t)0L),
	  mLastBytePos((uint64_t)0L) 
      {};

      MPDRange(uint64_t firstBytePos, uint64_t lastBytePos)
	: mFirstBytePos(firstBytePos),
	  mLastBytePos(lastBytePos) 
      {};

    protected:
      virtual ~MPDRange() {};

    private:
      uint64_t mFirstBytePos;
      uint64_t mLastBytePos;

      DISALLOW_EVIL_CONSTRUCTORS(MPDRange);
    };

    /*
     * MPDRatio
     */
    class MPDRatio : public RefBase {
    public:
      MPDRatio()
	: mNum(0),
	  mDen(1) 
      {};

      MPDRatio(uint32_t num, uint32_t den)
	: mNum(num),
	  mDen(den) {
	if (mDen == 0)
	  mDen = 1;
      };

    protected:
      virtual ~MPDRatio() {};

    private:
      uint32_t mNum;
      uint32_t mDen;

      DISALLOW_EVIL_CONSTRUCTORS(MPDRatio);
    };

    /*
     * MPDFrameRate
     */
    class MPDFrameRate : public MPDRatio {
    public:
      MPDFrameRate()
	: MPDRatio() 
      {};

      MPDFrameRate(uint32_t num, uint32_t den)
	: MPDRatio(num, den) 
      {};

    protected:
      virtual ~MPDFrameRate() {};

    private:

      DISALLOW_EVIL_CONSTRUCTORS(MPDFrameRate);
    };

    /*
     * MPDConditionalUintType
     */
    class MPDConditionalUintType : public RefBase {
    public:
      MPDConditionalUintType(bool flag, uint32_t value)
	: mFlag(flag),
	  mValue(value) 
      {};

    protected:
      virtual ~MPDConditionalUintType() {};

    private:
      bool mFlag;
      uint32_t mValue;

      DISALLOW_EVIL_CONSTRUCTORS(MPDConditionalUintType);
    };

    /*
     * MPD_SNode
     */
    class MPD_SNode : public RefBase {
    public:
      MPD_SNode(uint64_t t = 0, uint64_t d = 0, uint32_t r = 0)
	: mT(t),
	  mD(d),
	  mR(r) 
      {};

    protected:
      virtual ~MPD_SNode() {};

    private:
      uint64_t mT;
      uint64_t mD;
      uint32_t mR;

      DISALLOW_EVIL_CONSTRUCTORS(MPD_SNode);
    };

    /*
     * MPDSegmentTimelineNode
     */
    class MPDSegmentTimelineNode : public RefBase {
    public:
      MPDSegmentTimelineNode() 
	: mSNodes(new vector<MPD_SNode *>()) 
      {};

    protected:
      virtual ~MPDSegmentTimelineNode() {
	mSNodes.clear();
	delete mSNodes;
	mSNodes = (vector<MPD_SNode> *)NULL;
      };

    private:
      vector<MPD_SNode> *mSNodes;

      DISALLOW_EVIL_CONSTRUCTORS(MPDSegmentTimelineNode);
    };

    /*
     * MPDUrlType
     */
    class MPDUrlType : public RefBase {
    public:
      MPDUrlType(const char *sourceUrl, MPDRange*& range)
	: mSourceUrl(sourceUrl),
	  mRange(range) {
	incStrong(range);
      };

    protected:
      virtual ~MPDUrlType () {
	decStrong(range);
      };

    private:
      AString mSourceUrl;
      MPDRange *mRange;

      DISALLOW_EVIL_CONSTRUCTORS(MPDUrlType);
    };

    /*
     * MPDSegmentBaseType
     */
    class MPDSegmentBaseType : public RefBase {
    public:
      MPDSegmentBaseType() 
	: mTimescale(0),
	  mPresentationTimeOffset(0),
	  mIndexRange(new MPDRange()),
	  mIndexRangeExact(false),
	  mInitialization("", new MPDRange()),
	  mRepresentationIndex("", new MPDRange())

    protected:
      virtual ~MPDSegmentBaseType() {};

    private:
      uint32_t mTimescale;
      uint32_t mPresentationTimeOffset;
      MPDRange &mIndexRange;
      boolean mIndexRangeExact;
      MPDUrlType &mInitialization;
      MPDUrlType &mRepresentationIndex;

      DISALLOW_EVIL_CONSTRUCTORS(MPDSegmentBaseType);
    };

    /*
     * MPDMultSegmentBaseType
     */
    class MPDMultSegmentBaseType : public RefBase {
    public:
      MPDMultSegmentBaseType()
	: mDuration(0),
	  mStartNumber(0),
	  mSegmentBaseType(new MPDSegmentBaseType()),
	  mSegmentTimeline(new MPDSegmentTimelineNode()),
	  mBitstreamSwitching(new MPDUrlType())
      {};

    protected:
      virtual ~MPDMultSegmentBaseType() {};

    private:
      uint32_t mDuration;
      uint32_t mStartNumber;
      MPDSegmentBaseType *mSegmentBaseType;
      MPDSegmentTimelineNode *mSegmentTimeline;
      MPDUrlType *mBitstreamSwitching;

      DISALLOW_EVIL_CONSTRUCTORS(MPDMultSegmentBaseType);
    };

    /*
     * MPDSegmentListNode
     */
    class MPDSegmentListNode : public RefBase {
    public:
      MPDSegmentListNode()
	: mMultSegBaseType(new MPDMultSegmentBaseType()),
	  mSegmentUrlNodes(new vector<AString>())
      {};

    protected:
      virtual ~MPDSegmentListNode() {
	mSegmentUrlNodes.clear();
	delete mSegmentUrlNodes;
	mSegmentUrlNodes = (vector<AString> *)NULL;
      };

    private:
      MPDMultSegmentBaseType *mMultSegBaseType;
      vector<AString> *mSegmentUrlNodes;
      
      DISALLOW_EVIL_CONSTRUCTORS(MPDSegmentListNode);
    };

    /*
     * MPDSegmentTemplateNode
     */
    class MPDSegmentTemplateNode : public RefBase {
      MPDSegmentTemplateNode()
	: mMultSegBaseType(),
	  mMedia(),
	  mIndex(),
	  mInitialization(),
	  mBitstreamSwitching()
      {};

    protected:
      virtual ~MPDSegmentTemplateNode() {};

    private:
      MPDMultSegmentBaseType *mMultSegBaseType;
      AString *mMedia;
      AString *mIndex;
      AString *mInitialization;
      AString *mBitstreamSwitching;

      DISALLOW_EVIL_CONSTRUCTORS(MPDSegmentTemplateNode);
    };

    /*
     * MPDSegmentURLNode
     */
    class MPDSegmentURLNode : public RefBase {
    public:
      MPDSegmentURLNode()
	: mMedia(),
	  mMediaRange(),
	  mIndex(),
	  mIndexRange()
      {};

    protected:
      virtual ~MPDSegmentURLNode() {};

    private:
      AString *mMedia;
      MPDRange *mMediaRange;
      AString *mIndex;
      MPDRange *mIndexRange;

      DISALLOW_EVIL_CONSTRUCTORS(MPDSegmentURLNode);
    };

    /*
     * MPDRepresentationBaseType
     */
    class MPDRepresentationBaseType : public RefBase {
    public:
      AString *mProfiles;
      uint32_t mWidth;
      uint32_t mHeight;
      MPDRatio *mSar;
      MPDFrameRate *mFrameRate;
      AString *mAudioSamplingRate
      AString *mMimeType;
      AString *mSegmentProfiles;
      AString *mCodecs;
      double mMaximumSAPPeriod;
      MPDSapType mStartWithSAP;
      double mMaxPlayoutRate;
      bool mCodingDependency;
      AString *mScanType;
      vector<AString> *mFramePacking;
      vector<AString> *mAudioChannelConfiguration;
      vector<AString> *mContentProtection;

    private:

      DISALLOW_EVIL_CONSTRUCTORS(MPDRepresentationBaseType);
    };

    /*
     * MPDSubRepresentationNode
     */
    class MPDSubRepresentationNode : public RefBase {
    public:
      MPDRepresentationBaseType *mRepresentationBase;
      uint32_t mLevel;
      vector<uint32_t> *mDependencyLevel;
      uint32_t mSize;
      uint32_t mBandwidth;
      vector<AString> *mContentComponent;

    private:

      DISALLOW_EVIL_CONSTRUCTORS(MPDSubRepresentationNode);
    };

    /*
     * MPDRepresentationNode
     */
    class MPDRepresentationNode : public RefBase {
    public:
        gchar *id;
      uint32_t mBandwidth;
      uint32_t mQualityRanking;
      vector<AString> *mDependencyId;              /* StringVectorType */
      vector<AString> *mMediaStreamStructureId;    /* StringVectorType */
      /* RepresentationBase extension */
      MPDRepresentationBaseType *mRepresentationBase;
      /* list of BaseURL nodes */
      vector<AString> *mBaseURLs;
      /* list of SubRepresentation nodes */
      vector<AString> *mSubRepresentations;
      /* SegmentBase node */
      MPDSegmentBaseType *mSegmentBase;
      /* SegmentTemplate node */
      MPDSegmentTemplateNode *mSegmentTemplate;
      /* SegmentList node */
      MPDSegmentListNode *mSegmentList;

    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDRepresentationNode);
    };

    /*
     * MPDDescriptorType
     */
    class MPDDescriptorType : public RefBase {
    public:
      AString mSchemeIdUri;
      AString mValue;

    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDDescriptorType);
    };

    /*
     * MPDContentComponentNode
     */
    class MPDContentComponentNode : public RefBase {
    public:
      uint32_t mId;
      vector<AString> mLang;                      /* LangVectorType RFC 5646 */
      AString *mContentType;
      MPDRatio *mPar;
      /* list of Accessibility DescriptorType nodes */
      vector<MPDDescriptorType> *mAccessibility;
      /* list of Role DescriptorType nodes */
      vector<MPDDescriptorType> *mRole;
      /* list of Rating DescriptorType nodes */
      vector<MPDDescriptorType> *mRating;
      /* list of Viewpoint DescriptorType nodes */
      vector<MPDDescriptorType> *mViewpoint;

    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDContentComponentNode);
    };

    /*
     * MPDAdaptationSetNode
     */
    class MPDAdaptationSetNode : public RefBase {
    public:

      uint32_t mId;
      uint32_t mGroup;
      AString mLang;                      /* LangVectorType RFC 5646 */
      AString mContentType;
      MPDRatio *mPar;
      uint32_t mMinBandwidth;
      uint32_t mMaxBandwidth;
      uint32_t mMinWidth;
      uint32_t mMaxWidth;
      uint32_t mMinHeight;
      uint32_t mMaxHeight;
      MPDFrameRate *mMinFrameRate;
      MPDFrameRate *mMaxFrameRate;
      MPDConditionalUintType *mSegmentAlignment;
      MPDConditionalUintType *mSubsegmentAlignment;
      MPDSAPType mSubsegmentStartsWithSAP;
      bool mBitstreamSwitching;
      /* list of Accessibility DescriptorType nodes */
      vector<MPDDescriptorType> *mAccessibility;
      /* list of Role DescriptorType nodes */
      vector<MPDDescriptorType> *mRole;
      /* list of Rating DescriptorType nodes */
      vector<MPDDescriptorType> *mRating;
      /* list of Viewpoint DescriptorType nodes */
      vector<MPDDescriptorType> *mViewpoint;
      /* RepresentationBase extension */
      MPDRepresentationBaseType *mRepresentationBase;
      /* SegmentBase node */
      MPDSegmentBaseType *mSegmentBase;
      /* SegmentList node */
      MPDSegmentListNode *mSegmentList;
      /* SegmentTemplate node */
      MPDSegmentTemplateNode *mSegmentTemplate;
      /* list of BaseURL nodes */
      vector<MPDBaseUrl> *mBaseURLs;
      /* list of Representation nodes */
      vector<MPDRepresentationNode> *mRepresentations;
      /* list of ContentComponent nodes */
      vector<MPDContentComponentNode> *mContentComponents;
      
    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDAdaptationSetNode);
    };

    /*
     * MPDSubsetNode
     */
    class MPDSubsetNode : public RefBase {
    public:
      vector<uint32_t> mContains;                   /* UIntVectorType */
      uint32_t mSize;                               /* size of the "contains" array */

    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDSubsetNode);
    };

    /*
     * MPDPeriodNode
     */
    class MPDPeriodNode : public RefBase {
    public:
      AString mId;
      int64_t mStart;                      /* [ms] */
      int64_t mDuration;                   /* [ms] */
      bool mBitstreamSwitching;
      /* SegmentBase node */
      MPDSegmentBaseType *mSegmentBase;
      /* SegmentList node */
      MPDSegmentListNode *mSegmentList;
      /* SegmentTemplate node */
      MPDSegmentTemplateNode *mSegmentTemplate;
      /* list of Adaptation Set nodes */
      vector<MPDAdaptationSetNode> AdaptationSets;
      /* list of Representation nodes */
      vector<MPDRepresentationNode> mSubsets;
      /* list of BaseURL nodes */
      vector<MPDBaseUrl> mBaseURLs;

    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDPeriodNode);
    };

    /*
     * MPDProgramInformationNode
     */
    class MPDProgramInformationNode : public RefBase {
    public:
      AString mLang;                      /* LangVectorType RFC 5646 */
      AString mMoreInformationURL;
      /* children nodes */
      AString mTitle;
      AString mSource;
      AString mCopyright;
      
    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDProgramInformationNode);
    };

    /*
     * MPDMetricsRangeNode
     */
    class MPDMetricsRangeNode : public RefBase {
    public:
      int64_t mStarttime;                  /* [ms] */
      int64_t mDuration;                   /* [ms] */
      
    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDMetricsRangeNode);
    };

    /*
     * MPDMetricsNode
     */
    class MPDMetricsNode : public RefBase {
    public:
      AString mMetrics;
      /* list of Metrics Range nodes */
      vector<MPDMetricsRangeNode> mMetricsRanges;
      /* list of Reporting nodes */
      vector<MPDDescriptorType> mReportings;
      
    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDMetricsNode);
    };

    /*
     * MPDDateTime
     */
    class MPDDateTime :  public RefBase {

    public:
      MPDDateTime()
	: mYear(0),
	  mMonth(0),
	  mDay(0),
	  mHour(0),
	  mMinute(0),
	  mSecond(0)
      {};
      MPDDateTime(uint16_t year, uint8_t month, uint8t day, 
		  uint8t hour, uint8t minute, uint8t second)
	: mYear(year),
	  mMonth(month),
	  mDay(day),
	  mHour(hour),
	  mMinute(minute),
	  mSecond(second)
      {};

      void clear()
      {
	mYear = 0;
	mMonth = mDay = mHour = mMinute = mSecond = 0;
      };

    protected:
      virtual ~MPDDateTime() {};

    private:
      uint16_t mYear;
      uint8_t mMonth;
      uint8_t mDay;
      uint8_t mHour;
      uint8_t mMinute;
      uint8_t mSecond;

      DISALLOW_EVIL_CONSTRUCTORS(MPDDateTime);
    };

    /*
     * MPDMpdNode
     */
    class MPDMpdNode : public RefBase {
    public:
      MPDMpdNode() 
	: mDefault_namespace(""),
	  mNamespace_xsi(""),
	  mNamespace_ext(""),
	  mSchemaLocation(""),
	  mId(""),
	  mProfiles(""),
	  mType(""),
	  mMediaPresentationDuration(0L),
	  mMinimumUpdatePeriod(0L),
	  mMinBufferTime(0L),
	  mTimeShiftBufferDepth(0L),
	  mSuggestedPresentationDelay(0L),
	  mMaxSegmentDuration(0L),
	  mMaxSubsegmentDuration(0L),
	  mBaseURLs(new vector<MPDBaseUrl>()),
	  mLocations(new vector<AString>()),
	  mProgramInfo(new vector<MPDProgramInformationNode>()),
	  mPeriods(new vector<MPDPeriodNode>()),
	  mMetrics(new vector<MPDMetricsNode>())
      {	 
	mAvailabilityStartTime.clear();
	mAvailabilityEndTime.clear();
      };

      AString mDefault_namespace;
      AString mNamespace_xsi;
      AString mNamespace_ext;
      AString mSchemaLocation;
      AString mId;
      AString mProfiles;
      AString mType;
      MPDDateTime mAvailabilityStartTime;
      MPDDateTime mAvailabilityEndTime;
      int64_t mMediaPresentationDuration;  /* [ms] */
      int64_t mMinimumUpdatePeriod;        /* [ms] */
      int64_t mMinBufferTime;              /* [ms] */
      int64_t mTimeShiftBufferDepth;       /* [ms] */
      int64_t mSuggestedPresentationDelay; /* [ms] */
      int64_t mMaxSegmentDuration;         /* [ms] */
      int64_t mMaxSubsegmentDuration;      /* [ms] */
      /* list of BaseURL nodes */
      vector<MPDBaseUrl> *mBaseURLs;
      /* list of Location nodes */
      vector<AString> *mLocations;
      /* List of ProgramInformation nodes */
      vector<MPDProgramInformationNode> *mProgramInfo;
      /* list of Periods nodes */
      vector<MPDPeriodNode> *mPeriods;
      /* list of Metrics nodes */
      vector<MPDMetricsNode> *mMetrics;
      
    protected:
      virtual ~MPDMpdNode() {
	if (mDefault_namespace) {
	  mDefault_namespace.clear(); delete mDefault_namespace; 
	  mDefault_namespace = (AString *)NULL;
	}
	if (mNamespace_xsi) {
	  mNamespace_xsi.clear(); delete mNamespace_xsi;
	  mNamespace_xsi = (AString *)NULL;
	}
	if (mNamespace_ext) {
	  mNamespace_ext.clear(); delete mNamespace_ext;
	  mNamespace_ext = (AString *)NULL;
	}
	if (mSchemaLocation) {
	  mSchemaLocation.clear(); delete mSchemaLocation;
	  mSchemaLocation = (AString *)NULL;
	}
	if (mId) {
	  mId.clear(); delete mId;
	  mId = (AString *)NULL;
	}
	if (mProfiles) {
	  mProfiles.clear(); delete mProfiles;
	  mProfiles = (AString *)NULL;
	}
	if (mType) {
	  mType.clear(); delete mType;
	  mType = (AString *)NULL;
	}
	mAvailabilityStartTime.clear();
	mAvailabilityEndTime.clear();
	mMediaPresentationDuration = 0L;
	mMinimumUpdatePeriod = 0L;
	mMinBufferTime = 0L;
	mTimeShiftBufferDepth = 0L;
	mSuggestedPresentationDelay = 0L;
	mMaxSegmentDuration = 0L;
	mMaxSubsegmentDuration = 0L;
	if (mBaseURLs) {
	  mBaseURLs->clear(); delete mBaseURLs; mBaseURLs = (vector<MPDBaseUrl> *)NULL;
	}
	if (mLocations) {
	  mLocations->clear(); delete mLocations; mLocations = (vector<AString> *)NULL;
	}
	if (mProgramInfo) {
	  mProgramInfo->clear(); delete mProgramInfo; mProgramInfo = (vector<AString> *)NULL;
	}
	if (mPeriods) {
	  mPeriods->clear(); delete mPeriods; mPeriods = (vector<MPDPeriodNode> *)NULL;
	}
	if (mMetrics) {
	  mMetrics->clear(); delete mMetrics; mMetrics = (vector<MPDMetricsNode> *)NULL;
	}
      };

    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDMpdNode);
    };

    /*
     * MPDStreamPeriod
     */
    class MPDStreamPeriod : public RefBase {
    public:
      MPDPeriodNode mPeriod;
      uint32_t mNumber;
      MPDClockTime mStart;
      MPDClockTime mDuration;
      
    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDStreamPeriod);
    };

    /*
     * MPDMediaSegment
     */
    class MPDMediaSegment : public RefBase {
    public:
      MPDSegmentURLNode *mSegmentURL;              /* this is NULL when using a SegmentTemplate */
      uint32_t mNumber;                            /* segment number */
      uint64_t mStart;                             /* segment start time in timescale units */
      MPDClockTime mStart_time;                    /* segment start time */
      MPDClockTime mDuration;                      /* segment duration */

    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDMediaSegment);
    };

    /*
     * MPDMediaFragmentInfo
     */
    class MPDMediaFragmentInfo : public RefBase {
    public:
      AString mUri;
      int64_t mRangeStart;
      int64_t mRangeEnd;
      
      AString mIndexUri;
      int64_t mIndexRangeStart;
      int64_t mIndexRangeEnd;
      
      bool mDiscontinuity;
      MPDClockTime mTimestamp;
      MPDClockTime mDuration;      

    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDMediaFragmentInfo);
    };

    /*
     * MPDActiveStream
     */
    class MPDActiveStream : public RefBase {
    public:
      MPDStreamMimeType mimeType;                 /* video/audio/application */

      uint32_t mBaseUrlIdx;                       /* index of the baseURL used for last request */
      AString mBaseUrl;                           /* active baseURL used for last request */
      AString mQueryUrl;                          /* active baseURL used for last request */
      uint32_t mMaxBandwidth;                     /* max bandwidth allowed for this mimeType */

      MPDAdaptationSetNode *mCurAdaptSet;         /* active adaptation set */
      int32_t mRepresentationIdx;                 /* index of current representation */
      MPDRepresentationNode *mCurRepresentation;  /* active representation */
      MPDSegmentBaseType *mCurSegmentBase;        /* active segment base */
      MPDSegmentListNode *mCurSegmentList;        /* active segment list */
      MPDSegmentTemplateNode *mCurSegTemplate;    /* active segment template */
      uint32_t mSegmentIdx;                       /* index of next sequence chunk */
      vector<MPDMediaSegment> mSegments;          /* array of MPDMediaSegment */
      
    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDActiveStream);
    };

    /*
     * MPDMpdClient
     */
    class MPDMpdClient : public RefBase {
    public:
      MPDMpdNode *mMpdNode;                       /* active MPD manifest file */

      vector<MPDStreamPeriod> mPeriods;           /* list of MPDStreamPeriod */
      uint32_t mPeriodIdx;                        /* index of current Period */

      vector<MPDActiveStream> mActiveStreams;     /* list of MPDActiveStream */

      uint32_t mUpdateFailedCount;
      AString mMpdUri;                            /* manifest file URI */
      Mutex mLock;
      
    private:
      DISALLOW_EVIL_CONSTRUCTORS(MPDMpdClient);
    };












    class Item : public RefBase {
    public:
      Item() {};

    private:
      AString mURI;
      sp<AMessage> mMeta;

      DISALLOW_EVIL_CONSTRUCTORS(Item);
    };






    MPDMpdClient *mClient;

    status_t mInitCheck;

    AString mBaseURI;
    bool mIsComplete;
    bool mIsEvent;
    
    sp<AMessage> mMeta;
    Vector<Item> mItems;

    status_t parse(const void *data, size_t size);

    DISALLOW_EVIL_CONSTRUCTORS(MPDParser);
  };

};

#endif /* MPD_PARSER_H_ */

// Local Variables:
// mode:C++
// End:
