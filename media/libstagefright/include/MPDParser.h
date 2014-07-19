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
  public:
    MPDParser(const char *baseURI, const void *data, size_t size);

    status_t initCheck() const;
    
    bool isComplete() const;
    bool isEvent() const;

    sp<AMessage> meta();

    size_t size();
    bool itemAt(size_t index, AString *uri, sp<AMessage> *meta = NULL);

  protected:
    virtual ~MPDParser();

  public:

    /*
     * Stream type
     */
    typedef enum MPDStreamType_e {
      kStreamVideo,           /* video stream (the main one) */
      kStreamAudio,           /* audio stream (optional) */
      kStreamApplication      /* application stream (optional): for timed text/subtitles */
    } MPDStreamType;

    /*
     * MPD Type
     */
    typedef enum MPDMpdType_e {
      kMpdTypeUninitialized,  /* No MPD@type */
      kMpdTypeStatic,	      /* MPD@type == static */
      kMpdTypeDynamic         /* MPD@type == dynamic */
    } MPDMpdType;

    /*
     * SAP types
     */
    typedef enum MPDSapType_e {
      kSapType_0 = 0,
      kSapType_1,
      kSapType_2,
      kSapType_3,
      kSapType_4,
      kSapType_5,
      kSapType_6
    } MPDSapType;

    /*
     * Specific ClockTime values
     */
    typedef enum MPDClockTime_e {
      kClockTimeNone = -1
    } MPDClockTime;

    /*
     * MPDBaseUrl
     */
    class MPDBaseUrl : public RefBase {

    public:
      MPDBaseUrl()
	: mBaseUrl(AString("")),
	  mServiceLocation(AString("")),
	  mByteRange(AString("")) 
      {};

      MPDBaseUrl(const char *baseUrl)
	: mBaseUrl(AString(baseUrl)),
	  mServiceLocation(AString("")),
	  mByteRange(AString("")) 
      {};

      virtual ~MPDBaseUrl() {
      };

      AString mBaseUrl;
      AString mServiceLocation;
      AString mByteRange;
      
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

      virtual ~MPDRange() {};

      uint64_t mFirstBytePos;
      uint64_t mLastBytePos;

    private:
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

      virtual ~MPDRatio() {};

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

      virtual ~MPDFrameRate() {};

      DISALLOW_EVIL_CONSTRUCTORS(MPDFrameRate);
    };

    /*
     * MPDConditionalUintType
     */
    class MPDConditionalUintType : public RefBase {
    public:
      MPDConditionalUintType()
	: mFlag(false),
	  mValue(0) 
      {};
      MPDConditionalUintType(bool flag, uint32_t value)
	: mFlag(flag),
	  mValue(value) 
      {};

      virtual ~MPDConditionalUintType() {};

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
      MPD_SNode(const MPD_SNode &rhv)
	: RefBase(),
	  mT(rhv.mT),
	  mD(rhv.mD),
	  mR(rhv.mR) 
      {};
      virtual ~MPD_SNode() {};

      uint64_t mT;
      uint64_t mD;
      uint32_t mR;
      
      MPD_SNode & operator=(const MPD_SNode &rhv)
      {
	this->mT = rhv.mT;
	this->mD = rhv.mD;
	this->mR = rhv.mR;
	return *this;
      };

      //      DISALLOW_EVIL_CONSTRUCTORS(MPD_SNode);
    };

    /*
     * MPDSegmentTimelineNode
     */
    class MPDSegmentTimelineNode : public RefBase {
    public:
      MPDSegmentTimelineNode() 
	: mSNodes(new vector<MPD_SNode>()) 
      {};

      virtual ~MPDSegmentTimelineNode() {
	mSNodes->clear();
	delete mSNodes;
	mSNodes = (vector<MPD_SNode> *)NULL;
      };

      vector<MPD_SNode> *mSNodes;

      DISALLOW_EVIL_CONSTRUCTORS(MPDSegmentTimelineNode);
    };

    /*
     * MPDUrlType
     */
    class MPDUrlType : public RefBase {
    public:
      MPDUrlType()
	: mSourceUrl(AString("")),
	  mRange() 
      {};
      MPDUrlType(const char *sourceUrl, MPDRange* range)
	: mSourceUrl(sourceUrl),
	  mRange(range) {
	incStrong(range);
      };

      virtual ~MPDUrlType () {
	decStrong(mRange);
      };

      AString mSourceUrl;
      MPDRange *mRange;

    private:
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
	  mInitialization(new MPDUrlType()),
	  mRepresentationIndex(new MPDUrlType())
      {};

      virtual ~MPDSegmentBaseType() {};

    public:
      uint32_t mTimescale;
      uint32_t mPresentationTimeOffset;
      MPDRange *mIndexRange;
      bool mIndexRangeExact;
      MPDUrlType *mInitialization;
      MPDUrlType *mRepresentationIndex;

    private:
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

      virtual ~MPDMultSegmentBaseType() {};

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
    class MPDSegmentUrlNode;
    class MPDSegmentListNode : public RefBase {
    public:
      MPDSegmentListNode()
	: mMultSegBaseType(new MPDMultSegmentBaseType()),
	  mSegmentUrlNodes(new vector<MPDSegmentUrlNode>())
      {};

      virtual ~MPDSegmentListNode() 
      {
	mSegmentUrlNodes->clear();
	delete mSegmentUrlNodes;
	mSegmentUrlNodes = (vector<MPDSegmentUrlNode> *)NULL;
      };

      MPDMultSegmentBaseType *mMultSegBaseType;
      vector<MPDSegmentUrlNode> *mSegmentUrlNodes;
      
      DISALLOW_EVIL_CONSTRUCTORS(MPDSegmentListNode);
    };

    /*
     * MPDSegmentTemplateNode
     */
    class MPDSegmentTemplateNode : public RefBase {
    public:
      MPDSegmentTemplateNode()
	: mMultSegBaseType(),
	  mMedia(AString("")),
	  mIndex(AString("")),
	  mInitialization(AString("")),
	  mBitstreamSwitching(AString(""))
      {};

      virtual ~MPDSegmentTemplateNode() {};

      MPDMultSegmentBaseType *mMultSegBaseType;
      AString mMedia;
      AString mIndex;
      AString mInitialization;
      AString mBitstreamSwitching;

      DISALLOW_EVIL_CONSTRUCTORS(MPDSegmentTemplateNode);
    };

    /*
     * MPDSegmentUrlNode
     */
    class MPDSegmentUrlNode : public RefBase {
    public:
      MPDSegmentUrlNode()
	: mMedia(AString("")),
	  mMediaRange(),
	  mIndex(AString("")),
	  mIndexRange()
      {};

      virtual ~MPDSegmentUrlNode() {};

      AString mMedia;
      MPDRange *mMediaRange;
      AString mIndex;
      MPDRange *mIndexRange;

      DISALLOW_EVIL_CONSTRUCTORS(MPDSegmentUrlNode);
    };

    /*
     * MPDRepresentationBaseType
     */
    class MPDDescriptorType;
    class MPDRepresentationBaseType : public RefBase {
    public:
      MPDRepresentationBaseType()
	: mProfiles(AString()),
	  mWidth(0),
	  mHeight(0),
	  mSar(new MPDRatio()),
	  mFrameRate(new MPDFrameRate()),
	  mAudioSamplingRate(AString()),
	  mMimeType(AString()),
	  mSegmentProfiles(AString()),
	  mCodecs(AString()),
	  mMaximumSAPPeriod(0.0),
	  mStartWithSAP(kSapType_0),
	  mMaxPlayoutRate(0.0),
	  mCodingDependency(false),
	  mScanType(AString()),
	  mFramePacking(vector<MPDDescriptorType>()),
	  mAudioChannelConfiguration(vector<MPDDescriptorType>()),
	  mContentProtection(vector<MPDDescriptorType>())
      {};

      virtual ~MPDRepresentationBaseType() 
      {
	if (mSar != (MPDRatio *)NULL)
	  {
	    delete mSar; mSar = (MPDRatio *)NULL;
	  }
	if (mFrameRate != (MPDFrameRate *)NULL)
	  {
	    delete mFrameRate; mFrameRate = (MPDFrameRate *)NULL;
	  }
      };

      AString mProfiles;
      uint32_t mWidth;
      uint32_t mHeight;
      MPDRatio *mSar;
      MPDFrameRate *mFrameRate;
      AString mAudioSamplingRate;
      AString mMimeType;
      AString mSegmentProfiles;
      AString mCodecs;
      double mMaximumSAPPeriod;
      MPDSapType mStartWithSAP;
      double mMaxPlayoutRate;
      bool mCodingDependency;
      AString mScanType;
      vector<MPDDescriptorType> mFramePacking;
      vector<MPDDescriptorType> mAudioChannelConfiguration;
      vector<MPDDescriptorType> mContentProtection;

      DISALLOW_EVIL_CONSTRUCTORS(MPDRepresentationBaseType);
    };

    /*
     * MPDSubRepresentationNode
     */
    class MPDSubRepresentationNode : public RefBase {
    public:
      MPDSubRepresentationNode()
	: mRepresentationBase(new MPDRepresentationBaseType()),
	  mLevel(0),
	  mDependencyLevel(vector<uint32_t>()),
	  mSize(0),
	  mBandwidth(0),
	  mContentComponent(vector<AString>())
      {};

      virtual ~MPDSubRepresentationNode() {
	delete mRepresentationBase; mRepresentationBase = (MPDRepresentationBaseType *)NULL;
      };

      MPDRepresentationBaseType *mRepresentationBase;
      uint32_t mLevel;
      vector<uint32_t> mDependencyLevel;
      uint32_t mSize;
      uint32_t mBandwidth;
      vector<AString> mContentComponent;

      DISALLOW_EVIL_CONSTRUCTORS(MPDSubRepresentationNode);
    };

    /*
     * MPDRepresentationNode
     */
    class MPDRepresentationNode : public RefBase {
    public:
      MPDRepresentationNode()
	: mId(AString("")),
	  mBandwidth(0),
	  mQualityRanking(0),
	  mDependencyId(vector<AString>()),
	  mMediaStreamStructureId(vector<AString>()),
	  mRepresentationBase(new MPDRepresentationBaseType()),
	  mBaseUrls(vector<AString>()),
	  mSubRepresentations(vector<AString>()),
	  mSegmentBase(new MPDSegmentBaseType()),
	  mSegmentTemplate(new MPDSegmentTemplateNode()),
	  mSegmentList(new MPDSegmentListNode())
      {};

      virtual ~MPDRepresentationNode() {
	delete mRepresentationBase; mRepresentationBase = (MPDRepresentationBaseType *)NULL;
	delete mSegmentBase; mSegmentBase = (MPDSegmentBaseType *)NULL;
	delete mSegmentTemplate; mSegmentTemplate = (MPDSegmentTemplateNode *)NULL;
	delete mSegmentList; mSegmentList = (MPDSegmentListNode *)NULL;
      };


      AString mId;
      uint32_t mBandwidth;
      uint32_t mQualityRanking;
      vector<AString> mDependencyId;              /* StringVectorType */
      vector<AString> mMediaStreamStructureId;    /* StringVectorType */
      /* RepresentationBase extension */
      MPDRepresentationBaseType *mRepresentationBase;
      /* list of BaseUrl nodes */
      vector<AString> mBaseUrls;
      /* list of SubRepresentation nodes */
      vector<AString> mSubRepresentations;
      /* SegmentBase node */
      MPDSegmentBaseType *mSegmentBase;
      /* SegmentTemplate node */
      MPDSegmentTemplateNode *mSegmentTemplate;
      /* SegmentList node */
      MPDSegmentListNode *mSegmentList;

      DISALLOW_EVIL_CONSTRUCTORS(MPDRepresentationNode);
    };

    /*
     * MPDDescriptorType
     */
    class MPDDescriptorType : public RefBase {
    public:
      MPDDescriptorType()
	: mSchemeIdUri(AString("")),
	  mValue(AString(""))
      {};

      virtual ~MPDDescriptorType()
      {};

      AString mSchemeIdUri;
      AString mValue;

      DISALLOW_EVIL_CONSTRUCTORS(MPDDescriptorType);
    };

    /*
     * MPDContentComponentNode
     */
    class MPDContentComponentNode : public RefBase {
    public:
      MPDContentComponentNode()
	: mId(0),
	  mLang(AString()),
	  mContentType(AString()),
	  mPar(new MPDRatio()),
	  mAccessibility(vector<MPDDescriptorType>()),
	  mRole(vector<MPDDescriptorType>()),
	  mRating(vector<MPDDescriptorType>()),
	  mViewpoint(vector<MPDDescriptorType>())
      {};

      virtual ~MPDContentComponentNode() 
      {
	delete mPar; mPar = (MPDRatio *)NULL;
      };

      uint32_t mId;
      AString mLang;                      /* LangVectorType RFC 5646 */
      AString mContentType;
      MPDRatio *mPar;
      /* list of Accessibility DescriptorType nodes */
      vector<MPDDescriptorType> mAccessibility;
      /* list of Role DescriptorType nodes */
      vector<MPDDescriptorType> mRole;
      /* list of Rating DescriptorType nodes */
      vector<MPDDescriptorType> mRating;
      /* list of Viewpoint DescriptorType nodes */
      vector<MPDDescriptorType> mViewpoint;

      DISALLOW_EVIL_CONSTRUCTORS(MPDContentComponentNode);
    };

    /*
     * MPDAdaptationSetNode
     */
    class MPDAdaptationSetNode : public RefBase {
    public:
      MPDAdaptationSetNode()
	: mId(0),
	  mGroup(0),
	  mLang(AString("")),
	  mContentType(AString("")),
	  mPar(new MPDRatio()),
	  mMinBandwidth(0),
	  mMaxBandwidth(0),
	  mMinWidth(0),
	  mMaxWidth(0),
	  mMinHeight(0),
	  mMaxHeight(0),
	  mMinFrameRate(new MPDFrameRate()),
	  mMaxFrameRate(new MPDFrameRate()),
	  mSegmentAlignment(new MPDConditionalUintType()),
	  mSubSegmentAlignment(new MPDConditionalUintType()),
	  mSubSegmentStartsWithSAP(kSapType_0),
	  mBitstreamSwitching(false),
	  mAccessibility(vector<MPDDescriptorType>()),
	  mRole(vector<MPDDescriptorType>()),
	  mRating(vector<MPDDescriptorType>()),
  	  mViewpoint(vector<MPDDescriptorType>()),
	  mRepresentationBase(MPDRepresentationBaseType()),
	  mSegmentBase(new MPDSegmentBaseType()),
	  mSegmentList(new MPDSegmentListNode()),
	  mSegmentTemplate(new MPDSegmentTemplateNode()),
	  mBaseUrls(vector<MPDBaseUrl>()),
	  mRepresentations(vector<MPDRepresentationNode>()),
	  mContentComponents(vector<MPDContentComponentNode>())
        {};

      virtual ~MPDAdaptationSetNode()
      {
	delete mPar; mPar = (MPDRatio *)NULL;
	delete mMinFrameRate; mMinFrameRate = (MPDFrameRate *)NULL;
	delete mMaxFrameRate; mMaxFrameRate = (MPDFrameRate *)NULL;
	delete mSegmentAlignment; mSegmentAlignment = (MPDConditionalUintType *)NULL;
	delete mSubSegmentAlignment; mSubSegmentAlignment = (MPDConditionalUintType *)NULL;
	delete mSegmentBase; mSegmentBase = (MPDSegmentBaseType *)NULL;
	delete mSegmentList; mSegmentList = (MPDSegmentListNode *)NULL;
	delete mSegmentTemplate; mSegmentTemplate = (MPDSegmentTemplateNode *)NULL;
      };

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
      MPDConditionalUintType *mSubSegmentAlignment;
      MPDSapType mSubSegmentStartsWithSAP;
      bool mBitstreamSwitching;
      /* list of Accessibility DescriptorType nodes */
      vector<MPDDescriptorType> mAccessibility;
      /* list of Role DescriptorType nodes */
      vector<MPDDescriptorType> mRole;
      /* list of Rating DescriptorType nodes */
      vector<MPDDescriptorType> mRating;
      /* list of Viewpoint DescriptorType nodes */
      vector<MPDDescriptorType> mViewpoint;
      /* RepresentationBase extension */
      MPDRepresentationBaseType mRepresentationBase;
      /* SegmentBase node */
      MPDSegmentBaseType *mSegmentBase;
      /* SegmentList node */
      MPDSegmentListNode *mSegmentList;
      /* SegmentTemplate node */
      MPDSegmentTemplateNode *mSegmentTemplate;
      /* list of BaseUrl nodes */
      vector<MPDBaseUrl> mBaseUrls;
      /* list of Representation nodes */
      vector<MPDRepresentationNode> mRepresentations;
      /* list of ContentComponent nodes */
      vector<MPDContentComponentNode> mContentComponents;
      
      DISALLOW_EVIL_CONSTRUCTORS(MPDAdaptationSetNode);
    };

    /*
     * MPDSubsetNode
     */
    class MPDSubsetNode : public RefBase {
    public:
      MPDSubsetNode()
	: mContains(vector<uint32_t>()),
	  mSize(0)
      {};

      vector<uint32_t> mContains;                   /* UIntVectorType */
      uint32_t mSize;                               /* size of the "contains" array */

      DISALLOW_EVIL_CONSTRUCTORS(MPDSubsetNode);
    };

    /*
     * MPDPeriodNode
     */
    class MPDPeriodNode : public RefBase {

    public:
      MPDPeriodNode()
	: mId(AString("")),
	  mStart(0L),
	  mDuration(0L),
	  mBitstreamSwitching(false),
	  mSegmentBase(new MPDSegmentBaseType()),
	  mSegmentList(new MPDSegmentListNode()),
	  mSegmentTemplate(new MPDSegmentTemplateNode()),
	  mAdaptationSets(vector<MPDAdaptationSetNode>()),
	  mSubsets(vector<MPDSubsetNode>()),
	  mBaseUrls(vector<MPDBaseUrl>())
      {};

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
      vector<MPDAdaptationSetNode> mAdaptationSets;
      /* list of Representation nodes */
      vector<MPDSubsetNode> mSubsets;
      /* list of BaseUrl nodes */
      vector<MPDBaseUrl> mBaseUrls;

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
      
      DISALLOW_EVIL_CONSTRUCTORS(MPDProgramInformationNode);
    };

    /*
     * MPDMetricsRangeNode
     */
    class MPDMetricsRangeNode : public RefBase {
    public:
      int64_t mStarttime;                  /* [ms] */
      int64_t mDuration;                   /* [ms] */
      
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
      MPDDateTime(uint16_t year, uint8_t month, uint8_t day, 
		  uint8_t hour, uint8_t minute, uint8_t second)
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

      virtual ~MPDDateTime() {};

    private:
      uint16_t mYear;
      uint8_t mMonth;
      uint8_t mDay;
      uint8_t mHour;
      uint8_t mMinute;
      uint8_t mSecond;

    public:
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
	  mType(kMpdTypeUninitialized),
	  mMediaPresentationDuration(0L),
	  mMinimumUpdatePeriod(0L),
	  mMinBufferTime(0L),
	  mTimeShiftBufferDepth(0L),
	  mSuggestedPresentationDelay(0L),
	  mMaxSegmentDuration(0L),
	  mMaxSubSegmentDuration(0L),
	  mBaseUrls(vector<MPDBaseUrl>()),
	  mLocations(vector<AString>()),
	  mProgramInfo(vector<MPDProgramInformationNode>()),
	  mPeriods(vector<MPDPeriodNode>()),
	  mMetrics(vector<MPDMetricsNode>())
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
      MPDMpdType mType;
      MPDDateTime mAvailabilityStartTime;
      MPDDateTime mAvailabilityEndTime;
      int64_t mMediaPresentationDuration;  /* [ms] */
      int64_t mMinimumUpdatePeriod;        /* [ms] */
      int64_t mMinBufferTime;              /* [ms] */
      int64_t mTimeShiftBufferDepth;       /* [ms] */
      int64_t mSuggestedPresentationDelay; /* [ms] */
      int64_t mMaxSegmentDuration;         /* [ms] */
      int64_t mMaxSubSegmentDuration;      /* [ms] */
      /* list of BaseUrl nodes */
      vector<MPDBaseUrl> mBaseUrls;
      /* list of Location nodes */
      vector<AString> mLocations;
      /* List of ProgramInformation nodes */
      vector<MPDProgramInformationNode> mProgramInfo;
      /* list of Periods nodes */
      vector<MPDPeriodNode> mPeriods;
      /* list of Metrics nodes */
      vector<MPDMetricsNode> mMetrics;
      
      virtual ~MPDMpdNode() 
      {
	mDefault_namespace.clear(); 
	mNamespace_xsi.clear(); 
	mNamespace_ext.clear(); 
	mSchemaLocation.clear(); 
	mId.clear(); 
	mProfiles.clear(); 
	mType = kMpdTypeUninitialized; 
	
	mAvailabilityStartTime.clear();
	mAvailabilityEndTime.clear();
	mMediaPresentationDuration = 0L;
	mMinimumUpdatePeriod = 0L;
	mMinBufferTime = 0L;
	mTimeShiftBufferDepth = 0L;
	mSuggestedPresentationDelay = 0L;
	mMaxSegmentDuration = 0L;
	mMaxSubSegmentDuration = 0L;
	mBaseUrls.clear(); 
	mLocations.clear(); 
	mProgramInfo.clear(); 
	mPeriods.clear(); 
	mMetrics.clear(); 
      };

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
      MPDSegmentUrlNode *mSegmentURL;              /* this is NULL when using a SegmentTemplate */
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
      MPDStreamType mimeType;                     /* video/audio/application */

      uint32_t mBaseUrlIdx;                       /* index of the baseUrl used for last request */
      AString mBaseUrl;                           /* active baseUrl used for last request */
      AString mQueryUrl;                          /* active baseUrl used for last request */
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
      Item()
	: mUri(),
	  mMeta()
      {};

      AString mUri;
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
