#ifndef IOV_HELPERS_H
#define IOV_HELPERS_H
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

#include "helpers.h"

#include <sstream>
#include <string>
#include <tuple>

namespace GitCondDB
{
  namespace Helpers
  {
    std::tuple<std::string, CondDB::IOV> get_key_iov( const std::string& data, const CondDB::time_point_t t,
                                                      const CondDB::IOV& boundary = {} )
    {
      std::tuple<std::string, CondDB::IOV> out;
      auto& key   = std::get<0>( out );
      auto& since = std::get<1>( out ).since;
      auto& until = std::get<1>( out ).until;

      if ( UNLIKELY( t < boundary.since || t >= boundary.until ) ) {
        since = until = 0;
      } else {
        CondDB::time_point_t current = 0;
        std::string          line;

        std::istringstream stream{data};

        while ( std::getline( stream, line ) ) {
          std::istringstream is{line};
          is >> current;
          if ( current > t ) {
            until = current; // what we read is the "until" for the previous key
            break;           // and we need to use the previous key
          }
          is >> key;
          since = current; // the time we read is the "since" for the read key
        }
        std::get<1>( out ).cut( boundary );
      }
      return out;
    }
  }
}

#endif // IOV_HELPERS_H
