/**
 * @file icscfproxy_test.cpp UT for I-CSCF proxy class.
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

#include <string>
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <boost/lexical_cast.hpp>

#include "pjutils.h"
#include "constants.h"
#include "siptest.hpp"
#include "utils.h"
#include "test_utils.hpp"
#include "icscfsproutlet.h"
#include "fakehssconnection.hpp"
#include "test_interposer.hpp"
#include "sproutletproxy.h"
#include "common_sip_processing.h"
#include "counter.h"

using namespace std;
using testing::StrEq;
using testing::ElementsAre;
using testing::MatchesRegex;
using testing::HasSubstr;
using testing::Not;

class CommonProcessingTest : public SipTest
{
public:
  /// TX data for testing.  Will be cleaned up.  Each message in a
  /// forked flow has its URI stored in _uris, and its txdata stored
  /// in _tdata against that URI.

  /// Set up test case.  Caller must clear host_mapping.
  static void SetUpTestCase()
  {
    SipTest::SetUpTestCase(false);

    // Schedule timers.
    SipTest::poll();
  }

  static void TearDownTestCase()
  {
    // Shut down the transaction module first, before we destroy the
    // objects that might handle any callbacks!
    SipTest::TearDownTestCase();
  }

  CommonProcessingTest()
  {
    _log_traffic = PrintingTestLogger::DEFAULT.isPrinting();

    // Create a TCP connection to the I-CSCF listening port.
    _tp = new TransportFlow(TransportFlow::Protocol::TCP,
                                          stack_data.icscf_port,
                                          "1.2.3.4",
                                          49152);

    
    // Load monitor with one token in the bucket at startup.
    _lm = new LoadMonitor(0, 1, 0, 0);
    
    _requests_counter = new StatisticCounter("incoming_requests",
                                             stack_data.stats_aggregator);
    _overload_counter = new StatisticCounter("rejected_overload",
                                             stack_data.stats_aggregator);

    init_common_sip_processing(_lm, _requests_counter, _overload_counter);
  }

  ~CommonProcessingTest()
  {
    delete(_tp);
    delete(_lm);
    delete(_requests_counter);
    delete(_overload_counter);
    unregister_common_processing_module();
    pjsip_tsx_layer_dump(true);

    // Terminate all transactions
    list<pjsip_transaction*> tsxs = get_all_tsxs();
    for (list<pjsip_transaction*>::iterator it2 = tsxs.begin();
         it2 != tsxs.end();
         ++it2)
    {
      pjsip_tsx_terminate(*it2, PJSIP_SC_SERVICE_UNAVAILABLE);
    }

    // PJSIP transactions aren't actually destroyed until a zero ms
    // timer fires (presumably to ensure destruction doesn't hold up
    // real work), so poll for that to happen. Otherwise we leak!
    // Allow a good length of time to pass too, in case we have
    // transactions still open. 32s is the default UAS INVITE
    // transaction timeout, so we go higher than that.
    cwtest_advance_time_ms(33000L);
    poll();
    // Stop and restart the transaction layer just in case
    pjsip_tsx_layer_instance()->stop();
    pjsip_tsx_layer_instance()->start();
  }

  class Message
  {
  public:
    string _method;
    string _requri; //< overrides toscheme:to@todomain
    string _toscheme;
    string _status;
    string _from;
    string _fromdomain;
    string _to;
    string _todomain;
    string _content_type;
    string _body;
    string _extra;
    int _forwards;
    int _unique; //< unique to this dialog; inserted into Call-ID
    string _via;
    string _route;
    int _cseq;

    Message() :
      _method("INVITE"),
      _toscheme("sip"),
      _status("200 OK"),
      _from("6505551000"),
      _fromdomain("homedomain"),
      _to("6505551234"),
      _todomain("homedomain"),
      _content_type("application/sdp"),
      _forwards(68),
      _via("10.83.18.38:36530"),
      _cseq(16567)
    {
      static int unique = 1042;
      _unique = unique;
      unique += 10; // leave room for manual increments
    }

    void set_route(pjsip_msg* msg)
    {
      string route = get_headers(msg, "Record-Route");
      if (route != "")
      {
        // Convert to a Route set by replacing all instances of Record-Route: with Route:
        for (size_t n = 0; (n = route.find("Record-Route:", n)) != string::npos;)
        {
          route.replace(n, 13, "Route:");
        }
      }
      _route = route;
    }

    string get_request()
    {
      char buf[16384];

      // The remote target.
      string target = string(_toscheme).append(":").append(_to);
      if (!_todomain.empty())
      {
        target.append("@").append(_todomain);
      }

      string requri = target;
      string route = _route.empty() ? "" : _route + "\r\n";

      int n = snprintf(buf, sizeof(buf),
                       "%1$s %9$s SIP/2.0\r\n"
                       "Via: SIP/2.0/TCP %12$s;rport;branch=z9hG4bKPjmo1aimuq33BAI4rjhgQgBr4sY%11$04dSPI\r\n"
                       "From: <sip:%2$s@%3$s>;tag=10.114.61.213+1+8c8b232a+5fb751cf\r\n"
                       "To: <%10$s>\r\n"
                       "Max-Forwards: %8$d\r\n"
                       "Call-ID: 0gQAAC8WAAACBAAALxYAAAL8P3UbW8l4mT8YBkKGRKc5SOHaJ1gMRqs%11$04dohntC@10.114.61.213\r\n"
                       "CSeq: %14$d %1$s\r\n"
                       "User-Agent: Accession 2.0.0.0\r\n"
                       "Allow: PRACK, INVITE, ACK, BYE, CANCEL, UPDATE, SUBSCRIBE, NOTIFY, REFER, MESSAGE, OPTIONS\r\n"
                       "%4$s"
                       "%7$s"
                       "%13$s"
                       "Content-Length: %5$d\r\n"
                       "\r\n"
                       "%6$s",
                       /*  1 */ _method.c_str(),
                       /*  2 */ _from.c_str(),
                       /*  3 */ _fromdomain.c_str(),
                       /*  4 */ _content_type.empty() ? "" : string("Content-Type: ").append(_content_type).append("\r\n").c_str(),
                       /*  5 */ (int)_body.length(),
                       /*  6 */ _body.c_str(),
                       /*  7 */ _extra.empty() ? "" : string(_extra).append("\r\n").c_str(),
                       /*  8 */ _forwards,
                       /*  9 */ _requri.empty() ? requri.c_str() : _requri.c_str(),
                       /* 10 */ target.c_str(),
                       /* 11 */ _unique,
                       /* 12 */ _via.c_str(),
                       /* 13 */ route.c_str(),
                       /* 14 */ _cseq
        );

      EXPECT_LT(n, (int)sizeof(buf));

      string ret(buf, n);
      // cout << ret <<endl;
      return ret;
    }

    string get_response()
    {
      char buf[16384];

      int n = snprintf(buf, sizeof(buf),
                       "SIP/2.0 %9$s\r\n"
                       "Via: SIP/2.0/TCP %13$s;rport;branch=z9hG4bKPjmo1aimuq33BAI4rjhgQgBr4sY%11$04dSPI\r\n"
                       "From: <sip:%2$s@%3$s>;tag=10.114.61.213+1+8c8b232a+5fb751cf\r\n"
                       "To: <sip:%7$s%8$s>\r\n"
                       "Call-ID: 0gQAAC8WAAACBAAALxYAAAL8P3UbW8l4mT8YBkKGRKc5SOHaJ1gMRqs%11$04dohntC@10.114.61.213\r\n"
                       "CSeq: %12$d %1$s\r\n"
                       "User-Agent: Accession 2.0.0.0\r\n"
                       "Allow: PRACK, INVITE, ACK, BYE, CANCEL, UPDATE, SUBSCRIBE, NOTIFY, REFER, MESSAGE, OPTIONS\r\n"
                       "%4$s"
                       "%10$s"
                       "Content-Length: %5$d\r\n"
                       "\r\n"
                       "%6$s",
                       /*  1 */ _method.c_str(),
                       /*  2 */ _from.c_str(),
                       /*  3 */ _fromdomain.c_str(),
                       /*  4 */ _content_type.empty() ? "" : string("Content-Type: ").append(_content_type).append("\r\n").c_str(),
                       /*  5 */ (int)_body.length(),
                       /*  6 */ _body.c_str(),
                       /*  7 */ _to.c_str(),
                       /*  8 */ _todomain.empty() ? "" : string("@").append(_todomain).c_str(),
                       /*  9 */ _status.c_str(),
                       /* 10 */ _extra.empty() ? "" : string(_extra).append("\r\n").c_str(),
                       /* 11 */ _unique,
                       /* 12 */ _cseq,
                       /* 13 */ _via.c_str()
        );

      EXPECT_LT(n, (int)sizeof(buf));

      string ret(buf, n);
      // cout << ret <<endl;
      return ret;
    }
  };

protected:
  TransportFlow* _tp;
  LoadMonitor* _lm;
  StatisticCounter* _requests_counter;
  StatisticCounter* _overload_counter;
};

TEST_F(CommonProcessingTest, RequestAllowed)
{
  // Tests that, when there is a token in the load monitor's bucket, a
  // request is not rejected.
    
  // Inject a request.
  Message msg1;
  inject_msg(msg1.get_request(), _tp);

  // As only the common processing module is loaded (and not anything
  // that will actually handle the request), expect it to just disappear.
  ASSERT_EQ(0, txdata_count());
}

TEST_F(CommonProcessingTest, RequestRejectedWithOverload)
{
  // Tests that, when there is no token in the load monitor's bucket, a
  // request is rejected with 503 Service Unavailable.

  pjsip_tx_data* tdata;

  // Consume the only token in the bucket.
  _lm->admit_request();

  // Inject a request.
  Message msg1;
  inject_msg(msg1.get_request(), _tp);

  // Expect a 503 response code.
  ASSERT_EQ(1, txdata_count());
  tdata = current_txdata();
  RespMatcher r1(503);
  r1.matches(tdata->msg);

  free_txdata();
}

TEST_F(CommonProcessingTest, AckRequestAlwaysAllowed)
{
  // Tests that, even when there is no token in the load monitor's bucket, an
  // ACK request is not rejected.

  // Consume the only token in the bucket.
  _lm->admit_request();
    
  // Inject an ACK request.
  Message msg1;
  msg1._method = "ACK";
  inject_msg(msg1.get_request(), _tp);

  // As only the common processing module is loaded (and not anything
  // that will actually handle the request), expect it to just disappear.
  ASSERT_EQ(0, txdata_count());
}

TEST_F(CommonProcessingTest, BadRequestRejected)
{
  // Tests that a malformed request receives a 400 Bad Request error.
  pjsip_tx_data* tdata;

  // Inject a request with an invalid Contact.
  Message msg1;
  msg1._extra = "Contact: ;;";
  inject_msg(msg1.get_request(), _tp);

  // Expect a 400 error.
  ASSERT_EQ(1, txdata_count());
  tdata = current_txdata();
  RespMatcher r1(400);
  r1.matches(tdata->msg);

  free_txdata();
}

TEST_F(CommonProcessingTest, BadResponseDropped)
{
  // Tests that a malformed response is just dropped,
  // rather than receiving a 400 Bad Request error.

  // Inject a response with an invalid Contact.
  Message msg1;
  msg1._extra = "Contact: ;;";
  inject_msg(msg1.get_response(), _tp);

  // Expect it to just vanish.
  ASSERT_EQ(0, txdata_count());
}
