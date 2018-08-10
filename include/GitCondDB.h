#ifndef GITCONDDB_H
#define GITCONDDB_H
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

#include <gitconddb_export.h>

#include <chrono>
#include <limits>
#include <memory>
#include <string_view>

namespace GitCondDB
{
  inline namespace v1
  {
    namespace details
    {
      struct DBImpl;
    }

    struct CondDB;

    GITCONDDB_EXPORT CondDB connect( std::string_view repository );

    struct GITCONDDB_EXPORT CondDB {
      using time_point_t = std::uint64_t;

      struct Key {
        std::string  tag;
        std::string  path;
        time_point_t time_point;
      };

      struct IOVInfo {
        time_point_t since = std::numeric_limits<time_point_t>::min();
        time_point_t until = std::numeric_limits<time_point_t>::max();
      };

      /// RAII object to limit the time the connection to the repository stay open.
      /// The lifetime of the CondDB object must be longer than the AccessGuard.
      class AccessGuard
      {
        AccessGuard( CondDB& db ) : db( db ) {}

        CondDB& db;

      public:
        ~AccessGuard() { db.disconnect(); }
      };

      void disconnect() const;

      std::tuple<std::string, IOVInfo> get( const Key& key ) const;

      std::chrono::system_clock::time_point commit_time( const std::string& commit_id ) const;

      CondDB( CondDB&& ) = default;
      ~CondDB();

    private:
      CondDB( std::unique_ptr<details::DBImpl> impl );

      std::unique_ptr<details::DBImpl> m_impl;

      friend GITCONDDB_EXPORT CondDB connect( std::string_view repository );
    };
  }
}

#endif // GITCONDDB_H
