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

TEST( JSONImpl, Connection )
{
  {
    details::JSONImpl db{"test_data/json/minimal.json"};
  }
  details::JSONImpl db{"{}"};
  EXPECT_TRUE( db.connected() );
  db.disconnect();
  EXPECT_TRUE( db.connected() );
  EXPECT_TRUE( db.exists( "HEAD" ) );
  EXPECT_TRUE( db.connected() );
}

TEST( JSONImpl, FailAccess )
{
  try {
    details::JSONImpl{"test_data/json/no-file"};
    FAIL() << "exception expected for invalid db";
  } catch ( std::runtime_error& err ) {
    EXPECT_EQ( std::string_view{err.what()}, "invalid JSON" );
  }
}

TEST( JSONImpl, AccessMemory )
{
  details::JSONImpl db{
      R"({
      "TheDir": {
        "TheFile.txt": "JSON data\n"
      },
      "Cond": {
        "IOVs": "0 a\n100 b\n",
        "a": "data a",
        "b": "data b"
      },
      "BadType": 123
    })"};

  EXPECT_EQ( std::get<0>( db.get( "HEAD:TheDir/TheFile.txt" ) ), "JSON data\n" );
  EXPECT_EQ( std::get<0>( db.get( "foobar:TheDir/TheFile.txt" ) ), "JSON data\n" );

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
    EXPECT_EQ( cont.files, std::vector<std::string>{{"BadType"}} );
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
    EXPECT_EQ( std::string_view{err.what()}, "cannot resolve object HEAD:Nothing" );
  }

  try {
    db.get( "HEAD:BadType" );
    FAIL() << "exception expected for invalid path";
  } catch ( std::runtime_error& err ) {
    EXPECT_EQ( std::string_view{err.what()}, "invalid type at HEAD:BadType" );
  }

  EXPECT_EQ( db.commit_time( "HEAD" ), std::chrono::time_point<std::chrono::system_clock>::max() );
}

int main( int argc, char** argv )
{
  ::testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
