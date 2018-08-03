#ifndef GITCONDDB_H
#define GITCONDDB_H

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
