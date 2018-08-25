[![pipeline status](https://gitlab.cern.ch/clemenci/GitCondDB/badges/master/pipeline.svg)](https://gitlab.cern.ch/clemenci/GitCondDB/commits/master) [![coverage report](https://gitlab.cern.ch/clemenci/GitCondDB/badges/master/coverage.svg)](https://gitlab.cern.ch/clemenci/GitCondDB/commits/master)

# Git CondDB

This is a stand alone, experiment independent Conditions Database library using a Git repository as
storage backend.

It's based on the code developed for the LHCb experiment.


## External software

Software projects:
- [CMake](https://cmake.org) for building
- [Python](https://python.org) for helper scripts
- [Google Test](https://github.com/google/googletest) for unit testing
- [Clang Format](https://clang.llvm.org/docs/ClangFormat.html) for C++ code formatting
- [YAPF](https://github.com/google/yapf) for Python code formatting
- [LCOV](http://ltp.sourceforge.net/coverage/lcov.php) for test coverage reports

Libraries:
- [libgit2](https://libgit2.org/) for the Git backend
- [JSON for Modern C++](https://nlohmann.github.io/json) for the JSON backend
