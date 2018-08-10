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

#include "gtest/gtest.h"

using namespace GitCondDB::v1;

TEST( GitConDBTests, Construction )
{
  CondDB db = connect( "test_data/repo-bare.git" );
  EXPECT_EQ( std::get<0>( db.get( {"HEAD", "valid_runs.txt", 0} ) ),
             "10\n20\n30\n95\n96\n97\n98\n99\n100\n101\n103\n200\n" );
}

int main( int argc, char** argv )
{
  ::testing::InitGoogleTest( &argc, argv );
  return RUN_ALL_TESTS();
}
