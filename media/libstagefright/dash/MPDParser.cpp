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
#define LOG_TAG "MPDParser"
#include <utils/Log.h>

#include "include/MPDParser.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaErrors.h>

namespace android {

  MPDParser::MPDParser(
	const char *baseURI, const void *data, size_t size)
    : mInitCheck(NO_INIT),
      mBaseURI(baseURI),
      mIsComplete(false),
      mIsEvent(false) {
    mInitCheck = parse(data, size);
  }

  MPDParser::~MPDParser() {
  }

  status_t MPDParser::initCheck() const {
    return mInitCheck;
  }

  bool MPDParser::isComplete() const {
    return mIsComplete;
  }
  
  bool MPDParser::isEvent() const {
    return mIsEvent;
  }

  sp<AMessage> MPDParser::meta() {
    return mMeta;
  }

  size_t MPDParser::size() {
    return mItems.size();
  }

  bool MPDParser::itemAt(size_t index, AString *uri, sp<AMessage> *meta) {
    if (uri) {
      uri->clear();
    }
    
    if (meta) {
      *meta = NULL;
    }

    if (index >= mItems.size()) {
      return false;
    }

    if (uri) {
      *uri = mItems.itemAt(index).mURI;
    }

    if (meta) {
      *meta = mItems.itemAt(index).mMeta;
    }

    return true;
  }

  static bool MakeURL(const char *baseURL, const char *url, AString *out) {
    out->clear();

    if (strncasecmp("http://", baseURL, 7)
	&& strncasecmp("https://", baseURL, 8)
	&& strncasecmp("file://", baseURL, 7)) {
      // Base URL must be absolute
      return false;
    }

    if (!strncasecmp("http://", url, 7) || !strncasecmp("https://", url, 8)) {
      // "url" is already an absolute URL, ignore base URL.
      out->setTo(url);
      
      ALOGV("base:'%s', url:'%s' => '%s'", baseURL, url, out->c_str());
      
      return true;
    }
    
    if (url[0] == '/') {
      // URL is an absolute path.
      
      char *protocolEnd = strstr(baseURL, "//") + 2;
      char *pathStart = strchr(protocolEnd, '/');
      
      if (pathStart != NULL) {
	out->setTo(baseURL, pathStart - baseURL);
      } else {
	out->setTo(baseURL);
      }
      
      out->append(url);
    } else {
      // URL is a relative path
      
      size_t n = strlen(baseURL);
      if (baseURL[n - 1] == '/') {
	out->setTo(baseURL);
	out->append(url);
      } else {
	const char *slashPos = strrchr(baseURL, '/');
	
	if (slashPos > &baseURL[6]) {
	  out->setTo(baseURL, slashPos - baseURL);
	} else {
	  out->setTo(baseURL);
	}
	
	out->append("/");
	out->append(url);
      }
    }

    ALOGV("base:'%s', url:'%s' => '%s'", baseURL, url, out->c_str());

    return true;
  }

  static AString *
  mpdparser_get_xml_node_namespace (xmlNode *a_node, const char *prefix)
  {
    xmlNs *curr_ns;
    char *namespace = NULL;

    if (prefix == NULL) 
      {
	/* return the default namespace */
	namespace = xmlMemStrdup ((const char *) a_node->ns->href);
	if (namespace) 
	  {
	    ALOGV (" - default namespace: %s", namespace);
	  }
      } 
    else 
      {
	/* look for the specified prefix in the namespace list */
	for (curr_ns = a_node->ns; curr_ns; curr_ns = curr_ns->next) {
	  if (xmlStrcmp (curr_ns->prefix, (xmlChar *) prefix) == 0) {
	    namespace = xmlMemStrdup ((const char *) curr_ns->href);
	    if (namespace)
	      {
		ALOGV (" - %s namespace: %s", curr_ns->prefix, curr_ns->href);
	      }
	  }
	}
      }
    
    return new AString(namespace);
  }

  /*
    DateTime Data Type
    
    The dateTime data type is used to specify a date and a time.
    
    The dateTime is specified in the following form "YYYY-MM-DDThh:mm:ss" where:
    
    * YYYY indicates the year
    * MM indicates the month
    * DD indicates the day
    * T indicates the start of the required time section
    * hh indicates the hour
    * mm indicates the minute
    * ss indicates the second
    
    Note: All components are required!
  */
  static gboolean
  mpdparser_get_xml_prop_dateTime (xmlNode * a_node,
				   const char * property_name, 
				   MPDDateTime ** property_value)
  {
    xmlChar *prop_string;
    char *str;
    int ret, pos;
    uint16_t year;
    uint8_t month, day, hour, minute, second;
    bool exists = FALSE;
    
    prop_string = xmlGetProp (a_node, (const xmlChar *) property_name);
    if (prop_string) 
      {
	str = (gchar *) prop_string;
	ALOGV ("dateTime: %s, len %d", str, xmlStrlen (prop_string));

	/* parse year */
	ret = sscanf (str, "%d", &year);
	if (ret != 1)
	  goto error;
	pos = strcspn (str, "-");
	str += (pos + 1);
	ALOGV (" - year %d", year);

	/* parse month */
	ret = sscanf (str, "%d", &month);
	if (ret != 1)
	  goto error;
	pos = strcspn (str, "-");
	str += (pos + 1);
	ALOGV (" - month %d", month);

	/* parse day */
	ret = sscanf (str, "%d", &day);
	if (ret != 1)
	  goto error;
	pos = strcspn (str, "T");
	str += (pos + 1);
	ALOGV (" - day %d", day);

	/* parse hour */
	ret = sscanf (str, "%d", &hour);
	if (ret != 1)
	  goto error;
	pos = strcspn (str, ":");
	str += (pos + 1);
	ALOGV (" - hour %d", hour);
	
	/* parse minute */
	ret = sscanf (str, "%d", &minute);
	if (ret != 1)
	  goto error;
	pos = strcspn (str, ":");
	str += (pos + 1);
	ALOGV (" - minute %d", minute);
	
	/* parse second */
	ret = sscanf (str, "%d", &second);
	if (ret != 1)
	  goto error;
	ALOGV (" - second %d", second);
      
	ALOGV (" - %s: %4d/%02d/%02d %02d:%02d:%02d", 
	       property_name,
	       year, month, day, hour, minute, second);
      
	exists = TRUE;
	*property_value =
	  new MPDDateTime(year, month, day, hour, minute, second);
	xmlFree (prop_string);
      }
    
    return exists;
    
  error:
    xmlFree (prop_string);
    ALOGW ("failed to parse property %s from xml string %s", 
	   property_name, prop_string);
    return exists;
  }

  /*
    Duration Data Type

    The duration data type is used to specify a time interval.

    The time interval is specified in the following form "-PnYnMnDTnHnMnS" where:

      * - indicates the negative sign (optional)
      * P indicates the period (required)
      * nY indicates the number of years
      * nM indicates the number of months
      * nD indicates the number of days
      * T indicates the start of a time section (required if you are going to specify hours, minutes, or seconds)
      * nH indicates the number of hours
      * nM indicates the number of minutes
      * nS indicates the number of seconds
    */

  /* this function computes decimals * 10 ^ (3 - pos) */
  static int64_t
  convert_to_millisecs (gint decimals, gint pos)
  {
    int num = 1, den = 1, i = 3 - pos;

    while (i < 0) 
      {
	den *= 10;
	i++;
      }
    while (i > 0) 
      {
	num *= 10;
	i--;
      }

    /* if i == 0 we have exactly 3 decimals and nothing to do */
    return decimals * num / den;
  }

  static gboolean
  mpdparser_get_xml_prop_duration (xmlNode * a_node,
				   const gchar * property_name, 
				   int64_t default_value, 
				   int64_t * property_value)
  {
    xmlChar *prop_string;
    char *str;
    int ret, read, len, pos, posT;
    int years = 0, months = 0, days = 0, 
      hours = 0, minutes = 0, seconds = 0, 
      decimals = 0;
    int sign = 1;
    bool have_ms = FALSE;
    bool exists = FALSE;

    prop_string = xmlGetProp (a_node, (const xmlChar *) property_name);
    if (prop_string) 
      {
	len = xmlStrlen (prop_string);
	str = (char *) prop_string;
	ALOGV ("duration: %s, len %d", str, len);
	/* read "-" for sign, if present */
	pos = strcspn (str, "-");
	if (pos < len)             /* found "-" */
	  {
	    if (pos != 0) 
	      {
		ALOGW ("sign \"-\" non at the beginning of the string");
		goto error;
	      }
	    ALOGV ("found - sign at the beginning");
	    sign = -1;
	    str++;
	    len--;
	  }
	/* read "P" for period */
	pos = strcspn (str, "P");
	if (pos != 0) 
	  {
	    ALOGW ("P not found at the beginning of the string!");
	    goto error;
	  }
	str++;
	len--;
	/* read "T" for time (if present) */
	posT = strcspn (str, "T");
	len -= posT;
	if (posT > 0) 
	  {
	    /* there is some room between P and T, so there must be a period section */
	    /* read years, months, days */
	    do 
	      {
		ALOGV ("parsing substring %s", str);
		pos = strcspn (str, "YMD");
		ret = sscanf (str, "%d", &read);
		if (ret != 1) 
		  {
		    ALOGW ("can not read integer value from string %s!", str);
		    goto error;
		  }
		switch (str[pos]) 
		  {
		  case 'Y':
		    years = read;
		    break;
		  case 'M':
		    months = read;
		    break;
		  case 'D':
		    days = read;
		    break;
		  default:
		    ALOGW ("unexpected char %c!", str[pos]);
		    goto error;
		    break;
		  }
		ALOGV ("read number %d type %c", read, str[pos]);
		str += (pos + 1);
		posT -= (pos + 1);
	      } 
	    while (posT > 0);

	    ALOGV ("Y:M:D=%d:%d:%d", years, months, days);
	  }
	/* read "T" for time (if present) */
	/* here T is at pos == 0 */
	str++;
	len--;
	pos = 0;
	if (pos < len) 
	  {
	    /* T found, there is a time section */
	    /* read hours, minutes, seconds, cents of second */
	    do 
	      {
		ALOGV ("parsing substring %s", str);
		pos = strcspn (str, "HMS,.");
		ret = sscanf (str, "%d", &read);
		if (ret != 1) 
		  {
		    ALOGW ("can not read integer value from string %s!", str);
		    goto error;
		  }
		switch (str[pos]) 
		  {
		  case 'H':
		    hours = read;
		    break;
		  case 'M':
		    minutes = read;
		    break;
		  case 'S':
		    if (have_ms) 
		      {
			/* we have read the decimal part of the seconds */
			decimals = convert_to_millisecs (read, pos);
			ALOGV ("decimal number %d (%d digits) -> %d ms", read, pos,
			       decimals);
		      } 
		    else 
		      {
			/* no decimals */
			seconds = read;
		      }
		    break;
		  case '.':
		  case ',':
		    /* we have read the integer part of a decimal number in seconds */
		    seconds = read;
		    have_ms = TRUE;
		    break;

		  default:
		    ALOGV ("unexpected char %c!", str[pos]);
		    goto error;
		    break;
		  }
		ALOGV ("read number %d type %c", read, str[pos]);
		str += pos + 1;
		len -= (pos + 1);
	      } 
	    while (len > 0);

	    ALOGV ("H:M:S.MS=%d:%d:%d.%03d", hours, minutes, seconds, decimals);
	  }

	xmlFree (prop_string);
	exists = TRUE;
	*property_value =
	  sign * ((((((int64) years * 365 + months * 30 + days) * 24 +
		     hours) * 60 + minutes) * 60 + seconds) * 1000 + decimals);
	ALOGV (" - %s: %ld", property_name, *property_value);
      }

    if (!exists) 
      {
	*property_value = default_value;
      }
    return exists;

  error:
    xmlFree (prop_string);
    return exists;
  }

  static bool
  mpdparser_get_xml_prop_string (xmlNode * a_node,
				 const gchar * property_name, 
				 gchar ** property_value)
  {
    xmlChar *prop_string;
    bool exists = FALSE;

    prop_string = xmlGetProp (a_node, (const xmlChar *) property_name);
    if (prop_string) 
      {
	*property_value = new AString((char *) prop_string);
	exists = TRUE;
	ALOGV (" - %s: %s", property_name, prop_string);
      }
    
    return exists;
  }

  static bool
  mpdparser_get_xml_prop_boolean (xmlNode * a_node,
				  const char * property_name, 
				  bool default_val,
				  bool * property_value)
  {
    xmlChar *prop_string;
    bool exists = FALSE;

    *property_value = default_val;
    prop_string = xmlGetProp (a_node, (const xmlChar *) property_name);
    if (prop_string) 
      {
	if (xmlStrcmp (prop_string, (xmlChar *) "false") == 0) 
	  {
	    exists = TRUE;
	    *property_value = FALSE;
	    ALOGV (" - %s: false", property_name);
	  } 
	else if (xmlStrcmp (prop_string, (xmlChar *) "true") == 0) 
	  {
	    exists = TRUE;
	    *property_value = TRUE;
	    ALOGV (" - %s: true", property_name);
	  } 
	else 
	  {
	    ALOGW ("failed to parse boolean property %s from xml string %s",
		   property_name, prop_string);
	  }
	xmlFree (prop_string);
      }

    return exists;
  }

  static void
  mpdparser_parse_seg_base_type_ext (MPDSegmentBaseType ** pointer,
				     xmlNode * a_node, MPDSegmentBaseType * parent)
  {
    xmlNode *cur_node;
    MPDSegmentBaseType *seg_base_type;
    uint32_t intval;
    bool boolval;
    MPDRange *rangeval;

    mpdparser_free_seg_base_type_ext (*pointer);
    *pointer = seg_base_type = new MPDSegmentBaseType();

    /* Initialize values that have defaults */
    seg_base_type->mIndexRangeExact = FALSE;

    /* Inherit attribute values from parent */
    if (parent) 
      {
	seg_base_type->mTimescale = parent->mTimescale;
	seg_base_type->mPresentationTimeOffset = parent->mPresentationTimeOffset;
	seg_base_type->mIndexRange = mpdparser_clone_range (parent->mIndexRange);
	seg_base_type->mIndexRangeExact = parent->mIndexRangeExact;
	seg_base_type->mInitialization =
	  mpdparser_clone_URL (parent->mInitialization);
	seg_base_type->mRepresentationIndex =
	  mpdparser_clone_URL (parent->mRepresentationIndex);
      }
    
    /* We must retrieve each value first to see if it exists.  If it does not
     * exist, we do not want to overwrite an inherited value 
     */
    ALOGV ("attributes of SegmentBaseType extension:");
    if (mpdparser_get_xml_prop_unsigned_integer (a_node, "timescale", 0,
						 &intval)) 
      {
	seg_base_type->mTimescale = intval;
      }
    if (mpdparser_get_xml_prop_unsigned_integer (a_node,
						 "presentationTimeOffset", 0, &intval)) 
      {
	seg_base_type->mPresentationTimeOffset = intval;
      }
    if (mpdparser_get_xml_prop_range (a_node, "indexRange", &rangeval)) 
      {
	if (seg_base_type->indexRange) 
	  {
	    /*TBD*/ g_slice_free (GstRange, seg_base_type->indexRange);
	  }
	seg_base_type->mIndexRange = rangeval;
      }
    if (mpdparser_get_xml_prop_boolean (a_node, "indexRangeExact",
					FALSE, &boolval)) 
      {
	seg_base_type->mIndexRangeExact = boolval;
      }

    /* explore children nodes */
    for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) 
      {
	if (cur_node->type == XML_ELEMENT_NODE) 
	  {
	    if (xmlStrcmp (cur_node->name, (xmlChar *) "Initialization") == 0 ||
		xmlStrcmp (cur_node->name, (xmlChar *) "Initialisation") == 0) 
	      {
		if (seg_base_type->Initialization) 
		  {
		    mpdparser_free_url_type_node (seg_base_type->mInitialization);
		  }
		mpdparser_parse_url_type_node (&seg_base_type->mInitialization,
					       cur_node);
	      } 
	    else if (xmlStrcmp (cur_node->name,
				(xmlChar *) "RepresentationIndex") == 0) 
	      {
		if (seg_base_type->mRepresentationIndex) 
		  {
		    mpdparser_free_url_type_node (seg_base_type->mRepresentationIndex);
		  }
		mpdparser_parse_url_type_node (&seg_base_type->mRepresentationIndex,
					       cur_node);
	      }
	  }
      }
  }

  static void
  mpdparser_parse_period_node (GList ** list, xmlNode * a_node)
  {
    xmlNode *cur_node;
    MPDPeriodNode *new_period;

    new_period = new MPDPeriodNode();
    if (new_period == NULL) 
      {
	ALOGW ("Allocation of Period node failed!");
	return;
      }
    *list = g_list_append (*list, new_period);

    new_period->start = GST_CLOCK_TIME_NONE;

    ALOGV ("attributes of Period node:");
    mpdparser_get_xml_prop_string (a_node, "id", &new_period->mId);
    mpdparser_get_xml_prop_duration (a_node, "start", -1, &new_period->mStart);
    mpdparser_get_xml_prop_duration (a_node, "duration", -1,
				     &new_period->mDuration);
    mpdparser_get_xml_prop_boolean (a_node, "bitstreamSwitching",
				    FALSE, &new_period->bitstreamSwitching);

    /* explore children nodes */
    for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) 
      {
	if (cur_node->type == XML_ELEMENT_NODE) 
	  {
	    if (xmlStrcmp (cur_node->name, (xmlChar *) "SegmentBase") == 0) 
	      {
		mpdparser_parse_seg_base_type_ext (&new_period->mSegmentBase,
						   cur_node, NULL);
	      } 
	    else if (xmlStrcmp (cur_node->name, (xmlChar *) "SegmentList") == 0) 
	      {
		mpdparser_parse_segment_list_node (&new_period->mSegmentList,
						   cur_node, NULL);
	      } 
	    else if (xmlStrcmp (cur_node->name, (xmlChar *) "SegmentTemplate") == 0) 
	      {
		mpdparser_parse_segment_template_node (&new_period->mSegmentTemplate,
						       cur_node, NULL);
	      } 
	    else if (xmlStrcmp (cur_node->name, (xmlChar *) "Subset") == 0) 
	      {
		mpdparser_parse_subset_node (&new_period->mSubsets, cur_node);
	      } 
	    else if (xmlStrcmp (cur_node->name, (xmlChar *) "BaseURL") == 0) 
	      {
		mpdparser_parse_baseURL_node (&new_period->mBaseURLs, cur_node);
	      }
	  }
      }
    
    /* We must parse AdaptationSet after everything else in the Period has been
     * parsed because certain AdaptationSet child elements can inherit attributes
     * specified by the same element in the Period
     */
    for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) 
      {
	if (cur_node->type == XML_ELEMENT_NODE) 
	  {
	    if (xmlStrcmp (cur_node->name, (xmlChar *) "AdaptationSet") == 0) 
	      {
		mpdparser_parse_adaptation_set_node (&new_period->mAdaptationSets,
						     cur_node, new_period);
	      }
	  }
      }
  }
  
  static void
  parse_root_node (MPDMpdNode **an_mpdNode, xmlNode * a_node)
  {
    xmlNode *cur_node;
    MPDMpdNode *new_mpd;

    delete *an_mpdNode;
    *an_mpdNode = new_mpd = new MPDMpdNode();

    ALOGV ("namespaces of root MPD node:");
    new_mpd->mDefault_namespace =
      mpdparser_get_xml_node_namespace (a_node, NULL);
    new_mpd->mNamespace_xsi = 
      mpdparser_get_xml_node_namespace (a_node, "xsi");
    new_mpd->mNamespace_ext = 
      mpdparser_get_xml_node_namespace (a_node, "ext");

    ALOGV ("attributes of root MPD node:");
    
    mpdparser_get_xml_prop_string (a_node, "schemaLocation",
				   &new_mpd->mSchemaLocation);
    mpdparser_get_xml_prop_string (a_node, "id", 
				   &new_mpd->mId);
    mpdparser_get_xml_prop_string (a_node, "profiles", 
				   &new_mpd->mProfiles);
    mpdparser_get_xml_prop_type (a_node, "type", 
				 &new_mpd->type);
    mpdparser_get_xml_prop_dateTime (a_node, "availabilityStartTime",
				     &new_mpd->mAvailabilityStartTime);
    mpdparser_get_xml_prop_dateTime (a_node, "availabilityEndTime",
				     &new_mpd->mAvailabilityEndTime);

    mpdparser_get_xml_prop_duration (a_node, "mediaPresentationDuration", -1,
				     &new_mpd->mMediaPresentationDuration);
    mpdparser_get_xml_prop_duration (a_node, "minimumUpdatePeriod", -1,
				     &new_mpd->mMinimumUpdatePeriod);
    mpdparser_get_xml_prop_duration (a_node, "minBufferTime", -1,
				     &new_mpd->mMinBufferTime);
    mpdparser_get_xml_prop_duration (a_node, "timeShiftBufferDepth", -1,
				     &new_mpd->mTimeShiftBufferDepth);
    mpdparser_get_xml_prop_duration (a_node, "suggestedPresentationDelay", -1,
				     &new_mpd->mSuggestedPresentationDelay);
    mpdparser_get_xml_prop_duration (a_node, "maxSegmentDuration", -1,
				     &new_mpd->mMaxSegmentDuration);
    mpdparser_get_xml_prop_duration (a_node, "maxSubsegmentDuration", -1,
				     &new_mpd->mMaxSubsegmentDuration);

    /* explore children Period nodes */
    for (cur_node = a_node->children; cur_node; cur_node = cur_node->next)
      {
	if (cur_node->type == XML_ELEMENT_NODE) 
	  {
	    if (xmlStrcmp (cur_node->name, (xmlChar *) "Period") == 0) {
	      mpdparser_parse_period_node (&new_mpd->Periods, cur_node);
	    } else if (xmlStrcmp (cur_node->name,
				  (xmlChar *) "ProgramInformation") == 0) {
	      gst_mpdparser_parse_program_info_node (&new_mpd->ProgramInfo, cur_node);
	    } else if (xmlStrcmp (cur_node->name, (xmlChar *) "BaseURL") == 0) {
	      gst_mpdparser_parse_baseURL_node (&new_mpd->BaseURLs, cur_node);
	    } else if (xmlStrcmp (cur_node->name, (xmlChar *) "Location") == 0) {
	      gst_mpdparser_parse_location_node (&new_mpd->Locations, cur_node);
	    } else if (xmlStrcmp (cur_node->name, (xmlChar *) "Metrics") == 0) {
	      gst_mpdparser_parse_metrics_node (&new_mpd->Metrics, cur_node);
	    }
	  }
      }
  }

  status_t MPDParser::parse(const void *_data, size_t size) 
  {
    int32_t lineNo = 0;
    sp<AMessage> itemMeta;

    if (mClient != (MPDMpdClient *)NULL && mClient->mMpdNode != (MPDMpdNode *)NULL)
      {
	delete mClient->mMpdNode;
	mClient->mMpdNode = (MPDMpdNode *)NULL;
      }

    const char *data = (const char *)_data;
    if (data) {
      xmlDocPtr doc;
      xmlNode *root_element = NULL;

      ALOGV ("MPD file fully buffered, start parsing...");

      /* parse the complete MPD file into a tree (using the libxml2 default parser API) */

      /* this initialize the library and check potential ABI mismatches
       * between the version it was compiled for and the actual shared
       * library used
       */
      LIBXML_TEST_VERSION

      /* parse "data" into a document (which is a libxml2 tree structure xmlDoc) */
      doc = xmlReadMemory (data, size, "unnamed-mpd.xml", NULL, 0);

      if (doc == NULL) 
	{
	  ALOGE ("failed to parse the MPD file");
	  return BAD_TYPE;
	} 
      else 
	{
	  /* get the root element node */
	  root_element = xmlDocGetRootElement (doc);	
	  if (root_element->type != XML_ELEMENT_NODE ||
	      xmlStrcmp (root_element->name, (xmlChar *) "MPD") != 0)
	    {
	      ALOGE("can not find the root element MPD, failed to parse the MPD file");
	    }
	  else 
	    /* now we can parse the MPD root node and all children nodes, recursively */
	    parse_root_node (&mClient->mMpdNode, root_element);
	
	  /* free the document */
	  xmlFreeDoc (doc);
	}
      
      return NO_ERROR;
    }

    return BAD_VALUE;
  }

