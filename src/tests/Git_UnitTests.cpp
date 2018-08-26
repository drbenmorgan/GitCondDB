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

TEST( GitImpl, Connection )
{
  details::GitImpl db{"test_data/repo.git"};
  EXPECT_TRUE( db.connected() );
  db.disconnect();
  EXPECT_FALSE( db.connected() );
  EXPECT_TRUE( db.exists( "HEAD" ) );
  EXPECT_TRUE( db.connected() );
}

void access_test( const details::GitImpl& db )
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

    std::vector<std::string> expected{"Cond", "TheDir"};
    sort( begin( cont.dirs ), end( cont.dirs ) );
    EXPECT_EQ( cont.dirs, expected );
    EXPECT_EQ( cont.files, std::vector<std::string>{} );
    EXPECT_EQ( cont.root, "" );
  }

  {
    EXPECT_TRUE( db.exists( "HEAD:TheDir" ) );
    EXPECT_TRUE( db.exists( "HEAD:TheDir/TheFile.txt" ) );
    EXPECT_FALSE( db.exists( "HEAD:NoFile" ) );
  }

  try {
    db.get( "HEAD:Nothing" );
    FAIL() << "exception expected for invalid path";
  } catch ( std::runtime_error& err ) {
    EXPECT_EQ( std::string_view{err.what()}.substr( 0, 34 ), "cannot resolve object HEAD:Nothing" );
  }

  EXPECT_EQ( std::chrono::system_clock::to_time_t( db.commit_time( "HEAD" ) ), 1483225200 );
}

TEST( GitImpl, Access ) { access_test( details::GitImpl{"test_data/repo"} ); }

TEST( GitImpl, FailAccess )
{
  try {
    details::GitImpl{"test_data/no-repo"};
    FAIL() << "exception expected for invalid db";
  } catch ( std::runtime_error& err ) {
    EXPECT_EQ( std::string_view{err.what()}.substr( 0, 22 ), "cannot open repository" );
  }
}

TEST( GitImpl, AccessBare ) { access_test( details::GitImpl{"test_data/repo.git"} ); }

int main( int argc, char** argv )
{
  ::testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
