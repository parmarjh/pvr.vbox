/*
*      Copyright (C) 2015 Sam Stenvall
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
*  MA 02110-1301  USA
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#include "Content.h"
#include <tinyxml2.h>
#include "../Channel.h"
#include "../util/Url.h"
#include "../xmltv/Utilities.h"

using namespace tinyxml2;
using namespace vbox;
using namespace vbox::response;
using namespace vbox::util;

std::string Content::GetString(const std::string &parameter) const
{
  XMLElement *element = GetParameterElement(parameter);
  
  if (element && element->GetText())
    return std::string(element->GetText());

  return "";
}

int Content::GetInteger(const std::string &parameter) const
{
  int value;

  XMLElement *element = GetParameterElement(parameter);
  if (element)
    element->QueryIntText(&value);

  return value;
}

tinyxml2::XMLElement* Content::GetParameterElement(const std::string &parameter) const
{
  return m_content->FirstChildElement(parameter.c_str());
}

std::vector<ChannelPtr> XMLTVResponseContent::GetChannels() const
{
  std::vector<ChannelPtr> channels;

  // Remember the index each channel had, it's needed for certain API requests
  unsigned int index = 1;

  for (XMLElement *element = m_content->FirstChildElement("channel");
    element != NULL; element = element->NextSiblingElement("channel"))
  {
    ChannelPtr channel = CreateChannel(element);
    channel->m_index = index++;
    channels.push_back(std::move(channel));
  }

  return channels;
}

xmltv::Guide XMLTVResponseContent::GetGuide() const
{
  xmltv::Guide guide;

  for (XMLElement *element = m_content->FirstChildElement("programme");
    element != NULL; element = element->NextSiblingElement("programme"))
  {
    // Extract the channel name and the programme
    std::string channelName = Url::Decode(element->Attribute("channel"));
    xmltv::ProgrammePtr programme = CreateProgramme(element);

    // Create an empty schedule if this is the first time we've seen this channel
    if (guide.find(channelName) == guide.cend())
      guide[channelName] = xmltv::SchedulePtr(new xmltv::Schedule);

    guide[channelName]->push_back(std::move(programme));
  }

  return guide;
}

xmltv::SchedulePtr XMLTVResponseContent::GetSchedule(const ChannelPtr &channel) const
{
  xmltv::SchedulePtr schedule(new std::vector<xmltv::ProgrammePtr>);

  for (XMLElement *element = m_content->FirstChildElement("programme");
    element != NULL; element = element->NextSiblingElement("programme"))
  {
    // Skip programmes that are not for this channel
    std::string channelName = Url::Decode(element->Attribute("channel"));
    if (channelName != channel->m_xmltvName)
      continue;

    xmltv::ProgrammePtr programme = CreateProgramme(element);
    schedule->push_back(std::move(programme));
  }

  return schedule;
}

ChannelPtr XMLTVResponseContent::CreateChannel(const tinyxml2::XMLElement *xml) const
{
  // Extract data from the various <display-name> elements
  const XMLElement *displayElement = xml->FirstChildElement("display-name");

  std::string name = displayElement->GetText();
  displayElement = displayElement->NextSiblingElement("display-name");
  std::string type = displayElement->GetText();
  displayElement = displayElement->NextSiblingElement("display-name");
  std::string uniqueId = displayElement->GetText();
  displayElement = displayElement->NextSiblingElement("display-name");
  std::string encryption = displayElement->GetText();
  std::string xmltvName = Url::Decode(xml->Attribute("id"));

  // Create the channel with some basic information
  ChannelPtr channel(new Channel(uniqueId, xmltvName, name,
    xml->FirstChildElement("url")->Attribute("src")));

  // Extract the LCN (optional)
  displayElement = displayElement->NextSiblingElement("display-name");

  if (displayElement)
  {
    unsigned int lcn = std::stoul(displayElement->GetText());
    channel->m_number = lcn;
  }

  // Set icon URL if it exists
  const char *iconUrl = xml->FirstChildElement("icon")->Attribute("src");
  if (iconUrl != NULL)
    channel->m_iconUrl = iconUrl;

  // Set radio and encryption status
  channel->m_radio = type == "Radio";
  channel->m_encrypted = encryption == "Encrypted";

  return channel;
}

xmltv::ProgrammePtr XMLTVResponseContent::CreateProgramme(const tinyxml2::XMLElement *xml) const
{
  // Construct a basic event
  std::string startTime = xml->Attribute("start");
  std::string endTime = xml->Attribute("stop");
  std::string channel = Url::Decode(xml->Attribute("channel"));
  xmltv::ProgrammePtr programme(new xmltv::Programme(startTime, endTime, channel));

  // Add title and description, if present
  const XMLElement *title = xml->FirstChildElement("title");
  if (title)
    programme->m_title = title->GetText();

  const XMLElement *description = xml->FirstChildElement("desc");
  if (description)
    programme->m_description = description->GetText();

  return programme;
}

std::vector<RecordingPtr> RecordingResponseContent::GetRecordings() const
{
  std::vector<RecordingPtr> recordings;

  for (XMLElement *element = m_content->FirstChildElement("record");
    element != NULL; element = element->NextSiblingElement("record"))
  {
    RecordingPtr recording = CreateRecording(element);
    recordings.push_back(std::move(recording));
  }

  return recordings;
}

RecordingPtr RecordingResponseContent::CreateRecording(const tinyxml2::XMLElement *xml) const
{
  // Extract mandatory properties
  std::string channelId = Url::Decode(xml->Attribute("channel"));
  std::string channelName = xml->FirstChildElement("channel-name")->GetText();
  unsigned int id;
  xml->FirstChildElement("record-id")->QueryUnsignedText(&id);
  RecordingState state = GetState(xml->FirstChildElement("state")->GetText());

  // Construct the object
  RecordingPtr recording(new Recording(id, channelId, channelName, state));

  // Add additional properties
  recording->m_start = xmltv::Utilities::XmltvToUnixTime(xml->Attribute("start"));

  // TODO: External recordings don't have an end time, default to one hour
  if (xml->Attribute("stop") != NULL)
    recording->m_end = xmltv::Utilities::XmltvToUnixTime(xml->Attribute("stop"));
  else
    recording->m_end = recording->m_start + 86400;

  if (xml->FirstChildElement("programme-title"))
    recording->m_title = xml->FirstChildElement("programme-title")->GetText();
  else
  {
    // TODO: Some recordings don't have a name, especially external ones
    if (state == RecordingState::EXTERNAL)
      recording->m_title = "External recording (channel " + channelName + ")";
    else
      recording->m_title = "Unnamed recording (channel " + channelName + ")";
  }

  if (xml->FirstChildElement("programme-desc"))
    recording->m_description = xml->FirstChildElement("programme-desc")->GetText();

  if (xml->FirstChildElement("url"))
    recording->m_url = xml->FirstChildElement("url")->GetText();

  return recording;
}

RecordingState RecordingResponseContent::GetState(const std::string &state) const
{
  if (state == "recorded")
    return RecordingState::RECORDED;
  else if (state == "recording")
    return RecordingState::RECORDING;
  else if (state == "scheduled")
    return RecordingState::SCHEDULED;
  else
    return RecordingState::EXTERNAL;
}
