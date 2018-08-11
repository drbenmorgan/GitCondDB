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

#include <GitCondDB.h>

#include "DBImpl.h"

#include <cassert>

using namespace GitCondDB::v1;

CondDB::CondDB( std::unique_ptr<details::DBImpl> impl ) : m_impl{std::move( impl )} { assert( m_impl ); }
CondDB::~CondDB() {}

void CondDB::disconnect() const { m_impl->disconnect(); }

std::tuple<std::string, CondDB::IOV> CondDB::get( const Key& key ) const
{
  std::string object_id = key.tag + ":" + key.path;
  auto        data      = m_impl->get( object_id.c_str() );
  return {( data.index() == 0 ) ? std::get<0>( data ) : "<directory>", {}};
}

std::chrono::system_clock::time_point CondDB::commit_time( const std::string& commit_id ) const
{
  return m_impl->commit_time( commit_id.c_str() );
}

CondDB GitCondDB::v1::connect( std::string_view repository )
{
  return {std::make_unique<details::GitDBImpl>( repository )};
}
