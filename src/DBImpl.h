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

#if __GNUC__ >= 8
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include <fstream>
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

        virtual bool connected() const = 0;

        virtual bool exists( const char* object_id ) const = 0;

        virtual std::variant<std::string, dir_content> get( const char* object_id ) const = 0;

        virtual std::chrono::system_clock::time_point commit_time( const char* commit_id ) const = 0;

        inline static std::string_view strip_tag( std::string_view object_id )
        {
          if ( const auto pos = object_id.find_first_of( ':' ); pos != object_id.npos ) {
            object_id.remove_prefix( pos + 1 );
          }
          return object_id;
        }
      };

      class GitDBImpl : public DBImpl
      {
        using git_object_ptr     = GitCondDB::Helpers::git_object_ptr;
        using git_repository_ptr = GitCondDB::Helpers::git_repository_ptr;

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

        bool connected() const override { return m_repository.is_set(); }

        bool exists( const char* object_id ) const override
        {
          git_object* tmp = nullptr;
          git_revparse_single( &tmp, m_repository.get(), object_id );
          bool result = tmp;
          git_object_free( tmp );
          return result;
        }

        std::variant<std::string, dir_content> get( const char* object_id ) const override
        {
          std::variant<std::string, dir_content> out;
          auto obj = get_object( object_id );
          if ( git_object_type( obj.get() ) == GIT_OBJ_TREE ) {
            dir_content entries;
            entries.root = strip_tag( object_id );

            const git_tree* tree = reinterpret_cast<const git_tree*>( obj.get() );

            const std::size_t     max_i = git_tree_entrycount( tree );
            const git_tree_entry* te    = nullptr;

            for ( std::size_t i = 0; i < max_i; ++i ) {
              te = git_tree_entry_byindex( tree, i );
              ( ( git_tree_entry_type( te ) == GIT_OBJ_TREE ) ? entries.dirs : entries.files )
                  .emplace_back( git_tree_entry_name( te ) );
            }
            out = std::move( entries );
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

      class FilesystemImpl : public DBImpl
      {
      public:
        FilesystemImpl( std::string_view root ) : m_root( root )
        {
          if ( !is_directory( m_root ) ) throw std::runtime_error( "invalid path " + m_root.string() );
        }

        void disconnect() const override {}

        bool connected() const override { return true; }

        bool exists( const char* object_id ) const override
        {
          // return true for any tag name (i.e. id without a ':') and existing paths
          const std::string_view id{object_id};
          return id.find_first_of( ':' ) == id.npos || fs::exists( to_path( id ) );
        }

        std::variant<std::string, dir_content> get( const char* object_id ) const override
        {
          std::variant<std::string, dir_content> out;
          const auto path = to_path( object_id );

          if ( is_directory( path ) ) {
            dir_content entries;
            entries.root = strip_tag( object_id );

            for ( auto p : fs::directory_iterator( path ) ) {
              ( is_directory( p.path() ) ? entries.dirs : entries.files ).emplace_back( p.path().filename() );
            }

            out = std::move( entries );
          } else {
            std::ifstream stream{path.string()};
            stream.seekg( 0, stream.end );
            const auto size = stream.tellg();
            stream.seekg( 0 );

            std::string data( static_cast<std::size_t>( size ), 0 );
            stream.read( data.data(), size );

            out = std::move( data );
          }
          return out;
        }

        std::chrono::system_clock::time_point commit_time( const char* ) const override
        {
          return std::chrono::time_point<std::chrono::system_clock>::max();
        }

      private:
        inline fs::path to_path( std::string_view object_id ) const { return m_root / strip_tag( object_id ); }

        fs::path m_root;
      };
    }
  }
}

#endif // DBIMPL_H
