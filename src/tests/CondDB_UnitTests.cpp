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

TEST( CondDB, Connection )
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

TEST( CondDB, Access )
{
  {
    CondDB db = connect( "test_data/repo.git" );
    EXPECT_EQ( std::get<0>( db.get( {"HEAD", "TheDir/TheFile.txt", 0} ) ), "some data\n" );
    EXPECT_EQ( std::chrono::system_clock::to_time_t( db.commit_time( "HEAD" ) ), 1483225200 );
  }
  {
    CondDB db = connect( "git:test_data/repo.git" );
    EXPECT_EQ( std::get<0>( db.get( {"HEAD", "TheDir/TheFile.txt", 0} ) ), "some data\n" );
    EXPECT_EQ( std::chrono::system_clock::to_time_t( db.commit_time( "HEAD" ) ), 1483225200 );
  }
  {
    CondDB db = connect( "file:test_data/repo" );
    EXPECT_EQ( std::get<0>( db.get( {"HEAD", "TheDir/TheFile.txt", 0} ) ), "some uncommitted data\n" );
    EXPECT_EQ( db.commit_time( "HEAD" ), std::chrono::time_point<std::chrono::system_clock>::max() );
  }
  {
    CondDB db = connect( R"(json:
                         {"TheDir": {"TheFile.txt": "some JSON (memory) data\n"}}
                         )" );
    EXPECT_EQ( std::get<0>( db.get( {"HEAD", "TheDir/TheFile.txt", 0} ) ), "some JSON (memory) data\n" );
    EXPECT_EQ( db.commit_time( "HEAD" ), std::chrono::time_point<std::chrono::system_clock>::max() );
  }
  {
    CondDB db = connect( "json:test_data/json/basic.json" );
    EXPECT_EQ( std::get<0>( db.get( {"HEAD", "TheDir/TheFile.txt", 0} ) ), "some JSON (file) data\n" );
    EXPECT_EQ( db.commit_time( "HEAD" ), std::chrono::time_point<std::chrono::system_clock>::max() );
  }
}

TEST( CondDB, Directory )
{
  CondDB db = connect( "test_data/lhcb/repo" );
  auto[data, iov] = db.get( {"HEAD", "Direct", 0} );
  EXPECT_EQ( iov.since, GitCondDB::CondDB::IOV::min() );
  EXPECT_EQ( iov.until, GitCondDB::CondDB::IOV::max() );
  EXPECT_EQ( data, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>"
                   "<!DOCTYPE DDDB SYSTEM \"git:/DTD/structure.dtd\">"
                   "<DDDB><catalog name=\"Direct\">"
                   "<catalogref href=\"Direct/Nested\"/>"
                   "<conditionref href=\"Direct/Cond1\"/>"
                   "<conditionref href=\"Direct/Cond2\"/>"
                   "</catalog></DDDB>" );
}

TEST( CondDB, IOVAccess )
{
  CondDB db = connect( "test_data/repo" );

  {
    auto[data, iov] = db.get( {"v0", "Cond", 0} );
    EXPECT_EQ( iov.since, 0 );
    EXPECT_EQ( iov.until, 100 );
    EXPECT_EQ( data, "data 0" );
  }
  {
    auto[data, iov] = db.get( {"v0", "Cond", 110} );
    EXPECT_EQ( iov.since, 100 );
    EXPECT_EQ( iov.until, GitCondDB::CondDB::IOV::max() );
    EXPECT_EQ( data, "data 1" );
  }
  {
    auto[data, iov] = db.get( {"v1", "Cond", 0} );
    EXPECT_EQ( iov.since, 0 );
    EXPECT_EQ( iov.until, 100 );
    EXPECT_EQ( data, "data 0" );
  }
  {
    auto[data, iov] = db.get( {"v1", "Cond", 110} );
    EXPECT_EQ( iov.since, 100 );
    EXPECT_EQ( iov.until, 150 );
    EXPECT_EQ( data, "data 1" );
  }
  {
    auto[data, iov] = db.get( {"v1", "Cond", 150} );
    EXPECT_EQ( iov.since, 150 );
    EXPECT_EQ( iov.until, 200 );
    EXPECT_EQ( data, "data 2" );
  }
  {
    auto[data, iov] = db.get( {"v1", "Cond", 210} );
    EXPECT_EQ( iov.since, 200 );
    EXPECT_EQ( iov.until, GitCondDB::CondDB::IOV::max() );
    EXPECT_EQ( data, "data 3" );
  }

  // for attempt of invalid retrieval
  {
    auto[data, iov] = db.get( {"v1", "Cond", 210}, {0, 200} );
    EXPECT_FALSE( iov.valid() );
    EXPECT_EQ( data, "" );
  }
}

TEST( CondDB, Directory_FS )
{
  CondDB db = connect( "file:test_data/lhcb/repo" );
  auto[data, iov] = db.get( {"dummy", "Direct", 0} );
  EXPECT_EQ( iov.since, GitCondDB::CondDB::IOV::min() );
  EXPECT_EQ( iov.until, GitCondDB::CondDB::IOV::max() );
  EXPECT_EQ( data, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>"
                   "<!DOCTYPE DDDB SYSTEM \"git:/DTD/structure.dtd\">"
                   "<DDDB><catalog name=\"Direct\">"
                   "<catalogref href=\"Direct/Nested\"/>"
                   "<conditionref href=\"Direct/Cond1\"/>"
                   "<conditionref href=\"Direct/Cond2\"/>"
                   "</catalog></DDDB>" );
}

TEST( CondDB, IOVAccess_FS )
{
  CondDB db = connect( "file:test_data/repo" );

  {
    auto[data, iov] = db.get( {"v1", "Cond", 0} );
    EXPECT_EQ( iov.since, 0 );
    EXPECT_EQ( iov.until, 100 );
    EXPECT_EQ( data, "data 0" );
  }
  {
    auto[data, iov] = db.get( {"v1", "Cond", 110} );
    EXPECT_EQ( iov.since, 100 );
    EXPECT_EQ( iov.until, 150 );
    EXPECT_EQ( data, "data 1" );
  }
  {
    auto[data, iov] = db.get( {"v1", "Cond", 150} );
    EXPECT_EQ( iov.since, 150 );
    EXPECT_EQ( iov.until, 200 );
    EXPECT_EQ( data, "data 2" );
  }
  {
    auto[data, iov] = db.get( {"v1", "Cond", 210} );
    EXPECT_EQ( iov.since, 200 );
    EXPECT_EQ( iov.until, GitCondDB::CondDB::IOV::max() );
    EXPECT_EQ( data, "data 3" );
  }

  // for attempt of invalid retrieval
  {
    auto[data, iov] = db.get( {"v1", "Cond", 210}, {0, 200} );
    EXPECT_FALSE( iov.valid() );
    EXPECT_EQ( data, "" );
  }
}

TEST( CondDB, IOVAccess_JSON )
{
  CondDB db = connect( R"(json:
                       {"Cond": {"IOVs": "0 v0\n100 group\n200 v2\n",
                                 "v0": "data 0",
                                 "v1": "data 1",
                                 "v2": "data 2",
                                 "group": {"IOVs": "50 ../v1"}}}
                       )" );

  {
    auto[data, iov] = db.get( {"v1", "Cond", 0} );
    EXPECT_EQ( iov.since, 0 );
    EXPECT_EQ( iov.until, 100 );
    EXPECT_EQ( data, "data 0" );
  }
  {
    auto[data, iov] = db.get( {"v1", "Cond", 110} );
    EXPECT_EQ( iov.since, 100 );
    EXPECT_EQ( iov.until, 200 );
    EXPECT_EQ( data, "data 1" );
  }
  {
    auto[data, iov] = db.get( {"v1", "Cond", 210} );
    EXPECT_EQ( iov.since, 200 );
    EXPECT_EQ( iov.until, GitCondDB::CondDB::IOV::max() );
    EXPECT_EQ( data, "data 2" );
  }

  // for attempt of invalid retrieval
  {
    auto[data, iov] = db.get( {"v1", "Cond", 210}, {0, 200} );
    EXPECT_FALSE( iov.valid() );
    EXPECT_EQ( data, "" );
  }
}

int main( int argc, char** argv )
{
  ::testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
