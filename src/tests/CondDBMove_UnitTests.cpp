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
#include <optional>

#include "GitCondDB.h"

#include "gtest/gtest.h"

using namespace GitCondDB::v1;

TEST( CondDB, Move ) {
  // Making sure the move constructor for the CondDB is functional
  auto temporary_db = connect( "test_data/repo.git" );
  auto db           = std::move( temporary_db );

  EXPECT_TRUE( db.connected() );
  EXPECT_EQ( std::get<0>( db.get( {"HEAD", "TheDir/TheFile.txt", 0} ) ), "some data\n" );
}

TEST( CondDB, Optional ) {
  // Check that we can use the db in a std::optional, this requires it can be moved.
  std::optional<CondDB> db = std::nullopt;
  db.emplace( connect( "test_data/repo.git" ) );

  EXPECT_TRUE( db->connected() );
  EXPECT_EQ( std::get<0>( db->get( {"HEAD", "TheDir/TheFile.txt", 0} ) ), "some data\n" );
}

int main( int argc, char** argv ) {
  ::testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
