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

#include <string_view>

namespace GitCondDB
{
  inline namespace v1
  {
    class GITCONDDB_EXPORT CondDB
    {
    };

    GITCONDDB_EXPORT CondDB connect( std::string_view repository );
  }
}

#endif // GITCONDDB_H
