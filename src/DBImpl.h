#ifndef DBIMPL_H
#define DBIMPL_H
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

#include <variant>

#include "helpers.h"

namespace GitCondDB
{
  inline namespace v1
  {
    namespace details
    {
      template <class RET, class FUNC, class... ARGS>
      RET git_call( std::string_view err_msg, std::string_view key, FUNC func, ARGS&&... args )
      {
        typename RET::element_type* tmp = nullptr;
        if ( UNLIKELY( func( &tmp, std::forward<ARGS>( args )... ) ) )
          throw std::runtime_error{std::string{err_msg} + " " + std::string{key} + ": " + giterr_last()->message};
        return RET{tmp};
      }

      struct dir_content {
        std::string              root;
        std::vector<std::string> dirs;
        std::vector<std::string> files;
      };

      struct DBImpl {
        virtual ~DBImpl() = default;

        virtual void disconnect() const = 0;

        virtual std::variant<std::string, dir_content> get( const char* object_id ) const = 0;

        virtual std::chrono::system_clock::time_point commit_time( const char* commit_id ) const = 0;
      };

      class GitDBImpl : public DBImpl
      {
      public:
        GitDBImpl( std::string_view repository )
            : m_repository_url( repository ), m_repository{[this]() -> git_repository_ptr::storage_t {
              auto res = git_call<git_repository_ptr::storage_t>( "cannot open repository", m_repository_url,
                                                                  git_repository_open, m_repository_url.c_str() );
              if ( UNLIKELY( !res ) ) throw std::runtime_error{"invalid Git repository: '" + m_repository_url + "'"};
              return res;
            }}
        {
          // Initialize Git library
          git_libgit2_init();

          // try access during construction
          m_repository.get();
        }

        ~GitDBImpl() override
        {
          // Finalize Git library
          git_libgit2_shutdown();
        }

        void disconnect() const override { m_repository.reset(); }

        std::variant<std::string, dir_content> get( const char* object_id ) const override
        {
          std::variant<std::string, dir_content> out;
          auto obj = get_object( object_id );
          if ( git_object_type( obj.get() ) == GIT_OBJ_TREE ) {
            out = dir_content{};
          } else {
            auto blob = reinterpret_cast<const git_blob*>( obj.get() );
            out       = std::string{reinterpret_cast<const char*>( git_blob_rawcontent( blob ) ),
                              static_cast<std::size_t>( git_blob_rawsize( blob ) )};
          }
          return out;
        }

        std::chrono::system_clock::time_point commit_time( const char* commit_id ) const override
        {
          auto obj = get_object( commit_id, "commit" );
          return std::chrono::system_clock::from_time_t(
              git_commit_time( reinterpret_cast<git_commit*>( obj.get() ) ) );
        }

      private:
        git_object_ptr get_object( const char* commit_id, const std::string& obj_type = "object" ) const
        {
          return git_call<git_object_ptr>( "cannot resolve " + obj_type, commit_id, git_revparse_single,
                                           m_repository.get(), commit_id );
        }

        std::string m_repository_url;

        mutable git_repository_ptr m_repository;
      };
    }
  }
}

#endif // DBIMPL_H
