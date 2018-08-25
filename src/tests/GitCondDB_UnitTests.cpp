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

TEST( FSImpl, Connection )
{
  details::FilesystemImpl db{"test_data/repo"};
  EXPECT_TRUE( db.connected() );
  db.disconnect();
  EXPECT_TRUE( db.connected() );
  EXPECT_TRUE( db.exists( "HEAD" ) );
  EXPECT_TRUE( db.connected() );
}

TEST( FSImpl, Access )
{
  details::FilesystemImpl db{"test_data/repo"};

  EXPECT_EQ( std::get<0>( db.get( "HEAD:TheDir/TheFile.txt" ) ), "some uncommitted data\n" );
  EXPECT_EQ( std::get<0>( db.get( "foobar:TheDir/TheFile.txt" ) ), "some uncommitted data\n" );

  {
    auto cont = std::get<1>( db.get( "HEAD:TheDir" ) );
    EXPECT_EQ( cont.dirs, std::vector<std::string>{} );
    EXPECT_EQ( cont.files, std::vector<std::string>{"TheFile.txt"} );
    EXPECT_EQ( cont.root, "TheDir" );
  }
  {
    auto cont = std::get<1>( db.get( "HEAD:" ) );

    std::vector<std::string> expected{".git", "Cond", "TheDir"};
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
    EXPECT_EQ( std::string_view{err.what()}, "cannot resolve object HEAD:Nothing" );
  }

  EXPECT_EQ( db.commit_time( "HEAD" ), std::chrono::time_point<std::chrono::system_clock>::max() );
}

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

TEST( GitCondDB, Directory )
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

TEST( GitCondDB, IOVAccess )
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

TEST( GitCondDB, Directory_FS )
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

TEST( GitCondDB, IOVAccess_FS )
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
