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
    bool isVariantManifest();
    bool isVariantComputed();

    sp<AMessage> meta();

    size_t size();
    bool itemAt(size_t index, AString *uri, sp<AMessage> *meta = NULL);

    virtual ~MPDParser();

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
      
      MPDBaseUrl(const MPDBaseUrl &copyin) 
	: RefBase(),
	  mBaseUrl(copyin.mBaseUrl),
	  mServiceLocation(copyin.mServiceLocation),
	  mByteRange(copyin.mByteRange)
      {};

      MPDBaseUrl &operator=(const MPDBaseUrl &rhs) 
      {
	mBaseUrl = rhs.mBaseUrl;
	mServiceLocation = rhs.mServiceLocation;
	mByteRange = rhs.mByteRange;
	
	return *this;
      };
      
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

      MPDRange(const MPDRange &copyin) 
	: RefBase(),
	  mFirstBytePos(copyin.mFirstBytePos),
	  mLastBytePos(copyin.mLastBytePos)
      {};
      MPDRange &operator=(const MPDRange &rhs)
      {
	mFirstBytePos = rhs.mFirstBytePos;
	mLastBytePos = rhs.mLastBytePos;

	return *this;
      };

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

      MPDRatio(const MPDRatio &copyin)
	: RefBase()
      {
	mNum = copyin.mNum;
	mDen = copyin.mDen;
      };
      MPDRatio &operator=(const MPDRatio &rhs)
      {
	mNum = rhs.mNum;
	mDen = rhs.mDen;

	return *this;
      };

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

      MPDFrameRate(const MPDFrameRate &copy)
	: MPDRatio(copy.mNum, copy.mDen) 
      {};

      MPDFrameRate & operator=(const MPDFrameRate &rhs) 
      {
	mNum = rhs.mNum; 
	mDen = rhs.mDen; 

	return *this;
      };
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

      MPDConditionalUintType(const MPDConditionalUintType &copyin) 
	: RefBase()
      {
	mFlag = copyin.mFlag;
	mValue = copyin.mValue;
      };

      MPDConditionalUintType & operator=(const MPDConditionalUintType &rhs) {
	mFlag = rhs.mFlag;
	mValue = rhs.mValue;
	
	return *this;
      };
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

      virtual ~MPD_SNode() {};

      uint64_t mT;
      uint64_t mD;
      uint32_t mR;

      MPD_SNode(const MPD_SNode &copyin)
	: RefBase(),
	  mT(copyin.mT),
	  mD(copyin.mD),
	  mR(copyin.mR)
      {};	  
      MPD_SNode & operator=(const MPD_SNode &rhv)
      {
	mT = rhv.mT;
	mD = rhv.mD;
	mR = rhv.mR;

	return *this;
      };
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

      MPDSegmentTimelineNode(const MPDSegmentTimelineNode &copyin) 
	: RefBase()
      {
	mSNodes = new vector<MPD_SNode>(*copyin.mSNodes);
      };
      MPDSegmentTimelineNode & operator=(const MPDSegmentTimelineNode &rhs) {
	mSNodes = new vector<MPD_SNode>(*rhs.mSNodes);

	return *this;
      };
    };

    /*
     * MPDUrlType
     */
    class MPDUrlType : public RefBase {
    public:
      MPDUrlType()
	: mSourceUrl(AString("")),
	  mRange(new MPDRange()) 
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

      MPDUrlType(const MPDUrlType &copyin)
	: RefBase(),
	  mSourceUrl(copyin.mSourceUrl),
	  mRange(new MPDRange(*copyin.mRange))
      {};
      MPDUrlType &operator=(const MPDUrlType &rhs)
      {
	mSourceUrl = rhs.mSourceUrl;
	mRange = new MPDRange(*rhs.mRange);

	return *this;
      };
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

      MPDSegmentBaseType(const MPDSegmentBaseType &copyin)
	: RefBase()
      {
	mTimescale = copyin.mTimescale;
	mPresentationTimeOffset = copyin.mPresentationTimeOffset;
	mIndexRange = new MPDRange(*copyin.mIndexRange);
	mIndexRangeExact = copyin.mIndexRangeExact;
	mInitialization = new MPDUrlType(*copyin.mInitialization);
	mRepresentationIndex = new MPDUrlType(*copyin.mRepresentationIndex);
      };
      MPDSegmentBaseType &operator=(const MPDSegmentBaseType &rhs)
      {
	mTimescale = rhs.mTimescale;
	mPresentationTimeOffset = rhs.mPresentationTimeOffset;
	mIndexRange = new MPDRange(*rhs.mIndexRange);
	mIndexRangeExact = rhs.mIndexRangeExact;
	mInitialization = new MPDUrlType(*rhs.mInitialization);
	mRepresentationIndex = new MPDUrlType(*rhs.mRepresentationIndex);
	
	return *this;
      };
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

      MPDMultSegmentBaseType(const MPDMultSegmentBaseType &copyin) 
	: RefBase(),
	  mDuration(copyin.mDuration),
	  mStartNumber(copyin.mStartNumber),
	  mSegmentBaseType(new MPDSegmentBaseType(*copyin.mSegmentBaseType)),
	  mSegmentTimeline(new MPDSegmentTimelineNode(*copyin.mSegmentTimeline)),
	  mBitstreamSwitching(new MPDUrlType(*copyin.mBitstreamSwitching))
      {};

      MPDMultSegmentBaseType & operator=(const MPDMultSegmentBaseType &rhs) {
	mDuration = rhs.mDuration;
	mStartNumber = rhs.mStartNumber;
	mSegmentBaseType = new MPDSegmentBaseType(*rhs.mSegmentBaseType);
	mSegmentTimeline = new MPDSegmentTimelineNode(*rhs.mSegmentTimeline);
	mBitstreamSwitching = new MPDUrlType(*rhs.mBitstreamSwitching);

	return *this;
      };
      
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
      
      MPDSegmentListNode(const MPDSegmentListNode &copyin) 
	: RefBase(),
	  mMultSegBaseType(new MPDMultSegmentBaseType(*copyin.mMultSegBaseType)),
	  mSegmentUrlNodes(new vector<MPDSegmentUrlNode>(*copyin.mSegmentUrlNodes))
      {};

      MPDSegmentListNode &operator=(const MPDSegmentListNode &rhs) 
      {
	mMultSegBaseType = new MPDMultSegmentBaseType(*rhs.mMultSegBaseType);
	mSegmentUrlNodes = new vector<MPDSegmentUrlNode>(*rhs.mSegmentUrlNodes);

	return *this;
      };
    };

    /*
     * MPDSegmentTemplateNode
     */
    class MPDSegmentTemplateNode : public RefBase {
    public:
      MPDSegmentTemplateNode()
	: mMultSegBaseType(new MPDMultSegmentBaseType()),
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

      MPDSegmentTemplateNode(const MPDSegmentTemplateNode &copyin) 
	: RefBase(),
	  mMultSegBaseType(new MPDMultSegmentBaseType(*copyin.mMultSegBaseType)),
	  mMedia(copyin.mMedia),
	  mIndex(copyin.mIndex),
	  mInitialization(copyin.mInitialization),
	  mBitstreamSwitching(copyin.mBitstreamSwitching)
      {};

      MPDSegmentTemplateNode &operator=(const MPDSegmentTemplateNode &rhs) {
	mMultSegBaseType = new MPDMultSegmentBaseType(*rhs.mMultSegBaseType);
	mMedia = rhs.mMedia;
	mIndex = rhs.mIndex;
	mInitialization = rhs.mInitialization;
	mBitstreamSwitching = rhs.mBitstreamSwitching;

	return *this;
      };
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

      MPDSegmentUrlNode(const MPDSegmentUrlNode &copyin) 
	: RefBase(),
	  mMedia(copyin.mMedia),
	  mMediaRange(new MPDRange(*copyin.mMediaRange)),
	  mIndex(copyin.mIndex),
	  mIndexRange(new MPDRange(*copyin.mIndexRange))
      {};

      MPDSegmentUrlNode &operator=(const MPDSegmentUrlNode &rhs) {
	mMedia = rhs.mMedia;
	mMediaRange = new MPDRange(*rhs.mMediaRange);
	mIndex = rhs.mIndex;
	mIndexRange = new MPDRange(*rhs.mIndexRange);

	return *this;
      };
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

      MPDRepresentationBaseType(const MPDRepresentationBaseType &copyin) 
	: RefBase(),
	  mProfiles(copyin.mProfiles),
	  mWidth(copyin.mWidth),
	  mHeight(copyin.mHeight),
	  mSar(new MPDRatio(*copyin.mSar)),
	  mFrameRate(new MPDFrameRate(*copyin.mFrameRate)),
	  mAudioSamplingRate(copyin.mAudioSamplingRate),
	  mMimeType(copyin.mMimeType),
	  mSegmentProfiles(copyin.mSegmentProfiles),
	  mCodecs(copyin.mCodecs),
	  mMaximumSAPPeriod(copyin.mMaximumSAPPeriod),
	  mStartWithSAP(copyin.mStartWithSAP),
	  mMaxPlayoutRate(copyin.mMaxPlayoutRate),
	  mCodingDependency(copyin.mCodingDependency),
	  mScanType(copyin.mScanType),
	  mFramePacking(vector<MPDDescriptorType>(copyin.mFramePacking)),
	  mAudioChannelConfiguration(vector<MPDDescriptorType>(copyin.mAudioChannelConfiguration)),
	  mContentProtection(vector<MPDDescriptorType>(copyin.mContentProtection))
      {};

      MPDRepresentationBaseType &operator=(const MPDRepresentationBaseType &rhs) {
	mProfiles = rhs.mProfiles;
	mWidth = rhs.mWidth;
	mHeight = rhs.mHeight;
	mSar = new MPDRatio(*rhs.mSar);
	mFrameRate = new MPDFrameRate(*rhs.mFrameRate);
	mAudioSamplingRate = rhs.mAudioSamplingRate;
	mMimeType = rhs.mMimeType;
	mSegmentProfiles = rhs.mSegmentProfiles;
	mCodecs = rhs.mCodecs;
	mMaximumSAPPeriod = rhs.mMaximumSAPPeriod;
	mStartWithSAP = rhs.mStartWithSAP;
	mMaxPlayoutRate = rhs.mMaxPlayoutRate;
	mCodingDependency = rhs.mCodingDependency;
	mScanType = rhs.mScanType;
	mFramePacking = vector<MPDDescriptorType>(rhs.mFramePacking);
	mAudioChannelConfiguration = vector<MPDDescriptorType>(rhs.mAudioChannelConfiguration) ;
	mContentProtection = vector<MPDDescriptorType>(rhs.mContentProtection);

	return *this;
      };

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

      MPDSubRepresentationNode(const MPDSubRepresentationNode &copyin)
	: RefBase(),
	  mRepresentationBase(new MPDRepresentationBaseType(*copyin.mRepresentationBase)),
	  mLevel(copyin.mLevel),
	  mDependencyLevel(vector<uint32_t>(copyin.mDependencyLevel)),
	  mSize(copyin.mSize),
	  mBandwidth(copyin.mBandwidth),
	  mContentComponent(vector<AString>(copyin.mContentComponent))
      {};

       MPDSubRepresentationNode &operator=(const MPDSubRepresentationNode &rhs) {
	mRepresentationBase = new MPDRepresentationBaseType(*rhs.mRepresentationBase);
	mLevel = rhs.mLevel;
	mDependencyLevel = vector<uint32_t>(rhs.mDependencyLevel);
	mSize = rhs.mSize;
	mBandwidth = rhs.mBandwidth;
	mContentComponent = vector<AString>(rhs.mContentComponent);

	return *this;
      };
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
	  mBaseUrls(vector<MPDBaseUrl>()),
	  mSubRepresentations(vector<MPDSubRepresentationNode>()),
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
      vector<MPDBaseUrl> mBaseUrls;
      /* list of SubRepresentation nodes */
      vector<MPDSubRepresentationNode> mSubRepresentations;
      /* SegmentBase node */
      MPDSegmentBaseType *mSegmentBase;
      /* SegmentTemplate node */
      MPDSegmentTemplateNode *mSegmentTemplate;
      /* SegmentList node */
      MPDSegmentListNode *mSegmentList;

      MPDRepresentationNode(const MPDRepresentationNode &copyin) 
	: RefBase(),
	  mId(copyin.mId),
	  mBandwidth(copyin.mBandwidth),
	  mQualityRanking(copyin.mQualityRanking),
	  mDependencyId(vector<AString>(copyin.mDependencyId)),
	  mMediaStreamStructureId(vector<AString>(copyin.mMediaStreamStructureId)),
	  mRepresentationBase(new MPDRepresentationBaseType(*copyin.mRepresentationBase)),
	  mBaseUrls(vector<MPDBaseUrl>(copyin.mBaseUrls)),
	  mSubRepresentations(vector<MPDSubRepresentationNode>(copyin.mSubRepresentations)),
	  mSegmentBase(new MPDSegmentBaseType(*copyin.mSegmentBase)),
	  mSegmentTemplate(new MPDSegmentTemplateNode(*copyin.mSegmentTemplate)),
	  mSegmentList(new MPDSegmentListNode(*copyin.mSegmentList))
      {};

      MPDRepresentationNode &operator=(const MPDRepresentationNode &rhs) {
	mId = rhs.mId;
	mBandwidth = rhs.mBandwidth;
	mQualityRanking = rhs.mQualityRanking;
	mDependencyId = vector<AString>(rhs.mDependencyId);
	mMediaStreamStructureId = vector<AString>(rhs.mMediaStreamStructureId);
	mRepresentationBase = new MPDRepresentationBaseType(*rhs.mRepresentationBase);
	mBaseUrls = vector<MPDBaseUrl>(rhs.mBaseUrls);
	mSubRepresentations = vector<MPDSubRepresentationNode>(rhs.mSubRepresentations);
	mSegmentBase = new MPDSegmentBaseType(*rhs.mSegmentBase);
	mSegmentTemplate = new MPDSegmentTemplateNode(*rhs.mSegmentTemplate);
	mSegmentList = new MPDSegmentListNode(*rhs.mSegmentList);

	return *this;
      };

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

      MPDDescriptorType(const MPDDescriptorType &copyin) 
	: RefBase(),
	  mSchemeIdUri(copyin.mSchemeIdUri),
	  mValue(copyin.mValue)
      {};
      
      MPDDescriptorType &operator=(const MPDDescriptorType &rhs) {
	mSchemeIdUri = rhs.mSchemeIdUri;
	mValue = rhs.mValue;

	return *this;
      };


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

      MPDContentComponentNode (const MPDContentComponentNode &copyin)
	: RefBase(),
	  mId(copyin.mId),
	  mLang(copyin.mLang),
	  mContentType(copyin.mContentType),
	  mPar(new MPDRatio(*copyin.mPar)),
	  mAccessibility(vector<MPDDescriptorType>(copyin.mAccessibility)),
	  mRole(vector<MPDDescriptorType>(copyin.mRole)),
	  mRating(vector<MPDDescriptorType>(copyin.mRating)),
	  mViewpoint(vector<MPDDescriptorType>(copyin.mViewpoint))
      {};
      MPDContentComponentNode &operator=(const MPDContentComponentNode &rhs)
      {
	mId = rhs.mId;
	mLang = rhs.mLang;
	mContentType = rhs.mContentType;
	mPar = new MPDRatio(*rhs.mPar);
	mAccessibility = vector<MPDDescriptorType>(rhs.mAccessibility);
	mRole = vector<MPDDescriptorType>(rhs.mRole);
	mRating = vector<MPDDescriptorType>(rhs.mRating);
	mViewpoint = vector<MPDDescriptorType>(rhs.mViewpoint);

	return *this;
      };
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
      
      MPDAdaptationSetNode (const MPDAdaptationSetNode &copyin)
	: RefBase(),
	  mId(copyin.mId),
	  mGroup(copyin.mGroup),
	  mLang(copyin.mLang),
	  mContentType(copyin.mContentType),
	  mPar(new MPDRatio(*copyin.mPar)),
	  mMinBandwidth(copyin.mMinBandwidth),
	  mMaxBandwidth(copyin.mMaxBandwidth),
	  mMinWidth(copyin.mMinWidth),
	  mMaxWidth(copyin.mMaxWidth),
	  mMinHeight(copyin.mMinHeight),
	  mMaxHeight(copyin.mMaxHeight),
	  mMinFrameRate(new MPDFrameRate(*copyin.mMinFrameRate)),
	  mMaxFrameRate(new MPDFrameRate(*copyin.mMaxFrameRate)),
	  mSegmentAlignment(new MPDConditionalUintType(*copyin.mSegmentAlignment)),
	  mSubSegmentAlignment(new MPDConditionalUintType(*copyin.mSubSegmentAlignment)),
	  mSubSegmentStartsWithSAP(copyin.mSubSegmentStartsWithSAP),
	  mBitstreamSwitching(copyin.mBitstreamSwitching),
	  mAccessibility(vector<MPDDescriptorType>(copyin.mAccessibility)),
	  mRole(vector<MPDDescriptorType>(copyin.mRole)),
	  mRating(vector<MPDDescriptorType>(copyin.mRating)),
	  mViewpoint(vector<MPDDescriptorType>(copyin.mViewpoint)),
	  mRepresentationBase(copyin.mRepresentationBase),
	  mSegmentBase(new MPDSegmentBaseType(*copyin.mSegmentBase)),
	  mSegmentList(new MPDSegmentListNode(*copyin.mSegmentList)),
	  mSegmentTemplate(copyin.mSegmentTemplate),
	  mBaseUrls(copyin.mBaseUrls),
	  mRepresentations(copyin.mRepresentations),
	  mContentComponents(copyin.mContentComponents)
      {};

      MPDAdaptationSetNode &operator=(const MPDAdaptationSetNode &rhs)
      {
	mId = rhs.mId;
	mGroup = rhs.mGroup;
	mLang = rhs.mLang;
	mContentType = rhs.mContentType;
	mPar = new MPDRatio(*rhs.mPar);
	mMinBandwidth = rhs.mMinBandwidth;
	mMaxBandwidth = rhs.mMaxBandwidth;
	mMinWidth = rhs.mMinWidth;
	mMaxWidth = rhs.mMaxWidth;
	mMinHeight = rhs.mMinHeight;
	mMaxHeight = rhs.mMaxHeight;
	mMinFrameRate = new MPDFrameRate(*rhs.mMinFrameRate);
	mMaxFrameRate = new MPDFrameRate(*rhs.mMaxFrameRate);
	mSegmentAlignment = new MPDConditionalUintType(*rhs.mSegmentAlignment);
	mSubSegmentAlignment = new MPDConditionalUintType(*rhs.mSubSegmentAlignment);
	mSubSegmentStartsWithSAP = rhs.mSubSegmentStartsWithSAP;
	mBitstreamSwitching = rhs.mBitstreamSwitching;
	mAccessibility = vector<MPDDescriptorType>(rhs.mAccessibility);
	mRole = vector<MPDDescriptorType>(rhs.mRole);
	mRating = vector<MPDDescriptorType>(rhs.mRating);
	mViewpoint = vector<MPDDescriptorType>(rhs.mViewpoint);
	mRepresentationBase = rhs.mRepresentationBase;
	mSegmentBase = new MPDSegmentBaseType(*rhs.mSegmentBase);
	mSegmentList = new MPDSegmentListNode(*rhs.mSegmentList);
	mSegmentTemplate = rhs.mSegmentTemplate;
	mBaseUrls = rhs.mBaseUrls;
	mRepresentations = rhs.mRepresentations;
	mContentComponents = rhs.mContentComponents;

	return *this;
      };
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

      MPDSubsetNode(const MPDSubsetNode &copyin)
	: RefBase(),
	  mContains(vector<uint32_t>(copyin.mContains)),
	  mSize(copyin.mSize)
      {};

      MPDSubsetNode &operator=(const MPDSubsetNode &rhs)
      {
	mContains = vector<uint32_t>(rhs.mContains);
	mSize = rhs.mSize;

	return *this;
      };
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

      MPDPeriodNode(const MPDPeriodNode &copyin)
	: RefBase(),
	  mId(copyin.mId),
	  mStart(copyin.mStart),
	  mDuration(copyin.mDuration),
	  mBitstreamSwitching(copyin.mBitstreamSwitching),
	  mSegmentBase(new MPDSegmentBaseType(*copyin.mSegmentBase)),
	  mSegmentList(new MPDSegmentListNode(*copyin.mSegmentList)),
	  mSegmentTemplate(new MPDSegmentTemplateNode(*copyin.mSegmentTemplate)),
	  mAdaptationSets(vector<MPDAdaptationSetNode>(copyin.mAdaptationSets)),
	  mSubsets(vector<MPDSubsetNode>(copyin.mSubsets)),
	  mBaseUrls(vector<MPDBaseUrl>(copyin.mBaseUrls))
      {};

      MPDPeriodNode &operator=(const MPDPeriodNode &rhs)
      {
	mId = rhs.mId;
	mStart = rhs.mStart;
	mDuration = rhs.mDuration;
	mBitstreamSwitching = rhs.mBitstreamSwitching;
	mSegmentBase = new MPDSegmentBaseType(*rhs.mSegmentBase);
	mSegmentList = new MPDSegmentListNode(*rhs.mSegmentList);
	mSegmentTemplate = new MPDSegmentTemplateNode(*rhs.mSegmentTemplate);
	mAdaptationSets = vector<MPDAdaptationSetNode>(rhs.mAdaptationSets);
	mSubsets = vector<MPDSubsetNode>(rhs.mSubsets);
	mBaseUrls = vector<MPDBaseUrl>(rhs.mBaseUrls);

	return *this;
      };
    };

    /*
     * MPDProgramInformationNode
     */
    class MPDProgramInformationNode : public RefBase {
    public:
      MPDProgramInformationNode()
	: mLang(AString("")),
	  mMoreInformationURL(AString("")),
	  mTitle(AString("")),
	  mSource(AString("")),
	  mCopyright(AString(""))
      {};

      AString mLang;                      /* LangVectorType RFC 5646 */
      AString mMoreInformationURL;
      /* children nodes */
      AString mTitle;
      AString mSource;
      AString mCopyright;
      
      MPDProgramInformationNode(const MPDProgramInformationNode &copyin)
	: RefBase(),
	  mLang(copyin.mLang),
	  mMoreInformationURL(copyin.mMoreInformationURL),
	  mTitle(copyin.mTitle),
	  mSource(copyin.mSource),
	  mCopyright(copyin.mCopyright)
      {};

      MPDProgramInformationNode &operator=(const MPDProgramInformationNode &rhs)
      {
	mLang = rhs.mLang;
	mMoreInformationURL = rhs.mMoreInformationURL;
	mTitle = rhs.mTitle;
	mSource = rhs.mSource;
	mCopyright = rhs.mCopyright;

	return *this;
      };
    };

    /*
     * MPDMetricsRangeNode
     */
    class MPDMetricsRangeNode : public RefBase {
    public:
      MPDMetricsRangeNode()
	: mStarttime(0L),
	  mDuration(0L)
      {};

      int64_t mStarttime;                  /* [ms] */
      int64_t mDuration;                   /* [ms] */
      
      MPDMetricsRangeNode(const MPDMetricsRangeNode &copyin)
	: RefBase(),
	  mStarttime(copyin.mStarttime),
	  mDuration(copyin.mDuration)
      {};

      MPDMetricsRangeNode &operator=(const MPDMetricsRangeNode &rhs)
      {
	mStarttime = rhs.mStarttime;
	mDuration = rhs.mDuration;

	return *this;
      };
    };

    /*
     * MPDMetricsNode
     */
    class MPDMetricsNode : public RefBase {
    public:
      MPDMetricsNode()
	: mMetrics(AString("")),
	  mMetricsRanges(vector<MPDMetricsRangeNode>()),
	  mReportings(vector<MPDDescriptorType>())
      {};

      AString mMetrics;
      /* list of Metrics Range nodes */
      vector<MPDMetricsRangeNode> mMetricsRanges;
      /* list of Reporting nodes */
      vector<MPDDescriptorType> mReportings;
      
      MPDMetricsNode(const MPDMetricsNode &copyin)
	: RefBase(),
	  mMetrics(copyin.mMetrics),
	  mMetricsRanges(vector<MPDMetricsRangeNode>(copyin.mMetricsRanges)),
	  mReportings(vector<MPDDescriptorType>(copyin.mReportings))
      {};

      MPDMetricsNode &operator=(const MPDMetricsNode &rhs)
      {
	mMetrics = rhs.mMetrics;
	mMetricsRanges = vector<MPDMetricsRangeNode>(rhs.mMetricsRanges);
	mReportings = vector<MPDDescriptorType>(rhs.mReportings);

	return *this;
      };
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

      uint16_t mYear;
      uint8_t mMonth;
      uint8_t mDay;
      uint8_t mHour;
      uint8_t mMinute;
      uint8_t mSecond;

      MPDDateTime(const MPDDateTime &copyin)
	: RefBase()
      {
	mYear = copyin.mYear;
	mMonth = copyin.mMonth;
	mDay = copyin.mDay;
	mHour = copyin.mHour;
	mMinute = copyin.mMinute;
	mSecond = copyin.mSecond;
      };

      MPDDateTime &operator=(const MPDDateTime &rhs)
      {
	mYear = rhs.mYear;
	mMonth = rhs.mMonth;
	mDay = rhs.mDay;
	mHour = rhs.mHour;
	mMinute = rhs.mMinute;
	mSecond = rhs.mSecond;

	return *this;
      };
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

      MPDMpdNode(const MPDMpdNode &copyin)
	: RefBase(),
	  mDefault_namespace(copyin.mDefault_namespace),
	  mNamespace_xsi(copyin.mNamespace_xsi),
	  mNamespace_ext(copyin.mNamespace_ext),
	  mSchemaLocation(copyin.mSchemaLocation),
	  mId(copyin.mId),
	  mProfiles(copyin.mProfiles),
	  mType(copyin.mType),
	  mAvailabilityStartTime(copyin.mAvailabilityStartTime),
	  mAvailabilityEndTime(copyin.mAvailabilityEndTime),
	  mMediaPresentationDuration(copyin.mMediaPresentationDuration),
	  mMinimumUpdatePeriod(copyin.mMinimumUpdatePeriod),
	  mMinBufferTime(copyin.mMinBufferTime),
	  mTimeShiftBufferDepth(copyin.mTimeShiftBufferDepth),
	  mSuggestedPresentationDelay(copyin.mSuggestedPresentationDelay),
	  mMaxSegmentDuration(copyin.mMaxSegmentDuration),
	  mMaxSubSegmentDuration(copyin.mMaxSubSegmentDuration),
	  mBaseUrls(vector<MPDBaseUrl>(copyin.mBaseUrls)),
	  mLocations(vector<AString>(copyin.mLocations)),
	  mProgramInfo(vector<MPDProgramInformationNode>(copyin.mProgramInfo)),
	  mPeriods(vector<MPDPeriodNode>(copyin.mPeriods)),
	  mMetrics(vector<MPDMetricsNode>(copyin.mMetrics))
      {};
      
      MPDMpdNode &operator=(const MPDMpdNode &rhs)
      {
	mDefault_namespace = rhs.mDefault_namespace;
	mNamespace_xsi = rhs.mNamespace_xsi;
	mNamespace_ext = rhs.mNamespace_ext;
	mSchemaLocation = rhs.mSchemaLocation;
	mId = rhs.mId;
	mProfiles = rhs.mProfiles;
	mType = rhs.mType;
	mAvailabilityStartTime = rhs.mAvailabilityStartTime;
	mAvailabilityEndTime = rhs.mAvailabilityEndTime;
	mMediaPresentationDuration = rhs.mMediaPresentationDuration;
	mMinimumUpdatePeriod = rhs.mMinimumUpdatePeriod;
	mMinBufferTime = rhs.mMinBufferTime;
	mTimeShiftBufferDepth = rhs.mTimeShiftBufferDepth;
	mSuggestedPresentationDelay = rhs.mSuggestedPresentationDelay;
	mMaxSegmentDuration = rhs.mMaxSegmentDuration;
	mMaxSubSegmentDuration = rhs.mMaxSubSegmentDuration;
	mBaseUrls = vector<MPDBaseUrl>(rhs.mBaseUrls);
	mLocations = vector<AString>(rhs.mLocations);
	mProgramInfo = vector<MPDProgramInformationNode>(rhs.mProgramInfo);
	mPeriods = vector<MPDPeriodNode>(rhs.mPeriods);
	mMetrics = vector<MPDMetricsNode>(rhs.mMetrics);

	return *this;
      };
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
      
      MPDStreamPeriod(const MPDStreamPeriod &copyin)
	: RefBase()
      {
	mPeriod = MPDPeriodNode(copyin.mPeriod);
	mNumber = copyin.mNumber;
	mStart = copyin.mStart;
	mDuration = copyin.mDuration;
      };

      MPDStreamPeriod &operator=(const MPDStreamPeriod &rhs)
      {
	mPeriod = MPDPeriodNode(rhs.mPeriod);
	mNumber = rhs.mNumber;
	mStart = rhs.mStart;
	mDuration = rhs.mDuration;

	return *this;
      };
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

      MPDMediaSegment(const MPDMediaSegment &copyin)
	: RefBase()
      {
	mSegmentURL = new MPDSegmentUrlNode(*copyin.mSegmentURL);
	mNumber = copyin.mNumber;
	mStart = copyin.mStart;
	mDuration = copyin.mDuration;
      };

      MPDMediaSegment &operator=(const MPDMediaSegment &rhs)
      {
	mSegmentURL = new MPDSegmentUrlNode(*rhs.mSegmentURL);
	mNumber = rhs.mNumber;
	mStart = rhs.mStart;
	mDuration = rhs.mDuration;

	return *this;
      };
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

      MPDMediaFragmentInfo(const MPDMediaFragmentInfo &copyin)
	: RefBase()
      {
	mUri = copyin.mUri;
	mRangeStart = copyin.mRangeStart;
	mRangeEnd = copyin.mRangeEnd;
	mIndexUri = copyin.mIndexUri;
	mIndexRangeStart = copyin.mIndexRangeStart;
	mIndexRangeEnd = copyin.mIndexRangeEnd;
	mDiscontinuity = copyin.mDiscontinuity;
	mTimestamp = copyin.mTimestamp;
	mDuration = copyin.mDuration;
      };

      MPDMediaFragmentInfo &operator=(const MPDMediaFragmentInfo &rhs)
      {
	mUri = rhs.mUri;
	mRangeStart = rhs.mRangeStart;
	mRangeEnd = rhs.mRangeEnd;
	mIndexUri = rhs.mIndexUri;
	mIndexRangeStart = rhs.mIndexRangeStart;
	mIndexRangeEnd = rhs.mIndexRangeEnd;
	mDiscontinuity = rhs.mDiscontinuity;
	mTimestamp = rhs.mTimestamp;
	mDuration = rhs.mDuration;

	return *this;
      };
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
      
      MPDActiveStream(const MPDActiveStream &copyin)
	: RefBase()
      {
	mimeType = copyin.mimeType;
	mBaseUrlIdx = copyin.mBaseUrlIdx;
	mBaseUrl = copyin.mBaseUrl;
	mQueryUrl = copyin.mQueryUrl;
	mMaxBandwidth = copyin.mMaxBandwidth;
	mCurAdaptSet = new MPDAdaptationSetNode(*copyin.mCurAdaptSet);
	mRepresentationIdx = copyin.mRepresentationIdx;
	mCurRepresentation = new MPDRepresentationNode(*copyin.mCurRepresentation);
	mCurSegmentBase = new MPDSegmentBaseType(*copyin.mCurSegmentBase);
	mCurSegmentList = new MPDSegmentListNode(*copyin.mCurSegmentList);
	mCurSegTemplate = new MPDSegmentTemplateNode(*copyin.mCurSegTemplate);;
	mSegmentIdx = copyin.mSegmentIdx;
	mSegments = vector<MPDMediaSegment>(mSegments);
      };

      MPDActiveStream &operator=(const MPDActiveStream &rhs)
      {
	mimeType = rhs.mimeType;
	mBaseUrlIdx = rhs.mBaseUrlIdx;
	mBaseUrl = rhs.mBaseUrl;
	mQueryUrl = rhs.mQueryUrl;
	mMaxBandwidth = rhs.mMaxBandwidth;
	mCurAdaptSet = new MPDAdaptationSetNode(*rhs.mCurAdaptSet);
	mRepresentationIdx = rhs.mRepresentationIdx;
	mCurRepresentation = new MPDRepresentationNode(*rhs.mCurRepresentation);
	mCurSegmentBase = new MPDSegmentBaseType(*rhs.mCurSegmentBase);
	mCurSegmentList = new MPDSegmentListNode(*rhs.mCurSegmentList);
	mCurSegTemplate = new MPDSegmentTemplateNode(*rhs.mCurSegTemplate);;
	mSegmentIdx = rhs.mSegmentIdx;
	mSegments = vector<MPDMediaSegment>(mSegments);

	return *this;
      };
    };

    /*
     * MPDMpdClient
     */
    class MPDMpdClient : public RefBase {
    public:
      MPDMpdClient(const char *baseUri)
	: RefBase(),
	  mMpdNode((MPDMpdNode *)NULL),
	  mPeriods(vector<MPDStreamPeriod>()),
	  mActiveStreams(vector<MPDActiveStream>()),
	  mUpdateFailedCount(0),
	  mMpdUri(AString(baseUri))
      {};
	  
      MPDMpdNode *mMpdNode;                       /* active MPD manifest file */

      vector<MPDStreamPeriod> mPeriods;           /* list of MPDStreamPeriod */
      uint32_t mPeriodIdx;                        /* index of current Period */

      vector<MPDActiveStream> mActiveStreams;     /* list of MPDActiveStream */

      uint32_t mUpdateFailedCount;
      AString mMpdUri;                            /* manifest file URI */
      Mutex mLock;
      
      MPDMpdClient(const MPDMpdClient &copyin)
	: RefBase()
      {
	mMpdNode = new MPDMpdNode(*mMpdNode);
	mPeriods = vector<MPDStreamPeriod>(mPeriods);
	mPeriodIdx = copyin.mPeriodIdx;
	mActiveStreams = vector<MPDActiveStream>(mActiveStreams);
	mUpdateFailedCount = copyin.mUpdateFailedCount;
	mMpdUri = copyin.mMpdUri;
      };

      MPDMpdClient &operator=(const MPDMpdClient &rhs)
      {
	mMpdNode = new MPDMpdNode(*mMpdNode);
	mPeriods = vector<MPDStreamPeriod>(mPeriods);
	mPeriodIdx = rhs.mPeriodIdx;
	mActiveStreams = vector<MPDActiveStream>(mActiveStreams);
	mUpdateFailedCount = rhs.mUpdateFailedCount;
	mMpdUri = rhs.mMpdUri;

	return *this;
      };
    };

    /*
     * This class is used for managing chunks bandwidth
     */
    class Item : public RefBase {
    public:
      Item()
	: mUri(),
	  mMeta()
      {};

      AString mUri;
      sp<AMessage> mMeta;

      Item(const Item &copyin)
	: RefBase()
      {
	mUri = copyin.mUri;
	mMeta = sp<AMessage>(copyin.mMeta);
      };

      Item &operator=(const Item &rhs)
      {
	mUri = rhs.mUri;
	mMeta = sp<AMessage>(rhs.mMeta);
	
	return *this;
      };
    };


    /* =================
     * Parser properties
     */

    MPDMpdClient *mClient;

    status_t mInitCheck;

    AString mBaseURI;
    bool mIsComplete;
    bool mIsEvent;
    bool mIsVariant;
    bool mIsVariantComputed;
    
    sp<AMessage> mMeta;
    Vector<Item> mItems;

    status_t parse(const void *data, size_t size);

    MPDParser(const MPDParser &copyin)
      : RefBase(),
	mInitCheck(copyin.mInitCheck),
	mBaseURI(copyin.mBaseURI),
	mIsComplete(copyin.mIsComplete),
	mIsEvent(copyin.mIsEvent),
	mIsVariant(copyin.mIsVariant),
	mIsVariantComputed(copyin.mIsVariantComputed),
	mMeta(sp<AMessage>(copyin.mMeta)),
	mItems(Vector<Item>(copyin.mItems))
    {};
    
    MPDParser &operator=(const MPDParser &rhs)
    {
      mInitCheck = rhs.mInitCheck;
      mBaseURI = rhs.mBaseURI;
      mIsComplete = rhs.mIsComplete;
      mIsEvent = rhs.mIsEvent;
      mIsVariant = rhs.mIsVariant;
      mIsVariantComputed = rhs.mIsVariantComputed;
      mMeta = sp<AMessage>(rhs.mMeta);
      mItems = Vector<Item>(rhs.mItems);

      return *this;
    };
  };

};

#endif /* MPD_PARSER_H_ */

// Local Variables:
// mode:C++
// End:
