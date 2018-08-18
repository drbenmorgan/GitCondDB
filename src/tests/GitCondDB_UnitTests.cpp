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

TEST( GitDBImpl, Connection )
{
  details::GitDBImpl db{"test_data/repo.git"};
  EXPECT_TRUE( db.connected() );
  db.disconnect();
  EXPECT_FALSE( db.connected() );
  EXPECT_TRUE( db.exists( "HEAD" ) );
  EXPECT_TRUE( db.connected() );
}

void access_test( const details::GitDBImpl& db )
{
  EXPECT_EQ( std::get<0>( db.get( "HEAD:TheDir/TheFile.txt" ) ), "some data\n" );

  {
    auto cont = std::get<1>( db.get( "HEAD:TheDir" ) );
    EXPECT_EQ( cont.dirs, std::vector<std::string>{} );
    EXPECT_EQ( cont.files, std::vector<std::string>{"TheFile.txt"} );
    EXPECT_EQ( cont.root, "TheDir" );
  }
  {
    auto cont = std::get<1>( db.get( "HEAD:" ) );
    EXPECT_EQ( cont.dirs, std::vector<std::string>{"TheDir"} );
    EXPECT_EQ( cont.files, std::vector<std::string>{} );
    EXPECT_EQ( cont.root, "" );
  }

  {
    EXPECT_TRUE( db.exists( "HEAD:TheDir" ) );
    EXPECT_TRUE( db.exists( "HEAD:TheDir/TheFile.txt" ) );
    EXPECT_FALSE( db.exists( "HEAD:NoFile" ) );
  }

  EXPECT_EQ( std::chrono::system_clock::to_time_t( db.commit_time( "HEAD" ) ), 1483225200 );
}

TEST( GitDBImpl, Access ) { access_test( details::GitDBImpl{"test_data/repo"} ); }

TEST( GitDBImpl, FailAccess )
{
  try {
    access_test( details::GitDBImpl{"test_data/no-repo"} );
    FAIL() << "exception expected for invalid db";
  } catch ( std::runtime_error& err ) {
    EXPECT_EQ( std::string_view{err.what()}.substr( 0, 22 ), "cannot open repository" );
  }
}

TEST( GitDBImpl, AccessBare ) { access_test( details::GitDBImpl{"test_data/repo.git"} ); }

TEST( GitCondDB, Connection )
{
  CondDB db = connect( "test_data/repo.git" );

  EXPECT_TRUE( db.connected() );
  db.disconnect();
  EXPECT_FALSE( db.connected() );
  EXPECT_EQ( std::get<0>( db.get( {"HEAD", "TheDir/TheFile.txt", 0} ) ), "some data\n" );
  EXPECT_TRUE( db.connected() );

  db.disconnect();
  EXPECT_FALSE( db.connected() );
  {
    auto _ = db.scoped_connection();
    EXPECT_FALSE( db.connected() );
    EXPECT_EQ( std::get<0>( db.get( {"HEAD", "TheDir/TheFile.txt", 0} ) ), "some data\n" );
    EXPECT_TRUE( db.connected() );
  }
  EXPECT_FALSE( db.connected() );
}

TEST( GitCondDB, Access )
{
  CondDB db = connect( "test_data/repo.git" );
  EXPECT_EQ( std::chrono::system_clock::to_time_t( db.commit_time( "HEAD" ) ), 1483225200 );
}

int main( int argc, char** argv )
{
  ::testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}

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
