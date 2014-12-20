/**
 * @file avstore_test.cpp UT for Sprout authentication vector store.
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
#include <json/reader.h>
#include <set>

#include "utils.h"
#include "sas.h"
#include "memcachedstoreview.h"
#include "avstore.h"
#include "test_utils.hpp"
#include "test_interposer.hpp"

using namespace std;

class MemcachedViewTest : public ::testing::Test
{
  MemcachedViewTest()
  {
  }

  virtual ~MemcachedViewTest()
  {
  }

  void SetUp()
  {
    _num_vbuckets = 8;
    _num_replicas = 2;
    _view = new MemcachedStoreView(_num_vbuckets, _num_replicas);
  }

  bool sets_are_equal(vector<int>, vector<int>)
  {
    return false;
  }
/*
  vector<vector<int>> get_current_read_set()
  {
    vector<vector<int>> read_set;
    read_set.resize(_num_vbuckets);
    for (int ii = 0; ii < 8; ii++)
    {
      read_set[ii] = _view->read_replicas(ii);
    }
    return read_set;
  }

  vector<vector<int>> get_current_write_set()
  {
    vector<vector<int>> write_set;
    write_set.resize(_num_vbuckets);
    for (int ii = 0; ii < 8; ii++)
    {
      write_set[ii] = _view->write_replicas(ii);
    }
    return write_set;
  }
*/
  MemcachedStoreView* _view;
  int _num_vbuckets;
  int _num_replicas;
};
/*
TEST_F(MemcachedViewTest, ReadersAndWritersMatch)
{
  vector<string> servers = {"1.2.3.4", "1.2.3.5", "1.2.3.6"};
  vector<string> empty = {};

  view.update(servers, empty);

  for (int ii = 0; ii < _num_vbuckets; ii++)
  {
    ASSERT_TRUE(sets_are_equal(view.read_replicas[ii], view.write_replicas[ii]))
  }
}

TEST_F(MemcachedViewScaleUpTest, AppendingServersIsLossless)
{
  vector<string> servers = {"1.2.3.4", "1.2.3.5", "1.2.3.6"};
  vector<string> empty = {};
  vector<string> new_servers = {"1.2.3.4", "1.2.3.5", "1.2.3.6", "1.2.3.7", "1.2.3.8"};

  view.update(servers, empty);

  vector<vector<int>> initial_read_set = get_current_read_set();
  vector<vector<int>> initial_write_set = get_current_write_set();

  view.update(servers, new_servers);

  vector<vector<int>> new_read_set = get_current_read_set();
  vector<vector<int>> new_write_set = get_current_write_set();

  // For each vbucket, the read replicas should be a superset of both
  // the previous write replicas and the current write replicas 

  ASSERT_TRUE(all_vbuckets_are_superset(new_read_set, initial_write_set))
  ASSERT_TRUE(all_vbuckets_are_superset(new_read_set, scaling_up_write_set))
}

TEST_F(MemcachedViewScaleUpTest, PrependingServersIsLossless)
{
  vector<string> servers = {"1.2.3.4", "1.2.3.5", "1.2.3.6"};
  vector<string> empty = {};
  vector<string> new_servers = {"1.2.3.7", "1.2.3.8", "1.2.3.4", "1.2.3.5", "1.2.3.6"};

  view.update(servers, empty);

  vector<vector<int>> initial_read_set = get_current_read_set();
  vector<vector<int>> initial_write_set = get_current_write_set();

  view.update(servers, new_servers);

  vector<vector<int>> scaling_read_set = get_current_read_set();
  vector<vector<int>> scaling_write_set = get_current_write_set();

  // For each vbucket, the read replicas should be a superset of both
  // the previous write replicas and the current write replicas 

  ASSERT_TRUE(all_vbuckets_are_superset(new_read_set, initial_write_set));
  ASSERT_TRUE(all_vbuckets_are_superset(new_read_set, scaling_up_write_set));

  view.update(new_servers, empty);

  vector<vector<int>> new_read_set = get_current_read_set();
  vector<vector<int>> new_write_set = get_current_write_set();

  // For each vbucket, the read replicas should be a superset of both
  // the previous write replicas and the current write replicas 

  ASSERT_TRUE(all_vbuckets_are_superset(new_read_set, initial_write_set))
  ASSERT_TRUE(all_vbuckets_are_superset(new_read_set, scaling_up_write_set))
}

TEST_F(MemcachedViewScaleUpTest, IntermixingServersIsLossless)
{
  vector<string> servers = {"1.2.3.4", "1.2.3.5", "1.2.3.6"};
  vector<string> empty = {};
  vector<string> new_servers = {"1.2.3.4", "1.2.3.7", "1.2.3.5", "1.2.3.8", "1.2.3.6"};

  view.update(servers, empty);

  vector<vector<int>> initial_read_set = get_current_read_set();
  vector<vector<int>> initial_write_set = get_current_write_set();

  view.update(servers, new_servers);

  vector<vector<int>> new_read_set = get_current_read_set();
  vector<vector<int>> new_write_set = get_current_write_set();

  // For each vbucket, the read replicas should be a superset of both
  // the previous write replicas and the current write replicas 

  ASSERT_TRUE(all_vbuckets_are_superset(new_read_set, initial_write_set))
  ASSERT_TRUE(all_vbuckets_are_superset(new_read_set, scaling_up_write_set))

}

TEST_F(MemcachedViewScaleUpTest, ReplacingAllServersIsLossless)
{
}

TEST_F(MemcachedViewScaleDownTest, RemovingEndServersIsLossless)
{
}

TEST_F(MemcachedViewScaleDownTest, RemovingFirstServersIsLossless)
{
}

TEST_F(MemcachedViewScaleDownTest, RemovingMiddleServersIsLossless)
{
}

TEST_F(MemcachedViewScaleDownTest, ReplacingAllServersIsLossless)
{
}
*/

TEST_F(MemcachedViewTest, CreateStore)
{
  MemcachedStoreView view(8, 2);
  vector<string> servers = {"1.2.3.4", "1.2.3.5", "1.2.3.6"};
  vector<string> empty = {};
//  vector<string> new_servers = {"1.2.3.7", "1.2.3.8", "1.2.3.4", "1.2.3.5", "1.2.3.6"};
  vector<string> new_servers = {"8.2.3.7", "9.2.3.8"};

  view.update(servers, empty);  

  // All vbuckets should be unique
  
  printf("%s\n", view.view_to_string().c_str());

  for (int jj = 0; jj < view.servers().size(); jj++)
  {
    printf("%s, ", view.servers()[jj].c_str());
  }
  printf("\n\n\n");
  
  view.update(servers, new_servers);

  printf("%s\n", view.view_to_string().c_str());
  for (int jj = 0; jj < view.servers().size(); jj++)
  {
    printf("%s, ", view.servers()[jj].c_str());
  }
  printf("\n");

  view.update(new_servers, empty);

  printf("%s\n", view.view_to_string().c_str());
  for (int jj = 0; jj < view.servers().size(); jj++)
  {
    printf("%s, ", view.servers()[jj].c_str());
  }
  printf("\n");

  /*
  view.update(new_servers, end_servers);

  printf("%s\n", view.view_to_string().c_str());
  for (int jj = 0; jj < view.servers().size(); jj++)
  {
    printf("%s, ", view.servers()[jj].c_str());
  }
  printf("\n");

  view.update(end_servers, empty);

  printf("%s\n", view.view_to_string().c_str());
  for (int jj = 0; jj < view.servers().size(); jj++)
  {
    printf("%s, ", view.servers()[jj].c_str());
  }
  printf("\n");
  */
}


