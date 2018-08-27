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
      using time_point_t = std::uint_fast64_t;

      struct Key {
        std::string  tag;
        std::string  path;
        time_point_t time_point;
      };

      struct IOV {
        time_point_t since = min();
        time_point_t until = max();

        constexpr static time_point_t min() { return std::numeric_limits<time_point_t>::min(); }
        constexpr static time_point_t max() { return std::numeric_limits<time_point_t>::max(); }

        IOV& cut( const IOV& boundary )
        {
          since = std::max( since, boundary.since );
          until = std::min( until, boundary.until );
          return *this;
        }
        bool valid() const { return since < until; }
        bool contains( const time_point_t point ) const { return point >= since && point < until; }
        bool contains( const IOV& other ) const
        {
          return other.valid() && contains( other.since ) && other.until > since && other.until <= until;
        }
        bool overlaps( IOV other ) const { return other.cut( *this ).valid(); }
      };

      /// RAII object to limit the time the connection to the repository stay open.
      /// The lifetime of the CondDB object must be longer than the AccessGuard.
      class AccessGuard
      {
        friend struct CondDB;
        AccessGuard( const CondDB& db ) : db( db ) {}

        const CondDB& db;

      public:
        ~AccessGuard() { db.disconnect(); }
      };

      void disconnect() const;

      bool connected() const;

      AccessGuard scoped_connection() const { return AccessGuard( *this ); }

      // This siganture is required because of https://gcc.gnu.org/bugzilla/show_bug.cgi?id=58328
      std::tuple<std::string, IOV> get( const Key& key ) const { return get( key, {} ); }

      std::tuple<std::string, IOV> get( const Key& key, const IOV& bounds ) const;

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
