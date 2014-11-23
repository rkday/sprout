/**
 * @file fakehssconnection.cpp Fake HSS Connection (for testing).
 *
 * Project Clearwater - IMS in the Cloud
 * Copyright (C) 2013  Metaswitch Networks Ltd
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version, along with the "Special Exception" for use of
 * the program along with SSL, set forth below. This program is distributed
 * in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details. You should have received a copy of the GNU General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * The author can be reached by email at clearwater@metaswitch.com or by
 * post at Metaswitch Networks Ltd, 100 Church St, Enfield EN2 6BQ, UK
 *
 * Special Exception
 * Metaswitch Networks Ltd  grants you permission to copy, modify,
 * propagate, and distribute a work formed by combining OpenSSL with The
 * Software, or a work derivative of such a combination, even if such
 * copying, modification, propagation, or distribution would otherwise
 * violate the terms of the GPL. You must comply with the GPL in all
 * respects for all of the code used other than OpenSSL.
 * "OpenSSL" means OpenSSL toolkit software distributed by the OpenSSL
 * Project and licensed under the OpenSSL Licenses, or a work based on such
 * software and licensed under the OpenSSL Licenses.
 * "OpenSSL Licenses" means the OpenSSL License and Original SSLeay License
 * under which the OpenSSL Project distributes the OpenSSL toolkit software,
 * as those licenses appear in the file LICENSE-OPENSSL.
 */

#include <cstdio>
#include "fakehssconnection.hpp"
#include <json/reader.h>
#include "gtest/gtest.h"

FakeHSSConnection::FakeHSSConnection() : HSSConnection("localhost", NULL, NULL, NULL, NULL)
{
}


FakeHSSConnection::~FakeHSSConnection()
{
  flush_all();
}


void FakeHSSConnection::flush_all()
{
  _results.clear();
  _calls.clear();
}

void FakeHSSConnection::set_result(const std::string& url,
                                   const std::string& result)
{
  _results[UrlBody(url, "")] = result;
}

void FakeHSSConnection::set_impu_result(const std::string& impu,
                                        const std::string& expected_body,
                                        const std::string& state,
                                        std::string subxml,
                                        std::string extra_params)
{
  std::string url = "/impu/" + Utils::url_escape(impu) + "/reg-data" + extra_params;

  if (subxml.empty())
  {
    subxml = ("<IMSSubscription><ServiceProfile>\n"
              "<PublicIdentity><Identity>"+impu+"</Identity></PublicIdentity>"
              "  <InitialFilterCriteria>\n"
              "  </InitialFilterCriteria>\n"
              "</ServiceProfile></IMSSubscription>");
  }

  std::string chargingaddrsxml = ("<ChargingAddresses>\n"
                                  "  <CCF priority=\"1\">ccf1</CCF>\n"
                                  "  <ECF priority=\"1\">ecf1</ECF>\n"
                                  "  <ECF priority=\"2\">ecf2</ECF>\n"
                                  "</ChargingAddresses>");

  std::string result = ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                        "<ClearwaterRegData><RegistrationState>" + state + "</RegistrationState>"
                        + subxml + chargingaddrsxml + "</ClearwaterRegData>");

  _results[UrlBody(url, expected_body)] = result;
}


void FakeHSSConnection::delete_result(const std::string& url)
{
  _results.erase(UrlBody(url, ""));
}

long FakeHSSConnection::put_for_xml_object(const std::string& path, std::string body, rapidxml::xml_document<>*& root, SAS::TrailId trail)
{
  return FakeHSSConnection::get_xml_object(path,
                                           body,
                                           root,
                                           trail);
}


void FakeHSSConnection::set_rc(const std::string& url,
                               long rc)
{
  _rcs[url] = rc;
}


void FakeHSSConnection::delete_rc(const std::string& url)
{
  _rcs.erase(url);
}


long FakeHSSConnection::get_json_object(const std::string& path,
                                        Json::Value*& object,
                                        SAS::TrailId trail)
{
  _calls.insert(UrlBody(path, ""));
  HTTPCode http_code = HTTP_NOT_FOUND;

  std::map<UrlBody, std::string>::const_iterator i = _results.find(UrlBody(path, ""));

  if (i != _results.end())
  {
    object = new Json::Value;
    Json::Reader reader;
    LOG_DEBUG("Found HSS data for %s\n%s", path.c_str(), i->second.c_str());
    bool parsingSuccessful = reader.parse(i->second, *object);
    if (parsingSuccessful)
    {
      http_code = HTTP_OK;
    }
    else
    {
      // report to the user the failure and their locations in the document.
      LOG_ERROR("Failed to parse Homestead response:\n %s\n %s\n %s\n",
                path.c_str(),
                i->second.c_str(),
                reader.getFormatedErrorMessages().c_str());
      delete object;
      object = NULL;
    }
  }
  else
  {
    LOG_DEBUG("Failed to find JSON result for URL %s", path.c_str());
  }

  std::map<std::string, long>::const_iterator i2 = _rcs.find(path);
  if (i2 != _rcs.end())
  {
    http_code = i2->second;
  }

  return http_code;
}

long FakeHSSConnection::get_xml_object(const std::string& path,
                                       rapidxml::xml_document<>*& root,
                                       SAS::TrailId trail)
{
  return get_xml_object(path, "", root, trail);
}

long FakeHSSConnection::get_xml_object(const std::string& path,
                                       std::string body,
                                       rapidxml::xml_document<>*& root,
                                       SAS::TrailId trail)
{
  _calls.insert(UrlBody(path, body));
  HTTPCode http_code = HTTP_NOT_FOUND;

  std::map<UrlBody, std::string>::const_iterator i = _results.find(UrlBody(path, body));

  if (i != _results.end())
  {
    root = new rapidxml::xml_document<>;
    try
    {
      root->parse<0>(root->allocate_string(i->second.c_str()));
      http_code = HTTP_OK;
    }
    catch (rapidxml::parse_error& err)
    {
      // report to the user the failure and their locations in the document.
      printf("Failed to parse Homestead response:\n %s\n %s\n %s\n",
             path.c_str(),
             i->second.c_str(),
             err.what());
      LOG_ERROR("Failed to parse Homestead response:\n %s\n %s\n %s\n",
                path.c_str(),
                i->second.c_str(),
                err.what());
      delete root;
      root = NULL;
    }
  }
  else
  {
    LOG_ERROR("Failed to find XML result for URL %s", path.c_str());
  }

  std::map<std::string, long>::const_iterator i2 = _rcs.find(path);
  if (i2 != _rcs.end())
  {
    http_code = i2->second;
  }

  return http_code;
}

bool FakeHSSConnection::url_was_requested(const std::string& url, const std::string& body)
{
  return (_calls.find(UrlBody(url, body)) != _calls.end());
}
