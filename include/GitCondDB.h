#ifndef GITCONDDB_H
#define GITCONDDB_H

#include <string_view>

namespace GitCondDB
{
  inline namespace v1
  {
    class CondDB
    {
    };

    CondDB connect( std::string_view repository );
  }
}

#endif // GITCONDDB_H
