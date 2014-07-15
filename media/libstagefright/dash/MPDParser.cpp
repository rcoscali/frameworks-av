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

  /* memory management functions */
  static void
  free_mpd_node (GstMPDNode * mpd_node)
  {
    if (mpd_node) {
    if (mpd_node->default_namespace)
      xmlFree (mpd_node->default_namespace);
    if (mpd_node->namespace_xsi)
      xmlFree (mpd_node->namespace_xsi);
    if (mpd_node->namespace_ext)
      xmlFree (mpd_node->namespace_ext);
    if (mpd_node->schemaLocation)
      xmlFree (mpd_node->schemaLocation);
    if (mpd_node->id)
      xmlFree (mpd_node->id);
    if (mpd_node->profiles)
      xmlFree (mpd_node->profiles);
    if (mpd_node->availabilityStartTime)
      gst_date_time_unref (mpd_node->availabilityStartTime);
    if (mpd_node->availabilityEndTime)
      gst_date_time_unref (mpd_node->availabilityEndTime);
    g_list_free_full (mpd_node->ProgramInfo,
        (GDestroyNotify) gst_mpdparser_free_prog_info_node);
    g_list_free_full (mpd_node->BaseURLs,
        (GDestroyNotify) gst_mpdparser_free_base_url_node);
    g_list_free_full (mpd_node->Locations, (GDestroyNotify) xmlFree);
    g_list_free_full (mpd_node->Periods,
        (GDestroyNotify) gst_mpdparser_free_period_node);
    g_list_free_full (mpd_node->Metrics,
        (GDestroyNotify) gst_mpdparser_free_metrics_node);
    g_slice_free (GstMPDNode, mpd_node);
  }
}

  static void
  parse_root_node (struct MPDNode ** pointer, xmlNode * a_node)
  {
    xmlNode *cur_node;
    MPDNode *new_mpd;

    gst_mpdparser_free_mpd_node (*pointer);
  *pointer = new_mpd = g_slice_new0 (GstMPDNode);
  if (new_mpd == NULL) {
    GST_WARNING ("Allocation of MPD node failed!");
    return;
  }

  GST_LOG ("namespaces of root MPD node:");
  new_mpd->default_namespace =
      gst_mpdparser_get_xml_node_namespace (a_node, NULL);
  new_mpd->namespace_xsi = gst_mpdparser_get_xml_node_namespace (a_node, "xsi");
  new_mpd->namespace_ext = gst_mpdparser_get_xml_node_namespace (a_node, "ext");

  GST_LOG ("attributes of root MPD node:");
  gst_mpdparser_get_xml_prop_string (a_node, "schemaLocation",
      &new_mpd->schemaLocation);
  gst_mpdparser_get_xml_prop_string (a_node, "id", &new_mpd->id);
  gst_mpdparser_get_xml_prop_string (a_node, "profiles", &new_mpd->profiles);
  gst_mpdparser_get_xml_prop_type (a_node, "type", &new_mpd->type);
  gst_mpdparser_get_xml_prop_dateTime (a_node, "availabilityStartTime",
      &new_mpd->availabilityStartTime);
  gst_mpdparser_get_xml_prop_dateTime (a_node, "availabilityEndTime",
      &new_mpd->availabilityEndTime);
  gst_mpdparser_get_xml_prop_duration (a_node, "mediaPresentationDuration", -1,
      &new_mpd->mediaPresentationDuration);
  gst_mpdparser_get_xml_prop_duration (a_node, "minimumUpdatePeriod", -1,
      &new_mpd->minimumUpdatePeriod);
  gst_mpdparser_get_xml_prop_duration (a_node, "minBufferTime", -1,
      &new_mpd->minBufferTime);
  gst_mpdparser_get_xml_prop_duration (a_node, "timeShiftBufferDepth", -1,
      &new_mpd->timeShiftBufferDepth);
  gst_mpdparser_get_xml_prop_duration (a_node, "suggestedPresentationDelay", -1,
      &new_mpd->suggestedPresentationDelay);
  gst_mpdparser_get_xml_prop_duration (a_node, "maxSegmentDuration", -1,
      &new_mpd->maxSegmentDuration);
  gst_mpdparser_get_xml_prop_duration (a_node, "maxSubsegmentDuration", -1,
      &new_mpd->maxSubsegmentDuration);

  /* explore children Period nodes */
  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp (cur_node->name, (xmlChar *) "Period") == 0) {
        gst_mpdparser_parse_period_node (&new_mpd->Periods, cur_node);
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

  status_t MPDParser::parse(const void *_data, size_t size) {
    int32_t lineNo = 0;

    sp<AMessage> itemMeta;

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
	    parse_root_node (root_element);
	
	  /* free the document */
	  xmlFreeDoc (doc);
	}
      
      return NO_ERROR;
    }

    return BAD_VALUE;
  }

