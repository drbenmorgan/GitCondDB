/*****************************************************************************\
* (c) Copyright 2018 CERN for the benefit of the LHCb Collaboration           *
*                                                                             *
* This software is distributed under the terms of the Apache version 2        *
* licence, copied verbatim in the file "COPYING".                             *
*                                                                             *
* In applying this licence, CERN does not waive the privileges and immunities *
* granted to it by virtue of its status as an Intergovernmental Organization  *
* or submit itself to any jurisdiction.                                       *
\*****************************************************************************/

#include "GitCondDB.h"

#include "DBImpl.h"
#include "iov_helpers.h"

#include "gtest/gtest.h"

using namespace GitCondDB::v1;

TEST( IOVHelpers, ParseIOVs )
{
  using GitCondDB::Helpers::get_key_iov;

  const std::string test_data{"0 a\n"
                              "100 b\n"
                              "200 c\n"
                              "300 d\n"};

  {
    auto[key, iov] = get_key_iov( test_data, 0 );
    EXPECT_TRUE( iov.valid() );
    EXPECT_EQ( key, "a" );
    EXPECT_EQ( iov.since, 0 );
    EXPECT_EQ( iov.until, 100 );
  }
  {
    auto[key, iov] = get_key_iov( test_data, 100 );
    EXPECT_TRUE( iov.valid() );
    EXPECT_EQ( key, "b" );
    EXPECT_EQ( iov.since, 100 );
    EXPECT_EQ( iov.until, 200 );
  }
  {
    auto[key, iov] = get_key_iov( test_data, 230 );
    EXPECT_TRUE( iov.valid() );
    EXPECT_EQ( key, "c" );
    EXPECT_EQ( iov.since, 200 );
    EXPECT_EQ( iov.until, 300 );
  }
  {
    auto[key, iov] = get_key_iov( test_data, 500 );
    EXPECT_TRUE( iov.valid() );
    EXPECT_EQ( key, "d" );
    EXPECT_EQ( iov.since, 300 );
    EXPECT_EQ( iov.until, GitCondDB::CondDB::IOV::max() );
  }

  {
    auto[key, iov] = get_key_iov( test_data, 240, {210, 1000} );
    EXPECT_TRUE( iov.valid() );
    EXPECT_EQ( key, "c" );
    EXPECT_EQ( iov.since, 210 );
    EXPECT_EQ( iov.until, 300 );
  }
  {
    auto[key, iov] = get_key_iov( test_data, 500, {210, 1000} );
    EXPECT_TRUE( iov.valid() );
    EXPECT_EQ( key, "d" );
    EXPECT_EQ( iov.since, 300 );
    EXPECT_EQ( iov.until, 1000 );
  }

  {
    auto[key, iov] = get_key_iov( test_data, 2000, {210, 1000} );
    EXPECT_FALSE( iov.valid() );
    EXPECT_EQ( key, "" );
  }
}

int main( int argc, char** argv )
{
  ::testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
